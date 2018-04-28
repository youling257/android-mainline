/*
 * Copyright (C) 2017 TCA
 *
 *  Create whitelist proc file: wl_proc, switch_proc
 *
 *
 * File: imm_data_comm.h
 *              Import white list to kernel and set processes control signal through proc.
 *              Totally 2 sets of proc file operation.          
 */


#include <linux/proc_fs.h>
//#include <asm/uaccess.h>
#include <linux/sched.h>
#include "imm_data_comm.h"
#include "imm_ctrl.h"



struct proc_dir_entry *proc_dir;
struct proc_dir_entry *proc_file;
struct proc_dir_entry *proc_signal;

//
// Function Implementation
//

static ssize_t proc_file_write(struct file *file, const char __user *buffer, size_t count, loff_t *data){
  char procfs_buffer[PROCFS_BUF_MAX_SIZE];
  char *hash_value;
  char *dir_name;
  WLNode *temp;   
  memset(procfs_buffer, 0, PROCFS_BUF_MAX_SIZE);
  if(count>PROCFS_BUF_MAX_SIZE)
    count = PROCFS_BUF_MAX_SIZE;

  if (copy_from_user(procfs_buffer, buffer, count)) {
    return -EFAULT;
  }
  
  hash_value = procfs_buffer; 
  dir_name = procfs_buffer+ HASH_STRING_SIZE + 1; 
  
  temp = insert_WhiteList(WLhead, WLend, hash_value, dir_name);
  if(temp!=NULL){
    WLend = temp;
  }
  
  return count;
}

static ssize_t proc_file_read(struct file *file, char __user *data, size_t len, loff_t *off){
  printk(KERN_INFO "WHITELIST READ NOT ALLOWED\n");
  return 0;

  /*
  static int finished = 0;
  if(finished){
    printk(KERN_INFO "WHITELIST READ END\n");
    finished = 0;
    return 0;
  }
  finished = 1;
  */
}

static ssize_t proc_signal_read(struct file *file, char __user *data, size_t len, loff_t *off){
  printk(KERN_INFO "SIGNAL READ NOT ALLOWED\n");
  return 0;
}



static ssize_t proc_signal_write(struct file *file, const char __user *buffer,
                size_t count, loff_t *data)
{
        char procfs_buffer[PROCFS_BUF_MAX_SIZE];        
        
        if(count>PROCFS_BUF_MAX_SIZE)
                count = PROCFS_BUF_MAX_SIZE;

        if (copy_from_user(procfs_buffer, buffer, count)) 
        {
                return -EFAULT;
        }
        
        if(procfs_buffer[0]=='0')
        {
                atomic_set(&wl_flag,0);
                set_state_CloseCtrl();
        }

        if(procfs_buffer[0]=='1')
        {
                atomic_set(&wl_flag,1);
                set_state_OpenCtrl();
        }

        if(procfs_buffer[0]=='2')
        {
                delete_WhiteList(WLhead);
                WLend = WLhead;
                atomic_set(&wl_exist_flag,0);
                set_wl_state(&wl_flag);
        }

        if(procfs_buffer[0]=='3')// ask kernel whether has white list
        {               
                set_wl_state(&wl_exist_flag);           
        }

        if(procfs_buffer[0]=='4')// ask kernel print white list
        {               
                print_kernel_whitelist_tofile(WLhead);          
        }

        return count;
}



static struct file_operations wl_proc_ops = {
  .owner = THIS_MODULE,
  .read = proc_file_read,
  .write = proc_file_write
};

static struct file_operations wl_mng_proc_ops = {
  .owner = THIS_MODULE,
  .read = proc_signal_read,
  .write = proc_signal_write
};


int proc_wl_init(void){

  proc_dir = proc_mkdir(my_proc_dir_name, NULL);
  if(!proc_dir){
    return -EFAULT;
  }

  proc_file = proc_create(my_proc_file_name, 0666, proc_dir, &wl_proc_ops);
  if(!proc_file){
    return -EFAULT;
  }

  proc_signal = proc_create(my_proc_signal_name, 0666, proc_dir, &wl_mng_proc_ops);
  if(!proc_signal){
    return -EFAULT;
  }

  return 0;
}

/*
//
static int proc_file_write(struct file *file, const char __user *buffer,
                unsigned long count, void *data)
{
        char procfs_buffer[PROCFS_BUF_MAX_SIZE];
        char *hash_value;
        char *dir_name;
        WLNode *temp;   
        memset(procfs_buffer, 0, PROCFS_BUF_MAX_SIZE);
        if(count>PROCFS_BUF_MAX_SIZE)
                count = PROCFS_BUF_MAX_SIZE;

        if (copy_from_user(procfs_buffer, buffer, count)) {
                return -EFAULT;
        }
        
        hash_value = procfs_buffer; 
        dir_name = procfs_buffer+ HASH_STRING_SIZE + 1; 

        temp = insert_WhiteList(WLhead, WLend, hash_value, dir_name);
        if(temp!=NULL){
                WLend = temp;
        }

        return count;
}

static int proc_signal_write(struct file *file, const char __user *buffer,
                unsigned long count, void *data)
{
        char procfs_buffer[PROCFS_BUF_MAX_SIZE];        
        
        if(count>PROCFS_BUF_MAX_SIZE)
                count = PROCFS_BUF_MAX_SIZE;

        if (copy_from_user(procfs_buffer, buffer, count)) 
        {
                return -EFAULT;
        }
        
        if(procfs_buffer[0]=='0')
        {
                atomic_set(&wl_flag,0);
                set_state_CloseCtrl();
        }

        if(procfs_buffer[0]=='1')
        {
                atomic_set(&wl_flag,0);
                set_state_OpenCtrl();
        }

        if(procfs_buffer[0]=='2')
        {
                delete_WhiteList(WLhead);
                WLend = WLhead;
                atomic_set(&wl_exist_flag,0);
                set_wl_state(&wl_flag);
        }

        if(procfs_buffer[0]=='3')// ask kernel whether has white list
        {               
                set_wl_state(&wl_exist_flag);           
        }

        if(procfs_buffer[0]=='4')// ask kernel print white list
        {               
                print_kernel_whitelist_tofile(WLhead);          
        }


        return count;
}

int create_my_proc_dir(void)
{
        int ret = 0;

        proc_dir = proc_mkdir(my_proc_dir_name, NULL);
        if (!proc_dir) 
        {
                ret = -1;
                return ret;
        }

        return 0;
}

void remove_my_proc_dir(void)
{
        remove_proc_entry(my_proc_dir_name, NULL);
}

int create_my_proc_file(void)
{
        int ret = 0;

	//proc_file = create_proc_entry(my_proc_file_name, 0666, proc_dir);
	proc_file = proc_create(my_proc_file_name, 0666, NULL, &proc_file_fops);
        if (!proc_file)
        {
                ret = -1;
                return ret;
        }
        proc_file->write_proc = proc_file_write;
        return 0;
}

void remove_my_proc_file(void)
{
        remove_proc_entry(my_proc_file_name, proc_dir);
}


int create_my_proc_signal(void)
{
        int ret = 0;

        proc_signal = create_proc_entry(my_proc_signal_name, 0666, proc_dir);
        if (!proc_signal)
        {
                ret = -1;
                return ret;
        }
       
        proc_signal->write_proc = proc_signal_write;
        return 0;
}

void remove_my_proc_signal(void)
{
         remove_proc_entry(my_proc_signal_name, proc_dir);
}
*/
