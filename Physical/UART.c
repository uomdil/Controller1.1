#include "global.h"
#include "UART.h"
#include "LCD.h"
#include "ProductDB.h"

#define DESIRED_BAUDRATE_1A   		(9600)      //The desired BaudRate 1A


void hal_allUARTInit(void){

	mPORTGOpenDrainOpen(BIT_8);			//Tray
	//mPORTDOpenDrainOpen(BIT_15);		//GSM
	
	UART_INIT(CONSOLE_PORT,DESIRED_BAUDRATE_1A,8_BITS)
	UART_INIT(UART3,DESIRED_BAUDRATE_1A,8_BITS)	//tray

	
}

//function prototype - void hal_sendString_##PORT (const char *string)
UART_SEND_STRING(CONSOLE_PORT)



//function prototype - void hal_sendChar_##PORT (const char character)
UART_SEND_CHAR(CONSOLE_PORT)


//interrupt service routines
UART_INT(CONSOLE_VECTOR, ipl2){
	//UART_INT_TEST(CONSOLE_PORT)
	if(INTGetFlag(INT_SOURCE_UART_RX(CONSOLE_PORT)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(CONSOLE_PORT));	    
	    if (UARTReceivedDataIsAvailable(CONSOLE_PORT))      
		{                                           
			uint8 x=UARTGetDataByte(CONSOLE_PORT);
			
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
	if ( INTGetFlag(INT_SOURCE_UART_TX(CONSOLE_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(CONSOLE_PORT));	    
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
		hal_sendChar_CONSOLE_PORT('0');
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
		hal_sendChar_CONSOLE_PORT(val[i]);
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

