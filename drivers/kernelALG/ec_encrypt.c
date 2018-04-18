/******************************************
File	:ec_encrypt.c 
Author	:linyang
Date	:11/21/2006
comment :国标算法
******************************************/

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "tcm_rand.h"
#include "debug.h"
#include "tcm_hash.h"
#include "tcm_bn.h"
//#include <malloc.h>
#include <linux/vmalloc.h>
//#include <memory.h>


/*
基于EC的加密算法
功能：实体A对消息msg加密，把加密后的信息cipher发给实体B
输出：加密后的信息cipher
输入：group，包括p,a,b,N
输入：basepoint G(Gx,Gy)
输入：B的公钥Pb
输入：消息msg
输入：消息msg的长度msgLen
输入：sta端的mac地址
*/
int ECC_Encrypt(unsigned char *cipher,const EC_GROUP *group,const EC_POINT *G,const EC_POINT *Pb,unsigned char *msg, const int msgLen)
{
    BIGNUM *x1,*y1;
	BIGNUM *k,*x2,*y2,*z2,*m,*N,*pc;
	BIGNUM *c2, *c3,*ctemp;
	BIGNUM *h, *h1;
	EC_POINT *Pt;
	unsigned char* pstr_xy=NULL;
	unsigned char* pstr_t=NULL;
	unsigned char* pstr_h=NULL;
	unsigned char mac[HASH_NUMBITS/8];
	int iret;
	int i;
	/* x2||M||y2 的长度 */
	const int hLen = g_uNumbits/8 + msgLen + g_uNumbits/8;
	//
	unsigned char *pTemp_k;	/* 随机数 */

	//
	BN_CTX *ctx= BN_CTX_new();

	/* 检查公钥是否为空 */
	if (Pb == NULL)
	{
		BN_CTX_free(ctx);
		return 1;
	}
	x1=BN_new();
	y1=BN_new();
	x2=BN_new();
	y2=BN_new();
	z2=BN_new();
	ctemp=BN_new();
	k=BN_new();
	m=BN_new();
	N=BN_new();
	pc=BN_new();
	c2=BN_new();
	c3=BN_new();
	h=BN_new();
	h1=BN_new();
	//
	Pt=EC_POINT_new();
	//
	pTemp_k = (unsigned char*)vmalloc(RANDOM_LEN);
	//
	pstr_xy = (unsigned char*)vmalloc(2*(g_uNumbits/8));
	//分配明文空间，做kdf操作并且加密
	pstr_t = (unsigned char *)vmalloc(msgLen);
	//分配空间，准备进行hash运算
	// x2||M||y2
	pstr_h = (unsigned char *)vmalloc(hLen);
	//
	if ( x1 == NULL || y1 == NULL || x2 == NULL || y2 == NULL || z2 == NULL ||
		 ctemp == NULL || k == NULL || m == NULL || N == NULL || 
		 pc == NULL || Pt == NULL || ctx == NULL ||
		 c2 == NULL || c3 == NULL ||
		 h == NULL || h1 == NULL ||
		 pTemp_k == NULL ||
		 pstr_xy == NULL || pstr_t == NULL || pstr_h == NULL)
	{
		return 1;
	}

	EC_GROUP_get_order(group,N);	/* 阶 */
//	BN_copy(p,&group->p);			/* p为大素数 */

	/* A1 */
	/* 产生随机数 */
step1:
	tcm_rng( g_uNumbits/8, pTemp_k );
	BN_bin2bn(pTemp_k, g_uNumbits/8, k);	
	/* 确使k属于[1, N-1] */
	BN_nnmod(k, k, N, ctx);			
	/* 这里还必须判断是否为0 */
	if( BN_is_zero(k) )
	{
#ifdef TEST
		printf("k is zeor\n");
#endif
		goto step1;
	}

#ifdef TEST_FIXED
{
	if( g_uNumbits == 256 )
	{
		//for Fp-256
		BN_hex2bn(&k,"4C62EEFD6ECFC2B95B92FD6C3D9575148AFA17425546D49018E5388D49DD7B4F");
	}
	else
	{
		//for Fp-192
		BN_hex2bn(&k,"384F30353073AEECE7A1654330A96204D37982A3E15B2CB5");
	}
}
#endif

	/* A2 */
	/* 计算 C1 = [k]G = (x1, y1) */
	EC_POINT_mul(group,Pt,k,G);
	EC_POINT_affine2gem(group,Pt,Pt);
	EC_POINT_get_point(Pt,x1,y1,z2);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(x1);
		printf("x1: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(y1);
		printf("y1: %s\n",str);
		OPENSSL_free(str);
	}
#endif


	/* A3 */
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


	BN_mod_mul(ctemp,h,h1,N,ctx);

	str = BN_bn2hex(ctemp);
	printf("h1*h mod n: %s\n",str);
	OPENSSL_free(str);
}
#endif

	/* 计算 [h]Pb */
	EC_POINT_mul(group,Pt,h,Pb);

	/* 计算[k*h^(-1)] */
	/* 临时使用ctemp */
	BN_mul(ctemp,k,h1,ctx);

	/* 计算 [k*h^(-1)][h]Pb */
	EC_POINT_mul(group,Pt,ctemp,Pt);
	EC_POINT_affine2gem(group,Pt,Pt);
	EC_POINT_get_point(Pt,x2,y2,z2);
	/*******************************/
	/* 优化算法，用于h为1的情况 */
	/* S = [k*h^(-1)][h]Pb = (x2,y2) */
//	EC_POINT_mul(group,Pt,k,Pb);
//	EC_POINT_affine2gem(group,Pt,Pt);
//	EC_POINT_get_point(Pt,x2,y2,z2);
	/*******************************/
	/* 如果是无穷远点，返回失败 */
	if( EC_POINT_is_at_infinity(group, Pt) )
	{
		iret = 1;
		goto end;
	}
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

	/* X2||Y2 */
	tcm_bn_bn2bin(x2, g_uNumbits/8, pstr_xy);
	tcm_bn_bn2bin(y2, g_uNumbits/8, &pstr_xy[g_uNumbits/8]);

	/* A4 */
	/* t = KDF(x2||y2, msgLen) */
	iret = tcm_kdf(pstr_t, msgLen, pstr_xy, 2*(g_uNumbits/8));
#ifdef TEST_FIXED
{
	if( g_uNumbits == 256 )
	{
		//for Fp-256
		const unsigned char tArray[] = { 0xCA, 0xD8, 0xBA, 0xB1, 0x11, 0x21, 0xB6, 
					0x1C, 0x4E, 0x98, 0x2C, 0xD7, 0xFC, 0x25, 0xC1,
					0x4F, 0x67, 0xEC, 0x79};

		ASSERT( msgLen == sizeof(tArray) );
		memcpy(pstr_t, tArray, sizeof(tArray));
	}
	else
	{
		//for Fp-192
		const unsigned char tArray[] = { 0x6B, 0x8F, 0x54, 0xC0, 0x34, 0x69, 0x9C,
					0x61, 0x09, 0x7F, 0xA4, 0xEF, 0xBB, 0x53, 0x19, 
					0xE9, 0x5E, 0xD4, 0x60};

		ASSERT( msgLen == sizeof(tArray) );
		memcpy(pstr_t, tArray, sizeof(tArray));
	}

}
#endif

	/* 如果t为全0比特串，返回A1 */
	BN_bin2bn(pstr_t, msgLen, z2);
	if( BN_is_zero(z2) )
	{
		//释放加密后的密文空间
#ifdef TEST
		printf("pT is zeor\n");
#endif
		goto step1;
	}
#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(z2);
		printf("t: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* A5 */
	/* 进行异或计算 C2=M^t */
	for(i=0;i<msgLen;i++)
	{
		pstr_t[i] ^= (unsigned char)msg[i];
	}
	BN_bin2bn(pstr_t, msgLen, c2);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(c2);
		printf("c2: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	/* A6 */
	/* 计算C3 */
	/* C3 = Hash(x2||M||y2) */
	BN_bin2bn(msg, msgLen, m);
	BN_copy(ctemp,x2);
	BN_lshift(ctemp,ctemp, msgLen*8);
	BN_add(ctemp,ctemp,m);
	BN_lshift(ctemp,ctemp, g_uNumbits);
	BN_add(ctemp,ctemp,y2);

	//
	tcm_bn_bn2bin(ctemp, hLen, pstr_h);

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(ctemp);
		printf("x2||M||y2: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	tcm_sch_hash(hLen, pstr_h, mac);
	vfree(pstr_h);
	pstr_h=NULL;
	//
	BN_bin2bn(mac, sizeof(mac), c3);


#ifdef TEST_FIXED
{
	if( g_uNumbits == 256 )
	{
		//for Fp-256
		BN_hex2bn(&c3, "E1C8D1101EDE0D3430ACCDA0C9E45901BAA902BD44B03466930840210766195C");
	}
	else
	{
		//for Fp-192
		BN_hex2bn(&c3, "8DCF8E4DC8C92FCBA5A2DDCE0A3BED07588A6A634AAA09216D098954FDBD6A51");
	}
}
#endif

#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(c3);
		printf("c3: %s\n",str);
		OPENSSL_free(str);
	}
#endif


	/* A7 */
	/* 输出密文 C=C1||C2||C3 */

	BN_hex2bn(&pc,"4");
	//
	BN_lshift(pc,pc,g_uNumbits);
	BN_add(pc,pc,x1);
	//
	BN_lshift(pc,pc,g_uNumbits);
	BN_add(pc,pc,y1);
	//
	BN_lshift(pc,pc,msgLen*8);
	BN_add(pc,pc,c2);
	//
	BN_lshift(pc,pc, HASH_NUMBITS);
	BN_add(pc,pc,c3);
	//
	tcm_bn_bn2bin(pc, 1+g_uNumbits/8+g_uNumbits/8+msgLen+HASH_NUMBITS/8, cipher);
	
#ifdef TEST
	{
		char *str;
		str = BN_bn2hex(pc);
		printf("C=C1||C2||C3: %s\n",str);
		OPENSSL_free(str);
	}
#endif

	iret = 0;
end:
    BN_free(x1);
	BN_free(y1);
	BN_free(k);
	BN_free(x2);
	BN_free(y2);
	BN_free(z2);
    BN_free(m);
	BN_free(N);
	BN_free(ctemp);
	BN_free(pc);

	BN_free(c2);
	BN_free(c3);

	BN_free(h);
	BN_free(h1);

	EC_POINT_free(Pt);
	BN_CTX_free(ctx);
	//
	//free(pstr_xy);
	//free(pstr_t);
	
	return iret;
}
