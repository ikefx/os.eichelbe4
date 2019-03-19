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
#include <errno.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdbool.h>

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getnamed(char *name, sem_t **sem, int val);
bool isPalindrome(char * str);
void writeIsPalin(char * str);
void writeNoPalin(char * str);
void removeSpaces(char * str);

int main(int argc, char * argv[]){

	/* if Either argv was empty or not supplied */
	if(argc <= 1){
		return 0;
	} else if(argv[1][0] == '\0'){
		return 0;
	}

	/* load semaphore */
	sem_t *semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	int num;
	sem_getvalue(semaphore, &num);
	printf("--> %d\n", num);
	/* load strings from shared posix memory */
	int fd_shm;
	fd_shm = shm_open("STRINGS", O_RDONLY, 0666);
	void ** sharedPtr = mmap(NULL, sizeof(char[256][256]), PROT_READ, MAP_SHARED, fd_shm, 0);

	/* convert argv1 to long */
	char * indexPtr;
	long indexLong = strtol(argv[1], &indexPtr, 10);

	printf("\tPID:%d| %s %s", getpid(), argv[1], (isPalindrome(argv[1])) ? "is a palindrome\n" : "is not palindrome\n");
	isPalindrome(argv[1]) ? writeIsPalin(argv[1]) : writeNoPalin(argv[1]);	
	
	shm_unlink("STRINGS");

	if(sem_post(semaphore) < 0){
		perror("sem_post() error in child");
	}


	sem_getvalue(semaphore, &num);
	printf("-->%d\n", num);
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}
	shm_unlink("/SEMA");	

	return 0;
}

int getnamed(char *name, sem_t **sem, int val){
	/* a function to access a named seamphore, creating it if it dosn't already exist */
	while(((*sem = sem_open(name, FLAGS , PERMS , val)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED)
		return 0;
	if(errno != EEXIST)
		return -1;
	while(((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED)
		return 0;
	return -1;	
}

bool isPalindrome(char * str){
	
	char * strDup = strdup(str);
	removeSpaces(strDup);

	int low = 0;
	int high = strlen(strDup) - 1;
	while( high > low ){
		if(strDup[low++] != strDup[high--]){
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

void removeSpaces(char * str){
	/* remove white spaces from string */
	int count = 0;
	for(int i = 0; str[i]; i++){
		if(str[i] != ' ')
			str[count++] = str[i];
	}
	str[count] = '\0';
}
