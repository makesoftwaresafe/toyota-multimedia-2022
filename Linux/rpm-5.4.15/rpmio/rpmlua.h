#ifndef RPMLUA_H
#define RPMLUA_H

typedef enum rpmluavType_e {
    RPMLUAV_NIL		= 0,
    RPMLUAV_STRING	= 1,
    RPMLUAV_NUMBER	= 2
} rpmluavType;

#if defined(_RPMLUA_INTERNAL)

#include <stdarg.h>
#include <lua.h>

struct rpmlua_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    lua_State *L;
    int pushsize;
    int storeprint;
    size_t printbufsize;
    size_t printbufused;
/*@relnull@*/
    char *printbuf;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

struct rpmluav_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
    rpmluavType keyType;
    rpmluavType valueType;
    union {
	const char *str;
	const void *ptr;
	double num;
    } key;
    union {
	const char *str;
	const void *ptr;
	double num;
    } value;
    int listmode;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#endif /* _RPMLUA_INTERNAL */

typedef /*@abstract@*/ struct rpmlua_s * rpmlua;
typedef /*@abstract@*/ struct rpmluav_s * rpmluav;

#ifdef __cplusplus
extern "C" {
#endif

/*@unchecked@*/ /*@observer@*/
extern const char * rpmluaFiles;

/*@unchecked@*/ /*@observer@*/
extern const char * rpmluaPath;

/*@-exportlocal@*/
/*@only@*/ /*@exposed@*/ /*@relnull@*/
rpmlua rpmluaGetGlobalState(void)
	/*@*/;

rpmlua rpmluaNew(void)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;
/*@=exportlocal@*/
/*@null@*/
void *rpmluaFree(/*@only@*/ rpmlua lua)
	/*@globals internalState @*/
	/*@modifies lua, internalState @*/;

int rpmluaCheckScript(/*@null@*/ rpmlua _lua, const char *script,
		      /*@null@*/ const char *name)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
int rpmluaRunScript(/*@null@*/ rpmlua _lua, const char *script,
		    /*@null@*/ const char *name)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
/*@-exportlocal@*/
int rpmluaRunScriptFile(/*@null@*/ rpmlua _lua, const char *filename)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
/*@=exportlocal@*/
void rpmluaInteractive(/*@null@*/ rpmlua _lua)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;

/*@null@*/
void *rpmluaGetData(/*@null@*/ rpmlua _lua, const char *key)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
/*@-exportlocal@*/
void rpmluaSetData(/*@null@*/ rpmlua _lua, const char *key, const void *data)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
/*@=exportlocal@*/

/*@exposed@*/
const char *rpmluaGetPrintBuffer(/*@null@*/ rpmlua _lua)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
void rpmluaSetPrintBuffer(/*@null@*/ rpmlua _lua, int flag)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;

void rpmluaGetVar(/*@null@*/ rpmlua _lua, rpmluav var)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, var, fileSystem, internalState @*/;
void rpmluaSetVar(/*@null@*/ rpmlua _lua, rpmluav var)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, var, fileSystem, internalState @*/;
void rpmluaDelVar(/*@null@*/ rpmlua _lua, const char *key, ...)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
int rpmluaVarExists(/*@null@*/ rpmlua _lua, const char *key, ...)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
void rpmluaPushTable(/*@null@*/ rpmlua _lua, const char *key, ...)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;
void rpmluaPop(/*@null@*/ rpmlua _lua)
	/*@globals fileSystem, internalState @*/
	/*@modifies _lua, fileSystem, internalState @*/;

/*@only@*/
rpmluav rpmluavNew(void)
	/*@*/;
/*@null@*/
void * rpmluavFree(/*@only@*/ rpmluav var)
	/*@modifes var @*/;
void rpmluavSetListMode(rpmluav var, int flag)
	/*@modifies var @*/;
/*@-exportlocal@*/
void rpmluavSetKey(rpmluav var, rpmluavType type, const void *value)
	/*@modifies var @*/;
/*@=exportlocal@*/
/*@-exportlocal@*/
void rpmluavSetValue(rpmluav var, rpmluavType type, const void *value)
	/*@modifies var @*/;
/*@=exportlocal@*/
/*@-exportlocal@*/
void rpmluavGetKey(rpmluav var, /*@out@*/ rpmluavType *type, /*@out@*/ void **value)
	/*@modifies *type, *value @*/;
/*@=exportlocal@*/
/*@-exportlocal@*/
void rpmluavGetValue(rpmluav var, /*@out@*/ rpmluavType *type, /*@out@*/ void **value)
	/*@modifies *type, *value @*/;
/*@=exportlocal@*/

/* Optional helpers for numbers. */
void rpmluavSetKeyNum(rpmluav var, double value)
	/*@modifies var @*/;
void rpmluavSetValueNum(rpmluav var, double value)
	/*@modifies var @*/;
double rpmluavGetKeyNum(rpmluav var)
	/*@*/;
double rpmluavGetValueNum(rpmluav var)
	/*@*/;
int rpmluavKeyIsNum(rpmluav var)
	/*@*/;
int rpmluavValueIsNum(rpmluav var)
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMLUA_H */
