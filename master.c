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
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP )

void clearOldOutput();
void cleanExit(char * str);

int main(int argc, char * argv[]){
	
	clearOldOutput();

	sem_t *semaphore = sem_open("/semaphore_example", O_CREAT | O_EXCL, SEM_PERMS, 1);
	if(semaphore == SEM_FAILED){
		perror("sem_open(3) failed in master ln39");
		sem_unlink("/semaphore_example");
		exit(EXIT_FAILURE);
	}

	int inputIndex = 101;
	char * indexStr = NULL;
	pid_t pid;
	
	/* create children */
	for(int i = 0; i < 19; i++){
		if((pid = fork()) == 0){
			indexStr = malloc(sizeof(char)*(int)(inputIndex));
			sprintf(indexStr, "%d", inputIndex);
			char * args[] = {"./palin", indexStr, '\0'};
			execvp("./palin", args);	
		}
	}
	
	/* wait for children then unlink and clear memeory */
	cleanExit(indexStr);
	return 0;
}

void clearOldOutput(){
	/* delete previous output files */
	int status1 = remove("palin.out");
	int status2 = remove("nopalin.out");
	if(status1 == 0){
		printf("Previous %s deleted.\n", "palin.out");
	}
	if(status2 == 0){
		printf("Previous %s deleted.\n", "nopalin.out");
	}
	if(status2 == 0 || status1 == 0){
		printf("\n");
	}
	return;
}
void cleanExit(char * str){
	for(int i = 0; i < 19; i++) wait(NULL);
	sem_unlink("/semaphore_example");
	free(str);
}
