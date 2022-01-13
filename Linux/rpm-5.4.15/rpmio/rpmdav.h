#ifndef RPMDAV_H
#define RPMDAV_H

/** \ingroup rpmio
 * \file rpmio/rpmdav.h
 */
#include <rpmio.h>
#include <rpmurl.h>

/**
 */
/*@unchecked@*/
extern int _ftp_debug;
/*@unchecked@*/
extern int _dav_debug;
/*@unchecked@*/
extern int rpmioHttpReadTimeoutSecs;
/*@unchecked@*/
extern int rpmioHttpConnectTimeoutSecs;
/*@unchecked@*/ /*@null@*/
extern const char * rpmioHttpAccept;
/*@unchecked@*/ /*@null@*/
extern const char * rpmioHttpUserAgent;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create an argv directory from an ftp:// URI.
 * @param path		ftp:// directory path
 * @return 		argv DIR
 */
/*@null@*/
DIR * ftpOpendir(const char * path)
	/*@globals h_errno, errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * Close active neon transfer(s) (if any).
 * @param _u		URL container
 * @return		0 on success
 */
int davDisconnect(void * _u)
	/*@globals internalState @*/
	/*@modifies _u, internalState @*/;

/**
 * Free persistent neon session state.
 * @param u		URL container
 * @return		0 on success
 */
int davFree(urlinfo u)
	/*@globals internalState @*/
	/*@modifies u, internalState @*/;

/**
 * Free global neon+openSSL state memory.
 */
void davDestroy(void)
	/*@globals internalState @*/
	/*@modifies internalState @*/;

/**
 * Send a http request.
 * @param ctrl		connection stream
 * @param httpCmd	http command
 * @param httpArg	http command argument (NULL if none)
 * @returns		0 on success
 */
int davReq(FD_t ctrl, const char * httpCmd, /*@null@*/ const char * httpArg)
	/*@globals fileSystem, internalState @*/
	/*@modifies ctrl, fileSystem, internalState @*/;

/**
 * Read a http response.
 * @param u		URL container
 * @param ctrl		connection stream
 * @retval *str		error msg		
 * @returns		0 on success
 */
/*@-exportlocal@*/
int davResp(urlinfo u, FD_t ctrl, /*@out@*/ /*@null@*/ char *const * str)
	/*@globals fileSystem, internalState @*/
	/*@modifies ctrl, *str, fileSystem, internalState @*/;
/*@=exportlocal@*/

/**
 * Open a URL.
 * @param url
 * @param flags
 * @param mode
 * @retval *uret
 * @return
 */
/*@null@*/
FD_t davOpen(const char * url, /*@unused@*/ int flags,
		/*@unused@*/ mode_t mode, /*@out@*/ urlinfo * uret)
        /*@globals internalState @*/
        /*@modifies *uret, internalState @*/;

/*@null@*/
FD_t httpOpen(const char * url, /*@unused@*/ int flags,
                /*@unused@*/ mode_t mode, /*@out@*/ urlinfo * uret)
        /*@globals internalState @*/
        /*@modifies *uret, internalState @*/;

/**
 */
ssize_t davRead(void * cookie, /*@out@*/ char * buf, size_t count)
        /*@globals errno, fileSystem, internalState @*/
        /*@modifies buf, errno, fileSystem, internalState @*/;

/**
 */
ssize_t davWrite(void * cookie, const char * buf, size_t count)
        /*@globals fileSystem, internalState @*/
        /*@modifies fileSystem, internalState @*/;

/**
 */
int davSeek(void * cookie, _libio_pos_t pos, int whence)
        /*@globals fileSystem, internalState @*/
        /*@modifies fileSystem, internalState @*/;

/**
 */
int davClose(void * cookie)
	/*@globals fileSystem, internalState @*/
	/*@modifies cookie, fileSystem, internalState @*/;

/**
 */
int davMkdir(const char * path, mode_t mode)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 */
int davRmdir(const char * path)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 */
int davRename(const char * oldpath, const char * newpath)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 */
int davUnlink(const char * path)
	/*@globals fileSystem, internalState @*/
	/*@modifies fileSystem, internalState @*/;

/**
 * Close a DAV collection.
 * @param dir		argv DIR
 * @return 		0 always
 */
int davClosedir(/*@only@*/ DIR * dir)
	/*@globals fileSystem @*/
	/*@modifies dir, fileSystem @*/;

/**
 * Return next entry from a DAV collection.
 * @param dir		argv DIR
 * @return 		next entry
 */
/*@dependent@*/ /*@null@*/
struct dirent * davReaddir(DIR * dir)
	/*@globals fileSystem @*/
	/*@modifies fileSystem @*/;

/**
 * Create an argv directory from DAV collection.
 * @param path		URL for DAV collection path
 * @return 		argv DIR
 */
/*@null@*/
DIR * davOpendir(const char * path)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies errno, fileSystem, internalState @*/;

/**
 * stat(2) clone.
 */
int davStat(const char * path, /*@out@*/ struct stat * st)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies *st, errno, fileSystem, internalState @*/;

/**
 * lstat(2) clone.
 */
int davLstat(const char * path, /*@out@*/ struct stat * st)
	/*@globals errno, fileSystem, internalState @*/
	/*@modifies *st, errno, fileSystem, internalState @*/;

/**
 * realpath(3) clone.
 */
char * davRealpath(const char * path, /*@out@*/ /*@null@*/ char * resolved_path)
	/*@modifies *resolved_path @*/;

#ifdef __cplusplus
}
#endif

#endif /* RPMDAV_H */
