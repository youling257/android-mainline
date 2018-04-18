/* err.h */

#ifndef HEADER_ERR_H
#define HEADER_ERR_H

#ifndef OPENSSL_NO_FP_API
#include <stdio.h>
#include <stdlib.h>
#endif
#ifndef OPENSSL_NO_LHASH
#include "lhash.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef OPENSSL_NO_ERR
#define ERR_PUT_error(a,b,c,d,e)	ERR_put_error(a,b,c,d,e)
#else
#define ERR_PUT_error(a,b,c,d,e)	ERR_put_error(a,b,c,NULL,0)
#endif

#include <errno.h>

#define ERR_TXT_MALLOCED	0x01
#define ERR_TXT_STRING		0x02

#define ERR_NUM_ERRORS	16
typedef struct err_state_st
	{
	unsigned long pid;
	unsigned long err_buffer[ERR_NUM_ERRORS];
	char *err_data[ERR_NUM_ERRORS];
	int err_data_flags[ERR_NUM_ERRORS];
	const char *err_file[ERR_NUM_ERRORS];
	int err_line[ERR_NUM_ERRORS];
	int top,bottom;
	} ERR_STATE;

/* library */
#define ERR_LIB_NONE		1
#define ERR_LIB_SYS		2
#define ERR_LIB_BN		3
#define ERR_LIB_EVP		6
#define ERR_LIB_BUF		7
#define ERR_LIB_OBJ		8
#define ERR_LIB_ASN1		13
#define ERR_LIB_CONF		14
#define ERR_LIB_CRYPTO		15
#define ERR_LIB_EC		16
#define ERR_LIB_BIO		32
#define ERR_LIB_USER		128

#define BNerr(f,r)   ERR_PUT_error(ERR_LIB_BN,(f),(r),__FILE__,__LINE__)
#define EVPerr(f,r)  ERR_PUT_error(ERR_LIB_EVP,(f),(r),__FILE__,__LINE__)
#define CRYPTOerr(f,r) ERR_PUT_error(ERR_LIB_CRYPTO,(f),(r),__FILE__,__LINE__)
#define BIOerr(f,r)  ERR_PUT_error(ERR_LIB_BIO,(f),(r),__FILE__,__LINE__)

/* Borland C seems too stupid to be able to shift and do longs in
 * the pre-processor :-( */
#define ERR_PACK(l,f,r)		(((((unsigned long)l)&0xffL)*0x1000000)| \
				((((unsigned long)f)&0xfffL)*0x1000)| \
				((((unsigned long)r)&0xfffL)))
#define ERR_GET_LIB(l)		(int)((((unsigned long)l)>>24L)&0xffL)
#define ERR_GET_FUNC(l)		(int)((((unsigned long)l)>>12L)&0xfffL)
#define ERR_GET_REASON(l)	(int)((l)&0xfffL)

/* OS functions */
#define SYS_F_FOPEN		1
#define SYS_F_CONNECT		2
#define SYS_F_GETSERVBYNAME	3
#define SYS_F_SOCKET		4
#define SYS_F_IOCTLSOCKET	5
#define SYS_F_BIND		6
#define SYS_F_LISTEN		7
#define SYS_F_ACCEPT		8
#define SYS_F_WSASTARTUP	9 /* Winsock stuff */
#define SYS_F_OPENDIR		10
#define SYS_F_FREAD		11

/* reasons */
#define ERR_R_SYS_LIB	ERR_LIB_SYS       /* 2 */
#define ERR_R_BN_LIB	ERR_LIB_BN        /* 3 */
#define ERR_R_EVP_LIB	ERR_LIB_EVP       /* 6 */
#define ERR_R_BUF_LIB	ERR_LIB_BUF       /* 7 */
#define ERR_R_OBJ_LIB	ERR_LIB_OBJ       /* 8 */
#define ERR_R_ASN1_LIB	ERR_LIB_ASN1     /* 13 */
#define ERR_R_CONF_LIB	ERR_LIB_CONF     /* 14 */
#define ERR_R_CRYPTO_LIB ERR_LIB_CRYPTO  /* 15 */
#define ERR_R_BIO_LIB	ERR_LIB_BIO      /* 32 */
#define ERR_R_NESTED_ASN1_ERROR			58
#define ERR_R_BAD_ASN1_OBJECT_HEADER		59
#define ERR_R_BAD_GET_ASN1_OBJECT_CALL		60
#define ERR_R_EXPECTING_AN_ASN1_SEQUENCE	61
#define ERR_R_ASN1_LENGTH_MISMATCH		62
#define ERR_R_MISSING_ASN1_EOS			63

/* fatal error */
#define ERR_R_FATAL				64
#define	ERR_R_MALLOC_FAILURE			(1|ERR_R_FATAL)
#define	ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED	(2|ERR_R_FATAL)
#define	ERR_R_PASSED_NULL_PARAMETER		(3|ERR_R_FATAL)
#define	ERR_R_INTERNAL_ERROR			(4|ERR_R_FATAL)

typedef struct ERR_string_data_st
	{
	unsigned long error;
	const char *string;
	} ERR_STRING_DATA;

void ERR_put_error(int lib, int func,int reason,const char *file,int line);

#ifndef OPENSSL_NO_FP_API
void ERR_print_errors_fp(FILE *fp);
#endif
ERR_STATE *ERR_get_state(void);

#ifndef OPENSSL_NO_LHASH
LHASH *ERR_get_string_table(void);
LHASH *ERR_get_err_state_table(void);
#endif

typedef struct st_ERR_FNS ERR_FNS;
const ERR_FNS *ERR_get_implementation(void);

#ifdef	__cplusplus
}
#endif

#endif
