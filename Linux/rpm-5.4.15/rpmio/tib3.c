#include <string.h>
#include "tib3.h"

enum { SUCCESS = 0, FAIL = 1, BAD_HASHBITLEN = 2 };

/* Processes 4 64 bits words, and returns one.
 * Input words:
 *	di10, di8, di3, di2
 * Output word:
 *	di
 */
#define PSI(di,di10,di8,di3,di2) \
	di=(di3+(di2<<32))^di10^di8^(di2>>32);\
	di+=(di<<32)+(di<<43);\
	di^=(di>>39);

/* Expands 8 64 bit words to 32
 * Input:
 *	D: an array of 64 bit words, with the first 8 already filled
 *	pk: the previous block
 *	bits: bits processed, including this block
 * Output:
 *	D: the array with the 32 words filled
 */
#define EXPANSION_256(D,pk,bits) \
   PSI(D[8],(D[3]^pk[0]),(D[4]^pk[1]),(D[5]^pk[2]),(D[1]^pk[3]));\
   PSI(D[9],(D[2]^0x428a2f98d728ae22ULL^pk[4]),(D[7]^bits^pk[5]),(D[6]^pk[7]),(D[0]^pk[6]));\
   for( i = 10; i!=32;i++) {\
     PSI(D[i],D[i-10],D[i-8],D[i-3],D[i-2]);\
   }

/* Macros used in the round function */
#define F(X,Y,Z) ((~X)^(Y&(~Z)))
#define G(X,Y,Z) (Z^((~X)&(~Y)))
#define H(X,Y,Z) (Y^(X&Z))

/* Bitsliced Sbox_256 */
#define Sbox_256(m0,m1,m2)    \
{ register uint64_t temp0, temp1;\
        temp0=F(m0,m1,m2);         \
        temp1=G(m0,m1,m2);         \
        m2=H(m0,m1,m2);         \
        m0=temp0;                  \
	    m1=temp1; }

#define PHTX_256(D)  \
       D+=(D<<32)+(D<<47);\
       D^=(D>>32)^(D>>43);

/*original diffusion
#define DIFFUSIONoriginal(A,C,E,G) \
	A=((A&0xffffffff00000000)+(G&0xffffffff00000000))|((A+G)&0xffffffff);\
	PHTX_256(C);\
	G=((G&0xffffffff00000000)+(E&0xffffffff00000000))|((G+E)&0xffffffff);
*/

/*new diffusion*/
#define DIFFUSION_256(A,C,E,G) \
	A=(G<<37)^(G>>27)^(A<<12)^(A>>52)^(A>>1);\
	PHTX_256(C);\
	G=((G&0xffffffff00000000ULL)+(E&0xffffffff00000000ULL))|((G+E)&0xffffffffULL);


/* Round function
 * Input:
 *	A,C,E,G: state, as 4 64 bit words
 *	k0,k2,k4: round key, as 3 64 bit words.
 * Output:
 *	A,C,E,G: state, modified by the S-Box
 */
#define ROUND_256(A,C,E,G,k0,k2,k4) \
    A^=k0;\
	C^=k2;\
	E^=k4;\
	G^=C;\
	PHTX_256(G);\
    Sbox_256(A,C,E) ;\
    DIFFUSION_256(A,C,E,G);

/* Calls the round function 4 times, rotating the words of the state,
 * and changing the round key.
 * Input:
 *	k0,k2,k4,k6,k8,k10,k12,k14,k16,k18,k20,k22: key material
 *	h0,h2,h4,h6: 256 bits state
 * Output:
 *	h0,h2,h4,h6: state, modified by the 4 round functions.
 */
#define FOURROUNDSBOX_256(k0,k2,k4,k6,k8,k10,k12,k14,k16,k18,k20,k22,h0,h2,h4,h6)\
	ROUND_256(h0,h2,h4,h6,k0, k2, k4);\
	ROUND_256(h2,h4,h6,h0,k6, k8, k10);\
	ROUND_256(h4,h6,h0,h2,k12,k14,k16);\
	ROUND_256(h6,h0,h2,h4,k18,k20,k22);

/* First four-round group, using the expanded D and the block k */
#define INPUTONE_256\
	FOURROUNDSBOX_256(D[0],k[0],D[1],D[2],k[1],D[3],D[4],k[2],D[5],D[6],k[3],D[7],hsh[0],hsh[1],hsh[2],hsh[3]);

/* Second four-round group, using the expanded D and the remaining words of block k  */
#define INPUTTWO_256\
	FOURROUNDSBOX_256(D[8],k[4],D[9],D[10],k[5],D[11],D[12],k[6],D[13],D[14],k[7],D[15],hsh[0],hsh[1],hsh[2],hsh[3]);

/* Third four-round group, using the expanded D and all the words of the previous block pk  */
#define INPUTTHREE_256\
	FOURROUNDSBOX_256(pk[0],D[16],pk[1],pk[2],D[17],pk[3],pk[4],D[18],pk[5],pk[6],D[19],pk[7],hsh[0],hsh[1],hsh[2],hsh[3]);

/* Fourth four-round group, using the remaining words of the expanded D  */
#define INPUTFOUR_256\
	FOURROUNDSBOX_256(D[20],D[21],D[22],D[23],D[24],D[25],D[26],D[27],D[28],D[29],D[30],D[31],hsh[0],hsh[1],hsh[2],hsh[3]);

/* Encrypt function. Encrypts the state, using the block, the previous block 
 * and the bits processed as key material. Modifies the state in place.
 * Input:
 *	hsh: Array of 4 64 bit words, storing the state.
 *	k: Array of 8 64 bit words, storing the current block.
 *	pk: Array of 8 64 bit words, storing the previous block.
 *	bits: 64 bit word representing the bits processes, up to and including the current block.
 * Output:
 *	hsh: the state, encrypted.
 */
static
void Encrypt256(uint64_t hsh[4], uint64_t k[8], uint64_t pk[8],
		uint64_t bits)
{

	uint64_t D[32];
	int i;

	/* Start  */

	/*expansion*/
	D[0] = pk[0] ^ k[0];
	for (i = 7; i; i--)
		D[i] = pk[i] ^ k[i];

	EXPANSION_256(D,pk,bits);

	INPUTONE_256;

	INPUTTWO_256;

	INPUTTHREE_256;

	INPUTFOUR_256;

}

/* ===== encrypt512.c */

/* Processes 8 64 bits words, and returns 2.
 * Input words:
 *	wi20,wi19,wi16,wi15,wi6,wi5,wi4,wi3
 * Output words:
 *	wi,wim1
 */
#define PHI(wi,wim1,wi20,wi19,wi16,wi15,wi6,wi5,wi4,wi3) \
      wi=wi20^wi16^wi6^wi3;\
      wim1=((wi5+wi4)^wi19^wi15)+wi+(wi<<23);\
      wi^=(wim1>>15);

/* Expands 16 64 bit words to 64
 * Input:
 *	W: an array of 64 bit words, with the first 16 already filled
 *	pk: the previous block
 *	bits: bits processed, including this block
 * Output:
 *	W: the array with the 64 words filled
 */
#define EXPANSION_512(W,pk,bits) \
	PHI(W[16],W[17],(W[6]^pk[0]),(W[7]^pk[1]),(W[8]^pk[2]),(W[9]^pk[3]),(W[10]^pk[4]),(W[11]^pk[5]),(W[2]^pk[6]),(W[3]^pk[7]));\
    PHI(W[18],W[19],(W[4]^0x428a2f98d728ae22ULL^pk[8]),(W[5]^pk[9]),(W[14]^bits^pk[10]),(W[15]^pk[11]),(W[12]^pk[14]),(W[13]^pk[15]),(W[0]^pk[12]),(W[1]^pk[13]));\
    for( i = 20; i!=64; i=i+2) {\
        PHI(W[i],W[i+1],W[i-20],W[i-19],W[i-16],W[i-15],W[i-6],W[i-5],W[i-4],W[i-3]);\
    }

/*Sbox_512 in bitslice mode */
#define Sbox_512(m0,m1,m2) \
  { register uint64_t temp0, temp1;\
		temp0=F(m0,m1,m2);\
        temp1=G(m0,m1,m2);\
        m2=H(m0,m1,m2);\
        m0=temp0;\
		m1=temp1; }

#define PHTX_512(D)  \
       D+=(D<<32)+(D<<47);\
       D^=(D>>32)^(D>>43);

#define PHTXD_512(L,H)  \
	H^=L;\
	PHTX_512(H);\
	L^=H;\
	PHTX_512(L);

/*original diffusion 
#define DIFFUSIONoriginal(A,B,C,D,E,F,G,H) \
	A+=G;\
	B+=H;\
	PHTXD_512(C,D);\
	G+=E;\
	H+=F;

*/

#define DIFFUSION_512(A,B,C,D,E,F,G,H) \
	A=(H<<37)^(H>>27)^(A<<12)^(A>>52)^(A>>1);\
	B=(G<<37)^(G>>27)^(B<<12)^(B>>52)^(B>>1);\
	PHTXD_512(C,D);\
	G+=E;\
	H+=F;

/* Round function
 * Input:
 *	A,B,C,D,E,F,G,H: state, as 8 64 bit words
 *	k0,k1,k2,k3,k4,k5: round key, as 6 64 bit words.
 * Output:
 *	A,B,C,D,E,F,G,H: state, modified by the S-Box
 */
#define ROUND_512(A,B,C,D,E,F,G,H,k0,k1,k2,k3,k4,k5) \
    A^=k0;\
	B^=k1;\
	C^=k2;\
	D^=k3;\
	E^=k4;\
    F^=k5;\
	G^=C;\
	H^=D;\
	PHTXD_512(G,H);\
    Sbox_512(A,C,E) ;\
	Sbox_512(B,D,F) ;\
    DIFFUSION_512(A,B,C,D,E,F,G,H);

/* Calls the round function 4 times, rotating the words of the state,
 * and changing the round key.
 * Input:
 *	k0,k1,k2,k3,k4,k5,k6,k7,k8,k9,k10,k11,k12,k13,k14,k15,k16,k17,k18,k19,k20,k21,k22,k23: key material
 *	h0,h1,h2,h3,h4,h5,h6,h7: 512 bits state
 * Output:
 *	h0,h1,h2,h3,h4,h5,h6,h7: state, modified by the 4 round functions.
 */

#define FOURROUNDSBOX_512(k0,k1,k2,k3,k4,k5,k6,k7,k8,k9,k10,k11,k12,k13,k14,k15,k16,k17,k18,k19,k20,k21,k22,k23,h0,h1,h2,h3,h4,h5,h6,h7)\
	ROUND_512(h0,h1,h2,h3,h4,h5,h6,h7, k0, k1, k2, k3, k4, k5);\
	ROUND_512(h2,h3,h4,h5,h6,h7,h0,h1, k6, k7, k8, k9,k10,k11);\
	ROUND_512(h4,h5,h6,h7,h0,h1,h2,h3,k12,k13,k14,k15,k16,k17);\
	ROUND_512(h6,h7,h0,h1,h2,h3,h4,h5,k18,k19,k20,k21,k22,k23);

/* First four-round group, using the expanded W and the block k */
#define INPUTONE_512\
	FOURROUNDSBOX_512(W[0],W[1],k[0],k[1],W[2],W[3],W[4],W[5],k[2],k[3],W[6],W[7],W[8],W[9],k[4],k[5],W[10],W[11],W[12],W[13],k[6],k[7],W[14],W[15],hsh[0],hsh[1],hsh[2],hsh[3],hsh[4],hsh[5],hsh[6],hsh[7]);

/* Second four-round group, using the expanded W and the remaining words of block k  */
#define INPUTTWO_512\
	FOURROUNDSBOX_512(W[16],W[17],k[8],k[9],W[18],W[19],W[20],W[21],k[10],k[11],W[22],W[23],W[24],W[25],k[12],k[13],W[26],W[27],W[28],W[29],k[14],k[15],W[30],W[31],hsh[0],hsh[1],hsh[2],hsh[3],hsh[4],hsh[5],hsh[6],hsh[7]);

/* Third four-round group, using the expanded W and all the words of the previous block pk  */
#define INPUTTHREE_512\
	FOURROUNDSBOX_512(pk[0],pk[1],W[32],W[33],pk[2],pk[3],pk[4],pk[5],W[34],W[35],pk[6],pk[7],pk[8],pk[9],W[36],W[37],pk[10],pk[11],pk[12],pk[13],W[38],W[39],pk[14],pk[15],hsh[0],hsh[1],hsh[2],hsh[3],hsh[4],hsh[5],hsh[6],hsh[7]);

/* Fourth four-round group, using the remaining words of the expanded W  */
#define INPUTFOUR_512\
	FOURROUNDSBOX_512(W[40],W[41],W[42],W[43],W[44],W[45],W[46],W[47],W[48],W[49],W[50],W[51],W[52],W[53],W[54],W[55],W[56],W[57],W[58],W[59],W[60],W[61],W[62],W[63],hsh[0],hsh[1],hsh[2],hsh[3],hsh[4],hsh[5],hsh[6],hsh[7]);


/* Encrypt function. Encrypts the state, using the block, the previous block 
 * and the bits processed as key material. Modifies the state in place.
 * Input:
 *	hsh: Array of 8 64 bit words, storing the state.
 *	k: Array of 16 64 bit words, storing the current block.
 *	pk: Array of 16 64 bit words, storing the previous block.
 *	bits: 64 bit word representing the bits processed, up to and including the current block.
 * Output:
 *	hsh: the state, encrypted.
 */
static
void Encrypt512(uint64_t hsh[8], uint64_t k[16], uint64_t pk[16],
		uint64_t bits)
{
	/* Start  */

	uint64_t W[64];
	int i;

	/*expansion*/
	W[0] = pk[0] ^ k[0];
	for (i = 15; i; i--)
		W[i] = pk[i] ^ k[i];

	EXPANSION_512(W,pk,bits);

	INPUTONE_512;

	INPUTTWO_512;

	INPUTTHREE_512;

	INPUTFOUR_512;

}

/* Updates the bits processed before calling Transform. 
 * The increment is BLOCK_BITS_256 in all the blocks except the last.
 */
#define UPDATEBITS_256   state->bits_processed += 512;

/* Bitmask for zeroing the unused bits of the last byte of the message */
static
const unsigned char BITMASK[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
/* Bitmask for adding a 1 after the last bit of the message */
static
const unsigned char BITPADDING[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

/* IV for the 256 bit version */
static
const uint64_t i256[STATE_DWORDS_256] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL
};

/* IV for the 224 bit version */
static
const uint64_t  i224[STATE_DWORDS_256] = {
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
};

/* Process the block stored in the buffer. 
 * Parameters:
 *	state: a structure that holds the hashState256 information
 */
static
void Transform256(hashState256* state)
{
	uint64_t temp[STATE_DWORDS_256];  /* Temporal state, so we can apply the final xor */
	uint64_t* swap_aux;
	int i;
	/* We copy the state to the temp state, and call Encrypt256 */
	memcpy(temp,state->state,STATE_BYTES_256);
	Encrypt256(temp, state->data_block, state->previous_block,state->bits_processed);

	/* Now we perform the xor between the words of the original state,
	 * and the output of Encrypt256
	 */
	for (i = 0; i < STATE_DWORDS_256; i++)
		state->state[i] ^= temp[i];

	/* We swap the pointers, so the current data_block becomes the previous_block */
	swap_aux = state->data_block;
	state->data_block = state->previous_block;
	state->previous_block = swap_aux; /* now previous_block points to the block just processed */
}

/* Process the supplied data.
 * Parameters:
 *	state: a structure that holds the hashState256 information
 *	data: the data to be hashed
 *	databitlen: the length, in bits, of the data to be hashed. 
 *	It must be a multiple of 8, except, possibly, in the last call
 * Returns:
 *	SUCCESS - on success
 */
static
HashReturn Update256(hashState256 *state, const BitSequence *data,
		DataLength databitlen)
{
	DataLength offset, databytes;
	unsigned int remaining, index;
	BitSequence* byte_buffer;
	databytes = (databitlen >> 3); /* number of bytes to process */

	/*bytes waiting to be processed
	 * According to specifications, except for the last time, update will never be called with
	 * a number of bits not multiple of 8. Therefore, the number of bits that have been
	 * fed into the buffer but not processed after the last call of update must be
	 * a multiple of 8. Also, this number should be less than BLOCK_BYTES_256
	 */
	index = (state->bits_waiting_for_process >> 3);

	remaining = BLOCK_BYTES_256 - index; /* space available in the buffer */
	offset = 0;

	/* If we have bytes waiting from a previous call, and there are enough
	 * to fill a block, we copy them and call to Transform
	 */
	if ((index != 0) && (databytes >= remaining)) {
		byte_buffer = (BitSequence*) state->data_block; /* buffer as an array of char */
		memcpy(byte_buffer + index, data, remaining); /* We fill the buffer */
		UPDATEBITS_256;
		Transform256(state);
		offset += remaining; /* We consumed 'remaining' bytes from data */
		index = 0; /* There are not bytes waiting for process */
		state->bits_waiting_for_process = 0; /* Therefore, no bits */
	}

	/* We process as many blocks as we can */
	while (offset + BLOCK_BYTES_256 <= databytes) {
		memcpy(state->data_block, data + offset, BLOCK_BYTES_256);
		UPDATEBITS_256;
		Transform256(state);
		offset += BLOCK_BYTES_256;
	}
	if (databitlen & 7)
		databytes++; /* The number of bits is not a multiple of 8. This can only happen in the last call to update */
	byte_buffer = (BitSequence*) state->data_block;

	/* If there are still bytes in data, we copy them to the buffer */
	memcpy(byte_buffer + index, data + offset,(size_t) (databytes - offset));
	state->bits_waiting_for_process += (uint32_t) (databitlen - 8 * offset);;
	return SUCCESS;
}

/* Perform post processing and output filtering and return the final hash value.
 * Parameters:
 *	state: a structure that holds the hashState256 information
 *	hashval: the storage for the final hash value to be returned
 *	hashbytes: the number of bytes to store in hashval
 *		(28 for the 224 bit hash, 32 for the 256 bit hash)
 * Returns:
 *	SUCCESS - on success
 */
static
HashReturn _Final256 (hashState256 *state, BitSequence *hashval, int hashbytes)
{
	int j;
	BitSequence* byte_buffer;
	unsigned int index, bits;

	/* if there is any data to be processed, we need to pad and process, 
	 * otherwise we	go to the last block*/
	if(state->bits_waiting_for_process){
		/* If bits_waiting_for_process is not a multiple of 8, index will be the last byte,
		 * and bits will be the number of valid bits in that byte.
		 */
		index = state->bits_waiting_for_process >> 3;
		bits =  state->bits_waiting_for_process & 7;
		byte_buffer = (BitSequence*) state->data_block;

		/* We zero the unused bits of the last byte, and set the fist unused bit to 1 */
		byte_buffer[index] = (byte_buffer[index] & BITMASK[bits]) | BITPADDING[bits];
		index++;
		memset(byte_buffer+index, 0, BLOCK_BYTES_256 - index); /* We fill the rest of the block with zeros */

		/*the usual update of bits processed cannot be used here, since
		 *less than a full block of DATA has been processed
		 */
		state->bits_processed += state->bits_waiting_for_process; 

		Transform256(state);
	}/*endif(bits_waiting_for_process)*/
   
	/* All data (plus any padding) has been processed. A last call, so that the last block
	 * of data is treated like all the others (i.e., twice) is made. The new block is simply the number of
	 * bits processed, xor the last state. 
	 */
	state->data_block[0]=state->bits_processed;
	for(j=1;j < STATE_DWORDS_256;j++) state->data_block[j]=0; /* The size of the block is twice the size of the state */
	memcpy(state->data_block + STATE_DWORDS_256, state->state, STATE_BYTES_256);
	state->bits_processed = 0; /* This ensures that the final transform is the same regardless of the bits processed */
	
	Transform256(state);
	
	/* We output the appropriate number of bytes, truncating the state if necessary */
	for (j = 0; j < hashbytes; j++)
		hashval[j] = ((BitSequence*)state->state)[j];
	return SUCCESS;
}

/* Updates the bits processed before calling Transform. 
 * The increment is BLOCK_BITS_512 in all the blocks except the last.
 */
#define UPDATEBITS_512   state->bits_processed += 1024;

/* IV for the 512 bit version */
static
const uint64_t  i512[STATE_DWORDS_512] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

/* IV for the 384 bit version */
static
const uint64_t  i384[STATE_DWORDS_512] = {
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL
};

/* Process the block stored in the buffer. 
 * Parameters:
 *	state: a structure that holds the hashState512 information
 */
static
void Transform512(hashState512* state)
{
	uint64_t temp[STATE_DWORDS_512];  /* Temporal state, so we can apply the final xor */
	uint64_t* swap_aux;
	int i;
	/* We copy the state to the temp state, and call Encrypt512 */
	memcpy(temp,state->state,STATE_BYTES_512);
	Encrypt512(temp, state->data_block, state->previous_block,state->bits_processed);

	/* Now we perform the xor between the words of the original state,
	 * and the output of Encrypt512
	 */
	for (i = 0; i < STATE_DWORDS_512; i++)
		state->state[i] ^= temp[i];

	/* We swap the pointers, so the current data_block becomes the previous_block */
	swap_aux = state->data_block;
	state->data_block = state->previous_block;
	state->previous_block = swap_aux; /* now previous_block points to the block just processed */
}

/* Process the supplied data.
 * Parameters:
 *	state: a structure that holds the hashState512 information
 *	data: the data to be hashed
 *	databitlen: the length, in bits, of the data to be hashed. 
 *	It must be a multiple of 8, except, possibly, in the last call
 * Returns:
 *	SUCCESS - on success
 */
static
HashReturn Update512(hashState512 *state, const BitSequence *data,
		DataLength databitlen)
{
	DataLength offset, databytes;
	unsigned int remaining, index;
	BitSequence* byte_buffer;
	databytes = (databitlen >> 3); /* number of bytes to process */

	/*bytes waiting to be processed
	 * According to specifications, except for the last time, update will never be called with
	 * a number of bits not multiple of 8. Therefore, the number of bits that have been
	 * fed into the buffer but not processed after the last call of update must be
	 * a multiple of 8. Also, this number should be less than BLOCK_BYTES_512
	 */
	index = (state->bits_waiting_for_process >> 3);

	remaining = BLOCK_BYTES_512 - index; /* space available in the buffer */
	offset = 0;

	/* If we have bytes waiting from a previous call, and there are enough
	 * to fill a block, we copy them and call to Transform
	 */
	if ((index != 0) && (databytes >= remaining)) {
		byte_buffer = (BitSequence*) state->data_block; /* buffer as an array of char */
		memcpy(byte_buffer + index, data, remaining); /* We fill the buffer */
		UPDATEBITS_512;
		Transform512(state);
		offset += remaining; /* We consumed 'remaining' bytes from data */
		index = 0; /* There are not bytes waiting for process */
		state->bits_waiting_for_process = 0; /* Therefore, no bits */
	}

	/* We process as many blocks as we can */
	while (offset + BLOCK_BYTES_512 <= databytes) {
		memcpy(state->data_block, data + offset, BLOCK_BYTES_512);
		UPDATEBITS_512;
		Transform512(state);
		offset += BLOCK_BYTES_512;
	}
	if (databitlen & 7)
		databytes++; /* The number of bits is not a multiple of 8. This can only happen in the last call to update */
	byte_buffer = (BitSequence*) state->data_block;

	/* If there are still bytes in data, we copy them to the buffer */
	memcpy(byte_buffer + index, data + offset,(size_t) (databytes - offset));
	state->bits_waiting_for_process += (uint32_t) (databitlen - 8 * offset);;
	return SUCCESS;
}

/* Perform post processing and output filtering and return the final hash value.
 * Parameters:
 *	state: a structure that holds the hashState512 information
 *	hashval: the storage for the final hash value to be returned
 *	hashbytes: the number of bytes to store in hashval
 *		(48 for the 384 bit hash, 64 for the 512 bit hash)
 * Returns:
 *	SUCCESS - on success
 */
static
HashReturn _Final512 (hashState512 *state, BitSequence *hashval, int hashbytes)
{
	int j;
	BitSequence* byte_buffer;
	unsigned int index, bits;

	/* if there is any data to be processed, we need to pad and process, 
	 * otherwise we	go to the last block*/
	if(state->bits_waiting_for_process){
		/* If bits_waiting_for_process is not a multiple of 8, index will be the last byte,
		 * and bits will be the number of valid bits in that byte.
		 */
		index = state->bits_waiting_for_process >> 3;
		bits =  state->bits_waiting_for_process & 7;

		/* We zero the unused bits of the last byte, and set the fist unused bit to 1 */
		byte_buffer = (BitSequence*) state->data_block;
		byte_buffer[index] = (byte_buffer[index] & BITMASK[bits]) | BITPADDING[bits];
		index++;
		memset(byte_buffer+index, 0, BLOCK_BYTES_512 - index); /* We fill the rest of the block with zeros */

		/*the usual update of bits processed cannot be used here, since
		 *less than a full block of DATA has been processed
		 */	
		state->bits_processed += state->bits_waiting_for_process; 

		Transform512(state);
	}/*endif(bits_waiting_for_process)*/
   
	/* All data (plus any padding) has been processed. A last call, so that the last block
	 * of data is treated like all the others (i.e., twice) is made. The new block is simply the number of
	 * bits processed, xor the last state. 
	 */
	state->data_block[0]=state->bits_processed;
	for(j=1;j < STATE_DWORDS_512;j++) state->data_block[j]=0; /* The size of the block is twice the size of the state */
	memcpy(state->data_block + STATE_DWORDS_512, state->state, STATE_BYTES_512);
	state->bits_processed = 0; /* This ensures that the final transform is the same regardless of the bits processed */
	Transform512(state);
	
	/* We output the appropriate number of bytes, truncating the state if necessary */
	for (j = 0; j < hashbytes; j++)
		hashval[j] = ((BitSequence*)state->state)[j];
	return SUCCESS;
}

HashReturn Init(hashState *state, int hashbitlen)
{
    HashReturn ret = SUCCESS;

    state->hashbitlen = hashbitlen;

    switch (hashbitlen) {
    case 224:
    case 256:
    {	hashState256 *hs = state->uu->state256;
	const uint64_t * iv  = (hashbitlen == 224 ? i224 : i256);
	const uint64_t * oiv = (hashbitlen == 224 ? i256 : i224);
	memset(hs, 0, sizeof(*hs));
	hs->previous_block = hs->buffer;
	hs->data_block = hs->buffer+BLOCK_DWORDS_256;
	memcpy(hs->state, iv, STATE_BYTES_256);
	/* Use 2 copies of the other IV to initialize previous block. */
	memcpy(hs->previous_block, oiv, STATE_BYTES_256);
	memcpy(hs->previous_block+STATE_DWORDS_256, oiv, STATE_BYTES_256);
    }	break;
    case 384:
    case 512:
    {	hashState512 *hs = state->uu->state512;
	const uint64_t * iv  = (hashbitlen == 384 ? i384 : i512);
	const uint64_t * oiv = (hashbitlen == 384 ? i512 : i384);
	memset(hs, 0, sizeof(*hs));
	hs->previous_block = hs->buffer;
	hs->data_block = hs->buffer+BLOCK_DWORDS_512;
	memcpy(hs->state, iv, STATE_BYTES_512);
	/* Use 2 copies of the other IV to initialize previous block. */
	memcpy(hs->previous_block, oiv, STATE_BYTES_512);
	memcpy(hs->previous_block+STATE_DWORDS_512, oiv, STATE_BYTES_512);
    }	break;
    default:	ret = BAD_HASHBITLEN;	break;
    }
    return ret;
}

HashReturn Update(hashState *state , const BitSequence *data, DataLength databitlen)
{
    HashReturn ret;
    switch (state->hashbitlen){
    case 224:
    case 256:	ret =  Update256(state->uu->state256, data, databitlen);	break;
    case 384:
    case 512:	ret =  Update512(state->uu->state512, data, databitlen);	break;
    default:	ret =  BAD_HASHBITLEN;	break;
    }
    return ret;
}

HashReturn Final(hashState *state, BitSequence *hashval)
{
    HashReturn ret;
    switch (state->hashbitlen) {
    case 224:
    case 256:
	ret = _Final256(state->uu->state256, hashval, (state->hashbitlen/8));
	break;
    case 384:
    case 512:
	ret = _Final512(state->uu->state512, hashval, (state->hashbitlen/8));
	break;
    default:	ret = BAD_HASHBITLEN;	break;
    }
    return ret;
}

HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval)
{
    hashState state;
    HashReturn ret;

    if ((ret = Init(&state, hashbitlen)) != SUCCESS)
	return ret;
    if ((ret = Update(&state, data, databitlen)) != SUCCESS)
	return ret;
    return Final(&state, hashval);
}
