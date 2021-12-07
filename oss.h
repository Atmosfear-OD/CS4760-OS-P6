/*
 * oss.h by Pascal Odijk 11/29/2021
 * P6 CMPSCI 4760 Prof. Bhatia
 *
 *  Header file for oss.c, contains the function prototypes.
 */

#ifndef OSS_H
#define OSS_H

#define SECOND_TIMER 100
#define PROCESS_MAX 18

// Function prototypes
void helpMessage();
void killTimer();
void catchSegFault();
void doFork();
int checkArray();

#endif
