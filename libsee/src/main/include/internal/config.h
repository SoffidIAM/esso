/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef _SEE_h_config_
#define _SEE_h_config_

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have the `abort' function. */
#define HAVE_ABORT 1

/* Define to 1 if you have the `acos' function. */
#define HAVE_ACOS 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
/* #undef HAVE_ALLOCA_H */

/* Define to 1 if you have the `atan' function. */
#define HAVE_ATAN 1

/* Define to 1 if you have the `atan2' function. */
#define HAVE_ATAN2 1

/* Define to 1 if you have the `ceil' function. */
#define HAVE_CEIL 1

/* Define if your compiler handles ANSI hex fp constants like '0x1p3' */
#define HAVE_CONSTANT_HEX_FLOAT 1

/* Define if your compiler treats the expression 1.0/0.0 as constant */
#define HAVE_CONSTANT_INF_DIV 1

/* Define if your compiler treats the expression 0.0/0.0 as constant */
/* #undef HAVE_CONSTANT_NAN_DIV */

/* Define to 1 if you have the `copysign' function. */
#ifdef __GNUC__
#define HAVE_COPYSIGN 1
#endif

/* Define to 1 if you have the `copysignf' function. */
#ifdef __GNUC__
#define HAVE_COPYSIGNF 1
#endif

/* Define to 1 if you have the `cos' function. */
#define HAVE_COS 1

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `dtoa' function. */
/* #undef HAVE_DTOA */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `exp' function. */
#define HAVE_EXP 1

/* Define to 1 if you have the `finite' function. */
#ifdef __GNUC__
#define HAVE_FINITE 1
#endif

/* Define to 1 if you have the `finitef' function. */
/* #undef HAVE_FINITEF */

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if you have the `freedtoa' function. */
/* #undef HAVE_FREEDTOA */

/* Define to 1 if you have the `GC_dump' function. */
/* #undef HAVE_GC_DUMP */

/* Define to 1 if you have the `GC_free' function. */
/* #undef HAVE_GC_FREE */

/* Define to 1 if you have the `GC_gcollect' function. */
/* #undef HAVE_GC_GCOLLECT */

/* Define to 1 if you have the <gc/gc.h> header file. */
/* #undef HAVE_GC_GC_H */

/* Define to 1 if you have the `GC_malloc' function. */
/* #undef HAVE_GC_MALLOC */

/* Define to 1 if you have the `GC_malloc_atomic' function. */
/* #undef HAVE_GC_MALLOC_ATOMIC */

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `GetSystemTimeAsFileTime' function. */
/* #undef HAVE_GETSYSTEMTIMEASFILETIME */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#ifdef __GNUC__
#define HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `isatty' function. */
#define HAVE_ISATTY 1

/* Define to 1 if you have the `isfinite' function. */
/* #undef HAVE_ISFINITE */

/* Define to 1 if you have the `isinf' function. */
/* #undef HAVE_ISINF */

/* Define to 1 if you have the `isinff' function. */
/* #undef HAVE_ISINFF */

/* Define to 1 if you have the `isnan' function. */
#ifdef __GNUC__
#define HAVE_ISNAN 1
#endif

/* Define to 1 if you have the `isnanf' function. */
#ifdef __GNUC__
#define HAVE_ISNANF 1
#endif

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `localtime' function. */
#define HAVE_LOCALTIME 1

/* Define to 1 if you have the `log' function. */
#define HAVE_LOG 1

/* Define if you have working longjmp() and setjmp() functions */
#define HAVE_LONGJMP 1

/* Define if you have a working memcmp() function */
#define HAVE_MEMCMP 1

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mktime' function. */
#define HAVE_MKTIME 1

/* Define to 1 if you have the `pcre_compile' function. */
/* #undef HAVE_PCRE_COMPILE */

/* Define to 1 if you have the `pcre_exec' function. */
/* #undef HAVE_PCRE_EXEC */

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 if you have the `qsort' function. */
#define HAVE_QSORT 1

/* Define to 1 if you have the <readline.h> header file. */
/* #undef HAVE_READLINE_H */

/* Define to 1 if you have the <readline/readline.h> header file. */
/* #undef HAVE_READLINE_READLINE_H */

/* Define to 1 if you have the <setjmp.h> header file. */
#define HAVE_SETJMP_H 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `sin' function. */
#define HAVE_SIN 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcmp' function. */
#define HAVE_STRCMP 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtod' function. */
#define HAVE_STRTOD 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `tan' function. */
#define HAVE_TAN 1

/* Define to 1 if you have the `time' function. */
#ifdef __GNUC__
#define HAVE_TIME 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if your preprocessor understands GNU-style variadic macros */
#ifdef __GNUC__
#define HAVE_VARIADIC_MACROS 1
#endif

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the `_copysign' function. */
#define HAVE__COPYSIGN 1

/* Define to 1 if you have the `_finite' function. */
#define HAVE__FINITE 1

/* Define to 1 if you have the `_isinf' function. */
/* #undef HAVE__ISINF */

/* Define to 1 if you have the `_isnan' function. */
#define HAVE__ISNAN 1

/* Define if you have working _longjmp() and _setjmp() functions */
/* #undef HAVE__LONGJMP */

/* Define to 1 if the compiler generates __FUNCTION__ */
#define HAVE___FUNCTION__ 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "see"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "leonard@users.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "see"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "see 3.1.1424"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "see"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.1.1424"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT 4

/* The size of `signed int', as computed by sizeof. */
#define SIZEOF_SIGNED_INT 4

/* The size of `signed long', as computed by sizeof. */
#define SIZEOF_SIGNED_LONG 4

/* The size of `signed LONGLONG', as computed by sizeof. */
#define SIZEOF_SIGNED_LONGLONG 0

/* The size of `signed long long', as computed by sizeof. */
#define SIZEOF_SIGNED_LONG_LONG 8

/* The size of `signed short', as computed by sizeof. */
#define SIZEOF_SIGNED_SHORT 2

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* The size of `unsigned LONGLONG', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONGLONG 0

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "3.1.1424"

/* Define if you have Boehm GC */
/* #undef WITH_BOEHM_GC */

/* Define if you want SEE to catch longjmp corruption */
#define WITH_LONGJMPERROR 1

/* Define if you want to use the code generator */
#define WITH_PARSER_CODEGEN 1

/* Define if you want to use the stable AST evaluator */
/* #undef WITH_PARSER_EVAL */

/* Define if you want to be able to print function bodies */
#define WITH_PARSER_PRINT 1

/* Define if you want to include experimental AST visitor code */
/* #undef WITH_PARSER_VISIT */

/* Define if you want to use the PCRE regex library */
/* #undef WITH_PCRE */

/* Define to 1 if you want the Unicode tables for ECMA262 compliance */
#define WITH_UNICODE_TABLES 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

#define NDEBUG 1

#endif /* _SEE_h_config_ */
