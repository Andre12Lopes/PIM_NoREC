#if defined(HASH_SET)

#include <alloc.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include <norec.h>
#include <thread_def.h>

#include "hash_set.h"
#include "dpu_alloc.h"
#include "intset_macros.h"

#define NB_BUCKETS	(1UL << 10)

#define HASH(a)		(hash((uint32_t)a) & (NB_BUCKETS - 1))


static uint32_t hash(uint32_t a)
{
    /* Knuth's multiplicative hash function */
    a *= 2654435761UL;
    return a;
}


static __mram_ptr bucket_t *new_entry(val_t val, __mram_ptr bucket_t *next, int transactional)
{
    __mram_ptr bucket_t *b;

    // if (!transactional)
    // {
        b = (__mram_ptr bucket_t *)mram_malloc(sizeof(bucket_t));
    // }
    // else
    // {
    //     b = (bucket_t *)TM_MALLOC(sizeof(bucket_t));
    // }

    if (b == NULL)
    {
        exit(1);
    }

    b->val = val;
    b->next = next;

    return b;
}

__mram_ptr intset_t *set_new()
{
    __mram_ptr intset_t *set;

    if ((set = (__mram_ptr intset_t *)mram_malloc(sizeof(intset_t))) == NULL)
    {
        exit(1);
    }

    if ((set->buckets = (__mram_ptr uintptr_t *)mram_malloc(NB_BUCKETS * sizeof(bucket_t))) == NULL)
    {
        exit(1);
    }

    for (int i = 0; i < NB_BUCKETS; ++i)
    {
    	set->buckets[i] = 0;
    }

    return set;
}

int set_contains(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val)
{
    int result, i;
    __mram_ptr bucket_t *b;
    val_t v;

    START(tx);
    i = HASH(val);
    b = (__mram_ptr bucket_t *)LOAD(tx, &(set->buckets[i]));
    result = 0;

    while (b != NULL)
    {
        v = LOAD_RO(tx, &(b->val));
        if (v == val)
        {
            result = 1;
            break;
        }

        b = (__mram_ptr bucket_t *)LOAD_RO(tx, &(b->next));
    }

    if (tx->status == 4)
    {
        continue;
    }

    COMMIT(tx);

    return result;
}

int set_add(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val, int transactional)
{
    int result, i;
    __mram_ptr bucket_t *b, *first;
    val_t v;

    if (!transactional)
    {
        i = HASH(val);
        first = (__mram_ptr bucket_t *)set->buckets[i];
        b = (__mram_ptr bucket_t *)set->buckets[i];
        result = 1;
        while (b != NULL)
        {
            if (b->val == val)
            {
                result = 0;
                break;
            }
            b = b->next;
        }

        if (result)
        {
            set->buckets[i] = (uintptr_t)new_entry(val, first, 0);
        }
    }
    else
    {
        START(tx);
        i = HASH(val);
        first = b = (__mram_ptr bucket_t *)LOAD(tx, &(set->buckets[i]));
        result = 1;
        while (b != NULL)
        {
            v = LOAD_RO(tx, &(b->val));
            if (v == val)
            {
                result = 0;
                break;
            }

            b = (__mram_ptr bucket_t *)LOAD_RO(tx, &(b->next));
        }

        if (tx->status == 4)
        {
            continue;
        }

        if (result)
        {
            STORE(tx, &(set->buckets[i]), new_entry(val, first, 1));
        }

        COMMIT(tx);
    }

    return result;
}

int set_remove(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val)
{
    int result, i;
    __mram_ptr bucket_t *b, *prev;
    __mram_ptr bucket_t *bucket;
    val_t v;

    START(tx);
    i = HASH(val);
    prev = b = (__mram_ptr bucket_t *)LOAD(tx, &(set->buckets[i]));
    result = 0;

    while (b != NULL)
    {
        v = LOAD_RO(tx, &(b->val));
        if (v == val)
        {
            result = 1;
            break;
        }

        prev = b;
        b = (__mram_ptr bucket_t *)LOAD_RO(tx, &(b->next));
    }

    if (tx->status == 4)
    {
        continue;
    }

    if (result)
    {
        if (prev == b)
        {
            /* First element of bucket */
            bucket = (__mram_ptr bucket_t *)LOAD(tx, &(b->next));
            STORE(tx, &(set->buckets[i]), bucket);
        }
        else
        {
            bucket = (__mram_ptr bucket_t *)LOAD(tx, &(b->next));            
            STORE(tx, &(prev->next), bucket);
        }
        /* Free memory (delayed until commit) */
        // TM_FREE2(b, sizeof(bucket_t));
    }

    COMMIT(tx);

    return result;
}

#endif