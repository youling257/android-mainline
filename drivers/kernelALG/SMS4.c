#include "SMS4.h"
//#include "string.h"
#include <linux/string.h>

/* 
使用局部变量，避免多线程时出错 linyang 2006.12.21
SMS4_KEY smsKey;
*/

UINT32 Unpack32(BYTE * src)
{	
	return (((UINT32) src[0]) << 24
		| ((UINT32) src[1]) << 16
		| ((UINT32) src[2]) << 8
		| (UINT32) src[3]);
}

/* 
可以不再使用 linyang 2006.12.21
void SMS4Init(SMS4_KEY *pSmsKey)
{
	int i;
	for(i=0;i<32;i++)
		pSmsKey->rk[i]=0;
}
*/

/* ROTATE_LEFT rotates x left n bits.*/
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* S box data */
static UINT8 SBX[16][16] = {
  0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
		0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
		0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
		0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
		0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
		0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
		0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
		0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
		0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
		0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
		0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
		0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
		0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
		0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
		0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
		0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48};

UINT8 SBox(UINT8 into)
{
	UINT8 h = into>>4;
	return SBX[h][into^(h<<4)];
}

/* data convertion  */
UINT32 T(UINT32 A)
{
	UINT8 b0 = SBox((UINT8)(A>>24));
	UINT8 b1 = SBox((UINT8)(A>>16));
	UINT8 b2 = SBox((UINT8)(A>>8));
	UINT8 b3 = SBox((UINT8)A);

	UINT32 B = (b0<<24) | (b1<<16) | (b2<<8) | b3;
	UINT32 B1 = ROTATE_LEFT(B,2);
	UINT32 B2 = ROTATE_LEFT(B,10);
	UINT32 B3 = ROTATE_LEFT(B,18);
	UINT32 B4 = ROTATE_LEFT(B,24);
	UINT32 C = B^B1^B2^B3^B4;
	return C;
}

/* ring function */

static UINT32 FK[4] = {0xA3B1BAC6,0x56AA3350,0x677D9197,0xB27022DC};
static UINT32 CK[32] = {
	0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
		0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
		0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
		0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
		0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
		0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
		0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
		0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279};

/* key convertion  */
UINT32 KT(UINT32 A)
{
	UINT8 b0 = SBox((UINT8)(A>>24));
	UINT8 b1 = SBox((UINT8)(A>>16));
	UINT8 b2 = SBox((UINT8)(A>>8));
	UINT8 b3 = SBox((UINT8)A);

	UINT32 B = (b0<<24) | (b1<<16) | (b2<<8) | b3;
	UINT32 B1 = ROTATE_LEFT(B,13);
	UINT32 B2 = ROTATE_LEFT(B,23);
	UINT32 C = B^B1^B2;
	return C;
}

/* Key expantions  */
void SMS4Key(UINT32 MK[4], SMS4_KEY *pSmsKey)
{
	int i;
	UINT32 tmpMK[4] = {	Unpack32((BYTE *)&MK[0]), Unpack32((BYTE *)&MK[1]), 
		Unpack32((BYTE *)&MK[2]), Unpack32((BYTE *)&MK[3]) };

	UINT32 K[36] = {
		tmpMK[0]^FK[0],tmpMK[1]^FK[1],tmpMK[2]^FK[2],tmpMK[3]^FK[3],
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

//	tmpMK[0] = Unpack32((BYTE *)&MK[0]);
//	tmpMK[1] = Unpack32((BYTE *)&MK[1]);
//	tmpMK[2] = Unpack32((BYTE *)&MK[2]);
//	tmpMK[3] = Unpack32((BYTE *)&MK[3]);

	for(i=0;i<32;i++)
	{
		pSmsKey->rk[i] = K[i]^KT(K[i+1]^K[i+2]^K[i+3]^CK[i]);
		K[i+4] = pSmsKey->rk[i];
	}
	return ;

}
/* SMS4 Encrypt data*/
/* Input : UINT32 Plain
   Output: UINT32 Cipher */
void SMS4Enc (UINT32 Plain[4],UINT32 Cipher[4], SMS4_KEY *pSmsKey)
{
	int i;
	UINT32	tmpPlain[4] = { Unpack32((BYTE *)&Plain[0]), Unpack32((BYTE *)&Plain[1]),
		Unpack32((BYTE *)&Plain[2]), Unpack32((BYTE *)&Plain[3]) };

	UINT32 X[36] = { 
		tmpPlain[0],tmpPlain[1],tmpPlain[2],tmpPlain[3],
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

//	tmpPlain[0] = Unpack32((BYTE *)&Plain[0]);
//	tmpPlain[1] = Unpack32((BYTE *)&Plain[1]);
//	tmpPlain[2] = Unpack32((BYTE *)&Plain[2]);
//	tmpPlain[3] = Unpack32((BYTE *)&Plain[3]);


	for(i=0;i<32;i++)
	{
		X[i] = X[i]^T(X[i+1]^X[i+2]^X[i+3]^pSmsKey->rk[i]);
		X[i+4] = X[i];
	}
	Cipher[0] = Unpack32((BYTE *)&X[35]);
	Cipher[1] = Unpack32((BYTE *)&X[34]);
	Cipher[2] = Unpack32((BYTE *)&X[33]);
	Cipher[3] = Unpack32((BYTE *)&X[32]);
	return;
}
/* SMS4 Decrypt data*/
void SMS4Dec (UINT32 Cipher[4],UINT32 Plain[4], SMS4_KEY *pSmsKey)
{
	int i;
	UINT32	tmpCipher[4] = {Unpack32((BYTE *)&Cipher[0]), Unpack32((BYTE *)&Cipher[1]),
		Unpack32((BYTE *)&Cipher[2]), Unpack32((BYTE *)&Cipher[3]) };

	UINT32 X[36] = { 
		tmpCipher[0],tmpCipher[1],tmpCipher[2],tmpCipher[3],
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

//	tmpCipher[0] = Unpack32((BYTE *)&Cipher[0]);
//	tmpCipher[1] = Unpack32((BYTE *)&Cipher[1]);
//	tmpCipher[2] = Unpack32((BYTE *)&Cipher[2]);
//	tmpCipher[3] = Unpack32((BYTE *)&Cipher[3]);

	for(i=0;i<32;i++)
	{
	//	X[i] = X[i]^T(X[i+1]^X[i+2]^X[i+3]^smsKey.rk[i]);
		X[i] = X[i]^T(X[i+1]^X[i+2]^X[i+3]^pSmsKey->rk[31-i]);
		X[i+4] = X[i];
	}
	Plain[0] = Unpack32((BYTE *)&X[35]);
	Plain[1] = Unpack32((BYTE *)&X[34]);
	Plain[2] = Unpack32((BYTE *)&X[33]);
	Plain[3] = Unpack32((BYTE *)&X[32]);
	return;
}


/*
* func : sms4CBC_xor()
* Input :  
* 		BYTE * str1 : 异或源地址1
*		BYTE * str2 : 异或源地址2
*		BYTE * outStr：异或输出地址
*		UINT32 len：异或长度
* Output :
*		0 : 异或失败
*		Len : 异或长度
* Func Desp : CBC模式的异或运算
*/
UINT32 sms4CBC_xor(BYTE * str1, BYTE * str2, BYTE * outStr, UINT32 len)
{
	UINT32 i;

	for (i = 0; i < len; i++)
		outStr[i] = str1[i] ^ str2[i];
	return len;
}

/*
* func : Sms4CBC_E()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits
*		BYTE *M : 明文地址
*		UINT32 mLen: 明文长度
*		BYTE *S：密文地址
* 		BYTE *pucKey : 密钥
* Output :
*		－1 : 加密失败
*		Len : 加密后长度
* Func Desp : CBC模式的加密运算
*/
INT32 Sms4CBC_E(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *pucKey)
{
	SMS4_KEY smsKey = {0};
	INT32	i = 0;
	BYTE	sms4Buff[SMS4_BLOCK_SIZE];
	BYTE	*tmpM, *tmpS;
	
	tmpM = M;
	tmpS = S;

	SMS4Key((UINT32 *)pucKey, &smsKey);

	memcpy(sms4Buff, IV, SMS4_BLOCK_SIZE);

	while((mLen - i) > 0)
	{
		sms4CBC_xor(tmpM, sms4Buff, sms4Buff, SMS4_BLOCK_SIZE);	//先异或
		SMS4Enc((UINT32 *)sms4Buff, (UINT32 *)tmpS, &smsKey);	//再加密
		memcpy(sms4Buff, tmpS, SMS4_BLOCK_SIZE);				//保留上一轮的加密结果
		tmpS = tmpS + SMS4_BLOCK_SIZE;
		tmpM = tmpM + SMS4_BLOCK_SIZE;
		i = i + SMS4_BLOCK_SIZE;
	}
	return	i;
}

/*
* func : Sms4CBC_D()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits
*		BYTE *M : 明文地址
*		UINT32 mLen: 密文长度
*		BYTE *S：密文地址
* 		BYTE *pucKey : 密钥
* Output :
*		－1 : 解密失败
*		Len : 解密后长度
* Func Desp : CBC模式的解密运算
*/
INT32 Sms4CBC_D(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *pucKey)
{
	SMS4_KEY smsKey = {0};
	INT32	i = 0;
	BYTE	sms4Buff[SMS4_BLOCK_SIZE];
	BYTE	*tmpM, *tmpS;
	
	tmpM = M;
	tmpS = S;
	SMS4Key((UINT32 *)pucKey, &smsKey);

	memcpy(sms4Buff, IV, SMS4_BLOCK_SIZE);

	while((mLen - i) > 0)
	{
		SMS4Dec((UINT32 *)tmpS, (UINT32 *)tmpM, &smsKey);					//先解密
		sms4CBC_xor(tmpM, sms4Buff, tmpM, SMS4_BLOCK_SIZE);			//再异或
		memcpy(sms4Buff, tmpS, SMS4_BLOCK_SIZE);					//保留上一轮结果
		tmpS = tmpS + SMS4_BLOCK_SIZE;
		tmpM = tmpM + SMS4_BLOCK_SIZE;
		i = i + SMS4_BLOCK_SIZE;
	}
	return	i;
}


/*
* func : Sms4ECB_E()
* Input :  
*		BYTE *M : 明文地址
*		UINT32 mLen: 明文长度
*		BYTE *S：密文地址
* 		BYTE *pucKey : 密钥
* Output :
*		－1 : 加密失败
*		Len : 加密后长度
* Func Desp : CBC模式的加密运算
*/
INT32 Sms4ECB_E(BYTE *M, INT32 mLen, BYTE *S, BYTE *pucKey)
{
	SMS4_KEY smsKey = {0};
	INT32	i = 0;
	BYTE	*tmpM, *tmpS;
	
	tmpM = M;
	tmpS = S;
	SMS4Key((UINT32 *)pucKey, &smsKey);

	while((mLen - i) > 0)
	{
		SMS4Enc((UINT32 *)tmpM, (UINT32 *)tmpS, &smsKey);							//加密
		tmpS = tmpS + SMS4_BLOCK_SIZE;
		tmpM = tmpM + SMS4_BLOCK_SIZE;
		i = i + SMS4_BLOCK_SIZE;
	}
	return	i;
}

/*
* func : Sms4ECB_D()
* Input :  
*		BYTE *M : 明文地址
*		UINT32 mLen: 密文长度
*		BYTE *S：密文地址
* 		BYTE *pucKey : 密钥
* Output :
*		－1 : 解密失败
*		Len : 解密后长度
* Func Desp : CBC模式的解密运算
*/
INT32 Sms4ECB_D(BYTE *M, INT32 mLen, BYTE *S, BYTE *pucKey)
{
	SMS4_KEY smsKey = {0};
	INT32	i = 0;
	BYTE	*tmpM, *tmpS;
	
	tmpM = M;
	tmpS = S;
	SMS4Key((UINT32 *)pucKey, &smsKey);

	while((mLen - i) > 0)
	{
		SMS4Dec((UINT32 *)tmpS, (UINT32 *)tmpM, &smsKey);			//解密
		tmpS = tmpS + SMS4_BLOCK_SIZE;
		tmpM = tmpM + SMS4_BLOCK_SIZE;
		i = i + SMS4_BLOCK_SIZE;
	}
	return	i;
}


/*
* func : SMS4_fill()
* Input :  
* 		BYTE *in : 源地址
*		BYTE *len : 源长度
* Output :
* Func Desp : 填充函数
*/
void SMS4_fill(BYTE *in, INT32 *len)
{
	INT32 tmpLen;
	BYTE  *p,fillByte;

	tmpLen = *len;
	p = in;

	if (tmpLen & 0x0f)	//不是16的整数倍
	{
		fillByte = 16 - (tmpLen & 0x0f);						
	}else		//16的整数倍
	{
		fillByte = 16;
	}
	memset(p + tmpLen, fillByte, fillByte);
	*len = *len + fillByte;
}

/*
* func : SMS4_E()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits，如果是ECB模式，可以填0
*		BYTE *M : 明文地址
*		UINT32 mLen: 明文长度
*		BYTE *S：密文地址
* 		BYTE *key : 密钥
*		BYTE mode:SMS4模式选择
* Output :
*		－1 : 加密失败
*		Len : 加密后长度
* Func Desp : SMS4的加密运算
*/
INT32 SMS4_E(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key, BYTE mode)
{
	INT32 tmpLen = mLen;

	//
	// 2009/07/29
	// 删除对明文长度的判断
	//
	//if (!M || !S || !key || mLen <= 0 || mLen > SMS4_MAX_LEN)
	//	return	-1;

	//SMS4_fill(M, &tmpLen);
	//if (tmpLen & 0x0f || tmpLen > (SMS4_MAX_LEN + SMS4_BLOCK_SIZE))
	//	return	-1;

	if (!M || !S || !key || mLen <= 0)
		return	-1;

	SMS4_fill(M, &tmpLen);
	if (tmpLen & 0x0f)
		return	-1;


	switch (mode)
	{
		case SMS4_MODE_ECB:
			return	Sms4ECB_E(M, tmpLen, S, key);
			break;
		case SMS4_MODE_CBC:
			if (!IV)
				return	-1;
			return	Sms4CBC_E(IV, M, tmpLen, S, key);
			break;
		default:
			return -1;
	}	
}

/*
* func : SMS4_D()
* Input :  
* 		BYTE *IV : IV，和密钥长度一样，128bits，如果是ECB模式，可以填0
*		BYTE *M : 明文地址
*		UINT32 mLen: 密文长度
*		BYTE *S：密文地址
* 		BYTE *key : 密钥
*		BYTE mode:SMS4模式选择
* Output :
*		－1 : 加密失败
*		Len : 加密后长度
* Func Desp : SMS4的解密运算
*/
INT32 SMS4_D(BYTE *IV, BYTE *M, INT32 mLen, BYTE *S, BYTE *key, BYTE mode)
{
	INT32 tmpLen = mLen;

	if (!M || !S || !key || mLen <= 0)
		return	-1;

	//
	// 2009/07/29
	// 删除对明文长度的判断
	//
	//if (mLen & 0x0f || mLen > (SMS4_MAX_LEN + SMS4_BLOCK_SIZE))
	//	return	-1;

	if (mLen & 0x0f)
		return	-1;

	switch (mode)
	{
		case SMS4_MODE_ECB:
			tmpLen = Sms4ECB_D(M, mLen, S, key);
			break;
		case SMS4_MODE_CBC:
			if (!IV)
				return	-1;
			tmpLen = Sms4CBC_D(IV, M, mLen, S, key);
			break;
		default:
			return -1;
	}	
	tmpLen = tmpLen - *(M + tmpLen - 1);
	return	tmpLen;
}

