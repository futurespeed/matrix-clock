#include <stdint.h>
#include "main.h"

typedef struct dck_datetime_def
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
		uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t lunar_month;
    uint8_t lunar_day;
} dck_datetime_def;

dck_datetime_def* dck_get_datetime();
void dck_set_time(uint32_t year, uint32_t month, uint32_t day,
                  uint32_t hour, uint32_t minute, uint32_t second);
void dck_set_lunar(uint32_t month, uint32_t day);
void dck_set_temp(uint32_t temp);
void dck_set_weather(weather_t weather);
void dck_draw_clock();

void TimestampToNormalTime(dck_datetime_def *time, uint32_t Timestamp);
uint8_t IsLeapYear(uint16_t year);
void getWEEK(dck_datetime_def *time);