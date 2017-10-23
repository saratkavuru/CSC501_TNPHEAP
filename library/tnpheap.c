#include <npheap/tnpheap_ioctl.h>
#include <npheap/npheap.h>
#include <npheap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>

struct list_tnpheap_TM {
	
    __u64 version_number;
    __u64 offset;
    __u64 transaction_number;
    __u64 size;
    void *local_buffer;
    list_tnpheap_TM *next;

}
struct list_tnpheap_TM *head=NULL;
__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
      // Search this list_npheap_TM using offset as index
	struct list_tnpheap_TM *temp = TM_head;
        while(temp!=NULL)
        {
            // if found, return the version number.
            if(temp->offest == offset)
                return temp->version_number;
             temp=temp->next;
        }

        return -1 ;
}



int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    
    if(npheap_alloc(npheap_dev,offset,size))
    { 
    	struct tnpheap_cmd cmd;
    	cmd->offset = offset*getpagesize();
    	//populate the transaction map on successful alloc
    	struct list_tnpheap_TM *new_node = malloc(sizeof(struct list_tnpheap_TM));
    	//initialise to negative values.
    	new_node->version_number = -1;	
    	new_node->transaction_number = -1;
    	new_node->offset = -1;
    	new->node->size = 0;
    	new_node->local_buffer = NULL;
    	new_node->next =NULL;
    	//populate the list
    	new_node->transaction_number = current_tx;
    	new_node->offset = offset;
    	new_node->version_number = tnpheap_ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
    	new_node->size = size;
     	new_node->local_buffer = malloc(size);
    	struct list_tnpheap_TM *temp = head;
    	if(temp==NULL){
    		temp = new_node;
    	}
    	else{
    		while(temp->next != NULL){
    			temp=temp->next;
    		}
    		temp->next=new_node;
    	}

    	return new_node->local_buffer;     
    }
    return -1;
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
	struct tnpheap_cmd cmd;
    return tnpheap_ioctl(tnpheap_dev,TNPHEAP_IOCTL_START_TX,&cmd);
}

int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
	__u64 local_version_number = -1;
	// Search this list_npheap_TM using transaction number as index
	struct list_tnpheap_TM *temp = TM_head;
        while(temp!=NULL)
        {
            // if found, return the version number.
            if(temp->transaction_number == current_tx){
            	struct tnpheap_cmd cmd;
            	cmd->version_number = temp->version_number;
            	cmd->offset = temp->offset;
            	cmd->data = temp->local_buffer;
            	cmd->size = temp->size;
            	if( temp->version_number == tnpheap_ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd))
                	if(tnpheap_ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd))
                		return 0;
            }


                
             temp=temp->next;
        }

    return 1;
}

