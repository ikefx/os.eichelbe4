/* NEIL EICHELBERGER
 * cs4760 assignment 3
 * master file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHMKEY 859047

int main(int argc, char * argv[]){
	sem_t mutex;
	sem_init(&mutex, 1, 1);

	int inputIndex = 101;
	char * indexStr = NULL;
	pid_t pid;

	for(int i = 0; i < 1; i++){
		if((pid = fork()) == 0){
			indexStr = malloc(sizeof(char)*(int)(inputIndex));
			sprintf(indexStr, "%d", inputIndex);
			printf("conversion %s test\n", indexStr);
			char * args[] = {"./palin", indexStr, '\0'};
			printf("conversion %s test\n", args[1]);
			execvp("./palin", args);	
		} else {
			int returnStatus;
			waitpid(pid, &returnStatus, 0);
			free(indexStr);
			sem_destroy(&mutex);
			return 0;
		}
	}
}
