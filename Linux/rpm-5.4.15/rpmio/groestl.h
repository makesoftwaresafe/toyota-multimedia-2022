#ifndef __groestl_ref_h
#define __groestl_ref_h

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define	BitSequence	groestl_BitSequence
#define	DataLength	groestl_DataLength
#define	hashState	groestl_hashState
#define	HashReturn	int

#define	Init		groestl_Init
#define	Update		groestl_Update
#define	Final		groestl_Final
#define	Hash		groestl_Hash

#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8
#define COLS1024 16
#define SIZE512 (ROWS*COLS512)
#define SIZE1024 (ROWS*COLS1024)
#define ROUNDS512 10
#define ROUNDS1024 14

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

typedef struct {
    uint8_t chaining[ROWS][COLS1024];	/* the actual state */
    uint64_t block_counter;		/* block counter */
    int hashbitlen;			/* output length */
    BitSequence buffer[SIZE1024];	/* block buffer */
    int buf_ptr;			/* buffer pointer */
    int bits_in_last_byte;		/* number of bits in incomplete byte */
    int columns;			/* number of columns in state */
    int rounds;				/* number of rounds in P and Q */
    int statesize;			/* size of state (ROWS*columns) */
} hashState;

#ifdef __cplusplus
extern "C" {
#endif

HashReturn Hash(int, const BitSequence*, DataLength, BitSequence*);
HashReturn Init(hashState*, int);
HashReturn Update(hashState*, const BitSequence*, DataLength);
HashReturn Final(hashState*, BitSequence*);

/* Impedance match bytes -> bits length. */
static inline
int _groestl_Update(void * param, const void * _data, size_t _len)
{
    return Update((hashState *)param, (BitSequence *)_data, (DataLength)(8 * _len));
}

#ifdef __cplusplus
}
#endif

#endif /* __groestl_ref_h */
