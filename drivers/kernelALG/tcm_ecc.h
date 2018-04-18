/* tcm_ecc.h*/
/* 这个文档中的函数需要被导出 */

#ifndef HEADER_TCM_ECC_H
#define HEADER_TCM_ECC_H


#ifdef	__cplusplus
extern "C" {
#endif
#ifndef BOOL
typedef int BOOL;
#endif
/* 
1. 返回值为int类型的函数，0表示成功，1表示失败。
2. 返回值为BOOL类型的函数，TRUE表示成功，FALSE表示失败 
*/


/*************************************************************************
椭圆曲线初始化函数
函数 : tcm_ecc_init()
返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_init(void);

/*************************************************************************
椭圆曲线释放资源函数
函数 : tcm_ecc_release()
返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_release(void);


/* 用于测试国标算法素数域256bit */
int tcm_ecc_init_test256(void);
/* 用于测试国标算法素数域192bit */
int tcm_ecc_init_test192(void);



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
基于EC的私钥生成算法
func : ECC_gen_d()
输入参数  
 		prikey:			私钥缓冲区
		puPrikeyLen:	输入的prikey缓冲区长度

输出参数 
 		prikey:			返回的私钥
		puPrikeyLen:	输出的prikey大小

返回值：解密成功返回0，否则返回1
*************************************************************************/
int ECC_gen_d(unsigned char *prikey, unsigned int *puPrikeyLen);

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

#ifdef	__cplusplus
}
#endif


#endif	// HEADER_TCM_ECC_H
