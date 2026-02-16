#include "ds3231.h"

static uint8_t BinToBCD(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

static uint8_t BCDToBin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

HAL_StatusTypeDef DS3231_SetTime(I2C_HandleTypeDef *hi2c, DS3231_Time *time)
{
    uint8_t buffer[3];
    buffer[0] = BinToBCD(time->seconds);
    buffer[1] = BinToBCD(time->minutes);
    buffer[2] = BinToBCD(time->hours);

    return HAL_I2C_Mem_Write(
        hi2c,
        DS3231_ADDRESS,
        0x00,
        I2C_MEMADD_SIZE_8BIT,
        buffer,
        3,
        100
    );
}

HAL_StatusTypeDef DS3231_GetTime(I2C_HandleTypeDef *hi2c, DS3231_Time *time)
{
    uint8_t buffer[3];

    if (HAL_I2C_Mem_Read(
            hi2c,
            DS3231_ADDRESS,
            0x00,
            I2C_MEMADD_SIZE_8BIT,
            buffer,
            3,
            100
        ) != HAL_OK)
    {
        return HAL_ERROR;
    }

    time->seconds = BCDToBin(buffer[0]);
    time->minutes = BCDToBin(buffer[1]);
    time->hours   = BCDToBin(buffer[2] & 0x3F); // 24h mode

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_SetDate(I2C_HandleTypeDef *hi2c, DS3231_Date *date)
{
    uint8_t buffer[3];
    buffer[0] = BinToBCD(date->day);
    buffer[1] = BinToBCD(date->month);
    buffer[2] = BinToBCD(date->year);

    return HAL_I2C_Mem_Write(
        hi2c,
        DS3231_ADDRESS,
        0x04,
        I2C_MEMADD_SIZE_8BIT,
        buffer,
        3,
        100
    );
}

HAL_StatusTypeDef DS3231_GetDate(I2C_HandleTypeDef *hi2c, DS3231_Date *date)
{
    uint8_t buffer[3];

    if (HAL_I2C_Mem_Read(
            hi2c,
            DS3231_ADDRESS,
            0x04,
            I2C_MEMADD_SIZE_8BIT,
            buffer,
            3,
            100
        ) != HAL_OK)
    {
        return HAL_ERROR;
    }

    date->day   = BCDToBin(buffer[0]);
    date->month = BCDToBin(buffer[1]);
    date->year  = BCDToBin(buffer[2]);

    return HAL_OK;
}
