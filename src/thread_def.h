#ifndef _THREAD_DEF_H_
#define _THREAD_DEF_H_

#define NOREC_INIT_NUM_ENTRY      2

typedef int BitMap;

typedef struct _AVPair
{
    struct _AVPair *Next;
    struct _AVPair *Prev;
    volatile intptr_t *Addr;
    intptr_t Valu;
    long Ordinal;
} AVPair;

typedef struct _Log
{
    AVPair List[NOREC_INIT_NUM_ENTRY];
    AVPair *put;        /* Insert position - cursor */
    AVPair *tail;       /* CCM: Pointer to last valid entry */
    AVPair *end;        /* CCM: Pointer to last entry */
    long ovf;           /* Overflow - request to grow */
    BitMap BloomFilter; /* Address exclusion fast-path test */
} Log;

struct _Thread
{
    long UniqID;
    volatile long Retries;
    long Starts;
    long Aborts; /* Tally of # of aborts */
    long snapshot;
    unsigned long long rng;
    unsigned long long xorrng[1];
    Log rdSet;
    Log wrSet;
    long status;
    // sigjmp_buf* envPtr;
};

#endif