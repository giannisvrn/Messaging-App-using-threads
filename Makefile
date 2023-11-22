all: process-a process-b

process-a: process-a.c 
	gcc -g -Wall -o process-a process-a.c 

process-b: process-b.c 
	gcc -g -Wall -o process-b process-b.c 


clean:
	rm -f process-a process-b