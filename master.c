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
#include <time.h>
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

int getnamed(char *name, sem_t **sem, int val);
char ** splitString(char * str, const char delimiter);
int getLineCount(char * filename);
void clearOldOutput();
void cleanExit(char * str, int childCount);
void sigintHandler(int sig_num);
void printOptions();

int main(int argc, char * argv[]){

	extern int optind;
	extern char * optarg;
	static char usage[] = "usage: [-h] [-i string]\n";
	int c;
	char * inFile = "input.txt";
	while(( c = getopt ( argc, argv, "hi:i")) != -1){
		switch(c){
			case 'h':
				printOptions();
				break;
			case 'i': 
				inFile = optarg;
				break;
			case '?':
				if(optopt == 'i'){
					fprintf(stderr, "ERROR:\n   --> Expected an argument for -i (string), see usage\n\t%s %s", argv[0], usage);
				}
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option -%c'.  See usage\n", optopt);
				else
					fprintf(stderr, "Unknown option character.  See usage\n");
				return 1;
			default:
				abort();
		}
	}
	printf("\t\t--> Master Start <--\n");
	signal(SIGINT, sigintHandler);
	
	int tokenLines;
	char * cdata;
	size_t dataSize;
	
	/* read input file */
	FILE *file;
	int errnum;
	file = fopen(inFile, "r");
	if(file == NULL){
		errnum = errno;
		fprintf(stderr, "\t%sValue of errno: %d\n", argv[0], errno);
		perror("\tError with fopen()");
		fprintf(stderr, "\tError opening input file: %s\n", strerror(errnum));
		exit(1);
	} else {
		/* get file size with fseek, rewind with fseek, assign file content to char pointer */
		file = fopen(inFile, "rb");
		fseek(file, 0, SEEK_END);
		long fsize = ftell(file);
		fseek(file, 0, SEEK_SET);	
		cdata = malloc(fsize + 1);
		fread(cdata, fsize, 1, file);
		fclose(file);

		/* parse file content into 2d char array and get line count */
		tokenLines = getLineCount("input.txt");
		dataSize = strlen(cdata)-1;
		cdata[dataSize] = '\0';
		fflush(stdout);
	}
	clearOldOutput();

	/* place cdata into shared memory */
	int fd_shm;
	fd_shm = shm_open( "STRINGS", O_CREAT | O_RDWR, 0666);
	ftruncate( fd_shm, dataSize);
	void * mapPtr = mmap(0, dataSize, PROT_WRITE, MAP_SHARED, fd_shm, 0);
	sprintf(mapPtr, "%s", cdata);

	/* create names semaphore */
	sem_t *semaphore;
	if(getnamed("/SEMA", &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}

	char * sizeStr = (char*)malloc(sizeof(dataSize) * sizeof(char));
	sprintf(sizeStr, "%zu", dataSize);
	pid_t pid;

	/* create children */
	for(int i = 0; i < tokenLines-1; i++){
		if(i >= 20){
			wait(NULL);
			i--;	
		}
		char iStr[32];
		if((pid = fork()) == 0){
			srand(time(NULL));
			sprintf(iStr, "%d", i);
			char * args[] = {"./palin", "NULL", iStr, sizeStr, '\0'};
			execvp("./palin", args);	
		}
	}	

	/* wait for children then unlink and clear memeory */
	cleanExit(sizeStr, tokenLines-1);
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
	free(str);
	shm_unlink("STRINGS");
	sem_unlink("/SEMA");
	printf("---> Finished.\n");
	exit(0);
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
