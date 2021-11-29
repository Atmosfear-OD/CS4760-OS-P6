/*
 * oss.c by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 * 
 *  This file is the entry point of the program.
 */

#include "oss.h"
#include "shmem.h"

// Globals
FILE *fp;
int alrm, procCount, frameTablePos = 0;
int percent = 50;
int maxProcL = 900;
int arr[20] = {0};

// Main
int main(int argc, char *argv[]) {
	int opt;

        // Check for opt args
        while((opt = getopt(argc, argv, "hp:")) != -1) {
                switch(opt) {
                        case 'h':
                                helpMessage();
                                exit(0);
                        case 'p':
				if(strspn(argv[2], "0123456789") != strlen(argv[2])) {
	                	        printf("oss: Error: Entered value for -p not an integer\n\n");
        		                helpMessage();
                        		exit(0);
                		}
				procCount  = atoi(optarg);
                                if(procCount < 1 || procCount > 20) {
					printf("oss: Error: Entered process count greater than 20 or less than 1, defualt of 20 will be used\n");
					procCount = 20;
				}
                                break;
                        default:
                                perror("oss: Error: Invalid argument");
                                helpMessage();
                                exit(1);
                }
        }

	if((argc == 1) || (strcmp(argv[1], "-p") != 0)) {
                printf("oss: Error: Insufficient arguments, must execute ./oss with -p\n\n");
                helpMessage();
                return 0;
        }

	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = catchSegFault;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &sa, NULL);
	
	// For shared memory info
	char childMsg[20], ch;
        char requestType[20];
	char shPositionMem[10];
	char shPercentMem[10];
        char shTimeMem[10];
	char resShmem[10];
	char shSemMem[10];
	char shLimitMem[10];

        int address;
        int forked = 0;
	int lines = 0;
	int frameLoop = 0;
	int pagefault = 0;
	int requests = 0;
	int initFork = 0;
	//int j = 0, i = 0;
	int tempPid = 0;
	int status;
	
	// For timer
        unsigned int *sec = 0;
	unsigned int *ns = 0;
	unsigned int forkTimeSec = 0;
	unsigned int forkTimeNs = 0;
	unsigned int accessSpeed = 0;
	
	// Open output files
        char logName[20] = "outfile.log"; // outfile for logging
        fp = fopen(logName, "w");
	freopen("memManagement.txt", "a", fp);
        srand(time(NULL));
	
	key_t msgKey = ftok(".", 432820), timeKey = 0, resKey = 0, semKey = 0;
        int msgID = msgget(msgKey, 0666 | IPC_CREAT), timeID = 0, resID = 0, semID = 0, placeMarker = 0;

        Shmem *resPtr = NULL;
        sem_t *semPtr = NULL;
        assignShmemKeys(&timeKey, &semKey, &resKey);
        makeShmem(&timeID, &semID, &resID, timeKey, semKey, resKey);
        attachShmem(&sec, &ns, &semPtr, &resPtr, timeID, semID, resID);
        double pageFaults = 0, memoryAccesses = 0, memoryAccessesPerSecond = 0;
        float childRequestAddress = 0;
        signal(SIGALRM, killTimer);
        alarm(2);

	do {
		if(initFork == 0){
                        forkTime(sec, ns, &forkTimeSec, &forkTimeNs);
                        initFork = 1;
                        fprintf(fp, "OSS: Fork starts at time %d:%d\n", forkTimeSec, forkTimeNs);
                }

                *ns += 50000;
                
		if(*ns >= 1000000000){
                        *sec += 1;
                        *ns = 0;
                        memoryAccessesPerSecond = (memoryAccesses/ *sec);
                }

		if(((*sec == forkTimeSec) && (*ns >= forkTimeNs)) || (*sec > forkTimeSec)){
                        if(checkArray(&placeMarker) == 1){
                                forked++;
                                initFork = 0;
                                fprintf(fp, "OSS: Forked at time %d:%d\n", *sec, *ns);
                                mngArgs(shTimeMem, shSemMem, shPositionMem, resShmem, shLimitMem, shPercentMem, timeID, semID, resID, placeMarker, maxProcL, percent);
                                pid_t childPid = doFork(shTimeMem, shSemMem, shPositionMem, resShmem, shLimitMem, shPercentMem);
                                resArraySize[placeMarker] = malloc(sizeof(struct Shmem));
                                (*resArrayPtr)[placeMarker]->pid = childPid;

                                fprintf(fp, "OSS: Child %d created with PID %d at time %d:%d\n", placeMarker, childPid, *sec, *ns);
                                for(int i = 0 ; i < 32 ; i++){
                                        (*resArrayPtr)[placeMarker]->table[i] = -1;
                                }
                                (*resArrayPtr)[placeMarker]->resMarker = 1;
                        }
                }
	
		while((ch = fgetc(fp)) != EOF) {
			if(ch == '\n') lines++;
		}
	
		// Stop printing and close file if line number exceeds 100000
		if(lines >= 100000) {
			fprintf(fp, "\nOSS: Total line number has exceeded 100000 -- now closing file\n");
			fclose(fp);
		}
	} while((*sec < SECOND_TIMER + 10000) && alrm == 0 && forked < 100);
	
	// Print final statistics
	fprintf(fp, "\nOSS: Program complete\n");
	fprintf(fp, "----- STATISTICS -----\n\tMemory Accessed Per Second: %f\n\tPagefaults Per Memory Access: %f\n\tAverage Access Speed in nanosec: %f\n\tTotal Forks: %d", 
			memoryAccessesPerSecond, pageFaults/memoryAccesses, accessSpeed/memoryAccesses, forked);

	// Cleanup and close log file
	fclose(fp);
	shmdt(sec);
        shmdt(semPtr);
        shmdt(resPtr);
        msgctl(msgID, IPC_RMID, NULL);
        shmctl(msgID, IPC_RMID, NULL);
        shmctl(resID, IPC_RMID, NULL);
        shmctl(timeID, IPC_RMID, NULL);
        shmctl(semID, IPC_RMID, NULL);
        kill(0, SIGTERM);

	return 0;
}

// Prints help message
void helpMessage() {
        printf("---------- USAGE ----------\n");
        printf("./oss [-h] [-p]\n");
        printf("-h\tDisplays usage message (optional)\n");
        printf("-p\tSpecify number of user processes in system (must be integer less than or equal to 20 and greater than 0, else 20 will be used as defualt)\n");
        printf("---------------------------\n");
}

void killTimer(int signal) {
	alrm = 1;
}

// Catch segfault and print address
void catchSegFault(int signal, siginfo_t *sigInfo) {
	fprintf(stderr, "Caught segfault at address %p\n", sigInfo->si_addr);
	kill(0, SIGTERM);
}

// Calculate time it took to fork
void forkTime(unsigned int *sec, unsigned int *ns, unsigned int *forkTimeSec, unsigned int *forkTimeNs) {
	unsigned int r = rand() % 500000000;
	*forkTimeNs = 0;
	*forkTimeSec = 0;

	if((r + ns[0]) >= 1000000000) {
		*forkTimeSec += 1;
		*forkTimeNs = (r + *ns) - 1000000000;
	} else {
		*forkTimeNs = r + *ns;
	}

	*forkTimeNs = *sec;
}

int checkArray(int placeMarker) {
	for(int i = 0 ; i < procCount ; i++) {
		if(arr[i] == 0) {
			arr[i] = 1;
			placeMarker = i;

			return 1;
		}
	}

	return 0;
}
