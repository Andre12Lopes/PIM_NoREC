#ifndef _NOREC_H_
#define _NOREC_H_

#include <stdint.h>

#ifdef TX_IN_MRAM
#define TYPE __mram_ptr
#else
#define TYPE
#endif

typedef struct _Thread Thread;

void TxAbort(TYPE Thread *);

void TxInit(TYPE Thread *t, long id);

void TxStart(TYPE Thread *);

intptr_t TxLoad(TYPE Thread *, volatile intptr_t *);

void TxStore(TYPE Thread *, volatile intptr_t *, intptr_t);

int TxCommit(TYPE Thread *);
// int TxCommitSTM(Thread *);

#endif