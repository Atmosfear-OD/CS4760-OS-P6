/*
 * shmem.h by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 *
 *  This file contains the necessary functions and strcutures for shared memory management.
 */

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdbool.h>

#define STRUCT_SZ ((sizeof(rscArraySz)/sizeof(rscArraySz[0])) * 18)
#define SEM_SIZE sizeof(int)
#define QUE_SZ 18

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

// Structure for memory manager
typedef struct memory_manager{
        pid_t pid;
        int resource_Marker;
        int tableSz[32];

} memory_manager;

struct memory_manager *rscArraySz[18];
struct memory_manager *(*rscArrayPointer)[] = &rscArraySz;

// Globals
pid_t pid = 0;
extern int limit;
extern int percentage;

// Fork a child
pid_t forkChild(char *sharedTimeMem, char *sharedSemMem, char*sharedPositionMem, char*rscShrdMem, char*sharedLimitMem, char*sharedPercentageMem) {
        if((pid = fork()) == 0) {
                execlp("./user", "./user", sharedTimeMem, sharedSemMem, sharedPositionMem, rscShrdMem, sharedLimitMem, sharedPercentageMem, NULL);
        }

        if(pid < 0) {
                printf("Fork Error %s\n", strerror(errno));
        }

        return pid;
};

// Initialize shared memory
void makeShMem(int *timeid, int *semid, int*rscID, key_t timeKey, key_t semKey, key_t rscKey) {
       *timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *semid = shmget(semKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *rscID = shmget(rscKey, (sizeof(memory_manager) *2),0666|IPC_CREAT);

        if(*timeid == -1) perror("shmem: Error: Failure to create shmem for time");
        if(*semid == -1) perror("shmem: Error: Failure to create shmem for sem");
        if(*rscID == -1) perror("shmem: Error: Failure to create shmem for res");
};

void makeShMemKey(key_t *rscKey, key_t *semKey, key_t *timeKey) {
        *rscKey = ftok(".", 4328);
        *semKey = ftok(".", 8993);
        *timeKey = ftok(".", 2820);
};

void rsg_manage_args(char *sharedTimeMem, char *sharedSemMem, char *sharedPositionMem, char *rscShrdMem, char *sharedLimitMem, char *sharedPercentageMem, int timeid, int semid, int rscID, int placementMarker, int limit, int percentage) {
        snprintf(sharedTimeMem, sizeof(sharedTimeMem)+25, "%d", timeid);
        snprintf(sharedSemMem, sizeof(sharedSemMem)+25, "%d", semid);
        snprintf(sharedPositionMem, sizeof(sharedPositionMem)+25, "%d", placementMarker);
        snprintf(rscShrdMem, sizeof(rscShrdMem)+25, "%d", rscID);
        snprintf(sharedLimitMem, sizeof(sharedLimitMem)+25, "%d", limit);
        snprintf(sharedPercentageMem, sizeof(sharedPercentageMem)+25, "%d", percentage);
};

// Attach shared memory
void ShMemAttach(unsigned int **seconds, unsigned int **nanoseconds, sem_t **semaphore, memory_manager **rscPointer, int timeid, int semid, int rscID) {
        *seconds = (unsigned int*)shmat(timeid, NULL, 0);
        if(**seconds == -1) perror("shmem: Error: Failure to attach shared memory for sec");

        *nanoseconds = *seconds + 1;
        if(**nanoseconds == -1) perror("shmem: Error: Failure to attach shared memory for ns");
        
        *semaphore = (sem_t*)shmat(semid, NULL, 0);
        if(*semaphore == (void*)-1) perror("shmem: Error: Failure to attach shared memory for sem");
        
        *rscPointer = (memory_manager*)shmat(rscID, NULL, 0);
        if(*rscPointer == (void*)-1) perror("shmem: Error: Failure to attach shared memory for resPointer");
};
