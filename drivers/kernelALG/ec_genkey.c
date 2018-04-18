/* ec_genkey.c */

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "openssl/ec_operations.h"
#include "tcm_rand.h"
#include "tcm_ecc.h"
#include "tcm_bn.h"
//#include <malloc.h>
//#include <memory.h>
#include <linux/vmalloc.h>


/*************************************************************************
基于EC的私钥生成算法
func : ECC_gen_d()
输入参数  
 		prikey:			私钥缓冲区
		puPrikeyLen:	输入的prikey缓冲区长度

输出参数 
 		prikey:			返回的私钥
		puPrikeyLen:	输出的prikey大小

返回值：功返回0，否则返回1
*************************************************************************/
int ECC_gen_d(unsigned char *prikey, unsigned int *puPrikeyLen)
{
	unsigned char*	pTemp_k = NULL;		/* 随机数 */
	BIGNUM 		*N;					/* 阶数 */
	BIGNUM		*kt;
	BN_CTX 		*ctx;
	//
	// 输入的长度参数必须大于私钥长度与公钥长度
	if( prikey == NULL  || *puPrikeyLen < g_uNumbits/8 )
	{
		return 1;
	}
	//

	N = BN_new();
	kt = BN_new();
	ctx= BN_CTX_new();
	pTemp_k = (unsigned char*)vmalloc(RANDOM_LEN);
	//
	if ( kt == NULL || ctx == NULL || pTemp_k == NULL )
	{
		return 1;
	}

	EC_GROUP_get_order(group, N);

	/* step 1 */
step1:

	tcm_rng(g_uNumbits/8, pTemp_k );
	BN_bin2bn(pTemp_k, g_uNumbits/8, kt);
	BN_nnmod(kt, kt, N, ctx);					/* 确使kt属于[1, N-1] */
	/* kt必须满足在区间[1, n-1] */
	/* 这里还必须判断是否为0 */
	if( BN_is_zero(kt) )
	{
#ifdef TEST
		printf("kt is zeor\n");
#endif
		goto step1;
	}
	tcm_bn_bn2bin(kt, g_uNumbits/8, prikey);					/* 大数转换为私钥 */
	*puPrikeyLen = g_uNumbits/8;
	//
	BN_free(N);
	BN_free(kt);
	BN_CTX_free(ctx);
	vfree(pTemp_k);

	return 0;
}


/*
ECC密钥对生成函数
输入：sta端的mac地址
输出：私钥pPrikey_out，私钥长度pPrikeyLen_out
      公钥pPubkey_out, 公钥长度pPubkeyLen_out
	  返回非压缩形式
返回：成功返回0，否则返回1
*/
int tcm_ecc_genkey(unsigned char *prikey, unsigned int *puPrikeyLen, unsigned char *pubkey, unsigned int *puPubkeyLen)
{
	int iret;
	/* 先生成私钥 */
	iret = ECC_gen_d(prikey, puPrikeyLen);
	if( iret == 1 )
		return 1;

	iret = tcm_ecc_point_from_privatekey(prikey, *puPrikeyLen, pubkey, puPubkeyLen);

	if( iret == 1 )
		return 1;
	return 0;
}


/*************************************************************************
基于EC椭圆曲线的从私钥得到公钥
函数 : tcm_ecc_point_from_privatekey()
输入参数  
 		prikey:			私钥
输出参数 
		pubkey:			公钥

返回值：成功返回0，否则返回1
*************************************************************************/
int tcm_ecc_point_from_privatekey(const unsigned char *prikey, const unsigned int uPrikeyLen, unsigned char *pubkey, unsigned int *puPubkeyLen)
{
	BIGNUM 		*N;					/* 阶数 */
	BIGNUM		*kt;
	BIGNUM		*x;
	BIGNUM		*y;
	EC_POINT	*Pt;
	EC_POINT	*Pz;
	//
	BN_CTX 		*ctx;
	//

	if( prikey == NULL || pubkey == NULL || puPubkeyLen == NULL )
	{
		return 1;
	}
	// 输入的长度参数必须大于私钥长度与公钥长度
	if( (uPrikeyLen != g_uNumbits/8) || (*puPubkeyLen < 1 + 2 * g_uNumbits/8) )
	{
		return 1;
	}


	ctx= BN_CTX_new();
	N = BN_new();
	kt = BN_new();
	x = BN_new();
	y = BN_new();
	Pt = EC_POINT_new();
	Pz = EC_POINT_new();
	//

	if ( N == NULL || kt == NULL || x == NULL || y == NULL ||
		 Pt == NULL || Pz == NULL || ctx == NULL 
		)
	{
		return 1;
	}

	EC_GROUP_get_order(group, N);

	/* step 1 */
	BN_bin2bn(prikey, g_uNumbits/8, kt);
	/* 确使kt属于[1, N-1], 是否为0暂时不判断 */
	BN_nnmod(kt, kt, N, ctx);

	/* step 2 */
	EC_POINT_mul(group, Pt, kt, G);		/* 计算公钥 */
	EC_POINT_affine2gem(group, Pt, Pz);	
	EC_POINT_get_point(Pz, x, y, kt);

/*#ifdef TEST
{
		char *str;
		str = BN_bn2hex(x);
		printf("x: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(y);
		printf("y: %s\n",str);
		OPENSSL_free(str);
}
#endif*/

	BN_hex2bn(&kt, "04");
	//
	BN_lshift(kt, kt, g_uNumbits);
	BN_add(kt, kt, x);
	//
	BN_lshift(kt, kt, g_uNumbits);
	BN_add(kt, kt, y);
	//
	tcm_bn_bn2bin(kt, 1 + 2 * g_uNumbits/8, pubkey);

	/* 设置返回的公钥大小 */
	*puPubkeyLen = 1 + 2 * g_uNumbits/8;

#ifdef TEST
{
		char *str;
		str = BN_bn2hex(kt);
		printf("04||x||y: %s\n",str);
		OPENSSL_free(str);
}
#endif

	BN_free(N);
	BN_free(kt);
	BN_free(x);
	BN_free(y);
	EC_POINT_free(Pt);
	EC_POINT_free(Pz);
	BN_CTX_free(ctx);

	return 0;
}


/*************************************************************************
基于EC椭圆曲线的判断公私钥对是否匹配
函数 : tcm_ecc_is_match()
输入参数  
 		prikey:			椭圆曲线上的点，必须是非压缩形式
		pubkey:			私钥
输出参数 
		（无）

返回值：成功返回TRUE，否则返回FALSE
其他：本函数只用于测试。这里只用于小于256bit(包括)的曲线。
*************************************************************************/
BOOL tcm_ecc_is_key_match(const unsigned char *prikey, const unsigned int uPrikeyLen, 
						  const unsigned char *pubkey, const unsigned int uPubkeyLen)
{
	int iret;
	unsigned char pubkey_temp[65];
	unsigned int uPubkey_tempLen = 65;

	if( prikey == NULL || pubkey == NULL)
	{
		return 1;
	}
	if( (uPrikeyLen != g_uNumbits/8) || (uPubkeyLen != 1 + 2 * g_uNumbits/8) )
	{
		return 1;
	}

	iret = tcm_ecc_point_from_privatekey(prikey, uPrikeyLen, pubkey_temp, &uPubkey_tempLen);
	if( iret == 1 )
		return FALSE;
	if( memcmp(pubkey_temp, pubkey, uPubkey_tempLen) != 0 )
		return FALSE;
	return TRUE;
}
