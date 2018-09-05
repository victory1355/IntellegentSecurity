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

OS_Q queue;						//������Ϣ����
OS_Q command_queue;						//������Ϣ����
OS_Q time_queue;						//������Ϣ����
OS_Q card_queue;						//������Ϣ����
OS_Q th_queue;						//������Ϣ����
OS_Q fire_queue;						//������Ϣ����
OS_Q gas_queue;						//������Ϣ����

OS_FLAG_GRP g_os_flag;			//�����¼���־��

OS_MUTEX mutex;					//����һ����������������Դ�ı���

OS_SEM SYNC_SEM;				//����һ���ź���������RTC���ݵ�ͬ��
OS_SEM SYNC_SEM_RED;				//����һ���ź��������ں���ģ���ͬ��
OS_SEM SYNC_SEM_Card;				//����һ���ź��������ں���ģ���ͬ��
OS_SEM SYNC_SEM_KEY;				//����һ���ź��������ڰ���ģ���ͬ��
OS_SEM SYNC_SEM_Sensors;				//����һ���ź��������ڴ�������ͬ��
OS_SEM SYNC_SEM_TH;				//����һ���ź�����������ʪ��ģ���ͬ��
OS_SEM SYNC_SEM_DISTANCE;				//����һ���ź��������ھ��봫����ģ���ͬ��

OS_SEM SYNC_SEM_FIRE;				//����һ���ź��������ڻ��洫����ģ���ͬ��
OS_SEM SYNC_SEM_GAS;				//����һ���ź��������ڿ�ȼ���崫����ģ���ͬ��


//����1DHT11���ƿ�
OS_TCB Dht11_Task_TCB;

void DHT11_Task(void *parg);

CPU_STK Dht11_Task_Stk[128*2];			//����2�������ջ����СΪ128�֣�Ҳ����512�ֽ�


//����2BLUE���ƿ�
OS_TCB Blue_Task_TCB;

void Blue_Task(void *parg);

CPU_STK Blue_Task_Stk[128];			//����3�������ջ����СΪ128�֣�Ҳ����512�ֽ�



//����3KEY���ƿ�
OS_TCB Key_Task_TCB;

void Key_Task(void *parg);

CPU_STK Key_Task_Stk[128*2*2*2];			//����5�������ջ����СΪ128�֣�Ҳ����512�ֽ�




//����4Rtc_Task���ƿ�
OS_TCB Rtc_Task_TCB;

void Rtc_Task(void *parg);

CPU_STK Rtc_Task_Stk[128*2];			//����5�������ջ����СΪ128�֣�Ҳ����512�ֽ�


//����5Red_Task���ƿ�
OS_TCB Red_Task_TCB;

void Red_Task(void *parg);

CPU_STK Red_Task_Stk[128*2];			//����5�������ջ����СΪ128�֣�Ҳ����512�ֽ�


//����6Card_Task���ƿ�
OS_TCB Card_Task_TCB;

void Card_Task(void *parg);

CPU_STK Card_Task_Stk[256];			//����5�������ջ����СΪ256�֣�Ҳ����256*4�ֽ�




//����7Flash_Task���ƿ�
OS_TCB Flash_Task_TCB;

void Flash_Task(void *parg);

CPU_STK Flash_Task_Stk[256*2];			//����5�������ջ����СΪ128�֣�Ҳ����512�ֽ�


//����8Key1234_Task���ƿ�
OS_TCB Key1234_Task_TCB;

void Key1234_Task(void *parg);

CPU_STK Key1234_Task_Stk[128];	



//����9Sensors_Task���ƿ�
OS_TCB Sensors_Task_TCB;

void Sensors_Task(void *parg);

CPU_STK Sensors_Task_Stk[128*2];	


//����10Distance_Task���ƿ�
OS_TCB Distance_Task_TCB;

void Distance_Task(void *parg);

CPU_STK Distance_Task_Stk[128*2];

//����11Fire_Task���ƿ�
OS_TCB Fire_Task_TCB;

void Fire_Task(void *parg);

CPU_STK Fire_Task_Stk[128*2];


//����12Gas_Task���ƿ�
OS_TCB Gas_Task_TCB;

void Gas_Task(void *parg);

CPU_STK Gas_Task_Stk[128*2];



//����13Select_Task���ƿ�
OS_TCB Select_Task_TCB;

void Select_Task(void *parg);

CPU_STK Select_Task_Stk[128*2];


//����14_Task���ƿ�
OS_TCB B_Task_TCB;

void B_Task(void *parg);

CPU_STK B_Task_Stk[128*2];





volatile uint8_t red_flag = 0;


//������
int main(void)
{
    static NVIC_InitTypeDef 	NVIC_InitStructure;		
    static EXTI_InitTypeDef  	EXTI_InitStructure;
    
	OS_ERR err;  
    
	delay_init(168);  													//ʱ�ӳ�ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);						//�жϷ�������
	uart_init(9600);  				 									//���ڳ�ʼ��
	
    //LED��ʼ��	    
    LED_Init(); 
	//������ʼ��
	key1234_config();	
    //���÷�����
    beep_config();
    
    OLED_Init();			//��ʼ��OLED  
    OLED_Clear(); 			//�����Ļ
    INIT_UI();
    boot_logo();  
    
    //��ʼ����ʱ��3
    //tim3_init();
    //ʹ�ö������Ź�
    //IWDG_Init();
    //ʹ�ô��ڿ��Ź�
    //WWDG_Init();
    
    //rtc��ʼ��
	if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x4567)
	{  
		rtc_init();
	}
	else
	{
		/* Enable the PWR clock ��ʹ�ܵ�Դʱ��*/
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		
		/* Allow access to RTC���������RTC���ݼĴ��� */
		PWR_BackupAccessCmd(ENABLE);
		
		/* Wait for RTC APB registers synchronisation���ȴ����е�RTC�Ĵ������� */
		RTC_WaitForSynchro();	
		

		//�رջ��ѹ���
		RTC_WakeUpCmd(DISABLE);
		
		//Ϊ���ѹ���ѡ��RTC���úõ�ʱ��Դ
		RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
		
		//���û��Ѽ���ֵΪ�Զ����أ�д��ֵĬ����0��1->0
		RTC_SetWakeUpCounter(0);
		
		//���RTC�����жϱ�־
		RTC_ClearITPendingBit(RTC_IT_WUT);
		
		//ʹ��RTC�����ж�
		RTC_ITConfig(RTC_IT_WUT, ENABLE);

		//ʹ�ܻ��ѹ���
		RTC_WakeUpCmd(ENABLE);			


		/* �����ⲿ�жϿ�����22��ʵ��RTC����*/
		EXTI_ClearITPendingBit(EXTI_Line22);
		EXTI_InitStructure.EXTI_Line = EXTI_Line22;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);
		
		/* ʹ��RTC�����ж� */
		NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
			
		
	}
          
    /* Check if the system has resumed from IWDG reset ����鵱ǰϵͳ��λ�Ƿ��п��Ź���λ����*/
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
    /* Clear reset flags��������и�λ��� */
	RCC_ClearFlag();
    
    
	//OS��ʼ�������ǵ�һ�����еĺ���,��ʼ�����ֵ�ȫ�ֱ����������ж�Ƕ�׼����������ȼ����洢��
	OSInit(&err);


	//����Key_Task�������ȼ����2
	OSTaskCreate(	(OS_TCB *)&Key_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Key_Task",									//���������
					(OS_TASK_PTR)Key_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)1,											 	//��������ȼ�		
					(CPU_STK *)Key_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2*2*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2*2*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);	       

#if 1					
	//����BLUE����
	OSTaskCreate(	(OS_TCB *)&Blue_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Blue_Task",									//���������
					(OS_TASK_PTR)Blue_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)4,											 	//��������ȼ�		
					(CPU_STK *)Blue_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);
#endif	               

                
	//����Rtc_Task����
	OSTaskCreate(	(OS_TCB *)&Rtc_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Rtc_Task",									//���������
					(OS_TASK_PTR)Rtc_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)3,											 	//��������ȼ�		
					(CPU_STK *)Rtc_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);  

	//����Red_Task
	OSTaskCreate(	(OS_TCB *)&Red_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Red_Task",									//���������
					(OS_TASK_PTR)Red_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)3,											 	//��������ȼ�		
					(CPU_STK *)Red_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				); 
					
	//����Card_Task
	OSTaskCreate(	(OS_TCB *)&Card_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Card_Task",									//���������
					(OS_TASK_PTR)Card_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)5,											 	//��������ȼ�		
					(CPU_STK *)Card_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)256/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)256,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				); 
					
	//����Flash_Task
	OSTaskCreate(	(OS_TCB *)&Flash_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Flash_Task",									//���������
					(OS_TASK_PTR)Flash_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)2,											 	//��������ȼ�		
					(CPU_STK *)Flash_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)256*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)256*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				); 					

	//����Key1234_Task
	OSTaskCreate(	(OS_TCB *)&Key1234_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Key1234_Task",									//���������
					(OS_TASK_PTR)Key1234_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)5,											 	//��������ȼ�		
					(CPU_STK *)Key1234_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				); 					
	//����Sensors_Task
	OSTaskCreate(	(OS_TCB *)&Sensors_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Sensors_Task",									//���������
					(OS_TASK_PTR)Sensors_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)2,											 	//��������ȼ�		
					(CPU_STK *)Sensors_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				); 						
#if 1
	//����Distance_Task����
	OSTaskCreate(	(OS_TCB *)&Distance_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Distance_Task",									//���������
					(OS_TASK_PTR)Distance_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)Distance_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif					
	

#if 1
	//����DHT11����
	OSTaskCreate(	(OS_TCB *)&Dht11_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Dht11_Task",									//���������
					(OS_TASK_PTR)DHT11_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)Dht11_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif	


#if 1
	//����Fire_Task����
	OSTaskCreate(	(OS_TCB *)&Fire_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Fire_Task",									//���������
					(OS_TASK_PTR)Fire_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)Fire_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif	

#if 1
	//����Gas_Task����
	OSTaskCreate(	(OS_TCB *)&Gas_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Gas_Task",									//���������
					(OS_TASK_PTR)Gas_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)Gas_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif	
#if 1
	//����Select_Task����
	OSTaskCreate(	(OS_TCB *)&Select_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"Select_Task",									//���������
					(OS_TASK_PTR)Select_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)Select_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif
#if 1
	//����B_Task����
	OSTaskCreate(	(OS_TCB *)&B_Task_TCB,									//������ƿ�
					(CPU_CHAR *)"B_Task",									//���������
					(OS_TASK_PTR)B_Task,										//������
					(void *)0,												//���ݲ���
					(OS_PRIO)6,											 	//��������ȼ�		
					(CPU_STK *)B_Task_Stk,									//�����ջ����ַ
					(CPU_STK_SIZE)128*2/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					(CPU_STK_SIZE)128*2,										//�����ջ��С			
					(OS_MSG_QTY)0,											//��ֹ������Ϣ����
					(OS_TICK)0,												//Ĭ��ʱ��Ƭ����																
					(void  *)0,												//����Ҫ�����û��洢��
					(OS_OPT)OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

#endif



					
	//����һ���ź������ź����ĳ�ֵΪ0
	OSSemCreate(&SYNC_SEM,"SYNC_SEM",0,&err);
	OSSemCreate(&SYNC_SEM_RED,"SYNC_SEM_RED",0,&err);
	OSSemCreate(&SYNC_SEM_Card,"SYNC_SEM_Card",0,&err);		
	OSSemCreate(&SYNC_SEM_KEY,"SYNC_SEM_KEY",0,&err);	
	OSSemCreate(&SYNC_SEM_Sensors,"SYNC_SEM_Sensors",0,&err);
	OSSemCreate(&SYNC_SEM_TH,"SYNC_SEM_TH",0,&err);						
	OSSemCreate(&SYNC_SEM_DISTANCE,"SYNC_SEM_DISTANCE",0,&err);	
	OSSemCreate(&SYNC_SEM_FIRE,"SYNC_SEM_FIRE",0,&err);	
    OSSemCreate(&SYNC_SEM_GAS,"SYNC_SEM_GAS",0,&err);	

	//��������������������
	OSMutexCreate(&mutex,"mutex",&err);					
    
    //�����¼���־��,���б�־λ0
	OSFlagCreate(&g_os_flag,"g_os_flag",0,&err);                     

    //������Ϣ���У�֧��64����Ϣ
	OSQCreate(&queue,"queue",64,&err);
	OSQCreate(&command_queue,"command_queue",64,&err);
	OSQCreate(&time_queue,"time_queue",64,&err);
	OSQCreate(&card_queue,"card_queue",64,&err);      
	OSQCreate(&th_queue,"th_queue",64,&err);   
	OSQCreate(&fire_queue,"fire_queue",64,&err);
	OSQCreate(&gas_queue,"gas_queue",64,&err);     
		
    //����OS�������������
	OSStart(&err);
}


















