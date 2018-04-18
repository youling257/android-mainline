/******************************************
File	:ec_verify.c 
Author	:linyang
Date	:11/21/2006
comment :国标算法
******************************************/

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "openssl/ec_operations.h"
//#include <malloc.h>
#include <linux/vmalloc.h>

/*
基于EC椭圆曲线的数字签名验证算法
输出：如果签名有效，输出0，否则输出1
输入：group，包括p, a, b, N
输入：基点G(Gx,Gy)
输入：签名公钥Pa = (Wx, Wy)
输入：被签名的消息msg
输入：被签名消息长度msgLen
输入：被验证的数字签名字节串sig = Mr || Ms
*/
int ECC_Verify(const EC_GROUP *group,const EC_POINT *G,const EC_POINT *Pa,
			   unsigned char *pDigest,
			   unsigned char *pSignature)
{
	unsigned char*	pstr_r=NULL;
	unsigned char*	pstr_s=NULL;

	unsigned int	j;

	BIGNUM 		*e;			/* 哈希杂凑值对应的GF(p)中的整数 */
	BIGNUM 		*x;
	BIGNUM 		*N;			/* 阶数 */
	BIGNUM 		*s;
	BIGNUM      *r;
	BIGNUM		*one;
	BIGNUM		*ztmp;
	BIGNUM		*t;
	BIGNUM		*R;

	BN_CTX 		*ctx = BN_CTX_new();
	
	EC_POINT 	*P0;
	EC_POINT	*P00;
	EC_POINT	*P;

	if(ctx == NULL)
	{
		//uiPrintf("****1\n");
		return 1;
	}
   
	e = BN_new();
	if(e == NULL)
	{   //uiPrintf("****2\n");
		BN_CTX_free(ctx);
		return 1;
	}
	
	x = BN_new();
	if(x == NULL)
	{  
		BN_CTX_free(ctx);
		BN_free(e);
		return 1;
	}
	s = BN_new();
	if(s == NULL)
	{   
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		return 1;
	} 
	r = BN_new();
	if(r == NULL)
	{   
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	 
		return 1;
	}
	one = BN_new();
	if(one == NULL)
	{   
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		return 1;
	}
	t = BN_new();
	if(t == NULL)
	{   
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		return 1;
	}

	R = BN_new();
	if(R == NULL)
	{   //	uiPrintf("****4\n");
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		return 1;
	}
	ztmp = BN_new();
	if(ztmp == NULL)
	{   
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		return 1;
	}
	N = BN_new();
	if(N == NULL)
	{ 	//uiPrintf("****5\n");  
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		return 1;
	}
	//
	pstr_r = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_r == NULL)
	{ 	//uiPrintf("****5\n");  
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		BN_free(N);
		return 1;
	}
	pstr_s = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_s == NULL)
	{ 	//uiPrintf("****5\n");  
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		BN_free(N);
		vfree(pstr_r);
		return 1;
	}


     /*ASSERT(group);*/
	EC_GROUP_get_order(group,N);	/* 阶 */
	 	
	P = EC_POINT_new();
	if(P == NULL)
	{  	//uiPrintf("****6\n"); 
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		BN_free(N);
		return 1;
	}
	P0 = EC_POINT_new();
	if(P0 == NULL)
	{   //	uiPrintf("****7\n");
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		BN_free(N);
		EC_POINT_free(P);
		return 1;
	}
	P00 = EC_POINT_new();
	if(P00 == NULL)
	{   //	uiPrintf("****7\n");
		BN_CTX_free(ctx);
		BN_free(e);
		BN_free(x);
		BN_free(s);
	    BN_free(r);
		BN_free(one);
		BN_free(t);
		BN_free(R);
		BN_free(ztmp);
		BN_free(N);
		EC_POINT_free(P);
		EC_POINT_free(P0);
		return 1;
	}


	/* 得到r与s */
	for (j = 0; j < g_uNumbits/8; j++) {
		pstr_r[j] = pSignature[j];
		pstr_s[j] = pSignature[g_uNumbits/8 + j];
	}

	BN_bin2bn(pstr_r,g_uNumbits/8,r);
	BN_bin2bn(pstr_s,g_uNumbits/8,s);

	BN_hex2bn(&one, "1");
	
	/* B1 */
	/*检验 1 <= r <= n-1 */
	/* B2 */
	/*检验 1 <= s <= n-1 */
	if(BN_cmp(r, one) == -1) goto invalid;	/* r < 1 */
	if(BN_cmp(s, one) == -1) goto invalid;	/* s < 1 */
	if(BN_cmp(r, N) == 1) goto invalid;		/* r > N-1 */
	if(BN_cmp(s, N) == 1) goto invalid;		/* s > N-1 */
	
#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(r);
		printf("r: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(s);
		printf("s: %s\n",str);
		OPENSSL_free(str);

	}
#endif  

	/* B3 */
	/* B4 */
	/* 设置需要验证的hash数据 */
	BN_bin2bn(pDigest, g_uNumbits/8, e);
#ifdef TEST
{
	char *str;
	str = BN_bn2hex(e);
	printf("e: %s\n",str);
	OPENSSL_free(str);
}
#endif  


	/* B5 */
	/* t = r+s mod n*/
	/* 如果 t=0 验证不通过 */
	BN_add(t, r, s);
	BN_nnmod(t, t, N, ctx);
	if(BN_is_zero(t))
		goto invalid;	/* t == 0 */
	

	/* B6 */
	/* (x1, y1)=[s]G+[t]Pa */
	EC_POINT_mul(group, P0, s, G);
	EC_POINT_mul(group, P00, t, Pa);
	EC_POINT_add(group, P, P0, P00);


	/* 坐标变换 */
	EC_POINT_affine2gem(group, P, P);
	EC_POINT_get_point(P, x, R, ztmp);

#ifdef TEST
	{
		char *str;

		//
		EC_POINT_affine2gem(group, P0, P0);
		EC_POINT_get_point(P0, x, R, ztmp);

		str = BN_bn2hex(x);
		printf("x0: %s\n", str);
		OPENSSL_free(str);

		str = BN_bn2hex(R);
		printf("y0: %s\n",str);
		OPENSSL_free(str);
		//
		//
		EC_POINT_affine2gem(group, P00, P00);
		EC_POINT_get_point(P00, x, R, ztmp);

		str = BN_bn2hex(x);
		printf("x00: %s\n", str);
		OPENSSL_free(str);

		str = BN_bn2hex(R);
		printf("y00: %s\n",str);
		OPENSSL_free(str);
		//
		//
		EC_POINT_affine2gem(group, P, P);
		EC_POINT_get_point(P, x, R, ztmp);

		str = BN_bn2hex(x);
		printf("x1: %s\n", str);
		OPENSSL_free(str);

		str = BN_bn2hex(R);
		printf("y1: %s\n",str);
		OPENSSL_free(str);
		//
	}
#endif  

	/* B7 */
	/* R=(e+x) mod n */
	BN_add(R, e, x);
	BN_nnmod(R, R, N, ctx);

#ifdef TEST
	{
		char *str;
		//
		str = BN_bn2hex(R);
		printf("R: %s\n", str);
		OPENSSL_free(str);
	}
#endif  


	//uiPrintf("*****1111111111111111111111\n");
	/* step 7 */

	/* 判断 R=r ，不相同验证失败*/
	if (BN_cmp(R, r) == 0) 
	{
		BN_free(e);
		BN_free(x);
		BN_free(N);
		BN_free(s);
		BN_free(r);
		BN_free(one);
		BN_free(ztmp);
		BN_free(t);
		BN_free(R);

		BN_CTX_free(ctx);

		EC_POINT_free(P0);
		EC_POINT_free(P00);
		EC_POINT_free(P);

		//
		vfree(pstr_r);
		vfree(pstr_s);

		return 0;
	}

invalid:
	      
		BN_free(e);
		BN_free(x);
		BN_free(N);
		BN_free(s);
		BN_free(r);
		BN_free(one);
		BN_free(ztmp);
		BN_free(t);
		BN_free(R);

		BN_CTX_free(ctx);

		EC_POINT_free(P0);
		EC_POINT_free(P00);
		EC_POINT_free(P);
		//
		vfree(pstr_r);
		vfree(pstr_s);

		return 1;
}
