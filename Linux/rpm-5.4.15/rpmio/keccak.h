/*
Algorithm Name: Keccak
Authors: Guido Bertoni, Joan Daemen, Michaël Peeters and Gilles Van Assche
Date: April 21, 2009

This code, originally by Guido Bertoni, Joan Daemen, Michaël Peeters and
Gilles Van Assche as a part of the SHA-3 submission, is hereby put in the
public domain. It is given as is, without any guarantee.

For more information, feedback or questions, please refer to our website:
http://keccak.noekeon.org/
*/

#ifndef _KeccakNISTInterface_h_
#define _KeccakNISTInterface_h_

#define	BitSequence	keccak_BitSequence
#define	DataLength	keccak_DataLength
#define	hashState	keccak_hashState
#define	HashReturn	int

#define	Init		keccak_Init
#define	Update		keccak_Update
#define	Final		keccak_Final
#define	Hash		keccak_Hash

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

#define KeccakPermutationSize 1600
#define KeccakPermutationSizeInBytes (KeccakPermutationSize/8)
#define KeccakMaximumRate 1024
#define KeccakMaximumRateInBytes (KeccakMaximumRate/8)

#if defined(__GNUC__)
#define ___ALIGN __attribute__ ((aligned(32)))
#elif defined(_MSC_VER)
#define ___ALIGN __declspec(align(32))
#else
#define ___ALIGN
#endif

___ALIGN typedef struct {
    ___ALIGN unsigned char state[KeccakPermutationSizeInBytes];
    ___ALIGN unsigned char dataQueue[KeccakMaximumRateInBytes];
    unsigned int rate;
    unsigned int capacity;
    unsigned char diversifier;
    unsigned int hashbitlen;
    unsigned int bitsInQueue;
    int squeezing;
    unsigned int bitsAvailableForSqueezing;
} hashState;

#ifdef __cplusplus
extern "C" {
#endif

HashReturn Init(hashState *state, int hashbitlen);
HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen);
HashReturn Final(hashState *state, BitSequence *_hashval);
HashReturn Squeeze(hashState *state, BitSequence *output, DataLength outputLength);

HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval);

/* Impedance match bytes -> bits length. */
static inline
int _keccak_Update(void * param, const void * _data, size_t _len)
{
    return Update((hashState *)param, (BitSequence *)_data, (DataLength)(8 * _len));
}

#ifdef __cplusplus
}
#endif

#endif
