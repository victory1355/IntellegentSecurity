#ifndef _TASKS_H
#define _TASKS_H


#include "stm32f4xx.h"

extern volatile uint8_t g_usart3_buf[64];			//存储串口3接收数据的内容
extern volatile uint8_t g_usart3_cnt;				//存储串口3接收数据的个数
extern volatile uint8_t g_usart3_event;				//记录串口3接收数据是否完毕



#endif


