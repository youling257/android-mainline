#CONFIG_MODULE_SIG=n
#sources :=$(wildcard *.c)
#lib-y := initmod.o bn_kron.o seal.o bn_ctx.o bn_gcd.o bn_sqr.o mem.o SMS4.o tcm_ecc_exchange.o ec_operations.o bn_add.o initmod.o bn_asm.o ec_encrypt.o ec_verify.o bn_div.o ec_signature.o tcm_hash.o bn_shift.o tcm_ecc.o bn_mod.o bn_print.o tcm_bn.o ec_genkey.o sch.o bn_lib.o bn_mont.o bn_word.o bn_exp.o debug.o mem_clr.o bn_recp.o calculate_hash.o ec_decrypt.o bn_mul.o bn_sqrt.o bnEx.o ec_init.o tcm_rand.o tcm_sms4.o
#obj-$(CONFIG_TCMALG)	 += tcmalg.o
obj-y		+= tcmalg.o
#tcmalg-objs :=$(sources:.c=.o)
tcmalg-objs := initmod.o bn_kron.o seal.o bn_ctx.o bn_gcd.o bn_sqr.o mem.o SMS4.o tcm_ecc_exchange.o ec_operations.o bn_add.o initmod.o bn_asm.o ec_encrypt.o ec_verify.o bn_div.o ec_signature.o tcm_hash.o bn_shift.o tcm_ecc.o bn_mod.o bn_print.o tcm_bn.o ec_genkey.o sch.o bn_lib.o bn_mont.o bn_word.o bn_exp.o debug.o mem_clr.o bn_recp.o calculate_hash.o ec_decrypt.o bn_mul.o bn_sqrt.o bnEx.o ec_init.o tcm_rand.o tcm_sms4.o 

#all:
#	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

#clean:
#	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
