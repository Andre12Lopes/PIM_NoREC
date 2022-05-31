#ifndef _INTSET_MACROS_H_
#define _INTSET_MACROS_H_

#define START(t)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        TxStart(t);

#define LOAD(t, addr)                                                                                                  \
	    TxLoad(t, (__mram_ptr intptr_t *)addr);                                                                                               \
	    if ((t)->status == 4)                                                                                          \
	    {                                                                                                              \
	        continue;                                                                                                  \
	    }

#define LOAD_RO(t, addr)                                                                                           \
	    TxLoad(t, (__mram_ptr intptr_t *)addr);                                                                                             \
	    if ((t)->status == 4)                                                                                         \
	    {                                                                                                              \
	        break;                                                                                                     \
	    }

#define STORE(t, addr, val)                                                                                            \
	    TxStore(t, (__mram_ptr intptr_t *)addr, (intptr_t)val);                                                                                         \
	    if ((t)->status == 4)                                                                                          \
	    {                                                                                                              \
	        continue;                                                                                                  \
	    }

#define COMMIT(t)                                                                                                      \
	    TxCommit(t);                                                                                                   \
	    if ((t)->status != 4)                                                                                          \
	    {                                                                                                              \
	        break;                                                                                                     \
	    }                                                                                                              \
    }                                                                                                                  \
    while (1)

#define RAND_R_FNC(seed)                                                                                               \
    ({                                                                                                                 \
        uint64_t next = (seed);                                                                                        \
        uint64_t result;                                                                                               \
        next *= 1103515245;                                                                                            \
        next += 12345;                                                                                                 \
        result = (uint64_t)(next >> 16) & (2048 - 1);                                                                  \
        next *= 1103515245;                                                                                            \
        next += 12345;                                                                                                 \
        result <<= 10;                                                                                                 \
        result ^= (uint64_t)(next >> 16) & (1024 - 1);                                                                 \
        next *= 1103515245;                                                                                            \
        next += 12345;                                                                                                 \
        result <<= 10;                                                                                                 \
        result ^= (uint64_t)(next >> 16) & (1024 - 1);                                                                 \
        (seed) = next;                                                                                                 \
        result; /* returns result */                                                                                   \
    })

#endif /* _INTSET_MACROS_H_ */