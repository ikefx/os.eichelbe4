/* NEIL EICHELBERGER
 * cs4760 assignment 4
 * user file */

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
#define CSIZE 128

size_t CBLOCKS_SIZE = sizeof(CSIZE) * 18;
size_t SEC_SIZE = sizeof(unsigned long);

char * getColumnString(char * str, int row, int col);
void setColumnString(char * str, char * newStr, int row, int col);
int getRandomNumber(int low, int high);
char ** splitString(char * str, const char delimiter);
int getRowCount(char * str);
int getColumnCount(char * str);

int main(int argc, char * argv[]){
	/* convert argv[1] to long */
	char * indexPtr;
	long myProcIndex = strtol(argv[1], &indexPtr, 10);	

	/* convert argv[2] to long */
	char * startStr;
	long startTime = strtol(argv[2], &startStr, 10);

	/* open second counter from shared memory */
	int fd_shm1 = shm_open("SECONDS", O_RDWR, 0666);
	ftruncate(fd_shm1, SEC_SIZE);
	unsigned long * secondsPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm1, 0);
	
	/* open nano counter from shared memory */
	int fd_shm2 = shm_open("NANOS", O_RDWR, 0666);
	ftruncate(fd_shm2, SEC_SIZE);
	unsigned long * nanosPtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm2, 0);

	/* open nano counter from shared memory */
	int fd_shm3 = shm_open("IDLE", O_RDWR, 0666);
	ftruncate(fd_shm3, SEC_SIZE);
	unsigned long * idlePtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm3, 0);
	
	/* open control block list from shared memory */
	int fd_shm0 = shm_open("CBLOCKS", O_RDWR, 0666);
	ftruncate(fd_shm0, CBLOCKS_SIZE);
	void * controlBlocksPtr = mmap(0, CBLOCKS_SIZE, PROT_WRITE, MAP_SHARED, fd_shm0, 0);

	srand(getpid());

	/* loop until termination criteria is met */
	while(1){	

		/* if process priority is 0  */
		if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 2), "0") == 0){
			* secondsPtr += 1;
			* nanosPtr += 1e9;
			/* if process is complete */
			if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 3), "1") == 0){
				*idlePtr += 1e9;
				printf("\t\t\tCPU is idle (child process completed before quantum expired)...\n");
			}
			/* if process is incomplete */
			else if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 3), "0") == 0){
				int roll = getRandomNumber(0, 100);
				/* print data to stdout, mostly for development */
				printf("\t\t  Child %3ld:%s is active! at %lu : %lu\n", myProcIndex, getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 0), *secondsPtr, *nanosPtr);
				printf("\t\t----------------------------------------\n");
				printf("\t\t| Child %3ld:%s | Control Block : %s\n", myProcIndex, getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 0), splitString(strdup((char*)controlBlocksPtr),'\n')[myProcIndex]);
				printf("\t\t| Child %3ld:%s | Dice Roll     : %d\n", myProcIndex, getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 0), roll);
				printf("\t\t----------------------------------------\n");
				if(roll < 15){
					/* if child meets termination criteria */
					printf("\t\t\t---> Child %ld:%d has completed!\n", myProcIndex, getpid());
					setColumnString((char*)controlBlocksPtr, "1", myProcIndex, 3);
					printf("\t\t\t\t---> Child completed at %lu:%lu and started at %ld\n", *secondsPtr, *nanosPtr, startTime);
					char buff[128];
					snprintf(buff, sizeof buff, "%lu", *nanosPtr);
					setColumnString((char*)controlBlocksPtr, buff, myProcIndex, 5);
				}
			}
			usleep(150000);

		/* if priority is not 0 (ie child is locked) */
		}if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 2), "0") != 0){
			/* if child is complete, terminate */
			if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), myProcIndex, 3), "1") == 0){
				exit(0);
			}
			else {
				usleep(150000);
			}
		}
	}
	/* should not reach here */
	exit(0);
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
//		assert(idx == count - 1);
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
	char * tempRow = strdup(tokens[row]);
	char ** tempValues = splitString(strdup(tempRow),'|');
	strcpy(tempValues[col], newStr);
	char * strCpy = strdup(str);
	strcpy(str, "");	

	/* assemble all rows */
	for(int i = 0; i < getRowCount(strdup(strCpy)); i++){
		if(i == row){
			/* assemble the new row */
			char newRow[CSIZE];
			strcpy(newRow, "");
			for(int j = 0; j <= getColumnCount(strdup(tempRow)); j++){

				if(j < getColumnCount(strdup(tempRow))){
					/* if not the last column */
					if(j == col){
						strcat(newRow, newStr);
						strcat(newRow, "|");
					} else {
						strcat(newRow, splitString(strdup(tempRow),'|')[j]);
						strcat(newRow, "|");
					}
				}else if(j == getColumnCount(strdup(tempRow))){
					/* is the last column */
					if(j == col){
						strcat(newRow, newStr);
					} else {
						strcat(newRow, splitString(strdup(tempRow),'|')[j]);
					}
				}
			}
			strcat(str, newRow);
		}else{
			char * tempRows = splitString(strdup(strCpy),'\n')[i];
			strcat(str, tempRows);
		}
		strcat(str, "\n");
	}
}

int getRandomNumber(int low, int high){
	/* get random number within range */
	int num;
	for ( int i = 0; i < 2; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

int getRowCount(char * str){
	/* get number of lines in the file */
	int count = 0;
	for(int i = 0; i < strlen(str); i++){
		if(str[i] == '\n'){
			count++;
		}
	}
	return count;
}

int getColumnCount(char * str){
	/* get number of columns in the file */
	int count = 0;
	for(int i = 0; i < strlen(str); i++){
		if(str[i] == '|'){
			count++;
		}
	}
	return count;
}

