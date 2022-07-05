#ifndef _THREAD_DEF_H_
#define _THREAD_DEF_H_

#include <perfcounter.h>

#define NOREC_INIT_NUM_ENTRY      2

typedef int BitMap;

typedef struct _AVPair
{
    TYPE struct _AVPair *Next;
    TYPE struct _AVPair *Prev;
    volatile TYPE_ACC intptr_t *Addr;
    intptr_t Valu;
    long Ordinal;
} AVPair;

typedef struct _Log
{
    AVPair List[NOREC_INIT_NUM_ENTRY];
    TYPE AVPair *put;        /* Insert position - cursor */
    TYPE AVPair *tail;       /* CCM: Pointer to last valid entry */
    TYPE AVPair *end;        /* CCM: Pointer to last entry */
    long ovf;                /* Overflow - request to grow */
    BitMap BloomFilter;      /* Address exclusion fast-path test */
} Log;

struct _Thread
{
    long UniqID;
    volatile long Retries;
    long Starts;
    long Aborts;            /* Tally of # of aborts */
    long snapshot;
    unsigned long long rng;
    unsigned long long xorrng[1];
    Log rdSet;
    Log wrSet;
    long status;
    perfcounter_t time;
    perfcounter_t start_time;
    perfcounter_t start_read;
    perfcounter_t start_write;
    perfcounter_t start_validation;
    uint64_t process_cycles;
    uint64_t read_cycles;
    uint64_t write_cycles;
    uint64_t validation_cycles;
    uint64_t total_read_cycles;
    uint64_t total_write_cycles;
    uint64_t total_validation_cycles;
    uint64_t total_commit_validation_cycles;
    uint64_t commit_cycles;
    uint64_t total_cycles;
};

#endif
