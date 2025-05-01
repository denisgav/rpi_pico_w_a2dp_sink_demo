#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#if HAVE_LWIP
#include "pico/lwip_freertos.h"
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "build_defs.h"
#include "board_defines.h"

#include "pcf8563_i2c/pcf8563_i2c.h"
#include "ssd1306/ssd1306.h"

#ifndef RUN_FREERTOS_ON_CORE
#define RUN_FREERTOS_ON_CORE 0
#endif

#define MAIN_TASK_PRIORITY           ( tskIDLE_PRIORITY + 2UL )
#define BLINK_TASK_PRIORITY          ( tskIDLE_PRIORITY + 0UL )
#define STATUS_UPDATE_TASK_PRIORITY  ( tskIDLE_PRIORITY + 1UL )

void main_task(__unused void *params);

void vLaunch( void);

//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_i2c();
void setup_ssd1306();
void display_time(pcf8532_time_and_date_t compilation_time, pcf8532_time_and_date_t cur_time);
//---------------------------------------

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

    printf("Hello, PCF8520!\n");

    setup_i2c();
    setup_ssd1306();

    pcf8563_reset(I2C_INST);

    pcf8532_time_and_date_t compilation_time = pcf8563_get_compilation_time();

    pcf8563_set_time_and_date(I2C_INST, compilation_time);
    //pcf8563_set_alarm(I2C_INST);
    //pcf8563_check_alarm(I2C_INST);

    //uint8_t raw_time[7];
    //int real_time[7];
    char days_of_week[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    while (1) {
        pcf8532_time_and_date_t cur_time = pcf8563_read_time_and_date(I2C_INST);
        pcf8532_time_and_date_t compilation_time = pcf8563_get_compilation_time();

        printf("Cur Time: %02d : %02d : %02d\n", cur_time.hour, cur_time.minute, cur_time.second);
        printf("Cur Date: %s, %02d / %02d / %02d\n", days_of_week[cur_time.day_of_week], cur_time.year, cur_time.month, cur_time.day_of_month);

        printf("Compilation Time: %02d : %02d : %02d\n", compilation_time.hour, compilation_time.minute, compilation_time.second);
        printf("Compilation Date: %s, %02d / %02d / %02d\n", days_of_week[compilation_time.day_of_week], compilation_time.year, compilation_time.month, compilation_time.day_of_month);

        display_time(compilation_time, cur_time);
        
        vTaskDelay(500);

    }
}

//-------------------------
// SSD1306 functions
//-------------------------
void setup_i2c(){
    i2c_init(I2C_INST, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}
void setup_ssd1306(){
  disp.external_vcc=false;
  ssd1306_init(&disp, I2C_SSD1306_WIDTH, I2C_SSD1306_HEIGHT, I2C_SSD1306_ADDR, I2C_INST);
  ssd1306_clear(&disp);

  ssd1306_draw_string(&disp, 4, 16, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 8, 32, 1, "Watches");
  ssd1306_show(&disp);
}

void display_time(pcf8532_time_and_date_t compilation_time, pcf8532_time_and_date_t cur_time){
    char fmt_tmp_str[32] = "";
    char cur_print_str[32] = "";

    char days_of_week[7][12] = {"S", "M", "Tue", "W", "Th", "F", "S"};

    ssd1306_clear(&disp);

    ssd1306_draw_string(&disp, 4, 0, 1, "Current time");

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    memset(cur_print_str, 0x0, sizeof(cur_print_str));

    itoa(cur_time.hour, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);
    strcat(cur_print_str, ":");

    itoa(cur_time.minute, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);
    strcat(cur_print_str, ":");

    itoa(cur_time.second, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);

    ssd1306_draw_string(&disp, 0, 8, 1, cur_print_str);

    memset(fmt_tmp_str, 0x0, sizeof(fmt_tmp_str));
    memset(cur_print_str, 0x0, sizeof(cur_print_str));

    strcat(cur_print_str, days_of_week[cur_time.day_of_week]);
    strcat(cur_print_str, ", ");

    itoa(cur_time.year, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);
    strcat(cur_print_str, "/");

    itoa(cur_time.month, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);
    strcat(cur_print_str, "/");

    itoa(cur_time.day_of_month, fmt_tmp_str, 10);
    strcat(cur_print_str, fmt_tmp_str);

    ssd1306_draw_string(&disp, 0, 16, 1, cur_print_str);
    
    ssd1306_show(&disp);
}
//-------------------------