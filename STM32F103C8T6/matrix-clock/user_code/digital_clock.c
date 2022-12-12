#include "digital_clock.h"
#include "simple_gl.h"

dck_datetime_def dck_datetime;

uint32_t dck_temp;
uint8_t dck_blink;
weather_t dck_weather;

extern gl_font_t font_4x7;
extern gl_font_t font_8x8;

extern gl_img_t gl_img_sun;
extern gl_img_t gl_img_rain;
extern gl_img_t gl_img_cloud;
extern gl_img_t gl_img_snow;

dck_datetime_def* dck_get_datetime(){
    return &dck_datetime;
}

void dck_set_time(uint32_t year, uint32_t month, uint32_t day,
                  uint32_t hour, uint32_t minute, uint32_t second)
{
    dck_datetime.year = year;
    dck_datetime.month = month;
    dck_datetime.day = day;
    dck_datetime.hour = hour;
    dck_datetime.minute = minute;
    dck_datetime.second = second;
}

void dck_set_lunar(uint32_t month, uint32_t day)
{
    dck_datetime.lunar_month = month;
    dck_datetime.lunar_day = day;
}

void dck_set_temp(uint32_t temp)
{
    dck_temp = temp;
}

void dck_set_weather(weather_t weather)
{
	dck_weather = weather;
}

void dck_draw_clock()
{
    //gl_fill(0);
		dck_blink = (dck_blink + 1) & 1;

    uint32_t front_color = (0x1f<<11)|(0x1f<<5)|(0x1);
    uint32_t bg_color = (0x0<<11)|(0x0<<5)|(0x0);
    // xx:xx
    gl_draw_font(font_4x7, dck_datetime.hour / 10, 2, 2, front_color, bg_color, 2);
    gl_draw_font(font_4x7, dck_datetime.hour % 10, 12, 2, front_color, bg_color, 2);
    gl_draw_font(font_4x7, 10, 22, 2, dck_blink ? front_color : bg_color, bg_color, 2);
    gl_draw_font(font_4x7, dck_datetime.minute / 10, 32 - 2, 2, front_color, bg_color, 2);
    gl_draw_font(font_4x7, dck_datetime.minute % 10, 42 - 2, 2, front_color, bg_color, 2);

    front_color = (0x0<<11)|(0x1f<<5)|(0x1);
    gl_draw_font(font_4x7, dck_datetime.second / 10, 52, 9, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.second % 10, 57, 9, front_color, bg_color, 1);

    front_color = (0x0<<11)|(0x0<<5)|(0x1f);
    // 0000-00-00
    gl_draw_font(font_4x7, dck_datetime.year / 1000 % 10, 15-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.year / 100 % 10, 20-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.year / 10 % 10, 25-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.year % 10, 30-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, 11, 35-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.month / 10, 40-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.month % 10, 45-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, 11, 50-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.day / 10, 55-4, 20, front_color, bg_color, 1);
    gl_draw_font(font_4x7, dck_datetime.day % 10, 60-4, 20, front_color, bg_color, 1);

    bg_color = (0x0<<11)|(0x1<<5)|(0x1);
    gl_draw_rectangle(0, 30, 63, 63, bg_color, 1);

    front_color = (0x1f<<11)|(0x0<<5)|(0x0);
    // xx月xx
    if (dck_datetime.lunar_month / 10)
    {
        gl_draw_font(font_8x8, dck_datetime.lunar_month / 10, 12, 36, front_color, bg_color, 1);
    }
    gl_draw_font(font_8x8, dck_datetime.lunar_month % 10, 20, 36, front_color, bg_color, 1);
    gl_draw_font(font_8x8, 10, 28, 36, front_color, bg_color, 1);
    gl_draw_font(font_8x8, dck_datetime.lunar_day / 10, 36, 36, front_color, bg_color, 1);
    gl_draw_font(font_8x8, dck_datetime.lunar_day % 10, 44, 36, front_color, bg_color, 1);

    front_color = (0x1f<<11)|(0x1f<<5)|(0x1f);
    // xx度
    if (dck_temp)
    {
        gl_draw_font(font_4x7, dck_temp / 10, 40, 52, front_color, bg_color, 1);
        gl_draw_font(font_4x7, dck_temp % 10, 45, 52, front_color, bg_color, 1);
    }
    else
    {
        gl_draw_font(font_4x7, 11, 40, 52, front_color, bg_color, 1);
        gl_draw_font(font_4x7, 11, 45, 52, front_color, bg_color, 1);
    }
    gl_draw_font(font_4x7, 12, 50, 52, front_color, bg_color, 1);
    gl_draw_font(font_4x7, 13, 55, 52, front_color, bg_color, 1);
		
		// weather
		if(WEATHER_SUN == dck_weather)
		{
			gl_draw_image(gl_img_sun, 20, 52, 1, 0);
		}else if(WEATHER_RAIN == dck_weather)
		{
			gl_draw_image(gl_img_rain, 20, 52, 1, 0);
		}else if(WEATHER_CLOUD == dck_weather)
		{
			gl_draw_image(gl_img_cloud, 20, 52, 1, 0);
		}else if(WEATHER_SNOW == dck_weather)
		{
			gl_draw_image(gl_img_snow, 20, 52, 1, 0);
		}

}


/*
 * 时间戳转换为普通时间
 */
void TimestampToNormalTime(dck_datetime_def *time, uint32_t Timestamp)
{

uint16_t year = 1970;
uint32_t Counter = 0, CounterTemp; //随着年份迭加，Counter记录从1970 年 1 月 1 日（00:00:00 GMT）到累加到的年份的最后一天的秒数
uint8_t Month[12] = {31, 28, 31, 30, 31, 30,          31, 31, 30, 31, 30, 31};
uint8_t i;

while(Counter <= Timestamp)     //假设今天为2018年某一天，则时间戳应小于等于1970-1-1 0:0:0 到 2018-12-31 23:59:59的总秒数
{ 
CounterTemp = Counter; //CounterTemp记录完全1970-1-1 0:0:0 到 2017-12-31 23:59:59的总秒数后退出循环
Counter += 31536000; //加上今年（平年）的秒数
if(IsLeapYear(year))
{
Counter += 86400; //闰年多加一天
}
year++;
}
time->year = year - 1; //跳出循环即表示到达计数值当前年
Month[1] = (IsLeapYear(time->year)?29:28);
Counter = Timestamp - CounterTemp; //Counter = Timestamp - CounterTemp  记录2018年已走的总秒数
CounterTemp = Counter/86400;          //CounterTemp = Counter/(24*3600)  记录2018年已【过去】天数
Counter -= CounterTemp*86400;        //记录今天已走的总秒数
time->hour = Counter / 3600; //时       得到今天的小时
time->minute = Counter % 3600 / 60; //分
time->second = Counter % 60; //秒
for(i=0; i<12; i++)
{
if(CounterTemp < Month[i])    //不能包含相等的情况，相等会导致最后一天切换到下一个月第一天时
{//（即CounterTemp已走天数刚好为n个月完整天数相加时（31+28+31...）），
time->month = i + 1;  // 月份不加1，日期溢出（如：出现32号）
time->day = CounterTemp + 1; //应不作处理，CounterTemp = Month[i] = 31时，会继续循环，月份加一，
break;//日期变为1，刚好符合实际日期
}
CounterTemp -= Month[i];
}
getWEEK(time);
}

/*
 * 判断闰年平年
 */
uint8_t IsLeapYear(uint16_t year)
{
if(((year)%4==0 && (year)%100!=0) || (year)%400==0)
return 1; //是闰年
return 0;   //是平年
}

/*
 *  函数功能:根据具体日期得到星期
 *  吉姆拉尔森公式  week=(date+2*month+3*(month+1)/5+year+year/4-y/100+y/400)%7
 *  注 : 把一月 二月看成是上一年的13 14月    ,    得到结果 0 -- 6 
 */
void getWEEK(dck_datetime_def *time)
{
uint16_t YY = 0;
uint8_t MM = 0;
if(time->month==1 || time->month==2)
{
MM = time->month + 12;
YY = time->year - 1;
}else{
MM = time->month;
YY = time->year;
}
time->week = ( (time->day+2*MM+3*(MM+1)/5+YY+YY/4-YY/100+YY/400)%7 ) + 1;
}                     //(29 + 16 + 5 + 2018 +2018/4 - 2018/100 + 2018/400)%7
//(29 + 16 + 5 + 18 +18/4 - 18/100 + 18/400)%7


