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
#include <sys/wait.h>
#include <stdbool.h>

#define SHMKEY 859047

#define FLAGS (O_RDONLY)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getnamed(char *name, sem_t **sem, int val);
bool isPalindrome(char * str);
void writeIsPalin(char * str, int i);
void writeNoPalin(char * str, int i);
void removeSpaces(char * str);
pid_t r_wait(int * stat_loc);

int main(int argc, char * argv[]){
	fflush(stdout);
//	printf("%s\n", argv[2]);
//	fflush(stdout);
	/* if Either argv was empty or not supplied */
	if(argc <= 1){
		return 0;
	} else if(argv[1][0] == '\0'){
		return 0;
	}

//	char * lenPtr;
//	long len = strtol(argv[2], &lenPtr, 10);
//
//	char * indexPtr2;
//	long index = strtol(argv[3], &indexPtr2, 10);

//	int num;
	/* load semaphore */
	sem_t *semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
//	int num;
//	printf("\tSEM in child begin: %d\n", sem_getvalue(semaphore, &num));
//	int shmid = shmget(SHMKEY, 4*sizeof(char), IPC_CREAT | 0666);
//	if(shmid == -1){
///		fprintf(stderr, "Error in shmget\n");
//		exit(1);
//	}
//	char * buffer = (char*)(shmat(shmid, 0,0));
//	char * shBuff = (char*)(buffer);

//	printf("TEST:%s\n", buffer);


//	sem_getvalue(semaphore, &num);
//	printf("SEMA in child: %d\n", sem_getvalue(semaphore, &num));

//	while(sem_wait(semaphore) == -1)
//		if(errno != EINTR){
//			perror("Failed to lock semaphore");
//			return 1;
//		}

	/* load strings from shared posix memory */
//	int fd_shm;
//	fd_shm = shm_open("STRINGS", O_RDONLY, 0666);
//	void ** sharedPtr = mmap(NULL, sizeof(char[12][256]), PROT_READ, MAP_SHARED, fd_shm, 0);

//	printf("%s\n", (char*)sharedPtr[0]);

//	int fd_shm;
//	fd_shm = shm_open("STRINGS", O_RDONLY, 0666);
//	void ** shmPtr = mmap(0, sizeof(char[255]), PROT_READ, MAP_SHARED, fd_shm, 0);
//	char * new = strdup((char*)shmPtr[0]);

//	printf("THIS IS A TEST test: %s\n", (char*)sharedPtr[0]);

	/* convert argv1 to long */
//	char * indexPtr;
//	long indexLong = strtol(argv[1], &indexPtr, 10);

	/* convert arg3 to long */
	char * iPtr;
	long index = strtol(argv[2], &iPtr, 10);

	printf("\tPID:%d | \"%s\"\t%ld\n", getpid(), argv[1], index);
	isPalindrome(argv[1]) ? writeIsPalin(argv[1], index) : writeNoPalin(argv[1], index);	
	
	while(sem_post(semaphore) == -1){
		perror("failed to unlock semlock");
		return 1;
	}

//	printf("\tSEM in child after post: %d\n", sem_getvalue(semaphore, &num));
	if(r_wait(NULL) == -1)
		return 1;

	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}
//	shm_unlink("/SEMA");	
	exit(0);
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

void writeIsPalin(char * str, int i){
	/* write string to palin.out */
	FILE *fp;
	fp = fopen("palin.out", "a");
	char wroteLine[355];
	sprintf(wroteLine, "%d %d \"%s\"\n", getpid(), i, str);
	fprintf(fp, wroteLine);
	fclose(fp);
	return;
}

void writeNoPalin(char * str, int i){
	/* write string to nopalin.out */
	FILE *fp;
	fp = fopen("nopalin.out", "a");
	char wroteLine[355];	
	sprintf(wroteLine, "%d %d \"%s\"\n", getpid(), i, str);
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

pid_t r_wait(int * stat_loc){
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
