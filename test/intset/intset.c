#include <assert.h>
#include <barrier.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <defs.h>
#include <norec.h>
#include <thread_def.h>

#include "intset_macros.h"

#if defined(LINKED_LIST)
#include "linked_list.h"
#elif defined(SKIP_LIST)
#include "skip_list.h"
#elif defined(HASH_SET)
#include "hash_set.h"
#endif

#define UPDATE_PERCENTAGE   0
#define SET_INITIAL_SIZE    256
#define RAND_RANGE          512

#define N_TRANSACTIONS      100

#ifdef TX_IN_MRAM
#define TYPE __mram_ptr
#else
#define TYPE
#endif

BARRIER_INIT(barr, NR_TASKLETS);

__host uint64_t nb_cycles;
__host uint64_t nb_process_cycles;
__host uint64_t nb_process_read_cycles;
__host uint64_t nb_process_write_cycles;
__host uint64_t nb_process_validation_cycles;
__host uint64_t nb_commit_cycles;
__host uint64_t nb_commit_validation_cycles;
__host uint64_t nb_wasted_cycles;
__host uint64_t n_aborts;
__host uint64_t n_trans;
__host uint64_t n_tasklets;

__mram_ptr intset_t *set;

Thread __mram_noinit t_mram[NR_TASKLETS];

void test(TYPE Thread *tx, __mram_ptr intset_t *set, uint64_t *seed, int *last);

int main()
{
#ifndef TX_IN_MRAM
    Thread tx;
#endif
    int val; 
    int tid;
    uint64_t seed;
    int i = 0;
    int last = -1;
    perfcounter_t initial_time;

    seed = me();
    tid = me();

#ifdef TX_IN_MRAM
    TxInit(&t_mram[tid], tid);

    t_mram[tid].process_cycles = 0;
    t_mram[tid].read_cycles = 0;
    t_mram[tid].write_cycles = 0;
    t_mram[tid].validation_cycles = 0;
    t_mram[tid].total_read_cycles = 0;
    t_mram[tid].total_write_cycles = 0;
    t_mram[tid].total_validation_cycles = 0;
    t_mram[tid].total_commit_validation_cycles = 0;
    t_mram[tid].commit_cycles = 0;
    t_mram[tid].total_cycles = 0;
    t_mram[tid].start_time = 0;
    t_mram[tid].start_read = 0;
    t_mram[tid].start_write = 0;
    t_mram[tid].start_validation = 0;
#else
    TxInit(&tx, tid);

    tx.process_cycles = 0;
    tx.read_cycles = 0;
    tx.write_cycles = 0;
    tx.validation_cycles = 0;
    tx.total_read_cycles = 0;
    tx.total_write_cycles = 0;
    tx.total_validation_cycles = 0;
    tx.total_commit_validation_cycles = 0;
    tx.commit_cycles = 0;
    tx.total_cycles = 0;
    tx.start_time = 0;
    tx.start_read = 0;
    tx.start_write = 0;
    tx.start_validation = 0;
#endif

    if (tid == 0)
    {
        set = set_new(INIT_SET_PARAMETERS);

        while (i < SET_INITIAL_SIZE) 
        {
            val = (RAND_R_FNC(seed) % RAND_RANGE) + 1;
            if (set_add(NULL, set, val, 0))
            {
                i++;
            }            
        }

        n_trans = N_TRANSACTIONS * NR_TASKLETS;
        n_tasklets = NR_TASKLETS;
        n_aborts = 0;

        initial_time = perfcounter_config(COUNT_CYCLES, false);

        // print_linked_list(set);
    }

    barrier_wait(&barr);

    for (int i = 0; i < N_TRANSACTIONS; ++i)
    {
#ifdef TX_IN_MRAM
        test(&(t_mram[tid]), set, &seed, &last);
#else
        test(&tx, set, &seed, &last);
#endif
    }

    barrier_wait(&barr);

    if (me() == 0)
    {
        nb_cycles = perfcounter_get() - initial_time;

        nb_process_cycles = 0;
        nb_commit_cycles = 0;
        nb_wasted_cycles = 0;
        nb_process_read_cycles = 0;
        nb_process_write_cycles = 0;
        nb_process_validation_cycles = 0;
        nb_commit_validation_cycles = 0;
    }

    for (int i = 0; i < NR_TASKLETS; ++i)
    {
        if (me() == i)
        {
#ifdef TX_IN_MRAM
            n_aborts += t_mram[tid].Aborts;

            nb_process_cycles += ((double) t_mram[tid].process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_read_cycles += ((double) t_mram[tid].total_read_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_write_cycles += ((double) t_mram[tid].total_write_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_validation_cycles += ((double) t_mram[tid].total_validation_cycles / (N_TRANSACTIONS * NR_TASKLETS));

            nb_commit_cycles += ((double) t_mram[tid].commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_validation_cycles += ((double) t_mram[tid].total_commit_validation_cycles / (N_TRANSACTIONS * NR_TASKLETS));

            nb_wasted_cycles += ((double) (t_mram[tid].total_cycles - (t_mram[tid].process_cycles + t_mram[tid].commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#else
            n_aborts += tx.Aborts;

            nb_process_cycles += ((double) tx.process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_read_cycles += ((double) tx.total_read_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_write_cycles += ((double) tx.total_write_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_process_validation_cycles += ((double) tx.total_validation_cycles / (N_TRANSACTIONS * NR_TASKLETS));

            nb_commit_cycles += ((double) tx.commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_validation_cycles += ((double) tx.total_commit_validation_cycles / (N_TRANSACTIONS * NR_TASKLETS));

            nb_wasted_cycles += ((double) (tx.total_cycles - (tx.process_cycles + tx.commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#endif
        }

        barrier_wait(&barr);
    }

    return 0;
}


void test(TYPE Thread *tx, __mram_ptr intset_t *set, uint64_t *seed, int *last)
{
    int val, op;

    op = RAND_R_FNC(*seed) % 100;
    if (op < UPDATE_PERCENTAGE)
    {
        if (*last < 0)
        {
            /* Add random value */
            val = (RAND_R_FNC(*seed) % RAND_RANGE) + 1;
            if (set_add(tx, set, val, 1))
            {
                *last = val;
            }
        }
        else
        {
            /* Remove last value */
            set_remove(tx, set, *last);
            *last = -1;
        }
    }
    else
    {
        /* Look for random value */
        val = (RAND_R_FNC(*seed) % RAND_RANGE) + 1;
        set_contains(tx, set, val);
    }
}