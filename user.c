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

const int CSIZE = 255;

int getnamed(char *name, sem_t **sem, int val);
void writeIsPalin(char * str, int i);
void writeNoPalin(char * str, int i);
void removeSpaces(char * str);
char * getColumnString(char * str, int row, int col);
void setColumnString(char * str, char * newStr, int row, int col);
int getRandomNumber(int low, int high);
char ** splitString(char * str, const char delimiter);
pid_t r_wait(int * stat_loc);
int getLineCount(char * str);
void insertBlockInBlocks(char * str1, char * str2, int index);

int main(int argc, char * argv[]){
	fflush(stdout);
	
	/* convert argv[1] to long */
	char * indexPtr;
	long myProcIndex = strtol(argv[1], &indexPtr, 10);
	
	/* open control block list from shared memory */
	size_t CBLOCKS_SIZE = sizeof(CSIZE) * 19;
	int fd_shm0 = shm_open("CBLOCKS", O_RDWR, 0666);
	void * controlBlocksPtr = mmap(0, CBLOCKS_SIZE, PROT_WRITE, MAP_SHARED, fd_shm0, 0);
	char * controlBlocksDup = strdup((char*)controlBlocksPtr);
	
	/* loop until termination criteria is met */
	while(1){		
		/* open second counter from shared memory */
		size_t SEC_SIZE = sizeof(unsigned long);
		int fd_shm1 = shm_open("SECONDS", O_RDWR, 0666);
		void * secondsPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm1, 0);

		/* open nano counter from shared memory */
		int fd_shm2 = shm_open("NANOS", O_RDWR, 0666);
		void * nanosPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm2, 0);

		//setColumnString(strdup((char*)controlBlocksPtr), strdup((char*)nanosPtr), myProcIndex, 1);
		

		/* get user control block from block table */
//		char ** procTable;
//		procTable = splitString(strdup(controlBlocksDup), '\n');
//		char * myBlock = strdup(procTable[myProcIndex]);
		
		/* check if the priority is 0 queue */
		if(true){
			srand(getpid());
			int roll = getRandomNumber(0, 100);
	
			/* print data to stdout, mostly for development */
			printf("\t\t  Child %3ld:%s\n", myProcIndex, getColumnString(strdup(controlBlocksDup), myProcIndex, 0));
			printf("\t\t----------------------------------------\n");
			printf("\t\t| Child %3ld:%s | Control Block : %s\n", myProcIndex, getColumnString(strdup(controlBlocksDup), myProcIndex, 0), splitString(strdup((char*)controlBlocksPtr),'\n')[myProcIndex]);
			printf("\t\t| Child %3ld:%s | Dice Roll     : %d\n", myProcIndex, getColumnString(strdup(controlBlocksDup), myProcIndex, 0),  roll);
			printf("\t\t----------------------------------------\n");
			printf("test %s\n", getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 3));
			if(roll < 15 && roll > 0){
				/* if child meets termination criteria */
				printf("\t\t\t---> Child %ld:%d has completed!\n", myProcIndex, getpid());
				setColumnString((char*)controlBlocksPtr, "1", myProcIndex, 3);

				shm_unlink("CBLOCKS");
				shm_unlink("SECONDS");
				shm_unlink("NANOS");
			//	printf("\t\t\t\t---> Child completed at %s:%s and started at %s\n", (char*)secondsPtr, (char*)nanosPtr, "0");
				printf("%s", (char*)controlBlocksPtr);
				exit(0);
			}
			if(roll < 15)
				printf("\t\tERROR: Child shouldnt get here\n");
		
			sleep(1);
		}
	}

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

void insertBlockInBlocks(char * str1, char * str2, int index){
	/* replace a string row with str2 in 2d array at index */
	int lineCount = getLineCount(str1);
	char ** tokens = splitString(str1, '\n');
	sprintf(tokens[index], "%s", str2);

	strcpy(str1, "");
	for(int i = 0; i < lineCount; i++){
		strcat(str1, tokens[i]);
	}
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
	printf("This is a test of child setCol\n");
}

int getRandomNumber(int low, int high){
	/* get random number within range */
	srand(time(NULL));
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

