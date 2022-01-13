/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*!\file ripemd256.h
 * \brief RIPEMD-256 hash function, headers.
 * \author Jeff Johnson <jbj@rpm5.org>
 * \author Bob Deblier <bob.deblier@telenet.be>
 * \ingroup HASH_m HASH_rmd256_m
 */

#ifndef _RIPEMD256_H
#define _RIPEMD256_H

#include "beecrypt/beecrypt.h"

/*!\brief Holds all the parameters necessary for the RIPEMD-128 algorithm.
 * \ingroup HASH_rmd256_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI ripemd256Param
#else
struct _ripemd256Param
#endif
{
	/*!\var h
	 */
	uint32_t h[8];
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
};

#ifndef __cplusplus
typedef struct _ripemd256Param ripemd256Param;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var ripemd256
 * \brief Holds the full API description of the RIPEMD-128 algorithm.
 */
extern BEECRYPTAPI const hashFunction ripemd256;

/*!\fn void ripemd256Process(ripemd256Param* mp)
 * \brief This function performs the core of the RIPEMD-128 hash algorithm; it
 *  processes a block of 64 bytes.
 * \param mp The hash function's parameter block.
 */
BEECRYPTAPI
void ripemd256Process(ripemd256Param* mp);

/*!\fn int ripemd256Reset(ripemd256Param* mp)
 * \brief This function resets the parameter block so that it's ready for a
 *  new hash.
 * \param mp The hash function's parameter block.
 * \retval 0 on success. 
 */
BEECRYPTAPI
int  ripemd256Reset  (ripemd256Param* mp);

/*!\fn int ripemd256Update(ripemd256Param* mp, const byte* data, size_t size)
 * \brief This function should be used to pass successive blocks of data 
 *  to be hashed.
 * \param mp The hash function's parameter block.
 * \param data
 * \param size
 * \retval 0 on success.
 */
BEECRYPTAPI
int  ripemd256Update (ripemd256Param* mp, const byte* data, size_t size);

/*!\fn int ripemd256Digest(ripemd256Param* mp, byte* digest)
 * \brief This function finishes the current hash computation and copies
 *  the digest value into \a digest.
 * \param mp The hash function's parameter block.
 * \param digest The place to store the 20-byte digest.
 * \retval 0 on success.
 */
BEECRYPTAPI
int  ripemd256Digest (ripemd256Param* mp, byte* digest);

#ifdef __cplusplus
}
#endif

#endif
