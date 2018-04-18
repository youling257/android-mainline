#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "tcm_ecc.h"

/* 椭圆曲线群group和基点G，全局变量 */
EC_GROUP *group = NULL;
EC_POINT *G = NULL;

/* 密钥长度 */
unsigned int g_uNumbits = 256;
/* 当前使用的HASH长度,仅用于签名前的处理 */
unsigned int g_uSCH_Numbits = 256;

/********************************Fp-256-TEST************************************/
/* Modular p is a big prime */
#define BIGPRIME_256_TEST "8542D69E4C044F18E8B92435BF6FF7DE457283915C45517D722EDB8B08F1DFC3"

/* The first parameter of curve a */
#define CURVE_A_256_TEST "787968B4FA32C3FD2417842E73BBFEFF2F3C848B6831D7E0EC65228B3937E498"

/* The first parameter of curve b */
#define CURVE_B_256_TEST "63E4C6D3B23B0C849CF84241484BFE48F61D59A5B16BA06E6E12D1DA27C5249A"

/* The scope of random value N */
#define BIGLIMIT_256_TEST "8542D69E4C044F18E8B92435BF6FF7DD297720630485628D5AE74EE7C32E79B7"

/* The x-cordinate of basepoint */
#define BASEPOINT_X_256_TEST "421DEBD61B62EAB6746434EBC3CC315E32220B3BADD50BDC4C4E6C147FEDD43D"

/* The x-cordinate of basepoint */
#define BASEPOINT_Y_256_TEST "0680512BCBB42C07D47349D2153B70C4E5D7FDFCBFA36EA1A85841B9E46E09A2"

/* The cofactor */
#define COFACTOR_256_TEST "1"
/********************************Fp-256-TEST************************************/

/********************************Fp-192-TEST************************************/
/* Modular p is a big prime */
#define BIGPRIME_192_TEST  "BDB6F4FE3E8B1D9E0DA8C0D46F4C318CEFE4AFE3B6B8551F"

/* The first parameter of curve a */
#define CURVE_A_192_TEST "BB8E5E8FBC115E139FE6A814FE48AAA6F0ADA1AA5DF91985"

/* The first parameter of curve b */
#define CURVE_B_192_TEST "1854BEBDC31B21B7AEFC80AB0ECD10D5B1B3308E6DBF11C1"

/* The scope of random value N */
#define BIGLIMIT_192_TEST "BDB6F4FE3E8B1D9E0DA8C0D40FC962195DFAE76F56564677"

/* The x-cordinate of basepoint */
#define BASEPOINT_X_192_TEST "4AD5F7048DE709AD51236DE65E4D4B482C836DC6E4106640"

/* The x-cordinate of basepoint */
#define BASEPOINT_Y_192_TEST "02BB3A02D4AAADACAE24817A4CA3A1B014B5270432DB27D2"

/* The cofactor */
#define COFACTOR_192_TEST "1"
/********************************Fp-192-TEST************************************/


/********************************Fp-256************************************/
/* Modular p is a big prime */
#define BIGPRIME "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"

/* The first parameter of curve a */
#define CURVE_A "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"

/* The first parameter of curve b */
#define CURVE_B "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"

/* The scope of random value N */
#define BIGLIMIT "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123"

/* The x-cordinate of basepoint */
#define BASEPOINT_X "32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7"

/* The x-cordinate of basepoint */
#define BASEPOINT_Y "BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0"

/* The cofactor */
#define COFACTOR_TEST "1"
/********************************Fp-256************************************/



int tcm_ecc_init_set(int field,	//0表示素数域；1表示二元域
				  int bitslen,
				  const char* pBigPrime,
				  const char* pCurve_A,
				  const char* pCurve_B,
				  const char* BigLimit,
				  const char* pBasePoint_x,
				  const char* pBasePoint_y,
				  const char* pCofactor);
/*
Feb 12th 创建
ECC初始化
输入：椭圆曲线基本参数p，a，b，r，x，y
输出：椭圆曲线群group
输出：基点G
初始化成功返回0，否则返回1
*/
int tcm_ecc_init()
{
	
	int iret;
	int field = 0;		// 表示素数域
	int bitslen = 256;
	iret = tcm_ecc_init_set(field,
				  bitslen,
				  BIGPRIME,
				  CURVE_A,
				  CURVE_B,
				  BIGLIMIT,
				  BASEPOINT_X,
				  BASEPOINT_Y,
				  COFACTOR_TEST);
	return iret;
}

int tcm_ecc_init_test256()
{
	
	int iret;
	int field = 0;		// 表示素数域
	int bitslen = 256;
	iret = tcm_ecc_init_set(field,
				  bitslen,
				  BIGPRIME_256_TEST,
				  CURVE_A_256_TEST,
				  CURVE_B_256_TEST,
				  BIGLIMIT_256_TEST,
				  BASEPOINT_X_256_TEST,
				 BASEPOINT_Y_256_TEST,
				 COFACTOR_256_TEST);
	return iret;
}

int tcm_ecc_init_test192()
{
	
	int iret;
	int field = 0;		// 表示素数域
	int bitslen = 192;
	iret = tcm_ecc_init_set(field,
				  bitslen,
				  BIGPRIME_192_TEST,
				  CURVE_A_192_TEST,
				  CURVE_B_192_TEST,
				  BIGLIMIT_192_TEST,
				  BASEPOINT_X_192_TEST,
				 BASEPOINT_Y_192_TEST,
				 COFACTOR_192_TEST);
	return iret;
}

/*
ECC初始化
输入：椭圆曲线基本参数field, bitslen p，a，b，r，x，y
输出：椭圆曲线群group
输出：基点G
初始化成功返回0，否则返回1
*/
int tcm_ecc_init_set(int field,	//0表示素数域；1表示二元域
				  int bitslen,
				  const char* pBigPrime,
				  const char* pCurve_A,
				  const char* pCurve_B,
				  const char* BigLimit,
				  const char* pBasePoint_x,
				  const char* pBasePoint_y,
				  const char* pCofactor)
{
	/* ECC的相关参数：素数p，系数a，b，阶数r，基点坐标x，y，余因子h */
	BIGNUM *p, *a, *b, *r, *x, *y, *z, *h;

	if( group != NULL && G != NULL )
	{/*已经初始化成功。仅用于tcm_ecc_init（）函数*/
		return 0;
	}
	if( group != NULL )
	{
		EC_GROUP_free(group);
		group = NULL;
	}
	if( G != NULL )
	{
		EC_POINT_free(G);
		G = NULL;
	}

	
	p = BN_new();
	if(p== NULL)
	{
	  return 1;
	}
	a = BN_new();
	if(a== NULL)
	{
	  BN_free(p);
	  return 1;
	}
	b = BN_new();
	if(b== NULL)
	{
	  BN_free(a);
	  BN_free(p);
	  return 1;
	}
	r = BN_new();
	if(r== NULL)
	{
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  return 1;
	}
	x = BN_new();
	if(x== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  return 1;
	}
	y = BN_new();
	if(y== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  BN_free(x);
	  return 1;
	}
	z = BN_new();
	if(z== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  BN_free(x);
	  BN_free(y);
	  return 1;
	}
	h = BN_new();
	if(z== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  BN_free(x);
	  BN_free(y);
	  BN_free(z);
	  return 1;
	}

	G = EC_POINT_new();
    if(G== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  BN_free(x);
	  BN_free(y);
	  BN_free(z);
	  BN_free(h);
	  G = NULL;
	  group = NULL;
	  return 1;
	}
	group = EC_GROUP_new();
	if(group== NULL)
	{
	  BN_free(r);
	  BN_free(p);
	  BN_free(a);
	  BN_free(b);
	  BN_free(x);
	  BN_free(y);
	  BN_free(z);
	  BN_free(h);
	  EC_POINT_free(G);
	  G = NULL;
	  group = NULL;
	  return 1;
	}

	//设置曲线长度
	g_uNumbits = bitslen;
	g_uSCH_Numbits = bitslen;
	/* 设置ECC的相关参数 */
	BN_hex2bn(&p, pBigPrime);
	BN_hex2bn(&a, pCurve_A);
	BN_hex2bn(&b, pCurve_B);
	BN_hex2bn(&r, BigLimit);
	BN_hex2bn(&x, pBasePoint_x);
	BN_hex2bn(&y, pBasePoint_y);
	BN_hex2bn(&h, pCofactor);

	/* 生成椭圆曲线群GROUP */
	EC_GROUP_set_curve_GFp(group,p,a,b);
	EC_GROUP_set_order(group,r);

	/* 设置椭圆曲线余因子h */
	EC_GROUP_set_cofactor(group, h);

	/* G Base point is (,) */
	BN_dec2bn(&z,"1");
	EC_POINT_set_point(G,x,y,z);


	/* 释放相关资源 */
	BN_free(p);
	BN_free(a);
	BN_free(b);
	BN_free(r);
	BN_free(x);
	BN_free(y);
	BN_free(z);
	BN_free(h);

	return 0;
}


int tcm_ecc_release()
{
	EC_GROUP_free(group);
	group = NULL;
	EC_POINT_free(G);
	G = NULL;
	return 0;
}
