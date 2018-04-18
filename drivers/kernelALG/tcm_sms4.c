//#include <malloc.h>
//#include <memory.h>
#include <linux/vmalloc.h>


//#include "tcm_sms4.h"
#include "SMS4.h"

#ifndef NULL
#define NULL    0
/*#define NULL ((void *)0)*/
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
INT32 tcm_sms4_encrypt(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key)
{
	INT32 iret;
	BYTE *pTempM;
	/* 分配内存，为cbc模式作准备 */
	pTempM = (BYTE *)vmalloc(mLen + 16);
	if( pTempM == NULL )
		return -1;

	memset(pTempM, 0, mLen+16);
	memcpy(pTempM, M, mLen);
	/* 开始CBC模式加密 */
	iret = SMS4_E(IV, pTempM, mLen, S, key, SMS4_MODE_CBC);
	/* 释放分配的内存 */
	vfree(pTempM);
	return iret;
}


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
INT32 tcm_sms4_decrypt(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key)
{
	return SMS4_D(IV, M, mLen, S, key, SMS4_MODE_CBC);
}
