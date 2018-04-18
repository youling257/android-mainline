/* bn_add.c */

//#include <stdio.h>
#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"

/* r can == a or b */
int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
	{
	const BIGNUM *tmp;
	int a_neg = a->neg;

	bn_check_top(a);
	bn_check_top(b);

	/*  a +  b	a+b
	 *  a + -b	a-b
	 * -a +  b	b-a
	 * -a + -b	-(a+b)
	 */
	if (a_neg ^ b->neg)
		{
		/* only one is negative */
		if (a_neg)
			{ tmp=a; a=b; b=tmp; }

		/* we are now a - b */

		if (BN_ucmp(a,b) < 0)
			{
			if (!BN_usub(r,b,a)) return(0);
			r->neg=1;
			}
		else
			{
			if (!BN_usub(r,a,b)) return(0);
			r->neg=0;
			}
		return(1);
		}

	if (!BN_uadd(r,a,b)) return(0);
	if (a_neg) /* both are neg */
		r->neg=1;
	else
		r->neg=0;
	return(1);
	}

/* unsigned add of b to a, r must be large enough */
int BN_uadd(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
	{
	register int i;
	int max,min;
	BN_ULONG *ap,*bp,*rp,carry,t1;
	const BIGNUM *tmp;

	bn_check_top(a);
	bn_check_top(b);

	if (a->top < b->top)
		{ tmp=a; a=b; b=tmp; }
	max=a->top;
	min=b->top;

	if (bn_wexpand(r,max+1) == NULL)
		return(0);

	r->top=max;


	ap=a->d;
	bp=b->d;
	rp=r->d;
	carry=0;

	carry=bn_add_words(rp,ap,bp,min);
	rp+=min;
	ap+=min;
	bp+=min;
	i=min;

	if (carry)
		{
		while (i < max)
			{
			i++;
			t1= *(ap++);
			if ((*(rp++)=(t1+1)&BN_MASK2) >= t1)
				{
				carry=0;
				break;
				}
			}
		if ((i >= max) && carry)
			{
			*(rp++)=1;
			r->top++;
			}
		}
	if (rp != ap)
		{
		for (; i<max; i++)
			*(rp++)= *(ap++);
		}
	/* memcpy(rp,ap,sizeof(*ap)*(max-i));*/
	r->neg = 0;
	return(1);
	}

/* unsigned subtraction of b from a, a must be larger than b. */
int BN_usub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
	{
	int max,min;
	register BN_ULONG t1,t2,*ap,*bp,*rp;
	int i,carry;
#if defined(IRIX_CC_BUG) && !defined(LINT)
	int dummy;
#endif

	bn_check_top(a);
	bn_check_top(b);

	if (a->top < b->top) /* hmm... should not be happening */
		{
		return(0);
		}

	max=a->top;
	min=b->top;
	if (bn_wexpand(r,max) == NULL) return(0);

	ap=a->d;
	bp=b->d;
	rp=r->d;

#if 1
	carry=0;
	for (i=0; i<min; i++)
		{
		t1= *(ap++);
		t2= *(bp++);
		if (carry)
			{
			carry=(t1 <= t2);
			t1=(t1-t2-1)&BN_MASK2;
			}
		else
			{
			carry=(t1 < t2);
			t1=(t1-t2)&BN_MASK2;
			}
#if defined(IRIX_CC_BUG) && !defined(LINT)
		dummy=t1;
#endif
		*(rp++)=t1&BN_MASK2;
		}
#else
	carry=bn_sub_words(rp,ap,bp,min);
	ap+=min;
	bp+=min;
	rp+=min;
	i=min;
#endif
	if (carry) /* subtracted */
		{
		while (i < max)
			{
			i++;
			t1= *(ap++);
			t2=(t1-1)&BN_MASK2;
			*(rp++)=t2;
			if (t1 > t2) break;
			}
		}
#if 0
	memcpy(rp,ap,sizeof(*rp)*(max-i));
#else
	if (rp != ap)
		{
		for (;;)
			{
			if (i++ >= max) break;
			rp[0]=ap[0];
			if (i++ >= max) break;
			rp[1]=ap[1];
			if (i++ >= max) break;
			rp[2]=ap[2];
			if (i++ >= max) break;
			rp[3]=ap[3];
			rp+=4;
			ap+=4;
			}
		}
#endif

	r->top=max;
	r->neg=0;
	bn_fix_top(r);
	return(1);
	}

int BN_sub(BIGNUM *r, const BIGNUM *a, const BIGNUM *b)
	{
	int max;
	int add=0,neg=0;
	const BIGNUM *tmp;

	bn_check_top(a);
	bn_check_top(b);

	/*  a -  b	a-b
	 *  a - -b	a+b
	 * -a -  b	-(a+b)
	 * -a - -b	b-a
	 */
	if (a->neg)
		{
		if (b->neg)
			{ tmp=a; a=b; b=tmp; }
		else
			{ add=1; neg=1; }
		}
	else
		{
		if (b->neg) { add=1; neg=0; }
		}

	if (add)
		{
		if (!BN_uadd(r,a,b)) return(0);
		r->neg=neg;
		return(1);
		}

	/* We are actually doing a - b :-) */

	max=(a->top > b->top)?a->top:b->top;
	if (bn_wexpand(r,max) == NULL) return(0);
	if (BN_ucmp(a,b) < 0)
		{
		if (!BN_usub(r,b,a)) return(0);
		r->neg=1;
		}
	else
		{
		if (!BN_usub(r,a,b)) return(0);
		r->neg=0;
		}
	return(1);
	}

