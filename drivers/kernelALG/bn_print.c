/* bn_print.c */

//#include <stdio.h>
#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"


static const char *Hex="0123456789ABCDEF";

/* Must 'OPENSSL_free' the returned data */
char *BN_bn2hex(const BIGNUM *a)
	{
	int i,j,v,z=0;
	char *buf;
	char *p;

	buf=(char *)OPENSSL_malloc(a->top*BN_BYTES*2+2);
	if (buf == NULL)
		{
		goto err;
		}
	p=buf;
	if (a->neg) *(p++)='-';
	if (a->top == 0) *(p++)='0';
	for (i=a->top-1; i >=0; i--)
		{
		for (j=BN_BITS2-8; j >= 0; j-=8)
			{
			
			v=((int)(a->d[i]>>(long)j))&0xff;
			if (z || (v != 0))
				{
				*(p++)=Hex[v>>4];
				*(p++)=Hex[v&0x0f];
				z=1;
				}
			}
		}
	*p='\0';
err:
	return(buf);
	
	}

int BN_hex2bn(BIGNUM **bn, const char *a)
	{
	BIGNUM *ret=NULL;
	BN_ULONG l=0;
	int neg=0,h,m,i,j,k,c;
	int num;

	if ((a == NULL) || (*a == '\0')) return(0);

	if (*a == '-') { neg=1; a++; }

	for (i=0;(((unsigned char)a[i])>='a'&&((unsigned char)a[i])<='f')||(((unsigned char)a[i])>='A'&&((unsigned char)a[i])<='F')||(((unsigned char)a[i])>='0'&&((unsigned char)a[i])<='9');i++)
		;

	num=i+neg;
	if (bn == NULL) return(num);

	/* a is the start of the hex digits, and it is 'i' long */
	if (*bn == NULL)
		{
		if ((ret=BN_new()) == NULL) return(0);
		}
	else
		{
		ret= *bn;
		BN_zero(ret);
		}

	/* i is the number of hex digests; */
	if (bn_expand(ret,i*4) == NULL) goto err;

	j=i; /* least significant 'hex' */
	m=0;
	h=0;
	while (j > 0)
		{
		m=((BN_BYTES*2) <= j)?(BN_BYTES*2):j;
		l=0;
		for (;;)
			{
			c=a[j-m];
			if ((c >= '0') && (c <= '9')) k=c-'0';
			else if ((c >= 'a') && (c <= 'f')) k=c-'a'+10;
			else if ((c >= 'A') && (c <= 'F')) k=c-'A'+10;
			else k=0; 
			l=(l<<4)|k;

			if (--m <= 0)
				{
				ret->d[h++]=l;
				break;
				}
			}
		j-=(BN_BYTES*2);
		}
	ret->top=h;
	bn_fix_top(ret);
	ret->neg=neg;

	*bn=ret;
	return(num);
err:
	if (*bn == NULL) BN_free(ret);
	return(0);
	}

int BN_dec2bn(BIGNUM **bn, const char *a)
	{
	BIGNUM *ret=NULL;
	BN_ULONG l=0;
	int neg=0,i,j;
	int num;

	if ((a == NULL) || (*a == '\0')) return(0);
	if (*a == '-') { neg=1; a++; }

	for (i=0; ((unsigned char)a[i])>=48 && ((unsigned char)a[i])<=57; i++)
		;

	num=i+neg;
	if (bn == NULL) return(num);

	/* a is the start of the digits, and it is 'i' long.
	 * We chop it into BN_DEC_NUM digits at a time */
	if (*bn == NULL)
		{
		if ((ret=BN_new()) == NULL) return(0);
		}
	else
		{
		ret= *bn;
		BN_zero(ret);
		}

	/* i is the number of digests, a bit of an over expand; */
	if (bn_expand(ret,i*4) == NULL) goto err;

	j=BN_DEC_NUM-(i%BN_DEC_NUM);
	if (j == BN_DEC_NUM) j=0;
	l=0;

	while (*a)
		{
		l*=10;
		l+= *a-'0';
		a++;
		if (++j == BN_DEC_NUM)
			{
			BN_mul_word(ret,BN_DEC_CONV);
			BN_add_word(ret,l);
			l=0;
			j=0;
			}
		}

	ret->neg=neg;

	bn_fix_top(ret);
	*bn=ret;
	return(num);
err:
	if (*bn == NULL) BN_free(ret);
	return(0);
	}

#ifndef OPENSSL_NO_BIO
#ifndef OPENSSL_NO_FP_API
/* int BN_print_fp(FILE *fp, const BIGNUM *a)
	{
	BIO *b;
	int ret;

	if ((b=BIO_new(BIO_s_file())) == NULL)
		return(0);
	BIO_set_fp(b,fp,BIO_NOCLOSE);
	ret=BN_print(b,a);
	BIO_free(b);
	return(ret);
	} */
#endif
/*
int BN_print(BIO *bp, const BIGNUM *a)
	{
	int i,j,v,z=0;
	int ret=0;

	if ((a->neg) && (BIO_write(bp,"-",1) != 1)) goto end;
	if ((a->top == 0) && (BIO_write(bp,"0",1) != 1)) goto end;
	for (i=a->top-1; i >=0; i--)
		{
		for (j=BN_BITS2-4; j >= 0; j-=4)
			{
			v=((int)(a->d[i]>>(long)j))&0x0f;
			if (z || (v != 0))
				{
				if (BIO_write(bp,&(Hex[v]),1) != 1)
					goto end;
				z=1;
				}
			}
		}
	ret=1;
end:
	return(ret);
	} */
#endif
