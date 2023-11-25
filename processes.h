#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>


#define BUFFER_SIZE 15

#define error_exit(error_message) do {perror(error_message); exit(EXIT_FAILURE); } while(0)

void init_str(char *input_string) { 
    int i = 0;
    while(input_string[i] != '\0') { 
        strcpy(&input_string[i],"");
        i++;
    }
}

struct shm_struct{ 
    sem_t sem_a;
    sem_t sem_b;
    sem_t sem_c;
    sem_t sem_d;
    char buf_a[BUFFER_SIZE];
    char buf_b[BUFFER_SIZE];
    int count_messages_a;
    int count_messages_b;
    int count_chunks_a;
    int count_chunks_b;
    int new_string_received_a;
    int new_string_received_b;
    int last_chunk_a;
    int last_chunk_b;
};