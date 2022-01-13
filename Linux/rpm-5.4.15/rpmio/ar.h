#ifndef H_AR
#define H_AR

/** \ingroup payload
 * \file rpmio/ar.h
 *  Structures used for ar(1) archives.
 */

/*
 */
typedef	struct arHeader_s * arHeader;

/* ar(1) file constants  */
# define AR_MAGIC	"!<arch>\n"
# define AR_MARKER	"`\n"

/** \ingroup payload
 * ar(1) archive header.
 */
struct arHeader_s {
	char name[16];
	char mtime[12];
	char uid[6];
	char gid[6];
	char mode[8];
	char filesize[10];
	char marker[2];
};

/*@unchecked@*/
extern int _ar_debug;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read ar(1) header.
 * @retval _iosm	file path and stat info
 * @retval st
 * @return		0 on success
 */
int arHeaderRead(void * _iosm, struct stat * st)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, *st, fileSystem, internalState @*/;

/**
 * Write ar(1) header.
 * @retval _iosm		file path and stat info
 * @param st
 * @return		0 on success
 */
int arHeaderWrite(void * _iosm, struct stat * st)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/;

/**
 * Write ar(1) trailer.
 * @retval _iosm	file path and stat info
 * @return		0 on success
 */
int arTrailerWrite(void * _iosm)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_AR */
