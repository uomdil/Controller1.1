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

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

#define PRODUCT_SELECT 1	//use keys for product select
#define AMOUNT_SELECT 2	//use keys to select number of products


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

uint8 count=0;  					//count pressing keys

uint8 keyuse;						//keypad mode: either entering product no or 

uint32 product_no=0;				//to store entered product no
uint32 amount=0;					//to store entered prosuct amount
uint32 total=0;
int balance=0;
uint32 enteredValue=0;
uint32 canAcceptBills=0;


uint8 motor_no=0;

/* 
********************************************************************************************************* 
*                                        FUNCTION PROTOTYPES 
********************************************************************************************************* 
*/ 

void board_init();
void stateMachine(uint8 eventId);
void changeState(uint8 nextStateId);
void onStateExit(uint8 stateId);
uint8 deque();
void onStateEntry(uint8 stateId);
uint8 getNext(uint8 index);


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





/* 
********************************************************************************************************* 
*                                        DEFINES 
********************************************************************************************************* 
*/ 

//#define MONEY_STATE 

#define DEBUG 

int main(void)
{
	board_init();												//Initialize the circuit board configurations
  	changeState(START);											//Initialize the state
  	enque(DIAGNOSTIC);											//first do diagnostic

  	
  	hal_mdbInit();
  	
  	/*#ifdef SHOW_MESSAGES
	    hal_sendString_UART1("Machine started");
	    hal_sendChar_UART1('\n');
    #endif
  	*/
  	
  	INTEnableInterrupts();
  	INTEnableSystemMultiVectoredInt();
	
	setTraySize(3);
    setNoOfTrays(2);
	
	char name[]="Dilshan";
	addData(1,1,name,5,2000,0);
	
	flashDB();
	InitDB();
	
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
     	mdbStateMachine();
     	
     	//hal_sendChar_UART3(0xAA);
     	
  	}	
  	
}


void board_init(){
	DDPCONbits.JTAGEN = 0;										//disable JTAG
	DBINIT();     												// Initialize the IO channel
  	hal_allUARTInit();											//Initialize all UARTs
  	keypad_init();												//Initialize keypad
	Disp_Init();
	Disp_GLCDInit();

}



void stateMachine(uint8 eventId){
  switch(state){
    case START :
    	if(eventId == (uint8)DIAGNOSTIC){
	    	enque(INIT_VARS);
    	    changeState(DIAGNOSTIC);
    	}          	
     break;
    
    case DIAGNOSTIC:
      	if(eventId == (uint8)INIT_VARS){
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
	    	if(count<3 && key<10){
		 		product_no=product_no*10+key;
		 		//write to LCD
		 		Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"Entered Value");
		 		DelayMs(10);
		 		Disp_GLCDWrite(3,1,(product_no/10)+0x30);
		 		DelayMs(10);
		 		Disp_GLCDWrite(4,1,(product_no-(product_no/10)*10)+0x30);
		 		DelayMs(10);
		 		#ifdef DEBUG
					hal_sendString_UART1("product no = ");
					hal_uartWriteNumber(product_no);
					hal_sendChar_UART1('\n');
				#endif
		 	}
    	}
    	else if(eventId == (uint8)CANCEL){
	    	product_no=0;
	    	enque(INIT_VARS);
	    	changeState(RETURN_MONEY);
	    	
    	}
    	
      
     break;
    
    case WAIT_AMOUNT:
    	if(eventId == (uint8)OK){
	    	#ifdef DEBUG
				hal_sendString_UART1("Got amount");
				hal_sendChar_UART1('\n');
			#endif
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
		 		Disp_GLCDWriteText(0,3,"Total = ");
		 		uint8 thou=total/1000;
	 			Disp_GLCDWrite(3,1,(thou)+'0');
	 			Disp_GLCDData(((total/100)-thou*10)+'0');
	 			Disp_GLCDWrite(4,1,((total%100)-total%10)/10+'0');
	 			Disp_GLCDData(total%10+'0');
	 			Disp_GLCDWrite(5,1,'.');
	 			Disp_GLCDData('0');
	 			Disp_GLCDWrite(6,1,'0');
    	    	changeState(WAIT_MONEY); 
			}
			else 
			{
				total=amount*tbl[product_no].valDec;
				Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,3,"Total = ");
		 		uint8 thou=total/1000;
	 			Disp_GLCDWrite(3,1,(thou)+'0');
	 			Disp_GLCDData(((total/100)-thou*10)+'0');
	 			Disp_GLCDWrite(4,1,((total%100)-total%10)/10+'0');
	 			Disp_GLCDData(total%10+'0');
	 			Disp_GLCDWrite(5,1,'.');
	 			Disp_GLCDData('0');
	 			Disp_GLCDWrite(6,1,'0');
    	    	changeState(WAIT_MONEY);  
   			} 	    
    	}    
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    	if(count<3 && key<10){	
		 		amount=amount*10+key;
		 		//write to LCD
		 		Disp_GLCDClearDisp();
		 		Disp_GLCDWriteText(0,0,"Amount Entered");
		 		Disp_GLCDWrite(3,1,(amount/10)+0x30);
		 		Disp_GLCDWrite(4,1,(amount-(amount/10)*10)+0x30);
				uint32 *nm=tbl[product_no].name;
				uint8 i=0;
				for(i=0;i<WORD_SIZE/2;i++){
					uint8 tmp = *nm;
					if((tmp>'a' && tmp <'z' )|| (tmp>'A' && tmp<'Z')){
						Disp_GLCDWrite(i,2,*nm);
						nm++;
					}
					tmp = *nm;	
					if((tmp>'a' && tmp <'z' )|| (tmp>'A' && tmp<'Z')){	
						Disp_GLCDData(tmp);
						nm++;
					}
				}
				uint32 valD=tbl[product_no].valDec;
				uint32 valC=tbl[product_no].valCent;
	 			Disp_GLCDWriteText(0,3,"Price:");
	 			uint8 thou=valD/1000;
	 			Disp_GLCDWrite(4,3,(thou)+'0');
	 			Disp_GLCDData(((valD/100)-thou*10)+'0');
	 			Disp_GLCDWrite(5,3,((valD%100)-valD%10)/10+'0');
	 			Disp_GLCDData(valD%10+'0');
	 			Disp_GLCDWrite(6,3,'.');
	 			Disp_GLCDData((valC/10)+'0');
	 			Disp_GLCDWrite(7,3,(valC-(valC/10)*10)+'0');
	 			
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
    
    case WAIT_MONEY:   	    
    	if(eventId == (uint8)COIN_IN){
    	}
    	else if(eventId == (uint8)CASH_IN){
	    	enteredValue=enteredValue+mdbBillValue;
	    	balance=enteredValue-total;
	    	if(balance<0){
	    		Disp_GLCDWriteText(0,2,"Entered = ");			
	 			
	 			uint8 thou=enteredValue/1000;
	 			Disp_GLCDWrite(5,2,(thou)+'0');
	 			Disp_GLCDData(((enteredValue/100)-thou*10)+'0');
	 			Disp_GLCDWrite(6,2,((enteredValue%100)-enteredValue%10)/10+'0');
	 			Disp_GLCDData(enteredValue%10+'0');

				
	    	}else{
	    		Disp_GLCDWriteText(0,2,"Entered = ");			
	    		uint8 thou=enteredValue/1000;
	 			Disp_GLCDWrite(5,2,(thou)+'0');
	 			Disp_GLCDData(((enteredValue/100)-thou*10)+'0');
	 			Disp_GLCDWrite(6,2,((enteredValue%100)-enteredValue%10)/10+'0');
	 			Disp_GLCDData(enteredValue%10+'0');

	 			Disp_GLCDWriteText(0,3,"Press Enter");
	 			canAcceptBills=0;							//stop bill accepting
	 			
	 		#ifdef MONEY_STATE
	    		enque(OK);
    		#endif 
	 			
	 		
	    	}
    	}
    	else if(eventId == (uint8)NFC_IN){	
	    	enque( NFC_GET_CONFIRM);
    	    changeState(NFC_PAY);    	
    	}
    	else if(eventId == (uint8)CANCEL){
	    	amount=0;
	    	product_no=0;
	    	enteredValue =0;
	    	enque(INIT_VARS);
	    	changeState(RETURN_MONEY);
    	}
    	else if(eventId == (uint8)TIME_OUT){
	    	enque(INIT_VARS);	    	
	    	changeState(RETURN_MONEY);
    	}
    	else if(eventId == (uint8)ENTER_NO){
	    	check_key()
	    }
	    else if(eventId == (uint8)OK){
		    Disp_GLCDClearDisp();
	    	Disp_GLCDWriteText(0,1,"Please wait");
	    	changeState(DISPENSE);
	    	enque(FIRST_MOTOR);
	    }	
    	
     break;
    
    case BALANCE:
    	if(eventId == (uint8)OK){
    	    changeState(DISPENSE);  
    	    enque(FIRST_MOTOR);
    	}    
    	else if(eventId == (uint8)CANCEL){
	    	enque(INIT_VARS);
	    	changeState(RETURN_MONEY);
    	}
    
     break;
      
    case DISPENSE:
      	if(eventId == (uint8)INIT_VARS){
    	    changeState(INIT);  
    	}    
    	else if(eventId == (uint8)ERROR){
	    	enque(ERROR);	    	
	    	changeState(RETURN_MONEY);
    	}
    	else if(eventId == (uint8)FIRST_MOTOR){
    		VendMotor1(product_no);
    	}
    	else if(eventId == (uint8)SECOND_MOTOR){
    		VendMotor2();
    	}
    	else if(eventId == (uint8)THIRD_MOTOR){
    		VendMotor3();
    	}
    	else if(eventId == (uint8)MOTOR_OK){
    		motor_no++;
    		if(motor_no==1){
	    		enque(SECOND_MOTOR);
	    	}
	    	else if(motor_no==2){
	    		enque(THIRD_MOTOR);
	    	}	
	    	else if(motor_no==3){
    			motor_no=0;
    			changeState(INIT);
    		}
    		
    	}
    	
     break;
    
    case RETURN_MONEY:
    	if(eventId == (uint8)INIT_VARS){
	    	enque(PRODUCT_NO);
    	    changeState(INIT);  
    	}    
    	else if(eventId == (uint8)ERROR){
	    	enque(SEND_ERROR);
    	    changeState(SYSTEM_LOCK);
    	}
      
     break;
      
    case SYSTEM_LOCK:
    	if(eventId == (uint8)SEND_ERROR){
    	    changeState(GSM_CONTROL);  
    	} 
      
     break;
      
    case NFC_PAY:
    	if(eventId == (uint8)NFC_GET_CONFIRM){
	    	enque(NFC_SET_CONFIRM);
    	    changeState(GSM_CONTROL);  
    	}    
    	else if(eventId == (uint8)PRODUCT_NO){
	    	changeState(WAIT_PRODUCT); 
    	}
      
     break;
      
    case GSM_CONTROL:
    	if(eventId == (uint8)NFC_SET_CONFIRM){
    	    changeState(NFC_PAY);  
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
		DelayMs(20);
    	Disp_GLCDWriteText(2, 0, "WELCOME");
    	DelayMs(100); 			//long delay
		amount=0;
	    product_no=0;
    break;
    
    case UPDATE_VARS :
      
   
    break;
    
    case WAIT_MONEY:
	    Disp_GLCDClearDisp();
		DelayMs(20);
	    Disp_GLCDWriteText(0, 0, " INSERT MONEY");
	    Disp_GLCDWriteText(0,1,"Total = ");
		uint8 thou=total/1000;
		Disp_GLCDWrite(3,1,(thou)+'0');
		Disp_GLCDData(((total/100)-thou*10)+'0');
		Disp_GLCDWrite(4,1,((total%100)-total%10)/10+'0');
		Disp_GLCDData(total%10+'0');
		Disp_GLCDWrite(5,1,'.');
		Disp_GLCDData('0');
		Disp_GLCDWrite(6,1,'0');
		DelayMs(1);
		keyuse=PRODUCT_SELECT;
		
		canAcceptBills=1;
    break;
    
    case WAIT_PRODUCT:
	    Disp_GLCDClearDisp();
		DelayMs(20);
		Disp_GLCDWriteText(0, 0, "INSERT PRODUCT");
		Disp_GLCDWriteText(0, 1,"NUMBER");
	    count=0; 
	    keyuse=PRODUCT_SELECT;     
      	
      	#ifdef MONEY_STATE
    		changeState(WAIT_MONEY);		
    		total=100;
    		amount=2;
    		product_no =0x99;
    	#endif 
      	
      	
    break;
    
    case WAIT_AMOUNT:
	    Disp_GLCDClearDisp();
		DelayMs(20);
	    Disp_GLCDWriteText(0, 0, " ENTER QUANTITY");
		DelayMs(50);				//long delay
	    keyuse==AMOUNT_SELECT; 
	    count=0; 
    
    break;
    
    case BALANCE:
    
    break;
      
    case DISPENSE:
      
      break;
    
    case RETURN_MONEY:
      
      break;
      
    case SYSTEM_LOCK:
      
      break;
      
    case NFC_PAY:
      
      break;
      
    case GSM_CONTROL:
      
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
	count=0; 
      
    break;
    
    case WAIT_AMOUNT:
	count=0;
    
    break;
    
    case BALANCE:
    
      break;
      
    case DISPENSE:
      balance=0;
      total=0;
      enteredValue=0;
      break;
    
    case RETURN_MONEY:
      
      break;
      
    case SYSTEM_LOCK:
      
      break;
      
    case NFC_PAY:
      
      break;
      
    case GSM_CONTROL:
      
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





