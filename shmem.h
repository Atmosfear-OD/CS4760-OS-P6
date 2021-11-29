/*
 * shmem.h by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 *
 *  This file contains the necessary functions and strcutures for shared memory management.
 */

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/mman.h>


// Structure for shmem
typedef struct Shmem {
	pid_t pid;
	int resMarker, table[32];
} Shmem;

// Globals
struct Shmem *resArraySize[18];
struct Shmem *(*resArrayPtr)[] = &resArraySize;
pid_t pid = 0;
extern int limit, percent;

// Fork a child and execlp user.c if successful
pid_t doFork(char *shTimeMem, char *shSemMem, char*shPositionMem, char*resShmem, char*shLimitMem, char*shPercentMem) {
	if((pid = fork()) == 0) execlp("./user.c", "./user.c", shTimeMem, shSemMem, shPositionMem, resShmem, shLimitMem, shPercentMem, NULL);
	if(pid < 0) perror("shmem: Error: Failure to fork");

	return pid;
};

// Assign necessary shared memory keys
void assignShmemKeys(key_t *resKey, key_t *semKey, key_t *timeKey) {
	*resKey = ftok(".", 4328);
        *semKey = ftok(".", 8993);
        *timeKey = ftok(".", 2820);
};

// Create shared memory
void makeShmem(int *timeID, int *semID, int*resID, key_t timeKey, key_t semKey, key_t resKey) {
	*timeID = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *semID = shmget(semKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *resID = shmget(resKey, (sizeof(Shmem) * 2), 0666 | IPC_CREAT);

        if(*timeID == -1) perror("shmem: Error: Failure to create shmem for time");
        if(*semID == -1) perror("shmem: Error: Failure to create shmem for sem");
        if(*resID == -1) perror("shmem: Error: Failure to create shmem for res");
};

// Attach shared memory
void attachShmem(unsigned int **sec, unsigned int **ns, sem_t **sem, Shmem **resPointer, int timeID, int semID, int resID) {
	*sec = (unsigned int*)shmat(timeID, NULL, 0);
	if(**sec == -1) perror("shmem: Error: Failure to attach shared memory for sec");
        
	*ns = *sec + 1;
        if(**ns == -1) perror("shmem: Error: Failure to attach shared memory for ns");
       
        *sem = (sem_t*)shmat(semID, NULL, 0);
        if(*sem == (void*)-1) perror("shmem: Error: Failure to attach shared memory for sem");
        
        *resPointer = (Shmem*)shmat(resID, NULL, 0);
        if(*resPointer == (void*)-1) perror("shmem: Error: Failure to attach shared memory for resPointer");
};

void mngArgs(char *shTimeMem, char *shSemMem, char*shPositionMem, char*resShmem, char*shLimitMem, char*shPercentMem, int timeID, int semID, int resID, int placeMarker, int lim, int per) {
	snprintf(shTimeMem, sizeof(shTimeMem)+25, "%d", timeID);
        snprintf(shSemMem, sizeof(shSemMem)+25, "%d", semID);
        snprintf(shPositionMem, sizeof(shPositionMem)+25, "%d", placeMarker);
        snprintf(resShmem, sizeof(resShmem)+25, "%d", resID);
        snprintf(shLimitMem, sizeof(shLimitMem)+25, "%d", lim);
        snprintf(shPercentMem, sizeof(shPercentMem)+25, "%d", per);
};
