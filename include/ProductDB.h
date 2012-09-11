/*
********************************************************************************************************* 
 *					 	Global header file 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						Product database 
 *
 * Filename      : ProductDB.h
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

#ifndef DB
#define DB
 
 
 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 


#include <p32xxxx.h> 
#include <plib.h>                   // include peripheral library function
#include "main.h"
#include "global.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

#define TABLE_SIZE 100  
#define WORD_SIZE 10 

//states
#define CONSOLE_UPDATE 0
#define WAIT 1
#define UPDATE 2

//events
#define FIN_TABLE 0
#define FILL_TABLE 1
#define END 2
#define NAME 3
#define NUMBER 4
#define AMOUNT 5
#define DEC 6
#define CENT 7
#define LENGTH 8 
/* 
********************************************************************************************************* 
*                                               DATA TYPES 
********************************************************************************************************* 
*/ 

typedef struct 
  {
   	uint32 name[WORD_SIZE];		//can enter 20 characters for a product name
    uint32 number;
    uint32 amount;
    uint32 valDec;
    uint32 valCent; 
  } Table;


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

Table tbl[TABLE_SIZE];		
extern uint32 update;
extern uint32 dataDB;
extern uint32 event;
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

void InitDB();
void addData(uint32 size,uint32 num,char name[],uint32 amount,uint32 valDec,uint32 valCent);
Table getTable(uint32 trayNum,uint32 num);
void flashDB();
void setTraySize(uint32 size);
void setNoOfTrays(uint32 num);
void process();
uint32 getVMSerial();

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
