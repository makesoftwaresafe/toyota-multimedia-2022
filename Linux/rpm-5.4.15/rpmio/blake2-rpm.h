#ifndef	_BLAKE2_RPM_H
#define	_BLAKE2_RPM_H

#include <blake2.h>
#include <blake2-impl.h>

#include <beecrypt/beecrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blake2bParam_s {
    blake2b_state S;
    blake2b_param P;
} blake2bParam;

BEECRYPTAPI
int blake2bInit(blake2bParam * sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int blake2bReset(blake2bParam * sp)
	/*@*/;
BEECRYPTAPI
int blake2bUpdate(blake2bParam * sp, const byte * data, size_t size)
	/*@*/;
BEECRYPTAPI
int blake2bDigest(blake2bParam * sp, byte * digest)
	/*@*/;

typedef	struct blake2bpParam_s {
    blake2bp_state S;
    blake2b_param P;
} blake2bpParam;

BEECRYPTAPI
int blake2bpInit(blake2bpParam * sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int blake2bpReset(blake2bpParam * sp)
	/*@*/;
BEECRYPTAPI
int blake2bpUpdate(blake2bpParam * sp, const byte * data, size_t size)
	/*@*/;
BEECRYPTAPI
int blake2bpDigest(blake2bpParam * sp, byte * digest)
	/*@*/;

typedef	struct blake2sParam_s {
    blake2s_state S;
    blake2s_param P;
} blake2sParam;

BEECRYPTAPI
int blake2sInit(blake2sParam * sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int blake2sReset(blake2sParam * sp)
	/*@*/;
BEECRYPTAPI
int blake2sUpdate(blake2sParam * sp, const byte * data, size_t size)
	/*@*/;
BEECRYPTAPI
int blake2sDigest(blake2sParam * sp, byte * digest)
	/*@*/;

typedef	struct blake2spParam_s {
    blake2sp_state S;
    blake2s_param P;
} blake2spParam;

BEECRYPTAPI
int blake2spInit(blake2spParam * sp, int hashbitlen)
	/*@*/;
BEECRYPTAPI
int blake2spReset(blake2spParam * sp)
	/*@*/;
BEECRYPTAPI
int blake2spUpdate(blake2spParam * sp, const byte * data, size_t size)
	/*@*/;
BEECRYPTAPI
int blake2spDigest(blake2spParam * sp, byte * digest)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif	/* _BLAKE2_RPM_H */
