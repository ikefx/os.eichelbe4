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

char ** splitString(char * str, const char delimiter);
int getLineCount(char * filename);
void clearOldOutput();
void cleanExit(char * str);

int main(int argc, char * argv[]){

	char ** tokens;
	int tokenLines;
	/* read input file */
	FILE *infile;
	int errnum;
	infile = fopen("input.txt", "r");
	if(infile == NULL){
		errnum = errno;
		fprintf(stderr, "\t%sValue of errno: %d\n", argv[0], errno);
		perror("\tError with fopen()");
		fprintf(stderr, "\tError opening file: %s\n", strerror(errnum));
	} else {
		/* get file size with fseek, rewind with fseek, assign file content to char pointer */
		infile = fopen("input.txt", "rb");
		fseek(infile, 0, SEEK_END);
		long fsize = ftell(infile);
		fseek(infile, 0, SEEK_SET);	
		char * cdata = malloc(fsize + 1);
		fread(cdata, fsize, 1, infile);
		fclose(infile);

		/* parse file content into 2d char array and get line count */
		tokenLines = getLineCount("input.txt");
		size_t dataSize = strlen(cdata) - 2;
		cdata[dataSize] = '\0';
		char * cdataDup = strdup(cdata);
		tokens = splitString(cdataDup, '\n');
		fflush(stdout);
	}
	
	clearOldOutput();

	/* create names semaphore */
	sem_t *semaphore = sem_open("/semaphore_example", O_CREAT | O_EXCL, SEM_PERMS, 1);
	if(semaphore == SEM_FAILED){
		perror("sem_open(3) failed in master ln39");
		sem_unlink("/semaphore_example");
		exit(EXIT_FAILURE);
	}

	/* place tokens (char**) into shared memory */
	char ** sharedPtr;
	int fd_shm;
	fd_shm = shm_open ("/STRINGS", O_RDWR | O_CREAT, 0660);
	ftruncate(fd_shm, sizeof(char[256][256]));
	sharedPtr = mmap(NULL, sizeof(char[256][256]), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
	sharedPtr = tokens;

	printf("%s\n", sharedPtr[10]);

	int inputIndex = 101;
	char * indexStr = NULL;
	pid_t pid;
	
	/* create children */
	for(int i = 0; i < 19; i++){
		if((pid = fork()) == 0){
			indexStr = malloc(sizeof(char)*(int)(inputIndex));
			sprintf(indexStr, "%d", inputIndex);
			char * args[] = {"./palin", "eva can i stab bats in a cave", '\0'};
			execvp("./palin", args);	
		}
	}
	
	/* wait for children then unlink and clear memeory */
	cleanExit(indexStr);
	shm_unlink("STRINGS");
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
