#include "tasks.h"

#include "dht11.h"
#include "wdg.h"
#include "blue.h"
#include "time.h"
#include "oled.h"
#include "key.h"
#include "led.h"
#include "ui.h"
#include "rtc.h"
#include "red.h"
#include "pwm.h"
#include "w25q128.h"
#include "mfrc522.h"
#include "distance.h"
#include "fire.h"
#include "gas.h"


extern OS_FLAG_GRP g_os_flag;			//定义事件标志组
extern OS_Q queue;						//定义消息队列
extern OS_Q command_queue;						//定义消息队列
extern OS_Q time_queue;						//定义消息队列
extern OS_Q card_queue;						//定义消息队列
extern OS_Q th_queue;						//定义消息队列
extern OS_Q fire_queue;						//定义消息队列
extern OS_Q gas_queue;						//定义消息队列

extern OS_MUTEX mutex;					//定义一个互斥量，用于资源的保护

extern OS_SEM SYNC_SEM;				    //定义一个信号量，用于RTC数据的同步
extern OS_SEM SYNC_SEM_RED;				//定义一个信号量，用于红外模块的同步
extern OS_SEM SYNC_SEM_Card;			//定义一个信号量，用于RFID模块的同步
extern OS_SEM SYNC_SEM_KEY;				//定义一个信号量，用于按键模块的同步
extern OS_SEM SYNC_SEM_Sensors;				//定义一个信号量，用于传感器的同步
extern OS_SEM SYNC_SEM_TH;				//定义一个信号量，用于温湿度模块的同步
extern OS_SEM SYNC_SEM_DISTANCE;				//定义一个信号量，用于距离传感器模块的同步
extern OS_SEM SYNC_SEM_FIRE;				//定义一个信号量，用于火焰传感器模块的同步
extern OS_SEM SYNC_SEM_GAS;				//定义一个信号量，用于可燃气体传感器模块的同步


extern OS_TCB Key_Task_TCB;
extern OS_TCB Sensors_Task_TCB;

extern volatile uint8_t red_flag;

#define key1 PEin(3)
#define key2 PEin(4)


uint8_t date_buf[64];
uint8_t time_buf[64];
 
void Rtc_Task(void *parg)
{
    char *p = NULL,*p_t = NULL;
    uint32_t i=0;

    OS_MSG_SIZE msg_size=0;
    static RTC_TimeTypeDef  	RTC_TimeStructure;
    static RTC_DateTypeDef 	RTC_DateStructure;
    
    static RTC_TimeTypeDef  	RTC_TimeStructure1;
    static RTC_DateTypeDef 	RTC_DateStructure1;
    OS_ERR err;
    OS_FLAGS flags; 
    
    printf("Rtc_Task is create ok\r\n");
    //清空缓冲区
    memset(date_buf, 0, 64);
    memset(time_buf, 0, 64);
        
    //保证其他任务先执行
    OSTimeDlyHMSM(0,0,0,300,OS_OPT_TIME_HMSM_STRICT,&err); //延时300ms，并让出CPU资源，使当前的任务进行睡眠	
	while(1)
	{
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
		//等待按键TIME按下
		OSSemPend(&SYNC_SEM,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		          
        //显示时间界面
        Time_UI();
        
        //检测按键
        while(1)
        {
            //如果按下按键3，则设置系统时间，通过按键修改时间
            if(!PEin(3))
            {
                delay_ms(50);
                if(PEin(3))
                {
                    //进入时间设置界面，未定义
                    printf("set time\r\n");
                 
                }
            }
            //如果接受到蓝牙修改时间指令，通过蓝牙修改时间
            
            //一直阻塞等待互斥量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，msg_size为接收数据的大小，NULL不记录时间戳
            //返回值就是数据内容的指针，也就是指向数据的内容
            p=OSQPend(&time_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);            
            if(p && msg_size)
            {	
                //判断消息的内容，如果是修改指令则获取
                
                OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
                printf("rtc task recv msg：[%s],size:[%d]\r\n",p,msg_size);
                OSMutexPost(&mutex,OS_OPT_POST_1,&err); 
                if(strstr(p, "DATE") != NULL)
                {
                    //修改日期
                    
                    printf("change date:%s\r\n",p);
                    //判断接收到的字符串为DATE SET
                    //示例：DATE SET-2017-10-12-4\n
                    
					//以等号分割字符串
					strtok((char *)p,"-");
					
					//获取年
					p_t=strtok(NULL,"-");
					
					//2018-2000=18 
					i = atoi(p_t)-2000;
					//转换为16进制 18 ->0x18
					i= (i/10)*16+i%10;
					RTC_DateStructure.RTC_Year = i;
					
					//获取月
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//转换为16进制
					i= (i/10)*16+i%10;						
					RTC_DateStructure.RTC_Month=i;


					//获取日
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//转换为16进制
					i= (i/10)*16+i%10;		
					RTC_DateStructure.RTC_Date = i;
					
					//获取星期
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//转换为16进制
					i= (i/10)*16+i%10;						
					RTC_DateStructure.RTC_WeekDay = i;
					
					//设置日期
					if(RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure) == SUCCESS)
                    {
                        //串口1信息提示
                        printf("set date ok\r\n");
                        
                        //串口3信息提示
                        usart3_send_str("set date ok\r\n");
                    }
                    else
                    {
                        printf("set date failed\r\n");  
                        usart3_send_str("set date failed\r\n");                        
                    }

				                 
                    
                }
                if(strstr(p, "TIME") != NULL)
                {
                    printf("change time:%s\r\n",p);                
                    //修改时间
					//以等号分割字符串
					strtok((char *)p,"-");
					
					//获取时
					p_t=strtok(NULL,"-");
					i = atoi(p_t);
					
					//通过时，判断是AM还是PM
					if(i<12)
						RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
					else
						RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
						
					//转换为16进制
					i= (i/10)*16+i%10;
					RTC_TimeStructure.RTC_Hours   = i;
					
					//获取分
					p_t=strtok(NULL,"-");
					i = atoi(p_t);						
					//转换为16进制
					i= (i/10)*16+i%10;	
					RTC_TimeStructure.RTC_Minutes = i;
					
					//获取秒
					p_t=strtok(NULL,"-");
					i = atoi(p_t);						
					//转换为16进制
					i= (i/10)*16+i%10;					
					RTC_TimeStructure.RTC_Seconds = i; 					
					
					if(RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure) == SUCCESS)
                    {
                        //串口1信息提示
                        printf("set time ok\r\n");  
                        //串口3信息提示
                        usart3_send_str("set time ok\r\n");                        
                    }
                    else
                    {
                        printf("set time failed\r\n");
                        usart3_send_str("set time failed\r\n");                              
                    }

                    
                }
		
            }            
            
            //如果按下按键1则退出该循环，返回到主菜单
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("resume key task\r\n");
                    //唤醒按键任务
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    
                    //跳出按键检测，让任务重新阻塞
                    break;
                }
            }
            
            //不断更新时间
            flags=OSFlagPend(&g_os_flag,0x01,0,OS_OPT_PEND_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x01)
            {
                //获取日期
                RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure1);
                sprintf((char*)date_buf,"20%02x/%02x/%02xWeek:%x",RTC_DateStructure1.RTC_Year,RTC_DateStructure1.RTC_Month,RTC_DateStructure1.RTC_Date,RTC_DateStructure1.RTC_WeekDay);
                printf("20%02x/%02x/%02xWeek:%x\r\n",RTC_DateStructure1.RTC_Year,RTC_DateStructure1.RTC_Month,RTC_DateStructure1.RTC_Date,RTC_DateStructure1.RTC_WeekDay);                
                
                //刷新日期
                //x=24,第2行显示
                OLED_ShowString(0,2,(u8 *)date_buf,16);

                
                //获取时间
                RTC_GetTime(RTC_Format_BCD,&RTC_TimeStructure1);
                sprintf((char*)time_buf,"%02x:%02x:%02x",RTC_TimeStructure1.RTC_Hours,RTC_TimeStructure1.RTC_Minutes,RTC_TimeStructure1.RTC_Seconds);	
                //g_rtc_wakeup_event=0;
                
                //刷新时间
                //x=24,第2行显示
                OLED_ShowString(0,4,(u8 *)time_buf,16);
            }
            //清空消息队列的内容
            memset((char *)p, 0, sizeof p);            
            printf("update time every second\r\n");
            //每隔5ms扫描一次按键并让出CPU资源，使当前的任务进行睡眠	
            OSTimeDlyHMSM(0,0,0,5,OS_OPT_TIME_HMSM_STRICT,&err); //延时1ms，并让出CPU资源，使当前的任务进行睡眠	
            
        }
        printf("Rtc_Task is running ...\r\n");   
		OSTimeDlyHMSM(0,0,0,300,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s，并让出CPU资源，使当前的任务进行睡眠	
        
	}


}




//按键任务
void Red_Control_Task(void *parg)
{

	OS_ERR err;
    OS_FLAGS flags;
    
    int8_t rt;
	static volatile uint8_t ir_data[4];    
    
    uint8_t red_flag = 0;
    uint8_t sensor_flag = 0;
    uint8_t time_flag = 0;
    uint8_t card_flag = 0;
    uint8_t key_flag = 0;    
       
    uint8_t key1_times = 0,key2_times = 0, key3_times = 0;
	
	printf("Red_Control_Task is create ok\r\n");
         
    
	while(1)
	{
        //切换菜单
                     
        //红外切换
        if(red_flag)
        {
            //printf("red mode\r\n");
            
            memset((void *)ir_data, 0 ,sizeof ir_data);
            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
            rt = ir_read_data((unsigned char *)ir_data);
            OSMutexPost(&mutex,OS_OPT_POST_1,&err);	 
            
            if(rt == 0)
            {
                printf("%02X %02X %02X %02X\r\n",ir_data[0],ir_data[1],ir_data[2],ir_data[3]);              
                if(ir_data[2] == 0x45 && ir_data[3] == 0xBA)
                {
                    
                    key3_times++;               
                    if(key3_times == 1)
                    {
                        printf("time\r\n");
                        
                        OLED_ShowChar(0, 2, '>', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,' ', 16);

                        
                        time_flag = 1;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 0;

                    }
                    if(key3_times == 2)
                    {


                        printf("card\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, '>', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,' ', 16);
                        
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 1;
                        key_flag = 0;  
                          
                      
                    }

                    if(key3_times == 3)
                    {
                        printf("key\r\n");                 
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, '>', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30,6 ,' ', 16);
                        
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 1;                   
                       
                    }
                    
                    if(key3_times == 4)
                    {                       
                        printf("red\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, '>', 16);                    
                        OLED_ShowChar(30, 6 ,' ', 16);
                                                               
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 1;
                        card_flag = 0;
                        key_flag = 0;
                                                                     
                    }    
                    if(key3_times == 5)
                    {
                        printf("sensor\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,'>', 16);

                        time_flag = 0;
                        sensor_flag = 1;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 0; 
                        
                        
                        key3_times = 0;
                    }

                        
                } 
                if(ir_data[2] == 0x09 && ir_data[3] == 0xF6)
                {  
                        
                    if(time_flag == 1)
                    {
                        time_flag = 0;
                        printf("time task resume key tasks");
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();
                    }
                    if(red_flag == 1)
                    {
                        printf("red task resume key tasks\r\n");
                        red_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();                    
                    }
                    if(sensor_flag == 1)
                    {
                        printf("sensors task resume key tasks\r\n");
                        sensor_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   
                    }
                    if(key_flag == 1)
                    {					
                        printf("key1234 task resume key tasks\r\n");
                        key_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   														
                    }                
                    if(card_flag == 1)
                    {                   
                        printf("red task resume key tasks\r\n");
                        card_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   				
                    }                
                                            
                }                
                
            }                
       
        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时10ms，并让出CPU资源，使当前的任务进行睡眠		
	}
//	OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时10ms，并让出CPU资源，使当前的任务进行睡眠

}


//按键任务
void Key_Task(void *parg)
{

	OS_ERR err;
    OS_FLAGS flags;
    
    int8_t rt;
	static volatile uint8_t ir_data[4];    
    
    uint8_t red_flag = 0;
    uint8_t sensor_flag = 0;
    uint8_t time_flag = 0;
    uint8_t card_flag = 0;
    uint8_t key_flag = 0;    
       
    uint8_t key1_times = 0,key2_times = 0, key3_times = 0;
	
	printf("Key_Task is create ok\r\n");
    
    
    OLED_Init();			//初始化OLED  
    OLED_Clear(); 			//清除屏幕
    
    //显示主界面
    Main_UI();


    //模式的选择
    while(1)
    {
        if(!PEin(3))//按下按键1，标志位反转，按下按键2则进入对应的模式之中
		{
            
            delay_ms(100);
            
            if(!PEin(3))
            {
                //红外模式
                printf("Rmode\r\n");
                
                OLED_ShowChar(30, 3, '>', 16);
                OLED_ShowChar(30, 6, ' ', 16);
                //break;
                
                key1_times++;               
                if(key1_times%2 == 0)
                {
                    red_flag = 1;
                    break;
                }

            }                    
            
        }

		if(!PEin(4))
		{
            delay_ms(100);
               
            
            if(!PEin(4))
            {
                //默认模式
                printf("Kmode\r\n");
                OLED_ShowChar(30, 6, '>', 16);
                OLED_ShowChar(30, 3, ' ', 16);
                //break;
                key2_times++;
                
                if(key2_times%2 == 0)
                {
                    red_flag = 0;
                    break;
                }

            }                    
            

		}
        
        OSTimeDlyHMSM(0,0,0,10  ,OS_OPT_TIME_HMSM_STRICT,&err); //延时10ms，并让出CPU资源，使当前的任务进行睡眠
        
    }

    //进入菜单   
    Menu_UI();
    
	while(1)
	{
        //切换菜单
                     
        //红外切换
//        if(red_flag)
//        {
//            printf("red mode\r\n");
//            
////            memset((void *)ir_data, 0 ,sizeof ir_data);
//            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
//            rt = ir_read_data((unsigned char *)ir_data);
//            OSMutexPost(&mutex,OS_OPT_POST_1,&err);	 
//            
//            if(rt == 0)
//            {
//                printf("%02X %02X %02X %02X\r\n",ir_data[0],ir_data[1],ir_data[2],ir_data[3]);              
//                if(ir_data[2] == 0x45 && ir_data[3] == 0xBA)
//                {
//                    
//                    key3_times++;               
//                    if(key3_times == 1)
//                    {
//                        printf("time\r\n");
//                        
//                        OLED_ShowChar(0, 2, '>', 16);
//                        OLED_ShowChar(70, 2, ' ', 16);
//                        OLED_ShowChar(0, 4, ' ', 16);
//                        OLED_ShowChar(70, 4, ' ', 16);
//                        
//                        OLED_ShowChar(30, 6 ,' ', 16);

//                        
//                        time_flag = 1;
//                        sensor_flag = 0;
//                        red_flag = 0;
//                        card_flag = 0;
//                        key_flag = 0;

//                    }
//                    if(key3_times == 2)
//                    {


//                        printf("card\r\n");
//                              
//                        OLED_ShowChar(0, 2, ' ', 16);
//                        OLED_ShowChar(70, 2, '>', 16);
//                        OLED_ShowChar(0, 4, ' ', 16);
//                        OLED_ShowChar(70, 4, ' ', 16);
//                        
//                        OLED_ShowChar(30, 6 ,' ', 16);
//                        
//                        time_flag = 0;
//                        sensor_flag = 0;
//                        red_flag = 0;
//                        card_flag = 1;
//                        key_flag = 0;  
//                          
//                      
//                    }

//                    if(key3_times == 3)
//                    {
//                        printf("key\r\n");                 
//                        OLED_ShowChar(0, 2, ' ', 16);
//                        OLED_ShowChar(70, 2, ' ', 16);
//                        OLED_ShowChar(0, 4, '>', 16);
//                        OLED_ShowChar(70, 4, ' ', 16);
//                        
//                        OLED_ShowChar(30,6 ,' ', 16);
//                        
//                        time_flag = 0;
//                        sensor_flag = 0;
//                        red_flag = 0;
//                        card_flag = 0;
//                        key_flag = 1;                   
//                       
//                    }
//                    
//                    if(key3_times == 4)
//                    {                       
//                        printf("red\r\n");
//                              
//                        OLED_ShowChar(0, 2, ' ', 16);
//                        OLED_ShowChar(70, 2, ' ', 16);
//                        OLED_ShowChar(0, 4, ' ', 16);
//                        OLED_ShowChar(70, 4, '>', 16);                    
//                        OLED_ShowChar(30, 6 ,' ', 16);
//                                                               
//                        time_flag = 0;
//                        sensor_flag = 0;
//                        red_flag = 1;
//                        card_flag = 0;
//                        key_flag = 0;
//                                                                     
//                    }    
//                    if(key3_times == 5)
//                    {
//                        printf("sensor\r\n");
//                              
//                        OLED_ShowChar(0, 2, ' ', 16);
//                        OLED_ShowChar(70, 2, ' ', 16);
//                        OLED_ShowChar(0, 4, ' ', 16);
//                        OLED_ShowChar(70, 4, ' ', 16);
//                        
//                        OLED_ShowChar(30, 6 ,'>', 16);

//                        time_flag = 0;
//                        sensor_flag = 1;
//                        red_flag = 0;
//                        card_flag = 0;
//                        key_flag = 0; 
//                        
//                        
//                        key3_times = 0;
//                    }

//                        
//                } 
//                if(ir_data[2] == 0x09 && ir_data[3] == 0xF6)
//                {  
//                        
//                    if(time_flag == 1)
//                    {
//                        time_flag = 0;
//                        printf("time task resume key tasks");
//                        //唤醒时间任务，挂起当前任务
//                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
//                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
//                        
//                        //挂起当前的任务
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //如果被唤醒则刷新菜单界面                   
//                        Menu_UI();
//                    }
//                    if(red_flag == 1)
//                    {
//                        printf("red task resume key tasks\r\n");
//                        red_flag = 0;
//                        //唤醒时间任务，挂起当前任务
//                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
//                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
//                        
//                        //挂起当前的任务
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //如果被唤醒则刷新菜单界面                   
//                        Menu_UI();                    
//                    }
//                    if(sensor_flag == 1)
//                    {
//                        printf("sensors task resume key tasks\r\n");
//                        sensor_flag = 0;
//                        //唤醒时间任务，挂起当前任务
//                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
//                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
//                        
//                        //挂起当前的任务
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //如果被唤醒则刷新菜单界面                   
//                        Menu_UI();   
//                    }
//                    if(key_flag == 1)
//                    {					
//                        printf("key1234 task resume key tasks\r\n");
//                        key_flag = 0;
//                        //唤醒时间任务，挂起当前任务
//                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
//                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
//                        
//                        //挂起当前的任务
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //如果被唤醒则刷新菜单界面                   
//                        Menu_UI();   														
//                    }                
//                    if(card_flag == 1)
//                    {                   
//                        printf("red task resume key tasks\r\n");
//                        card_flag = 0;
//                        //唤醒时间任务，挂起当前任务
//                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
//                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
//                        
//                        //挂起当前的任务
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //如果被唤醒则刷新菜单界面                   
//                        Menu_UI();   				
//                    }                
//                                            
//                }                
//                
//            }                
//        }
//        else
//        {
            //按键切换
            if(!PEin(4))//按下按键1，标志位反转，按下按键2则进入对应的模式之中
            {
                
                delay_ms(100);
                
                if(!PEin(4))
                {

                    //break;
                    
                    key3_times++;               
                    if(key3_times == 1)
                    {
                        printf("time\r\n");
                        
                        OLED_ShowChar(0, 2, '>', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,' ', 16);

                        
                        time_flag = 1;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 0;

                    }
                    if(key3_times == 2)
                    {


                        printf("card\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, '>', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,' ', 16);
                        
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 1;
                        key_flag = 0;  
                          
                      
                    }

                    if(key3_times == 3)
                    {
                        printf("key\r\n");                 
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, '>', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30,6 ,' ', 16);
                        
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 1;                   
                       
                    }
                    
                    if(key3_times == 4)
                    {                       
                        printf("red\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, '>', 16);                    
                        OLED_ShowChar(30, 6 ,' ', 16);
                                                               
                        time_flag = 0;
                        sensor_flag = 0;
                        red_flag = 1;
                        card_flag = 0;
                        key_flag = 0;
                        

                      
                       
                    }    
                    if(key3_times == 5)
                    {
                        printf("sensor\r\n");
                              
                        OLED_ShowChar(0, 2, ' ', 16);
                        OLED_ShowChar(70, 2, ' ', 16);
                        OLED_ShowChar(0, 4, ' ', 16);
                        OLED_ShowChar(70, 4, ' ', 16);
                        
                        OLED_ShowChar(30, 6 ,'>', 16);


                        time_flag = 0;
                        sensor_flag = 1;
                        red_flag = 0;
                        card_flag = 0;
                        key_flag = 0;  
                        
                        key3_times = 0;
                    }

                        
                }                    
                
            }
            
            if(!PEin(3))
            {
   
                delay_ms(50);
                if(!PEin(3))
                {   
                    printf("test\r\n");
                    
                    if(time_flag == 1)
                    {
                        time_flag = 0;
                        printf("time task resume key tasks");
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();
                    }
                    if(red_flag == 1)
                    {
                        printf("red task resume key tasks\r\n");
                        red_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();                    
                    }
                    if(sensor_flag == 1)
                    {
                        printf("sensors task resume key tasks\r\n");
                        sensor_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   
                    }
                    if(key_flag == 1)
                    {					
                        printf("key1234 task resume key tasks\r\n");
                        key_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   														
                    }                
                    if(card_flag == 1)
                    {                   
                        printf("red task resume key tasks\r\n");
                        card_flag = 0;
                        //唤醒时间任务，挂起当前任务
                        //仅向等待该最高优先级的任务发送信号量，信号量的值会加1
                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
                        
                        //挂起当前的任务
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //如果被唤醒则刷新菜单界面                   
                        Menu_UI();   				
                    }                
                    
                }
                
            }            
//        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时10ms，并让出CPU资源，使当前的任务进行睡眠		
	}
	OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时10ms，并让出CPU资源，使当前的任务进行睡眠

}



/*
	注：没有使用外部中断进行检测，实时性不高
*/
void Red_Task(void *parg)
{
	OS_ERR err;
    
    int8_t rt;
	static volatile uint8_t ir_data[4];
    
    ir_init();
       
    //初始化定时器14的pwm通道
    tim14_init();
    TIM_SetCompare1(TIM14,100);  
    
	printf("Red_Task is create ok\r\n");


    //添加代码
    while(1)
    {
        //通过按键是否启动红外模式
        
        //红外模式
//        if(red_flag == 1)
//        {
//            memset((void *)ir_data, 0 ,sizeof ir_data);
//            
//            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
//            rt = ir_read_data((unsigned char *)ir_data);
//            OSMutexPost(&mutex,OS_OPT_POST_1,&err);	            

//                
//            printf("get menu command\r\n");
//            if(rt == 0)
//            {
//                printf("%02X %02X %02X %02X\r\n",ir_data[0],ir_data[1],ir_data[2],ir_data[3]);
//                
//                
//                
////                //根据不同的码值实现控制LED和蜂鸣器
////                //熄灭
////                if(ir_data[2] == 0x07 && ir_data[3] == 0xF8)
////                {
////                    printf("get -\r\n");
////                    TIM_SetCompare1(TIM14,100);
////                    //delay_ms(500);
////                }
////                //点亮
////                if(ir_data[2] == 0x15 && ir_data[3] == 0xEA)
////                {
////                    printf("get +\r\n");
////                    TIM_SetCompare1(TIM14,0);
////                    //delay_ms(500);
////                }
////                //呼吸灯效果
////                if(ir_data[2] == 0x16 && ir_data[3] == 0xE9)
////                {
////                    printf("get \r\n");

////                }
////				//控制蜂鸣器，未定义                
////                if(ir_data[2] == 0x1 && ir_data[3] == 0x0)
////                {
////                    printf("get \r\n");

////                    
////                }
//                //控制菜单切换命令
//                if(ir_data[2] == 0x46 && ir_data[3] == 0xB9)
//                {
//                    //这里是对事件标志组的bit3置1操作
//                    printf("red get com:%02X %02X\r\n", ir_data[2],ir_data[3]);
//                    OSFlagPost(&g_os_flag,0x04,OS_OPT_POST_FLAG_SET,&err);

//                }
//            }          
//            else
//            {
//                //串口打印错误信息
//                printf("rt = %d \r\n",rt);	
//                    
//            }            
//        }
        
        //默认模式
        printf("Red_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
        //等待按键TIME按下
        OSSemPend(&SYNC_SEM_RED,0,OS_OPT_PEND_BLOCKING,NULL,&err);             
        //显示界面
        Red_UI(); 

        //检测按键
        while(1)
        {
                                         
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("red task resume key task\r\n");
                    //唤醒按键任务
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    //关闭定时器14
                    TIM_Cmd(TIM14, DISABLE);
                    //跳出按键检测，让任务重新阻塞
                    
                    break;
                }
            }
            memset((void *)ir_data, 0 ,sizeof ir_data);
            rt = ir_read_data((unsigned char *)ir_data);
            
            
            if(rt == 0)
            {
                printf("%02X %02X %02X %02X\r\n",ir_data[0],ir_data[1],ir_data[2],ir_data[3]);
                
                //根据不同的码值实现控制LED和蜂鸣器
                //熄灭
                if(ir_data[2] == 0x07 && ir_data[3] == 0xF8)
                {
                    printf("get -\r\n");
                    TIM_SetCompare1(TIM14,100);
                    //delay_ms(500);
                }
                //点亮
                if(ir_data[2] == 0x15 && ir_data[3] == 0xEA)
                {
                    printf("get +\r\n");
                    TIM_SetCompare1(TIM14,0);
                    //delay_ms(500);
                }
                //呼吸灯效果
                if(ir_data[2] == 0x16 && ir_data[3] == 0xE9)
                {
                    printf("breathe light\r\n");
//                    for(pwm_cmp=0; pwm_cmp<=500; pwm_cmp++)
//                    {
//                        printf("get 0\r\n");
//                        //设置比较值
//                        TIM_SetCompare1(TIM14,pwm_cmp);
//                        
//                        //延时20ms
//                        delay_ms(20);
                }
                //控制蜂鸣器，未定义                
                if(ir_data[2] == 0x1 && ir_data[3] == 0x0)
                {
                    printf("control beep\r\n");
                    //开启蜂鸣器
                    PFout(8)=1;
                    delay_ms(100);
                    PFout(8)=0;	
                    delay_ms(100);						
                    //开启蜂鸣器
                    PFout(8)=1;
                    delay_ms(100);
                    PFout(8)=0;	
                    delay_ms(100);	
                    
                }
                //控制菜单切换命令
                if(ir_data[2] == 0x46 && ir_data[3] == 0xB9)
                {
                    //这里是对事件标志组的bit3置1操作
                    OSFlagPost(&g_os_flag,0x04,OS_OPT_POST_FLAG_SET,&err);

                }
            }          
            else
            {
                //串口打印错误信息
                printf("rt = %d \r\n",rt);	
                    
            }
            printf("detect red signal\r\n");
                                        
            OSTimeDlyHMSM(0,0,0,3,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)            
                        
        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)
		
    }
}


//任务代码模板
void Key1234_Task(void *parg)
{  
	OS_ERR err;
		
	printf("Key1234_Task is create ok\r\n");

	while(1)
	{
		printf("Key1234_Task is running ...\r\n");		
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
        //等待按键TIME按下
        OSSemPend(&SYNC_SEM_KEY,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //显示界面
        Key_UI(); 
	
		while(1)
		{
			//按键1
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{   
                    while(!PAin(0));
                    
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("exit...\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
					//delay_ms(100);
					
                    printf("key1234 task resume key task\r\n");
                    //唤醒按键任务
                    OS_TaskResume(&Key_Task_TCB,&err);                    
                    
                    //跳出按键检测，让任务重新阻塞
                    break;						
				}
			}
			//按键2
			if(!PEin(2))
			{
				delay_ms(50);
				if(PEin(2))
				{
					//等待按键松开
					while(PEin(2));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("play music...\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);							
					
					//未定义，有时间拓展					
				}
				
			}
			//按键3
			if(!PEin(3))
			{
				delay_ms(50);
				if(!PEin(3))
				{
					//等待按键松开
					while(PEin(3));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("open beep...\r\n");											
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
					
					//开启蜂鸣器
					PFout(8)=1;
					delay_ms(100);
					PFout(8)=0;	
					delay_ms(100);						
					//开启蜂鸣器
					PFout(8)=1;
					delay_ms(100);
					PFout(8)=0;	
					delay_ms(100);													  						
				}
					
			}
			//按键4
			if(!PEin(4))
			{
				delay_ms(50);

				if(!PEin(4))
				{
					//等待按键松开
					while(!PEin(4));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("open led4...\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
					
					PEout(14) = 0;
					delay_ms(100);
					PEout(14) = 1;					             											
				}				
			}						
			OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时50ms，并让出CPU资源，使当前的任务进行睡眠	
		}
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s，并让出CPU资源，使当前的任务进行睡眠		
	}
}


//MFRC522数据区
extern uint8_t  mfrc552pidbuf[18];
extern uint8_t  card_pydebuf[2];
extern uint8_t  card_numberbuf[5];
extern uint8_t  card_key0Abuf[6];
extern uint8_t  card_writebuf[16];
extern uint8_t  card_readbuf[18];

uint32_t id=0;
//uint8_t  buf[64]={0};

volatile uint8_t cardid_buf[12];
//RFID任务
void Card_Task(void *parg)
{      
	OS_ERR err;
	
	memset((void *)cardid_buf, 0 ,sizeof cardid_buf);
		
	MFRC522_Initializtion();
		
	printf("Card_Task is create ok\r\n");

	while(1)
	{
		
		printf("Card_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
        //等待按键TIME按下
        OSSemPend(&SYNC_SEM_Card,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //显示界面
        Card_UI(); 
		
        //检测按键
        while(1)
        {                                      
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("card task resume key task\r\n");
                    //唤醒按键任务
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    
                    //跳出按键检测，让任务重新阻塞
                    break;
                }
            }
			
			//一直阻塞等待互斥量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
			OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
			//MFRC522
			MFRC522_Initializtion();
		
			//MFRC522Test();
			memset((void *)cardid_buf, 0 ,sizeof cardid_buf);			
			Get_Card_ID(cardid_buf);
			printf("card task cardid:%s\r\n", cardid_buf);
			//delay_ms(100);	
			//仅向等待该最高优先级的任务发送信号量，互斥量的值会加1
			OSMutexPost(&mutex,OS_OPT_POST_1,&err);			
//			delay_ms(500);
			//end MFRC522
			
			//使用先进先出的形式发送消息        
			OSQPost(&queue,(uint8_t *)cardid_buf,sizeof cardid_buf,OS_OPT_POST_FIFO,&err);
			

			OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			printf("detect cardid\r\n");
			OSMutexPost(&mutex,OS_OPT_POST_1,&err);	                                       
            OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)   

			
		}
		
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s，并让出CPU资源，使当前的任务进行睡眠	
	}
}


//各种传感器
void Sensors_Task(void *parg)
{
	uint8_t	firesensor = 0;
	uint8_t	distancesensor = 0;
	uint8_t	gassensor = 0;
	uint8_t	temp_humisensor = 0,key3_times = 0;    
    
	OS_ERR err;
	
	printf("Sensors_Task is create ok\r\n");

	while(1)
	{
		
		printf("Sensors_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
        //等待按键TIME按下
        OSSemPend(&SYNC_SEM_Sensors,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //显示界面
        Sensors_UI(); 
		
        //检测按键
        while(1)
        {                                      
			//切换菜单
			if(!PEin(4))//按下按键1，标志位反转，按下按键2则进入对应的模式之中
			{
				
				delay_ms(100);
				
				if(!PEin(4))
				{

					//break;
					while(!PEin(4));
					key3_times++;               
					if(key3_times == 1)
					{
						printf("firesensor\r\n");
						
						OLED_ShowChar(0, 3, '>', 16);
						OLED_ShowChar(50 , 3, ' ', 16);
						OLED_ShowChar(0, 6, ' ', 16);						
						OLED_ShowChar(50, 6 ,' ', 16);
						
						firesensor = 1;
						distancesensor = 0;
						gassensor = 0;
						temp_humisensor = 0;

					}
					if(key3_times == 2)
					{

						
						printf("temp_humisensor\r\n");
							  
						OLED_ShowChar(0, 3, ' ', 16);
						OLED_ShowChar(50 , 3, '>', 16);
						OLED_ShowChar(0, 6, ' ', 16);						
						OLED_ShowChar(50, 6 ,' ', 16);
						
						firesensor = 0;
						distancesensor = 0;
						gassensor = 0;
						temp_humisensor = 1;
						
			  
					}

					if(key3_times == 3)
					{
						printf("gassensor\r\n");
							  
						OLED_ShowChar(0, 3, ' ', 16);
						OLED_ShowChar(50 , 3, ' ', 16);
						OLED_ShowChar(0, 6, '>', 16);						
						OLED_ShowChar(50, 6 ,' ', 16);
						
						firesensor = 0;
						distancesensor = 0;
						gassensor = 1;
						temp_humisensor = 0;                 
					   
					}
					
					if(key3_times == 4)
					{
						printf("distancesensor\r\n");
							  
						OLED_ShowChar(0, 3, ' ', 16);
						OLED_ShowChar(50 , 3, ' ', 16);
						OLED_ShowChar(0, 6, ' ', 16);						
						OLED_ShowChar(50, 6 ,'>', 16);

						firesensor = 0;
						distancesensor = 1;
						gassensor = 0;
						temp_humisensor = 0;
					    key3_times = 0;		                        
					}    
						
				}                    
				
			}
			
			if(!PEin(3))
			{   
				delay_ms(50);
				if(!PEin(3))
				{   
					while(!PEin(3));
					if(firesensor == 1)
					{
						firesensor = 0;
						printf("firesensor resume firesensor tasks");
						//唤醒时间任务，挂起当前任务
						//仅向等待该最高优先级的任务发送信号量，信号量的值会加1
						OSSemPost(&SYNC_SEM_FIRE,OS_OPT_POST_1,&err);
						
						//挂起当前的任务
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//如果被唤醒则刷新菜单界面                   
						Sensors_UI(); 
					}
					if(distancesensor == 1)
					{
						printf("distancesensor resume distancesensor tasks\r\n");
						distancesensor = 0;
						//唤醒时间任务，挂起当前任务
						//仅向等待该最高优先级的任务发送信号量，信号量的值会加1
						OSSemPost(&SYNC_SEM_DISTANCE,OS_OPT_POST_1,&err);
						
						//挂起当前的任务
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//如果被唤醒则刷新菜单界面                   
						Sensors_UI();                 
					}
					if(gassensor == 1)
					{					
						printf("gassensor resume gassensor tasks\r\n");
						gassensor = 0;
						//唤醒时间任务，挂起当前任务
						//仅向等待该最高优先级的任务发送信号量，信号量的值会加1
						OSSemPost(&SYNC_SEM_GAS,OS_OPT_POST_1,&err);
						
						//挂起当前的任务
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//如果被唤醒则刷新菜单界面                   
						Sensors_UI(); 													
					}                
					if(temp_humisensor == 1)
					{                   
						printf("temp_humisensor resume temp_humisensor tasks\r\n");
						temp_humisensor = 0;
						//唤醒时间任务，挂起当前任务
						//仅向等待该最高优先级的任务发送信号量，信号量的值会加1
						OSSemPost(&SYNC_SEM_TH,OS_OPT_POST_1,&err);
						
						//挂起当前的任务
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//如果被唤醒则刷新菜单界面                   
						Sensors_UI(); 			
					}                
					
				}
				
			}
			//返回到主菜单
			if(!PAin(0))
			{
				delay_ms(100);
				if(!PAin(0))
				{
					//等待按键松开
					while(!PAin(0));
					
					//唤醒按键任务
                    printf("sensors task resume key task\r\n");
                    //唤醒按键任务
                    OS_TaskResume(&Key_Task_TCB,&err);
					break;
					
				}
			}
			printf("detect key\r\n");
            OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)   			
		}		
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s，并让出CPU资源，使当前的任务进行睡眠	
	}

}

/*可燃气体传感器模块*/
void Gas_Task(void *parg)
{
	OS_ERR err;
	uint32_t adc_val=0;
	uint32_t adc_vol=0;
	uint32_t dt=0,i; 
    uint8_t gas_value[3];
    OS_FLAGS flags;

    
    memset(gas_value, 0 ,sizeof gas_value);
    
	//进行ADC初始化
    //gas_adc_init();//PA2(ADC123_IN2) 
    extie6_init(); //PE6(EXTI)
    

    
	printf("Gas_Task is create ok\r\n");
    //添加代码
	while(1)
	{
        printf("Gas_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
		//等待按键TIME按下
		OSSemPend(&SYNC_SEM_GAS,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //显示界面
        Gas_UI();

        //使能定时器4工作
        //TIM_Cmd(TIM4, ENABLE);
        
        //检测按键
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));                
					printf("Gas_Task resume Sensors_Task\r\n");
					//唤醒按键任务
					OS_TaskResume(&Sensors_Task_TCB,&err);

                    //使能定时器4工作
//                    TIM_Cmd(TIM4, DISABLE);										
					//跳出按键检测，让任务重新阻塞
					break;
				}
			}
            //设置为不阻塞等待
            flags=OSFlagPend(&g_os_flag,0x04,0,OS_OPT_PEND_NON_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x04)
            {   
                printf("gas warning\r\n");
                for(i = 0; i<20;i++)
                {
                    PEout(14) = 0; 
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                    delay_ms(50);
                    PEout(14) = 1;
                    delay_ms(50);
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                }                  
                
            }
            //启动ADC1的转换
            ADC_SoftwareStartConv(ADC1);                        
            //等待转换完成
            while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)!=SET);                        
            //获取ADC1的转换结果
            adc_val=ADC_GetConversionValue(ADC1);            
            //将结果值转换为电压值
            adc_vol=adc_val*3300/4095;
            
            //火焰传感器，有火焰，就检测到很低的电压值为0mv；若没有火焰，则检测到很高的电压值
            printf("adc_vol=%dmv\r\n",adc_vol);
                                 
            //OLED显示效果               
            if(adc_vol >= 20 && adc_vol <=1000)
            {
                OLED_ShowString(70,2,(u8 *)"4",16);            
                OLED_ShowString(60,4,(u8 *)"      ",16);	
                OLED_ShowString(60,4,(u8 *)">>>>>>",16);
                
                //LED急速闪烁
//                    TIM_Cmd(TIM14, ENABLE);                      
                for(i = 0; i<20;i++)
                {
                    PEout(14) = 0; 
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                    delay_ms(50);
                    PEout(14) = 1;
                    delay_ms(50);
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                }                 //使用蜂鸣器进行报警
                
                
            }				
            if(adc_vol > 1000 && adc_vol <=2000)
            {
                OLED_ShowString(70,2,(u8 *)"3",16);
                
                OLED_ShowString(60,4,(u8 *)"      ",16);	
                OLED_ShowString(60,4,(u8 *)">>>>>",16);
            }
            if(adc_vol > 2000 && adc_vol <=3000)
            {
                OLED_ShowString(70,2,(u8 *)"2",16);  
                
                OLED_ShowString(60,4,(u8 *)"      ",16);	
                OLED_ShowString(60,4,(u8 *)">>",16);
                
            }
            if(adc_vol > 3000 && adc_vol <=3300)
            {
                OLED_ShowString(70,2,(u8 *)"1",16); 
                
                OLED_ShowString(60,4,(u8 *)"      ",16);	
                OLED_ShowString(60,4,(u8 *)">",16);
                
            }  
            sprintf((char *)gas_value, "%dmv", adc_vol);
            OLED_ShowString(60,6,(u8 *)"        ",16);            
            OLED_ShowString(60,6,(u8 *)gas_value,16);
            
            memset(gas_value, 0 ,sizeof gas_value);
                
            //传感器采样时间间隔			         				
            OSTimeDlyHMSM(0,0,0,5,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)		
            printf("detect key to exit\r\n");
        }
        OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠         
    }            
   
}

/*火焰传感器模块*/
void Fire_Task(void *parg)
{
	OS_ERR err;
	uint32_t adc_val=0,i;
	uint32_t adc_vol=0;
    OS_FLAGS flags;     
    uint8_t fire_value[3];
        
    memset(fire_value, 0 ,sizeof fire_value);
 
	//进行ADC初始化
	adc_init();   
    //exti8_init();
    tim4_init();
    TIM_Cmd(TIM4, DISABLE);	
        
	printf("Fire_Task is create ok\r\n");
    //添加代码
	while(1)
	{
        printf("Fire_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
		//等待按键TIME按下
		OSSemPend(&SYNC_SEM_FIRE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //显示界面
        Fire_UI();

        //使能定时器4工作
        TIM_Cmd(TIM4, ENABLE);
        
        //检测按键
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));
					printf("Fire_Task resume Sensors_Task\r\n");
					//唤醒按键任务
					OS_TaskResume(&Sensors_Task_TCB,&err);

                    //使能定时器4工作
                    TIM_Cmd(TIM4, DISABLE);										
					//跳出按键检测，让任务重新阻塞
					break;
				}
			}
            //设置为不阻塞等待
            flags=OSFlagPend(&g_os_flag,0x02,0,OS_OPT_PEND_NON_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x02)
            {
                printf("get time4 flag\r\n");
                //启动ADC1的转换
                ADC_SoftwareStartConv(ADC1);
                
                //等待ADC1转换完成
                while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
                
                
                //获取转化结果值
                adc_val=ADC_GetConversionValue(ADC1);
                
                //将结果值转换为电压值
                adc_vol=adc_val * 3300/4095;
                
                printf("adc_vol=%dmv\r\n",adc_vol);
                
                //OLED显示效果
      
                
                if(adc_vol >= 20 && adc_vol <=1000)
                {
                    OLED_ShowString(70,2,(u8 *)"4",16);                
                
                    OLED_ShowString(60,4,(u8 *)"      ",16);	
                    OLED_ShowString(60,4,(u8 *)">>>>>>",16);                      
                    for(i = 0; i<20;i++)
                    {
                        PEout(14) = 0; 
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                        delay_ms(50);
                        PEout(14) = 1;
                        delay_ms(50);
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                    }                        
                    
                    
                }				
                if(adc_vol > 1000 && adc_vol <=2000)
                {
                    OLED_ShowString(70,2,(u8 *)"3",16);                
                    OLED_ShowString(60,4,(u8 *)"      ",16);	
                    OLED_ShowString(60,4,(u8 *)">>>>>",16);
                    
                    
                    //LED急速闪烁
//                    TIM_Cmd(TIM14, ENABLE);                      
                    for(i = 0; i<20;i++)
                    {
                        PEout(14) = 0; 
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                        delay_ms(50);
                        PEout(14) = 1;
                        delay_ms(50);
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                    }  
                    
                }
                if(adc_vol > 2000 && adc_vol <=3000)
                {
                    OLED_ShowString(70,2,(u8 *)"2",16);                
                    OLED_ShowString(60,4,(u8 *)"      ",16);	
                    OLED_ShowString(60,4,(u8 *)">>",16);
                    
                }
                if(adc_vol > 3000 && adc_vol <=3300)
                {
                    OLED_ShowString(70,2,(u8 *)"1",16);
                    OLED_ShowString(60,4,(u8 *)"      ",16);	
                    OLED_ShowString(60,4,(u8 *)">",16);
                    
                }  
                sprintf((char *)fire_value, "%dmv", adc_vol);
                OLED_ShowString(70,6,(u8 *)"        ",16);            
                OLED_ShowString(70,6,(u8 *)fire_value,16);
                
                memset(fire_value, 0 ,sizeof fire_value);    
                //传感器采样时间间隔			  
                //OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)				
            }        				
            OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)		
            printf("detect key to exit\r\n");
        }
    }
}



/*距离传感器模块*/
void Distance_Task(void *parg)
{
	uint32_t distance = 0, i = 0;
	OS_ERR err;
	uint8_t dis_buf[10];
		
	memset(dis_buf, 0, sizeof dis_buf);
	
	//超声波模块初始化
	sr04_init();
	
	printf("Distance_Task is create ok\r\n");

    //添加代码
	while(1)
	{
        printf("Distance_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
		//等待按键TIME按下
		OSSemPend(&SYNC_SEM_DISTANCE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //显示界面
        Distance_UI();
        
        //检测按键
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));
					printf("Distance_Task resume Sensors_Task\r\n");
					//唤醒按键任务
					OS_TaskResume(&Sensors_Task_TCB,&err);
										
					//跳出按键检测，让任务重新阻塞
					break;
				}
			}
			distance = sr04_get_distance();								
			if(distance == 0xFFFFFFFF)
			{
				printf("sr04 get distance error\r\n");
				
			}			
			if(distance >=20 && distance<=4000)
			{			
				printf("distance=%dmm\r\n",distance);

				//OLED 数据显示效果，未定义
				sprintf((char *)dis_buf, "%dmm", distance);
                OLED_ShowString(60,6,(u8 *)"      ",16);  
				OLED_ShowString(60,6,(u8 *)dis_buf,16);
				if(distance > 20 && distance <1000)
				{
					OLED_ShowString(70,2,(u8 *)"4",16);   
                    
					OLED_ShowString(60,4,(u8 *)"          ",16);	
					OLED_ShowString(60,4,(u8 *)">>>>>!",16);
                    
                    //LED急速闪烁
//                    TIM_Cmd(TIM14, ENABLE);                      
                    for(i = 0; i<20;i++)
                    {
                        PEout(13) = 0; 
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                        delay_ms(50);
                        PEout(13) = 1;
                        delay_ms(50);
                        //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                    }                   
                    //设置比较值                   
                    //TIM_Cmd(TIM14, DISABLE);                    
				}				
				if(distance > 1000 && distance <2000)
				{
					OLED_ShowString(70,2,(u8 *)"3",16);     
                    
					OLED_ShowString(60,4,(u8 *)"          ",16);	
					OLED_ShowString(60,4,(u8 *)">>>",16);
				}
				if(distance > 2000 && distance <3000)
				{
					OLED_ShowString(70,2,(u8 *)"2",16); 
                    
					OLED_ShowString(60,4,(u8 *)"          ",16);	
					OLED_ShowString(60,4,(u8 *)">>",16);
					
				}
				if(distance > 3000 && distance <4000)
				{
					OLED_ShowString(70,2,(u8 *)"1",16); 
                    
					OLED_ShowString(60,4,(u8 *)"          ",16);	
					OLED_ShowString(60,4,(u8 *)">",16);
                   
					
				}				
			}
			memset(dis_buf, 0, sizeof dis_buf);
			//传感器采样时间间隔			  
			OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)				
		}        				
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)		
	}
}

/*温湿度模块，任务1*/
void DHT11_Task(void *parg)
{
	int32_t rt = 0,sem_value =0;
	char *p = NULL;	
	uint8_t dht11_data[5],temp_buf[3], humi_buf[3];
    OS_MSG_SIZE msg_size=0;  	
	OS_ERR err;
	
	//温湿度模块初始化
	dht11_init();
	
    //清空缓冲区
    memset(temp_buf, 0, 3);
    memset(humi_buf, 0, 3);	
    memset(dht11_data, 0, 5);	
	
	printf("DHT11_Task is create ok\r\n");

    //添加代码
	while(1)
	{
        printf("DHT11_Task is running ...\r\n");
        //一直阻塞等待信号量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
		//等待按键TIME按下
		sem_value = OSSemPend(&SYNC_SEM_TH,0,OS_OPT_PEND_BLOCKING,NULL,&err);
//		if(sem_value != 0)
//        {
            //显示界面
            TH_UI();
            
            //检测按键
            while(1)
            {
                if(!PAin(0))
                {
                    delay_ms(100);
                    if(!PAin(0))
                    {
                        while(!PAin(0));                    
                        printf("DHT11_Task resume Sensors_Task\r\n");
                        //唤醒按键任务
                        OS_TaskResume(&Sensors_Task_TCB,&err);
                                            
                        //跳出按键检测，让任务重新阻塞
                        break;
                    }
                }       
                rt = dht11_read_data(dht11_data);
                if(rt == 0)
                {
                    printf("get data\r\n");
                    sprintf((char*)temp_buf, "%d.%dC",dht11_data[2],dht11_data[3]);
                    OLED_ShowString(40,3,(u8 *)temp_buf,16);           
                    sprintf((char*)humi_buf, "%%%d.%d",dht11_data[0],dht11_data[1]);
                    OLED_ShowString(40,6,(u8 *)humi_buf,16);

                    //

                    //温度OLED等级
                    if(dht11_data[2] >= 27 && dht11_data[2] <=30)
                    {
                        OLED_ShowString(85,3,(u8 *)"||",16);                     
                    }
                    if(dht11_data[2] >= 30 && dht11_data[2] <=32)
                    {
                        OLED_ShowString(85,3,(u8 *)"||||",16);                     
                    }   
                    //湿度OLED等级
                    if(dht11_data[0] >= 60 && dht11_data[0] <=65)
                    {
                        printf("humi oled\r\n");
                       
                    }
                    if(dht11_data[0] >= 65 && dht11_data[0] <=75)
                    {
                        OLED_ShowString(85,6,(u8 *)"||||",16);                     
                    } 

                    OLED_ShowString(85,6,(u8 *)"||",16);                  
                    
                }
                else
                {
                    //串口打印错误信息
                    printf("rt = %d \r\n",rt);	
                        
                } 

                memset(temp_buf, 0, 3);
                memset(humi_buf, 0, 3);	
                memset(dht11_data, 0, 5);    

                
                printf("detect temp & humi data\r\n");    
                //温湿度模块数据采样时间间隔需要6s以上，产品特性决定
                OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)            
            }
          
//       				
//		}
        //第一步，检测是否查询指令
//		p=OSQPend(&th_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
//		if(p && msg_size)
//        {
//            printf("select temp&humi  %s  %s\r\n",temp_buf ,humi_buf);
//            usart3_send_str((char*)temp_buf);
//            usart3_send_str((char*)humi_buf);            
//        }        
            
        //温湿度模块数据采样时间间隔需要6s以上，产品特性决定
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)		
	}
}


static volatile uint8_t buf[128];
static RTC_TimeTypeDef  	RTC_TimeStructure;
static RTC_DateTypeDef 		RTC_DateStructure;
uint8_t card_buf[4];
volatile uint8_t p_buf[4];
volatile uint8_t record[20];


void Flash_Task(void *parg)
{
	uint32_t i,rt,card_rec_cnt,new_card_flag = 0,card_flag = 0;
	char *p = NULL,*pcard = NULL,*p_card_queue = NULL;
    OS_MSG_SIZE msg_size=0;    
	OS_ERR err;
	
	memset(card_buf, 0 ,sizeof card_buf);
	memset((void *)p_buf, 0 ,sizeof p_buf);
	memset((void *)record, 0 ,sizeof record);	
	//w25qxx初始化
	w25qxx_init();
	//清空扇区
	rt = w25qxx_sector_erase(0);
	if(rt != 0)
		printf("erase failed\r\n");	
	
	printf("Flash_Task is create ok\r\n");

	while(1)
	{
		
		printf("Flash_Task is running ...\r\n");
		
		//一直阻塞等待互斥量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，msg_size为接收数据的大小，NULL不记录时间戳
		//返回值就是数据内容的指针，也就是指向数据的内容
        
        //第一步，检测是否有读卡动作
		p=OSQPend(&queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);

		if(p && msg_size)
		{
            printf("p:%s\r\n", p);			
			//判断消息的内容，如果是修改指令则获取
			if(strstr(p, "flash") != NULL)	
			{
				//一直阻塞等待互斥量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，NULL不记录时间戳
				OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);
//				printf("Flash_Task  recv msg：[%s],size:[%d]\r\n",p,msg_size);
				
				strtok(p, ":");
				pcard = strtok(NULL,":");
				
				strcpy((char*)card_buf, pcard);
				printf("pcard:%s\r\n", pcard);

				strcpy((char *)p_buf, pcard);	
				
				//仅向等待该最高优先级的任务发送信号量，互斥量的值会加1
				OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
				//尝试获取100条记录

				
				//查询是否存在该卡号
				for(i=0;i<100;i++)
				{
					memset((void*)buf, 0 ,sizeof buf);
					//获取存储的记录
					flash_read_record(buf,i);
					
					//检查记录是否存在换行符号，不存在则不打印输出
					if(strstr((const char *)buf,"\n")==0)
					{
						new_card_flag = 1;
						break;	
					}						
					else
					{
						//打印记录					
						OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
						printf("record:%s", buf);
						printf("p_buf:%s\r\n", p_buf);
						OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
						
						//判断是否存在该卡号
						if(strstr((const char *)buf,(const char*)p_buf) != NULL)
						{

							OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
							printf("the card has existed\r\n");
                            //串口3打印
                            //usart3_send_str("the card has existed\r\n");   
                            
							OSMutexPost(&mutex,OS_OPT_POST_1,&err);
							
							//如果存在该卡号且合法，即识别成功，则响一声,并点亮LED0						
							if(strstr((const char*)buf, "<1>") != NULL)
							{
								OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);
                                
								printf("the card is legal\r\n");
                                //串口3打印
                                //usart3_send_str("the card is legal\r\n");                                  
								OSMutexPost(&mutex,OS_OPT_POST_1,&err);								
								//开启蜂鸣器，并点亮LED0						
								PFout(8)=1;
								delay_ms(100);
								PFout(8)=0;	
								delay_ms(100);	

								PFout(8)=1;
								delay_ms(100);
								PFout(8)=0;	
								delay_ms(100);
																
								new_card_flag = 0;
								
								break;
								
							}
							//如果存在该卡号且不合法，则进行蜂鸣器长鸣报警，并熄灭Led0								
							else if(strstr((const char*)buf, "<0>") != NULL)
							{
								//蜂鸣器长鸣报警，并熄灭Led0					
								PFout(8)=1;
								delay_ms(100);
								PFout(8)=0;	
								delay_ms(100);		
								
								new_card_flag = 0;
								
								//continue;
								break;									
							}
								
						}
						else
						{
							continue;
						}
						OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
						printf("read buf:%s", buf);
						OSMutexPost(&mutex,OS_OPT_POST_1,&err);						
//						else		
//						{
//							//第一次刷卡
//							OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
//							printf("strang card\r\n");
//							OSMutexPost(&mutex,OS_OPT_POST_1,&err);									
//							new_card_flag = 1;
//							
//							break;
//						}						
					}
					delay_ms(1000);
				}				
				//如果i等于0，代表没有一条记录
				if(i==0)
				{
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
					printf("There is no record\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);				
				}
				
				//如果不存在则存储到flash中，即第一次录入card信息，记录为100条
				if(new_card_flag)
				{
					new_card_flag = 0;
					memset((void*)buf, 0 ,sizeof buf);	
					
					//尝试获取100条记录
					for(i=0;i<100;i++)
					{
						//获取存储的记录
						flash_read_record((char *)buf,i);
						
						//检查记录是否存在换行符号，不存在则不打印输出
						if(strstr((const char *)buf,"\n")==0)
							break;		
						
					}
					card_rec_cnt=i;
                    
                    //显示记录的总数
                    sprintf((char *)record, "total record:%d", i+1);
                    OLED_ShowString(0,2,(u8 *)record,16);
                    memset((void*)record,0 ,sizeof record);
                    
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
					printf("data records count=%d\r\n",card_rec_cnt);
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);					
					
					//检查当前的数据记录是否小于100条
					if(card_rec_cnt<100)
					{
						// 获取日期
						RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure);
						
						// 获取时间
						RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
						
						//写入之前清空缓冲区
						memset((void*)buf, 0 ,sizeof buf);						
						//格式化字符串,末尾添加\r\n作为一个结束标记，方便我们读取的时候进行判断
						sprintf((char *)buf,"[%03d]20%02x/%02x/%02x Week:%x %02x:%02x:%02x %s <1>\r\n", \
										card_rec_cnt,\
										RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay,\
										RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds,\
										p_buf);
					
						//写入读卡记录
						if(0==flash_write_record((char *)buf,card_rec_cnt))
						{
							//显示						
							OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
							printf("write:%s", buf);
							OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
							//记录自加1
							card_rec_cnt++;						
						}
						else
						{
							//数据记录清零，重头开始存储数据
							card_rec_cnt=0;
						}
						
					}
					else
					{
						//超过100条记录则打印
						printf("The record has reached 100 and cannot continue writing\r\n");
					}					
				}	
				delay_ms(500);							
				
			}
			
		}
        //判断是否有修改卡的指令
        //第二步，检测是否有修改卡号指令
		p_card_queue=OSQPend(&card_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_card_queue&&msg_size)
        {
            printf("select card id\r\n");
            printf("card com:%s\r\n", p_card_queue);
            card_flag = 0;
            //1，遍历flash，判断是否存在该卡号
            
            p_card_queue = strtok(p_card_queue, ":");
            p_card_queue = strtok(NULL, ":");
            
            printf("id:%s\r\n", p_card_queue);
            //尝试获取100条记录
            for(i=0;i<100;i++)
            {
                //获取存储的记录
                flash_read_record((char *)buf,i);
                
                //检查记录是否存在换行符号，不存在则不打印输出
                if(strstr((const char *)buf,"\n")==0)
                    break;
                else 
                {
                    printf("select records:%s", buf);
                    if(strstr((const char *)buf,p_card_queue)!= NULL)
                    {
                        printf("found the card\r\n");
                        usart3_send_str("found the card\r\n");                         
                        card_flag = 1;
//                        break;		
                    } 
                    else
                        continue;
                }                    
                memset((void *)buf,0, sizeof buf);

                                  
            }
             //2，如果不存在，通过蓝牙或者串口发送消息，提示用户
            if(i < 100 && (card_flag == 0))
            {
                printf("the card not in flash   %d\r\n", i);
                //串口3打印
                usart3_send_str("the card not in flash\r\n");                
                card_flag = 0;
            }

            //3，如果存在则修改卡的合法性
            if(card_flag == 1)
            {
                    //未定义
            }
                       
        }
        
		memset(p_card_queue, 0, (sizeof p_card_queue));
		memset(p, 0, strlen(p));

		OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
		printf("flash task ....\r\n");
		OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //延时1s，并让出CPU资源，使当前的任务进行睡眠		
	}


}

volatile uint8_t update_time_buf[64];

/*****蓝牙模块，任务2****/
void Blue_Task(void *parg)
{

	OS_ERR err;

    char *p = NULL;
    OS_MSG_SIZE msg_size=0;
	
	printf("Blue_Task is create ok\r\n");
    
    memset((void*)update_time_buf, 0 ,sizeof update_time_buf);
    //添加代码
    //初始化波特率为9600bps
	usart3_init(9600);
    
#if BLUE_DEBUG
	//配置蓝牙模块
	ble_set_config();
#endif

	while(1)
	{

        OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
        printf("Blue_Task is running ...\r\n");
        OSMutexPost(&mutex,OS_OPT_POST_1,&err);          
//使用消息队列
#if 1
		//一直阻塞等待互斥量,0代表是永久等待，OS_OPT_PEND_BLOCKING阻塞等待，msg_size为接收数据的大小，NULL不记录时间戳
		//返回值就是数据内容的指针，也就是指向数据的内容
		p=OSQPend(&command_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);
		
		if(p && msg_size)
		{	
			//判断消息的内容，如果是修改指令则获取
            
            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			printf("blue task recv msg：[%s],size:[%d]\r\n",p,msg_size);
            OSMutexPost(&mutex,OS_OPT_POST_1,&err);            

            //判断信息内容，如果是修改时间指令则发送指令给时间任务
            if(strstr(p, "DATE SET") != NULL)
            {
                printf("blue date com:%s\r\n", p);
                
                //发送指令
                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                printf("update_time_buf:%s\r\n", update_time_buf);
                
                OSQPost(&time_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
           
            }
            //修改日期
            if(strstr(p, "TIME SET") != NULL)
            {
                printf("blue time com:%s\r\n", p);  
                
                //发送指令
                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                OSQPost(&time_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }

            //如果是查询卡号指令，则发送指令给巡卡任务
            if(strstr(p, "CARD ID") != NULL)
            {
                printf("blue card com:%s\r\n", p);  
                
                //发送指令
                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                OSQPost(&card_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            } 
            //如果是查询温湿度指令，则发送指令给温湿度任务
            if(strstr(p, "TH") != NULL)
            {
                printf("blue th com:%s\r\n", p);  
                                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                printf("blue th %s", update_time_buf);
                OSQPost(&th_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }  
            //查询火焰传感器是否超标
            if(strstr(p, "FIRE") != NULL)
            {
                printf("blue fire com:%s\r\n", p);  
                                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                printf("blue fire %s", update_time_buf);
                OSQPost(&fire_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }             
                        
            //查询烟雾传感器是否超标
            if(strstr(p, "GAS") != NULL)
            {
                printf("blue gas com:%s\r\n", p);  
                                
                //使用先进先出的形式发送消息
                strcpy((void*)update_time_buf, p);
                printf("blue gas %s", update_time_buf);
                OSQPost(&gas_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            } 
            
		}
        //清空消息队列的内容
        memset((char *)p, 0, sizeof p);        
        //OSQFlush(&queue, &err);
        
//使用全局变量
#elif 0
        if(g_usart3_event==1)
        {

            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
            printf("%s", g_usart3_buf); 
            OSMutexPost(&mutex,OS_OPT_POST_1,&err);                
            g_usart3_event = 0;
            g_usart3_cnt = 0;
            memset((char *)g_usart3_buf, 0, sizeof g_usart3_buf);            
        }
        
#endif        
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时300ms,并让出CPU资源，使当前的任务进行睡眠
	}
            
	
}


/*查询任务*/
void Select_Task(void *parg)
{
	int32_t rt = 0,sem_value =0;
	char *p_th = NULL;	
	uint8_t dht11_data[5],temp_buf[8], humi_buf[8];
    OS_MSG_SIZE msg_size=0;  	
	OS_ERR err;
    OS_FLAGS flags; 

    
	uint32_t adc_val=0;
	uint32_t adc_vol=0;    
    uint8_t fire_value[20];
         		
    //清空缓冲区
    memset(temp_buf, 0, sizeof temp_buf);
    memset(humi_buf, 0, sizeof humi_buf);	
    memset(dht11_data, 0, sizeof dht11_data);	
    memset(fire_value, 0 ,sizeof fire_value);  
	
	printf("Select_Task is create ok\r\n");

    //添加代码
	while(1)
	{
        printf("Select_Task is running ...\r\n");
        
		p_th=OSQPend(&th_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_th&&msg_size)
        {   
            printf("start get th date\r\n");
            rt = dht11_read_data(dht11_data);
            if(rt == 0)
            {
                sprintf((char*)temp_buf, "T:%d.%dC",dht11_data[2],dht11_data[3]);
                sprintf((char*)humi_buf, " H:%%%d.%d\r\n",dht11_data[0],dht11_data[1]); 
                
                printf("select th result %s %s\r\n",temp_buf, humi_buf);                
                usart3_send_str((char*)temp_buf);
                usart3_send_str((char*)humi_buf);
            }
            else
            {
                //串口打印错误信息
                printf("rt = %d \r\n",rt);	
                    
            } 
            memset(temp_buf, 0, sizeof temp_buf);
            memset(humi_buf, 0, sizeof humi_buf);	
            memset(dht11_data, 0, sizeof dht11_data);             
        }


        //火焰
        p_th=OSQPend(&fire_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_th&&msg_size)
        {
                //启动ADC1的转换
                ADC_SoftwareStartConv(ADC1);
                
                //等待ADC1转换完成
                while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
                                
                //获取转化结果值
                adc_val=ADC_GetConversionValue(ADC1);
                
                //将结果值转换为电压值
                adc_vol=adc_val * 3300/4095;
                
                printf("adc_vol=%dmv\r\n",adc_vol);     
                
                if(adc_vol >= 20 && adc_vol <=1000)
                {
                    sprintf((char*)fire_value, "level=4,vol=%dmv", adc_vol);
                    usart3_send_str((char*)fire_value);
                }				
                if(adc_vol > 1000 && adc_vol <=2000)
                {
                    sprintf((char*)fire_value, "level=3,vol=%dmv", adc_vol);
                    usart3_send_str((char*)fire_value);  
                    
                }
                if(adc_vol > 2000 && adc_vol <=3000)
                {
                    sprintf((char*)fire_value, "level=2,vol=%dmv", adc_vol);
                    usart3_send_str((char*)fire_value);
                    
                }
                if(adc_vol > 3000 && adc_vol <=3300)
                {
                    sprintf((char*)fire_value, "level=1,vol=%dmv", adc_vol);
                    usart3_send_str((char*)fire_value);
                }                  
                memset(fire_value, 0 ,sizeof fire_value);                    
        }
                    
        
        //烟雾
		p_th=OSQPend(&gas_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_th&&msg_size)
        {
            //设置为不阻塞等待
            flags=OSFlagPend(&g_os_flag,0x04,0,OS_OPT_PEND_NON_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x04)
            {   
                usart3_send_str("warning\r\n");                 
                
            }
            else
                usart3_send_str("normal\r\n");  
        }                
               
        OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)            
    }	
}



/*距离传感器模块*/
void B_Task(void *parg)
{
	uint32_t distance = 0, i = 0;
	OS_ERR err;
	uint8_t dis_buf[10];
	uint32_t adc_val=0;
	uint32_t adc_vol=0;  
    uint8_t fire_value[3];
    
	memset(dis_buf, 0, sizeof dis_buf);
		
	printf("B_Task is create ok\r\n");

    //添加代码
	while(1)
	{
        printf("B_Task is running ...\r\n");


        distance = sr04_get_distance();								
        if(distance == 0xFFFFFFFF)
        {
            printf("sr04 get distance error\r\n");
            
        }			
        if(distance >=20 && distance<=4000)
        {			
            printf("distance=%dmm\r\n",distance);
            if(distance > 20 && distance <700)
            {
               
                //LED急速闪烁;                      
                for(i = 0; i<20;i++)
                {
                    PEout(13) = 0; 
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                    delay_ms(50);
                    PEout(13) = 1;
                    delay_ms(50);
//                    PFout(8) = 1; 
//                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
//                    delay_ms(100);
//                    PFout(8) = 0;
//                    delay_ms(50);
                        
                }                           
                                 
            }				
			
        }
        //实时监测火焰传感器
        ADC_SoftwareStartConv(ADC1);        
        //等待ADC1转换完成
        while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
               
        //获取转化结果值
        adc_val=ADC_GetConversionValue(ADC1);        
        //将结果值转换为电压值
        adc_vol=adc_val * 3300/4095;        
        printf("adc_vol=%dmv\r\n",adc_vol);       
        if(adc_vol >= 20 && adc_vol <=1000)
        {
                   
            for(i = 0; i<20;i++)
            {
                PEout(14) = 0; 
                //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                delay_ms(50);
                PEout(14) = 1;
                delay_ms(50);
                PFout(8) = 1; 
                //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                delay_ms(100);
                PFout(8) = 0;
                delay_ms(100);                
                //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
            }                                    
            
        }				
        if(adc_vol > 1000 && adc_vol <=2000)
        {
                                         
            for(i = 0; i<20;i++)
            {
                PEout(14) = 0; 
                //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                delay_ms(50);
                PEout(14) = 1;
                delay_ms(50);
                
                PFout(8) = 1; 
                //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                delay_ms(100);
                PFout(8) = 0;
                delay_ms(100);
             
        }
        if(adc_vol > 2000 && adc_vol <=3000)
        {

            
        }
        if(adc_vol > 3000 && adc_vol <=3300)
        {

            
        }         
        memset(fire_value, 0 ,sizeof fire_value);                           
        memset(dis_buf, 0, sizeof dis_buf);
        //传感器采样时间间隔			  
        OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_HMSM_STRICT,&err); //延时6s,并让出CPU资源，使当前的任务进行睡眠 sleep(1)				
	       				
        }
    }
}



























