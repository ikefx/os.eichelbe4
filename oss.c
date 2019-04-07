/* NEIL EICHELBERGER
 * cs4760 assignment 4
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
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

const int CSIZE = 255;

int getnamed(char *name, sem_t **sem, int val);
void longToString(unsigned long num, char * out);
char ** splitString(char * str, const char delimiter);
void clearOldOutput();
void cleanExit(char * str, int childCount);
void sigintHandler(int sig_num);
void printOptions();
unsigned long getRandomNumber(unsigned long low, unsigned long high);
bool allJobsFinished(char * str);
char * getColumnString(char * str, int row, int col);
void setColumnString(char * str, char * newStr, int row, int col);
int getLineCount(char * str);

int main(int argc, char * argv[]){

	printf("\t\t-->>> OSS Start <<<--\n\t\t (Parent ID is %d)\n", getpid());
	shm_unlink("CBLOCKS");
	/* allocate a control block list to shared memory */
	size_t CBLOCKS_SIZE = sizeof(CSIZE) * 19;
	int fd_shm0 = shm_open("CBLOCKS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm0, CBLOCKS_SIZE );
	void * controlBlocksPtr = mmap(0, CBLOCKS_SIZE, PROT_WRITE, MAP_SHARED, fd_shm0, 0);

	/* allocate a second counter to shared memory */
	size_t SEC_SIZE = sizeof(unsigned long);
	int fd_shm1 = shm_open("SECONDS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm1, SEC_SIZE );
	unsigned long * secondsPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	*secondsPtr = 0;

	/* allocate a nano second counter to shared memory */
	int fd_shm2 = shm_open("NANOS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm2, SEC_SIZE );
	unsigned long * nanosPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm2, 0);
	*nanosPtr = 0;
	
	/* Declare loop variables */
	unsigned long randNum = 0;
	int procCount = 0;
	pid_t pid = NULL;
	int procStates[18];
	signal(SIGINT, sigintHandler);
	
	while(1){

		/* create random number */
		srand(time(NULL));
		randNum = getRandomNumber(0,100);

		printf("------------------------------------\n");
		printf("Seconds:%15lu\nNanos:%17lu\nRandom: %15lu\n", *secondsPtr, *nanosPtr, randNum);
		printf("\tAll Control Blocks:\n%s", (char*)controlBlocksPtr);
		printf("\n------------------------------------\n");

		/* create a child goes here, requires condition */
		if(procCount < 1 || *secondsPtr == 8){
			/* add new process to states array */
			procStates[procCount] = 0;

			/* Establish an index for next child */
			procCount++;
			printf("\t Creating a new child! Total: %d\n", procCount);
	
			/* Create the child */
			if((pid = fork()) == 0){	
			
				int childIndex = procCount - 1;
				char cIndex[32];
				sprintf(cIndex, "%d", childIndex);
				printf("Child is index:pid => %d:%d \n", childIndex, getpid());	
				/* control block singleton */
				char controlBlock[CSIZE];
				sprintf(controlBlock, "%d|%lu|0|0\n", getpid(), *nanosPtr);
				
				/* add control block to shared list */
				strcat((char*)controlBlocksPtr, controlBlock);	
	
				/* set up command line args */
				char * args[] = {"./user", cIndex, '\0'};
				execvp("./user", args);
			}
		}	
		
		/* Increment clock and get new random value */
		* secondsPtr += 1;
		* nanosPtr += 1e9;	

		sleep(1);

		/* round robin */
//		for(int i = 0; i < 18; i++){
//			/* flag this process for action */
//			printf("%d ", procStates[i]);
//			if(procStates[i] == 0){
//				setColumnString(strdup((char*)controlBlocksPtr), "0", i, 2);				
//				break;
//			}
//		}
//		if(allJobsFinished(strdup((char*)controlBlocksPtr))){
//			/* if all jobs are finished, exit */
//			printf("All jobs complete.. exiting\n");
//			break;
//		}
		if(*secondsPtr > 100)
			/* if second counter hits 100, exit */
			break;
	}

	kill(0,SIGTERM);
	shm_unlink("CBLOCKS");
	shm_unlink("SECONDS");
	shm_unlink("NANOS");
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

void longToString(unsigned long num, char * out){
	/* convert number to string */
	const int n = snprintf(NULL, 0, "%lu", num);
	assert(n > 0);
	char buf[n+1];
	int c = snprintf(buf, n+1, "%lu", num);
	assert(buf[n] == '\0');
	assert(c == n);
	strcpy(out, buf);
	return;
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

void clearOldOutput(){
	/* delete previous output files */
	int status1 = remove("palin.out");
	int status2 = remove("nopalin.out");
	if(status1 == 0){
		printf("--> Previous %s deleted.\n", "palin.out");
	}
	if(status2 == 0){
		printf("--> Previous %s deleted.\n", "nopalin.out");
	}
	if(status2 == 0 || status1 == 0){
		printf("\n");
	}
	return;
}

void cleanExit(char * str, int childC){
	/* wait for children, if 100 seconds accures terminate premature */
	time_t start, stop;
	start = time(NULL);
	for(int i = 0; i < 19; i++){
		stop = time(NULL);	
		if(stop - start > 100){		
			printf("--> 100 seconds elapsed, terminating all processes.\n");
			kill(0, SIGTERM);
			free(str);
			shm_unlink("STRINGS");
			sem_unlink("/SEMA");
		}
		wait(NULL);
	}
}

void sigintHandler(int sig_num){
	/* ctrl-c kill */
	signal(SIGINT, sigintHandler);
	printf("\nTerminating all...\n");
	shm_unlink("CBLOCKS");
	shm_unlink("SECONDS");
	shm_unlink("NANOS");
	shm_unlink("/SEMA");
	exit(0);
}

void printOptions(){
	/* print command line arguments for user reference */
	printf("\n========== Command-Line Options ==========\n\n> Optional: -h (view command-line options)\n");
	printf("> Optional: -i (specify input name, default input.txt)\n");
	fflush(stdout);
	exit(0);
}

unsigned long getRandomNumber(unsigned long low, unsigned long high){
	/* get random number within range */
	unsigned long num;
	for ( int i = 0; i < 1; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

bool allJobsFinished(char * str){
	/* parse all complete columns from process table
 	 * if there are no incomplete jobs, return true */
	char ** tokens = splitString(strdup(str), '\n');
	int lines = getLineCount(str);
	for(int i = 0; i < lines; i++){
		char * lastCol = splitString(strdup(tokens[i]), '|')[3];
		if(strcmp(strdup(lastCol), "0") == 0)
			return false;
	}
	return true;
}

char * getColumnString(char * str, int row, int col){
	/* returns the value at (row,col) as a string */
	return strdup(splitString(strdup(splitString(strdup(str), '\n')[row]), '|')[col]); 
}

void setColumnString(char * str, char * newStr, int row, int col){
	/* set a string to (row,col), replacing existing value */
	char ** tokens = splitString(strdup(str), '\n');
	char * tempRow = tokens[row];
	char ** rowValues = splitString(strdup(tempRow), '|');
	strcpy(rowValues[col], newStr);
	char newRow[CSIZE];
	strcat(newRow, rowValues[0]);
	strcat(newRow, "|");
	strcat(newRow, rowValues[1]);
	strcat(newRow, "|");
	strcat(newRow, rowValues[2]);
	strcat(newRow, "|");
	strcat(newRow, rowValues[3]);
	strcat(newRow, "\n");
	strcpy(tokens[row], newRow);
	
	for(int i = 0; i < getLineCount(strdup(str)); i++){
		strcpy(str, tokens[0]);
	}
//	printf("SET NEW RESULT:\nnew string: %s\nBLocks:\n%s\n", newStr, str);
}

int getLineCount(char * str){
	/* get number of lines in the file */
	int count = 0;
	for(int i = 0; i < strlen(str); i++){
		if(str[i] == '\n'){
			count++;
		}
	}
	return count;
}
