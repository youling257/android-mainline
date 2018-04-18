/* bn_word.c */

//#include <stdio.h>
#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"

BN_ULONG BN_mod_word(const BIGNUM *a, BN_ULONG w)
	{
#ifndef BN_LLONG
	BN_ULONG ret=0;
#else
	BN_ULLONG ret=0;
#endif
	int i;

	w&=BN_MASK2;
	for (i=a->top-1; i>=0; i--)
		{
#ifndef BN_LLONG
		ret=((ret<<BN_BITS4)|((a->d[i]>>BN_BITS4)&BN_MASK2l))%w;
		ret=((ret<<BN_BITS4)|(a->d[i]&BN_MASK2l))%w;
#else
		ret=(BN_ULLONG)(((ret<<(BN_ULLONG)BN_BITS2)|a->d[i])%
			(BN_ULLONG)w);
#endif
		}
	return((BN_ULONG)ret);
	}

BN_ULONG BN_div_word(BIGNUM *a, BN_ULONG w)
	{
	BN_ULONG ret;
	int i;

	if (a->top == 0) return(0);
	ret=0;
	w&=BN_MASK2;
	for (i=a->top-1; i>=0; i--)
		{
		BN_ULONG l,d;
		
		l=a->d[i];
		d=bn_div_words(ret,l,w);
		ret=(l-((d*w)&BN_MASK2))&BN_MASK2;
		a->d[i]=d;
		}
	if ((a->top > 0) && (a->d[a->top-1] == 0))
		a->top--;
	return(ret);
	}

int BN_add_word(BIGNUM *a, BN_ULONG w)
	{
	BN_ULONG l;
	int i;

	if (a->neg)
		{
		a->neg=0;
		i=BN_sub_word(a,w);
		if (!BN_is_zero(a))
			a->neg=!(a->neg);
		return(i);
		}
	w&=BN_MASK2;
	if (bn_wexpand(a,a->top+1) == NULL) return(0);
	i=0;
	for (;;)
		{
		if (i >= a->top)
			l=w;
		else
			l=(a->d[i]+(BN_ULONG)w)&BN_MASK2;
		a->d[i]=l;
		if (w > l)
			w=1;
		else
			break;
		i++;
		}
	if (i >= a->top)
		a->top++;
	return(1);
	}

int BN_sub_word(BIGNUM *a, BN_ULONG w)
	{
	int i;

	if (BN_is_zero(a) || a->neg)
		{
		a->neg=0;
		i=BN_add_word(a,w);
		a->neg=1;
		return(i);
		}

	w&=BN_MASK2;
	if ((a->top == 1) && (a->d[0] < w))
		{
		a->d[0]=w-a->d[0];
		a->neg=1;
		return(1);
		}
	i=0;
	for (;;)
		{
		if (a->d[i] >= w)
			{
			a->d[i]-=w;
			break;
			}
		else
			{
			a->d[i]=(a->d[i]-w)&BN_MASK2;
			i++;
			w=1;
			}
		}
	if ((a->d[i] == 0) && (i == (a->top-1)))
		a->top--;
	return(1);
	}

int BN_mul_word(BIGNUM *a, BN_ULONG w)
	{
	BN_ULONG ll;

	w&=BN_MASK2;
	if (a->top)
		{
		if (w == 0)
			BN_zero(a);
		else
			{
			ll=bn_mul_words(a->d,a->d,a->top,w);
			if (ll)
				{
				if (bn_wexpand(a,a->top+1) == NULL) return(0);
				a->d[a->top++]=ll;
				}
			}
		}
	return(1);
	}

