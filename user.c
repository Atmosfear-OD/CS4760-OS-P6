/*
 * user.c by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 *
 * User process called by oss.c.
 */

#include "user.h"
#include "shmem.h"

// Structure for messages
struct message {
    long msgString;
    char msgChar[100];
} message;

// Main
int main(int argc, char *argv[]) {
        int complete = 0, request = 0;
        srand(getpid());
        key_t msgKey = ftok(".",432820);
        int msgid = msgget(msgKey, 0666 | IPC_CREAT);
        int timeid = atoi(argv[1]);
        int semid = atoi(argv[2]);
        int rscID = atoi(argv[4]);
        int limit = atoi(argv[5]);
        int percentage = atoi(argv[6]);
        int event = 0;
        pid_t pid = getpid();
        sem_t *semPtr;
        memory_manager *rscPointer;
        unsigned int *seconds = 0, *nanoseconds = 0, eventTimeSeconds = 0, eventTimeNanoseconds = 0, requests = 0;
        ShMemAttach(&seconds, &nanoseconds, &semPtr, &rscPointer, timeid, semid, rscID);
        message.msgString = pid;
        message.msgString = 12345;
        randomTimer(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);
        
	while(complete == 0) {
                if((*seconds == eventTimeSeconds && *nanoseconds >= eventTimeNanoseconds) || *seconds > eventTimeSeconds) {
                        event = rand() % 99;
                        request = rand() % 32001;
                        requests++;
                        randomTimer(seconds, nanoseconds, &eventTimeSeconds, &eventTimeNanoseconds);

                        if(requests == limit && event < 75) {
                                message.msgString = (int)pid;
                                sprintf(message.msgChar,"%d", 99999);
                                msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                                msgrcv(msgid, &message, sizeof(message)-sizeof(long), (pid + 118), 0);
                                complete = 1;
                        } else if(event < percentage) {
                                message.msgString = (int)pid;
                                sprintf(message.msgChar,"%d %d", request, 0);
                                msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                                msgrcv(msgid, &message, sizeof(message)-sizeof(long), (pid + 118), 0);
                        } else if(event >= (99 - percentage)) {
                                message.msgString = (int)pid;
                                sprintf(message.msgChar,"%d %d", request, 1);
                                msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                                msgrcv(msgid, &message, sizeof(message)-sizeof(long), (pid + 118), 0);
                        }
                }
        }

	// Cleanup
        shmdt(seconds);
        shmdt(semPtr);
        shmdt(rscPointer);
        shmctl(msgid, IPC_RMID, NULL);
        exit(0);
}

// Generate random times
void randomTimer(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *eventTimeSeconds, unsigned int *eventTimeNanoseconds){
        unsigned int r = rand() % 500000000;
        *eventTimeNanoseconds = 0;
        *eventTimeSeconds = 0;
        if((r + *nanoseconds) >= 1000000000) {
                *eventTimeSeconds += 1;
                *eventTimeNanoseconds = (r + *nanoseconds) - 1000000000;
        } else {
                *eventTimeNanoseconds = r + *nanoseconds;
        }

        *eventTimeSeconds = *seconds;
}
