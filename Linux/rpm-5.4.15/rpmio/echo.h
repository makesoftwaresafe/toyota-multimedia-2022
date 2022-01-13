/* echo.h */

/*******************************************************************************
 * Header file for the reference ansi C implementation of the ECHO hash
 * function proposal.
 * Author(s) : Gilles Macario-Rat - Orange Labs - October 2008.
*******************************************************************************/

#ifndef __ECHO_H__
#define __ECHO_H__

#define	BitSequence	echo_BitSequence
#define	DataLength	echo_DataLength
#define	hashState	echo_hashState
#define	HashReturn	int

#define	Init		echo_Init
#define	Update		echo_Update
#define	Final		echo_Final
#define	Hash		echo_Hash

#include <string.h>
#include <stdint.h>

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

typedef struct {
    uint8_t tab [4][4][4][4];
    uint8_t tab_backup [4][4][4][4];
    uint8_t k1 [4][4];
    uint8_t k2 [4][4];
    uint8_t * Addresses[256];
    int index;
    int bit_index;
    int hashbitlen;
    int cv_size;
    int message_size;
    int messlenhi;
    int messlenlo;
    int counter_hi;
    int counter_lo;
    int rounds;
    int Computed;
} hashState;


#ifdef __cplusplus
extern "C" {
#endif
HashReturn Init(hashState *state, int hashbitlen);
HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen);
HashReturn Final(hashState *state, BitSequence *hashval);
HashReturn Hash(int hashbitlen, const BitSequence *data,
                DataLength databitlen, BitSequence *hashval);
#ifdef __cplusplus
}
#endif

/* Impedance match bytes -> bits length. */
static inline
int _echo_Update(void * param, const void * _data, size_t _len)
{
    return Update((hashState *)param, (BitSequence *)_data, (DataLength)(8 * _len));
}
#endif 
