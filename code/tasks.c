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


extern OS_FLAG_GRP g_os_flag;			//�����¼���־��
extern OS_Q queue;						//������Ϣ����
extern OS_Q command_queue;						//������Ϣ����
extern OS_Q time_queue;						//������Ϣ����
extern OS_Q card_queue;						//������Ϣ����
extern OS_Q th_queue;						//������Ϣ����
extern OS_Q fire_queue;						//������Ϣ����
extern OS_Q gas_queue;						//������Ϣ����

extern OS_MUTEX mutex;					//����һ����������������Դ�ı���

extern OS_SEM SYNC_SEM;				    //����һ���ź���������RTC���ݵ�ͬ��
extern OS_SEM SYNC_SEM_RED;				//����һ���ź��������ں���ģ���ͬ��
extern OS_SEM SYNC_SEM_Card;			//����һ���ź���������RFIDģ���ͬ��
extern OS_SEM SYNC_SEM_KEY;				//����һ���ź��������ڰ���ģ���ͬ��
extern OS_SEM SYNC_SEM_Sensors;				//����һ���ź��������ڴ�������ͬ��
extern OS_SEM SYNC_SEM_TH;				//����һ���ź�����������ʪ��ģ���ͬ��
extern OS_SEM SYNC_SEM_DISTANCE;				//����һ���ź��������ھ��봫����ģ���ͬ��
extern OS_SEM SYNC_SEM_FIRE;				//����һ���ź��������ڻ��洫����ģ���ͬ��
extern OS_SEM SYNC_SEM_GAS;				//����һ���ź��������ڿ�ȼ���崫����ģ���ͬ��


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
    //��ջ�����
    memset(date_buf, 0, 64);
    memset(time_buf, 0, 64);
        
    //��֤����������ִ��
    OSTimeDlyHMSM(0,0,0,300,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ300ms�����ó�CPU��Դ��ʹ��ǰ���������˯��	
	while(1)
	{
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
		//�ȴ�����TIME����
		OSSemPend(&SYNC_SEM,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
		          
        //��ʾʱ�����
        Time_UI();
        
        //��ⰴ��
        while(1)
        {
            //������°���3��������ϵͳʱ�䣬ͨ�������޸�ʱ��
            if(!PEin(3))
            {
                delay_ms(50);
                if(PEin(3))
                {
                    //����ʱ�����ý��棬δ����
                    printf("set time\r\n");
                 
                }
            }
            //������ܵ������޸�ʱ��ָ�ͨ�������޸�ʱ��
            
            //һֱ�����ȴ�������,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���msg_sizeΪ�������ݵĴ�С��NULL����¼ʱ���
            //����ֵ�����������ݵ�ָ�룬Ҳ����ָ�����ݵ�����
            p=OSQPend(&time_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);            
            if(p && msg_size)
            {	
                //�ж���Ϣ�����ݣ�������޸�ָ�����ȡ
                
                OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
                printf("rtc task recv msg��[%s],size:[%d]\r\n",p,msg_size);
                OSMutexPost(&mutex,OS_OPT_POST_1,&err); 
                if(strstr(p, "DATE") != NULL)
                {
                    //�޸�����
                    
                    printf("change date:%s\r\n",p);
                    //�жϽ��յ����ַ���ΪDATE SET
                    //ʾ����DATE SET-2017-10-12-4\n
                    
					//�ԵȺŷָ��ַ���
					strtok((char *)p,"-");
					
					//��ȡ��
					p_t=strtok(NULL,"-");
					
					//2018-2000=18 
					i = atoi(p_t)-2000;
					//ת��Ϊ16���� 18 ->0x18
					i= (i/10)*16+i%10;
					RTC_DateStructure.RTC_Year = i;
					
					//��ȡ��
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//ת��Ϊ16����
					i= (i/10)*16+i%10;						
					RTC_DateStructure.RTC_Month=i;


					//��ȡ��
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//ת��Ϊ16����
					i= (i/10)*16+i%10;		
					RTC_DateStructure.RTC_Date = i;
					
					//��ȡ����
					p_t=strtok(NULL,"-");
					i=atoi(p_t);
					//ת��Ϊ16����
					i= (i/10)*16+i%10;						
					RTC_DateStructure.RTC_WeekDay = i;
					
					//��������
					if(RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure) == SUCCESS)
                    {
                        //����1��Ϣ��ʾ
                        printf("set date ok\r\n");
                        
                        //����3��Ϣ��ʾ
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
                    //�޸�ʱ��
					//�ԵȺŷָ��ַ���
					strtok((char *)p,"-");
					
					//��ȡʱ
					p_t=strtok(NULL,"-");
					i = atoi(p_t);
					
					//ͨ��ʱ���ж���AM����PM
					if(i<12)
						RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
					else
						RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
						
					//ת��Ϊ16����
					i= (i/10)*16+i%10;
					RTC_TimeStructure.RTC_Hours   = i;
					
					//��ȡ��
					p_t=strtok(NULL,"-");
					i = atoi(p_t);						
					//ת��Ϊ16����
					i= (i/10)*16+i%10;	
					RTC_TimeStructure.RTC_Minutes = i;
					
					//��ȡ��
					p_t=strtok(NULL,"-");
					i = atoi(p_t);						
					//ת��Ϊ16����
					i= (i/10)*16+i%10;					
					RTC_TimeStructure.RTC_Seconds = i; 					
					
					if(RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure) == SUCCESS)
                    {
                        //����1��Ϣ��ʾ
                        printf("set time ok\r\n");  
                        //����3��Ϣ��ʾ
                        usart3_send_str("set time ok\r\n");                        
                    }
                    else
                    {
                        printf("set time failed\r\n");
                        usart3_send_str("set time failed\r\n");                              
                    }

                    
                }
		
            }            
            
            //������°���1���˳���ѭ�������ص����˵�
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("resume key task\r\n");
                    //���Ѱ�������
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    
                    //����������⣬��������������
                    break;
                }
            }
            
            //���ϸ���ʱ��
            flags=OSFlagPend(&g_os_flag,0x01,0,OS_OPT_PEND_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x01)
            {
                //��ȡ����
                RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure1);
                sprintf((char*)date_buf,"20%02x/%02x/%02xWeek:%x",RTC_DateStructure1.RTC_Year,RTC_DateStructure1.RTC_Month,RTC_DateStructure1.RTC_Date,RTC_DateStructure1.RTC_WeekDay);
                printf("20%02x/%02x/%02xWeek:%x\r\n",RTC_DateStructure1.RTC_Year,RTC_DateStructure1.RTC_Month,RTC_DateStructure1.RTC_Date,RTC_DateStructure1.RTC_WeekDay);                
                
                //ˢ������
                //x=24,��2����ʾ
                OLED_ShowString(0,2,(u8 *)date_buf,16);

                
                //��ȡʱ��
                RTC_GetTime(RTC_Format_BCD,&RTC_TimeStructure1);
                sprintf((char*)time_buf,"%02x:%02x:%02x",RTC_TimeStructure1.RTC_Hours,RTC_TimeStructure1.RTC_Minutes,RTC_TimeStructure1.RTC_Seconds);	
                //g_rtc_wakeup_event=0;
                
                //ˢ��ʱ��
                //x=24,��2����ʾ
                OLED_ShowString(0,4,(u8 *)time_buf,16);
            }
            //�����Ϣ���е�����
            memset((char *)p, 0, sizeof p);            
            printf("update time every second\r\n");
            //ÿ��5msɨ��һ�ΰ������ó�CPU��Դ��ʹ��ǰ���������˯��	
            OSTimeDlyHMSM(0,0,0,5,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1ms�����ó�CPU��Դ��ʹ��ǰ���������˯��	
            
        }
        printf("Rtc_Task is running ...\r\n");   
		OSTimeDlyHMSM(0,0,0,300,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s�����ó�CPU��Դ��ʹ��ǰ���������˯��	
        
	}


}




//��������
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
        //�л��˵�
                     
        //�����л�
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
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();
                    }
                    if(red_flag == 1)
                    {
                        printf("red task resume key tasks\r\n");
                        red_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();                    
                    }
                    if(sensor_flag == 1)
                    {
                        printf("sensors task resume key tasks\r\n");
                        sensor_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   
                    }
                    if(key_flag == 1)
                    {					
                        printf("key1234 task resume key tasks\r\n");
                        key_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   														
                    }                
                    if(card_flag == 1)
                    {                   
                        printf("red task resume key tasks\r\n");
                        card_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   				
                    }                
                                            
                }                
                
            }                
       
        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ10ms�����ó�CPU��Դ��ʹ��ǰ���������˯��		
	}
//	OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ10ms�����ó�CPU��Դ��ʹ��ǰ���������˯��

}


//��������
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
    
    
    OLED_Init();			//��ʼ��OLED  
    OLED_Clear(); 			//�����Ļ
    
    //��ʾ������
    Main_UI();


    //ģʽ��ѡ��
    while(1)
    {
        if(!PEin(3))//���°���1����־λ��ת�����°���2������Ӧ��ģʽ֮��
		{
            
            delay_ms(100);
            
            if(!PEin(3))
            {
                //����ģʽ
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
                //Ĭ��ģʽ
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
        
        OSTimeDlyHMSM(0,0,0,10  ,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ10ms�����ó�CPU��Դ��ʹ��ǰ���������˯��
        
    }

    //����˵�   
    Menu_UI();
    
	while(1)
	{
        //�л��˵�
                     
        //�����л�
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
//                        //����ʱ�����񣬹���ǰ����
//                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
//                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
//                        
//                        //����ǰ������
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //�����������ˢ�²˵�����                   
//                        Menu_UI();
//                    }
//                    if(red_flag == 1)
//                    {
//                        printf("red task resume key tasks\r\n");
//                        red_flag = 0;
//                        //����ʱ�����񣬹���ǰ����
//                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
//                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
//                        
//                        //����ǰ������
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //�����������ˢ�²˵�����                   
//                        Menu_UI();                    
//                    }
//                    if(sensor_flag == 1)
//                    {
//                        printf("sensors task resume key tasks\r\n");
//                        sensor_flag = 0;
//                        //����ʱ�����񣬹���ǰ����
//                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
//                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
//                        
//                        //����ǰ������
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //�����������ˢ�²˵�����                   
//                        Menu_UI();   
//                    }
//                    if(key_flag == 1)
//                    {					
//                        printf("key1234 task resume key tasks\r\n");
//                        key_flag = 0;
//                        //����ʱ�����񣬹���ǰ����
//                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
//                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
//                        
//                        //����ǰ������
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //�����������ˢ�²˵�����                   
//                        Menu_UI();   														
//                    }                
//                    if(card_flag == 1)
//                    {                   
//                        printf("red task resume key tasks\r\n");
//                        card_flag = 0;
//                        //����ʱ�����񣬹���ǰ����
//                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
//                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
//                        
//                        //����ǰ������
//                        OS_TaskSuspend(&Key_Task_TCB,&err);
//                        
//                        //�����������ˢ�²˵�����                   
//                        Menu_UI();   				
//                    }                
//                                            
//                }                
//                
//            }                
//        }
//        else
//        {
            //�����л�
            if(!PEin(4))//���°���1����־λ��ת�����°���2������Ӧ��ģʽ֮��
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
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();
                    }
                    if(red_flag == 1)
                    {
                        printf("red task resume key tasks\r\n");
                        red_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_RED,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();                    
                    }
                    if(sensor_flag == 1)
                    {
                        printf("sensors task resume key tasks\r\n");
                        sensor_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_Sensors,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   
                    }
                    if(key_flag == 1)
                    {					
                        printf("key1234 task resume key tasks\r\n");
                        key_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_KEY,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   														
                    }                
                    if(card_flag == 1)
                    {                   
                        printf("red task resume key tasks\r\n");
                        card_flag = 0;
                        //����ʱ�����񣬹���ǰ����
                        //����ȴ���������ȼ����������ź������ź�����ֵ���1
                        OSSemPost(&SYNC_SEM_Card,OS_OPT_POST_1,&err);
                        
                        //����ǰ������
                        OS_TaskSuspend(&Key_Task_TCB,&err);
                        
                        //�����������ˢ�²˵�����                   
                        Menu_UI();   				
                    }                
                    
                }
                
            }            
//        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ10ms�����ó�CPU��Դ��ʹ��ǰ���������˯��		
	}
	OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ10ms�����ó�CPU��Դ��ʹ��ǰ���������˯��

}



/*
	ע��û��ʹ���ⲿ�жϽ��м�⣬ʵʱ�Բ���
*/
void Red_Task(void *parg)
{
	OS_ERR err;
    
    int8_t rt;
	static volatile uint8_t ir_data[4];
    
    ir_init();
       
    //��ʼ����ʱ��14��pwmͨ��
    tim14_init();
    TIM_SetCompare1(TIM14,100);  
    
	printf("Red_Task is create ok\r\n");


    //��Ӵ���
    while(1)
    {
        //ͨ�������Ƿ���������ģʽ
        
        //����ģʽ
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
////                //���ݲ�ͬ����ֵʵ�ֿ���LED�ͷ�����
////                //Ϩ��
////                if(ir_data[2] == 0x07 && ir_data[3] == 0xF8)
////                {
////                    printf("get -\r\n");
////                    TIM_SetCompare1(TIM14,100);
////                    //delay_ms(500);
////                }
////                //����
////                if(ir_data[2] == 0x15 && ir_data[3] == 0xEA)
////                {
////                    printf("get +\r\n");
////                    TIM_SetCompare1(TIM14,0);
////                    //delay_ms(500);
////                }
////                //������Ч��
////                if(ir_data[2] == 0x16 && ir_data[3] == 0xE9)
////                {
////                    printf("get \r\n");

////                }
////				//���Ʒ�������δ����                
////                if(ir_data[2] == 0x1 && ir_data[3] == 0x0)
////                {
////                    printf("get \r\n");

////                    
////                }
//                //���Ʋ˵��л�����
//                if(ir_data[2] == 0x46 && ir_data[3] == 0xB9)
//                {
//                    //�����Ƕ��¼���־���bit3��1����
//                    printf("red get com:%02X %02X\r\n", ir_data[2],ir_data[3]);
//                    OSFlagPost(&g_os_flag,0x04,OS_OPT_POST_FLAG_SET,&err);

//                }
//            }          
//            else
//            {
//                //���ڴ�ӡ������Ϣ
//                printf("rt = %d \r\n",rt);	
//                    
//            }            
//        }
        
        //Ĭ��ģʽ
        printf("Red_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
        //�ȴ�����TIME����
        OSSemPend(&SYNC_SEM_RED,0,OS_OPT_PEND_BLOCKING,NULL,&err);             
        //��ʾ����
        Red_UI(); 

        //��ⰴ��
        while(1)
        {
                                         
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("red task resume key task\r\n");
                    //���Ѱ�������
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    //�رն�ʱ��14
                    TIM_Cmd(TIM14, DISABLE);
                    //����������⣬��������������
                    
                    break;
                }
            }
            memset((void *)ir_data, 0 ,sizeof ir_data);
            rt = ir_read_data((unsigned char *)ir_data);
            
            
            if(rt == 0)
            {
                printf("%02X %02X %02X %02X\r\n",ir_data[0],ir_data[1],ir_data[2],ir_data[3]);
                
                //���ݲ�ͬ����ֵʵ�ֿ���LED�ͷ�����
                //Ϩ��
                if(ir_data[2] == 0x07 && ir_data[3] == 0xF8)
                {
                    printf("get -\r\n");
                    TIM_SetCompare1(TIM14,100);
                    //delay_ms(500);
                }
                //����
                if(ir_data[2] == 0x15 && ir_data[3] == 0xEA)
                {
                    printf("get +\r\n");
                    TIM_SetCompare1(TIM14,0);
                    //delay_ms(500);
                }
                //������Ч��
                if(ir_data[2] == 0x16 && ir_data[3] == 0xE9)
                {
                    printf("breathe light\r\n");
//                    for(pwm_cmp=0; pwm_cmp<=500; pwm_cmp++)
//                    {
//                        printf("get 0\r\n");
//                        //���ñȽ�ֵ
//                        TIM_SetCompare1(TIM14,pwm_cmp);
//                        
//                        //��ʱ20ms
//                        delay_ms(20);
                }
                //���Ʒ�������δ����                
                if(ir_data[2] == 0x1 && ir_data[3] == 0x0)
                {
                    printf("control beep\r\n");
                    //����������
                    PFout(8)=1;
                    delay_ms(100);
                    PFout(8)=0;	
                    delay_ms(100);						
                    //����������
                    PFout(8)=1;
                    delay_ms(100);
                    PFout(8)=0;	
                    delay_ms(100);	
                    
                }
                //���Ʋ˵��л�����
                if(ir_data[2] == 0x46 && ir_data[3] == 0xB9)
                {
                    //�����Ƕ��¼���־���bit3��1����
                    OSFlagPost(&g_os_flag,0x04,OS_OPT_POST_FLAG_SET,&err);

                }
            }          
            else
            {
                //���ڴ�ӡ������Ϣ
                printf("rt = %d \r\n",rt);	
                    
            }
            printf("detect red signal\r\n");
                                        
            OSTimeDlyHMSM(0,0,0,3,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)            
                        
        }
        OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)
		
    }
}


//�������ģ��
void Key1234_Task(void *parg)
{  
	OS_ERR err;
		
	printf("Key1234_Task is create ok\r\n");

	while(1)
	{
		printf("Key1234_Task is running ...\r\n");		
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
        //�ȴ�����TIME����
        OSSemPend(&SYNC_SEM_KEY,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //��ʾ����
        Key_UI(); 
	
		while(1)
		{
			//����1
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
                    //���Ѱ�������
                    OS_TaskResume(&Key_Task_TCB,&err);                    
                    
                    //����������⣬��������������
                    break;						
				}
			}
			//����2
			if(!PEin(2))
			{
				delay_ms(50);
				if(PEin(2))
				{
					//�ȴ������ɿ�
					while(PEin(2));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("play music...\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);							
					
					//δ���壬��ʱ����չ					
				}
				
			}
			//����3
			if(!PEin(3))
			{
				delay_ms(50);
				if(!PEin(3))
				{
					//�ȴ������ɿ�
					while(PEin(3));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("open beep...\r\n");											
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
					
					//����������
					PFout(8)=1;
					delay_ms(100);
					PFout(8)=0;	
					delay_ms(100);						
					//����������
					PFout(8)=1;
					delay_ms(100);
					PFout(8)=0;	
					delay_ms(100);													  						
				}
					
			}
			//����4
			if(!PEin(4))
			{
				delay_ms(50);

				if(!PEin(4))
				{
					//�ȴ������ɿ�
					while(!PEin(4));
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
					printf("open led4...\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
					
					PEout(14) = 0;
					delay_ms(100);
					PEout(14) = 1;					             											
				}				
			}						
			OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ50ms�����ó�CPU��Դ��ʹ��ǰ���������˯��	
		}
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s�����ó�CPU��Դ��ʹ��ǰ���������˯��		
	}
}


//MFRC522������
extern uint8_t  mfrc552pidbuf[18];
extern uint8_t  card_pydebuf[2];
extern uint8_t  card_numberbuf[5];
extern uint8_t  card_key0Abuf[6];
extern uint8_t  card_writebuf[16];
extern uint8_t  card_readbuf[18];

uint32_t id=0;
//uint8_t  buf[64]={0};

volatile uint8_t cardid_buf[12];
//RFID����
void Card_Task(void *parg)
{      
	OS_ERR err;
	
	memset((void *)cardid_buf, 0 ,sizeof cardid_buf);
		
	MFRC522_Initializtion();
		
	printf("Card_Task is create ok\r\n");

	while(1)
	{
		
		printf("Card_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
        //�ȴ�����TIME����
        OSSemPend(&SYNC_SEM_Card,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //��ʾ����
        Card_UI(); 
		
        //��ⰴ��
        while(1)
        {                                      
            if(!PAin(0))
            {
                delay_ms(50);
                if(!PAin(0))
                {
                    while(!PAin(0));
                    printf("card task resume key task\r\n");
                    //���Ѱ�������
                    OS_TaskResume(&Key_Task_TCB,&err);
                    
                    
                    //����������⣬��������������
                    break;
                }
            }
			
			//һֱ�����ȴ�������,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
			OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
			//MFRC522
			MFRC522_Initializtion();
		
			//MFRC522Test();
			memset((void *)cardid_buf, 0 ,sizeof cardid_buf);			
			Get_Card_ID(cardid_buf);
			printf("card task cardid:%s\r\n", cardid_buf);
			//delay_ms(100);	
			//����ȴ���������ȼ����������ź�������������ֵ���1
			OSMutexPost(&mutex,OS_OPT_POST_1,&err);			
//			delay_ms(500);
			//end MFRC522
			
			//ʹ���Ƚ��ȳ�����ʽ������Ϣ        
			OSQPost(&queue,(uint8_t *)cardid_buf,sizeof cardid_buf,OS_OPT_POST_FIFO,&err);
			

			OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			printf("detect cardid\r\n");
			OSMutexPost(&mutex,OS_OPT_POST_1,&err);	                                       
            OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)   

			
		}
		
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s�����ó�CPU��Դ��ʹ��ǰ���������˯��	
	}
}


//���ִ�����
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
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
        //�ȴ�����TIME����
        OSSemPend(&SYNC_SEM_Sensors,0,OS_OPT_PEND_BLOCKING,NULL,&err);
        
        //��ʾ����
        Sensors_UI(); 
		
        //��ⰴ��
        while(1)
        {                                      
			//�л��˵�
			if(!PEin(4))//���°���1����־λ��ת�����°���2������Ӧ��ģʽ֮��
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
						//����ʱ�����񣬹���ǰ����
						//����ȴ���������ȼ����������ź������ź�����ֵ���1
						OSSemPost(&SYNC_SEM_FIRE,OS_OPT_POST_1,&err);
						
						//����ǰ������
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//�����������ˢ�²˵�����                   
						Sensors_UI(); 
					}
					if(distancesensor == 1)
					{
						printf("distancesensor resume distancesensor tasks\r\n");
						distancesensor = 0;
						//����ʱ�����񣬹���ǰ����
						//����ȴ���������ȼ����������ź������ź�����ֵ���1
						OSSemPost(&SYNC_SEM_DISTANCE,OS_OPT_POST_1,&err);
						
						//����ǰ������
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//�����������ˢ�²˵�����                   
						Sensors_UI();                 
					}
					if(gassensor == 1)
					{					
						printf("gassensor resume gassensor tasks\r\n");
						gassensor = 0;
						//����ʱ�����񣬹���ǰ����
						//����ȴ���������ȼ����������ź������ź�����ֵ���1
						OSSemPost(&SYNC_SEM_GAS,OS_OPT_POST_1,&err);
						
						//����ǰ������
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//�����������ˢ�²˵�����                   
						Sensors_UI(); 													
					}                
					if(temp_humisensor == 1)
					{                   
						printf("temp_humisensor resume temp_humisensor tasks\r\n");
						temp_humisensor = 0;
						//����ʱ�����񣬹���ǰ����
						//����ȴ���������ȼ����������ź������ź�����ֵ���1
						OSSemPost(&SYNC_SEM_TH,OS_OPT_POST_1,&err);
						
						//����ǰ������
						OS_TaskSuspend(&Sensors_Task_TCB,&err);
						
						//�����������ˢ�²˵�����                   
						Sensors_UI(); 			
					}                
					
				}
				
			}
			//���ص����˵�
			if(!PAin(0))
			{
				delay_ms(100);
				if(!PAin(0))
				{
					//�ȴ������ɿ�
					while(!PAin(0));
					
					//���Ѱ�������
                    printf("sensors task resume key task\r\n");
                    //���Ѱ�������
                    OS_TaskResume(&Key_Task_TCB,&err);
					break;
					
				}
			}
			printf("detect key\r\n");
            OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)   			
		}		
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s�����ó�CPU��Դ��ʹ��ǰ���������˯��	
	}

}

/*��ȼ���崫����ģ��*/
void Gas_Task(void *parg)
{
	OS_ERR err;
	uint32_t adc_val=0;
	uint32_t adc_vol=0;
	uint32_t dt=0,i; 
    uint8_t gas_value[3];
    OS_FLAGS flags;

    
    memset(gas_value, 0 ,sizeof gas_value);
    
	//����ADC��ʼ��
    //gas_adc_init();//PA2(ADC123_IN2) 
    extie6_init(); //PE6(EXTI)
    

    
	printf("Gas_Task is create ok\r\n");
    //��Ӵ���
	while(1)
	{
        printf("Gas_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
		//�ȴ�����TIME����
		OSSemPend(&SYNC_SEM_GAS,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //��ʾ����
        Gas_UI();

        //ʹ�ܶ�ʱ��4����
        //TIM_Cmd(TIM4, ENABLE);
        
        //��ⰴ��
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));                
					printf("Gas_Task resume Sensors_Task\r\n");
					//���Ѱ�������
					OS_TaskResume(&Sensors_Task_TCB,&err);

                    //ʹ�ܶ�ʱ��4����
//                    TIM_Cmd(TIM4, DISABLE);										
					//����������⣬��������������
					break;
				}
			}
            //����Ϊ�������ȴ�
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
            //����ADC1��ת��
            ADC_SoftwareStartConv(ADC1);                        
            //�ȴ�ת�����
            while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)!=SET);                        
            //��ȡADC1��ת�����
            adc_val=ADC_GetConversionValue(ADC1);            
            //�����ֵת��Ϊ��ѹֵ
            adc_vol=adc_val*3300/4095;
            
            //���洫�������л��棬�ͼ�⵽�ܵ͵ĵ�ѹֵΪ0mv����û�л��棬���⵽�ܸߵĵ�ѹֵ
            printf("adc_vol=%dmv\r\n",adc_vol);
                                 
            //OLED��ʾЧ��               
            if(adc_vol >= 20 && adc_vol <=1000)
            {
                OLED_ShowString(70,2,(u8 *)"4",16);            
                OLED_ShowString(60,4,(u8 *)"      ",16);	
                OLED_ShowString(60,4,(u8 *)">>>>>>",16);
                
                //LED������˸
//                    TIM_Cmd(TIM14, ENABLE);                      
                for(i = 0; i<20;i++)
                {
                    PEout(14) = 0; 
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);
                    delay_ms(50);
                    PEout(14) = 1;
                    delay_ms(50);
                    //OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_HMSM_STRICT,&err);                        
                }                 //ʹ�÷��������б���
                
                
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
                
            //����������ʱ����			         				
            OSTimeDlyHMSM(0,0,0,5,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)		
            printf("detect key to exit\r\n");
        }
        OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯��         
    }            
   
}

/*���洫����ģ��*/
void Fire_Task(void *parg)
{
	OS_ERR err;
	uint32_t adc_val=0,i;
	uint32_t adc_vol=0;
    OS_FLAGS flags;     
    uint8_t fire_value[3];
        
    memset(fire_value, 0 ,sizeof fire_value);
 
	//����ADC��ʼ��
	adc_init();   
    //exti8_init();
    tim4_init();
    TIM_Cmd(TIM4, DISABLE);	
        
	printf("Fire_Task is create ok\r\n");
    //��Ӵ���
	while(1)
	{
        printf("Fire_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
		//�ȴ�����TIME����
		OSSemPend(&SYNC_SEM_FIRE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //��ʾ����
        Fire_UI();

        //ʹ�ܶ�ʱ��4����
        TIM_Cmd(TIM4, ENABLE);
        
        //��ⰴ��
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));
					printf("Fire_Task resume Sensors_Task\r\n");
					//���Ѱ�������
					OS_TaskResume(&Sensors_Task_TCB,&err);

                    //ʹ�ܶ�ʱ��4����
                    TIM_Cmd(TIM4, DISABLE);										
					//����������⣬��������������
					break;
				}
			}
            //����Ϊ�������ȴ�
            flags=OSFlagPend(&g_os_flag,0x02,0,OS_OPT_PEND_NON_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x02)
            {
                printf("get time4 flag\r\n");
                //����ADC1��ת��
                ADC_SoftwareStartConv(ADC1);
                
                //�ȴ�ADC1ת�����
                while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
                
                
                //��ȡת�����ֵ
                adc_val=ADC_GetConversionValue(ADC1);
                
                //�����ֵת��Ϊ��ѹֵ
                adc_vol=adc_val * 3300/4095;
                
                printf("adc_vol=%dmv\r\n",adc_vol);
                
                //OLED��ʾЧ��
      
                
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
                    
                    
                    //LED������˸
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
                //����������ʱ����			  
                //OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)				
            }        				
            OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)		
            printf("detect key to exit\r\n");
        }
    }
}



/*���봫����ģ��*/
void Distance_Task(void *parg)
{
	uint32_t distance = 0, i = 0;
	OS_ERR err;
	uint8_t dis_buf[10];
		
	memset(dis_buf, 0, sizeof dis_buf);
	
	//������ģ���ʼ��
	sr04_init();
	
	printf("Distance_Task is create ok\r\n");

    //��Ӵ���
	while(1)
	{
        printf("Distance_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
		//�ȴ�����TIME����
		OSSemPend(&SYNC_SEM_DISTANCE,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		         
        //��ʾ����
        Distance_UI();
        
        //��ⰴ��
		while(1)
		{
			if(!PAin(0))
			{
				delay_ms(50);
				if(!PAin(0))
				{
                    while(!PAin(0));
					printf("Distance_Task resume Sensors_Task\r\n");
					//���Ѱ�������
					OS_TaskResume(&Sensors_Task_TCB,&err);
										
					//����������⣬��������������
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

				//OLED ������ʾЧ����δ����
				sprintf((char *)dis_buf, "%dmm", distance);
                OLED_ShowString(60,6,(u8 *)"      ",16);  
				OLED_ShowString(60,6,(u8 *)dis_buf,16);
				if(distance > 20 && distance <1000)
				{
					OLED_ShowString(70,2,(u8 *)"4",16);   
                    
					OLED_ShowString(60,4,(u8 *)"          ",16);	
					OLED_ShowString(60,4,(u8 *)">>>>>!",16);
                    
                    //LED������˸
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
                    //���ñȽ�ֵ                   
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
			//����������ʱ����			  
			OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)				
		}        				
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)		
	}
}

/*��ʪ��ģ�飬����1*/
void DHT11_Task(void *parg)
{
	int32_t rt = 0,sem_value =0;
	char *p = NULL;	
	uint8_t dht11_data[5],temp_buf[3], humi_buf[3];
    OS_MSG_SIZE msg_size=0;  	
	OS_ERR err;
	
	//��ʪ��ģ���ʼ��
	dht11_init();
	
    //��ջ�����
    memset(temp_buf, 0, 3);
    memset(humi_buf, 0, 3);	
    memset(dht11_data, 0, 5);	
	
	printf("DHT11_Task is create ok\r\n");

    //��Ӵ���
	while(1)
	{
        printf("DHT11_Task is running ...\r\n");
        //һֱ�����ȴ��ź���,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
		//�ȴ�����TIME����
		sem_value = OSSemPend(&SYNC_SEM_TH,0,OS_OPT_PEND_BLOCKING,NULL,&err);
//		if(sem_value != 0)
//        {
            //��ʾ����
            TH_UI();
            
            //��ⰴ��
            while(1)
            {
                if(!PAin(0))
                {
                    delay_ms(100);
                    if(!PAin(0))
                    {
                        while(!PAin(0));                    
                        printf("DHT11_Task resume Sensors_Task\r\n");
                        //���Ѱ�������
                        OS_TaskResume(&Sensors_Task_TCB,&err);
                                            
                        //����������⣬��������������
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

                    //�¶�OLED�ȼ�
                    if(dht11_data[2] >= 27 && dht11_data[2] <=30)
                    {
                        OLED_ShowString(85,3,(u8 *)"||",16);                     
                    }
                    if(dht11_data[2] >= 30 && dht11_data[2] <=32)
                    {
                        OLED_ShowString(85,3,(u8 *)"||||",16);                     
                    }   
                    //ʪ��OLED�ȼ�
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
                    //���ڴ�ӡ������Ϣ
                    printf("rt = %d \r\n",rt);	
                        
                } 

                memset(temp_buf, 0, 3);
                memset(humi_buf, 0, 3);	
                memset(dht11_data, 0, 5);    

                
                printf("detect temp & humi data\r\n");    
                //��ʪ��ģ�����ݲ���ʱ������Ҫ6s���ϣ���Ʒ���Ծ���
                OSTimeDlyHMSM(0,0,0,30,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)            
            }
          
//       				
//		}
        //��һ��������Ƿ��ѯָ��
//		p=OSQPend(&th_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
//		if(p && msg_size)
//        {
//            printf("select temp&humi  %s  %s\r\n",temp_buf ,humi_buf);
//            usart3_send_str((char*)temp_buf);
//            usart3_send_str((char*)humi_buf);            
//        }        
            
        //��ʪ��ģ�����ݲ���ʱ������Ҫ6s���ϣ���Ʒ���Ծ���
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)		
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
	//w25qxx��ʼ��
	w25qxx_init();
	//�������
	rt = w25qxx_sector_erase(0);
	if(rt != 0)
		printf("erase failed\r\n");	
	
	printf("Flash_Task is create ok\r\n");

	while(1)
	{
		
		printf("Flash_Task is running ...\r\n");
		
		//һֱ�����ȴ�������,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���msg_sizeΪ�������ݵĴ�С��NULL����¼ʱ���
		//����ֵ�����������ݵ�ָ�룬Ҳ����ָ�����ݵ�����
        
        //��һ��������Ƿ��ж�������
		p=OSQPend(&queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);

		if(p && msg_size)
		{
            printf("p:%s\r\n", p);			
			//�ж���Ϣ�����ݣ�������޸�ָ�����ȡ
			if(strstr(p, "flash") != NULL)	
			{
				//һֱ�����ȴ�������,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���NULL����¼ʱ���
				OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);
//				printf("Flash_Task  recv msg��[%s],size:[%d]\r\n",p,msg_size);
				
				strtok(p, ":");
				pcard = strtok(NULL,":");
				
				strcpy((char*)card_buf, pcard);
				printf("pcard:%s\r\n", pcard);

				strcpy((char *)p_buf, pcard);	
				
				//����ȴ���������ȼ����������ź�������������ֵ���1
				OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
				//���Ի�ȡ100����¼

				
				//��ѯ�Ƿ���ڸÿ���
				for(i=0;i<100;i++)
				{
					memset((void*)buf, 0 ,sizeof buf);
					//��ȡ�洢�ļ�¼
					flash_read_record(buf,i);
					
					//����¼�Ƿ���ڻ��з��ţ��������򲻴�ӡ���
					if(strstr((const char *)buf,"\n")==0)
					{
						new_card_flag = 1;
						break;	
					}						
					else
					{
						//��ӡ��¼					
						OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
						printf("record:%s", buf);
						printf("p_buf:%s\r\n", p_buf);
						OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
						
						//�ж��Ƿ���ڸÿ���
						if(strstr((const char *)buf,(const char*)p_buf) != NULL)
						{

							OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
							printf("the card has existed\r\n");
                            //����3��ӡ
                            //usart3_send_str("the card has existed\r\n");   
                            
							OSMutexPost(&mutex,OS_OPT_POST_1,&err);
							
							//������ڸÿ����ҺϷ�����ʶ��ɹ�������һ��,������LED0						
							if(strstr((const char*)buf, "<1>") != NULL)
							{
								OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);
                                
								printf("the card is legal\r\n");
                                //����3��ӡ
                                //usart3_send_str("the card is legal\r\n");                                  
								OSMutexPost(&mutex,OS_OPT_POST_1,&err);								
								//������������������LED0						
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
							//������ڸÿ����Ҳ��Ϸ�������з�����������������Ϩ��Led0								
							else if(strstr((const char*)buf, "<0>") != NULL)
							{
								//������������������Ϩ��Led0					
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
//							//��һ��ˢ��
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
				//���i����0������û��һ����¼
				if(i==0)
				{
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
					printf("There is no record\r\n");
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);				
				}
				
				//�����������洢��flash�У�����һ��¼��card��Ϣ����¼Ϊ100��
				if(new_card_flag)
				{
					new_card_flag = 0;
					memset((void*)buf, 0 ,sizeof buf);	
					
					//���Ի�ȡ100����¼
					for(i=0;i<100;i++)
					{
						//��ȡ�洢�ļ�¼
						flash_read_record((char *)buf,i);
						
						//����¼�Ƿ���ڻ��з��ţ��������򲻴�ӡ���
						if(strstr((const char *)buf,"\n")==0)
							break;		
						
					}
					card_rec_cnt=i;
                    
                    //��ʾ��¼������
                    sprintf((char *)record, "total record:%d", i+1);
                    OLED_ShowString(0,2,(u8 *)record,16);
                    memset((void*)record,0 ,sizeof record);
                    
					OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
					printf("data records count=%d\r\n",card_rec_cnt);
					OSMutexPost(&mutex,OS_OPT_POST_1,&err);					
					
					//��鵱ǰ�����ݼ�¼�Ƿ�С��100��
					if(card_rec_cnt<100)
					{
						// ��ȡ����
						RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure);
						
						// ��ȡʱ��
						RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
						
						//д��֮ǰ��ջ�����
						memset((void*)buf, 0 ,sizeof buf);						
						//��ʽ���ַ���,ĩβ���\r\n��Ϊһ��������ǣ��������Ƕ�ȡ��ʱ������ж�
						sprintf((char *)buf,"[%03d]20%02x/%02x/%02x Week:%x %02x:%02x:%02x %s <1>\r\n", \
										card_rec_cnt,\
										RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay,\
										RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds,\
										p_buf);
					
						//д�������¼
						if(0==flash_write_record((char *)buf,card_rec_cnt))
						{
							//��ʾ						
							OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
							printf("write:%s", buf);
							OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
							//��¼�Լ�1
							card_rec_cnt++;						
						}
						else
						{
							//���ݼ�¼���㣬��ͷ��ʼ�洢����
							card_rec_cnt=0;
						}
						
					}
					else
					{
						//����100����¼���ӡ
						printf("The record has reached 100 and cannot continue writing\r\n");
					}					
				}	
				delay_ms(500);							
				
			}
			
		}
        //�ж��Ƿ����޸Ŀ���ָ��
        //�ڶ���������Ƿ����޸Ŀ���ָ��
		p_card_queue=OSQPend(&card_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_card_queue&&msg_size)
        {
            printf("select card id\r\n");
            printf("card com:%s\r\n", p_card_queue);
            card_flag = 0;
            //1������flash���ж��Ƿ���ڸÿ���
            
            p_card_queue = strtok(p_card_queue, ":");
            p_card_queue = strtok(NULL, ":");
            
            printf("id:%s\r\n", p_card_queue);
            //���Ի�ȡ100����¼
            for(i=0;i<100;i++)
            {
                //��ȡ�洢�ļ�¼
                flash_read_record((char *)buf,i);
                
                //����¼�Ƿ���ڻ��з��ţ��������򲻴�ӡ���
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
             //2����������ڣ�ͨ���������ߴ��ڷ�����Ϣ����ʾ�û�
            if(i < 100 && (card_flag == 0))
            {
                printf("the card not in flash   %d\r\n", i);
                //����3��ӡ
                usart3_send_str("the card not in flash\r\n");                
                card_flag = 0;
            }

            //3������������޸Ŀ��ĺϷ���
            if(card_flag == 1)
            {
                    //δ����
            }
                       
        }
        
		memset(p_card_queue, 0, (sizeof p_card_queue));
		memset(p, 0, strlen(p));

		OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
		printf("flash task ....\r\n");
		OSMutexPost(&mutex,OS_OPT_POST_1,&err);	
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ1s�����ó�CPU��Դ��ʹ��ǰ���������˯��		
	}


}

volatile uint8_t update_time_buf[64];

/*****����ģ�飬����2****/
void Blue_Task(void *parg)
{

	OS_ERR err;

    char *p = NULL;
    OS_MSG_SIZE msg_size=0;
	
	printf("Blue_Task is create ok\r\n");
    
    memset((void*)update_time_buf, 0 ,sizeof update_time_buf);
    //��Ӵ���
    //��ʼ��������Ϊ9600bps
	usart3_init(9600);
    
#if BLUE_DEBUG
	//��������ģ��
	ble_set_config();
#endif

	while(1)
	{

        OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
        printf("Blue_Task is running ...\r\n");
        OSMutexPost(&mutex,OS_OPT_POST_1,&err);          
//ʹ����Ϣ����
#if 1
		//һֱ�����ȴ�������,0���������õȴ���OS_OPT_PEND_BLOCKING�����ȴ���msg_sizeΪ�������ݵĴ�С��NULL����¼ʱ���
		//����ֵ�����������ݵ�ָ�룬Ҳ����ָ�����ݵ�����
		p=OSQPend(&command_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);
		
		if(p && msg_size)
		{	
			//�ж���Ϣ�����ݣ�������޸�ָ�����ȡ
            
            OSMutexPend(&mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			printf("blue task recv msg��[%s],size:[%d]\r\n",p,msg_size);
            OSMutexPost(&mutex,OS_OPT_POST_1,&err);            

            //�ж���Ϣ���ݣ�������޸�ʱ��ָ������ָ���ʱ������
            if(strstr(p, "DATE SET") != NULL)
            {
                printf("blue date com:%s\r\n", p);
                
                //����ָ��
                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                printf("update_time_buf:%s\r\n", update_time_buf);
                
                OSQPost(&time_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
           
            }
            //�޸�����
            if(strstr(p, "TIME SET") != NULL)
            {
                printf("blue time com:%s\r\n", p);  
                
                //����ָ��
                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                OSQPost(&time_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }

            //����ǲ�ѯ����ָ�����ָ���Ѳ������
            if(strstr(p, "CARD ID") != NULL)
            {
                printf("blue card com:%s\r\n", p);  
                
                //����ָ��
                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                OSQPost(&card_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            } 
            //����ǲ�ѯ��ʪ��ָ�����ָ�����ʪ������
            if(strstr(p, "TH") != NULL)
            {
                printf("blue th com:%s\r\n", p);  
                                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                printf("blue th %s", update_time_buf);
                OSQPost(&th_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }  
            //��ѯ���洫�����Ƿ񳬱�
            if(strstr(p, "FIRE") != NULL)
            {
                printf("blue fire com:%s\r\n", p);  
                                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                printf("blue fire %s", update_time_buf);
                OSQPost(&fire_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            }             
                        
            //��ѯ���������Ƿ񳬱�
            if(strstr(p, "GAS") != NULL)
            {
                printf("blue gas com:%s\r\n", p);  
                                
                //ʹ���Ƚ��ȳ�����ʽ������Ϣ
                strcpy((void*)update_time_buf, p);
                printf("blue gas %s", update_time_buf);
                OSQPost(&gas_queue,(uint8_t *)update_time_buf,strlen((const char*)update_time_buf),OS_OPT_POST_FIFO,&err);
                  
            } 
            
		}
        //�����Ϣ���е�����
        memset((char *)p, 0, sizeof p);        
        //OSQFlush(&queue, &err);
        
//ʹ��ȫ�ֱ���
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
		OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ300ms,���ó�CPU��Դ��ʹ��ǰ���������˯��
	}
            
	
}


/*��ѯ����*/
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
         		
    //��ջ�����
    memset(temp_buf, 0, sizeof temp_buf);
    memset(humi_buf, 0, sizeof humi_buf);	
    memset(dht11_data, 0, sizeof dht11_data);	
    memset(fire_value, 0 ,sizeof fire_value);  
	
	printf("Select_Task is create ok\r\n");

    //��Ӵ���
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
                //���ڴ�ӡ������Ϣ
                printf("rt = %d \r\n",rt);	
                    
            } 
            memset(temp_buf, 0, sizeof temp_buf);
            memset(humi_buf, 0, sizeof humi_buf);	
            memset(dht11_data, 0, sizeof dht11_data);             
        }


        //����
        p_th=OSQPend(&fire_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_th&&msg_size)
        {
                //����ADC1��ת��
                ADC_SoftwareStartConv(ADC1);
                
                //�ȴ�ADC1ת�����
                while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
                                
                //��ȡת�����ֵ
                adc_val=ADC_GetConversionValue(ADC1);
                
                //�����ֵת��Ϊ��ѹֵ
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
                    
        
        //����
		p_th=OSQPend(&gas_queue,0,OS_OPT_PEND_NON_BLOCKING,&msg_size,NULL,&err);
        if(p_th&&msg_size)
        {
            //����Ϊ�������ȴ�
            flags=OSFlagPend(&g_os_flag,0x04,0,OS_OPT_PEND_NON_BLOCKING|OS_OPT_PEND_FLAG_SET_ANY|OS_OPT_PEND_FLAG_CONSUME,NULL,&err);
            if(flags&0x04)
            {   
                usart3_send_str("warning\r\n");                 
                
            }
            else
                usart3_send_str("normal\r\n");  
        }                
               
        OSTimeDlyHMSM(0,0,1,0,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)            
    }	
}



/*���봫����ģ��*/
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

    //��Ӵ���
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
               
                //LED������˸;                      
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
        //ʵʱ�����洫����
        ADC_SoftwareStartConv(ADC1);        
        //�ȴ�ADC1ת�����
        while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);
               
        //��ȡת�����ֵ
        adc_val=ADC_GetConversionValue(ADC1);        
        //�����ֵת��Ϊ��ѹֵ
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
        //����������ʱ����			  
        OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_HMSM_STRICT,&err); //��ʱ6s,���ó�CPU��Դ��ʹ��ǰ���������˯�� sleep(1)				
	       				
        }
    }
}



























