/**********************************************************
Module name:ec_operations.c
Written by:ChenJUN
Date:2003-6-16
Purpose:椭圆曲线的基本操作，及签名验证，加解密算法
全部采用仿射坐标实现。
************************************************************/

#include "openssl/bn.h"
#include "openssl/bnEx.h"
#include "openssl/crypto.h"
//#include <time.h>

EC_POINT *EC_POINT_new()
{
	EC_POINT *ret;
	ret = OPENSSL_malloc(sizeof *ret);
	BN_init(&ret->X);
	BN_init(&ret->Y);
	BN_init(&ret->Z);
	return ret;
}


void EC_POINT_free(EC_POINT *point)
{
	if(!point) return;

	BN_free(&point->X);
	BN_free(&point->Y);
	BN_free(&point->Z);
	OPENSSL_free(point);
}

int EC_POINT_is_at_infinity(const EC_GROUP *group,const EC_POINT *point)
{
	return BN_is_zero(&point->Z);
}

int EC_POINT_set_to_infinity(const EC_GROUP *group,EC_POINT *point)
{
	point->Z_is_one = 0;
	return (BN_zero(&point->Z));
}

int EC_POINT_set_point(EC_POINT *point,const BIGNUM *x,const BIGNUM *y,const BIGNUM *z)
{
	BN_copy(&(point->X),x);
	BN_copy(&(point->Y),y);
	BN_copy(&(point->Z),z);
	return 1;
}

int EC_POINT_get_point(const EC_POINT *point,BIGNUM *x,BIGNUM *y,BIGNUM *z)
{
	BN_copy(x,&(point->X));
	BN_copy(y,&(point->Y));
	BN_copy(z,&(point->Z));
	return 1;
}

int EC_POINT_copy(EC_POINT *dest, const EC_POINT *src)
{
	if(!BN_copy(&(dest->X),&(src->X))) return 0;
	if(!BN_copy(&(dest->Y),&(src->Y))) return 0;
	if(!BN_copy(&(dest->Z),&(src->Z))) return 0;
	return 1;
}

void EC_POINT_print(EC_POINT *P)
{
	char *a=BN_bn2hex(&P->X);
	//fprintf(stdout,"(%s",a); 
	a=BN_bn2hex(&P->Y);
	//fprintf(stdout,",%s",a); 
	a=BN_bn2hex(&P->Z);
	//fprintf(stdout,",%s)",a); 
	//printf("\n"); 
}

int EC_POINT_invert(const EC_GROUP *group,EC_POINT *point)
{
	if (EC_POINT_is_at_infinity(group, point) || BN_is_zero(&point->Y))
		/* point is its own inverse */
		return 1;
	
	return BN_usub(&point->Y, &group->p, &point->Y);
}

/* 仿射坐标转换为几何坐标 */
int EC_POINT_affine2gem(const EC_GROUP *group,const EC_POINT *P,EC_POINT *R)
{
	BIGNUM *x,*y,*z,*tmp,*one;
	BN_CTX *ctx;
	const BIGNUM *p;		/* add const definition */
	 
	x=BN_new();
	y=BN_new();
	z=BN_new();
	one=BN_new();
	
	EC_POINT_get_point(P,x,y,z);

	BN_dec2bn(&one,"1");
	if(BN_cmp(z,one)==0)		/* z==1 */
	{
		EC_POINT_set_point(R,x,y,z);
		BN_free(x);
		BN_free(y);
		BN_free(z);
		BN_free(one);

		return 1;
	}

	tmp=BN_new();
	ctx= BN_CTX_new();
	p=&group->p;

	/* tmp=z2^2 */
	BN_mul(tmp,z,z,ctx);
	BN_nnmod(tmp,tmp,p,ctx);
	BN_div_mod(x,x,tmp,p);

	/* tmp=z2^3 */
	BN_mul(tmp,z,z,ctx);
    BN_mul(tmp,tmp,z,ctx);
	BN_nnmod(tmp,tmp,p,ctx);
	BN_div_mod(y,y,tmp,p);

	BN_dec2bn(&z,"1");
	EC_POINT_set_point(R,x,y,z);

	BN_free(x);
	BN_free(y);
	BN_free(z);
	BN_free(one);
	BN_free(tmp);
	BN_CTX_free(ctx);
	return 1;
}

int EC_POINT_sub(const EC_GROUP *group, EC_POINT *R, const EC_POINT *P0, const EC_POINT *P1)
{
	EC_POINT *tmp;
	tmp=EC_POINT_new();

	EC_POINT_copy(tmp,P1);
    EC_POINT_invert(group,tmp);
	EC_POINT_add(group,R,P0,tmp);

	EC_POINT_free(tmp);
	return 1;
}

/* R=2P用仿射坐标下求椭圆曲线的点的倍乘 */
int EC_POINT_dbl(const EC_GROUP *group, EC_POINT *R, const EC_POINT *P)
{
	BIGNUM *t1,*t2,*t3,*t4,*t5,*tmp,*one;
	BIGNUM *p,*a;
	BN_CTX *ctx;

	t1=BN_new();
	t2=BN_new();
	t3=BN_new();
	tmp=BN_new();
	one=BN_new();
	p=BN_new();
	a=BN_new();

	BN_dec2bn(&one,"1");

	EC_POINT_get_point(P,t1,t2,t3);
	EC_GROUP_get_curve_GFp(group,p,a,tmp);

	/* step4 */
	if(BN_is_zero(t2)||BN_is_zero(t3)) 
	{
		BN_dec2bn(&tmp,"0");
		EC_POINT_set_point(R,one,one,tmp);

		BN_free(t1);
		BN_free(t2);
		BN_free(t3);
		BN_free(tmp);
		BN_free(one);
		BN_free(p);
		BN_free(a);

		return 1;
	}

	t4=BN_new();
	t5=BN_new();
	ctx= BN_CTX_new();

	/* step5 */
	BN_copy(t4,a);
	BN_mul(t5,t3,t3,ctx);		/* t5=t3×t3 */
	BN_nnmod(t5,t5,p,ctx);

	BN_mul(t5,t5,t5,ctx);		/* t5=t5×t5 */
	BN_nnmod(t5,t5,p,ctx);

	BN_mul(t5,t4,t5,ctx);		/* t5=t4×t5 */
	BN_nnmod(t5,t5,p,ctx);

	BN_mul(t4,t1,t1,ctx);		/* t4=t1×t1 */
	BN_nnmod(t4,t4,p,ctx);

	/* t4=3×t4 */
	BN_dec2bn(&tmp,"3");
	BN_mul(t4,tmp,t4,ctx);
	BN_nnmod(t4,t4,p,ctx);

	BN_add(t4,t4,t5);			/* t4=t4+t5 */

	/* step6-20 */
	BN_mul(t3,t2,t3,ctx);		/* t3=t2×t3 */
	BN_nnmod(t3,t3,p,ctx);
	BN_add(t3,t3,t3);			/* t3=2×t3 */
	BN_nnmod(t3,t3,p,ctx);		/* z2 */

	BN_mul(t2,t2,t2,ctx);		/* t2=t2×t2 */
	BN_nnmod(t2,t2,p,ctx);
	BN_mul(t5,t1,t2,ctx);		/* t5=t1×t2 */
	BN_nnmod(t5,t5,p,ctx);

	/* t5=4×t5 */
	BN_add(t5,t5,t5);
	BN_add(t5,t5,t5);

	BN_mul(t1,t4,t4,ctx);		/* t1=t4×t4 */
 
	/* t1=t1 -2×t5 */
	BN_add(tmp,t5,t5);
	BN_sub(t1,t1,tmp);
	BN_nnmod(t1,t1,p,ctx);		/* x2 */

	BN_mul(t2,t2,t2,ctx);		/* t2=t2×t2 */

	/* t2=8×t2 */
	BN_add(t2,t2,t2);
	BN_add(t2,t2,t2);
	BN_add(t2,t2,t2);

	BN_sub(t5,t5,t1);			/* t5=t5-t1 */
	BN_mul(t5,t4,t5,ctx);		/* t5=t4×t5 */
	BN_sub(t2,t5,t2);			/* t2=t5-t2 */
	BN_nnmod(t2,t2,p,ctx);		/* y2 */

	/* x2=t1,y2=t2,z2=t3	*/
	EC_POINT_set_point(R,t1,t2,t3);

	BN_free(t1);
	BN_free(t2);
	BN_free(t3);
	BN_free(t4);
	BN_free(t5);
	BN_free(tmp);
	BN_free(one);
	BN_free(p);
	BN_free(a);
	BN_CTX_free(ctx);

	return 1;
}

/* 用仿射坐标下求椭圆曲线的点加法 */
int EC_POINT_add(const EC_GROUP *group, EC_POINT *R, const EC_POINT *P0,const EC_POINT *P1)
{
	BIGNUM *t1,*t2,*t3,*t4,*t5,*t6,*t7,*tmp,*one;
	const BIGNUM *p,*z0,*z1;
	BN_CTX *ctx;

	z0=&P0->Z;
    if(BN_is_zero(z0))
	{
		EC_POINT_copy(R,P1);
		return 1;
	}

	z1=&P1->Z;
    if(BN_is_zero(z1))
	{
		EC_POINT_copy(R,P0);
		return 1;
	}

	t1=BN_new();
	t2=BN_new();
	t3=BN_new();
	t4=BN_new();
	t5=BN_new();
	t6=BN_new();
	
	EC_POINT_get_point(P0,t1,t2,t3);	/* t1<-x0,t2<-y0,t3<-z0 */
	EC_POINT_get_point(P1,t4,t5,t6);	/* t4<-x1,t5<-y1,t6<-z1 */

	p=&group->p;
	
	/* P0=P1时，用倍乘 */
	if(BN_cmp(t1,t4)==0&&BN_cmp(t2,t5)==0&&BN_cmp(t3,t6)==0)
	{
		BN_free(t1);
		BN_free(t2);
		BN_free(t3);
		BN_free(t4);
		BN_free(t5);
		BN_free(t6);

		return EC_POINT_dbl(group,R,P0);
	}

	t7=BN_new();
	tmp=BN_new();
	one=BN_new();
	ctx= BN_CTX_new();

	BN_dec2bn(&one,"1");

	/* step6 */
	/* if z1<>1 */
	if(BN_cmp(t6,one)!=0)
	{
		BN_mul(t7,t6,t6,ctx);		/* t7=t6×t6 */
		BN_nnmod(t7,t7,p,ctx);
		BN_mul(t1,t1,t7,ctx);		/* t1=t1×t7 */
		BN_nnmod(t1,t1,p,ctx);
		BN_mul(t7,t6,t7,ctx);		/* t7=t6×t7 */
		BN_nnmod(t7,t7,p,ctx);
		BN_mul(t2,t2,t7,ctx);		/* t2=t2×t7 */
		BN_nnmod(t2,t2,p,ctx);
	}

	/* step7~12 */
	BN_mul(t7,t3,t3,ctx);			/* t7=t3×t3 */
	BN_nnmod(t7,t7,p,ctx);
	BN_mul(t4,t4,t7,ctx);			/* t4=t4×t7 */
	BN_nnmod(t4,t4,p,ctx);
	BN_mul(t7,t3,t7,ctx);			/* t7=t3×t7 */
	BN_nnmod(t7,t7,p,ctx);
	BN_mul(t5,t5,t7,ctx);			/* t5=t5×t7 */
	BN_nnmod(t5,t5,p,ctx);
	BN_sub(t4,t1,t4);				/* t4=t1-t4 */
	BN_sub(t5,t2,t5);				/* t5=t2-t5 */

	/* step13 */
	if(BN_is_zero(t4)) 
	{
		if(BN_is_zero(t5))
			return EC_POINT_dbl(group,R,P0);
		else
			BN_dec2bn(&tmp,"0");
			EC_POINT_set_point(R,one,one,tmp);
		return 1;
	}

	/* step14 */
	/* t1=2t1 -t4 */
    BN_add(tmp,t1,t1);
	BN_sub(t1,tmp,t4);

	/* step15 */
	/* t2=2t2 -t5 */
    BN_add(tmp,t2,t2);
	BN_sub(t2,tmp,t5);

	/* step16 */
	/* if z1<>1 */
	z1=&P1->Z;
	if(BN_cmp(z1,one)!=0)
		BN_mul(t3,t3,t6,ctx);

	/* step17~22 */
	BN_mul(t3,t3,t4,ctx);			/* t3=t3×t4 */
	BN_nnmod(t3,t3,p,ctx);

	BN_mul(t7,t4,t4,ctx);			/* t7=t4×t4 */
	BN_mul(t4,t4,t7,ctx);			/*t4=t4×t7 */
	BN_mul(t7,t1,t7,ctx);			/* t7=t1×t7 */
	BN_mul(t1,t5,t5,ctx);			/* t1=t5×t5 */
	BN_sub(t1,t1,t7);				/* t1=t1-t7 */
	BN_nnmod(t1,t1,p,ctx);

	/* step23 */
	/* t7=t7 -2×t1 */
    BN_add(tmp,t1,t1);
	BN_sub(t7,t7,tmp);

	/* step24-30 */
	BN_mul(t5,t5,t7,ctx);			/*t5=t5×t7 */
	BN_mul(t4,t2,t4,ctx);			/* t4=t2×t4 */
	BN_sub(t2,t5,t4);				/* t2=t5-t4 */
	BN_dec2bn(&tmp,"2");
    BN_rshift(t2,t2,1);				/* t2=t2/2 */
	BN_nnmod(t2,t2,p,ctx);

	EC_POINT_set_point(R,t1,t2,t3);

	BN_free(t1);
	BN_free(t2);
	BN_free(t3);
	BN_free(t4);
	BN_free(t5);
	BN_free(t6);
	BN_free(t7);
	BN_free(tmp);
	BN_free(one);
	BN_CTX_free(ctx);

	return 1;
}

/* 用仿射坐标下求椭圆曲线的点加法(不待取模) */
int EC_POINT_add2(const EC_GROUP *group, EC_POINT *R, const EC_POINT *P0,const EC_POINT *P1)
{
	BIGNUM *t1,*t2,*t3,*t4,*t5,*t6,*t7,*tmp,*one;
	const BIGNUM *p,*z0,*z1;
	BN_CTX *ctx;

	z0=&P0->Z;
    if(BN_is_zero(z0))
	{
		EC_POINT_copy(R,P1);
		return 1;
	}

	z1=&P1->Z;
    if(BN_is_zero(z1))
	{
		EC_POINT_copy(R,P0);
		return 1;
	}

	t1=BN_new();
	t2=BN_new();
	t3=BN_new();
	t4=BN_new();
	t5=BN_new();
	t6=BN_new();
	
	EC_POINT_get_point(P0,t1,t2,t3);		/* t1<-x0,t2<-y0,t3<-z0 */
	EC_POINT_get_point(P1,t4,t5,t6);		/* t4<-x1,t5<-y1,t6<-z1 */

	p=&group->p;
	
	/* P0=P1时，用倍乘 */
	if(BN_cmp(t1,t4)==0&&BN_cmp(t2,t5)==0&&BN_cmp(t3,t6)==0)
	{
		BN_free(t1);
		BN_free(t2);
		BN_free(t3);
		BN_free(t4);
		BN_free(t5);
		BN_free(t6);

		return EC_POINT_dbl(group,R,P0);
	}

	t7=BN_new();
	tmp=BN_new();
	one=BN_new();
	ctx= BN_CTX_new();

	BN_dec2bn(&one,"1");

	/* step6 */
	/* if z1<>1 */
	if(BN_cmp(t6,one)!=0)
	{
		BN_mul(t7,t6,t6,ctx);			/* t7=t6×t6 */
		BN_mul(t1,t1,t7,ctx);			/* t1=t1×t7 */
		BN_mul(t7,t6,t7,ctx);			/* t7=t6×t7 */
		BN_mul(t2,t2,t7,ctx);			/* t2=t2×t7 */
	}

	/* step7~12 */
	BN_mul(t7,t3,t3,ctx);				/* t7=t3×t3 */
	BN_mul(t4,t4,t7,ctx);				/* t4=t4×t7 */
	BN_mul(t7,t3,t7,ctx);				/* t7=t3×t7 */
	BN_mul(t5,t5,t7,ctx);				/* t5=t5×t7 */
	BN_sub(t4,t1,t4);					/* t4=t1-t4 */
	BN_sub(t5,t2,t5);					/* t5=t2-t5 */

	/* step13 */
	if(BN_is_zero(t4)) 
	{
		if(BN_is_zero(t5))
			return EC_POINT_dbl(group,R,P0);
		else
			BN_dec2bn(&tmp,"0");
			EC_POINT_set_point(R,one,one,tmp);
		return 1;
	}

	/* step14 */
	/* t1=2t1 -t4 */
    BN_add(tmp,t1,t1);
	BN_sub(t1,tmp,t4);

	/* step15 */
	/* t2=2t2 -t5 */
    BN_add(tmp,t2,t2);
	BN_sub(t2,tmp,t5);

	/* step16 */
	/* if z1<>1 */
	z1=&P1->Z;
	if(BN_cmp(z1,one)!=0)
		BN_mul(t3,t3,t6,ctx);

	/* step17~22 */
	BN_mul(t3,t3,t4,ctx);			/* t3=t3×t4 */
	BN_nnmod(t3,t3,p,ctx);
	BN_mul(t7,t4,t4,ctx);			/* t7=t4×t4 */
	BN_mul(t4,t4,t7,ctx);			/* t4=t4×t7 */
	BN_mul(t7,t1,t7,ctx);			/*t7=t1×t7 */
	BN_mul(t1,t5,t5,ctx);			/* t1=t5×t5 */
	BN_sub(t1,t1,t7);				/* t1=t1-t7 */
	BN_nnmod(t1,t1,p,ctx);

	/* step23 */
	/* t7=t7 -2×t1 */
    BN_add(tmp,t1,t1);
	BN_sub(t7,t7,tmp);

	/* step24-30 */
	BN_mul(t5,t5,t7,ctx);			/* t5=t5×t7 */
	BN_mul(t4,t2,t4,ctx);			/* t4=t2×t4 */
	BN_sub(t2,t5,t4);				/* t2=t5-t4 */
	BN_dec2bn(&tmp,"2");

	BN_div(t2,t7,t2,tmp,ctx);
	BN_nnmod(t2,t2,p,ctx);

	EC_POINT_set_point(R,t1,t2,t3);

	BN_free(t1);
	BN_free(t2);
	BN_free(t3);
	BN_free(t4);
	BN_free(t5);
	BN_free(t6);
	BN_free(t7);
	BN_free(tmp);
	BN_free(one);
	BN_CTX_free(ctx);

	return 1;
}

/* 
Ecc Scalar Multiplication仿射坐标下的数乘
S=nP
*/
int EC_POINT_mul(const EC_GROUP *group,EC_POINT *S,const BIGNUM *n, const EC_POINT *P)
{

	EC_POINT *Q;
	BIGNUM *k,*_3k,*x,*y,*z,*xt,*yt,*zt,*one,*zero;
	const BIGNUM *p;
	BN_CTX *ctx;

	int n_neg,i,lh,lk;

	x=BN_new();
	y=BN_new();
	z=BN_new();
	one=BN_new();
	zero=BN_new();

	BN_dec2bn(&one,"1");
	BN_dec2bn(&zero,"0");

	EC_POINT_get_point(P,x,y,z);

	if(BN_is_zero(n)||BN_is_zero(z)) 
	{
	
		EC_POINT_set_point(S,one,one,zero);

		BN_free(x);
		BN_free(y);
		BN_free(z);
		BN_free(one);
		BN_free(zero);

		return 1;
	}

	xt=BN_new();
	yt=BN_new();
	zt=BN_new();
	Q=EC_POINT_new();
	k=BN_new();
	_3k=BN_new();
	ctx= BN_CTX_new();

    p=&group->p;

	/* step2 */
	BN_copy(xt,x);
	BN_copy(zt,z);
	
	/* step3 */
	n_neg = n->neg;
	if(n_neg)			/*n<0 */
	{
		/* k=-n */
		BN_copy(k,n);
		k->neg=1;

		/* y1=-y */
		BN_copy(yt,y);
		yt->neg=1;

		BN_nnmod(yt,yt,p,ctx);
	}
	else
	{
		BN_copy(k,n);		
		BN_copy(yt,y);		
	}

	/* step8 */
	EC_POINT_set_point(S,xt,yt,zt);
	EC_POINT_copy(Q,S);

	/* step9 */
	BN_add(_3k,k,k);
	BN_add(_3k,k,_3k);
	lh=BN_num_bits(_3k);

	/* step10 */
	lk=BN_num_bits(k);
	
	/* step11 */
	for(i=lh-2;i>=1;i--)
	{
		EC_POINT_dbl(group,S,S);
		if(BN_is_bit_set(_3k,i)&&!BN_is_bit_set(k,i))
		{
			EC_POINT_add(group,S,S,Q);
		}
		
		if(!BN_is_bit_set(_3k,i)&&BN_is_bit_set(k,i))
		{
			EC_POINT_sub(group,S,S,Q);
		}	
	}

	BN_free(x);
	BN_free(y);
	BN_free(z);
	BN_free(one);
	BN_free(zero);
	BN_free(xt);
	BN_free(yt);
	BN_free(zt);
	BN_free(k);
	BN_free(_3k);
    BN_CTX_free(ctx);
	EC_POINT_free(Q);

	return 1;
}	

EC_GROUP *EC_GROUP_new()
{ 
	EC_GROUP *ret;
	ret = OPENSSL_malloc(sizeof *ret);
	BN_init(&(ret->p));
	BN_init(&(ret->a));
	BN_init(&(ret->b));
	ret->generator=EC_POINT_new();
	BN_init(&ret->order);
	BN_init(&ret->cofactor);
	return ret;
}

void EC_GROUP_free(EC_GROUP *group)
{
	if(!group) return;

	BN_free(&group->p);
	BN_free(&group->a);
	BN_free(&group->b);
	if (group->generator != NULL)
		EC_POINT_free(group->generator);
	BN_free(&group->order);
	BN_free(&group->cofactor);
	
	OPENSSL_free(group);
}

int EC_GROUP_set_curve_GFp(EC_GROUP *group, const BIGNUM *p, const BIGNUM *a, const BIGNUM *b)
{
	BN_copy(&(group->p),p);
	BN_copy(&(group->a),a);
	BN_copy(&(group->b),b);	
	return 1;
}

int EC_GROUP_get_curve_GFp(const EC_GROUP *group, BIGNUM *p, BIGNUM *a, BIGNUM *b)
{
	BN_copy(p,&(group->p));
	BN_copy(a,&(group->a));
	BN_copy(b,&(group->b));
	return 1;
}

int EC_GROUP_set_generator(EC_GROUP *group, const EC_POINT *generator, const BIGNUM *r, const BIGNUM *cofactor)
{
	EC_POINT_copy(group->generator,generator);
	BN_copy(&(group->order),r);
	BN_copy(&(group->cofactor),cofactor);	
	return 1;
}

int EC_GROUP_set_order(EC_GROUP *group,const  BIGNUM *order)
{
	BN_copy(&(group->order),order);
	return 1;
}

int EC_GROUP_get_order(const EC_GROUP *group, BIGNUM *r)
{
	BN_copy(r,&(group->order));
	return 1;
}

/* 
2006.12.4
linyang
*/
int EC_GROUP_set_cofactor(EC_GROUP *group, const BIGNUM *cofactor)
{
	BN_copy(&(group->cofactor),cofactor);
	return 1;
}

int EC_GROUP_get_cofactor(const EC_GROUP *group, BIGNUM *cofactor)
{
	BN_copy(cofactor,&(group->cofactor));
	return 1;
}

/* 
判断点是否在曲线上
为方便与效率，全部使用取模计算
如果点在曲线上，返回TRUE
author: linyang 
time: 2006.11
comment:
	2006.12.31
	增加point是无穷远点的判断
	增加NP是否为无穷远点的判断
*/
BOOL EC_POINT_is_on_curve(const EC_GROUP *group, const EC_POINT *point)
{
	// y^2=x^3+ax+b
	BIGNUM *x, *y, *z, *a, *b;
	BIGNUM *tmp, *left, *right;
	BIGNUM *N;
	BN_CTX *ctx;
	EC_POINT *P_NP;
	const BIGNUM *p=&(group->p);

	/* 首先判断是否无穷远点，如果是，返回失败 */
	if( EC_POINT_is_at_infinity(group, point) )
	{
		return FALSE;
	}

	/* 分配内存操作 */
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
	N=BN_new();
	//
	P_NP=EC_POINT_new();
	//
	ctx = BN_CTX_new();

	//
	BN_copy(a,&(group->a));
	BN_copy(b,&(group->b));

	EC_POINT_get_point(point,x,y,z);

	if ( x == NULL || y == NULL || a == NULL || b == NULL || 
		tmp == NULL || left == NULL, right == NULL || N == NULL ||	
		P_NP == NULL ||
		ctx == NULL )
	{
		return FALSE;
	}

	/* 得到阶 */
	EC_GROUP_get_order(group,N);	/* 阶 */

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

	//
	BN_mod_mul(left,y,y,p,ctx);

	//比较
	if(BN_cmp(left, right) == 0 )
	{
		/* 判断[n]P==O */
		EC_POINT_mul(group,P_NP,N,point);
//		EC_POINT_affine2gem(group,Pt,Pt);
//		EC_POINT_get_point(Pt,x1,y1,z2);
		if( EC_POINT_is_at_infinity(group, P_NP) )
		{
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
			BN_free(N);
			//
			EC_POINT_free(P_NP);
			//
			BN_CTX_free(ctx);
			return TRUE;
		}
	}
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
	BN_free(N);
	//
	EC_POINT_free(P_NP);
	//
	BN_CTX_free(ctx);

	return FALSE;
}
