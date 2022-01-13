#ifndef H_FSM
#define H_FSM

/** \ingroup payload
 * \file lib/fsm.h
 * File state machine to handle a payload within an rpm package.
 */

#include <iosm.h>

/** \ingroup payload
 * File state machine data.
 */
typedef /*@abstract@*/ struct iosm_s * FSM_t;

/*@-exportlocal@*/
/*@unchecked@*/
extern int _fsm_debug;
/*@=exportlocal@*/

/** \ingroup payload
 * Iterator across package file info, forward on install, backward on erase.
 */
typedef /*@abstract@*/ struct iosmIterator_s * FSMI_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create file state machine instance.
 * @return		file state machine
 */
/*@only@*/ IOSM_t newFSM(void)
	/*@*/;

/**
 * Destroy file state machine instance.
 * @param fsm		file state machine
 * @return		always NULL
 */
/*@null@*/ IOSM_t freeFSM(/*@only@*/ /*@null@*/ IOSM_t fsm)
	/*@globals fileSystem @*/
	/*@modifies fsm, fileSystem @*/;

/**
 * Load external data into file state machine.
 * @param _fsm		file state machine
 * @param goal
 * @param afmt		archive format (NULL uses cpio)
 * @param _ts		transaction set
 * @param _fi		transaction element file info
 * @param cfd		payload descriptor
 * @retval archiveSize	pointer to archive size
 * @retval failedFile	pointer to first file name that failed.
 * @return		0 on success
 */
int fsmSetup(void * _fsm, iosmFileStage goal, /*@null@*/ const char * afmt,
		const void * _ts,
		const void * _fi,
		FD_t cfd,
		/*@out@*/ unsigned int * archiveSize,
		/*@out@*/ const char ** failedFile)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies cfd, *archiveSize, *failedFile,
		fileSystem, internalState @*/;

/**
 * Clean file state machine.
 * @param _fsm		file state machine
 * @return		0 on success
 */
int fsmTeardown(void * _fsm)
	/*@globals h_errno, fileSystem, internalState @*/
	/*@modifies _fsm, fileSystem, internalState @*/;

/**
 * Map next file path and action.
 * @param fsm		file state machine
 */
int fsmMapPath(IOSM_t fsm)
	/*@modifies fsm @*/;

/**
 * Map file stat(2) info.
 * @param fsm		file state machine
 */
int fsmMapAttrs(IOSM_t fsm)
	/*@modifies fsm @*/;
/*@=exportlocal@*/

/**
 * File state machine driver.
 * @param fsm		file state machine
 * @param nstage	next stage
 * @return		0 on success
 */
int fsmNext(IOSM_t fsm, iosmFileStage nstage)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies fsm, errno, fileSystem, internalState @*/;

/**
 * File state machine driver.
 * @param fsm		file state machine
 * @param stage		next stage
 * @return		0 on success
 */
/*@-exportlocal@*/
int fsmStage(/*@partial@*/ IOSM_t fsm, iosmFileStage stage)
	/*@globals errno, h_errno, fileSystem, internalState @*/
	/*@modifies fsm, errno, fileSystem, internalState @*/;
/*@=exportlocal@*/

#ifdef __cplusplus
}
#endif

#endif	/* H_FSM */
