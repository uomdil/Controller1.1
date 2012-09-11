#include "global.h"
#include "UART.h"
#include "LCD.h"
#include "ProductDB.h"

#define DESIRED_BAUDRATE_1A   		(9600)      //The desired BaudRate 1A


uint8 gsm_status =0;
uint8 gsm_count =0;

uint8 motor_response=0;
uint8 motor=0;

uint8 resposnse[3][5];
uint8 first_idx=0;
uint8 second_idx=0;


void hal_allUARTInit(void){

	mPORTGOpenDrainOpen(BIT_8);			//Tray
	//mPORTDOpenDrainOpen(BIT_15);		//GSM
	
	UART_INIT(UART1,DESIRED_BAUDRATE_1A,8_BITS)
	UART_INIT(UART3,DESIRED_BAUDRATE_1A,8_BITS)	//tray

	
}

//function prototype - void hal_sendString_##PORT (const char *string)
UART_SEND_STRING(UART1)



//function prototype - void hal_sendChar_##PORT (const char character)
UART_SEND_CHAR(UART1)
UART_SEND_CHAR(UART3)  //for tray


//interrupt service routines
UART_INT(_UART_1, ipl2){
	//UART_INT_TEST(UART1)
	if(INTGetFlag(INT_SOURCE_UART_RX(UART1)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(UART1));	    
	    if (UARTReceivedDataIsAvailable(UART1))      
		{                                           
			uint8 x=UARTGetDataByte(UART1);
			
			if(update !=UPDATE){
				switch(x){
					case 'r':enque(UPDATE_DATA);break;
					case 's':update=CONSOLE_UPDATE;break;		//s for start
					case 'd':update=UPDATE;break;		
				}
			}
			if(update ==UPDATE){
				dataDB=x;
				
				if(x=='#'){
					event = END;
				}else {
					process();
				}								
			}
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(UART1)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(UART1));	    
	}
}	
/*UART_INT(_UART_2, ipl2){
	//UART_INT_TEST(UART2)
	
	if(INTGetFlag(INT_SOURCE_UART_RX(UART2)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(UART2));	    
	    if (UARTReceivedDataIsAvailable(UART2))      
		{                                           
			uint8 x=UARTGetDataByte(UART2);
			
			switch(x){

			}
			
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(UART2)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(UART2));	    
	}
	
	
}*/

UART_INT(_UART_3, ipl2){
	//UART_INT_TEST(UART3)
	
	if(INTGetFlag(INT_SOURCE_UART_RX(UART3)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(UART3));	    
	    if (UARTReceivedDataIsAvailable(UART3))      
		{                                           
			uint8 x=UARTGetDataByte(UART3);
			
			resposnse[first_idx][second_idx]=x;
			motor_response++;
			second_idx++;
			if(motor_response==5 && motor <2){
				motor_response=0;
				enque(MOTOR_OK);
				motor++;
				first_idx++;
				second_idx=0;
				//Disp_GLCDClearDisp();
	    		//Disp_GLCDWriteText(0,1,"motor res");
	    		//DelayMs(1000);
			}else if(motor_response==5 && motor==2){   //response for the third motor
				motor_response=0;
				enque(MOTOR_OK);
				motor=0;
				first_idx=0;
				second_idx=0;
			}
			
		}											
	}												
	if ( INTGetFlag(INT_SOURCE_UART_TX(UART3)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(UART3));	    
	}
	
}

/*
UART_INT(_UART_5, ipl2){
	UART_INT_TEST(UART5)
}
UART_INT(_UART_6, ipl2){
	UART_INT_TEST(UART6)
}*/


//only for debug port only - UART3A
void hal_uartWriteNumber(unsigned int no){	
	
	if(no==0){
		hal_sendChar_UART1('0');
		return;
	}


	char val[10];

	unsigned int i=0;
	while(no>0){
		unsigned int dec = no%10;
		no = no/10;
		i++;
		val[i]=dec+'0';
	}

	for(;i>0;i--){
		hal_sendChar_UART1(val[i]);
	}		
}






/*
// Is this an RX interrupt?
	if(INTGetFlag(INT_SOURCE_UART_RX(UART3A)))
	{
		// Clear the RX interrupt Flag
	    INTClearFlag(INT_SOURCE_UART_RX(UART3A));

		// Echo what we just received.
		//PutCharacter(UARTGetDataByte(UART3A));
		hal_sendChar_UART3A(UARTGetDataByte(UART3A));

	}

	// We don't care about TX interrupt
	if ( INTGetFlag(INT_SOURCE_UART_TX(UART3A)) )
	{
		INTClearFlag(INT_SOURCE_UART_TX(UART3A));
	}
*/

uint8 getGSMStatus()
{

return gsm_status;
}

