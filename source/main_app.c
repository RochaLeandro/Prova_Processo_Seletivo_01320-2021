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
#include <math.h>
#include <string.h>

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
#define mainADC_READ_CYCLE_TIME_MS                1UL
#define mainADC_READ_CYCLE_TIME_TICKS             pdMS_TO_TICKS( mainADC_READ_CYCLE_TIME_MS )
#define mainSIGNAL_PROCESSING_CYCLE_TIME_TICKS    pdMS_TO_TICKS( 100UL )
#define mainINTERFACE_CYCLE_TIME_TICKS            pdMS_TO_TICKS( 1UL )
#define mainSHOW_RUNTIME_STATUS_CYCLE_TIME_TIKS   pdMS_TO_TICKS( 3000UL )

/* Constants */
#define ADC_READ_BUFFER_SIZE                    1000U
#define ALLOW_ADC_BUFFER_OVERWRITE              0U
#define SIGNAL_PROCESSING_BUFFER_SIZE           1000U
#define ALLOW_SIGNAL_BUFFER_OVERWRITE           1U
#define PI_VALUE                                3.141592
#define SINE_WAVE_FREQ_HZ                       60U

/*-----------------------------------------------------------*/

/*
 * The tasks.
 */
static void prvACDReadTask( void * pvParameters );
static void prvSignalProcessingTask( void * pvParameters );
static void prvSerialInterfaceTask( void * pvParameters );
static void prvShowRunTimeStatus( void *pvParameters );

/* 
 * ADC. 
 */
static void enqueue_adc_sample(double p_sample);
static uint32_t dequeue_adc_sample(double *const p_sample_p);
static void clear_adc_queue(void);

/* 
 * Signal processing. 
 */
static void enqueue_signal_sample(double p_sample);
static uint32_t dequeue_signal_sample(double *const p_sample_p);
static void clear_signal_queue(void);
static void get_signal(void);

/*-----------------------------------------------------------*/

/* 
 * Global variables.
 */

/* 
 * ADC. 
 */
SemaphoreHandle_t xADCMutex;
double g_adc_read_buffer[ADC_READ_BUFFER_SIZE] = {0.0};
uint32_t g_adc_buffer_count = 0;
uint32_t g_adc_buffer_tail = 0;
uint32_t g_adc_buffer_head = 0;
uint32_t g_adc_first_overflow = 0;

/* 
 * Signal processing. 
 */
SemaphoreHandle_t xSignalMutex;
double g_signal_processing_buffer[SIGNAL_PROCESSING_BUFFER_SIZE] = {0.0};
uint32_t g_signal_buffer_count = 0;
uint32_t g_signal_buffer_tail = 0;
uint32_t g_signal_buffer_head = 0;
uint32_t g_signal_first_overflow = 0;

/*-----------------------------------------------------------*/

/**
 * @brief Main application
 * 
 */
void main_app( void )
{
    xADCMutex = xSemaphoreCreateMutex();
    if( xADCMutex == NULL )
    {
        while(1);
    }

    xSignalMutex = xSemaphoreCreateMutex();
    if( xSignalMutex == NULL )
    {
        while(1);
    }

    /* Start the tasks. */
    xTaskCreate( prvACDReadTask,                     /* The function that implements the task. */
                    "ACDRead",                       /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to the task. */
                    NULL,                            /* The parameter passed to the task - not used in this simple case. */
                    mainADC_READ_CYCLE_TIME_TICKS,      /* The priority assigned to the task. */
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
 * @brief Task that reads ADC
 * 
 * @param pvParameters 
 */
static void prvACDReadTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainADC_READ_CYCLE_TIME_TICKS;
    uint32_t cycle_counter = 0;
    double time = 0;
    double sample = 0;

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

        /* ADC reading */
        time = (double)cycle_counter * ((double)mainADC_READ_CYCLE_TIME_MS / 1000);
        sample = sin(2*PI_VALUE*SINE_WAVE_FREQ_HZ*time);

        enqueue_adc_sample(sample);

        if (cycle_counter < INT32_MAX)
        {
            cycle_counter++;
        }
        else
        {
            cycle_counter = 0;
        }

        /* ONLY TO GEN RUNTIME STATUS */
        int k;
        for(k=0;k<1000000;k++)
        {
            __asm volatile ( "NOP" );
        }
    }
}

/**
 * @brief Task that process data
 * 
 * @param pvParameters 
 */
static void prvSignalProcessingTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainSIGNAL_PROCESSING_CYCLE_TIME_TICKS;
    double sample = 0;
    uint32_t has_sample = 0;

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

        /* Process all avlailable samples */
        do
        {
            has_sample = dequeue_adc_sample(&sample);
            if (has_sample)
            {
                sample *= PI_VALUE;
                enqueue_signal_sample(sample);
            }
        } while (has_sample);

        /* ONLY TO GEN RUNTIME STATUS */
        int k;
        for(k=0;k<1000000;k++)
        {
            __asm volatile ( "NOP" );
        }
    }
}

/**
 * @brief Task that monitors serial input
 * 
 * @param pvParameters 
 */
static void prvSerialInterfaceTask( void * pvParameters )
{
    /* Prevent the compiler warning about the unused parameter. */
    ( void ) pvParameters;
	char user_input_char;
	char user_input_string[1000] = {0};
	uint32_t user_input_counter = 0;
	uint32_t i = 0;

	const char getCommandStr[] = "obter\n";
	const char clearCommandStr[] = "zerar\n";

    while( 1 )
    {
        /* Get user input */
        user_input_char = getchar();
		if (user_input_char != -1)
		{
            /* enqueue user input */
			user_input_string[user_input_counter] = user_input_char;
			user_input_counter++;

            /* check if input is ENTER key */
            if (user_input_char == '\n')
            {
                /* check command (must have 6 chars (counting \n) */
                if (user_input_counter == 6U)
                {
                    if (!strcmp(getCommandStr, &user_input_string[user_input_counter-6U]))
                    {                  
                        /* get command */      
                        console_print("Obtendo dados...\n");
                        get_signal();
                        console_print("Obtenção de dados concluída!\n");
                    }
                    else
                    {
                        if (!strcmp(clearCommandStr, &user_input_string[user_input_counter-6U]))
                        {
                            /* clear command */
                            console_print("Limpando buffers...\n");
                            clear_adc_queue();
                            clear_signal_queue();
                            console_print("Limpeza de buffers concluída!\n");
                        }
                        else
                        {
                            /* unknown command */
                            console_print("Undefined command!\n");
                        }                       
                    }
                }
                else
                {
                    /* unknown command */
                    console_print("Undefined command!\n");
                }

                /* Reset user input string */
                for (i = 0; i < user_input_counter; i++)
                {
                    user_input_string[i] = 0;
                }
                /* Reset user input counter */
                user_input_counter = 0;
            }
        }
        vTaskDelay(mainINTERFACE_CYCLE_TIME_TICKS);
    }
}

/**
 * @brief Task that shows tasks runtime status
 * 
 * @param pvParameters 
 * 
 */
static void prvShowRunTimeStatus( void *pvParameters )
{
	TickType_t xNextWakeTime;
    const TickType_t xBlockTime = mainSHOW_RUNTIME_STATUS_CYCLE_TIME_TIKS;

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
		console_print("\nTASKS RUNTIME STATUS:\n%s\n", pcWriteBuffer);
	}
}

/**
 * @brief Enqueue ADC sample
 * 
 * @param p_sample 
 */
static void enqueue_adc_sample(double p_sample)
{
    xSemaphoreTake(xADCMutex, portMAX_DELAY);

    if (ALLOW_ADC_BUFFER_OVERWRITE || (g_adc_buffer_count < ADC_READ_BUFFER_SIZE))
    {
        /* Can enqueue message */
        if (g_adc_buffer_count < (ADC_READ_BUFFER_SIZE - 1U))
        {
            /* Reset overflow flag */
            g_adc_first_overflow = 0;

            /* Increment count */
            g_adc_buffer_count++;
        }
        else
        {
            /* overwrite */
            /* Increment head index */
            if (g_adc_buffer_head < (ADC_READ_BUFFER_SIZE - 1U))
            {
                g_adc_buffer_head++;
            }
            else
            {
                g_adc_buffer_head = 0;
            }
        }

        /* Increment tail index */
        if (g_adc_buffer_tail < (ADC_READ_BUFFER_SIZE - 1U))
        {
            g_adc_buffer_tail++;
        }
        else
        {
            g_adc_buffer_tail = 0;
        }

        /* Copy data into buffer */
        g_adc_read_buffer[g_adc_buffer_tail] = p_sample;
    }
    else
    {
        /* Cant enqueue message */
        /* Data lost */
        if (!g_adc_first_overflow)
        {
            g_adc_first_overflow = 1;
            console_print("ADC buffer overflow\n");
        }
    }

    xSemaphoreGive(xADCMutex);
}

/**
 * @brief Dequeue adc sample
 * 
 * @param p_sample_p 
 * @return uint32_t 
 */
static uint32_t dequeue_adc_sample(double *const p_sample_p)
{
    uint32_t ret_val = 0;

    xSemaphoreTake(xADCMutex, portMAX_DELAY);

    if (g_adc_buffer_count > 0)
    {
        /* There is a message */
        /* Copy data from buffer */
        *p_sample_p = g_adc_read_buffer[g_adc_buffer_head];

        /* Increment head index */
        if (g_adc_buffer_head < (ADC_READ_BUFFER_SIZE - 1U))
        {
            g_adc_buffer_head++;
        }
        else
        {
            g_adc_buffer_head = 0;
        }

        /* Decrement count */
        g_adc_buffer_count--;

        ret_val = 1;
    }

    xSemaphoreGive(xADCMutex);

    return ret_val;
}

/**
 * @brief Clear the ADC queue
 * 
 */
static void clear_adc_queue(void)
{
    xSemaphoreTake(xADCMutex, portMAX_DELAY);
    g_adc_buffer_count = 0;
    g_adc_buffer_tail = 0;
    g_adc_buffer_head = 0;
    g_adc_first_overflow = 0;
    xSemaphoreGive(xADCMutex);
}

/**
 * @brief Enqueue signal sample
 * 
 * @param p_sample 
 */
static void enqueue_signal_sample(double p_sample)
{
    xSemaphoreTake(xSignalMutex, portMAX_DELAY);

    if (ALLOW_SIGNAL_BUFFER_OVERWRITE || (g_adc_buffer_count < ADC_READ_BUFFER_SIZE))
    {
        /* Can enqueue message */
        if (g_signal_buffer_count < (SIGNAL_PROCESSING_BUFFER_SIZE - 1U))
        {
            /* Reset overflow flag */
            g_signal_first_overflow = 0;

            /* Increment count */
            g_signal_buffer_count++;
        }
        else
        {
            /* overwrite */
            /* Increment head index */
            if (g_signal_buffer_head < (SIGNAL_PROCESSING_BUFFER_SIZE - 1U))
            {
                g_signal_buffer_head++;
            }
            else
            {
                g_signal_buffer_head = 0;
            }
        }

        /* Increment tail index */
        if (g_signal_buffer_tail < (SIGNAL_PROCESSING_BUFFER_SIZE - 1U))
        {
            g_signal_buffer_tail++;
        }
        else
        {
            g_signal_buffer_tail = 0;
        }

        /* Copy data into buffer */
        g_signal_processing_buffer[g_signal_buffer_tail] = p_sample;
    }
    else
    {
        /* Cant enqueue message */
        /* Data lost */
        if (!g_signal_first_overflow)
        {
            g_signal_first_overflow = 1;
            console_print("Signal buffer overflow\n");
        }
    }

    xSemaphoreGive(xSignalMutex);
}

/**
 * @brief Dequeue signal sample
 * 
 * @param p_sample_p 
 * @return uint32_t 1 if has new message
 */
static uint32_t dequeue_signal_sample(double *const p_sample_p)
{
    uint32_t ret_val = 0;

    xSemaphoreTake(xSignalMutex, portMAX_DELAY);

    if (g_signal_buffer_count > 0)
    {
        /* There is a message */
        /* Copy data from buffer */
        *p_sample_p = g_signal_processing_buffer[g_signal_buffer_head];

        /* Increment head index */
        if (g_signal_buffer_head < (SIGNAL_PROCESSING_BUFFER_SIZE - 1U))
        {
            g_signal_buffer_head++;
        }
        else
        {
            g_signal_buffer_head = 0;
        }

        /* Decrement count */
        g_signal_buffer_count--;

        ret_val = 1;
    }

    xSemaphoreGive(xSignalMutex);

    return ret_val;
}

/**
 * @brief Clear the signal queue
 * 
 */
static void clear_signal_queue(void)
{
    xSemaphoreTake(xSignalMutex, portMAX_DELAY);
    g_signal_buffer_count = 0;
    g_signal_buffer_tail = 0;
    g_signal_buffer_head = 0;
    g_signal_first_overflow = 0;
    xSemaphoreGive(xSignalMutex);
}

/**
 * @brief Get the signal samples (print)
 * 
 */
static void get_signal(void)
{
    uint32_t has_sample;
    double sample;

    console_print("Samples = [ ");

    do
    {
        has_sample = dequeue_signal_sample(&sample);
        if (has_sample)
        {
            console_print("%lf\t", sample);
        }
        else
        {
            console_print("]\n");
        }
    } while (has_sample);   
}
