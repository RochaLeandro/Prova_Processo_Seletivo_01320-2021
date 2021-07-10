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
#include "semphr.h"

/* Local includes. */
#include "console.h"

/* Priorities at which the tasks are created. */
#define mainADC_READ_TASK_PRIORITY              ( tskIDLE_PRIORITY + 3 )
#define mainSIGNAL_PROCESSING_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
#define mainSERIAL_INTERFACE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1 )
#define mainSHOW_RUNTIME_STATUS_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */
#define mainADC_READ_CYCLE_TIME_MS              pdMS_TO_TICKS( 1UL )
#define mainSIGNAL_PROCESSING_CYCLE_TIME_MS     pdMS_TO_TICKS( 100UL )
#define mainSHOW_RUNTIME_STATUS_CYCLE_TIME_MS   pdMS_TO_TICKS( 3000UL )

/*-----------------------------------------------------------*/

/*
 * The tasks.
 */
static void prvACDReadTask( void * pvParameters );
static void prvSignalProcessingTask( void * pvParameters );
static void prvSerialInterfaceTask( void * pvParameters );
static void prvShowRunTimeStatus( void *pvParameters );

/*-----------------------------------------------------------*/

/**
 * @brief main application
 * 
 */
void main_app( void )
{
    /* Start the tasks. */
    xTaskCreate( prvACDReadTask,                     /* The function that implements the task. */
                    "ACDRead",                       /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                    NULL,                            /* The parameter passed to the task - not used in this simple case. */
                    mainADC_READ_CYCLE_TIME_MS,      /* The priority assigned to the task. */
                    NULL );                          /* The task handle is not required, so NULL is passed. */

    xTaskCreate( prvSignalProcessingTask, 
                    "SignalProcessing", 
                    configMINIMAL_STACK_SIZE, 
                    NULL, 
                    mainSIGNAL_PROCESSING_TASK_PRIORITY, 
                    NULL );

    xTaskCreate( prvSerialInterfaceTask, 
                    "SerialInterface", 
                    configMINIMAL_STACK_SIZE, 
                    NULL, 
                    mainSERIAL_INTERFACE_TASK_PRIORITY, 
                    NULL );

    xTaskCreate( prvShowRunTimeStatus, 
                    "ShowRunTimeStatus", 
                    configMINIMAL_STACK_SIZE, 
                    NULL, 
                    mainSHOW_RUNTIME_STATUS_TASK_PRIORITY, 
                    NULL );

    /* Start the tasks and timer running. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks	to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    while( 1 );
}

/**
 * @brief task that reads ADC
 * 
 * @param pvParameters 
 */
static void prvACDReadTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainADC_READ_CYCLE_TIME_MS;

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
    }
}

/**
 * @brief task that process data
 * 
 * @param pvParameters 
 */
static void prvSignalProcessingTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainSIGNAL_PROCESSING_CYCLE_TIME_MS;

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
    }
}

/**
 * @brief task that monitors serial input
 * 
 * @param pvParameters 
 */
static void prvSerialInterfaceTask( void * pvParameters )
{
    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;

    while( 1 )
    {
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
	TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainSIGNAL_PROCESSING_CYCLE_TIME_MS;

	/* Prevent the compiler warning about the unused parameter. */
	( void ) pvParameters;
	
	/* initialize pcWriteBuffer */
	char pcWriteBuffer[300];

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	while(1)
	{
		/* Place this task in the blocked state until it is time to run again.
		The block time is specified in ticks, pdMS_TO_TICKS() was used to
		convert a time specified in milliseconds into a time specified in ticks.
		While in the Blocked state this task will not consume any CPU time. */
		vTaskDelayUntil( &xNextWakeTime, xBlockTime );

		/* copy status to pcWriteBuffer and print on console */
		vTaskGetRunTimeStats(pcWriteBuffer);
		console_print("TASKS RUNTIME STATUS:\n%s", pcWriteBuffer);
	}
}
