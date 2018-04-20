/*
 * Copyright (C) 2014 TCA
 *
 * Authors:
 * Yang Bo
 *
 * Maintained by: Yang Bo
 *
 * LSM Integrity Measurement Module.
 *
 * File: imm_ctrl.h
 *              define and implement functions for processes control with white list strategy.
 */

#include <linux/fs.h>
#include <linux/elf.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "imm_ctrl.h"
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#include <linux/string.h>
#include "ima.h"

WLNode *WLhead, *WLend;

/*
 * processes control signal:
 * 0: deactivate processes control
 * 1: activate processes control
 */ 

atomic_t wl_flag;

/*
 * white list exist flag:
 * 0: kernel has no white list 
 * 1: kernel has white list
 */ 

atomic_t wl_exist_flag;

#ifdef DEBUG_IMM
// print the content of white list
static void print_WhiteList(WLNode *head);
#endif

//
// Function Implementation
//
void DecToHex(char *decHash, char *hexHashStr)
{
	int hashCount;
	for(hashCount=0; hashCount<HASH_SIZE; hashCount++)
	{
		sprintf(hexHashStr+hashCount*2, "%02x", (unsigned char)decHash[hashCount]);
	}
	hexHashStr[HASH_STRING_SIZE] = '\0';
}

void HecToDec(char *hexHashStr, char *decHash)
{
	int i;
	int decValue = 0;
	int temp = 0;
	for(i=0; i<HASH_SIZE; i++)
	{
		decValue = 0;

		if(hexHashStr[2*i]>='A' && hexHashStr[2*i]<='F')//the first char
			temp = hexHashStr[2*i]-'A'+10;
		else if(hexHashStr[2*i]>='a' && hexHashStr[2*i]<='f')
			temp = hexHashStr[2*i]-'a'+10;
		else temp = hexHashStr[2*i]-'0';
		decValue = temp*16;

		if(hexHashStr[2*i+1]>='A' && hexHashStr[2*i+1]<='F')//the second char
			temp = hexHashStr[2*i+1]-'A'+10;
		else if(hexHashStr[2*i+1]>='a' && hexHashStr[2*i+1]<='f')
			temp = hexHashStr[2*i+1]-'a'+10;
		else temp = hexHashStr[2*i+1]-'0';
		decValue = decValue + temp;

		decHash[i] = decValue;		
	}	
}

WLNode *create_WhiteList(void)
{
	WLNode *p;
	p = (WLNode*)kmalloc(sizeof(WLNode), GFP_KERNEL);
	if(NULL==p){
		printk(KERN_INFO "内存申请失败！\n");
		return NULL;		
	}else{
		p->next = NULL;
		return(p);
	}
}


WLNode *insert_WhiteList(WLNode *head, WLNode *end, char* hash_value, char *dir_name)
{		
	WLNode *p;
	char hash_temp[HASH_SIZE];	

	if(end==NULL){
		printk(KERN_INFO "链表结尾为空！\n");
		return NULL;
	}

	HecToDec(hash_value, hash_temp);	
	if(!compare_WhiteList(head, hash_temp)){
		printk(KERN_INFO "白名单已存在该项！\n");
		return NULL;
	}


	p = (WLNode*)kmalloc(sizeof(WLNode), GFP_KERNEL);
	if(NULL==p){
		printk(KERN_INFO "内存申请失败！\n");
		return NULL;
	}else{
		p->next = NULL;
		end->next = p;
		end = p;
		HecToDec(hash_value,p->value);
		memmove(p->name, dir_name, strlen(dir_name));
		p->name[strlen(dir_name)] = '\0';	
		return end;	
	}
}

int compare_WhiteList(WLNode *head, char *target_hash)
{
	WLNode *p;

	if(head==NULL)
	{
		printk(KERN_INFO "链表不存在！\n");
		return -1;
	}

	p = head->next;
	while(p!=NULL)
	{		
		if(memcmp(target_hash, p->value, 20)==0)
			return 0;
		p = p->next;
	}
	return -1;
}

#ifdef DEBUG_IMM
static void print_WhiteList(WLNode *head)
{	
	WLNode *p;
	char HexHashStr[HASH_STRING_SIZE+1];	

	if(head==NULL)
	{
		printk(KERN_INFO "链表不存在！\n");
		return;
	}

	p = head->next;
	while(p!=NULL)
	{
		DecToHex(p->value, HexHashStr);		
		printk(KERN_INFO "%s  %s\n", HexHashStr, p->name);		
		p = p->next;
	}	
}

#endif

void delete_WhiteList(WLNode *head)
{
	WLNode *p,*q;
	if (head==NULL)
		return ;
	
	p = head->next;
	while(p!=NULL)
	{
		q = p->next;
		kfree(p);
		p = q;
	}
	head->next = NULL;
}

int IS_ELF_exe(struct file *fp)
{
	static int ELF_HEAD_SIZE = 52;	
	char str[ELF_HEAD_SIZE];
	Elf32_Ehdr lan_elf;
	loff_t pos = 0;
	mm_segment_t fs;
	int res = -1;

	memset(str, 0, ELF_HEAD_SIZE);
	
	fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_read(fp, str, ELF_HEAD_SIZE, &pos);
	set_fs(fs);
	
	if((str[0] == 0x7f) && (str[1] == 'E') && (str[2] == 'L') && (str[3] == 'F'))
	{
		lan_elf.e_type = *((Elf32_Half *)&str[16]);
		switch(lan_elf.e_type)
		{
			case 0:
				//printk("未知文件类型\n");
				break;
			case 1:
				//printk("可重定位文件类型\n");
				break;
			case 2:
				//printk("可执行文件\n");
				res = 0;
				break;
			case 3:
				//printk("动态链接库文件\n");
				break;
			case 4:
				//printk("CORE文件\n");
				break;
			default:
				break;                        
		}
	}
	return res;
}

int is_executable(struct file *file)
{
  /* retval: -3:else; -2: nomem; -1:not execuable; 0:executable */
  int retval, error;
  struct elfhdr elf_ex;
  loff_t pos = 0;

  char filename[NAME_MAX];
  const char *pathname = NULL;
  char *pathbuf = NULL;
  char *temppath;

  error = -ENOEXEC;
  retval = kernel_read(file, &elf_ex, sizeof(elf_ex), &pos);
  if(retval != sizeof(elf_ex))
    goto out;

  if(memcmp(elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
    goto out;

  //zhao, ET_EXEC type should return success
  if(elf_ex.e_type == ET_EXEC)
    return 0;
  //zhao, ET_DYN type, if not .so file, should return success
  if(elf_ex.e_type == ET_DYN){
    pathname = ima_d_path(&file->f_path, &pathbuf, filename);
    //pr_info("pathname: %s, filename:%s", pathname, filename);
    //exclude dynamic libraries
    if(strstr(pathname, ".so.") != NULL){
      goto out;
    }
    temppath = pathname + strlen(pathname) - strlen(".so");
    if(strcmp(temppath, ".so") == 0){
      goto out;
    }
    return 0;
  }

  
 out:
  return error;
}


static DEFINE_MUTEX(error_log_mutex);
void error_log(char *hash_val, char *hash_name)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	struct timex  txc;
	struct rtc_time tm;
	mutex_lock(&error_log_mutex);
	
	char time_temp[30];
	fp = filp_open("/data/ima/error_log",O_WRONLY | O_APPEND| O_CREAT,0666);
	if(IS_ERR(fp))
	{
		mutex_unlock(&error_log_mutex);
		printk("create file error\n");
		return ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;

	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec,&tm);
	sprintf(time_temp,"UTC-Time:%d-%02d-%02d %02d:%02d:%02d",tm.tm_year+1900,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
	
	vfs_write(fp, time_temp, strlen(time_temp), &pos);
	vfs_write(fp, "   ", 3, &pos);
	vfs_write(fp, hash_val, strlen(hash_val), &pos);
	vfs_write(fp, "   ", 3, &pos);	
	vfs_write(fp, hash_name, strlen(hash_name), &pos);
	vfs_write(fp, "\n", 1, &pos);	

	filp_close(fp,NULL);
	set_fs(fs);
	printk("%s  %s  %s\n",time_temp, hash_val, hash_name);
	mutex_unlock(&error_log_mutex);
}

void set_state_OpenCtrl(void)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;

	fp = filp_open("/data/ima/imm_state",O_WRONLY | O_CREAT,0644);
	if(IS_ERR(fp))
	{
		printk("create file error\n");
		return ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	
	vfs_write(fp, "1", 1, &pos);	
	vfs_write(fp, "\n", 1, &pos);	

	filp_close(fp,NULL);
	set_fs(fs);	
}

void set_state_CloseCtrl(void)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	
	//printk("Hello Enter!\n");
	fp = filp_open("/data/ima/imm_state",O_WRONLY | O_CREAT,0644);
	if(IS_ERR(fp))
	{
		printk("create file error\n");
		return ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	
	vfs_write(fp, "0", 1, &pos);	
	vfs_write(fp, "\n", 1, &pos);	

	filp_close(fp,NULL);
	set_fs(fs);	
}

void set_wl_state(atomic_t* flag)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	
	//printk("Hello Enter!\n");
	fp = filp_open("/data/ima/imm_wl_exist",O_WRONLY | O_CREAT,0644);
	if(IS_ERR(fp))
	{
		printk("create file error\n");
		return ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;
	
	if(atomic_read(flag) == 0 )
		vfs_write(fp, "0", 1, &pos);
	else
		vfs_write(fp, "1", 1, &pos);
	
	vfs_write(fp, "\n", 1, &pos);	

	filp_close(fp,NULL);
	set_fs(fs);	
}

void print_kernel_whitelist_tofile(WLNode *head)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;
	WLNode *p;
	char HexHashStr[HASH_STRING_SIZE+1];
	
	//printk("Hello Enter!\n");
	fp = filp_open("/data/ima/imm_wl_content",O_WRONLY | O_CREAT | O_TRUNC,0644);
	if(IS_ERR(fp))
	{
		printk("create file error\n");
		return ;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0;

	if(head==NULL || head->next==NULL)
	{
		//printk(KERN_INFO "链表不存在！\n");
		vfs_write(fp, "#", 1, &pos);
		return;
	}

	p = head->next;
	while(p!=NULL)
	{
		DecToHex(p->value, HexHashStr);		
		//printk(KERN_INFO "%s  %s\n", HexHashStr, p->name);
		vfs_write(fp, HexHashStr, HASH_STRING_SIZE, &pos);
		vfs_write(fp, " ", 1, &pos);
		vfs_write(fp, p->name, strlen(p->name), &pos);
		//vfs_write(fp, "\n", 1, &pos);		
		p = p->next;
	}

	filp_close(fp,NULL);
	set_fs(fs);	
}

