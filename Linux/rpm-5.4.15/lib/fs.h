#ifndef H_FS
#define H_FS

/** \ingroup rpmtrans
 * \file lib/fs.h
 * Access mounted file system information.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Release storage used by file system usage cache.
 */
void rpmFreeFilesystems(void)
	/*@globals internalState@*/
	/*@modifies internalState@*/;

/**
 * Return (cached) file system mount points.
 * @retval listptr		addess of file system names (or NULL)
 * @retval num			address of number of file systems (or NULL)
 * @return			0 on success, 1 on error
 */
/*@-incondefs@*/
int rpmGetFilesystemList( /*@null@*/ /*@out@*/ const char *** listptr,
		/*@null@*/ /*@out@*/ rpmuint32_t * num)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies *listptr, *num, fileSystem, internalState @*/
	/*@requires maxSet(listptr) >= 0 /\ maxSet(num) >= 0 @*/;
/*@=incondefs@*/

/**
 * Determine per-file system usage for a list of files.
 * @param fileList		array of absolute file names
 * @param fssizes		array of file sizes
 * @param numFiles		number of files in list
 * @retval usagesPtr		address of per-file system usage array (or NULL)
 * @param flags			(unused)
 * @return			0 on success, 1 on error
 */
/*@-incondefs@*/
int rpmGetFilesystemUsage(const char ** fileList, rpmuint32_t * fssizes,
		int numFiles, /*@null@*/ /*@out@*/ rpmuint64_t ** usagesPtr,
		int flags)
	/*@globals rpmGlobalMacroContext, h_errno,
		fileSystem, internalState @*/
	/*@modifies *usagesPtr, rpmGlobalMacroContext,
		fileSystem, internalState @*/
	/*@requires maxSet(fileList) >= 0 /\ maxSet(fssizes) == 0
		/\ maxSet(usagesPtr) >= 0 @*/;
/*@=incondefs@*/

#ifdef __cplusplus
}
#endif

#endif  /* H_FS */
