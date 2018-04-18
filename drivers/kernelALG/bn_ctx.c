/* bn_ctx.c */

#ifndef BN_CTX_DEBUG
# undef NDEBUG /* avoid conflicting definitions */
# define NDEBUG
#endif

//#include <stdio.h>
//#include <assert.h>
#include <linux/string.h>
#include <linux/types.h>

#include "openssl/cryptlib.h"
#include "openssl/bn_lcl.h"

BN_CTX *BN_CTX_new(void)
	{
	BN_CTX *ret;

	ret=(BN_CTX *)OPENSSL_malloc(sizeof(BN_CTX));
	if (ret == NULL)
		{
		return(NULL);
		}

	BN_CTX_init(ret);
	ret->flags=BN_FLG_MALLOCED;
	return(ret);
	}

void BN_CTX_init(BN_CTX *ctx)
	{
#if 0 /* explicit version */
	int i;
	ctx->tos = 0;
	ctx->flags = 0;
	ctx->depth = 0;
	ctx->too_many = 0;
	for (i = 0; i < BN_CTX_NUM; i++)
		BN_init(&(ctx->bn[i]));
#else
	memset(ctx, 0, sizeof *ctx);
#endif
	}

void BN_CTX_free(BN_CTX *ctx)
	{
	int i;

	if (ctx == NULL) return;
	//assert(ctx->depth == 0);

	for (i=0; i < BN_CTX_NUM; i++)
		BN_clear_free(&(ctx->bn[i]));
	if (ctx->flags & BN_FLG_MALLOCED)
		OPENSSL_free(ctx);
	}

void BN_CTX_start(BN_CTX *ctx)
	{
	if (ctx->depth < BN_CTX_NUM_POS)
		ctx->pos[ctx->depth] = ctx->tos;
	ctx->depth++;
	}

BIGNUM *BN_CTX_get(BN_CTX *ctx)
	{
	/* Note: If BN_CTX_get is ever changed to allocate BIGNUMs dynamically,
	 * make sure that if BN_CTX_get fails once it will return NULL again
	 * until BN_CTX_end is called.  (This is so that callers have to check
	 * only the last return value.)
	 */
	if (ctx->depth > BN_CTX_NUM_POS || ctx->tos >= BN_CTX_NUM)
		{
		if (!ctx->too_many)
			{
			/* disable error code until BN_CTX_end is called: */
			ctx->too_many = 1;
			}
		return NULL;
		}
	return (&(ctx->bn[ctx->tos++]));
	}

void BN_CTX_end(BN_CTX *ctx)
	{
	if (ctx == NULL) return;
	//assert(ctx->depth > 0);
	if (ctx->depth == 0)
		/* should never happen, but we can tolerate it if not in
		 * debug mode (could be a 'goto err' in the calling function
		 * before BN_CTX_start was reached) */
		BN_CTX_start(ctx);

	ctx->too_many = 0;
	ctx->depth--;
	if (ctx->depth < BN_CTX_NUM_POS)
		ctx->tos = ctx->pos[ctx->depth];
	}
