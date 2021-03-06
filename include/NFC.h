/*
********************************************************************************************************* 
 *					 	Global header file 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						GSM module 
 *
 * Filename      : NFC.h
 * Version       : V1.0
 * Programmer(s) : DIL
 *
 * Note(s)		 :
 *
 *
********************************************************************************************************* 
*/


/* 
********************************************************************************************************* 
*                                            MODULE
********************************************************************************************************* 
*/ 

#ifndef NFC
#define NFC
 
 
 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 


#include <plib.h>                   // include peripheral library function
#include "global.h"
#include "UART.h"
#include "skpic32_glcd.h"
/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 


#define NFC_RETRY_LIMIT				200


/* 
********************************************************************************************************* 
*                                               DATA TYPES 
********************************************************************************************************* 
*/ 




/* 
********************************************************************************************************* 
*                                               EXTERNS 
********************************************************************************************************* 
*/ 


/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 


/* 
********************************************************************************************************* 
*                                              MACROS 
********************************************************************************************************* 
*/ 



/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 
void NFC_Init();
char* genNFCmsg();
void NFCStart();
void closeNFCTimer();
/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 


/* 
********************************************************************************************************* 
*                                           MODULE END 
********************************************************************************************************* 
*/




#endif
