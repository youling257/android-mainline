/******************************************
File	:tcm_ecc.c 
Author	:linyang
Date	:11/21/2006
******************************************/

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/ec_operations.h"
#include "openssl/crypto.h"
#include "tcm_ecc.h"
#include "tcm_bn.h"
//#include <malloc.h>
#include <linux/vmalloc.h>
//#include <string.h>
#include <linux/string.h>

/*
基于EC的加密算法
输入：明文pPlaintext_in，明文长度plaintextLen_in
      对方公钥pPubkey_in, 对方公钥长度pubkeyLen_in
      sta端mac地址pstamac
输出：密文pCipher_out，密文长度pCipherLen_out
返回：加密成功返回0，否则返回1
其它：
*/
int tcm_ecc_encrypt(unsigned char *pPlaintext_in, unsigned int plaintextLen_in, unsigned char *pPubkey_in, unsigned int pubkeyLen_in, unsigned char *pCipher_out, unsigned int *pCipherLen_out)
{
//返回的密文长度
#define CIPHER_LEN (1+2*g_uNumbits/8 + plaintextLen_in+ HASH_NUMBITS/8)

	unsigned int i;
	unsigned char*	pstr_r=NULL;
	unsigned char*	pstr_s=NULL;

	
	BIGNUM	*x, *y, *one;
	EC_POINT	*P;


	/* 检查明文是否有效 */
	if( (pPlaintext_in==NULL) || (plaintextLen_in <= 0) )
	{
		return 1;
	}

	/*检查密文空间是否有效*/
	if( pCipher_out==NULL )
	{
		return 1;
	}
	if( *pCipherLen_out < CIPHER_LEN )
	{
		return 1;
	}


	/* 检查公钥pubkeyLen_in长度 */
	if (pubkeyLen_in != PUBKEY_LEN)
	{
		//uiPrintf("****pubkeyLen_in\n");
		return 1;
	}
	/* 检查公钥pPubkey_in是否为空 */
	if (pPubkey_in ==NULL)
	{
		//uiPrintf("*****pPubkey_in ==NULL\n");
		return 1;
	}
	//如果不是非压缩形式，返回失败
	if( pPubkey_in[0] != 0x04 )
	{
		return 1;
	}

	//为签名分配内存
	pstr_r=(unsigned char*)vmalloc(g_uNumbits/8);
	pstr_s=(unsigned char*)vmalloc(g_uNumbits/8);
	//
	x = BN_new();
	y = BN_new();
	one = BN_new();
	P = EC_POINT_new();

	if ( pstr_r == NULL || pstr_s == NULL ||
		x == NULL || y == NULL || one == NULL || P == NULL)
	{
		return 1;
	}

	/* 将公钥字符串(48字节)分成两部分(24, 24)字节 */
	for (i = 0; i < (g_uNumbits/8); i++) {
		pstr_r[i] = pPubkey_in[1+i];
		pstr_s[i] = pPubkey_in[1+g_uNumbits/8 + i];
	}

	/* 将两部分(g_uNumbits/8, g_uNumbits/8)字节串转换为大数 */
	BN_bin2bn(pstr_r, g_uNumbits/8, x);
	BN_bin2bn(pstr_s, g_uNumbits/8, y);

	BN_hex2bn(&one, "1");
	
	/* 生成公钥点P */
	EC_POINT_set_point(P, x, y, one);
	if (!(ECC_Encrypt(pCipher_out, group, G, P, pPlaintext_in, plaintextLen_in)))
	{
		/* 密文长度 */
		*pCipherLen_out = CIPHER_LEN;
#ifdef TEST_FIXED
	{
		//明文为固定19个字节
		*pCipherLen_out = (1+2*g_uNumbits/8 + 19+ HASH_NUMBITS/8);
	}
#endif


		//释放内存
		vfree(pstr_r);
		vfree(pstr_s);
		//
		BN_free(x);
		BN_free(y);
		BN_free(one);
		EC_POINT_free(P);
		//		
		return 0;
	}
	else
	{
		//释放内存
		vfree(pstr_r);
		vfree(pstr_s);
		//
		BN_free(x);
		BN_free(y);
		BN_free(one);
		EC_POINT_free(P);
		//		
		return 1;
	}
}

/*
基于EC的解密算法
输入：密文pCipher_in，密文长度cipherLen_in
      私钥pPrikey_in , 私钥长度prikeyLen_in
输出：明文pPlaintext_out，明文长度pPlaintextLen_out
返回：解密成功返回0，否则返回1
其它： 
*/
int tcm_ecc_decrypt(unsigned char *pCipher_in, unsigned int cipherLen_in, unsigned char *pPrikey_in, unsigned int prikeyLen_in, unsigned char *pPlaintext_out, unsigned int *pPlaintextLen_out)
{

//返回的明文长度
#define PLAIN_LEN (cipherLen_in - (1+2*g_uNumbits/8 + HASH_NUMBITS/8) )

	BIGNUM *skey;
	//检查明文指针是否有效
	if( pPlaintext_out == NULL )
	{
		return 1;
	}
	//检查明文长度是否合格
	if( *pPlaintextLen_out < PLAIN_LEN )
	{
		return 1;
	}
	//
	/* 检查密文是否为空 */
	if (pCipher_in == NULL)
	{
		return 1;
	}

	//这里只处理非压缩形式
	if (pCipher_in[0] != 04)
	{
		return 1;
	}


	/* 检查密文cipherLen_in长度 */
	/* 如果刚好等于1+2*g_uNumbits/8 + HASH_NUMBITS/8，则无明文 */
	if( cipherLen_in< (1+2*g_uNumbits/8 + HASH_NUMBITS/8) )
	{
		return 1;
	}


	/* 检查私钥是否为空 */
	if (pPrikey_in == NULL)
	{
		return 1;
	}
	/* 检查私钥prikeyLen_in长度 */
	if (prikeyLen_in != g_uNumbits/8)
	{
		return 1;
	}

	skey = BN_new();

	if (skey == NULL)
	{
      	return 1;
	}

	/* 转换私钥字符串为大数 */
	BN_bin2bn(pPrikey_in, g_uNumbits/8, skey);
	if (!(ECC_Decrypt(pPlaintext_out, group, pCipher_in, cipherLen_in, skey)))
	{
		*pPlaintextLen_out = PLAIN_LEN;
		BN_free(skey);
		return 0;
	}
	else
	{
		*pPlaintextLen_out = 0;
		BN_free(skey);
		return 1;
	}
}

/*
基于EC椭圆曲线的数字签名算法
输入：待签名摘要值pDigest，待签名摘要值长度uDigestLen
      私钥pPrikey_in，私钥长度prikeyLen_in
      sta端mac地址pstamac
输出：返回签名结果sigData
      签名后数据长度puSigDataLen
返回：签名成功返回0，否则返回1
其它：
*/
int tcm_ecc_signature(	   unsigned char *pDigest, unsigned int uDigestLen,
						   unsigned char *pPrikey_in, unsigned int prikeyLen_in, 
						   /*out*/unsigned char *pSigData,
						   /*out*/unsigned int *puSigDataLen
						   )
{
	BIGNUM *skey;

	if( pSigData == NULL )
	{
		return 1;
	}
	//签名长度不够，返回失败
	if( *puSigDataLen < 2*g_uSCH_Numbits/8 )
	{
		*puSigDataLen = 2*g_uSCH_Numbits/8;
		return 1;
	}

	//如果输入的hash值指针指向NULL，返回错误
	if( pDigest==NULL )
	{
		return 1;

	}
	//如果需要签名的hash值长度不一致，返回错误。
	if( uDigestLen != g_uSCH_Numbits/8 )
	{
		return 1;
	}

	/* 检查私钥是否为空 */
	if (pPrikey_in == NULL)
	{
		return 1;
	}

	/* 检查私钥prikeyLen_in长度 */
	if (prikeyLen_in != g_uNumbits/8)
	{
		return 1;
	}

	//

	skey = BN_new();

	if ( skey == NULL)
	{
		return 1;
	}

	/* 转换私钥字符串为大数 */
	BN_bin2bn(pPrikey_in, g_uNumbits/8, skey);
	
	if (!(ECC_Signature(pSigData, group, G, skey, pDigest)))
	{
		*puSigDataLen = 2*g_uSCH_Numbits/8;
		BN_free(skey);
		return 0;
	}
	else
	{
		*puSigDataLen = 0;
		BN_free(skey);
		return 1;
	}
}

/*
基于EC椭圆曲线的数字签名验证算法
输入：明文数据pData_in，明文数据长度dataLen_in
      签名数据pSigndata_in, 签名数据长度signdataLen_in
      公钥pPubkey_in，公钥长度pubkeyLen_in
输出：无 
返回：验证算法成功返回0，否则返回1
其它：
*/
int tcm_ecc_verify(unsigned char *pDigest, unsigned int uDigestLen, unsigned char *pSigndata_in, unsigned int signdataLen_in, unsigned char *pPubkey_in, unsigned int pubkeyLen_in)
{
	unsigned int i;
	unsigned char*	pstr_r = NULL;
	unsigned char*	pstr_s = NULL;
	
	BIGNUM	*x, *y, *one;
	EC_POINT	*P;
	
	//如果输入的hash值指针指向NULL，返回错误
	if( pDigest==NULL )
	{
		return 1;

	}
	//如果需要验证的hash值长度不一致，返回错误。
	if( uDigestLen != g_uSCH_Numbits/8 )
	{
		return 1;
	}
	//


	/* 检查签名数据是否为空 */
	if (pSigndata_in == NULL)
	{
		 //uiPrintf("*****pSigndata_in == NULL\n");
		return 1;
	}

	/* 检查签名数据signdataLen_in的长度 */
	if (signdataLen_in != 2 * (g_uNumbits/8))
	{
		 //uiPrintf("*****signdataLen_in:%d\n",signdataLen_in);
		return 1;
	}


	/* 检查公钥pubkeyLen_in长度 */
	if (pubkeyLen_in != PUBKEY_LEN)
	{
		//uiPrintf("****pubkeyLen_in\n");
		return 1;
	}
	/* 检查公钥pPubkey_in是否为空 */
	if (pPubkey_in ==NULL)
	{
		//uiPrintf("*****pPubkey_in ==NULL\n");
		return 1;
	}
	//如果不是非压缩形式，返回失败
	if( pPubkey_in[0] != 0x04 )
	{
		return 1;
	}


	x = BN_new();
	y = BN_new();
	one = BN_new();
	P = EC_POINT_new();
	//
	pstr_r = (unsigned char*)vmalloc(g_uNumbits/8);
	pstr_s = (unsigned char*)vmalloc(g_uNumbits/8);

	if ( x == NULL || y == NULL || one == NULL || P == NULL ||
		pstr_r == NULL || pstr_s == NULL  )

	{
		return 1;
	}
 

	/* 将公钥字符串(48字节)分成两部分(g_uNumbits/8, g_uNumbits/8)字节 */
	for (i = 0; i < (g_uNumbits/8); i++) 
	{
		pstr_r[i] = pPubkey_in[1+i];
		pstr_s[i] = pPubkey_in[1+g_uNumbits/8 + i];
	}

	/* 将两部分(g_uNumbits/8, g_uNumbits/8)字节串转换为大数 */
	BN_bin2bn(pstr_r, g_uNumbits/8, x);
	BN_bin2bn(pstr_s, g_uNumbits/8, y);

	BN_hex2bn(&one, "1");
 
   
	/* 生成公钥点P */
	EC_POINT_set_point(P, x, y, one);
	if (!(ECC_Verify(group, G, P, pDigest, pSigndata_in)))
	{
		BN_free(x);
		BN_free(y);
		BN_free(one);
		EC_POINT_free(P);
		//
		vfree(pstr_r);
		vfree(pstr_s);
		//
		return 0;
	}
	else
	{
		BN_free(x);
		BN_free(y);
		BN_free(one);
		EC_POINT_free(P);
		//
		vfree(pstr_r);
		vfree(pstr_s);
		//
		return 1;
	}
  
}


/*
判断是否奇数
输入：比特串，串字节长度
输出：无 
返回：奇数返回1，偶数返回0。
	其他错误返回-1
其它：
*/
int tcm_ecc_string_is_odd(unsigned char *string,  unsigned int len)
{
	int iret;
	BIGNUM	*x;
	//
	x = BN_new();
	if( x == NULL )
	{
		return -1;
	}
	BN_bin2bn(string, len, x);
	iret = BN_is_odd(x);
	BN_free(x);
	return iret;
}

/*
根据x比特串计算y比特串
输入：x比特串，y比特串，串字节长度，压缩模式
输出：无 
返回：返回：成功返回0，否则返回1
其它：
*/
int tcm_ecc_x_to_y(unsigned char *xstr, unsigned char *ystr, unsigned int len,
				   unsigned int form)
{

	// y^2=x^3+ax+b
	BIGNUM *x, *y, *z, *a, *b;
	BIGNUM *tmp, *left, *right;
	BN_CTX *ctx;
	const BIGNUM *p=&(group->p);
	int y_bit;
	int iret;

	//
	x=BN_new();
	y=BN_new();
	z=BN_new();
	a=BN_new();
	b=BN_new();
	//
	tmp=BN_new();
	left=BN_new();
	right=BN_new();
	//
	ctx = BN_CTX_new();

	//
	BN_copy(a,&(group->a));
	BN_copy(b,&(group->b));
	//

	
	if ( x == NULL || y == NULL || a == NULL || b == NULL || 
		tmp == NULL || left == NULL || right == NULL ||		
		ctx == NULL )
	{
		return 1;
	}

	BN_bin2bn(xstr, len, x);

	if( form == 02 || form == 06 )
	{
		y_bit = 0;
	}
	else if( form == 03 || form == 07 )
	{
		y_bit = 1;
	}
	else
	{
		iret = 1;
		goto end;
	}

//	BN_nnmod(x,x,p,ctx);
	// tmp := x^3 
	BN_mod_sqr(tmp, x, p, ctx);
	BN_mod_mul(tmp, tmp, x, p, ctx);

	
	//
	BN_copy(right, tmp);

	// tmp := ax
	BN_mod_mul(tmp,a,x,p,ctx);

	// x^3+ax+b
	BN_mod_add(right, right, tmp, p, ctx);
	BN_mod_add(right, right, b, p, ctx);
	// 计算计算完毕右边


	//计算y
	if( !BN_mod_sqrt(y, right, p, ctx) )
	{
		//没有平方根，返回错误
		iret = 1;
		goto end;
	}


	if (y_bit != BN_is_odd(y))
	{
		if (BN_is_zero(y))
		{
			iret = 1;
			goto end;
		}
		if (!BN_usub(y, p, y))
		{
			iret = 1;
			goto end;
		}
		if (y_bit != BN_is_odd(y))
		{
			iret = 1;
			goto end;
		}
	}


	tcm_bn_bn2bin(y, len, ystr);

	iret = 0;
end:
	//
	BN_free(x);
	BN_free(y);
	BN_free(z);
	BN_free(a);
	BN_free(b);
	//
	BN_free(tmp);
	BN_free(left);
	BN_free(right);
	//
	BN_CTX_free(ctx);

	return iret;

}

/*
 基于EC椭圆曲线的判断比特串是否表示曲线上的一个点，
  如果是，返回TRUE
  否则返回FALSE
  这个比特串必须是非压缩形式，可以使用tcm_ecc_point_to_uncompressed函数得到非压缩形式的串
*/
BOOL tcm_ecc_is_point_valid(unsigned char *pPoint, unsigned int pointLen)
{
#define	UNCOMP_LEN		(1 + 2*g_uNumbits/8)

	unsigned char *pstr_x = NULL;
	unsigned char *pstr_y = NULL;

//nt iret;
	BOOL bret;

	if( pointLen != UNCOMP_LEN )
	{
		return 0;
	}

	//为x,y坐标分配内存
	if( (pstr_x = (unsigned char*)vmalloc(g_uNumbits/8)) == NULL )
	{
		return FALSE;
	}
	if( (pstr_y = (unsigned char*)vmalloc(g_uNumbits/8)) == NULL )
	{
		vfree(pstr_x);
		return FALSE;
	}

	//判断是否非压缩形式
	if( pPoint[0]!= 04 )
	{
		vfree(pstr_x);
		vfree(pstr_y);
		return FALSE;
	}

	//
	memcpy( pstr_x, &pPoint[1], g_uNumbits/8 );
	memcpy( pstr_y, &pPoint[1+g_uNumbits/8], g_uNumbits/8 );
	

	//下面判断点是否在曲线上
	{
		BIGNUM	*x, *y, *z;
		EC_POINT *P;

		x = BN_new();
		y = BN_new();
		z = BN_new();
		P = EC_POINT_new();
		//

		BN_bin2bn(pstr_x, g_uNumbits/8, x);
		BN_bin2bn(pstr_y, g_uNumbits/8, y);
		BN_hex2bn(&z, "1");

		//
		EC_POINT_set_point(P, x, y, z);
		/* 检查C1是否满足曲线方程 */
		bret = EC_POINT_is_on_curve(group, P);
		//		
		BN_free(x);
		BN_free(y);
		BN_free(z);
		EC_POINT_free(P);
	}

	//释放分配的内存
	vfree(pstr_x);
	vfree(pstr_y);

	//
	return bret;
}


/*
基于EC椭圆曲线的判断比特串是否表示曲线上的一个点
输入：点的字节串pData_in，字节串长度dataLen_in

输出：无 
返回：是曲线上的一个点返回0，否则返回1。
	如果是曲线上的点，返回点的非压缩形式。
其它：可对0点进行有效分析
*/
BOOL tcm_ecc_point_to_uncompressed(unsigned char *pPoint, unsigned int pubkeyLen_in,
								unsigned char *string, unsigned int *puStringLen)
{
#define ZERO_LEN		1
#define	COMP_LEN		(1 + g_uNumbits/8)
#define	UNCOMP_LEN		(1 + 2*g_uNumbits/8)

	unsigned char *pstr_x = NULL;
	unsigned char *pstr_y = NULL;

	BOOL bret;


	if( (pubkeyLen_in != ZERO_LEN) && (pubkeyLen_in != COMP_LEN) && (pubkeyLen_in != UNCOMP_LEN) )
	{
		return FALSE;
	}

	if( (string == NULL )  || *puStringLen < UNCOMP_LEN )
	{
		return FALSE;
	}

	//为x,y坐标分配内存
	if( (pstr_x = (unsigned char*)vmalloc(g_uNumbits/8)) == NULL )
	{
		return FALSE;
	}
	if( (pstr_y = (unsigned char*)vmalloc(g_uNumbits/8)) == NULL )
	{
		vfree(pstr_x);
		return FALSE;
	}


	switch(pPoint[0])
	{
		case 00:
			{
				//0点不需要分析x,y
				//释放分配的内存
				vfree(pstr_x);
				vfree(pstr_y);
				//
				if( pubkeyLen_in != ZERO_LEN )
				{
					return FALSE;
				}
				//0点返回0
				string[0] = 0;
				*puStringLen = 1;
				return TRUE;
			}


		case 02:
		case 03:
			{
				if( pubkeyLen_in != COMP_LEN )
				{
					//释放分配的内存
					vfree(pstr_x);
					vfree(pstr_y);
					//
					return FALSE;
				}
				memcpy(pstr_x, &pPoint[1], g_uNumbits/8 );
				if( tcm_ecc_x_to_y(pstr_x, pstr_y, g_uNumbits/8, pPoint[0]) == 1 )
				{
					//释放分配的内存
					vfree(pstr_x);
					vfree(pstr_y);
					//
					return FALSE;
				}
				break;
			}


		case 06:	//混合压缩形式，Y必须为偶数
		case 07:	//混合压缩形式，Y必须为奇数
			{
				//
				if( (pPoint[0] == 06) && (tcm_ecc_string_is_odd(pPoint, 1+2*g_uNumbits/8) != 0) )
				{
					//释放分配的内存
					vfree(pstr_x);
					vfree(pstr_y);
					//
					return FALSE;
				}
				//
				if( (pPoint[0] == 07) &&  (tcm_ecc_string_is_odd(pPoint, 1+2*g_uNumbits/8) != 1) )
				{
					//释放分配的内存
					vfree(pstr_x);
					vfree(pstr_y);
					//
					return FALSE;
				}
			}
		case 04:	//非压缩模式
			{
				if( pubkeyLen_in != UNCOMP_LEN )
				{
					//释放分配的内存
					vfree(pstr_x);
					vfree(pstr_y);
					//
					return FALSE;
				}
				memcpy(pstr_x, &pPoint[1], g_uNumbits/8);
				memcpy(pstr_y, &pPoint[1+g_uNumbits/8], g_uNumbits/8);
				break;
			}
		default:	//不支持这种模式
			{
				//释放分配的内存
				vfree(pstr_x);
				vfree(pstr_y);
				//
				return FALSE;
			}
	}

	

	//下面判断点是否在曲线上
	{
		BIGNUM	*x, *y, *z;
		EC_POINT *P;

		x = BN_new();
		y = BN_new();
		z = BN_new();
		P = EC_POINT_new();
		//

		BN_bin2bn(pstr_x, g_uNumbits/8, x);
		BN_bin2bn(pstr_y, g_uNumbits/8, y);
		BN_hex2bn(&z, "1");

		//
		EC_POINT_set_point(P, x, y, z);
		/* 检查C1是否满足曲线方程 */
		bret = EC_POINT_is_on_curve(group, P);
		if( bret == TRUE )
		{
			//设置为未压缩形式
			string[0] = (unsigned char)04;
			//设置x与y
			memcpy(&string[1], pstr_x, g_uNumbits/8);
			memcpy(&string[1+g_uNumbits/8], pstr_y, g_uNumbits/8);
		}
		BN_free(x);
		BN_free(y);
		BN_free(z);
		EC_POINT_free(P);
	}

	//释放分配的内存
	vfree(pstr_x);
	vfree(pstr_y);

	//
	return bret;
}
