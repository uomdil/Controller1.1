/*
********************************************************************************************************* 
 *					 	Tray Motor Control
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						Tray Motor Control
 *
 * Filename      : TrayMotorControl.c
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
#include "TrayMotorControl.h"
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

void tray_init()
{



}

void VendMotor1(uint8 trayNo)
{
	//motor1
	hal_sendChar_UART3((uint8)0x00);
	hal_sendChar_UART3((uint8)0xFF);
	hal_sendChar_UART3((uint8)trayNo);		//motor 32 for 0x98
	hal_sendChar_UART3((uint8)(~trayNo));
	hal_sendChar_UART3((uint8)0x55);
	hal_sendChar_UART3((uint8)0xAA);

}

void VendMotor2()
{
	//motor2
	hal_sendChar_UART3((uint8)0x00);
	hal_sendChar_UART3((uint8)0xFF);
	hal_sendChar_UART3((uint8)0x64);
	hal_sendChar_UART3((uint8)0x9B);
	hal_sendChar_UART3((uint8)0x55);
	hal_sendChar_UART3((uint8)0xAA);
	
}

void VendMotor3()
{	
	//motor3
	hal_sendChar_UART3((uint8)0x00);
	hal_sendChar_UART3((uint8)0xFF);
	hal_sendChar_UART3((uint8)0x20);
	hal_sendChar_UART3((uint8)0xDF);
	hal_sendChar_UART3((uint8)0xAA);
	hal_sendChar_UART3((uint8)0x55);
}	




