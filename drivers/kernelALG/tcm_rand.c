/******************************************
File	:tcm_rand.c 
Author	:linyang
Date	:11/21/2006
******************************************/

//#include <stdio.h>
//#include <time.h>
#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"
#include "tcm_rand.h"

int tcm_bn_pseudo_rand(BIGNUM *rnd, int bits)
{
	unsigned char *pbuffer = NULL;
	pbuffer = OPENSSL_malloc(bits/8);

	//
	if( tcm_rng(bits/8, pbuffer) == 1 )
	{
		return 0;
	}
	//
	BN_bin2bn(pbuffer, bits/8, rnd);
	OPENSSL_free(pbuffer);
	return 1;
}


