/* e_os.h */

#ifndef HEADER_E_OS_H
#define HEADER_E_OS_H

#include "opensslconf.h"
#include "e_os2.h"

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(OPENSSL_SYS_VXWORKS)
#  define NO_SYS_PARAM_H
#  define NO_CHMOD
#  define NO_SYSLOG
#endif
  
/********************************************************************
 The Microsoft section
 ********************************************************************/
/* The following is used becaue of the small stack in some
 * Microsoft operating systems */
#if defined(OPENSSL_SYS_MSDOS) && !defined(OPENSSL_SYSNAME_WIN32)
#  define MS_STATIC	static
#else
#  define MS_STATIC
#endif

#if defined(OPENSSL_SYS_WIN32) && !defined(WIN32)
#  define WIN32
#endif
#if defined(OPENSSL_SYS_WIN16) && !defined(WIN16)
#  define WIN16
#endif
#if defined(OPENSSL_SYS_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(OPENSSL_SYS_MSDOS) && !defined(MSDOS)
#  define MSDOS
#endif

#if defined(MSDOS) && !defined(GETPID_IS_MEANINGLESS)
#  define GETPID_IS_MEANINGLESS
#endif

#ifdef WIN32
#define get_last_sys_error()	GetLastError()
#define clear_sys_error()	SetLastError(0)
#if !defined(WINNT)
#define WIN_CONSOLE_BUG
#endif
#else
#define get_last_sys_error()	errno
#define clear_sys_error()	errno=0
#endif

#if defined(WINDOWS)
#define get_last_socket_error()	WSAGetLastError()
#define clear_socket_error()	WSASetLastError(0)
#define readsocket(s,b,n)	recv((s),(b),(n),0)
#define writesocket(s,b,n)	send((s),(b),(n),0)
#define EADDRINUSE		WSAEADDRINUSE
#elif defined(__DJGPP__)
#define WATT32
#define get_last_socket_error()	errno
#define clear_socket_error()	errno=0
#define closesocket(s)		close_s(s)
#define readsocket(s,b,n)	read_s(s,b,n)
#define writesocket(s,b,n)	send(s,b,n,0)
#elif defined(MAC_OS_pre_X)
#define get_last_socket_error()	errno
#define clear_socket_error()	errno=0
#define closesocket(s)		MacSocket_close(s)
#define readsocket(s,b,n)	MacSocket_recv((s),(b),(n),true)
#define writesocket(s,b,n)	MacSocket_send((s),(b),(n))
#elif defined(OPENSSL_SYS_VMS)
#define get_last_socket_error() errno
#define clear_socket_error()    errno=0
#define ioctlsocket(a,b,c)      ioctl(a,b,c)
#define closesocket(s)          close(s)
#define readsocket(s,b,n)       recv((s),(b),(n),0)
#define writesocket(s,b,n)      send((s),(b),(n),0)
#else
#define get_last_socket_error()	errno
#define clear_socket_error()	errno=0
#define ioctlsocket(a,b,c)	ioctl(a,b,c)
#define closesocket(s)		close(s)
#define readsocket(s,b,n)	read((s),(b),(n))
#define writesocket(s,b,n)	write((s),(b),(n))
#endif

#ifdef WIN16
#  define OPENSSL_NO_FP_API
#  define MS_CALLBACK	_far _loadds
#  define MS_FAR	_far
#else
#  define MS_CALLBACK
#  define MS_FAR
#endif

#ifdef OPENSSL_NO_STDIO
#  define OPENSSL_NO_FP_API
#endif */

#if (defined(WINDOWS) || defined(MSDOS))

#  ifdef __DJGPP__
#    include <unistd.h>
#    include <sys/stat.h>
#    include <sys/socket.h>
#    include <tcp.h>
#    include <netdb.h>
#    define _setmode setmode
#    define _O_TEXT O_TEXT
#    define _O_BINARY O_BINARY
#  endif /* __DJGPP__ */

#  ifndef S_IFDIR
#    define S_IFDIR	_S_IFDIR
#  endif

#  ifndef S_IFMT
#    define S_IFMT	_S_IFMT
#  endif

#  if !defined(WINNT) && !defined(__DJGPP__)
#    define NO_SYSLOG
#  endif
#  define NO_DIRENT

#  ifdef WINDOWS
#    include <windows.h>
#    include <stddef.h>
#    include <errno.h>
#    include <string.h>
#    include <malloc.h>
#  endif
#  include <io.h>
#  include <fcntl.h>

#  define ssize_t long

#  if defined(WIN16) && defined(SSLEAY) && defined(_WINEXITNOPERSIST)
#    define EXIT(n) _wsetexit(_WINEXITNOPERSIST)
#    define OPENSSL_EXIT(n) do { if (n == 0) EXIT(n); return(n); } while(0)
#  else
#    define EXIT(n) return(n)
#  endif
#  define LIST_SEPARATOR_CHAR ';'
#  ifndef X_OK
#    define X_OK	0
#  endif
#  ifndef W_OK
#    define W_OK	2
#  endif
#  ifndef R_OK
#    define R_OK	4
#  endif
#  define OPENSSL_CONF	"openssl.cnf"
#  define SSLEAY_CONF	OPENSSL_CONF
#  define NUL_DEV	"nul"
#  define RFILE		".rnd"
#  ifdef OPENSSL_SYS_WINCE
#    define DEFAULT_HOME  ""
#  else
#    define DEFAULT_HOME  "C:"
#  endif

#else /* The non-microsoft world world */

#  ifdef OPENSSL_SYS_VMS
#    define VMS 1
  /* some programs don't include stdlib, so exit() and others give implicit 
     function warnings */
#    include <stdlib.h>
#    if defined(__DECC)
#      include <unistd.h>
#    else
#      include <unixlib.h>
#    endif
#    define OPENSSL_CONF	"openssl.cnf"
#    define SSLEAY_CONF		OPENSSL_CONF
#    define RFILE		".rnd"
#    define LIST_SEPARATOR_CHAR ','
#    define NUL_DEV		"NLA0:"
  /* We don't have any well-defined random devices on VMS, yet... */
/* #    undef DEVRANDOM */

#    define NO_SYS_PARAM_H
#  else
     /* !defined VMS */
#    ifdef OPENSSL_SYS_MPE
#      define NO_SYS_PARAM_H
#    endif
#    ifdef OPENSSL_UNISTD
#      include OPENSSL_UNISTD
#    else
#      include <unistd.h>
#    endif
#    ifndef NO_SYS_TYPES_H
#      include <sys/types.h>
#    endif
#    if defined(NeXT) || defined(OPENSSL_SYS_NEWS4)
#      define pid_t int /* pid_t is missing on NEXTSTEP/OPENSTEP
                         * (unless when compiling with -D_POSIX_SOURCE,
                         * which doesn't work for us) */
#      define ssize_t int /* ditto */
#    endif
#    ifdef OPENSSL_SYS_NEWS4 /* setvbuf is missing on mips-sony-bsd */
#      define setvbuf(a, b, c, d) setbuffer((a), (b), (d))
       typedef unsigned long clock_t;
#    endif

#    define OPENSSL_CONF	"openssl.cnf"
#    define SSLEAY_CONF		OPENSSL_CONF
#    define RFILE		".rnd"
#    define LIST_SEPARATOR_CHAR ':'
#    define NUL_DEV		"/dev/null"
#    define EXIT(n)		exit(n)
#  endif

#  define SSLeay_getpid()	getpid()

#endif

#ifdef USE_SOCKETS
#  if defined(WINDOWS) || defined(MSDOS)
      /* windows world */

#    ifdef OPENSSL_NO_SOCK
#      define SSLeay_Write(a,b,c)	(-1)
#      define SSLeay_Read(a,b,c)	(-1)
#      define SHUTDOWN(fd)		close(fd)
#      define SHUTDOWN2(fd)		close(fd)
#    elif !defined(__DJGPP__)
#      include <winsock.h>
extern HINSTANCE _hInstance;
#      define SSLeay_Write(a,b,c)	send((a),(b),(c),0)
#      define SSLeay_Read(a,b,c)	recv((a),(b),(c),0)
#      define SHUTDOWN(fd)		{ shutdown((fd),0); closesocket(fd); }
#      define SHUTDOWN2(fd)		{ shutdown((fd),2); closesocket(fd); }
#    else
#      define SSLeay_Write(a,b,c)	write_s(a,b,c,0)
#      define SSLeay_Read(a,b,c)	read_s(a,b,c)
#      define SHUTDOWN(fd)		close_s(fd)
#      define SHUTDOWN2(fd)		close_s(fd)
#    endif

#  elif defined(MAC_OS_pre_X)
#  else

#    ifndef NO_SYS_PARAM_H
#      include <sys/param.h>
#    endif
#    ifdef OPENSSL_SYS_VXWORKS
#      include <time.h> 
#    elif !defined(OPENSSL_SYS_MPE)
#      include <sys/time.h> /* Needed under linux for FD_XXX */
#    endif

#    include <netdb.h>
#    if defined(OPENSSL_SYS_VMS_NODECC)
#      include <socket.h>
#      include <in.h>
#      include <inet.h>
#    else
#      include <sys/socket.h>
#      ifdef FILIO_H
#        include <sys/filio.h> /* Added for FIONBIO under unixware */
#      endif
#      include <netinet/in.h>
#      include <arpa/inet.h>
#    endif

#    if defined(NeXT) || defined(_NEXT_SOURCE)
#      include <sys/fcntl.h>
#      include <sys/types.h>
#    endif

#    define SSLeay_Read(a,b,c)     read((a),(b),(c))
#    define SSLeay_Write(a,b,c)    write((a),(b),(c))
#    define SHUTDOWN(fd)    { shutdown((fd),0); closesocket((fd)); }
#    define SHUTDOWN2(fd)   { shutdown((fd),2); closesocket((fd)); }
#    ifndef INVALID_SOCKET
#    define INVALID_SOCKET	(-1)
#    endif /* INVALID_SOCKET */
#  endif
#endif

#if defined(__ultrix)
#  ifndef ssize_t
#    define ssize_t int 
#  endif
#endif

#if defined(sun) && !defined(__svr4__) && !defined(__SVR4)
  /* include headers first, so our defines don't break it */
#include <stdlib.h>
#include <string.h>
  /* bcopy can handle overlapping moves according to SunOS 4.1.4 manpage */
# define memmove(s1,s2,n) bcopy((s2),(s1),(n))
# define strtoul(s,e,b) ((unsigned long int)strtol((s),(e),(b)))
extern char *sys_errlist[]; extern int sys_nerr;
# define strerror(errnum) \
	(((errnum)<0 || (errnum)>=sys_nerr) ? NULL : sys_errlist[errnum])
#endif

#ifndef OPENSSL_EXIT
# if defined(MONOLITH) && !defined(OPENSSL_C)
#  define OPENSSL_EXIT(n) return(n)
# else
#  define OPENSSL_EXIT(n) do { EXIT(n); return(n); } while(0)
# endif
#endif

#ifdef WIN16
/* How to do this needs to be thought out a bit more.... */
/*char *GETENV(char *);
#define Getenv	GETENV*/
#define Getenv	getenv
#else
#define Getenv getenv
#endif

#define DG_GCC_BUG	/* gcc < 2.6.3 on DGUX */

#ifdef sgi
#define IRIX_CC_BUG	/* all version of IRIX I've tested (4.* 5.*) */
#endif
#ifdef OPENSSL_SYS_SNI
#define IRIX_CC_BUG	/* CDS++ up to V2.0Bsomething suffered from the same bug.*/
#endif

#if defined(OPENSSL_SYS_OS2) && defined(__EMX__)
# include <io.h>
# include <fcntl.h>
# define NO_SYSLOG
# define strcasecmp stricmp
#endif

/* vxworks */
#if defined(OPENSSL_SYS_VXWORKS)
#include <ioLib.h>
#include <tickLib.h>
#include <sysLib.h>

#define TTY_STRUCT int

#define sleep(a) taskDelay((a) * sysClkRateGet())
#if defined(ioctlsocket)
#undef ioctlsocket
#endif
#define ioctlsocket(a,b,c) ioctl((a),(b),*(c))

#include <vxWorks.h>
#include <sockLib.h>
#include <taskLib.h>

#define getpid taskIdSelf

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const char *addr, int length, int type);
struct servent *getservbyname(const char *name, const char *proto);

#endif
/* end vxworks */

#ifdef  __cplusplus
}
#endif

#endif

