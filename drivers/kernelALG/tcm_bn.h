/* tcm_bn.h*/
/*
改写bn函数
*/

#ifndef HEADER_TCM_BN_H
#define HEADER_TCM_BN_H


#ifdef	__cplusplus
extern "C" {
#endif


/*
如果最高位为0，也需要转换出来。其他同BN_bn2bin
linyang 2007.1.5
*/
int tcm_bn_bn2bin(const BIGNUM *a, int len, unsigned char *to);


#ifdef	__cplusplus
}
#endif


#endif	// HEADER_TCM_ECC_H
