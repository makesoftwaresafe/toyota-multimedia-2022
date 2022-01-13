#include "system.h"

#include <rpmiotypes.h>

#include "debug.h"

/*==============================================================*/
/* --- Sexp.h */
/* SEXP standard header file: sexp.h 
 * Ronald L. Rivest
 * 6/29/1997
 */

/* GLOBAL DECLARATIONS */
#define TRUE    1
#define FALSE   0

/* PRINTING MODES */
#define CANONICAL 1		/* standard for hashing and tranmission */
#define BASE64    2		/* base64 version of canonical */
#define ADVANCED  3		/* pretty-printed */

/* ERROR MESSAGE LEVELS */
#define WARNING 1
#define ERROR 2

#define DEFAULTLINELENGTH 75

/* TYPES OF OBJECTS */
#define SEXP_STRING 1
#define SEXP_LIST   2

typedef struct sexp_simplestring_s sexpSimpleString;
typedef struct sexp_string_s sexpString;
typedef struct sexp_list_s sexpList;
typedef union sexp_object_s sexpObject;
typedef sexpList sexpIter;
typedef struct sexp_inputstream_s sexpInputStream;
typedef struct sexp_outputstream_s sexpOutputStream;

/* sexpSimpleString */
struct sexp_simplestring_s {
    size_t blen;
    size_t allocated;
    uint8_t * b;
};

/* sexpString */
struct sexp_string_s {
    int type;
    sexpSimpleString *presentationHint;
    sexpSimpleString *string;
};

/* sexpList */
/* If first is NULL, then rest must also be NULL; this is empty list */
struct sexp_list_s {
    char type;
    sexpObject *first;
    sexpList *rest;
};

/* sexpObject */
/* so we can have a pointer to something of either type */
union sexp_object_u {
    sexpString string;
    sexpList list;
};

/* sexpIter */
/* an "iterator" for going over lists */
/* In this implementation, it is the same as a list */

struct sexp_inputstream_s {
    int nextChar;		/* character currently being scanned */
    int byteSize;		/* 4 or 6 or 8 == currently scanning mode */
    int bits;			/* Bits waiting to be used */
    int nBits;			/* number of such bits waiting to be used */
    void (*getChar) (sexpInputStream *is);
    int count;			/* number of 8-bit characters output by getChar */
    FILE *inputFile;		/* where to get input, if not stdin */
};

struct sexp_outputstream_s {
    long int column;		/* column where next character will go */
    long int maxcolumn;		/* max usable column, or -1 if no maximum */
    long int indent;		/* current indentation level (starts at 0) */
    void (*putChar) (sexpOutputStream *os, int c); /* output a character */
    void (*newLine) (sexpOutputStream *os, int mode); /* go to next line (and indent) */
    int byteSize;		/* 4 or 6 or 8 depending on output mode */
    int bits;			/* bits waiting to go out */
    int nBits;			/* number of bits waiting to go out */
    long int base64Count;	/* number of hex or base64 chars printed 
				   this region */
    int mode;			/* BASE64, ADVANCED, or CANONICAL */
    FILE *outputFile;		/* where to put output, if not stdout */
};

/* Function prototypes */

/* sexp-basic */
void ErrorMessage(int level, char *msg, int c1, int c2);
void initializeMemory(void);
sexpSimpleString *newSimpleString(void);
long int simpleStringLength(sexpSimpleString *ss);
uint8_t * simpleStringString(sexpSimpleString *ss);
sexpSimpleString *reallocateSimpleString(sexpSimpleString *ss);
void appendCharToSimpleString(int c, sexpSimpleString *ss);
sexpString *newSexpString(void);
sexpSimpleString *sexpStringPresentationHint(sexpString *s);
sexpSimpleString *sexpStringString(sexpString *s);
void setSexpStringPresentationHint(sexpString *s, sexpSimpleString *ss);
void setSexpStringString(sexpString *s, sexpSimpleString *ss);
void closeSexpString(sexpString *s);
sexpList *newSexpList(void);
void sexpAddSexpListObject(sexpList *list, sexpObject *object);
void closeSexpList(sexpList *list);
sexpIter *sexpListIter(sexpList *list);
sexpIter *sexpIterNext(sexpIter *iter);
sexpObject *sexpIterObject(sexpIter *iter);
int isObjectString(sexpObject *object);
int isObjectList(sexpObject *object);

/* sexp-input */
void initializeCharacterTables(void);
int isWhiteSpace(int c);
int isDecDigit(int c);
int isHexDigit(int c);
int isBase64Digit(int c);
int isTokenChar(int c);
int isAlpha(int c);
void changeInputByteSize(sexpInputStream *is, int newByteSize);
void getChar(sexpInputStream *is);
sexpInputStream *newSexpInputStream(void);
void skipWhiteSpace(sexpInputStream *is);
void skipChar(sexpInputStream *is, int c);
void scanToken(sexpInputStream *is, sexpSimpleString *ss);
sexpObject *scanToEOF(sexpInputStream *is);
unsigned long int scanDecimal(sexpInputStream *is);
void scanVerbatimString(sexpInputStream *is, sexpSimpleString *ss, long int length);
void scanQuotedString(sexpInputStream *is, sexpSimpleString *ss, long int length);
void scanHexString(sexpInputStream *is, sexpSimpleString *ss, long int length);
void scanBase64String(sexpInputStream *is, sexpSimpleString *ss, long int length);
sexpSimpleString *scanSimpleString(sexpInputStream *is);
sexpString *scanString(sexpInputStream *is);
sexpList *scanList(sexpInputStream *is);
sexpObject *scanObject(sexpInputStream *is);

/* sexp-output */
void putChar(sexpOutputStream *os, int c);
void varPutChar(sexpOutputStream *os, int c);
void changeOutputByteSize(sexpOutputStream *os, int newByteSize, int mode);
void flushOutput(sexpOutputStream *os);
void newLine(sexpOutputStream *os, int mode);
sexpOutputStream *newSexpOutputStream(void);
void printDecimal(sexpOutputStream *os, long int n);
void canonicalPrintVerbatimSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
void canonicalPrintString(sexpOutputStream *os, sexpString *s);
void canonicalPrintList(sexpOutputStream *os, sexpList *list);
void canonicalPrintObject(sexpOutputStream *os, sexpObject *object);
void base64PrintWholeObject(sexpOutputStream *os, sexpObject *object);
int canPrintAsToken(sexpOutputStream *os, sexpSimpleString *ss);
void advancedPrintTokenSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
int advancedLengthSimpleStringToken(sexpSimpleString *ss);
void advancedPrintVerbatimSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
int advancedLengthSimpleStringVerbatim(sexpSimpleString *ss);
void advancedPrintBase64SimpleString(sexpOutputStream *os, sexpSimpleString *ss);
void advancedPrintHexSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
int advancedLengthSimpleStringHexadecimal(sexpSimpleString *ss);
int canPrintAsQuotedString(sexpSimpleString *ss);
void advancedPrintQuotedStringSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
int advancedLengthSimpleStringQuotedString(sexpSimpleString *ss);
void advancedPrintSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
void advancedPrintString(sexpOutputStream *os, sexpString *s);
int advancedLengthSimpleStringBase64(sexpSimpleString *ss);
int advancedLengthSimpleString(sexpOutputStream *os, sexpSimpleString *ss);
int advancedLengthString(sexpOutputStream *os, sexpString *s);
int advancedLengthList(sexpOutputStream *os, sexpList *list);
void advancedPrintList(sexpOutputStream *os, sexpList *list);
void advancedPrintObject(sexpOutputStream *os, sexpObject *object);

/*==============================================================*/
/* --- sexp-basic.c */
/* SEXP implementation code sexp-basic.c
 * List-based implementation of S-expressions.
 * Ron Rivest
 * 5/5/1997
 */

/******************/
/* ERROR MESSAGES */
/******************/

/* ErrorMessage(level,msg,c1,c2)
 * prints error message on standard output (msg is a c format string)
 * c1 and c2 are (optional) integer parameters for the message.
 * Terminates iff level==ERROR, otherwise returns.
 */
void ErrorMessage(int level, char *msg, int c1, int c2)
{
    fflush(stdout);
    printf("\n*** ");
    if (level == WARNING)
	printf("Warning: ");
    else if (level == ERROR)
	printf("Error: ");
    printf(msg, c1, c2);
    printf(" ***\n");
    if (level == ERROR)
	exit(1);
}

/***********************************/
/* SEXP SIMPLE STRING MANIPULATION */
/***********************************/

/* newSimpleString()
 * Creates and initializes new sexpSimpleString object.
 * Allocates 16-character buffer to hold string.
 */
sexpSimpleString *newSimpleString(void)
{
    sexpSimpleString *ss;
    ss = (sexpSimpleString *) xmalloc(sizeof(sexpSimpleString));
    ss->blen = 0;
    ss->allocated = 16;
    ss->b = (uint8_t *) xmalloc(16);
    return ss;
}

/* simpleStringLength(ss)
 * returns length of simple string 
 */
long int simpleStringLength(sexpSimpleString *ss)
{
    return ss->blen;
}

/* simpleStringString(ss)
 * returns pointer to character array of simple string 
 */
uint8_t * simpleStringString(sexpSimpleString *ss)
{
    return ss->b;
}

/* reallocateSimpleString(ss)
 * Changes space allocated to ss.
 * Space allocated is set to roughly 3/2 the current string length, plus 16.
 */
sexpSimpleString *reallocateSimpleString(sexpSimpleString *ss)
{
    uint8_t * newstring;
    size_t newsize;
    size_t i;

    if (ss == NULL)
	ss = newSimpleString();
    if (ss->b == NULL)
	ss->b = (uint8_t *) xmalloc(16);
    else {
	newsize = 16 + 3 * (ss->blen) / 2;
	newstring = (uint8_t *) xmalloc(newsize);
	for (i = 0; i < ss->blen; i++)
	    newstring[i] = ss->b[i];
	/* zeroize string before freeing; as it may be sensitive */
	for (i = 0; i < ss->allocated; i++)
	    ss->b[i] = 0;
	free(ss->b);
	ss->b = newstring;
	ss->allocated = newsize;
    }
    return ss;
}

/* appendCharToSimpleString(c,ss)
 * Appends the character c to the end of simple string ss.
 * Reallocates storage assigned to s if necessary to make room for c.
 */
void appendCharToSimpleString(int c, sexpSimpleString *ss)
{
    if (ss == NULL)
	ss = newSimpleString();
    if (ss->b == NULL || ss->blen == ss->allocated)
	ss = reallocateSimpleString(ss);
    ss->b[ss->blen] = (uint8_t) (c & 0xFF);
    ss->blen++;
}

/****************************/
/* SEXP STRING MANIPULATION */
/****************************/

/* newSexpString()
 * Creates and initializes a new sexpString object.
 * Both the presentation hint and the string are initialized to NULL.
 */
sexpString *newSexpString(void)
{
    sexpString *s;
    s = (sexpString *) xmalloc(sizeof(sexpString));
    s->type = SEXP_STRING;
    s->presentationHint = NULL;
    s->string = NULL;
    return s;
}

/* sexpStringPresentationHint()
 * returns presentation hint field of the string 
 */
sexpSimpleString *sexpStringPresentationHint(sexpString *s)
{
    return s->presentationHint;
}

/* setSexpStringPresentationHint()
 * assigns the presentation hint field of the string
 */
void setSexpStringPresentationHint(sexpString *s, sexpSimpleString *ss)
{
    s->presentationHint = ss;
}

/* setSexpStringString()
 * assigns the string field of the string
 */
void setSexpStringString(sexpString *s, sexpSimpleString *ss)
{
    s->string = ss;
}

/* sexpStringString()
 * returns the string field of the string
 */
sexpSimpleString *sexpStringString(sexpString *s)
{
    return s->string;
}

/* closeSexpString()
 * finish up string computations after created 
 */
void closeSexpString(sexpString *s)
{;
}				/* do nothing in this implementation */

/**************************/
/* SEXP LIST MANIPULATION */
/**************************/

/* newSexpList()
 * Creates and initializes a new sexpList object.
 * Both the first and rest fields are initialized to NULL, which is
 * SEXP's representation of an empty list.
 */
sexpList *newSexpList(void)
{
    sexpList *list;
    list = (sexpList *) xmalloc(sizeof(sexpList));
    list->type = SEXP_LIST;
    list->first = NULL;
    list->rest = NULL;
    return list;
}

/* sexpAddSexpListObject()
 * add object to end of list
 */
void sexpAddSexpListObject(sexpList *list, sexpObject *object)
{
    if (list->first == NULL)
	list->first = object;
    else {
	while (list->rest != NULL)
	    list = list->rest;
	list->rest = newSexpList();
	list = list->rest;
	list->first = object;
    }
}

/* closeSexpList()
 * finish off a list that has just been input
 */
void closeSexpList(sexpList *list)
{;
}				/* nothing in this implementation */

/* Iteration on lists.
   To accommodate different list representations, we introduce the
   notion of an "iterator".
*/

/* sexpListIter()
 * return the iterator for going over a list 
 */
sexpIter *sexpListIter(sexpList *list)
{
    return (sexpIter *) list;
}

/* sexpIterNext()
 * advance iterator to next element of list, or else return null
 */
sexpIter *sexpIterNext(sexpIter *iter)
{
    if (iter == NULL)
	return NULL;
    return (sexpIter *) (((sexpList *) iter)->rest);
}

/* sexpIterObject ()
 * return object corresponding to current state of iterator
 */
sexpObject *sexpIterObject(sexpIter *iter)
{
    if (iter == NULL)
	return NULL;
    return ((sexpList *) iter)->first;
}

/****************************/
/* SEXP OBJECT MANIPULATION */
/****************************/

int isObjectString(sexpObject *object)
{
    if (((sexpString *) object)->type == SEXP_STRING)
	return TRUE;
    else
	return FALSE;
}

int isObjectList(sexpObject *object)
{
    if (((sexpList *) object)->type == SEXP_LIST)
	return TRUE;
    else
	return FALSE;
}


/*==============================================================*/
/* --- sexp-input.c */
/* SEXP implementation code sexp-input.c
 * Ron Rivest
 * 7/21/1997
 */

/**************************************/
/* CHARACTER ROUTINES AND DEFINITIONS */
/**************************************/

char upper[256];		/* upper[c] is upper case version of c */
char whitespace[256];		/* whitespace[c] is nonzero if c is whitespace */
char decdigit[256];		/* decdigit[c] is nonzero if c is a dec digit */
char decvalue[256];		/* decvalue[c] is value of c as dec digit */
char hexdigit[256];		/* hexdigit[c] is nonzero if c is a hex digit */
char hexvalue[256];		/* hexvalue[c] is value of c as a hex digit */
char base64digit[256];		/* base64char[c] is nonzero if c is base64 digit */
char base64value[256];		/* base64value[c] is value of c as base64 digit */
char tokenchar[256];		/* tokenchar[c] is true if c can be in a token */
char alpha[256];		/* alpha[c] is true if c is alphabetic A-Z a-z */

/* initializeCharacterTables
 * initializes all of the above arrays
 */
void initializeCharacterTables(void)
{
    int i;
    for (i = 0; i < 256; i++)
	upper[i] = i;
    for (i = 'a'; i <= 'z'; i++)
	upper[i] = i - 'a' + 'A';
    for (i = 0; i <= 255; i++)
	alpha[i] = decdigit[i] = whitespace[i] = base64digit[i] = FALSE;
    whitespace[' '] = whitespace['\n'] = whitespace['\t'] = TRUE;
    whitespace['\v'] = whitespace['\r'] = whitespace['\f'] = TRUE;
    for (i = '0'; i <= '9'; i++) {
	base64digit[i] = hexdigit[i] = decdigit[i] = TRUE;
	decvalue[i] = hexvalue[i] = i - '0';
	base64value[i] = (i - '0') + 52;
    }
    for (i = 'a'; i <= 'f'; i++) {
	hexdigit[i] = hexdigit[upper[i]] = TRUE;
	hexvalue[i] = hexvalue[upper[i]] = i - 'a' + 10;
    }
    for (i = 'a'; i <= 'z'; i++) {
	base64digit[i] = base64digit[upper[i]] = TRUE;
	alpha[i] = alpha[upper[i]] = TRUE;
	base64value[i] = i - 'a' + 26;
	base64value[upper[i]] = i - 'a';
    }
    base64digit['+'] = base64digit['/'] = TRUE;
    base64value['+'] = 62;
    base64value['/'] = 63;
    base64value['='] = 0;
    for (i = 0; i < 255; i++)
	tokenchar[i] = FALSE;
    for (i = 'a'; i <= 'z'; i++)
	tokenchar[i] = tokenchar[upper[i]] = TRUE;
    for (i = '0'; i <= '9'; i++)
	tokenchar[i] = TRUE;
    tokenchar['-'] = TRUE;
    tokenchar['.'] = TRUE;
    tokenchar['/'] = TRUE;
    tokenchar['_'] = TRUE;
    tokenchar[':'] = TRUE;
    tokenchar['*'] = TRUE;
    tokenchar['+'] = TRUE;
    tokenchar['='] = TRUE;
}

/* isWhiteSpace(c)
 * Returns TRUE if c is a whitespace character (space, tab, etc. ).
 */
int isWhiteSpace(int c)
{
    return ((c >= 0 && c <= 255) && whitespace[c]);
}

/* isDecDigit(c)
 * Returns TRUE if c is a decimal digit.
 */
int isDecDigit(int c)
{
    return ((c >= 0 && c <= 255) && decdigit[c]);
}

/* isHexDigit(c)
 * Returns TRUE if c is a hexadecimal digit.
 */
int isHexDigit(int c)
{
    return ((c >= 0 && c <= 255) && hexdigit[c]);
}

/* isBase64Digit(c)
 * returns TRUE if c is a base64 digit A-Z,a-Z,0-9,+,/
 */
int isBase64Digit(int c)
{
    return ((c >= 0 && c <= 255) && base64digit[c]);
}

/* isTokenChar(c)
 * Returns TRUE if c is allowed in a token
 */
int isTokenChar(int c)
{
    return ((c >= 0 && c <= 255) && tokenchar[c]);
}

/* isAlpha(c)
 * Returns TRUE if c is alphabetic
 */
int isAlpha(int c)
{
    return ((c >= 0 && c <= 255) && alpha[c]);
}

/**********************/
/* SEXP INPUT STREAMS */
/**********************/

/* changeInputByteSize(is,newByteSize)
 */
void changeInputByteSize(sexpInputStream *is, int newByteSize)
{
    is->byteSize = newByteSize;
    is->nBits = 0;
    is->bits = 0;
}

/* getChar(is)
 * This is one possible character input routine for an input stream.
 * (This version uses the standard input stream.)
 * getChar places next 8-bit character into is->nextChar.
 * It also updates the count of number of 8-bit characters read.
 * The value EOF is obtained when no more input is available.  
 * This code handles 4-bit/6-bit/8-bit channels.
 */
void getChar(sexpInputStream *is)
{
    int c;
    if (is->nextChar == EOF) {
	is->byteSize = 8;
	return;
    }
    while (TRUE) {
	c = is->nextChar = fgetc(is->inputFile);
	if (c == EOF)
	    return;
	if ((is->byteSize == 6 && (c == '|' || c == '}'))
	    || (is->byteSize == 4 && (c == '#')))
	    /* end of region reached; return terminating character, after
	       checking for unused bits */
	{
	    if (is->nBits > 0 && (((1 << is->nBits) - 1) & is->bits) != 0)
		ErrorMessage(WARNING,
			     "%d-bit region ended with %d unused bits left-over",
			     is->byteSize, is->nBits);
	    changeInputByteSize(is, 8);
	    return;
	} else if (is->byteSize != 8 && isWhiteSpace(c))
	    ;	/* ignore white space in hex and base64 regions */
	else if (is->byteSize == 6 && c == '=')
	    ;	/* ignore equals signs in base64 regions */
	else if (is->byteSize == 8) {
	    is->count++;
	    return;
	} else if (is->byteSize < 8) {
	    is->bits = is->bits << is->byteSize;
	    is->nBits += is->byteSize;
	    if (is->byteSize == 6 && isBase64Digit(c))
		is->bits = is->bits | base64value[c];
	    else if (is->byteSize == 4 && isHexDigit(c))
		is->bits = is->bits | hexvalue[c];
	    else
		ErrorMessage(ERROR,
			     "character %c found in %d-bit coding region",
			     (int) is->nextChar, is->byteSize);
	    if (is->nBits >= 8) {
		is->nextChar = (is->bits >> (is->nBits - 8)) & 0xFF;
		is->nBits -= 8;
		is->count++;
		return;
	    }
	}
    }
}

/* newSexpInputStream()
 * Creates and initializes a new sexpInputStream object.
 * (Prefixes stream with one blank, and initializes stream
 *  so that it reads from standard input.)
 */
sexpInputStream *newSexpInputStream(void)
{
    sexpInputStream *is;
    is = (sexpInputStream *) xmalloc(sizeof(sexpInputStream));
    is->nextChar = ' ';
    is->getChar = getChar;
    is->count = -1;
    is->byteSize = 8;
    is->bits = 0;
    is->nBits = 0;
    is->inputFile = stdin;
    return is;
}

/*****************************************/
/* INPUT (SCANNING AND PARSING) ROUTINES */
/*****************************************/

/* skipWhiteSpace(is)
 * Skip over any white space on the given sexpInputStream.
 */
void skipWhiteSpace(sexpInputStream *is)
{
    while (isWhiteSpace(is->nextChar))
	is->getChar(is);
}

/* skipChar(is,c)
 * Skip the following input character on input stream is, if it is
 * equal to the character c.  If it is not equal, then an error occurs.
 */
void skipChar(sexpInputStream *is, int c)
{
    if (is->nextChar == c)
	is->getChar(is);
    else
	ErrorMessage(ERROR,
		     "character %x (hex) found where %c (char) expected",
		     (int) is->nextChar, (int) c);
}

/* scanToken(is,ss)
 * scan one or more characters into simple string ss as a token.
 */
void scanToken(sexpInputStream *is, sexpSimpleString *ss)
{
    skipWhiteSpace(is);
    while (isTokenChar(is->nextChar)) {
	appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
    }
    return;
}

/* scanToEOF(is)
 * scan one or more characters (until EOF reached)
 * return an object that is just that string
 */
sexpObject *scanToEOF(sexpInputStream *is)
{
    sexpSimpleString *ss = newSimpleString();
    sexpString *s = newSexpString();

    setSexpStringString(s, ss);
    skipWhiteSpace(is);
    while (is->nextChar != EOF) {
	appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
    }
    return (sexpObject *) s;
}

/* scanDecimal(is)
 * returns long integer that is value of decimal number
 */
unsigned long int scanDecimal(sexpInputStream *is)
{
    unsigned long int value = 0L;
    int i = 0;
    while (isDecDigit(is->nextChar)) {
	value = value * 10 + decvalue[is->nextChar];
	is->getChar(is);
	if (i++ > 8)
	    ErrorMessage(ERROR, "Decimal number %d... too long.",
			 (int) value, 0);
    }
    return value;
}

/* scanVerbatimString(is,ss,length)
 * Reads verbatim string of given length into simple string ss.
 */
void scanVerbatimString(sexpInputStream *is, sexpSimpleString *ss, long int length)
{
    long int i = 0L;
    skipWhiteSpace(is);
    skipChar(is, ':');
    if (length == -1L)		/* no length was specified */
	ErrorMessage(ERROR, "Verbatim string had no declared length.", 0,
		     0);
    for (i = 0; i < length; i++) {
	appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
    }
    return;
}

/* scanQuotedString(is,ss,length)
 * Reads quoted string of given length into simple string ss.
 * Handles ordinary C escapes. 
 * If of indefinite length, length is -1.
 */
void scanQuotedString(sexpInputStream *is, sexpSimpleString *ss, long int length)
{
    int c;

    skipChar(is, '"');
    while (length == -1 || simpleStringLength(ss) <= length) {
	if (is->nextChar == '\"') {
	    if (length == -1 || (simpleStringLength(ss) == length)) {
		skipChar(is, '\"');
		return;
	    } else
		ErrorMessage(ERROR,
			     "Quoted string ended too early. Declared length was %d",
			     (int) length, 0);
	} else if (is->nextChar == '\\') {	/* handle escape sequence */
	    is->getChar(is);
	    c = is->nextChar;
	    if (c == 'b')
		appendCharToSimpleString('\b', ss);
	    else if (c == 't')
		appendCharToSimpleString('\t', ss);
	    else if (c == 'v')
		appendCharToSimpleString('\v', ss);
	    else if (c == 'n')
		appendCharToSimpleString('\n', ss);
	    else if (c == 'f')
		appendCharToSimpleString('\f', ss);
	    else if (c == 'r')
		appendCharToSimpleString('\r', ss);
	    else if (c == '\"')
		appendCharToSimpleString('\"', ss);
	    else if (c == '\'')
		appendCharToSimpleString('\'', ss);
	    else if (c == '\\')
		appendCharToSimpleString('\\', ss);
	    else if (c >= '0' && c <= '7') {	/* octal number */
		int j, val;
		val = 0;
		for (j = 0; j < 3; j++) {
		    if (c >= '0' && c <= '7') {
			val = ((val << 3) | (c - '0'));
			if (j < 2) {
			    is->getChar(is);
			    c = is->nextChar;
			}
		    } else
			ErrorMessage(ERROR,
				     "Octal character \\%o... too short.",
				     val, 0);
		}
		if (val > 255)
		    ErrorMessage(ERROR, "Octal character \\%o... too big.",
				 val, 0);
		appendCharToSimpleString(val, ss);
	    } else if (c == 'x') {	/* hexadecimal number */
		int j, val;
		val = 0;
		is->getChar(is);
		c = is->nextChar;
		for (j = 0; j < 2; j++) {
		    if (isHexDigit(c)) {
			val = ((val << 4) | hexvalue[c]);
			if (j < 1) {
			    is->getChar(is);
			    c = is->nextChar;
			}
		    } else
			ErrorMessage(ERROR,
				     "Hex character \\x%x... too short.",
				     val, 0);
		}
		appendCharToSimpleString(val, ss);
	    } else if (c == '\n') {	/* ignore backslash line feed *//* also ignore following carriage-return if present */
		is->getChar(is);
		if (is->nextChar != '\r')
		    goto gotnextchar;
	    } else if (c == '\r') {	/* ignore backslash carriage-return *//* also ignore following linefeed if present */
		is->getChar(is);
		if (is->nextChar != '\n')
		    goto gotnextchar;
	    } else
		ErrorMessage(WARNING, "Escape character \\%c... unknown.",
			     c, 0);
	} /* end of handling escape sequence */
	else
	    appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
        gotnextchar:;	/* XXX label at end of compound statement */
    }				/* end of main while loop */
    return;
}

/* scanHexString(is,ss,length)
 * Reads hexadecimal string into simple string ss.
 * String is of given length result, or length = -1 if indefinite length.
 */
void scanHexString(sexpInputStream *is, sexpSimpleString *ss, long int length)
{
    changeInputByteSize(is, 4);
    skipChar(is, '#');
    while (is->nextChar != EOF
	   && (is->nextChar != '#' || is->byteSize == 4)) {
	appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
    }
    skipChar(is, '#');
    if (simpleStringLength(ss) != length && length >= 0)
	ErrorMessage(WARNING,
		     "Hex string has length %d different than declared length %d",
		     (int) simpleStringLength(ss), (int) length);
}

/* scanBase64String(is,ss,length)
 * Reads base64 string into simple string ss.
 * String is of given length result, or length = -1 if indefinite length.
 */
void scanBase64String(sexpInputStream *is, sexpSimpleString *ss, long int length)
{
    changeInputByteSize(is, 6);
    skipChar(is, '|');
    while (is->nextChar != EOF
	   && (is->nextChar != '|' || is->byteSize == 6)) {
	appendCharToSimpleString(is->nextChar, ss);
	is->getChar(is);
    }
    skipChar(is, '|');
    if (simpleStringLength(ss) != length && length >= 0)
	ErrorMessage(WARNING,
		     "Base64 string has length %d different than declared length %d",
		     (int) simpleStringLength(ss), (int) length);
}

/* scanSimpleString(is)
 * Reads and returns a simple string from the input stream.
 * Determines type of simple string from the initial character, and
 * dispatches to appropriate routine based on that. 
 */
sexpSimpleString *scanSimpleString(sexpInputStream *is)
{
    sexpSimpleString * ss = newSimpleString();
    long int length;

    skipWhiteSpace(is);
    /* Note that it is important in the following code to test for token-ness
     * before checking the other cases, so that a token may begin with ":",
     * which would otherwise be treated as a verbatim string missing a length.
     */
    if (isTokenChar(is->nextChar) && !isDecDigit(is->nextChar))
	scanToken(is, ss);
    else if (isDecDigit(is->nextChar)
	     || is->nextChar == '\"'
	     || is->nextChar == '#'
	     || is->nextChar == '|' || is->nextChar == ':') {
	if (isDecDigit(is->nextChar))
	    length = scanDecimal(is);
	else
	    length = -1L;
	if (is->nextChar == '\"')
	    scanQuotedString(is, ss, length);
	else if (is->nextChar == '#')
	    scanHexString(is, ss, length);
	else if (is->nextChar == '|')
	    scanBase64String(is, ss, length);
	else if (is->nextChar == ':')
	    scanVerbatimString(is, ss, length);
    } else
	ErrorMessage(ERROR,
		     "illegal character at position %d: %d (decimal)",
		     is->count, is->nextChar);
    if (simpleStringLength(ss) == 0)
	ErrorMessage(WARNING, "Simple string has zero length.", 0, 0);
    return ss;
}

/* scanString(is)
 * Reads and returns a string [presentationhint]string from input stream.
 */
sexpString *scanString(sexpInputStream *is)
{
    sexpSimpleString *ss;
    sexpString * s = newSexpString();

    if (is->nextChar == '[') {
	/* scan presentation hint */
	skipChar(is, '[');
	ss = scanSimpleString(is);
	setSexpStringPresentationHint(s, ss);
	skipWhiteSpace(is);
	skipChar(is, ']');
	skipWhiteSpace(is);
    }
    ss = scanSimpleString(is);
    setSexpStringString(s, ss);
    closeSexpString(s);
    return s;
}

/* scanList(is)
 * Read and return a sexpList from the input stream.
 */
sexpList *scanList(sexpInputStream *is)
{
    sexpList *list;
    sexpObject *object;
    skipChar(is, '(');
    skipWhiteSpace(is);
    list = newSexpList();
    if (is->nextChar == ')') {	/* ErrorMessage(ERROR,"List () with no contents is illegal.",0,0); */
	;			/* OK */
    } else {
	object = scanObject(is);
	sexpAddSexpListObject(list, object);
    }
    while (TRUE) {
	skipWhiteSpace(is);
	if (is->nextChar == ')') {	/* we just grabbed last element of list */
	    skipChar(is, ')');
	    closeSexpList(list);
	    break;
	} else {
	    object = scanObject(is);
	    sexpAddSexpListObject(list, object);
	}
    }
    return list;
}

/* scanObject(is)
 * Reads and returns a sexpObject from the given input stream.
 */
sexpObject *scanObject(sexpInputStream *is)
{
    sexpObject *object;
    skipWhiteSpace(is);
    if (is->nextChar == '{') {
	changeInputByteSize(is, 6);	/* order of this statement and next is */
	skipChar(is, '{');	/*   important! */
	object = scanObject(is);
	skipChar(is, '}');
    } else {
	if (is->nextChar == '(')
	    object = (sexpObject *) scanList(is);
	else
	    object = (sexpObject *) scanString(is);
    }
    return object;
}



/*==============================================================*/
/* --- sexp-output.c */
/* SEXP implementation code sexp-output.c
 * Ron Rivest
 * 5/5/1997
 */

static char *hexDigits = "0123456789ABCDEF";

static char *base64Digits =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/***********************/
/* SEXP Output Streams */
/***********************/

/* putChar(os,c)
 * Puts the character c out on the output stream os.
 * Keeps track of the "column" the next output char will go to.
 */
void putChar(sexpOutputStream *os, int c)
{
    putc(c, os->outputFile);
    os->column++;
}

/* varPutChar(os,c)
 * putChar with variable sized output bytes considered.
 */
void varPutChar(sexpOutputStream *os, int c)
                     
      				/* this is always an eight-bit byte being output */
{
    c &= 0xFF;
    os->bits = (os->bits << 8) | c;
    os->nBits += 8;
    while (os->nBits >= os->byteSize) {
	if ((os->byteSize == 6 || os->byteSize == 4
	     || c == '}' || c == '{' || c == '#' || c == '|')
	    && os->maxcolumn > 0 && os->column >= os->maxcolumn)
	    os->newLine(os, os->mode);
	if (os->byteSize == 4)
	    os->putChar(os,
			hexDigits[(os->bits >> (os->nBits - 4)) & 0x0F]);
	else if (os->byteSize == 6)
	    os->putChar(os,
			base64Digits[(os->
				      bits >> (os->nBits - 6)) & 0x3F]);
	else if (os->byteSize == 8)
	    os->putChar(os, os->bits & 0xFF);
	os->nBits -= os->byteSize;
	os->base64Count++;
    }
}

/* changeOutputByteSize(os,newByteSize,mode)
 * Change os->byteSize to newByteSize
 * record mode in output stream for automatic line breaks
 */
void changeOutputByteSize(sexpOutputStream *os, int newByteSize, int mode)
{
    if (newByteSize != 4 && newByteSize != 6 && newByteSize != 8)
	ErrorMessage(ERROR, "Illegal output base %d.", newByteSize, 0);
    if (newByteSize != 8 && os->byteSize != 8)
	ErrorMessage(ERROR,
		     "Illegal change of output byte size from %d to %d.",
		     os->byteSize, newByteSize);
    os->byteSize = newByteSize;
    os->nBits = 0;
    os->bits = 0;
    os->base64Count = 0;
    os->mode = mode;
}

/* flushOutput(os)
 * flush out any remaining bits 
 */
void flushOutput(sexpOutputStream *os)
{
    if (os->nBits > 0) {
	if (os->byteSize == 4)
	    os->putChar(os,
			hexDigits[(os->bits << (4 - os->nBits)) & 0x0F]);
	else if (os->byteSize == 6)
	    os->putChar(os,
			base64Digits[(os->
				      bits << (6 - os->nBits)) & 0x3F]);
	else if (os->byteSize == 8)
	    os->putChar(os, os->bits & 0xFF);
	os->nBits = 0;
	os->base64Count++;
    }
    if (os->byteSize == 6)	/* and add switch here */
	while ((os->base64Count & 3) != 0) {
	    if (os->maxcolumn > 0 && os->column >= os->maxcolumn)
		os->newLine(os, os->mode);
	    os->putChar(os, '=');
	    os->base64Count++;
	}
}

/* newLine(os,mode)
 * Outputs a newline symbol to the output stream os.
 * For ADVANCED mode, also outputs indentation as one blank per 
 * indentation level (but never indents more than half of maxcolumn).
 * Resets column for next output character.
 */
void newLine(sexpOutputStream *os, int mode)
{
    int i;
    if (mode == ADVANCED || mode == BASE64) {
	os->putChar(os, '\n');
	os->column = 0;
    }
    if (mode == ADVANCED)
	for (i = 0; i < os->indent && (4 * i) < os->maxcolumn; i++)
	    os->putChar(os, ' ');
}

/* newSexpOutputStream()
 * Creates and initializes new sexpOutputStream object.
 */
sexpOutputStream *newSexpOutputStream(void)
{
    sexpOutputStream *os;
    os = (sexpOutputStream *) xmalloc(sizeof(sexpOutputStream));
    os->column = 0;
    os->maxcolumn = DEFAULTLINELENGTH;
    os->indent = 0;
    os->putChar = putChar;
    os->newLine = newLine;
    os->byteSize = 8;
    os->bits = 0;
    os->nBits = 0;
    os->outputFile = stdout;
    os->mode = CANONICAL;
    return os;
}

/*******************/
/* OUTPUT ROUTINES */
/*******************/

/* printDecimal(os,n)
 * print out n in decimal to output stream os
 */
void printDecimal(sexpOutputStream *os, long int n)
{
    char buffer[50];
    int i;
    sprintf(buffer, "%ld", n);
    for (i = 0; buffer[i] != 0; i++)
	varPutChar(os, buffer[i]);
}

/********************/
/* CANONICAL OUTPUT */
/********************/

/* canonicalPrintVerbatimSimpleString(os,ss)
 * Print out simple string ss on output stream os as verbatim string.
 */
void canonicalPrintVerbatimSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    long int i;

    if (c == NULL)
	ErrorMessage(ERROR, "Can't print NULL string verbatim", 0, 0);
    /* print out len: */
    printDecimal(os, len);
    varPutChar(os, ':');
    /* print characters in fragment */
    for (i = 0; i < len; i++)
	varPutChar(os, (int) *c++);
}

/* canonicalPrintString(os,s)
 * Prints out sexp string s onto output stream os
 */
void canonicalPrintString(sexpOutputStream *os, sexpString *s)
{
    sexpSimpleString * ph = sexpStringPresentationHint(s);
    sexpSimpleString * ss = sexpStringString(s);

    if (ph != NULL) {
	varPutChar(os, '[');
	canonicalPrintVerbatimSimpleString(os, ph);
	varPutChar(os, ']');
    }
    if (ss == NULL)
	ErrorMessage(ERROR, "NULL string can't be printed.", 0, 0);
    canonicalPrintVerbatimSimpleString(os, ss);
}

/* canonicalPrintList(os,list)
 * Prints out the list "list" onto output stream os
 */
void canonicalPrintList(sexpOutputStream *os, sexpList *list)
{
    sexpIter *iter;
    sexpObject *object;
    varPutChar(os, '(');
    iter = sexpListIter(list);
    while (iter != NULL) {
	object = sexpIterObject(iter);
	if (object != NULL)
	    canonicalPrintObject(os, object);
	iter = sexpIterNext(iter);
    }
    varPutChar(os, ')');
}

/* canonicalPrintObject(os,object)
 * Prints out object on output stream os
 * Note that this uses the common "type" field of lists and strings.
 */
void canonicalPrintObject(sexpOutputStream *os, sexpObject *object)
{
    if (isObjectString(object))
	canonicalPrintString(os, (sexpString *) object);
    else if (isObjectList(object))
	canonicalPrintList(os, (sexpList *) object);
    else
	ErrorMessage(ERROR, "NULL object can't be printed.", 0, 0);
}

/* *************/
/* BASE64 MODE */
/* *************/
/* Same as canonical, except all characters get put out as base 64 ones */

void base64PrintWholeObject(sexpOutputStream *os, sexpObject *object)
{
    changeOutputByteSize(os, 8, BASE64);
    varPutChar(os, '{');
    changeOutputByteSize(os, 6, BASE64);
    canonicalPrintObject(os, object);
    flushOutput(os);
    changeOutputByteSize(os, 8, BASE64);
    varPutChar(os, '}');
}

/*****************/
/* ADVANCED MODE */
/*****************/

/* TOKEN */

/* canPrintAsToken(ss)
 * Returns TRUE if simple string ss can be printed as a token.
 * Doesn't begin with a digit, and all characters are tokenchars.
 */
int canPrintAsToken(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    int i;

    if (len <= 0)
	return FALSE;
    if (isDecDigit((int) *c))
	return FALSE;
    if (os->maxcolumn > 0 && os->column + len >= os->maxcolumn)
	return FALSE;
    for (i = 0; i < len; i++)
	if (!isTokenChar((int) (*c++)))
	    return FALSE;
    return TRUE;
}

/* advancedPrintTokenSimpleString(os,ss)
 * Prints out simple string ss as a token (assumes that this is OK).
 * May run over max-column, but there is no fragmentation allowed...
 */
void advancedPrintTokenSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    int i;

    if (os->maxcolumn > 0 && os->column > (os->maxcolumn - len))
	os->newLine(os, ADVANCED);
    for (i = 0; i < len; i++)
	os->putChar(os, (int) (*c++));
}

/* advancedLengthSimpleStringToken(ss)
 * Returns length for printing simple string ss as a token 
 */
int advancedLengthSimpleStringToken(sexpSimpleString *ss)
{
    return simpleStringLength(ss);
}


/* VERBATIM */

/* advancedPrintVerbatimSimpleString(os,ss)
 * Print out simple string ss on output stream os as verbatim string.
 * Again, can't fragment string, so max-column is just a suggestion...
 */
void advancedPrintVerbatimSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    long int i;

    if (c == NULL)
	ErrorMessage(ERROR, "Can't print NULL string verbatim", 0, 0);
    if (os->maxcolumn > 0 && os->column > (os->maxcolumn - len))
	os->newLine(os, ADVANCED);
    printDecimal(os, len);
    os->putChar(os, ':');
    for (i = 0; i < len; i++)
	os->putChar(os, (int) *c++);
}

/* advancedLengthSimpleStringVerbatim(ss)
 * Returns length for printing simple string ss in verbatim mode
 */
int advancedLengthSimpleStringVerbatim(sexpSimpleString *ss)
{
    long int len = simpleStringLength(ss);
    int i = 1;
    while (len > 9L) {
	i++;
	len = len / 10;
    }
    return (i + 1 + len);
}

/* BASE64 */

/* advancedPrintBase64SimpleString(os,ss)
 * Prints out simple string ss as a base64 value.
 */
void advancedPrintBase64SimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    long int i;

    if (c == NULL)
	ErrorMessage(ERROR, "Can't print NULL string base 64", 0, 0);
    varPutChar(os, '|');
    changeOutputByteSize(os, 6, ADVANCED);
    for (i = 0; i < len; i++)
	varPutChar(os, (int) (*c++));
    flushOutput(os);
    changeOutputByteSize(os, 8, ADVANCED);
    varPutChar(os, '|');
}

/* HEXADECIMAL */

/* advancedPrintHexSimpleString(os,ss)
 * Prints out simple string ss as a hexadecimal value.
 */
void advancedPrintHexSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    uint8_t * c = simpleStringString(ss);
    long int len = simpleStringLength(ss);
    long int i;

    if (c == NULL)
	ErrorMessage(ERROR, "Can't print NULL string hexadecimal", 0, 0);
    os->putChar(os, '#');
    changeOutputByteSize(os, 4, ADVANCED);
    for (i = 0; i < len; i++)
	varPutChar(os, (int) (*c++));
    flushOutput(os);
    changeOutputByteSize(os, 8, ADVANCED);
    os->putChar(os, '#');
}

/* advancedLengthSimpleStringHexadecimal(ss)
 * Returns length for printing simple string ss in hexadecimal mode
 */
int advancedLengthSimpleStringHexadecimal(sexpSimpleString *ss)
{
    long int len = simpleStringLength(ss);
    return (1 + 2 * len + 1);
}

/* QUOTED STRING */

/* canPrintAsQuotedString(ss)
 * Returns TRUE if simple string ss can be printed as a quoted string.
 * Must have only tokenchars and blanks.
 */
int canPrintAsQuotedString(sexpSimpleString *ss)
{
    long int i, len;
    uint8_t * c = simpleStringString(ss);
    len = simpleStringLength(ss);
    if (len < 0)
	return FALSE;
    for (i = 0; i < len; i++, c++)
	if (!isTokenChar((int) (*c)) && *c != ' ')
	    return FALSE;
    return TRUE;
}

/* advancedPrintQuotedStringSimpleString(os,ss)
 * Prints out simple string ss as a quoted string 
 * This code assumes that all characters are tokenchars and blanks,
 *  so no escape sequences need to be generated.
 * May run over max-column, but there is no fragmentation allowed...
 */
void advancedPrintQuotedStringSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    long int i;
    long int len = simpleStringLength(ss);
    uint8_t * c = simpleStringString(ss);
    os->putChar(os, '\"');
    for (i = 0; i < len; i++) {
	if (os->maxcolumn > 0 && os->column >= os->maxcolumn - 2) {
	    os->putChar(os, '\\');
	    os->putChar(os, '\n');
	    os->column = 0;
	}
	os->putChar(os, *c++);
    }
    os->putChar(os, '\"');
}

/* advancedLengthSimpleStringQuotedString(ss)
 * Returns length for printing simple string ss in quoted-string mode
 */
int advancedLengthSimpleStringQuotedString(sexpSimpleString *ss)
{
    long int len = simpleStringLength(ss);
    return (1 + len + 1);
}

/* SIMPLE STRING */

/* advancedPrintSimpleString(os,ss)
 * Prints out simple string ss onto output stream ss
 */
void advancedPrintSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    long int len = simpleStringLength(ss);
    if (canPrintAsToken(os, ss))
	advancedPrintTokenSimpleString(os, ss);
    else if (canPrintAsQuotedString(ss))
	advancedPrintQuotedStringSimpleString(os, ss);
    else if (len <= 4 && os->byteSize == 8)
	advancedPrintHexSimpleString(os, ss);
    else if (os->byteSize == 8)
	advancedPrintBase64SimpleString(os, ss);
    else
	ErrorMessage(ERROR,
		     "Can't print advanced mode with restricted output character set.",
		     0, 0);
}

/* advancedPrintString(os,s)
 * Prints out sexp string s onto output stream os
 */
void advancedPrintString(sexpOutputStream *os, sexpString *s)
{
    sexpSimpleString *ph = sexpStringPresentationHint(s);
    sexpSimpleString *ss = sexpStringString(s);
    if (ph != NULL) {
	os->putChar(os, '[');
	advancedPrintSimpleString(os, ph);
	os->putChar(os, ']');
    }
    if (ss == NULL)
	ErrorMessage(ERROR, "NULL string can't be printed.", 0, 0);
    advancedPrintSimpleString(os, ss);
}

/* advancedLengthSimpleStringBase64(ss)
 * Returns length for printing simple string ss as a base64 string
 */
int advancedLengthSimpleStringBase64(sexpSimpleString *ss)
{
    return (2 + 4 * ((simpleStringLength(ss) + 2) / 3));
}

/* advancedLengthSimpleString(os,ss)
 * Returns length of printed image of s
 */
int advancedLengthSimpleString(sexpOutputStream *os, sexpSimpleString *ss)
{
    long int len = simpleStringLength(ss);
    if (canPrintAsToken(os, ss))
	return advancedLengthSimpleStringToken(ss);
    else if (canPrintAsQuotedString(ss))
	return advancedLengthSimpleStringQuotedString(ss);
    else if (len <= 4 && os->byteSize == 8)
	return advancedLengthSimpleStringHexadecimal(ss);
    else if (os->byteSize == 8)
	return advancedLengthSimpleStringBase64(ss);
    else
	return 0;		/* an error condition */
}

/* advancedLengthString(os,s)
 * Returns length of printed image of string s
 */
int advancedLengthString(sexpOutputStream *os, sexpString *s)
{
    int len = 0;
    sexpSimpleString *ph = sexpStringPresentationHint(s);
    sexpSimpleString *ss = sexpStringString(s);
    if (ph != NULL)
	len += 2 + advancedLengthSimpleString(os, ph);
    if (ss != NULL)
	len += advancedLengthSimpleString(os, ss);
    return len;
}

/* advancedLengthList(os,list)
 * Returns length of printed image of list given as iterator
 */
int advancedLengthList(sexpOutputStream *os, sexpList *list)
{
    int len = 1;		/* for left paren */
    sexpIter *iter;
    sexpObject *object;
    iter = sexpListIter(list);
    while (iter != NULL) {
	object = sexpIterObject(iter);
	if (object != NULL) {
	    if (isObjectString(object))
		len += advancedLengthString(os, ((sexpString *) object));
	    /* else */
	    if (isObjectList(object))
		len += advancedLengthList(os, ((sexpList *) object));
	    len++;		/* for space after item */
	}
	iter = sexpIterNext(iter);
    }
    return len + 1;		/* for final paren */
}

/* advancedPrintList(os,list)
 * Prints out the list "list" onto output stream os.
 * Uses print-length to determine length of the image.  If it all fits
 * on the current line, then it is printed that way.  Otherwise, it is
 * written out in "vertical" mode, with items of the list starting in
 * the same column on successive lines.
 */
void advancedPrintList(sexpOutputStream *os, sexpList *list)
{
    int vertical = FALSE;
    int firstelement = TRUE;
    sexpIter *iter;
    sexpObject *object;
    os->putChar(os, '(');
    os->indent++;
    if (advancedLengthList(os, list) > os->maxcolumn - os->column)
	vertical = TRUE;
    iter = sexpListIter(list);
    while (iter != NULL) {
	object = sexpIterObject(iter);
	if (object != NULL) {
	    if (!firstelement) {
		if (vertical)
		    os->newLine(os, ADVANCED);
		else
		    os->putChar(os, ' ');
	    }
	    advancedPrintObject(os, object);
	}
	iter = sexpIterNext(iter);
	firstelement = FALSE;
    }
    if (os->maxcolumn > 0 && os->column > os->maxcolumn - 2)
	os->newLine(os, ADVANCED);
    os->indent--;
    os->putChar(os, ')');
}

/* advancedPrintObject(os,object)
 * Prints out object on output stream os 
 */
void advancedPrintObject(sexpOutputStream *os, sexpObject *object)
{
    if (os->maxcolumn > 0 && os->column > os->maxcolumn - 4)
	os->newLine(os, ADVANCED);
    if (isObjectString(object))
	advancedPrintString(os, (sexpString *) object);
    else if (isObjectList(object))
	advancedPrintList(os, (sexpList *) object);
    else
	ErrorMessage(ERROR, "NULL object can't be printed.", 0, 0);
}



/*==============================================================*/
/* --- sexp-main.c */
/* SEXP implementation code sexp-main.c
 * Ron Rivest
 * 6/29/1997
 */


char *help =
    "The program `sexp' reads, parses, and prints out S-expressions.\n"
    " INPUT:\n"
    "   Input is normally taken from stdin, but this can be changed:\n"
    "      -i filename      -- takes input from file instead.\n"
    "      -p               -- prompts user for console input\n"
    "   Input is normally parsed, but this can be changed:\n"
    "      -s               -- treat input up to EOF as a single string\n"
    " CONTROL LOOP:\n"
    "   The main routine typically reads one S-expression, prints it out again, \n"
    "   and stops.  This may be modified:\n"
    "      -x               -- execute main loop repeatedly until EOF\n"
    " OUTPUT:\n"
    "   Output is normally written to stdout, but this can be changed:\n"
    "      -o filename      -- write output to file instead\n"
    "   The output format is normally canonical, but this can be changed:\n"
    "      -a               -- write output in advanced transport format\n"
    "      -b               -- write output in base-64 output format\n"
    "      -c               -- write output in canonical format\n"
    "      -l               -- suppress linefeeds after output\n"
    "   More than one output format can be requested at once.\n"
    " There is normally a line-width of 75 on output, but:\n"
    "      -w width         -- changes line width to specified width.\n"
    "                          (0 implies no line-width constraint)\n"
    " The default switches are: -p -a -b -c -x\n"
    " Typical usage: cat certificate-file | sexp -a -x \n";

/*************************************************************************/
/* main(argc,argv)
 */
int main(int argc, char **argv)
{
    char *c;
    int swa = TRUE;
    int swb = TRUE;
    int swc = TRUE;
    int swp = TRUE;
    int sws = FALSE;
    int swx = TRUE;
    int swl = FALSE;
    int i;
    sexpObject *object;
    sexpInputStream *is;
    sexpOutputStream *os;
    initializeCharacterTables();
    is = newSexpInputStream();
    os = newSexpOutputStream();
    /* process switches */
    if (argc > 1)
	swa = swb = swc = swp = sws = swx = swl = FALSE;
    for (i = 1; i < argc; i++) {
	c = argv[i];
	if (*c != '-') {
	    printf("Unrecognized switch %s\n", c);
	    exit(0);
	}
	c++;
	if (*c == 'a')		/* advanced output */
	    swa = TRUE;
	else if (*c == 'b')	/* base-64 output */
	    swb = TRUE;
	else if (*c == 'c')	/* canonical output */
	    swc = TRUE;
	else if (*c == 'h') {	/* help */
	    printf("%s", help);
	    fflush(stdout);
	    exit(0);
	} else if (*c == 'i') {	/* input file */
	    if (i + 1 < argc)
		i++;
	    is->inputFile = fopen(argv[i], "r");
	    if (is->inputFile == NULL)
		ErrorMessage(ERROR, "Can't open input file.", 0, 0);
	} else if (*c == 'l')	/* suppress linefeeds after output */
	    swl = TRUE;
	else if (*c == 'o') {	/* output file */
	    if (i + 1 < argc)
		i++;
	    os->outputFile = fopen(argv[i], "w");
	    if (os->outputFile == NULL)
		ErrorMessage(ERROR, "Can't open output file.", 0, 0);
	} else if (*c == 'p')	/* prompt for input */
	    swp = TRUE;
	else if (*c == 's')	/* treat input as one big string */
	    sws = TRUE;
	else if (*c == 'w') {	/* set output width */
	    if (i + 1 < argc)
		i++;
	    os->maxcolumn = atoi(argv[i]);
	} else if (*c == 'x')	/* execute repeatedly */
	    swx = TRUE;
	else {
	    printf("Unrecognized switch: %s\n", argv[i]);
	    exit(0);
	}
    }
    if (swa == FALSE && swb == FALSE && swc == FALSE)
	swc = TRUE;		/* must have some output format! */

    /* main loop */
    if (swp == 0)
	is->getChar(is);
    else
	is->nextChar = -2;	/* this is not EOF */
    while (is->nextChar != EOF) {
	if (swp) {
	    printf("Input:\n");
	    fflush(stdout);
	}

	changeInputByteSize(is, 8);
	if (is->nextChar == -2)
	    is->getChar(is);

	skipWhiteSpace(is);
	if (is->nextChar == EOF)
	    break;

	if (sws == FALSE)
	    object = scanObject(is);
	else
	    object = scanToEOF(is);

	if (swc) {
	    if (swp) {
		printf("Canonical output:");
		fflush(stdout);
		os->newLine(os, ADVANCED);
	    }
	    canonicalPrintObject(os, object);
	    if (!swl) {
		printf("\n");
		fflush(stdout);
	    }
	}

	if (swb) {
	    if (swp) {
		printf("Base64 (of canonical) output:");
		fflush(stdout);
		os->newLine(os, ADVANCED);
	    }
	    base64PrintWholeObject(os, object);
	    if (!swl) {
		printf("\n");
		fflush(stdout);
	    }
	}

	if (swa) {
	    if (swp) {
		printf("Advanced transport output:");
		fflush(stdout);
		os->newLine(os, ADVANCED);
	    }
	    advancedPrintObject(os, object);
	    if (!swl) {
		printf("\n");
		fflush(stdout);
	    }
	}

	if (!swx)
	    break;
	if (!swp)
	    skipWhiteSpace(is);
	else if (!swl) {
	    printf("\n");
	    fflush(stdout);
	}
    }
    return 0;
}
