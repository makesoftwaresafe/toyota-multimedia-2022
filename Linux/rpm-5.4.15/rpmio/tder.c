/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Optimized ASN.1 DER decoder */

#include "system.h"
#include <stddef.h>

#include <rpmio.h>
#include <poptIO.h>

#define	DEBUG	1

#ifdef __cplusplus 
# define SEC_BEGIN_PROTOS extern "C" {
# define SEC_END_PROTOS }
#else
# define SEC_BEGIN_PROTOS
# define SEC_END_PROTOS
#endif

/*
 * define XP_WIN, XP_BEOS, or XP_UNIX, in case they are not defined
 * by anyone else
 */
#ifdef _WINDOWS
# ifndef XP_WIN
# define XP_WIN
# endif
#if defined(_WIN32) || defined(WIN32)
# ifndef XP_WIN32
# define XP_WIN32
# endif
#endif
#endif

#ifdef __BEOS__
# ifndef XP_BEOS
# define XP_BEOS
# endif
#endif

#ifdef unix
# ifndef XP_UNIX
# define XP_UNIX
# endif
#endif

/* ========== <nspr4/plarena.h> */
typedef long		PRWord;
typedef unsigned long	PRUword;
typedef unsigned int	PRUint32;
typedef int		PRInt32;
typedef double		PRFloat64;

typedef int		PRIntn;
typedef unsigned int	PRUintn;

typedef PRIntn		PRBool;
#define PR_TRUE 1
#define PR_FALSE 0

#define PR_CALLBACK
#define PR_CALLBACK_DECL

typedef struct PLArena	PLArena;
struct PLArena {
    PLArena     *next;          /* next arena for this lifetime */
    PRUword     base;           /* aligned base address, follows this header */
    PRUword     limit;          /* one beyond last byte in arena */
    PRUword     avail;          /* points to next available byte */
};

#ifdef PL_ARENAMETER
typedef struct PLArenaStats PLArenaStats;
struct PLArenaStats {
    PLArenaStats  *next;        /* next in arenaStats list */
    char          *name;        /* name for debugging */
    PRUint32      narenas;      /* number of arenas in pool */
    PRUint32      nallocs;      /* number of PL_ARENA_ALLOCATE() calls */
    PRUint32      nreclaims;    /* number of reclaims from freeArenas */
    PRUint32      nmallocs;     /* number of malloc() calls */
    PRUint32      ndeallocs;    /* number of lifetime deallocations */
    PRUint32      ngrows;       /* number of PL_ARENA_GROW() calls */
    PRUint32      ninplace;     /* number of in-place growths */
    PRUint32      nreleases;    /* number of PL_ARENA_RELEASE() calls */
    PRUint32      nfastrels;    /* number of "fast path" releases */
    PRUint32      nbytes;       /* total bytes allocated */
    PRUint32      maxalloc;     /* maximum allocation size in bytes */
    PRFloat64     variance;     /* size variance accumulator */
};
#endif

typedef struct PLArenaPool	PLArenaPool;
struct PLArenaPool {
    PLArena     first;          /* first arena in pool list */
    PLArena     *current;       /* arena from which to allocate space */
    PRUint32    arenasize;      /* net exact size of a new arena */
    PRUword     mask;           /* alignment mask (power-of-2 - 1) */
#ifdef PL_ARENAMETER
    PLArenaStats stats;
#endif
};
/* ========== */

/* ========== <nspr4/plhash.h> */

/* ========== <nspr4/plstr.h> */

/* ========== <nspr4/prtypes.h> */

/* ========== <nspr4/prlink.h> */
typedef struct PRLibrary PRLibrary;
typedef void (*PRFuncPtr)(void);
/* ========== */

/* ========== <nspr4/prlog.h> */

/* ========== <nss3/utilrename.h> */

/* ========== <nss3/secerr.h> */

#define SEC_ERROR_BASE				(-0x2000)
#define SEC_ERROR_LIMIT				(SEC_ERROR_BASE + 1000)

#define IS_SEC_ERROR(code) \
    (((code) >= SEC_ERROR_BASE) && ((code) < SEC_ERROR_LIMIT))

#ifndef NO_SECURITY_ERROR_ENUM
typedef enum {

SEC_ERROR_INVALID_ARGS 			    =	SEC_ERROR_BASE + 5,
SEC_ERROR_BAD_DER 			    =	SEC_ERROR_BASE + 9,
SEC_ERROR_NO_MEMORY 			    =	SEC_ERROR_BASE + 19,
SEC_ERROR_BAD_TEMPLATE			    =	(SEC_ERROR_BASE + 136),
SEC_ERROR_EXTRA_INPUT                       =   (SEC_ERROR_BASE + 140),

/* Add new error codes above here. */
SEC_ERROR_END_OF_LIST 
} SECErrorCodes;
#endif /* NO_SECURITY_ERROR_ENUM */

/* ========== <nss3/secport.h> */

extern void PORT_SetError(int value);
extern int PORT_GetError(void);
extern void *PORT_ArenaZAlloc(PLArenaPool *arena, size_t size);

SEC_BEGIN_PROTOS

/*
 * Load a shared library called "newShLibName" in the same directory as
 * a shared library that is already loaded, called existingShLibName.
 * A pointer to a static function in that shared library,
 * staticShLibFunc, is required.
 *
 * existingShLibName:
 *   The file name of the shared library that shall be used as the 
 *   "reference library". The loader will attempt to load the requested
 *   library from the same directory as the reference library.
 *
 * staticShLibFunc:
 *   Pointer to a static function in the "reference library".
 *
 * newShLibName:
 *   The simple file name of the new shared library to be loaded.
 *
 * We use PR_GetLibraryFilePathname to get the pathname of the loaded 
 * shared lib that contains this function, and then do a
 * PR_LoadLibraryWithFlags with an absolute pathname for the shared
 * library to be loaded.
 *
 * On Windows, the "alternate search path" strategy is employed, if available.
 * On Unix, if existingShLibName is a symbolic link, and no link exists for the
 * new library, the original link will be resolved, and the new library loaded
 * from the resolved location.
 *
 * If the new shared library is not found in the same location as the reference
 * library, it will then be loaded from the normal system library path.
 */
PRLibrary *
PORT_LoadLibraryFromOrigin(const char* existingShLibName,
                 PRFuncPtr staticShLibFunc,
                 const char *newShLibName);

SEC_END_PROTOS

/* ========== <nss3/seccomon.h> */

typedef enum {
    siBuffer = 0,
    siClearDataBuffer = 1,
    siCipherDataBuffer = 2,
    siDERCertBuffer = 3,
    siEncodedCertBuffer = 4,
    siDERNameBuffer = 5,
    siEncodedNameBuffer = 6,
    siAsciiNameString = 7,
    siAsciiString = 8,
    siDEROID = 9,
    siUnsignedInteger = 10,
    siUTCTime = 11,
    siGeneralizedTime = 12,
    siVisibleString = 13,
    siUTF8String = 14,
    siBMPString = 15
} SECItemType;

typedef struct SECItemStr SECItem;
struct SECItemStr {
    SECItemType type;
    unsigned char *data;
    unsigned int len;
};

/*
** A status code. Status's are used by procedures that return status
** values. Again the motivation is so that a compiler can generate
** warnings when return values are wrong. Correct testing of status codes:
**
**	SECStatus rv;
**	rv = some_function (some_argument);
**	if (rv != SECSuccess)
**		do_an_error_thing();
**
*/
typedef enum _SECStatus {
    SECWouldBlock = -2,
    SECFailure = -1,
    SECSuccess = 0
} SECStatus;

/*
** A comparison code. Used for procedures that return comparision
** values. Again the motivation is so that a compiler can generate
** warnings when return values are wrong.
*/
typedef enum _SECComparison {
    SECLessThan = -1,
    SECEqual = 0,
    SECGreaterThan = 1
} SECComparison;

/* ========== <nss3/secasn1t.h> */
/*
** An array of these structures defines a BER/DER encoding for an object.
**
** The array usually starts with a dummy entry whose kind is SEC_ASN1_SEQUENCE;
** such an array is terminated with an entry where kind == 0.  (An array
** which consists of a single component does not require a second dummy
** entry -- the array is only searched as long as previous component(s)
** instruct it.)
*/
typedef struct sec_ASN1Template_struct {
    /*
    ** Kind of item being decoded/encoded, including tags and modifiers.
    */
    unsigned long kind;

    /*
    ** The value is the offset from the base of the structure to the
    ** field that holds the value being decoded/encoded.
    */
    unsigned long offset;

    /*
    ** When kind suggests it (SEC_ASN1_POINTER, SEC_ASN1_GROUP, SEC_ASN1_INLINE,
    ** or a component that is *not* a SEC_ASN1_UNIVERSAL), this points to
    ** a sub-template for nested encoding/decoding,
    ** OR, iff SEC_ASN1_DYNAMIC is set, then this is a pointer to a pointer
    ** to a function which will return the appropriate template when called
    ** at runtime.  NOTE! that explicit level of indirection, which is
    ** necessary because ANSI does not allow you to store a function
    ** pointer directly as a "void *" so we must store it separately and
    ** dereference it to get at the function pointer itself.
    */
    const void *sub;

    /*
    ** In the first element of a template array, the value is the size
    ** of the structure to allocate when this template is being referenced
    ** by another template via SEC_ASN1_POINTER or SEC_ASN1_GROUP.
    ** In all other cases, the value is ignored.
    */
    unsigned int size;
} SEC_ASN1Template;


/* default size used for allocation of encoding/decoding stuff */
/* XXX what is the best value here? */
#define SEC_ASN1_DEFAULT_ARENA_SIZE	(2048)

/*
** BER/DER values for ASN.1 identifier octets.
*/
#define SEC_ASN1_TAG_MASK		0xff

/*
 * BER/DER universal type tag numbers.
 * The values are defined by the X.208 standard; do not change them!
 * NOTE: if you add anything to this list, you must add code to secasn1d.c
 * to accept the tag, and probably also to secasn1e.c to encode it.
 * XXX It appears some have been added recently without being added to
 * the code; so need to go through the list now and double-check them all.
 * (Look especially at those added in revision 1.10.)
 */
#define SEC_ASN1_TAGNUM_MASK		0x1f
#define SEC_ASN1_BOOLEAN		0x01
#define SEC_ASN1_INTEGER		0x02
#define SEC_ASN1_BIT_STRING		0x03
#define SEC_ASN1_OCTET_STRING		0x04
#define SEC_ASN1_NULL			0x05
#define SEC_ASN1_OBJECT_ID		0x06
#define SEC_ASN1_OBJECT_DESCRIPTOR      0x07
/* External type and instance-of type   0x08 */
#define SEC_ASN1_REAL                   0x09
#define SEC_ASN1_ENUMERATED		0x0a
#define SEC_ASN1_EMBEDDED_PDV           0x0b
#define SEC_ASN1_UTF8_STRING		0x0c
/*                                      0x0d */
/*                                      0x0e */
/*                                      0x0f */
#define SEC_ASN1_SEQUENCE		0x10
#define SEC_ASN1_SET			0x11
#define SEC_ASN1_NUMERIC_STRING         0x12
#define SEC_ASN1_PRINTABLE_STRING	0x13
#define SEC_ASN1_T61_STRING		0x14
#define SEC_ASN1_VIDEOTEX_STRING        0x15
#define SEC_ASN1_IA5_STRING		0x16
#define SEC_ASN1_UTC_TIME		0x17
#define SEC_ASN1_GENERALIZED_TIME	0x18
#define SEC_ASN1_GRAPHIC_STRING         0x19
#define SEC_ASN1_VISIBLE_STRING		0x1a
#define SEC_ASN1_GENERAL_STRING         0x1b
#define SEC_ASN1_UNIVERSAL_STRING	0x1c
/*                                      0x1d */
#define SEC_ASN1_BMP_STRING		0x1e
#define SEC_ASN1_HIGH_TAG_NUMBER	0x1f
#define SEC_ASN1_TELETEX_STRING 	SEC_ASN1_T61_STRING

/*
** Modifiers to type tags.  These are also specified by a/the
** standard, and must not be changed.
*/

#define SEC_ASN1_METHOD_MASK		0x20
#define SEC_ASN1_PRIMITIVE		0x00
#define SEC_ASN1_CONSTRUCTED		0x20

#define SEC_ASN1_CLASS_MASK		0xc0
#define SEC_ASN1_UNIVERSAL		0x00
#define SEC_ASN1_APPLICATION		0x40
#define SEC_ASN1_CONTEXT_SPECIFIC	0x80
#define SEC_ASN1_PRIVATE		0xc0

/*
** Our additions, used for templates.
** These are not defined by any standard; the values are used internally only.
** Just be careful to keep them out of the low 8 bits.
** XXX finish comments
*/
#define SEC_ASN1_OPTIONAL	0x00100
#define SEC_ASN1_EXPLICIT	0x00200
#define SEC_ASN1_ANY		0x00400
#define SEC_ASN1_INLINE		0x00800
#define SEC_ASN1_POINTER	0x01000
#define SEC_ASN1_GROUP		0x02000	/* with SET or SEQUENCE means
					 * SET OF or SEQUENCE OF */
#define SEC_ASN1_DYNAMIC	0x04000 /* subtemplate is found by calling
					 * a function at runtime */
#define SEC_ASN1_SKIP		0x08000 /* skip a field; only for decoding */
#define SEC_ASN1_INNER		0x10000	/* with ANY means capture the
					 * contents only (not the id, len,
					 * or eoc); only for decoding */
#define SEC_ASN1_SAVE		0x20000 /* stash away the encoded bytes first;
					 * only for decoding */
#define SEC_ASN1_MAY_STREAM	0x40000	/* field or one of its sub-fields may
					 * stream in and so should encode as
					 * indefinite-length when streaming
					 * has been indicated; only for
					 * encoding */
#define SEC_ASN1_SKIP_REST	0x80000	/* skip all following fields;
					   only for decoding */
#define SEC_ASN1_CHOICE        0x100000 /* pick one from a template */
#define SEC_ASN1_NO_STREAM     0X200000 /* This entry will not stream
                                           even if the sub-template says
                                           streaming is possible.  Helps
                                           to solve ambiguities with potential
                                           streaming entries that are 
                                           optional */
#define SEC_ASN1_DEBUG_BREAK   0X400000 /* put this in your template and the
                                           decoder will assert when it
                                           processes it. Only for use with
                                           SEC_QuickDERDecodeItem */

                                          

/* Shorthand/Aliases */
#define SEC_ASN1_SEQUENCE_OF	(SEC_ASN1_GROUP | SEC_ASN1_SEQUENCE)
#define SEC_ASN1_SET_OF		(SEC_ASN1_GROUP | SEC_ASN1_SET)
#define SEC_ASN1_ANY_CONTENTS	(SEC_ASN1_ANY | SEC_ASN1_INNER)

/* Maximum depth of nested SEQUENCEs and SETs */
#define SEC_ASN1D_MAX_DEPTH 32

/*
** Function used for SEC_ASN1_DYNAMIC.
** "arg" is a pointer to the structure being encoded/decoded
** "enc", when true, means that we are encoding (false means decoding)
*/
typedef const SEC_ASN1Template * SEC_ASN1TemplateChooser(void *arg, PRBool enc);
typedef SEC_ASN1TemplateChooser * SEC_ASN1TemplateChooserPtr;

#if defined(_WIN32) || defined(ANDROID)
#define SEC_ASN1_GET(x)        NSS_Get_##x(NULL, PR_FALSE)
#define SEC_ASN1_SUB(x)        &p_NSS_Get_##x
#define SEC_ASN1_XTRN          SEC_ASN1_DYNAMIC
#define SEC_ASN1_MKSUB(x) \
static const SEC_ASN1TemplateChooserPtr p_NSS_Get_##x = &NSS_Get_##x;
#else
#define SEC_ASN1_GET(x)        x
#define SEC_ASN1_SUB(x)        x
#define SEC_ASN1_XTRN          0
#define SEC_ASN1_MKSUB(x) 
#endif

#define SEC_ASN1_CHOOSER_DECLARE(x) \
extern const SEC_ASN1Template * NSS_Get_##x (void *arg, PRBool enc);

#define SEC_ASN1_CHOOSER_IMPLEMENT(x) \
const SEC_ASN1Template * NSS_Get_##x(void * arg, PRBool enc) \
{ return x; }

/*
** Opaque object used by the decoder to store state.
*/
typedef struct sec_DecoderContext_struct SEC_ASN1DecoderContext;

/*
** Opaque object used by the encoder to store state.
*/
typedef struct sec_EncoderContext_struct SEC_ASN1EncoderContext;

/*
 * This is used to describe to a filter function the bytes that are
 * being passed to it.  This is only useful when the filter is an "outer"
 * one, meaning it expects to get *all* of the bytes not just the
 * contents octets.
 */
typedef enum {
    SEC_ASN1_Identifier = 0,
    SEC_ASN1_Length = 1,
    SEC_ASN1_Contents = 2,
    SEC_ASN1_EndOfContents = 3
} SEC_ASN1EncodingPart;

/*
 * Type of the function pointer used either for decoding or encoding,
 * when doing anything "funny" (e.g. manipulating the data stream)
 */ 
typedef void (* SEC_ASN1NotifyProc)(void *arg, PRBool before,
				    void *dest, int real_depth);

/*
 * Type of the function pointer used for grabbing encoded bytes.
 * This can be used during either encoding or decoding, as follows...
 *
 * When decoding, this can be used to filter the encoded bytes as they
 * are parsed.  This is what you would do if you wanted to process the data
 * along the way (like to decrypt it, or to perform a hash on it in order
 * to do a signature check later).  See SEC_ASN1DecoderSetFilterProc().
 * When processing only part of the encoded bytes is desired, you "watch"
 * for the field(s) you are interested in with a "notify proc" (see
 * SEC_ASN1DecoderSetNotifyProc()) and for even finer granularity (e.g. to
 * ignore all by the contents bytes) you pay attention to the "data_kind"
 * parameter.
 *
 * When encoding, this is the specification for the output function which
 * will receive the bytes as they are encoded.  The output function can
 * perform any postprocessing necessary (like hashing (some of) the data
 * to create a digest that gets included at the end) as well as shoving
 * the data off wherever it needs to go.  (In order to "tune" any processing,
 * you can set a "notify proc" as described above in the decoding case.)
 *
 * The parameters:
 * - "arg" is an opaque pointer that you provided at the same time you
 *   specified a function of this type
 * - "data" is a buffer of length "len", containing the encoded bytes
 * - "depth" is how deep in a nested encoding we are (it is not usually
 *   valuable, but can be useful sometimes so I included it)
 * - "data_kind" tells you if these bytes are part of the ASN.1 encoded
 *   octets for identifier, length, contents, or end-of-contents
 */ 
typedef void (* SEC_ASN1WriteProc)(void *arg,
				   const char *data, unsigned long len,
				   int depth, SEC_ASN1EncodingPart data_kind);
/* ========== */

/*
** An X.500 algorithm identifier
*/
typedef struct SECAlgorithmIDStr SECAlgorithmID;
struct SECAlgorithmIDStr {
    SECItem algorithm;
    SECItem parameters;
};

/* ========== nss/lib/util/templates.c */

const SEC_ASN1Template SECOID_AlgorithmIDTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};
SEC_ASN1_CHOOSER_DECLARE(SECOID_AlgorithmIDTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SECOID_AlgorithmIDTemplate)

const SEC_ASN1Template SEC_AnyTemplate[] = {
    { SEC_ASN1_ANY | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_AnyTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_AnyTemplate)

const SEC_ASN1Template SEC_BMPStringTemplate[] = {
    { SEC_ASN1_BMP_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_BMPStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BMPStringTemplate)

const SEC_ASN1Template SEC_BitStringTemplate[] = {
    { SEC_ASN1_BIT_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_BitStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BitStringTemplate)

const SEC_ASN1Template SEC_BooleanTemplate[] = {
    { SEC_ASN1_BOOLEAN, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_BooleanTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BooleanTemplate)

const SEC_ASN1Template SEC_GeneralizedTimeTemplate[] = {
    { SEC_ASN1_GENERALIZED_TIME | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};
SEC_ASN1_CHOOSER_DECLARE(SEC_GeneralizedTimeTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_GeneralizedTimeTemplate)

const SEC_ASN1Template SEC_IA5StringTemplate[] = {
    { SEC_ASN1_IA5_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_IA5StringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_IA5StringTemplate)

const SEC_ASN1Template SEC_IntegerTemplate[] = {
    { SEC_ASN1_INTEGER, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_IntegerTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_IntegerTemplate)

const SEC_ASN1Template SEC_NullTemplate[] = {
    { SEC_ASN1_NULL, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_NullTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_NullTemplate)

const SEC_ASN1Template SEC_ObjectIDTemplate[] = {
    { SEC_ASN1_OBJECT_ID, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_ObjectIDTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_ObjectIDTemplate)

const SEC_ASN1Template SEC_OctetStringTemplate[] = {
    { SEC_ASN1_OCTET_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_OctetStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_OctetStringTemplate)

const SEC_ASN1Template SEC_PointerToAnyTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_AnyTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToAnyTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToAnyTemplate)

const SEC_ASN1Template SEC_PointerToOctetStringTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_MAY_STREAM, 0, SEC_OctetStringTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToOctetStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToOctetStringTemplate)

const SEC_ASN1Template SEC_SetOfAnyTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_AnyTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_SetOfAnyTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SetOfAnyTemplate)

const SEC_ASN1Template SEC_UTCTimeTemplate[] = {
    { SEC_ASN1_UTC_TIME | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_UTCTimeTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UTCTimeTemplate)

const SEC_ASN1Template SEC_UTF8StringTemplate[] = {
    { SEC_ASN1_UTF8_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};
SEC_ASN1_CHOOSER_DECLARE(SEC_UTF8StringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UTF8StringTemplate)

/* ========== nss/lib/util/secasn1d.c */
const SEC_ASN1Template SEC_SequenceOfAnyTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_AnyTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_SequenceOfAnyTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SequenceOfAnyTemplate)

const SEC_ASN1Template SEC_EnumeratedTemplate[] = {
    { SEC_ASN1_ENUMERATED, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_EnumeratedTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_EnumeratedTemplate)

const SEC_ASN1Template SEC_PointerToEnumeratedTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_EnumeratedTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToEnumeratedTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToEnumeratedTemplate)

const SEC_ASN1Template SEC_SetOfEnumeratedTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_EnumeratedTemplate }
};

const SEC_ASN1Template SEC_PointerToGeneralizedTimeTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_GeneralizedTimeTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_PointerToGeneralizedTimeTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToGeneralizedTimeTemplate)

const SEC_ASN1Template SEC_SequenceOfObjectIDTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, SEC_ObjectIDTemplate }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_SequenceOfObjectIDTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SequenceOfObjectIDTemplate)

const SEC_ASN1Template SEC_PrintableStringTemplate[] = {
    { SEC_ASN1_PRINTABLE_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};
SEC_ASN1_CHOOSER_DECLARE(SEC_PrintableStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PrintableStringTemplate)

const SEC_ASN1Template SEC_T61StringTemplate[] = {
    { SEC_ASN1_T61_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_T61StringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_T61StringTemplate)

const SEC_ASN1Template SEC_UniversalStringTemplate[] = {
    { SEC_ASN1_UNIVERSAL_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};
SEC_ASN1_CHOOSER_DECLARE(SEC_UniversalStringTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UniversalStringTemplate)

const SEC_ASN1Template SEC_VisibleStringTemplate[] = {
    { SEC_ASN1_VISIBLE_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

/*
 * Template for skipping a subitem.
 *
 * Note that it only makes sense to use this for decoding (when you want
 * to decode something where you are only interested in one or two of
 * the fields); you cannot encode a SKIP!
 */
const SEC_ASN1Template SEC_SkipTemplate[] = {
    { SEC_ASN1_SKIP }
};
SEC_ASN1_CHOOSER_DECLARE(SEC_SkipTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SkipTemplate)

/* ========== <nss3/secasn1.h> */
/************************************************************************/
SEC_BEGIN_PROTOS

extern SECStatus SEC_QuickDERDecodeItem(PLArenaPool* arena, void* dest,
                     const SEC_ASN1Template* templateEntry,
                     const SECItem* src);

/*
** Utilities.
*/

/*
 * We have a length that needs to be encoded; how many bytes will the
 * encoding take?
 */
extern int SEC_ASN1LengthLength (unsigned long len);

/*
 * Find the appropriate subtemplate for the given template.
 * This may involve calling a "chooser" function, or it may just
 * be right there.  In either case, it is expected to *have* a
 * subtemplate; this is asserted in debug builds (in non-debug
 * builds, NULL will be returned).
 *
 * "thing" is a pointer to the structure being encoded/decoded
 * "encoding", when true, means that we are in the process of encoding
 *	(as opposed to in the process of decoding)
 */
extern const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *inTemplate, void *thing,
			PRBool encoding);

/* whether the template is for a primitive type or a choice of
 * primitive types
 */
extern PRBool SEC_ASN1IsTemplateSimple(const SEC_ASN1Template *theTemplate);

/************************************************************************/

#ifdef	DYING
/*
 * Generic Templates
 * One for each of the simple types, plus a special one for ANY, plus:
 *	- a pointer to each one of those
 *	- a set of each one of those
 *	- a sequence of each one of those
 *
 * Note that these are alphabetical (case insensitive); please add new
 * ones in the appropriate place.
 */

extern const SEC_ASN1Template SEC_PointerToBitStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToBMPStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToBooleanTemplate[];
extern const SEC_ASN1Template SEC_PointerToIA5StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToIntegerTemplate[];
extern const SEC_ASN1Template SEC_PointerToNullTemplate[];
extern const SEC_ASN1Template SEC_PointerToObjectIDTemplate[];
extern const SEC_ASN1Template SEC_PointerToPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_PointerToT61StringTemplate[];
extern const SEC_ASN1Template SEC_PointerToUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_PointerToUTF8StringTemplate[];

extern const SEC_ASN1Template SEC_SequenceOfAnyTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBitStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBMPStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfBooleanTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfGeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfIA5StringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfIntegerTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfNullTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfObjectIDTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfOctetStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfT61StringTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_SequenceOfUTF8StringTemplate[];

extern const SEC_ASN1Template SEC_SetOfBitStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfBMPStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfBooleanTemplate[];
extern const SEC_ASN1Template SEC_SetOfGeneralizedTimeTemplate[];
extern const SEC_ASN1Template SEC_SetOfIA5StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfIntegerTemplate[];
extern const SEC_ASN1Template SEC_SetOfNullTemplate[];
extern const SEC_ASN1Template SEC_SetOfObjectIDTemplate[];
extern const SEC_ASN1Template SEC_SetOfOctetStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfPrintableStringTemplate[];
extern const SEC_ASN1Template SEC_SetOfT61StringTemplate[];
extern const SEC_ASN1Template SEC_SetOfUTCTimeTemplate[];
extern const SEC_ASN1Template SEC_SetOfUTF8StringTemplate[];
#endif	/* DYING */

SEC_END_PROTOS

/* ========== <nss3/secder.h> */

/* ========== <nss3/secoidt.h> */

/*
 * Misc object IDs - these numbers are for convenient handling.
 * They are mapped into real object IDs
 *
 * NOTE: the order of these entries must mach the array "oids" of SECOidData
 * in util/secoid.c.
 */
typedef enum {
    SEC_OID_UNKNOWN = 0,
    SEC_OID_MD2 = 1,
    SEC_OID_MD4 = 2,
    SEC_OID_MD5 = 3,
    SEC_OID_SHA1 = 4,
    SEC_OID_RC2_CBC = 5,
    SEC_OID_RC4 = 6,
    SEC_OID_DES_EDE3_CBC = 7,
    SEC_OID_RC5_CBC_PAD = 8,
    SEC_OID_DES_ECB = 9,
    SEC_OID_DES_CBC = 10,
    SEC_OID_DES_OFB = 11,
    SEC_OID_DES_CFB = 12,
    SEC_OID_DES_MAC = 13,
    SEC_OID_DES_EDE = 14,
    SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE = 15,
    SEC_OID_PKCS1_RSA_ENCRYPTION = 16,
    SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION = 17,
    SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION = 18,
    SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION = 19,
    SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION = 20,
    SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC = 21,
    SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC = 22,
    SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC = 23,
    SEC_OID_PKCS7 = 24,
    SEC_OID_PKCS7_DATA = 25,
    SEC_OID_PKCS7_SIGNED_DATA = 26,
    SEC_OID_PKCS7_ENVELOPED_DATA = 27,
    SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA = 28,
    SEC_OID_PKCS7_DIGESTED_DATA = 29,
    SEC_OID_PKCS7_ENCRYPTED_DATA = 30,
    SEC_OID_PKCS9_EMAIL_ADDRESS = 31,
    SEC_OID_PKCS9_UNSTRUCTURED_NAME = 32,
    SEC_OID_PKCS9_CONTENT_TYPE = 33,
    SEC_OID_PKCS9_MESSAGE_DIGEST = 34,
    SEC_OID_PKCS9_SIGNING_TIME = 35,
    SEC_OID_PKCS9_COUNTER_SIGNATURE = 36,
    SEC_OID_PKCS9_CHALLENGE_PASSWORD = 37,
    SEC_OID_PKCS9_UNSTRUCTURED_ADDRESS = 38,
    SEC_OID_PKCS9_EXTENDED_CERTIFICATE_ATTRIBUTES = 39,
    SEC_OID_PKCS9_SMIME_CAPABILITIES = 40,
    SEC_OID_AVA_COMMON_NAME = 41,
    SEC_OID_AVA_COUNTRY_NAME = 42,
    SEC_OID_AVA_LOCALITY = 43,
    SEC_OID_AVA_STATE_OR_PROVINCE = 44,
    SEC_OID_AVA_ORGANIZATION_NAME = 45,
    SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME = 46,
    SEC_OID_AVA_DN_QUALIFIER = 47,
    SEC_OID_AVA_DC = 48,

    SEC_OID_NS_TYPE_GIF = 49,
    SEC_OID_NS_TYPE_JPEG = 50,
    SEC_OID_NS_TYPE_URL = 51,
    SEC_OID_NS_TYPE_HTML = 52,
    SEC_OID_NS_TYPE_CERT_SEQUENCE = 53,
    SEC_OID_MISSI_KEA_DSS_OLD = 54,
    SEC_OID_MISSI_DSS_OLD = 55,
    SEC_OID_MISSI_KEA_DSS = 56,
    SEC_OID_MISSI_DSS = 57,
    SEC_OID_MISSI_KEA = 58,
    SEC_OID_MISSI_ALT_KEA = 59,

    /* Netscape private certificate extensions */
    SEC_OID_NS_CERT_EXT_NETSCAPE_OK = 60,
    SEC_OID_NS_CERT_EXT_ISSUER_LOGO = 61,
    SEC_OID_NS_CERT_EXT_SUBJECT_LOGO = 62,
    SEC_OID_NS_CERT_EXT_CERT_TYPE = 63,
    SEC_OID_NS_CERT_EXT_BASE_URL = 64,
    SEC_OID_NS_CERT_EXT_REVOCATION_URL = 65,
    SEC_OID_NS_CERT_EXT_CA_REVOCATION_URL = 66,
    SEC_OID_NS_CERT_EXT_CA_CRL_URL = 67,
    SEC_OID_NS_CERT_EXT_CA_CERT_URL = 68,
    SEC_OID_NS_CERT_EXT_CERT_RENEWAL_URL = 69,
    SEC_OID_NS_CERT_EXT_CA_POLICY_URL = 70,
    SEC_OID_NS_CERT_EXT_HOMEPAGE_URL = 71,
    SEC_OID_NS_CERT_EXT_ENTITY_LOGO = 72,
    SEC_OID_NS_CERT_EXT_USER_PICTURE = 73,
    SEC_OID_NS_CERT_EXT_SSL_SERVER_NAME = 74,
    SEC_OID_NS_CERT_EXT_COMMENT = 75,
    SEC_OID_NS_CERT_EXT_LOST_PASSWORD_URL = 76,
    SEC_OID_NS_CERT_EXT_CERT_RENEWAL_TIME = 77,
    SEC_OID_NS_KEY_USAGE_GOVT_APPROVED = 78,

    /* x.509 v3 Extensions */
    SEC_OID_X509_SUBJECT_DIRECTORY_ATTR = 79,
    SEC_OID_X509_SUBJECT_KEY_ID = 80,
    SEC_OID_X509_KEY_USAGE = 81,
    SEC_OID_X509_PRIVATE_KEY_USAGE_PERIOD = 82,
    SEC_OID_X509_SUBJECT_ALT_NAME = 83,
    SEC_OID_X509_ISSUER_ALT_NAME = 84,
    SEC_OID_X509_BASIC_CONSTRAINTS = 85,
    SEC_OID_X509_NAME_CONSTRAINTS = 86,
    SEC_OID_X509_CRL_DIST_POINTS = 87,
    SEC_OID_X509_CERTIFICATE_POLICIES = 88,
    SEC_OID_X509_POLICY_MAPPINGS = 89,
    SEC_OID_X509_POLICY_CONSTRAINTS = 90,
    SEC_OID_X509_AUTH_KEY_ID = 91,
    SEC_OID_X509_EXT_KEY_USAGE = 92,
    SEC_OID_X509_AUTH_INFO_ACCESS = 93,

    SEC_OID_X509_CRL_NUMBER = 94,
    SEC_OID_X509_REASON_CODE = 95,
    SEC_OID_X509_INVALID_DATE = 96,
    /* End of x.509 v3 Extensions */    

    SEC_OID_X500_RSA_ENCRYPTION = 97,

    /* alg 1485 additions */
    SEC_OID_RFC1274_UID = 98,
    SEC_OID_RFC1274_MAIL = 99,

    /* PKCS 12 additions */
    SEC_OID_PKCS12 = 100,
    SEC_OID_PKCS12_MODE_IDS = 101,
    SEC_OID_PKCS12_ESPVK_IDS = 102,
    SEC_OID_PKCS12_BAG_IDS = 103,
    SEC_OID_PKCS12_CERT_BAG_IDS = 104,
    SEC_OID_PKCS12_OIDS = 105,
    SEC_OID_PKCS12_PBE_IDS = 106,
    SEC_OID_PKCS12_SIGNATURE_IDS = 107,
    SEC_OID_PKCS12_ENVELOPING_IDS = 108,
   /* SEC_OID_PKCS12_OFFLINE_TRANSPORT_MODE,
    SEC_OID_PKCS12_ONLINE_TRANSPORT_MODE, */
    SEC_OID_PKCS12_PKCS8_KEY_SHROUDING = 109,
    SEC_OID_PKCS12_KEY_BAG_ID = 110,
    SEC_OID_PKCS12_CERT_AND_CRL_BAG_ID = 111,
    SEC_OID_PKCS12_SECRET_BAG_ID = 112,
    SEC_OID_PKCS12_X509_CERT_CRL_BAG = 113,
    SEC_OID_PKCS12_SDSI_CERT_BAG = 114,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4 = 115,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4 = 116,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC = 117,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC = 118,
    SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC = 119,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_128_BIT_RC4 = 120,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_40_BIT_RC4 = 121,
    SEC_OID_PKCS12_RSA_ENCRYPTION_WITH_TRIPLE_DES = 122,
    SEC_OID_PKCS12_RSA_SIGNATURE_WITH_SHA1_DIGEST = 123,
    /* end of PKCS 12 additions */

    /* DSA signatures */
    SEC_OID_ANSIX9_DSA_SIGNATURE = 124,
    SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST = 125,
    SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST = 126,

    /* Verisign OIDs */
    SEC_OID_VERISIGN_USER_NOTICES = 127,

    /* PKIX OIDs */
    SEC_OID_PKIX_CPS_POINTER_QUALIFIER = 128,
    SEC_OID_PKIX_USER_NOTICE_QUALIFIER = 129,
    SEC_OID_PKIX_OCSP = 130,
    SEC_OID_PKIX_OCSP_BASIC_RESPONSE = 131,
    SEC_OID_PKIX_OCSP_NONCE = 132,
    SEC_OID_PKIX_OCSP_CRL = 133,
    SEC_OID_PKIX_OCSP_RESPONSE = 134,
    SEC_OID_PKIX_OCSP_NO_CHECK = 135,
    SEC_OID_PKIX_OCSP_ARCHIVE_CUTOFF = 136,
    SEC_OID_PKIX_OCSP_SERVICE_LOCATOR = 137,
    SEC_OID_PKIX_REGCTRL_REGTOKEN = 138,
    SEC_OID_PKIX_REGCTRL_AUTHENTICATOR = 139,
    SEC_OID_PKIX_REGCTRL_PKIPUBINFO = 140,
    SEC_OID_PKIX_REGCTRL_PKI_ARCH_OPTIONS = 141,
    SEC_OID_PKIX_REGCTRL_OLD_CERT_ID = 142,
    SEC_OID_PKIX_REGCTRL_PROTOCOL_ENC_KEY = 143,
    SEC_OID_PKIX_REGINFO_UTF8_PAIRS = 144,
    SEC_OID_PKIX_REGINFO_CERT_REQUEST = 145,
    SEC_OID_EXT_KEY_USAGE_SERVER_AUTH = 146,
    SEC_OID_EXT_KEY_USAGE_CLIENT_AUTH = 147,
    SEC_OID_EXT_KEY_USAGE_CODE_SIGN = 148,
    SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT = 149,
    SEC_OID_EXT_KEY_USAGE_TIME_STAMP = 150,
    SEC_OID_OCSP_RESPONDER = 151,

    /* Netscape Algorithm OIDs */
    SEC_OID_NETSCAPE_SMIME_KEA = 152,

    /* Skipjack OID -- ### mwelch temporary */
    SEC_OID_FORTEZZA_SKIPJACK = 153,

    /* PKCS 12 V2 oids */
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4 = 154,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4 = 155,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC = 156,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC = 157,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC = 158,
    SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC = 159,
    SEC_OID_PKCS12_SAFE_CONTENTS_ID = 160,
    SEC_OID_PKCS12_PKCS8_SHROUDED_KEY_BAG_ID = 161,

    SEC_OID_PKCS12_V1_KEY_BAG_ID = 162,
    SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID = 163,
    SEC_OID_PKCS12_V1_CERT_BAG_ID = 164,
    SEC_OID_PKCS12_V1_CRL_BAG_ID = 165,
    SEC_OID_PKCS12_V1_SECRET_BAG_ID = 166,
    SEC_OID_PKCS12_V1_SAFE_CONTENTS_BAG_ID = 167,
    SEC_OID_PKCS9_X509_CERT = 168,
    SEC_OID_PKCS9_SDSI_CERT = 169,
    SEC_OID_PKCS9_X509_CRL = 170,
    SEC_OID_PKCS9_FRIENDLY_NAME = 171,
    SEC_OID_PKCS9_LOCAL_KEY_ID = 172,
    SEC_OID_BOGUS_KEY_USAGE = 173,

    /*Diffe Helman OIDS */
    SEC_OID_X942_DIFFIE_HELMAN_KEY = 174,

    /* Netscape other name types */
    /* SEC_OID_NETSCAPE_NICKNAME is an otherName field of type IA5String
     * in the subjectAltName certificate extension.  NSS dropped support
     * for SEC_OID_NETSCAPE_NICKNAME in NSS 3.13. */
    SEC_OID_NETSCAPE_NICKNAME = 175,

    /* Cert Server OIDS */
    SEC_OID_NETSCAPE_RECOVERY_REQUEST = 176,

    /* New PSM certificate management OIDs */
    SEC_OID_CERT_RENEWAL_LOCATOR = 177,
    SEC_OID_NS_CERT_EXT_SCOPE_OF_USE = 178,
    
    /* CMS (RFC2630) OIDs */
    SEC_OID_CMS_EPHEMERAL_STATIC_DIFFIE_HELLMAN = 179,
    SEC_OID_CMS_3DES_KEY_WRAP = 180,
    SEC_OID_CMS_RC2_KEY_WRAP = 181,

    /* SMIME attributes */
    SEC_OID_SMIME_ENCRYPTION_KEY_PREFERENCE = 182,

    /* AES OIDs */
    SEC_OID_AES_128_ECB 	= 183,
    SEC_OID_AES_128_CBC 	= 184,
    SEC_OID_AES_192_ECB 	= 185,
    SEC_OID_AES_192_CBC 	= 186,
    SEC_OID_AES_256_ECB 	= 187,
    SEC_OID_AES_256_CBC 	= 188,

    SEC_OID_SDN702_DSA_SIGNATURE = 189,

    SEC_OID_MS_SMIME_ENCRYPTION_KEY_PREFERENCE = 190,

    SEC_OID_SHA256              = 191,
    SEC_OID_SHA384              = 192,
    SEC_OID_SHA512              = 193,

    SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION = 194,
    SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION = 195,
    SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION = 196,

    SEC_OID_AES_128_KEY_WRAP	= 197,
    SEC_OID_AES_192_KEY_WRAP	= 198,
    SEC_OID_AES_256_KEY_WRAP	= 199,

    /* Elliptic Curve Cryptography (ECC) OIDs */
    SEC_OID_ANSIX962_EC_PUBLIC_KEY  = 200,
    SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE = 201,

#define SEC_OID_ANSIX962_ECDSA_SIGNATURE_WITH_SHA1_DIGEST \
	SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE

    /* ANSI X9.62 named elliptic curves (prime field) */
    SEC_OID_ANSIX962_EC_PRIME192V1  = 202,
    SEC_OID_ANSIX962_EC_PRIME192V2  = 203,
    SEC_OID_ANSIX962_EC_PRIME192V3  = 204,
    SEC_OID_ANSIX962_EC_PRIME239V1  = 205,
    SEC_OID_ANSIX962_EC_PRIME239V2  = 206,
    SEC_OID_ANSIX962_EC_PRIME239V3  = 207,
    SEC_OID_ANSIX962_EC_PRIME256V1  = 208,

    /* SECG named elliptic curves (prime field) */
    SEC_OID_SECG_EC_SECP112R1       = 209,
    SEC_OID_SECG_EC_SECP112R2       = 210,
    SEC_OID_SECG_EC_SECP128R1       = 211,
    SEC_OID_SECG_EC_SECP128R2       = 212,
    SEC_OID_SECG_EC_SECP160K1       = 213,
    SEC_OID_SECG_EC_SECP160R1       = 214, 
    SEC_OID_SECG_EC_SECP160R2       = 215,
    SEC_OID_SECG_EC_SECP192K1       = 216,
    /* SEC_OID_SECG_EC_SECP192R1 is SEC_OID_ANSIX962_EC_PRIME192V1 */
    SEC_OID_SECG_EC_SECP224K1       = 217,
    SEC_OID_SECG_EC_SECP224R1       = 218,
    SEC_OID_SECG_EC_SECP256K1       = 219,
    /* SEC_OID_SECG_EC_SECP256R1 is SEC_OID_ANSIX962_EC_PRIME256V1 */
    SEC_OID_SECG_EC_SECP384R1       = 220,
    SEC_OID_SECG_EC_SECP521R1       = 221,

    /* ANSI X9.62 named elliptic curves (characteristic two field) */
    SEC_OID_ANSIX962_EC_C2PNB163V1  = 222,
    SEC_OID_ANSIX962_EC_C2PNB163V2  = 223,
    SEC_OID_ANSIX962_EC_C2PNB163V3  = 224,
    SEC_OID_ANSIX962_EC_C2PNB176V1  = 225,
    SEC_OID_ANSIX962_EC_C2TNB191V1  = 226,
    SEC_OID_ANSIX962_EC_C2TNB191V2  = 227,
    SEC_OID_ANSIX962_EC_C2TNB191V3  = 228,
    SEC_OID_ANSIX962_EC_C2ONB191V4  = 229,
    SEC_OID_ANSIX962_EC_C2ONB191V5  = 230,
    SEC_OID_ANSIX962_EC_C2PNB208W1  = 231,
    SEC_OID_ANSIX962_EC_C2TNB239V1  = 232,
    SEC_OID_ANSIX962_EC_C2TNB239V2  = 233,
    SEC_OID_ANSIX962_EC_C2TNB239V3  = 234,
    SEC_OID_ANSIX962_EC_C2ONB239V4  = 235,
    SEC_OID_ANSIX962_EC_C2ONB239V5  = 236,
    SEC_OID_ANSIX962_EC_C2PNB272W1  = 237,
    SEC_OID_ANSIX962_EC_C2PNB304W1  = 238,
    SEC_OID_ANSIX962_EC_C2TNB359V1  = 239,
    SEC_OID_ANSIX962_EC_C2PNB368W1  = 240,
    SEC_OID_ANSIX962_EC_C2TNB431R1  = 241,

    /* SECG named elliptic curves (characteristic two field) */
    SEC_OID_SECG_EC_SECT113R1       = 242,
    SEC_OID_SECG_EC_SECT113R2       = 243,
    SEC_OID_SECG_EC_SECT131R1       = 244,
    SEC_OID_SECG_EC_SECT131R2       = 245,
    SEC_OID_SECG_EC_SECT163K1       = 246,
    SEC_OID_SECG_EC_SECT163R1       = 247,
    SEC_OID_SECG_EC_SECT163R2       = 248,
    SEC_OID_SECG_EC_SECT193R1       = 249,
    SEC_OID_SECG_EC_SECT193R2       = 250,
    SEC_OID_SECG_EC_SECT233K1       = 251,
    SEC_OID_SECG_EC_SECT233R1       = 252,
    SEC_OID_SECG_EC_SECT239K1       = 253,
    SEC_OID_SECG_EC_SECT283K1       = 254,
    SEC_OID_SECG_EC_SECT283R1       = 255,
    SEC_OID_SECG_EC_SECT409K1       = 256,
    SEC_OID_SECG_EC_SECT409R1       = 257,
    SEC_OID_SECG_EC_SECT571K1       = 258,
    SEC_OID_SECG_EC_SECT571R1       = 259,

    SEC_OID_NETSCAPE_AOLSCREENNAME  = 260,

    SEC_OID_AVA_SURNAME              = 261,
    SEC_OID_AVA_SERIAL_NUMBER        = 262,
    SEC_OID_AVA_STREET_ADDRESS       = 263,
    SEC_OID_AVA_TITLE                = 264,
    SEC_OID_AVA_POSTAL_ADDRESS       = 265,
    SEC_OID_AVA_POSTAL_CODE          = 266,
    SEC_OID_AVA_POST_OFFICE_BOX      = 267,
    SEC_OID_AVA_GIVEN_NAME           = 268,
    SEC_OID_AVA_INITIALS             = 269,
    SEC_OID_AVA_GENERATION_QUALIFIER = 270,
    SEC_OID_AVA_HOUSE_IDENTIFIER     = 271,
    SEC_OID_AVA_PSEUDONYM            = 272,

    /* More OIDs */
    SEC_OID_PKIX_CA_ISSUERS          = 273,
    SEC_OID_PKCS9_EXTENSION_REQUEST  = 274,

    /* new EC Signature oids */
    SEC_OID_ANSIX962_ECDSA_SIGNATURE_RECOMMENDED_DIGEST = 275,
    SEC_OID_ANSIX962_ECDSA_SIGNATURE_SPECIFIED_DIGEST = 276,
    SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE = 277,
    SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE = 278,
    SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE = 279,
    SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE = 280,

    /* More id-ce and id-pe OIDs from RFC 3280 */
    SEC_OID_X509_HOLD_INSTRUCTION_CODE      = 281,
    SEC_OID_X509_DELTA_CRL_INDICATOR        = 282,
    SEC_OID_X509_ISSUING_DISTRIBUTION_POINT = 283,
    SEC_OID_X509_CERT_ISSUER                = 284,
    SEC_OID_X509_FRESHEST_CRL               = 285,
    SEC_OID_X509_INHIBIT_ANY_POLICY         = 286,
    SEC_OID_X509_SUBJECT_INFO_ACCESS        = 287,

    /* Camellia OIDs (RFC3657)*/
    SEC_OID_CAMELLIA_128_CBC                = 288,
    SEC_OID_CAMELLIA_192_CBC                = 289,
    SEC_OID_CAMELLIA_256_CBC                = 290,

    /* PKCS 5 V2 OIDS */
    SEC_OID_PKCS5_PBKDF2                    = 291,
    SEC_OID_PKCS5_PBES2                     = 292,
    SEC_OID_PKCS5_PBMAC1                    = 293,
    SEC_OID_HMAC_SHA1                       = 294,
    SEC_OID_HMAC_SHA224                     = 295,
    SEC_OID_HMAC_SHA256                     = 296,
    SEC_OID_HMAC_SHA384                     = 297,
    SEC_OID_HMAC_SHA512                     = 298,

    SEC_OID_PKIX_TIMESTAMPING               = 299,
    SEC_OID_PKIX_CA_REPOSITORY              = 300,

    SEC_OID_ISO_SHA1_WITH_RSA_SIGNATURE     = 301,

    SEC_OID_SEED_CBC			    = 302,

    SEC_OID_X509_ANY_POLICY                 = 303,

    SEC_OID_PKCS1_RSA_OAEP_ENCRYPTION       = 304,
    SEC_OID_PKCS1_MGF1                      = 305,
    SEC_OID_PKCS1_PSPECIFIED                = 306,
    SEC_OID_PKCS1_RSA_PSS_SIGNATURE         = 307,
    SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION = 308,

    SEC_OID_SHA224                          = 309,

    SEC_OID_EV_INCORPORATION_LOCALITY       = 310,
    SEC_OID_EV_INCORPORATION_STATE          = 311,
    SEC_OID_EV_INCORPORATION_COUNTRY        = 312,
    SEC_OID_BUSINESS_CATEGORY               = 313,

    SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA224_DIGEST     = 314,
    SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA256_DIGEST     = 315,

    /* Microsoft Trust List Signing
     * szOID_KP_CTL_USAGE_SIGNING 
     * where KP stands for Key Purpose
     */
    SEC_OID_MS_EXT_KEY_USAGE_CTL_SIGNING    = 316,

    /* The 'name' attribute type in X.520 */
    SEC_OID_AVA_NAME                        = 317,

    SEC_OID_TOTAL
} SECOidTag;

#define SEC_OID_SECG_EC_SECP192R1 SEC_OID_ANSIX962_EC_PRIME192V1
#define SEC_OID_SECG_EC_SECP256R1 SEC_OID_ANSIX962_EC_PRIME256V1
#define SEC_OID_PKCS12_KEY_USAGE  SEC_OID_X509_KEY_USAGE

/* fake OID for DSS sign/verify */
#define SEC_OID_SHA SEC_OID_MISS_DSS

typedef enum {
    INVALID_CERT_EXTENSION = 0,
    UNSUPPORTED_CERT_EXTENSION = 1,
    SUPPORTED_CERT_EXTENSION = 2
} SECSupportExtenTag;

typedef struct SECOidDataStr SECOidData;
struct SECOidDataStr {
    SECItem            oid;
    SECOidTag          offset;
    const char *       desc;
    unsigned long      mechanism;
    SECSupportExtenTag supportedExtension;	
    				/* only used for x.509 v3 extensions, so
				   that we can print the names of those
				   extensions that we don't even support */
};

/* New Opaque extended OID table API.  
 * These are algorithm policy Flags, used with functions
 * NSS_SetAlgorithmPolicy & NSS_GetAlgorithmPolicy.
 */
#define NSS_USE_ALG_IN_CERT_SIGNATURE  0x00000001  /* CRLs and OCSP, too */
#define NSS_USE_ALG_IN_CMS_SIGNATURE   0x00000002  /* used in S/MIME */
#define NSS_USE_ALG_RESERVED           0xfffffffc  /* may be used in future */

/* Code MUST NOT SET or CLEAR reserved bits, and must NOT depend on them
 * being all zeros or having any other known value.  The reserved bits
 * must be ignored.
 */

/* ========== <nss3/secoid.h> */

/* ========== <nss3/secdigt.h> */

/* ========== <nss3/secdert.h> */

/* ========== <nss3/secdig.h> */

#include "debug.h"

/* ========== nss/lib/util/secasn1u.c */
/*
 * We have a length that needs to be encoded; how many bytes will the
 * encoding take?
 *
 * The rules are that 0 - 0x7f takes one byte (the length itself is the
 * entire encoding); everything else takes one plus the number of bytes
 * in the length.
 */
int
SEC_ASN1LengthLength (unsigned long len)
{
    int lenlen = 1;

    if (len > 0x7f) {
	do {
	    lenlen++;
	    len >>= 8;
	} while (len);
    }

    return lenlen;
}

/*
 * XXX Move over (and rewrite as appropriate) the rest of the
 * stuff in dersubr.c!
 */

/*
 * Find the appropriate subtemplate for the given template.
 * This may involve calling a "chooser" function, or it may just
 * be right there.  In either case, it is expected to *have* a
 * subtemplate; this is asserted in debug builds (in non-debug
 * builds, NULL will be returned).
 *
 * "thing" is a pointer to the structure being encoded/decoded
 * "encoding", when true, means that we are in the process of encoding
 *	(as opposed to in the process of decoding)
 */
const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *theTemplate, void *thing,
			PRBool encoding)
{
    const SEC_ASN1Template *subt = NULL;

assert(theTemplate->sub != NULL);
    if (theTemplate->sub != NULL) {
	if (theTemplate->kind & SEC_ASN1_DYNAMIC) {
	    SEC_ASN1TemplateChooserPtr chooserp;

	    chooserp = *(SEC_ASN1TemplateChooserPtr *) theTemplate->sub;
	    if (chooserp) {
		if (thing != NULL)
		    thing = (char *)thing - theTemplate->offset;
		subt = (* chooserp)(thing, encoding);
	    }
	} else {
	    subt = (SEC_ASN1Template*)theTemplate->sub;
	}
    }
    return subt;
}

PRBool SEC_ASN1IsTemplateSimple(const SEC_ASN1Template *theTemplate)
{
    if (!theTemplate) {
	return PR_TRUE; /* it doesn't get any simpler than NULL */
    }
    /* only templates made of one primitive type or a choice of primitive
       types are considered simple */
    if (! (theTemplate->kind & (~SEC_ASN1_TAGNUM_MASK))) {
	return PR_TRUE; /* primitive type */
    }
    if (!(theTemplate->kind & SEC_ASN1_CHOICE)) {
	return PR_FALSE; /* no choice means not simple */
    }
    while (++theTemplate && theTemplate->kind) {
	if (theTemplate->kind & (~SEC_ASN1_TAGNUM_MASK)) {
	    return PR_FALSE; /* complex type */
	}
    }
    return PR_TRUE; /* choice of primitive types */
}

/* ========== nss/lib/util/quickder.c */
/* * simple definite-length ASN.1 decoder */

static unsigned char* definite_length_decoder(const unsigned char *buf,
                                              const unsigned int length,
                                              unsigned int *data_length,
                                              PRBool includeTag)
{
    unsigned char tag;
    unsigned int used_length= 0;
    unsigned int data_len;

    if (used_length >= length)
    {
        return NULL;
    }
    tag = buf[used_length++];

    /* blow out when we come to the end */
    if (tag == 0)
    {
        return NULL;
    }

    if (used_length >= length)
    {
        return NULL;
    }
    data_len = buf[used_length++];

    if (data_len&0x80)
    {
        int  len_count = data_len & 0x7f;

        data_len = 0;

        while (len_count-- > 0)
        {
            if (used_length >= length)
            {
                return NULL;
            }
            data_len = (data_len << 8) | buf[used_length++];
        }
    }

    if (data_len > (length-used_length) )
    {
        return NULL;
    }
    if (includeTag) data_len += used_length;

    *data_length = data_len;
    return ((unsigned char*)buf + (includeTag ? 0 : used_length));
}

static SECStatus GetItem(SECItem* src, SECItem* dest, PRBool includeTag)
{
    if ( (!src) || (!dest) || (!src->data && src->len) )
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (!src->len)
    {
        /* reaching the end of the buffer is not an error */
        dest->data = NULL;
        dest->len = 0;
        return SECSuccess;
    }

    dest->data = definite_length_decoder(src->data,  src->len, &dest->len,
        includeTag);
    if (dest->data == NULL)
    {
        PORT_SetError(SEC_ERROR_BAD_DER);
        return SECFailure;
    }
    src->len -= (dest->data - src->data) + dest->len;
    src->data = dest->data + dest->len;
    return SECSuccess;
}

/* check if the actual component's type matches the type in the template */

static SECStatus MatchComponentType(const SEC_ASN1Template* templateEntry,
                                    SECItem* item, PRBool* match, void* dest)
{
    unsigned long kind = 0;
    unsigned char tag = 0;

    if ( (!item) || (!item->data && item->len) || (!templateEntry) || (!match) )
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (!item->len)
    {
        *match = PR_FALSE;
        return SECSuccess;
    }

    kind = templateEntry->kind;
    tag = *(unsigned char*) item->data;

    if ( ( (kind & SEC_ASN1_INLINE) ||
           (kind & SEC_ASN1_POINTER) ) &&
           (0 == (kind & SEC_ASN1_TAG_MASK) ) )
    {
        /* These cases are special because the template's "kind" does not
           give us the information for the ASN.1 tag of the next item. It can
           only be figured out from the subtemplate. */
        if (!(kind & SEC_ASN1_OPTIONAL))
        {
            /* This is a required component. If there is a type mismatch,
               the decoding of the subtemplate will fail, so assume this
               is a match at the parent level and let it fail later. This
               avoids a redundant check in matching cases */
            *match = PR_TRUE;
            return SECSuccess;
        }
        else
        {
            /* optional component. This is the hard case. Now we need to
               look at the subtemplate to get the expected kind */
            const SEC_ASN1Template* subTemplate = 
                SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
            if (!subTemplate)
            {
                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                return SECFailure;
            }
            if ( (subTemplate->kind & SEC_ASN1_INLINE) ||
                 (subTemplate->kind & SEC_ASN1_POINTER) )
            {
                /* disallow nesting SEC_ASN1_POINTER and SEC_ASN1_INLINE,
                   otherwise you may get a false positive due to the recursion
                   optimization above that always matches the type if the
                   component is required . Nesting these should never be
                   required, so that no one should miss this ability */
                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                return SECFailure;
            }
            return MatchComponentType(subTemplate, item, match,
                                      (void*)((char*)dest + templateEntry->offset));
        }
    }

    if (kind & SEC_ASN1_CHOICE)
    {
        /* we need to check the component's tag against each choice's tag */
        /* XXX it would be nice to save the index of the choice here so that
           DecodeChoice wouldn't have to do this again. However, due to the
           recursivity of MatchComponentType, we don't know if we are in a
           required or optional component, so we can't write anywhere in
           the destination within this function */
        unsigned choiceIndex = 1;
        const SEC_ASN1Template* choiceEntry;
        while ( (choiceEntry = &templateEntry[choiceIndex++]) && (choiceEntry->kind))
        {
            if ( (SECSuccess == MatchComponentType(choiceEntry, item, match,
                                (void*)((char*)dest + choiceEntry->offset))) &&
                 (PR_TRUE == *match) )
            {
                return SECSuccess;
            }
        }
	/* no match, caller must decide if this is BAD DER, or not. */
        *match = PR_FALSE;
        return SECSuccess;
    }

    if (kind & SEC_ASN1_ANY)
    {
        /* SEC_ASN1_ANY always matches */
        *match = PR_TRUE;
        return SECSuccess;
    }

    if ( (0 == ((unsigned char)kind & SEC_ASN1_TAGNUM_MASK)) &&
         (!(kind & SEC_ASN1_EXPLICIT)) &&
         ( ( (kind & SEC_ASN1_SAVE) ||
             (kind & SEC_ASN1_SKIP) ) &&
           (!(kind & SEC_ASN1_OPTIONAL)) 
         )
       )
    {
        /* when saving or skipping a required component,  a type is not
           required in the template. This is for legacy support of
           SEC_ASN1_SAVE and SEC_ASN1_SKIP only. XXX I would like to
           deprecate these usages and always require a type, as this
           disables type checking, and effectively forbids us from
           transparently ignoring optional components we aren't aware of */
        *match = PR_TRUE;
        return SECSuccess;
    }

    /* first, do a class check */
    if ( (tag & SEC_ASN1_CLASS_MASK) !=
         (((unsigned char)kind) & SEC_ASN1_CLASS_MASK) )
    {
#ifdef DEBUG
        /* this is only to help debugging of the decoder in case of problems */
        unsigned char tagclass = tag & SEC_ASN1_CLASS_MASK;
        unsigned char expectedclass = (unsigned char)kind & SEC_ASN1_CLASS_MASK;
        tagclass = tagclass;
        expectedclass = expectedclass;
#endif
        *match = PR_FALSE;
        return SECSuccess;
    }

    /* now do a tag check */
    if ( ((unsigned char)kind & SEC_ASN1_TAGNUM_MASK) !=
         (tag & SEC_ASN1_TAGNUM_MASK))
    {
        *match = PR_FALSE;
        return SECSuccess;
    }

    /* now, do a method check. This depends on the class */
    switch (tag & SEC_ASN1_CLASS_MASK)
    {
    case SEC_ASN1_UNIVERSAL:
        /* For types of the SEC_ASN1_UNIVERSAL class, we know which must be
           primitive or constructed based on the tag */
        switch (tag & SEC_ASN1_TAGNUM_MASK)
        {
        case SEC_ASN1_SEQUENCE:
        case SEC_ASN1_SET:
        case SEC_ASN1_EMBEDDED_PDV:
            /* this component must be a constructed type */
            /* XXX add any new universal constructed type here */
            if (tag & SEC_ASN1_CONSTRUCTED)
            {
                *match = PR_TRUE;
                return SECSuccess;
            }
            break;

        default:
            /* this component must be a primitive type */
            if (! (tag & SEC_ASN1_CONSTRUCTED))
            {
                *match = PR_TRUE;
                return SECSuccess;
            }
            break;
        }
        break;

    default:
        /* for all other classes, we check the method based on the template */
        if ( (unsigned char)(kind & SEC_ASN1_METHOD_MASK) ==
             (tag & SEC_ASN1_METHOD_MASK) )
        {
            *match = PR_TRUE;
            return SECSuccess;
        }
        /* method does not match between template and component */
        break;
    }

    *match = PR_FALSE;
    return SECSuccess;
}

#ifdef DEBUG

static SECStatus CheckSequenceTemplate(const SEC_ASN1Template* sequenceTemplate)
{
    SECStatus rv = SECSuccess;
    const SEC_ASN1Template* sequenceEntry = NULL;
    unsigned long seqIndex = 0;
    unsigned long lastEntryIndex = 0;
    unsigned long ambiguityIndex = 0;
    PRBool foundAmbiguity = PR_FALSE;

    do
    {
        sequenceEntry = &sequenceTemplate[seqIndex++];
        if (sequenceEntry->kind)
        {
            /* ensure that we don't have an optional component of SEC_ASN1_ANY
               in the middle of the sequence, since we could not handle it */
            /* XXX this function needs to dig into the subtemplates to find
               the next tag */
            if ( (PR_FALSE == foundAmbiguity) &&
                 (sequenceEntry->kind & SEC_ASN1_OPTIONAL) &&
                 (sequenceEntry->kind & SEC_ASN1_ANY) )
            {
                foundAmbiguity = PR_TRUE;
                ambiguityIndex = seqIndex - 1;
            }
        }
    } while (sequenceEntry->kind);

    lastEntryIndex = seqIndex - 2;

    if (PR_FALSE != foundAmbiguity)
    {
        if (ambiguityIndex < lastEntryIndex)
        {
            /* ambiguity can only be tolerated on the last entry */
            PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
            rv = SECFailure;
        }
    }

    /* XXX also enforce ASN.1 requirement that tags be
       distinct for consecutive optional components */

    return rv;
}

#endif

static SECStatus DecodeItem(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag);

static SECStatus DecodeSequence(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem source;
    SECItem sequence;
    const SEC_ASN1Template* sequenceTemplate = &(templateEntry[1]);
    const SEC_ASN1Template* sequenceEntry = NULL;
    unsigned long seqindex = 0;

#ifdef DEBUG
    /* for a sequence, we need to validate the template. */
    rv = CheckSequenceTemplate(sequenceTemplate);
#endif

    source = *src;

    /* get the sequence */
    if (SECSuccess == rv)
    {
        rv = GetItem(&source, &sequence, PR_FALSE);
    }

    /* process it */
    if (SECSuccess == rv)
    do
    {
        sequenceEntry = &sequenceTemplate[seqindex++];
        if ( (sequenceEntry && sequenceEntry->kind) &&
             (sequenceEntry->kind != SEC_ASN1_SKIP_REST) )
        {
            rv = DecodeItem(dest, sequenceEntry, &sequence, arena, PR_TRUE);
        }
    } while ( (SECSuccess == rv) &&
              (sequenceEntry->kind &&
               sequenceEntry->kind != SEC_ASN1_SKIP_REST) );
    /* we should have consumed all the bytes in the sequence by now
       unless the caller doesn't care about the rest of the sequence */
    if (SECSuccess == rv && sequence.len &&
        sequenceEntry && sequenceEntry->kind != SEC_ASN1_SKIP_REST)
    {
        /* it isn't 100% clear whether this is a bad DER or a bad template.
           The problem is that logically, they don't match - there is extra
           data in the DER that the template doesn't know about */
        PORT_SetError(SEC_ERROR_BAD_DER);
        rv = SECFailure;
    }

    return rv;
}

static SECStatus DecodeInline(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    const SEC_ASN1Template* inlineTemplate = 
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
    return DecodeItem((void*)((char*)dest + templateEntry->offset),
                            inlineTemplate, src, arena, checkTag);
}

static SECStatus DecodePointer(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    const SEC_ASN1Template* ptrTemplate = 
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);
    void* subdata = PORT_ArenaZAlloc(arena, ptrTemplate->size);
    *(void**)((char*)dest + templateEntry->offset) = subdata;
    if (subdata)
    {
        return DecodeItem(subdata, ptrTemplate, src, arena, checkTag);
    }
    else
    {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
}

static SECStatus DecodeImplicit(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    if (templateEntry->kind & SEC_ASN1_POINTER)
    {
        return DecodePointer((void*)((char*)dest ),
                             templateEntry, src, arena, PR_FALSE);
    }
    else
    {
        return DecodeInline((void*)((char*)dest ),
                             templateEntry, src, arena, PR_FALSE);
    }
}

static SECStatus DecodeChoice(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem choice;
    const SEC_ASN1Template* choiceTemplate = &(templateEntry[1]);
    const SEC_ASN1Template* choiceEntry = NULL;
    unsigned long choiceindex = 0;

    /* XXX for a choice component, we should validate the template to make
       sure the tags are distinct, in debug builds. This hasn't been
       implemented yet */
    /* rv = CheckChoiceTemplate(sequenceTemplate); */

    /* process it */
    do
    {
        choice = *src;
        choiceEntry = &choiceTemplate[choiceindex++];
        if (choiceEntry->kind)
        {
            rv = DecodeItem(dest, choiceEntry, &choice, arena, PR_TRUE);
        }
    } while ( (SECFailure == rv) && (choiceEntry->kind));

    if (SECFailure == rv)
    {
        /* the component didn't match any of the choices */
        PORT_SetError(SEC_ERROR_BAD_DER);
    }
    else
    {
        /* set the type in the union here */
        int *which = (int *)((char *)dest + templateEntry->offset);
        *which = (int)choiceEntry->size;
    }

    /* we should have consumed all the bytes by now */
    /* fail if we have not */
    if (SECSuccess == rv && choice.len)
    {
        /* there is extra data that isn't listed in the template */
        PORT_SetError(SEC_ERROR_BAD_DER);
        rv = SECFailure;
    }
    return rv;
}

static SECStatus DecodeGroup(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem source;
    SECItem group;
    PRUint32 totalEntries = 0;
    PRUint32 entryIndex = 0;
    void** entries = NULL;

    const SEC_ASN1Template* subTemplate =
        SEC_ASN1GetSubtemplate (templateEntry, dest, PR_FALSE);

    source = *src;

    /* get the group */
    if (SECSuccess == rv)
    {
        rv = GetItem(&source, &group, PR_FALSE);
    }

    /* XXX we should check the subtemplate in debug builds */
    if (SECSuccess == rv)
    {
        /* first, count the number of entries. Benchmarking showed that this
           counting pass is more efficient than trying to allocate entries as
           we read the DER, even if allocating many entries at a time
        */
        SECItem counter = group;
        do
        {
            SECItem anitem;
            rv = GetItem(&counter, &anitem, PR_TRUE);
            if (SECSuccess == rv && (anitem.len) )
            {
                totalEntries++;
            }
        }  while ( (SECSuccess == rv) && (counter.len) );

        if (SECSuccess == rv)
        {
            /* allocate room for pointer array and entries */
            /* we want to allocate the array even if there is 0 entry */
            entries = (void**)PORT_ArenaZAlloc(arena, sizeof(void*)*
                                          (totalEntries + 1 ) + /* the extra one is for NULL termination */
                                          subTemplate->size*totalEntries); 

            if (entries)
            {
                entries[totalEntries] = NULL; /* terminate the array */
            }
            else
            {
                PORT_SetError(SEC_ERROR_NO_MEMORY);
                rv = SECFailure;
            }
            if (SECSuccess == rv)
            {
                void* entriesData = (unsigned char*)entries + (unsigned long)(sizeof(void*)*(totalEntries + 1 ));
                /* and fix the pointers in the array */
                PRUint32 entriesIndex = 0;
                for (entriesIndex = 0;entriesIndex<totalEntries;entriesIndex++)
                {
                    entries[entriesIndex] =
                        (char*)entriesData + (subTemplate->size*entriesIndex);
                }
            }
        }
    }

    if (SECSuccess == rv && totalEntries)
    do
    {
        if (!(entryIndex<totalEntries))
        {
            rv = SECFailure;
            break;
        }
        rv = DecodeItem(entries[entryIndex++], subTemplate, &group, arena, PR_TRUE);
    } while ( (SECSuccess == rv) && (group.len) );
    /* we should be at the end of the set by now */    
    /* save the entries where requested */
    memcpy(((char*)dest + templateEntry->offset), &entries, sizeof(void**));

    return rv;
}

static SECStatus DecodeExplicit(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena)
{
    SECStatus rv = SECSuccess;
    SECItem subItem;
    SECItem constructed = *src;

    rv = GetItem(&constructed, &subItem, PR_FALSE);

    if (SECSuccess == rv)
    {
        if (templateEntry->kind & SEC_ASN1_POINTER)
        {
            rv = DecodePointer(dest, templateEntry, &subItem, arena, PR_TRUE);
        }
        else
        {
            rv = DecodeInline(dest, templateEntry, &subItem, arena, PR_TRUE);
        }
    }

    return rv;
}

/* new decoder implementation. This is a recursive function */

static SECStatus DecodeItem(void* dest,
                     const SEC_ASN1Template* templateEntry,
                     SECItem* src, PLArenaPool* arena, PRBool checkTag)
{
    SECStatus rv = SECSuccess;
    SECItem temp;
    SECItem mark;
    PRBool pop = PR_FALSE;
    PRBool decode = PR_TRUE;
    PRBool save = PR_FALSE;
    unsigned long kind;
    PRBool match = PR_TRUE;
    PRBool optional = PR_FALSE;

assert(src && dest && templateEntry && arena);
#if 0
    if (!src || !dest || !templateEntry || !arena)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        rv = SECFailure;
    }
#endif

    if (SECSuccess == rv)
    {
        /* do the template validation */
        kind = templateEntry->kind;
        optional = (0 != (kind & SEC_ASN1_OPTIONAL));
        if (!kind)
        {
            PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
            rv = SECFailure;
        }
    }

    if (SECSuccess == rv)
    {
#ifdef DEBUG
        if (kind & SEC_ASN1_DEBUG_BREAK)
        {
            /* when debugging the decoder or a template that fails to
            decode, put SEC_ASN1_DEBUG in the component that gives you
            trouble. The decoder will then get to this block and assert.
            If you want to debug the rest of the code, you can set a
            breakpoint and set dontassert to PR_TRUE, which will let
            you skip over the assert and continue the debugging session
            past it. */
            PRBool dontassert = PR_FALSE;
assert(dontassert); /* set bkpoint here & set dontassert*/
        }
#endif

        if ((kind & SEC_ASN1_SKIP) ||
            (kind & SEC_ASN1_SAVE))
        {
            /* if skipping or saving this component, don't decode it */
            decode = PR_FALSE;
        }
    
        if (kind & (SEC_ASN1_SAVE | SEC_ASN1_OPTIONAL))
        {
            /* if saving this component, or if it is optional, we may not want to
               move past it, so save the position in case we have to rewind */
            mark = *src;
            if (kind & SEC_ASN1_SAVE)
            {
                save = PR_TRUE;
                if (0 == (kind & SEC_ASN1_SKIP))
                {
                    /* we will for sure have to rewind when saving this
                       component and not skipping it. This is true for all
                       legacy uses of SEC_ASN1_SAVE where the following entry
                       in the template would causes the same component to be
                       processed again */
                    pop = PR_TRUE;
                }
            }
        }

        rv = GetItem(src, &temp, PR_TRUE);
    }

    if (SECSuccess == rv)
    {
        /* now check if the component matches what we expect in the template */

        if (PR_TRUE == checkTag)

        {
            rv = MatchComponentType(templateEntry, &temp, &match, dest);
        }

        if ( (SECSuccess == rv) && (PR_TRUE != match) )
        {
            if (kind & SEC_ASN1_OPTIONAL)
            {

                /* the optional component is missing. This is not fatal. */
                /* Rewind, don't decode, and don't save */
                pop = PR_TRUE;
                decode = PR_FALSE;
                save = PR_FALSE;
            }
            else
            {
                /* a required component is missing. abort */
                PORT_SetError(SEC_ERROR_BAD_DER);
                rv = SECFailure;
            }
        }
    }

    if ((SECSuccess == rv) && (PR_TRUE == decode))
    {
        /* the order of processing here is is the tricky part */
        /* we start with our special cases */
        /* first, check the component class */
        if (kind & SEC_ASN1_INLINE)
        {
            /* decode inline template */
            rv = DecodeInline(dest, templateEntry, &temp , arena, PR_TRUE);
        }

        else
        if (kind & SEC_ASN1_EXPLICIT)
        {
            rv = DecodeExplicit(dest, templateEntry, &temp, arena);
        }
        else
        if ( (SEC_ASN1_UNIVERSAL != (kind & SEC_ASN1_CLASS_MASK)) &&

              (!(kind & SEC_ASN1_EXPLICIT)))
        {

            /* decode implicitly tagged components */
            rv = DecodeImplicit(dest, templateEntry, &temp , arena);
        }
        else
        if (kind & SEC_ASN1_POINTER)
        {
            rv = DecodePointer(dest, templateEntry, &temp, arena, PR_TRUE);
        }
        else
        if (kind & SEC_ASN1_CHOICE)
        {
            rv = DecodeChoice(dest, templateEntry, &temp, arena);
        }
        else
        if (kind & SEC_ASN1_ANY)
        {
            /* catch-all ANY type, don't decode */
            save = PR_TRUE;
            if (kind & SEC_ASN1_INNER)
            {
                /* skip the tag and length */
                SECItem newtemp = temp;
                rv = GetItem(&newtemp, &temp, PR_FALSE);
            }
        }
        else
        if (kind & SEC_ASN1_GROUP)
        {
            if ( (SEC_ASN1_SEQUENCE == (kind & SEC_ASN1_TAGNUM_MASK)) ||
                 (SEC_ASN1_SET == (kind & SEC_ASN1_TAGNUM_MASK)) )
            {
                rv = DecodeGroup(dest, templateEntry, &temp , arena);
            }
            else
            {
                /* a group can only be a SET OF or SEQUENCE OF */
                PORT_SetError(SEC_ERROR_BAD_TEMPLATE);
                rv = SECFailure;
            }
        }
        else
        if (SEC_ASN1_SEQUENCE == (kind & SEC_ASN1_TAGNUM_MASK))
        {
            /* plain SEQUENCE */
            rv = DecodeSequence(dest, templateEntry, &temp , arena);
        }
        else
        {
            /* handle all other types as "save" */
            /* we should only get here for primitive universal types */
            SECItem newtemp = temp;
            rv = GetItem(&newtemp, &temp, PR_FALSE);
            save = PR_TRUE;
            if ((SECSuccess == rv) &&
                SEC_ASN1_UNIVERSAL == (kind & SEC_ASN1_CLASS_MASK))
            {
                unsigned long tagnum = kind & SEC_ASN1_TAGNUM_MASK;
                if ( temp.len == 0 && (tagnum == SEC_ASN1_BOOLEAN ||
                                       tagnum == SEC_ASN1_INTEGER ||
                                       tagnum == SEC_ASN1_BIT_STRING ||
                                       tagnum == SEC_ASN1_OBJECT_ID ||
                                       tagnum == SEC_ASN1_ENUMERATED ||
                                       tagnum == SEC_ASN1_UTC_TIME ||
                                       tagnum == SEC_ASN1_GENERALIZED_TIME) )
                {
                    /* these types MUST have at least one content octet */
                    PORT_SetError(SEC_ERROR_BAD_DER);
                    rv = SECFailure;
                }
                else
                switch (tagnum)
                {
                /* special cases of primitive types */
                case SEC_ASN1_INTEGER:
                    {
                        /* remove leading zeroes if the caller requested
                           siUnsignedInteger
                           This is to allow RSA key operations to work */
                        SECItem* destItem = (SECItem*) ((char*)dest +
                                            templateEntry->offset);
                        if (destItem && (siUnsignedInteger == destItem->type))
                        {
                            while (temp.len > 1 && temp.data[0] == 0)
                            {              /* leading 0 */
                                temp.data++;
                                temp.len--;
                            }
                        }
                        break;
                    }

                case SEC_ASN1_BIT_STRING:
                    {
                        /* change the length in the SECItem to be the number
                           of bits */
                        temp.len = (temp.len-1)*8 - (temp.data[0] & 0x7);
                        temp.data++;
                        break;
                    }

                default:
                    {
                        break;
                    }
                }
            }
        }
    }

    if ((SECSuccess == rv) && (PR_TRUE == save))
    {
        SECItem* destItem = (SECItem*) ((char*)dest + templateEntry->offset);
        if (destItem)
        {
            /* we leave the type alone in the destination SECItem.
               If part of the destination was allocated by the decoder, in
               cases of POINTER, SET OF and SEQUENCE OF, then type is set to
               siBuffer due to the use of PORT_ArenaZAlloc*/
            destItem->data = temp.len ? temp.data : NULL;
            destItem->len = temp.len;
        }
        else
        {
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
            rv = SECFailure;
        }
    }

    if (PR_TRUE == pop)
    {
        /* we don't want to move ahead, so restore the position */
        *src = mark;
    }
    return rv;
}

/* the function below is the public one */

SECStatus SEC_QuickDERDecodeItem(PLArenaPool* arena, void* dest,
                     const SEC_ASN1Template* templateEntry,
                     const SECItem* src)
{
    SECStatus rv = SECSuccess;
    SECItem newsrc;

    if (!arena || !templateEntry || !src)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        rv = SECFailure;
    }

    if (SECSuccess == rv)
    {
        newsrc = *src;
        rv = DecodeItem(dest, templateEntry, &newsrc, arena, PR_TRUE);
        if (SECSuccess == rv && newsrc.len)
        {
            rv = SECFailure;
            PORT_SetError(SEC_ERROR_EXTRA_INPUT);
        }
    }

    return rv;
}

/* ========== */

static struct poptOption rpmasnOptionsTable[] = {

 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_(" Common options for all rpmio executables:"), NULL },

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
"), NULL },

  POPT_TABLEEND
};

int
main(int argc, char *argv[])
{
    poptContext con = rpmioInit(argc, argv, rpmasnOptionsTable);
    int ec = 0;

    con = rpmioFini(con);

    return ec;
}
