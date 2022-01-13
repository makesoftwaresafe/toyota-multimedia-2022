#ifndef _H_RPMSPEC_
#define _H_RPMSPEC_

/** \ingroup rpmbuild
 * \file build/rpmspec.h
 *  The Spec and Package data structures used during build.
 */

#include <rpmevr.h>
#include <rpmfi.h>	/* XXX rpmfi typedef */
#include <rpmcli.h>	/* XXX QVA_t typedef */

/** \ingroup rpmbuild
 */
typedef /*@abstract@*/ /*@refcounted@*/ struct Package_s * Package;

/** \ingroup rpmbuild
 */
typedef struct Source * SpecSource;

/** \ingroup rpmbuild
 */
struct TriggerFileEntry {
    int index;
/*@only@*/
    char * fileName;
/*@only@*/
    char * script;
/*@only@*/
    char * prog;
/*@owned@*/
    struct TriggerFileEntry * next;
};

#define RPMBUILD_DEFAULT_LANG "C"

/** \ingroup rpmbuild
 */
struct Source {
/*@owned@*/
    const char * fullSource;
/*@dependent@*/ /*@relnull@*/
    const char * source;	/* Pointer into fullSource */
    int flags;
    rpmuint32_t num;
/*@owned@*/
    struct Source * next;
};

/** \ingroup rpmbuild
 */
/*@-typeuse@*/
typedef struct ReadLevelEntry {
    int reading;
/*@dependent@*/
    struct ReadLevelEntry * next;
} RLE_t;
/*@=typeuse@*/

/** \ingroup rpmbuild
 */
typedef struct OpenFileInfo {
/*@only@*/
    const char * fileName;
/*@relnull@*/
    FD_t fd;
    int lineNum;
    char readBuf[BUFSIZ];
/*@dependent@*/
    char * readPtr;
/*@owned@*/
    struct OpenFileInfo * next;
} OFI_t;

/** \ingroup rpmbuild
 */
typedef struct spectag_s {
    int t_tag;
    int t_startx;
    int t_nlines;
/*@only@*/
    const char * t_lang;
/*@only@*/
    const char * t_msgid;
} * spectag;

/** \ingroup rpmbuild
 */
typedef struct spectags_s {
/*@owned@*/
    spectag st_t;
    int st_nalloc;
    int st_ntags;
} * spectags;

/** \ingroup rpmbuild
 */
typedef struct speclines_s {
/*@only@*/
    char **sl_lines;
    int sl_nalloc;
    int sl_nlines;
} * speclines;

/** \ingroup rpmbuild
 * The structure used to store values parsed from a spec file.
 */
struct Spec_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@only@*/
    const char * specFile;	/*!< Name of the spec file. */
/*@only@*/
    const char * buildSubdir;
/*@only@*/
    const char * rootURL;

/*@owned@*/ /*@null@*/
    speclines sl;
/*@owned@*/ /*@null@*/
    spectags st;

/*@owned@*/
    struct OpenFileInfo * fileStack;
/*@owned@*/
    char *lbuf;
    size_t lbuf_len;
/*@dependent@*/
    char *lbufPtr;
    char nextpeekc;
/*@dependent@*/
    char * nextline;
/*@dependent@*/
    char * line;
    int lineNum;

/*@owned@*/
    struct ReadLevelEntry * readStack;

/*@owned@*/ /*@null@*/
    Spec * BASpecs;
/*@only@*/ /*@null@*/
    const char ** BANames;
    int BACount;
    int recursing;		/*!< parse is recursive? */
    int toplevel;

    int force;
    int anyarch;

/*@null@*/
    char * passPhrase;
    int timeCheck;
/*@null@*/
    const char * cookie;

/*@owned@*/
    struct Source * sources;
    int numSources;
    int noSource;

/*@only@*/
    const char * sourceRpmName;
/*@only@*/
    unsigned char * sourcePkgId;
/*@refcounted@*/
    Header sourceHeader;
/*@refcounted@*/
    rpmfi fi;
    int sourceHdrInit;

/*@dependent@*/ /*@null@*/
    MacroContext macros;

    rpmRC (*_parseRCPOT) (Spec spec, Package pkg, const char *field, rpmTag tagN,
               rpmuint32_t index, rpmsenseFlags tagflags);

    rpmuint32_t sstates[RPMSCRIPT_MAX];		/*!< scriptlet states. */
    rpmuint32_t smetrics[RPMSCRIPT_MAX];	/*!< scriptlet time metrics. */

/*@only@*/
    rpmiob prep;		/*!< %prep scriptlet. */
/*@only@*/
    rpmiob build;		/*!< %build scriptlet. */
/*@only@*/
    rpmiob install;		/*!< %install scriptlet. */
/*@only@*/
    rpmiob check;		/*!< %check scriptlet. */
/*@only@*/
    rpmiob clean;		/*!< %clean scriptlet. */

    size_t nfoo;
/*@only@*/ /*@relnull@*/
    tagStore_t foo;

    void * dig;			/*!< Auto-signing keypair. */

    Package packages;		/*!< Package list. */
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

/** \ingroup rpmbuild
 * The structure used to store values for a package.
 */
struct Package_s {
    struct rpmioItem_s _item;	/*!< usage mutex and pool identifier. */
/*@refcounted@*/
    Header header;
/*@refcounted@*/
    rpmds ds;			/*!< Requires: N = EVR */
/*@refcounted@*/
    rpmfi fi;

    int autoReq;
    int autoProv;
    int noarch;

/*@only@*/
    const char * preInFile;	/*!< %pre scriptlet. */
/*@only@*/
    const char * postInFile;	/*!< %post scriptlet. */
/*@only@*/
    const char * preUnFile;	/*!< %preun scriptlet. */
/*@only@*/
    const char * postUnFile;	/*!< %postun scriptlet. */
/*@only@*/
    const char * preTransFile;	/*!< %pretrans scriptlet. */
/*@only@*/
    const char * postTransFile;	/*!< %posttrans scriptlet. */
/*@only@*/
    const char * verifyFile;	/*!< %verifyscript scriptlet. */
/*@only@*/
    const char * sanityCheckFile;/*!< %sanitycheck scriptlet. */

/*@only@*/
    rpmiob specialDoc;

/*@only@*/
    struct TriggerFileEntry * triggerFiles;

/*@only@*/
    const char * fileFile;
/*@only@*/
    rpmiob fileList;		/* If NULL, package will not be written */

    Package next;
#if defined(__LCLINT__)
/*@refs@*/
    int nrefs;			/*!< (unused) keep splint happy */
#endif
};

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup rpmbuild
 * Destroy a spec file control structure.
 * @todo Rename to specFree.
 * @param spec		spec file control structure
 * @return		NULL on last dereference
 */
/*@null@*/
Spec freeSpec(/*@killref@*/ /*@null@*/ Spec spec)
	/*@globals fileSystem, internalState @*/
	/*@modifies spec, fileSystem, internalState @*/;
#define	freeSpec(_spec)	\
    ((Spec)rpmioFreePoolItem((rpmioItem)(_spec), __FUNCTION__, __FILE__, __LINE__))

/** \ingroup rpmbuild
 * Create and initialize Spec structure.
 * @return spec		spec file control structure
 */
/*@only@*/
Spec newSpec(void)
	/*@globals rpmGlobalMacroContext, h_errno, internalState @*/
	/*@modifies rpmGlobalMacroContext, internalState @*/;

/** \ingroup rpmbuild
 * Function to query spec file(s).
 * @param ts		transaction set
 * @param qva		parsed query/verify options
 * @param arg		query argument
 * @return		0 on success, else no. of failures
 */
int rpmspecQuery(rpmts ts, QVA_t qva, const char * arg)
	/*@globals rpmCLIMacroContext,
		rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies ts, qva, rpmCLIMacroContext, rpmGlobalMacroContext,
		fileSystem, internalState @*/;

/** \ingroup rpmbuild
 */
struct OpenFileInfo * newOpenFileInfo(void)
	/*@*/;

/** \ingroup rpmbuild
 * stashSt.
 * @param spec		spec file control structure
 * @param h		header
 * @param tag		tag
 * @param lang		locale
 * @return		ptr to saved entry
 */
spectag stashSt(Spec spec, Header h, rpmTag tag, const char * lang)
	/*@globals internalState @*/
	/*@modifies spec->st, internalState @*/;

/** \ingroup rpmbuild
 * addSource.
 * @param spec		spec file control structure
 * @param pkg		package control
 * @param field		field to parse
 * @param tag		tag
 * @return		0 on success
 */
int addSource(Spec spec, Package pkg, const char * field, rpmTag tag)
	/*@globals rpmGlobalMacroContext, h_errno, fileSystem, internalState @*/
	/*@modifies spec->sources, spec->numSources,
		spec->st, spec->macros,
		rpmGlobalMacroContext, fileSystem, internalState @*/;

/** \ingroup rpmbuild
 * parseNoSource.
 * @param spec		spec file control structure
 * @param field		field to parse
 * @param tag		tag
 * @return		0 on success
 */
int parseNoSource(Spec spec, const char * field, rpmTag tag)
	/*@*/;

/** \ingroup rpmbuild
 * Return the count of source set in specfile
 * @param spec      spec file control structure
 * @return  the count of source
 */
int SpecSourceCount(Spec spec)
	/*@*/;

/** \ingroup rpmbuild
 * Return a source control structure
 * @param spec      spec file control structure
 * @param num       the number of the wanted source (starting from 0)
 * @return          a SpecSource structure, NULL if not found
 */
SpecSource getSource(Spec spec, int num)
	/*@*/;

/** \ingroup rpmbuild
 * Return a ptr to the source file name
 * @param source    SpecSource control structure
 * @return          ptr to filename
 */
/*@exposed@*/
const char * specSourceName(SpecSource source)
	/*@*/;

/** \ingroup rpmbuild
 * Return a ptr to the full url of the source
 * @param source    SpecSource control structure
 * @return          ptr to url
 */
/*@exposed@*/
const char * specFullSourceName(SpecSource source)
	/*@*/;

/** \ingroup rpmbuild
 * Return the spec or source patch number
 * @param source    SpecSource control structure
 * @return          the number of the source
 */
int specSourceNum(SpecSource source)
	/*@*/;

/** \ingroup rpmbuild
 * Return flags set for the source
 * @param source    SpecSource control structure
 * @return          flags
 */
int specSourceFlags(SpecSource source)
	/*@*/;

/** \ingroup rpmbuild
 * Return the macro directory location from source file flags
 * @param attr      rpmfileAttrs from source
 * @return          string containings macros about location, NULL on failure
 */
/*@null@*/
#if defined(RPM_VENDOR_OPENPKG) /* splitted-source-directory */
const char * getSourceDir(rpmfileAttrs attr, const char *filename)
#else
const char * getSourceDir(rpmfileAttrs attr)
#endif
	/*@*/;

#ifdef __cplusplus
}
#endif

#endif /* _H_SPEC_ */
