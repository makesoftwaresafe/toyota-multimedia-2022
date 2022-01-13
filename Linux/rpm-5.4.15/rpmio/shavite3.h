/*******************************************************************/
/*  SHA3api_ref.h -- Definitions required by the API               */
/*                                                                 */
/*  Written by Eli Biham and Orr Dunkelman                         */
/*                                                                 */
/*******************************************************************/

#ifndef H_SHAVITE3
#define H_SHAVITE3

#include <string.h>

#define	BitSequence	shavite3_BitSequence
#define	DataLength	shavite3_DataLength
#define	hashState	shavite3_hashState
#define	HashReturn	int

#define	Init		shavite3_Init
#define	Update		shavite3_Update
#define	Final		shavite3_Final
#define	Hash		shavite3_Hash

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

/* SHAvite-3 definition */

typedef struct {
   DataLength bitcount;            /* The number of bits compressed so far   */
   BitSequence chaining_value[64]; /* An array containing the chaining value */
   BitSequence buffer[128];        /* A buffer storing bytes until they are  */
				   /* compressed			     */
   BitSequence partial_byte;       /* A byte to store a fraction of a byte   */
				   /* in case the input is not fully byte    */
				   /* aligned				     */
   BitSequence salt[64];           /* The salt used in the hash function     */ 
   int DigestSize;		   /* The requested digest size              */
   int BlockSize;		   /* The message block size                 */
} hashState;

/* Function calls imposed by the API */

#ifdef __cplusplus
extern "C" {
#endif

HashReturn Init (hashState *state, int hashbitlen);

HashReturn Update (hashState *state, const BitSequence *data, DataLength
                   databitlen);

HashReturn Final (hashState *state, BitSequence *hashval);

HashReturn Hash (int hashbitlen, const BitSequence *data, 
		 DataLength databitlen, BitSequence *hashval);

/* Impedance match bytes -> bits length. */
static inline
int _shavite3_Update(void * param, const void * _data, size_t _len)
{
    return Update((hashState *)param, (BitSequence *)_data, (DataLength)(8 * _len));
}

#ifdef __cplusplus
}
#endif

#endif
