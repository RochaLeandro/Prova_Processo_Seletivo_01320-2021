/**
 * @file main_app.c
 * @author Alisson Lopes Furlani (alisson.furlani@gmail.com)
 * @brief main application
 * @version 0.1
 * @date 2021-07-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/* System includes. */
#include <stdio.h>
#include <pthread.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

/* Priorities at which the tasks are created. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY    ( tskIDLE_PRIORITY + 3 )
#define mainQUEUE_SEND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 2 )
#define mainRUNTIME_TASK_PRIORITY          ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainTASK_SEND_FREQUENCY_MS         pdMS_TO_TICKS( 200UL )
#define mainTIMER_SEND_FREQUENCY_MS        pdMS_TO_TICKS( 2000UL )
#define mainRUNTIME_TASK_MS                pdMS_TO_TICKS( 3000UL )

/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH                   ( 2 )

/* The values sent to the queue receive task from the queue send task and the
 * queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK           ( 100UL )
#define mainVALUE_SENT_FROM_TIMER          ( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks.
 */
static void prvQueueReceiveTask( void * pvParameters );
static void prvQueueSendTask( void * pvParameters );
static void prvShowRunTimeStatus( void *pvParameters );

/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/**
 * @brief main application
 * 
 */
void main_app( void )
{
    const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

    /* Create the queue. */
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    if( xQueue != NULL )
    {
        /* Start the tasks. */
        xTaskCreate( prvQueueReceiveTask,             /* The function that implements the task. */
                     "Rx",                            /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                     configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                     NULL,                            /* The parameter passed to the task - not used in this simple case. */
                     mainQUEUE_RECEIVE_TASK_PRIORITY, /* The priority assigned to the task. */
                     NULL );                          /* The task handle is not required, so NULL is passed. */

        xTaskCreate( prvQueueSendTask, 
                     "TX", 
                     configMINIMAL_STACK_SIZE, 
                     NULL, 
                     mainQUEUE_SEND_TASK_PRIORITY, 
                     NULL );

        xTaskCreate( prvShowRunTimeStatus, 
                     "ShowRunTimeStatus", 
                     configMINIMAL_STACK_SIZE, 
                     NULL, 
                     mainRUNTIME_TASK_PRIORITY, 
                     NULL );

        /* Create the software timer, but don't start it yet. */
        xTimer = xTimerCreate( "Timer",                     /* The text name assigned to the software timer - for debug only as it is not used by the kernel. */
                               xTimerPeriod,                /* The period of the software timer in ticks. */
                               pdTRUE,                      /* xAutoReload is set to pdTRUE. */
                               NULL,                        /* The timer's ID is not used. */
                               prvQueueSendTimerCallback ); /* The function executed when the timer expires. */

        if( xTimer != NULL )
        {
            xTimerStart( xTimer, 0 );
        }

        /* Start the tasks and timer running. */
        vTaskStartScheduler();
    }

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks	to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    while( 1 );
}

/**
 * @brief task that enqueue data periodically
 * 
 * @param pvParameters 
 */
static void prvQueueSendTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
    const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    while( 1 )
    {
        /* Place this task in the blocked state until it is time to run again.
        *  The block time is specified in ticks, pdMS_TO_TICKS() was used to
        *  convert a time specified in milliseconds into a time specified in ticks.
        *  While in the Blocked state this task will not consume any CPU time. */
        vTaskDelayUntil( &xNextWakeTime, xBlockTime );

        /* Send to the queue - causing the queue receive task to unblock and
         * write to the console.  0 is used as the block time so the send operation
         * will not block - it shouldn't need to block as the queue should always
         * have at least one space at this point in the code. */
        xQueueSend( xQueue, &ulValueToSend, 0U );

        unsigned int i;
        for (i=0; i<10000000; i++)
        {
            __asm volatile ( "NOP" );
        }
    }
}

/**
 * @brief timer callback that enqueue data
 * 
 * @param xTimerHandle 
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
    const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

    /* This is the software timer callback function.  The software timer has a
     * period of two seconds and is reset each time a key is pressed.  This
     * callback function will execute if the timer expires, which will only happen
     * if a key is not pressed for two seconds. */

    /* Avoid compiler warnings resulting from the unused parameter. */
    ( void ) xTimerHandle;

    /* Send to the queue - causing the queue receive task to unblock and
     * write out a message.  This function is called from the timer/daemon task, so
     * must not block.  Hence the block time is set to 0. */
    xQueueSend( xQueue, &ulValueToSend, 0U );
}

/**
 * @brief task that dequeue data
 * 
 * @param pvParameters 
 */
static void prvQueueReceiveTask( void * pvParameters )
{
    uint32_t ulReceivedValue;

    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    while( 1 )
    {
        /* Wait until something arrives in the queue - this task will block
         * indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
         * FreeRTOSConfig.h.  It will not use any CPU time while it is in the
         * Blocked state. */
        xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

        /* To get here something must have been received from the queue, but
         * is it an expected value?  Normally calling printf() from a task is not
         * a good idea.  Here there is lots of stack space and only one task is
         * using console IO so it is ok.  However, there are risks of making Linux 
         * system calls (such as console output) from a FreeRTOS task. */
        if( ulReceivedValue == mainVALUE_SENT_FROM_TASK )
        {
            console_print( "Message received from task\n" );
        }
        else if( ulReceivedValue == mainVALUE_SENT_FROM_TIMER )
        {
            console_print( "Message received from software timer\n" );
        }
        else
        {
            console_print( "Unexpected message\n" );
        }
    }
}

/**
 * @brief task that shows tasks runtime status
 * 
 * @param pvParameters 
 * 
 */
static void prvShowRunTimeStatus( void *pvParameters )
{
	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;

	TickType_t xNextWakeTime;
	
	/* initialize pcWriteBuffer */
	char pcWriteBuffer[160];

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	while(1)
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, mainRUNTIME_TASK_MS );

		/* copy status to pcWriteBuffer and print on console */
		vTaskGetRunTimeStats(pcWriteBuffer);
		console_print("TASKS RUNTIME STATUS:\n%s", pcWriteBuffer);
	}
}
