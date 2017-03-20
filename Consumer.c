#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "sharedMem.h"

bool complete = false;			
struct sharedMem *shmem_ptr;	/* pointer to shared memory */
time_t totalTime;		/* Total processing time*/
int fdout;			/* file descriptor */ 
char buf[MAX_SIZE];		/* buffer to write to file*/

main(int argc, char *argv[])
{
    void rt_handler(int s, siginfo_t* i , void* arg);
    void send_rt_signal(int signo, int value);

    /* Signal declarations */
    struct sigaction action;
    sigset_t mask;   

    mode_t perms = 0740;	/* file permissions */
    int id;         		/* shared memory identifier */       

    /* File pointer for output file*/
    char* outputFile = (char *) malloc(512*sizeof(char));
    strcpy(outputFile, "output.txt");

    /* Get segment id of the segment that the parent process created */
    id = shmget (atoi(argv[1]), 0, 0);
    if (id == -1)
    {
        perror ("child shmget failed");
        exit(1);
    }

    /* Attach this segment into the address space */
    shmem_ptr = shmat (id, (void *) NULL, 1023);
    if (shmem_ptr == (void *) -1)
    {
        perror ("child shmat failed");
        exit(2);
    }

    /* Add signal handler and set flag to SIGINFO to allow receving interger value */
    action.sa_sigaction = rt_handler;
    action.sa_flags     = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    /* Add SIGRTMIN+1 to signals we will handle */
    if ((sigaction(SIGRTMIN+1, &action, NULL) < 0)){
	fprintf(stderr, "sigaction error: %s\n", strerror(errno));
    }

    /* Create set of signals that will wake up process */
    sigfillset(&mask);
    sigdelset(&mask, SIGRTMIN+1);

    /* Open the output file */
    if ( (fdout = open (outputFile, (O_WRONLY | O_CREAT | O_APPEND ),  perms)) == -1 ) {
    	perror( "Error in creating output file:");
        exit(3);
    }
    
    while(!complete) {
	/* Wait for signal from producer */
        sigsuspend(&mask);
	/* Signal the producer */ 
	send_rt_signal(SIGRTMIN+1, 1);	
    }

    /* Report total processing time*/
    printf("Total time: %ld\n", totalTime);
    fflush(stdout);

    /* Free file pointer memory */
    free(outputFile);

    /* Detach the shared segment and terminate */
    shmdt ( (void *) shmem_ptr);
    

    /* Destroy the shared memory segment and returning it to the system */
    shmctl (id, IPC_RMID, NULL);

    /* Exit Gracefully */
    exit(1);
}

/* Signal handler: stores the number of bytes available in shared memory segment */
void rt_handler(int signal, siginfo_t *info, void *arg __attribute__ ((__unused__)))
{
    int size = info->si_value.sival_int;

    if(size <= 0) { 
	/* Received EOF */
	printf("Consumer: Received EOF\n");
	complete = true;
    } else {
	/* Update total processing time */ 	
        totalTime +=  (time(NULL) - shmem_ptr->time);
	/* Copy shared memory contents into local memory */
	memcpy(buf, shmem_ptr->buffer, size);
	//printf("Consumer: writing to output file: %s \n", buf);
	//printf("Consumer: writing %d bytes to file\n", size);
	fflush(stdout);
       	/* Write the buffer contents to the output file */
       	if ( write (fdout, buf, size) != size ) {
		perror("Error in writing to report:");
	 	exit(3);
	}
    }	
    return;
}

/* send 'value' along with a signal numbered 'signo' */
void send_rt_signal(int signo, int value)
{
    union sigval sivalue;
    sivalue.sival_int = value;

    /* queue the signal that will be sent to parent */
    if (sigqueue(getppid(), signo, sivalue) < 0) {
        fprintf(stderr, "sigqueue failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}






