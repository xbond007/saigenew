/*********************************************************
*Copyright (C), 2015, Shanghai Eastsoft Microelectronics Co., Ltd.
*文件名:  type.h
*作  者:  ESM Application Team 
*版  本:  V1.01
*日  期:  2016/11/01
*描  述:  type define
*备  注:  适用于HRSDK-GDB-8P506
          本软件仅供学习和演示使用，对用户直接引用代码所带来的风险或后果不承担任何法律责任。
**********************************************************/
#ifndef __TYPE_H__
#define __TYPE_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define  __align(x) __attribute__((aligned(x)))
#define  __intstack __attribute__((interrupt("WCH-Interrupt-fast")))

/***************BIG DATA*******************/

typedef union
{ 
    
    u8 Byte[2];
	
	u16 SHORT;
    
} big16;

typedef union
{ 
    
    u8 Byte[4];

	u32 Word;
    
} big32;
/***************BIG DATA END*******************/

#endif
