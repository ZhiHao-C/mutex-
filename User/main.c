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

/**************************** 全局变量 ********************************/



/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
 /* 创建任务句柄 */
static TaskHandle_t AppTaskCreate_Handle = NULL;
//创建低优先级任务句柄
static TaskHandle_t  LowPriority_Task_Handle=NULL;
//创建中优先级任务句柄
static TaskHandle_t  MidPriority_Task_Handle = NULL;
//创建高优先级任务句柄
static TaskHandle_t  HighPriority_Task_Handle = NULL;

//二值信号量句柄
SemaphoreHandle_t MuxSem_Handle=NULL;



//声明函数
static void LowPriority_Task(void* parameter);
static void MidPriority_Task(void* parameter);
static void HighPriority_Task(void* parameter);

static void AppTaskCreate(void);

static void BSP_Init(void)
{
	/* 
	* STM32 中断优先级分组为 4，即 4bit 都用来表示抢占优先级，范围为：0~15 
	* 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断， 
	* 都统一用这个优先级分组，千万不要再分组，切忌。 
	*/ 
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 ); 
	LED_GPIO_Config();
	KEY1_GPIO_Config();
	KEY2_GPIO_Config();
	USART_Config();
	
	//测试
//	led_G(on);
//	printf("串口测试");
}

int main()
{
	BaseType_t xReturn = NULL;/* 定义一个创建信息返回值，默认为pdPASS */
	
	
	
	BSP_Init();
	printf("这是全系列开发板-FreeRTOS-动态创建任务!\r\n");

	  /* 创建AppTaskCreate任务 */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* 任务入口函数 */
                        (const char*    )"AppTaskCreate",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )1, /* 任务的优先级 */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* 任务控制块指针 */ 
																							
	if(xReturn==pdPASS)
	{
		printf("初始任务创建成功\r\n");
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


//低优先级任务函数
static void LowPriority_Task(void* parameter)
{
	static uint32_t i;
	BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为 pdTRUE */
	while(1)
	{
		printf("LowPriority_Task 获取信号量\n"); 
		xReturn=xSemaphoreTake(MuxSem_Handle,portMAX_DELAY);//死等信号量
		if(xReturn==pdTRUE)
		{
			printf("正在运行低优先级任务\n"); 
			printf("正在进行任务调度\n"); 
		}
		for (i=0; i<2000000; i++)//模拟低优先级任务占用信号量    
		{
			taskYIELD();//发起任务调度
		}
		printf("LowPriority_Task 释放信号量!\r\n");

		//给出互斥信号量如果高优先级任务在等待该互斥量则立即转换执行高优先级任务
		xReturn = xSemaphoreGive( MuxSem_Handle );//给出互斥信号量

		
		printf("测试这条代码有没有被运行\n");//实验证明可以运行到这条代码		
		LED_G_TOGGLE();
		
		vTaskDelay(1000); 
	}
}

//中优先级任务函数
static void MidPriority_Task(void* parameter)
{
	while(1)
	{
		printf("正在运行中优先级任务\n"); 
		vTaskDelay(1000);/* 延时 500 个 tick */ 
	}    
}


//高优先级任务函数
static void HighPriority_Task(void* parameter)
{
	BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为 pdTRUE */
	
	while(1)
	{
		printf("HighPriority_Task 获取信号量\n"); 
		xReturn=xSemaphoreTake(MuxSem_Handle,portMAX_DELAY);//死等信号量
		if(pdTRUE == xReturn)
		{
			printf("正在运行高优先级任务\n"); 
		}
//		else //如果获取失败运行不到这里
//		{
//			printf("HighPriority_Task 获取信号量失败\n");
//		}
		LED_G_TOGGLE(); 
		
		xReturn = xSemaphoreGive( MuxSem_Handle );//给出二值信号量
		printf("HighPriority_Task 释放信号量\n"); 
		vTaskDelay(1000); 
	}    
}

static void AppTaskCreate(void)
{
	BaseType_t xReturn = NULL;/* 定义一个创建信息返回值，默认为pdPASS */
	
	taskENTER_CRITICAL();           //进入临界区
	//创建一个互斥信号量
	MuxSem_Handle=xSemaphoreCreateMutex();//默认有效信号量


	//创建低优先级任务函数
  xReturn=xTaskCreate((TaskFunction_t	)LowPriority_Task,		//任务函数
															(const char* 	)"LowPriority_Task",		//任务名称
															(uint16_t 		)512,	//任务堆栈大小
															(void* 		  	)NULL,				//传递给任务函数的参数
															(UBaseType_t 	)2, 	//任务优先级
															(TaskHandle_t*  )&LowPriority_Task_Handle);/* 任务控制块指针 */ 	
															
	if(xReturn == pdPASS)/* 创建成功 */
		printf("低优先级任务创建成功!\n");
	else
		printf("低优先级任务创建失败!\n");
	
	
	 //创建中优先级任务
	 xReturn=xTaskCreate((TaskFunction_t	)MidPriority_Task,		//任务函数
															(const char* 	)"MidPriority_Task",		//任务名称
															(uint16_t 		)512,	//任务堆栈大小
															(void* 		  	)NULL,				//传递给任务函数的参数
															(UBaseType_t 	)3, 	//任务优先级
															(TaskHandle_t*  )&MidPriority_Task_Handle);/* 任务控制块指针 */ 
															
	if(xReturn == pdPASS)/* 创建成功 */
		printf("中优先级任务创建成功!\n");
	else
		printf("中优先级任务创建失败!\n");
	
	//创建高优先级任务
	 xReturn=xTaskCreate((TaskFunction_t	)HighPriority_Task,		//任务函数
															(const char* 	)"HighPriority_Task",		//任务名称
															(uint16_t 		)512,	//任务堆栈大小
															(void* 		  	)NULL,				//传递给任务函数的参数
															(UBaseType_t 	)4, 	//任务优先级
															(TaskHandle_t*  )&HighPriority_Task_Handle);/* 任务控制块指针 */ 
															
	if(xReturn == pdPASS)/* 创建成功 */
		printf("高优先级任务创建成功!\n");
	else
		printf("高优先级任务创建失败!\n");
	
	vTaskDelete(AppTaskCreate_Handle); //删除AppTaskCreate任务
	
	taskEXIT_CRITICAL();            //退出临界区
}


//静态创建任务才需要
///**
//  **********************************************************************
//  * @brief  获取空闲任务的任务堆栈和任务控制块内存
//	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
//	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
//	*					pulTimerTaskStackSize	:		任务堆栈大小
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
//								   StackType_t **ppxIdleTaskStackBuffer, 
//								   uint32_t *pulIdleTaskStackSize)
//{
//	*ppxIdleTaskTCBBuffer=&Idle_Task_TCB;/* 任务控制块内存 */
//	*ppxIdleTaskStackBuffer=Idle_Task_Stack;/* 任务堆栈内存 */
//	*pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;/* 任务堆栈大小 */
//}



///**
//  *********************************************************************
//  * @brief  获取定时器任务的任务堆栈和任务控制块内存
//	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
//	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
//	*					pulTimerTaskStackSize	:		任务堆栈大小
//  * @author  fire
//  * @version V1.0
//  * @date    2018-xx-xx
//  **********************************************************************
//  */ 
//void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
//									StackType_t **ppxTimerTaskStackBuffer, 
//									uint32_t *pulTimerTaskStackSize)
//{
//	*ppxTimerTaskTCBBuffer=&Timer_Task_TCB;/* 任务控制块内存 */
//	*ppxTimerTaskStackBuffer=Timer_Task_Stack;/* 任务堆栈内存 */
//	*pulTimerTaskStackSize=configTIMER_TASK_STACK_DEPTH;/* 任务堆栈大小 */
//}
