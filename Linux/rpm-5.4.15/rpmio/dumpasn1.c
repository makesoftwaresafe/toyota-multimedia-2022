/* ASN.1 object dumping code, copyright Peter Gutmann
   <pgut001@cs.auckland.ac.nz>, based on ASN.1 dump program by David Kemp
   <dpkemp@missi.ncsc.mil>, with contributions from various people including
   Matthew Hamrick <hamrick@rsa.com>, Bruno Couillard
   <bcouillard@chrysalis-its.com>, Hallvard Furuseth
   <h.b.furuseth@usit.uio.no>, Geoff Thorpe <geoff@raas.co.nz>, David Boyce
   <d.boyce@isode.com>, John Hughes <john.hughes@entegrity.com>, Life is hard,
   and then you die <ronald@trustpoint.com>, Hans-Olof Hermansson
   <hans-olof.hermansson@postnet.se>, Tor Rustad <Tor.Rustad@bbs.no>, Kjetil
   Barvik <kjetil.barvik@bbs.no>, James Sweeny <jsweeny@us.ibm.com>, Chris
   Ridd <chris.ridd@isode.com>, David Lemley <dev@ziggurat29.com> and several
   other people whose names I've misplaced (a number of those email addresses
   probably no longer work, since this code has been around for awhile).

   Available from http://www.cs.auckland.ac.nz/~pgut001/dumpasn1.c. Last
   updated 18 March 2010 (version 20100318, if you prefer it that way).  
   To build under Windows, use 'cl /MD dumpasn1.c'.  To build on OS390 or 
   z/OS, use '/bin/c89 -D OS390 -o dumpasn1 dumpasn1.c'.

   This code grew slowly over time without much design or planning, and with
   extra features being tacked on as required.  It's not representative of my
   normal coding style.  cryptlib,
   http://www.cs.auckland.ac.nz/~pgut001/cryptlib/, does a much better job of
   checking ASN.1 than this does, since dumpasn1 is a display program written
   to accept the widest possible range of input and not a compliance checker.
   In other words it will bend over backwards to accept even invalid data,
   since a common use for it is to try and locate encoding problems that lead
   to invalid encoded data.  While it will warn about some types of common
   errors, the fact that dumpasn1 will display an ASN.1 data item doesn't mean
   that the item is valid.

   This version of dumpasn1 requires a config file dumpasn1.cfg to be present
   in the same location as the program itself or in a standard directory where
   binaries live (it will run without it but will display a warning message,
   you can configure the path either by hardcoding it in or using an
   environment variable as explained further down).  The config file is
   available from http://www.cs.auckland.ac.nz/~pgut001/dumpasn1.cfg.

   This code assumes that the input data is binary, having come from a MIME-
   aware mailer or been piped through a decoding utility if the original
   format used base64 encoding.  If you need to decode it, it's recommended
   that you use a utility like uudeview, which will strip virtually any kind
   of encoding (MIME, PEM, PGP, whatever) to recover the binary original.

   You can use this code in whatever way you want, as long as you don't try to
   claim you wrote it.

   Editing notes: Tabs to 4, phasers to stun (and in case anyone wants to
   complain about that, see "Program Indentation and Comprehensiblity",
   Richard Miara, Joyce Musselman, Juan Navarro, and Ben Shneiderman,
   Communications of the ACM, Vol.26, No.11 (November 1983), p.861) */

#include "system.h"

#include <ctype.h>
#include <limits.h>
#include <wchar.h>

#include <rpmiotypes.h>
#include <poptIO.h>

#include "debug.h"

/* The update string, printed as part of the help screen */

#define UPDATE_STRING	"18 March 2010"

/* Useful defines */

#ifndef TRUE
#define FALSE	0
#define TRUE	( !FALSE )
#endif				/* TRUE */

/* SunOS 4.x doesn't define seek codes or exit codes or FILENAME_MAX (it does
   define _POSIX_MAX_PATH, but in funny locations and to different values
   depending on which include file you use).  Strictly speaking this code
   isn't right since we need to use PATH_MAX, however not all systems define
   this, some use _POSIX_PATH_MAX, and then there are all sorts of variations
   and other defines that you have to check, which require about a page of
   code to cover each OS, so we just use max( FILENAME_MAX, 512 ) which
   should work for everything */

#ifndef FILENAME_MAX
#define FILENAME_MAX	512
#else
#if FILENAME_MAX < 128
#undef FILENAME_MAX
#define FILENAME_MAX	512
#endif				/* FILENAME_MAX < 128 */
#endif				/* FILENAME_MAX */

/* Under Unix we can do special-case handling for paths and Unicode strings.
   Detecting Unix systems is a bit tricky but the following should find most
   versions.  This define implicitly assumes that the system has wchar_t
   support, but this is almost always the case except for very old systems,
   so it's best to default to allow-all rather than deny-all */

#define __UNIX__

/* Macros to avoid problems with sign extension */

#define byteToInt( x )	( ( unsigned char ) ( x ) )

/* The level of recursion can get scary for deeply-nested structures so we
   use a larger-than-normal stack under DOS */

/* When we dump a nested data object encapsulated within a larger object, the
   length is initially set to a magic value which is adjusted to the actual
   length once we start parsing the object */

#define LENGTH_MAGIC	177545L

/* Tag classes */

#define CLASS_MASK	0xC0	/* Bits 8 and 7 */
#define UNIVERSAL	0x00	/* 0 = Universal (defined by ITU X.680) */
#define APPLICATION	0x40	/* 1 = Application */
#define CONTEXT		0x80	/* 2 = Context-specific */
#define PRIVATE		0xC0	/* 3 = Private */

/* Encoding type */

#define FORM_MASK	0x20	/* Bit 6 */
#define PRIMITIVE	0x00	/* 0 = primitive */
#define CONSTRUCTED	0x20	/* 1 = constructed */

/* Universal tags */

#define TAG_MASK	0x1F	/* Bits 5 - 1 */
#define EOC		0x00	/*  0: End-of-contents octets */
#define BOOLEAN		0x01	/*  1: Boolean */
#define INTEGER		0x02	/*  2: Integer */
#define BITSTRING	0x03	/*  2: Bit string */
#define OCTETSTRING	0x04	/*  4: Byte string */
#define NULLTAG		0x05	/*  5: NULL */
#define OID		0x06	/*  6: Object Identifier */
#define OBJDESCRIPTOR	0x07	/*  7: Object Descriptor */
#define EXTERNAL	0x08	/*  8: External */
#define REAL		0x09	/*  9: Real */
#define ENUMERATED	0x0A	/* 10: Enumerated */
#define EMBEDDED_PDV	0x0B	/* 11: Embedded Presentation Data Value */
#define UTF8STRING	0x0C	/* 12: UTF8 string */
#define SEQUENCE	0x10	/* 16: Sequence/sequence of */
#define SET		0x11	/* 17: Set/set of */
#define NUMERICSTRING	0x12	/* 18: Numeric string */
#define PRINTABLESTRING	0x13	/* 19: Printable string (ASCII subset) */
#define T61STRING	0x14	/* 20: T61/Teletex string */
#define VIDEOTEXSTRING	0x15	/* 21: Videotex string */
#define IA5STRING	0x16	/* 22: IA5/ASCII string */
#define UTCTIME		0x17	/* 23: UTC time */
#define GENERALIZEDTIME	0x18	/* 24: Generalized time */
#define GRAPHICSTRING	0x19	/* 25: Graphic string */
#define VISIBLESTRING	0x1A	/* 26: Visible string (ASCII subset) */
#define GENERALSTRING	0x1B	/* 27: General string */
#define UNIVERSALSTRING	0x1C	/* 28: Universal string */
#define BMPSTRING	0x1E	/* 30: Basic Multilingual Plane/Unicode string */

/* Length encoding */

#define LEN_XTND 	0x80	/* Indefinite or long form */
#define LEN_MASK	0x7F	/* Bits 7 - 1 */

/** Various special-case operations to perform on strings. */
typedef enum {
    STR_NONE,			/* No special handling */
    STR_UTCTIME,		/* Check it's UTCTime */
    STR_GENERALIZED,		/* Check it's GeneralizedTime */
    STR_PRINTABLE,		/* Check it's a PrintableString */
    STR_IA5,			/* Check it's an IA5String */
    STR_LATIN1,			/* Read and display string as latin-1 */
    STR_BMP,			/* Read and display string as Unicode */
    STR_BMP_REVERSED		/* STR_BMP with incorrect endianness */
} STR_OPTION;

/** Structure to hold info about an ASN.1 item. */
typedef struct {
    int id;			/* Tag class + primitive/constructed */
    int tag;			/* Tag */
    long length;		/* Data length */
    int indefinite;		/* Item has indefinite length */
    int headerSize;		/* Size of tag+length */
    unsigned char header[8];	/* Tag+length data */
} ASN1_ITEM;

/** Information about an ASN.1 Object Identifier. */
#define MAX_OID_SIZE	32
typedef struct tagOIDINFO {
    struct tagOIDINFO *next;	/* Next item in list */
    unsigned char oid[MAX_OID_SIZE];
    int oidLength;
    char *comment;		/* Name, rank, ... */
    char *description;		/* ... and serial number */
    int warn;			/* Whether to warn if OID encountered */
} OIDINFO;

/* Table to identify valid string chars (taken from cryptlib).  Note that
   IA5String also allows control chars, but we warn about these since
   finding them in a certificate is a sign that there's something
   seriously wrong */

#define P	1		/* PrintableString */
#define I	2		/* IA5String */
#define PI	3		/* IA5String and PrintableString */

static uint8_t rpmasnCharFlags[] = {
    /* 00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F */
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /* 10  11  12  13  14  15  16  17  18  19  1A  1B  1C  1D  1E  1F */
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*      !   "   #   $   %   &   '   (   )   *   +   ,   -   .   / */
       PI,  I,  I,  I,  I,  I,  I, PI, PI, PI,  I, PI, PI, PI, PI, PI,
    /*  0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ? */
       PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,  I,  I, PI,  I, PI,
    /*  @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O */
        I, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,
    /*  P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _ */
       PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,  I,  I,  I,  I,  I,
    /*  `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o */
        I, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,
    /*  p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~  DL */
       PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,  I,  I,  I,  I,  0
};

/*==============================================================*/

#define _KFB(n) (1U << (n))
#define _AFB(n) (_KFB(n) | 0x40000000)

typedef struct rpmasn_s * rpmasn;

#define F_ISSET(_f, _FLAG) (((_f) & ((ASN_FLAGS_##_FLAG) & ~0x40000000)) != ASN_FLAGS_NONE)
#define AF_ISSET(_FLAG) F_ISSET(asn->flags, _FLAG)

enum asnFlags_e {
    ASN_FLAGS_NONE	= 0,
    ASN_FLAGS_DOTS	= _AFB( 0),	/* -d */
    ASN_FLAGS_PURE	= _AFB( 1),	/* -p */
    ASN_FLAGS_OIDS	= _AFB( 2),	/* -l */
    ASN_FLAGS_HEX	= _AFB( 3),	/* -x */
    ASN_FLAGS_STDIN	= _AFB( 4),	/* */
    ASN_FLAGS_ZEROLEN	= _AFB( 5),	/* -z */
    ASN_FLAGS_TEXT	= _AFB( 6),	/* -t */
    ASN_FLAGS_ALLDATA	= _AFB( 7),	/* -a */
    ASN_FLAGS_ENCAPS	= _AFB( 8),	/* -e */
    ASN_FLAGS_CHARSET	= _AFB( 9),	/* -o */
    ASN_FLAGS_BITFLIP	= _AFB(10),	/* -r */
    ASN_FLAGS_CHKONLY	= _AFB(11),	/* -s */
    ASN_FLAGS_RAWTIME	= _AFB(12),	/* -u */
    ASN_FLAGS_SHALLOW	= _AFB(13),	/* -i */
};

struct rpmasn_s {
    enum asnFlags_e flags;
    const char * cfn;
    const char * ofn;

    FILE * ifp;		/* Input stream */
    int fPos;		/* Position in the input stream */
    int lineNo;
    long offset;

    FILE * ofp;		/* Output stream */
    size_t outwidth;
    int hdrlevel;	/* Dump tag+len in hex (level = 0, 1, 2) */

    OIDINFO * oidList;
    uint8_t * charFlags;

    int noErrors;	/* Number of errors found */
    int noWarnings;	/* Number of warnings */
};

static struct rpmasn_s __rpmasn = {
    .flags	= ASN_FLAGS_ENCAPS | ASN_FLAGS_CHARSET | ASN_FLAGS_BITFLIP,
    .outwidth	= 80,		/* 80-column display */
    .charFlags	= rpmasnCharFlags
};

static rpmasn _rpmasn = &__rpmasn;

/*==============================================================*/

/* The indent size and fixed indent string to the left of the data */

#define INDENT_STRING	"         : "
#define	INDENT_SIZE	(sizeof(INDENT_STRING)-1)

static inline int rpmasnGetc(rpmasn asn)
{
   return fgetc(asn->ifp);
}

static inline int rpmasnPutc(int c, rpmasn asn)
{
   return fputc(c, asn->ofp);
}

static inline int rpmasnPuts(const char * s, rpmasn asn)
{
   return fputs(s, asn->ofp);
}

static inline int rpmasnIndent(rpmasn asn, const int level, int nl)
{
    const char * s = (AF_ISSET(DOTS) ? ". " : AF_ISSET(SHALLOW) ? " " : "  ");
    int rc = 0;
    int i;

    if (nl)
	rpmasnPuts("\n", asn);

    if (!AF_ISSET(PURE))
	rc = rpmasnPuts(INDENT_STRING, asn);

    for (i = 0; i < level; i++)
	rc = rpmasnPuts(s, asn);

    return rc;
}

/*==============================================================*/

/* If the config file isn't present in the current directory, we search the
   following paths (this is needed for Unix with dumpasn1 somewhere in the
   path, since this doesn't set up argv[0] to the full path).  Anything
   beginning with a '$' uses the appropriate environment variable.  In
   addition under Unix we also walk down $PATH looking for it */

#define CONFIG_NAME		"dumpasn1.cfg"

static const char *configPaths[] = {
    /* Unix absolute paths */
    "/usr/bin/", "/usr/local/bin/", "/etc/dumpasn1/",

    /* Unix environment-based paths */
    "$HOME/", "$HOME/bin/",

    /* It's my program, I'm allowed to hardcode in strange paths that no-one
       else uses */
    "$HOME/BIN/",

    /* General environment-based paths */
    "$DUMPASN1_PATH/",

    NULL
};

#define isEnvTerminator( c )	\
	( ( ( c ) == '/' ) || ( ( c ) == '.' ) || ( ( c ) == '$' ) || \
	  ( ( c ) == '\0' ) || ( ( c ) == '~' ) )

/* ===== Object Identification/Description Routines */

/* Return descriptive strings for universal tags */

static char *idstr(const int tagID)
{
    switch (tagID) {
    case EOC:			return "End-of-contents octets";
    case BOOLEAN:		return "BOOLEAN";
    case INTEGER:		return "INTEGER";
    case BITSTRING:		return "BIT STRING";
    case OCTETSTRING:		return "OCTET STRING";
    case NULLTAG:		return "NULL";
    case OID:			return "OBJECT IDENTIFIER";
    case OBJDESCRIPTOR:		return "ObjectDescriptor";
    case EXTERNAL:		return "EXTERNAL";
    case REAL:			return "REAL";
    case ENUMERATED:		return "ENUMERATED";
    case EMBEDDED_PDV:		return "EMBEDDED PDV";
    case UTF8STRING:		return "UTF8String";
    case SEQUENCE:		return "SEQUENCE";
    case SET:			return "SET";
    case NUMERICSTRING:		return "NumericString";
    case PRINTABLESTRING:	return "PrintableString";
    case T61STRING:		return "TeletexString";
    case VIDEOTEXSTRING:	return "VideotexString";
    case IA5STRING:		return "IA5String";
    case UTCTIME:		return "UTCTime";
    case GENERALIZEDTIME:	return "GeneralizedTime";
    case GRAPHICSTRING:		return "GraphicString";
    case VISIBLESTRING:		return "VisibleString";
    case GENERALSTRING:		return "GeneralString";
    case UNIVERSALSTRING:	return "UniversalString";
    case BMPSTRING:		return "BMPString";
    default:			return "Unknown (Reserved)";
    }
}

/** Return information about an object identifier. */
static OIDINFO *getOIDinfo(rpmasn asn, unsigned char *oid, const int oidLength)
{
    OIDINFO * oidPtr;

    for (oidPtr = asn->oidList; oidPtr != NULL; oidPtr = oidPtr->next) {
	if (oidLength == oidPtr->oidLength - 2
	 && !memcmp(oidPtr->oid + 2, oid, oidLength))
	    break;
    }

    return oidPtr;
}

static inline int isPrintable(rpmasn asn, int ch)
{
    return ((ch < 128 && (asn->charFlags[ch] & P)) ? TRUE : FALSE);
}

static inline int isIA5(rpmasn asn, int ch)
{
    return ((ch < 128 && (asn->charFlags[ch] & I)) ? TRUE : FALSE);
}

/* ===== Config File Read Routines */

#define CPM_EOF		0x1A	/* ^Z = CPM EOF char */
#define MAX_LINESIZE	512	/* The maximum input line length */

/** Read a line of text from the config file. */
static int readLine(rpmasn asn, FILE * file, char *buffer)
{
    int bufCount = 0;
    int ch;

    /* Skip whitespace */
    while (((ch = getc(file)) == ' ' || ch == '\t') && !feof(file));

    /* Get a line into the buffer */
    while (ch != '\r' && ch != '\n' && ch != CPM_EOF && !feof(file)) {
	/* Check for an illegal char in the data.  Note that we don't just
	   check for chars with high bits set because these are legal in
	   non-ASCII strings */
	if (!isprint(ch)) {
	    printf("Bad character '%c' in config file line %d.\n",
		   ch, asn->lineNo);
	    return FALSE;
	}

	/* Check to see if it's a comment line */
	if (ch == '#' && !bufCount) {
	    /* Skip comment section and trailing whitespace */
	    while (ch != '\r' && ch != '\n' && ch != CPM_EOF
		   && !feof(file))
		ch = getc(file);
	    break;
	}

	/* Make sure that the line is of the correct length */
	if (bufCount > MAX_LINESIZE) {
	    printf("Config file line %d too long.\n", asn->lineNo);
	    return FALSE;
	} else if (ch)		/* Can happen if we read a binary file */
	    buffer[bufCount++] = ch;

	/* Get next character */
	ch = getc(file);
    }

    /* If we've just passed a CR, check for a following LF */
    if (ch == '\r') {
	if ((ch = getc(file)) != '\n')
	    ungetc(ch, file);
    }

    /* Skip trailing whitespace and add der terminador */
    while (bufCount > 0 &&
	   ((ch = buffer[bufCount - 1]) == ' ' || ch == '\t'))
	bufCount--;
    buffer[bufCount] = '\0';

    /* Handle special-case of ^Z if file came off an MSDOS/CPM system */
    if (ch == CPM_EOF) {
	while (!feof(file)) {
	    /* Keep going until we hit the true EOF (or some sort of error) */
	    ch = getc(file);
	}
    }

    return (ferror(file) ? FALSE : TRUE);
}

/** Process an OID specified as space-separated decimal or hex digits. */
static int processOID(rpmasn asn, OIDINFO * oidInfo, char *string)
{
    unsigned char binaryOID[MAX_OID_SIZE];
    int firstValue;
    int value;
    int valueIndex = 0;
    int oidIndex = 3;

    memset(binaryOID, 0, MAX_OID_SIZE);
    binaryOID[0] = OID;
    while (*string && oidIndex < MAX_OID_SIZE) {
	if (oidIndex >= MAX_OID_SIZE - 4) {
	    printf("Excessively long OID in config file line %d.\n",
		   asn->lineNo);
	    return FALSE;
	}
	if (sscanf(string, "%d", &value) != 1 || value < 0) {
	    printf("Invalid value in config file line %d.\n", asn->lineNo);
	    return FALSE;
	}
	if (valueIndex == 0) {
	    firstValue = value;
	    valueIndex++;
	} else {
	    if (valueIndex == 1) {
		if (firstValue < 0 || firstValue > 2 || value < 0 ||
		    ((firstValue < 2 && value > 39) ||
		     (firstValue == 2 && value > 175))) {
		    printf("Invalid value in config file line %d.\n",
			   asn->lineNo);
		    return FALSE;
		}
		binaryOID[2] = (firstValue * 40) + value;
		valueIndex++;
	    } else {
		int hasHighBits = FALSE;

		if (value >= 0x200000L) {	/* 2^21 */
		    binaryOID[oidIndex++] = 0x80 | (value >> 21);
		    value %= 0x200000L;
		    hasHighBits = TRUE;
		}
		if ((value >= 0x4000) || hasHighBits) {	/* 2^14 */
		    binaryOID[oidIndex++] = 0x80 | (value >> 14);
		    value %= 0x4000;
		    hasHighBits = TRUE;
		}
		if ((value >= 0x80) || hasHighBits) {	/* 2^7 */
		    binaryOID[oidIndex++] = 0x80 | (value >> 7);
		    value %= 128;
		}
		binaryOID[oidIndex++] = value;
	    }
	}
	while (*string && isdigit(byteToInt(*string)))
	    string++;
	if (*string && *string++ != ' ') {
	    printf("Invalid OID string in config file line %d.\n", asn->lineNo);
	    return FALSE;
	}
    }
    binaryOID[1] = oidIndex - 2;
    memcpy(oidInfo->oid, binaryOID, oidIndex);
    oidInfo->oidLength = oidIndex;

    return TRUE;
}

static int processHexOID(rpmasn asn, OIDINFO * oidInfo, char *string)
{
    int index = 0;
    int value;

    while (*string && index < MAX_OID_SIZE - 1) {
	if (sscanf(string, "%x", &value) != 1 || value > 255) {
	    printf("Invalid hex value in config file line %d.\n", asn->lineNo);
	    return FALSE;
	}
	oidInfo->oid[index++] = value;
	string += 2;
	if (*string && *string++ != ' ') {
	    printf("Invalid hex string in config file line %d.\n", asn->lineNo);
	    return FALSE;
	}
    }
    oidInfo->oid[index] = 0;
    oidInfo->oidLength = index;
    if (index >= MAX_OID_SIZE - 1) {
	printf("OID value in config file line %d too long.\n", asn->lineNo);
	return FALSE;
    }
    return TRUE;
}

/** Read a config file. */
static int readConfig(rpmasn asn, const char *path, const int isDefaultConfig)
{
    static OIDINFO dummyOID = { NULL, "Dummy", 0, "Dummy", "Dummy", 1 };
    OIDINFO *oidPtr;
    FILE *file;
    int seenHexOID = FALSE;
    char buffer[MAX_LINESIZE];
    int status;

    /* Try and open the config file */
    if ((file = fopen(path, "rb")) == NULL) {
	/* If we can't open the default config file, warn and continue. */
	if (isDefaultConfig) {
	    puts("Cannot open config file 'dumpasn1.cfg', which should be in the same");
	    puts("directory as the dumpasn1 program, a standard system directory, or");
	    puts("in a location pointed to by the DUMPASN1_PATH environment variable.");
	    puts("Operation will continue without the ability to display Object ");
	    puts("Identifier information.");
	    puts("");
	    puts("If the config file is located elsewhere, you can set the environment");
	    puts("variable DUMPASN1_PATH to the path to the file.");
	    return TRUE;
	}

	printf("Cannot open config file '%s'.\n", path);
	return FALSE;
    }

    /* Add the new config entries at the appropriate point in the OID list */
    if (asn->oidList == NULL)
	oidPtr = &dummyOID;
    else
	for (oidPtr = asn->oidList; oidPtr->next != NULL;
	     oidPtr = oidPtr->next);

    /* Read each line in the config file */
    asn->lineNo = 1;
    while ((status = readLine(asn, file, buffer)) == TRUE && !feof(file)) {
	/* If it's a comment line, skip it */
	if (!*buffer) {
	    asn->lineNo++;
	    continue;
	}

	/* Check for an attribute tag */
	if (!strncmp(buffer, "OID = ", 6)) {
	    /* Make sure that all of the required attributes for the current
	       OID are present */
	    if (oidPtr->description == NULL) {
		printf("OID ending on config file line %d has no "
		       "description attribute.\n", asn->lineNo - 1);
		return FALSE;
	    }

	    /* Allocate storage for the new OID */
	    oidPtr->next = (OIDINFO *) xcalloc(1, sizeof(OIDINFO));
	    oidPtr = oidPtr->next;
	    if (asn->oidList == NULL)
		asn->oidList = oidPtr;

	    /* Add the new OID */
	    if (!strncmp(buffer + 6, "06", 2)) {
		seenHexOID = TRUE;
		if (!processHexOID(asn, oidPtr, buffer + 6))
		    return FALSE;
	    } else {
		if (!processOID(asn, oidPtr, buffer + 6))
		    return FALSE;
	    }
	} else if (!strncmp(buffer, "Description = ", 14)) {
	    if (oidPtr->description != NULL) {
		printf
		    ("Duplicate OID description in config file line %d.\n",
		     asn->lineNo);
		return FALSE;
	    }
	    oidPtr->description = xstrdup(buffer + 14);
	} else if (!strncmp(buffer, "Comment = ", 10)) {
	    if (oidPtr->comment != NULL) {
		printf("Duplicate OID comment in config file line %d.\n",
		       asn->lineNo);
		return FALSE;
	    }
	    oidPtr->comment = xstrdup(buffer + 10);
	} else if (!strncmp(buffer, "Warning", 7)) {
	    if (oidPtr->warn) {
		printf("Duplicate OID warning in config file line %d.\n",
		       asn->lineNo);
		return FALSE;
	    }
	    oidPtr->warn = TRUE;
	} else {
	    printf("Unrecognised attribute '%s', line %d.\n", buffer,
		   asn->lineNo);
	    return FALSE;
	}

	asn->lineNo++;
    }
    fclose(file);

    /* If we're processing an old-style config file, tell the user to
       upgrade */
    if (seenHexOID) {
	puts("\nWarning: Use of old-style hex OIDs detected in "
	     "configuration file, please\n         update your dumpasn1 "
	     "configuration file.\n");
    }

    return status;
}

/** Check for the existence of a config file path. */
static int testConfigPath(const char *path)
{
    FILE *file;

/* XXX FIXME: use access(2) */
    /* Try and open the config file */
    if ((file = fopen(path, "rb")) == NULL)
	return FALSE;
    fclose(file);

    return TRUE;
}

/** Build a config path by substituting environment strings for $NAMEs. */
static void buildConfigPath(char *path, const char * template)
{
    char b[FILENAME_MAX];
    char * t = b;
    char * te = b;
    size_t nt = sizeof(b) - 1;
    char nfn[FILENAME_MAX];
    size_t ix = 0;
    size_t last = 0;

    *te = '\0';
    /* Add the config file name at the end */
    te = stpncpy(te, template, nt);
    *te = '\0';
    te = stpncpy(te, CONFIG_NAME, (nt - strlen(t)));
    *te = '\0';
    nt = strlen(t);

    while (ix < nt) {
	int substringSize;

	/* Find the next $ and copy the data before it to the new path */
	if ((te = strstr(t + ix, "$")) != NULL)
	    substringSize = (int) ((te - t) - ix);
	else
	    substringSize = nt - ix;
	if (substringSize > 0) {
	    memcpy(nfn + last, t + ix,
		   substringSize);
	}
	last += substringSize;
	ix += substringSize;

	/* Get the environment string for the $NAME */
	if (te != NULL) {
	    char envName[MAX_LINESIZE];
	    char *envString;
	    int i;

	    /* Skip the '$', find the end of the $NAME, and copy the name
	       into an internal buffer */
	    ix++;		/* Skip the $ */
	    for (i = 0; !isEnvTerminator(t[ix + i]); i++);
	    memcpy(envName, t + ix, i);
	    envName[i] = '\0';

	    /* Get the env.string and copy it over */
	    if ((envString = getenv(envName)) != NULL) {
		const int envStrLen = strlen(envString);

		if (last + envStrLen < FILENAME_MAX - 2) {
		    memcpy(nfn + last, envString, envStrLen);
		    last += envStrLen;
		}
	    }
	    ix += i;
	}
    }
    nfn[last] = '\0';	/* Add der terminador */

    /* Copy the new path to the output */
    strcpy(path, nfn);
}

/** Read the global config file. */
static int readGlobalConfig(rpmasn asn, const char *path)
{
    char buffer[FILENAME_MAX];
    char *searchPos = (char *) path;
    char *namePos;
    char *lastPos = NULL;
    char *envPath;
    int i;

    /* First, try and find the config file in the same directory as the
       executable by walking down the path until we find the last occurrence
       of the program name.  This requires that argv[0] be set up properly,
       which isn't the case if Unix search paths are being used and is a
       bit hit-and-miss under Windows where the contents of argv[0] depend
       on how the program is being executed.  To avoid this we perform some
       Windows-specific processing to try and find the path to the
       executable if we can't otherwise find it */
    do {
	namePos = lastPos;
	lastPos = strstr(searchPos, "dumpasn1");
	if (lastPos == NULL)
	    lastPos = strstr(searchPos, "DUMPASN1");
	searchPos = lastPos + 1;
    }
    while (lastPos != NULL);

    if (namePos == NULL && (namePos = strrchr(path, '/')) != NULL) {
	const int endPos = (int) (namePos - path) + 1;

	/* If the executable isn't called dumpasn1, we won't be able to find
	   it with the above code, fall back to looking for directory
	   separators.  This requires a system where the only separator is
	   the directory separator (ie it doesn't work for Windows or most
	   mainframe environments) */
	if (endPos < FILENAME_MAX - 13) {
	    memcpy(buffer, path, endPos);
	    strcpy(buffer + endPos, CONFIG_NAME);
	    if (testConfigPath(buffer))
		return readConfig(asn, buffer, TRUE);
	}

	/* That didn't work, try the absolute locations and $PATH */
	namePos = NULL;
    }

    if (strlen(path) < FILENAME_MAX - 13 && namePos != NULL) {
	strcpy(buffer, path);
	strcpy(buffer + (int) (namePos - (char *) path), CONFIG_NAME);
	if (testConfigPath(buffer))
	    return readConfig(asn, buffer, TRUE);
    }

    /* Now try each of the possible absolute locations for the config file */
    for (i = 0; configPaths[i] != NULL; i++) {
	buildConfigPath(buffer, configPaths[i]);
	if (testConfigPath(buffer))
	    return readConfig(asn, buffer, TRUE);
    }

    /* On Unix systems we can also search for the config file on $PATH */
    if ((envPath = getenv("PATH")) != NULL) {
	char *pathPtr = strtok(envPath, ":");

	do {
	    sprintf(buffer, "%s/%s", pathPtr, CONFIG_NAME);
	    if (testConfigPath(buffer))
		return readConfig(asn, buffer, TRUE);
	    pathPtr = strtok(NULL, ":");
	} while (pathPtr != NULL);
    }

    /* Default to just the config name (which should fail as it was the
       first entry in configPaths[]).  readConfig() will display the
       appropriate warning */
    return readConfig(asn, CONFIG_NAME, TRUE);
}

/** Free the in-memory config data. */
static void freeConfig(rpmasn asn)
{
    OIDINFO *oidPtr = asn->oidList;

    while (oidPtr != NULL) {
	OIDINFO *oidCursor = oidPtr;

	oidPtr = oidPtr->next;
	if (oidCursor->comment != NULL)
	    free(oidCursor->comment);
	if (oidCursor->description != NULL)
	    free(oidCursor->description);
	free(oidCursor);
    }
}

/* ===== Output/Formatting Routines */

/** Complain about an error in the ASN.1 object. */
static void complain(rpmasn asn, const char *message, const int level)
{
    rpmasnIndent(asn, level + 1, 0);
    fprintf(asn->ofp, "Error: %s.\n", message);
    asn->noErrors++;
}

/** Dump data as a string of hex digits up to a maximum of 128 bytes. */
static void dumpHex(rpmasn asn, long length, int level, int isInteger)
{
    const int lineLength = AF_ISSET(TEXT) ? 8 : 16;
    char printable[9];
    long noBytes = length;
    int zeroPadded = FALSE;
    int warnPadding = FALSE;
    int warnNegative = isInteger;
    int singleLine = FALSE;
    int maxLevel = (AF_ISSET(PURE)) ? 15 : 8;
    int prevCh = -1;
    int ch;
    int i;

    /* Check if LHS status info + indent + "OCTET STRING" string + data wraps */
    if ((AF_ISSET(PURE) ? 0 : INDENT_SIZE) + (level * 2) + 12 +
	(length * 3) < asn->outwidth)
	singleLine = TRUE;

    if (noBytes > 128 && !AF_ISSET(ALLDATA))
	noBytes = 128;		/* Only output a maximum of 128 bytes */
    if (level > maxLevel)
	level = maxLevel;	/* Make sure we don't go off edge of screen */
    printable[8] = printable[0] = '\0';

    for (i = 0; i < noBytes; i++) {

	if (!(i % lineLength)) {
	    if (singleLine)
		rpmasnPuts(" ", asn);
	    else {
		if (AF_ISSET(TEXT)) {
		    /* If we're dumping text alongside the hex data, print
		       the accumulated text string */
		    rpmasnPuts("    ", asn);
		    rpmasnPuts(printable, asn);
		}
		rpmasnIndent(asn, level + 1, 1);
	    }
	}
	ch = rpmasnGetc(asn);
	fprintf(asn->ofp, "%s%02X", i % lineLength ? " " : "", ch);
	printable[i % 8] = (ch >= ' ' && ch < 127) ? ch : '.';
	asn->fPos++;

	/* Check for negative values and zero padding (if needed). */
	if (i == 0) {
	    prevCh = ch;
	    if (!ch)
		zeroPadded = TRUE;
	    if (!(ch & 0x80))
		warnNegative = FALSE;
	}
	if (i == 1) {
	    /* Check for the first 9 bits being identical */
	    if ((prevCh == 0x00) && ((ch & 0x80) == 0x00))
		warnPadding = TRUE;
	    if ((prevCh == 0xFF) && ((ch & 0x80) == 0x80))
		warnPadding = TRUE;
	}
    }
    if (AF_ISSET(TEXT)) {
	/* Print any remaining text */
	i %= lineLength;
	printable[i] = '\0';
	while (i < lineLength) {
	    rpmasnPuts("   ", asn);
	    i++;
	}
	rpmasnPuts("    ", asn);
	rpmasnPuts(printable, asn);
    }
    if (length > 128 && !AF_ISSET(ALLDATA)) {
	length -= 128;
	rpmasnIndent(asn, level + 5, 1);
	fprintf(asn->ofp, "[ Another %ld bytes skipped ]", length);
	asn->fPos += length;
	if (AF_ISSET(STDIN)) {

	    while (length--) {
		if ((ch = rpmasnGetc(asn)) == EOF)
		    break;
	    }
	} else
	    assert(fseek(asn->ifp, length, SEEK_CUR) == 0);
    }
    rpmasnPuts("\n", asn);

    if (isInteger) {
	if (warnPadding)
	    complain(asn, "Integer has non-DER encoding", level);
	if (warnNegative)
	    complain(asn, "Integer has a negative value", level);
    }
}

/** Convert a binary OID to its string equivalent. */
static int oidToString(char *textOID, int *textOIDlength,
		       const unsigned char *oid, const int oidLength)
{
    uint8_t uuidBuffer[32];
    long value;
    int length = 0;
    int uuidBufPos = 0;
    int uuidBitCount = 0;
    int i;
    int validEncoding = TRUE;
    int isUUID = FALSE;

    memset(uuidBuffer, 0, sizeof(uuidBuffer));	/* XXX coverity #1060695 */
    for (i = 0, value = 0; i < oidLength; i++) {
	const unsigned char data = oid[i];
	const long valTmp = value << 7;

	/* Pick apart the encoding.  We keep going after hitting an encoding
	   error at the start of an arc because the overall length is 
	   bounded and we may still be able to recover something worth 
	   printing */
	if (value == 0 && data == 0x80) {
	    /* Invalid leading zero value, 0x80 & 0x7F == 0 */
	    validEncoding = FALSE;
	}
	if (isUUID) {
	    value = 1;		/* Use a dummy value f bypassing normal read */
	    if (uuidBitCount == 0)
		uuidBuffer[uuidBufPos] |= data << 1;
	    else {
		uuidBuffer[uuidBufPos++] |= data >> (8 - uuidBitCount);
		if (uuidBitCount < 7)
		    uuidBuffer[uuidBufPos] = data << (uuidBitCount + 1);
	    }
	    uuidBitCount++;
	    if (uuidBitCount > 7)
		uuidBitCount = 0;
	    if (!(data & 0x80)) {
		if (uuidBufPos != 16) {
		    validEncoding = FALSE;
		    break;
		}
		length += sprintf(textOID + length,
				  " { %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x }",
				  uuidBuffer[0], uuidBuffer[1],
				  uuidBuffer[2], uuidBuffer[3],
				  uuidBuffer[4], uuidBuffer[5],
				  uuidBuffer[6], uuidBuffer[7],
				  uuidBuffer[8], uuidBuffer[9],
				  uuidBuffer[10], uuidBuffer[11],
				  uuidBuffer[12], uuidBuffer[13],
				  uuidBuffer[14], uuidBuffer[15]);
		value = 0;
	    }
	    continue;
	}
	if (value >= (LONG_MAX >> 7) || valTmp >= LONG_MAX - (data & 0x7F)) {
	    validEncoding = FALSE;
	    break;
	}
	value = valTmp | (data & 0x7F);
	if (value < 0 || value > LONG_MAX / 2) {
	    validEncoding = FALSE;
	    break;
	}
	if (!(data & 0x80)) {
	    if (i == 0) {
		long x;
		long y;

		/* The first two levels are encoded into one byte since the 
		   root level has only 3 nodes (40*x + y), however if x = 
		   joint-iso-itu-t(2) then y may be > 39, so we have to add 
		   special-case handling for this */
		x = value / 40;
		y = value % 40;
		if (x > 2) {
		    /* Handle special case for large y if x == 2 */
		    y += (x - 2) * 40;
		    x = 2;
		}
		if (x < 0 || x > 2 || y < 0 ||
		    ((x < 2 && y > 39) ||
		     (x == 2 && (y > 50 && y != 100)))) {
		    /* If x = 0 or 1 then y has to be 0...39, for x = 3
		       it can take any value but there are no known 
		       assigned values over 50 except for one contrived
		       example in X.690 which sets y = 100, so if we see
		       something outside this range it's most likely an 
		       encoding error rather than some bizarre new ID 
		       that's just appeared */
		    validEncoding = FALSE;
		    break;
		}
		length = sprintf(textOID, "%ld %ld", x, y);

		/* An insane ITU facility lets people register UUIDs as OIDs
		   (see http://www.itu.int/ITU-T/asn1/uuid.html), if we find
		   one of these, which live under the arc '1 25' = 0x69 we
		   have to continue decoding the OID as a UUID instead of a
		   standard OID */
		if (data == 0x69)
		    isUUID = TRUE;
	    } else
		length += sprintf(textOID + length, " %ld", value);
	    value = 0;
	}
    }
    if (value != 0) {
	/* We stopped in the middle of a continued value */
	validEncoding = FALSE;
    }
    textOID[length] = '\0';
    *textOIDlength = length;

    return validEncoding;
}

/** Dump a bitstring, reversing the bits into standard order. */
static void dumpBitString(rpmasn asn, const int length,
			  const int unused, const int level)
{
    unsigned int bitString = 0;
    unsigned int currentBitMask = 0x80;
    unsigned int remainderMask = 0xFF;
    int bitFlag;
    int value = 0;
    int noBits;
    int bitNo = -1;
    int i;
    char *errorStr = NULL;

    if (unused < 0 || unused > 7)
	complain(asn, "Invalid number of unused bits", level);
    noBits = (length * 8) - unused;

    /* ASN.1 bitstrings start at bit 0, reverse the bits (if necessary) */
    if (length) {
	bitString = rpmasnGetc(asn);
	asn->fPos++;
    }
    for (i = noBits - 8; i > 0; i -= 8) {
	bitString = (bitString << 8) | rpmasnGetc(asn);
	currentBitMask <<= 8;
	remainderMask = (remainderMask << 8) | 0xFF;
	asn->fPos++;
    }
    if (AF_ISSET(BITFLIP)) {
	for (i = 0, bitFlag = 1; i < noBits; i++) {
	    if (bitString & currentBitMask)
		value |= bitFlag;
	    if (!(bitString & remainderMask)) {
		/* The last valid bit should be a one bit */
		errorStr = "Spurious zero bits in bitstring";
	    }
	    bitFlag <<= 1;
	    bitString <<= 1;
	}
	if (noBits < (int)sizeof(int) && ((remainderMask << noBits) & value)) {
	    /* There shouldn't be any bits set after the last valid one.  We
	       have to do the noBits check to avoid a fencepost error when
	       there's exactly 32 bits */
	    errorStr = "Spurious one bits in bitstring";
	}
    } else
	value = bitString;

    /* Now that it's in the right order, dump it.  If there's only one bit
       set (which is often the case for bit flags) we also print the bit
       number to save users having to count the zeroes to figure out which
       flag is set */
    rpmasnIndent(asn, level + 1, 1);
    rpmasnPuts("\'", asn);
    if (AF_ISSET(BITFLIP))
	currentBitMask = 1 << (noBits - 1);
    for (i = 0; i < noBits; i++) {
	if (value & currentBitMask) {
	    bitNo = (bitNo == -1) ? (noBits - 1) - i : -2;
	    rpmasnPuts("1", asn);
	} else
	    rpmasnPuts("0", asn);
	currentBitMask >>= 1;
    }
    if (bitNo >= 0)
	fprintf(asn->ofp, "'B (bit %d)\n", bitNo);
    else
	rpmasnPuts("'B\n", asn);

    if (errorStr != NULL)
	complain(asn, errorStr, level);
}

/** Display data as a text string.
   Display data as a text string up to a maximum of 240 characters (8 lines
   of 48 chars to match the hex limit of 8 lines of 16 bytes) with special
   treatement for control characters and other odd things that can turn up
   in BMPString and UniversalString types.

   If the string is less than 40 chars in length, we try to print it on the
   same line as the rest of the text (even if it wraps), otherwise we break
   it up into 48-char chunks in a somewhat less nice text-dump format */

static void displayString(rpmasn asn, long length, int level,
			  STR_OPTION strOption)
{
    char timeStr[64];
    long noBytes = length;
    int lineLength = 48;
    int maxLevel = (AF_ISSET(PURE)) ? 15 : 8;
    int i;
    int firstTime = TRUE;
    int doTimeStr = FALSE;
    int warnIA5 = FALSE;
    int warnPrintable = FALSE;
    int warnTime = FALSE;
    int warnBMP = FALSE;

    if (noBytes > 384 && !AF_ISSET(ALLDATA))
	noBytes = 384;		/* Only output a maximum of 384 bytes */
    if (strOption == STR_UTCTIME || strOption == STR_GENERALIZED) {
	if ((strOption == STR_UTCTIME && length != 13) ||
	    (strOption == STR_GENERALIZED && length != 15))
	    warnTime = TRUE;
	else
	    doTimeStr = AF_ISSET(RAWTIME) ? FALSE : TRUE;
    }
    if (!doTimeStr && length <= 40)
	rpmasnPuts(" '", asn);	/* Print string on same line */
    if (level > maxLevel)
	level = maxLevel;	/* Make sure we don't go off edge of screen */

    for (i = 0; i < noBytes; i++) {
	int ch;

	/* Long strings (> 40 chars) are broken into multiple sections */
	if (length > 40 && !(i % lineLength)) {
	    if (!firstTime)
		rpmasnPuts("\'", asn);
	    rpmasnIndent(asn, level + 1, 1);
	    rpmasnPuts("\'", asn);
	    firstTime = FALSE;
	}
	ch = rpmasnGetc(asn);

	if (strOption == STR_BMP) {
	    if (i == noBytes - 1 && (noBytes & 1))
		/* Odd-length BMP string, complain */
		warnBMP = TRUE;
	    else {
		const wchar_t wCh = (ch << 8) | rpmasnGetc(asn);
		char outBuf[8];
		int outLen;

		/* Attempting to display Unicode characters is pretty hit and
		   miss, and if it fails nothing is displayed.  To try and
		   detect this we use wcstombs() to see if anything can be
		   displayed, if it can't we drop back to trying to display
		   the data as non-Unicode.  There's one exception to this
		   case, which is for a wrong-endianness Unicode string, for
		   which the first character looks like a single ASCII char */
		outLen = wcstombs(outBuf, &wCh, 1);
		if (outLen < 1) {
		    /* Can't be displayed as Unicode, fall back to
		       displaying it as normal text */
		    ungetc(wCh & 0xFF, asn->ifp);
		} else {
		    lineLength++;
		    i++;	/* We've read two characters for a wchar_t */

		    /* Some Unix environments differentiate between char
		       and wide-oriented stdout (!!!), so it's necessary to
		       manually switch the orientation of stdout to make it
		       wide-oriented before calling a widechar output
		       function or nothing will be output (exactly what
		       level of braindamage it takes to have an
		       implementation function like this is a mystery).  In
		       order to safely display widechars, we therefore have
		       to use the fwide() kludge function to change stdout
		       modes around the display of the widechar */
		    if (fwide(asn->ofp, 1) > 0) {
			fputwc(wCh, asn->ofp);
			fwide(asn->ofp, -1);
		    } else
			rpmasnPutc(wCh, asn);

		    asn->fPos += 2;
		    continue;
		}
	    }
	}

	switch (strOption) {
	case STR_PRINTABLE:
	case STR_IA5:
	case STR_LATIN1:
	    if (strOption == STR_PRINTABLE && !isPrintable(asn, ch))
		warnPrintable = TRUE;
	    if (strOption == STR_IA5 && !isIA5(asn, ch))
		warnIA5 = TRUE;
	    if (strOption == STR_LATIN1) {
		if (!isprint(ch & 0x7F))
		    ch = '.';	/* Convert non-ASCII to placeholders */
	    } else {
		if (!isprint(ch))
		    ch = '.';	/* Convert non-ASCII to placeholders */
	    }
	    break;

	case STR_UTCTIME:
	case STR_GENERALIZED:
	    if (!isdigit(ch) && ch != 'Z') {
		warnTime = TRUE;
		if (!isprint(ch))
		    ch = '.';	/* Convert non-ASCII to placeholders */
	    }
	    break;

	case STR_BMP_REVERSED:
	    if (i == noBytes - 1 && (noBytes & 1))
		warnBMP = TRUE;	/* Odd-length BMP string, complain */

	    /* Wrong-endianness BMPStrings (Microsoft Unicode) can't be
	       handled through the usual widechar-handling mechanism
	       above since the first widechar looks like an ASCII char
	       followed by a null terminator, so we just treat them as
	       ASCII chars, skipping the following zero byte.  This is
	       safe since the code that detects reversed BMPStrings
	       has already checked that every second byte is zero */
	    rpmasnGetc(asn);
	    i++;
	    asn->fPos++;
	    /* Drop through */

	default:
	    if (!isprint(ch))
		ch = '.';	/* Convert control chars to placeholders */
	}
	if (doTimeStr)
	    timeStr[i] = ch;
	else
	    rpmasnPutc(ch, asn);
	asn->fPos++;
    }

    if (length > 384 && !AF_ISSET(ALLDATA)) {
	length -= 384;
	rpmasnIndent(asn, level + 5, 1);
	fprintf(asn->ofp, "[ Another %ld characters skipped ]", length);
	asn->fPos += length;
	while (length--) {
	    int ch = rpmasnGetc(asn);

	    if (strOption == STR_PRINTABLE && !isPrintable(asn, ch))
		warnPrintable = TRUE;
	    if (strOption == STR_IA5 && !isIA5(asn, ch))
		warnIA5 = TRUE;
	}
    } else {
	if (doTimeStr) {
	    const char *timeStrPtr = (strOption == STR_UTCTIME) ?
		timeStr : timeStr + 2;

	    fprintf(asn->ofp, " %c%c/%c%c/", timeStrPtr[4], timeStrPtr[5],
		    timeStrPtr[2], timeStrPtr[3]);
	    if (strOption == STR_UTCTIME)
		fprintf(asn->ofp, (timeStr[0] < '5') ? "20" : "19");
	    else
		fprintf(asn->ofp, "%c%c", timeStr[0], timeStr[1]);
	    fprintf(asn->ofp, "%c%c %c%c:%c%c:%c%c GMT", timeStrPtr[0],
		    timeStrPtr[1], timeStrPtr[6], timeStrPtr[7],
		    timeStrPtr[8], timeStrPtr[9], timeStrPtr[10],
		    timeStrPtr[11]);
	} else
	    rpmasnPuts("\'", asn);
    }
    rpmasnPuts("\n", asn);

    /* Display any problems we encountered */
    if (warnPrintable)
	complain(asn, "PrintableString contains illegal character(s)", level);
    if (warnIA5)
	complain(asn, "IA5String contains illegal character(s)", level);
    if (warnTime)
	complain(asn, "Time is encoded incorrectly", level);
    if (warnBMP)
	complain(asn, "BMPString has missing final byte/half character", level);
}

/* ===== ASN.1 Parsing Routines	*/

/** Get an integer value. */
static long getValue(rpmasn asn, const long length)
{
    long value = 0;
    int i;

    for (i = 0; i < length; i++)
	value = (value << 8) | rpmasnGetc(asn);
    asn->fPos += length;

    return value;
}

/** Get an ASN.1 objects tag and length. */
static int getItem(rpmasn asn, ASN1_ITEM * item)
{
    int tag;
    int length;
    int index = 0;

    memset(item, 0, sizeof(ASN1_ITEM));
    item->indefinite = FALSE;
    tag = item->header[index++] = rpmasnGetc(asn);
    item->id = tag & ~TAG_MASK;
    tag &= TAG_MASK;
    if (tag == TAG_MASK) {
	int value;

	/* Long tag encoded as sequence of 7-bit values.  This doesn't try to
	   handle tags > INT_MAX, it'd be pretty peculiar ASN.1 if it had to
	   use tags this large */
	tag = 0;
	do {
	    value = rpmasnGetc(asn);
	    tag = (tag << 7) | (value & 0x7F);
	    item->header[index++] = value;
	    asn->fPos++;
	}
	while (value & LEN_XTND && index < 5 && !feof(asn->ifp));
	if (index == 5) {
	    asn->fPos++;		/* Tag */
	    return FALSE;
	}
    }
    item->tag = tag;
    if (feof(asn->ifp)) {
	asn->fPos++;
	return FALSE;
    }
    asn->fPos += 2;			/* Tag + length */
    length = item->header[index++] = rpmasnGetc(asn);
    item->headerSize = index;
    if (length & LEN_XTND) {
	int i;

	length &= LEN_MASK;
	if (length > 4) {
	    /* Impossible length value, probably we've run into the weeds */
	    return -1;
	}
	item->headerSize += length;
	item->length = 0;
	if (!length)
	    item->indefinite = TRUE;
	for (i = 0; i < length; i++) {
	    int ch = rpmasnGetc(asn);

	    item->length = (item->length << 8) | ch;
	    item->header[i + index] = ch;
	}
	asn->fPos += length;
    } else
	item->length = length;

    return TRUE;
}

/** Check whether a BIT STRING or OCTET STRING encapsulates another object. */
static int checkEncapsulate(rpmasn asn, const int length)
{
    ASN1_ITEM nestedItem;
    const int currentPos = asn->fPos;
    int diffPos;

    /* If we're not looking for encapsulated objects, return */
    if (!AF_ISSET(ENCAPS))
	return FALSE;

    /* Read the details of the next item in the input stream */
    getItem(asn, &nestedItem);
    diffPos = asn->fPos - currentPos;
    asn->fPos = currentPos;
    assert(fseek(asn->ifp, -diffPos, SEEK_CUR) == 0);

    /* If it's not a standard tag class, don't try and dig down into it */
    if ((nestedItem.id & CLASS_MASK) != UNIVERSAL &&
	(nestedItem.id & CLASS_MASK) != CONTEXT)
	return FALSE;

    /* If it doesn't fit exactly within the current item it's not an
       encapsulated object */
    if (nestedItem.length != length - diffPos)
	return FALSE;

    /* If it doesn't have a valid-looking tag, don't try and go any further */
    if (nestedItem.tag <= 0 || nestedItem.tag > 0x31)
	return FALSE;

    /* Now things get a bit complicated because it's possible to get some
       (very rare) false positives, for example if a NUMERICSTRING of
       exactly the right length is nested within an OCTET STRING, since
       numeric values all look like constructed tags of some kind.  To
       handle this we look for nested constructed items that should really
       be primitive */
    if ((nestedItem.id & FORM_MASK) == PRIMITIVE)
	return TRUE;

    /* It's constructed, make sure that it's something for which it makes
       sense as a constructed object.  At worst this will give some false
       negatives for really weird objects (nested constructed strings inside
       OCTET STRINGs), but these should probably never occur anyway */
    if (nestedItem.tag == SEQUENCE || nestedItem.tag == SET)
	return TRUE;

    return FALSE;
}

/** Check whether a zero-length item is OK. */
static int zeroLengthOK(rpmasn asn, const ASN1_ITEM * item)
{
    /* An implicitly-tagged NULL can have a zero length.  An occurrence of this
       type of item is almost always an error, however OCSP uses a weird status
       encoding that encodes result values in tags and then has to use a NULL
       value to indicate that there's nothing there except the tag that encodes
       the status, so we allow this as well if zero-length content is explicitly
       enabled */
    if (AF_ISSET(ZEROLEN) && (item->id & CLASS_MASK) == CONTEXT)
	return TRUE;

    /* If we can't recognise the type from the tag, reject it */
    if ((item->id & CLASS_MASK) != UNIVERSAL)
	return FALSE;

    /* The following types are zero-length by definition */
    if (item->tag == EOC || item->tag == NULLTAG)
	return TRUE;

    /* A real with a value of zero has zero length */
    if (item->tag == REAL)
	return TRUE;

    /* Everything after this point requires input from the user to say that
       zero-length data is OK (usually it's not, so we flag it as a
       problem) */
    if (!AF_ISSET(ZEROLEN))
	return FALSE;

    /* String types can have zero length except for the Unrestricted
       Character String type ([UNIVERSAL 29]) which has to have at least one
       octet for the CH-A/CH-B index */
    if (item->tag == OCTETSTRING || item->tag == NUMERICSTRING ||
	item->tag == PRINTABLESTRING || item->tag == T61STRING ||
	item->tag == VIDEOTEXSTRING || item->tag == VISIBLESTRING ||
	item->tag == IA5STRING || item->tag == GRAPHICSTRING ||
	item->tag == GENERALSTRING || item->tag == UNIVERSALSTRING ||
	item->tag == BMPSTRING || item->tag == UTF8STRING ||
	item->tag == OBJDESCRIPTOR)
	return TRUE;

    /* SEQUENCE and SET can be zero if there are absent optional/default
       components */
    if (item->tag == SEQUENCE || item->tag == SET)
	return TRUE;

    return FALSE;
}

/** Check whether the next item looks like text. */
static STR_OPTION checkForText(rpmasn asn, const int length)
{
    char buffer[16];
    int isBMP = FALSE;
    int isUnicode = FALSE;
    int sampleLength = (length < 16 ? length : 16);
    int i;
    STR_OPTION rc = STR_NONE;

    /* If the sample is very short, we're more careful about what we
       accept */
    if (sampleLength < 4) {
	/* If the sample size is too small, don't try anything */
	if (sampleLength <= 2)
	    goto exit;

	/* For samples of 3-4 characters we only allow ASCII text.  These
	   short strings are used in some places (eg PKCS #12 files) as
	   IDs */
	sampleLength = fread(buffer, 1, sampleLength, asn->ifp);
	assert(fseek(asn->ifp, -sampleLength, SEEK_CUR) == 0);
	for (i = 0; i < sampleLength; i++) {
	    const int ch = byteToInt(buffer[i]);

	    if (!(isalpha(ch) || isdigit(ch) || isspace(ch)))
		goto exit;
	}
	rc = STR_IA5;
	goto exit;
    }

    /* Check for ASCII-looking text */
    sampleLength = fread(buffer, 1, sampleLength, asn->ifp);
    assert(fseek(asn->ifp, -sampleLength, SEEK_CUR) == 0);
    if (isdigit(byteToInt(buffer[0])) &&
	(length == 13 || length == 15) && buffer[length - 1] == 'Z') {
	/* It looks like a time string, make sure that it really is one */
	for (i = 0; i < length - 1; i++) {
	    if (!isdigit(byteToInt(buffer[i])))
		break;
	}
	if (i == length - 1) {
	    rc = (length == 13 ? STR_UTCTIME : STR_GENERALIZED);
	    goto exit;
	}
    }

    for (i = 0; i < sampleLength; i++) {
	/* If even bytes are zero, it could be a BMPString.  Initially
	   we set isBMP to FALSE, if it looks like a BMPString we set it to
	   TRUE, if we then encounter a nonzero byte it's neither an ASCII
	   nor a BMPString */
	if (!(i & 1)) {
	    if (!buffer[i]) {
		/* If we thought we were in a Unicode string but we've found a
		   zero byte where it'd occur in a BMP string, it's neither a
		   Unicode nor BMP string */
		if (isUnicode)
		    goto exit;

		/* We've collapsed the eigenstate (in an earlier incarnation
		   isBMP could take values of -1, 0, or 1, with 0 being
		   undecided, in which case this comment made a bit more
		   sense) */
		if (i < sampleLength - 2) {
		    /* If the last char(s) are zero but preceding ones
		       weren't, don't treat it as a BMP string.  This can
		       happen when storing a null-terminated string if the
		       implementation gets the length wrong and stores the
		       null as well */
		    isBMP = TRUE;
		}
		continue;
	    } else {
		/* If we thought we were in a BMPString but we've found a
		   nonzero byte where there should be a zero, it's neither
		   an ASCII nor BMP string */
		if (isBMP)
		    goto exit;
	    }
	} else {
	    /* Just to make it tricky, Microsoft stuff Unicode strings into
	       some places (to avoid having to convert them to BMPStrings,
	       presumably) so we have to check for these as well */
	    if (!buffer[i]) {
		if (isBMP)
		    goto exit;
		isUnicode = TRUE;
		continue;
	    } else {
		if (isUnicode)
		    goto exit;
	    }
	}
	if (buffer[i] < 0x20 || buffer[i] > 0x7E)
	    goto exit;
    }

    /* It looks like a text string */
    rc = (isUnicode ? STR_BMP_REVERSED : isBMP ? STR_BMP : STR_IA5);

exit:
    return rc;
}

/** Dump the header bytes for an object. */
static void dumpHeader(rpmasn asn, const ASN1_ITEM * item)
{
    int extraLen = 24 - item->headerSize;
    int i;

    /* Dump the tag and length bytes */
    if (!AF_ISSET(PURE))
	rpmasnPuts("    ", asn);
    fprintf(asn->ofp, "<%02X", *item->header);

    for (i = 1; i < item->headerSize; i++)
	fprintf(asn->ofp, " %02X", item->header[i]);

    /* If we're asked for more, dump enough extra data to make up 24 bytes.
       This is somewhat ugly since it assumes we can seek backwards over the
       data, which means it won't always work on streams */
    if (extraLen > 0 && asn->hdrlevel > 1) {
	/* Make sure that we don't print too much data.  This doesn't work
	   for indefinite-length data, we don't try and guess the length with
	   this since it involves picking apart what we're printing */
	if (extraLen > item->length && !item->indefinite)
	    extraLen = (int) item->length;

	for (i = 0; i < extraLen; i++) {
	    int ch = rpmasnGetc(asn);

	    if (feof(asn->ifp))
		extraLen = i;	/* Exit loop and get fseek() correct */
	    else
		fprintf(asn->ofp, " %02X", ch);
	}
	assert(fseek(asn->ifp, -extraLen, SEEK_CUR) == 0);
    }

    rpmasnPuts(">\n", asn);
}

/* forward ref */
static int printAsn1(rpmasn asn, const int level, long length,
		     const int isIndefinite);

/** Print a constructed ASN.1 object. */
static void printConstructed(rpmasn asn, int level, const ASN1_ITEM * item)
{
    int result;

    /* Special case for zero-length objects */
    if (!item->length && !item->indefinite) {
	rpmasnPuts(" {}\n", asn);
	return;
    }

    rpmasnPuts(" {\n", asn);
    result = printAsn1(asn, level + 1, item->length, item->indefinite);
    if (result) {
	fprintf(asn->ofp,
		"Error: Inconsistent object length, %d byte%s difference.\n",
		result, (result > 1) ? "s" : "");
	asn->noErrors++;
    }
    rpmasnIndent(asn, level + 1, 0);	/* XXX unset ASN_FLAGS_SHALLOW? */
    rpmasnPuts("}\n", asn);
}

/** Print a single ASN.1 object. */
static void printASN1object(rpmasn asn, ASN1_ITEM * item, int level)
{
    OIDINFO *oidInfo;
    STR_OPTION stringType;
    unsigned char buffer[MAX_OID_SIZE];
    long value;

    if ((item->id & CLASS_MASK) != UNIVERSAL) {
	static const char *const classtext[] =
	    { "UNIVERSAL ", "APPLICATION ", "", "PRIVATE " };

	/* Print the object type */
	fprintf(asn->ofp, "[%s%d]",
		classtext[(item->id & CLASS_MASK) >> 6], item->tag);

	/* Perform a sanity check */
	if ((item->tag != NULLTAG) && (item->length < 0)) {
	    int i;

	    fflush(asn->ofp);
	    fprintf(stderr,
		    "\nError: Object has bad length field, tag = %02X, "
		    "length = %lX, value =", item->tag, item->length);
	    fprintf(stderr, "<%02X", *item->header);
	    for (i = 1; i < item->headerSize; i++)
		fprintf(stderr, " %02X", item->header[i]);
	    fputs(">.\n", stderr);
	    exit(EXIT_FAILURE);
	}

	if (!item->length && !item->indefinite && !zeroLengthOK(asn, item)) {
	    rpmasnPuts("\n", asn);
	    complain(asn, "Object has zero length", level);
	    return;
	}

	/* If it's constructed, print the various fields in it */
	if ((item->id & FORM_MASK) == CONSTRUCTED) {
	    printConstructed(asn, level, item);
	    return;
	}

	/* It's primitive, if it's a seekable stream try and determine
	   whether it's text so we can display it as such */
	if (!AF_ISSET(STDIN) &&
	    (stringType = checkForText(asn, item->length)) != STR_NONE)
	{
	    /* It looks like a text string, dump it as text */
	    displayString(asn, item->length, level, stringType);
	    return;
	}

	/* This could be anything, dump it as hex data */
	dumpHex(asn, item->length, level, FALSE);

	return;
    }

    /* Print the object type */
    rpmasnPuts(idstr(item->tag), asn);

    /* Perform a sanity check */
    if ((item->tag != NULLTAG) && (item->length < 0)) {
	int i;

	fflush(asn->ofp);
	fprintf(stderr,
		"\nError: Object has bad length field, tag = %02X, "
		"length = %lX, value =", item->tag, item->length);
	fprintf(stderr, "<%02X", *item->header);
	for (i = 1; i < item->headerSize; i++)
	    fprintf(stderr, " %02X", item->header[i]);
	fputs(">.\n", stderr);
	exit(EXIT_FAILURE);
    }

    /* If it's constructed, print the various fields in it */
    if ((item->id & FORM_MASK) == CONSTRUCTED) {
	printConstructed(asn, level, item);
	return;
    }

    /* It's primitive */
    if (!item->length && !zeroLengthOK(asn, item)) {
	rpmasnPuts("\n", asn);
	complain(asn, "Object has zero length", level);
	return;
    }
    switch (item->tag) {
    case BOOLEAN:
	{
	    int ch;

	    ch = rpmasnGetc(asn);
	    rpmasnPuts((ch ? " TRUE\n" : " FALSE\n"), asn);
	    if (ch != 0 && ch != 0xFF)
		complain(asn, "BOOLEAN has non-DER encoding", level);
	    asn->fPos++;
	    break;
	}

    case INTEGER:
    case ENUMERATED:
	if (item->length > 4)
	    dumpHex(asn, item->length, level, TRUE);
	else {
	    value = getValue(asn, item->length);
	    fprintf(asn->ofp, " %ld\n", value);
	    if (value < 0)
		complain(asn, "Integer has a negative value", level);
	}
	break;

    case BITSTRING:
	{
	    int ch;

	    if ((ch = rpmasnGetc(asn)) != 0)
		fprintf(asn->ofp, " %d unused bit%s",
			ch, (ch != 1) ? "s" : "");
	    asn->fPos++;
	    if (!--item->length && !ch) {
		rpmasnPuts("\n", asn);
		complain(asn, "Object has zero length", level);
		return;
	    }
	    if (item->length <= (int)sizeof(int)) {
		/* It's short enough to be a bit flag, dump it as a sequence
		   of bits */
		dumpBitString(asn, (int) item->length, ch, level);
		break;
	    }
	    /* Drop through to dump it as an octet string */
	}

    case OCTETSTRING:
	if (checkEncapsulate(asn, item->length)) {
	    /* It's something encapsulated inside the string, print it as
	       a constructed item */
	    rpmasnPuts(", encapsulates", asn);
	    printConstructed(asn, level, item);
	    break;
	}
	if (!AF_ISSET(STDIN) && !AF_ISSET(TEXT) &&
	    (stringType = checkForText(asn, item->length)) != STR_NONE)
	{
	    /* If we'd be doing a straight hex dump and it looks like
	       encapsulated text, display it as such.  If the user has
	       overridden character set type checking and it's a string
	       type for which we normally perform type checking, we reset
	       its type to none */
	    displayString(asn, item->length, level,
			  (!AF_ISSET(CHARSET) && (stringType == STR_IA5 ||
					     stringType ==
					     STR_PRINTABLE)) ? STR_NONE :
			  stringType);
	    return;
	}
	dumpHex(asn, item->length, level, FALSE);
	break;

    case OID:
	{
	    char textOID[128];
	    int length;
	    int isValid;
	    size_t nr;

	    /* Hierarchical Object Identifier */
	    if (item->length > MAX_OID_SIZE) {
		fflush(asn->ofp);
		fprintf(stderr,
			"\nError: Object identifier length %ld too "
			"large.\n", item->length);
		exit(EXIT_FAILURE);
	    }
	    nr = fread(buffer, 1, (size_t) item->length, asn->ifp);
	    asn->fPos += item->length;
	    oidInfo = getOIDinfo(asn, buffer, (int) item->length);
	    if (oidInfo != NULL) {
		/* Convert the binary OID to text form */
		isValid = oidToString(textOID, &length, buffer,
				      (int) item->length);

		/* Check if LHS status info + indent + "OID " string + oid
		   name + "(" + oid value + ")" will wrap */
		if (((AF_ISSET(PURE)) ? 0 : INDENT_SIZE) + (level * 2) + 18 +
		    strlen(oidInfo->description) + 2 + length >=
		    asn->outwidth)
		{
		    rpmasnIndent(asn, level + 1, 1);
		} else
		    rpmasnPuts(" ", asn);
		fprintf(asn->ofp, "%s (%s)\n", oidInfo->description,
			textOID);

		/* Display extra comments about the OID if required */
		if (AF_ISSET(OIDS) && oidInfo->comment != NULL) {
		    rpmasnIndent(asn, level + 1, 0);
		    fprintf(asn->ofp, "(%s)\n", oidInfo->comment);
		}
		if (!isValid)
		    complain(asn, "OID has invalid encoding", level);

		/* If there's a warning associated with this OID, remember
		   that there was a problem */
		if (oidInfo->warn)
		    asn->noWarnings++;

		break;
	    }

	    /* Print the OID as a text string */
	    isValid = oidToString(textOID, &length, buffer, (int) item->length);
	    fprintf(asn->ofp, " '%s'\n", textOID);
	    if (!isValid)
		complain(asn, "OID has invalid encoding", level);
	    break;
	}

    case EOC:
    case NULLTAG:
	rpmasnPuts("\n", asn);
	break;

    case OBJDESCRIPTOR:
    case GRAPHICSTRING:
    case VISIBLESTRING:
    case GENERALSTRING:
    case UNIVERSALSTRING:
    case NUMERICSTRING:
    case VIDEOTEXSTRING:
    case UTF8STRING:
	displayString(asn, item->length, level, STR_NONE);
	break;
    case PRINTABLESTRING:
	displayString(asn, item->length, level, STR_PRINTABLE);
	break;
    case BMPSTRING:
	displayString(asn, item->length, level, STR_BMP);
	break;
    case UTCTIME:
	displayString(asn, item->length, level, STR_UTCTIME);
	break;
    case GENERALIZEDTIME:
	displayString(asn, item->length, level, STR_GENERALIZED);
	break;
    case IA5STRING:
	displayString(asn, item->length, level, STR_IA5);
	break;
    case T61STRING:
	displayString(asn, item->length, level, STR_LATIN1);
	break;

    default:
	rpmasnIndent(asn, level + 1, 1);
	rpmasnPuts("Unrecognised primitive, hex value is:", asn);
	dumpHex(asn, item->length, level, FALSE);
	asn->noErrors++;		/* Treat it as an error */
    }
}

/** Print a complex ASN.1 object. */
static int printAsn1(rpmasn asn, const int level, long length,
		     const int isIndefinite)
{
    ASN1_ITEM item;
    long lastPos = asn->fPos;
    int seenEOC = FALSE;
    int status;
    int rc = 0;

    /* Special-case for zero-length objects */
    if (!length && !isIndefinite)
	goto exit;

    while ((status = getItem(asn, &item)) > 0) {
	/* Perform various special checks the first time we're called */
	if (length == LENGTH_MAGIC) {
	    /* If the length isn't known and the item has a definite length,
	       set the length to the item's length */
	    if (!item.indefinite)
		length = item.headerSize + item.length;

	    /* If the input isn't seekable, turn off some options that
	       require the use of fseek().  This check isn't perfect (some
	       streams are slightly seekable due to buffering) but it's
	       better than nothing */
	    if (fseek(asn->ifp, -item.headerSize, SEEK_CUR) != 0) {
		asn->flags |= ASN_FLAGS_STDIN;
		asn->flags &= ~ASN_FLAGS_ENCAPS;
		puts("Warning: Input is non-seekable, some functionality "
		     "has been disabled.");
	    } else
		assert(fseek(asn->ifp, item.headerSize, SEEK_CUR) == 0);
	}

	/* Dump the header as hex data if requested */
	if (asn->hdrlevel)
	    dumpHeader(asn, &item);

	/* Print offset into buffer, tag, and length */
	if (item.header[0] == EOC) {
	    seenEOC = TRUE;
	    if (!isIndefinite)
		complain(asn, "Spurious EOC in definite-length item", level);
	}

	if (!AF_ISSET(PURE)) {
	    if (item.indefinite)
		fprintf(asn->ofp, AF_ISSET(HEX) ? "%04lX NDEF: " :
			"%4ld NDEF: ", lastPos);
	    else {
		if (!seenEOC)
		    fprintf(asn->ofp,
			AF_ISSET(HEX) ? "%04lX %4lX: " : "%4ld %4ld: ",
			lastPos, item.length);
	    }
	}

	/* Print details on the item */
	if (!seenEOC) {
int flags = asn->flags;
asn->flags |= ASN_FLAGS_PURE;	/* XXX Skip INDENT_STRING. */
	    rpmasnIndent(asn, level, 0);
asn->flags = flags;
	    printASN1object(asn, &item, level);
	}

	/* If it was an indefinite-length object (no length was ever set) and
	   we've come back to the top level, exit */
	if (length == LENGTH_MAGIC)
	    goto exit;

	length -= asn->fPos - lastPos;
	lastPos = asn->fPos;
	if (isIndefinite) {
	    if (seenEOC)
		goto exit;
	} else {
	    if (length <= 0) {
		if (length < 0)
		    rc = (int) -length;
		goto exit;
	    } else {
		if (length == 1) {
		    const int ch = rpmasnGetc(asn);

		    /* No object can be one byte long, try and recover.  This
		       only works sometimes because it can be caused by
		       spurious data in an OCTET STRING hole or an incorrect
		       length encoding.  The following workaround tries to
		       recover from spurious data by skipping the byte if
		       it's zero or a non-basic-ASN.1 tag, but keeping it if
		       it could be valid ASN.1 */
		    if (ch && ch <= 0x31)
			ungetc(ch, asn->ifp);
		    else {
			asn->fPos++;
			rc = 1;
			goto exit;
		    }
		}
	    }
	}
    }
    if (status == -1) {
	int i;

	fflush(asn->ofp);
	fprintf(stderr, "\nError: Invalid data encountered at position "
		"%d:", asn->fPos);
	for (i = 0; i < item.headerSize; i++)
	    fprintf(stderr, " %02X", item.header[i]);
	fprintf(stderr, ".\n");
	exit(EXIT_FAILURE);
    }

    /* If we see an EOF and more data is expected, complain */
    if (length && length != LENGTH_MAGIC) {
	fprintf(asn->ofp, "Error: Inconsistent object length, %ld byte%s "
		"difference.\n", length, (length > 1) ? "s" : "");
	asn->noErrors++;
    }

exit:
    return rc;
}

/*==============================================================*/

static struct poptOption rpmasnInputOptionsTable[] = {
  { NULL, 'c', POPT_ARG_STRING,	&__rpmasn.cfn, 0,
	N_("Read Object Identifier info from alternate config file"), N_("<file>") },
  { NULL, 'n', POPT_ARG_LONG,	&__rpmasn.offset, 0,
	N_("Start <number> bytes into the file"), N_("<number>") },
  POPT_TABLEEND
};

static struct poptOption rpmasnOutputOptionsTable[] = {
  { NULL, 'f', POPT_ARG_STRING,	&__rpmasn.ofn, 0,
	N_("Dump object at offset -<number> to file (allows data to be extracted from encapsulating objects)"), N_("<file>") },
  { NULL, 'w', POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT,	&__rpmasn.outwidth, 0,
	N_("Set width of output"), N_("<number>") },
  POPT_TABLEEND
};

static struct poptOption rpmasnDisplayOptionsTable[] = {
  { NULL, 'a', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_ALLDATA,
	N_("Print all data in data blocks, not just first 128 bytes"), NULL },
  { NULL, 'd', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_DOTS,
	N_("Print dots to show column alignment"), NULL },

  { NULL, 'h', POPT_ARG_INT|POPT_ARGFLAG_CALCULATOR,	&__rpmasn.hdrlevel, 1,
	N_("Hex dump object header (tag+length) before decoded output"), "+" },
#ifdef	DYING
  { "hh", '\0', POPT_BIT_SET|POPT_ARGFLAG_ONEDASH,	&__rpmasn.flags, ASN_FLAGS_SHALLOW,
	N_("Same as -h but display more of the object as hex data"), NULL },
#endif

  { NULL, 'i', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_SHALLOW,
	N_("Use shallow indenting, for deeply-nested objects"), NULL },
  { NULL, 'l', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_OIDS,
	N_("Long format, display extra info about Object Identifiers"), NULL },
  { NULL, 'p', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_PURE,
	N_("Pure ASN.1 output without encoding information"), NULL },
  { NULL, 't', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_TEXT,
	N_("Display text values next to hex dump of data"), NULL },
  POPT_TABLEEND
};

static struct poptOption rpmasnFormatOptionsTable[] = {
  { NULL, 'e', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_ENCAPS,
	N_("Don't print encapsulated data inside OCTET/BIT STRINGs"), NULL },
  { NULL, 'r', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_BITFLIP,
	N_("Print bits in BIT STRING as encoded in reverse order"), NULL },
  { NULL, 'u', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_RAWTIME,
	N_("Don't format UTCTime/GeneralizedTime string data"), NULL },
  { NULL, 'x', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_HEX,
	N_("Display size and offset in hex not decimal"), NULL },
  POPT_TABLEEND
};

static struct poptOption rpmasnCheckOptionsTable[] = {
  { NULL, 'o', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_CHARSET,
	N_("Don't validate character strings hidden in octet strings"), NULL },
  { NULL, 's', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_CHKONLY,
	N_("Syntax check only, don't dump ASN.1 structures"), NULL },
  { NULL, 'z', POPT_BIT_SET,	&__rpmasn.flags, ASN_FLAGS_ZEROLEN,
	N_("Allow zero-length items"), NULL },
  POPT_TABLEEND
};

static struct poptOption rpmasnOptionsTable[] = {

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmasnInputOptionsTable, 0,
        N_(" Input options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmasnOutputOptionsTable, 0,
        N_(" Output options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmasnDisplayOptionsTable, 0,
        N_(" Display options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmasnFormatOptionsTable, 0,
        N_(" Format options:"), NULL },

  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmasnCheckOptionsTable, 0,
        N_(" Checking options:"), NULL },

#ifdef	NOTYET
 { NULL, '\0', POPT_ARG_INCLUDE_TABLE, rpmioAllPoptTable, 0,
	N_(" Common options for all rpmio executables:"), NULL },
#endif

  POPT_AUTOALIAS
  POPT_AUTOHELP

  { NULL, (char)-1, POPT_ARG_INCLUDE_TABLE, NULL, 0,
        N_("\
DumpASN1 - ASN.1 object dump/syntax check program.\n\
Copyright Peter Gutmann 1997 - 2010.  Last updated " UPDATE_STRING ".\n\
\n\
Usage: dumpasn1 [-acdefhlprstuxz] <file>\n\
  Input options:\n\
       - = Take input from stdin (some options may not work properly)\n\
       -<number> = Start <number> bytes into the file\n\
       -- = End of arg list\n\
       -c<file> = Read Object Identifier info from alternate config file\n\
            (values will override equivalents in global config file)\n\
\n\
  Output options:\n\
       -f<file> = Dump object at offset -<number> to file (allows data to be\n\
            extracted from encapsulating objects)\n\
       -w<number> = Set width of output, default = 80 columns\n\
\n\
  Display options:\n\
       -a = Print all data in long data blocks, not just the first 128 bytes\n\
       -d = Print dots to show column alignment\n\
       -h = Hex dump object header (tag+length) before the decoded output\n\
       -hh = Same as -h but display more of the object as hex data\n\
       -i = Use shallow indenting, for deeply-nested objects\n\
       -l = Long format, display extra info about Object Identifiers\n\
       -p = Pure ASN.1 output without encoding information\n\
       -t = Display text values next to hex dump of data\n\
\n\
  Format options:\n\
       -e = Don't print encapsulated data inside OCTET/BIT STRINGs\n\
       -r = Print bits in BIT STRING as encoded in reverse order\n\
       -u = Don't format UTCTime/GeneralizedTime string data\n\
       -x = Display size and offset in hex not decimal\n\
\n\
  Checking options:\n\
       -o = Don't check validity of character strings hidden in octet strings\n\
       -s = Syntax check only, don't dump ASN.1 structures\n\
       -z = Allow zero-length items\n\
\n\
Warnings generated by deprecated OIDs require the use of '-l' to be displayed.\n\
Program return code is the number of errors found or EXIT_SUCCESS.\n\
"), NULL },

  POPT_TABLEEND
};

int main(int argc, char *argv[])
{
    rpmasn asn = _rpmasn;
    poptContext con = rpmioInit(argc, argv, rpmasnOptionsTable);
    const char ** av = NULL;
    const char * ifn = NULL;
    int ec = EXIT_FAILURE;
    int ch;

    /* Display usage if no args given */
    if (argc < 2) {
poptSetOtherOptionHelp(con, "[-acdefhlnprstuxz] <file>");
poptPrintHelp(con, stderr, 0);
	goto exit;
    }
    asn->ofp = stdout;		/* Needs to be assigned at runtime */

    if (AF_ISSET(CHKONLY)) {
	FILE * _fp;

	/* Safety feature in case any Unix libc is as broken as Win32 version */
	_fp = freopen("/dev/null", "w", asn->ofp);
    }

    if (asn->cfn) {
	if (!readConfig(asn, asn->cfn, FALSE))
	    goto exit;
    }

    /* We can't use options that perform an fseek() if reading from stdin */
    if (AF_ISSET(STDIN) && (asn->hdrlevel || asn->ofn != NULL)) {
	puts("Can't use -f or -h when taking input from stdin");
	goto exit;
    }

    /* Check args and read the config file.  We don't bother weeding out
       dups during the read because (a) the linear search would make the
       process n^2, (b) during the dump process the search will terminate on
       the first match so dups aren't that serious, and (c) there should be
       very few dups present */
    av = poptGetArgs(con);
    if (!(av && av[0] && av[1] == NULL)) {
poptSetOtherOptionHelp(con, "[-acdefhlnprstuxz] <file>");
poptPrintHelp(con, stderr, 0);
	goto exit;
    }
ifn = av[0];

    if (!readGlobalConfig(asn, argv[0]))
	goto exit;

    /* Dump the given file */
    if (AF_ISSET(STDIN) || !strcmp(ifn, "-")) {
	asn->ifp = stdin;
	asn->flags |= ASN_FLAGS_STDIN;
    } else {
	if ((asn->ifp = fopen(ifn, "rb")) == NULL) {
	    perror(ifn);
	    goto exit;
	}
    }

    if (AF_ISSET(STDIN)) {
	while (asn->offset--) {
	    if ((ch = rpmasnGetc(asn)) == EOF)
		break;;
	}
    } else
	assert(fseek(asn->ifp, asn->offset, SEEK_SET) == 0);

    if (asn->ofn != NULL) {
	FILE * ofp;
	ASN1_ITEM item;
	long length;
	int status;
	int i;

	if ((ofp = fopen(asn->ofn, "wb")) == NULL) {
	    perror(asn->ofn);
	    goto exit;
	}
	/* Make sure that there's something there, and that it has a
	   definite length */
	status = getItem(asn, &item);
	if (status == -1) {
	    puts("Non-ASN.1 data encountered.");
	    goto exit;
	}
	if (status == 0) {
	    puts("Nothing to read.");
	    goto exit;
	}
	if (item.indefinite) {
	    puts("Cannot process indefinite-length item.");
	    goto exit;
	}

	/* Copy the item across, first the header and then the data */
	for (i = 0; i < item.headerSize; i++)
	    putc(item.header[i], ofp);
	for (length = 0; length < item.length && !feof(asn->ifp); length++)
	    putc(rpmasnGetc(asn), ofp);
	fclose(ofp);

	assert(fseek(asn->ifp, asn->offset, SEEK_SET) == 0);
    }

    printAsn1(asn, 0, LENGTH_MAGIC, 0);
    if (!AF_ISSET(STDIN) && asn->offset == 0) {
	unsigned char buffer[16];
	long position = ftell(asn->ifp);
	size_t nr;

	/* If we're dumping a standalone ASN.1 object and there's further
	   data appended to it, warn the user of its existence.  This is a
	   bit hit-and-miss since there may or may not be additional EOCs
	   present, dumpasn1 always stops once it knows that the data should
	   end (without trying to read any trailing EOCs) because data from
	   some sources has the EOCs truncated, and most apps know that they
	   have to stop at min( data_end, EOCs ).  To avoid false positives,
	   we skip at least 4 EOCs worth of data and if there's still more
	   present, we complain */
	nr = fread(buffer, 1, 8, asn->ifp);	/* Skip 4 EOCs */
	if (!feof(asn->ifp)) {
	    fprintf(asn->ofp, "Warning: Further data follows ASN.1 data at "
		    "position %ld.\n", position);
	    asn->noWarnings++;
	}
    }

    /* Print a summary of warnings/errors if it's required or appropriate */
    if (!AF_ISSET(PURE)) {
	fflush(asn->ofp);
	if (!AF_ISSET(CHKONLY))
	    fputc('\n', stderr);
	fprintf(stderr, "%d warning%s, %d error%s.\n", asn->noWarnings,
		(asn->noWarnings != 1) ? "s" : "", asn->noErrors,
		(asn->noErrors != 1) ? "s" : "");
    }
    ec = (asn->noErrors ? asn->noErrors : EXIT_SUCCESS);

exit:
    if (asn->ifp && asn->ifp != stdin)
	fclose(asn->ifp);
    asn->ifp = NULL;
    freeConfig(asn);

    asn->cfn = _free(asn->cfn);
    asn->ofn = _free(asn->ofn);

    con = rpmioFini(con);

    return ec;
}
