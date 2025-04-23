#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "board_defines.h"

#include "pcf8563_i2c/pcf8563_i2c.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )

void main_task(__unused void *params);

void vLaunch( void);

/*------------- MAIN -------------*/
int main(void){
  stdio_init_all();

    /* Configure the hardware ready to run the demo. */
    const char *rtos_name;
#if ( configNUMBER_OF_CORES > 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( configNUMBER_OF_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREE_RTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainTask", 4096, NULL, MAIN_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

void main_task(__unused void *params) {
    stdio_init_all();

    printf("Hello, PCF8520! Reading raw data from registers...\n");

    i2c_init(I2C_INST, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

    pcf8563_reset(I2C_INST);

    pcf8563_write_current_time(I2C_INST);
    //pcf8563_set_alarm(I2C_INST);
    //pcf8563_check_alarm(I2C_INST);

    uint8_t raw_time[7];
    int real_time[7];
    char days_of_week[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    while (1) {

        pcf8563_read_raw(I2C_INST, raw_time);
        pcf8563_convert_time(real_time, raw_time);

        printf("Time: %02d : %02d : %02d\n", real_time[2], real_time[1], real_time[0]);
        printf("Date: %s %02d / %02d / %02d\n", days_of_week[real_time[4]], real_time[3], real_time[5], real_time[6]);
        //pcf8563_check_alarm(I2C_INST);
        
        vTaskDelay(500);

        // Clear terminal 
        //printf("\033[1;1H\033[2J");
    }
}