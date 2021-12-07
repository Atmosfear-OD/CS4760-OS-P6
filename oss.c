/*
 * oss.c by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 * 
 *  This file is the entry point of the program.
 */

#include "oss.h"
#include "shmem.h"

// Globals
int alrm, processCount, frameTablePos = 0;
int setArr[20] = {0};

// Strucutre for messages
struct message {
    long msgString;
    char msgChar[100];
} message;

// Main
int main(int argc, char *argv[]) {
	int opt;
	processCount = PROCESS_MAX; // Default 18 processes, macro is in oss.h
        
        while((opt = getopt(argc, argv, "hp:")) != -1){
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
                                processCount = atoi(optarg);
                                if(processCount < 1 || processCount > 18) {
					printf("oss: Error: Entered process count greater than 18 or less than 1, default of 18 will be used\n");
					processCount = PROCESS_MAX;
				}
				break;
                        default:
			       perror("oss: Error: Invalid argument");
                                helpMessage();
                                exit(1);
                }
        }
	
	// Ensure ./oss is ran with -p
	if((argc == 1) || (strcmp(argv[1], "-p") != 0)) {
                printf("oss: Error: Insufficient arguments, must execute ./oss with -p\n\n");
                helpMessage();
                return 0;
        }

	printf("oss: Starting program with a process count of %d\n", processCount);
	
	struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sigemptyset(&sa.sa_mask);
        sa.sa_sigaction = catchSegFault;
        sa.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &sa, NULL);

	// For shared memory info
        char childMsg[20], ch;
        char requestType[20];
        char sharedPositionMem[10];
        char sharedPercentageMem[10];
        char sharedTimeMem[10];
        char rscShrdMem[10];
        char sharedSemMem[10];
        char sharedLimitMem[10];

	int address;
        int forked = 0;
        int lines = 0;
        int frameLoop = 0;
        int pagefault = 0;
        int requests = 0;
        int initialFork = 0;
        int tempPid = 0;
        int status;
	
	// For timer
	unsigned int *seconds = 0;
        unsigned int *nanoseconds = 0;
        unsigned int forkTimeSeconds = 0;
        unsigned int forkTimeNanoseconds = 0;
        unsigned int accessSpeed = 0;

	// Open output file
	printf("oss: Opening output file...\n");
	srand(time(NULL));
        char* fileName =  "outfile.log";
        FILE *fp = fopen(fileName, "w");
        freopen("outfile.log", "a", fp);
        int percentage = 50, maxProcL = 900;
        int frameTable[256][3] = {{0}};

        key_t msgKey = ftok(".", 432820), timeKey = 0, rscKey = 0, semKey = 0;
        int msgid = msgget(msgKey, 0666 | IPC_CREAT), timeid = 0, rscID = 0, semid = 0, placementMarker = 0;

        memory_manager *rscPointer = NULL;
        sem_t *semPtr = NULL;
        makeShMemKey(&timeKey, &semKey, &rscKey);
        makeShMem(&timeid, &semid, &rscID, timeKey, semKey, rscKey);
        ShMemAttach(&seconds, &nanoseconds, &semPtr, &rscPointer, timeid, semid, rscID);
        double pageFaults = 0, memoryAccesses = 0, memoryAccessesPerSecond = 0;
        float childRequestAddress = 0;
        signal(SIGALRM, killTimer);
        alarm(2);
        
	do {
                if(initialFork == 0) {
                        doFork(seconds, nanoseconds, &forkTimeSeconds, &forkTimeNanoseconds);
                        initialFork = 1;
                        fprintf(fp, "OSS: Fork Time starts at %d:%d\n", forkTimeSeconds, forkTimeNanoseconds);
                }

                *nanoseconds += 50000;
                
		if(*nanoseconds >= 1000000000){
                        *seconds += 1;
                        *nanoseconds = 0;
                        memoryAccessesPerSecond = (memoryAccesses / *seconds);
                }
                
		if(((*seconds == forkTimeSeconds) && (*nanoseconds >= forkTimeNanoseconds)) || (*seconds > forkTimeSeconds)){
                        if(checkArray(&placementMarker) == 1){
                                forked++;
                                initialFork = 0;
                                fprintf(fp, "OSS: Forked at time %d:%d \n", *seconds, *nanoseconds);
                                rsg_manage_args(sharedTimeMem, sharedSemMem, sharedPositionMem, rscShrdMem, sharedLimitMem, sharedPercentageMem, timeid, semid, rscID, placementMarker, maxProcL, percentage);
                                pid_t childPid = forkChild(sharedTimeMem, sharedSemMem, sharedPositionMem, rscShrdMem, sharedLimitMem, sharedPercentageMem);
                                rscArraySz[placementMarker] = malloc(sizeof(struct memory_manager));
                                (*rscArrayPointer)[placementMarker]->pid = childPid;
                                fprintf(fp, "OSS: Child %d spawned with PID %d at time %d:%d\n", placementMarker, childPid, *seconds, *nanoseconds);
                                
				for(int i = 0 ; i < 32 ; i++){
                                        (*rscArrayPointer)[placementMarker]->tableSz[i] = -1;
                                }
                                
				(*rscArrayPointer)[placementMarker]->resource_Marker = 1; 

                        }
                }

                for(int i = 0 ; i < processCount ; i++) {
                        if(setArr[i] == 1) {
                                tempPid =  (*rscArrayPointer)[i]->pid;

                                if((msgrcv(msgid, &message, sizeof(message) - sizeof(long), tempPid, IPC_NOWAIT|MSG_NOERROR)) > 0) {
                                        // If it recieved a read or write
					if(atoi(message.msgChar) != 99999) {
                                                fprintf(fp, "OSS: P%d requesting address %d to ",i ,atoi(message.msgChar));
                                                strcpy(childMsg, strtok(message.msgChar, " "));
                                                address = atoi(childMsg);
                                                strcpy(requestType, strtok(NULL, " "));
                                                if(atoi(requestType) == 0) {
                                                        fprintf(fp, "be read at time %d:%d\n", *seconds, *nanoseconds);
                                                } else {
                                                        fprintf(fp, "be written at time %d:%d\n", *seconds, *nanoseconds);
                                                }
                                                childRequestAddress = (atoi(childMsg)) / 1000;
                                                childRequestAddress = (int)(floor(childRequestAddress));
						// If page table position is empty or is no longer associated with the child request address -- assign to frame table
                                                if((*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] == -1 || frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][0] != (*rscArrayPointer)[i]->pid) {
                                                        frameLoop = 0;
							// Check for first available frame
                                                        while(frameTable[frameTablePos][0] != 0 && frameLoop < 255) {
                                                                frameTablePos++;
                                                                frameLoop++;

                                                                if(frameTablePos == 256) frameTablePos = 0;
                                                                if(frameLoop == 255) pagefault = 1;
                                                        }
                                                        
							if(pagefault == 1) { 
                                                                pageFaults++;
                                                                fprintf(fp, "OSS: Address %d is not in a frame, page fault\n", address);
                                                                
								while(frameTable[frameTablePos][1] != 0){ //Check for second frame if it exists
                                                                        frameTable[frameTablePos][1] = 0; //Set to 0 if it was 1
                                                                        frameTablePos++; //Move to next frame
                                                                        
									if(frameTablePos == 256) frameTablePos = 0;
                                                                }

                                                                if(frameTable[frameTablePos][1] == 0) {
                                                                        memoryAccesses++;
                                                                        fprintf(fp, "OSS: Clearing frame %d and swapping in P%d page %d at time %d:%d\n", 
											frameTablePos, i, (int)childRequestAddress, *seconds, *nanoseconds);
                                                                        //New page frame goes in this section
                                                                        (*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] = frameTablePos;
                                                                        frameTable[frameTablePos][0] = (*rscArrayPointer)[i]->pid;
                                                                        frameTable[frameTablePos][2] = atoi(requestType);
                                                                        fprintf(fp, "OSS: Address %d in frame %d giving data to P%d at time %d:%d\n", 
											address, frameTablePos, i, *seconds, *nanoseconds);
                                                                        frameTablePos++; //Our clock increments in sec/nanosec
                                                                        if(frameTablePos == 256) frameTablePos = 0;
                                                                        requests++;
                                                                }

                                                                accessSpeed +=  15000000;
                                                                *nanoseconds += 15000000;
                                                                fprintf(fp, "OSS: Dirty bit is set to %d and clock is incremented 15ms\n", atoi(requestType));
                                                        } 
							// If an empty frame is found
							else {
                                                                memoryAccesses++;
                                                                (*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] = frameTablePos; 
                                                                frameTable[frameTablePos][0] = (*rscArrayPointer)[i]->pid;
                                                                frameTable[frameTablePos][1] = 0; //Resources is now clear
                                                                frameTable[frameTablePos][2] = atoi(requestType);
                                                                fprintf(fp, "OSS: Address %d in frame %d giving data to P%d at time %d:%d\n", 
										address, frameTablePos, i, *seconds, *nanoseconds);
                                                                frameTablePos++; //Advance clock.
                                                                if(frameTablePos == 256){
                                                                        frameTablePos = 0;
                                                                }
                                                                accessSpeed  += 10000000;
                                                                *nanoseconds += 10000000;
                                                                requests++;
                                                                fprintf(fp, "OSS: Dirty bit is set to %d and and is incremented an addtional 10ms to the clock\n", atoi(requestType));
                                                        }
                                                } else {
                                                        memoryAccesses++;
                                                        frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][1] = 1; //Referenced bit set in function
                                                        frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][2] = atoi(requestType); //Dirty Bit is set 
                                                        *nanoseconds += 10000000;
                                                        accessSpeed +=  10000000;
                                                        requests++; 
                                                        fprintf(fp, "OSS: Dirty bit is set to %d and is incremented an addtional 10ms to the clock\n", atoi(requestType));
                                                }

                                                message.msgString = ((*rscArrayPointer)[i]->pid + 118);
                                                sprintf(message.msgChar,"wakey");
                                                msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                                        } else if(atoi(message.msgChar) == 99999) { 
                                                setArr[i] = 0;
                                                message.msgString = ((*rscArrayPointer)[i]->pid + 118);
                                                fprintf(fp, "OSS: P%d is complete -- clearing frames: ", i);

                                                for(int j = 0 ; j < 32 ; j++) {
                                                        if((*rscArrayPointer)[i]->tableSz[j] != -1 && frameTable[(*rscArrayPointer)[i]->tableSz[j]] == (*rscArrayPointer)[i]->tableSz[j]) {
                                                                fprintf(fp, "%d, ", j);
                                                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][0] = 0;
                                                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][1] = 0;
                                                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][2] = 0;
                                                                (*rscArrayPointer)[i]->tableSz[j] = -1;
                                                        }
                                                }

                                                fprintf(fp, "\n");
                                                msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0);
                                                waitpid(((*rscArrayPointer)[i]->pid), &status, 0);
                                                free(rscArraySz[i]);
                                        }
                                }
                        }
                }
		
                while((ch = fgetc(fp)) != EOF){
                        if(ch == '\n'){
                                lines++;
                        }
                }
		// Stop printing and close file if line number exceeds 100000
                if(lines >= 100000){
			fprintf(fp, "\nOSS: Total line number has exceeded 100000 -- now closing file\n");
                        fclose(fp);
                }

        } while((*seconds < SECOND_TIMER + 10000) && alrm == 0 && forked < 100);

        // Print final statistics
	fprintf(fp, "\nOSS: Program complete\n");
	fprintf(fp, "----- STATISTICS -----\n\tMemory Accessed Per Second: %f\n\tPagefaults Per Memory Access: %f\n\tAverage Access Speed in nanosec: %f\n\tTotal Forks: %d\n\n", 
			memoryAccessesPerSecond, pageFaults/memoryAccesses, accessSpeed/memoryAccesses, forked);
	
	// Cleanup and close output file
	printf("oss: Now closing output file and removing shared memory/message queue...\n");
        fclose(fp);
        shmdt(seconds);
        shmdt(semPtr);
        shmdt(rscPointer);
        msgctl(msgid, IPC_RMID, NULL);
        shmctl(msgid, IPC_RMID, NULL);
        shmctl(rscID, IPC_RMID, NULL);
        shmctl(timeid, IPC_RMID, NULL);
        shmctl(semid, IPC_RMID, NULL);

	printf("oss: Terminating program\n");
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

int checkArray(int *placementMarker) {
        for(int inc = 0 ; inc < processCount ; inc++){
                if(setArr[inc] == 0){
                        setArr[inc] = 1;
                        *placementMarker = inc;
                        return 1;
                }
        }

        return 0;
}

void catchSegFault(int signal, siginfo_t *si, void *arg) {
        fprintf(stderr, "Caught segfault at address %p\n", si->si_addr);
        kill(0, SIGTERM);
}

void doFork(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *forkTimeSeconds, unsigned int *forkTimeNanoseconds){
        unsigned int random = rand()%500000000;
        *forkTimeNanoseconds = 0;
        *forkTimeSeconds = 0;

        if((random + nanoseconds[0]) >= 1000000000) {
                *forkTimeSeconds += 1;
                *forkTimeNanoseconds = (random + *nanoseconds) - 1000000000;
        } else {
                *forkTimeNanoseconds = random + *nanoseconds;
        }

        *forkTimeSeconds = *seconds;
}

