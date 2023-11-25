#include "processes.h"

void *thread_send_function(void *args);
void *thread_receive_function(void *args);

pthread_t thread_to_cancel;
pthread_t thread_to_cancel2;


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
    char *input_string = (char *)malloc(1000 * sizeof(char));
    struct shm_struct *shm_p = (struct shm_struct *)arg;
    int running = 1,offset=0;

    thread_to_cancel = pthread_self();

    while(running) {
        // sem down
        if( sem_wait(&shm_p->sem_b) == -1)
            error_exit("sem_wait");

        // CS
        if( shm_p->new_string_received_a) { 
            init_str(input_string);
            offset = 0;
            if(strncmp(shm_p->buf_a,"BYE",3) == 0) {
                running =0;
                pthread_cancel(thread_to_cancel2);
            }
        }
        else { 
            offset += 15;
        }

        strncpy(input_string+offset,shm_p->buf_a,BUFFER_SIZE);

        if(shm_p->last_chunk_a) { 
            printf("Process B read:%s\n",input_string);
        }

        // sem up
        if( sem_post(&shm_p->sem_a) == -1)
            error_exit("sem_post");
    }

    return NULL;
}

void *thread_send_function(void *arg) { 
    char *input_string = (char *)malloc(1000 * sizeof(char));
    struct shm_struct *shm_p = (struct shm_struct *)arg;
    int running = 1,new_input=1,offset,i=0,chunks;

    thread_to_cancel2 = pthread_self();

    while(running) { 
        // sem down
        if( sem_wait(&shm_p->sem_c) == -1)
            error_exit("sem_wait");

        // CS
        init_str(shm_p->buf_b);
        if( new_input) { 
            fgets(input_string,1000,stdin);
            shm_p->new_string_received_b = 1;
            offset = 0;
            new_input = 0;
            shm_p->last_chunk_b = 0;
            chunks = strlen(input_string) / BUFFER_SIZE;
            if(strlen(input_string) % BUFFER_SIZE)
                chunks ++;
            i = 0;

            if(strncmp(input_string,"BYE",3) == 0) {
                running = 0;
                pthread_cancel(thread_to_cancel);
            }
            else 
                shm_p->count_b++;
        }
        else  {
            shm_p->new_string_received_b = 0;
            offset += BUFFER_SIZE;
        }
        if( ++i == chunks) { 
            shm_p->last_chunk_b = 1;
            new_input = 1;
        }
        strncpy(shm_p->buf_b,input_string+offset,BUFFER_SIZE);
        printf("sending:%s\n",shm_p->buf_b);

        // sem up
        if( sem_post(&shm_p->sem_d) == -1)
            error_exit("sem_post");

    }
    return NULL;
}