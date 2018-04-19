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
 *
 * File: imm_data_comm.h
 *              Import white list to kernel and set processes control signal through proc.
 *		Totally 2 sets of proc file operation.		
 */


#ifndef _LINUX_IMM_DATA_COMM_H
#define _LINUX_IMM_DATA_COMM_H

#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>


#define PROCFS_BUF_MAX_SIZE 512
#define my_proc_dir_name "imm_ctrl"
#define my_proc_file_name "import_whitelist"
#define my_proc_signal_name "signal"


// Function Declaration
//
// import white list function triggered by proc file writing by user program
//int proc_file_write(struct file *file, const char __user *buffer,
//                unsigned long count, void *data);

//initialize whitelist proc file
int proc_wl_init(void);

// create proc file dir
int create_my_proc_dir(void);

// remove proc file dir
void remove_my_proc_dir(void);

// create proc file for importing white list
int create_my_proc_file(void);

// remove proc file for importing white list
void remove_my_proc_file(void);

// set control signal triggered by proc file writing by user program
//int proc_signal_write(struct file *file, const char __user *buffer,
//                unsigned long count, void *data);

// create proc file of signal for setting control signal
int create_my_proc_signal(void);

// remove proc file of signal for setting control signal
void remove_my_proc_signal(void);

#endif
