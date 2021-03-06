/*
********************************************************************************************************* 
 *					 	Vending machine controller 
 *
 * (c) Copyright 2012 D2NM, All rights reserved
********************************************************************************************************* 
*/


/*
********************************************************************************************************* 
 * 						Product database
 *
 * Filename      : ProductDB.c
 * Version       : V1.0
 * Programmer(s) : DIL
 *
 * Note(s)		 :
 *
 *
********************************************************************************************************* 
*/


void fill();
 
/* 
********************************************************************************************************* 
*                                            INCLUDE FILES 
********************************************************************************************************* 
*/ 

#include <p32xxxx.h> 
#include <plib.h>                   // include peripheral library functions
#include "global.h"
#include "ProductDB.h"
#include "Flash_Controller.h"

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

uint32 dataDB;
uint32 update;
uint32 event;
uint32 traySize=0;
uint32 noOftrays=0;

uint32 VMSerial=0;

uint8 i=0;
uint8 j=0;
uint8 k=0;
uint8 len=0;	

uint16 rx=0;
uint32 val=0;

uint32 x=0;
uint8 arr[500];

void InitDB()
{
	uint32* vmserial = read_flash_page( 4, 2);	//vending machine serial
	VMSerial=*vmserial;
	uint32* tmp  = read_flash_page( sizeof(tbl)/4, 0);
	uint16 i=0;
	uint16 count=WORD_SIZE+4;
	
	#ifdef DEBUG
		hal_sendString_CONSOLE_PORT("Database downloading");
		hal_sendChar_CONSOLE_PORT('\n');
	#endif

	traySize=read_flash_word(NVM_PROGRAM_PAGE+NVM_PAGE_SIZE);
	noOftrays=read_flash_word(NVM_PROGRAM_PAGE+NVM_PAGE_SIZE+32);
	uint16 num=0;
	if(traySize!=0 && noOftrays!=0)	{
		//prompt for data 
		num=traySize*noOftrays+1;
	}
	else 
	{
		;
	}

	for(i=0;i<num*count;i++){
		uint8 j=i%count;
		uint8 k=i/count;
		if(j<WORD_SIZE){
			tbl[k].name[j]=*tmp;
		}
		else if(j==WORD_SIZE)
			tbl[k].number=*tmp;
		else if(j==WORD_SIZE+1)
			tbl[k].amount=*tmp;
		else if(j==WORD_SIZE+2)
			tbl[k].valDec=*tmp;
		else if(j==WORD_SIZE+3)
			tbl[k].valCent=*tmp;
			
		tmp++;
	}
}


void addData(uint32 trayNum,uint32 num,char name[],uint32 amount,uint32 valDec,uint32 valCent)
{
	uint8 idx=(trayNum-1)*traySize+num;
	if(idx>0 && idx <TABLE_SIZE){
		uint8 i;
		for(i=0;i<WORD_SIZE;i++){
			uint8 tmp =name[i];
			if((tmp>='a' && tmp <='z' )|| (tmp>='A' && tmp<='Z')){
			tbl[idx].name[i]=(uint32)name[i];
			}
			else
			tbl[idx].name[i]=0;	
		}
			
		tbl[idx].number=idx;
		tbl[idx].amount=amount;
		tbl[idx].valDec=valDec;
		tbl[idx].valCent=valCent;
	}
}


Table getTable(uint32 trayNum,uint32 num)
{
	if(trayNum>0 && trayNum<=noOftrays)
		return tbl[(trayNum-1)*traySize+num];
	else
		return tbl[0];
		
}

void flashDB()
{
	erase_flash_page(0);
	write_flash_page((uint32)tbl, sizeof(tbl)/4 , 0);
	write_flash_word(traySize, NVM_PROGRAM_PAGE+NVM_PAGE_SIZE);
	write_flash_word(noOftrays, NVM_PROGRAM_PAGE+NVM_PAGE_SIZE+32);
	write_flash_word(VMSerial, NVM_PROGRAM_PAGE+2*NVM_PAGE_SIZE);
}

void setTraySize(uint32 size)
{
	traySize = size;
}

void setNoOfTrays(uint32 num)
{
	noOftrays = num;
}

uint32 getVMSerial()
{
	return VMSerial;
}


uint32 getTraySize()
{
	return traySize;
}

uint32 getNoOfTrays()
{
	return noOftrays;
}

void fsm_update()
{

	switch(update)
	{
		case CONSOLE_UPDATE:
		for(i=0;i<2;i++){
			for(j=0;j<WORD_SIZE;j++){
				if(tbl[i].name[j]==0xFFFFFFFF)
				tbl[i].name[j]=0;
			}	
		}
				
		hal_sendChar_CONSOLE_PORT('s');
		hal_sendChar_CONSOLE_PORT('\n');
		uint16 num=traySize*noOftrays+1;
		hal_sendChar_CONSOLE_PORT((unsigned char)num);
		hal_sendChar_CONSOLE_PORT('\n');
		hal_sendChar_CONSOLE_PORT((unsigned char)traySize);
		hal_sendChar_CONSOLE_PORT('\n');
		hal_sendChar_CONSOLE_PORT((unsigned char)noOftrays);
		hal_sendChar_CONSOLE_PORT('\n');
		
			for(i=1;i<num;i++){
				char tmp[WORD_SIZE+2];
				for(j=0;j<WORD_SIZE;j++){
					if(((char)tbl[i].name[j] >= '0' && (char)tbl[i].name[j] <= '9')||((char)tbl[i].name[j] <= 'z' &&(char)tbl[i].name[j] >= 'a') || ((char)tbl[i].name[j] <= 'Z' &&(char)tbl[i].name[j] >= 'A'))
					tmp[j]=(char)tbl[i].name[j];
					else
					tmp[j]=' ';
				}
				tmp[j]='\n';
				tmp[j+1]='\0';
				hal_sendString_CONSOLE_PORT(tmp);
				
				hal_uartWriteNumber(tbl[i].number);
				hal_sendChar_CONSOLE_PORT('\n');
				hal_uartWriteNumber(tbl[i].amount);
				hal_sendChar_CONSOLE_PORT('\n');
				hal_uartWriteNumber(tbl[i].valDec);
				hal_sendChar_CONSOLE_PORT('\n');
				//hal_uartWriteNumber(i);
				//hal_sendChar_CONSOLE_PORT('\n');
			}
			hal_sendChar_CONSOLE_PORT('#');
			hal_sendChar_CONSOLE_PORT('\n');
			enque(FIN);	
			update=5;
			i=1;
			j=0;
		break;
		case UPDATE:
			if(event==(uint8)END){
				fill();
				enque(FIN);	
				flashDB();				
				i=1;
				j=0;
				update=5;
				event=19;	
			}
		break;	
	}	

}


void process(){
		arr[x]=dataDB;
		x++;
}

void fill()	{
	uint8 cnt=1;
	traySize = 0;
	noOftrays = 0;
	i=1;
	j=0;
	while(arr[cnt]<='0' || arr[cnt]>='9')
	{
		cnt++;
	}
	while((char)arr[cnt]!='\n')
	{
		traySize=traySize*10+(arr[cnt]-'0');
		cnt++;
	}
	cnt++;
	while((char)arr[cnt]!='\n')
	{
		noOftrays=noOftrays*10+(arr[cnt]-'0');
		cnt++;
	}
	
	for(i=0;i<traySize*noOftrays;i++){
		for(j=0;j<WORD_SIZE;j++){
			tbl[i].name[j]=0;
		}	
	}
	i=1;
	j=0;
	
	cnt++;
	for(k=cnt;k<x;k++){
		if(rx==0)
		{
			if((char)arr[k]=='\n'){
				rx++;
				j=0;
			}else{
				tbl[i].name[j]=(char)arr[k];
				j++;
			}		
		}
		else if(rx==1)
		{
			if((char)arr[k]=='\n'){
				tbl[i].number=val;
				rx++;
				val=0;
			}else{
				val=val*10+((char)arr[k]-'0');
			}
		}
		else if(rx==2)
		{
			if((char)arr[k]=='\n'){
				tbl[i].amount=val;
				rx++;
				val=0;
			}else{
				val=val*10+((char)arr[k]-'0');
			}
		}
		else if(rx==3)
		{
			if((char)arr[k]=='\n'){
				tbl[i].valDec=val;
				tbl[i].valCent=0;
				rx=0;
				val=0;
				i++;
				
			}else{
				val=val*10+((char)arr[k]-'0');
			}
		}
	}	
	x=0;
	
	}
