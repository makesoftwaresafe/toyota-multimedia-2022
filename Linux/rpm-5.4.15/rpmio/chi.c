/* chi.c: CHI hash main implementation */

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

#include "chi.h"

const hashFunction chi256 = {
    .name = "CHI-256",
    .paramsize = sizeof(chiParam),
    .blocksize = 64,
    .digestsize = 256/8,	/* XXX default to CHI-256 */
    .reset = (hashFunctionReset) chiReset,
    .update = (hashFunctionUpdate) chiUpdate,
    .digest = (hashFunctionDigest) chiDigest
};

/*
 * Error code for external Interface
 */
enum {
    SUCCESS       = 0,
    FAIL          = 1,
    BAD_HASHLEN   = 2,
    BAD_STATE     = 3,
    BAD_DATA      = 4,
    TOO_MUCH_DATA = 5,
    BAD_HASHVALPTR
};
/*
 * BAD_STATE            State either NULL or corrupted.
 * BAD_DATA             Either NULL passed for data or Update called with
 *                      a partial byte, then called again with more data.
 * TOO_MUCH_DATA        Very unlikely case that too much data was passed to
 *                      hash function.
 * BAD_HASHVALPTR       Pointer for returning hash value invalid.
 */

#if !defined(INLINE)
#define	INLINE	inline
#endif

/*
 * Fundamental algorithm constants.
 */
#define _256_STEPS      20      /* Steps in 256 bit hash.                     */
#define _512_STEPS      40      /* Steps in 512 bit hash.                     */
#define _256_MSG_N      (_256_MSG_BLK_LEN / 64)
                                /* The length in 64 bit ints of the message
                                 * block in bits for the 256 bit hash.
                                 */
#define _512_MSG_N      (_512_MSG_BLK_LEN / 64)
                                /* The length in 64 bit ints of the message
                                 * block in bits for the 256 bit hash.
                                 */

/*
 * Valid message block lengths.
 * */
typedef enum {
    _256_MSG_BLK_LEN = 512,
    _512_MSG_BLK_LEN = 1024
} MsgBlkLen;
/*
 * _256_MSG_BLK_LEN     The length in bits of the message block for the 256
 *                      bit hash.
 * _512_MSG_BLK_LEN     The length in bits of the message block for the 512
 *                      bit hash.
 */

/*
 * Valid hash bit lengths.
 */
typedef enum {
    _224_HASH_BIT_LEN = 224,
    _256_HASH_BIT_LEN = 256,
    _384_HASH_BIT_LEN = 384,
    _512_HASH_BIT_LEN = 512
} HashBitLen;

/*
 * Standard MIN macro
 */
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

/*
 * Step constants
 */
static const uint64_t K[] = {
    0x9B05688C2B3E6C1FULL, 0xDBD99E6FF3C90BDCULL,
    0x4DBC64712A5BB168ULL, 0x767E27C3CF76C8E7ULL,
    0x21EE9AC5EF4C823AULL, 0xB36CCFC1204A9BD8ULL,
    0x754C8A7FB36BD941ULL, 0xCF20868F04A825E9ULL,
    0xABB379E0A8838BB0ULL, 0x12D8B70A5959E391ULL,
    0xB59168FA9F69E181ULL, 0x15C7D4918739F18FULL,
    0x4B547673EA8D68E0ULL, 0x3CED7326FCD0EF81ULL,
    0x09F1D77309998460ULL, 0x1AF3937415E91F32ULL,
    0x1F83D9ABFB41BD6BULL, 0x23C4654C2A217583ULL,
    0x2842012131573F2AULL, 0xA59916ACA3991FCAULL,
    0xEC1B7C06FD19D256ULL, 0x1EC785BCDC8BAF26ULL,
    0x69CA4E0FF2E6BDD8ULL, 0xCA2575EE6C950D0EULL,
    0x5BCF66D2FB3D99F6ULL, 0x9D6D08C7BBCA18F3ULL,
    0xEEF64039F2175E83ULL, 0x00ED5AEBAA2AB6E0ULL,
    0x5040712FC29AD308ULL, 0x6DAFE433438D2C43ULL,
    0xBD7FAA3F06C71F15ULL, 0x03B5AA8CE9B6A4DDULL,
    0x5BE0CD19137E2179ULL, 0x867F5E3B72221265ULL,
    0x43B6CBE0D67F4A20ULL, 0xDB99D767CB0E4933ULL,
    0xDC450DBC469248BDULL, 0xFE1E5E4876100D6FULL,
    0xB799D29EA1733137ULL, 0x16EA7ABCF92053C4ULL,
    0xBE3ECE968DBA92ACULL, 0x18538F84D82C318BULL,
    0x38D79F4E9C8A18C0ULL, 0xA8BBC28F1271F1F7ULL,
    0x2796E71067D2C8CCULL, 0xDE1BF2334EDB3FF6ULL,
    0xB094D782A857F9C1ULL, 0x639B484B0C1DAED1ULL,
    0xCBBB9D5DC1059ED8ULL, 0xE7730EAFF25E24A3ULL,
    0xF367F2FC266A0373ULL, 0xFE7A4D34486D08AEULL,
    0xD41670A136851F32ULL, 0x663914B66B4B3C23ULL,
    0x1B9E3D7740A60887ULL, 0x63C11D86D446CB1CULL,
    0xD167D2469049D628ULL, 0xDDDBB606B9A49E38ULL,
    0x557F1C8BEE68A7F7ULL, 0xF99DC58B50F924BDULL,
    0x205ACC9F653512A5ULL, 0x67C66344E4BAB193ULL,
    0x18026E467960D0C8ULL, 0xA2F5D84DAECA8980ULL,
    0x629A292A367CD507ULL, 0x98E67012D90CBB6DULL,
    0xEED758D1D18C7E35ULL, 0x031C02E4437DC71EULL,
    0x79B63D6482198EB7ULL, 0x936A9D7E8C9E4B33ULL,
    0xB30CA682C3E6C65DULL, 0xCC442382BA4262FAULL,
    0x51179BA5A1D37FF6ULL, 0x7202BDE7A98EEA51ULL,
    0x2B9F65D1DF9C610FULL, 0xF56B742B0AF1CE83ULL,
    0xF9989D199B75848BULL, 0xD142F19D8B46D578ULL,
    0x7A7580514D75EA33ULL, 0xB74F9690808E704DULL
};

/*
 * Initial state values for different bit lengths
 */
static const uint64_t _224_init[] = {
    0xA54FF53A5F1D36F1ULL, 0xCEA7E61FC37A20D5ULL,
    0x4A77FE7B78415DFCULL, 0x8E34A6FE8E2DF92AULL,
    0x4E5B408C9C97D4D8ULL, 0x24A05EEE29922401ULL
};

static const uint64_t _256_init[] = {
    0x510E527FADE682D1ULL, 0xDE49E330E42B4CBBULL,
    0x29BA5A455316E0C6ULL, 0x5507CD18E9E51E69ULL,
    0x4F9B11C81009A030ULL, 0xE3D3775F155385C6ULL
};

static const uint64_t _384_init[] = {
    0xA54FF53A5F1D36F1ULL, 0xCEA7E61FC37A20D5ULL,
    0x4A77FE7B78415DFCULL, 0x8E34A6FE8E2DF92AULL,
    0x4E5B408C9C97D4D8ULL, 0x24A05EEE29922401ULL,
    0x5A8176CFFC7C2224ULL, 0xC3EDEBDA29BEC4C8ULL,
    0x8A074C0F4D999610ULL
};

static const uint64_t _512_init[] = {
    0x510E527FADE682D1ULL, 0xDE49E330E42B4CBBULL,
    0x29BA5A455316E0C6ULL, 0x5507CD18E9E51E69ULL,
    0x4F9B11C81009A030ULL, 0xE3D3775F155385C6ULL,
    0x489221632788FB30ULL, 0x41921DB8FEEB38C2ULL,
    0x9AF94A7C48BBD5B6ULL
};

/*
 * Regardless of endian-ness converts a byte to / from a big endian array of
 * bytes.
 */
#define BYTE2WORD(b) (                          \
          (((uint64_t)(b)[0] & 0xFF) << 56)       \
        | (((uint64_t)(b)[1] & 0xFF) << 48)       \
        | (((uint64_t)(b)[2] & 0xFF) << 40)       \
        | (((uint64_t)(b)[3] & 0xFF) << 32)       \
        | (((uint64_t)(b)[4] & 0xFF) << 24)       \
        | (((uint64_t)(b)[5] & 0xFF) << 16)       \
        | (((uint64_t)(b)[6] & 0xFF) << 8)        \
        | (((uint64_t)(b)[7] & 0xFF))             \
    )

/*
 * Rotate uint32_t to the right by r
 */
static INLINE
uint32_t rotr32(uint32_t X, int r)
{
    return (X >> r) | (X << (32 - r));
}

/*
 * Rotate uint64_t to the right by r
 */
static INLINE
uint64_t rotr64(uint64_t X, int r)
{
    return (X >> r) | (X << (64 - r));
}

/*
 * Rotate Upper and Lower 32 bits of uint64_t to
 * the right by r1 and r2 respectively.
 */
static INLINE
uint64_t drotr32(uint64_t X, int r1, int r2)
{
    return (uint64_t)rotr32((uint32_t)(X >> 32), r1) << 32 | rotr32((uint32_t)X, r2);
}

/*
 * Byte swap
 */
static INLINE
uint64_t swap8(uint64_t X)
{
    X = (X & 0xFFFFFFFF00000000ULL) >> 32 | (X & 0x00000000FFFFFFFFULL) << 32;
    X = (X & 0xFFFF0000FFFF0000ULL) >> 16 | (X & 0x0000FFFF0000FFFFULL) << 16;
    X = (X & 0xFF00FF00FF00FF00ULL) >>  8 | (X & 0x00FF00FF00FF00FFULL) <<  8;

    return X;
}

/*
 * Swap the Upper and Lower 32 bits of uint64_t
 */
static INLINE
uint64_t swap32(uint64_t X)
{
    return rotr64(X, 32);
}

static INLINE
uint64_t map0(uint64_t R, uint64_t S, uint64_t T, uint64_t U)
{
    return    ( R & ~S &  T &  U)
            | (     ~S & ~T & ~U)
            | (~R      & ~T &  U)
            | (~R &  S &  T     )
            | ( R      & ~T & ~U);
}

static INLINE
uint64_t map1(uint64_t R, uint64_t S, uint64_t T, uint64_t U)
{
    return    ( R &  S &  T & ~U)
            | ( R & ~S & ~T & ~U)
            | (~R & ~S &  T     )
            | (      S & ~T &  U)
            | (~R           &  U);
}

static INLINE
uint64_t map2(uint64_t R, uint64_t S, uint64_t T, uint64_t U)
{
    return    (~R & ~S &  T &  U)
            | (~R &  S &  T & ~U)
            | (      S & ~T &  U)
            | ( R      & ~T & ~U)
            | ( R &  S      &  U)
            | ( R & ~S      & ~U);
}

/*
 * Non-linear MAP function.
 */
static INLINE
void map(uint64_t R, uint64_t S, uint64_t T, uint64_t U, uint64_t *X)
{
    uint64_t  i = (T | U) ^ R;
    X[0] = (i &  S) | ((~T ^ (U & R)) & ~S);
    X[1] = (i & ~S) | (( U ^ (R & T)) &  S);
    X[2] = ( U & (S ^ (T & ~R))) | (~U & (R ^ (T &  S)));
}

/*
 * CHI-256
 */
#define _256_MAP map

static INLINE
uint64_t _256_theta0(uint64_t X)
{
    uint32_t U = (uint32_t)(X >> 32);
    uint32_t L = (uint32_t)X;
    U = rotr32(U, 21) ^ rotr32(U, 26) ^ rotr32(U, 30);
    L = rotr32(L, 21) ^ rotr32(L, 26) ^ rotr32(L, 30);

    return (uint64_t)U << 32 | L;
}

static INLINE
uint64_t _256_theta1(uint64_t X)
{
    uint32_t U = (uint32_t)(X >> 32);
    uint32_t L = (uint32_t)X;
    U = rotr32(U, 14) ^ rotr32(U, 24) ^ rotr32(U, 31);
    L = rotr32(L, 14) ^ rotr32(L, 24) ^ rotr32(L, 31);

    return (uint64_t)U << 32 | L;
}

static INLINE
uint64_t _256_mu0(uint64_t X)
{
    return rotr64(X, 36) ^ rotr64(X, 18) ^ (X >> 1);
}

static INLINE
uint64_t _256_mu1(uint64_t X)
{
    return rotr64(X, 59) ^ rotr64(X, 37) ^ (X >> 10);
}

static INLINE
void _256_premix(uint64_t A, uint64_t B, uint64_t D, uint64_t E,
		uint64_t *R, uint64_t *S, uint64_t *T, uint64_t *U)
{
    uint64_t dswap;

    *T = drotr32(swap32(A), 7, 26) ^ drotr32(D, 14, 22);
    dswap = swap32(D);
    *R = drotr32(B, 8, 8)          ^ drotr32(dswap, 5, 1);
    *S = A                         ^ drotr32(dswap, 18, 17);
    *U = drotr32(A, 17, 12)        ^ drotr32(swap32(E), 2, 23);
}

static INLINE
void _256_datainput(uint64_t V0, uint64_t V1,
		uint64_t *R, uint64_t *S, uint64_t *T, uint64_t *U)
{
    *R ^= V0;
    *S ^= V1;
    *T ^= _256_theta0(V0);
    *U ^= _256_theta1(V1);
}


static INLINE
void _256_premix_and_di(uint64_t A, uint64_t B, uint64_t D, uint64_t E,
		uint64_t V0, uint64_t V1,
		uint64_t *R, uint64_t *S, uint64_t *T, uint64_t *U)
{
    uint64_t dswap;

    *T = drotr32(D, 14, 22) ^ drotr32(swap32(A), 7, 26) ^ _256_theta0(V0);
    dswap = swap32(D);
    *R = drotr32(B,  8,  8) ^ drotr32(dswap,     5,  1) ^ V0;
    *S = A                  ^ drotr32(dswap,    18, 17) ^ V1;
    *U = drotr32(A, 17, 12) ^ drotr32(swap32(E), 2, 23) ^ _256_theta1(V1);
}

static INLINE
void _256_postmix(uint64_t X, uint64_t Y, uint64_t Z, uint64_t *F, uint64_t *C)
{
    uint64_t  YY;

    YY = swap8(drotr32(Y, 5, 5));
    *F ^= rotr64(X + YY, 16);
    *C ^= rotr64(swap32(Z) + YY, 48);
}

#define _256_STEP(A, B, C, D, E, F, i)                                         \
    do {                                                                       \
        _256_premix(A, B, D, E, &R, &S, &T, &U);                               \
        _256_datainput(W[2*(i)]^K[2*(i)], W[2*(i)+1]^K[2*(i)+1], &R,&S,&T,&U); \
        _256_MAP(R, S, T, U, M);                                               \
        _256_postmix(M[(i)%3], M[((i)+1)%3], M[((i)+2)%3], &F, &C);            \
    } while(0)

#define _256_STEP_2(A, B, C, D, E, F, i)                                       \
    do {                                                                       \
        _256_STEP(A, B, C, D, E, F, (i) + 0);                                  \
        _256_STEP(F, A, B, C, D, E, (i) + 1);                                  \
    } while(0)

#define _256_STEP_6(A, B, C, D, E, F, i)                                       \
    do {                                                                       \
        _256_STEP(A, B, C, D, E, F, (i) + 0);                                  \
        _256_STEP(F, A, B, C, D, E, (i) + 1);                                  \
        _256_STEP(E, F, A, B, C, D, (i) + 2);                                  \
        _256_STEP(D, E, F, A, B, C, (i) + 3);                                  \
        _256_STEP(C, D, E, F, A, B, (i) + 4);                                  \
        _256_STEP(B, C, D, E, F, A, (i) + 5);                                  \
    } while(0)

#define _256_MSG(W, i) (W[(i)-8] ^ _256_mu0(W[(i)-7]) ^ _256_mu1(W[(i)-2]))

#define _256_MSG_EXP(W, b)                                                     \
    do {int i;                                                                 \
        W[0] = BYTE2WORD(b + 8*0);                                             \
        W[1] = BYTE2WORD(b + 8*1);                                             \
        W[2] = BYTE2WORD(b + 8*2);                                             \
        W[3] = BYTE2WORD(b + 8*3);                                             \
        W[4] = BYTE2WORD(b + 8*4);                                             \
        W[5] = BYTE2WORD(b + 8*5);                                             \
        W[6] = BYTE2WORD(b + 8*6);                                             \
        W[7] = BYTE2WORD(b + 8*7);                                             \
        for (i = 8; i < 2 * _256_STEPS; i++)                                   \
            W[i] = _256_MSG(W, i);                                             \
    } while(0)

/*
 * 256 bit update routine.  Called from hash().
 *
 * Parameters:
 *      sp      The hash state.
 *      final   1 for the final block in the hash, 0 for intermediate blocks.
 */
static
void _256_update(chiParam *sp, int final)
{
    uint64_t    W[2*_256_STEPS];

    uint64_t    A, B, C, D, E, F;
    uint64_t    R, S, T, U;
    uint64_t    M[3];

    if (final) {
        A = sp->hs_State.small[0] = rotr64(sp->hs_State.small[0], 1);
        B = sp->hs_State.small[1] = rotr64(sp->hs_State.small[1], 1);
        C = sp->hs_State.small[2] = rotr64(sp->hs_State.small[2], 1);
        D = sp->hs_State.small[3] = rotr64(sp->hs_State.small[3], 1);
        E = sp->hs_State.small[4] = rotr64(sp->hs_State.small[4], 1);
        F = sp->hs_State.small[5] = rotr64(sp->hs_State.small[5], 1);
    } else {
        sp->hs_State.small[0] = rotr64(A = sp->hs_State.small[0], 1);
        sp->hs_State.small[1] = rotr64(B = sp->hs_State.small[1], 1);
        sp->hs_State.small[2] = rotr64(C = sp->hs_State.small[2], 1);
        sp->hs_State.small[3] = rotr64(D = sp->hs_State.small[3], 1);
        sp->hs_State.small[4] = rotr64(E = sp->hs_State.small[4], 1);
        sp->hs_State.small[5] = rotr64(F = sp->hs_State.small[5], 1);
    }

    _256_MSG_EXP(W, sp->hs_DataBuffer);

    _256_STEP_6(A, B, C, D, E, F,  0);
    _256_STEP_6(A, B, C, D, E, F,  6);
    _256_STEP_6(A, B, C, D, E, F, 12);
    _256_STEP_2(A, B, C, D, E, F, 18);

    sp->hs_State.small[0] ^= E;
    sp->hs_State.small[1] ^= F;
    sp->hs_State.small[2] ^= A;
    sp->hs_State.small[3] ^= B;
    sp->hs_State.small[4] ^= C;
    sp->hs_State.small[5] ^= D;
}

/*
 * CHI-512
 */
#define _512_MAP map

static INLINE
uint64_t _512_theta0(uint64_t X)
{
    return rotr64(X,  5) ^ rotr64(X,  6) ^ rotr64(X, 43);
}

static INLINE
uint64_t _512_theta1(uint64_t X)
{
    return rotr64(X, 20) ^ rotr64(X, 30) ^ rotr64(X, 49);
}

static INLINE
uint64_t _512_mu0(uint64_t X)
{
    return rotr64(X, 36) ^ rotr64(X, 18) ^ (X >> 1);
}

static INLINE
uint64_t _512_mu1(uint64_t X)
{
    return rotr64(X, 60) ^ rotr64(X, 30) ^ (X >> 3);
}

static INLINE
void _512_premix(uint64_t A, uint64_t B, uint64_t D, uint64_t E,
		uint64_t G, uint64_t P,
		uint64_t *R, uint64_t *S, uint64_t *T, uint64_t *U)
{
    *T = P;
    *S = A;
    *T ^= rotr64(A, 11);
    *U = rotr64(A, 26);
    *R = rotr64(B, 11);
    *R ^= rotr64(D, 8);
    *S ^= rotr64(D, 21);
    *T ^= rotr64(D, 38);
    *U ^= rotr64(E, 40);
    *R ^= rotr64(G, 13);
    *S ^= rotr64(G, 29);
    *U ^= rotr64(G, 50);
}

static INLINE
void _512_datainput(uint64_t V0, uint64_t V1,
		uint64_t *R, uint64_t *S, uint64_t *T, uint64_t *U)
{
    *R ^= V0;
    *S ^= V1;
    *T ^= _512_theta0(V0);
    *U ^= _512_theta1(V1);
}

static INLINE
void _512_postmix(uint64_t X, uint64_t Y, uint64_t Z,
		uint64_t *Q, uint64_t *F, uint64_t *C)
{
    uint64_t  YY, ZZ;

    YY = swap8(rotr64(Y, 31));
    ZZ = swap32(Z);
    *Q ^= rotr64( X + YY, 16);
    *C ^= rotr64(ZZ + YY, 48);
    *F ^= ZZ + (X << 1);
}

#define _512_STEP(A, B, C, D, E, F, G, P, Q, i)                                \
    do {                                                                       \
        _512_premix(A, B, D, E, G, P, &R, &S, &T, &U);                         \
        _512_datainput(W[2*(i)]^K[2*(i)], W[2*(i)+1]^K[2*(i)+1],&R,&S,&T,&U);  \
        _512_MAP(R, S, T, U, M);                                               \
        _512_postmix(M[(i)%3], M[((i)+1)%3], M[((i)+2)%3], Q, F, C);           \
    } while(0)

#define _512_STEP_4(A, B, C, D, E, F, G, P, Q, i)                              \
    do {                                                                       \
        _512_STEP(A, B, &C, D, E, &F, G, P, &Q, (i) + 0);                      \
        _512_STEP(Q, A, &B, C, D, &E, F, G, &P, (i) + 1);                      \
        _512_STEP(P, Q, &A, B, C, &D, E, F, &G, (i) + 2);                      \
        _512_STEP(G, P, &Q, A, B, &C, D, E, &F, (i) + 3);                      \
    } while(0)

#define _512_STEP_9(A, B, C, D, E, F, G, P, Q, i)                              \
    do {                                                                       \
        _512_STEP(A, B, &C, D, E, &F, G, P, &Q, (i) + 0);                      \
        _512_STEP(Q, A, &B, C, D, &E, F, G, &P, (i) + 1);                      \
        _512_STEP(P, Q, &A, B, C, &D, E, F, &G, (i) + 2);                      \
        _512_STEP(G, P, &Q, A, B, &C, D, E, &F, (i) + 3);                      \
        _512_STEP(F, G, &P, Q, A, &B, C, D, &E, (i) + 4);                      \
        _512_STEP(E, F, &G, P, Q, &A, B, C, &D, (i) + 5);                      \
        _512_STEP(D, E, &F, G, P, &Q, A, B, &C, (i) + 6);                      \
        _512_STEP(C, D, &E, F, G, &P, Q, A, &B, (i) + 7);                      \
        _512_STEP(B, C, &D, E, F, &G, P, Q, &A, (i) + 8);                      \
    } while(0)

#define _512_MSG(W,i) (W[i-16] ^ W[i-7] ^ _512_mu0(W[i-15]) ^ _512_mu1(W[i-2]))

#define _512_MSG_EXP(W, b)                                                     \
    do {int i;                                                                 \
        W[0] = BYTE2WORD(b + 8*0);                                             \
        W[1] = BYTE2WORD(b + 8*1);                                             \
        W[2] = BYTE2WORD(b + 8*2);                                             \
        W[3] = BYTE2WORD(b + 8*3);                                             \
        W[4] = BYTE2WORD(b + 8*4);                                             \
        W[5] = BYTE2WORD(b + 8*5);                                             \
        W[6] = BYTE2WORD(b + 8*6);                                             \
        W[7] = BYTE2WORD(b + 8*7);                                             \
        W[8] = BYTE2WORD(b + 8*8);                                             \
        W[9] = BYTE2WORD(b + 8*9);                                             \
        W[10] = BYTE2WORD(b + 8*10);                                           \
        W[11] = BYTE2WORD(b + 8*11);                                           \
        W[12] = BYTE2WORD(b + 8*12);                                           \
        W[13] = BYTE2WORD(b + 8*13);                                           \
        W[14] = BYTE2WORD(b + 8*14);                                           \
        W[15] = BYTE2WORD(b + 8*15);                                           \
        for (i = 16; i < 2 * _512_STEPS; i++)                                  \
            W[i] = _512_MSG(W, i);                                             \
    } while(0)

/*
 * Parameters:
 *      sp      The hash state.
 *      final   1 for the final block in the hash, 0 for intermediate blocks.
 */
static
void _512_update(chiParam *sp, int final)
{
    uint64_t      W[2*_512_STEPS];
    uint64_t      A, B, C, D, E, F, G, P, Q;
    uint64_t      R, S, T, U;
    uint64_t      M[3];

    if (final) {
        A = sp->hs_State.large[0] = rotr64(sp->hs_State.large[0], 1);
        B = sp->hs_State.large[1] = rotr64(sp->hs_State.large[1], 1);
        C = sp->hs_State.large[2] = rotr64(sp->hs_State.large[2], 1);
        D = sp->hs_State.large[3] = rotr64(sp->hs_State.large[3], 1);
        E = sp->hs_State.large[4] = rotr64(sp->hs_State.large[4], 1);
        F = sp->hs_State.large[5] = rotr64(sp->hs_State.large[5], 1);
        G = sp->hs_State.large[6] = rotr64(sp->hs_State.large[6], 1);
        P = sp->hs_State.large[7] = rotr64(sp->hs_State.large[7], 1);
        Q = sp->hs_State.large[8] = rotr64(sp->hs_State.large[8], 1);
    } else {
        sp->hs_State.large[0] = rotr64(A = sp->hs_State.large[0], 1);
        sp->hs_State.large[1] = rotr64(B = sp->hs_State.large[1], 1);
        sp->hs_State.large[2] = rotr64(C = sp->hs_State.large[2], 1);
        sp->hs_State.large[3] = rotr64(D = sp->hs_State.large[3], 1);
        sp->hs_State.large[4] = rotr64(E = sp->hs_State.large[4], 1);
        sp->hs_State.large[5] = rotr64(F = sp->hs_State.large[5], 1);
        sp->hs_State.large[6] = rotr64(G = sp->hs_State.large[6], 1);
        sp->hs_State.large[7] = rotr64(P = sp->hs_State.large[7], 1);
        sp->hs_State.large[8] = rotr64(Q = sp->hs_State.large[8], 1);
    }

    _512_MSG_EXP(W, sp->hs_DataBuffer);

    _512_STEP_9(A, B, C, D, E, F, G, P, Q, 0);
    _512_STEP_9(A, B, C, D, E, F, G, P, Q, 9);
    _512_STEP_9(A, B, C, D, E, F, G, P, Q,18);
    _512_STEP_9(A, B, C, D, E, F, G, P, Q,27);
    _512_STEP_4(A, B, C, D, E, F, G, P, Q,36);

    sp->hs_State.large[0] ^= F;
    sp->hs_State.large[1] ^= G;
    sp->hs_State.large[2] ^= P;
    sp->hs_State.large[3] ^= Q;
    sp->hs_State.large[4] ^= A;
    sp->hs_State.large[5] ^= B;
    sp->hs_State.large[6] ^= C;
    sp->hs_State.large[7] ^= D;
    sp->hs_State.large[8] ^= E;
}

/* chi-common.c: CHI hash common functions */

/*
 * Byte i of 64 bit x.  i==0 returns top byte.
 */
#define BYTE(x, i) ((byte)(((x) >> (8 * (7 - i))) & 0xFF))

#define WORD2BYTE(w, b)         \
    do {                        \
        (b)[7] = BYTE(w, 7);    \
        (b)[6] = BYTE(w, 6);    \
        (b)[5] = BYTE(w, 5);    \
        (b)[4] = BYTE(w, 4);    \
        (b)[3] = BYTE(w, 3);    \
        (b)[2] = BYTE(w, 2);    \
        (b)[1] = BYTE(w, 1);    \
        (b)[0] = BYTE(w, 0);    \
    } while(0)

/*
 * Hashes the bytes in hs_DataBuffer.  Either calls te 256 or 512 hash.
 *
 * sp       The current state.
 * final    1 for final block in hash, 0 for intermediate blocks.
 */
static
void hash(chiParam *sp, int final)
{
    switch (sp->hs_HashBitLen) {
    case 224:
    case 256:
        _256_update(sp, final);
        break;
    case 384:
    case 512:
        _512_update(sp, final);
        break;
    }
}

/*
 * Small routine for incrementing total input data length by hs_DataLen.
 *
 * Parameters:
 *      sp          A structure that holds the chiParam information
 *
 * Returns:
 *      SUCCESS         - on success.
 *      TOO_MUCH_DATA   - on overflow.
 */
static INLINE
int inc_total_len(chiParam *sp)
{
    uint64_t    old_len;

    old_len = sp->hs_TotalLenLow;
    sp->hs_TotalLenLow += sp->hs_DataLen;
    if (old_len > sp->hs_TotalLenLow) {
        switch (sp->hs_HashBitLen) {
        case 224:
        case 256:
            return TOO_MUCH_DATA;
        case 384:
        case 512:
            if (++sp->hs_TotalLenHigh == 0)
                return TOO_MUCH_DATA;
        }
    }

    return SUCCESS;
}

/*
 * Initializes a chiParam with the intended hash length of this particular
 * instantiation.
 *
 * Additionally, any data independent setup is performed.
 *
 * Parameters:
 *     sp           A structure that holds the chiParam information
 *     hashbitlen   An integer value that indicates the length of the hash
 *                  output in bits.
 *
 * Returns:
 *     SUCCESS        - on success
 *     BAD_HASHBITLEN - hashbitlen is invalid (e.g., unknown value)
 *
 */
int chiInit(chiParam *sp, int hashbitlen)
{
    if (sp == NULL)
       return BAD_STATE;

    switch (hashbitlen) {
    default:
        return BAD_HASHLEN;
    case 224:
        memcpy(sp->hs_State.small, _224_init, _256_STATE_N * sizeof(uint64_t));
        sp->hs_MessageLen = _256_MSG_BLK_LEN;
        break;
    case 256:
        memcpy(sp->hs_State.small, _256_init, _256_STATE_N * sizeof(uint64_t));
        sp->hs_MessageLen = _256_MSG_BLK_LEN;
        break;
    case 384:
        memcpy(sp->hs_State.large, _384_init, _512_STATE_N * sizeof(uint64_t));
        sp->hs_MessageLen = _512_MSG_BLK_LEN;
        break;
    case 512:
        memcpy(sp->hs_State.large, _512_init, _512_STATE_N * sizeof(uint64_t));
        sp->hs_MessageLen = _512_MSG_BLK_LEN;
        break;
    }

    sp->hs_HashBitLen = (HashBitLen)hashbitlen;
    sp->hs_DataLen      = 0;
    sp->hs_TotalLenLow  = 0;
    sp->hs_TotalLenHigh = 0;
   
    return SUCCESS;
}

int
chiReset(chiParam *sp)
{
    return chiInit(sp, sp->hs_HashBitLen);
}

/*
 * Process the supplied data.
 *
 * Parameters:
 *    sp            A structure that holds the chiParam information
 *    data          The data to be hashed
 *    size          The length, in bytes, of the data to be hashed
 *
 * Returns:
 *    SUCCESS  - on success
 *    BAD_DATA - data is NULL, or call made to update and previous
 *               call passed in a partial byte.
 *
 */
int chiUpdate(chiParam *sp, const byte *data, size_t size)
{
    uint64_t databitlen = 8 * size;
    int          ret;
    uint32_t            cs;
    /*
     * cs       Amount of data copied out of data into hash block.
     */

    if (sp == NULL)
       return BAD_STATE;
    if (data == NULL)
        return BAD_DATA;

    /*
     * Handle partial bytes only if last call to update
     */
    if ((sp->hs_DataLen & 0x7) != 0)
        return BAD_DATA;

    while (databitlen > 0) {
        cs = MIN((uint32_t)databitlen, sp->hs_MessageLen - sp->hs_DataLen);

        memcpy(sp->hs_DataBuffer+sp->hs_DataLen / 8, data, (cs + 7) / 8);
        data += cs / 8;
        databitlen -= cs;
        sp->hs_DataLen += cs;

        if (sp->hs_DataLen >= (uint32_t)sp->hs_MessageLen) {
            /*
             * Hash hs_DataBuffer
             */
            hash(sp, 0);
            ret = inc_total_len(sp);
            if (ret != SUCCESS)
                return ret;
            sp->hs_DataLen = 0;
        }
    }

    return SUCCESS;
}

/*
 * Perform any post processing and output filtering required and return
 * the final hash value.
 *
 * Parameters:
 *     sp       A structure that holds the chiParam information
 *     digest   The storage for the final hash value to be returned
 *
 * Returns:
 *     SUCCESS - on success
 *
 */
int chiDigest(chiParam *sp, byte *digest)
{
    int  ret;
    uint32_t    whole_bytes;
    uint32_t    last_byte_bits;
    uint32_t    left_over_bytes;
    uint32_t    length_bytes;
    /*
     * whole_bytes      Number of whole bytes in last block with data in them.
     * last_byte_bits   Number of bits in last byte in last block with data in
     *                  them.
     * left_over_bytes  The number of bytes in the last message block that
     *                  have padding & the message length in them.
     * length_bytes     The number of bytes to use right at the end of the
     *                  message to store the message length.
     */

    if (sp == NULL)
       return BAD_STATE;
    if (digest == NULL)
       return BAD_HASHVALPTR;

    ret = inc_total_len(sp);
    if (ret != SUCCESS)
        return ret;

    switch (sp->hs_HashBitLen) {
    case 224:
    case 256:
        length_bytes = sizeof (uint64_t);
        break;

    default: /* Suppress compiler warning. */
        assert(0);
    case 384:
    case 512:
        length_bytes = sizeof (uint64_t) * 2;
        break;
    }

    whole_bytes = sp->hs_DataLen / 8;
    last_byte_bits = sp->hs_DataLen % 8;
   
    /*
     * Padding
     *
     * Append a 1 to end of buffer and pad byte with zeros.
     */
    sp->hs_DataBuffer[whole_bytes] &= ~((1 << (7-last_byte_bits)) - 1);
    sp->hs_DataBuffer[whole_bytes] |= (1 << (7-last_byte_bits));

    /*
     * Skip to end of byte
     */
    sp->hs_DataLen += (8 - last_byte_bits);

    left_over_bytes = (sp->hs_MessageLen - sp->hs_DataLen) / 8;

    if (left_over_bytes < length_bytes) {
        /*
         * length doesn't fit in the end of this block.  Pad this block to the
         * end using zeros then write the length in the next block.
         */
        memset(sp->hs_DataBuffer + sp->hs_DataLen/8, 0, left_over_bytes);
        hash(sp, 0);
        sp->hs_DataLen = 0;
        left_over_bytes = sp->hs_MessageLen / 8;
    }

    memset(sp->hs_DataBuffer + sp->hs_DataLen / 8, 0, left_over_bytes);

    /*
     * Write 64 bit or 128 bit message length.
     */
    whole_bytes = sp->hs_MessageLen / 8 - length_bytes;
    if (length_bytes == sizeof (uint64_t) * 2) {
        WORD2BYTE(sp->hs_TotalLenHigh, sp->hs_DataBuffer + whole_bytes);
        whole_bytes += sizeof (uint64_t);
    } else {
        assert(sp->hs_TotalLenHigh == 0);
        if (sp->hs_TotalLenHigh != 0)
            return BAD_STATE;
    }

    WORD2BYTE(sp->hs_TotalLenLow, sp->hs_DataBuffer + whole_bytes);
    sp->hs_DataLen = sp->hs_MessageLen;

    /*
     * Hash final block
     * */
    hash(sp, 1);
    sp->hs_DataLen = 0;

    /*
     * Write hash output into buffer in network byte order.
     */
    switch (sp->hs_HashBitLen) {
    case 224:
        WORD2BYTE(sp->hs_State.large[0],digest);
        WORD2BYTE(sp->hs_State.large[1],digest + 8);
        WORD2BYTE(sp->hs_State.large[3],digest + 16);
        digest[24] = BYTE(sp->hs_State.large[4], 0);
        digest[25] = BYTE(sp->hs_State.large[4], 1);
        digest[26] = BYTE(sp->hs_State.large[4], 2);
        digest[27] = BYTE(sp->hs_State.large[4], 3);
        break;
    case 256:
        WORD2BYTE(sp->hs_State.large[0],digest);
        WORD2BYTE(sp->hs_State.large[1],digest + 8);
        WORD2BYTE(sp->hs_State.large[3],digest + 16);
        WORD2BYTE(sp->hs_State.large[4],digest + 24);
        break;
    case 384:
        WORD2BYTE(sp->hs_State.large[0],digest);
        WORD2BYTE(sp->hs_State.large[1],digest + 8);
        WORD2BYTE(sp->hs_State.large[2],digest + 16);
        WORD2BYTE(sp->hs_State.large[3],digest + 24);
        WORD2BYTE(sp->hs_State.large[4],digest + 32);
        WORD2BYTE(sp->hs_State.large[5],digest + 40);
        break;
    case 512:
        WORD2BYTE(sp->hs_State.large[0],digest);
        WORD2BYTE(sp->hs_State.large[1],digest + 8);
        WORD2BYTE(sp->hs_State.large[2],digest + 16);
        WORD2BYTE(sp->hs_State.large[3],digest + 24);
        WORD2BYTE(sp->hs_State.large[4],digest + 32);
        WORD2BYTE(sp->hs_State.large[5],digest + 40);
        WORD2BYTE(sp->hs_State.large[6],digest + 48);
        WORD2BYTE(sp->hs_State.large[7],digest + 56);
        break;
    }

    return SUCCESS;
}
