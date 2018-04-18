/* tcm_hash.*/
/* 这个文档中的函数需要被导出 */

#ifndef HEADER_TCM_HASH_H
#define HEADER_TCM_HASH_H
#include "sch.h"

#ifdef	__cplusplus
extern "C" {
#endif

void tcm_sch_starts( sch_context *ctx );
void tcm_sch_update( sch_context *ctx, uint8 *input, uint32 length );
void tcm_sch_finish( sch_context *ctx, uint8 digest[32] );


/*哈希函数*/
/*************************************************************************
函数 : tcm_sch_hash()
输入参数  
		length:		消息长度
		input:		消息

输出参数 
		digest:			输出HASH值
返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_sch_hash( unsigned int length, unsigned char *input, unsigned char digest[32]);
int tcm_sch_192( unsigned int length, unsigned char *input, unsigned char digest[24]);
int tcm_sch_256( unsigned int length, unsigned char *input, unsigned char digest[32]);


/*************************************************************************
hmac函数
函数 : tcm_hmac()
输入参数  
		text:			消息
		text_len:		消息长度
 		key:			密钥
		key_len:		密钥长度

输出参数 
		digest:			输出HMAC值

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_hmac(unsigned char *text, unsigned int text_len, unsigned char *key, unsigned int key_len, unsigned char digest[32]);


/*************************************************************************
密钥派生函数
函数 : tcm_kdf()
输入参数  
		klen:			需要输出的长度，调用前必须为mask分配大于klen的空间
		msgLen:			消息长度
 		z:				公享的种子信息
		zlen:			z的长度

输出参数 
		mask:			输出的缓冲区

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_kdf(/*out*/unsigned char *mask, /*in*/int klen, /*in*/unsigned char *z, /*in*/ int zlen);


/*************************************************************************
计算消息的hash值
函数 : tcm_get_message_hash()
输入参数  
		msg:			消息
		msgLen:			消息长度
 		userID:			用户信息
		uUserIDLen:		用户信息长度
		pubkey:			公钥地址
		uPubkeyLen:		公钥长度
		puDigestLen:	输入的digest缓冲区大小

输出参数 
		digest:			输出摘要
		puDigestLen;	输出的digest缓冲区长度

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_get_message_hash(unsigned char *msg, unsigned int msgLen,  
					   unsigned char  *userID, unsigned short int uUserIDLen, 
					   unsigned char *pubkey, unsigned int uPubkeyLen,
					   unsigned char *digest,
					   unsigned int *puDigestLen);


/*************************************************************************
计算Za，总是使用256bits的HASH算法
函数 : tcm_get_usrinfo_value()
输入参数  
 		userID:			用户信息
		uUserIDLen:		用户信息长度
		pubkey:			公钥地址
		uPubkeyLen:		公钥长度

输出参数 
		digest:			输出摘要

返回值：计算成功返回0，否则返回1
*************************************************************************/
int tcm_get_usrinfo_value(
					   unsigned char *userID, unsigned short int uUserIDLen, 
					   unsigned char *pubkey, unsigned int uPubkeyLen,
					   unsigned char digest[32]);


#ifdef	__cplusplus
}
#endif


#endif	// HEADER_TCM_HASH_H






