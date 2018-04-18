/******************************************
File	:tcm_ecc_exchange.c 
Author	:linyang
Date	:12/23/2006
Comment :用于测试firmware的密钥协商功能
******************************************/

#include "openssl/bn.h"
#include "openssl/ec_operations.h"
#include "openssl/crypto.h"
#include "tcm_ecc.h"
#include "tcm_hash.h"
#include "tcm_bn.h"
#include <linux/string.h>

/*************************************************************************
基于EC的国标交换密钥算法。仅用用于firmware的测试函数，只考虑256bit情况
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
					 )
{
	unsigned int i;
	unsigned char str_x[32]={0};
	unsigned char str_y[32]={0};
	//
	unsigned char str_buffer[7*32]={0};
	unsigned char str_temp65[65]={0};
	unsigned char str_temp32[32]={0};


	BIGNUM 	*N;			/* 阶数 */
	BIGNUM	*one;
	BIGNUM	*da;
	BIGNUM	*ra;
	BIGNUM	*ta;
	BIGNUM	*x1, *y1, *x_1, *r_A;
	BIGNUM	*x2, *y2, *x_2;
	BIGNUM	*x_temp, *y_temp;
	BIGNUM	*xu, *yu;
	BIGNUM	*bn_127, *temp;

	EC_POINT *P_PB, *P_RB, *P_temp;

	BN_CTX *ctx;

	/* 只验证256vbit的密钥交换协议 */
	if( g_uNumbits != 256 )
		return 1;

	//三个公钥需要是非压缩形式，否则返回失败
	if( pubkey_A[0] != 0x04 ||
		pubkey_B[0] != 0x04 ||
		pubkey_RB[0] != 0x04)
	{
		return 1;
	}

	//
	ctx= BN_CTX_new();
	//
	N=BN_new();
	//
	one = BN_new();
	//
	da = BN_new();
	ra = BN_new();
	ta = BN_new();
	//
	x1 = BN_new();
	y1 = BN_new();
	x_1 = BN_new();
	r_A = BN_new();
	//
	x2 = BN_new();
	y2 = BN_new();
	x_2 = BN_new();
	//
	x_temp = BN_new();
	y_temp = BN_new();
	//
	xu = BN_new();
	yu = BN_new();
	//
	temp = BN_new();
	bn_127 = BN_new();
	//
	P_PB = EC_POINT_new();
	P_RB = EC_POINT_new();
	P_temp = EC_POINT_new();

	if ( ctx == NULL || N == NULL ||
		one == NULL ||
		da == NULL || ra == NULL || ta == NULL ||
		x1 == NULL || y1 == NULL || x_1 == NULL || r_A == NULL ||
		x2 == NULL || y2 == NULL || x_2 == NULL ||
		x_temp == NULL || y_temp == NULL ||
		xu == NULL || yu == NULL ||
		temp == NULL || bn_127 == NULL ||
		P_PB == NULL ||
		P_RB == NULL ||
		P_temp == NULL
	)
	{
		return 1;
	}

	EC_GROUP_get_order(group,N);	/* 阶 */
//	BN_copy(p,&group->p); /* 大素数 */

	/* 取得r_A */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = prikey_RA[i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, r_A);

	/* 取得x1 */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = pubkey_RA[1+i];
		str_y[i] = pubkey_RA[1+g_uNumbits/8+i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, x1);
	BN_bin2bn(str_y, g_uNumbits/8, y1);

	/* 取得x2 */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = pubkey_RB[1+i];
		str_y[i] = pubkey_RB[1+g_uNumbits/8+i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, x2);
	BN_bin2bn(str_y, g_uNumbits/8, y2);

	/* 将公钥字符串(48字节)分成两部分(24, 24)字节 */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = pubkey_B[1+i];
		str_y[i] = pubkey_B[1+g_uNumbits/8 + i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, x_temp);
	BN_bin2bn(str_y, g_uNumbits/8, y_temp);
	BN_hex2bn(&one, "1");
	/* 生成公钥点Pb */
	EC_POINT_set_point(P_PB, x_temp, y_temp, one);

	/* 将公钥字符串(48字节)分成两部分(24, 24)字节 */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = pubkey_RB[1+i];
		str_y[i] = pubkey_RB[1+g_uNumbits/8 + i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, x_temp);
	BN_bin2bn(str_y, g_uNumbits/8, y_temp);
	BN_hex2bn(&one, "1");
	/* 生成公钥点Rb */
	EC_POINT_set_point(P_RB, x_temp, y_temp, one);


	/* 得到da */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = prikey_A[i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, da);

	/* 得到ra */
	for (i = 0; i < (g_uNumbits/8); i++) {
		str_x[i] = prikey_RA[i];
	}
	BN_bin2bn(str_x, g_uNumbits/8, ra);
	

	/*计算x_1 */
	/*计算2^127*/
	if( g_uNumbits == 256 )
	{
	//	BN_hex2bn(&bn_127, "0FFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF");
		BN_hex2bn(&bn_127, "80000000000000000000000000000000");
	}
	else /*if( g_uNumbits == 128 )*/
	{
		BN_dec2bn(&bn_127, "95");
	}

	/* 计算 2^127-1 */
	BN_sub(temp,bn_127,one);
	BN_nnmod(temp,temp,N,ctx);

	/*计算x1&(2^127-1)*/
	/*临时使用str_x与str_y*/
	tcm_bn_bn2bin(x1, 32, str_x);
	tcm_bn_bn2bin(temp, 16, str_y);
	for(i=0;i<16;i++)
	{
		str_x[i] = str_x[i+16] & str_y[i];
	}
	BN_bin2bn(str_x, 16, temp);
	BN_add(x_1, bn_127, temp);

	
#ifdef TEST
{
	char *str;
	str = BN_bn2hex(x_1);
	printf("x_1: %s\n",str);
	OPENSSL_free(str);
}
#endif

	/* 计算ta */
	/* 计算x_1*ra */
	BN_mul(temp, x_1, ra, ctx);
	BN_nnmod(temp, temp, N, ctx);

	BN_add(ta, da, temp);
	BN_nnmod(ta, ta, N, ctx);
#ifdef TEST
{
	char *str;
	str = BN_bn2hex(ta);
	printf("ta: %s\n",str);
	OPENSSL_free(str);
}
#endif

	/* 计算x_2 */
	/* 计算 2^127-1 */
	BN_sub(temp,bn_127,one);
	BN_nnmod(temp,temp,N,ctx);

	/*计算x2&(2^127-1)*/
	/*临时使用str_x与str_y*/
	tcm_bn_bn2bin(x2, 32, str_x);
	tcm_bn_bn2bin(temp, 16, str_y);
	for(i=0;i<16;i++)
	{
		str_x[i] = str_x[i+16] & str_y[i];
	}
	BN_bin2bn(str_x, 16, temp);
	BN_add(x_2, bn_127, temp);
#ifdef TEST
{
	char *str;
	str = BN_bn2hex(x_2);
	printf("x_2: %s\n",str);
	OPENSSL_free(str);
}
#endif

	/* 计算[h*ta[Pb+[x_2]Rb] */
	/* 先计算 [x_2]RB */
	EC_POINT_mul(group, P_temp, x_2, P_RB);
#ifdef TEST
{
		char *str;
		BIGNUM *xb0, *yb0;
		xb0 = BN_new();
		yb0 = BN_new();
		EC_POINT_affine2gem(group,P_temp,P_temp);
		EC_POINT_get_point(P_temp,xb0,yb0,temp);

		str = BN_bn2hex(xb0);
		printf("xb0: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(yb0);
		printf("yb0: %s\n",str);
		OPENSSL_free(str);

		BN_free(xb0);
		BN_free(yb0);
}
#endif

	/* 再计算 PB+[x_2]RB */
	EC_POINT_add(group, P_temp, P_PB, P_temp);
#ifdef TEST
{
		char *str;
		BIGNUM *xb1, *yb1;
		xb1 = BN_new();
		yb1 = BN_new();
		EC_POINT_affine2gem(group,P_temp,P_temp);
		EC_POINT_get_point(P_temp,xb1,yb1,temp);

		str = BN_bn2hex(xb1);
		printf("xb1: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(yb1);
		printf("yb1: %s\n",str);
		OPENSSL_free(str);

		BN_free(xb1);
		BN_free(yb1);
}
#endif
	EC_POINT_mul(group, P_temp, ta, P_temp);
	EC_POINT_affine2gem(group, P_temp, P_temp);
	EC_POINT_get_point(P_temp, xu, yu, temp);

#ifdef TEST
{
		char *str;
		str = BN_bn2hex(xu);
		printf("xu: %s\n",str);
		OPENSSL_free(str);

		str = BN_bn2hex(yu);
		printf("yu: %s\n",str);
		OPENSSL_free(str);
}
#endif
	/* 计算KDF(xu||yu||Za||Zb, klen) */
	tcm_bn_bn2bin(xu, 32, &str_buffer[0]);
	tcm_bn_bn2bin(yu, 32, &str_buffer[32]);
	memcpy(&str_buffer[32*2], Za, 32);
	memcpy(&str_buffer[32*3], Zb, 32);

#ifdef TEST
{
	printf("xu||yu||Za||Zb:\n");
	for(i=0;i<32*4;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_buffer[i]);
	}
	printf("\n\n");
}
#endif

	//
	tcm_kdf(key, 16, str_buffer, 4*32);
#ifdef TEST
{
	printf("KA\n");
	for(i=0;i<16;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", key[i]);
	}
	printf("\n\n");
}
#endif


	/* 计算S1 */
	if( fA == TRUE )
	{//主动方
		tcm_bn_bn2bin(xu, 32, &str_buffer[0]);
		memcpy(&str_buffer[32], Za, 32);
		memcpy(&str_buffer[32*2], Zb, 32);
		tcm_bn_bn2bin(x1, 32, &str_buffer[32*3]);
		tcm_bn_bn2bin(y1, 32, &str_buffer[32*4]);
		tcm_bn_bn2bin(x2, 32, &str_buffer[32*5]);
		tcm_bn_bn2bin(y2, 32, &str_buffer[32*6]);
	}
	else//被动方
	{
		tcm_bn_bn2bin(xu, 32, &str_buffer[0]);
		memcpy(&str_buffer[32], Za, 32);
		memcpy(&str_buffer[32*2], Zb, 32);
		tcm_bn_bn2bin(x2, 32, &str_buffer[32*3]);
		tcm_bn_bn2bin(y2, 32, &str_buffer[32*4]);
		tcm_bn_bn2bin(x1, 32, &str_buffer[32*5]);
		tcm_bn_bn2bin(y1, 32, &str_buffer[32*6]);
	}

#ifdef TEST
{
	printf("xu||Za||Zb||x1||y1||x2||y2:\n");
	for(i=0;i<32*7;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_buffer[i]);
	}
	printf("\n\n");
}
#endif

	tcm_sch_hash(32*7, str_buffer, str_temp32);
#ifdef TEST
{
	printf("Hash(xu||Za||Zb||x1||y1||x2||y2):\n");
	for(i=0;i<32*7;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_buffer[i]);
	}
	printf("\n\n");
}
#endif

	str_temp65[0] = 0x02;
	tcm_bn_bn2bin(yu, 32, &str_temp65[1]);
	memcpy(&str_temp65[1+32], str_temp32, 32);

#ifdef TEST
{
	printf("0x02||yu||Hash(xu||Za||Zb||x1||y1||x2||y2):\n");
	for(i=0;i<32*7;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_buffer[i]);
	}
	printf("\n\n");
}
#endif
	if( fA == TRUE )
	{
		tcm_sch_hash(65, str_temp65, S1);
	}
	else
	{
		tcm_sch_hash(65, str_temp65, Sa);
	}
#ifdef TEST
{
	if( fA == TRUE )
	{
		printf("S1:\n");
		for(i=0;i<32;i++)
		{
			if( i%32 == 0 && i!=0)
				printf("\n");
			if( i%4 == 0 )
				printf(" ");
			printf("%.2X", S1[i]);
		}
	}
	else
	{
		printf("SB:\n");
		for(i=0;i<32;i++)
		{
			if( i%32 == 0 && i!=0)
				printf("\n");
			if( i%4 == 0 )
				printf(" ");
			printf("%.2X", Sa[i]);
		}
	}
	printf("\n\n");
}
#endif

	/* 计算Sa */
	tcm_sch_hash(32*7, str_buffer, str_temp32);
#ifdef TEST
{
	printf("Hash(xu||Za||Zb||x1||y1||x2||y2):\n");
	for(i=0;i<32*7;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_temp32[i]);
	}
	printf("\n\n");
}
#endif

	str_temp65[0] = 0x03;
	tcm_bn_bn2bin(yu, 32, &str_temp65[1]);
	memcpy(&str_temp65[1+32], str_temp32, 32);
#ifdef TEST
{
	printf("0x03||yu||Hash(xu||Za||Zb||x1||y1||x2||y2):\n");
	for(i=0;i<32*7;i++)
	{
		if( i%32 == 0 && i!=0)
			printf("\n");
		if( i%4 == 0 )
			printf(" ");
		printf("%.2X", str_buffer[i]);
	}
	printf("\n\n");
}
#endif
	if( fA == TRUE )
	{
		tcm_sch_hash(65, str_temp65, Sa);
	}
	else
	{
		tcm_sch_hash(65, str_temp65, S1);
	}
#ifdef TEST
{
	if( fA == TRUE )
	{
		printf("SA:\n");
		for(i=0;i<32;i++)
		{
			if( i%32 == 0 && i!=0)
				printf("\n");
			if( i%4 == 0 )
				printf(" ");
			printf("%.2X", Sa[i]);
		}
	}
	else
	{
		printf("S2:\n");
		for(i=0;i<32;i++)
		{
			if( i%32 == 0 && i!=0)
				printf("\n");
			if( i%4 == 0 )
				printf(" ");
			printf("%.2X", S1[i]);
		}

	}

	printf("\n\n");
}
#endif

	BN_free(N);
	BN_free(one);
	BN_free(da);
	BN_free(ra);
	BN_free(ta);
	BN_free(x1);
	BN_free(y1);
	BN_free(x_1);
	BN_free(r_A);
	BN_free(x2);
	BN_free(y2);
	BN_free(x_2);
	BN_free(x_temp);
	BN_free(y_temp);
	BN_free(xu);
	BN_free(yu);
	BN_free(bn_127);
	BN_free(temp);

	EC_POINT_free(P_PB);
	EC_POINT_free(P_RB);
	EC_POINT_free(P_temp);
	
	BN_CTX_free(ctx);

	return 0;
}

