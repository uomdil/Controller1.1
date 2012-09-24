/*
********************************************************************************************************* 
 *					 	Vending machine controller 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						Vending machine controller 
 *
 * Filename      : main.c
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
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 

#include <p32xxxx.h>
#include <plib.h>                   // include peripheral library functions
#include "global.h"
#include "keys.h"
#include "Flash_Controller.h"
#include "skpic32_glcd.h"
#include "ProductDB.h"
#include "MDB.h"
#include "TrayMotorControl.h"
#include "GSM.h"
#include "NFC.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

//using keys
#define PRODUCT_SELECT 1	//use keys for product select
#define AMOUNT_SELECT 2	//use keys to select number of products
#define CHECK_METHOD 3  //use keys to select payment method


//sms/nfc pay
#define PIN_LENGTH 8
#define NFC_MSG_LENGTH 20


//#define MONEY_STATE 
//#define SMS_STATE  
//#define DEBUG 
//#define DUMMY_CURRNCY_VAL   // for 2000 val
#define DUMMY_DB_FLASH 

/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 



//queue management
uint8 state;
uint8 g_EventQueue[NO_OF_EVENTS];
int queueHead;
int queueTail;

//for keys
uint8 keyPressCount=0;  					//count pressing keys
uint8 keyuse;						//keypad mode: either entering product no or 

//product details
uint32 product_no=0;				//to store entered product no
uint32 amount=0;					//to store entered prosuct amount
uint32 total=0;
int balance=0;


//mdb commd
uint32 enteredValue=0;
uint32 canAcceptBills=0;


//motor
uint8 motor_no=0;

//GSM
uint32 VMSerial; 
char globalMsg[20];
char vendingMachineSerial[PIN_LENGTH];

char errorMsg[50];
uint8 motorTestPass = 0;
uint32 timerCount = 0;

/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 

//init
void board_init();

//FSM
void stateMachine(uint8 eventId);
void changeState(uint8 nextStateId);
void onStateExit(uint8 stateId);
uint8 deque();
void onStateEntry(uint8 stateId);
uint8 getNext(uint8 index);

//GSM
char* genSMSPIN();

//FSM helper functions
void showTotalValue();
void showProductNameAndValue();
void handlePaymentMethod();
void handleSMSPayment();	 
char* sendErrorMsg(char* msg);

//timer
void startWaitTimer();
void closeWaitTimer();


//diagnose
void diagnoseSystem();


/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 

#pragma config FNOSC 	= PRIPLL       	// Internal Fast RC oscillator (8 MHz) w/ PLL  //FRCPLL 
#pragma config FPLLIDIV = DIV_2    		// Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL  = MUL_20     	// PLL Multiply (now 80 MHz)
                                    
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_8         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = XT            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
#pragma config DEBUG    = OFF           // Debugger nabled for Starter Kit



int main(void)
{
	board_init();												//Initialize the circuit board configurations
  	changeState(START);											//Initialize the state
  	enque(DIAGNOSE);											//first do diagnostic

  	/*#ifdef SHOW_MESSAGES
	    hal_sendString_UART1("Machine started");
	    hal_sendChar_UART1('\n');
    #endif
  	*/
  	
  	INTEnableInterrupts();
  	INTEnableSystemMultiVectoredInt();
	
	#ifdef DUMMY_DB_FLASH
		setTraySize(3);
	    setNoOfTrays(2);
		char name[]="soda";
		addData(1,1,name,5,40,0);
		char name2[]="banana";
		addData(1,2,name2,3,20,0);
		
		flashDB();
	#endif
	
  	while(1){
	  	
    	uint8 nextEvent = deque();								//get next event
    	if(nextEvent)
	    {
     		stateMachine(nextEvent);							//enter the event functions
    	}
  
  		if(state==UPDATE_VARS)
  		{
  			fsm_update();
  		}
  		
    	if(keyuse ==(uint8)PRODUCT_SELECT)								//check key press for product selection
     	{	    
     		keypad_pole();
     	}
     	else if(keyuse ==(uint8)AMOUNT_SELECT)							//check key press for amount selection
     	{
     		keypad_pole();	
     	}
     	else if(keyuse ==(uint8)CHECK_METHOD)
     	{
     		keypad_pole();	
   		}  		
     	
     	//mdbStateMachine();
     	//gsmStateMachine();

     	
  	}	
  	
}


void board_init(){
	
	DDPCONbits.JTAGEN = 0;				//disable JTAG
	DBINIT();     						// Initialize the IO channel
	hal_allUARTInit();					//Initialize all UARTs
	keypad_init();						//Initialize keypad
	Disp_Init();
	Disp_GLCDInit();
	//gsmInit();							//GSM init
	//hal_mdbInit();					//MDB Init
	//NFC_Init();						//NFC Init
	InitDB();							//product database init
}



void stateMachine(uint8 eventId){
  switch(state){
    case START :
    	if(eventId == (uint8)DIAGNOSE){
	    	enque(INIT_VARS);
    	    changeState(DIAGNOSTIC);
    	}          	
     break;
    
    case DIAGNOSTIC:
      	if(eventId == (uint8)INIT_VARS){
	      	diagnoseSystem();
	      	enque(PRODUCT_NO); 			//for test only
    	    changeState(INIT);  
    	}    
    	else if(eventId == (uint8)ERROR){	    	
	    	enque(SEND_ERROR);
    	    changeState(SYSTEM_LOCK);
    	}
    
     break;
    
    case INIT:
      	if(eventId == (uint8)PRODUCT_NO){
    	    changeState(WAIT_PRODUCT);    
    	}
   		else if(eventId == (uint8)UPDATE_DATA){		
   	    	changeState(UPDATE_VARS);	
    	}
    
     break;
    
    case UPDATE_VARS :
    	
      	if(eventId == (uint8)FIN){
    	    changeState(INIT);  
    	}    
   
     break;
    
    case WAIT_PRODUCT:
      	if(eventId == (uint8)OK){
	      	#ifdef DEBUG
				hal_sendString_UART1("Got product");
				hal_sendChar_UART1('\n');
			#endif
			if(tbl[product_no].amount==0 || tbl[product_no].amount == 0xFFFFFFFF)
			{
				Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"UNAVAILABLE");
				DelayMs(200);
				enque(CANCEL);
			}else if(product_no==0)
			{
				Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"UNAVAILABLE");
				DelayMs(100);
				enque(CANCEL);
			}
			else
			{
    	    	changeState(WAIT_AMOUNT);
   			} 	      
    	}    
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    	if(keyPressCount<3 && key<10){
		 		product_no=product_no*10+key;
		 		Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"Entered Value");
				
				Disp_GLCDNumber(product_no,1,3,1);

		 		#ifdef DEBUG
					hal_sendString_UART1("product no = ");
					hal_uartWriteNumber(product_no);
					hal_sendChar_UART1('\n');
				#endif
		 	}
    	}
    	else if(eventId == (uint8)CANCEL){
	    	product_no=0;
	    	enque(PRODUCT_NO);
    	    changeState(INIT);	
    	}
    	
      
     break;
    
    case WAIT_AMOUNT:
    	if(eventId == (uint8)OK){
	    	#ifdef DEBUG
				hal_sendString_UART1("Got amount");
				hal_sendChar_UART1('\n');
			#endif
			showTotalValue(); 	    
    	}    
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    	if(keyPressCount<3 && key<10){	
		 		amount=amount*10+key;
		 		Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"Amount Entered");
		 		
		 		Disp_GLCDNumber(amount,1,3,1);
		 		
				showProductNameAndValue();		
	 			
		 		#ifdef DEBUG
					hal_sendString_UART1("amount = ");
					hal_uartWriteNumber(amount);
					hal_sendChar_UART1('\n');
				#endif
		 	}
    	}
    	else if(eventId == (uint8)CANCEL){
	    	amount=0;
	    	product_no=0;
    	    changeState(WAIT_PRODUCT);  
    	}
    
     break;
    
    case PAYMENT_METHOD:
    	if(eventId == (uint8)CANCEL){
	    	amount=0;
	    	product_no=0;
	    	total =0;
	    	
	    	enque(PRODUCT_NO);
    	    changeState(INIT);  
    	}    
    	else if(eventId == (uint8)OK){
    	    changeState(WAIT_MONEY);
    	}
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    	handlePaymentMethod();
	    }	
    	else if(eventId == (uint8)ERROR){
	    	enque(SEND_ERROR);
    	    changeState(SYSTEM_LOCK);
    	}
      
     break;
    
    case WAIT_MONEY:   	    
    	if(eventId == (uint8)COIN_IN){
    	}
    	else if(eventId == (uint8)CASH_IN){
	    	enteredValue=enteredValue+mdbBillValue;
	    	balance=enteredValue-total;
	    	if(balance<0){
	    		Disp_GLCDWriteText(0,2,"Entered = ");			
	 			Disp_GLCDNumber(enteredValue,2,5,1);				
	    	}else{
	    		Disp_GLCDWriteText(0,2,"Entered = ");			
	    		Disp_GLCDNumber(enteredValue,2,5,1);
	 			Disp_GLCDWriteText(0,3,"Press Enter");
	 			canAcceptBills=0;							//stop bill accepting
	 			
	 		#ifdef MONEY_STATE
	    		enque(OK);
    		#endif 
	    	}
    	}
    	
    	else if(eventId == (uint8)CANCEL){
	    	amount=0;
	    	product_no=0;
	    	enteredValue =0;
	    	enque(PRODUCT_NO);
    	    changeState(INIT);
    	}
    	else if(eventId == (uint8)TIME_OUT){
	    	
    	}
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    }
	    else if(eventId == (uint8)OK){
		    #ifdef DUMMY_CURRNCY_VAL
		    	enteredValue = 2000;
		    #endif
		    balance=enteredValue-total;
	    	if(balance>=0){
		    	Disp_GLCDClearDisp();
		    	Disp_GLCDWriteText(0,1,"Please wait");
		    	changeState(DISPENSE);
		    	enque(FIRST_MOTOR);
		    }	
	    }	
    	
     break;
    
    case SMS_PAY:
    	if(eventId == (uint8)OK){
    	    changeState(DISPENSE);  
    	    enque(FIRST_MOTOR);
    	}  
    	else if(eventId == (uint8)GSM_UNIT_ERROR){
	    	Disp_GLCDClearDisp();
		    Disp_GLCDWriteText(0,0,"GSM ERROR");
		    //should stop sms paynment 
    	}
    	else if(eventId == (uint8)GSM_UNIT_NOT_RESPONDING){
	    	Disp_GLCDClearDisp();
		    Disp_GLCDWriteText(0,0,"GSM NO RESPONSE");
		    //should stop sms paynment 
    	}
    	else if(eventId == (uint8)GSM_UNIT_SMS_RECVD){
			handleSMSPayment();	    	
    	}
    	else if(eventId == (uint8)CANCEL){
	    	amount=0;
	    	product_no=0;
	    	enteredValue =0;
	    	enque(PRODUCT_NO);
    	    changeState(INIT);
    	}
    	
     break;
  
    case NFC_PAY:
    	if(eventId == (uint8)NFC_INFO){
			NFCStart();
     	}
     	else if(eventId == (uint8)NFC_GET_CONFIRM) {
     		char* NFCmsg=genNFCmsg();    	
	      	gsmSetSmsParameters(NFCmsg,18);
	      	gsmEnque(SEND_SMS);
     	}
     	else if(eventId == (uint8)NFC_CONFIRM) {
     		changeState(DISPENSE);
     	}
     	else if(NFC_ERROR){
     		amount=0;
	    	product_no=0;
	    	enteredValue =0;
	    	Disp_GLCDClearDisp();
		    Disp_GLCDWriteText(0,0,"GSM ERROR");
		    //should stop nfc paynment 
     	}
     	else if(CANCEL){
     		amount=0;
	    	product_no=0;
	    	enteredValue =0;
	    	enque(PRODUCT_NO);
    	    changeState(INIT);
     	}		
     break;
    
    case DISPENSE:
      	if(eventId == (uint8)INIT_VARS){
    	    changeState(INIT);  
    	}    
    	else if(eventId == (uint8)ERROR){
	    	sendErrorMsg("SENSOR NOT WORKING");
	    	enque(LOCK_DOWN);
			changeState(SYSTEM_LOCK);
    	}
    	else if(eventId == (uint8)MOTOR_ERROR){
	    	sendErrorMsg("MOTOR NOT WORKING"); //add motor no
	    	enque(LOCK_DOWN);
			changeState(SYSTEM_LOCK);
    	}
    	else if(eventId == (uint8)FIRST_MOTOR){
    		VendMotor1(product_no+0x97);
    		
    		//Disp_GLCDClearDisp();
	    	//Disp_GLCDWriteText(0,1,"motor 1");
	    	//DelayMs(1000);
    	}
    	else if(eventId == (uint8)SECOND_MOTOR){
    		VendMotor2();
    		
    		//Disp_GLCDClearDisp();
	    	//Disp_GLCDWriteText(0,1,"motor 2");
	    	//DelayMs(1000);
    	}
    	else if(eventId == (uint8)THIRD_MOTOR){
    		VendMotor3();
    		
    		//Disp_GLCDClearDisp();
	    	//Disp_GLCDWriteText(0,1,"motor 3");
	    	//DelayMs(1000);
    	}
    	else if(eventId == (uint8)MOTOR_OK){
    		motor_no++;
    		if(motor_no==1){
	    		enque(SECOND_MOTOR);
	    		
	    		//Disp_GLCDClearDisp();
	    		//Disp_GLCDWriteText(0,1,"motor1 resp");
	    	}
	    	else if(motor_no==2){
	    		enque(THIRD_MOTOR);
	    		
	    		//Disp_GLCDClearDisp();
	    		//Disp_GLCDWriteText(0,1,"motor2 resp");
	    		
	    	}	
	    	else if(motor_no==3){
    			tbl[product_no].amount=tbl[product_no].amount-amount;
    			flashDB();
    			
    			//Disp_GLCDClearDisp();
	    		//Disp_GLCDWriteText(0,1,"motor1 resp");
	    		
    			motor_no=0;
    			enque(PRODUCT_NO);
    			changeState(INIT);
    		}
    	}
    	
     break;
      
    case SYSTEM_LOCK:
    	if(eventId == (uint8)SEND_ERROR){
 			gsmSetSmsParameters(errorMsg,18);
	      	gsmEnque(SEND_SMS);
    	} 
    	else if(eventId == (uint8)LOCK_DOWN){
    	      
    	}
    	
    	
    	
     break;
         
    default:
    
     break;
    
  }
}




void changeState(uint8 nextStateId){
  if(nextStateId != state){
    onStateExit(state);
  }
  state = nextStateId;
  onStateEntry(nextStateId);
}

void onStateEntry(uint8 stateId){
  switch(stateId){
    case START :
          	
      break;
    
    case DIAGNOSTIC:
      
	
    break;
    
    case INIT:
		Disp_GLCDClearDisp();
		DelayMs(10);
    	Disp_GLCDWriteText(2, 0, "WELCOME");
    	DelayMs(100); 			//long delay
		amount=0;
	    product_no=0;
    break;
    
    case UPDATE_VARS :
      
   
    break;
    
    case WAIT_MONEY:
	    Disp_GLCDClearDisp();
	    Disp_GLCDWriteText(0, 0, " INSERT MONEY");
	    Disp_GLCDWriteText(0,1,"Total = ");
		
		Disp_GLCDNumber(total,1,3,1);
		keyuse=PRODUCT_SELECT;
		
		canAcceptBills=1;
    break;
    
    case WAIT_PRODUCT:
	    Disp_GLCDClearDisp();
		Disp_GLCDWriteText(0, 0, "INSERT PRODUCT");
		Disp_GLCDWriteText(0, 1,"NUMBER");
	    keyPressCount=0; 
	    keyuse=PRODUCT_SELECT;     
      	
      	#ifdef MONEY_STATE
    		changeState(WAIT_MONEY);		
    		total=100;
    		amount=2;
    		product_no =1;
    		enque(OK);
    	#endif 
    	
    	
    	#ifdef SMS_STATE
    		total =500;
    		changeState(SMS_PAY);  
    		//enque(OK); 
    				
    	#endif
      	
      	
    break;
    
    case WAIT_AMOUNT:
	    Disp_GLCDClearDisp();
		DelayMs(20);
	    Disp_GLCDWriteText(0, 0, " ENTER QUANTITY");
		DelayMs(50);				//long delay
	    keyuse==AMOUNT_SELECT; 
	    keyPressCount=0; 
    break;
    
      
    case DISPENSE:
      
      break;
    
    case PAYMENT_METHOD:
    	
    	Disp_GLCDClearDisp();
	    Disp_GLCDWriteText(0, 0, "PAYMENT METHOD");
    	Disp_GLCDWriteText(0, 1, "1: CURRENCY");
    	Disp_GLCDWriteText(0, 2, "2: SMS");
    	Disp_GLCDWriteText(0, 3, "3: NFC");
    
    	keyuse=CHECK_METHOD;
    break;
    
    case NFC_PAY:
      	Disp_GLCDClearDisp();
		Disp_GLCDWriteText(0, 0, "USE NFC CARD");
      break;
      
    case SMS_PAY:
      	Disp_GLCDClearDisp();
	    Disp_GLCDWriteText(0, 0, "PIN NUMBER");
	    char* serial=genSMSPIN();
	    Disp_GLCDWriteText(1, 1, serial);
	    Disp_GLCDWriteText(0, 2, "SMS PIN to");
	    Disp_GLCDWriteText(0, 3, "4565");	//given from mobitel
	    
      break;
    
      
    case SYSTEM_LOCK:
      
      break;
      
    default:
    
      break;
    
  }
}

void onStateExit(uint8 stateId){
  
  switch(stateId){
    case START :
    
          	
      break;
    
    case DIAGNOSTIC:
      
	
      break;
    
    case INIT:
      
 
    
      break;
    
    case UPDATE_VARS :
      
   
      break;
    
    case WAIT_MONEY:
    keyuse=0;
    
      break;
    
    case WAIT_PRODUCT:
		keyPressCount=0; 
      
    break;
    
    case WAIT_AMOUNT:
		keyPressCount=0;
    
    break;
    
      
    case DISPENSE:
      	balance=0;
      	total=0;
      	enteredValue=0;
    break;
    
    case PAYMENT_METHOD:
      	keyuse = 0;
      break;
      
    case SYSTEM_LOCK:
      
      break;
      
    case NFC_PAY:
      
      break;
         
    default:
    
      break;
   
  }
  
}



//////////////////////////////////////////////////////////////////////////////////
//Event queue management

uint8 enque(uint8 eventId){
  if(getNext(queueTail) == queueHead){
    return FALSE;
  }
  else{
    g_EventQueue[queueTail] = eventId;
    queueTail = getNext(queueTail);
    return TRUE;
  }
}

uint8 deque(){
  if(queueHead == queueTail){
    return 0;
  }
  else{
    uint8 val = g_EventQueue[queueHead];
    queueHead = getNext(queueHead);
    return val;
  }
}

     
uint8 getNext(uint8 index){
    if(index == NO_OF_EVENTS - 1){
      return 0;
    }
    else{
      return ++index;
    }
}


//fsm helper functions

//diagnostic helper functions
void diagnoseSystem(){
	
	//motor test
	
	uint8 i=0;
	uint8 sendCommand=0;
	uint8 errorMotors[getNoOfTrays()*getNoOfTrays()];
	startWaitTimer();
	while(i<getNoOfTrays()*getNoOfTrays()){
		if(sendCommand == 0){
			testMotor(i+0x96);
			sendCommand =1;
		}
		if(motorTestPass == 1){
			timerCount =0;
			i++;
			sendCommand = 0;
		}else if(timerCount>5){
			errorMotors[i] = 1;
			i++;
			timerCount =0;
			sendCommand = 0;
		}
	}	
	closeWaitTimer();
	
	//mdb test
	
	
	
	//gsm test
	startWaitTimer();
	while(isGsmInitialized){
		if(timerCount>8){
			Disp_GLCDClearDisp();
    		Disp_GLCDWriteText(0, 0, "GSM NOT WORKING");
    		DelayMs(300);
			break;
		}
	}
	
	
}


void setMotorTestPass(){
	motorTestPass =1;
}



//gsm helper functions


char* sendErrorMsg(char* msg){
	uint8 i=0;
   	for(i=0;i<sizeof(msg);i++){
   		errorMsg[i]=msg[i];
   	}
   	
   	gsmSetSmsParameters(errorMsg,18);
	gsmEnque(SEND_SMS);
}



//product purchasing mechanism helper functions


char* genSMSPIN(){
	VMSerial=getVMSerial();
	uint8 i=0;
	for(i=0;i<4;i++){
		vendingMachineSerial[i]=VMSerial%26+'A';
		VMSerial = VMSerial/26;
	}
	uint32 tot=total;
	for(i=0;i<3;i++){
		vendingMachineSerial[i+4]=tot%26+'A';
		tot = tot/26;
	}
	vendingMachineSerial[7]='\0';
	return vendingMachineSerial;
}



void showTotalValue(){
	if(tbl[product_no].amount<amount)
	{
		Disp_GLCDClearDisp();
		Disp_GLCDWriteText(0,0,"AMOUNT EXCEEDED");
		DelayMs(100);
		amount=0;
		changeState(WAIT_AMOUNT);
	}
	else if(amount==0)		//if the amount wasnt entered
	{
		amount=1;
		total=amount*tbl[product_no].valDec;
		Disp_GLCDClearDisp();
		Disp_GLCDWriteText(0,0,"Total = ");
		
		Disp_GLCDNumber(total,0,4,1);
		DelayMs(300);	 			
	 	changeState(PAYMENT_METHOD); 
	}
	else 
	{
		total=amount*tbl[product_no].valDec;
		Disp_GLCDClearDisp();
		Disp_GLCDWriteText(0,0,"Total = ");
		
		Disp_GLCDNumber(total,0,4,1);
		DelayMs(300);
		changeState(PAYMENT_METHOD);  
	}
}


void showProductNameAndValue(){
	uint32 *nm=tbl[product_no].name;
	uint8 i=0;
	
	for(i=0;i<WORD_SIZE/2;i++){
		char tmp = *nm;
		if((tmp>='a' && tmp <='z' )|| (tmp>='A' && tmp<='Z')){
			Disp_GLCDWrite(i,2,*nm);
			nm++;
		}
		tmp = *nm;	
		if((tmp>='a' && tmp <='z' )|| (tmp>='A' && tmp<='Z')){	
			Disp_GLCDData(*nm);
			nm++;
		}
	}
	
	uint32 valD=tbl[product_no].valDec;
	uint32 valC=tbl[product_no].valCent;
	Disp_GLCDWriteText(0,3,"Price:");
	Disp_GLCDNumber(valD,3,4,1); 	
}


void handlePaymentMethod(){
	Disp_GLCDClearDisp();
    Disp_GLCDWriteText(0, 0, "PAYMENT METHOD");
   	Disp_GLCDWriteText(0, 1, "1: CURRENCY");
   	Disp_GLCDWriteText(0, 2, "2: SMS");
   	Disp_GLCDWriteText(0, 3, "3: NFC");
   	if(key==1)
    {
   		changeState(WAIT_MONEY);
   	}
   	else if(key==2)
   	{
    	if(isGsmInitialized == 0){
  			enque(ENTER_NO);
  			Disp_GLCDClearDisp();
    		Disp_GLCDWriteText(0, 0, "SMS DISABLED");
    		DelayMs(200);
    		key=0;
 		}
    	else{
   			changeState(SMS_PAY);
   		}	
   	}	
   	else if(key==3)
   	{
 		if(isGsmInitialized == 0){
			enque(ENTER_NO);
			Disp_GLCDClearDisp();
    		Disp_GLCDWriteText(0, 0, "NFC DISABLED");
    		DelayMs(200);
    		key=0;
 		}
   		else{
    		enque(NFC_INFO);
   			changeState(NFC_PAY);
   		}
   	}
}

void handleSMSPayment(){
	Disp_GLCDClearDisp();
	char smsState=gsmPaymentInfo[0][0];
	
	uint8 i=0;
   	uint8 pinMatch=1;
   	for(i=0;i<PIN_LENGTH;i++){
    	if(vendingMachineSerial[i] != gsmPaymentInfo[3][i]){
    		pinMatch=0;
    		break;
    	}
   	}   	
   	
   	if(smsState =='1' && pinMatch ==1){
	    	Disp_GLCDWriteText(0, 0,"SUCCESS");
	    	Disp_GLCDWriteText(0, 1,"Initials: ");
	    	Disp_GLCDWriteText(5, 1, gsmPaymentInfo[1]);
	    	Disp_GLCDWriteText(0, 2,"Payment: ");				
	    	Disp_GLCDWriteText(5, 2, gsmPaymentInfo[2]);		
	    	Disp_GLCDWriteText(0, 3,"TID No: ");
	    	Disp_GLCDWriteText(4, 3, gsmPaymentInfo[3]);
    		DelayMs(500);
    		enque(OK);
   	}
   	else if(smsState == '1' && pinMatch ==0){
   		Disp_GLCDWriteText(0, 0,"PIN MISMATCH");
   		DelayMs(500);
   		enque(CANCEL);
   	}
   	else if(smsState == '0'){
   		Disp_GLCDWriteText(0, 0,"FAILURE");
   		DelayMs(500);
   		enque(CANCEL);
   	}
}



//timer functions

void startWaitTimer(){
	OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, SECOND);
	ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_5);	
}	

void closeWaitTimer(){
	CloseTimer1();	
	WriteTimer1(0);
}	

void __ISR(_TIMER_1_VECTOR, ipl5) WaitIntHandler(void){
	mT3ClearIntFlag();

	timerCount++;
	
}




