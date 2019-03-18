/* NEIL EICHELBERGER
 * cs4760 assignment 3
 * palindrome file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>

bool isPalindrome(char * str);
void writeIsPalin(char * str);
void writeNoPalin(char * str);


int main(int argc, char * argv[]){

	sem_t *semaphore = sem_open("/semaphore_example", O_RDWR);
	if(semaphore == SEM_FAILED){
		perror("sem_open(3) failed in child ln28");
		exit(EXIT_FAILURE);
	}

	if(sem_wait(semaphore) < 0){
		perror("sem_wait() failed in child");
	}

	/* if Either argv was empty or not supplied */
	if(argc <= 1){
		return 0;
	} else if(argv[1][0] == '\0'){
		return 0;
	}
	printf("\tPID:%d| %s %s", getpid(), argv[1], (isPalindrome(argv[1])) ? "is a palindrome\n" : "is not palindrome\n");
	isPalindrome(argv[1]) ? writeIsPalin(argv[1]) : writeNoPalin(argv[1]);
	
	if(sem_post(semaphore) < 0){
		perror("sem_post() error in child");
	}
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}
	return 0;
}

bool isPalindrome(char * str){
	int low = 0;
	int high = strlen(str) - 1;
	while( high > low ){
		if(str[low++] != str[high--]){
			return false;
		}
	}
	return true;
}

void writeIsPalin(char * str){
	/* write string to palin.out */
	FILE *fp;
	fp = fopen("palin.out", "a");
	char wroteLine[355];
	sprintf(wroteLine, "%s\n", str);
	fprintf(fp, wroteLine);
	fclose(fp);
	return;
}

void writeNoPalin(char * str){
	/* write string to nopalin.out */
	FILE *fp;
	fp = fopen("nopalin.out", "a");
	char wroteLine[355];
	sprintf(wroteLine, "%s\n", str);
	fprintf(fp, wroteLine);
	fclose(fp);
	return;
}
