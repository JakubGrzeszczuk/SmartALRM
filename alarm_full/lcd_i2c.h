#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "stm32g0xx_hal.h"
#include "main.h"

#define LCD_ADDRESS (0x27 << 1)

void lcd_init(I2C_HandleTypeDef *hi2c);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(char *str);
void lcd_put_cur(int row, int col);
void lcd_clear(void);

#endif /* INC_LCD_I2C_H_ */
