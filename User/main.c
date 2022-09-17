//#include "stm32f10x.h"                  // Device header
#include "string.h"
#include <stdio.h>

#include "bps_led.h"
#include "bps_usart.h"
#include "key.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**************************** ȫ�ֱ��� ********************************/



/**************************** ������ ********************************/
/* 
 * ��������һ��ָ�룬����ָ��һ�����񣬵����񴴽���֮�����;�����һ��������
 * �Ժ�����Ҫ��������������Ҫͨ�������������������������������Լ�����ô
 * ����������ΪNULL��
 */
 /* ���������� */
static TaskHandle_t AppTaskCreate_Handle = NULL;
//���������ȼ�������
static TaskHandle_t  LowPriority_Task_Handle=NULL;
//���������ȼ�������
static TaskHandle_t  MidPriority_Task_Handle = NULL;
//���������ȼ�������
static TaskHandle_t  HighPriority_Task_Handle = NULL;

//��ֵ�ź������
SemaphoreHandle_t MuxSem_Handle=NULL;



//��������
static void LowPriority_Task(void* parameter);
static void MidPriority_Task(void* parameter);
static void HighPriority_Task(void* parameter);

static void AppTaskCreate(void);

static void BSP_Init(void)
{
	/* 
	* STM32 �ж����ȼ�����Ϊ 4���� 4bit ��������ʾ��ռ���ȼ�����ΧΪ��0~15 
	* ���ȼ�����ֻ��Ҫ����һ�μ��ɣ��Ժ������������������Ҫ�õ��жϣ� 
	* ��ͳһ��������ȼ����飬ǧ��Ҫ�ٷ��飬�мɡ� 
	*/ 
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 ); 
	LED_GPIO_Config();
	KEY1_GPIO_Config();
	KEY2_GPIO_Config();
	USART_Config();
	
	//����
//	led_G(on);
//	printf("���ڲ���");
}

int main()
{
	BaseType_t xReturn = NULL;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
	
	
	
	BSP_Init();
	printf("����ȫϵ�п�����-FreeRTOS-��̬��������!\r\n");

	  /* ����AppTaskCreate���� */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* ������ں��� */
                        (const char*    )"AppTaskCreate",/* �������� */
                        (uint16_t       )512,  /* ����ջ��С */
                        (void*          )NULL,/* ������ں������� */
                        (UBaseType_t    )1, /* ��������ȼ� */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* ������ƿ�ָ�� */ 
																							
	if(xReturn==pdPASS)
	{
		printf("��ʼ���񴴽��ɹ�\r\n");
		vTaskStartScheduler();
	}
	else 
	{
		return -1;
	}
	while(1)
	{
		
	}

}


//�����ȼ�������
static void LowPriority_Task(void* parameter)
{
	static uint32_t i;
	BaseType_t xReturn = pdTRUE;/* ����һ��������Ϣ����ֵ��Ĭ��Ϊ pdTRUE */
	while(1)
	{
		printf("LowPriority_Task ��ȡ�ź���\n"); 
		xReturn=xSemaphoreTake(MuxSem_Handle,portMAX_DELAY);//�����ź���
		if(xReturn==pdTRUE)
		{
			printf("�������е����ȼ�����\n"); 
			printf("���ڽ����������\n"); 
		}
		for (i=0; i<2000000; i++)//ģ������ȼ�����ռ���ź���    
		{
			taskYIELD();//�����������
		}
		printf("LowPriority_Task �ͷ��ź���!\r\n");

		//���������ź�����������ȼ������ڵȴ��û�����������ת��ִ�и����ȼ�����
		xReturn = xSemaphoreGive( MuxSem_Handle );//���������ź���

		
		printf("��������������û�б�����\n");//ʵ��֤���������е���������		
		LED_G_TOGGLE();
		
		vTaskDelay(1000); 
	}
}

//�����ȼ�������
static void MidPriority_Task(void* parameter)
{
	while(1)
	{
		printf("�������������ȼ�����\n"); 
		vTaskDelay(1000);/* ��ʱ 500 �� tick */ 
	}    
}


//�����ȼ�������
static void HighPriority_Task(void* parameter)
{
	BaseType_t xReturn = pdTRUE;/* ����һ��������Ϣ����ֵ��Ĭ��Ϊ pdTRUE */
	
	while(1)
	{
		printf("HighPriority_Task ��ȡ�ź���\n"); 
		xReturn=xSemaphoreTake(MuxSem_Handle,portMAX_DELAY);//�����ź���
		if(pdTRUE == xReturn)
		{
			printf("�������и����ȼ�����\n"); 
		}
//		else //�����ȡʧ�����в�������
//		{
//			printf("HighPriority_Task ��ȡ�ź���ʧ��\n");
//		}
		LED_G_TOGGLE(); 
		
		xReturn = xSemaphoreGive( MuxSem_Handle );//������ֵ�ź���
		printf("HighPriority_Task �ͷ��ź���\n"); 
		vTaskDelay(1000); 
	}    
}

static void AppTaskCreate(void)
{
	BaseType_t xReturn = NULL;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
	
	taskENTER_CRITICAL();           //�����ٽ���
	//����һ�������ź���
	MuxSem_Handle=xSemaphoreCreateMutex();//Ĭ����Ч�ź���


	//���������ȼ�������
  xReturn=xTaskCreate((TaskFunction_t	)LowPriority_Task,		//������
															(const char* 	)"LowPriority_Task",		//��������
															(uint16_t 		)512,	//�����ջ��С
															(void* 		  	)NULL,				//���ݸ��������Ĳ���
															(UBaseType_t 	)2, 	//�������ȼ�
															(TaskHandle_t*  )&LowPriority_Task_Handle);/* ������ƿ�ָ�� */ 	
															
	if(xReturn == pdPASS)/* �����ɹ� */
		printf("�����ȼ����񴴽��ɹ�!\n");
	else
		printf("�����ȼ����񴴽�ʧ��!\n");
	
	
	 //���������ȼ�����
	 xReturn=xTaskCreate((TaskFunction_t	)MidPriority_Task,		//������
															(const char* 	)"MidPriority_Task",		//��������
															(uint16_t 		)512,	//�����ջ��С
															(void* 		  	)NULL,				//���ݸ��������Ĳ���
															(UBaseType_t 	)3, 	//�������ȼ�
															(TaskHandle_t*  )&MidPriority_Task_Handle);/* ������ƿ�ָ�� */ 
															
	if(xReturn == pdPASS)/* �����ɹ� */
		printf("�����ȼ����񴴽��ɹ�!\n");
	else
		printf("�����ȼ����񴴽�ʧ��!\n");
	
	//���������ȼ�����
	 xReturn=xTaskCreate((TaskFunction_t	)HighPriority_Task,		//������
															(const char* 	)"HighPriority_Task",		//��������
															(uint16_t 		)512,	//�����ջ��С
															(void* 		  	)NULL,				//���ݸ��������Ĳ���
															(UBaseType_t 	)4, 	//�������ȼ�
															(TaskHandle_t*  )&HighPriority_Task_Handle);/* ������ƿ�ָ�� */ 
															
	if(xReturn == pdPASS)/* �����ɹ� */
		printf("�����ȼ����񴴽��ɹ�!\n");
	else
		printf("�����ȼ����񴴽�ʧ��!\n");
	
	vTaskDelete(AppTaskCreate_Handle); //ɾ��AppTaskCreate����
	
	taskEXIT_CRITICAL();            //�˳��ٽ���
}


//��̬�����������Ҫ
///**
//  **********************************************************************
//  * @brief  ��ȡ��������������ջ��������ƿ��ڴ�
//	*					ppxTimerTaskTCBBuffer	:		������ƿ��ڴ�
//	*					ppxTimerTaskStackBuffer	:	�����ջ�ڴ�
//	*					pulTimerTaskStackSize	:		�����ջ��С
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
//								   StackType_t **ppxIdleTaskStackBuffer, 
//								   uint32_t *pulIdleTaskStackSize)
//{
//	*ppxIdleTaskTCBBuffer=&Idle_Task_TCB;/* ������ƿ��ڴ� */
//	*ppxIdleTaskStackBuffer=Idle_Task_Stack;/* �����ջ�ڴ� */
//	*pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;/* �����ջ��С */
//}



///**
//  *********************************************************************
//  * @brief  ��ȡ��ʱ������������ջ��������ƿ��ڴ�
//	*					ppxTimerTaskTCBBuffer	:		������ƿ��ڴ�
//	*					ppxTimerTaskStackBuffer	:	�����ջ�ڴ�
//	*					pulTimerTaskStackSize	:		�����ջ��С
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
//									StackType_t **ppxTimerTaskStackBuffer, 
//									uint32_t *pulTimerTaskStackSize)
//{
//	*ppxTimerTaskTCBBuffer=&Timer_Task_TCB;/* ������ƿ��ڴ� */
//	*ppxTimerTaskStackBuffer=Timer_Task_Stack;/* �����ջ�ڴ� */
//	*pulTimerTaskStackSize=configTIMER_TASK_STACK_DEPTH;/* �����ջ��С */
//}
