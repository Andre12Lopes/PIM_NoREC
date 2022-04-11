#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <alloc.h>
#include <assert.h>
#include <barrier.h>
#include <perfcounter.h>

#include <norec.h>
#include <thread_def.h>
#include <defs.h>
#include <mram.h>

#include "macros.h"


#define TRANSFER 2
#define N_ACCOUNTS 800
#define ACCOUNT_V 1000
#define N_TRANSACTIONS 1000

BARRIER_INIT(my_barrier, NR_TASKLETS);

__host uint32_t nb_cycles;
__host uint32_t nb_process_cycles;
__host uint32_t nb_commit_cycles;
__host uint32_t nb_wasted_cycles;
__host uint32_t n_aborts;
__host uint32_t n_trans;
__host uint32_t n_tasklets;

#ifdef ACC_IN_MRAM
int __mram_noinit bank[N_ACCOUNTS];
#else
int bank[N_ACCOUNTS];
#endif

void initialize_accounts();
void check_total();

#ifdef TX_IN_MRAM
Thread __mram_noinit t_mram[NR_TASKLETS];
#endif

int main()
{
    Thread t;
    int ra, rb, tid;
    int a, b;
    uint64_t s;
    perfcounter_t initial_time;

    s = (uint64_t)me();
    tid = me();

#ifdef TX_IN_MRAM
    TxInit(&t_mram[tid], tid);

    t_mram[tid].process_cycles = 0;
    t_mram[tid].commit_cycles = 0;
    t_mram[tid].total_cycles = 0;
    t_mram[tid].start_time = 0;
#else
    TxInit(&t, tid);

    t.process_cycles = 0;
    t.commit_cycles = 0;
    t.total_cycles = 0;
    t.start_time = 0;
#endif

    initialize_accounts();

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        n_trans = N_TRANSACTIONS * NR_TASKLETS;
        n_tasklets = NR_TASKLETS;

        initial_time = perfcounter_config(COUNT_CYCLES, false);
    }

    // ------------------------------------------------------

    for (int i = 0; i < N_TRANSACTIONS; ++i)
    {
        ra = RAND_R_FNC(s) % N_ACCOUNTS;
        rb = RAND_R_FNC(s) % N_ACCOUNTS;

#ifdef TX_IN_MRAM
        START(&t_mram[tid]);

        a = LOAD(&t_mram[tid], &bank[ra]);
        a -= TRANSFER;
        STORE(&t_mram[tid], &bank[ra], a);

        b = LOAD(&t_mram[tid], &bank[rb]);
        b += TRANSFER;
        STORE(&t_mram[tid], &bank[rb], b);

        COMMIT(&t_mram[tid]);
#else
        START(&t);

        a = LOAD(&t, &bank[ra]);
        a -= TRANSFER;
        STORE(&t, &bank[ra], a);

        b = LOAD(&t, &bank[rb]);
        b += TRANSFER;
        STORE(&t, &bank[rb], b);

        COMMIT(&t);
#endif
    }

    // ------------------------------------------------------

    barrier_wait(&my_barrier);

    if (me() == 0)
    {
        nb_cycles = perfcounter_get() - initial_time;

        nb_process_cycles = 0;
        nb_commit_cycles = 0;
        nb_wasted_cycles = 0;
    }

    for (int i = 0; i < NR_TASKLETS; ++i)
    {
        if (me() == i)
        {
            n_aborts += t.Aborts;

#ifdef TX_IN_MRAM
            nb_process_cycles += ((double) t_mram[tid].process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_cycles += ((double) t_mram[tid].commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_wasted_cycles += ((double) (t_mram[tid].total_cycles - (t_mram[tid].process_cycles + t_mram[tid].commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#else
            nb_process_cycles += ((double) t.process_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_commit_cycles += ((double) t.commit_cycles / (N_TRANSACTIONS * NR_TASKLETS));
            nb_wasted_cycles += ((double) (t.total_cycles - (t.process_cycles + t.commit_cycles)) / (N_TRANSACTIONS * NR_TASKLETS));
#endif
        }

        barrier_wait(&my_barrier);
    }

    check_total();
    
    return 0;
}

void initialize_accounts()
{
    if (me() == 0)
    {
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            bank[i] = ACCOUNT_V;
        }    
    }
}

void check_total()
{
    if (me() == 0)
    {
        printf("[");
        unsigned int total = 0;
        for (int i = 0; i < N_ACCOUNTS; ++i)
        {
            printf("%u,", bank[i]);
            total += bank[i];
        }
        printf("]\n");

        printf("TOTAL = %u\n", total);

        assert(total == (N_ACCOUNTS * ACCOUNT_V));
    }
}