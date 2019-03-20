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
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>

#define FLAGS (O_RDWR | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int getnamed(char *name, sem_t **sem, int val);
bool isPalindrome(char * str);
void writeIsPalin(char * str, int i);
void writeNoPalin(char * str, int i);
void removeSpaces(char * str);
int getRandomNumber(int low, int high);
char ** splitString(char * str, const char delimiter);
pid_t r_wait(int * stat_loc);

int main(int argc, char * argv[]){
	fflush(stdout);
	srand(getpid());

	/* if Either argv was empty or not supplied */
	if(argc <= 1){
		return 0;
	} else if(argv[1][0] == '\0'){
		return 0;
	}
	
	/* convert arg[3] to a long */
	char * sizePtr;
	long dataSize = strtoul(argv[3], &sizePtr, 10);

	/* convert arg[2] to long */
	char * iPtr;
	long index = strtol(argv[2], &iPtr, 10);

	/* read in strings from shared memory */
	int shm_fd = shm_open("STRINGS", O_RDONLY, 0666);
	void * cdata = mmap(0, dataSize, PROT_READ, MAP_SHARED, shm_fd, 0);
	char * cdataDup = strdup(cdata);
	char ** tokens;

	/* process is sleeping for rand() 6 times */
	for( int i = 0; i < 5; i++ ){
		sleep(getRandomNumber(0,10));
	}

	tokens = splitString(cdataDup, '\n');
	
	/* load semaphore */
	sem_t *semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	
	/* Do palindrome check and output result to file & stdout */
	printf("\tPID:%d%38s  \t%ld\t%s\n", getpid(), tokens[index], index, (isPalindrome(tokens[index]) ? "Palin? Yes" : "Palin? No"));
	isPalindrome(tokens[index]) ? writeIsPalin(tokens[index], index) : writeNoPalin(tokens[index], index);	
	
	while(sem_post(semaphore) == -1){
		perror("failed to unlock semlock");
		return 1;
	}

	if(r_wait(NULL) == -1)
		return 1;

	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");
	}
	shm_unlink("STRINGS");
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
	/* check if string is a palindrome, ignore white spaces */
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
	sprintf(wroteLine, "\t%d\t%d%38s\n", getpid(), i, str);
	fprintf(fp, wroteLine);
	fclose(fp);
	return;
}

void writeNoPalin(char * str, int i){
	/* write string to nopalin.out */
	FILE *fp;
	fp = fopen("nopalin.out", "a");
	char wroteLine[355];	
	sprintf(wroteLine, "\t%d\t%d%38s\n", getpid(), i, str);
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

char ** splitString(char * str, const char delimiter){
	/* generate 2d array of strings from a string, delimited by parameter */
	char ** result = 0;
	size_t count = 0;
	char * tmp = str;
	char * last = 0;
	char delim[2];
	delim[0] = delimiter;
	delim[1] = 0;
	while(*tmp){
		if(delimiter == *tmp){
			count++;
			last = tmp;
		}
		tmp++;
	}
	count += last < (str + strlen(str) - 1);
	count++;
	result = malloc(sizeof(char*) * count);	
	if(result){
		size_t idx = 0;
		char * token = strtok(str, delim);
		while(token){
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count - 1);
		*(result + idx) = 0;
	}
	return result;
}

int getRandomNumber(int low, int high){
	/* get random number within range */
	int num;
	for ( int i = 0; i < 2; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

pid_t r_wait(int * stat_loc){
	/* a function that restarts wait if interrupted by a signal */
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
