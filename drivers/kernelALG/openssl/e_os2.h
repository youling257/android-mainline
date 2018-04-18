/* e_os2.h */

#include "opensslconf.h"

#ifndef HEADER_E_OS2_H
#define HEADER_E_OS2_H

#ifdef  __cplusplus
extern "C" {
#endif

#define OPENSSL_SYS_UNIX

/* ---------------------- Microsoft operating systems ---------------------- */

/* The 16 bit environments are pretty straightforward */
#if defined(OPENSSL_SYSNAME_WIN16) || defined(OPENSSL_SYSNAME_MSDOS)
# undef OPENSSL_SYS_UNIX
# define OPENSSL_SYS_MSDOS
#endif
#if defined(OPENSSL_SYSNAME_WIN16)
# undef OPENSSL_SYS_UNIX
# define OPENSSL_SYS_WIN16
#endif

/* For 32 bit environment, there seems to be the CygWin environment and then
   all the others that try to do the same thing Microsoft does... */
#if defined(OPENSSL_SYSNAME_UWIN)
# undef OPENSSL_SYS_UNIX
# define OPENSSL_SYS_WIN32_UWIN
#else
# if defined(__CYGWIN32__) || defined(OPENSSL_SYSNAME_CYGWIN32)
#  undef OPENSSL_SYS_UNIX
#  define OPENSSL_SYS_WIN32_CYGWIN
# else
#  if defined(_WIN32) || defined(OPENSSL_SYSNAME_WIN32)
#   undef OPENSSL_SYS_UNIX
#   define OPENSSL_SYS_WIN32
#  endif
#  if defined(OPENSSL_SYSNAME_WINNT)
#   undef OPENSSL_SYS_UNIX
#   define OPENSSL_SYS_WINNT
#  endif
#  if defined(OPENSSL_SYSNAME_WINCE)
#   undef OPENSSL_SYS_UNIX
#   define OPENSSL_SYS_WINCE
#  endif
# endif
#endif

/* Anything that tries to look like Microsoft is "Windows" */
#if defined(OPENSSL_SYS_WIN16) || defined(OPENSSL_SYS_WIN32) || defined(OPENSSL_SYS_WINNT) || defined(OPENSSL_SYS_WINCE)
# undef OPENSSL_SYS_UNIX
# define OPENSSL_SYS_WINDOWS
# ifndef OPENSSL_SYS_MSDOS
#  define OPENSSL_SYS_MSDOS
# endif
#endif

#ifdef OPENSSL_SYS_WINDOWS
# ifndef OPENSSL_OPT_WINDLL
#  if defined(_WINDLL) /* This is used when building OpenSSL to indicate that
                          DLL linkage should be used */
#   define OPENSSL_OPT_WINDLL
#  endif
# endif
#endif

/* ------------------------------- VxWorks --------------------------------- */
#ifdef OPENSSL_SYSNAME_VXWORKS
# define OPENSSL_SYS_VXWORKS
#endif

#ifdef OPENSSL_SYS_MSDOS
# define OPENSSL_UNISTD_IO <io.h>
# define OPENSSL_DECLARE_EXIT extern void exit(int);
#else
# define OPENSSL_UNISTD_IO OPENSSL_UNISTD
# define OPENSSL_DECLARE_EXIT /* declared in unistd.h */
#endif

#if defined(OPENSSL_SYS_VMS_NODECC)
# define OPENSSL_EXPORT globalref
# define OPENSSL_IMPORT globalref
# define OPENSSL_GLOBAL globaldef
#elif defined(OPENSSL_SYS_WINDOWS) && defined(OPENSSL_OPT_WINDLL)
# define OPENSSL_EXPORT extern _declspec(dllexport)
# define OPENSSL_IMPORT extern _declspec(dllimport)
# define OPENSSL_GLOBAL
#else
# define OPENSSL_EXPORT extern
# define OPENSSL_IMPORT extern
# define OPENSSL_GLOBAL
#endif
#define OPENSSL_EXTERN OPENSSL_IMPORT

#ifdef OPENSSL_EXPORT_VAR_AS_FUNCTION
# define OPENSSL_IMPLEMENT_GLOBAL(type,name) static type _hide_##name; \
        type *_shadow_##name(void) { return &_hide_##name; } \
        static type _hide_##name
# define OPENSSL_DECLARE_GLOBAL(type,name) type *_shadow_##name(void)
# define OPENSSL_GLOBAL_REF(name) (*(_shadow_##name()))
#else
# define OPENSSL_IMPLEMENT_GLOBAL(type,name) OPENSSL_GLOBAL type _shadow_##name
# define OPENSSL_DECLARE_GLOBAL(type,name) OPENSSL_EXPORT type _shadow_##name
# define OPENSSL_GLOBAL_REF(name) _shadow_##name
#endif

#ifdef  __cplusplus
}
#endif
#endif
