#include "message_slot.h"

#include <fcntl.h>     
#include <unistd.h>  
#include <string.h>  
#include <sys/ioctl.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


//================== ASSISTING FUNCTION DECLARATION ===========================
void error(char *, int );

void error_with_string(char *, int ,char * );


//================== END OF ASSISTING FUNCTION DECLARATION ===========================

//--------------------------ERROR HANDLING-------------------------------------

void error(char *error, int exit_code){

if(strcmp(error,"arguments")==0){
	fprintf(stderr, "error in number of arguments, Error: %s\n", strerror(EINVAL));
		exit(exit_code);

}


if(strcmp(error,"ioctl")==0){
fprintf(stderr, "ioctl failed Error: %s\n", strerror(errno));
		exit(exit_code);
}
if(strcmp(error,"write")==0){
fprintf(stderr, "write failed Error: %s\n", strerror(errno));
		exit(exit_code);
}


}


void error_with_string(char *error, int exit_code,char * str){
if(strcmp(error,"open")==0){
	fprintf(stderr, "failed to open file descriptor:%s\n Error: %s\n",str, strerror(errno));
		exit(exit_code);
}
}
//--------------------------END OF ERROR HANDLING-------------------------------------


//---------------------------OPEN------------------------------------

int open_file(int fd, char * file){
     fd = open(file, O_RDWR);
	if (fd < 0) {
	error_with_string("open",FAIL,file);
	}

return fd;
}
//---------------------------END OF OPEN------------------------------------


//---------------------------IOCTL-------------------------------------

int ioctl_file(int returned_signal,int fd,unsigned long channel_info,char *message,int message_length){

returned_signal = ioctl(fd, MSG_SLOT_MESSAGE_FILE, channel_info);
	
	if (returned_signal != 0) {
		error("ioctl",FAIL);
	}
	
	
	returned_signal = write(fd, message, message_length);
	if (returned_signal != message_length) {
			error("write",FAIL);
	}

return returned_signal;

}

//---------------------------IOCTL-------------------------------------


//---------------------------MAIN-------------------------------------

// main function for message_sender

int main(int argc, char** argv)
{  

	int fd=-1;
        int returned_signal=0;
	char *message = argv[3];
	int message_length = strlen(message);
	unsigned long channel_info = atoi(argv[2]);
        char * file = argv[1];
       
	
	if (argc != 4) {
	error("arguments",FAIL);
	}


fd = open_file(fd,file);
returned_signal = ioctl_file(returned_signal,fd, channel_info,message, message_length);
	close(fd);
 

	exit(SUCCESS);
}

//---------------------------END OF MAIN-------------------------------------
