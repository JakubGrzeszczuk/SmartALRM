#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "stm32g0xx_hal.h"

// Adres LCD (sprawdź na swoim module, zwykle 0x27 lub 0x3F)
#define LCD_ADDRESS 0x27 << 1  // HAL używa 8-bitowego adresu

// Funkcje
void lcd_init(I2C_HandleTypeDef *hi2c);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(char *str);

#endif /* INC_LCD_I2C_H_ */
