//----------FILE : PCC_CLIENT.C-------//



//----------INCLUDES-------//

#include <errno.h>

#include <string.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <errno.h>

#include <arpa/inet.h>

#include <netinet/in.h>

#include <netdb.h>

//----------END OF INCLUDES-------//



//------- MACROS------------------//

#define FAILED 1

#define SUCCESS 0

#define D -1

#define _DEFAULT_SOURCE

//------END OF MACROS------------------//


//----ASSISTING FUNCTIONS DECLARATION----------------//


void failed(char *);

struct sockaddr_in initallize_server_data (struct sockaddr_in,char *,char *);

char * buffer_creation(FILE *, char *, uint32_t);

uint32_t find_N(FILE *, uint32_t, char *);

void print_C(int C,int network_integer_value);

void write_bytes(int,int , int , int,char *,int,int ,int );

void read_bytes(int ,int , int , int ,char * );
void validate_number_of_arguments(int);
int create_socket(int);
int connect_socket_to_server(int,struct sockaddr_in );


//----END OF ASSISTING FUNCTIONS DECLARATION----------------//


//error handelling



void failed(char * error){

 if (strcmp(error,"arguments")==0)

{

  fprintf(stderr, "wrong number of arguments, Error: %s\n", strerror(EINVAL));

        exit(FAILED);



}

 if (strcmp(error,"open")==0)

{

   fprintf(stderr, "failed to open input file Error: %s\n" , strerror(errno));

        exit(FAILED);



}



if(strcmp(error,"allocate")==0){

fprintf(stderr, "failed to allocate bytes for the buffer: %s\n", strerror(errno));

        exit(FAILED);

}



if(strcmp(error,"read")==0){

fprintf(stderr, "failed to read from file: %s\n", strerror(errno));

        exit(FAILED);

}



 if (strcmp(error,"socket creation")==0)

{

  fprintf(stderr, "problem with socket creation Error: %s\n", strerror(errno));

        exit(FAILED);



}



 if (strcmp(error,"connect")==0)

{

fprintf(stderr, "Connect Failed, Error: %s\n", strerror(errno));

        exit(FAILED);



}

 if (strcmp(error,"write N")==0)

{

fprintf(stderr, "failed to write N to the server, Error: %s\n", strerror(errno));

        	exit(FAILED);



}

 if (strcmp(error,"write data")==0)

{

fprintf(stderr, "failed to write data to server, Error: %s\n", strerror(errno));

        	exit(FAILED);



}



 if (strcmp(error,"read C")==0)

{

fprintf(stderr, "failed to read C from server, Error: %s\n", strerror(errno));

        	exit(FAILED);



}

}



// init server private data


struct sockaddr_in initallize_server_data (struct sockaddr_in server_address,char * file,char * port){



	memset(&server_address, 0, sizeof(server_address));

	inet_pton(AF_INET,file, &server_address.sin_addr); // sets IP from args

	server_address.sin_family = AF_INET;

	server_address.sin_port = htons(atoi(port)); // sets port from args, Note: htons for endiannes

return server_address;





}


//create buffer


char * buffer_creation(FILE *read_file, char * out_buffer, uint32_t N){

fseek(read_file, 0, SEEK_SET);

	out_buffer = malloc(N); // allocate N bytes for the buffer

        

        //check if buffer is functional

	

	//check if it could be opened:

	if (out_buffer == NULL){

		   failed("allocate");

	}

        //check if it could be read from:

	if (fread(out_buffer, 1, N, read_file) != N){

		 failed("read");

	}

	fclose (read_file);

        return out_buffer;

}



uint32_t find_N(FILE *read_file, uint32_t N, char * fd){

     

	

        //detect errors while opening the file 

	if (read_file == NULL) {

       failed("open");

        }



        // assign N to be read_file's size in bytes

 

	

	fseek(read_file, 0, SEEK_END);

	N = ftell(read_file);

return N;

}



//write bytes to srever


void write_bytes(int all_bytes,int value, int curr_bytes, int client_socket,char * buffer,int num,int N,int C){



all_bytes = 0;



	while(all_bytes < value){

                 //attempt to write value to the server

		curr_bytes = write(client_socket, buffer +all_bytes, value-all_bytes);

		if(curr_bytes < 0){

			if(num == N){ 

			failed("write N");

			}

			else if (num == D){ 

			failed("write data");

			}

		}

		all_bytes += curr_bytes;

	}



}


//read bytes from server


void read_bytes(int all_bytes,int value, int curr_bytes, int client_socket,char * buffer){



all_bytes = 0;



	while(all_bytes < value){

                 //attempt to write value to the server

		curr_bytes = read(client_socket, buffer +all_bytes, value-all_bytes);

		if(curr_bytes < 0){

			failed("read C");

		}

		all_bytes += curr_bytes;

	}



}


//print the value of C

void print_C(int C,int network_integer_value){

	C = ntohl(network_integer_value);

	printf("# of printable characters: %u\n", C);

}



// validate the correct number of arguments

void validate_number_of_arguments(int argc){
if (argc != 4) {

        failed("arguments");

        }
}

//create the socket with wchich you connect to the server

int create_socket(int client_socket){
if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){

		failed("socket creation");

	}
return client_socket;
}

// connect the created socket to the new server

int connect_socket_to_server(int client_socket, struct sockaddr_in  server_address){
if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){

		failed("connect");

	}
return client_socket;
}





//----MAIN----------------//

int main(int argc, char *argv[])

{

        //----VARIABLES----------//

	char * file = argv[1];

	char * port = argv[2];

	char * fd = argv[3];

	FILE *read_file;

	uint32_t N=0;

	char *out_buffer="";

	int client_socket = -1;

	struct sockaddr_in server_address;

	uint32_t network_integer_value=0;

	char *integer_buffer="";

	int all_bytes=0;

	int curr_bytes=0;

	uint32_t C=0;

	//int value=0;

	//----END OF VARIABLES----------//



	// validate that the correct number of command line arguments is passed

	validate_number_of_arguments(argc);

        

        //open file

	read_file = fopen(argv[3],"rb");



        //find N

	N=find_N(read_file,N,fd);



    //----BUFFER CREATION----------//

         out_buffer = buffer_creation(read_file,out_buffer,N);

        //----END OF BUFFER CREATION----------//





        //from this point on we only work with the buffer and the socket

        

       //----SOCKET CREATION----------//



	//attempt to create socket

	client_socket = create_socket(client_socket);



	//initallize the private data of the server 

       server_address = initallize_server_data(server_address,file,port);



	//attempt to connect the socket to the server
	
	connect_socket_to_server(client_socket,server_address);


       //----END OF SOCKET CREATION----------//



       // 1. copy N to server to indicate size

	network_integer_value = (htonl(N));

	integer_buffer = (char*)&network_integer_value;



        //Writing the value of N which is a 32-bit unsigned integer in network byte order

	write_bytes(all_bytes,4, curr_bytes, client_socket, integer_buffer,N,N,C);



	// 2. copy all N bytes from the buffer to the server

	write_bytes(all_bytes,N, curr_bytes, client_socket, out_buffer,D,N,C);



        //end of out buffer use

	free(out_buffer);



	// 3. recieve C from the server

	read_bytes(all_bytes,4, curr_bytes, client_socket,integer_buffer);



        //end of client socket use

	close(client_socket);



	// convert C to host long and print the numebr of printable chars

	print_C(C, network_integer_value);

	exit(SUCCESS);

}



//----END OF MAIN----------------//

//----END OF FILE----------------//