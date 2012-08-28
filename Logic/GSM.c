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

#include <p32xxxx.h>			// include chip specific header file
#include <plib.h>                   // include peripheral library functions
#include "global.h"
#include "UART.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 




/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 




/* 
********************************************************************************************************* 
*                                        CONFIGURATION BITS 
********************************************************************************************************* 
*/ 


void gsm_init()
{
	hal_sendString_UART4("AT");
	hal_sendChar_UART4('\n');
}



void gsm_sendSMS(uint32 number,char* msg, uint8 len)
{
	hal_sendString_UART4("AT");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+CMGF=1");
	hal_sendChar_UART4('\n');
	//hal_sendString_UART4("AT+CMGS="+94");
	// send the number   770812312
	hal_sendChar_UART4('\n');
	hal_sendString_UART4(msg);
	//hal_sendChar_UART4();
	hal_sendChar_UART4('\n');
	
	
}


char* gsm_readSMS()
{
	hal_sendString_UART4("AT+CMGR=1");
	//hal_sendString_UART4("AT+CMGL=REC UNREAD");
	hal_sendChar_UART4('\n');
	
	
	//response
	
	/*+CMGR: “REC UNREAD”,”0146290800”,
	”98/10/01,18 :22 :11+00”,<CR><LF>
	ABCdefGHI
	OK
	*/
}


void gsm_connectGPRS()
{
	hal_sendString_UART4("AT");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WOPEN=1");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPCFG=1");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPBR=1,6");
	hal_sendChar_UART4('\n');
	//hal_sendString_UART4("AT+WIPBR=2,6,11,"ISP"");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPBR=2,6,0,""");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPBR=2,6,1,""");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPBR=4,6,0");
	hal_sendChar_UART4('\n');
	//hal_sendString_UART4("AT+WIPCREATE=5,1,"www.mrt.ac.lk",3128,"",""");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPOPT=5,1,1,51");
	hal_sendChar_UART4('\n');
	hal_sendString_UART4("AT+WIPOPT=5,1,2,53,6");
	hal_sendChar_UART4('\n');
	//hal_sendString_UART4("AT+WIPFILE=5,1,1,"/web/index.php","","","Accept","text/html"");
	hal_sendChar_UART4('\n');
	
}

