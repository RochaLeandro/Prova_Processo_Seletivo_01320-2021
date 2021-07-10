/**
 * @file main.c
 * @author Alisson Lopes Furlani (alisson.furlani@gmail.com)
 * @brief FreeRTOS init
 * @version 0.1
 * @date 2021-07-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <signal.h>
#include <errno.h>
#include <sys/select.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Local includes. */
#include "console.h"

/* This demo uses heap_3.c (the libc provided malloc() and free()). */

/*-----------------------------------------------------------*/

/* main app */
extern void main_app( void );

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.  See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize );

/*
 * Signal handler for Ctrl_C to cause the program to exit, and generate the
 * profiling info.
 */
static void handle_sigint( int signal );

/*-----------------------------------------------------------*/

/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
 * use a callback function to optionally provide the memory required by the idle
 * and timer tasks.  This is the stack that will be used by the timer task.  It is
 * declared here, as a global, so it can be checked by a test that is implemented
 * in a different file. */
StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

/*-----------------------------------------------------------*/

/**
 * @brief Main
 * 
 * @return int 
 */
int main( void )
{
    /* SIGINT is not blocked by the posix port */
    signal( SIGINT, handle_sigint );

    console_init();
    console_print( "Starting main app\n" );
    main_app();

    return 0;
}

/**
 * @brief vApplicationMallocFailedHook() will only be called if
 *      configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
 *      function that will get called if a call to pvPortMalloc() fails.
 *      pvPortMalloc() is called internally by the kernel whenever a task, queue,
 *      timer or semaphore is created.  It is also called by various parts of the
 *      demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
 *      size of the    heap available to pvPortMalloc() is defined by
 *      configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
 *      API function can be used to query the size of free heap space that remains
 *      (although it does not provide information on how the remaining heap might be
 *      fragmented).  See http://www.freertos.org/a00111.html for more
 *      information.
 * 
 */
void vApplicationMallocFailedHook( void )
{
    vAssertCalled( __FILE__, __LINE__ );
}

/**
 * @brief vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
 *      to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
 *      task.  It is essential that code added to this hook function never attempts
 *      to block in any way (for example, call xQueueReceive() with a block time
 *      specified, or call vTaskDelay()).  If application tasks make use of the
 *      vTaskDelete() API function to delete themselves then it is also important
 *      that vApplicationIdleHook() is permitted to return to its calling function,
 *      because it is the responsibility of the idle task to clean up memory
 *      allocated by the kernel to any task that has since deleted itself.
 * 
 */
void vApplicationIdleHook( void )
{
    usleep( 15000 );
}

/**
 * @brief Run time stack overflow checking is performed if
 *      configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
 *      function is called if a stack overflow is detected.  This function is
 *      provided as an example only as stack overflow checking does not function
 *      when running the FreeRTOS POSIX port.
 * 
 * @param pxTask 
 * @param pcTaskName 
 */
void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    vAssertCalled( __FILE__, __LINE__ );
}

/**
 * @brief This function will be called by each tick interrupt if
 *      configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
 *      added here, but the tick hook is called from an interrupt context, so
 *      code must not attempt to block, and only the interrupt safe FreeRTOS API
 *      functions can be used (those that end in FromISR()).
 * 
 */
void vApplicationTickHook( void )
{
}

/**
 * @brief 
 * 
 * @param pcFormat 
 * @param ... 
 */
void vLoggingPrintf( const char * pcFormat,
                     ... )
{
    va_list arg;

    va_start( arg, pcFormat );
    vprintf( pcFormat, arg );
    va_end( arg );
}

/**
 * @brief This function will be called once only, when the daemon task starts to
 *      execute    (sometimes called the timer task).  This is useful if the
 *      application includes initialisation code that would benefit from executing
 *      after the scheduler has been started.
 * 
 */
void vApplicationDaemonTaskStartupHook( void )
{
}

/**
 * @brief Called if an assertion passed to configASSERT() fails.  See
 *      http://www.freertos.org/a00110.html#configASSERT for more information.
 * 
 * @param pcFileName 
 * @param ulLine 
 */
void vAssertCalled( const char * const pcFileName,
                    unsigned long ulLine )
{
    static BaseType_t xPrinted = pdFALSE;
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Parameters are not used. */
    ( void ) ulLine;
    ( void ) pcFileName;


    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
         * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
         * value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
            __asm volatile ( "NOP" );
            __asm volatile ( "NOP" );
        }
    }
    taskEXIT_CRITICAL();
}

/*-----------------------------------------------------------*/

/**
 * @brief configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 *      implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 *      used by the Idle task.
 * 
 * @param ppxIdleTaskTCBBuffer 
 * @param ppxIdleTaskStackBuffer 
 * @param pulIdleTaskStackSize 
 */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/**
 * @brief configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 *      application must provide an implementation of vApplicationGetTimerTaskMemory()
 *      to provide the memory that is used by the Timer service task.
 * 
 * @param ppxTimerTaskTCBBuffer 
 * @param ppxTimerTaskStackBuffer 
 * @param pulTimerTaskStackSize 
 */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/**
 * @brief 
 * 
 * @param signal 
 */
void handle_sigint( int signal )
{
    console_print( "\nExecution stopped by user (by hitting Ctrl+C)\n" );
    exit( 2 );
}
