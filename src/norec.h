#ifndef _NOREC_H_
#define _NOREC_H_

#include <stdint.h>

typedef struct _Thread Thread;

void TxAbort(Thread *);

void TxInit(Thread *t, long id);

void TxStart(Thread *);

intptr_t TxLoad(Thread *, volatile intptr_t *);

void TxStore(Thread *, volatile intptr_t *, intptr_t);

int TxCommit(Thread *);
// int TxCommitSTM(Thread *);

#endif