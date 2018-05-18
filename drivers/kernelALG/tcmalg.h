/************************************************************
文件名：tcmalg.h
链接库：tcmalg.lib(静态)
说明：每个函数的返回值表示的含义可能不一样，具体说明请参看函数的定义说明
版本修订人: linyang
时间：
  2006.12.18
       改正sch在release版本下出错.
	   改正ecc解密大于32字节的数据出错
  2006.12.18
      使用动态连结c语言库(msvcrt.dll/msvcr71.dll)
  2006.12.21
		为支持多线程，改sms4代码中的全局密钥为局部变量。
		更正模式中栈溢出问题
		删除初始化sms4密钥函数
  2006.12.25
		为支持测试firmware中密钥协商功能，在密码库中增加了密钥协商功能函数。
		仅仅测试使用，不用于其他用途。仅支持256bit国标曲线，不支持192国标曲线
  2006.12.31
		为支持测试密钥对是否匹配，增加了两个函数接口用于测试。
		tcm_ecc_is_key_match 用于测试公私钥是否匹配
		tcm_ecc_point_from_privatekey 用于从公钥得到私钥
		仅仅支持256曲线。
  2007.1.5
		修改生成的大数不满32字节而发生错误的情况。
  2007.3.29
		原定义SMS4只能加密512字节长度的明文。现在取消这个限制。
************************************************************/


#ifndef HEADER_TCM_ALG_H
#define HEADER_TCM_ALG_H


#ifdef	__cplusplus
extern "C" {
#endif

#ifndef BOOL
typedef int BOOL;
#endif


/*************************************************************************
椭圆曲线初始化函数
函数 : tcm_ecc_init()
返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_init();

/*************************************************************************
椭圆曲线释放资源函数
函数 : tcm_ecc_release()
返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_release();


/*************************************************************************
基于EC的加密算法
函数 : tcm_ecc_encrypt()
输入参数  
		plaintext:		明文地址
		uPlaintextLen:	明文长度
		pubkey:			公钥地址
		uPubkeyLen：	公钥长度
		puCiphertextLen：输入的ciphertext缓冲区的大小
输出参数 
		ciphertext: 密文地址
		puCiphertextLen : 密文长度地址指针，指明输出的密文长度
返回值：加密成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_encrypt(unsigned char *plaintext, unsigned int uPlaintextLen, unsigned char *pubkey, unsigned int uPubkeyLen, unsigned char *ciphertext, unsigned int *puCiphertextLen);

/*************************************************************************
基于EC的解密算法
func : tcm_ecc_decrypt()
输入参数  
 		ciphertext:		密文地址
		uCiphertextLen:	密文长度
		prikey:			私钥地址
		uPrikeyLen：	私钥长度
		puPlaintextLen: 输入的plaintext缓冲区的大小

输出参数 
		plaintext:		输出明文
		puCiphertextLen : 明文长度地址指针，指明输出的明文长度

返回值：解密成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_decrypt(unsigned char *ciphertext, unsigned int uCiphertextLen, unsigned char *prikey, unsigned int uPrikeyLen, unsigned char *plaintext, unsigned int *puPlaintextLen);

/*************************************************************************
基于EC椭圆曲线的数字签名算法
函数 : tcm_ecc_signature()
输入参数  
 		digest:			需要被签名的摘要
		uDigestLen:		摘要长度
		prikey:			私钥地址
		uPrikeyLen：	私钥长度
		puSigLen:		输入的sig缓冲区大小

输出参数 
		sig:			输出签名
		puSigLen:		签名长度地址指针，指明输出的签名长度

返回值：签名成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_signature( unsigned char *digest, unsigned int uDigestLen,
					   unsigned char *prikey, unsigned int uPrikeyLen, 
					   /*out*/unsigned char *sig,
					   /*out*/unsigned int *puSigLen);

/*************************************************************************
基于EC椭圆曲线的数字签名验证算法
函数 : tcm_ecc_verify()
输入参数  
 		digest:			需要被签名的摘要
		uDigestLen:		摘要长度
		sig:			签名
		uSigLen:		签名的长度
		pubkey:			公钥
		uPubkeyLen:		公钥长度

输出参数 
		（无）

返回值：验证成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_verify(unsigned char *digest, unsigned int uDigestLen, unsigned char *sig, unsigned int uSigLen, unsigned char *pubkey, unsigned int uPubkeyLen);



/*************************************************************************
基于EC椭圆曲线的判断比特串是否表示曲线上的一个点，
函数 : tcm_ecc_is_point_valid()
输入参数  
 		point:			椭圆曲线上的点，必须是非压缩形式
		uPointLen:		点的长度
输出参数 
		（无）

返回值：验证成功返回TRUE，否则返回FALSE
其他：可以使用tcm_ecc_point_to_uncompressed函数得到非压缩形式的串
*************************************************************************/
BOOL tcm_ecc_is_point_valid(unsigned char *point, unsigned int uPointLen);


/*************************************************************************
基于EC椭圆曲线的判断比特串是否表示曲线上的一个点，如果是，转变为非压缩形式
函数 : tcm_ecc_point_to_uncompressed()
输入参数  
 		point:			椭圆曲线上的点
		uPointLen:		点的长度
		puUncompressedpointLen：	输入的uncompressedpoint缓冲区大小指针

输出参数 
		uncompressedpoint:			点的非压缩形式
		puUncompressedpointLen：	返回uncompressedpoint缓冲区长度
返回值：转换成功返回TRUE，否则返回FALSE
*************************************************************************/
BOOL tcm_ecc_point_to_uncompressed(unsigned char *point, unsigned int uPointLen,
								unsigned char *uncompressedpoint, unsigned int *puUncompressedpointLen);


/*************************************************************************
基于EC椭圆曲线密钥对生成函数
函数 : tcm_ecc_genkey()

输入参数
		（无）
输出参数  
 		prikey:			私钥
		puPrikeyLen:	私钥地址
		pubkey:			公钥
		puPubkeyLen:	公钥地址	

返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_genkey(unsigned char *prikey, unsigned int *puPrikeyLen, unsigned char *pubkey, unsigned int *puPubkeyLen);



/*************************************************************************
基于EC的国标交换密钥算法。仅用于firmware的测试函数，只考虑256bit情况
函数 : tcm_ecc_exchange()
输入参数  
		fA:				当为TRUE时，为发起方；否则为响应方
		prikey_A：		
						当fa为TRUE时，为发起方的静态密钥私钥；
						否则，为响应方的静态密钥私钥。
		pubkey_A：
						当fa为TRUE时，为发起方的静态密钥共钥；
						否则，为响应方的静态密钥共钥。
		prikey_RA：		
						当fa为TRUE时，为发起方的临时密钥私钥；
						否则，为响应方的临时密钥私钥；
		pubkey_RA
						当fa为TRUE时，为发起方的临时密钥共钥；
						否则，为响应方的临时密钥共钥。
		pubkey_B
						当fa为TRUE时，为响应方的静态密钥共钥；
						否则，为发起方的静态密钥共钥。
		pubkey_RB
						当fa为TRUE时，为响应方的临时密钥共钥；
						否则，为发起方的临时密钥共钥。
		Za
						主动方用户的摘要信息
		Zb
						响应方用户的摘要信息
输出参数 
 						
		key				输出的密钥
		S1 
						当fa为TRUE时，为响应方的S1；
						否则，为发起方的SB。
		Sa
						当fa为TRUE时，为响应方的Sa；
						否则，为发起方的S2。

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_exchange(BOOL fA,
					 unsigned char prikey_A[32], unsigned char pubkey_A[65],
					 unsigned char prikey_RA[32], unsigned char pubkey_RA[65],
					 unsigned char pubkey_B[65], unsigned char pubkey_RB[65], 
					 unsigned char Za[32], unsigned char Zb[32],
					 /*out*/unsigned char key[16],
					 /*out*/unsigned char S1[32], 
					 /*out*/unsigned char Sa[32]
					 );
/************************************************************************************/


/*************************************************************************
基于EC椭圆曲线的从私钥得到公钥
函数 : tcm_ecc_point_from_privatekey()
输入参数  
 		prikey:			私钥
		uPrikeyLen:		私钥长度
输出参数 
		pubkey:			公钥
		puPubkeyLen:	公钥长度
返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_point_from_privatekey(const unsigned char *prikey, const unsigned int uPrikeyLen, unsigned char *pubkey, unsigned int *puPubkeyLen);

/*************************************************************************
基于EC椭圆曲线的判断公私钥对是否匹配
函数 : tcm_ecc_is_match()
输入参数  
 		prikey:			椭圆曲线上的点，必须是非压缩形式
		uPrikeyLen:		私钥长度
		pubkey:			公钥
		uPubkeyLen:		公钥长度
输出参数 
		（无）

返回值：成功返回TRUE，否则返回FALSE
其他：本函数只用于测试。
*************************************************************************/
BOOL tcm_ecc_is_key_match(const unsigned char *prikey, const unsigned int uPrikeyLen, 
						  const unsigned char *pubkey, const unsigned int uPubkeyLen);

/************************************************************************************/
/************************************************************************************/
/* sch函数 */
#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
//#define uint32 unsigned long int
#define uint32 unsigned int
#endif

typedef struct
{
    uint32 total[2];
    uint32 state[8];
    uint8 buffer[64];
}
sch_context;

/* sch */
void tcm_sch_starts( sch_context *ctx );
void tcm_sch_update( sch_context *ctx, uint8 *input, uint32 length );
void tcm_sch_finish( sch_context *ctx, uint8 digest[32] );

/*************************************************************************
函数 : tcm_sch_hash()
输入参数  
		length:		消息长度
		input:		消息

输出参数 
		digest:			输出HASH值
返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_sch_hash( unsigned int datalen_in, unsigned char *pdata_in, unsigned char digest[32]);


/*************************************************************************
hmac函数
函数 : tcm_hmac()
输入参数  
		text:			消息
		text_len:		消息长度
 		key:			密钥
		key_len:		密钥长度

输出参数 
		digest:			输出HMAC值

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_hmac(unsigned char *text, unsigned int text_len, unsigned char *key, unsigned int key_len, unsigned char digest[32]);


/*************************************************************************
密钥派生函数
函数 : tcm_kdf()
输入参数  
		klen:			需要输出的长度，调用前必须为mask分配大于klen的空间
		msgLen:			消息长度
 		z:				公享的种子信息
		zlen:			z的长度

输出参数 
		mask:			输出的缓冲区

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_kdf(/*out*/unsigned char *mask, /*in*/int klen, /*in*/unsigned char *z, /*in*/ int zlen);


/*************************************************************************
计算消息的hash值
函数 : tcm_get_message_hash()
输入参数  
		msg:			消息
		msgLen:			消息长度
 		userID:			用户信息
		uUserIDLen:		用户信息长度
		pubkey:			公钥地址
		uPubkeyLen:		公钥长度
		puDigestLen:	输入的digest缓冲区大小

输出参数 
		digest:			输出摘要
		puDigestLen;	输出的digest缓冲区长度

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_get_message_hash(unsigned char *msg, unsigned int msgLen,  
					   unsigned char  *userID, unsigned short int uUserIDLen, 
					   unsigned char *pubkey, unsigned int uPubkeyLen,
					   unsigned char *digest,
					   unsigned int *puDigestLen);


/*************************************************************************
计算Za，总是使用256bits的HASH算法
函数 : tcm_get_usrinfo_value()
输入参数  
 		userID:			用户信息
		uUserIDLen:		用户信息长度
		pubkey:			公钥地址
		uPubkeyLen:		公钥长度

输出参数 
		digest:			输出摘要

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_get_usrinfo_value(
					   unsigned char *userID, unsigned short int uUserIDLen, 
					   unsigned char *pubkey, unsigned int uPubkeyLen,
					   unsigned char digest[32]);


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/* sms4加解函数 */


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

/* sms4的块长 */
#define	SMS4_BLOCK_SIZE 16
/* sms4最大加密的明文字节长度，同时也用于ECC */
#define SMS4_MAX_LEN	512

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


#endif	// HEADER_TCM_ALG_H
