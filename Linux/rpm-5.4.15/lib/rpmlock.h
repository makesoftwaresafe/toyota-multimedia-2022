#ifndef RPMLOCK_H
#define RPMLOCK_H 

#ifdef __cplusplus
extern "C" {
#endif

/*@only@*/ /*@null@*/
void * rpmtsAcquireLock(rpmts ts)
	/*@globals h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/
	/*@modifies h_errno, rpmGlobalMacroContext, fileSystem, internalState @*/;

/*@null@*/
void * rpmtsFreeLock(/*@only@*/ /*@null@*/ void *lock)
	/*@globals fileSystem, internalState @*/
	/*@modifies lock, fileSystem, internalState @*/;

#ifdef __cplusplus
}
#endif

#endif
