/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 37 "D:/laby/src/wc/wc.y"

#include <stdio.h>
#include "wc.h"

#define YYSTYPE WcValue
#define yylval wclval
#define yylex WcNextToken
#define yyparse WcParse
#define yyerror wcerror

void wcerror(const char *msg);

#line 84 "D:/laby/src/wc/wcparse.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 284,                 /* "invalid token"  */
    WORLD = 257,                   /* WORLD  */
    RESERVED_258 = 258,            /* RESERVED_258  */
    RESERVED_259 = 259,            /* RESERVED_259  */
    OBJECT = 260,                  /* OBJECT  */
    POLY = 261,                    /* POLY  */
    VERTEX = 262,                  /* VERTEX  */
    COLOR = 263,                   /* COLOR  */
    MATERIAL = 264,                /* MATERIAL  */
    TEXTURE = 265,                 /* TEXTURE  */
    SHAPEFILE = 266,               /* SHAPEFILE  */
    CAMERA = 267,                  /* CAMERA  */
    FIELD = 268,                   /* FIELD  */
    NORMAL = 269,                  /* NORMAL  */
    URL = 270,                     /* URL  */
    SCENE = 271,                   /* SCENE  */
    ELEMENT = 272,                 /* ELEMENT  */
    LIGHT = 273,                   /* LIGHT  */
    POSITION = 274,                /* POSITION  */
    ORIENTATION = 275,             /* ORIENTATION  */
    OPEN = 276,                    /* OPEN  */
    CLOSE = 277,                   /* CLOSE  */
    BEGIN_ = 278,                  /* BEGIN_  */
    END_ = 279,                    /* END_  */
    IDENTIFIER = 280,              /* IDENTIFIER  */
    INTEGER = 281,                 /* INTEGER  */
    DOUBLE = 282,                  /* DOUBLE  */
    STRING = 283                   /* STRING  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 284
#define WORLD 257
#define RESERVED_258 258
#define RESERVED_259 259
#define OBJECT 260
#define POLY 261
#define VERTEX 262
#define COLOR 263
#define MATERIAL 264
#define TEXTURE 265
#define SHAPEFILE 266
#define CAMERA 267
#define FIELD 268
#define NORMAL 269
#define URL 270
#define SCENE 271
#define ELEMENT 272
#define LIGHT 273
#define POSITION 274
#define ORIENTATION 275
#define OPEN 276
#define CLOSE 277
#define BEGIN_ 278
#define END_ 279
#define IDENTIFIER 280
#define INTEGER 281
#define DOUBLE 282
#define STRING 283

/* Value type.  */


extern YYSTYPE yylval;


int yyparse (void);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_WORLD = 3,                      /* WORLD  */
  YYSYMBOL_RESERVED_258 = 4,               /* RESERVED_258  */
  YYSYMBOL_RESERVED_259 = 5,               /* RESERVED_259  */
  YYSYMBOL_OBJECT = 6,                     /* OBJECT  */
  YYSYMBOL_POLY = 7,                       /* POLY  */
  YYSYMBOL_VERTEX = 8,                     /* VERTEX  */
  YYSYMBOL_COLOR = 9,                      /* COLOR  */
  YYSYMBOL_MATERIAL = 10,                  /* MATERIAL  */
  YYSYMBOL_TEXTURE = 11,                   /* TEXTURE  */
  YYSYMBOL_SHAPEFILE = 12,                 /* SHAPEFILE  */
  YYSYMBOL_CAMERA = 13,                    /* CAMERA  */
  YYSYMBOL_FIELD = 14,                     /* FIELD  */
  YYSYMBOL_NORMAL = 15,                    /* NORMAL  */
  YYSYMBOL_URL = 16,                       /* URL  */
  YYSYMBOL_SCENE = 17,                     /* SCENE  */
  YYSYMBOL_ELEMENT = 18,                   /* ELEMENT  */
  YYSYMBOL_LIGHT = 19,                     /* LIGHT  */
  YYSYMBOL_POSITION = 20,                  /* POSITION  */
  YYSYMBOL_ORIENTATION = 21,               /* ORIENTATION  */
  YYSYMBOL_OPEN = 22,                      /* OPEN  */
  YYSYMBOL_CLOSE = 23,                     /* CLOSE  */
  YYSYMBOL_BEGIN_ = 24,                    /* BEGIN_  */
  YYSYMBOL_END_ = 25,                      /* END_  */
  YYSYMBOL_IDENTIFIER = 26,                /* IDENTIFIER  */
  YYSYMBOL_INTEGER = 27,                   /* INTEGER  */
  YYSYMBOL_DOUBLE = 28,                    /* DOUBLE  */
  YYSYMBOL_STRING = 29,                    /* STRING  */
  YYSYMBOL_30_ = 30,                       /* '{'  */
  YYSYMBOL_31_ = 31,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 32,                  /* $accept  */
  YYSYMBOL_world = 33,                     /* world  */
  YYSYMBOL_34_1 = 34,                      /* $@1  */
  YYSYMBOL_world_items = 35,               /* world_items  */
  YYSYMBOL_world_item = 36,                /* world_item  */
  YYSYMBOL_camera = 37,                    /* camera  */
  YYSYMBOL_camera_position = 38,           /* camera_position  */
  YYSYMBOL_camera_orientation = 39,        /* camera_orientation  */
  YYSYMBOL_camera_field = 40,              /* camera_field  */
  YYSYMBOL_object = 41,                    /* object  */
  YYSYMBOL_42_2 = 42,                      /* $@2  */
  YYSYMBOL_object_items = 43,              /* object_items  */
  YYSYMBOL_object_item = 44,               /* object_item  */
  YYSYMBOL_geometry = 45,                  /* geometry  */
  YYSYMBOL_poly = 46,                      /* poly  */
  YYSYMBOL_47_3 = 47,                      /* $@3  */
  YYSYMBOL_poly_items = 48,                /* poly_items  */
  YYSYMBOL_poly_item = 49,                 /* poly_item  */
  YYSYMBOL_vertex = 50,                    /* vertex  */
  YYSYMBOL_attribute = 51,                 /* attribute  */
  YYSYMBOL_normal = 52,                    /* normal  */
  YYSYMBOL_object_url = 53,                /* object_url  */
  YYSYMBOL_shapefile = 54,                 /* shapefile  */
  YYSYMBOL_scene = 55,                     /* scene  */
  YYSYMBOL_56_4 = 56,                      /* $@4  */
  YYSYMBOL_scene_items = 57,               /* scene_items  */
  YYSYMBOL_scene_item = 58,                /* scene_item  */
  YYSYMBOL_element = 59,                   /* element  */
  YYSYMBOL_60_5 = 60,                      /* $@5  */
  YYSYMBOL_element_items = 61,             /* element_items  */
  YYSYMBOL_element_item = 62,              /* element_item  */
  YYSYMBOL_light = 63,                     /* light  */
  YYSYMBOL_64_6 = 64,                      /* $@6  */
  YYSYMBOL_light_items = 65,               /* light_items  */
  YYSYMBOL_light_item = 66,                /* light_item  */
  YYSYMBOL_name = 67,                      /* name  */
  YYSYMBOL_open = 68,                      /* open  */
  YYSYMBOL_close = 69,                     /* close  */
  YYSYMBOL_number = 70                     /* number  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   151

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  32
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  66
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  143

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   284


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
       2,     2,     2,    30,     2,    31,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    65,    65,    64,    71,    72,    76,    77,    78,    82,
      86,    91,    97,   103,   102,   109,   110,   114,   115,   116,
     120,   121,   126,   125,   132,   133,   137,   138,   139,   143,
     148,   150,   152,   157,   162,   167,   173,   172,   179,   180,
     184,   185,   190,   189,   196,   197,   201,   203,   205,   208,
     214,   213,   220,   221,   225,   227,   229,   236,   237,   242,
     243,   244,   248,   249,   250,   254,   255
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "WORLD",
  "RESERVED_258", "RESERVED_259", "OBJECT", "POLY", "VERTEX", "COLOR",
  "MATERIAL", "TEXTURE", "SHAPEFILE", "CAMERA", "FIELD", "NORMAL", "URL",
  "SCENE", "ELEMENT", "LIGHT", "POSITION", "ORIENTATION", "OPEN", "CLOSE",
  "BEGIN_", "END_", "IDENTIFIER", "INTEGER", "DOUBLE", "STRING", "'{'",
  "'}'", "$accept", "world", "$@1", "world_items", "world_item", "camera",
  "camera_position", "camera_orientation", "camera_field", "object", "$@2",
  "object_items", "object_item", "geometry", "poly", "$@3", "poly_items",
  "poly_item", "vertex", "attribute", "normal", "object_url", "shapefile",
  "scene", "$@4", "scene_items", "scene_item", "element", "$@5",
  "element_items", "element_item", "light", "$@6", "light_items",
  "light_item", "name", "open", "close", "number", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-34)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
       5,    -2,    41,   -34,   -20,   -34,   -34,   -34,   -34,   -34,
     -34,    20,    -2,   -20,    -2,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -34,   -20,    22,   -20,   -34,   -12,    29,   -34,
     -34,   -34,   -34,   -12,   -12,    38,   -34,   110,   -12,   -12,
     -12,     7,   120,   -20,   -12,   -12,    24,    26,    28,   -34,
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   -12,   -12,   -34,
     -20,   -20,   -34,   -34,   -34,   -34,   -34,   -12,   -12,   -12,
     -34,   -34,   -12,   -12,   -34,   -34,   -34,   -12,   -34,   -12,
     -12,   -12,   -34,   -34,   119,   -34,    34,   -12,   -34,    93,
      -3,   -12,   -12,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
      35,    36,   -12,   -12,   -34,   -34,    39,   -12,   -12,   -34,
     -34,   -12,   -12,   -34,   -34,   -12,   -12,    37,   -12,   -12,
     -12,   -12,   -12,   -12,    62,   -12,   -12,   -34,   -34,   -34,
     -12,   -12,   -34,   -12,   -12,   -12,   -12,   -12,   -12,   -12,
     -34,   -34,   -34
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    57,     0,    58,     0,     1,    59,    60,    61,     2,
       4,     0,    57,     0,    57,    62,    63,    64,     5,     8,
       6,     7,     3,     0,     0,     0,    13,     0,     0,    36,
      15,    65,    66,     0,     0,     0,    38,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    16,
      17,    20,    18,    19,    21,    14,    10,     0,     0,     9,
       0,     0,    39,    40,    41,    37,    22,     0,     0,     0,
      35,    34,     0,     0,    42,    50,    24,     0,    31,     0,
       0,     0,    44,    52,     0,    30,     0,     0,    12,     0,
       0,     0,     0,    25,    26,    27,    28,    23,    32,    11,
       0,     0,     0,     0,    45,    43,     0,     0,     0,    53,
      51,     0,     0,    46,    49,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    29,    33,    47,
       0,     0,    55,     0,     0,     0,     0,     0,     0,     0,
      48,    54,    56
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -11,
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,   -34,
     -34,   -34,   -34,   -34,   -34,     9,     6,   -28,   -33
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     2,    10,    11,    18,    19,    28,    35,    41,    20,
      30,    37,    49,    50,    51,    76,    84,    93,    94,    52,
      96,    53,    54,    21,    36,    42,    62,    63,    82,    89,
     104,    64,    83,    90,   109,     4,     9,    22,    33
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      38,    39,     6,   106,     7,    56,    57,    58,     1,    55,
       8,    67,    68,    59,    65,    31,    32,   107,   108,    24,
      15,    23,    16,    25,    72,    73,    12,     3,    17,    26,
      15,    29,    16,    13,    77,    78,    79,    14,    17,    80,
      81,     5,    27,    15,    85,    16,    86,    87,    88,    66,
      34,    17,    40,    69,    99,    70,    97,    71,   111,   112,
      98,   105,   110,   124,   113,   114,    74,    75,   117,   115,
     116,   131,     0,    95,   118,   119,     0,     0,   120,   121,
       0,     0,   122,   123,     0,   125,   126,   127,   128,   129,
     130,     0,   132,   133,     0,     0,     0,   134,   135,   100,
     136,   137,   138,   139,   140,   141,   142,     0,     0,   101,
       0,     0,     0,   102,   103,     0,    15,    43,    16,    44,
      45,    46,    47,     0,    17,     0,    48,    91,    44,    45,
      46,     0,     0,    15,    92,    16,     0,     0,    60,    61,
       0,    17,    15,    15,    16,    16,     0,     0,     0,     0,
      17,    17
};

static const yytype_int16 yycheck[] =
{
      33,    34,    22,     6,    24,    38,    39,    40,     3,    37,
      30,    44,    45,    41,    42,    27,    28,    20,    21,    13,
      23,    12,    25,    14,    57,    58,     6,    29,    31,    23,
      23,    25,    25,    13,    67,    68,    69,    17,    31,    72,
      73,     0,    20,    23,    77,    25,    79,    80,    81,    43,
      21,    31,    14,    29,    87,    29,    84,    29,    91,    92,
      26,    89,    90,    26,    29,    29,    60,    61,    29,   102,
     103,     9,    -1,    84,   107,   108,    -1,    -1,   111,   112,
      -1,    -1,   115,   116,    -1,   118,   119,   120,   121,   122,
     123,    -1,   125,   126,    -1,    -1,    -1,   130,   131,     6,
     133,   134,   135,   136,   137,   138,   139,    -1,    -1,    16,
      -1,    -1,    -1,    20,    21,    -1,    23,     7,    25,     9,
      10,    11,    12,    -1,    31,    -1,    16,     8,     9,    10,
      11,    -1,    -1,    23,    15,    25,    -1,    -1,    18,    19,
      -1,    31,    23,    23,    25,    25,    -1,    -1,    -1,    -1,
      31,    31
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,    33,    29,    67,     0,    22,    24,    30,    68,
      34,    35,     6,    13,    17,    23,    25,    31,    36,    37,
      41,    55,    69,    67,    68,    67,    68,    20,    38,    68,
      42,    27,    28,    70,    21,    39,    56,    43,    70,    70,
      14,    40,    57,     7,     9,    10,    11,    12,    16,    44,
      45,    46,    51,    53,    54,    69,    70,    70,    70,    69,
      18,    19,    58,    59,    63,    69,    68,    70,    70,    29,
      29,    29,    70,    70,    68,    68,    47,    70,    70,    70,
      70,    70,    60,    64,    48,    70,    70,    70,    70,    61,
      65,     8,    15,    49,    50,    51,    52,    69,    26,    70,
       6,    16,    20,    21,    62,    69,     6,    20,    21,    66,
      69,    70,    70,    29,    29,    70,    70,    29,    70,    70,
      70,    70,    70,    70,    26,    70,    70,    70,    70,    70,
      70,     9,    70,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    70
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    32,    34,    33,    35,    35,    36,    36,    36,    37,
      38,    39,    40,    42,    41,    43,    43,    44,    44,    44,
      45,    45,    47,    46,    48,    48,    49,    49,    49,    50,
      51,    51,    51,    52,    53,    54,    56,    55,    57,    57,
      58,    58,    60,    59,    61,    61,    62,    62,    62,    62,
      64,    63,    65,    65,    66,    66,    66,    67,    67,    68,
      68,    68,    69,    69,    69,    70,    70
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     6,     0,     2,     1,     1,     1,     6,
       4,     7,     5,     0,     6,     0,     2,     1,     1,     1,
       1,     1,     0,     5,     0,     2,     1,     1,     1,     4,
       4,     3,     5,     4,     2,     2,     0,     6,     0,     2,
       1,     1,     0,     5,     0,     2,     2,     4,     7,     2,
       0,     5,     0,     2,     7,     4,     7,     0,     1,     1,
       1,     1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 2: /* $@1: %empty  */
#line 65 "D:/laby/src/wc/wc.y"
                { WcBeginWorld((yyvsp[-1].sval), (yyvsp[0].ival)); }
#line 1326 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 3: /* world: WORLD name open $@1 world_items close  */
#line 67 "D:/laby/src/wc/wc.y"
                { WcEndWorld((yyvsp[0].ival)); YYACCEPT; }
#line 1332 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 10: /* camera_position: POSITION number number number  */
#line 87 "D:/laby/src/wc/wc.y"
                { WcSetCameraPosition((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1338 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 11: /* camera_orientation: ORIENTATION number number number number number number  */
#line 92 "D:/laby/src/wc/wc.y"
                { WcSetCameraOrientation((yyvsp[-5].dval), (yyvsp[-4].dval), (yyvsp[-3].dval),
                                         (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1345 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 12: /* camera_field: FIELD number number number number  */
#line 98 "D:/laby/src/wc/wc.y"
                { WcSetCameraField((yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1351 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 13: /* $@2: %empty  */
#line 103 "D:/laby/src/wc/wc.y"
                { WcBeginObject((yyvsp[-1].sval), (yyvsp[0].ival)); }
#line 1357 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 14: /* object: OBJECT name open $@2 object_items close  */
#line 105 "D:/laby/src/wc/wc.y"
                { WcEndObject((yyvsp[0].ival)); }
#line 1363 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 22: /* $@3: %empty  */
#line 126 "D:/laby/src/wc/wc.y"
                { WcBeginPoly((yyvsp[0].ival)); }
#line 1369 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 23: /* poly: POLY open $@3 poly_items close  */
#line 128 "D:/laby/src/wc/wc.y"
                { WcEndPoly((yyvsp[0].ival)); }
#line 1375 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 29: /* vertex: VERTEX number number number  */
#line 144 "D:/laby/src/wc/wc.y"
                { WcAddVertex((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1381 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 30: /* attribute: COLOR number number number  */
#line 149 "D:/laby/src/wc/wc.y"
                { WcSetColor((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1387 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 31: /* attribute: MATERIAL number number  */
#line 151 "D:/laby/src/wc/wc.y"
                { WcSetMaterial((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1393 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 32: /* attribute: TEXTURE STRING number number IDENTIFIER  */
#line 153 "D:/laby/src/wc/wc.y"
                { WcSetTexture((yyvsp[-3].sval), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].sval)); }
#line 1399 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 33: /* normal: NORMAL number number number  */
#line 158 "D:/laby/src/wc/wc.y"
                { WcSetNormal((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1405 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 34: /* object_url: URL STRING  */
#line 163 "D:/laby/src/wc/wc.y"
                { WcSetObjectURL((yyvsp[0].sval)); }
#line 1411 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 35: /* shapefile: SHAPEFILE STRING  */
#line 168 "D:/laby/src/wc/wc.y"
                { WcSetShapeURL((yyvsp[0].sval)); }
#line 1417 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 36: /* $@4: %empty  */
#line 173 "D:/laby/src/wc/wc.y"
                { WcBeginScene((yyvsp[-1].sval), (yyvsp[0].ival)); }
#line 1423 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 37: /* scene: SCENE name open $@4 scene_items close  */
#line 175 "D:/laby/src/wc/wc.y"
                { WcEndScene((yyvsp[0].ival)); }
#line 1429 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 42: /* $@5: %empty  */
#line 190 "D:/laby/src/wc/wc.y"
                { WcBeginElement((yyvsp[0].ival)); }
#line 1435 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 43: /* element: ELEMENT open $@5 element_items close  */
#line 192 "D:/laby/src/wc/wc.y"
                { WcEndElement((yyvsp[0].ival)); }
#line 1441 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 46: /* element_item: OBJECT STRING  */
#line 202 "D:/laby/src/wc/wc.y"
                { WcSetElementObject((yyvsp[0].sval)); }
#line 1447 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 47: /* element_item: POSITION number number number  */
#line 204 "D:/laby/src/wc/wc.y"
                { WcSetElementPosition((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1453 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 48: /* element_item: ORIENTATION number number number number number number  */
#line 206 "D:/laby/src/wc/wc.y"
                { WcSetElementOrientation((yyvsp[-5].dval), (yyvsp[-4].dval), (yyvsp[-3].dval),
                                          (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1460 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 49: /* element_item: URL STRING  */
#line 209 "D:/laby/src/wc/wc.y"
                { WcSetElementURL((yyvsp[0].sval)); }
#line 1466 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 50: /* $@6: %empty  */
#line 214 "D:/laby/src/wc/wc.y"
                { WcBeginLight((yyvsp[0].ival)); }
#line 1472 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 51: /* light: LIGHT open $@6 light_items close  */
#line 216 "D:/laby/src/wc/wc.y"
                { WcEndLight((yyvsp[0].ival)); }
#line 1478 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 54: /* light_item: OBJECT STRING IDENTIFIER COLOR number number number  */
#line 226 "D:/laby/src/wc/wc.y"
                { WcSetLightObject((yyvsp[-5].sval), (yyvsp[-4].sval), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1484 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 55: /* light_item: POSITION number number number  */
#line 228 "D:/laby/src/wc/wc.y"
                { WcSetLightPosition((yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1490 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 56: /* light_item: ORIENTATION number number number number number number  */
#line 230 "D:/laby/src/wc/wc.y"
                { WcSetLightOrientation((yyvsp[-5].dval), (yyvsp[-4].dval), (yyvsp[-3].dval),
                                        (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 1497 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 57: /* name: %empty  */
#line 236 "D:/laby/src/wc/wc.y"
                { (yyval.sval) = (char *)""; }
#line 1503 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 58: /* name: STRING  */
#line 238 "D:/laby/src/wc/wc.y"
                { (yyval.sval) = (yyvsp[0].sval); }
#line 1509 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 59: /* open: OPEN  */
#line 242 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1515 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 60: /* open: BEGIN_  */
#line 243 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1521 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 61: /* open: '{'  */
#line 244 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1527 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 62: /* close: CLOSE  */
#line 248 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1533 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 63: /* close: END_  */
#line 249 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1539 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 64: /* close: '}'  */
#line 250 "D:/laby/src/wc/wc.y"
                        { (yyval.ival) = (yyvsp[0].ival); }
#line 1545 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 65: /* number: INTEGER  */
#line 254 "D:/laby/src/wc/wc.y"
                        { (yyval.dval) = (double)(yyvsp[0].ival); }
#line 1551 "D:/laby/src/wc/wcparse.cpp"
    break;

  case 66: /* number: DOUBLE  */
#line 255 "D:/laby/src/wc/wc.y"
                        { (yyval.dval) = (yyvsp[0].dval); }
#line 1557 "D:/laby/src/wc/wcparse.cpp"
    break;


#line 1561 "D:/laby/src/wc/wcparse.cpp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 258 "D:/laby/src/wc/wc.y"

