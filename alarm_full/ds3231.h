#ifndef INC_DS3231_H_
#define INC_DS3231_H_

#include "main.h"
#include "stm32g0xx_hal.h"

#define DS3231_ADDRESS (0x68 << 1)

typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} DS3231_Time;

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;
} DS3231_Date;

// FUNCTIONS
HAL_StatusTypeDef DS3231_SetTime(I2C_HandleTypeDef *hi2c, DS3231_Time *time);
HAL_StatusTypeDef DS3231_GetTime(I2C_HandleTypeDef *hi2c, DS3231_Time *time);

HAL_StatusTypeDef DS3231_SetDate(I2C_HandleTypeDef *hi2c, DS3231_Date *date);
HAL_StatusTypeDef DS3231_GetDate(I2C_HandleTypeDef *hi2c, DS3231_Date *date);

#endif /* INC_DS3231_H_ */
