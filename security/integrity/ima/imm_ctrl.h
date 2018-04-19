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
#ifndef _LINUX_IMM_CTRL_H
#define _LINUX_IMM_CTRL_H

#include <linux/fs.h>
#include <linux/elf.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define HASH_SIZE 32
#define HASH_STRING_SIZE 64

// the white list node (item) structure for building linked list
typedef struct WLNode
{
	char value[HASH_SIZE];
	char name[256];
	struct WLNode *next;
}WLNode;



extern WLNode *WLhead, *WLend;

/*
 * processes control signal:
 * 0: deactivate processes control
 * 1: activate processes control
 */ 

extern atomic_t wl_flag;

/*
 * white list exist flag:
 * 0: kernel has no white list 
 * 1: kernel has white list
 */ 

extern atomic_t wl_exist_flag;


//
// Function Declaration
//
// transform decimal SHA1 value array to hexadecimal string
void DecToHex(char *decHash, char *hexHashStr);

// transform hexadecimal SHA1 string to decimal array
void HecToDec(char *hexHashStr, char *decHash);

// create a blank white list (linked list)
WLNode *create_WhiteList(void);

// insert a measurement item into the white list
WLNode *insert_WhiteList(WLNode *head, WLNode *end, char* hash_value, char *dir_name);

// compare the measurement hash value with the whole white list (if has existed, returen 0)
int compare_WhiteList(WLNode *head, char *target_hash);

// delete the whole white list
void delete_WhiteList(WLNode *head);

//Judge whether the file is executable
int IS_ELF_exe(struct file *fp);
int is_executable(struct file *fp);

void error_log(char *hash_val, char *hash_name);
void set_state_OpenCtrl(void);
void set_state_CloseCtrl(void);
void set_wl_state(atomic_t* flag);
void print_kernel_whitelist_tofile(WLNode *head);

#endif
