#ifndef HEADER_TCM_SMS4_H
#define HEADER_TCM_SMS4_H

#ifdef  __cplusplus
extern "C" {
#endif

#define	SMS4_BLOCK_SIZE 16
#define SMS4_MAX_LEN	512

#define SMS4_MODE_CBC	0
#define	SMS4_MODE_ECB	1

#ifndef UINT8
typedef unsigned char       UINT8;
#endif

#ifndef UINT32
typedef unsigned int        UINT32;
#endif


#ifndef INT32
typedef signed int          INT32;
#endif


#ifndef BYTE
typedef unsigned char       BYTE;
#endif

/*
* func : tcm_sms4_encrypt()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits
*		BYTE *M : 明文地址
*		UINT32 mLen: 明文长度
*		BYTE *S：密文地址
* 		BYTE *key : 密钥
* Output :
*		－1 : 加密失败
*		Len : 加密后长度
* Func Desp : SMS4的加密运算
*/
INT32 tcm_sms4_encrypt(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key);


/*
* func : tcm_sms4_decrypt()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits
*		BYTE *M : 明文地址
*		UINT32 mLen: 密文长度
*		BYTE *S：密文地址
* 		BYTE *key : 密钥
*		BYTE mode:SMS4模式选择
* Output :
*		－1 : 解密失败
*		Len : 解密后长度
* Func Desp : SMS4的解密运算
*/
INT32 tcm_sms4_decrypt(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key);



#ifdef	__cplusplus
}
#endif


#endif /* HEADER_TCM_SMS4_H */