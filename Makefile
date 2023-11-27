all: process-a process-b

process-a: process-a.c 
	gcc -g -Wall -o process-a process-a.c -lpthread -lrt

process-b: process-b.c 
	gcc -g -Wall -o process-b process-b.c -lpthread -lrt


clean:
	rm -f process-a process-b