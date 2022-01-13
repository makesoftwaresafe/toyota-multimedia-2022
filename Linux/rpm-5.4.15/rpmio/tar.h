#ifndef H_TAR
#define H_TAR

/** \ingroup payload
 * \file rpmio/tar.h
 * Structures used for tar(1) archives.
 */

/**
 */
typedef	struct tarHeader_s * tarHeader;

/* Tar file constants */
# define TAR_MAGIC          "ustar"	/* ustar and a null */
# define TAR_VERSION        "  "	/* Be compatible with GNU tar format */

#define TAR_BLOCK_SIZE		512
#define TAR_MAGIC_LEN		6
#define TAR_VERSION_LEN		2

/* POSIX tar Header Block, from POSIX 1003.1-1990  */
#define TAR_NAME_SIZE		100

/** \ingroup payload
 * Tar archive header information.
 */
struct tarHeader_s {		/* byte offset */
	char name[TAR_NAME_SIZE];	/*   0-99 */
	char mode[8];		/* 100-107 */		/* mode */
	char uid[8];		/* 108-115 */		/* uid */
	char gid[8];		/* 116-123 */		/* gid */
	char filesize[12];	/* 124-135 */		/* ilesize */
	char mtime[12];		/* 136-147 */		/* mtime */
	char checksum[8];	/* 148-155 */		/* checksum */
	char typeflag;		/* 156-156 */
	char linkname[TAR_NAME_SIZE];	/* 157-256 */
	char magic[6];		/* 257-262 */		/* magic */
	char version[2];	/* 263-264 */
	char uname[32];		/* 265-296 */
	char gname[32];		/* 297-328 */
	char devMajor[8];	/* 329-336 */		/* devMajor */
	char devMinor[8];	/* 337-344 */		/* devMinor */
	char prefix[155];	/* 345-499 */
	char padding[12];	/* 500-512 (pad to exactly TAR_BLOCK_SIZE) */
};

/*@unchecked@*/
extern int _tar_debug;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read tar header from payload.
 * @retval _iosm	file path and stat info
 * @retval st
 * @return		0 on success
 */
int tarHeaderRead(void * _iosm, struct stat * st)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, *st, fileSystem, internalState @*/;

/**
 * Write tar header to payload.
 * @retval _iosm	file path and stat info
 * @param st
 * @return		0 on success
 */
int tarHeaderWrite(void * _iosm, struct stat * st)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/;

/**
 * Write cpio trailer to payload.
 * @retval _fsm		file path and stat info
 * @return		0 on success
 */
int tarTrailerWrite(void * _iosm)
	/*@globals fileSystem, internalState @*/
	/*@modifies _iosm, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif	/* H_TAR */
