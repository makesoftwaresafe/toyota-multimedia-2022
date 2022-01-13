/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 22 "ldgram.y" /* yacc.c:339  */

/*

 */

#define DONTDECLARE_MALLOC

#include "bfd.h"
#include "sysdep.h"
#include "bfdlink.h"
#include "ld.h"
#include "ldexp.h"
#include "ldver.h"
#include "ldlang.h"
#include "ldfile.h"
#include "ldemul.h"
#include "ldmisc.h"
#include "ldmain.h"
#include "mri.h"
#include "ldctor.h"
#include "ldlex.h"

#ifndef YYDEBUG
#define YYDEBUG 1
#endif

static enum section_type sectype;
static lang_memory_region_type *region;

FILE *saved_script_handle = NULL;
bfd_boolean force_make_executable = FALSE;

bfd_boolean ldgram_in_script = FALSE;
bfd_boolean ldgram_had_equals = FALSE;
bfd_boolean ldgram_had_keep = FALSE;
char *ldgram_vers_current_lang = NULL;

#define ERROR_NAME_MAX 20
static char *error_names[ERROR_NAME_MAX];
static int error_index;
#define PUSH_ERROR(x) if (error_index < ERROR_NAME_MAX) error_names[error_index] = x; error_index++;
#define POP_ERROR()   error_index--;

#line 110 "ldgram.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    NAME = 259,
    LNAME = 260,
    PLUSEQ = 261,
    MINUSEQ = 262,
    MULTEQ = 263,
    DIVEQ = 264,
    LSHIFTEQ = 265,
    RSHIFTEQ = 266,
    ANDEQ = 267,
    OREQ = 268,
    OROR = 269,
    ANDAND = 270,
    EQ = 271,
    NE = 272,
    LE = 273,
    GE = 274,
    LSHIFT = 275,
    RSHIFT = 276,
    UNARY = 277,
    END = 278,
    ALIGN_K = 279,
    BLOCK = 280,
    BIND = 281,
    QUAD = 282,
    SQUAD = 283,
    LONG = 284,
    SHORT = 285,
    BYTE = 286,
    SECTIONS = 287,
    PHDRS = 288,
    DATA_SEGMENT_ALIGN = 289,
    DATA_SEGMENT_RELRO_END = 290,
    DATA_SEGMENT_END = 291,
    SORT_BY_NAME = 292,
    SORT_BY_ALIGNMENT = 293,
    SIZEOF_HEADERS = 294,
    OUTPUT_FORMAT = 295,
    FORCE_COMMON_ALLOCATION = 296,
    OUTPUT_ARCH = 297,
    INHIBIT_COMMON_ALLOCATION = 298,
    SEGMENT_START = 299,
    INCLUDE = 300,
    MEMORY = 301,
    NOLOAD = 302,
    DSECT = 303,
    COPY = 304,
    INFO = 305,
    OVERLAY = 306,
    DEFINED = 307,
    TARGET_K = 308,
    SEARCH_DIR = 309,
    MAP = 310,
    ENTRY = 311,
    NEXT = 312,
    SIZEOF = 313,
    ADDR = 314,
    LOADADDR = 315,
    MAX_K = 316,
    MIN_K = 317,
    STARTUP = 318,
    HLL = 319,
    SYSLIB = 320,
    FLOAT = 321,
    NOFLOAT = 322,
    NOCROSSREFS = 323,
    ORIGIN = 324,
    FILL = 325,
    LENGTH = 326,
    CREATE_OBJECT_SYMBOLS = 327,
    INPUT = 328,
    GROUP = 329,
    OUTPUT = 330,
    CONSTRUCTORS = 331,
    ALIGNMOD = 332,
    AT = 333,
    SUBALIGN = 334,
    PROVIDE = 335,
    PROVIDE_HIDDEN = 336,
    AS_NEEDED = 337,
    CHIP = 338,
    LIST = 339,
    SECT = 340,
    ABSOLUTE = 341,
    LOAD = 342,
    NEWLINE = 343,
    ENDWORD = 344,
    ORDER = 345,
    NAMEWORD = 346,
    ASSERT_K = 347,
    FORMAT = 348,
    PUBLIC = 349,
    DEFSYMEND = 350,
    BASE = 351,
    ALIAS = 352,
    TRUNCATE = 353,
    REL = 354,
    INPUT_SCRIPT = 355,
    INPUT_MRI_SCRIPT = 356,
    INPUT_DEFSYM = 357,
    CASE = 358,
    EXTERN = 359,
    START = 360,
    VERS_TAG = 361,
    VERS_IDENTIFIER = 362,
    GLOBAL = 363,
    LOCAL = 364,
    VERSIONK = 365,
    INPUT_VERSION_SCRIPT = 366,
    KEEP = 367,
    ONLY_IF_RO = 368,
    ONLY_IF_RW = 369,
    SPECIAL = 370,
    EXCLUDE_FILE = 371,
    CONSTANT = 372,
    INPUT_DYNAMIC_LIST = 373
  };
#endif
/* Tokens.  */
#define INT 258
#define NAME 259
#define LNAME 260
#define PLUSEQ 261
#define MINUSEQ 262
#define MULTEQ 263
#define DIVEQ 264
#define LSHIFTEQ 265
#define RSHIFTEQ 266
#define ANDEQ 267
#define OREQ 268
#define OROR 269
#define ANDAND 270
#define EQ 271
#define NE 272
#define LE 273
#define GE 274
#define LSHIFT 275
#define RSHIFT 276
#define UNARY 277
#define END 278
#define ALIGN_K 279
#define BLOCK 280
#define BIND 281
#define QUAD 282
#define SQUAD 283
#define LONG 284
#define SHORT 285
#define BYTE 286
#define SECTIONS 287
#define PHDRS 288
#define DATA_SEGMENT_ALIGN 289
#define DATA_SEGMENT_RELRO_END 290
#define DATA_SEGMENT_END 291
#define SORT_BY_NAME 292
#define SORT_BY_ALIGNMENT 293
#define SIZEOF_HEADERS 294
#define OUTPUT_FORMAT 295
#define FORCE_COMMON_ALLOCATION 296
#define OUTPUT_ARCH 297
#define INHIBIT_COMMON_ALLOCATION 298
#define SEGMENT_START 299
#define INCLUDE 300
#define MEMORY 301
#define NOLOAD 302
#define DSECT 303
#define COPY 304
#define INFO 305
#define OVERLAY 306
#define DEFINED 307
#define TARGET_K 308
#define SEARCH_DIR 309
#define MAP 310
#define ENTRY 311
#define NEXT 312
#define SIZEOF 313
#define ADDR 314
#define LOADADDR 315
#define MAX_K 316
#define MIN_K 317
#define STARTUP 318
#define HLL 319
#define SYSLIB 320
#define FLOAT 321
#define NOFLOAT 322
#define NOCROSSREFS 323
#define ORIGIN 324
#define FILL 325
#define LENGTH 326
#define CREATE_OBJECT_SYMBOLS 327
#define INPUT 328
#define GROUP 329
#define OUTPUT 330
#define CONSTRUCTORS 331
#define ALIGNMOD 332
#define AT 333
#define SUBALIGN 334
#define PROVIDE 335
#define PROVIDE_HIDDEN 336
#define AS_NEEDED 337
#define CHIP 338
#define LIST 339
#define SECT 340
#define ABSOLUTE 341
#define LOAD 342
#define NEWLINE 343
#define ENDWORD 344
#define ORDER 345
#define NAMEWORD 346
#define ASSERT_K 347
#define FORMAT 348
#define PUBLIC 349
#define DEFSYMEND 350
#define BASE 351
#define ALIAS 352
#define TRUNCATE 353
#define REL 354
#define INPUT_SCRIPT 355
#define INPUT_MRI_SCRIPT 356
#define INPUT_DEFSYM 357
#define CASE 358
#define EXTERN 359
#define START 360
#define VERS_TAG 361
#define VERS_IDENTIFIER 362
#define GLOBAL 363
#define LOCAL 364
#define VERSIONK 365
#define INPUT_VERSION_SCRIPT 366
#define KEEP 367
#define ONLY_IF_RO 368
#define ONLY_IF_RW 369
#define SPECIAL 370
#define EXCLUDE_FILE 371
#define CONSTANT 372
#define INPUT_DYNAMIC_LIST 373

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 65 "ldgram.y" /* yacc.c:355  */

  bfd_vma integer;
  struct big_int
    {
      bfd_vma integer;
      char *str;
    } bigint;
  fill_type *fill;
  char *name;
  const char *cname;
  struct wildcard_spec wildcard;
  struct wildcard_list *wildcard_list;
  struct name_list *name_list;
  int token;
  union etree_union *etree;
  struct phdr_info
    {
      bfd_boolean filehdr;
      bfd_boolean phdrs;
      union etree_union *at;
      union etree_union *flags;
    } phdr;
  struct lang_nocrossref *nocrossref;
  struct lang_output_section_phdr_list *section_phdr;
  struct bfd_elf_version_deps *deflist;
  struct bfd_elf_version_expr *versyms;
  struct bfd_elf_version_tree *versnode;

#line 415 "ldgram.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 432 "ldgram.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
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
# elif ! defined YYSIZE_T
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

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
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
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

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
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1777

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  142
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  119
/* YYNRULES -- Number of rules.  */
#define YYNRULES  341
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  723

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   373

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   140,     2,     2,     2,    34,    21,     2,
      37,   137,    32,    30,   135,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,   136,
      24,    10,    25,    15,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   138,     2,   139,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    53,    19,    54,   141,     2,     2,     2,
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
       5,     6,     7,     8,     9,    11,    12,    13,    14,    17,
      18,    22,    23,    26,    27,    28,    29,    35,    36,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   163,   163,   164,   165,   166,   167,   171,   175,   175,
     185,   185,   198,   199,   203,   204,   205,   208,   211,   212,
     213,   215,   217,   219,   221,   223,   225,   227,   229,   231,
     233,   235,   236,   237,   239,   241,   243,   245,   247,   248,
     250,   249,   253,   255,   259,   260,   261,   265,   267,   271,
     273,   278,   279,   280,   284,   286,   288,   293,   293,   304,
     305,   311,   312,   313,   314,   315,   316,   317,   318,   319,
     320,   321,   323,   325,   327,   330,   332,   334,   336,   338,
     340,   339,   343,   346,   345,   349,   353,   357,   360,   363,
     366,   369,   372,   376,   375,   380,   379,   384,   383,   390,
     394,   395,   396,   400,   402,   403,   403,   411,   415,   419,
     426,   432,   438,   444,   450,   456,   462,   468,   474,   483,
     492,   503,   512,   523,   531,   535,   542,   544,   543,   550,
     551,   555,   556,   561,   566,   567,   572,   579,   580,   583,
     585,   589,   591,   593,   595,   597,   602,   609,   611,   615,
     617,   619,   621,   623,   625,   627,   629,   634,   634,   639,
     643,   651,   655,   663,   663,   667,   671,   672,   673,   678,
     677,   685,   693,   701,   702,   706,   707,   711,   713,   718,
     723,   724,   729,   731,   737,   739,   741,   745,   747,   753,
     756,   765,   776,   776,   782,   784,   786,   788,   790,   792,
     795,   797,   799,   801,   803,   805,   807,   809,   811,   813,
     815,   817,   819,   821,   823,   825,   827,   829,   831,   833,
     835,   837,   840,   842,   844,   846,   848,   850,   852,   854,
     856,   858,   860,   869,   871,   873,   875,   877,   879,   881,
     887,   888,   892,   893,   897,   898,   902,   903,   907,   908,
     909,   910,   913,   917,   920,   926,   928,   913,   935,   937,
     939,   944,   946,   934,   956,   958,   956,   966,   967,   968,
     969,   970,   974,   975,   976,   980,   981,   986,   987,   992,
     993,   998,   999,  1004,  1006,  1011,  1014,  1027,  1031,  1036,
    1038,  1029,  1046,  1049,  1051,  1055,  1056,  1055,  1065,  1110,
    1113,  1125,  1134,  1137,  1144,  1144,  1156,  1157,  1161,  1165,
    1174,  1174,  1188,  1188,  1198,  1199,  1203,  1207,  1211,  1218,
    1222,  1230,  1233,  1237,  1241,  1245,  1252,  1256,  1260,  1264,
    1269,  1268,  1282,  1281,  1291,  1295,  1299,  1303,  1307,  1311,
    1317,  1319
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NAME", "LNAME", "PLUSEQ",
  "MINUSEQ", "MULTEQ", "DIVEQ", "'='", "LSHIFTEQ", "RSHIFTEQ", "ANDEQ",
  "OREQ", "'?'", "':'", "OROR", "ANDAND", "'|'", "'^'", "'&'", "EQ", "NE",
  "'<'", "'>'", "LE", "GE", "LSHIFT", "RSHIFT", "'+'", "'-'", "'*'", "'/'",
  "'%'", "UNARY", "END", "'('", "ALIGN_K", "BLOCK", "BIND", "QUAD",
  "SQUAD", "LONG", "SHORT", "BYTE", "SECTIONS", "PHDRS",
  "DATA_SEGMENT_ALIGN", "DATA_SEGMENT_RELRO_END", "DATA_SEGMENT_END",
  "SORT_BY_NAME", "SORT_BY_ALIGNMENT", "'{'", "'}'", "SIZEOF_HEADERS",
  "OUTPUT_FORMAT", "FORCE_COMMON_ALLOCATION", "OUTPUT_ARCH",
  "INHIBIT_COMMON_ALLOCATION", "SEGMENT_START", "INCLUDE", "MEMORY",
  "NOLOAD", "DSECT", "COPY", "INFO", "OVERLAY", "DEFINED", "TARGET_K",
  "SEARCH_DIR", "MAP", "ENTRY", "NEXT", "SIZEOF", "ADDR", "LOADADDR",
  "MAX_K", "MIN_K", "STARTUP", "HLL", "SYSLIB", "FLOAT", "NOFLOAT",
  "NOCROSSREFS", "ORIGIN", "FILL", "LENGTH", "CREATE_OBJECT_SYMBOLS",
  "INPUT", "GROUP", "OUTPUT", "CONSTRUCTORS", "ALIGNMOD", "AT", "SUBALIGN",
  "PROVIDE", "PROVIDE_HIDDEN", "AS_NEEDED", "CHIP", "LIST", "SECT",
  "ABSOLUTE", "LOAD", "NEWLINE", "ENDWORD", "ORDER", "NAMEWORD",
  "ASSERT_K", "FORMAT", "PUBLIC", "DEFSYMEND", "BASE", "ALIAS", "TRUNCATE",
  "REL", "INPUT_SCRIPT", "INPUT_MRI_SCRIPT", "INPUT_DEFSYM", "CASE",
  "EXTERN", "START", "VERS_TAG", "VERS_IDENTIFIER", "GLOBAL", "LOCAL",
  "VERSIONK", "INPUT_VERSION_SCRIPT", "KEEP", "ONLY_IF_RO", "ONLY_IF_RW",
  "SPECIAL", "EXCLUDE_FILE", "CONSTANT", "INPUT_DYNAMIC_LIST", "','",
  "';'", "')'", "'['", "']'", "'!'", "'~'", "$accept", "file", "filename",
  "defsym_expr", "$@1", "mri_script_file", "$@2", "mri_script_lines",
  "mri_script_command", "$@3", "ordernamelist", "mri_load_name_list",
  "mri_abs_name_list", "casesymlist", "extern_name_list", "script_file",
  "$@4", "ifile_list", "ifile_p1", "$@5", "$@6", "input_list", "@7", "@8",
  "@9", "sections", "sec_or_group_p1", "statement_anywhere", "$@10",
  "wildcard_name", "wildcard_spec", "exclude_name_list", "file_NAME_list",
  "input_section_spec_no_keep", "input_section_spec", "$@11", "statement",
  "statement_list", "statement_list_opt", "length", "fill_exp", "fill_opt",
  "assign_op", "end", "assignment", "opt_comma", "memory",
  "memory_spec_list", "memory_spec", "$@12", "origin_spec", "length_spec",
  "attributes_opt", "attributes_list", "attributes_string", "startup",
  "high_level_library", "high_level_library_NAME_list",
  "low_level_library", "low_level_library_NAME_list",
  "floating_point_support", "nocrossref_list", "mustbe_exp", "$@13", "exp",
  "memspec_at_opt", "opt_at", "opt_align", "opt_subalign",
  "sect_constraint", "section", "$@14", "$@15", "$@16", "$@17", "$@18",
  "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "type", "atype",
  "opt_exp_with_type", "opt_exp_without_type", "opt_nocrossrefs",
  "memspec_opt", "phdr_opt", "overlay_section", "$@26", "$@27", "$@28",
  "phdrs", "phdr_list", "phdr", "$@29", "$@30", "phdr_type",
  "phdr_qualifiers", "phdr_val", "dynamic_list_file", "$@31",
  "dynamic_list_nodes", "dynamic_list_node", "dynamic_list_tag",
  "version_script_file", "$@32", "version", "$@33", "vers_nodes",
  "vers_node", "verdep", "vers_tag", "vers_defns", "@34", "@35",
  "opt_semicolon", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
      61,   265,   266,   267,   268,    63,    58,   269,   270,   124,
      94,    38,   271,   272,    60,    62,   273,   274,   275,   276,
      43,    45,    42,    47,    37,   277,   278,    40,   279,   280,
     281,   282,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   292,   293,   123,   125,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,    44,    59,    41,    91,    93,
      33,   126
};
# endif

#define YYPACT_NINF -625

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-625)))

#define YYTABLE_NINF -313

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     184,  -625,  -625,  -625,  -625,  -625,    54,  -625,  -625,  -625,
    -625,  -625,    75,  -625,   -16,  -625,    68,  -625,   835,   629,
      86,    11,    76,   -16,  -625,   104,    68,  -625,   451,    87,
      89,   111,  -625,   114,  -625,   143,   132,   120,   126,   152,
     177,   181,   189,   200,  -625,  -625,   201,   205,  -625,   213,
     221,   223,  -625,   228,  -625,  -625,  -625,  -625,   -53,  -625,
    -625,  -625,  -625,  -625,  -625,  -625,    64,  -625,   265,   143,
     272,   714,  -625,   276,   285,   293,  -625,  -625,   308,   309,
     311,   714,   315,   306,   317,   318,   320,   224,   714,  -625,
     321,  -625,   313,   314,   277,   204,    11,  -625,  -625,  -625,
     291,   212,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,
    -625,  -625,  -625,  -625,  -625,   345,   348,  -625,  -625,   350,
     357,   143,   143,   358,   143,     6,  -625,   360,    29,   328,
     143,   363,   367,   338,   318,  -625,  -625,  -625,   323,     2,
    -625,     3,  -625,  -625,   714,   714,   714,   340,   346,   347,
     349,   351,  -625,   353,   362,   364,   365,   372,   373,   375,
     377,   378,   385,   386,   387,   388,   714,   714,  1389,   343,
    -625,   259,  -625,   261,    24,  -625,  -625,   486,  1710,   262,
    -625,  -625,   292,  -625,    31,  -625,  -625,  1710,   342,   104,
     104,   264,   110,   379,   296,   110,  -625,   714,  -625,   260,
      32,   137,   298,  -625,  -625,  -625,   299,   301,   302,   303,
     304,  -625,  -625,   140,   153,    47,   307,  -625,  -625,   392,
      21,    29,   310,   432,   433,   714,    15,   -16,   714,   714,
    -625,   714,   714,  -625,  -625,  1026,   714,   714,   714,   714,
     714,   442,   444,   714,   446,   448,   449,   714,   714,   450,
     462,   714,   714,   464,  -625,  -625,   714,   714,   714,   714,
     714,   714,   714,   714,   714,   714,   714,   714,   714,   714,
     714,   714,   714,   714,   714,   714,   714,   714,  1710,   467,
     468,  -625,   469,   714,   714,  1710,   112,   470,  -625,   475,
    -625,   355,   356,  -625,  -625,   478,  -625,  -625,  -625,   -65,
    -625,  1710,   451,  -625,  -625,  -625,  -625,  -625,  -625,  -625,
    -625,   481,  -625,  -625,   906,   456,    19,  -625,  -625,  -625,
    -625,  -625,  -625,  -625,   143,  -625,   143,   360,  -625,  -625,
    -625,  -625,  -625,   457,    41,  -625,    25,  -625,  -625,  -625,
    1409,  -625,   -13,  1710,  1710,  1527,  1710,  1710,  -625,   844,
    1046,  1429,  1449,  1066,   352,   361,  1086,   370,   371,   381,
    1469,  1489,   382,   383,  1106,  1528,   384,  1670,  1727,  1743,
     803,  1006,  1627,   781,   781,   305,   305,   305,   305,   374,
     374,    66,    66,  -625,  -625,  -625,  1710,  1710,  1710,  -625,
    -625,  -625,  1710,  1710,  -625,  -625,  -625,  -625,   104,   116,
     110,   447,  -625,  -625,   -58,   562,   638,   562,   714,   366,
    -625,     5,   479,  -625,   350,  -625,  -625,  -625,  -625,    29,
    -625,  -625,  -625,   460,  -625,   389,   390,   482,  -625,  -625,
     714,  -625,  -625,   714,   714,  -625,   714,  -625,  -625,  -625,
    -625,  -625,   714,   714,  -625,  -625,  -625,   495,  -625,   714,
     368,   494,  -625,  -625,  -625,   218,   465,  1647,   496,   417,
    -625,  1690,   430,  -625,  1710,    18,   511,  -625,   518,     4,
    -625,   443,  -625,    34,    29,  -625,  -625,  -625,   394,  1126,
    1147,  1167,  1187,  1207,  1227,   395,  1710,   110,   483,   104,
     104,  -625,  -625,  -625,  -625,  -625,  -625,   396,   714,   180,
     513,  -625,   501,   502,  -625,  -625,   417,   489,   506,   507,
    -625,   402,  -625,  -625,  -625,   535,   412,  -625,    80,    29,
    -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,   413,
     368,  -625,  1247,  -625,   714,   514,   453,   453,  -625,   714,
      18,   714,   414,  -625,  -625,   466,  -625,    88,   110,   498,
      85,  1268,   714,   519,  -625,  -625,   359,  1288,  -625,  1308,
    -625,  -625,   545,  -625,  -625,  -625,   520,   542,  -625,  1328,
     714,   123,   515,  -625,  -625,    18,  -625,   714,  -625,  -625,
    1348,  -625,  -625,  -625,   516,  -625,  -625,  -625,  1368,  -625,
    -625,  -625,   530,   752,    43,   554,   568,  -625,  -625,  -625,
    -625,  -625,  -625,  -625,   546,   547,   548,  -625,  -625,   549,
     550,  -625,    59,  -625,   552,  -625,  -625,  -625,   752,   536,
     558,   -53,  -625,  -625,  -625,    38,   247,  -625,  -625,    62,
    -625,   559,  -625,   -83,    59,  -625,  -625,  -625,  -625,   538,
     572,   561,   566,   435,   567,   471,   569,   570,   472,   476,
    -625,    17,  -625,    16,   255,  -625,    59,   173,   572,   477,
     752,   611,   522,    62,    62,  -625,    62,  -625,    62,    62,
    -625,  -625,   487,   488,    62,  -625,  -625,  -625,   522,  -625,
     564,  -625,   595,  -625,   491,   492,    40,   497,   508,  -625,
    -625,  -625,  -625,   619,    91,   509,   521,    62,   523,   524,
      91,  -625,  -625,  -625,   627,  -625,  -625,  -625,   525,  -625,
    -625,  -625,    91,  -625,  -625,   412,  -625,   412,  -625,  -625,
    -625,   412,  -625
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    57,    10,     8,   310,   304,     0,     2,    60,     3,
      13,     6,     0,     4,     0,     5,     0,     1,    58,    11,
       0,   321,     0,   311,   314,     0,   305,   306,     0,     0,
       0,     0,    77,     0,    78,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   187,   188,     0,     0,    80,     0,
       0,     0,   105,     0,    70,    59,    62,    68,     0,    61,
      64,    65,    66,    67,    63,    69,     0,    16,     0,     0,
       0,     0,    17,     0,     0,     0,    19,    46,     0,     0,
       0,     0,     0,     0,    51,     0,     0,     0,     0,   327,
     338,   326,   334,   336,     0,     0,   321,   315,   334,   336,
       0,     0,   307,   149,   150,   151,   152,   192,   153,   154,
     155,   156,   192,   102,   293,     0,     0,     7,    83,     0,
       0,     0,     0,     0,     0,     0,   186,   189,     0,     0,
       0,     0,     0,     0,     0,   158,   157,   104,     0,     0,
      40,     0,   220,   234,     0,     0,     0,     0,     0,     0,
       0,     0,   221,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    14,     0,
      49,    31,    47,    32,    18,    33,    23,     0,    36,     0,
      37,    52,    38,    54,    39,    42,    12,     9,     0,     0,
       0,     0,   322,     0,     0,   309,   159,     0,   160,     0,
       0,     0,     0,    60,   169,   168,     0,     0,     0,     0,
       0,   181,   183,   164,   164,   189,     0,    87,    90,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      13,     0,     0,   198,   194,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   197,   199,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    25,     0,
       0,    45,     0,     0,     0,    22,     0,     0,    55,     0,
     332,     0,     0,   316,   329,   339,   328,   335,   337,     0,
     308,   193,   252,    99,   258,   264,   101,   100,   295,   292,
     294,     0,    74,    76,   312,   173,     0,    71,    72,    82,
     103,   179,   163,   180,     0,   184,     0,   189,   190,    85,
      93,    89,    92,     0,     0,    79,     0,    73,   192,   192,
       0,    86,     0,    27,    28,    43,    29,    30,   195,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   218,   217,
     215,   214,   213,   207,   208,   211,   212,   209,   210,   205,
     206,   203,   204,   200,   201,   202,    15,    26,    24,    50,
      48,    44,    20,    21,    35,    34,    53,    56,     0,   323,
     324,     0,   319,   317,     0,   273,     0,   273,     0,     0,
      84,     0,     0,   165,     0,   166,   182,   185,   191,     0,
      97,    88,    91,     0,    81,     0,     0,     0,   313,    41,
       0,   227,   233,     0,     0,   231,     0,   219,   196,   222,
     223,   224,     0,     0,   238,   239,   226,     0,   225,     0,
     340,   337,   330,   320,   318,     0,     0,   273,     0,   243,
     280,     0,   281,   265,   298,   299,     0,   177,     0,     0,
     175,     0,   167,     0,     0,    95,   161,   162,     0,     0,
       0,     0,     0,     0,     0,     0,   216,   341,     0,     0,
       0,   267,   268,   269,   270,   271,   274,     0,     0,     0,
       0,   276,     0,   245,   279,   282,   243,     0,   302,     0,
     296,     0,   178,   174,   176,     0,   164,    94,     0,     0,
     106,   228,   229,   230,   232,   235,   236,   237,   333,     0,
     340,   272,     0,   275,     0,     0,   247,   247,   102,     0,
     299,     0,     0,    75,   192,     0,    98,     0,   325,     0,
     273,     0,     0,     0,   253,   259,     0,     0,   300,     0,
     297,   171,     0,   170,    96,   331,     0,     0,   242,     0,
       0,   251,     0,   266,   303,   299,   192,     0,   277,   244,
       0,   248,   249,   250,     0,   260,   301,   172,     0,   246,
     254,   287,   273,   139,     0,     0,   123,   109,   108,   141,
     142,   143,   144,   145,     0,     0,     0,   130,   132,     0,
       0,   131,     0,   110,     0,   126,   134,   138,   140,     0,
       0,     0,   288,   261,   278,     0,     0,   192,   127,     0,
     107,     0,   122,   164,     0,   137,   255,   192,   129,     0,
     284,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     146,     0,   120,     0,     0,   124,     0,   164,   284,     0,
     139,     0,   241,     0,     0,   133,     0,   112,     0,     0,
     113,   136,   107,     0,     0,   119,   121,   125,   241,   135,
       0,   283,     0,   285,     0,     0,     0,     0,     0,   128,
     111,   285,   289,     0,   148,     0,     0,     0,     0,     0,
     148,   285,   240,   192,     0,   262,   115,   114,     0,   116,
     117,   256,   148,   147,   286,   164,   118,   164,   290,   263,
     257,   164,   291
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -625,  -625,   -63,  -625,  -625,  -625,  -625,   418,  -625,  -625,
    -625,  -625,  -625,  -625,   510,  -625,  -625,   429,  -625,  -625,
    -625,  -203,  -625,  -625,  -625,  -625,   105,  -196,  -625,   913,
    -569,   -15,    22,    -1,  -625,  -625,    35,  -625,    -8,  -625,
     -48,  -624,  -625,    36,  -543,  -212,  -625,  -625,  -289,  -625,
    -625,  -625,  -625,  -625,   190,  -625,  -625,  -625,  -625,  -625,
    -625,  -201,  -107,  -625,   -64,   -12,   157,  -625,   128,  -625,
    -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,
    -625,  -625,  -625,  -625,  -446,   266,  -625,  -625,    13,  -573,
    -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,  -625,
    -480,  -625,  -625,  -625,  -625,   646,  -625,  -625,  -625,  -625,
    -625,   452,   -19,  -625,   578,    -9,  -625,  -625,   148
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     6,   118,    11,    12,     9,    10,    19,    87,   230,
     174,   173,   171,   182,   184,     7,     8,    18,    55,   129,
     203,   220,   419,   519,   474,    56,   199,    57,   133,   613,
     614,   653,   633,   615,   616,   651,   617,   618,   619,   620,
     649,   705,   112,   137,    58,   656,    59,   316,   205,   315,
     516,   563,   412,   469,   470,    60,    61,   213,    62,   214,
      63,   216,   650,   197,   235,   683,   503,   536,   554,   584,
     307,   405,   571,   593,   658,   717,   406,   572,   591,   640,
     715,   407,   507,   497,   458,   459,   462,   506,   662,   694,
     594,   639,   701,   721,    64,   200,   310,   408,   542,   465,
     510,   540,    15,    16,    26,    27,   100,    13,    14,    65,
      66,    23,    24,   404,    94,    95,   490,   398,   488
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     196,   324,   326,   306,    97,   198,   140,   168,   467,   467,
     117,   500,   228,   231,   328,    89,   101,   178,   336,   288,
     630,   672,   508,   204,   187,   331,   332,   415,   281,   331,
     332,   597,   597,   217,   218,   288,   308,    21,   331,   332,
      21,   428,   630,   632,   630,   421,   422,   622,   598,   598,
     621,   215,   322,   597,    17,   597,   655,   402,   207,   208,
     558,   210,   212,   630,   453,   632,   630,   222,   631,   605,
     598,   403,   598,   413,   597,   621,   711,   597,   454,    20,
     233,   234,   135,   136,   331,   332,   309,   676,   718,   641,
     642,   598,   331,   332,   598,   586,    88,   623,   272,   273,
     274,   703,   254,   255,   567,   278,    22,   704,    89,    22,
     631,   605,   509,   285,   294,   394,   395,   621,   700,   333,
     294,    25,   499,   333,   566,   472,   418,   219,   712,    96,
     643,    90,   333,   301,    91,    92,    93,   229,   232,   423,
     113,   513,   114,   211,   468,   468,   595,   117,   115,   610,
     289,   116,   341,   674,   414,   612,   334,   120,   335,   282,
     334,   340,   424,   121,   343,   344,   289,   346,   347,   334,
     644,   517,   349,   350,   351,   352,   353,   697,   333,   356,
     291,   292,   327,   360,   361,   119,   333,   364,   365,   122,
     138,   610,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   123,   334,   473,   546,   124,   392,
     393,   142,   143,   334,    90,   564,   125,    91,    98,    99,
     295,   425,   426,   296,   297,   298,   295,   126,   127,   296,
     297,   451,   128,   491,   492,   493,   494,   495,   144,   145,
     130,   630,   581,   582,   583,   146,   147,   148,   131,   630,
     132,   416,   597,   417,   302,   134,   149,   150,   151,   139,
     597,   518,   311,   152,   312,   322,   141,   323,   153,   598,
     169,   491,   492,   493,   494,   495,   154,   598,   322,   170,
     325,   155,   156,   157,   158,   159,   160,   172,   646,   647,
       1,     2,     3,   161,   545,   162,   641,   642,   322,   180,
     677,     4,   175,   176,   303,   177,   547,   496,     5,   179,
     163,   181,   183,    97,   185,   188,   164,   304,   186,   189,
     190,   191,    40,   268,   269,   270,   271,   272,   273,   274,
     192,   457,   461,   457,   464,   194,   142,   143,   195,   201,
     305,   165,   202,   276,   204,   496,    50,    51,   166,   167,
     306,   206,   209,   302,   215,   221,   479,   223,    52,   480,
     481,   224,   482,   144,   145,   225,   227,   236,   483,   484,
     146,   147,   148,   237,   238,   486,   239,   644,   240,   450,
     241,   149,   150,   151,   279,   290,   280,   286,   152,   242,
     293,   243,   244,   153,   270,   271,   272,   273,   274,   245,
     246,   154,   247,   573,   248,   249,   155,   156,   157,   158,
     159,   160,   250,   251,   252,   253,   304,   287,   161,   330,
     162,    40,   300,   299,   532,   313,   317,   561,   318,   319,
     320,   321,   338,   339,   329,   163,   354,   337,   355,   305,
     357,   164,   358,   359,   362,    50,    51,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   363,    52,   366,   587,
     551,   389,   390,   391,   396,   557,   165,   559,   277,   397,
     529,   530,   401,   166,   167,   409,   478,   436,   569,   142,
     143,   399,   400,   411,   420,   471,   283,   475,   437,   485,
     452,   466,   498,   719,   487,   720,   580,   439,   440,   722,
     489,   502,   501,   588,   505,   511,   144,   145,   441,   444,
     445,   448,   512,   146,   147,   148,   476,   477,   515,   533,
     659,   520,   527,   531,   149,   150,   151,   528,   534,   543,
     535,   152,   538,   539,   541,   544,   153,   322,   553,   548,
     560,   552,   565,   562,   154,   576,   570,   577,   578,   155,
     156,   157,   158,   159,   160,   142,   143,   499,   585,   590,
     624,   161,   665,   162,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   625,   626,   627,   628,   629,   163,   634,
     636,   660,   144,   145,   164,   637,   654,   661,   663,   455,
     147,   148,   456,   664,   666,  -107,   668,   669,   667,   670,
     149,   150,   151,   671,   679,   681,   682,   152,   692,   165,
     693,   284,   153,   702,  -123,   689,   166,   167,   695,   696,
     154,   714,   314,    67,   698,   155,   156,   157,   158,   159,
     160,   142,   143,   556,   226,   699,   706,   161,   345,   162,
     673,   686,   680,   635,   460,   713,   657,   638,   707,   514,
     709,   710,   716,   537,   163,   555,   691,    68,   144,   145,
     164,   678,   102,   463,   193,   146,   147,   148,   549,   342,
       0,     0,     0,     0,     0,     0,   149,   150,   151,     0,
      69,     0,     0,   152,     0,   165,     0,     0,   153,     0,
       0,     0,   166,   167,     0,     0,   154,     0,     0,     0,
       0,   155,   156,   157,   158,   159,   160,   142,   143,     0,
       0,     0,    70,   161,     0,   162,     0,     0,    71,    72,
      73,    74,    75,   -43,    76,    77,    78,     0,    79,    80,
     163,    81,    82,    83,   144,   145,   164,     0,    84,    85,
      86,   146,   147,   148,     0,     0,   596,     0,     0,     0,
       0,     0,   149,   150,   151,     0,     0,   597,     0,   152,
       0,   165,     0,     0,   153,     0,     0,     0,   166,   167,
       0,     0,   154,     0,   598,     0,     0,   155,   156,   157,
     158,   159,   160,   599,   600,   601,   602,   603,     0,   161,
       0,   162,     0,   604,   605,   264,   265,   266,   267,   268,
     269,   270,   271,   272,   273,   274,   163,     0,     0,     0,
       0,     0,   164,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274,   606,    28,
     607,     0,     0,     0,   608,     0,     0,   165,    50,    51,
       0,     0,     0,     0,   166,   167,     0,     0,     0,   256,
       0,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,     0,
     609,    29,    30,     0,   610,     0,     0,     0,   611,     0,
     612,    31,    32,    33,    34,     0,    35,    36,     0,     0,
       0,     0,     0,     0,    37,    38,    39,    40,     0,     0,
      28,     0,     0,     0,    41,    42,    43,    44,    45,    46,
       0,     0,     0,     0,    47,    48,    49,     0,     0,     0,
       0,    50,    51,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   410,    52,     0,     0,     0,     0,     0,     0,
       0,     0,    29,    30,     0,    53,     0,     0,     0,     0,
       0,  -312,    31,    32,    33,    34,     0,    35,    36,     0,
       0,    54,     0,     0,     0,    37,    38,    39,    40,   430,
       0,   431,     0,     0,     0,    41,    42,    43,    44,    45,
      46,     0,     0,     0,     0,    47,    48,    49,     0,     0,
       0,     0,    50,    51,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    52,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    53,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,    54,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,     0,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,     0,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,     0,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,     0,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   256,     0,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,     0,   256,   348,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   432,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   435,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   438,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   446,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   521,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,     0,   256,   522,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   256,   523,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   256,   524,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   256,   525,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   256,   526,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   256,   550,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,     0,   256,   568,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   256,   574,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   256,   575,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   256,   579,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   256,   589,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   256,   592,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
     271,   272,   273,   274,   275,     0,     0,     0,     0,     0,
       0,    67,     0,     0,     0,     0,     0,     0,   645,   648,
       0,     0,   652,   256,   427,   257,   258,   259,   260,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   429,   433,    68,   675,   645,     0,     0,
       0,     0,     0,     0,     0,     0,   684,   685,     0,   652,
       0,   687,   688,     0,   434,     0,     0,   690,    69,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   675,
       0,     0,     0,     0,   442,     0,     0,     0,     0,     0,
     708,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      70,     0,     0,     0,   443,     0,    71,    72,    73,    74,
      75,     0,    76,    77,    78,     0,    79,    80,     0,    81,
      82,    83,     0,     0,     0,     0,    84,    85,    86,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   256,   447,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,     0,     0,   499,   256,   449,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   256,   504,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   256,     0,   257,   258,   259,
     260,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,   272,   273,   274
};

static const yytype_int16 yycheck[] =
{
     107,   213,   214,   199,    23,   112,    69,    71,     4,     4,
       4,   457,    10,    10,   215,     4,    25,    81,   221,     4,
       4,     4,     4,     4,    88,     4,     5,   316,     4,     4,
       5,    15,    15,     4,     5,     4,     4,    53,     4,     5,
      53,    54,     4,   612,     4,     4,     5,     4,    32,    32,
     593,     4,   135,    15,     0,    15,   139,   122,   121,   122,
     540,   124,   125,     4,   122,   634,     4,   130,    51,    52,
      32,   136,    32,    54,    15,   618,   700,    15,   136,     4,
     144,   145,   135,   136,     4,     5,    54,   656,   712,    51,
      52,    32,     4,     5,    32,   575,    10,    54,    32,    33,
      34,    10,   166,   167,   550,   169,   122,    16,     4,   122,
      51,    52,    94,   177,     4,     3,     4,   660,   691,    98,
       4,    53,    37,    98,    39,   414,   327,    98,   701,    53,
      92,   120,    98,   197,   123,   124,   125,   135,   135,    98,
      53,   137,    53,   137,   140,   140,   592,     4,    37,   132,
     135,    37,   137,   137,   135,   138,   135,    37,   137,   135,
     135,   225,   137,    37,   228,   229,   135,   231,   232,   135,
     132,   137,   236,   237,   238,   239,   240,   137,    98,   243,
     189,   190,   135,   247,   248,    53,    98,   251,   252,    37,
     126,   132,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,   271,   272,   273,
     274,   275,   276,   277,    37,   135,   419,   137,    37,   283,
     284,     3,     4,   135,   120,   137,    37,   123,   124,   125,
     120,   338,   339,   123,   124,   125,   120,    37,    37,   123,
     124,   125,    37,    63,    64,    65,    66,    67,    30,    31,
      37,     4,   129,   130,   131,    37,    38,    39,    37,     4,
      37,   324,    15,   326,     4,    37,    48,    49,    50,     4,
      15,   474,   135,    55,   137,   135,     4,   137,    60,    32,
       4,    63,    64,    65,    66,    67,    68,    32,   135,     4,
     137,    73,    74,    75,    76,    77,    78,     4,    51,    52,
     116,   117,   118,    85,   516,    87,    51,    52,   135,     3,
     137,   127,     4,     4,    54,     4,   519,   137,   134,     4,
     102,     4,     4,   342,     4,     4,   108,    67,   104,    16,
      16,    54,    72,    28,    29,    30,    31,    32,    33,    34,
     136,   405,   406,   407,   408,    54,     3,     4,   136,     4,
      90,   133,     4,    10,     4,   137,    96,    97,   140,   141,
     556,     4,     4,     4,     4,    37,   430,     4,   108,   433,
     434,     4,   436,    30,    31,    37,    53,    37,   442,   443,
      37,    38,    39,    37,    37,   449,    37,   132,    37,   398,
      37,    48,    49,    50,   135,    53,   135,   135,    55,    37,
     136,    37,    37,    60,    30,    31,    32,    33,    34,    37,
      37,    68,    37,    54,    37,    37,    73,    74,    75,    76,
      77,    78,    37,    37,    37,    37,    67,   135,    85,    37,
      87,    72,   136,    54,   498,   137,   137,   544,   137,   137,
     137,   137,    10,    10,   137,   102,     4,   137,     4,    90,
       4,   108,     4,     4,     4,    96,    97,     6,     7,     8,
       9,    10,    11,    12,    13,    14,     4,   108,     4,   576,
     534,     4,     4,     4,     4,   539,   133,   541,   135,     4,
     489,   490,     4,   140,   141,     4,     4,   135,   552,     3,
       4,   136,   136,    37,    37,    16,    10,    37,   137,     4,
      53,   135,    37,   715,   136,   717,   570,   137,   137,   721,
      16,    94,    16,   577,    84,     4,    30,    31,   137,   137,
     137,   137,     4,    37,    38,    39,   137,   137,    85,    16,
     637,   137,   137,   137,    48,    49,    50,    54,    37,   137,
      38,    55,    53,    37,    37,    10,    60,   135,    95,   136,
     136,    37,    54,    87,    68,    10,    37,    37,    16,    73,
      74,    75,    76,    77,    78,     3,     4,    37,    53,    53,
      16,    85,   137,    87,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    37,    37,    37,    37,    37,   102,    37,
      54,    53,    30,    31,   108,    37,    37,    25,    37,    37,
      38,    39,    40,    37,    37,    37,    37,    37,   137,   137,
      48,    49,    50,   137,   137,     4,    94,    55,    54,   133,
      25,   135,    60,     4,   137,   137,   140,   141,   137,   137,
      68,     4,   203,     4,   137,    73,    74,    75,    76,    77,
      78,     3,     4,   538,   134,   137,   137,    85,   230,    87,
     651,   666,   660,   618,    16,   703,   634,   621,   137,   469,
     137,   137,   137,   506,   102,   537,   678,    38,    30,    31,
     108,   658,    26,   407,    96,    37,    38,    39,   530,   227,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    50,    -1,
      61,    -1,    -1,    55,    -1,   133,    -1,    -1,    60,    -1,
      -1,    -1,   140,   141,    -1,    -1,    68,    -1,    -1,    -1,
      -1,    73,    74,    75,    76,    77,    78,     3,     4,    -1,
      -1,    -1,    93,    85,    -1,    87,    -1,    -1,    99,   100,
     101,   102,   103,   104,   105,   106,   107,    -1,   109,   110,
     102,   112,   113,   114,    30,    31,   108,    -1,   119,   120,
     121,    37,    38,    39,    -1,    -1,     4,    -1,    -1,    -1,
      -1,    -1,    48,    49,    50,    -1,    -1,    15,    -1,    55,
      -1,   133,    -1,    -1,    60,    -1,    -1,    -1,   140,   141,
      -1,    -1,    68,    -1,    32,    -1,    -1,    73,    74,    75,
      76,    77,    78,    41,    42,    43,    44,    45,    -1,    85,
      -1,    87,    -1,    51,    52,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,   102,    -1,    -1,    -1,
      -1,    -1,   108,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    86,     4,
      88,    -1,    -1,    -1,    92,    -1,    -1,   133,    96,    97,
      -1,    -1,    -1,    -1,   140,   141,    -1,    -1,    -1,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
     128,    46,    47,    -1,   132,    -1,    -1,    -1,   136,    -1,
     138,    56,    57,    58,    59,    -1,    61,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    70,    71,    72,    -1,    -1,
       4,    -1,    -1,    -1,    79,    80,    81,    82,    83,    84,
      -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      -1,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    36,   108,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    46,    47,    -1,   120,    -1,    -1,    -1,    -1,
      -1,   126,    56,    57,    58,    59,    -1,    61,    62,    -1,
      -1,   136,    -1,    -1,    -1,    69,    70,    71,    72,   135,
      -1,   137,    -1,    -1,    -1,    79,    80,    81,    82,    83,
      84,    -1,    -1,    -1,    -1,    89,    90,    91,    -1,    -1,
      -1,    -1,    96,    97,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   120,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,   136,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    15,    -1,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   137,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    15,   137,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    15,   137,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,   135,    -1,    -1,    -1,    -1,    -1,
      -1,     4,    -1,    -1,    -1,    -1,    -1,    -1,   625,   626,
      -1,    -1,   629,    15,   135,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    36,   135,    38,   653,   654,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   663,   664,    -1,   666,
      -1,   668,   669,    -1,   135,    -1,    -1,   674,    61,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   686,
      -1,    -1,    -1,    -1,   135,    -1,    -1,    -1,    -1,    -1,
     697,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      93,    -1,    -1,    -1,   135,    -1,    99,   100,   101,   102,
     103,    -1,   105,   106,   107,    -1,   109,   110,    -1,   112,
     113,   114,    -1,    -1,    -1,    -1,   119,   120,   121,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    15,   135,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    37,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    15,    -1,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   116,   117,   118,   127,   134,   143,   157,   158,   147,
     148,   145,   146,   249,   250,   244,   245,     0,   159,   149,
       4,    53,   122,   253,   254,    53,   246,   247,     4,    46,
      47,    56,    57,    58,    59,    61,    62,    69,    70,    71,
      72,    79,    80,    81,    82,    83,    84,    89,    90,    91,
      96,    97,   108,   120,   136,   160,   167,   169,   186,   188,
     197,   198,   200,   202,   236,   251,   252,     4,    38,    61,
      93,    99,   100,   101,   102,   103,   105,   106,   107,   109,
     110,   112,   113,   114,   119,   120,   121,   150,    10,     4,
     120,   123,   124,   125,   256,   257,    53,   254,   124,   125,
     248,   257,   247,     6,     7,     8,     9,    10,    11,    12,
      13,    14,   184,    53,    53,    37,    37,     4,   144,    53,
      37,    37,    37,    37,    37,    37,    37,    37,    37,   161,
      37,    37,    37,   170,    37,   135,   136,   185,   126,     4,
     144,     4,     3,     4,    30,    31,    37,    38,    39,    48,
      49,    50,    55,    60,    68,    73,    74,    75,    76,    77,
      78,    85,    87,   102,   108,   133,   140,   141,   206,     4,
       4,   154,     4,   153,   152,     4,     4,     4,   206,     4,
       3,     4,   155,     4,   156,     4,   104,   206,     4,    16,
      16,    54,   136,   256,    54,   136,   204,   205,   204,   168,
     237,     4,     4,   162,     4,   190,     4,   144,   144,     4,
     144,   137,   144,   199,   201,     4,   203,     4,     5,    98,
     163,    37,   144,     4,     4,    37,   156,    53,    10,   135,
     151,    10,   135,   206,   206,   206,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,   206,   206,    15,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,   135,    10,   135,   206,   135,
     135,     4,   135,    10,   135,   206,   135,   135,     4,   135,
      53,   257,   257,   136,     4,   120,   123,   124,   125,    54,
     136,   206,     4,    54,    67,    90,   169,   212,     4,    54,
     238,   135,   137,   137,   159,   191,   189,   137,   137,   137,
     137,   137,   135,   137,   187,   137,   187,   135,   203,   137,
      37,     4,     5,    98,   135,   137,   163,   137,    10,    10,
     206,   137,   253,   206,   206,   149,   206,   206,   137,   206,
     206,   206,   206,   206,     4,     4,   206,     4,     4,     4,
     206,   206,     4,     4,   206,   206,     4,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,     4,
       4,     4,   206,   206,     3,     4,     4,     4,   259,   136,
     136,     4,   122,   136,   255,   213,   218,   223,   239,     4,
      36,    37,   194,    54,   135,   190,   144,   144,   203,   164,
      37,     4,     5,    98,   137,   204,   204,   135,    54,    36,
     135,   137,   137,   135,   135,   137,   135,   137,   137,   137,
     137,   137,   135,   135,   137,   137,   137,   135,   137,    16,
     257,   125,    53,   122,   136,    37,    40,   206,   226,   227,
      16,   206,   228,   227,   206,   241,   135,     4,   140,   195,
     196,    16,   190,   163,   166,    37,   137,   137,     4,   206,
     206,   206,   206,   206,   206,     4,   206,   136,   260,    16,
     258,    63,    64,    65,    66,    67,   137,   225,    37,    37,
     226,    16,    94,   208,    16,    84,   229,   224,     4,    94,
     242,     4,     4,   137,   196,    85,   192,   137,   163,   165,
     137,   137,   137,   137,   137,   137,   137,   137,    54,   257,
     257,   137,   206,    16,    37,    38,   209,   208,    53,    37,
     243,    37,   240,   137,    10,   187,   137,   163,   136,   260,
     137,   206,    37,    95,   210,   210,   168,   206,   242,   206,
     136,   204,    87,   193,   137,    54,    39,   226,   137,   206,
      37,   214,   219,    54,   137,   137,    10,    37,    16,   137,
     206,   129,   130,   131,   211,    53,   242,   204,   206,   137,
      53,   220,   137,   215,   232,   226,     4,    15,    32,    41,
      42,    43,    44,    45,    51,    52,    86,    88,    92,   128,
     132,   136,   138,   171,   172,   175,   176,   178,   179,   180,
     181,   186,     4,    54,    16,    37,    37,    37,    37,    37,
       4,    51,   172,   174,    37,   178,    54,    37,   185,   233,
     221,    51,    52,    92,   132,   171,    51,    52,   171,   182,
     204,   177,   171,   173,    37,   139,   187,   174,   216,   204,
      53,    25,   230,    37,    37,   137,    37,   137,    37,    37,
     137,   137,     4,   175,   137,   171,   172,   137,   230,   137,
     180,     4,    94,   207,   171,   171,   173,   171,   171,   137,
     171,   207,    54,    25,   231,   137,   137,   137,   137,   137,
     231,   234,     4,    10,    16,   183,   137,   137,   171,   137,
     137,   183,   231,   182,     4,   222,   137,   217,   183,   187,
     187,   235,   187
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   142,   143,   143,   143,   143,   143,   144,   146,   145,
     148,   147,   149,   149,   150,   150,   150,   150,   150,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
     151,   150,   150,   150,   152,   152,   152,   153,   153,   154,
     154,   155,   155,   155,   156,   156,   156,   158,   157,   159,
     159,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     160,   160,   160,   160,   160,   160,   160,   160,   160,   160,
     161,   160,   160,   162,   160,   160,   160,   163,   163,   163,
     163,   163,   163,   164,   163,   165,   163,   166,   163,   167,
     168,   168,   168,   169,   169,   170,   169,   171,   171,   171,
     172,   172,   172,   172,   172,   172,   172,   172,   172,   173,
     173,   174,   174,   175,   175,   175,   176,   177,   176,   178,
     178,   178,   178,   178,   178,   178,   178,   179,   179,   180,
     180,   181,   181,   181,   181,   181,   182,   183,   183,   184,
     184,   184,   184,   184,   184,   184,   184,   185,   185,   186,
     186,   186,   186,   187,   187,   188,   189,   189,   189,   191,
     190,   192,   193,   194,   194,   195,   195,   196,   196,   197,
     198,   198,   199,   199,   200,   201,   201,   202,   202,   203,
     203,   203,   205,   204,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     207,   207,   208,   208,   209,   209,   210,   210,   211,   211,
     211,   211,   213,   214,   215,   216,   217,   212,   218,   219,
     220,   221,   222,   212,   223,   224,   212,   225,   225,   225,
     225,   225,   226,   226,   226,   227,   227,   227,   227,   228,
     228,   229,   229,   230,   230,   231,   231,   232,   233,   234,
     235,   232,   236,   237,   237,   239,   240,   238,   241,   242,
     242,   242,   243,   243,   245,   244,   246,   246,   247,   248,
     250,   249,   252,   251,   253,   253,   254,   254,   254,   255,
     255,   256,   256,   256,   256,   256,   257,   257,   257,   257,
     258,   257,   259,   257,   257,   257,   257,   257,   257,   257,
     260,   260
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     2,     2,     2,     1,     0,     4,
       0,     2,     3,     0,     2,     4,     1,     1,     2,     1,
       4,     4,     3,     2,     4,     3,     4,     4,     4,     4,
       4,     2,     2,     2,     4,     4,     2,     2,     2,     2,
       0,     5,     2,     0,     3,     2,     0,     1,     3,     1,
       3,     0,     1,     3,     1,     2,     3,     0,     2,     2,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     4,     4,     4,     4,     8,     4,     1,     1,     4,
       0,     5,     4,     0,     5,     4,     4,     1,     3,     2,
       1,     3,     2,     0,     5,     0,     7,     0,     6,     4,
       2,     2,     0,     4,     2,     0,     7,     1,     1,     1,
       1,     5,     4,     4,     7,     7,     7,     7,     8,     2,
       1,     3,     1,     1,     3,     4,     1,     0,     5,     2,
       1,     1,     1,     4,     1,     4,     4,     2,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     6,     6,     1,     0,     5,     2,     3,     0,     0,
       7,     3,     3,     0,     3,     1,     2,     1,     2,     4,
       4,     3,     3,     1,     4,     3,     0,     1,     1,     0,
       2,     3,     0,     2,     2,     3,     4,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     5,     3,     3,     4,
       1,     1,     4,     4,     4,     4,     4,     4,     6,     6,
       6,     4,     6,     4,     1,     6,     6,     6,     4,     4,
       3,     0,     4,     0,     4,     0,     4,     0,     1,     1,
       1,     0,     0,     0,     0,     0,     0,    19,     0,     0,
       0,     0,     0,    18,     0,     0,     7,     1,     1,     1,
       1,     1,     3,     0,     2,     3,     2,     6,    10,     2,
       1,     0,     1,     2,     0,     0,     3,     0,     0,     0,
       0,    11,     4,     0,     2,     0,     0,     6,     1,     0,
       3,     5,     0,     3,     0,     2,     1,     2,     4,     2,
       0,     2,     0,     5,     1,     2,     4,     5,     6,     1,
       2,     0,     2,     4,     4,     8,     1,     1,     3,     3,
       0,     9,     0,     7,     1,     3,     1,     3,     1,     3,
       0,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
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
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
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

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

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
      yychar = yylex ();
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
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 8:
#line 175 "ldgram.y" /* yacc.c:1646  */
    { ldlex_defsym(); }
#line 2263 "ldgram.c" /* yacc.c:1646  */
    break;

  case 9:
#line 177 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate();
		  lang_add_assignment(exp_assop((yyvsp[-1].token),(yyvsp[-2].name),(yyvsp[0].etree)));
		}
#line 2272 "ldgram.c" /* yacc.c:1646  */
    break;

  case 10:
#line 185 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_mri_script ();
		  PUSH_ERROR (_("MRI style script"));
		}
#line 2281 "ldgram.c" /* yacc.c:1646  */
    break;

  case 11:
#line 190 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  mri_draw_tree ();
		  POP_ERROR ();
		}
#line 2291 "ldgram.c" /* yacc.c:1646  */
    break;

  case 16:
#line 205 "ldgram.y" /* yacc.c:1646  */
    {
			einfo(_("%P%F: unrecognised keyword in MRI style script '%s'\n"),(yyvsp[0].name));
			}
#line 2299 "ldgram.c" /* yacc.c:1646  */
    break;

  case 17:
#line 208 "ldgram.y" /* yacc.c:1646  */
    {
			config.map_filename = "-";
			}
#line 2307 "ldgram.c" /* yacc.c:1646  */
    break;

  case 20:
#line 214 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2313 "ldgram.c" /* yacc.c:1646  */
    break;

  case 21:
#line 216 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-2].name), (yyvsp[0].etree)); }
#line 2319 "ldgram.c" /* yacc.c:1646  */
    break;

  case 22:
#line 218 "ldgram.y" /* yacc.c:1646  */
    { mri_public((yyvsp[-1].name), (yyvsp[0].etree)); }
#line 2325 "ldgram.c" /* yacc.c:1646  */
    break;

  case 23:
#line 220 "ldgram.y" /* yacc.c:1646  */
    { mri_format((yyvsp[0].name)); }
#line 2331 "ldgram.c" /* yacc.c:1646  */
    break;

  case 24:
#line 222 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2337 "ldgram.c" /* yacc.c:1646  */
    break;

  case 25:
#line 224 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-1].name), (yyvsp[0].etree));}
#line 2343 "ldgram.c" /* yacc.c:1646  */
    break;

  case 26:
#line 226 "ldgram.y" /* yacc.c:1646  */
    { mri_output_section((yyvsp[-2].name), (yyvsp[0].etree));}
#line 2349 "ldgram.c" /* yacc.c:1646  */
    break;

  case 27:
#line 228 "ldgram.y" /* yacc.c:1646  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2355 "ldgram.c" /* yacc.c:1646  */
    break;

  case 28:
#line 230 "ldgram.y" /* yacc.c:1646  */
    { mri_align((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2361 "ldgram.c" /* yacc.c:1646  */
    break;

  case 29:
#line 232 "ldgram.y" /* yacc.c:1646  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2367 "ldgram.c" /* yacc.c:1646  */
    break;

  case 30:
#line 234 "ldgram.y" /* yacc.c:1646  */
    { mri_alignmod((yyvsp[-2].name),(yyvsp[0].etree)); }
#line 2373 "ldgram.c" /* yacc.c:1646  */
    break;

  case 33:
#line 238 "ldgram.y" /* yacc.c:1646  */
    { mri_name((yyvsp[0].name)); }
#line 2379 "ldgram.c" /* yacc.c:1646  */
    break;

  case 34:
#line 240 "ldgram.y" /* yacc.c:1646  */
    { mri_alias((yyvsp[-2].name),(yyvsp[0].name),0);}
#line 2385 "ldgram.c" /* yacc.c:1646  */
    break;

  case 35:
#line 242 "ldgram.y" /* yacc.c:1646  */
    { mri_alias ((yyvsp[-2].name), 0, (int) (yyvsp[0].bigint).integer); }
#line 2391 "ldgram.c" /* yacc.c:1646  */
    break;

  case 36:
#line 244 "ldgram.y" /* yacc.c:1646  */
    { mri_base((yyvsp[0].etree)); }
#line 2397 "ldgram.c" /* yacc.c:1646  */
    break;

  case 37:
#line 246 "ldgram.y" /* yacc.c:1646  */
    { mri_truncate ((unsigned int) (yyvsp[0].bigint).integer); }
#line 2403 "ldgram.c" /* yacc.c:1646  */
    break;

  case 40:
#line 250 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2409 "ldgram.c" /* yacc.c:1646  */
    break;

  case 41:
#line 252 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 2415 "ldgram.c" /* yacc.c:1646  */
    break;

  case 42:
#line 254 "ldgram.y" /* yacc.c:1646  */
    { lang_add_entry ((yyvsp[0].name), FALSE); }
#line 2421 "ldgram.c" /* yacc.c:1646  */
    break;

  case 44:
#line 259 "ldgram.y" /* yacc.c:1646  */
    { mri_order((yyvsp[0].name)); }
#line 2427 "ldgram.c" /* yacc.c:1646  */
    break;

  case 45:
#line 260 "ldgram.y" /* yacc.c:1646  */
    { mri_order((yyvsp[0].name)); }
#line 2433 "ldgram.c" /* yacc.c:1646  */
    break;

  case 47:
#line 266 "ldgram.y" /* yacc.c:1646  */
    { mri_load((yyvsp[0].name)); }
#line 2439 "ldgram.c" /* yacc.c:1646  */
    break;

  case 48:
#line 267 "ldgram.y" /* yacc.c:1646  */
    { mri_load((yyvsp[0].name)); }
#line 2445 "ldgram.c" /* yacc.c:1646  */
    break;

  case 49:
#line 272 "ldgram.y" /* yacc.c:1646  */
    { mri_only_load((yyvsp[0].name)); }
#line 2451 "ldgram.c" /* yacc.c:1646  */
    break;

  case 50:
#line 274 "ldgram.y" /* yacc.c:1646  */
    { mri_only_load((yyvsp[0].name)); }
#line 2457 "ldgram.c" /* yacc.c:1646  */
    break;

  case 51:
#line 278 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = NULL; }
#line 2463 "ldgram.c" /* yacc.c:1646  */
    break;

  case 54:
#line 285 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name)); }
#line 2469 "ldgram.c" /* yacc.c:1646  */
    break;

  case 55:
#line 287 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name)); }
#line 2475 "ldgram.c" /* yacc.c:1646  */
    break;

  case 56:
#line 289 "ldgram.y" /* yacc.c:1646  */
    { ldlang_add_undef ((yyvsp[0].name)); }
#line 2481 "ldgram.c" /* yacc.c:1646  */
    break;

  case 57:
#line 293 "ldgram.y" /* yacc.c:1646  */
    {
	 ldlex_both();
	}
#line 2489 "ldgram.c" /* yacc.c:1646  */
    break;

  case 58:
#line 297 "ldgram.y" /* yacc.c:1646  */
    {
	ldlex_popstate();
	}
#line 2497 "ldgram.c" /* yacc.c:1646  */
    break;

  case 71:
#line 322 "ldgram.y" /* yacc.c:1646  */
    { lang_add_target((yyvsp[-1].name)); }
#line 2503 "ldgram.c" /* yacc.c:1646  */
    break;

  case 72:
#line 324 "ldgram.y" /* yacc.c:1646  */
    { ldfile_add_library_path ((yyvsp[-1].name), FALSE); }
#line 2509 "ldgram.c" /* yacc.c:1646  */
    break;

  case 73:
#line 326 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output((yyvsp[-1].name), 1); }
#line 2515 "ldgram.c" /* yacc.c:1646  */
    break;

  case 74:
#line 328 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output_format ((yyvsp[-1].name), (char *) NULL,
					    (char *) NULL, 1); }
#line 2522 "ldgram.c" /* yacc.c:1646  */
    break;

  case 75:
#line 331 "ldgram.y" /* yacc.c:1646  */
    { lang_add_output_format ((yyvsp[-5].name), (yyvsp[-3].name), (yyvsp[-1].name), 1); }
#line 2528 "ldgram.c" /* yacc.c:1646  */
    break;

  case 76:
#line 333 "ldgram.y" /* yacc.c:1646  */
    { ldfile_set_output_arch ((yyvsp[-1].name), bfd_arch_unknown); }
#line 2534 "ldgram.c" /* yacc.c:1646  */
    break;

  case 77:
#line 335 "ldgram.y" /* yacc.c:1646  */
    { command_line.force_common_definition = TRUE ; }
#line 2540 "ldgram.c" /* yacc.c:1646  */
    break;

  case 78:
#line 337 "ldgram.y" /* yacc.c:1646  */
    { command_line.inhibit_common_definition = TRUE ; }
#line 2546 "ldgram.c" /* yacc.c:1646  */
    break;

  case 80:
#line 340 "ldgram.y" /* yacc.c:1646  */
    { lang_enter_group (); }
#line 2552 "ldgram.c" /* yacc.c:1646  */
    break;

  case 81:
#line 342 "ldgram.y" /* yacc.c:1646  */
    { lang_leave_group (); }
#line 2558 "ldgram.c" /* yacc.c:1646  */
    break;

  case 82:
#line 344 "ldgram.y" /* yacc.c:1646  */
    { lang_add_map((yyvsp[-1].name)); }
#line 2564 "ldgram.c" /* yacc.c:1646  */
    break;

  case 83:
#line 346 "ldgram.y" /* yacc.c:1646  */
    { ldlex_script (); ldfile_open_command_file((yyvsp[0].name)); }
#line 2570 "ldgram.c" /* yacc.c:1646  */
    break;

  case 84:
#line 348 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 2576 "ldgram.c" /* yacc.c:1646  */
    break;

  case 85:
#line 350 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_nocrossref ((yyvsp[-1].nocrossref));
		}
#line 2584 "ldgram.c" /* yacc.c:1646  */
    break;

  case 87:
#line 358 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2591 "ldgram.c" /* yacc.c:1646  */
    break;

  case 88:
#line 361 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2598 "ldgram.c" /* yacc.c:1646  */
    break;

  case 89:
#line 364 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_search_file_enum,
				 (char *)NULL); }
#line 2605 "ldgram.c" /* yacc.c:1646  */
    break;

  case 90:
#line 367 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2612 "ldgram.c" /* yacc.c:1646  */
    break;

  case 91:
#line 370 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2619 "ldgram.c" /* yacc.c:1646  */
    break;

  case 92:
#line 373 "ldgram.y" /* yacc.c:1646  */
    { lang_add_input_file((yyvsp[0].name),lang_input_file_is_l_enum,
				 (char *)NULL); }
#line 2626 "ldgram.c" /* yacc.c:1646  */
    break;

  case 93:
#line 376 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = as_needed; as_needed = TRUE; }
#line 2632 "ldgram.c" /* yacc.c:1646  */
    break;

  case 94:
#line 378 "ldgram.y" /* yacc.c:1646  */
    { as_needed = (yyvsp[-2].integer); }
#line 2638 "ldgram.c" /* yacc.c:1646  */
    break;

  case 95:
#line 380 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = as_needed; as_needed = TRUE; }
#line 2644 "ldgram.c" /* yacc.c:1646  */
    break;

  case 96:
#line 382 "ldgram.y" /* yacc.c:1646  */
    { as_needed = (yyvsp[-2].integer); }
#line 2650 "ldgram.c" /* yacc.c:1646  */
    break;

  case 97:
#line 384 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = as_needed; as_needed = TRUE; }
#line 2656 "ldgram.c" /* yacc.c:1646  */
    break;

  case 98:
#line 386 "ldgram.y" /* yacc.c:1646  */
    { as_needed = (yyvsp[-2].integer); }
#line 2662 "ldgram.c" /* yacc.c:1646  */
    break;

  case 103:
#line 401 "ldgram.y" /* yacc.c:1646  */
    { lang_add_entry ((yyvsp[-1].name), FALSE); }
#line 2668 "ldgram.c" /* yacc.c:1646  */
    break;

  case 105:
#line 403 "ldgram.y" /* yacc.c:1646  */
    {ldlex_expression ();}
#line 2674 "ldgram.c" /* yacc.c:1646  */
    break;

  case 106:
#line 404 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate ();
		  lang_add_assignment (exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name))); }
#line 2681 "ldgram.c" /* yacc.c:1646  */
    break;

  case 107:
#line 412 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = (yyvsp[0].name);
			}
#line 2689 "ldgram.c" /* yacc.c:1646  */
    break;

  case 108:
#line 416 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = "*";
			}
#line 2697 "ldgram.c" /* yacc.c:1646  */
    break;

  case 109:
#line 420 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.cname) = "?";
			}
#line 2705 "ldgram.c" /* yacc.c:1646  */
    break;

  case 110:
#line 427 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2715 "ldgram.c" /* yacc.c:1646  */
    break;

  case 111:
#line 433 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[0].cname);
			  (yyval.wildcard).sorted = none;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-2].name_list);
			}
#line 2725 "ldgram.c" /* yacc.c:1646  */
    break;

  case 112:
#line 439 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2735 "ldgram.c" /* yacc.c:1646  */
    break;

  case 113:
#line 445 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2745 "ldgram.c" /* yacc.c:1646  */
    break;

  case 114:
#line 451 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2755 "ldgram.c" /* yacc.c:1646  */
    break;

  case 115:
#line 457 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2765 "ldgram.c" /* yacc.c:1646  */
    break;

  case 116:
#line 463 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment_name;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2775 "ldgram.c" /* yacc.c:1646  */
    break;

  case 117:
#line 469 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-2].cname);
			  (yyval.wildcard).sorted = by_alignment;
			  (yyval.wildcard).exclude_name_list = NULL;
			}
#line 2785 "ldgram.c" /* yacc.c:1646  */
    break;

  case 118:
#line 475 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.wildcard).name = (yyvsp[-1].cname);
			  (yyval.wildcard).sorted = by_name;
			  (yyval.wildcard).exclude_name_list = (yyvsp[-3].name_list);
			}
#line 2795 "ldgram.c" /* yacc.c:1646  */
    break;

  case 119:
#line 484 "ldgram.y" /* yacc.c:1646  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = (yyvsp[-1].name_list);
			  (yyval.name_list) = tmp;
			}
#line 2807 "ldgram.c" /* yacc.c:1646  */
    break;

  case 120:
#line 493 "ldgram.y" /* yacc.c:1646  */
    {
			  struct name_list *tmp;
			  tmp = (struct name_list *) xmalloc (sizeof *tmp);
			  tmp->name = (yyvsp[0].cname);
			  tmp->next = NULL;
			  (yyval.name_list) = tmp;
			}
#line 2819 "ldgram.c" /* yacc.c:1646  */
    break;

  case 121:
#line 504 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = (yyvsp[-2].wildcard_list);
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 2831 "ldgram.c" /* yacc.c:1646  */
    break;

  case 122:
#line 513 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_list *tmp;
			  tmp = (struct wildcard_list *) xmalloc (sizeof *tmp);
			  tmp->next = NULL;
			  tmp->spec = (yyvsp[0].wildcard);
			  (yyval.wildcard_list) = tmp;
			}
#line 2843 "ldgram.c" /* yacc.c:1646  */
    break;

  case 123:
#line 524 "ldgram.y" /* yacc.c:1646  */
    {
			  struct wildcard_spec tmp;
			  tmp.name = (yyvsp[0].name);
			  tmp.exclude_name_list = NULL;
			  tmp.sorted = none;
			  lang_add_wild (&tmp, NULL, ldgram_had_keep);
			}
#line 2855 "ldgram.c" /* yacc.c:1646  */
    break;

  case 124:
#line 532 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_wild (NULL, (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 2863 "ldgram.c" /* yacc.c:1646  */
    break;

  case 125:
#line 536 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_wild (&(yyvsp[-3].wildcard), (yyvsp[-1].wildcard_list), ldgram_had_keep);
			}
#line 2871 "ldgram.c" /* yacc.c:1646  */
    break;

  case 127:
#line 544 "ldgram.y" /* yacc.c:1646  */
    { ldgram_had_keep = TRUE; }
#line 2877 "ldgram.c" /* yacc.c:1646  */
    break;

  case 128:
#line 546 "ldgram.y" /* yacc.c:1646  */
    { ldgram_had_keep = FALSE; }
#line 2883 "ldgram.c" /* yacc.c:1646  */
    break;

  case 130:
#line 552 "ldgram.y" /* yacc.c:1646  */
    {
 		lang_add_attribute(lang_object_symbols_statement_enum);
	      	}
#line 2891 "ldgram.c" /* yacc.c:1646  */
    break;

  case 132:
#line 557 "ldgram.y" /* yacc.c:1646  */
    {

		  lang_add_attribute(lang_constructors_statement_enum);
		}
#line 2900 "ldgram.c" /* yacc.c:1646  */
    break;

  case 133:
#line 562 "ldgram.y" /* yacc.c:1646  */
    {
		  constructors_sorted = TRUE;
		  lang_add_attribute (lang_constructors_statement_enum);
		}
#line 2909 "ldgram.c" /* yacc.c:1646  */
    break;

  case 135:
#line 568 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_data ((int) (yyvsp[-3].integer), (yyvsp[-1].etree));
			}
#line 2917 "ldgram.c" /* yacc.c:1646  */
    break;

  case 136:
#line 573 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_add_fill ((yyvsp[-1].fill));
			}
#line 2925 "ldgram.c" /* yacc.c:1646  */
    break;

  case 141:
#line 590 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 2931 "ldgram.c" /* yacc.c:1646  */
    break;

  case 142:
#line 592 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 2937 "ldgram.c" /* yacc.c:1646  */
    break;

  case 143:
#line 594 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 2943 "ldgram.c" /* yacc.c:1646  */
    break;

  case 144:
#line 596 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 2949 "ldgram.c" /* yacc.c:1646  */
    break;

  case 145:
#line 598 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].token); }
#line 2955 "ldgram.c" /* yacc.c:1646  */
    break;

  case 146:
#line 603 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.fill) = exp_get_fill ((yyvsp[0].etree), 0, "fill value");
		}
#line 2963 "ldgram.c" /* yacc.c:1646  */
    break;

  case 147:
#line 610 "ldgram.y" /* yacc.c:1646  */
    { (yyval.fill) = (yyvsp[0].fill); }
#line 2969 "ldgram.c" /* yacc.c:1646  */
    break;

  case 148:
#line 611 "ldgram.y" /* yacc.c:1646  */
    { (yyval.fill) = (fill_type *) 0; }
#line 2975 "ldgram.c" /* yacc.c:1646  */
    break;

  case 149:
#line 616 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '+'; }
#line 2981 "ldgram.c" /* yacc.c:1646  */
    break;

  case 150:
#line 618 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '-'; }
#line 2987 "ldgram.c" /* yacc.c:1646  */
    break;

  case 151:
#line 620 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '*'; }
#line 2993 "ldgram.c" /* yacc.c:1646  */
    break;

  case 152:
#line 622 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '/'; }
#line 2999 "ldgram.c" /* yacc.c:1646  */
    break;

  case 153:
#line 624 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = LSHIFT; }
#line 3005 "ldgram.c" /* yacc.c:1646  */
    break;

  case 154:
#line 626 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = RSHIFT; }
#line 3011 "ldgram.c" /* yacc.c:1646  */
    break;

  case 155:
#line 628 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '&'; }
#line 3017 "ldgram.c" /* yacc.c:1646  */
    break;

  case 156:
#line 630 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = '|'; }
#line 3023 "ldgram.c" /* yacc.c:1646  */
    break;

  case 159:
#line 640 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_assop ((yyvsp[-1].token), (yyvsp[-2].name), (yyvsp[0].etree)));
		}
#line 3031 "ldgram.c" /* yacc.c:1646  */
    break;

  case 160:
#line 644 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_assop ('=', (yyvsp[-2].name),
						  exp_binop ((yyvsp[-1].token),
							     exp_nameop (NAME,
									 (yyvsp[-2].name)),
							     (yyvsp[0].etree))));
		}
#line 3043 "ldgram.c" /* yacc.c:1646  */
    break;

  case 161:
#line 652 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), FALSE));
		}
#line 3051 "ldgram.c" /* yacc.c:1646  */
    break;

  case 162:
#line 656 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_add_assignment (exp_provide ((yyvsp[-3].name), (yyvsp[-1].etree), TRUE));
		}
#line 3059 "ldgram.c" /* yacc.c:1646  */
    break;

  case 169:
#line 678 "ldgram.y" /* yacc.c:1646  */
    { region = lang_memory_region_lookup ((yyvsp[0].name), TRUE); }
#line 3065 "ldgram.c" /* yacc.c:1646  */
    break;

  case 170:
#line 681 "ldgram.y" /* yacc.c:1646  */
    {}
#line 3071 "ldgram.c" /* yacc.c:1646  */
    break;

  case 171:
#line 686 "ldgram.y" /* yacc.c:1646  */
    {
		  region->origin = exp_get_vma ((yyvsp[0].etree), 0, "origin");
		  region->current = region->origin;
		}
#line 3080 "ldgram.c" /* yacc.c:1646  */
    break;

  case 172:
#line 694 "ldgram.y" /* yacc.c:1646  */
    {
		  region->length = exp_get_vma ((yyvsp[0].etree), -1, "length");
		}
#line 3088 "ldgram.c" /* yacc.c:1646  */
    break;

  case 173:
#line 701 "ldgram.y" /* yacc.c:1646  */
    { /* dummy action to avoid bison 1.25 error message */ }
#line 3094 "ldgram.c" /* yacc.c:1646  */
    break;

  case 177:
#line 712 "ldgram.y" /* yacc.c:1646  */
    { lang_set_flags (region, (yyvsp[0].name), 0); }
#line 3100 "ldgram.c" /* yacc.c:1646  */
    break;

  case 178:
#line 714 "ldgram.y" /* yacc.c:1646  */
    { lang_set_flags (region, (yyvsp[0].name), 1); }
#line 3106 "ldgram.c" /* yacc.c:1646  */
    break;

  case 179:
#line 719 "ldgram.y" /* yacc.c:1646  */
    { lang_startup((yyvsp[-1].name)); }
#line 3112 "ldgram.c" /* yacc.c:1646  */
    break;

  case 181:
#line 725 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((char *)NULL); }
#line 3118 "ldgram.c" /* yacc.c:1646  */
    break;

  case 182:
#line 730 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3124 "ldgram.c" /* yacc.c:1646  */
    break;

  case 183:
#line 732 "ldgram.y" /* yacc.c:1646  */
    { ldemul_hll((yyvsp[0].name)); }
#line 3130 "ldgram.c" /* yacc.c:1646  */
    break;

  case 185:
#line 740 "ldgram.y" /* yacc.c:1646  */
    { ldemul_syslib((yyvsp[0].name)); }
#line 3136 "ldgram.c" /* yacc.c:1646  */
    break;

  case 187:
#line 746 "ldgram.y" /* yacc.c:1646  */
    { lang_float(TRUE); }
#line 3142 "ldgram.c" /* yacc.c:1646  */
    break;

  case 188:
#line 748 "ldgram.y" /* yacc.c:1646  */
    { lang_float(FALSE); }
#line 3148 "ldgram.c" /* yacc.c:1646  */
    break;

  case 189:
#line 753 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.nocrossref) = NULL;
		}
#line 3156 "ldgram.c" /* yacc.c:1646  */
    break;

  case 190:
#line 757 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-1].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3169 "ldgram.c" /* yacc.c:1646  */
    break;

  case 191:
#line 766 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_nocrossref *n;

		  n = (struct lang_nocrossref *) xmalloc (sizeof *n);
		  n->name = (yyvsp[-2].name);
		  n->next = (yyvsp[0].nocrossref);
		  (yyval.nocrossref) = n;
		}
#line 3182 "ldgram.c" /* yacc.c:1646  */
    break;

  case 192:
#line 776 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3188 "ldgram.c" /* yacc.c:1646  */
    break;

  case 193:
#line 778 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); (yyval.etree)=(yyvsp[0].etree);}
#line 3194 "ldgram.c" /* yacc.c:1646  */
    break;

  case 194:
#line 783 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('-', (yyvsp[0].etree)); }
#line 3200 "ldgram.c" /* yacc.c:1646  */
    break;

  case 195:
#line 785 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3206 "ldgram.c" /* yacc.c:1646  */
    break;

  case 196:
#line 787 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ((int) (yyvsp[-3].integer),(yyvsp[-1].etree)); }
#line 3212 "ldgram.c" /* yacc.c:1646  */
    break;

  case 197:
#line 789 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('!', (yyvsp[0].etree)); }
#line 3218 "ldgram.c" /* yacc.c:1646  */
    break;

  case 198:
#line 791 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[0].etree); }
#line 3224 "ldgram.c" /* yacc.c:1646  */
    break;

  case 199:
#line 793 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop ('~', (yyvsp[0].etree));}
#line 3230 "ldgram.c" /* yacc.c:1646  */
    break;

  case 200:
#line 796 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('*', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3236 "ldgram.c" /* yacc.c:1646  */
    break;

  case 201:
#line 798 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('/', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3242 "ldgram.c" /* yacc.c:1646  */
    break;

  case 202:
#line 800 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('%', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3248 "ldgram.c" /* yacc.c:1646  */
    break;

  case 203:
#line 802 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('+', (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3254 "ldgram.c" /* yacc.c:1646  */
    break;

  case 204:
#line 804 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('-' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3260 "ldgram.c" /* yacc.c:1646  */
    break;

  case 205:
#line 806 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (LSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3266 "ldgram.c" /* yacc.c:1646  */
    break;

  case 206:
#line 808 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (RSHIFT , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3272 "ldgram.c" /* yacc.c:1646  */
    break;

  case 207:
#line 810 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (EQ , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3278 "ldgram.c" /* yacc.c:1646  */
    break;

  case 208:
#line 812 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (NE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3284 "ldgram.c" /* yacc.c:1646  */
    break;

  case 209:
#line 814 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (LE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3290 "ldgram.c" /* yacc.c:1646  */
    break;

  case 210:
#line 816 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (GE , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3296 "ldgram.c" /* yacc.c:1646  */
    break;

  case 211:
#line 818 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('<' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3302 "ldgram.c" /* yacc.c:1646  */
    break;

  case 212:
#line 820 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('>' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3308 "ldgram.c" /* yacc.c:1646  */
    break;

  case 213:
#line 822 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('&' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3314 "ldgram.c" /* yacc.c:1646  */
    break;

  case 214:
#line 824 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('^' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3320 "ldgram.c" /* yacc.c:1646  */
    break;

  case 215:
#line 826 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop ('|' , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3326 "ldgram.c" /* yacc.c:1646  */
    break;

  case 216:
#line 828 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_trinop ('?' , (yyvsp[-4].etree), (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3332 "ldgram.c" /* yacc.c:1646  */
    break;

  case 217:
#line 830 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (ANDAND , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3338 "ldgram.c" /* yacc.c:1646  */
    break;

  case 218:
#line 832 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (OROR , (yyvsp[-2].etree), (yyvsp[0].etree)); }
#line 3344 "ldgram.c" /* yacc.c:1646  */
    break;

  case 219:
#line 834 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (DEFINED, (yyvsp[-1].name)); }
#line 3350 "ldgram.c" /* yacc.c:1646  */
    break;

  case 220:
#line 836 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_bigintop ((yyvsp[0].bigint).integer, (yyvsp[0].bigint).str); }
#line 3356 "ldgram.c" /* yacc.c:1646  */
    break;

  case 221:
#line 838 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (SIZEOF_HEADERS,0); }
#line 3362 "ldgram.c" /* yacc.c:1646  */
    break;

  case 222:
#line 841 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (SIZEOF,(yyvsp[-1].name)); }
#line 3368 "ldgram.c" /* yacc.c:1646  */
    break;

  case 223:
#line 843 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (ADDR,(yyvsp[-1].name)); }
#line 3374 "ldgram.c" /* yacc.c:1646  */
    break;

  case 224:
#line 845 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (LOADADDR,(yyvsp[-1].name)); }
#line 3380 "ldgram.c" /* yacc.c:1646  */
    break;

  case 225:
#line 847 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (CONSTANT,(yyvsp[-1].name)); }
#line 3386 "ldgram.c" /* yacc.c:1646  */
    break;

  case 226:
#line 849 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ABSOLUTE, (yyvsp[-1].etree)); }
#line 3392 "ldgram.c" /* yacc.c:1646  */
    break;

  case 227:
#line 851 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3398 "ldgram.c" /* yacc.c:1646  */
    break;

  case 228:
#line 853 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (ALIGN_K,(yyvsp[-3].etree),(yyvsp[-1].etree)); }
#line 3404 "ldgram.c" /* yacc.c:1646  */
    break;

  case 229:
#line 855 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_ALIGN, (yyvsp[-3].etree), (yyvsp[-1].etree)); }
#line 3410 "ldgram.c" /* yacc.c:1646  */
    break;

  case 230:
#line 857 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (DATA_SEGMENT_RELRO_END, (yyvsp[-1].etree), (yyvsp[-3].etree)); }
#line 3416 "ldgram.c" /* yacc.c:1646  */
    break;

  case 231:
#line 859 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (DATA_SEGMENT_END, (yyvsp[-1].etree)); }
#line 3422 "ldgram.c" /* yacc.c:1646  */
    break;

  case 232:
#line 861 "ldgram.y" /* yacc.c:1646  */
    { /* The operands to the expression node are
			     placed in the opposite order from the way
			     in which they appear in the script as
			     that allows us to reuse more code in
			     fold_binary.  */
			  (yyval.etree) = exp_binop (SEGMENT_START,
					  (yyvsp[-1].etree),
					  exp_nameop (NAME, (yyvsp[-3].name))); }
#line 3435 "ldgram.c" /* yacc.c:1646  */
    break;

  case 233:
#line 870 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_unop (ALIGN_K,(yyvsp[-1].etree)); }
#line 3441 "ldgram.c" /* yacc.c:1646  */
    break;

  case 234:
#line 872 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (NAME,(yyvsp[0].name)); }
#line 3447 "ldgram.c" /* yacc.c:1646  */
    break;

  case 235:
#line 874 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (MAX_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3453 "ldgram.c" /* yacc.c:1646  */
    break;

  case 236:
#line 876 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_binop (MIN_K, (yyvsp[-3].etree), (yyvsp[-1].etree) ); }
#line 3459 "ldgram.c" /* yacc.c:1646  */
    break;

  case 237:
#line 878 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_assert ((yyvsp[-3].etree), (yyvsp[-1].name)); }
#line 3465 "ldgram.c" /* yacc.c:1646  */
    break;

  case 238:
#line 880 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (ORIGIN, (yyvsp[-1].name)); }
#line 3471 "ldgram.c" /* yacc.c:1646  */
    break;

  case 239:
#line 882 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = exp_nameop (LENGTH, (yyvsp[-1].name)); }
#line 3477 "ldgram.c" /* yacc.c:1646  */
    break;

  case 240:
#line 887 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = (yyvsp[0].name); }
#line 3483 "ldgram.c" /* yacc.c:1646  */
    break;

  case 241:
#line 888 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = 0; }
#line 3489 "ldgram.c" /* yacc.c:1646  */
    break;

  case 242:
#line 892 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3495 "ldgram.c" /* yacc.c:1646  */
    break;

  case 243:
#line 893 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3501 "ldgram.c" /* yacc.c:1646  */
    break;

  case 244:
#line 897 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3507 "ldgram.c" /* yacc.c:1646  */
    break;

  case 245:
#line 898 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3513 "ldgram.c" /* yacc.c:1646  */
    break;

  case 246:
#line 902 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3519 "ldgram.c" /* yacc.c:1646  */
    break;

  case 247:
#line 903 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = 0; }
#line 3525 "ldgram.c" /* yacc.c:1646  */
    break;

  case 248:
#line 907 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = ONLY_IF_RO; }
#line 3531 "ldgram.c" /* yacc.c:1646  */
    break;

  case 249:
#line 908 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = ONLY_IF_RW; }
#line 3537 "ldgram.c" /* yacc.c:1646  */
    break;

  case 250:
#line 909 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = SPECIAL; }
#line 3543 "ldgram.c" /* yacc.c:1646  */
    break;

  case 251:
#line 910 "ldgram.y" /* yacc.c:1646  */
    { (yyval.token) = 0; }
#line 3549 "ldgram.c" /* yacc.c:1646  */
    break;

  case 252:
#line 913 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression(); }
#line 3555 "ldgram.c" /* yacc.c:1646  */
    break;

  case 253:
#line 917 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_script (); }
#line 3561 "ldgram.c" /* yacc.c:1646  */
    break;

  case 254:
#line 920 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_enter_output_section_statement((yyvsp[-8].name), (yyvsp[-6].etree),
							      sectype,
							      (yyvsp[-4].etree), (yyvsp[-3].etree), (yyvsp[-5].etree), (yyvsp[-1].token));
			}
#line 3571 "ldgram.c" /* yacc.c:1646  */
    break;

  case 255:
#line 926 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3577 "ldgram.c" /* yacc.c:1646  */
    break;

  case 256:
#line 928 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  lang_leave_output_section_statement ((yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
		}
#line 3586 "ldgram.c" /* yacc.c:1646  */
    break;

  case 257:
#line 933 "ldgram.y" /* yacc.c:1646  */
    {}
#line 3592 "ldgram.c" /* yacc.c:1646  */
    break;

  case 258:
#line 935 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3598 "ldgram.c" /* yacc.c:1646  */
    break;

  case 259:
#line 937 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_script (); }
#line 3604 "ldgram.c" /* yacc.c:1646  */
    break;

  case 260:
#line 939 "ldgram.y" /* yacc.c:1646  */
    {
			  lang_enter_overlay ((yyvsp[-5].etree), (yyvsp[-2].etree));
			}
#line 3612 "ldgram.c" /* yacc.c:1646  */
    break;

  case 261:
#line 944 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3618 "ldgram.c" /* yacc.c:1646  */
    break;

  case 262:
#line 946 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay ((yyvsp[-11].etree), (int) (yyvsp[-12].integer),
					      (yyvsp[0].fill), (yyvsp[-3].name), (yyvsp[-1].section_phdr), (yyvsp[-2].name));
			}
#line 3628 "ldgram.c" /* yacc.c:1646  */
    break;

  case 264:
#line 956 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3634 "ldgram.c" /* yacc.c:1646  */
    break;

  case 265:
#line 958 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  lang_add_assignment (exp_assop ('=', ".", (yyvsp[0].etree)));
		}
#line 3643 "ldgram.c" /* yacc.c:1646  */
    break;

  case 267:
#line 966 "ldgram.y" /* yacc.c:1646  */
    { sectype = noload_section; }
#line 3649 "ldgram.c" /* yacc.c:1646  */
    break;

  case 268:
#line 967 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 3655 "ldgram.c" /* yacc.c:1646  */
    break;

  case 269:
#line 968 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 3661 "ldgram.c" /* yacc.c:1646  */
    break;

  case 270:
#line 969 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 3667 "ldgram.c" /* yacc.c:1646  */
    break;

  case 271:
#line 970 "ldgram.y" /* yacc.c:1646  */
    { sectype = noalloc_section; }
#line 3673 "ldgram.c" /* yacc.c:1646  */
    break;

  case 273:
#line 975 "ldgram.y" /* yacc.c:1646  */
    { sectype = normal_section; }
#line 3679 "ldgram.c" /* yacc.c:1646  */
    break;

  case 274:
#line 976 "ldgram.y" /* yacc.c:1646  */
    { sectype = normal_section; }
#line 3685 "ldgram.c" /* yacc.c:1646  */
    break;

  case 275:
#line 980 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-2].etree); }
#line 3691 "ldgram.c" /* yacc.c:1646  */
    break;

  case 276:
#line 981 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (etree_type *)NULL;  }
#line 3697 "ldgram.c" /* yacc.c:1646  */
    break;

  case 277:
#line 986 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-3].etree); }
#line 3703 "ldgram.c" /* yacc.c:1646  */
    break;

  case 278:
#line 988 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-7].etree); }
#line 3709 "ldgram.c" /* yacc.c:1646  */
    break;

  case 279:
#line 992 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (yyvsp[-1].etree); }
#line 3715 "ldgram.c" /* yacc.c:1646  */
    break;

  case 280:
#line 993 "ldgram.y" /* yacc.c:1646  */
    { (yyval.etree) = (etree_type *) NULL;  }
#line 3721 "ldgram.c" /* yacc.c:1646  */
    break;

  case 281:
#line 998 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = 0; }
#line 3727 "ldgram.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1000 "ldgram.y" /* yacc.c:1646  */
    { (yyval.integer) = 1; }
#line 3733 "ldgram.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1005 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = (yyvsp[0].name); }
#line 3739 "ldgram.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1006 "ldgram.y" /* yacc.c:1646  */
    { (yyval.name) = DEFAULT_MEMORY_REGION; }
#line 3745 "ldgram.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1011 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.section_phdr) = NULL;
		}
#line 3753 "ldgram.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1015 "ldgram.y" /* yacc.c:1646  */
    {
		  struct lang_output_section_phdr_list *n;

		  n = ((struct lang_output_section_phdr_list *)
		       xmalloc (sizeof *n));
		  n->name = (yyvsp[0].name);
		  n->used = FALSE;
		  n->next = (yyvsp[-2].section_phdr);
		  (yyval.section_phdr) = n;
		}
#line 3768 "ldgram.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1031 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_script ();
			  lang_enter_overlay_section ((yyvsp[0].name));
			}
#line 3777 "ldgram.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1036 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); ldlex_expression (); }
#line 3783 "ldgram.c" /* yacc.c:1646  */
    break;

  case 290:
#line 1038 "ldgram.y" /* yacc.c:1646  */
    {
			  ldlex_popstate ();
			  lang_leave_overlay_section ((yyvsp[0].fill), (yyvsp[-1].section_phdr));
			}
#line 3792 "ldgram.c" /* yacc.c:1646  */
    break;

  case 295:
#line 1055 "ldgram.y" /* yacc.c:1646  */
    { ldlex_expression (); }
#line 3798 "ldgram.c" /* yacc.c:1646  */
    break;

  case 296:
#line 1056 "ldgram.y" /* yacc.c:1646  */
    { ldlex_popstate (); }
#line 3804 "ldgram.c" /* yacc.c:1646  */
    break;

  case 297:
#line 1058 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_new_phdr ((yyvsp[-5].name), (yyvsp[-3].etree), (yyvsp[-2].phdr).filehdr, (yyvsp[-2].phdr).phdrs, (yyvsp[-2].phdr).at,
				 (yyvsp[-2].phdr).flags);
		}
#line 3813 "ldgram.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1066 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.etree) = (yyvsp[0].etree);

		  if ((yyvsp[0].etree)->type.node_class == etree_name
		      && (yyvsp[0].etree)->type.node_code == NAME)
		    {
		      const char *s;
		      unsigned int i;
		      static const char * const phdr_types[] =
			{
			  "PT_NULL", "PT_LOAD", "PT_DYNAMIC",
			  "PT_INTERP", "PT_NOTE", "PT_SHLIB",
			  "PT_PHDR", "PT_TLS"
			};

		      s = (yyvsp[0].etree)->name.name;
		      for (i = 0;
			   i < sizeof phdr_types / sizeof phdr_types[0];
			   i++)
			if (strcmp (s, phdr_types[i]) == 0)
			  {
			    (yyval.etree) = exp_intop (i);
			    break;
			  }
		      if (i == sizeof phdr_types / sizeof phdr_types[0])
			{
			  if (strcmp (s, "PT_GNU_EH_FRAME") == 0)
			    (yyval.etree) = exp_intop (0x6474e550);
			  else if (strcmp (s, "PT_GNU_STACK") == 0)
			    (yyval.etree) = exp_intop (0x6474e551);
			  else
			    {
			      einfo (_("\
%X%P:%S: unknown phdr type `%s' (try integer literal)\n"),
				     s);
			      (yyval.etree) = exp_intop (0);
			    }
			}
		    }
		}
#line 3858 "ldgram.c" /* yacc.c:1646  */
    break;

  case 299:
#line 1110 "ldgram.y" /* yacc.c:1646  */
    {
		  memset (&(yyval.phdr), 0, sizeof (struct phdr_info));
		}
#line 3866 "ldgram.c" /* yacc.c:1646  */
    break;

  case 300:
#line 1114 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  if (strcmp ((yyvsp[-2].name), "FILEHDR") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).filehdr = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "PHDRS") == 0 && (yyvsp[-1].etree) == NULL)
		    (yyval.phdr).phdrs = TRUE;
		  else if (strcmp ((yyvsp[-2].name), "FLAGS") == 0 && (yyvsp[-1].etree) != NULL)
		    (yyval.phdr).flags = (yyvsp[-1].etree);
		  else
		    einfo (_("%X%P:%S: PHDRS syntax error at `%s'\n"), (yyvsp[-2].name));
		}
#line 3882 "ldgram.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1126 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.phdr) = (yyvsp[0].phdr);
		  (yyval.phdr).at = (yyvsp[-2].etree);
		}
#line 3891 "ldgram.c" /* yacc.c:1646  */
    break;

  case 302:
#line 1134 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.etree) = NULL;
		}
#line 3899 "ldgram.c" /* yacc.c:1646  */
    break;

  case 303:
#line 1138 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.etree) = (yyvsp[-1].etree);
		}
#line 3907 "ldgram.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1144 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("dynamic list"));
		}
#line 3916 "ldgram.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1149 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 3925 "ldgram.c" /* yacc.c:1646  */
    break;

  case 309:
#line 1166 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_append_dynamic_list ((yyvsp[-1].versyms));
		}
#line 3933 "ldgram.c" /* yacc.c:1646  */
    break;

  case 310:
#line 1174 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_file ();
		  PUSH_ERROR (_("VERSION script"));
		}
#line 3942 "ldgram.c" /* yacc.c:1646  */
    break;

  case 311:
#line 1179 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		  POP_ERROR ();
		}
#line 3951 "ldgram.c" /* yacc.c:1646  */
    break;

  case 312:
#line 1188 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_version_script ();
		}
#line 3959 "ldgram.c" /* yacc.c:1646  */
    break;

  case 313:
#line 1192 "ldgram.y" /* yacc.c:1646  */
    {
		  ldlex_popstate ();
		}
#line 3967 "ldgram.c" /* yacc.c:1646  */
    break;

  case 316:
#line 1204 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node (NULL, (yyvsp[-2].versnode), NULL);
		}
#line 3975 "ldgram.c" /* yacc.c:1646  */
    break;

  case 317:
#line 1208 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node ((yyvsp[-4].name), (yyvsp[-2].versnode), NULL);
		}
#line 3983 "ldgram.c" /* yacc.c:1646  */
    break;

  case 318:
#line 1212 "ldgram.y" /* yacc.c:1646  */
    {
		  lang_register_vers_node ((yyvsp[-5].name), (yyvsp[-3].versnode), (yyvsp[-1].deflist));
		}
#line 3991 "ldgram.c" /* yacc.c:1646  */
    break;

  case 319:
#line 1219 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.deflist) = lang_add_vers_depend (NULL, (yyvsp[0].name));
		}
#line 3999 "ldgram.c" /* yacc.c:1646  */
    break;

  case 320:
#line 1223 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.deflist) = lang_add_vers_depend ((yyvsp[-1].deflist), (yyvsp[0].name));
		}
#line 4007 "ldgram.c" /* yacc.c:1646  */
    break;

  case 321:
#line 1230 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, NULL);
		}
#line 4015 "ldgram.c" /* yacc.c:1646  */
    break;

  case 322:
#line 1234 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4023 "ldgram.c" /* yacc.c:1646  */
    break;

  case 323:
#line 1238 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-1].versyms), NULL);
		}
#line 4031 "ldgram.c" /* yacc.c:1646  */
    break;

  case 324:
#line 1242 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node (NULL, (yyvsp[-1].versyms));
		}
#line 4039 "ldgram.c" /* yacc.c:1646  */
    break;

  case 325:
#line 1246 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versnode) = lang_new_vers_node ((yyvsp[-5].versyms), (yyvsp[-1].versyms));
		}
#line 4047 "ldgram.c" /* yacc.c:1646  */
    break;

  case 326:
#line 1253 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4055 "ldgram.c" /* yacc.c:1646  */
    break;

  case 327:
#line 1257 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4063 "ldgram.c" /* yacc.c:1646  */
    break;

  case 328:
#line 1261 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, FALSE);
		}
#line 4071 "ldgram.c" /* yacc.c:1646  */
    break;

  case 329:
#line 1265 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), (yyvsp[0].name), ldgram_vers_current_lang, TRUE);
		}
#line 4079 "ldgram.c" /* yacc.c:1646  */
    break;

  case 330:
#line 1269 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4088 "ldgram.c" /* yacc.c:1646  */
    break;

  case 331:
#line 1274 "ldgram.y" /* yacc.c:1646  */
    {
			  struct bfd_elf_version_expr *pat;
			  for (pat = (yyvsp[-2].versyms); pat->next != NULL; pat = pat->next);
			  pat->next = (yyvsp[-8].versyms);
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4100 "ldgram.c" /* yacc.c:1646  */
    break;

  case 332:
#line 1282 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.name) = ldgram_vers_current_lang;
			  ldgram_vers_current_lang = (yyvsp[-1].name);
			}
#line 4109 "ldgram.c" /* yacc.c:1646  */
    break;

  case 333:
#line 1287 "ldgram.y" /* yacc.c:1646  */
    {
			  (yyval.versyms) = (yyvsp[-2].versyms);
			  ldgram_vers_current_lang = (yyvsp[-3].name);
			}
#line 4118 "ldgram.c" /* yacc.c:1646  */
    break;

  case 334:
#line 1292 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "global", ldgram_vers_current_lang, FALSE);
		}
#line 4126 "ldgram.c" /* yacc.c:1646  */
    break;

  case 335:
#line 1296 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "global", ldgram_vers_current_lang, FALSE);
		}
#line 4134 "ldgram.c" /* yacc.c:1646  */
    break;

  case 336:
#line 1300 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "local", ldgram_vers_current_lang, FALSE);
		}
#line 4142 "ldgram.c" /* yacc.c:1646  */
    break;

  case 337:
#line 1304 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "local", ldgram_vers_current_lang, FALSE);
		}
#line 4150 "ldgram.c" /* yacc.c:1646  */
    break;

  case 338:
#line 1308 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern (NULL, "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4158 "ldgram.c" /* yacc.c:1646  */
    break;

  case 339:
#line 1312 "ldgram.y" /* yacc.c:1646  */
    {
		  (yyval.versyms) = lang_new_vers_pattern ((yyvsp[-2].versyms), "extern", ldgram_vers_current_lang, FALSE);
		}
#line 4166 "ldgram.c" /* yacc.c:1646  */
    break;


#line 4170 "ldgram.c" /* yacc.c:1646  */
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

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
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
        yyerror (yymsgp);
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
                      yytoken, &yylval);
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

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
                  yystos[yystate], yyvsp);
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
  yyerror (YY_("memory exhausted"));
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
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
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
  return yyresult;
}
#line 1322 "ldgram.y" /* yacc.c:1906  */

void
yyerror(arg)
     const char *arg;
{
  if (ldfile_assumed_script)
    einfo (_("%P:%s: file format not recognized; treating as linker script\n"),
	   ldfile_input_filename);
  if (error_index > 0 && error_index < ERROR_NAME_MAX)
     einfo ("%P%F:%S: %s in %s\n", arg, error_names[error_index-1]);
  else
     einfo ("%P%F:%S: %s\n", arg);
}
