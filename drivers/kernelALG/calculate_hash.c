/******************************************
File	:calculate_hash.c 
Author	:linyang
Date	:11/21/2006
comment :国标算法
******************************************/


#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
#include "tcm_hash.h"
#include "tcm_bn.h"
//#include <sys/socket.h>
#include <linux/vmalloc.h>

typedef unsigned char BYTE;

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
//#define uint32 unsigned long int
#define uint32 unsigned int
#endif

/*
计算Za，总是使用256bits的HASH算法
根据用户数据，计算用户的hash数据
输出：hash数据 Digest
输入：用户标识串userid, 用户标识串长度userIDLen, 
输入：公钥pPubkey_in, 公钥长度pubkeyLen_in,
*/
int tcm_get_usrinfo_value(
					   unsigned char *userID, unsigned short int userIDLen, 
					   unsigned char *pPubkey_in, unsigned int pubkeyLen_in,
					   unsigned char digest[HASH_NUMBITS/8])
{
	int iret;
	/* ECC的相关参数：系数a，b，基点坐标xG，yG */
	BIGNUM *a, *b, *xG, *yG, *zG;
	BIGNUM *p;	/*这里无用*/
	//
	unsigned char	ENTL[2];
	unsigned char*	pstr_a=NULL;
	unsigned char*	pstr_b=NULL;
	unsigned char*	pstr_xG=NULL;
	unsigned char*	pstr_yG=NULL;
	//
	BYTE *pstr_Z=NULL;
	uint32 uZlen = 0;
	int iPos=0;

	/* 检查公钥pPubkey_in是否为空 */
	if (pPubkey_in ==NULL)
	{
		return 1;
	}
	/* 检查公钥pubkeyLen_in长度 */
	if (pubkeyLen_in != PUBKEY_LEN)
	{
		return 1;
	}

	//
	a = BN_new();
	if(a == NULL)
	{
	  return 1;
	}
	b = BN_new();
	if(b == NULL)
	{
	  BN_free(a);
	  return 1;
	}
	xG = BN_new();
	if(xG == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  return 1;
	}
	yG = BN_new();
	if(yG == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  return 1;
	}
	zG = BN_new();
	if(zG == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  return 1;
	}
	//
	p = BN_new();
	if(p == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  BN_free(zG);
	  return 1;
	}
	//
	pstr_a = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_a == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  BN_free(zG);
	  BN_free(p);
	  return 1;
	}
	//
	pstr_b = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_b == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  BN_free(zG);
	  BN_free(p);
	  //
	  vfree(pstr_a);
	  return 1;
	}
	//
	pstr_xG = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_xG == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  BN_free(zG);
	  BN_free(p);
	  //
	  vfree(pstr_a);
	  vfree(pstr_b);
	  return 1;
	}
	//
	pstr_yG = (unsigned char*)vmalloc(g_uNumbits/8);
	if(pstr_yG == NULL)
	{
	  BN_free(a);
	  BN_free(b);
	  BN_free(xG);
	  BN_free(yG);
	  BN_free(zG);
	  BN_free(p);
	  //
	  vfree(pstr_a);
	  vfree(pstr_b);
	  vfree(pstr_xG);
	  return 1;
	}

	/* 得到椭圆曲线群相关参数P */
	EC_GROUP_get_curve_GFp(group,p,a,b);
	EC_POINT_get_point(G, xG, yG, zG);


	//得到a,b的字符串
	tcm_bn_bn2bin(a, g_uNumbits/8, pstr_a);
	tcm_bn_bn2bin(b, g_uNumbits/8, pstr_b);

	//得到G点的字符串
	tcm_bn_bn2bin(xG, g_uNumbits/8, pstr_xG);
	tcm_bn_bn2bin(yG, g_uNumbits/8, pstr_yG);

	//
	{
		const u_short userIDBitsLen = userIDLen*8;
		*((u_short*)ENTL) = htons(userIDBitsLen);
	}

	//计算ZA的流程
	// ENTL||ID||a||b||xG||yG||xA||yA
	uZlen = 2	// sizeof(ENTL)
		 + userIDLen	// ID的长度
		 + 6 * g_uNumbits/8;	// a||b||xG||yG||xA||yA

	pstr_Z = (BYTE*)vmalloc(uZlen);
	if( pstr_Z == NULL )
	{
		iret = 1;
		goto end;
	}
	iPos = 0;
	// 
	memcpy(pstr_Z, ENTL, 2);
	iPos += 2;
	//
	if( userIDLen != 0 )
	{
		memcpy(pstr_Z+iPos, userID, userIDLen);
		iPos += userIDLen;
	}
	//
	memcpy(pstr_Z+iPos, pstr_a, g_uNumbits/8);
	iPos += g_uNumbits/8;
	//
	memcpy(pstr_Z+iPos, pstr_b, g_uNumbits/8);
	iPos += g_uNumbits/8;
	//
	memcpy(pstr_Z+iPos, pstr_xG, g_uNumbits/8);
	iPos += g_uNumbits/8;
	//
	memcpy(pstr_Z+iPos, pstr_yG, g_uNumbits/8);
	iPos += g_uNumbits/8;
	//
	memcpy(pstr_Z+iPos, &pPubkey_in[1], 2*g_uNumbits/8);
	iPos += 2*g_uNumbits/8;
	//
#ifdef TEST
{
	char *str;
	BN_bin2bn(pstr_Z,uZlen,a);
	str = BN_bn2hex(a);
	printf("ENTL||ID||a||b||x||y||x||y: %s\n",str);
	OPENSSL_free(str);
}
#endif
	

	//计算ZA
	iret = tcm_sch_256( uZlen, pstr_Z, digest);
	//
	vfree(pstr_Z);
	pstr_Z=NULL;

	//
	iret = 0;
end:
	BN_free(a);
	BN_free(b);
	BN_free(xG);
	BN_free(yG);
	BN_free(zG);
	BN_free(p);
	//
	vfree(pstr_a);
	vfree(pstr_b);
	vfree(pstr_xG);
	vfree(pstr_yG);
	//
	return iret;
}

/* 计算消息的hash */
int tcm_get_message_hash(unsigned char *msg, unsigned int msgLen,  
					   unsigned char *userID, unsigned short int userIDLen, 
					   unsigned char *pPubkey_in, unsigned int pubkeyLen_in,
					   unsigned char *pDigest,
					   unsigned int *puDigestLen)
{
	// 返回值
	int iret;
	//
	unsigned char schsum_Z[HASH_NUMBITS/8];	// 计算Za，总是使用256hash算法
	//
	BYTE *pstr_M=NULL;
	uint32 uMlen = 0;
	//
	iret = tcm_get_usrinfo_value(userID, userIDLen, 
					   pPubkey_in, pubkeyLen_in,
					   schsum_Z);
	if( iret == 1 )
	{
		return 1;
	}

	//
	//计算M的流程
	// ZA||M
	uMlen = HASH_NUMBITS/8+msgLen;
	pstr_M = (BYTE*)vmalloc(uMlen);
	if( pstr_M == NULL )
	{
		return 1;
	}
	// 
	memcpy(pstr_M, schsum_Z, sizeof(schsum_Z));
	//
	if( msgLen != 0 )
	{
		memcpy(pstr_M+sizeof(schsum_Z), msg, msgLen);
	}
	//

	//计算M
	if( g_uSCH_Numbits == 256 )
	{
		iret = tcm_sch_256( uMlen, pstr_M, pDigest);
		//
		vfree(pstr_M);
		pstr_M=NULL;
	}
	else if( g_uSCH_Numbits == 192 )
	{
		iret = tcm_sch_192( uMlen, pstr_M, pDigest);
		//
		vfree(pstr_M);
		pstr_M=NULL;
	}
	else
	{
		vfree(pstr_M);
		pstr_M=NULL;
		//
		return 1;
	}
	//
	*puDigestLen = g_uSCH_Numbits/8;
	//
	return 0;
}
