//Project 2: Sarat Kavuru, skavuru; Rachit Thirani, rthiran;
//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//
//   Description:
//     Skeleton of NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "tnpheap_ioctl.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/time.h>

DEFINE_MUTEX(commit_lock);
DEFINE_MUTEX(transaction_lock);
DEFINE_MUTEX(population_lock); 
static __u64 transaction_number =0;
struct list_tnpheap {

    __u64 version_number;
    __u64 offset;
    struct list_tnpheap *next;

};
struct list_tnpheap *trans_head=NULL;
struct miscdevice tnpheap_dev;

__u64 tnpheap_get_version(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
   // printk(KERN_CONT "Inside get version-kernel\n");
    if (!copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        struct list_tnpheap *temp = trans_head;
     //   printk(KERN_CONT "Inside get version-kernel copy from copy_from_user\n");
        while(temp!=NULL)
        {
            // if found, return the version number.
            if(temp->offset == (cmd.offset/PAGE_SIZE)){
       //         printk(KERN_CONT "Found  version_number %lu of offset %lu\n",temp->version_number,temp->offset);
                return temp->version_number;}
             temp=temp->next;
        }

        //if not found, create a new list entry with version number 0
        
        //printk(KERN_CONT "Haven't found it , so create new\n");
        temp = trans_head;
        //printk(KERN_CONT "Problem with temp?\n");
        mutex_lock(&population_lock);
        struct list_tnpheap *new_node=kzalloc(sizeof(struct list_tnpheap),GFP_KERNEL);
        //printk(KERN_CONT "Is this the problem %lu\n",cmd.offset/PAGE_SIZE);
        new_node->offset = (cmd.offset/PAGE_SIZE);
        new_node->version_number = 0;
        //printk(KERN_CONT "Offset- %lu , Version number- %lu\n",new_node->offset,new_node->version_number);
        //printk(KERN_CONT "Creating a new_node in get version-kernel\n");
        if(temp==NULL)
            {
                trans_head=new_node;
               // printk(KERN_CONT "kernel head was NULL\n");
                //return new_node->version_number;
            }

        else
        {
            //printk(KERN_CONT "Kernel head was not NULL\n");
            while(temp->next!=NULL)
                temp=temp->next;

            temp->next=new_node;
            //return new_node->version_number;
        } 
        
        mutex_unlock(&population_lock);
        return new_node->version_number;
        
    }    
    return -1;
}


__u64 tnpheap_start_tx(struct tnpheap_cmd __user *user_cmd)
{
  //printk(KERN_CONT "Inside start_tx-kernel\n");
    mutex_lock(&transaction_lock);
    transaction_number = transaction_number + 1 ;
    mutex_unlock(&transaction_lock);
    return transaction_number;
}

__u64 tnpheap_commit(struct tnpheap_cmd __user *user_cmd)
{
    
   // printk(KERN_CONT "Inside commit-kernel\n");
    struct tnpheap_cmd cmd;
    __u64 ret=0;
    if (!copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
       //printk(KERN_CONT "Inside copy_from_user of commit-kernel\n");
        //if(!copy_from_user(npheap_alloc(npdevfd,cmd->offset,cmd->size),cmd->data,cmd->size)){
        //  if (1) { 
      //__u64 aligned_size= ((cmd->size + getpagesize() - 1) / getpagesize())*getpagesize(); 
      //if(!copy_from_user(mmap(0,aligned_size,PROT_READ|PROT_WRITE,MAP_SHARED,tnpheap_dev,cmd->offset*getpagesize()),cmd->data,cmd->size)){ 
        struct list_tnpheap *temp = trans_head;

        while(temp!=NULL)
        {
            // if found, update the version number.
            
            if(temp->offset == (cmd.offset/PAGE_SIZE)){
         //      printk(KERN_CONT "Kernel while offset %ld and kversion %ld and version %ld\n",temp->offset,temp->version_number,cmd.version);
                if(temp->version_number == cmd.version){
                mutex_lock(&commit_lock);
                temp->version_number = temp->version_number + 1;
           //     printk(KERN_CONT "Updated version in commit-kernel\n");
                mutex_unlock(&commit_lock);
                return 1;
            }
            }
                 
             temp=temp->next;
        }

        }
    //}
    return ret;
}



__u64 tnpheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case TNPHEAP_IOCTL_START_TX:
        return tnpheap_start_tx((void __user *) arg);
    case TNPHEAP_IOCTL_GET_VERSION:
        return tnpheap_get_version((void __user *) arg);
    case TNPHEAP_IOCTL_COMMIT:
        return tnpheap_commit((void __user *) arg);
    default:
        return -ENOTTY;
    }
}

static const struct file_operations tnpheap_fops = {
    .owner                = THIS_MODULE,
    .unlocked_ioctl       = tnpheap_ioctl,
};

struct miscdevice tnpheap_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tnpheap",
    .fops = &tnpheap_fops,
};

static int __init tnpheap_module_init(void)
{
    int ret = 0;
    if ((ret = misc_register(&tnpheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else
        printk(KERN_ERR "\"npheap\" misc device installed\n");
    return 1;
}

static void __exit tnpheap_module_exit(void)
{
    misc_deregister(&tnpheap_dev);
    return;
}

MODULE_AUTHOR("Hung-Wei Tseng <htseng3@ncsu.edu>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(tnpheap_module_init);
module_exit(tnpheap_module_exit);
