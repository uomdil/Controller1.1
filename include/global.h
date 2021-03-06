/*
********************************************************************************************************* 
 *					 	Global header file 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						Vending machine controller 
 *
 * Filename      : global.h
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

#ifndef GLOBAL
#define GLOBAL
   
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 


//#include <p32xxxx.h>		// include chip specific header file
#include <plib.h>           // include peripheral library functions


/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 


//for debug
//#define DEBUG


//for console
//#define SHOW_MESSAGES


#define ON  1 
#define OFF 0 


//clocks
#define SYS_FREQ 					(80000000)
#define	GetSystemClock() 			(80000000ul)
#define	GetPeripheralClock()		(GetSystemClock()/(1 << OSCCONbits.PBDIV))
#define	GetInstructionClock()		(GetSystemClock())

#define PB_DIV                 	1
#define PRESCALE               	256
#define MILLISECOND				SYS_FREQ/PB_DIV/PRESCALE/1000
#define SECOND					SYS_FREQ/PB_DIV/PRESCALE




//define state IDs of the controller

#define START				1
#define DIAGNOSTIC			2
#define INIT				3
#define UPDATE_VARS			4
#define WAIT_MONEY			5
#define WAIT_PRODUCT		6
#define WAIT_AMOUNT			7
#define SMS_PAY				8
#define DISPENSE			9
#define PAYMENT_METHOD		10
#define SYSTEM_LOCK			11
#define NFC_PAY				12


//define event IDs for controller

#define INIT_VARS			1
#define UPDATE_DATA			2
#define FIN					3
#define MORE_DATA			4
#define CASH_IN				5
#define COIN_IN				6
#define PRODUCT_NO			7
#define OK					8
#define CANCEL				9
#define ENTER_NO			10
#define TIME_OUT			11
#define WRONG				12
#define ERROR				13
#define SEND_ERROR			14
#define NFC_IN				15
#define NFC_GET_CONFIRM		16
#define NFC_SET_CONFIRM		17
#define FIRST_MOTOR			18
#define SECOND_MOTOR		19
#define THIRD_MOTOR			20
#define MOTOR_OK			21
#define NFC_INFO			22
#define NFC_CONFIRM			23  //not needed
#define DIAGNOSE			24
#define GSM_UNIT_SMS_RECVD	25
#define NFC_OK				26
#define DROP_SENSOR_WAIT	27

//special error events
#define GSM_ERROR				27
#define GSM_NOT_RESPONDING 		28
#define LOCK_DOWN				29
#define NFC_UNIT_NOT_RESPONDING	30
#define NFC_ERROR				31
#define MOTOR_ERROR				32


#define NO_OF_EVENTS	4

//NFC mssages
#define NFC_SERIAL_LENGTH	7


//define true and false

#ifndef false
	#define false	0
#endif
#ifndef true
	#define true 	1
#endif
#ifndef FALSE
	#define FALSE	0
#endif
#ifndef TRUE
	#define TRUE	1
#endif
#ifndef False
	#define False	0
#endif
#ifndef True
	#define True 	1
#endif



/* 
********************************************************************************************************* 
*                                               DATA TYPES 
********************************************************************************************************* 
*/ 


typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short int uint16;
typedef signed short int int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned char bool;


/* 
********************************************************************************************************* 
*                                               EXTERNS 
********************************************************************************************************* 
*/

extern uint32 key;
extern uint32 canAcceptBills;
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

uint8 enque(uint8 eventId);

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
