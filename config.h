/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Enable Altivec backend */
#define ENABLE_BACKEND_ALTIVEC 0

/* Enable Arm backend */
#define ENABLE_BACKEND_ARM 1

/* Enable c64x backend */
#define ENABLE_BACKEND_C64X 0

/* Enable MMX backend */
#define ENABLE_BACKEND_MMX 0

/* Enable NEON backend */
#define ENABLE_BACKEND_NEON 0

/* Enable SSE backend */
#define ENABLE_BACKEND_SSE 0

#undef ENABLE_BACKEND_ALTIVEC 
#undef ENABLE_BACKEND_C64X
#undef ENABLE_BACKEND_MMX 
#undef ENABLE_BACKEND_NEON 
#undef ENABLE_BACKEND_SSE 
/* Defined if host is amd64 */
/* #undef HAVE_AMD64 */

/* Defined if host is arm */
/* #undef HAVE_ARM */

/* Defined if we have clock_gettime() */
#define HAVE_CLOCK_GETTIME 1

/* Use malloc to allocate code for execution */
/* #undef HAVE_CODEMEM_MALLOC */

/* Use mmap to allocate code for execution */
#define HAVE_CODEMEM_MMAP 1

/* Use VirtualAlloc to allocate code for execution */
/* #undef HAVE_CODEMEM_VIRTUALALLOC */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Defined if host is i386 */
#define HAVE_I386 0

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Defined if we have a monotonic clock */
#define HAVE_MONOTONIC_CLOCK 1

/* Defined if host OS is linux */
#define HAVE_OS_LINUX 1

/* Defined if host OS is MS Windows */
/* #undef HAVE_OS_WIN32 */

/* Defined if host is powerpc */
/* #undef HAVE_POWERPC */

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the `sigsetjmp' function. */
/* #undef HAVE_SIGSETJMP */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Use pthread thread code */
#define HAVE_THREAD_PTHREAD 1

/* Use Windows thread code */
/* #undef HAVE_THREAD_WIN32 */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Defined for compiling internal code */
#define ORC_EXPORTS 1

/* Name of package */
#define PACKAGE "orc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "orc"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "orc 0.4.6.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "orc"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.4.6.1"

/* The size of `char', as computed by sizeof. */
/* #undef SIZEOF_CHAR */

/* The size of `int', as computed by sizeof. */
/* #undef SIZEOF_INT */

/* The size of `long', as computed by sizeof. */
/* #undef SIZEOF_LONG */

/* The size of `short', as computed by sizeof. */
/* #undef SIZEOF_SHORT */

/* The size of `void*', as computed by sizeof. */
/* #undef SIZEOF_VOIDP */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "0.4.6.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
