#include "lcd_i2c.h"

extern I2C_HandleTypeDef hi2c1;

#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

static uint8_t lcd_backlight_state = LCD_BACKLIGHT;

static void lcd_send_internal(uint8_t data, uint8_t flags)
{
    HAL_StatusTypeDef res;
    uint8_t up = data & 0xF0;
    uint8_t lo = (data << 4) & 0xF0;

    uint8_t data_arr[4];

    data_arr[0] = up | flags | lcd_backlight_state | 0x04;  // EN = 1
    data_arr[1] = up | flags | lcd_backlight_state;          // EN = 0
    data_arr[2] = lo | flags | lcd_backlight_state | 0x04;   // EN = 1
    data_arr[3] = lo | flags | lcd_backlight_state;          // EN = 0

    res = HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDRESS, data_arr, 4, 100);

    if (res != HAL_OK) {
    }
}

void lcd_send_cmd(char cmd)
{
    lcd_send_internal(cmd, 0x00);
}

void lcd_send_data(char data)
{
    lcd_send_internal(data, 0x01);
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
    case 0:
        col |= 0x80;
        break;
    case 1:
        col |= 0xC0;
        break;
    case 2:
        col |= 0x94;
        break;
    case 3:
        col |= 0xD4;
        break;
    }
    lcd_send_cmd(col);
}

void lcd_send_string(char *str)
{
    while (*str)
    {
        lcd_send_data(*str++);
    }
}

void lcd_init(I2C_HandleTypeDef *hi2c)
{
    HAL_Delay(50);

    lcd_send_cmd(0x30);
    HAL_Delay(5);

    lcd_send_cmd(0x30);
    HAL_Delay(1);

    lcd_send_cmd(0x30);
    HAL_Delay(10);

    lcd_send_cmd(0x20);

    lcd_send_cmd(0x28);
    lcd_send_cmd(0x0C);
    lcd_send_cmd(0x06);
    lcd_clear();
}
