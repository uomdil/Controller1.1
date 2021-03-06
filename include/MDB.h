/*
********************************************************************************************************* 
 *					 	MDB header file 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						MDB controller 
 *
 * Filename      : billMDB.h
 * Version       : V1.0
 * Programmer(s) : 
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

#ifndef MDB
#define MDB
 
 
 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 


#include <plib.h>                   // include peripheral library function
#include "global.h"


/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 



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
extern unsigned int mdbBillValue;

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



void mdbStateMachine();
void hal_mdbInit(void);



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
