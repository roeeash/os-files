//----------FILE : PFIND.C-------//

//----------INCLUDES-------//
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
//----------END OF INCLUDES-------//

//------ MACROS------------------//
#define ACCEPTABLE_FAIL 1
#define SUCCESSFUL 0
#define UNACCEPTABLE_FAIL -1
#define MAX_LENGTH PATH_MAX
//------END OF MACROS------------------//

//------QUEUE------------------//
typedef struct search_node {
    //pointer to the next node
    struct search_node *next;
 //directory path name
    char dir_path[MAX_LENGTH];
} search_node;

typedef struct search_queue {
    //pointer to beginning of the queue
    struct search_node *first;
       //pointer to end of the queue
    struct search_node *last;
} search_queue;
//------END OF QUEUE------------------//


//------ASSISTING FUNCTIONS DECLARATION------------------//
void check_for_handoff(search_node *,int);
int increase_matches(int, char *);
void init_all_locks();
void destroy_all_locks();
int switch_waiting_mode(int);
int increase_error_threads (int, pthread_mutex_t,int);
void handle_error_with_file(char *,char *, int);
void handle_error(char *, int);
void error(char *, char *);
void max_num_of_threads_reached();
int allocate_directory(search_node * ,char * , int);
void check_for_threads_prepared(int, int);
void check_for_unsuccessful_remove_first(char * , int ,pthread_mutex_t);
void check_for_unsuccessful_search(char * , int ,pthread_mutex_t);
int check_for_successful_allocation(search_node *,char *,int );
void create_temp_content(search_queue *,char *,search_node *);
int found_dir_in_search(char *);
int found_dir_in_search(char *);
//------END OF ASSISTING FUNCTIONS DECLARATION------------------//


//prompts errors

void error(char * err, char * str){
if (strcmp(err,"empty queue")==0){
fprintf(stderr, "trying to delete from an empty queue Error: %s\n", strerror(errno));
}
if (strcmp(err,"open dir")==0){
   fprintf(stderr, "error opening directory %s: %s\n",str,strerror(errno));
}
if(strcmp(err,"get stats")==0){
fprintf(stderr, "failed to get stats on %s: %s\n", str,strerror(errno));
}
if(strcmp(err,"invalid name")==0){
 printf("name starts with . or .., so we ignore it. The path is:%s\n", str);
}
if(strcmp(err,"malloc")==0){
 fprintf(stderr, "malloc directory for queue failed: %s\n", strerror(errno));
}
if(strcmp(err,"permission denied")==0){
 fprintf(stderr, "Permission denied: %s\n", str);
}
if(strcmp(err,"remove first")==0){
fprintf(stderr, "problem with removing the first item Error: %s\n", strerror(errno));
}
if(strcmp(err,"bad directiory")==0){
printf("Directory %s: Permission denied.\n", str);
}



}




//----------REMOVE AND INSERT-------//

//inits queue

search_queue *initallize_queue(search_queue *queue){
queue->first=NULL;
            queue->last=NULL;
return queue;
}

//creates the content of "temp" in remove function

void create_temp_content(search_queue *queue,char *path,search_node *temp){
  strcpy(path,queue->first->dir_path);
        temp=queue->first;
}

//removes first element in queue

int remove_first(search_queue *queue,char *path){
    
    search_node *temp = malloc(sizeof(search_node));

    if(queue->first==NULL){
        error("empty queue","");
        return UNACCEPTABLE_FAIL;
    }
    else{
        // create temp
	create_temp_content(queue,path,temp);
       




        if (queue->first->next==NULL){
            queue = initallize_queue(queue);
        }
        else{
            queue->first=queue->first->next;  
        }
        free(temp);
        return SUCCESSFUL;
    }
}
//preforms insertion into the queue

search_queue *insertion(search_queue *queue,char * state, search_node *new_search){
	
	if(strcmp(state,"empty")==0){
queue->first=new_search;
        queue->last=new_search;
}
else{
	 queue->last->next=new_search;
        queue->last=new_search;
}
return queue;
}

//insert function - checks queue state and uses insetrion

int insert(search_queue *queue, search_node *new_search){
    if(queue->first==NULL){
        // inserting to empty queue
       queue=insertion(queue,"empty",new_search);
        return ACCEPTABLE_FAIL;
    }
   else{
        // inserting to queue's last element
         queue=insertion(queue,"not empty",new_search);
        return SUCCESSFUL;
    }
}


//----------END OF REMOVE AND INSERT-------//


//----VARIABLES----------//

//----FOR SEARCH DIR-----//
char *search_name;
pthread_mutex_t queue_lock;
pthread_cond_t non_empty;
pthread_mutex_t count_lock;
int wait_cond_threads = 0; 
//---END OF FOR SEARCH DIR---//

//----FOR SEARCH MULTIPLE THREADS-----//
int handoff_required = 0;
pthread_cond_t handoff_performed;
search_queue *queue;  
int matches = 0;     
int running_threads = 0; 
pthread_cond_t threads_prepared;
int error_threads = 0;   
int  thread_number;

pthread_mutex_t begin_lock;
pthread_cond_t begin_cond;
//----END OF FOR SEARCH MULTIPLE THREADS-----//

//----END OF VARIABLES----------//


//----------DIR SEARCH-------//

//checks if handoff is required

void check_for_handoff(search_node *next_search,int empty_prior){
    pthread_mutex_lock(&queue_lock);
                empty_prior = insert(queue,next_search);

                // insert would return acceptable fail if the queue was empty
                // and we tried to insert into it

                if (empty_prior == 1){ 
                   
                    if (wait_cond_threads > 0){
                        handoff_required = 1;
                    }
                    pthread_cond_signal(&non_empty);
                }
                pthread_mutex_unlock(&queue_lock);
            }

//increases the number of matches

int increase_matches(int matches,char * next_path){
 pthread_mutex_lock(&count_lock);
            matches++;
            pthread_mutex_unlock(&count_lock);
            printf("%s\n", next_path);
            return matches;  
}

//allocates new directory

int allocate_directory(search_node * dir,char * next_path, int empty){
        
                if (dir ==NULL){
                   error("malloc","");
                return UNACCEPTABLE_FAIL;
                }
                strcpy(dir->dir_path,next_path);
                dir->next=NULL;
                
                //atomically check if insertion is possible
               check_for_handoff(dir,empty);
return 0;
}

//prompts failed search

int failed_search(char *err,char *search_path){
if(strcmp(err,"open")==0){
       error("open dir",search_path);
        return UNACCEPTABLE_FAIL;
}

if(strcmp(err,"stats")==0){
error("get stats",search_path);
            return UNACCEPTABLE_FAIL;
}

return 1;
}

// prints search path

void print_search_path(char * next_path,char *search_path,char * dir_name){
sprintf(next_path, "%s/%s",search_path, dir_name); 
}

//checks if allocation was successful

int check_for_successful_allocation( search_node *next_search,char * next_path,int empty_prior){
 if(allocate_directory(next_search,next_path,empty_prior)==UNACCEPTABLE_FAIL)
                    return UNACCEPTABLE_FAIL;

return SUCCESSFUL;
}

//checks if user has permissions to the file

int user_has_access(struct stat info){
if(info.st_mode & S_IXUSR && info.st_mode & S_IRUSR){
	return 1;
}
else
	return 0;
}

//checks if found in search

int found_dir_in_search(char * dir_name){
if(strstr(dir_name, search_name) != NULL)
	return 1;
return 0;
}

int dir_search(char *search_path){

    //---VARIABLES---//
    int empty_prior;
    struct dirent *directory_entry;
    char * dir_name;
    struct stat info;
    DIR *directory = opendir(search_path);
    char next_path[MAX_LENGTH];
    search_node *next_search;
    //---END OF VARIABLES---//
    
    //---ERROR HANDELLING ON INPUT---//
    if(directory == NULL){
        return failed_search("open",search_path);
    }
    //---END OF ERROR HANDELLING ON INPUT---//

    while((directory_entry=readdir(directory)) != NULL){
       
       dir_name =  directory_entry->d_name;
       
        print_search_path(next_path,search_path,dir_name);

        //check if you can copy the stats 
        if (lstat(next_path, &info) != SUCCESSFUL){
         return failed_search("stats",search_path);
        }
        //if it is a directory
        if (S_ISDIR(info.st_mode)){

            if ((strcmp( dir_name, ".") == 0) || (strcmp(dir_name, "..") == 0)){
              error("invalid name",dir_name);
            }

             //if the file is user readable or user accessable 
             else if (user_has_access(info)){
                //allocate a directory
               next_search = malloc(sizeof(search_node));

                //check if allocation is successful
		if(check_for_successful_allocation(next_search,next_path,empty_prior) == UNACCEPTABLE_FAIL)
		return UNACCEPTABLE_FAIL;
               
            
            }
             else{
             
                  error("permission denied",next_path);
            }
        
        }
      else if (found_dir_in_search(dir_name)){

            //number of matches is increased atomically
            matches = increase_matches(matches,next_path);
           
        }
    }


    closedir(directory);
    return SUCCESSFUL;
}




//----------END OF SEARCH DIR-------//






//----------ASSISTING METHODS-------//

//----------SWITCH WAITING MODE-------//
//if the thread is waiting, switch it to not waiting and vice versa
int switch_waiting_mode(int waiting_mode){
if (waiting_mode==0)
return 1;
return 0;
}

//inits all locks

    void init_all_locks(){


pthread_mutex_init(&count_lock,NULL);
    pthread_mutex_init(&begin_lock,NULL);
    pthread_mutex_init(&queue_lock,NULL);
    pthread_cond_init(&threads_prepared, NULL);
    pthread_cond_init(&begin_cond, NULL);
    pthread_cond_init(&non_empty, NULL);
    pthread_cond_init(&handoff_performed, NULL);

}

//destroys all locks

void destroy_all_locks(){

    pthread_mutex_destroy(&count_lock);
    pthread_mutex_destroy(&begin_lock);
    pthread_mutex_destroy(&queue_lock);
    pthread_cond_destroy(&threads_prepared);
    pthread_cond_destroy(&begin_cond);
    pthread_cond_destroy(&non_empty);
    pthread_cond_destroy(&handoff_performed);


}


//------END OF ASSISTING METHODS------------------//

//----------INCREASE ERROR THREADS-------//

int increase_error_threads (int error_threads, pthread_mutex_t lock,int lock_needed){

  error("error threads","");
            if(lock_needed){   
pthread_mutex_lock(&lock);
}
            error_threads++;
            pthread_mutex_unlock(&lock);
            return error_threads;
            
}

//----------INCREASE ERROR THREADS-------//

//----------END OF ASSISTING METHODS-------//


void max_num_of_threads_reached(){
pthread_cond_broadcast(&non_empty);
pthread_cond_broadcast(&handoff_performed);
}

//check if all allowed threads are running

void check_for_threads_prepared(int running_threads, int thread_number){
 if(running_threads ==  thread_number){
        pthread_cond_broadcast(&threads_prepared);
    }
}

//checks if remove first was successful or not

void check_for_unsuccessful_remove_first(char * dir_path, int error_threads,pthread_mutex_t queue_lock){
 if (remove_first(queue,dir_path)!=SUCCESSFUL){
            error("remove first","");
             error_threads = increase_error_threads(error_threads,queue_lock,0);
            pthread_exit(NULL);
        }
}

//checks if search was successful or not
void check_for_unsuccessful_search(char * dir_path, int error_threads,pthread_mutex_t queue_lock){
 if (dir_search(dir_path)!=SUCCESSFUL){
            // return value from function indicating an error has occurred
          error_threads = increase_error_threads(error_threads,queue_lock,1);
         pthread_exit(NULL);
        }
}
//checks if handoff is needed or not

void check_for_needed_handoff(int handoff_required,int waiting_mode){
 if ((handoff_required != 0) && (waiting_mode != 0)){
            handoff_required = 0;
            pthread_cond_broadcast(&handoff_performed);
        }
}

//decreases number of threads waiting on cond

void decrease_wait_cond_threads(int waiting_mode, int wait_cond_threads){

        if (waiting_mode == 1){
            wait_cond_threads--;
        }
}
int increase_wait_cond_threads(int waiting_mode, int flag){
wait_cond_threads++;
if(flag){
waiting_mode = 1;
}
return waiting_mode;
}
//----------SEARCH MULTIPLE THREADS-------//
void *multiple_thread_search(void *i){

     //---VARIABLES---//
    int waiting_mode =0;
    char dir_path[MAX_LENGTH];
    //---END OF VARIABLES---//
    

    //if we've reached the desired number of threads, broadcast that all theadrs are ready
    pthread_mutex_lock(&begin_lock);
   running_threads++;
   check_for_threads_prepared(running_threads,thread_number);
    pthread_cond_wait(&begin_cond, &begin_lock);
    pthread_mutex_unlock(&begin_lock);
    

   //queue handelling loop
    while (1){

        waiting_mode =0;
        pthread_mutex_lock(&queue_lock);
        
        if (wait_cond_threads > 0 && (handoff_required != 0)){
           waiting_mode = increase_wait_cond_threads(waiting_mode,1);
            pthread_cond_wait(&handoff_performed,&queue_lock);
        }
        while(queue->first==NULL){
           
            //if no one is waiting, make sure the next thread waits
            if (waiting_mode == 0){
                increase_wait_cond_threads(waiting_mode,0);
               waiting_mode = switch_waiting_mode(waiting_mode);
            }

            // if we've produced the desired number of threads, inform using brodcast
            if (  thread_number == wait_cond_threads + error_threads){
                
                max_num_of_threads_reached();
                pthread_mutex_unlock(&queue_lock);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&non_empty,&queue_lock);
        }
        //if this current thread is waiting, when we handle it,
        // the number of waiting threads reduces by 1
        decrease_wait_cond_threads(waiting_mode, wait_cond_threads);

        // attempt to remove the first from the queue
        check_for_unsuccessful_remove_first(dir_path, error_threads,queue_lock);
       
        //if handoff is required and the current threads is waiting
        check_for_needed_handoff(handoff_required, waiting_mode);
        pthread_mutex_unlock(&queue_lock);
        // iterate on the directory's files, searching for matches and other directories
       check_for_unsuccessful_search(dir_path, error_threads,queue_lock);



    }
}




//----------END OF SEARCH MULTIPLE THREADS-------//

void handle_error_with_file(char *error,char * file, int exit_code){

//error in getting info on root
if (strcmp(error,"root")==0){
fprintf(stderr, "error getting stats on root - %s: %s\n", file,strerror(errno));
        exit(ACCEPTABLE_FAIL);
}

//error in user access
if(strcmp(error,"user")==0 ){
	 fprintf(stderr, "error no permission to access root - %s: %s\n", file,strerror(EACCES));
        exit(ACCEPTABLE_FAIL);
}
}


void handle_error(char *error, int exit_code){
 
//error in number of arguments
    if(strcmp(error,"arguments") ==0){
    fprintf(stderr, "error in number of arguments, Error: %s\n", strerror(EINVAL));
		exit(exit_code);

}



//error in malloc
if(strcmp(error,"malloc") ==0){
 fprintf(stderr, "attempted malloc for queue failed: %s\n", strerror(errno));
        exit(ACCEPTABLE_FAIL);
}

//error in insertion to queue
if(strcmp(error,"insert") ==0){
 fprintf(stderr, "error while adding root: %s\n", strerror(EBADE));
        exit(ACCEPTABLE_FAIL);
}

}

search_queue* init_queue(search_queue* queue){
    if (queue==NULL){
       handle_error("malloc",ACCEPTABLE_FAIL);
    }
    //init queue
    queue->first=NULL;
    queue->last=NULL;

return queue;
}

search_node * init_root(search_node * root_search, char* filename){
  //check for malloc problem
    if (root_search==NULL){
      handle_error("malloc",ACCEPTABLE_FAIL);
    }
    strcpy(root_search->dir_path,filename);
    root_search->next=NULL;
return root_search;
}



void init_threads(pthread_t * total_threads, int thread_number){
long i;
i = 0;
while ( i <  thread_number) {
        int rc = pthread_create(&total_threads[i], NULL, multiple_thread_search,  (void *)i);
        if (rc) {
            fprintf(stderr, "error while creating thread %ld: %s\n",i , strerror(rc));
            exit(ACCEPTABLE_FAIL);
        }

 ++i;


    }
}

void find_matches(pthread_t * total_threads, int thread_number){
long i;

i = 0;

 // attempt to wait for all running threads to finish using join
    while (i <  thread_number) {
        pthread_join(total_threads[i], NULL);

        ++i;
    }
    printf("Done searching, found %d files\n", matches);
}

void start_process(pthread_t * total_threads, int thread_number){
// run the desired amount of threads
    pthread_mutex_lock(&begin_lock);
    
    init_threads(total_threads, thread_number);
    // waiting for all locks to be ready to bgein the search and then signaling them all to start
    pthread_cond_wait(&threads_prepared, &begin_lock);
    pthread_cond_broadcast(&begin_cond); 
    pthread_mutex_unlock(&begin_lock);
}
void check_input_correctness(int argc){
   // check input corrrectness
    if (argc != 4) {
        handle_error("arguments",ACCEPTABLE_FAIL);	
    }
}

void check_errors_in_stats(struct stat root_stats, char * filename){
 if (!(root_stats.st_mode & S_IXUSR && root_stats.st_mode & S_IRUSR)){
        error("bad directiory",filename);
        handle_error_with_file("user",filename,ACCEPTABLE_FAIL);	
       
    }
}

//----------MAIN-------//
int main(int argc, char** argv){



    //--VARIABLES------//
   
    int root_insert;
    search_node *root_search;
   char* filename = argv[1];
    struct stat root_stats;
    //--END OF VARIABLES------//

 check_input_correctness(argc);

    search_name = argv[2];
    thread_number = atoi(argv[3]);

    // check if able to read stats
    if (lstat(filename, &root_stats) !=SUCCESSFUL){
         handle_error("root",ACCEPTABLE_FAIL);
    }
    //check if user is able to execute and read the file
    
   check_errors_in_stats(root_stats, filename);
    // creating the queue and adding the directory of the search root into the queue
    queue = malloc(sizeof(search_queue));
queue = init_queue(queue);


    root_search= malloc(sizeof(search_node));

  root_search = init_root(root_search,filename);
    root_insert = insert(queue, root_search);

    //check if we managed to insert
    if(root_insert!=1){
        handle_error("insert",ACCEPTABLE_FAIL);
    }
    // initallize mutex and locks  
    init_all_locks();

pthread_t total_threads[ thread_number];

    start_process(total_threads, thread_number);

find_matches(total_threads, thread_number);
   

    // destroying threads and conds and exiting
    destroy_all_locks();
    free(queue);

   //if some threads ended with an error
    if (error_threads != 0){
        exit(ACCEPTABLE_FAIL);
    }
    //otherwise exit with exit code success
    exit(SUCCESSFUL);
}
//----------END OF MAIN-------//






//----------END OF FILE-------//