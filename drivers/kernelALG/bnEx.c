/*************************************************************
Module Name:Extended bn operations.

**************************************************************/
#include "openssl/bnEx.h"

/* 用扩展欧几里德算法求d=g/h mod m */
int BN_div_mod(BIGNUM *d,const BIGNUM *g, const BIGNUM *h,const BIGNUM *m)
{
	BIGNUM *tmp=BN_new();
	BN_CTX *ctx = BN_CTX_new();
	if (ctx == NULL) return 0;

	BN_mod_inverse(tmp,h,m,ctx);
	BN_mul(d,g,tmp,ctx);
	BN_nnmod(d,d,m,ctx);

	BN_free(tmp);
	BN_CTX_free(ctx);	
	return 1;
}

int BN_div_mod2(BIGNUM *d,const BIGNUM *g, const BIGNUM *h,const BIGNUM *m)
{
	BIGNUM *zero,*r0,*r1,*r2,*s0,*s1,*s2,*q,*tmp;
	BN_CTX *ctx = BN_CTX_new();
	if (ctx == NULL) return 0;

	if(BN_is_one(h))
	{
		BN_nnmod(d,g,m,ctx);
		BN_CTX_free(ctx);
		return 1;
	}

	zero=BN_new();
	r0=BN_new();
	r1=BN_new();
	r2=BN_new();
	s0=BN_new();
	s1=BN_new();
	s2=BN_new();
	q=BN_new();
	tmp=BN_new();

	BN_copy(r0,m);
	BN_nnmod(r1,h,m,ctx);
	BN_dec2bn(&s0,"0");
	BN_nnmod(s1,g,m,ctx);

	BN_dec2bn(&zero,"0");
	while(BN_cmp(r1,zero)==1)		/* while r1>0 */
	{
		BN_div(q,r2,r0,r1,ctx);

		BN_mul(tmp,q,r1,ctx);
		BN_sub(tmp,r0,tmp);
		BN_mod(r2,tmp,m,ctx);

		BN_mul(tmp,q,s1,ctx);
		BN_sub(tmp,s0,tmp);
		BN_mod(s2,tmp,m,ctx);

		BN_copy(r0,r1);
		BN_copy(r1,r2);
		BN_copy(s0,s1);
		BN_copy(s1,s2);
	}
	BN_copy(d,s0);

	BN_free(zero);
	BN_free(r0);
	BN_free(r1);
	BN_free(r2);
	BN_free(s0);
	BN_free(s1);
	BN_free(s2);
	BN_free(q);
	BN_free(tmp);

	return 1;
}