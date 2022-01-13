/* A Bison parser, made by GNU Bison 2.7.1.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 12 "parser.y"

#define YY_HEADER_EXPORT_START_CONDITIONS 1

#include "nls.h"
#include "kbd.h"

#include "contextP.h"
#include "ksyms.h"
#include "modifiers.h"

#include "parser.h"
#include "analyze.h"

/* Line 371 of yacc.c  */
#line 82 "parser.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
/* Line 387 of yacc.c  */
#line 26 "parser.y"

#include "keymap.h"

#ifndef STRDATA_STRUCT
#define STRDATA_STRUCT
#define MAX_PARSER_STRING 512
struct strdata {
	unsigned int len;
	unsigned char data[MAX_PARSER_STRING];
};
#endif


/* Line 387 of yacc.c  */
#line 128 "parser.c"

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EOL = 258,
     NUMBER = 259,
     LITERAL = 260,
     CHARSET = 261,
     KEYMAPS = 262,
     KEYCODE = 263,
     EQUALS = 264,
     PLAIN = 265,
     SHIFT = 266,
     CONTROL = 267,
     ALT = 268,
     ALTGR = 269,
     SHIFTL = 270,
     SHIFTR = 271,
     CTRLL = 272,
     CTRLR = 273,
     CAPSSHIFT = 274,
     COMMA = 275,
     DASH = 276,
     STRING = 277,
     STRLITERAL = 278,
     COMPOSE = 279,
     TO = 280,
     CCHAR = 281,
     ERROR = 282,
     PLUS = 283,
     UNUMBER = 284,
     ALT_IS_META = 285,
     STRINGS = 286,
     AS = 287,
     USUAL = 288,
     ON = 289,
     FOR = 290
   };
#endif
/* Tokens.  */
#define EOL 258
#define NUMBER 259
#define LITERAL 260
#define CHARSET 261
#define KEYMAPS 262
#define KEYCODE 263
#define EQUALS 264
#define PLAIN 265
#define SHIFT 266
#define CONTROL 267
#define ALT 268
#define ALTGR 269
#define SHIFTL 270
#define SHIFTR 271
#define CTRLL 272
#define CTRLR 273
#define CAPSSHIFT 274
#define COMMA 275
#define DASH 276
#define STRING 277
#define STRLITERAL 278
#define COMPOSE 279
#define TO 280
#define CCHAR 281
#define ERROR 282
#define PLUS 283
#define UNUMBER 284
#define ALT_IS_META 285
#define STRINGS 286
#define AS 287
#define USUAL 288
#define ON 289
#define FOR 290



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 58 "parser.y"

	long long int num;
	struct strdata str;


/* Line 387 of yacc.c  */
#line 219 "parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void *scanner, struct lk_ctx *ctx);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */
/* Line 390 of yacc.c  */
#line 71 "parser.y"

static int
yyerror(yyscan_t scanner __attribute__ ((unused)),
        struct lk_ctx *ctx, const char *s)
{
	ERR(ctx, "%s", s);
	return 0;
}

static int
strings_as_usual(struct lk_ctx *ctx)
{
	/*
	 * 26 strings, mostly inspired by the VT100 family
	 */
	char *stringvalues[30] = {
		/* F1 .. F20 */
		"\033[[A",  "\033[[B",  "\033[[C",  "\033[[D",  "\033[[E",
		"\033[17~", "\033[18~", "\033[19~", "\033[20~", "\033[21~",
		"\033[23~", "\033[24~", "\033[25~", "\033[26~",
		"\033[28~", "\033[29~",
		"\033[31~", "\033[32~", "\033[33~", "\033[34~",
		/* Find,    Insert,     Remove,     Select,     Prior */
		"\033[1~",  "\033[2~",  "\033[3~",  "\033[4~",  "\033[5~",
		/* Next,    Macro,      Help,       Do,         Pause */
		"\033[6~",  0,          0,          0,          0
	};
	int i;

	for (i = 0; i < 30; i++) {
		if (stringvalues[i]) {
			struct kbsentry ke;
			ke.kb_func = i;
			strncpy((char *)ke.kb_string, stringvalues[i],
				sizeof(ke.kb_string));
			ke.kb_string[sizeof(ke.kb_string) - 1] = 0;

			if (lk_add_func(ctx, &ke) == -1)
				return -1;
		}
	}
	return 0;
}

static int
compose_as_usual(struct lk_ctx *ctx, char *charset)
{
	if (charset && strcmp(charset, "iso-8859-1")) {
		ERR(ctx, _("loadkeys: don't know how to compose for %s"), charset);
		return -1;

	} else {
		struct ccc {
			unsigned char c1, c2, c3;
		} def_latin1_composes[68] = {
			{ '`', 'A', 0300 }, { '`', 'a', 0340 },
			{ '\'', 'A', 0301 }, { '\'', 'a', 0341 },
			{ '^', 'A', 0302 }, { '^', 'a', 0342 },
			{ '~', 'A', 0303 }, { '~', 'a', 0343 },
			{ '"', 'A', 0304 }, { '"', 'a', 0344 },
			{ 'O', 'A', 0305 }, { 'o', 'a', 0345 },
			{ '0', 'A', 0305 }, { '0', 'a', 0345 },
			{ 'A', 'A', 0305 }, { 'a', 'a', 0345 },
			{ 'A', 'E', 0306 }, { 'a', 'e', 0346 },
			{ ',', 'C', 0307 }, { ',', 'c', 0347 },
			{ '`', 'E', 0310 }, { '`', 'e', 0350 },
			{ '\'', 'E', 0311 }, { '\'', 'e', 0351 },
			{ '^', 'E', 0312 }, { '^', 'e', 0352 },
			{ '"', 'E', 0313 }, { '"', 'e', 0353 },
			{ '`', 'I', 0314 }, { '`', 'i', 0354 },
			{ '\'', 'I', 0315 }, { '\'', 'i', 0355 },
			{ '^', 'I', 0316 }, { '^', 'i', 0356 },
			{ '"', 'I', 0317 }, { '"', 'i', 0357 },
			{ '-', 'D', 0320 }, { '-', 'd', 0360 },
			{ '~', 'N', 0321 }, { '~', 'n', 0361 },
			{ '`', 'O', 0322 }, { '`', 'o', 0362 },
			{ '\'', 'O', 0323 }, { '\'', 'o', 0363 },
			{ '^', 'O', 0324 }, { '^', 'o', 0364 },
			{ '~', 'O', 0325 }, { '~', 'o', 0365 },
			{ '"', 'O', 0326 }, { '"', 'o', 0366 },
			{ '/', 'O', 0330 }, { '/', 'o', 0370 },
			{ '`', 'U', 0331 }, { '`', 'u', 0371 },
			{ '\'', 'U', 0332 }, { '\'', 'u', 0372 },
			{ '^', 'U', 0333 }, { '^', 'u', 0373 },
			{ '"', 'U', 0334 }, { '"', 'u', 0374 },
			{ '\'', 'Y', 0335 }, { '\'', 'y', 0375 },
			{ 'T', 'H', 0336 }, { 't', 'h', 0376 },
			{ 's', 's', 0337 }, { '"', 'y', 0377 },
			{ 's', 'z', 0337 }, { 'i', 'j', 0377 }
		};
		int i;
		for (i = 0; i < 68; i++) {
			struct lk_kbdiacr ptr;
			struct ccc c = def_latin1_composes[i];

			ptr.diacr  = c.c1;
			ptr.base   = c.c2;
			ptr.result = c.c3;

			if (lk_append_compose(ctx, &ptr) == -1)
				return -1;
		}
	}
	return 0;
}


/* Line 390 of yacc.c  */
#line 354 "parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   86

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  50
/* YYNRULES -- Number of states.  */
#define YYNSTATES  91

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    29,    32,    37,    44,    49,    53,
      57,    59,    63,    65,    71,    78,    85,    87,    89,    95,
     102,   109,   112,   114,   116,   118,   120,   122,   124,   126,
     128,   130,   132,   133,   136,   138,   140,   143,   145,   148,
     150
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      37,     0,    -1,    -1,    37,    38,    -1,     3,    -1,    39,
      -1,    40,    -1,    41,    -1,    42,    -1,    43,    -1,    49,
      -1,    46,    -1,    47,    -1,     6,    23,     3,    -1,    30,
       3,    -1,    31,    32,    33,     3,    -1,    24,    32,    33,
      35,    23,     3,    -1,    24,    32,    33,     3,    -1,     7,
      44,     3,    -1,    44,    20,    45,    -1,    45,    -1,     4,
      21,     4,    -1,     4,    -1,    22,     5,     9,    23,     3,
      -1,    24,    48,    48,    25,    48,     3,    -1,    24,    48,
      48,    25,    54,     3,    -1,    26,    -1,    29,    -1,     8,
       4,     9,    52,     3,    -1,    50,     8,     4,     9,    54,
       3,    -1,    10,     8,     4,     9,    54,     3,    -1,    50,
      51,    -1,    51,    -1,    11,    -1,    12,    -1,    13,    -1,
      14,    -1,    15,    -1,    16,    -1,    17,    -1,    18,    -1,
      19,    -1,    -1,    53,    52,    -1,    54,    -1,     4,    -1,
      28,     4,    -1,    29,    -1,    28,    29,    -1,     5,    -1,
      28,     5,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   180,   180,   181,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   193,   210,   215,   222,   227,   233,   238,
     239,   241,   249,   255,   275,   285,   296,   297,   299,   359,
     365,   372,   373,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   387,   388,   390,   396,   397,   398,   399,   400,
     401
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EOL", "NUMBER", "LITERAL", "CHARSET",
  "KEYMAPS", "KEYCODE", "EQUALS", "PLAIN", "SHIFT", "CONTROL", "ALT",
  "ALTGR", "SHIFTL", "SHIFTR", "CTRLL", "CTRLR", "CAPSSHIFT", "COMMA",
  "DASH", "STRING", "STRLITERAL", "COMPOSE", "TO", "CCHAR", "ERROR",
  "PLUS", "UNUMBER", "ALT_IS_META", "STRINGS", "AS", "USUAL", "ON", "FOR",
  "$accept", "keytable", "line", "charsetline", "altismetaline",
  "usualstringsline", "usualcomposeline", "keymapline", "range", "range0",
  "strline", "compline", "compsym", "singleline", "modifiers", "modifier",
  "rvalue0", "rvalue1", "rvalue", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    36,    37,    37,    38,    38,    38,    38,    38,    38,
      38,    38,    38,    39,    40,    41,    42,    42,    43,    44,
      44,    45,    45,    46,    47,    47,    48,    48,    49,    49,
      49,    50,    50,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    52,    52,    53,    54,    54,    54,    54,    54,
      54
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     2,     4,     6,     4,     3,     3,
       1,     3,     1,     5,     6,     6,     1,     1,     5,     6,
       6,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     2,     1,     1,     2,     1,     2,     1,
       2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     4,     0,     0,     0,     0,    33,    34,
      35,    36,    37,    38,    39,    40,    41,     0,     0,     0,
       0,     3,     5,     6,     7,     8,     9,    11,    12,    10,
       0,    32,     0,    22,     0,    20,     0,     0,     0,    26,
      27,     0,     0,    14,     0,     0,    31,    13,     0,    18,
       0,    42,     0,     0,     0,     0,     0,     0,    21,    19,
      45,    49,     0,    47,     0,    42,    44,     0,     0,    17,
       0,     0,    15,     0,    46,    50,    48,    28,    43,     0,
      23,     0,    27,     0,     0,     0,    30,    16,    24,    25,
      29
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    21,    22,    23,    24,    25,    26,    34,    35,
      27,    28,    42,    29,    30,    31,    64,    65,    66
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -42
static const yytype_int8 yypact[] =
{
     -42,     3,   -42,   -42,   -21,     4,     8,    -1,   -42,   -42,
     -42,   -42,   -42,   -42,   -42,   -42,   -42,    26,    12,    37,
      10,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,
      50,   -42,    47,    30,    34,   -42,    43,    49,    46,   -42,
     -42,    23,    20,   -42,    24,    55,   -42,   -42,    66,   -42,
       4,    19,    62,    51,    -3,    48,    69,    67,   -42,   -42,
     -42,   -42,    31,   -42,    72,    19,   -42,    19,    74,   -42,
      56,     0,   -42,    19,   -42,   -42,   -42,   -42,   -42,    75,
     -42,    77,   -42,    78,    79,    80,   -42,   -42,   -42,   -42,
     -42
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,    35,
     -42,   -42,   -41,   -42,   -42,    54,    21,   -42,   -28
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      69,    55,    32,     2,    60,    61,     3,    37,    33,     4,
       5,     6,    36,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    60,    61,    17,    39,    18,    62,    82,
      83,    38,    70,    19,    20,    74,    75,    49,    39,    79,
      43,    40,    44,    84,    41,    85,    39,    62,    63,    40,
      47,    48,    51,    52,    50,    53,    54,    56,    45,    57,
      76,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      58,    67,    72,    71,    68,    77,    73,    80,    86,    81,
      87,    88,    89,    90,    46,    59,    78
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-42)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_uint8 yycheck[] =
{
       3,    42,    23,     0,     4,     5,     3,     8,     4,     6,
       7,     8,     4,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,     4,     5,    22,    26,    24,    28,    29,
      71,     5,    35,    30,    31,     4,     5,     3,    26,    67,
       3,    29,    32,    71,    32,    73,    26,    28,    29,    29,
       3,    21,     9,     4,    20,     9,    33,    33,     8,     4,
      29,    11,    12,    13,    14,    15,    16,    17,    18,    19,
       4,     9,     3,    25,    23,     3,     9,     3,     3,    23,
       3,     3,     3,     3,    30,    50,    65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    37,     0,     3,     6,     7,     8,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    22,    24,    30,
      31,    38,    39,    40,    41,    42,    43,    46,    47,    49,
      50,    51,    23,     4,    44,    45,     4,     8,     5,    26,
      29,    32,    48,     3,    32,     8,    51,     3,    21,     3,
      20,     9,     4,     9,    33,    48,    33,     4,     4,    45,
       4,     5,    28,    29,    52,    53,    54,     9,    23,     3,
      35,    25,     3,     9,     4,     5,    29,     3,    52,    54,
       3,    23,    29,    48,    54,    54,     3,     3,     3,     3,
       3
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (scanner, ctx, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, scanner, ctx); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner, struct lk_ctx *ctx)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner, ctx)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *scanner;
    struct lk_ctx *ctx;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (scanner);
  YYUSE (ctx);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner, struct lk_ctx *ctx)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, scanner, ctx)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *scanner;
    struct lk_ctx *ctx;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner, ctx);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void *scanner, struct lk_ctx *ctx)
#else
static void
yy_reduce_print (yyvsp, yyrule, scanner, ctx)
    YYSTYPE *yyvsp;
    int yyrule;
    void *scanner;
    struct lk_ctx *ctx;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , scanner, ctx);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, scanner, ctx); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *scanner, struct lk_ctx *ctx)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, scanner, ctx)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void *scanner;
    struct lk_ctx *ctx;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (scanner);
  YYUSE (ctx);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YYUSE (yytype);
}




/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *scanner, struct lk_ctx *ctx)
#else
int
yyparse (scanner, ctx)
    void *scanner;
    struct lk_ctx *ctx;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;


#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
static YYSTYPE yyval_default;
# define YY_INITIAL_VALUE(Value) = Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 13:
/* Line 1787 of yacc.c  */
#line 194 "parser.y"
    {
				if (lk_set_charset(ctx, (char *) (yyvsp[(2) - (3)].str).data)) {
					ERR(ctx,
						_("unknown charset %s - ignoring charset request\n"),
						(char *) (yyvsp[(2) - (3)].str).data);
					YYERROR;
				}
				ctx->keywords |= LK_KEYWORD_CHARSET;

				/* Unicode: The first 256 code points were made
				   identical to the content of ISO 8859-1 */
				if (ctx->flags & LK_FLAG_PREFER_UNICODE &&
				    !strcasecmp((char *) (yyvsp[(2) - (3)].str).data, "iso-8859-1"))
					ctx->flags ^= LK_FLAG_PREFER_UNICODE;
			}
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 211 "parser.y"
    {
				ctx->keywords |= LK_KEYWORD_ALTISMETA;
			}
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 216 "parser.y"
    {
				if (strings_as_usual(ctx) == -1)
					YYERROR;
				ctx->keywords |= LK_KEYWORD_STRASUSUAL;
			}
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 223 "parser.y"
    {
				if (compose_as_usual(ctx, (char *) (yyvsp[(5) - (6)].str).data) == -1)
					YYERROR;
			}
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 228 "parser.y"
    {
				if (compose_as_usual(ctx, 0) == -1)
					YYERROR;
			}
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 234 "parser.y"
    {
				ctx->keywords |= LK_KEYWORD_KEYMAPS;
			}
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 242 "parser.y"
    {
				int i;
				for (i = (yyvsp[(1) - (3)].num); i <= (yyvsp[(3) - (3)].num); i++) {
					if (lk_add_map(ctx, i) == -1)
						YYERROR;
				}
			}
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 250 "parser.y"
    {
				if (lk_add_map(ctx, (yyvsp[(1) - (1)].num)) == -1)
					YYERROR;
			}
    break;

  case 23:
/* Line 1787 of yacc.c  */
#line 256 "parser.y"
    {
				struct kbsentry ke;

				if (KTYP((yyvsp[(2) - (5)].num)) != KT_FN) {
					ERR(ctx, _("'%s' is not a function key symbol"),
						get_sym(ctx, KTYP((yyvsp[(2) - (5)].num)), KVAL((yyvsp[(2) - (5)].num))));
					YYERROR;
				}

				ke.kb_func = KVAL((yyvsp[(2) - (5)].num));
				strncpy((char *) ke.kb_string,
				        (char *) (yyvsp[(4) - (5)].str).data,
				        sizeof(ke.kb_string));
				ke.kb_string[sizeof(ke.kb_string) - 1] = 0;

				if (lk_add_func(ctx, &ke) == -1)
					YYERROR;
			}
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 276 "parser.y"
    {
				struct lk_kbdiacr ptr;
				ptr.diacr  = (yyvsp[(2) - (6)].num);
				ptr.base   = (yyvsp[(3) - (6)].num);
				ptr.result = (yyvsp[(5) - (6)].num);

				if (lk_append_compose(ctx, &ptr) == -1)
					YYERROR;
			}
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 286 "parser.y"
    {
				struct lk_kbdiacr ptr;
				ptr.diacr  = (yyvsp[(2) - (6)].num);
				ptr.base   = (yyvsp[(3) - (6)].num);
				ptr.result = (yyvsp[(5) - (6)].num);

				if (lk_append_compose(ctx, &ptr) == -1)
					YYERROR;
			}
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 296 "parser.y"
    {	(yyval.num) = (yyvsp[(1) - (1)].num);		}
    break;

  case 27:
/* Line 1787 of yacc.c  */
#line 297 "parser.y"
    {	(yyval.num) = (yyvsp[(1) - (1)].num) ^ 0xf000;	}
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 300 "parser.y"
    {
				unsigned int j, i, keycode;
				int *val;

				if (ctx->key_line->count == 1) {
					char one = 1;
					/* Some files do not have a keymaps line, and
					 * we have to wait until all input has been read
					 * before we know which maps to fill. */
					lk_array_set(ctx->key_constant, (yyvsp[(2) - (5)].num), &one);

					/* On the other hand, we now have include files,
					 * and it should be possible to override lines
					 * from an include file. So, kill old defs. */
					for (j = 0; j < ctx->keymap->total; j++) {
						if (!lk_map_exists(ctx, j))
							continue;

						if (lk_del_key(ctx, j, (yyvsp[(2) - (5)].num)) < 0)
							YYERROR;
					}
				}

				if (ctx->keywords & LK_KEYWORD_KEYMAPS) {
					i = 0;

					for (j = 0; j < ctx->keymap->total; j++) {
						if (!lk_map_exists(ctx, j))
							continue;

						if (ctx->key_line->count != 1 || i == 0) {
							keycode = K_HOLE;

							if (i < ctx->key_line->count) {
								val = lk_array_get(ctx->key_line, i);
								keycode = *val;
							}

							if (lk_add_key(ctx, j, (yyvsp[(2) - (5)].num), keycode) < 0)
								YYERROR;
						}
						i++;
					}

					if (i < ctx->key_line->count) {
						ERR(ctx, _("too many (%d) entries on one line"),
							ctx->key_line->count);
						YYERROR;
					}
				} else {
					for (i = 0; i < ctx->key_line->count; i++) {
						val = lk_array_get(ctx->key_line, i);

						if (lk_add_key(ctx, i, (yyvsp[(2) - (5)].num), *val) < 0)
							YYERROR;
					}
				}
			}
    break;

  case 29:
/* Line 1787 of yacc.c  */
#line 360 "parser.y"
    {
				if (lk_add_key(ctx, ctx->mod, (yyvsp[(3) - (6)].num), (yyvsp[(5) - (6)].num)) < 0)
					YYERROR;
				ctx->mod = 0;
			}
    break;

  case 30:
/* Line 1787 of yacc.c  */
#line 366 "parser.y"
    {
				if (lk_add_key(ctx, 0, (yyvsp[(3) - (6)].num), (yyvsp[(5) - (6)].num)) < 0)
					YYERROR;
				ctx->mod = 0;
			}
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 375 "parser.y"
    { ctx->mod |= M_SHIFT;	}
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 376 "parser.y"
    { ctx->mod |= M_CTRL;	}
    break;

  case 35:
/* Line 1787 of yacc.c  */
#line 377 "parser.y"
    { ctx->mod |= M_ALT;		}
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 378 "parser.y"
    { ctx->mod |= M_ALTGR;	}
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 379 "parser.y"
    { ctx->mod |= M_SHIFTL;	}
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 380 "parser.y"
    { ctx->mod |= M_SHIFTR;	}
    break;

  case 39:
/* Line 1787 of yacc.c  */
#line 381 "parser.y"
    { ctx->mod |= M_CTRLL;	}
    break;

  case 40:
/* Line 1787 of yacc.c  */
#line 382 "parser.y"
    { ctx->mod |= M_CTRLR;	}
    break;

  case 41:
/* Line 1787 of yacc.c  */
#line 383 "parser.y"
    { ctx->mod |= M_CAPSSHIFT;	}
    break;

  case 44:
/* Line 1787 of yacc.c  */
#line 391 "parser.y"
    {
				int val = (yyvsp[(1) - (1)].num);
				lk_array_append(ctx->key_line, &val);
			}
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 396 "parser.y"
    { (yyval.num) = convert_code(ctx, (yyvsp[(1) - (1)].num), TO_AUTO);		}
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 397 "parser.y"
    { (yyval.num) = add_capslock(ctx, (yyvsp[(2) - (2)].num));			}
    break;

  case 47:
/* Line 1787 of yacc.c  */
#line 398 "parser.y"
    { (yyval.num) = convert_code(ctx, (yyvsp[(1) - (1)].num)^0xf000, TO_AUTO);	}
    break;

  case 48:
/* Line 1787 of yacc.c  */
#line 399 "parser.y"
    { (yyval.num) = add_capslock(ctx, (yyvsp[(2) - (2)].num)^0xf000);		}
    break;

  case 49:
/* Line 1787 of yacc.c  */
#line 400 "parser.y"
    { (yyval.num) = (yyvsp[(1) - (1)].num);					}
    break;

  case 50:
/* Line 1787 of yacc.c  */
#line 401 "parser.y"
    { (yyval.num) = add_capslock(ctx, (yyvsp[(2) - (2)].num));			}
    break;


/* Line 1787 of yacc.c  */
#line 1979 "parser.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (scanner, ctx, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (scanner, ctx, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, scanner, ctx);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, scanner, ctx);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, ctx, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner, ctx);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, scanner, ctx);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


/* Line 2050 of yacc.c  */
#line 403 "parser.y"


int
lk_parse_keymap(struct lk_ctx *ctx, lkfile_t *f)
{
	yyscan_t scanner;
	int rc = -1;

	ctx->mod = 0;

	yylex_init(&scanner);
	yylex_init_extra(ctx, &scanner);

	INFO(ctx, _("Loading %s"), f->pathname);

	if (stack_push(ctx, f, scanner) == -1)
		goto fail;

	if (yyparse(scanner, ctx))
		goto fail;

	rc = 0;

	stack_pop(ctx, scanner);

 fail:	yylex_destroy(scanner);
	return rc;
}
