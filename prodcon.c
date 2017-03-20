#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>


#include "sharedMem.h"

pid_t chldpid; /* pid of child process */

/* signal handler */
void rt_handler(int signal, siginfo_t *info, void *arg __attribute__ ((__unused__)))
{
    int val = info->si_value.sival_int;
    //printf("Signal handler is has received a signal\n");
    // unsafe to use fprintf() in a signal handler - but will suffice for demo
    //fprintf(stderr, "Rx signal: SIGRTMIN+%d, value: %d\n", signal - SIGRTMIN, val);
    return;
}

/* send 'value' along with a signal numbered 'signo' */
void send_rt_signal(int signo, int value)
{
    union sigval sivalue;
    sivalue.sival_int = value;

    /* queue the signal */
    if (sigqueue(chldpid, signo, sivalue) < 0) {
        fprintf(stderr, "sigqueue failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return;
}

int main(int argc, char *argv[])
{
    
    // Verify correct user input
    if( argc != 2 ) {
	printf("Usage: %s [file_path]\n", argv[0]);
	return EXIT_SUCCESS;
    }
	
    time_t currentTime;
    struct sigaction action; 
    sigset_t mask;
    struct sharedMem *shmem_ptr;     /* pointer to shared segment */
    int fileRead, fp;
    char buf[MAX_SIZE];		     /* buffer to read from file */
    int shmem_id;       	     /* shared memory identifier */

    key_t key;          /* A key to access shared memory segments */
    int size;           /* Memory size needed, in bytes */
    int flag;           

    key = 4455;         
    size = MAX_SIZE;    
    flag = 1023;        

    /* Create a shared memory segment */
    shmem_id = shmget (key, size, flag);
    if (shmem_id == -1)
    {
        perror ("shmget failed");
        exit (1);
    }

    /* Attach the new segment into address space */
    shmem_ptr = shmat (shmem_id, (void *) NULL, flag);
    if (shmem_ptr == (void *) -1)
    {
        perror ("shmat failed");
        exit (2);
    }

    /* Signal initialization */
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

    if ((chldpid = fork()) > 0) /* this is the parent process */
    {
        sleep(2); /* Sleep to make sure child process is ready */

	/* Open the input file */
 	if ((fp = open(argv[1], O_RDONLY)) < 0) {
		perror("Failed to open file");
		exit(0);		
  	} else {
		/* Read the input file's contents */
		while( (fileRead = read(fp, buf, MAX_SIZE)) > 0){
			/* Copy file contents into shared memory*/
			memcpy(shmem_ptr->buffer, buf, fileRead);
			/* Store the current time */
			shmem_ptr->time = time(NULL);
			/* Signal child process */
			send_rt_signal(SIGRTMIN+1, fileRead);
			fflush(stdout);
			/* Wait for signal */
			sigsuspend(&mask);
			//printf("Producer: Received signal\n");
		}
	printf("Producer: Finished reading file contents\n");

	// Signal EOF to consumer
	send_rt_signal(SIGRTMIN+1, -1);
	}

	/* Wait for child process */
	sleep(5);
	printf("Producer: Waiting for child\n");
	int childStatus;
        pid_t returnValue = waitpid(chldpid, &childStatus, 0);

	/* Report child process exit status */
        if (returnValue > 0)
        {
            if (WIFEXITED(childStatus))
                printf("Child exit Code: %d\n\n", WEXITSTATUS(childStatus));
            else
                printf("Child exit Status: 0x%.4X\n", childStatus);
        }
        else if (returnValue == 0)
            printf("Child process still running\n");
        else
        {
            if (errno == ECHILD)
                printf(" Error ECHILD.\n");
            else if (errno == EINTR)
                printf(" Error EINTR.\n");
            else
                printf("Error EINVAL.\n");
        }
	
	/* Close the file */
        close(fp);

	/* Detach the shared segment and terminate */
        shmdt ( (void *)  shmem_ptr);

	return EXIT_SUCCESS;
    }
    else    /* this is the child process */
    {
        char keystr[10];

        /* Execute the child program in this process, passing it the key
           to shared memory segment as a command-line parameter.
	*/
        sprintf (keystr, "%d", key);
	printf("Executing child program.....NOW!");
        execl ("./Consumer", "child", keystr, NULL);
    }
}


