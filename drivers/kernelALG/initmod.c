#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "tcm_ecc.h"
#include "tcm_hash.h"

#define PRIKEYSIZE 32
#define PUBKEYSIZE 65
#define TEXTSIZE 128
#define DIGESTSIZE 32
#define SIGSIZE 64
#define PCRINDEX 23

MODULE_LICENSE("Dual BSD/GPL");

int __init init_module(void)
{
  printk("<1>Hello world 1.\n");

  int ret = 0;
  int i = 0;
  struct file *fp;
  mm_segment_t old_fs;
  loff_t pos;
  char buf[3];


  unsigned char* priKey = (unsigned char*)vmalloc(PRIKEYSIZE);
  unsigned int priKeyLength = 100;

  unsigned char* pubKey = (unsigned char*)vmalloc(PUBKEYSIZE);
  unsigned int pubKeyLength = 100;

  unsigned char* plainText = (unsigned char*)vmalloc(TEXTSIZE);
  unsigned int plainTextLength = TEXTSIZE;

  unsigned int cipherTextLength = plainTextLength+1024;
  unsigned char* cipherText = (unsigned char*)vmalloc(cipherTextLength);

  unsigned char digest[DIGESTSIZE];

  unsigned int sigLength = SIGSIZE;
  unsigned char* sig = (unsigned char*)vmalloc(SIGSIZE);

	

  printk("------ TCMALG.a Test Begins ------\n\n\n");

  //	INIT
  ret = tcm_ecc_init();
  if (ret != 0){
    printk(KERN_ALERT "fail to call function tcm_ecc_init() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }

  //	GENERATE KEY PAIR
  //	NOTE: priKeyLength and pubKeyLength should be value greater than zero
  //
  printk("CREATE KEY PAIR: SUCCESS\n");
  ret = tcm_ecc_genkey(priKey, &priKeyLength, pubKey, &pubKeyLength);
  if (ret != 0){
    printk("fail to call function tcm_ecc_genkey() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }

  printk("the priKey is: (length of %d)\n", priKeyLength);
  for (i = 0; i<PRIKEYSIZE; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(priKey+i));		
  }
  printk("\n");

  printk("the pubKey is:(length of %d)\n", pubKeyLength);
  //fp = fopen("pubkey.pub","a+");
  
  fp=filp_open("pubkey.pub",O_RDWR|O_CREAT|O_APPEND,0644);
  old_fs=get_fs();
  set_fs(KERNEL_DS);
  pos=0;
  for (i = 0; i<PUBKEYSIZE; i++){
    if ((i!=0)&&(i%16==0)){
      vfs_write(fp,"\n",sizeof(char),&pos);
      printk("\n");
    }
    sprintf(buf,"%02X ",*(pubKey+i));
    vfs_write(fp,buf,sizeof(buf),&pos);

    printk("%02X ", *(pubKey+i));		
  }

  vfs_write(fp,"\n",sizeof(char),&pos);

  printk("\n\n");
  filp_close(fp,NULL);
  set_fs(old_fs);

  //  ENCRYPT
  for (i = 0; i<TEXTSIZE; i++){
    *(plainText+i) = i;
  }

  //  
  ret = tcm_ecc_encrypt(plainText, plainTextLength, pubKey, pubKeyLength, cipherText, &cipherTextLength);
  if (ret != 0){
    printk("fail to call function tcm_ecc_encrypt() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }
  printk("ENCRYPT: SUCCESS\n");
  printk("the plain text is: (length of %d)\n", TEXTSIZE);
  for (i = 0; i<plainTextLength; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(plainText+i));		
  }
  printk("\n");
  printk("the cipher text is: (length of %d)\n", cipherTextLength);
  for (i = 0; i<cipherTextLength; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(cipherText+i));		
  }
  printk("\n\n");

  //	DECRYPT
  ret = tcm_ecc_decrypt(cipherText, cipherTextLength, priKey, priKeyLength, plainText, &plainTextLength);
  if (ret != 0){
    printk("fail to call function tcm_ecc_decrypt() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }
  printk("DECRYPT: SUCCESS\n");
  printk("the plain text is: (length of %d)\n", plainTextLength);
  for (i = 0; i<plainTextLength; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(plainText+i));		
  }
  printk("\n\n");

  //	HASH
  ret = tcm_sch_hash(plainTextLength, plainText, digest);
  if (ret != 0){
    printk("fail to call function tcm_sch_hash() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }
  printk("HASH: SUCCESS\n");
  printk("the digest text is: (length of %d)\n", DIGESTSIZE);
  for (i = 0; i<DIGESTSIZE; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(digest+i));		
  }
  printk("\n\n");

  //	SIGNATURE
  ret = tcm_ecc_signature(digest, DIGESTSIZE, priKey, priKeyLength, sig, &sigLength);
  if (ret != 0){
    printk("fail to call function tcm_ecc_signature() in alglib\n");
    printk("the return value is %d\n\n", ret);
    return 0;
  }
  printk("SIGNATURE: SUCCESS\n");
  printk("the signature is: (length of %d)\n", sigLength);
  for (i = 0; i<sigLength; i++){
    if ((i!=0)&&(i%16==0)){
      printk("\n");
    }
    printk("%02X ", *(sig+i));		
  }
  printk("\n\n");

  //	VERIFY
  ret = tcm_ecc_verify(digest, DIGESTSIZE, sig, sigLength, pubKey, pubKeyLength);
  if (ret != 0){
    printk("fail to verify signature\n");
    printk("the return value is %d\n\n", ret);
  }
  else{
    printk("VERIFY: SUCCESS\n");
  }
  printk("\n\n");

  //	RELEASE ECC
  //ret = tcm_ecc_release();
  //

  vfree(priKey);
  vfree(pubKey);
  vfree(plainText);
  vfree(cipherText);
  vfree(sig);
	
  // A non 0 return means init_module failed; module can't be loaded.
  return 0;
}


void __exit cleanup_module(void)
{
  printk(KERN_ALERT "Goodbye world 1.\n");
}


//module_init(init_module);
//mudule_exit(cleanup_module);
