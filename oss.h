/*
 * oss.h by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 *
 *  Header file for oss.c, contains function prototypes and structures.
 */

#ifndef OSS_H
#define OSS_H

#define SECOND_TIMER 100

typedef struct Message {
	long msgSize;
	char msgString[100];
} Message;

// Function prototypes
void helpMessage();
void killTimer();
void catchSegFault();
void forkTime();
int checkArray();

#endif
