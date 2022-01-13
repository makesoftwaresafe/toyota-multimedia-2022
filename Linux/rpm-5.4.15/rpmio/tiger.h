/*!\file tiger.h
 * \brief TIGER hash function.
 * \ingroup HASH_m HASH_tiger_m 
 */

#ifndef  _TIGER_H
#define  _TIGER_H

#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the TIGER algorithm.
 * \ingroup HASH_tiger_h
 */
typedef struct
{
	/*!\var cksum
	 */
	uint64_t h[3];
	/*!\var buf
	 */
	uint64_t data[8];
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
} tigerParam;

#ifdef __cplusplus
extern "C" {
#endif

/*!\var tiger
 * \brief Holds the full API description of the TIGER algorithm.
 */
/*@unchecked@*/ /*@observer@*/
extern BEECRYPTAPI const hashFunction tiger;

/*!\fn int tigerReset(tigerParam* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
void tigerProcess(tigerParam* mp)
	/*@modifies mp @*/;

/*!\fn int tigerReset(tigerParam* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
int tigerReset   (tigerParam* mp)
	/*@modifies mp @*/;

/*!\fn int tigerUpdate(tigerParam* mp, const byte* data, size_t size)
 * \brief This function should be used to pass successive blocks of data
 *  to be hashed.
 * \param mp The hash function's parameter block.
 * \param data
 * \param size
 * \retval 0 on success.
 */
BEECRYPTAPI
int tigerUpdate  (tigerParam* mp, const byte* data, size_t size)
	/*@modifies mp @*/;

/*!\fn int tigerDigest(tigerParam* mp, byte* digest)
 * \brief This function finishes the current hash computation and copies
 *  the digest value into \a digest.
 * \param mp The hash function's parameter block.
 * \param digest The place to store the 16-byte digest.
 * \retval 0 on success.
 */
BEECRYPTAPI
int tigerDigest  (tigerParam* mp, byte* digest)
	/*@modifies mp, digest @*/;

#ifdef __cplusplus
}
#endif

#endif
