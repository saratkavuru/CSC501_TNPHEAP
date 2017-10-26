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
int node_count=0;
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


void free_list(struct list_tnpheap_TM *head){
	struct list_tnpheap_TM *temp = head;
	struct list_tnpheap_TM *next;

	while(temp!=NULL){
		next = temp->next;
		//free(temp->local_buffer);
		free(temp);
		temp = next;
	}
head = NULL;
}

int tnpheap_handler(int sig, siginfo_t *si)
{
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    
    struct list_tnpheap_TM *temp = head;
    fprintf(stderr, "Just entered tnpheap_alloc-%d\n",getpid());
    void *ta = npheap_alloc(npheap_dev,offset,8192);
    __u64 kernel_version = -1;
    if(ta == -1){
    	fprintf(stderr, "npheap_alloc returned -1 for %d\n",getpid());
    }
    else{ 
    	fprintf(stderr, "Allocated npheap offset %lu for %d\n",offset,getpid());
    	struct tnpheap_cmd cmd;
    	cmd.offset = offset*getpagesize();
    	kernel_version=ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
    	if(kernel_version == -1){
    		fprintf(stderr, "kernel_version is -1 for %d\n",getpid());
    	}
    
    	else{
    		temp = head;
    		while(temp!=NULL)
    		{
    			if(temp->offset==offset){
    				temp->version_number == kernel_version;
    				return temp->local_buffer;
    			}
    			temp=temp->next;
 
    		}

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
    	//fprintf(stderr, "Made a new node-%d\n",getpid());
    	//populate the list
    	//new_node->transaction_number = current_tx;
    	new_node->offset = offset;
    	//fprintf(stderr, "Is the problem here offset- %lu vs cmd.offset-%lu of process %d\n",new_node->offset,cmd.offset,getpid());
    	new_node->version_number = kernel_version;
    	new_node->size = size;
    	//fprintf(stderr, "Or Is the problem here-%d\n",getpid());
     	new_node->local_buffer = calloc(size,sizeof(char));
     	//new_node->local_buffer = NULL;
     	fprintf(stderr, "Populated the new node-%d with transaction_number %lu\n",getpid(),current_tx);
    	temp = head;
    	if(temp==NULL){
    		head = new_node;
    		node_count ++; //fprintf(stderr, "head was null 81\n");
    	}
    	else{
    		while(temp->next != NULL){
    			temp=temp->next;
    		//	fprintf(stderr, "head was not null 86\n");
    		}
    		node_count++;
    		temp->next=new_node;
    	}

       fprintf(stderr, "Returning pointer of local_buffer for transaction-%lu in -%d\n",current_tx,getpid());
    	return new_node->local_buffer;     
    }
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
	__u64 kernel_version = -1;
	int permission = 0;
	void *ta;
	fprintf(stderr, "Just inside commit for transaction %lu\n",current_tx);
	// Search this list_npheap_TM using transaction number as index
	struct tnpheap_cmd cmd;
	struct list_tnpheap_TM *temp = head;
	if(temp == NULL){
    fprintf(stderr, "HEAD == NULL for transaction %lu\n",current_tx);
		return 1;
	}
	
	//int flag=1;
        while(temp!=NULL)
        {	
            	fprintf(stderr, "Inside the first while loop %lu\n",current_tx);
            	cmd.version = temp->version_number;
            	cmd.offset = temp->offset*getpagesize();
            	//cmd.data = temp->local_buffer;
            	//cmd.size = temp->size;
            	fprintf(stderr, "Start comparing verisons%lu\n",current_tx);
            	if(temp->local_buffer != 0){
            		fprintf(stderr, "Set dirty bit for %lu and version %lu in %lu\n",temp->offset,temp->version_number,current_tx);
            		temp->dirty_bit = 1;
            		permission=ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd);
            		if(permission){            		            	
          fprintf(stderr, "Committed transaction-%lu and offset %lu with version number %lu in -%d\n",current_tx,temp->offset,temp->version_number,getpid());
        	ta = npheap_alloc(npheap_dev,temp->offset,8192);
        	memcpy(ta,temp->local_buffer,temp->size);
        	fprintf(stderr, "Copied data to npheap at offset %lu for transaction %lu in -%d\n",temp->offset,current_tx,getpid());
        	permission =0;
        }
        else{
        	free_list(head);
        	return 1;
        }
   	}

     temp=temp->next;
     }
        	
        free_list(head);
        fprintf(stderr, "Transaction successful-%lu in -%d\n",current_tx,getpid());
        return 0;
 }
//fprintf(stderr, "Set all dirty bits in transaction %lu\n",current_tx);
        	//We have to abort even if read only offset versions are changed
            	
            	/*kernel_version = ioctl(tnpheap_dev,TNPHEAP_IOCTL_GET_VERSION,&cmd);
            	if(kernel_version == -1){
            		fprintf(stderr, "Kernel version is -1 in commit\n");
            	}
        	if(temp->version_number!= kernel_version)
        	{
        		//flag=0;
        		fprintf(stderr, "Conflict detected\n");
        		free_list(head);
        		fprintf(stderr, "Transaction failure-%lu in -%d\n",current_tx,getpid());
        		return 1;
        	}	
        	fprintf(stderr,"NodeCount = %d",node_count);
        	//fprintf(stderr, "Temp = Temp->next with local=%ld and kernel=%ld for offset %ld\n",temp->version_number,kernel_version,temp->offset);
        	temp=temp->next;
        }*/
       /* temp = head;
        while(temp!=NULL)
        {	

            	fprintf(stderr, "Please be here\n");
            	//cmd.version = temp->version_number;
            	cmd.offset = temp->offset*getpagesize();
            	fprintf(stderr, "Or here\n");
            	//cmd.data = temp->local_buffer;
            	//cmd.size = temp->size;

        	if(temp->dirty_bit){
        		fprintf(stderr, "Dear lord\n");
        		fprintf(stderr, "Temp size %lu vs Npheap size %ld \n",temp->size,npheap_getsize(npheap_dev,temp->offset));
        	ta = npheap_alloc(npheap_dev,temp->offset,8192);
        	fprintf(stderr, "After - Temp size %lu vs Npheap size %ld \n",temp->size,npheap_getsize(npheap_dev,temp->offset));
        	mutex_lock(&tnpheap_lock);
        	fprintf(stderr, "Locked tx-%ld and process %d\n",current_tx,getpid());
        	memcpy(ta,temp->local_buffer,temp->size);
        	fprintf(stderr, "Copied data to npheap at offset %lu for transaction %lu in -%d\n",temp->offset,current_tx,getpid());
        	if(ioctl(tnpheap_dev,TNPHEAP_IOCTL_COMMIT,&cmd)){            		            	
          fprintf(stderr, "Committed transaction-%lu and offset %lu with version number %lu in -%d\n",current_tx,temp->offset,temp->version_number,getpid());
        	}
        }
    temp =temp->next;
    }
         npheap_unlock(npheap_dev,);
         fprintf(stderr, "Unlocked tx-%ld and process %d\n",current_tx,getpid());
         free_list(head);
         fprintf(stderr, "Transaction successful-%lu in -%d\n",current_tx,getpid());
         return 0;
      }

        
            // if found, return the version number.
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

