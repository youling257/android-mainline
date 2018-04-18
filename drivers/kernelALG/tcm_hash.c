/******************************************
File	:tcm_hash.c  
Author	:linyang
Date	:11/21/2006
******************************************/

//#include <memory.h>
//#include <malloc.h>
#include <linux/vmalloc.h>
#include "openssl/ec_operations.h"
#include "sch.h"
#include "tcm_hash.h"


void tcm_sch_starts( sch_context *ctx )
{
	sch_starts(ctx);
}
EXPORT_SYMBOL(tcm_sch_starts);

void tcm_sch_update( sch_context *ctx, uint8 *input, uint32 length )
{
	sch_update( ctx, input, length );
}
EXPORT_SYMBOL(tcm_sch_update);
void tcm_sch_finish( sch_context *ctx, uint8 digest[32] )
{
	sch_finish( ctx, 32, digest );
}
EXPORT_SYMBOL(tcm_sch_finish);

int tcm_sch_256( unsigned int datalen_in, unsigned char *pdata_in, unsigned char digest[32])
{
	sch_context ctx;
	unsigned char sch256[32];

	sch_starts( &ctx );
	sch_update( &ctx, (uint8 *)pdata_in, datalen_in);
	sch_finish( &ctx, sizeof(sch256), sch256 );

	memcpy( digest, sch256, sizeof(sch256) );

	return 0;
}


int tcm_sch_192( unsigned int datalen_in, unsigned char *pdata_in, unsigned char digest[24])
{
	sch_context ctx;
	unsigned char sch192[24];

	sch_starts( &ctx );
	sch_update( &ctx, (uint8 *)pdata_in, datalen_in);
	sch_finish( &ctx, sizeof(sch192), sch192 );

	memcpy( digest, sch192, sizeof(sch192) );

	return 0;
}

int tcm_sch_160( unsigned int datalen_in, unsigned char *pdata_in, unsigned char digest[20])
{
	sch_context ctx;
	unsigned char sch160[20];

	sch_starts( &ctx );
	sch_update( &ctx, (uint8 *)pdata_in, datalen_in);
	sch_finish( &ctx, sizeof(sch160), sch160 );

	memcpy( digest, sch160, sizeof(sch160) );

	return 0;
}

//使用256bit的hash
int tcm_sch_hash( unsigned int datalen_in, unsigned char *pdata_in, unsigned char digest[32])
{
	return tcm_sch_256( datalen_in, pdata_in, digest);
}
EXPORT_SYMBOL(tcm_sch_hash);
int tcm_kdf(/*out*/uint8 *mask, /*in*/int klen, /*in*/uint8 *z, /*in*/ int zlen)
{
#define hLen (HASH_NUMBITS/8)
	int ct;
	uint8 T[hLen];
	int k;
	uint8 *seed;

	seed = (uint8*)vmalloc(zlen+4);
	if( seed == 0 )
	{
		return 1;
	}
	memset(seed, 0, zlen+4);
	memcpy(seed, z, zlen);

	for(ct=0;ct<(klen/hLen);ct++)
	{
		seed[zlen]=(uint8)((ct+1)>>24);
		seed[zlen+1]=(uint8)((ct+1)>>16);
		seed[zlen+2]=(uint8)((ct+1)>>8);
		seed[zlen+3]=(uint8)(ct+1);
		tcm_sch_256(zlen+4, seed, &mask[ct*32]);
	}

	if(klen%hLen !=0)
	{
		seed[zlen]=(uint8)((ct+1)>>24);
		seed[zlen+1]=(uint8)((ct+1)>>16);
		seed[zlen+2]=(uint8)((ct+1)>>8);
		seed[zlen+3]=(uint8)(ct+1);
		tcm_sch_256(zlen+4, seed, T);
		for(ct=ct*hLen,k=0;ct<klen;ct++,k++)
			mask[ct]=T[k];
	}

	vfree(seed);
	return 0;
}



/****************************************************************************************************
输入:unsigned char  *text;                //pointer to data stream 
输入:unsigned int   text_len;             //length of data stream 
输入:unsigned char  *key;                 //pointer to authentication key 
输入:unsigned int   key_len;              //length of authentication key 
输入:unsigned int   digest_len;           //length of digest
输出:unsigned char  *digest;              //caller digest to be filled in 

目前支持最大的key长度为256八位组
目前支持摘要最大长度为32八位组

返回值:正确返回0,错误返回1
*****************************************************************************************************/

int tcm_hmac(unsigned char *text,unsigned int text_len,unsigned char *key,unsigned int key_len,unsigned char digest[32])
{
	sch_context context;

	unsigned char k_ipad[65];    /* inner padding key XORd with ipad */
	unsigned char k_opad[65];    /* outer padding key XORd with opad */
	unsigned char tk[32];
	unsigned char tempdigest[32];
	unsigned char tempkey[256];
	int i;
	
	memcpy(tempkey,key,key_len);
	
	/* if key is longer than 64 bytes reset it to key=sch256(key) */
	if (key_len > 64)
	{
		sch_context tctx;           

		sch_starts(&tctx);
		sch_update(&tctx,(uint8 *)tempkey,key_len);
		sch_finish(&tctx, 32, tk);              

		memcpy(tempkey,tk,32);
		key_len = 32;
	}

	/*
	* the HMAC_sch transform looks like:
	*
	* sch(K XOR opad, sch256(K XOR ipad, text))
	*
	* where K is an n byte key
	* ipad is the byte 0x36 repeated 64 times
	* opad is the byte 0x5c repeated 64 times
	* and text is the data being protected
	*/

	/* start out by storing key in pads */
	memset( k_ipad, 0, sizeof(k_ipad));
	memset( k_opad, 0, sizeof(k_opad));
	memcpy( k_ipad, tempkey, key_len);
	memcpy( k_opad, tempkey, key_len);
       
	/* XOR key with ipad and opad values */
	for (i=0; i<64; i++)
	{
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}
        
	/* perform inner sch256 */       
	sch_starts(&context);
        
	/* init context for 1st pass */         
	sch_update(&context,(uint8 *)k_ipad, 64);
	sch_update(&context,(uint8 *)text,text_len);  
	sch_finish(&context, 32, tempdigest);
                                              
	/* perform outer sch256 */       
	sch_starts(&context);
        
	/* init context for 2nd pass */
	sch_update(&context,(uint8 *)k_opad,64);                                                   
	sch_update(&context,(uint8 *)tempdigest,32);                                      
	sch_finish(&context, 32, tempdigest);
	/* finish up 2nd pass */
        
	memcpy(digest,tempdigest,32);
       
	return 0;
}

