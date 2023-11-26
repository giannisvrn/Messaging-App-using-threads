#include "processes.h"

void *thread_send_function(void *args);
void *thread_receive_function(void *args);

// ena thread kanei cancel to allo ean o xristis(h to arxeio) exoun steilei to mhnyma BYE
pthread_t thread_to_cancel;
pthread_t thread_to_cancel2;


int main(void) { 
    struct shm_struct *shm_p;
    int fd;
    char *path = "/process";


    // dhmiourgia shared memory segment
    shm_unlink(path); // unlink se periptwsi pou to path einai hdh anoixto
    fd = shm_open(path,O_CREAT | O_EXCL | O_RDWR,0600);
    if( fd == -1) { 
        error_exit("shm_open");
    }

    // thetoume to megethos tou shared memory segment oso kai to struct
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



    // dhmiourgia twn dyo threads
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


    // join ta dyo threads
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
    char *input_string = (char *)malloc(1000 * sizeof(char));   // to megisto input mporei na einai 1000 xaraktires
    struct shm_struct *shm_p = (struct shm_struct *)arg;
    int running = 1,new_input=1,offset,i=0,chunks;

    thread_to_cancel2 = pthread_self();

    while(running) { 
        // sem down
        if( sem_wait(&shm_p->sem_a) == -1)
            error_exit("sem_wait");

        // CS
        init_str(shm_p->buf_a);   // svinoume kathe fora ta periexomena tou buffer 
        if( new_input) {
            fgets(input_string,1000,stdin);
            shm_p->new_string_received_a = 1;
            offset = 0;  // diavazoume to string apo tin arxi 
            new_input = 0;  // tha ksanaginei 1 otan ftasoume sto teleytaio chunk tou mhnymatos 
            shm_p->last_chunk_a = 0;
            chunks = strlen(input_string) / BUFFER_SIZE;
            if(strlen(input_string) % BUFFER_SIZE)  // ean h diairesh exei ypoloipo, tote prepei na prosthesoume akoma ena chunk 
                chunks ++;
            i = 0;  // metavliti gia na gnwrizoume se poio chunk vriskomaste 

            if(strncmp(input_string,"BYE",3) == 0 && input_string[3] == '\n') {  // sthn eisodo apo ton xristi, h fgets vazei meta apo to input thn allagi grammis
                running = 0;
                pthread_cancel(thread_to_cancel);  // cancel to allo thread 
            }
            else {
                shm_p->count_messages_a++;
                shm_p->count_chunks_a++;
            }
        }
        else  {  // otan den exoume kainourio input, diavazoume to epomeno chunk tou mhnymatos
            shm_p->new_string_received_a = 0;
            offset += BUFFER_SIZE;
            shm_p->count_chunks_a++;
        }
        if( ++i == chunks) { 
            shm_p->last_chunk_a = 1;  // vriskomaste sto teleytaio chunk tou mhnymatosn
            new_input = 1;
        }

        strncpy(shm_p->buf_a,input_string+offset,BUFFER_SIZE); // antigrafoume ston buffer tous xaraktires tou chunk sto opoio vriskomaste 
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

    gettimeofday(&temp_time,NULL);   // initialize to temp_time prin apo thn prwth epanalipsi 
    while(running) {
        // sem down
        if( sem_wait(&shm_p->sem_d) == -1)
            error_exit("sem_wait");

        // CS
        gettimeofday(&temp_time2,NULL);  // temp_time2 = h xronikh stigmi opou erxetai mhnyma apo to allo process 
        shm_p->total_time_waiting_a += temp_time2.tv_sec - temp_time.tv_sec;  // prosthetoume ston synoliko xrono thn diafora 
        if( shm_p->new_string_received_b) { 
            init_str(input_string);  // efoson exoume kainourio mhnyma, diagrafoume ta periexomena pou yphrxan apo prin
            offset = 0;
            if(strncmp(shm_p->buf_b,"BYE",3) == 0 && shm_p->buf_b[3] == '\n') {
                running =0;
                pthread_cancel(thread_to_cancel2);
            }
        }
        else { 
            offset += BUFFER_SIZE;
        }

        strncpy(input_string+offset,shm_p->buf_b,BUFFER_SIZE);

        if(shm_p->last_chunk_b) { 
            printf("Process A read:%s\n",input_string);
            gettimeofday(&temp_time,NULL);   // otan teleiwnoume me to teleytaio chunk tou mhnymatos, ksekinaei h metrhsh gia to pote tha erthei kainourio mhnyma 
        }

        // sem up
        if( sem_post(&shm_p->sem_c) == -1)
            error_exit("sem_post");
    }

    return NULL;
}