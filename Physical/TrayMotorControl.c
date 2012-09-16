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

#include "TrayMotorControl.h"

/* 
********************************************************************************************************* 
*                                               DEFINES
********************************************************************************************************* 
*/ 

#define TRAY_BAUDRATE	9600
#define TRAY_PORT		UART3
#define TRAY_VECTOR		_UART_3_VECTOR

/* 
********************************************************************************************************* 
*                                          GLOBAL VARIABLES 
********************************************************************************************************* 
*/ 

uint8 motor_response=0;
uint8 motor=0;

uint8 resposnse[3][5];
uint8 first_idx=0;
uint8 second_idx=0;


UART_SEND_CHAR(TRAY_PORT)

void Tray_init()
{
	UART_INIT(TRAY_PORT,TRAY_BAUDRATE,8_BITS)
}

void VendMotor1(uint8 trayNo)
{
	//motor1
	hal_sendChar_TRAY_PORT((uint8)0x00);
	hal_sendChar_TRAY_PORT((uint8)0xFF);
	hal_sendChar_TRAY_PORT((uint8)trayNo);		//motor 32 for 0x98
	hal_sendChar_TRAY_PORT((uint8)(~trayNo));
	hal_sendChar_TRAY_PORT((uint8)0x55);
	hal_sendChar_TRAY_PORT((uint8)0xAA);

}

void VendMotor2()
{
	//motor2
	hal_sendChar_TRAY_PORT((uint8)0x00);
	hal_sendChar_TRAY_PORT((uint8)0xFF);
	hal_sendChar_TRAY_PORT((uint8)0x64);
	hal_sendChar_TRAY_PORT((uint8)0x9B);
	hal_sendChar_TRAY_PORT((uint8)0x55);
	hal_sendChar_TRAY_PORT((uint8)0xAA);
	
}

void VendMotor3()
{	
	//motor3
	hal_sendChar_TRAY_PORT((uint8)0x00);
	hal_sendChar_TRAY_PORT((uint8)0xFF);
	hal_sendChar_TRAY_PORT((uint8)0x20);
	hal_sendChar_TRAY_PORT((uint8)0xDF);
	hal_sendChar_TRAY_PORT((uint8)0xAA);
	hal_sendChar_TRAY_PORT((uint8)0x55);
}	


UART_INT(TRAY_VECTOR, ipl2){
	
	if(INTGetFlag(INT_SOURCE_UART_RX(TRAY_PORT)))		
	{												
	    INTClearFlag(INT_SOURCE_UART_RX(TRAY_PORT));	    
	    if (UARTReceivedDataIsAvailable(TRAY_PORT))      
		{                                           
			uint8 x=UARTGetDataByte(TRAY_PORT);
			
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
	if ( INTGetFlag(INT_SOURCE_UART_TX(TRAY_PORT)) )  	
	{												
		INTClearFlag(INT_SOURCE_UART_TX(TRAY_PORT));	    
	}
	
}


