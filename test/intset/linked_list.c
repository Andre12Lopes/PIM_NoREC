#if defined(LINKED_LIST)

#include <alloc.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

#include <norec.h>
#include <thread_def.h>

#include "linked_list.h"
#include "dpu_alloc.h"
#include "intset_macros.h"

#define VAL_MIN 0
#define VAL_MAX INT_MAX

static __mram_ptr node_t *new_node(val_t val, __mram_ptr node_t *next, int transactional)
{
    __mram_ptr node_t *node;

    node = (__mram_ptr node_t *)mram_malloc(sizeof(node_t));

    if (node == NULL)
    {
        exit(1);
    }

    node->val = val;
    node->next = next;

    return node;
}

__mram_ptr intset_t *set_new()
{
    __mram_ptr intset_t *set;
    __mram_ptr node_t *min, *max;

    if ((set = (__mram_ptr intset_t *)mram_malloc(sizeof(intset_t))) == NULL)
    {
        exit(1);
    }

    max = new_node(VAL_MAX, NULL, 0);
    min = new_node(VAL_MIN, max, 0);
    set->head = min;

    return set;
}

int set_contains(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val)
{
    int result;
    __mram_ptr node_t *prev, *next;
    val_t v;

    START(tx);
    prev = (__mram_ptr node_t *)LOAD(tx, &(set->head));
    next = (__mram_ptr node_t *)LOAD(tx, &(prev->next));

    while (1)
    {
        v = LOAD_RO(tx, &(next->val));

        if (v >= val)
        {
            break;
        }

        prev = next;
        next = (__mram_ptr node_t *)LOAD_RO(tx, &(prev->next));
    }

    if (tx->status == 4)
    {
        continue;
    }

    result = (v == val);
    COMMIT(tx);

    return result;
}

int set_add(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val, int transactional)
{
    int result;
    __mram_ptr node_t *prev, *next;
    val_t v;

    if (!transactional)
    {
        prev = set->head;
        next = prev->next;
        while (next->val < val)
        {
            prev = next;
            next = prev->next;
        }
        result = (next->val != val);
        if (result)
        {
            prev->next = new_node(val, next, 0);
        }
    }
    else
    {
        START(tx);
        prev = (__mram_ptr node_t *)LOAD(tx, &(set->head));
        next = (__mram_ptr node_t *)LOAD(tx, &(prev->next));

        while (1)
        {
            v = LOAD_RO(tx, &(next->val));
            if (v >= val)
            {
                break;
            }

            prev = next;
            next = (__mram_ptr node_t *)LOAD_RO(tx, &(prev->next));
        }

        if (tx->status == 4)
        {
            continue;
        }

        result = (v != val);
        if (result)
        {
            STORE(tx, &(prev->next), new_node(val, next, 1));
        }

        COMMIT(tx);
    }

    return result;
}

int set_remove(TYPE Thread *tx, __mram_ptr intset_t *set, val_t val)
{
    int result = 0;
    __mram_ptr node_t *prev, *next;
    __mram_ptr node_t *n;
    val_t v;

    START(tx);
    prev = (__mram_ptr node_t *)LOAD(tx, &(set->head));
    next = (__mram_ptr node_t *)LOAD(tx, &(prev->next));

    while (1)
    {
        v = LOAD_RO(tx, &(next->val));
        if (v >= val)
        {
            break;
        }

        prev = next;
        next = (__mram_ptr node_t *)LOAD_RO(tx, &(prev->next));
    }

    if (tx->status == 4)
    {
        continue;
    }

    result = (v == val);
    if (result)
    {
        n = (__mram_ptr node_t *)LOAD(tx, &(next->next));

        STORE(tx, &(prev->next), n);
    }

    COMMIT(tx);

    return result;
}

#endif