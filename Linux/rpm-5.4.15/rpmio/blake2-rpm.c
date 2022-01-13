#include "system.h"

#include "blake2-rpm.h"

#include "debug.h"

int blake2bInit(blake2bParam * sp, int hashbitlen)
{
    sp->P.digest_length = hashbitlen/8;
    return blake2b_init(&sp->S, sp->P.digest_length);
}

int blake2bReset(blake2bParam * sp)
{
    return blake2b_init(&sp->S, sp->P.digest_length);
}

int blake2bUpdate(blake2bParam * sp, const byte * data, size_t size)
{
    return blake2b_update(&sp->S, data, size);
}

int blake2bDigest(blake2bParam * sp, byte * digest)
{
    return blake2b_final(&sp->S, digest, sp->P.digest_length);
}

const hashFunction _blake2b = {
    .name = "BLAKE2B",
    .paramsize = sizeof(blake2bParam),
    .blocksize = BLAKE2B_BLOCKBYTES,
    .digestsize = BLAKE2B_OUTBYTES,
    .reset = (hashFunctionReset) blake2bReset,
    .update = (hashFunctionUpdate) blake2bUpdate,
    .digest = (hashFunctionDigest) blake2bDigest
};


int blake2bpInit(blake2bpParam * sp, int hashbitlen)
{
    sp->P.digest_length = hashbitlen/8;
    return blake2bp_init(&sp->S, sp->P.digest_length);
}

int blake2bpReset(blake2bpParam * sp)
{
    return blake2bp_init(&sp->S, sp->P.digest_length);
}

int blake2bpUpdate(blake2bpParam * sp, const byte * data, size_t size)
{
    return blake2bp_update(&sp->S, data, size);
}

int blake2bpDigest(blake2bpParam * sp, byte * digest)
{
    return blake2bp_final(&sp->S, digest, sp->P.digest_length);
}

const hashFunction _blake2bp = {
    .name = "BLAKE2BP",
    .paramsize = sizeof(blake2bpParam),
    .blocksize = BLAKE2B_BLOCKBYTES,
    .digestsize = BLAKE2B_OUTBYTES,
    .reset = (hashFunctionReset) blake2bpReset,
    .update = (hashFunctionUpdate) blake2bpUpdate,
    .digest = (hashFunctionDigest) blake2bpDigest
};

int blake2sInit(blake2sParam * sp, int hashbitlen)
{
    sp->P.digest_length = hashbitlen/8;
    return blake2s_init(&sp->S, sp->P.digest_length);
}

int blake2sReset(blake2sParam * sp)
{
    return blake2s_init(&sp->S, sp->P.digest_length);
}

int blake2sUpdate(blake2sParam * sp, const byte * data, size_t size)
{
    return blake2s_update(&sp->S, data, size);
}

int blake2sDigest(blake2sParam * sp, byte * digest)
{
    return blake2s_final(&sp->S, digest, sp->P.digest_length);
}

const hashFunction _blake2s = {
    .name = "BLAKE2S",
    .paramsize = sizeof(blake2sParam),
    .blocksize = BLAKE2S_BLOCKBYTES,
    .digestsize = BLAKE2S_OUTBYTES,
    .reset = (hashFunctionReset) blake2sReset,
    .update = (hashFunctionUpdate) blake2sUpdate,
    .digest = (hashFunctionDigest) blake2sDigest
};

int blake2spInit(blake2spParam * sp, int hashbitlen)
{
    sp->P.digest_length = hashbitlen/8;
    return blake2sp_init(&sp->S, sp->P.digest_length);
}

int blake2spReset(blake2spParam * sp)
{
    return blake2sp_init(&sp->S, sp->P.digest_length);
}

int blake2spUpdate(blake2spParam * sp, const byte * data, size_t size)
{
    return blake2sp_update(&sp->S, data, size);
}

int blake2spDigest(blake2spParam * sp, byte * digest)
{
    return blake2sp_final(&sp->S, digest, sp->P.digest_length);
}

const hashFunction _blake2sp = {
    .name = "BLAKE2SP",
    .paramsize = sizeof(blake2spParam),
    .blocksize = BLAKE2S_BLOCKBYTES,
    .digestsize = BLAKE2S_OUTBYTES,
    .reset = (hashFunctionReset) blake2spReset,
    .update = (hashFunctionUpdate) blake2spUpdate,
    .digest = (hashFunctionDigest) blake2spDigest
};
