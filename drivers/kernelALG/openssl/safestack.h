/* safestack.h */

#ifndef HEADER_SAFESTACK_H
#define HEADER_SAFESTACK_H

#include "stack.h"

#ifdef DEBUG_SAFESTACK

#define STACK_OF(type) struct stack_st_##type
#define PREDECLARE_STACK_OF(type) STACK_OF(type);

#define DECLARE_STACK_OF(type) \
STACK_OF(type) \
    { \
    STACK stack; \
    };

#define IMPLEMENT_STACK_OF(type) /* nada (obsolete in new safestack approach)*/

#define SKM_sk_new_null(type) \
	((STACK_OF(type) * (*)(void))sk_new_null)()
#define SKM_sk_num(type, st) \
	((int (*)(const STACK_OF(type) *))sk_num)(st)
#define SKM_sk_value(type, st,i) \
	((type * (*)(const STACK_OF(type) *, int))sk_value)(st, i)
#define SKM_sk_set(type, st,i,val) \
	((type * (*)(STACK_OF(type) *, int, type *))sk_set)(st, i, val)
#define SKM_sk_push(type, st,val) \
	((int (*)(STACK_OF(type) *, type *))sk_push)(st, val)
#define SKM_sk_find(type, st,val) \
	((int (*)(STACK_OF(type) *, type *))sk_find)(st, val)
#define SKM_sk_pop_free(type, st,free_func) \
	((void (*)(STACK_OF(type) *, void (*)(type *)))sk_pop_free)\
	(st, free_func)
#else

#define STACK_OF(type) STACK
#define PREDECLARE_STACK_OF(type) /* nada */
#define DECLARE_STACK_OF(type)    /* nada */
#define IMPLEMENT_STACK_OF(type)  /* nada */

#define SKM_sk_new_null(type) \
	sk_new_null()
#define SKM_sk_num(type, st) \
	sk_num(st)
#define SKM_sk_value(type, st,i) \
	((type *)sk_value(st, i))
#define SKM_sk_set(type, st,i,val) \
	((type *)sk_set(st, i,(char *)val))
#define SKM_sk_push(type, st,val) \
	sk_push(st, (char *)val)
#define SKM_sk_find(type, st,val) \
	sk_find(st, (char *)val)
#define SKM_sk_pop_free(type, st,free_func) \
	sk_pop_free(st, (void (*)(void *))free_func)
#endif

#define sk_CRYPTO_EX_DATA_FUNCS_new_null() SKM_sk_new_null(CRYPTO_EX_DATA_FUNCS)
#define sk_CRYPTO_EX_DATA_FUNCS_num(st) SKM_sk_num(CRYPTO_EX_DATA_FUNCS, (st))
#define sk_CRYPTO_EX_DATA_FUNCS_value(st, i) SKM_sk_value(CRYPTO_EX_DATA_FUNCS, (st), (i))
#define sk_CRYPTO_EX_DATA_FUNCS_set(st, i, val) SKM_sk_set(CRYPTO_EX_DATA_FUNCS, (st), (i), (val))
#define sk_CRYPTO_EX_DATA_FUNCS_push(st, val) SKM_sk_push(CRYPTO_EX_DATA_FUNCS, (st), (val))
#define sk_CRYPTO_EX_DATA_FUNCS_pop_free(st, free_func) SKM_sk_pop_free(CRYPTO_EX_DATA_FUNCS, (st), (free_func))
#define sk_CRYPTO_dynlock_new_null() SKM_sk_new_null(CRYPTO_dynlock)
#define sk_CRYPTO_dynlock_num(st) SKM_sk_num(CRYPTO_dynlock, (st))
#define sk_CRYPTO_dynlock_value(st, i) SKM_sk_value(CRYPTO_dynlock, (st), (i))
#define sk_CRYPTO_dynlock_set(st, i, val) SKM_sk_set(CRYPTO_dynlock, (st), (i), (val))
#define sk_CRYPTO_dynlock_push(st, val) SKM_sk_push(CRYPTO_dynlock, (st), (val))
#define sk_CRYPTO_dynlock_find(st, val) SKM_sk_find(CRYPTO_dynlock, (st), (val))

#endif /* !defined HEADER_SAFESTACK_H */
