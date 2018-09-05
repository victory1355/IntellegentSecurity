#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "includes.h"

#include "time.h"
#include "wdg.h"
#include "rtc.h"
#include "oled.h"
#include "beep.h"
#include "ui.h"
#include "key.h"

OS_Q queue;						//定义消息队列
OS_Q command_queue;						//定义消息队列
OS_Q time_queue;						//定义消息队列
OS_Q card_queue;						//定义消息队列
OS_Q th_queue;						//定义消息队列
OS_Q fire_queue;						//定义消息队列
OS_Q gas_queue;						//定义消息队列

OS_FLAG_GRP g_os_flag;			//定义事件标志组

OS_MUTEX mutex;					//定义一个互斥量，用于资源的保护

OS_SEM SYNC_SEM;				//定义一个信号量，用于RTC数据的同步
OS_SEM SYNC_SEM_RED;				//定义一个信号量，用于红外模块的同步
OS_SEM SYNC_SEM_Card;				//定义一个信号量，用于红外模块的同步
OS_SEM SYNC_SEM_KEY;				//定义一个信号量，用于按键模块的同步
OS_SEM SYNC_SEM_Sensors;				//定义一个信号量，用于传感器的同步
OS_SEM SYNC_SEM_TH;				//定义一个信号量，用于温湿度模块的同步
OS_SEM SYNC_SEM_DISTANCE;				//定义一个信号量，用于距离传感器模块的同步

OS_SEM SYNC_SEM_FIRE;				//定义一个信号量，用于火焰传感器模块的同步
OS_SEM SYNC_SEM_GAS;				//定义一个信号量，用于可燃气体传感器模块的同步


//任务1DHT11控制块
OS_TCB Dht11_Task_TCB;

void DHT11_Task(void *parg);

CPU_STK Dht11_Task_Stk[128*2];			//任务2的任务堆栈，大小为128字，也就是512字节


//任务2BLUE控制块
OS_TCB Blue_Task_TCB;

void Blue_Task(void *parg);

CPU_STK Blue_Task_Stk[128];			//任务3的任务堆栈，大小为128字，也就是512字节



//任务3KEY控制块
OS_TCB Key_Task_TCB;

void Key_Task(void *parg);

CPU_STK Key_Task_Stk[128*2*2*2];			//任务5的任务堆栈，大小为128字，也就是512字节




//任务4Rtc_Task控制块
OS_TCB Rtc_Task_TCB;

void Rtc_Task(void *parg);

CPU_STK Rtc_Task_Stk[128*2];			//任务5的任务堆栈，大小为128字，也就是512字节


//任务5Red_Task控制块
OS_TCB Red_Task_TCB;

void Red_Task(void *parg);

CPU_STK Red_Task_Stk[128*2];			//任务5的任务堆栈，大小为128字，也就是512字节


//任务6Card_Task控制块
OS_TCB Card_Task_TCB;

void Card_Task(void *parg);

CPU_STK Card_Task_Stk[256];			//任务5的任务堆栈，大小为256字，也就是256*4字节




//任务7Flash_Task控制块
OS_TCB Flash_Task_TCB;

void Flash_Task(void *parg);

CPU_STK Flash_Task_Stk[256*2];			//任务5的任务堆栈，大小为128字，也就是512字节


//任务8Key1234_Task控制块
OS_TCB Key1234_Task_TCB;

void Key1234_Task(void *parg);

CPU_STK Key1234_Task_Stk[128];	



//任务9Sensors_Task控制块
OS_TCB Sensors_Task_TCB;

void Sensors_Task(void *parg);

CPU_STK Sensors_Task_Stk[128*2];	


//任务10Distance_Task控制块
OS_TCB Distance_Task_TCB;

void Distance_Task(void *parg);

CPU_STK Distance_Task_Stk[128*2];

//任务11Fire_Task控制块
OS_TCB Fire_Task_TCB;

void Fire_Task(void *parg);

CPU_STK Fire_Task_Stk[128*2];


//任务12Gas_Task控制块
OS_TCB Gas_Task_TCB;

void Gas_Task(void *parg);

CPU_STK Gas_Task_Stk[128*2];



//任务13Select_Task控制块
OS_TCB Select_Task_TCB;

void Select_Task(void *parg);

CPU_STK Select_Task_Stk[128*2];


//任务14_Task控制块
OS_TCB B_Task_TCB;

void B_Task(void *parg);

CPU_STK B_Task_Stk[128*2];





volatile uint8_t red_flag = 0;


//主函数
int main(void)
{
    static NVIC_InitTypeDef 	NVIC_InitStructure;		
    static EXTI_InitTypeDef  	EXTI_InitStructure;
    
	OS_ERR err;  
    
	delay_init(168);  													//时钟初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);						//中断分组配置
	uart_init(9600);  				 									//串口初始化
	
    //LED初始化	    
    LED_Init(); 
	//按键初始化
	key1234_config();	
    //配置蜂鸣器
    beep_config();
    
    OLED_Init();			//初始化OLED  
    OLED_Clear(); 			//清除屏幕
    INIT_UI();
    boot_logo();  
    
    //初始化定时器3
    //tim3_init();
    //使用独立看门狗
    //IWDG_Init();
    //使用窗口看门狗
    //WWDG_Init();
    
    //rtc初始化
	if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x4567)
	{  
		rtc_init();
	}
	else
	{
		/* Enable the PWR clock ，使能电源时钟*/
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		
		/* Allow access to RTC，允许访问RTC备份寄存器 */
		PWR_BackupAccessCmd(ENABLE);
		
		/* Wait for RTC APB registers synchronisation，等待所有的RTC寄存器就绪 */
		RTC_WaitForSynchro();	
		

		//关闭唤醒功能
		RTC_WakeUpCmd(DISABLE);
		
		//为唤醒功能选择RTC配置好的时钟源
		RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
		
		//设置唤醒计数值为自动重载，写入值默认是0，1->0
		RTC_SetWakeUpCounter(0);
		
		//清除RTC唤醒中断标志
		RTC_ClearITPendingBit(RTC_IT_WUT);
		
		//使能RTC唤醒中断
		RTC_ITConfig(RTC_IT_WUT, ENABLE);

		//使能唤醒功能
		RTC_WakeUpCmd(ENABLE);			


		/* 配置外部中断控制线22，实现RTC唤醒*/
		EXTI_ClearITPendingBit(EXTI_Line22);
		EXTI_InitStructure.EXTI_Line = EXTI_Line22;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);
		
		/* 使能RTC唤醒中断 */
		NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
			
		
	}
          
    /* Check if the system has resumed from IWDG reset ，检查当前系统复位是否有看门狗复位导致*/
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
	{
		/* IWDGRST flag set */
		printf("iwdg reset cpu\r\n");
	

	}
	else if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
	{
		/* WWDGRST flag set */
		printf("wwdg reset cpu\r\n");
	

	}	
	else
	{
		/* IWDGRST flag is not set */
		printf("normal reset cpu\r\n");

	}
    /* Clear reset flags，清空所有复位标记 */
	RCC_ClearFlag();
    
    
	//OS初始化，它是第一个运行的函数,初始化各种的全局变量，例如中断嵌套计数器、优先级、存储器
	OSInit(&err);


	//创建Key_Task任务，优先级最高2
	OSTaskCreate(	(OS_TCB *)&Key_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Key_Task",									//任务的名字
					(OS_TASK_PTR)Key_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)1,											 	//任务的优先级		
					(CPU_STK *)Key_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2*2*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2*2*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);	       

#if 1					
	//创建BLUE任务
	OSTaskCreate(	(OS_TCB *)&Blue_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Blue_Task",									//任务的名字
					(OS_TASK_PTR)Blue_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)4,											 	//任务的优先级		
					(CPU_STK *)Blue_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
#endif	               

                
	//创建Rtc_Task任务
	OSTaskCreate(	(OS_TCB *)&Rtc_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Rtc_Task",									//任务的名字
					(OS_TASK_PTR)Rtc_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)3,											 	//任务的优先级		
					(CPU_STK *)Rtc_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);  

	//创建Red_Task
	OSTaskCreate(	(OS_TCB *)&Red_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Red_Task",									//任务的名字
					(OS_TASK_PTR)Red_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)3,											 	//任务的优先级		
					(CPU_STK *)Red_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				); 
					
	//创建Card_Task
	OSTaskCreate(	(OS_TCB *)&Card_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Card_Task",									//任务的名字
					(OS_TASK_PTR)Card_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)5,											 	//任务的优先级		
					(CPU_STK *)Card_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)256/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)256,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				); 
					
	//创建Flash_Task
	OSTaskCreate(	(OS_TCB *)&Flash_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Flash_Task",									//任务的名字
					(OS_TASK_PTR)Flash_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)2,											 	//任务的优先级		
					(CPU_STK *)Flash_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)256*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)256*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				); 					

	//创建Key1234_Task
	OSTaskCreate(	(OS_TCB *)&Key1234_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Key1234_Task",									//任务的名字
					(OS_TASK_PTR)Key1234_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)5,											 	//任务的优先级		
					(CPU_STK *)Key1234_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				); 					
	//创建Sensors_Task
	OSTaskCreate(	(OS_TCB *)&Sensors_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Sensors_Task",									//任务的名字
					(OS_TASK_PTR)Sensors_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)2,											 	//任务的优先级		
					(CPU_STK *)Sensors_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				); 						
#if 1
	//创建Distance_Task任务
	OSTaskCreate(	(OS_TCB *)&Distance_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Distance_Task",									//任务的名字
					(OS_TASK_PTR)Distance_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)Distance_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif					
	

#if 1
	//创建DHT11任务
	OSTaskCreate(	(OS_TCB *)&Dht11_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Dht11_Task",									//任务的名字
					(OS_TASK_PTR)DHT11_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)Dht11_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif	


#if 1
	//创建Fire_Task任务
	OSTaskCreate(	(OS_TCB *)&Fire_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Fire_Task",									//任务的名字
					(OS_TASK_PTR)Fire_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)Fire_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif	

#if 1
	//创建Gas_Task任务
	OSTaskCreate(	(OS_TCB *)&Gas_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Gas_Task",									//任务的名字
					(OS_TASK_PTR)Gas_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)Gas_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif	
#if 1
	//创建Select_Task任务
	OSTaskCreate(	(OS_TCB *)&Select_Task_TCB,									//任务控制块
					(CPU_CHAR *)"Select_Task",									//任务的名字
					(OS_TASK_PTR)Select_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)Select_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif
#if 1
	//创建B_Task任务
	OSTaskCreate(	(OS_TCB *)&B_Task_TCB,									//任务控制块
					(CPU_CHAR *)"B_Task",									//任务的名字
					(OS_TASK_PTR)B_Task,										//任务函数
					(void *)0,												//传递参数
					(OS_PRIO)6,											 	//任务的优先级		
					(CPU_STK *)B_Task_Stk,									//任务堆栈基地址
					(CPU_STK_SIZE)128*2/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					(CPU_STK_SIZE)128*2,										//任务堆栈大小			
					(OS_MSG_QTY)0,											//禁止任务消息队列
					(OS_TICK)0,												//默认时间片长度																
					(void  *)0,												//不需要补充用户存储区
					(OS_OPT)OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

#endif



					
	//创建一个信号量，信号量的初值为0
	OSSemCreate(&SYNC_SEM,"SYNC_SEM",0,&err);
	OSSemCreate(&SYNC_SEM_RED,"SYNC_SEM_RED",0,&err);
	OSSemCreate(&SYNC_SEM_Card,"SYNC_SEM_Card",0,&err);		
	OSSemCreate(&SYNC_SEM_KEY,"SYNC_SEM_KEY",0,&err);	
	OSSemCreate(&SYNC_SEM_Sensors,"SYNC_SEM_Sensors",0,&err);
	OSSemCreate(&SYNC_SEM_TH,"SYNC_SEM_TH",0,&err);						
	OSSemCreate(&SYNC_SEM_DISTANCE,"SYNC_SEM_DISTANCE",0,&err);	
	OSSemCreate(&SYNC_SEM_FIRE,"SYNC_SEM_FIRE",0,&err);	
    OSSemCreate(&SYNC_SEM_GAS,"SYNC_SEM_GAS",0,&err);	

	//创建互斥量（互斥锁）
	OSMutexCreate(&mutex,"mutex",&err);					
    
    //创建事件标志组,所有标志位0
	OSFlagCreate(&g_os_flag,"g_os_flag",0,&err);                     

    //创建消息队列，支持64条消息
	OSQCreate(&queue,"queue",64,&err);
	OSQCreate(&command_queue,"command_queue",64,&err);
	OSQCreate(&time_queue,"time_queue",64,&err);
	OSQCreate(&card_queue,"card_queue",64,&err);      
	OSQCreate(&th_queue,"th_queue",64,&err);   
	OSQCreate(&fire_queue,"fire_queue",64,&err);
	OSQCreate(&gas_queue,"gas_queue",64,&err);     
		
    //启动OS，进行任务调度
	OSStart(&err);
}


















