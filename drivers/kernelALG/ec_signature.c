/******************************************
File	:ec_signature.c 
Author	:linyang
Date	:11/21/2006
comment :国标算法
******************************************/

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "openssl/ec_operations.h"
#include "tcm_rand.h"
#include "tcm_bn.h"
//#include <malloc.h>
#include <linux/vmalloc.h>
/*
基于EC椭圆曲线的数字签名算法
输出：签名字符串sig = Mr || Ms
输入：group，包括p, a, b, N
输入：基点G(Gx,Gy)
输入：签名私钥da
输入：待签名的消息msg
输入：待签名消息的长度msgLen
输入：sta端的mac地址
*/
int ECC_Signature(/*out*/unsigned char *pSignature,
				  const EC_GROUP *group, 
				  const EC_POINT *G, 
				  const BIGNUM *da, 
				  /*in*/unsigned char *pDigest)
{
	unsigned char*	pTemp_k = NULL;		/* 随机数 */

	BIGNUM 		*e;			/* 哈希杂凑值对应的GF(p)中的整数 */
	BIGNUM		*k;			/* 随机数对应的GF(p)中的整数 */
	BIGNUM 		*i, *tmp;
	BIGNUM 		*Vy;
	BIGNUM 		*N;			/* 阶数 */
	BIGNUM 		*s;
	BIGNUM 		*r;

	EC_POINT 	*V; 
	BN_CTX 		*ctx = BN_CTX_new();

	//
	k = BN_new();
	Vy = BN_new();
	i = BN_new();
	s = BN_new();
	r = BN_new();
	N = BN_new();
	tmp=BN_new();
	e=BN_new();
	V = EC_POINT_new();
	//
	pTemp_k = (unsigned char*)vmalloc(RANDOM_LEN);
	//

	if ( e == NULL || k == NULL || Vy == NULL || i == NULL || s == NULL ||
		 r == NULL || N == NULL || tmp == NULL || V == NULL || ctx == NULL ||
		 pTemp_k == NULL)
	{
		return 1;
	}
	
	EC_GROUP_get_order(group,N);
	/* A2 */
	/* e=CH(M) */
	/* 将哈希杂凑值转换为大整数 */
	/* 使用密钥长度，方便后面计算 */
	BN_bin2bn(pDigest, g_uNumbits/8, e);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(e);
		printf("e: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* A3 */
	/* 产生随机数 */
step3:
	tcm_rng(g_uNumbits/8, pTemp_k );
	BN_bin2bn(pTemp_k, g_uNumbits/8, k);	
	/* 确使k属于[1, N-1] */
	BN_nnmod(k, k, N, ctx);
	/* 这里还必须判断是否为0 */
	if( BN_is_zero(k) )
	{
#ifdef TEST
		printf("k is zeor\n");
#endif
		goto step3;
	}

#ifdef TEST_FIXED
{
	// 使用固定随机数
	if( g_uNumbits == 256 )
	{
		// for Fp-256
		BN_hex2bn(&k,"6CB28D99385C175C94F94E934817663FC176D925DD72B727260DBAAE1FB2F96F");
	}
	else
	{
		// for Fp-192
		BN_hex2bn(&k,"79443C2BB962971437ACB6246EA7E14165700FD733E14569");
	}
}
#endif
	
	/* A4 */
	/* 计算椭圆曲线点 (x,y) = [k]G */
	EC_POINT_mul(group, V, k, G);
	if (EC_POINT_is_at_infinity(group,V))
		goto step3;

	/* 仿射坐标到几何坐标 */
	EC_POINT_affine2gem(group, V, V);
	EC_POINT_get_point(V, i, Vy, tmp);

#ifdef TEST
	{

		char *str;
		EC_POINT_get_point(V, i, Vy, tmp);
		str = BN_bn2hex(i);
		printf("x1: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(Vy);
		printf("y1: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(tmp);
		printf("z1: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* A5 */
	/* 计算 r=(e+x1) mod n */
	BN_add(r, e, i);
	BN_nnmod(r, r, N, ctx);
	/* 若 r=0 或r+k=n，则返回A3 */
	if(BN_is_zero(r))
		goto step3;

	/* 测试是否r+k=n，如果是则返回A3 */
	BN_add(tmp, r, k);
	if(BN_cmp(tmp, N) == 0 )
		goto step3;


#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(r);
		printf("r: %s\n",str);
		OPENSSL_free(str);
	}
#endif


	/* A6 */
	/* 计算 (1+da) 的逆乘(k-rda) mod n */

	/* 计算 k-rda */
	BN_mul(tmp, r, da, ctx);
	BN_sub(s, k, tmp);
	/* 计算 (1+da) 的逆 */
	BN_dec2bn(&i,"1");
	BN_add(tmp, i, da);
	BN_div_mod(s, s, tmp, N);


#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(s);
		printf("s: %s\n",str);
		OPENSSL_free(str);
	}
#endif
	
	/* A7 */
	/* 消息M的签名为 (r,s) */
	BN_lshift(r,r,8*g_uNumbits/8);
	BN_add(r,r,s);
	//
	tcm_bn_bn2bin(r, 2*g_uSCH_Numbits/8, pSignature);

  	BN_free(e);
	BN_free(Vy);
	BN_free(i);
	BN_free(k);
	BN_free(s);
	BN_free(N);
	BN_free(tmp);
	BN_free(r);
	BN_CTX_free(ctx);
	EC_POINT_free(V);
	//
	vfree(pTemp_k);
	
	return 0;
}
