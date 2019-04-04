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

char ** splitString(char * str, const char delimiter);
int getLineCount(char * filename);
void clearOldOutput();
void cleanExit(char * str, int childCount);
void sigintHandler(int sig_num);
void printOptions();

int main(int argc, char * argv[]){

	// create process table control blocks
	// put them in shared memeory
	// max 18 control blocks
	// 
	// create a bit vector, local to oss
	//
	// create a clock, 1 in seconds, 1 in nanoseconds
	

	printf("\t\t-->>> OSS Start <<<--\n");
	signal(SIGINT, sigintHandler);
	
	unsigned int secClock = 0;
	unsigned long nanoClock = 0;

	while(1){
		sleep(1);



		secClock++;
		nanoClock += 1e9;
		printf("Seconds: %u\nNanos: %lu\n", secClock, nanoClock);
		if(secClock > 10)
			break;
	}	


	return 0;
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

int getLineCount(char * filename){
	/* get number of lines in the file */
	FILE * fp;
	int count = 0;
	char c;
	fp = fopen(filename, "r");
	if(fp == NULL){
		printf("Could not open file %s", filename);
		exit(1);
	}
	for(c = getc(fp); c !=EOF; c = getc(fp)){
		if(c == '\n'){
			count++;
		}
	}
	fclose(fp);
	return count;
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
	shm_unlink("STRINGS");
	sem_unlink("/SEMA");
	exit(0);
}

void printOptions(){
	/* print command line arguments for user reference */
	printf("\n========== Command-Line Options ==========\n\n> Optional: -h (view command-line options)\n");
	printf("> Optional: -i (specify input name, default input.txt)\n");
	fflush(stdout);
	exit(0);
}
