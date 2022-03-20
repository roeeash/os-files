//----------FILE : PCC_SERVER.C-------//



//----------INCLUDES-------//


#include <sys/types.h>

#include <assert.h>

#include <signal.h>

#include <unistd.h>

#include <stdio.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>


//----------END OF INCLUDES-------//



//------EXIT CODE MACROS------------------//

#define FAIL 1

#define SUCCESS 0

//------END OF EXIT CODE MACROS------------------//

//----ASSISTING FUNCTIONS DECLARATION----------------//

void failed (char *);
int error(char *,int, char*, int);
void update_pcc_total(int i,  uint32_t  *);
void validate_number_of_arguments(int );
void server_shutdown();
int if_unknown_error();
void initallize_printable_chars_buffer(int ,uint32_t *);
void sigint_handler();
void sigint_init( struct sigaction);
struct sockaddr_in server_init(struct sockaddr_in,char *);
int check_for_open(int,int);
int socket_creation(int,int);
int bind_socket_to_port(int,struct sockaddr_in);
int create_listen_queue(int);
int count_chars_read(int,int,char *,uint32_t *);
int read_byte(int, int,char *, int, int);
int write_byte(int, int,char *, int, int);



//----END OF ASSISTING FUNCTIONS DECLARATION----------------//

//---ERROR HANDELLING----//



//general problems

void failed (char * message){



//socket creation

 if (strcmp(message,"socket creatrion")==0)

{

  fprintf(stderr, "problem with socket creation Error: %s\n", strerror(errno));

        exit(FAIL);



}

//set socket

 if (strcmp(message,"setsockopt")==0)

{

  fprintf(stderr, "problem with setsockopt Error: %s\n", strerror(errno));

        exit(FAIL);



}



//bind



if(strcmp(message,"bind")==0)

{

 fprintf(stderr, "problem with bind, Error: %s \n", strerror(errno));

        exit(FAIL);

}



if(strcmp(message,"listen")==0)

{

 fprintf(stderr, "problem with listen, Error: %s \n", strerror(errno));

        exit(FAIL);

}

if(strcmp(message,"accept")==0)

{

 fprintf(stderr, "problem with accepting a connection , Error: %s \n", strerror(errno));

        exit(FAIL);

}





if(strcmp(message,"sigint")==0){

fprintf(stderr, "failed to register the signal. Error: %s\n", strerror(errno));

		exit(FAIL);

}





if(strcmp(message,"read N")==0){

 fprintf(stderr, "failed to read N, Error: %s\n", strerror(errno));

                exit(FAIL);

}





if(strcmp(message,"read data")==0){

 fprintf(stderr, "failed to read data, Error: %s\n", strerror(errno));

                exit(FAIL);

}

if(strcmp(message,"write C")==0){

fprintf(stderr, "failed to write C, Error: %s\n", strerror(errno));

                exit(FAIL);

}



if(strcmp(message,"arguments")==0){

fprintf(stderr, "error in number of arguments, Error: %s\n", strerror(errno));

                exit(FAIL);

}



}



//errors with clients

int error(char * error,int connected_file_fd, char* recieve_buffer, int free_needed){



if (strcmp(error,"read")==0){

 fprintf(stderr, "TCP Error while reading data: %s\n", strerror(errno));

                if(free_needed == 1){

                free(recieve_buffer);

}

                close(connected_file_fd);

                return -1;



}



if(strcmp(error,"read N")==0){



   fprintf(stderr, "error in reading N, "

                                "accepting new connection, error: %s\n", strerror(errno));

                  if(free_needed == 1){

                free(recieve_buffer);

}

                close(connected_file_fd);

                

                  return -1;

}





if(strcmp(error,"write C")==0){

 fprintf(stderr, "TCP connection issue while the process trying to write C: %s\n", strerror(errno));

                close(connected_file_fd);

                return -1;

}



return 0;



}







//----VARIABLES----------//



//Initialize a data structure pcc_total that will count how many times each printable character

//was observed in all client connections.

//the range of readable chars. 32 upto 126 including 32 and 126 is exactly 95 chars



uint32_t  pcc_total[95] = {0};



//file descriptor of a client being processed

int connected_file_fd = -1;



//inidcator implying if sigint was sent

int sigint_ind = 0;



//----END OF VARIABLES----------//








// set the correct value for pcc total

void update_pcc_total(int i,  uint32_t  * printable_chars_buffer){

      i=0;

        while(i < 95){

            pcc_total[i] += printable_chars_buffer[i];

	++i;        

}

}


// input validation

void validate_number_of_arguments(int argc){

if (argc != 2){

        failed("arguments");

    }

}

//----SERVER SHUTDOWN----------//

//the function responsible to handle a server shutdown 
void server_shutdown(){

   //loop counter

   int i=0;

   

   //loop

    while(i < 95){

        printf("char '%c' : %u times\n", (i+32),  pcc_total[i]);
	++i;
    }

    exit(SUCCESS);

}

//----END OF SERVER SHUTDOWN----------//

// detects if an unknown error occured

int if_unknown_error(){

if(!(errno == ETIMEDOUT) &&  !(errno == ECONNRESET) && !(errno == EPIPE))

	return 1;

return 0;

}


// inits the print chars buffer

void initallize_printable_chars_buffer(int i,uint32_t * printable_chars_buffer){

i=0;

        while(i < 95){

            printable_chars_buffer[i] = 0;

		++i;

        }

}

//----SIGINT_HANDLER----------//


// handler for sigint

void sigint_handler(){

    // if the server is not proccessing any client, shutdown the server

    if(connected_file_fd  < 0){

        server_shutdown();

    }

	

	//otherwise, we're still processing a user so sigint_ind remains 1

	else{ 

        sigint_ind = 1; 

    }

}

//----END OF SIGINT_HANDLER----------//

// init sigaction sigint
 
void sigint_init( struct sigaction sigint){


	sigint.sa_handler = &sigint_handler;

	sigemptyset(&sigint.sa_mask);

	sigint.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sigint, 0) != 0) {

		failed("arguments");

	}
}

// init server

struct sockaddr_in server_init(struct sockaddr_in server_address,char * address){

   memset(&server_address, 0, sizeof(server_address));

    //AF_INET is an address family that is used to designate the type of addresses that your socket can communicate with

    server_address.sin_family = AF_INET;

    // The argument INADDR_ANY locates any local machine address and sets it as the s_addr

    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 

    // The server port is given as an argument 

    server_address.sin_port = htons(atoi(address)); 
	return server_address;
}

// check if you can open the file

int check_for_open(int listener_socket,int connected_file_fd){
  // If the server receives a SIGINT signal, shutdown the server

        if(sigint_ind){

            server_shutdown();

        }

        // attempt to accept a connection

        connected_file_fd = accept(listener_socket, NULL, NULL);

        if (connected_file_fd < 0){

            failed("accept");

        }
	return connected_file_fd;
}

// create a socket

int socket_creation(int listener_socket,int ret_address){
if ((listener_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){

		  failed("socket creatrion");

	}

    if(setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, &ret_address, sizeof(int)) < 0){

        failed("setsockopt");

    }
	return listener_socket;
}


// bund socket to port

int bind_socket_to_port(int listener_socket,struct sockaddr_in server_address){

if (bind(listener_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0){

         failed("bind");

    }

return listener_socket;
}


// create the listen queue

int create_listen_queue(int listener_socket){
if (listen(listener_socket, 10) != 0){

            failed("listen");

    }

return listener_socket;
}

//count how many chars were read
int count_chars_read(int N,int C,  char *recieve_buffer,uint32_t *printable_chars_buffer){
int i;

i=0;

        while (i < N){

            if(32 <= recieve_buffer[i] && recieve_buffer[i] <= 126){

                C++;

                printable_chars_buffer[(int)(recieve_buffer[i]-32)]++;

            }
		++i;

        }

return C;
}

//read byte from client

int read_byte(int curr_bytes, int connected_file_fd,char * buffer, int all_bytes, int num){
curr_bytes = read(connected_file_fd, buffer+all_bytes, num-all_bytes);
return curr_bytes;
}

//write byte to client

int write_byte(int curr_bytes, int connected_file_fd,char * buffer, int all_bytes, int num){
curr_bytes = write(connected_file_fd, buffer+all_bytes, num-all_bytes);
return curr_bytes;
}

//----MAIN----------------//

// main function

int main(int argc, char *argv[])

{

    //----VARIABLES----------//

    int listener_socket = -1;

    int ret_address = 1;

    struct sockaddr_in server_address;

    char *integer_buffer;


    int all_bytes;

    int curr_bytes;

    uint32_t N;

    char *recieve_buffer="";
	char * address = argv[1];

    uint32_t network_integer_value, C;

    uint32_t printable_chars_buffer[95]={0};

	int i=0;
	 struct sigaction sigint;

    //----END OF VARIABLES----------//



    // validate number of arguments

  validate_number_of_arguments(argc);



     //----SIGINT INITALLIZATION----------//

   sigint_init(sigint);

      //----END OF SIGINT INITALLIZATION----------//



   //----SOCKET CREATION----------//

    

	listener_socket= socket_creation(listener_socket,ret_address);

   



   //----SERVER INITALLIZATION----------//



	server_address = server_init(server_address,address);

     

   //----END OF SERVER INITALLIZATION----------//

    

    //bind socket to port

	listener_socket =  bind_socket_to_port(listener_socket,server_address);



    //----END OF SOCKET CREATION----------//





    //Listen to incoming TCP connections on the specified server port. Using a listen() queue of size 10

    listener_socket = create_listen_queue(listener_socket);



     //----MAIN LOOP----------//

	 

    //once we started listenning 

   //we wait for connections in order to accept them 



    while (1){



      connected_file_fd = check_for_open(listener_socket,connected_file_fd);



        // read N from client 

        integer_buffer = (char*)&network_integer_value;

        all_bytes = 0;

        curr_bytes = 1;

        // as long as you can still read bytes, read them

        while (curr_bytes > 0){

		    curr_bytes = read_byte(curr_bytes,connected_file_fd,integer_buffer, all_bytes,4);
		

            all_bytes += curr_bytes;

	    }



            //check for errors in byte reading 

	    //if we're unable to read N

	    if(curr_bytes < 0 ){

            if (if_unknown_error()){

                failed("read N");

            }

            else{

               connected_file_fd=error("read",connected_file_fd, recieve_buffer,0);

                continue;

            }

	    }

            // if there are exactly 0 bytes when the reading ends

            else{ 

            // if we haven't read exactly 4 bytes, the process ended unexpectedly

            if(all_bytes != 4){

                 connected_file_fd=error("read N",connected_file_fd, recieve_buffer,0);

                continue;

            }

        }





    

        // cast N to network long form and allocate N bytes for reading

        N = ntohl(network_integer_value);

        recieve_buffer = malloc(N);



        // read file data from client (N bytes)

        all_bytes = 0;

        curr_bytes = 1;

        // as long as you can still read bytes, read them

        while (curr_bytes > 0){

		   curr_bytes = read_byte(curr_bytes,connected_file_fd,recieve_buffer, all_bytes,N);
		

            all_bytes += curr_bytes;

	    }



           //check for errors in byte reading 

	    //if we're unable to read all the bytes

	    if(curr_bytes < 0 ){

            if (if_unknown_error()){

                failed("read data");

            }

             else{

              connected_file_fd=error("read",connected_file_fd, recieve_buffer,1);

                continue;

            }

	    }



           // if there are exactly 0 bytes when the reading ends

           else{

            // if we haven't read exactly N bytes, the process ended unexpectedly

            if(all_bytes != N){ 

                connected_file_fd=error("read N",connected_file_fd, recieve_buffer,1);

                continue;

            }

        }



        // calculate C and count readable chars in pprintable_chars_buffer

        C = 0;

        //initallize printable chars buffer

	initallize_printable_chars_buffer(i,printable_chars_buffer);



        //count how many printable chars were read 
		C = count_chars_read(N,C, recieve_buffer,printable_chars_buffer);

        //end use of recieve buffer

        free(recieve_buffer);



        // send C's value to the client 

        network_integer_value = htonl(C);

        all_bytes = 0;

        curr_bytes = 1;



        // as long as you can still read bytes, read them

        while(curr_bytes > 0){

		    curr_bytes = write_byte(curr_bytes,connected_file_fd,integer_buffer, all_bytes,4);
			

		    all_bytes += curr_bytes;

	    }

            

            //check for errors in byte reading 

	    //if we're unable to write C

	    if(curr_bytes< 0){ 

            if (if_unknown_error()){

                failed("write C");

            }

            else{

              connected_file_fd=error("write C",connected_file_fd, recieve_buffer,0);

                continue;

            }

	    }

         // if there are exactly 0 bytes when the writing ends   

        else{ 

            // if we haven't written exactly 4 bytes, the process ended unexpectedly

            if(all_bytes != 4){ 

               connected_file_fd=error("read N",connected_file_fd, recieve_buffer,0);

                continue;

            }

        }

        //update the value of pcc total to bw the sum of all printable chars

	update_pcc_total(i,printable_chars_buffer);

        close(connected_file_fd);

        connected_file_fd = -1;

    }

   

    //----END OF MAIN LOOP----------//

}



//----END OF MAIN----------------//

//----END OF FILE----------------//

