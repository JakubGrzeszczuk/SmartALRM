#include "lcd_i2c.h"
#include <string.h>

static I2C_HandleTypeDef *lcd_hi2c;
static uint8_t lcd_backlight = 0x08; // włączony podświetlenie

// Wewnętrzne funkcje
static void lcd_send_cmd(uint8_t cmd);
static void lcd_send_data(uint8_t data);
static void lcd_write(uint8_t data);
static void lcd_pulse_enable(uint8_t data);

void lcd_init(I2C_HandleTypeDef *hi2c) {
    lcd_hi2c = hi2c;
    HAL_Delay(50);

    // inicjalizacja 4-bitowa
    lcd_send_cmd(0x33);
    lcd_send_cmd(0x32);
    lcd_send_cmd(0x28); // 2 linie, 5x8
    lcd_send_cmd(0x0C); // wyświetlacz ON, kursor OFF
    lcd_send_cmd(0x06); // przesuwanie kursora
    lcd_clear();
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? 0x80 + col : 0xC0 + col;
    lcd_send_cmd(addr);
}

void lcd_print(char *str) {
    for (size_t i = 0; i < strlen(str); i++) {
        lcd_send_data(str[i]);
    }
}

// --- FUNKCJE WEWNĘTRZNE ---
static void lcd_send_cmd(uint8_t cmd) {
    lcd_write(cmd & 0xF0); // wysyłamy wyższy nibble
    lcd_write((cmd << 4) & 0xF0); // wysyłamy niższy nibble
}

static void lcd_send_data(uint8_t data) {
    lcd_write((data & 0xF0) | 0x01); // RS = 1
    lcd_write(((data << 4) & 0xF0) | 0x01); // RS = 1
}

static void lcd_write(uint8_t data) {
    uint8_t data_t[1];
    data_t[0] = data | lcd_backlight;
    HAL_I2C_Master_Transmit(lcd_hi2c, LCD_ADDRESS, data_t, 1, HAL_MAX_DELAY);
    lcd_pulse_enable(data_t[0]);
}

static void lcd_pulse_enable(uint8_t data) {
    uint8_t data_t[1];
    data_t[0] = data | 0x04; // EN = 1
    HAL_I2C_Master_Transmit(lcd_hi2c, LCD_ADDRESS, data_t, 1, HAL_MAX_DELAY);
    HAL_Delay(1);
    data_t[0] = data & ~0x04; // EN = 0
    HAL_I2C_Master_Transmit(lcd_hi2c, LCD_ADDRESS, data_t, 1, HAL_MAX_DELAY);
    HAL_Delay(1);
}
