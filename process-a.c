#include "processes.h"

void *thread_send_function(void *args);
void *thread_receive_function(void *args);

int main(void) { 
    struct shm_struct *shm_p;
    int fd;
    char *path = "/process";


    // create shared memory segment
    shm_unlink(path); // unlink in case the file is open
    fd = shm_open(path,O_CREAT | O_EXCL | O_RDWR,0600); // mode = 0600 na to ksanadw
    if( fd == -1) { 
        error_exit("shm_open");
    }


    // set the size of the shared memory segment into our struct 
    if( ftruncate(fd,sizeof(struct shm_struct)) == -1) { 
        error_exit("ftruncate");
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

    sem_res = sem_init(&shm_p->sem_c,1,1); // init to 1
    if( sem_res == -1) 
        error_exit("sem_init");

    sem_res = sem_init(&shm_p->sem_d,1,0); // init to 0
    if( sem_res == -1) 
        error_exit("sem_init");



    // create the two threads
    pthread_t thread_send;
    pthread_t thread_receive;
    int res;

    shm_p->count_a = 0;
    shm_p->count_b = 0;

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

    printf("Number of messages sent:%d\n",shm_p->count_a);

    shm_unlink(path);
    exit(EXIT_SUCCESS);
}

void *thread_send_function(void *arg) { 
    char input_string[15];
    struct shm_struct *shm_p = (struct shm_struct *) arg;
    int running = 1;

    while(running) { 
        // wait sem_a
        if(sem_wait(&shm_p->sem_a) == -1 )
            error_exit("sem_wait");

        // CS
        printf("Process A waiting for input:\n");
        fgets(input_string,15,stdin); // 15 proswrino
        
        strcpy(shm_p->buf_a,input_string);

        // post sem_b
        if(sem_post(&shm_p->sem_b) == -1 )
            error_exit("sem_post");
        
        if(strncmp(input_string,"BYE",3) == 0) 
            running = 0;
        else 
            shm_p->count_a++;
    }

    return NULL;

}

void *thread_receive_function(void *arg) { 
    char input_string[15];
    struct shm_struct *shm_p = (struct shm_struct *) arg;
    int running = 1;

    while(running) {
        // wait sem_d
        if(sem_wait(&shm_p->sem_d) == -1 )
            error_exit("sem_wait");

        // CS
        printf("Process A getting input from process B:\n");
        
        strcpy(input_string,shm_p->buf_b);

        printf("Process A read: %s",input_string);

        // post sem_c
        if(sem_post(&shm_p->sem_c) == -1 )
            error_exit("sem_post");
    
        if(strncmp(input_string,"BYE",3) == 0) 
            running = 0;
    }

    return NULL;

}