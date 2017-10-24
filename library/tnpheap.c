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

__u64 current_tx;

struct list_tnpheap_TM {
	
    __u64 version_number;
    __u64 offset;
    __u64 size;
    int dirty_bit;
    void *local_buffer;
    struct list_tnpheap_TM *next;

};
struct list_tnpheap_TM *head=NULL;
__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
      // Search this list_npheap_TM using offset as index
	struct list_tnpheap_TM *temp = head;
        while(temp!=NULL)
        {
            // if found, return the version number.
            if(temp->offset == offset)
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
    fprintf(stderr, "Just entered tnpheap_alloc-%d\n",getpid());
    if(npheap_alloc(npheap_dev,offset,size))
    { 
    	fprintf(stderr, "Allocated npheap offset-%d\n",getpid());
    	struct tnpheap_cmd cmd;
    	cmd.offset = offset*getpagesize();
    	fprintf(stderr, "Allocated command offset-%d\n",getpid());
    	//populate the transaction map on successful alloc
    	struct list_tnpheap_TM *new_node = malloc(sizeof(struct list_tnpheap_TM));
    	//initialise to negative values.
    	new_node->version_number = -1;	
    	//new_node->transaction_number = -1;
    	new_node->offset = -1;
    	new_node->size = 0;
    	//new_node->local_buffer = NULL;
    	new_node->dirty_bit = 0;
    	new_node->next =NULL;
    	fprintf(stderr, "Made a new node-%d\n",getpid());
    	//populate the list
    	//new_node->transaction_number = current_tx;
    	new_node->offset = offset;
    	new_node->version_number = ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
    	new_node->size = size;
     	new_node->local_buffer = calloc(size,sizeof(char));
     	//new_node->local_buffer = NULL;
     	fprintf(stderr, "Populated the new node-%d with transaction_number %lu\n",getpid(),current_tx);
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

       fprintf(stderr, "Returning pointer of local_buffer for transaction-%lu in -%d\n",current_tx,getpid());
    	return new_node->local_buffer;     
    }
    return -1;
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
	struct tnpheap_cmd cmd;
	__u64 tx;
	tx = ioctl(tnpheap_dev,TNPHEAP_IOCTL_START_TX,&cmd);
	fprintf(stderr, "Started transaction%lu of -%d\n",tx,getpid());
    return tx;
}

int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
	//__u64 local_version_number = -1;
	// Search this list_npheap_TM using transaction number as index
	struct tnpheap_cmd cmd;
	struct list_tnpheap_TM *temp = head;
	if(temp == NULL){
    fprintf(stderr, "HEAD == NULL for transaction %lu\n",current_tx);
		return 1;
	}
	fprintf(stderr, "Just inside commit for transaction %lu\n",current_tx);
	int flag=1;
        while(temp!=NULL)
        {	
            	fprintf(stderr, "Inside the first while loop\n");
            	cmd.version = temp->version_number;
            	cmd.offset = temp->offset;
            	cmd.data = temp->local_buffer;
            	cmd.size = temp->size;
            	fprintf(stderr, "Start comparing verisons\n");
            	if(!temp->local_buffer){
            		temp->dirty_bit = 1;
            	}

        	if(temp->version_number!=ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd))
        	{
        		flag=0;
        	}
        	if(flag==0)
        	{
        		fprintf(stderr, "Conflict detected\n");
        		head==NULL;
        		return 1;
        	}
        	temp=temp->next;
        }
        temp = head;
        while(temp!=NULL)
        {	

            	cmd.version = temp->version_number;
            	cmd.offset = temp->offset;
            	cmd.data = temp->local_buffer;
            	cmd.size = temp->size;

        	if(temp->dirty_bit){
        	memcpy(npheap_alloc(npheap_dev,temp->offset,temp->size),temp->local_buffer,temp->size);
        	fprintf(stderr, "Copied data to npheap at offset %lu for transaction %lu in -%d\n",temp->offset,current_tx,getpid());
        	if(ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd)){            		            	
          fprintf(stderr, "Committed transaction-%lu in -%d\n",current_tx,getpid());
        	}
        }
    temp =temp->next;
    }
         head==NULL;
         return 0;
      }

        
            /*// if found, return the version number.
            if(temp->transaction_number == current_tx){
            	//struct tnpheap_cmd *cmd= malloc(sizeof(struct tnpheap_cmd));
            	fprintf(stderr, "Trying to commit transaction-%lu in -%d\n",current_tx,getpid());
            	struct tnpheap_cmd cmd;
            	cmd.version = temp->version_number;
            	cmd.offset = temp->offset;
            	cmd.data = temp->local_buffer;
            	cmd.size = temp->size;
            	if( temp->version_number == ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd)){
            	fprintf(stderr, "Proceed to commit transaction-%lu in -%d\n",current_tx,getpid());
            	memcpy(npheap_alloc(npheap_dev,temp->offset,temp->size),temp->local_buffer,temp->size);
            	if(ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd)){            		            	fprintf(stderr, "Trying to commit transaction-%lu in -%d\n",current_tx,getpid());
                	fprintf(stderr, "Committed transaction-%lu in -%d\n",current_tx,getpid());

                		return 0;}
            	}*/
         	
        //    }
                
             
        //}
//
  //  return 1;
//}

