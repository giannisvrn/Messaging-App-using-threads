#include "processes.h"

void *thread_send_function(void *args);
void *thread_receive_function(void *args);

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
    pthread_t thread_send;
    pthread_t thread_receive;
    int res;

    res = pthread_create(&thread_send,NULL,thread_send_function,(void *) shm_p);
    if (res != 0) 
        error_exit("pthread_create");
    res = pthread_create(&thread_receive,NULL,thread_receive_function,(void *)shm_p);
    if (res != 0) 
        error_exit("pthread_create");


    // join the two threads
    res = pthread_join(thread_send,NULL);
    if(res != 0) 
        error_exit("pthread_join");
    res = pthread_join(thread_receive,NULL);
    if(res != 0) 
        error_exit("pthread_join");

    printf("Number of messages sent:%d\n",shm_p->count_b);

    // shm_unlink(path);
    exit(EXIT_SUCCESS);
}

void *thread_receive_function(void *arg) { 
    char input_string[15];
    struct shm_struct *shm_p = (struct shm_struct *) arg;
    int running = 1;

    while(running) {
        // wait sem_b
        if(sem_wait(&shm_p->sem_b) == -1 )
            error_exit("sem_wait");

        // CS
        printf("Process B getting input from process A:\n");
        
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

void *thread_send_function(void *arg) { 
    char input_string[15];
    struct shm_struct *shm_p = (struct shm_struct *) arg;
    int running = 1;

    while(running) { 
        // wait sem_c
        if(sem_wait(&shm_p->sem_c) == -1 )
            error_exit("sem_wait");

        // CS
        printf("Process B waiting for input:\n");
        fgets(input_string,15,stdin); // 15 proswrino
        
        strcpy(shm_p->buf_b,input_string);

        // post sem_d
        if(sem_post(&shm_p->sem_d) == -1 )
            error_exit("sem_post");
        
        if(strncmp(input_string,"BYE",3) == 0) 
            running = 0;
        else 
            shm_p->count_b++;
    }

    return NULL;

}