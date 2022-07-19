#ifndef _NOREC_H_
#define _NOREC_H_

#include <stdint.h>

#ifdef TX_IN_MRAM
#define TYPE __mram_ptr
#else
#define TYPE
#endif

#ifdef ACC_IN_MRAM
#define TYPE_ACC __mram_ptr
#else
#define TYPE_ACC
#endif

#include "thread_def.h"

typedef struct _Thread Thread;

void TxAbort(Thread *);

void TxInit(Thread *t, long id, __mram_ptr AVPair *rs_overflow, __mram_ptr AVPair *ws_overflow);

void TxStart(Thread *);

intptr_t TxLoad(Thread *, volatile __mram_ptr intptr_t *);

void TxStore(Thread *, volatile __mram_ptr intptr_t *, intptr_t);

int TxCommit(Thread *);
// int TxCommitSTM(Thread *);

#endif