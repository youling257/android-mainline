/******************************************
File	:tcm_bn.c 
Author	:linyang
Date	:1/5/2006
******************************************/

#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"

/*
按照输入的字节数进行转换。
如果失败，返回-1。
linyang 2007.1.5
*/
int tcm_bn_bn2bin(const BIGNUM *a, int len, unsigned char *to)
{
	int n,i;
	BN_ULONG l;
	int lnum;
	
	n=i=BN_num_bytes(a);

	lnum = len - n;
	if( lnum < 0 )
	{
		return -1;
	}
	while (lnum-- > 0)
		{
		*(to++)=0;
		}

	while (i-- > 0)
		{
		l=a->d[i/BN_BYTES];
		*(to++)=(unsigned char)(l>>(8*(i%BN_BYTES)))&0xff;
		}
	return(n);
}

