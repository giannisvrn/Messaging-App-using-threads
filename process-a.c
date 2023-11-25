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

    shm_p->count_messages_a = 0;
    shm_p->count_messages_b = 0;
    shm_p->count_chunks_a = 0;
    shm_p->count_chunks_b = 0;
    shm_p->total_time_waiting_a=0;
    shm_p->total_time_waiting_b=0;

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

    float avg_chunks_a = shm_p->count_messages_a > 0 ? (float) shm_p->count_chunks_a / shm_p->count_messages_a : 0;
    float avg_chunks_b = shm_p->count_messages_b > 0 ? (float) shm_p->count_chunks_b / shm_p->count_messages_b : 0;

    printf("Number of messages sent:%d\n",shm_p->count_messages_a);
    printf("Number of messages received:%d\n",shm_p->count_messages_b);

    printf("Number of chunks sent:%d\n",shm_p->count_chunks_a);
    printf("Number of chunks received:%d\n",shm_p->count_chunks_b);

    printf("Average of chunks sent:%.2f\n",avg_chunks_a);
    printf("Average of chunks received:%.2f\n",avg_chunks_b);

    float avg_time_waiting = shm_p->count_messages_b > 0 ? (float) shm_p->total_time_waiting_a / shm_p->count_messages_b : 0;
    printf("Average time waiting to receive first chunk:%.2f\n",avg_time_waiting);
    
    shm_unlink(path);
    exit(EXIT_SUCCESS);
}


void *thread_send_function(void *arg) { 
    char *input_string = (char *)malloc(1000 * sizeof(char));
    struct shm_struct *shm_p = (struct shm_struct *)arg;
    int running = 1,new_input=1,offset,i=0,chunks;

    thread_to_cancel2 = pthread_self();

    while(running) { 
        // sem down
        if( sem_wait(&shm_p->sem_a) == -1)
            error_exit("sem_wait");

        // CS
        init_str(shm_p->buf_a);
        if( new_input) { 
            fgets(input_string,1000,stdin);
            shm_p->new_string_received_a = 1;
            offset = 0;
            new_input = 0;
            shm_p->last_chunk_a = 0;
            chunks = strlen(input_string) / BUFFER_SIZE;
            if(strlen(input_string) % BUFFER_SIZE)
                chunks ++;
            i = 0;

            if(strncmp(input_string,"BYE",3) == 0) {
                running = 0;
                pthread_cancel(thread_to_cancel);
            }
            else {
                shm_p->count_messages_a++;
                shm_p->count_chunks_a++;
            }
        }
        else  {
            shm_p->new_string_received_a = 0;
            offset += BUFFER_SIZE;
            shm_p->count_chunks_a++;
        }
        if( ++i == chunks) { 
            shm_p->last_chunk_a = 1;
            new_input = 1;
        }
        strncpy(shm_p->buf_a,input_string+offset,BUFFER_SIZE);
        printf("sending:%s\n",shm_p->buf_a);

        // sem up
        if( sem_post(&shm_p->sem_b) == -1)
            error_exit("sem_post");

    }
    return NULL;
}

void *thread_receive_function(void *arg) { 
    char *input_string = (char *)malloc(1000 * sizeof(char));
    struct shm_struct *shm_p = (struct shm_struct *)arg;
    int running = 1,offset=0;
    struct timeval temp_time;
    struct timeval temp_time2;

    thread_to_cancel = pthread_self();

    gettimeofday(&temp_time,NULL);
    while(running) {
        // sem down
        if( sem_wait(&shm_p->sem_d) == -1)
            error_exit("sem_wait");

        // CS
        gettimeofday(&temp_time2,NULL);
        shm_p->total_time_waiting_a += temp_time2.tv_sec - temp_time.tv_sec;
        if( shm_p->new_string_received_b) { 
            init_str(input_string);
            offset = 0;
            if(strncmp(shm_p->buf_b,"BYE",3) == 0) {
                running =0;
                pthread_cancel(thread_to_cancel2);
            }
        }
        else { 
            offset += 15;
        }

        strncpy(input_string+offset,shm_p->buf_b,BUFFER_SIZE);

        if(shm_p->last_chunk_b) { 
            printf("Process A read:%s\n",input_string);
            gettimeofday(&temp_time,NULL);
        }

        // sem up
        if( sem_post(&shm_p->sem_c) == -1)
            error_exit("sem_post");
    }

    return NULL;
}