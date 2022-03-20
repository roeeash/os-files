#include <sys/types.h> 
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define FAIL 1
#define SUCCESS 0

int status_code=0;

//exits failed processes
void failed(char * error){
 if (strcmp(error,"wait")==0)
{
fprintf(stderr, "wait failed in signal handler, error number %s\n", strerror(errno));
		exit(FAIL);
}

 if (strcmp(error,"register")==0)
{

fprintf(stderr, "Signal handle registration error, error number %s\n", strerror(errno));
		exit(FAIL);
}

 if (strcmp(error,"terminate")==0)
{
 printf("Process did not terminate correctly\n");
        exit(FAIL);
}

 if (strcmp(error,"execvp")==0)
{

fprintf(stderr, "execvp error, error number %s\n", strerror(errno));
		exit(FAIL);
}


}


//prints error message and returns int value
int error(char * error){
 if (strcmp(error,"pipe")==0){
fprintf(stderr, "pipe failed with error%s\n", strerror(errno));
}
 if (strcmp(error,"fork")==0){
fprintf(stderr, "fork failed with error%s\n", strerror(errno));
}

 if (strcmp(error,"dup")==0){
fprintf(stderr, "dup failed with error%s\n", strerror(errno));
}

 if (strcmp(error,"wait")==0){
fprintf(stderr, "wait failed with error%s\n", strerror(errno));
}

if(strcmp(error,"registration")==0){

fprintf(stderr, "Signal handle registration , error number %s\n", strerror(errno));
		return -1;
}

return 0;
}
void handle_error_in_child_process(int pid){
if(pid==-1 && errno!=EINTR && errno!=ECHILD){
		failed("wait"); 
	}
}



// a sigaction handler for child processes
void child_handler(int sig)
{
    int prev_errno = errno;
    pid_t pid;
    int status;
 while((pid = waitpid(-1, &status, WNOHANG)) > 0);
handle_error_in_child_process(pid);
	errno = prev_errno;
}

// inits sigaction

struct sigaction set_sigaction_data(struct sigaction name,int flag,void * handler){

  sigemptyset(&name.sa_mask);
     name.sa_flags = flag;
     name.sa_handler = handler;

return name;


}





sigset_t sigal_set;


int prepare(void) {
	
	 sigemptyset(&sigal_set);
	 sigaddset(&sigal_set, SIGINT);
	
	 // ignore sigint
	 
	 struct sigaction ign;
	 ign = set_sigaction_data(ign,SA_RESTART,SIG_IGN);


	 if (sigaction(SIGINT, &ign, 0) != 0) {
		return error("registration");
	}
	 
	 // link child hanlder
	 
     struct sigaction sa;
    sa = set_sigaction_data(sa,SA_RESTART,&child_handler);

     if (sigaction(SIGINT, &sa, 0) != 0) {
		return error("registration");
	}
return 0;
}

//performs execvp from file and arglist
	
int get_status_code_on_file(char * file, char ** arglist){
int code;
code = execvp(file,arglist);
	  if (code == -1) {
        failed("terminate");
}
return code;
}

//waits for process
int perform_wait(int pid, int status_code){

int wait_status=waitpid(pid, &status_code, 0);
if(wait_status==-1 && errno!=EINTR && errno!=ECHILD){
		return error("wait");
}
return 1;
}

//checks if two strings are equal
char strings_equal(char ** list, int index, char * string){
 if(strcmp(list[index],string)==0)
		return index;
return -1;
}


int get_status_code(int index,char ** arglist){
int code;
code = execvp(arglist[index],arglist+index);
	  if (code == -1) {
        failed("terminate");
}
return code;
}

// a function that restarts sigint and will be performed for each child process
void restart_sigint() {
	 struct sigaction sig_act;
   	set_sigaction_data(sig_act,SA_RESTART,SIG_DFL);
	if (sigaction(SIGINT, &sig_act, 0) != 0) {
		failed("register"); 
	}
}


//closes fd and restarts sigint

void close_and_restart(int * fd, int index){

//close read channel for process 
	close(fd[index]);
	// restart signint
	restart_sigint();
}

// piping function, pipes first file output to second file input.
// reutrns 1 upon success, 0 upon failure

int piping(char ** arglist, int index){
	
int pid1, pid2;
int fd[2];

arglist[index]=NULL;

// if pipe failed
if(pipe(fd)==-1){
return error("pipe");

}


//two child processes 

// if the first one failed
pid1 = fork();

sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
if (pid1<0){
return error("fork");
}

//child process 1 
sigprocmask(SIG_BLOCK, &sigal_set, NULL);
if(pid1==0){
	close_and_restart(fd, 0);
       sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
	// duplicate the info from output buffer to stdout using dup2(fd[1],1)
	
	if(dup2(fd[1],1)==-1){
	
	return error("dup");
	}
	close(fd[1]);
	status_code = get_status_code(0,arglist);
        
    
	
	
}

sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);

pid2 = fork();
// if the second one failed
if (pid2<0){
	return error("fork");
}

sigprocmask(SIG_BLOCK, &sigal_set, NULL);
if(pid2==0){
		close_and_restart(fd, 1);
        sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
	
	// duplicate the info from input buffer to stdin using dup2(fd[0],0)
	
	if(dup2(fd[0],0)==-1){
	
	return error("dup");
	}
	close(fd[0]);
	status_code = get_status_code(index+1,arglist);
    
}
sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
// close both input and output buffer channels
close(fd[0]);
close(fd[1]);

// parent process
// we must wait for child process to end to avoid zombies

int wait_status=perform_wait (pid1,status_code);

if( wait_status!=1){
return wait_status;
}

wait_status=perform_wait (pid2,status_code);
if( wait_status!=1){
return wait_status;
}

//succeded in piping the files
return 1;
}



// a function that checks if arglist has a "|" symbol, 
// returns location upon success, 0 upon failure

int obtain_pipe_position(char ** arglist){
	int position = 0;
        int lookup=-1;
	while(arglist[position]!=NULL){
        lookup= strings_equal(arglist,position,"|");
    if(lookup!=-1)
		return lookup;
	position++;
	}
	return 0;
}


// this function creates a new arglist from args appearing before the arrow

void create_new_arglist(char ** new_arglist,char ** arglist,int count){
int i;
for (i=0;i<count-2;i++){
	strcpy(new_arglist[i],arglist[i]);
	}
	
	new_arglist [count-2]=NULL;
      
}


//this function performs the arrow function

int output_redirect(int count, char** arglist)
{
	int pid;
        
	// open new file
	int fd = open(arglist[count-1], O_WRONLY | O_CREAT,0777); 
	arglist[count-2] = NULL; 
	arglist[count] = NULL;
        int wait=1;
       
	
	pid = fork();
	if (pid<0){
		return error("fork");
        }

	if(pid == 0)
	{
		if(dup2(fd,STDOUT_FILENO) == -1){
			return error("dup");
	        }
	
	

	// cut arglist at arrow position
	char * new_arglist [count-2];
	create_new_arglist(new_arglist,arglist,count);
	
	status_code = get_status_code(0,new_arglist);
	}
	// parent process
    
       // we must wait for child process to end to avoid zombies
close(fd);
    wait = perform_wait(pid,status_code);
if(wait != 1){
return wait;
}


return -1;
}









// a function that checks if arglist contains a ">" symbol, 
// returns location upon success, 0 upon failure

int obtain_arrow_position(char ** arglist){

	int position = 0;
        int lookup=-1;
	while(arglist[position]!=NULL){
        lookup= strings_equal(arglist,position,">");
    if(lookup!=-1)
		return lookup;
	position++;
	}
	return 0;
}


//this function performs the actions within a child process
int perform_child_action(char * file, char ** list){
restart_sigint();
	 sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
	 status_code = get_status_code_on_file(file,list);
         return status_code;
}

//this function checks if a command is performed in the background
// and if so, excutes it
int check_for_background_action(int count, char** arglist){
int pid;
int   curr_child = -1;
int   keep_working = 1;

	// check if last arg is ampersand
	
       if (strcmp(arglist[count - 1], "&") == 0) {
		pid = fork();
		
		// if fork failed
		
			if (pid < 0) {
				return error("fork");
		}
		if (pid == 0) { 
		
		//child process
			arglist[count - 1] = NULL;
			status_code = get_status_code(0,arglist);
			failed("execvp");
		}
	
		return 1;
	}
	
	

	
  sigprocmask(SIG_BLOCK, &sigal_set, NULL);
  pid = fork();
  
  //child process
   if(pid==0){
	status_code = perform_child_action(arglist[0],arglist);
	}
	
	// if the fork failed
sigprocmask(SIG_UNBLOCK, &sigal_set, NULL);
if (pid<0){
	return error("fork");
}
	
 curr_child = waitpid( pid,
                        &status_code,
                        0 );


  kill( pid, SIGTERM );

//wait for background process to terminate

  while( keep_working )
    if( -1 != ( curr_child = wait( &status_code ) ) ){}
    else
      keep_working = 0;
  

return 1;
}

int process_arglist(int count, char** arglist) {

    int pipe_position;
    int arrow_position;
    
 

    // check if it conatains a pipe symbol
	pipe_position = obtain_pipe_position(arglist);
	if (pipe_position!= 0) {
		return piping(arglist, pipe_position);
	}
	
	
	
	
	 
    // check if it conatains a arrow symbol
	arrow_position= obtain_arrow_position(arglist);
	if (arrow_position!= 0) {
		return output_redirect(count,arglist);
	}
	
	// checks if runs in background and runs it 
	return  check_for_background_action(count, arglist);
	
	
	
}


int finalize(void) {
	return 0;
}