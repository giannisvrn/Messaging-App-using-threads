#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>


#define BUFFER_SIZE 15

#define error_exit(error_message) do {perror(error_message); exit(EXIT_FAILURE); } while(0)

// allazoume ta periexomena enos string se ""
void init_str(char *input_string) { 
    int i = 0;
    while(input_string[i] != '\0') { 
        strcpy(&input_string[i],"");
        i++;
    }
}

struct shm_struct{ 
    sem_t sem_a;   // semaphore gia read tou process A apo ton xristi(h arxeio)
    sem_t sem_b;   // sempaphore gia receive tou process B apo to process A
    sem_t sem_c;   // semaphore gia read tou process B apo ton xristi(h arxeio)
    sem_t sem_d;   // sempahore gia receive tou process A apo to process B
    char buf_a[BUFFER_SIZE];   // buffer A->B
    char buf_b[BUFFER_SIZE];   // buffer B->A
    int count_messages_a;      // arithmos mhnymatwn A->B 
    int count_messages_b;      // arithmos mhnymatwn B->A
    int count_chunks_a;
    int count_chunks_b;
    int total_time_waiting_a;   // synolikos xronos anamonhs gia paralavi tou prwtou temaxiou gia to process A
    int total_time_waiting_b;
    int new_string_received_a;   // 1 ean to process A stelnei kainourio mhnyma sto process B, 0 ean stelnei epomeno chunk apo to idio mhnyma
    int new_string_received_b;
    int last_chunk_a;   // 1 ean to chunk pou stelnetai apo to process A einai to teleytaio toy mhnymatos, 0 diaforetika
    int last_chunk_b;
};