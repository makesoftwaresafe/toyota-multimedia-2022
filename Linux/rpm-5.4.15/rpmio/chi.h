/* chi.h: CHI hash */

/*
THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _CHI_H
#define _CHI_H

#include <stdint.h>
#include <beecrypt/beecrypt.h>

#define	_256_STATE_N	6	/* no. words in internal state (256 bit hash) */
#define	_512_STATE_N	9	/* no. words in internal state (512 bit hash) */

/*!\brief Holds all the parameters necessary for the CHI algorithm.
 * \ingroup HASH_chi_m
 */
#ifdef __cplusplus
struct BEECRYPTAPI chiParam
#else
struct _chiParam
#endif
{
    union {
	uint64_t small[_256_STATE_N];
	uint64_t large[_512_STATE_N];
	uint32_t small32[2*_256_STATE_N];
	uint32_t large32[2*_512_STATE_N];
    } hs_State;                     /* Contents of internal state             */

    int         hs_HashBitLen;      /* Length of the Hash.  Passed into Init. */
    int         hs_MessageLen;      /* Length of message block in bits.  This
                                     * is set in Init depending on hashbitlen
                                     * to _256_MSG_LENGTH for 224,256 bit hash
                                     * and _512_MSG_LENGTH for 384,512 bit
                                     * hash.
                                     */
    uint64_t    hs_TotalLenLow;     /* Total length of message in bits.  Does
                                     * not include length of padding.
                                     */
    uint64_t    hs_TotalLenHigh;    /* For exceedingly long messages.
                                     */
    uint32_t    hs_DataLen;         /* Number of unprocess bits in
                                     * hs_DataBuffer.
                                     */
    uint8_t hs_DataBuffer[128];     /* Buffer for accumulating message.       */
};

#ifndef __cplusplus
typedef struct _chiParam chiParam;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!\var chi256
 * \brief Holds the full API description of the CHI algorithm.
 */
extern BEECRYPTAPI const hashFunction chi256;

BEECRYPTAPI
int chiInit(chiParam* sp, int hashbitlen); 

BEECRYPTAPI
int chiReset(chiParam* sp);

BEECRYPTAPI
int chiUpdate(chiParam* sp, const byte *data, size_t size); 

BEECRYPTAPI
int chiDigest(chiParam* sp, byte *digest); 

#ifdef __cplusplus
}
#endif

#endif /* _CHI_H */
