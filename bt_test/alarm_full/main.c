/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (with BT icon on LCD)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "tim.h"

#include "lcd_i2c.h"
#include "ds3231.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* External handlers */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim3;

/* UART buffer */
#define UART_BUF_LEN 64
static uint8_t uart_rx_byte;
static char uart_buf[UART_BUF_LEN];
static uint8_t uart_buf_idx = 0;

static volatile uint8_t cmd_ready = 0;
static char cmd_queue[UART_BUF_LEN];

/* NEW: BT STATE PIN (HC-05 STATE -> PA8 by default) */
#ifndef BT_STATE_Pin
#define BT_STATE_Pin GPIO_PIN_8
#endif
#ifndef BT_STATE_GPIO_Port
#define BT_STATE_GPIO_Port GPIOA
#endif

/* Custom BT icon (5x8) - will be stored at CGRAM location 0 */
static const uint8_t BT_icon[8] = {
    0b00100,
    0b01010,
    0b11111,
    0b01010,
    0b00100,
    0b00000,
    0b00000,
    0b00000
};

/* Function prototypes */
void SystemClock_Config(void);
static void ProcessCommand(char *cmd);
static void LCD_ShowDateTime(void);
static void send_response(const char *msg);
static void buzzer_on(void);
static void buzzer_off(void);

/* Local LCD helpers (we implement here so no changes in lcd_i2c.c needed) */
static void lcd_create_char(uint8_t location, const uint8_t charmap[8]);
static void lcd_write_char(uint8_t location);

/* -------------------------------------------------------------------------- */
static void send_response(const char *msg)
{
    if (!msg) return;

    HAL_Delay(5);

#ifdef __HAL_UART_CLEAR_OREFLAG
    __HAL_UART_CLEAR_OREFLAG(&huart1);
#endif
#ifdef __HAL_UART_CLEAR_FEFLAG
    __HAL_UART_CLEAR_FEFLAG(&huart1);
#endif

    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 200);
}

static void buzzer_on(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 50);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

static void buzzer_off(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

/* -------------------------------------------------------------------------- */
static void lcd_create_char(uint8_t location, const uint8_t charmap[8])
{
    location &= 0x7;  // only 8 locations (0-7)
    lcd_send_cmd(0x40 | (location << 3)); // set CGRAM address
    for (int i = 0; i < 8; ++i)
    {
        lcd_send_data(charmap[i]);
    }
}

/* lcd_write_char: write a single raw char code to DDRAM (i.e. custom char) */
static void lcd_write_char(uint8_t location)
{
    lcd_send_data((char)location);
}

/* -------------------------------------------------------------------------- */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();
    MX_TIM3_Init();

    HAL_Delay(400);
    lcd_init(&hi2c1);
    HAL_Delay(30);

    /* create BT icon in CGRAM slot 0 */
    lcd_create_char(0, BT_icon);

    lcd_send_cmd(0x0C);
    HAL_Delay(5);
    lcd_clear();

    memset(uart_buf, 0, sizeof(uart_buf));
    uart_buf_idx = 0;
    cmd_ready = 0;
    memset(cmd_queue, 0, sizeof(cmd_queue));

    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

    lcd_put_cur(0, 0);
    lcd_send_string("RTC + BT ready");
    HAL_Delay(600);
    lcd_clear();

    uint32_t last_tick = HAL_GetTick();

    while (1)
    {

        if (cmd_ready)
        {
            char local_cmd[UART_BUF_LEN];

            __disable_irq();
            memcpy(local_cmd, cmd_queue, UART_BUF_LEN);
            cmd_ready = 0;
            __enable_irq();

            ProcessCommand(local_cmd);
        }

        /* Update LCD exactly every 1000 ms */
        uint32_t now = HAL_GetTick();
        if (now - last_tick >= 1000)
        {
            last_tick += 1000;
            LCD_ShowDateTime();
        }
    }
}

/* -------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        char c = (char)uart_rx_byte;

        if (c == '\n' || c == '\r')
        {
            if (uart_buf_idx > 0)
            {
                uart_buf[uart_buf_idx] = '\0';

                __disable_irq();
                strncpy(cmd_queue, uart_buf, UART_BUF_LEN - 1);
                cmd_queue[UART_BUF_LEN - 1] = '\0';
                cmd_ready = 1;
                __enable_irq();

                uart_buf_idx = 0;
                uart_buf[0] = '\0';
            }
        }
        else
        {
            if (uart_buf_idx < UART_BUF_LEN - 1)
                uart_buf[uart_buf_idx++] = c;
            else {
                uart_buf_idx = 0;
                uart_buf[0] = '\0';
            }
        }

        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

/* -------------------------------------------------------------------------- */
static void ProcessCommand(char *cmd)
{
    send_response("DBG: [");
    send_response(cmd);
    send_response("]\r\n");

    while (*cmd == ' ') cmd++;

    char tmp[UART_BUF_LEN];
    strncpy(tmp, cmd, UART_BUF_LEN-1);
    tmp[UART_BUF_LEN-1] = 0;

    for (char *p = tmp; *p; ++p)
        *p = toupper((unsigned char)*p);

    /* ===== ALARM CONTROL ===== */
    if (strcmp(tmp, "ALARM:ON") == 0)
    {
        buzzer_on();
        send_response("OK: alarm on\r\n");
        return;
    }

    if (strcmp(tmp, "MIS:OK") == 0)
    {
        buzzer_off();
        send_response("OK: alarm off\r\n");
        return;
    }

    if (strncmp(tmp, "SYNC ", 5) == 0)
    {
        int Y, M, D, h, m, s;

        if (sscanf(cmd + 5, "%d-%d-%d %d:%d:%d",
                   &Y, &M, &D, &h, &m, &s) == 6)
        {
            /* --- FIX: kompensacja sekundy --- */
            uint8_t adj_s = (uint8_t)((s + 59) % 60);

            DS3231_Date dd = { .year = (Y - 2000), .month = M, .day = D };
            DS3231_Time tt = { .hours = h, .minutes = m, .seconds = adj_s };

            if (DS3231_SetDate(&hi2c1, &dd) == HAL_OK &&
                DS3231_SetTime(&hi2c1, &tt) == HAL_OK)
            {
                send_response("OK: synced\r\n");
            }
            else send_response("ERR: sync fail\r\n");
        }
        else send_response("ERR: SYNC format\r\n");

        return;
    }


    if (strncmp(tmp, "SET ", 4) == 0)
    {
        int h, m, s;
        if (sscanf(cmd + 4, "%d:%d:%d", &h, &m, &s) == 3)
        {
            /* --- FIX: kompensacja sekundy --- */
            uint8_t adj_s = (uint8_t)((s + 59) % 60);

            DS3231_Time t = {h, m, adj_s};

            if (DS3231_SetTime(&hi2c1, &t) == HAL_OK)
                send_response("OK: time set\r\n");
            else
                send_response("ERR: I2C\r\n");
        }
        else send_response("ERR: SET format\r\n");

        return;
    }

    if (strncmp(tmp, "DATE ", 5) == 0)
    {
        int Y, M, D;
        if (sscanf(cmd + 5, "%d-%d-%d", &Y, &M, &D) == 3)
        {
            DS3231_Date date = { .year = (Y % 100), .month = M, .day = D };

            if (DS3231_SetDate(&hi2c1, &date) == HAL_OK)
                send_response("OK: date set\r\n");
            else
                send_response("ERR: I2C date\r\n");
        }
        else send_response("ERR: DATE format\r\n");

        return;
    }

    /* ===== UNKNOWN ===== */
    send_response("ERR: unknown cmd\r\n");
}

/* -------------------------------------------------------------------------- */
/* Show Date/Time on LCD and append BT icon if connected */
static void LCD_ShowDateTime(void)
{
    DS3231_Time t;
    DS3231_Date d;

    if (DS3231_GetTime(&hi2c1, &t) == HAL_OK &&
        DS3231_GetDate(&hi2c1, &d) == HAL_OK)
    {
        char line1[17];
        char line2[17];

        /* Build date string */
        snprintf(line1, sizeof(line1), "%04d-%02d-%02d", 2000 + d.year, d.month, d.day);

        /* Display date (clear first) */
        lcd_put_cur(0, 0);
        lcd_send_string("                ");
        lcd_put_cur(0, 0);
        lcd_send_string(line1);

        /* Show BT icon (custom char 0) if connected */
        if (HAL_GPIO_ReadPin(BT_STATE_GPIO_Port, BT_STATE_Pin))
        {
            /* append icon right after date */
            lcd_write_char(0);
        }

        /* Display time */
        snprintf(line2, sizeof(line2), "%02d:%02d:%02d", t.hours, t.minutes, t.seconds);
        lcd_put_cur(1, 0);
        lcd_send_string("                ");
        lcd_put_cur(1, 0);
        lcd_send_string(line2);
    }
    else
    {
        lcd_put_cur(0,0);
        lcd_send_string(" RTC ERROR     ");
        lcd_put_cur(1,0);
        lcd_send_string(" CHECK I2C     ");
    }
}

/* -------------------------------------------------------------------------- */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

/* -------------------------------------------------------------------------- */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
