#ifndef H_ENVVAR
#define H_ENVVAR

/** \ingroup rpmio
 * \file rpmio/envvar.h
 */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HAVE_CLEARENV) || defined(__LCLINT__)
/*@-globstate@*/	/*@ XXX annotate environ. @*/
static inline
int clearenv(void)
	/*@globals environ @*/
	/*@modifies environ @*/
{
    environ = NULL;
    return 0;
}
/*@=globstate@*/
#endif

/*@unused@*/
static inline
/*@observer@*/ /*@null@*/
const char * envGet(const char *name)
	/*@*/
{
    const char * s = getenv(name);
    return s;
}

/*@unused@*/
static inline
int envPut(/*@null@*/ const char *name, /*@null@*/ const char *value)
	/*@*/
{
/*@kept@*/
    char *t;
    char *te;
    size_t nb = 0;
    int rc;

    if (name && !*name)
	name = NULL;
    if (value && !*value)
	value = NULL;
    if (name == NULL && value == NULL) {
	rc = clearenv();
	return rc;
    }

    if (name)
	nb += strlen(name);
    if (value)
	nb += strlen(value) + 1;
    t = te = (char *) xmalloc(nb + 1);
    *te = '\0';
    if (name)
	te = stpcpy(te, name);
    if (value)
	te = stpcpy( stpcpy(te, "="), value);
    *te = '\0';

    rc = putenv(t);
    return rc;
}

#ifdef __cplusplus
}
#endif

#endif  /* H_ENVVAR */
