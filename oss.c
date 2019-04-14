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


int getIndexOfUnlockedState(int * stateArray, int size);
bool allProcessLocked(int * stateArray, int size);
void setPriority0(char * str, int index);
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
int getColumnCount(char * str);
int getLineCount(char * str);
void writeRow(char * filename, char * str);

int main(int argc, char * argv[]){
	char * outfile = "log.txt";

	printf("\t\t-->>> OSS Start <<<--\n\t\t (Parent ID is %d)\n", getpid());
	writeRow(outfile, "PROGRAM START:\n\n");
	shm_unlink("CBLOCKS");

	/* delete old log file if it exists */
	if(remove(outfile) == 0)
		printf("\t\tPrevious %s deleted.\n", outfile);

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
	
	/* allocate a nano second counter to shared memory */
	int fd_shm3 = shm_open("IDLE", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm3, SEC_SIZE );
	unsigned long * idlePtr = mmap(0, SEC_SIZE, PROT_WRITE, MAP_SHARED, fd_shm3, 0);
	*idlePtr = 0;
	
	/* Declare loop variables */
	int randNum = 0;
	int procCount = 0;
	pid_t pid = NULL;
	int procStates[18]; // 0 = locked, > 0 = active, -1 = complete
	int roundRobinIncrementer;

	signal(SIGINT, sigintHandler);
	
	while(1){
	
		/* set round-robin index selection */
		roundRobinIncrementer = (procCount > 0) ? ((roundRobinIncrementer + 1) % procCount) : 0;

		/* create random number */
		srand(getpid()^time(NULL));
		randNum = getRandomNumber(0,100);

		printf("------------------------------------\n");
		printf("Seconds:%15lu\nNanos:%17lu\nRandom: %15d\n", *secondsPtr, *nanosPtr, randNum);
		printf("Total Idle Time: %6.0lf:%ld\n", *idlePtr/1e9, *idlePtr);
		printf("\tAll Control Blocks:\n%s", (char*)controlBlocksPtr);
		printf("\n------------------------------------\n");

		/* create a child goes here, requires random condition */
		if(randNum <= 30 || *secondsPtr == 1){
			/* add new process to states array */
			procStates[procCount] = -0;

			/* Establish an index for next child */
			procCount++;
			printf("\t Creating a new child! Total: %d\n", procCount);
	
			/* Create the child */
			if((pid = fork()) == 0){	
				/* convert index to string for arg */	
				int childIndex = procCount - 1;
				char cIndex[32];
				char cbuf[256];
				sprintf(cIndex, "%d", childIndex);
				printf("\t\t Child is index:pid ---> %d:%d \n", childIndex, getpid());	
				snprintf(cbuf, sizeof cbuf, "Child %d:%d created at %lu\n", childIndex, getpid(), *nanosPtr);
				writeRow(outfile, cbuf);
				/* convert time to string for arg */
				char startStr[128];	
				sprintf(startStr, "%lu", *nanosPtr);

				/* control block singleton */
				char controlBlock[CSIZE];
				sprintf(controlBlock, "%d|%lu|1|0|0|-1\n", getpid(), *nanosPtr);
				
				/* add control block to shared list */
				strcat((char*)controlBlocksPtr, controlBlock);	
	
				/* set up command line args */
				char * args[] = {"./user", cIndex, startStr, '\0'};
				execvp("./user", args);
			}
		}	

		char buf[256];
		/* Round Robin Tournament */
		

		/* if all process locked or complete set a process to unlocked */
		if(allProcessLocked(procStates, procCount)){
			if(procStates[roundRobinIncrementer] == 0)
				procStates[roundRobinIncrementer] = 1;
		}else{
			int activeIndex = getIndexOfUnlockedState(procStates, procCount);
			procStates[activeIndex] = activeIndex;

			/* if process is in queue 1, set to zero and execute */
			if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 2), "1") == 0){
				
				/* add one to the process quantum count */
				char * strNum = getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 4);
				char * tempPtr;
				long quantumC = strtol(strNum, &tempPtr, 10);
				quantumC++;
				char quantumStr[128];
				sprintf(quantumStr, "%ld", quantumC);
				setColumnString((char*)controlBlocksPtr, quantumStr, activeIndex, 4);

				/* set priority to 0 */
				setColumnString((char*)controlBlocksPtr, "0", activeIndex, 2);
				snprintf(buf, sizeof buf, "\t--> Process %d:%s moved from Queue 1 to Queue 0 and given a quantum at %lu.", activeIndex, getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 0), *nanosPtr);
				writeRow(outfile, buf);
	
				/* QUANTUM */
				sleep(4);

				/* if the process terminated during parent sleep() */
				if( strcmp(getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 3), "1") == 0 	){
					procStates[activeIndex] = -1; // set state to -1
					setColumnString((char*)controlBlocksPtr, "-1", activeIndex, 2);
					snprintf(buf, sizeof buf, "Child %d:%s completed at %lu. It took %s quantums for this process to complete.\n", 
						activeIndex, 
						getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 0), 
						*nanosPtr,
						getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 4)
 					);
					writeRow(outfile, buf);
				} else {
					/* move process to queue 3 and change state to locked */
					setColumnString((char*)controlBlocksPtr, "3", activeIndex, 2);
					procStates[activeIndex] = 0;					
					snprintf(buf, sizeof buf, "\t--> Process %d:%s did not terminate during their quantum. Moving Process from Queue 0 to Queue 3.", activeIndex, getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 0));
					writeRow(outfile, buf);
				}
			}
			/* if process is in queue 2 promote to queue 1 */
			else if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 2), "2") == 0){
				setColumnString((char*)controlBlocksPtr, "1", activeIndex, 2);
				snprintf(buf, sizeof buf, "\t--> Process %d:%s has been moved from Queue 2 to Queue 1.", activeIndex, getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 0));
				writeRow(outfile, buf);
			}
			/* if process is in queue 3 promote to queue 2 */
			else if(strcmp(getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 2), "3") == 0){
				setColumnString((char*)controlBlocksPtr, "2", activeIndex, 2);
				snprintf(buf, sizeof buf, "\t--> Process %d:%s has been moved from Queue 3 to Queue 2.", activeIndex, getColumnString(strdup((char*)controlBlocksPtr), activeIndex, 0));
				writeRow(outfile, buf);
			}

		}

		
		/* Increment clock and get new random value */
		* secondsPtr += 1;
		* nanosPtr += 1e9;	

		writeRow(outfile, "...");
		sleep(1);

		if(*secondsPtr > 30){
			/* if second counter hits var, exit */
			printf("------------------------------------\n");
			printf("Seconds:%15lu\nNanos:%17lu\nRandom: %15d\n", *secondsPtr, *nanosPtr, randNum);
			printf("Total Idle Time: %6.0lf:%ld\n", *idlePtr/1e9, *idlePtr);
			printf("\tAll Control Blocks:\n%s", (char*)controlBlocksPtr);
			printf("\n------------------------------------\n");
			printf("\t--> Maximum duration reached, exiting program.\n");
			break;
		}
	}
	char finbuf[256];
	snprintf(finbuf, sizeof finbuf, "\nProgram terminated at %lu : %lu (seconds : nanoseconds)\n", *secondsPtr, *nanosPtr);
	writeRow(outfile, finbuf);
	snprintf(finbuf, sizeof finbuf, "The total amount of time the CPU was idle is %.0lf : %lu (seconds : nanoseconds)\n", *idlePtr/1e9, *idlePtr);
	writeRow(outfile, finbuf);

	kill(0,SIGTERM);
	shm_unlink("CBLOCKS");
	shm_unlink("SECONDS");
	shm_unlink("NANOS");
	shm_unlink("IDLE");
	return 0;
}

int getIndexOfUnlockedState(int * stateArray, int size){
	/* return index of element that is not locked */
	for(int i = 0; i < size; i++){
		if(stateArray[i] > 0){
			return i;
		}
	}
	return -1; //should not reach
}

bool allProcessLocked(int * stateArray, int size){
	bool allLocked = true;
	printf("\t---- Process States ----\n\t");
	for(int i = 0; i < size; i++){
		printf("%d ", stateArray[i]);
		if(stateArray[i] > 0){
			allLocked = false;
		}
	}
	printf("\n\t------------------------\n");
	return allLocked;
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

void setPriority0(char * str, int index){
	/* set a Control Block Priority to 0, moving others to 3 */
	char * strDup = (char*)malloc((CSIZE * 18)+1);
	strDup = strdup(str);
	int rowCount = getLineCount(strdup(strDup));

	if(rowCount == 0)
		return;
	
	for(int i = 0; i < rowCount; i++){
		if( strcmp(getColumnString(strdup(strDup), i, 2), "0") == 0 ){
			setColumnString(strDup, "1", i, 2);
		}
	}
	setColumnString((char*)strDup, "0", 0, 2);
	strcpy(str, strDup);
	free(strDup);
	return;
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
	shm_unlink("IDLE");
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
	char * tempRow = strdup(tokens[row]);
	char ** tempValues = splitString(strdup(tempRow),'|');
	strcpy(tempValues[col], newStr);
	char * strCpy = strdup(str);
	strcpy(str, "");	

	/* assemble all rows */
	for(int i = 0; i < getLineCount(strdup(strCpy)); i++){
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

int getColumnCount(char * str){
	/* get number of lines in the file */
	int count = 0;
	for(int i = 0; i < strlen(str); i++){
		if(str[i] == '|'){
			count++;
		}
	}
	return count;
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

void writeRow(char * filename, char * str){
	/* write the str to filename */
	FILE *fp;
	fp = fopen(filename, "a");
	fprintf(fp, str);
	fprintf(fp, "\n");
	fclose(fp);
	return;
}
