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
#define NFC_BAUDRATE	115200
#define NFC_PORT		UART5
#define NFC_VECTOR		_UART_5_VECTOR

//msg length
#define NFC_MSG_LENGTH		18
#define NFC_SERIAL_LENGTH	10

//NFC commands
#define RF_ON 			0x10
#define IDLE_REQUEST 	0x21
#define ANTI_COLLISION 	0x23
#define SELECT			0x24
#define RF_FIND_CARD	0x4C
#define READ_CARD		0x27
#define REQ_AUTH		0x32

//NFC polling states
#define RF_ON_STATE				1
#define IDLE_REQUEST_STATE 		2
#define ANTI_COLLISION_STATE	3
#define SELECT_STATE			4
#define RF_FIND_CARD_STATE		5
#define READ_CARD_STATE			6
#define REQ_AUTH_STATE			7


//response index
#define RF_ON_IDX			0
#define IDLE_REQUEST_IDX 	2
#define ANTI_COLLISION_IDX 	4
#define SELECT_IDX			1
#define RF_FIND_CARD_IDX	2
#define READ_CARD_IDX		0
#define REQ_AUTH_IDX		4


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
char NFCType;
uint8 NFCUIDBuffer[4];
uint8 NFCDataBuffer[100];

char NFCmsg[NFC_MSG_LENGTH];

uint8 responseRcvd=0;

uint8 NFCPollingState =0;

uint8 responseArray[]={5,6,7,8,9};

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
void sendNFCCmd(uint8 cmd,uint8 data[],uint8 len) ;
UART_SEND_CHAR_PROTO(NFC_PORT);

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

void clearBuf(){
uint8 i=0;

for(i=0;i<12;i++)
{
NFCRxBuffer[i] = 0;
}

}


void pollNFC()
{
	switch(NFCPollingState){
		case (uint8)IDLE_REQUEST_STATE:
			if(NFCRxBufferFilled >= responseArray[IDLE_REQUEST_IDX]){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){	
					NFCRetrys=0;
					NFCPollingState = (uint8)ANTI_COLLISION_STATE;
				}
				startNFCTimer();	
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;
			}
		break;
		case (uint8)ANTI_COLLISION_STATE:
			if(NFCRxBufferFilled >= responseArray[ANTI_COLLISION_IDX]){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){	
					NFCRetrys=0;
					NFCPollingState = (uint8)SELECT_STATE;
					uint8 i=0;
					for(i=0;i<4;i++){
						NFCUIDBuffer[i]=NFCRxBuffer[4+i];
					}
				}else{
					NFCPollingState = IDLE_REQUEST_STATE;			
				}
				startNFCTimer();
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;	
			}
		break;
		case (uint8)SELECT_STATE:
			if(NFCRxBufferFilled >= responseArray[SELECT_IDX]){				
				if(NFCRxBuffer[3]== 0x00){
					NFCPollingState = (uint8)RF_FIND_CARD_STATE;
					NFCRetrys=0;
				}else{
					NFCPollingState = IDLE_REQUEST_STATE;
				}	
				startNFCTimer();
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;
			}
		break;
		case (uint8)RF_FIND_CARD_STATE:
			if(NFCRxBufferFilled >= responseArray[RF_FIND_CARD_IDX]){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){
					NFCRetrys=0;
					NFCPollingState =(uint8)REQ_AUTH_STATE;	
				}else{					
					NFCPollingState = IDLE_REQUEST_STATE;
				}
				startNFCTimer();
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;	
			}
		break;
		case (uint8)REQ_AUTH_STATE:
			if(NFCRxBufferFilled >= responseArray[REQ_AUTH_IDX]){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){
					NFCRetrys=0;					
					NFCPollingState = READ_CARD_STATE;	
				}else{					
					NFCPollingState = IDLE_REQUEST_STATE;
				}
				startNFCTimer();
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;	
			}
		break;
		case (uint8)READ_CARD_STATE:
			if(NFCRxBufferFilled >= responseArray[READ_CARD_IDX]){				//have to check the length of the response if failed
				if(NFCRxBuffer[3]== 0x00){
					NFCRetrys=0;
					closeNFCTimer();					
					NFCPollingState = (uint8)IDLE_REQUEST_STATE;	
					getNFCSerial();
					enque(NFC_GET_CONFIRM);
				}else{					
					NFCPollingState = IDLE_REQUEST_STATE;
				}
				responseRcvd =0;
				clearBuf();
				NFCRxBufferFilled=0;	
			}
		break;	
		}
		
		
}


void getNFCSerial()
{
	if(NFCRxBufferFilled >= 20){
		uint8 i=0;
		for(i=0;i<NFC_SERIAL_LENGTH;i++){
			 NFCSerialBuffer[i]=NFCRxBuffer[4+i];	
		}
	}
}



char* genNFCmsg()  
{
	
	uint8 i=0;
	for(i=0;i<NFC_SERIAL_LENGTH;i++)
	{
		NFCmsg[i] = NFCSerialBuffer[i];
	}
	NFCmsg[NFC_SERIAL_LENGTH]='\0';

	return NFCmsg;
}

void sendNFCCmd(uint8 cmd,uint8* data,uint8 len) {
	uint8 i=0;
	hal_sendChar_NFC_PORT(0x02);
	hal_sendChar_NFC_PORT(0x00);
	hal_sendChar_NFC_PORT((uint8)len);
	hal_sendChar_NFC_PORT((uint8)cmd);
	uint8 LRC = cmd^((uint8)len);
	if(len >1){
		for(i=0;i<len-1;i++){
			LRC = LRC^data[i];
			hal_sendChar_NFC_PORT(data[i]);
		}
	}
	hal_sendChar_NFC_PORT((uint8)LRC);

}


void testNFC(){
	
	if(NFCRxBufferFilled == responseArray[RF_ON_IDX]){			
		if(NFCRxBuffer[3]== 0x00){
			clearBuf();	
			enque(NFC_OK);
			NFCPollingState = (uint8)IDLE_REQUEST_STATE;
			NFCRetrys=0;
		}
		responseRcvd =0;	
		startNFCTimer();
		NFCRxBufferFilled=0;
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
	OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, 100*MILLISECOND);
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
		NFCRetrys=0;
		NFCPollingState = (uint8)IDLE_REQUEST_STATE;
		enque(NFC_UNIT_NOT_RESPONDING); //for the main state machine
	}
	else if(NFCPollingState == (uint8)RF_ON_STATE){
		NFCRetrys++;
		//responseRcvd =0;
		//testNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)RF_ON,NFCDataBuffer,1);	
	}
	else if(NFCPollingState == (uint8)IDLE_REQUEST_STATE){
		NFCRetrys++;
		//responseRcvd =0;
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)IDLE_REQUEST,NFCDataBuffer,1);
	}
	else if(NFCPollingState == (uint8)ANTI_COLLISION_STATE){
		NFCRetrys++;
		//responseRcvd =0;
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)ANTI_COLLISION,NFCDataBuffer,1);
	}
	else if(NFCPollingState == (uint8)SELECT_STATE){
		NFCRetrys++;
		//responseRcvd=0;
		uint8 i=0;
		for(i=0;i<4;i++){
			NFCDataBuffer[i]=NFCUIDBuffer[i];
		}	
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)SELECT,NFCDataBuffer,5);
	}
	else if(NFCPollingState == (uint8)RF_FIND_CARD_STATE){
		NFCRetrys++;
		//responseRcvd=0;
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)RF_FIND_CARD,NFCDataBuffer,1);
	}
	else if(NFCPollingState == (uint8)REQ_AUTH_STATE){
		NFCRetrys++;
		//responseRcvd=0;
		uint8 i=0;
		uint8 tmpDat[]={0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
		for(i=0;i<sizeof(tmpDat);i++){
			NFCDataBuffer[i]=tmpDat[i];
		}
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)REQ_AUTH,NFCDataBuffer,0x0A);
	}
	else if(NFCPollingState == (uint8)READ_CARD_STATE){
		NFCRetrys++;
		//responseRcvd=0;
		NFCDataBuffer[0]=0x01;
		//pollNFC();
		if(responseRcvd==0)
		sendNFCCmd((uint8)READ_CARD,NFCDataBuffer,2);
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
			responseRcvd =1;
			if(NFCPollingState == (uint8)RF_ON_STATE){
				closeNFCTimer();
				testNFC();
			}else{
				closeNFCTimer();
				pollNFC();
			}
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(NFC_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(NFC_PORT));	    
	}

}




