#ifndef H_TIB3
#define H_TIB3

#define	BitSequence	tib3_BitSequence
#define	DataLength	tib3_DataLength
#define	hashState	tib3_hashState
#define	HashReturn	int

#define	Init		tib3_Init
#define	Update		tib3_Update
#define	Final		tib3_Final
#define	Hash		tib3_Hash

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

#include <stdint.h>

/* 256 bits */
#define STATE_DWORDS_256	4 /* Size of the internal state, in 64 bits units */
#define STATE_BYTES_256		32 /* Size of the internal state, in bytes */
#define BLOCK_DWORDS_256	8 /* Size of the block, in 64 bits units */
#define BLOCK_BYTES_256		64 /* Size of the block, in bytes */

/* 224 bits */
#define STATE_DWORDS_224	STATE_DWORDS_256
#define STATE_BYTES_224		STATE_BYTES_256
#define BLOCK_DWORDS_224	BLOCK_DWORDS_256
#define BLOCK_BYTES_224		BLOCK_BYTES_256

/* 512 bits */
#define STATE_DWORDS_512	8
#define STATE_BYTES_512		64
#define BLOCK_DWORDS_512	16
#define BLOCK_BYTES_512		128

/* 384 bits */
#define STATE_DWORDS_384	STATE_DWORDS_512
#define STATE_BYTES_384		STATE_BYTES_512
#define BLOCK_DWORDS_384	BLOCK_DWORDS_512
#define BLOCK_BYTES_384		BLOCK_BYTES_512

typedef struct {
    uint64_t state[STATE_DWORDS_256];		/* internal state */
    uint64_t bits_processed;			
    uint64_t buffer[2*BLOCK_DWORDS_256];    /* buffer for the block and the previous block */
    uint64_t *previous_block;
    uint64_t *data_block;
    uint32_t bits_waiting_for_process;		/* bits awaiting for process in the next call to Update() or Final() */
} hashState256;

typedef struct {
    uint64_t state[STATE_DWORDS_512];
    uint64_t bits_processed;
    uint64_t buffer[2*BLOCK_DWORDS_512];    
    uint64_t *previous_block;
    uint64_t *data_block;
    uint32_t bits_waiting_for_process;
} hashState512;

typedef struct {
    int hashbitlen;
    union {
	hashState256 state256[1];
	hashState512 state512[1];
    } uu[1];
} hashState;

#ifdef __cplusplus
extern "C" {
#endif

HashReturn Init (hashState *state, int hashbitlen);
HashReturn Update (hashState *state , const BitSequence *data, DataLength databitlen);
HashReturn Final (hashState *state, BitSequence *hashval);
HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval);

/* Impedance match bytes -> bits length. */
static inline
int _tib3_Update(void * param, const void * _data, size_t _len)
{
    return Update((hashState *)param, (BitSequence *)_data, (DataLength)(8 * _len));
}

#ifdef __cplusplus
}
#endif

#endif /* H_TIB3 */
