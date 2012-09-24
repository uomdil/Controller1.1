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

#include "NFC.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

//port		
#define NFC_BAUDRATE	9600
#define NFC_PORT		UART5
#define NFC_VECTOR		_UART_5_VECTOR

//msg length
#define NFC_MSG_LENGTH		18
#define NFC_SERIAL_LENGTH	20

//NFC commands
#define RF_ON 			0x10
#define IDLE_REQUEST 	0x21
#define ANTI_COLLISION 	0x23
#define SELECT			0x24
#define RF_FIND_CARD	0x4C

//NFC polling states
#define RF_ON_STATE				1
#define IDLE_REQUEST_STATE 		2
#define ANTI_COLLISION_STATE	3
#define SELECT_STATE			4
#define RF_FIND_CARD_STATE		5



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
char NFCSerialBuffer[NFC_SERIAL_LENGTH];
unsigned int NFCSerialBufferFilled = 0;
char NFCType;
char NFCUIDBuffer[4];

char NFCmsg[NFC_MSG_LENGTH];


uint8 NFCPollingState =0;

/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 
//NFC timer functions
void startNFCTimer();
void closeNFCTimer();
void getNFCSerial();
void NFCStart();
void NFCStop();
void sendNFCCmd(uint8 cmd);

/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 

UART_SEND_CHAR(NFC_PORT)


void NFC_Init()
{
	UART_INIT(NFC_PORT,NFC_BAUDRATE,8_BITS)
	NFCStart();
	NFCPollingState = (uint8)RF_ON_STATE;
}


void pollNFC()
{
	switch(NFCPollingState){
		case (uint8)IDLE_REQUEST_STATE:
			sendNFCCmd((uint8)IDLE_REQUEST);
			if(NFCRxBufferFilled == 7){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){	
					NFCRxBufferFilled=0;
					NFCRetrys=0;
					NFCPollingState = (uint8)ANTI_COLLISION_STATE;
				}else{
					closeNFCTimer();
					NFCRxBufferFilled=0;
					enque(NFC_ERROR);
				}	
			}
		break;
		case (uint8)ANTI_COLLISION_STATE:
			sendNFCCmd((uint8)ANTI_COLLISION);
			if(NFCRxBufferFilled == 9){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){	
					NFCRxBufferFilled=0;
					NFCRetrys=0;
					NFCPollingState = (uint8)SELECT_STATE;
					uint8 i=0;
					for(i=0;i<4;i++){
						NFCUIDBuffer[i]=NFCRxBuffer[4+i];
					}
				}else{
					closeNFCTimer();
					NFCRxBufferFilled=0;
					enque(NFC_ERROR);
				}	
			}
		break;
		case (uint8)SELECT_STATE:
			sendNFCCmd((uint8)SELECT);
			if(NFCRxBufferFilled == 6){				
				if(NFCRxBuffer[3]== 0x00){	
					NFCRxBufferFilled=0;
					NFCRetrys=0;
					NFCPollingState = (uint8)RF_FIND_CARD_STATE;
				}else{
					closeNFCTimer();
					NFCRxBufferFilled=0;
					enque(NFC_ERROR);
				}	
			}
		break;
		case (uint8)RF_FIND_CARD_STATE:
			sendNFCCmd((uint8)RF_FIND_CARD);
			if(NFCRxBufferFilled >= 7){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){
					NFCRetrys=0;
					getNFCSerial();	
				}else{
					closeNFCTimer();
					NFCPollingState = 0;
					NFCRxBufferFilled=0;
					enque(NFC_ERROR);
				}	
			}
		break;	}
}


void getNFCSerial()
{
	NFCSerialBuffer[NFCSerialBufferFilled]=NFCRxBuffer[NFCRxBufferFilled-1];	
	NFCSerialBufferFilled++;
	if(NFCRxBufferFilled >= 10){
		uint8 i=0;
		uint8 fin =1;
		for(i=0;i<4;i++){
			if(NFCRxBuffer[NFCRxBufferFilled-4+i] != NFCUIDBuffer[i]){
				fin=0;
				break;
			} 
		}
		if(fin==1){
			NFCSerialBufferFilled = NFCSerialBufferFilled - 4;
			NFCType =NFCRxBuffer[4];
			closeNFCTimer();
			enque(NFC_GET_CONFIRM);
		}	
	}

}


char* genNFCmsg()  
{
	
	uint8 i=0;
	for(i=0;i<NFCSerialBufferFilled;i++)
	{
		NFCmsg[i] = NFCRxBuffer[i];
		//blah blah
	}
	
	NFCmsg[NFCSerialBufferFilled]='\r';
	NFCmsg[NFCSerialBufferFilled]='\0';

	return NFCmsg;
}

void sendNFCCmd(uint8 cmd){

	hal_sendChar_NFC_PORT(0x02);
	hal_sendChar_NFC_PORT(0x00);
	hal_sendChar_NFC_PORT(0x01);
	hal_sendChar_NFC_PORT(cmd);
	uint8 LRC = cmd^0x01;
	hal_sendChar_NFC_PORT(LRC);

}

void testNFC(){
	sendNFCCmd((uint8)RF_ON);
	if(NFCRxBufferFilled == 4){				
		if(NFCRxBuffer[3]== 0x00){	
			closeNFCTimer();
			enque(NFC_OK);
			NFCPollingState = (uint8)IDLE_REQUEST_STATE;
		}else{
			NFCRxBufferFilled=0;
			NFCPollingState = 0;
			enque(NFC_ERROR);
		}	
	}
}


//NFC timer -  timer 3

void NFCStart()
{
	startNFCTimer();
}


void NFCStop()
{
	closeNFCTimer();
}




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
	if(NFCRetrys >= NFC_RETRY_LIMIT){
		NFCStop();
		enque(NFC_UNIT_NOT_RESPONDING); //for the main state machine
	}
	else if(NFCPollingState == (uint8)RF_ON_STATE){
		NFCRetrys++;
		testNFC();
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


