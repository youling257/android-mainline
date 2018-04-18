/* bnEx.h */
#ifndef HEADER_BN_EX_H
#define HEADER_BN_EX_H

#include "bn.h"

#ifdef	__cplusplus
extern "C" {
#endif

int BN_div_mod(BIGNUM *res,const BIGNUM *g, const BIGNUM *h,const BIGNUM *m);
int BN_div_mod2(BIGNUM *d,const BIGNUM *g, const BIGNUM *h,const BIGNUM *m);

#ifdef	__cplusplus
}
#endif

#endif