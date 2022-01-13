
#include <stdint.h>

// Specific algorithm definitions
#define EdonR224_DIGEST_SIZE  28
#define EdonR224_BLOCK_SIZE   64
#define EdonR256_DIGEST_SIZE  32
#define EdonR256_BLOCK_SIZE   64
#define EdonR384_DIGEST_SIZE  48
#define EdonR384_BLOCK_SIZE  128
#define EdonR512_DIGEST_SIZE  64
#define EdonR512_BLOCK_SIZE  128

typedef struct {
    uint32_t DoublePipe[32];
    unsigned char LastPart[EdonR256_BLOCK_SIZE * 2];
} Data256;
typedef struct
{
    uint64_t DoublePipe[32];
    unsigned char LastPart[EdonR512_BLOCK_SIZE * 2];
} Data512;

typedef struct {
    int hashbitlen;

    // + algorithm specific parameters
    uint64_t bits_processed;
    union { 
	Data256  p256[1];
	Data512  p512[1];
    } pipe[1];
    int unprocessed_bits;
} edonr_hashState;

#ifdef __cplusplus
extern "C" {
#endif

int edonr_Init(edonr_hashState *state, int hashbitlen);
int edonr_Update(edonr_hashState *state, const void *_data, size_t _len);
int edonr_Final(edonr_hashState *state, unsigned char *hashval);
int edonr_Hash(int hashbitlen, const void *_data, size_t _len, unsigned char *hashval);

#ifdef __cplusplus
}
#endif
