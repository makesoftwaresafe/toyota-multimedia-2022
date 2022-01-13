/*!\file md2.h
 * \brief MD2 hash function.
 * \ingroup HASH_m HASH_md2_m 
 */

#ifndef  _MD2_H
#define  _MD2_H

#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the MD2 algorithm.
 * \ingroup HASH_md2_h
 */
typedef struct
{
	/*!\var cksum
	 */
	byte chksum[16];
	/*!\var cksum
	 */
	byte X[48];
	/*!\var data
	 */
	byte buf[16];
	/*!\var length
	 * \brief Multi-precision integer counter for the bits that have been
	 *  processed so far.
	 */
	#if (MP_WBITS == 64)
	mpw length[1];
	#elif (MP_WBITS == 32)
	mpw length[2];
	#else
	# error
	#endif
    /*!\var offset
     * \brief Offset into \a data; points to the place where new data will be
     *  copied before it is processed.
     */
	uint32_t offset;
} md2Param;

#ifdef __cplusplus
extern "C" {
#endif

/*!\var md2
 * \brief Holds the full API description of the MD2 algorithm.
 */
/*@unchecked@*/ /*@observer@*/
extern BEECRYPTAPI const hashFunction md2;

/*!\fn int md2Reset(md2Param* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
void md2Process(md2Param* mp)
	/*@modifies mp @*/;

/*!\fn int md2Reset(md2Param* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
int md2Reset   (md2Param* mp)
	/*@modifies mp @*/;

/*!\fn int md2Update(md2Param* mp, const byte* data, size_t size)
 * \brief This function should be used to pass successive blocks of data
 *  to be hashed.
 * \param mp The hash function's parameter block.
 * \param data
 * \param size
 * \retval 0 on success.
 */
BEECRYPTAPI
int md2Update  (md2Param* mp, const byte* data, size_t size)
	/*@modifies mp @*/;

/*!\fn int md2Digest(md2Param* mp, byte* digest)
 * \brief This function finishes the current hash computation and copies
 *  the digest value into \a digest.
 * \param mp The hash function's parameter block.
 * \param digest The place to store the 16-byte digest.
 * \retval 0 on success.
 */
BEECRYPTAPI
int md2Digest  (md2Param* mp, byte* digest)
	/*@modifies mp, digest @*/;

#ifdef __cplusplus
}
#endif

#endif
