/*
********************************************************************************************************* 
* 						bill validator 
* 				(c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
* 						bill validator 
* Filename      : 
* Version       : V1.0
* Programmer(s) : TCY
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

#include "MDB.h"
#include "UART.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 
//HAL defines

//UART allocation
//CONSOLE_PORTA - CONSOLE_PORT 
//UART3A - UART2 
//UART2A - UART3 
//CONSOLE_PORTB - UART4 
//UART3B - UART5 
//UART2B - UART6

/*#define PB_DIV                 	8
#define PRESCALE               	256
#define MILLISECOND				SYS_FREQ/PB_DIV/PRESCALE/1000
#define SECOND					SYS_FREQ/PB_DIV/PRESCALE
*/

#define NOT_INIT	0
#define READY		1

#define NONE						0
#define NO_PENDING_CMD				0xFFFF

#define DESIRED_BAUDRATE    		(9600) 
#define MDB_PORT 					UART2
#define MDB_VECTOR 					_UART_2_VECTOR
	
#define NO_OF_MDB_EVENTS 			3
#define POLL_TIME					(SECOND)
#define RETRY_TIME					300*(MILLISECOND)

//states
#define IDLE_STATE			1
#define COMM_STATE	 		2

//events
#define COMM_NEXT			1
#define MESSAGE_RECEIVED	2
#define COMM_SUCCESSFUL		3
#define RETRY				4


//addresses
#define BILL_VALIDATOR_ADDRESS	0x30    //0x030 - all values are shifted by 1 to make room for address bit
#define COIN_CHANGER_ADDRESS	0x08     //0x08
#define MODE_BIT				0x100


//bill validator - commands
#define RESET 			0x00  //0x00 up to 0x07 - all values are shifted by 1 to make room for address bit
#define SETUP 			0x01
#define SECURITY 		0x02
#define POLL 			0x03
#define BILL_TYPE 		0x04
#define ESCROW 			0x05
#define STACKER 		0x06
#define EXPANSION 		0x07

//common and basic commands
#define ACK	 			0x00			
#define NAK				0xFF //0xFF
#define RET				0xAA //0xAA

//lengths of responses
#define SETUP_LENGTH	 	15

//positions on security setups
#define SECURITY_POSITION	8


#define BILL_TYPE_BITS     0b00001111
#define BILL_STATUS_BITS   0b01110000

//poll responses
#define BILL_STACKED	   					0
#define ESCROW_POSITION						1
#define BILL_RETURNED						2
#define BILL_TO_RECYCLER					3
#define DISABLED_BILL_REJECTED 				4
#define BILL_TO_RECYCLER_MANUAL_FILL 		5
#define MANUAL_DISPENSE						6
#define TRANSFERRED_FROM_RECYCLER			7

//setup response positions
#define BILL_TYPE_POS			11
#define CURRENCY_CODE_POS		1
#define BILL_SCALE_FACTOR_POS	3
#define LKR_CODE				0x1144

#define INIT_CMD_LENGTH 5

//coin commands
#define TUBE_STATUS		0x02
#define COIN_TYPE 		0x04
#define DISPENSE_COIN	0x05


//peripherlal bytes
#define SECURITY_HIGH_BYTE 0xFF //0
#define SECURITY_LOW_BYTE  0xFF //0

#define BILL_TYPE_ENABLE_HIGH_BYTE 			0x00
#define BILL_TYPE_ENABLE_LOW_BYTE  			0x00
#define BILL_TYPE_ESCROW_ENABLE_HIGH_BYTE 	0x00
#define BILL_TYPE_ESCROW_ENABLE_LOW_BYTE  	0x00

#define STACK_NOTE	0xFF
#define RETURN_NOTE	0x00

//amount of bytes to be received - without address bytes (have to add 1 if address byte)
#define SETUP_BYTES 		15
#define POLL_BYTES 			16
#define EXPANSION_BYTES 	29
#define STACKER_BYTES 		2
#define SECURITY_BYTES		1
#define BILL_TYPE_BYTES		1
#define ESCROW_BYTES		1
#define RESET_BYTES			1

//rember to raise error here
#define CHECK_BYTES(OF) 				\
	if(contentLength != OF##_BYTES)		\
	{									\
		mdbEnque(RETRY);				\
		return;							\
	}									\
	break;								\

#define NO_OF_BILLS	16

#define MDB_DEBUG

/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 

///////////Mdb state machine variables/////////////////////
unsigned char mdbQueue[NO_OF_MDB_EVENTS];
int mdbQueueHead;
int mdbQueueTail;
unsigned char mdbState;

////////////Mdb communication variables///////////////////
unsigned int mdbPacketTx[36];
unsigned int mdbPacketRx[36]; 

//////tyepdef for the struct having peripheral details///////////////////
typedef struct {
   unsigned int 	address;
   unsigned int 	status;
   unsigned char 	currentInit;   //current init command processing    
   unsigned int 	timeWithoutAns;
   unsigned int 	currencyType;
} peripheralDetails;

/////////////processing variables////////////////////////
unsigned int currentPeripheral; //current peripheral number
unsigned int currentCommand;    //current executing command
unsigned int pendingCommand;    //to store the next command to be executed

const unsigned int noOfPeripherals = 2;            //peripheral amount
peripheralDetails peripherals[2];    //peripheral details

unsigned int lengthRx = 0;  //lengthRx = 0 still has not received any
unsigned int lengthTx = 0;
unsigned int lastPacket = 0;   //logic not implemented
unsigned int contentLength;

const unsigned int billsAccepted[] = {20,50,100,500};
unsigned int billIndex[NO_OF_BILLS];
const unsigned int lengthBillsAccepted = 4;  //length of the above array
unsigned int sendBillsAccepted = 0;   //bills accepted mask
unsigned int mdbBillValue = 0;

const unsigned int initCommands[] = {RESET,POLL,SETUP,STACKER,BILL_TYPE};

/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 

////////////////////Queue management functions////////////////////////////////
uint8 mdbEnque(uint8 eventId);
uint8 mdbDeque();
uint8 mdbGetNext(uint8 index);

////////////////////HAL functions///////////////////////////////////////////
void startRetryTimer();
void closeRetryTimer();
void startPollTimer();
void closePollTimer();
MDB_SEND_9_DATA_PROTO(MDB_PORT);


///////////////////Mdb processing functions////////////////////////////
unsigned int preparePacket(unsigned int command,unsigned int unit);
void sendMdbPacket(unsigned int command,unsigned int unit);
unsigned int generateCheckSum(unsigned int length,unsigned int * buffer);
void receivePacket(unsigned int _9bitData);
void setStatus();
void processRxPacket();
void retry();
void getNextPeripheral();
void sendAck();



/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 



/* 
********************************************************************************************************* 
*                                        DEFINES 
********************************************************************************************************* 
*/ 





unsigned int preparePacket(unsigned int command,unsigned int unit){
	
	mdbPacketTx[0] = (command | unit | MODE_BIT); 
	unsigned int length = 1;
	
	currentCommand = command;
	if(unit == BILL_VALIDATOR_ADDRESS){
		switch(command){	
			case RESET: break;   //nothing to do 
			case SETUP: break;  //nothing to do
			case SECURITY: {
				mdbPacketTx[1] = SECURITY_HIGH_BYTE;
				mdbPacketTx[2] = SECURITY_LOW_BYTE;	
				length += 2;
				break;
			}	
			case POLL: break; //nothing to do
			case BILL_TYPE:{
				mdbPacketTx[1] = (sendBillsAccepted>>8);
				mdbPacketTx[2] = (sendBillsAccepted%256);
				mdbPacketTx[3] = mdbPacketTx[1];
				mdbPacketTx[4] = mdbPacketTx[2];
				length += 4;
				break;
			}	 
			case ESCROW:{
				if(canAcceptBills){
					mdbPacketTx[1] = STACK_NOTE;
				}
				else{
					mdbPacketTx[1] = RETURN_NOTE;
				}
				length += 1;
				pendingCommand = POLL;
				break;
			}	
			case STACKER: break;  //nothing to do
			case EXPANSION: break;  //see the sub command scene
			default:break;
		}
	}
	
	/*//coin commands - to be filled
	else if(unit == COIN_CHANGER_ADDRESS){
		switch(command){
			case RESET: break;
			case SETUP: break;
			case TUBE_STATUS: break;
			case POLL: break;
			case COIN_TYPE: break;
			case DISPENSE_COIN: break;
			case EXPANSION: break;
			default: break;
		}	
	}*/		
	
	return length;
}	

void sendMdbPacket(unsigned int command,unsigned int unit){
	
	unsigned int length = preparePacket(command,unit);
	unsigned int i = 0;
	#ifdef MDB_DEBUG
		hal_sendString_CONSOLE_PORT("tx");
	#endif
	for(;i<length;i++){
		hal_uartSend9bitData_MDB_PORT(mdbPacketTx[i]);
		#ifdef MDB_DEBUG
			hal_sendChar_CONSOLE_PORT(mdbPacketTx[i]);
		#endif
	}
	
	mdbPacketTx[i] = generateCheckSum(length,mdbPacketTx);
	hal_uartSend9bitData_MDB_PORT(mdbPacketTx[i]);
	#ifdef MDB_DEBUG
		hal_sendChar_CONSOLE_PORT(mdbPacketTx[i]);
	#endif
	lengthTx = length+1;	
	
}

void sendAck(){
	mdbPacketTx[0] = ACK;
	lengthTx = 1;
	#ifdef MDB_DEBUG
		hal_sendString_CONSOLE_PORT("tx");
		hal_sendChar_CONSOLE_PORT(mdbPacketTx[0]);
	#endif 
	hal_uartSend9bitData_MDB_PORT(mdbPacketTx[0]);
	
}

unsigned int generateCheckSum(unsigned int length,unsigned int * buffer){
	unsigned int i = 0;
	unsigned int checkSum = (*buffer);
	i++;
	for(;i<length;i++){
		checkSum += (*(buffer+i));
	}
	return ((checkSum)&0xFF);	
}

void receivePacket(unsigned int _9bitData){
	
	//algo - check for ACK or NAK first - also other commands like just reset etc. if yes raise events if it is what is wanted and return
	//otherwise check for the final packet - then call to process the packet
	unsigned int last = _9bitData & MODE_BIT;

	mdbPacketRx[lengthRx] = _9bitData;
	lengthRx++;
	
	unsigned int command = _9bitData ^ MODE_BIT;
	
	
	//first part of algo
	if((currentCommand == RESET || 
	   currentCommand == SECURITY || 
	   currentCommand == BILL_TYPE || 
	   currentCommand == ESCROW ||
	   currentCommand == POLL))
	{	
		if(command == ACK && last==MODE_BIT && lengthRx==1 ){ 
			mdbEnque(COMM_SUCCESSFUL);
			lengthRx = 0;
			setStatus();
			
			return;
		}  	  
		else if(command == NAK && last==MODE_BIT && lengthRx==1){
			mdbEnque(RETRY);
			lengthRx = 0;
			return;
		}
		/* else{
			mdbEnque(RETRY);
			return;
		}  */
	}
	else{
		if(command == ACK && last == MODE_BIT && lengthRx==1){
			lengthRx = 0;
			return;
		} 		
		else if(command == NAK && last == MODE_BIT && lengthRx==1 ){
			mdbEnque(RETRY);
			lengthRx = 0;
			return;
		} 	
	}
	
	//second part of algo
	if(last == MODE_BIT){
		contentLength = lengthRx - 1;  //-1 for the CHK byte
		processRxPacket();
		lengthRx = 0;
	}
	else{
		startRetryTimer();
	}	
	
}

void setStatus(){
	peripheralDetails * cur = peripherals + currentPeripheral;
	if(cur->status == NOT_INIT){
		if(cur->currentInit < INIT_CMD_LENGTH - 1){
			cur->currentInit = cur->currentInit + 1;
			currentCommand = initCommands[cur->currentInit];
			mdbEnque(COMM_NEXT);
		}
		else{
			cur->status = READY;
		}
		
		
	}
}



void processRxPacket(){
	
	//algo - first check for check sum if not raise exception
	//	   - next check for the number of bytes expected if not raise exception
	//	   - assign values for the variables
	//	   - raise event of successful reception 
	
	//checking checksum
	if( (mdbPacketRx[contentLength]^MODE_BIT) != generateCheckSum(contentLength,mdbPacketRx) ){
		#ifdef MDB_DEBUG
			hal_sendString_CONSOLE_PORT("cerror");
		#endif
		return;
	}	
	
	//checking the number of bytes received - remember to raise event in macro
	switch(currentCommand){
		
		case SETUP:{CHECK_BYTES(SETUP)}	
		case EXPANSION:{CHECK_BYTES(EXPANSION)}
		case STACKER:{CHECK_BYTES(STACKER)}
		case BILL_TYPE:{CHECK_BYTES(BILL_TYPE)}
		case ESCROW:{CHECK_BYTES(ESCROW)}
		case RESET:{CHECK_BYTES(RESET)}
		
	}
	
	//one for sure we have the correct data	- now process
	if(currentCommand == SETUP){
	
		unsigned int currencyCode = (unsigned int)((mdbPacketRx[CURRENCY_CODE_POS])<<8) + mdbPacketRx[CURRENCY_CODE_POS+1];
		unsigned int scalingFactor = (unsigned int)((mdbPacketRx[BILL_SCALE_FACTOR_POS])<<8) + mdbPacketRx[BILL_SCALE_FACTOR_POS+1];
		
		if(currencyCode != LKR_CODE){
			sendBillsAccepted = 0;
			return;
		}		
		
		unsigned int i;
		unsigned int j;
		sendBillsAccepted = 0;
		for(i=BILL_TYPE_POS;i<contentLength;i++){
			for(j=0;j<lengthBillsAccepted;j++){
				if( billsAccepted[j] == (mdbPacketRx[i])*(scalingFactor) ){
					sendBillsAccepted |= 0x01<<(i-BILL_TYPE_POS);
					billIndex[i-BILL_TYPE_POS] = j;
				}				
			}
		}
		
	}
	else if(currentCommand == POLL){
		//need to raise events depending on the response specially escrow
		unsigned int i=0;
		for(i=0;i<contentLength;i+=2){
			unsigned int bill = (mdbPacketRx[i] & BILL_TYPE_BITS);
			unsigned int statusOfBill = (mdbPacketRx[i] & BILL_STATUS_BITS)>>4;   //status error commands not implemented
			
			//checking for whether an acepted bill or not is bill validator's job - if it informs a bill is here it is counted
			
			if(statusOfBill == ESCROW_POSITION){
				pendingCommand = ESCROW;
			}
			else if(statusOfBill == BILL_STACKED){
				//give out the value to the vending machine
				mdbBillValue = billsAccepted[billIndex[bill]];
				enque(CASH_IN);
			}
			else if(statusOfBill == BILL_RETURNED){
				//nothing to do as at now
			}
		}
	}
	else if(currentCommand == EXPANSION){
	}
	else if(currentCommand == STACKER){
	}
	else{
		//RAISE EXP - INVALID COMMAND
	}
	
	mdbEnque(MESSAGE_RECEIVED);
}

void retry(){

	unsigned int i=0;
	for(i=0;i<lengthTx;i++){
		hal_uartSend9bitData_MDB_PORT(mdbPacketTx[i]);
	}	
	lengthRx = 0;
	
}

void getNextPeripheral(){
	if(currentPeripheral==noOfPeripherals-1){
		currentPeripheral = 0;
	}
	else{
		currentPeripheral++;
	}	
}

void mdbStateMachine(){

	uint8 eventId = mdbDeque();
	if(eventId){
		switch(mdbState){
		
			case IDLE_STATE:{
			
				if(eventId == COMM_NEXT){
					sendMdbPacket(currentCommand,peripherals[currentPeripheral].address);
					startRetryTimer();
					mdbState = COMM_STATE;
				}
				break;
				
			}
			case COMM_STATE:{
			
				if(eventId == MESSAGE_RECEIVED){
					sendAck();
					//lastPacket = 1;
					//closePollTimer();
					//startRetryTimer();
					mdbEnque(COMM_SUCCESSFUL);
					setStatus();
					lastPacket = 0;
				}
				else if(eventId == COMM_NEXT){
					mdbState = IDLE_STATE;
					mdbEnque(COMM_NEXT);
				}
				else if(eventId == COMM_SUCCESSFUL){
					if(pendingCommand != NO_PENDING_CMD){
						currentCommand = pendingCommand;
						mdbEnque(COMM_NEXT);
						pendingCommand = NO_PENDING_CMD;
					}
					else{
						startPollTimer();
					}
					mdbState = IDLE_STATE;
					#ifdef MDB_DEBUG
						hal_sendString_CONSOLE_PORT("over");
					#endif
				}
				else if(eventId == RETRY){
					retry();
				}
				
			}
		}
	}
}



////////////////////////////////////Hardware Abstraction Layer///////////////////////////////
void hal_mdbInit(void){
	
	mdbState = IDLE_STATE;
	UART_INIT(MDB_PORT,DESIRED_BAUDRATE,9_BITS)
	canAcceptBills = 1;
	
	peripherals[0].address 				= BILL_VALIDATOR_ADDRESS;
	peripherals[0].status  				= NOT_INIT;
	peripherals[0].timeWithoutAns 		= 0; 
	peripherals[0].currencyType 		= 0;
	peripherals[0].currentInit 			= 0;
	
	peripherals[1].address 		  		= COIN_CHANGER_ADDRESS;
	peripherals[1].status  		  		= NOT_INIT;
	peripherals[1].timeWithoutAns 		= 0; 
	peripherals[1].currencyType   		= 0;
	peripherals[1].currentInit 	  		= 0;
	
	currentPeripheral = 0;
	
	//hal_uartSend9bitData_MDB_PORT(0x123);
	
	startPollTimer();
				
	//legacy opening 
	//#define UBRG(baud) (((GetPeripheralClock())/4/(baud)-1)) 
	//OpenUART2(UART_EN | UART_NO_PAR_9BIT | UART_1STOPBIT | UART_MODE_SIMPLEX | UART_BRGH_FOUR, UART_RX_ENABLE | UART_TX_ENABLE, UBRG(DESIRED_BAUDRATE) );

}

void startRetryTimer(){

	OpenTimer4(T4_ON | T4_SOURCE_INT | T4_PS_1_256, RETRY_TIME);
	ConfigIntTimer4(T4_INT_ON | T4_INT_PRIOR_4);
	
}	

void closeRetryTimer(){
	
	CloseTimer4();	
	WriteTimer4(0);
	
}	

void startPollTimer(){

	OpenTimer5(T5_ON | T5_SOURCE_INT | T5_PS_1_256, POLL_TIME);
	ConfigIntTimer5(T5_INT_ON | T5_INT_PRIOR_4);
	
}	

void closePollTimer(){
	
	CloseTimer5();	
	WriteTimer5(0);
	
}	

//functions for sending 9 bit data
//function prototype - void hal_uartSend9bitData_##PORT (unsigned int no)
MDB_SEND_9_DATA(MDB_PORT)

//////////////////////MDB ISRs///////////////////////////////////////////////////

//uart isr
UART_INT(MDB_VECTOR, ipl2){
	if(INTGetFlag(INT_SOURCE_UART_RX(MDB_PORT))){
	    INTClearFlag(INT_SOURCE_UART_RX(MDB_PORT));	    
	    if (UARTReceivedDataIsAvailable(MDB_PORT))      
		{                
			//unsigned int temp =  ReadUART2();  -  legacy 
			UART_DATA data =  UARTGetData(MDB_PORT); 
			unsigned int temp = data.__data;      
			                
			//put an event and then handle 
			closeRetryTimer();
			closePollTimer();
			
			receivePacket(temp);
			#ifdef MDB_DEBUG
				hal_sendChar_CONSOLE_PORT(temp);
			#endif
			//mdbEnque(MESSAGE_RECEIVED);
		}
	}																								
	if ( INTGetFlag(INT_SOURCE_UART_TX(MDB_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(MDB_PORT));	    
	}
}

//timer ISR
void __ISR(_TIMER_5_VECTOR, ipl4) PollTimerIntHandler(void)
{
	mT5ClearIntFlag();

	mdbEnque(COMM_NEXT);	
	getNextPeripheral();
	if(peripherals[currentPeripheral].status == NOT_INIT){
		currentCommand = initCommands[peripherals[currentPeripheral].currentInit];
		pendingCommand = NO_PENDING_CMD;
	}
	else{
		currentCommand = POLL;
		pendingCommand = NO_PENDING_CMD;
	}
  
}

void __ISR(_TIMER_4_VECTOR, ipl4) RetryTimerIntHandler(void)
{
	mT4ClearIntFlag();
	if(!lastPacket){
		mdbEnque(RETRY);	
	}
	else{
		mdbEnque(COMM_SUCCESSFUL);
		setStatus();
		lastPacket = 0;
		startPollTimer();
	}
  
}

//////////////////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////
//Event queue management
/////////////////////////////////////////////////////////////////////////
uint8 mdbEnque(uint8 eventId){
  if(mdbGetNext(mdbQueueTail) == mdbQueueHead){
    return FALSE;
  }
  else{
    mdbQueue[mdbQueueTail] = eventId;
    mdbQueueTail = mdbGetNext(mdbQueueTail);
    return TRUE;
  }
}

uint8 mdbDeque(){
  if(mdbQueueHead == mdbQueueTail){
    return 0;
  }
  else{
    uint8 val = mdbQueue[mdbQueueHead];
    mdbQueueHead = mdbGetNext(mdbQueueHead);
    return val;
  }
}

     
uint8 mdbGetNext(uint8 index){
    if(index == NO_OF_EVENTS - 1){
      return 0;
    }
    else{
      return ++index;
    }
}



