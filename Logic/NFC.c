/*
********************************************************************************************************* 
 *					 	NFC module 
 *
 * (c) Copyright 2012 D2NM, 
All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						NFC module 
 *
 * Filename      : NFC.c
 * Version       : V1.1
 * Programmer(s) : DIL
 *
 * Note(s)		 :
 *
 *
********************************************************************************************************* 
*/



 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 

#include "global.h"
#include "NFC.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

		
#define NFC_BAUDRATE	9600
#define NFC_PORT		UART5
#define NFC_VECTOR		_UART_5

/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 

//for retrys
unsigned int NFCRetrys = 0;

//uart data buffer
char NFCRxBuffer[100];
unsigned int NFCRxBufferFilled = 0;

//NFC data
char NFCDetailBuffer[100];
unsigned int NFCDetailBufferFilled = 0;
char NFCType;
char NFCUIDBuffer[4];


/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 
//NFC timer functions
void startNFCTimer();
void stopNFCTimer();
void closeNFCTimer();
void getNFCDetails();

/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 



void NFC_Init()
{
	UART_INIT(NFC_PORT,NFC_BAUDRATE,8_BITS)
	hal_sendChar_NFC_PORT(0x10);
}


void pollNFC()
{
	hal_sendChar_NFC_PORT(0x21);
	hal_sendChar_NFC_PORT(0x23);
	hal_sendChar_NFC_PORT(0x24);
	hal_sendChar_NFC_PORT(0x4C);
	if(NFCRxBufferFilled >= 1){
		if(NFCRxBuffer[0]== 0x00){	
			closeNFCTimer();
			getNFCDetails();
		}else{
			NFCRxBufferFilled=0;
			enque(NFC_ERROR);
		}	
	}
}


void getNFCDetails()
{
	NFCType =NFCRxBuffer[1];
	uint8 i=0;
	for(i=3;i<NFCRxBufferFilled-4;i++)
	{
		NFCDetailBuffer[NFCDetailBufferFilled] = NFCRxBuffer[i];
		NFCDetailBufferFilled++;
	}
	for(i=0;i<4;i++)
	{
		NFCUIDBuffer[i]= NFCRxBuffer[NFCRxBufferFilled-4+i];
	}
	enque(NFC_GET_CONFIRM);
	//if needed
	switch(NFCType){
	case 'M':
		
	break;
	case 'A':
		
	break;
	case 'B':
	
	break;
	
	}
}



char* genNFCmsg()  
{
	char NFCmsg[NFCDetailBufferFilled+6];
	uint8 i=0;
	for(i=0;i<NFCDetailBufferFilled;i++)
	{
		NFCmsg[i] = NFCRxBuffer[i];
		//blah blah
	}
	
	NFCmsg[NFCDetailBufferFilled]='\r';
	NFCmsg[NFCDetailBufferFilled]='\0';

	return NFCmsg;
}



void NFCStart()
{
	startNFCTimer();
}


void NFCStop()
{
	closeNFCTimer();
}


//NFC timer -  timer 3

void startNFCTimer(){
	OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, SECOND);
	ConfigIntTimer3(T3_INT_ON | T3_INT_PRIOR_5);	
}	

void closeNFCTimer(){
	CloseTimer3();	
	WriteTimer3(0);
}	

void __ISR(_TIMER_3_VECTOR, ipl5) NFCIntHandler(void){
	mT3ClearIntFlag();
	if(NFCRetrys >= RETRY_LIMIT){
		NFCStop();
		enque(NFC_UNIT_NOT_RESPONDING); //for the main state machine
	}
	else{
		NFCRetrys++;
		pollNFC();
	}
	
}

UART_INT(NFC_VECTOR, ipl2){  //nfc - UART interrupt

	if(INTGetFlag(INT_SOURCE_UART_RX(NFC_PORT)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(NFC_PORT));	    
	    if (UARTReceivedDataIsAvailable(NFC_PORT))      
		{                     
			stopNFCTimer();
			char tmpChar=UARTGetDataByte(NFC_PORT);
			 
			NFCRxBuffer[NFCRxBufferFilled] = tmpChar;
			NFCRxBufferFilled++;
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(NFC_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(NFC_PORT));	    
	}

}


