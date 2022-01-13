/*!\file salsa20.h
 * \brief SALSA-20 hash function.
 * \ingroup HASH_m HASH_salsa20_m 
 */

#ifndef  _SALSA20_H
#define  _SALSA20_H

#include <beecrypt/beecrypt.h>

/*!\brief Holds all the parameters necessary for the SALSA-20 algorithm.
 * \ingroup HASH_salsa20_h
 */
typedef struct
{
        /*!\var digest
         */
        uint32_t h[16];
	/*!\var data
	 */
	uint32_t data[16];
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
} salsa20Param;

#ifdef __cplusplus
extern "C" {
#endif

/*!\var salsa20
 * \brief Holds the full API description of the SALSA-20 algorithm.
 */
/*@unchecked@*/ /*@observer@*/
extern BEECRYPTAPI const hashFunction salsa20;

/*!\fn int salsa20Reset(salsa20Param* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
void salsa20Process(salsa20Param* mp)
	/*@modifies mp @*/;

/*!\fn int salsa20Reset(salsa20Param* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success.
 */
BEECRYPTAPI
int salsa20Reset   (salsa20Param* mp)
	/*@modifies mp @*/;

/*!\fn int salsa20Update(salsa20Param* mp, const byte* data, size_t size)
 * \brief This function should be used to pass successive blocks of data
 *  to be hashed.
 * \param mp The hash function's parameter block.
 * \param data
 * \param size
 * \retval 0 on success.
 */
BEECRYPTAPI
int salsa20Update  (salsa20Param* mp, const byte* data, size_t size)
	/*@modifies mp @*/;

/*!\fn int salsa20Digest(salsa20Param* mp, byte* digest)
 * \brief This function finishes the current hash computation and copies
 *  the digest value into \a digest.
 * \param mp The hash function's parameter block.
 * \param digest The place to store the 20-byte digest.
 * \retval 0 on success.
 */
BEECRYPTAPI
int salsa20Digest  (salsa20Param* mp, byte* digest)
	/*@modifies mp, digest @*/;

#ifdef __cplusplus
}
#endif

#endif
