/*
********************************************************************************************************* 
 *					 	GSM module 
 *
 * (c) Copyright 2012 D2NM, 
All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 					    GSM module
 *
 * Filename      : GSM.c
 * Version       : V1.0
 * Programmer(s) : TCY
 *
 * Note(s)		 : How to use the library
 *
 *				  Initializing
 *					1. call gsmInit();
 *					2. then check 'isGsmInitialized' variable before starting any operation regarding GSM unit
 *					3. call gsmStateMachine() in while(1) of the main function in every iteration
 *					
 *				  Sending an sms
 *					1. First setup the sms parameters using 'gsmSetSmsParameters(char* msg, unsigned int len)' function.
 *						Remember to put '\x1A' and '\0' as the ending sequence in the message.
 *					2. then enque the event by gsmEnque(SEND_SMS); - the sms will be sent now
 *
 *				  Receiving an sms
 *					1. No need to worry when to receive. It is assumed that SMS's of a particular format is only received to the GSM unit.
 *					   When the SMS is fully received the event 'GSM_UNIT_SMS_RECEIVED' will be enqued in the main state machine
 *					2. Read the extern variable 'gsmPaymentInfo' to get the SMS details
 *
 *				 events that are passed to the main state machine - you need to define them in the global.h
 *					1. GSM_UNIT_NOT_RESPONDING
 *					2. GSM_UNIT_ERROR
 *					3. GSM_UNIT_SMS_RECVD
 *
 *
 *
********************************************************************************************************* 
*/


 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 

#include "GSM.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

#define GSM_BIT			BIT_15
#define GSM_BAUDRATE	9600
#define GSM_PORT		UART4
#define GSM_VECTOR		_UART_4_VECTOR



#define PB_DIV                 	8
#define PRESCALE               	256
#define MILLISECOND				SYS_FREQ/PB_DIV/PRESCALE/1000
#define SECOND					SYS_FREQ/PB_DIV/PRESCALE

#define GSM_DEBUG


/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 

//for gsm commands and buffers
char gsmRxBuffer[300];
unsigned int gsmRxBufferFilled = 0;
unsigned int gsmCommandIndex = 0;
unsigned int gsmCurrentCommand = 0;

//internal status of the gsm unit
unsigned int gsmStatus[] = {0,0,0};

//for retrys
unsigned int gsmRetrys = 0;
char * gsmLastCommand;

//queue management
uint8 gsmState;
uint8 gsmEventQueue[GSM_NO_OF_EVENTS];
int gsmQueueHead;
int gsmQueueTail;

//sms sending and receiving parameters

//sending
char * gsmSmsMsg;
unsigned int gsmSmsLength;

//receiving
unsigned int gsmSmsNumber = 0;
unsigned int gsmReadSMS = 0;
unsigned int gsmSMSLine = 0;
char gsmSmsRecvCmdCharArray[] = {'A','T','+','C','M','G','R','=','0','0','0','\r','\0'};

//externs
char gsmPaymentInfo[4][20];
unsigned char isGsmInitialized;

//processing received
unsigned char rxCRLF = 0;
unsigned char startResponse = 0;



/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 

/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 

//utility functions
UART_SEND_CHAR_PROTO(GSM_PORT);
UART_SEND_STRING_PROTO(GSM_PORT);
unsigned char compareString(const char * string);

//processing functions
void gsmProcessPacket();
void gsmSendPacket(char * string);
void gsmSendInitCommands();
void gsmSendSMS();
void gsmRecvSMS();

//event management functions
uint8 gsmGetNext(uint8 index);
uint8 gsmDeque();


//GSM timer functions
void startGSMTimer();
void closeGSMTimer();


/* 
********************************************************************************************************* 
*                                        DEFINES 
********************************************************************************************************* 
*/ 


void gsmInit(){
	gsmState = GSM_INIT_STATE;
	gsmEnque(CHECK_MODEM);
	startResponse = 0;
	isGsmInitialized = FALSE;
	//mPORTDOpenDrainOpen(GSM_BIT);		//GSM
	UART_INIT(GSM_PORT,GSM_BAUDRATE,8_BITS)
		
}

//////////////////utility modem communication functions/////////////////////

UART_SEND_STRING(GSM_PORT)	//gsm - for sending strings

UART_SEND_CHAR(GSM_PORT)	//gsm - for sending unsigned chars

UART_INT(GSM_VECTOR, ipl2){  //gsm - UART interrupt

	if(INTGetFlag(INT_SOURCE_UART_RX(GSM_PORT)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(GSM_PORT));	    
	    if (UARTReceivedDataIsAvailable(GSM_PORT))      
		{                     
			closeGSMTimer();
			char tmpChar=UARTGetDataByte(GSM_PORT);
			//hal_sendChar_CONSOLE_PORT(tmpChar);
			//processing event - call gsm process packet 
			//<CR><LF> COMMAND <CR><LF>
			if(gsmReadSMS){
				if(tmpChar == '\r'){
					rxCRLF = 1;
				}
				else if(tmpChar == '\n' && rxCRLF == 1){
					gsmProcessPacket();
					rxCRLF = 0;
				}
				else{
					gsmRxBuffer[gsmRxBufferFilled] = tmpChar;
					gsmRxBufferFilled++;
				}
			}
			if(tmpChar == '\r'){
				rxCRLF = 1;
				return;
			}
			else if(tmpChar == '\n' && rxCRLF == 1){
				startResponse ^= 1;
				rxCRLF = 0;
				if(startResponse == 0){
					gsmProcessPacket();
					gsmRxBufferFilled = 0;
				}
				
				return;
			}
			else if(tmpChar == '>'){
				gsmEnque(OK_RECEIVED);
				gsmRxBufferFilled=0;
				rxCRLF=0;
				startResponse=0;
			}
			else if(rxCRLF == 1){
				rxCRLF = 0;
			}
			else if(startResponse == 1){
				gsmRxBuffer[gsmRxBufferFilled] = tmpChar;
				gsmRxBufferFilled++;
			}
			
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(GSM_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(GSM_PORT));	    
	}

}

unsigned char compareString(const char * string){
	unsigned char i = 0;
	while(*string != '\0'){
		if(gsmRxBuffer[i] != *string){
			return FALSE;
		}	
		i++;
		string++;
	}	
	return TRUE;
}

//GSM timer -  timer 2

void startGSMTimer(){
	OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, SECOND);
	ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_5);	
}	

void closeGSMTimer(){
	CloseTimer2();	
	WriteTimer2(0);
}	

void __ISR(_TIMER_2_VECTOR, ipl5) gsmIntHandler(void){
	mT2ClearIntFlag();
	if(gsmRetrys >= GSM_RETRY_LIMIT){
		//enque(GSM_UNIT_NOT_RESPONDING); //for the main state machine
	}
	else{
		gsmRetrys++;
		//gsmSendPacket(gsmLastCommand);
	}
	
}


//////////////////end - utility modem communication functions/////////////////////


//////////////////GSM unit processing functions/////////////////////

void gsmStateMachine(){

	uint8 gsmEventNext = gsmDeque();								
    if(gsmEventNext){
	
		switch(gsmState){
		
			case GSM_INIT_STATE: 
				
				if(gsmEventNext == CHECK_MODEM){
					gsmCurrentCommand = INIT_COMMANDS;
					gsmSendInitCommands();
				}
				else if(gsmEventNext == OK_RECEIVED){
					if( (gsmStatus[0] != 99 || gsmCommandIndex != 1) && 
						(gsmStatus[2] == 1 || gsmCommandIndex != 2) ){
						gsmCommandIndex++;
					}
					if(gsmCurrentCommand == INIT_COMMANDS){
						gsmEnque(CHECK_MODEM);
					}
				}
				else if(gsmEventNext == ERROR_RECEIVED){
						//enque(GSM_UNIT_ERROR); //for the main state machine
				}
				else if(gsmEventNext == GSM_INITIALIZED){
						gsmState = GSM_READY_STATE;
						isGsmInitialized = TRUE;
				}
				break;
				
			case GSM_READY_STATE: 
				
				if(gsmEventNext == SEND_SMS){
					//send the sms
					gsmCurrentCommand = SMS_SEND_COMMAND;
					gsmSendSMS();
				}
				if(gsmEventNext == RECV_SMS){
					gsmCurrentCommand = SMS_RECV_COMMAND;
					gsmRecvSMS();
				}
				else if(gsmEventNext == OK_RECEIVED){
					gsmCommandIndex++;
					if(gsmCurrentCommand == SMS_SEND_COMMAND){
						gsmEnque(SEND_SMS);
					}
					else if(gsmCurrentCommand == SMS_RECV_COMMAND){
						gsmEnque(RECV_SMS);
					}
				}
				else if(gsmEventNext == ERROR_RECEIVED){
					//enque(GSM_UNIT_ERROR); //for the main state machine
				}
				else if(gsmEventNext == SMS_RECEIVED){
					gsmEnque(RECV_SMS);
				}
				
				break;

			default: break;
		}
	}
}

void gsmProcessPacket(){
	if(compareString("OK")){
		gsmEnque(OK_RECEIVED);
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("OK");
		#endif
		gsmReadSMS = 0;
	}
	else if(compareString("ERROR")){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("ERROR");
		#endif
		gsmEnque(ERROR_RECEIVED);
	}
	else if(compareString("+CMTI: \"SM\"")){   // +CMTI:”SM”,3
		//get the number of the SMS
		unsigned int i=0;
		gsmSmsNumber = 0;
		for(i=12;i<gsmRxBufferFilled;i++){
			gsmSmsNumber = gsmSmsNumber*10 + (gsmRxBuffer[i] - '0');
		}
		gsmEnque(SMS_RECEIVED);
	}
	else if(compareString("+CMGR: ")){
		
		gsmReadSMS = 1;
	}
	else if(compareString("+CSQ: ")){
		gsmStatus[0] = (gsmRxBuffer[6] - '0')*10 + (gsmRxBuffer[7] - '0');
		unsigned int i=0;
		for(i=9;i<gsmRxBufferFilled;i++){
			gsmStatus[1] = gsmStatus[1]*10 + (gsmRxBuffer[i] - '0'); 
		}
	}
	else if(compareString("+CREG: ")){
		gsmStatus[2] =  (gsmRxBuffer[9] - '0');
	}
	else if(gsmReadSMS){
		unsigned int i=0;
		for(i=0;i<gsmRxBufferFilled;i++){
			gsmPaymentInfo[gsmSMSLine][i] = gsmRxBuffer[i];
		}
		gsmPaymentInfo[gsmSMSLine][i] = '\0';
		gsmSMSLine++;
		if(gsmSMSLine == 4) gsmReadSMS = 0;
	}
	gsmRxBufferFilled = 0;
}

void gsmSendPacket(char * string){
	hal_sendString_GSM_PORT(string);
	startGSMTimer();
}

void gsmSetSmsParameters(char* msg, unsigned int len){
	gsmSmsMsg = msg;
	gsmSmsLength = len;
}

void gsmSendInitCommands(){

	if(gsmCommandIndex == 0){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT\r");
		#endif
		gsmSendPacket("AT\r");
		gsmLastCommand = "AT\r";
	}
	else if(gsmCommandIndex == 1){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT+CSQ\r");
		#endif
		gsmSendPacket("AT+CSQ\r");
		gsmLastCommand = "AT+CSQ\r";
	}
	else if(gsmCommandIndex == 2){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT+CREG?\r");
		#endif
		gsmSendPacket("AT+CREG?\r");
		gsmLastCommand = "AT+CREG?\r";
	}
	else{
		gsmCommandIndex = 0;
		gsmCurrentCommand = NO_COMMAND;
		gsmEnque(GSM_INITIALIZED);
	}
}

void gsmSendSMS(){
	/*
		AT
		OK
		AT+CMGF=1
		OK
		AT+CMGW="+85291234567"
		> A simple demo of SMS text messaging.
		+CMGW: 1

		OK
	*/
	
	if(gsmCommandIndex == 0){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT\r");
		#endif
		gsmSendPacket("AT\r");
		gsmLastCommand = "AT\r";
	}
	else if(gsmCommandIndex == 1){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT+CMGF=1\r");
		#endif
		gsmSendPacket("AT+CMGF=1\r");
		gsmLastCommand = "AT+CMGF=1\r";
	}
	else if(gsmCommandIndex == 2){
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT("AT+CMGS=\"+94718196279\"\r");
		#endif
		gsmSendPacket("AT+CMGS=\"+94718196279\"\r");
		gsmLastCommand = "AT+CMGS=\"+94718196279\"\r";
	}
	else if(gsmCommandIndex == 3){
		//put ctrl z first - check this may need to put into an array
		/*char * tmpCharPtr = gsmSmsMsg;
		while(*tmpCharPtr != '\0'){
			tmpCharPtr++;
		}
		*tmpCharPtr = '\x1A';
		tmpCharPtr++;
		*tmpCharPtr = '\0';*/
		#ifdef GSM_DEBUG
			hal_sendString_CONSOLE_PORT(gsmSmsMsg);
		#endif
		char tmpCharArray[26];
		unsigned int i=0;
		char * tmpPtr = gsmSmsMsg;
		while(*tmpPtr!='\0'){
			tmpCharArray[i]=*tmpPtr;
			tmpPtr++;
			i++;
		}
		gsmSendPacket(gsmSmsMsg);
		gsmLastCommand = gsmSmsMsg;
		
	}
	else if(gsmCommandIndex == 4){
		gsmCommandIndex = 0;
		gsmCurrentCommand = NO_COMMAND;
	}
	
	
}

void gsmRecvSMS(){

	if(gsmCommandIndex == 0){   //read the sms
		unsigned int i=8;
		gsmSmsRecvCmdCharArray[6] = 'R';
		while(gsmSmsNumber != 0){
			unsigned int dec = gsmSmsNumber%10;
			gsmSmsNumber = gsmSmsNumber/10;
			gsmSmsRecvCmdCharArray[i]=(char)(dec+'0');
			i++;
		}
		if(i==8){   //the number is zero
			gsmSmsRecvCmdCharArray[i+1] = '\r';
			gsmSmsRecvCmdCharArray[i+2] = '\0';
		}
		else{
			gsmSmsRecvCmdCharArray[i] = '\r';
			gsmSmsRecvCmdCharArray[i+1] = '\0';
		}	

		gsmSendPacket(gsmSmsRecvCmdCharArray);
		gsmLastCommand = gsmSmsRecvCmdCharArray;
	}
	else if(gsmCommandIndex == 1){   //delete the sms
	
		enque(GSM_UNIT_SMS_RECVD);
		gsmSMSLine = 0;
		gsmSmsRecvCmdCharArray[6] = 'D';
		gsmSendPacket(gsmSmsRecvCmdCharArray);
		gsmLastCommand = gsmSmsRecvCmdCharArray;
		
	}
	else{
		gsmCommandIndex = 0;
		gsmCurrentCommand = NO_COMMAND;
	}
		
}

//////////////////////////////////////////////////////////////////////////////////
//Event queue management

uint8 gsmEnque(uint8 eventId){
  if(gsmGetNext(gsmQueueTail) == gsmQueueHead){
    return FALSE;
  }
  else{
    gsmEventQueue[gsmQueueTail] = eventId;
    gsmQueueTail = gsmGetNext(gsmQueueTail);
    return TRUE;
  }
}

uint8 gsmDeque(){
  if(gsmQueueHead == gsmQueueTail){
    return 0;
  }
  else{
    uint8 val = gsmEventQueue[gsmQueueHead];
    gsmQueueHead = gsmGetNext(gsmQueueHead);
    return val;
  }
}
    
uint8 gsmGetNext(uint8 index){
    if(index == GSM_NO_OF_EVENTS - 1){
      return 0;
    }
    else{
      return ++index;
    }
}











