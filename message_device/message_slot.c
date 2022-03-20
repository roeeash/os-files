#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"


#include <linux/fs.h>      
#include <linux/uaccess.h> 
#include <linux/string.h>  
#include <linux/slab.h>
#include <linux/kernel.h>   
#include <linux/module.h>   

//~~~~~~~~~~~~~~~~struct message_file_list  ~~~~~~~~~~~~~~~~~~~~~~~~

typedef struct message_file_list {
	struct message_file *head;
} message_file_list;

//~~~~~~~~~~~~~~~~~~~~~end of struct message_file_list ~~~~~~~~~~~~~~~~~~~~~~~~~~~

MODULE_AUTHOR("Roee Ashkenazi");
MODULE_LICENSE("GPL");

message_file_list devices_info[257];

//~~~~~~~~~~~~~~~~~~~~~~~~ASSISTING FUNCTION DECLARATION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int error_symbol(char *);

static ssize_t error(char *);

static ssize_t error_with_file(char *,struct file*);

static ssize_t errors_for_write(const char __user*,info *,size_t);

struct info * assign_message_file(info *,unsigned long,message_file *);

void init_message_data(struct message_file* , unsigned long);


//~~~~~~~~~~~~~~~~~~~~~~~~ END OF ASSISTING FUNCTION DECLARATION ~~~~~~~~~~~~~~~~~~~~~~~~


//--------------------------ERROR HANDLING-------------------------------------
int error_symbol(char *err){
if(strcmp(err,"memory")==0){
printk("error : not enough memory");
			return -ENOMEM;
}

if(strcmp(err,"block")==0){
printk("error : descriptor was placed in non-blocking mode");
			return -EWOULDBLOCK;
}
if(strcmp(err,"space")==0){
printk("error : not enough space");
			return -EWOULDBLOCK;
}
if(strcmp(err,"IO")==0){
printk("error : IO problem");
			return -EIO;
}

if(strcmp(err,"size")==0){
printk("error : message size too small");
			return -EMSGSIZE;
}

if(strcmp(err,"value")==0){
printk("error : illegal value");
			return -EINVAL;
}

return 0;
}


static ssize_t error(char * err){
if(strcmp(err,"ioctl")==0){
printk("new message_file malloc failed in ioctl");
return error_symbol("memory");

}
return 0;

}

static ssize_t error_with_file(char * err,struct file*  fd){
if(strcmp(err,"open")==0){
printk("malloc failed for device_open(%p)\n", fd);
	
		return error_symbol("memory");
}
return 0;
}



//--------------------------END OF ERROR HANDLING-------------------------------------

//--------------------------READ-------------------------------------

static ssize_t read_dev(struct file* fd, char __user* message_buffer, size_t message_length, loff_t* offset) {
	

	char *next_message;
	int message_size;
        int failed = 0;
        int i;
	
	struct info *info = (struct info*)(fd->private_data);
	printk("performing device_read(%p,%ld)\n",fd, message_length);
	
        //error handling
        //no message file has been set on the file descriptor

	if ( message_buffer == NULL  || info->message_file_id == 0 ) {
		return error_symbol("value");

	}
	
	message_size = info->current_message_file->message_length;
	next_message = info->current_message_file->message;
        
	
        //no message exists on the message_file

	if (message_size==0) {
		return error_symbol("block");
	}

        //the provided buffer length is too small
        // to hold the last message written on the message_file

	if (message_size > message_length) {
		return error_symbol("space");
	}
          i = 0;
	
	while ( i < message_size) {
		
		//attempting to put the message into the user's buffer
		if (put_user(next_message[i], &message_buffer[i]) != 0) {
			failed = 1;
		}

             ++i;
	}
	// if we failed to allocate the massage into the user's buffer
	if (failed) {
		return error_symbol("IO");
	}
	// return the number of input characters used
	return i;
}

//-----------------------------END OF READ----------------------------------

//--------------------------ERROR HANDLING FOR WRITE-------------------------------------
static ssize_t errors_for_write(const char __user* message_buffer,struct info *info,size_t message_length){
       if (message_buffer == NULL || info->message_file_id == 0  ) {
		return error_symbol("value");

	}
 
                  
	//the passed message length is 0 or more than 128

        if (message_length == 0 || message_length > BUFFER_LENGTH) {
		return error_symbol("size");
	}
return 0;
}
//--------------------------END OF ERROR HANDLING FOR WRITE-------------------------------------

//--------------------------WRITE-------------------------------------

static ssize_t write_dev(struct file* fd, const char __user* message_buffer, size_t message_length, loff_t* offset) {
	
	
        int failed = 0;
	char kernel_buffer[BUFFER_LENGTH];
        int i;
ssize_t err;
   
	
	struct info *info = (struct info*)(fd->private_data);
	printk("performing device_write(%p,%ld)\n",fd, message_length);
	
        //error handling
  
	//no message_file has been set on the file descriptor
 err= errors_for_write(message_buffer,info,message_length);

if(err!=0){
return err;
}

  i = 0;
	
	while ( i < message_length) {
		
		
		if (get_user(kernel_buffer[i], &message_buffer[i]) != 0) {
			failed = 1;
		}

            ++i;
	}
	// if we failed to allocate the massage from the kernel's buffer
	if (failed) {
		return error_symbol("IO");
	}
	
	// upon success, copy the message into the buffer

	info->current_message_file->message_length = message_length;

i = 0; 

	while (i < message_length && i < BUFFER_LENGTH) {
		info->current_message_file->message[i] = kernel_buffer[i];

               ++i;
	}
	
	// return the number of input characters used
	return i;
}
//-----------------------------END OF WRITE----------------------------------

//---------------------------OPEN------------------------------------

static int open_dev(struct inode* inode_number, struct file*  fd) {

    int exit_code;
   	int minor_number; 	
	struct info *info;
ssize_t err;


	info = kmalloc(sizeof(struct info), GFP_KERNEL);
minor_number = iminor(inode_number);
        
	
	printk("performing device_open (%p) minor num is: %d\n", fd,minor_number);
	
	if (info==NULL) {
		  err = error_with_file("open",fd);
                  if(err!=0){
                   return err;
}
	}
	
	// save minor number
	info->minor = minor_number;
	info->message_file_id = 0;
	fd->private_data = (void*)info;

	exit_code = SUCCESS;
	return exit_code;
}
//-----------------------------END OF OPEN----------------------------------



struct info * assign_message_file(struct info  * info,unsigned long ioctl_long,message_file *temp){
	info ->message_file_id = ioctl_long;
	info ->current_message_file = temp;
         return info;
}


int ioctl_match_found(message_file *temp,struct info  *info , unsigned long ioctl_long){
if (temp->message_file_id== ioctl_long) {
				info  = assign_message_file(info ,ioctl_long,temp);
				return SUCCESS;
			}
return FAIL;
}

void init_message_data(struct message_file* message, unsigned long ioctl_long){


message->message_file_id = ioctl_long;
		message->message_length = 0;
		message->next = NULL;

}

//---------------------------IOCTL-------------------------------------

static long ioctl_dev(struct file* fd, unsigned int ioctl_id, unsigned long ioctl_long) {
	struct info  *info = (struct info*)(fd->private_data);
	message_file *head = devices_info[info ->minor].head;
	message_file *temp;
ssize_t err;

	
	printk("performing ioctl: setting message_file id to %ld\n", ioctl_long);

        // error handling
        //the passed command is not MSG_SLOT_MESSAGE_FILE

	if(ioctl_long == 0 ||ioctl_id != MSG_SLOT_MESSAGE_FILE){
		return error_symbol("value");

	}
	
	if (head != NULL)  {
		temp = head;
		while (temp->next != NULL) {
			if(ioctl_match_found(temp,info,ioctl_long)==SUCCESS)
{
return SUCCESS;
}
			temp = temp->next;
		}
		if(ioctl_match_found(temp,info,ioctl_long)==SUCCESS)
{
return SUCCESS;
}
		temp->next = kmalloc(sizeof(message_file), GFP_KERNEL);
		if (head==NULL) {
                        // error handling - head is null (the passed id is 0)
err = error("ioctl");
if(err!=0){
                   return err;
}

			
		}
		init_message_data(temp->next,ioctl_long);
		info = assign_message_file(info,ioctl_long,temp->next);
		return SUCCESS;
	}
	
	
	
	else{
		
		head = kmalloc(sizeof(message_file), GFP_KERNEL);
		if (head==NULL) {
			err = error("ioctl");
if(err!=0){
                   return err;
}
		}
		init_message_data(head,ioctl_long);
		info->message_file_id = ioctl_long;
		info->current_message_file = head;
		devices_info[info->minor].head=head;
		return SUCCESS;
	}
	
}

//-----------------------------END OF IOCTL----------------------------------





//------------------------------RELEASE---------------------------------
static int release_dev(struct inode* inode_number, struct file*  fd) {
	
	int exit_code;
	printk("performing device_release(%p,%p)\n", inode_number, fd);
	kfree(fd->private_data);
	exit_code = SUCCESS;
	return exit_code;
}

//-----------------------------END OF RELEASE----------------------------------


//-----------------------------DEV_OPS----------------------------------

struct file_operations devOps =
 {
	.owner = THIS_MODULE,
	.read = read_dev,
	.write = write_dev,
	.open = open_dev,
	.unlocked_ioctl = ioctl_dev,
	.release = release_dev,
 };

//----------------------------END OF DEV_OPS-----------------------------------


//-----------------------------INIT----------------------------------

static int __init initiallize(void)
{
	int exit_code;
        int i;
	// attempt to register new character device
	int rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &devOps);
	
	// failed to register
	if (rc < 0)
	{
		printk(KERN_ERR "%s registraion failed for  %d\n",DEVICE_RANGE_NAME, MAJOR_NUM);
		return rc;
	}
	
	// initiate list for every minor number possible
	i=0;
	while(i<257) {
		devices_info[i].head = NULL;
                i++;
	}
	printk("successful registraion");
	exit_code = SUCCESS;
	return exit_code;
}

//----------------------------END OF INIT-----------------------------------

//-----------------------------CELANUP----------------------------------

static void __exit cleanup(void)
{
	// Unregister the device

	message_file *head;
	message_file *temp;
int i;

i =0;

	// free every message_file and every list created
	while(i<257){
		head=devices_info[i].head;
		//if the chain link isn't null, delete it
		while (head != NULL)
		{
			temp = head;
			head = head->next;
			kfree(temp);
		}


      i++;

	}
	//unregister the character device
	unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//----------------------------END OF CLEANUP-----------------------------------

module_init(initiallize);
module_exit(cleanup);



//========================= END OF FILE =========================