#include "processes.h"

void *thread_a_function(void *args);
// void *thread_b_function(void *args);

int main(void) { 
    struct shm_struct *shm_p;
    int fd;
    char *path = "/process";


    // create shared memory segment
    fd = shm_open(path,O_RDWR,0);
    if( fd == -1) { 
        error_exit("shm_open");
    }

    // map the object 
    shm_p = mmap(NULL,sizeof(*shm_p), PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
    if( shm_p == MAP_FAILED) 
        error_exit("mmap");

    printf("Shared memory object: %s has been created at address: %p\n",path,shm_p);

    close(fd);

    // initialize the semaphores 
    int sem_res;

    sem_res = sem_init(&shm_p->sem_a,1,1); // init to 1
    if( sem_res == -1) 
        error_exit("sem_init");

    sem_res = sem_init(&shm_p->sem_b,1,0); // init to 0
    if( sem_res == -1) 
        error_exit("sem_init");



    // create the two threads
    pthread_t thread_a;
    // pthread_t thread_b;
    int res;

    res = pthread_create(&thread_a,NULL,thread_a_function,(void *) shm_p);
    if (res != 0) 
        error_exit("pthread_create");
    // res = pthread_create(&thread_b,NULL,thread_b_function,(void *)shm_p);
    // if (res != 0) 
    //     error_exit("pthread_create");


    // join the two threads
    res = pthread_join(thread_a,NULL);
    if(res != 0) 
        error_exit("pthread_join");
    // res = pthread_join(thread_b,NULL);
    // if(res != 0) 
    //     error_exit("pthread_join");


    shm_unlink(path);
    exit(EXIT_SUCCESS);
}

void *thread_a_function(void *arg) { 
    char input_string[15];
    struct shm_struct *shm_p = (struct shm_struct *) arg;
    int running = 1;

    while(running) {
        printf("Process B getting input from process A:\n");
        // wait sem_b
        if(sem_wait(&shm_p->sem_b) == -1 )
            error_exit("sem_wait");

        // CS
        
        strcpy(input_string,shm_p->buf_a);

        printf("Process B read: %s",input_string);

        // post sem_a
        if(sem_post(&shm_p->sem_a) == -1 )
            error_exit("sem_post");
    
        if(strncmp(input_string,"BYE",3) == 0) 
            running = 0;
    }

    return NULL;

}

// void *thread_b_function(void *arg) { 
//     char input_string[15];

//     printf("Process b waiting for input:\n");
//     fgets(input_string,15,stdin); // to 15 einai proswrino

//     printf("Process b output:%s",input_string);

//     return NULL;

// }