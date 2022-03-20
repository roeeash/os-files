#include "message_slot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>      
#include <unistd.h>     
#include <sys/ioctl.h>  



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

if(strcmp(error,"read")==0){
fprintf(stderr, "read failed Error: %s\n", strerror(errno));
		exit(exit_code);
}
}



void error_with_string(char *error, int exit_code,char * str){
if(strcmp(error,"open")==0){
	fprintf(stderr, "failed to open file descriptor:%s\n Error: %s\n",str, strerror(errno));
		exit(exit_code);
}
}

//--------------------------ERROR HANDLING-------------------------------------


//---------------------------OPEN------------------------------------
int open_file(int fd, char * file){
     fd = open(file, O_RDWR);
	if (fd < 0) {
	error_with_string("open",FAIL,file);
	}

return fd;
}
//---------------------------END OF OPEN------------------------------------

//---------------------------IOCTL------------------------------------
int ioctl_file(int returned_signal,int fd,unsigned long channel_info){

returned_signal = ioctl(fd, MSG_SLOT_MESSAGE_FILE, channel_info);
	
	if (returned_signal != 0) {
		error("ioctl",FAIL);
	}
	


return returned_signal;

}
//---------------------------END OF IOCTL------------------------------------

//---------------------------READ------------------------------------
int read_file(int fd,int returned_signal, char * buf){
	returned_signal = read(fd, buf, BUFFER_LENGTH);
	
	if (returned_signal <= 0) {
			error("read",FAIL);
	}
	//attempting to read from file descriptor
	if (write(1, buf,returned_signal) != returned_signal) {
			error("write",FAIL);
	}

return returned_signal;
}

//---------------------------READ------------------------------------

//---------------------------INPUT CORRECTNESS------------------------------------
void test_input_correctness(int argc){
if (argc != 3) {
	error("arguments",FAIL);
	}
}
//---------------------------END OF INPUT CORRECTNESS------------------------------------

// main function for message_reader

int main(int argc, char** argv)
{
    int fd=-1;

    unsigned long channel_info = atoi(argv[2]); // 0 for invaild arg, ioctl will handle
    char buf[BUFFER_LENGTH];
char * file = argv[1];
int returned_signal=0;
	
	test_input_correctness(argc);

       fd = open_file(fd,file);

	
	returned_signal = ioctl_file(returned_signal,fd, channel_info);
	
	//attempting to read from file descriptor
	// creating buffer
	returned_signal = read_file(fd,returned_signal,buf);
	
	


	close(fd);
	
	//success
	

	exit(SUCCESS);
	
}