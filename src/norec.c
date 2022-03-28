#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <alloc.h>

#include "norec.h"
#include "utils.h"

#define FILTERHASH(a)                   ((UNS(a) >> 2) ^ (UNS(a) >> 5))
#define FILTERBITS(a)                   (1 << (FILTERHASH(a) & 0x1F))

enum {
  TX_ACTIVE = 1,
  TX_COMMITTED = 2,
  TX_ABORTED = 4
};

#include "thread_def.h"

fsb_allocator_t allocator;

volatile long *LOCK;

// --------------------------------------------------------------

static inline unsigned long long 
MarsagliaXORV(unsigned long long x)
{
    if (x == 0)
    {
        x = 1;
    }

    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;

    return x;
}

static inline unsigned long long 
MarsagliaXOR(unsigned long long *seed)
{
    unsigned long long x = MarsagliaXORV(*seed);
    *seed = x;

    return x;
}

static inline unsigned long long 
TSRandom(Thread *Self)
{
    return MarsagliaXOR(&Self->rng);
}

static inline void 
backoff(Thread *Self, long attempt)
{
    unsigned long long stall = TSRandom(Self) & 0xF;
    stall += attempt >> 2;
    stall *= 10;
    /* CCM: timer function may misbehave */
    volatile typeof(stall) i = 0;
    while (i++ < stall)
    {
        PAUSE();
    }
}

void 
TxAbort(Thread *Self)
{
    Self->Retries++;
    Self->Aborts++;

    // if (Self->Retries > 3)
    // { /* TUNABLE */
    //     backoff(Self, Self->Retries);
    // }

    Self->status = TX_ABORTED;

    // SIGLONGJMP(*Self->envPtr, 1);
    // ASSERT(0);
}

// --------------------------------------------------------------

static inline AVPair*
MakeList (long sz, Log* log)
{
    AVPair* ap = log->List;
    // assert(ap);

    memset(ap, 0, sizeof(*ap) * sz);

    AVPair* List = ap;
    AVPair* Tail = NULL;
    long i;
    for (i = 0; i < sz; i++) {
        AVPair* e = ap++;
        e->Next    = ap;
        e->Prev    = Tail;
        e->Ordinal = i;
        Tail = e;
    }
    Tail->Next = NULL;

    return List;
}

void 
TxInit(Thread *t, long id)
{
    /* CCM: so we can access NOREC's thread metadata in signal handlers */
    // pthread_setspecific(global_key_self, (void*)t);

    memset(t, 0, sizeof(*t)); /* Default value for most members */

    t->UniqID = id;
    t->rng = id + 1;
    t->xorrng[0] = t->rng;

    MakeList(NOREC_INIT_NUM_ENTRY, &(t->wrSet));
    t->wrSet.put = t->wrSet.List;

    MakeList(NOREC_INIT_NUM_ENTRY, &(t->rdSet));
    t->rdSet.put = t->rdSet.List;
}

// --------------------------------------------------------------

static inline void
txReset (Thread* Self)
{
    Self->wrSet.put = Self->wrSet.List;
    Self->wrSet.tail = NULL;

    Self->wrSet.BloomFilter = 0;
    Self->rdSet.put = Self->rdSet.List;
    Self->rdSet.tail = NULL;

    Self->status = TX_ACTIVE;
}

void 
TxStart(Thread *Self)
{
    txReset(Self);

    MEMBARLDLD();

    // Self->envPtr = envPtr;

    Self->Starts++;

    do
    {
        Self->snapshot = *LOCK;
    } while ((Self->snapshot & 1) != 0);
}

// --------------------------------------------------------------

// returns -1 if not coherent
static inline long 
ReadSetCoherent(Thread *Self)
{
    long time;
    while (1)
    {
        MEMBARSTLD();
        time = *LOCK;
        if ((time & 1) != 0)
        {
            continue;
        }

        Log *const rd = &Self->rdSet;
        AVPair *const EndOfList = rd->put;
        AVPair *e;

        for (e = rd->List; e != EndOfList; e = e->Next)
        {
            if (e->Valu != LDNF(e->Addr))
            {
                return -1;
            }
        }

        if (*LOCK == time)
        {
            break;
        }
    }
    return time;
}

// inline AVPair *
// ExtendList(AVPair *tail)
// {
//     AVPair *e = (AVPair *)malloc(sizeof(*e));
//     assert(e);
//     memset(e, 0, sizeof(*e));
//     tail->Next = e;
//     e->Prev = tail;
//     e->Next = NULL;
//     e->Ordinal = tail->Ordinal + 1;
//     return e;
// }

intptr_t 
TxLoad(Thread *Self, volatile intptr_t *Addr)
{
    intptr_t Valu;

    // if (Self->snapshot == -1)
    // {
    //     printf("TID = %ld\n", Self->UniqID);
    // }

    intptr_t msk = FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk)
    {
        Log *wr = &Self->wrSet;
        AVPair *e;
        for (e = wr->tail; e != NULL; e = e->Prev)
        {
            if (e->Addr == Addr)
            {
                return e->Valu;
            }
        }
    }

    // if (Self->snapshot == -1)
    // {
    //     printf("2 TID = %ld\n", Self->UniqID);
    // }

    MEMBARLDLD();
    Valu = LDNF(Addr);
    while (*LOCK != Self->snapshot)
    {
        long newSnap = ReadSetCoherent(Self);
        if (newSnap == -1)
        {
            TxAbort(Self);
            return 0;
        }

        Self->snapshot = newSnap;
        MEMBARLDLD();
        Valu = LDNF(Addr);
    }

    Log *k = &Self->rdSet;
    AVPair *e = k->put;
    if (e == NULL)
    {
    	printf("[WARNING] Reached RS extend\n");
    	assert(0);

        // k->ovf++;
        // e = ExtendList(k->tail);
        // k->end = e;
    }

    k->tail = e;
    k->put = e->Next;
    e->Addr = Addr;
    e->Valu = Valu;

    return Valu;
}

// --------------------------------------------------------------

void 
TxStore(Thread *Self, volatile intptr_t *addr, intptr_t valu)
{
    Log *k = &Self->wrSet;

    k->BloomFilter |= FILTERBITS(addr);

    AVPair *e = k->put;
    if (e == NULL)
    {
    	printf("[WARNING] Reached WS extend\n");
    	assert(0);

        // k->ovf++;
        // e = ExtendList(k->tail);
        // k->end = e;
    }

    k->tail = e;
    k->put = e->Next;
    e->Addr = addr;
    e->Valu = valu;
}

// --------------------------------------------------------------

static inline void 
txCommitReset(Thread *Self)
{
    txReset(Self);
    Self->Retries = 0;

    Self->status = TX_COMMITTED;
}

static inline void 
WriteBackForward(Log *k)
{
    AVPair *e;
    AVPair *End = k->put;
    for (e = k->List; e != End; e = e->Next)
    {
        *(e->Addr) = e->Valu;
    }
}

static inline long 
TryFastUpdate(Thread *Self)
{
    Log *const wr = &Self->wrSet;
    // long ctr;

acquire:
    acquire(LOCK);

    if (*LOCK != Self->snapshot)
    {
    	release(LOCK);

    	long newSnap = ReadSetCoherent(Self);
        if (newSnap == -1)
        {
            return 0; // TxAbort(Self);
        }

        Self->snapshot = newSnap;

        goto acquire;
    }

    *LOCK = Self->snapshot + 1;

    release(LOCK);

    {
        WriteBackForward(wr); /* write-back the deferred stores */
    }

    MEMBARSTST(); /* Ensure the above stores are visible  */
    *LOCK = Self->snapshot + 2;
    MEMBARSTLD();

    return 1; /* success */
}

int 
TxCommit(Thread *Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        txCommitReset(Self);
        return 1;
    }

    if (TryFastUpdate(Self))
    {
        txCommitReset(Self);
        return 1;
    }

    TxAbort(Self);
    return 0;
}