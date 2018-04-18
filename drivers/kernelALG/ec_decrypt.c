/******************************************
File	:ec_decrypt.c 
Author	:linyang
Date	:11/21/2006
comment :国标算法
******************************************/

//#include <string.h>
#include <linux/string.h>
//#include <malloc.h>
#include <linux/vmalloc.h>
#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "openssl/ec_operations.h"
#include "tcm_bn.h"
#include "debug.h"
#include "tcm_hash.h"

/*
基于EC的解密算法
功能：实体B对收到的加密信息{(x1, y1), cipher}解密
输出：解密后的信息msg
解密后的消息长度根据密文长度动态算出
输入：group，包括p, a, b, N
输入：B的私钥kb
输入：加密信息{(x1, y1), cipher}
*/
int ECC_Decrypt(unsigned char *msg,const EC_GROUP *group,unsigned char *cipher, unsigned int cipherLen, const BIGNUM *kb)
{
	BIGNUM *x1,*y1,*one;
	BIGNUM *x2,*y2,*z2,*m,*c;
	BIGNUM *u;
	BIGNUM *h, *h1;
	BIGNUM *N;
	EC_POINT *C1,*S;

	//明文长度
	const int klen= (cipherLen - (1+2*g_uNumbits/8 + HASH_NUMBITS/8) );
	// pstr_h的长度，下面定义
	const int hLen = g_uNumbits/8 + klen + g_uNumbits/8;		// x2||M||y2
	int iret;

	//
	unsigned char* pstr_x1;
	unsigned char* pstr_y1;
	unsigned char* pstr_c;
	//
	unsigned char* pstr_xy;
	//
	unsigned char mac_in[HASH_NUMBITS/8];	//输入的hash
	unsigned char mac_u[HASH_NUMBITS/8];	//计算后得到的
	//
	unsigned char* pstr_t=NULL;	//为x2||y2分配的内存
	unsigned char* pstr_h=NULL;	//用于计算Hash(x2||M||y2)

	//
	int i;
	
	BN_CTX *ctx= BN_CTX_new();
	//

	/* 检查密文是否为空 */
	if (cipher == NULL)
	{
		return 1;
	}

	/* 检查私钥是否为空 */
	if (kb == NULL)
	{
		return 1;
	}

	x1=BN_new();
	y1=BN_new();
	x2=BN_new();
	y2=BN_new();
	z2=BN_new();
	one=BN_new();
	m=BN_new();
	c=BN_new();
	u=BN_new();
	//
	h=BN_new();
	h1=BN_new();
	//
	N=BN_new();
	//

	C1=EC_POINT_new();
	S=EC_POINT_new();
	//

	pstr_x1 = (unsigned char*)vmalloc(g_uNumbits/8);
	pstr_y1 = (unsigned char*)vmalloc(g_uNumbits/8);
	pstr_c = (unsigned char*)vmalloc(klen);
	//
	pstr_xy = (unsigned char*)vmalloc(2*(g_uNumbits/8));
	//
	//分配空间，准备加密
	pstr_t = (unsigned char *)vmalloc(klen);
	// x2||M||y2
	//分配空间，准备进行hash运算
	pstr_h = (unsigned char *)vmalloc(hLen);

	//
	if ( ctx == NULL ||
		 x1 == NULL || y1 == NULL || x2 == NULL || y2 == NULL ||
		 z2 == NULL || one == NULL || m == NULL || c == NULL ||
		 C1 == NULL || S == NULL || u == NULL ||
		 h == NULL || h1 == NULL || N == NULL ||
		 pstr_x1 == NULL || pstr_y1 == NULL || pstr_c == NULL ||
		 pstr_t == NULL || pstr_h == NULL  )
	{
		return 1;
	}

	EC_GROUP_get_order(group,N);	/* 阶 */
//	BN_copy(p,&group->p); /* 大素数 */

	/* B1 */
	/* step1:从cipher中提取x1,y1 */
	memcpy(pstr_x1,cipher+1,g_uNumbits/8);
	memcpy(pstr_y1,cipher+1+g_uNumbits/8,g_uNumbits/8);
	/* 提取密文 */
	memcpy(pstr_c,cipher+1+g_uNumbits/8+g_uNumbits/8,klen);
	/* 提取mac */
	memcpy(mac_in,cipher+1+g_uNumbits/8+g_uNumbits/8+klen,sizeof(mac_in));


	BN_bin2bn(pstr_x1,g_uNumbits/8,x1);
	BN_bin2bn(pstr_y1,g_uNumbits/8,y1);
	BN_bin2bn(pstr_c,g_uNumbits/8,c);

	BN_hex2bn(&one,"1");
	EC_POINT_set_point(C1,x1,y1,one);

	/* 检查C1是否满足曲线方程 */
	if( EC_POINT_is_on_curve(group, C1) == FALSE )
	{
		iret = 1;
		goto end;
	}

	/* B2 */
	/*******************************/
	/* 得到余因子 */
	EC_GROUP_get_cofactor(group, h);

	/* 得到余因子的逆 */
	BN_mod_inverse(h1, h, N, ctx);
	BN_nnmod(h1,h1,N,ctx);

#ifdef TEST
{
	//检查 h1是否是h的逆
	char *str;

	str = BN_bn2hex(h);
	printf("h: %s\n", str);
	OPENSSL_free(str);

	BN_mod_mul(z2,h,h1,N,ctx);

	str = BN_bn2hex(z2);
	printf("h1*h mod n: %s\n",str);
	OPENSSL_free(str);
}
#endif

	/* 计算 [h]C1 */
	EC_POINT_mul(group,S,h,C1);

	/* 计算[Db*h^(-1)] */
	/* 临时使用z2 */
	BN_mul(z2,kb,h1,ctx);

	/* 计算 [Db*h^(-1)][h]C1 */
	EC_POINT_mul(group,S,z2,S);
	EC_POINT_affine2gem(group,S,S);
	/*******************************/
	/* 优化算法，用于h为1的情况 */
	/* S=[Db*h^-1][h]C1=(x2,y2) */
	/* step2: */
//	EC_POINT_mul(group,S,kb,C1);
//	EC_POINT_affine2gem(group,S,S);
	/*******************************/
	/* 如果是无穷远点，返回失败 */
	if( EC_POINT_is_at_infinity(group, S) )
	{
		iret = 1;
		goto end;
	}
	EC_POINT_get_point(S,x2,y2,z2);
#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(x2);
		printf("x2: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(y2);
		printf("y2: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* B3 */
	/* 计算 t=KDF(x2||y2, klen) */	
	
	/* X2||Y2 */
	tcm_bn_bn2bin(x2, g_uNumbits/8, &pstr_xy[0]);
	tcm_bn_bn2bin(y2, g_uNumbits/8, &pstr_xy[(g_uNumbits/8)]);
	iret = tcm_kdf(pstr_t, klen, pstr_xy, 2*(g_uNumbits/8));
	//
#ifdef TEST_FIXED
{
	if( g_uNumbits == 256 )
	{
		const unsigned char tArray[] = { 0xCA, 0xD8, 0xBA, 0xB1, 0x11, 0x21, 0xB6, 
					0x1C, 0x4E, 0x98, 0x2C, 0xD7, 0xFC, 0x25, 0xC1,
					0x4F, 0x67, 0xEC, 0x79};

		ASSERT( klen == sizeof(tArray) ); 
		memcpy(pstr_t, tArray, sizeof(tArray));
	}
	else
	{
		const unsigned char tArray[] = { 0x6B, 0x8F, 0x54, 0xC0, 0x34, 0x69, 0x9C,
					0x61, 0x09, 0x7F, 0xA4, 0xEF, 0xBB, 0x53, 0x19, 
					0xE9, 0x5E, 0xD4, 0x60};
		ASSERT( klen == sizeof(tArray) ); 
		memcpy(pstr_t, tArray, sizeof(tArray));
	}
}
#endif

	BN_bin2bn(pstr_t, klen, z2);
	/* 如果t为全0比特串，错误返回 */
	if( BN_is_zero(z2) )
	{

#ifdef TEST
		printf("t is zeor\n");
#endif
		iret = 1;
		goto end;
	}
#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(z2);
		printf("t: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* B4 */
	/* 进行异或计算 M=C2^t */
	for(i=0;i<klen;i++)
	{
		pstr_c[i] ^= (unsigned char)pstr_t[i];
	}
	BN_bin2bn(pstr_c, klen, m);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(m);
		printf("M: %s\n",str);
		OPENSSL_free(str);
	}
#endif
	

	/* B5 */
	/* 计算u = Hash(x2||M||y2) */
	BN_copy(u,x2);
	BN_lshift(u,u, klen*8);
	//
	BN_add(u,u,m);
	BN_lshift(u,u, g_uNumbits);
	//
	BN_add(u,u,y2);

	//
	tcm_bn_bn2bin(u, hLen, pstr_h);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(u);
		printf("x2||M||y2: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	tcm_sch_hash(hLen, pstr_h, mac_u);
	//

#ifdef TEST_FIXED
{
	if( g_uNumbits == 256 )
	{
		BN_hex2bn(&u, "E1C8D1101EDE0D3430ACCDA0C9E45901BAA902BD44B03466930840210766195C");
		tcm_bn_bn2bin(u, mac_u);
	}
	else
	{
		//for Fp-192
		BN_hex2bn(&u, "8DCF8E4DC8C92FCBA5A2DDCE0A3BED07588A6A634AAA09216D098954FDBD6A51");
		tcm_bn_bn2bin(u, mac_u);
	}
}
#endif

	//比较hash，如果不同，返回失败
	for( i=0;i<sizeof(mac_u);i++)
	{
		if( mac_in[i]!=mac_u[i] )
		{

			iret = 1;
#ifdef TEST
		printf("mac校验失败\n");
#endif

			goto end;
		}
	}
		

	//输出明文
	memcpy(msg,pstr_c,klen);

	iret = 0;
end:
	BN_free(x1);
	BN_free(y1);
	BN_free(one);
	BN_free(x2);
	BN_free(y2);
	BN_free(z2);

	BN_free(m);
    BN_free(c);

    BN_free(u);

	//
	BN_free(h);
	BN_free(h1);
	BN_free(N);
	//


	EC_POINT_free(C1);
	EC_POINT_free(S);
	//
	vfree(pstr_x1);
	vfree(pstr_y1);
	vfree(pstr_c);
	//
	vfree(pstr_xy);
	vfree(pstr_t);

	return iret;
}
