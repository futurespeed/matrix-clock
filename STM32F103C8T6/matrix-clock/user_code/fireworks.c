#include <stdlib.h>
#include "fireworks.h"
#include "simple_gl.h"
#include "utils.h"

uint16_t step;
uint16_t max_step = 12;

uint8_t fw_cnt = 5;
uint16_t fw_px[5] = {15,36,32,42,50};
uint16_t fw_py[5] = {31,13,42,50,23};
uint16_t fw_offsets[5] = {0,2,6,3,10};
uint16_t fw_colors[5] = {
	(0x1f<<11|0x1f<<5|0x1f),
	(0x1<<11|0x1f<<5|0x1f),
	(0x1f<<11|0x1f<<5|0x1),
	(0x1f<<11|0x1<<5|0x1),
	(0x1<<11|0x1f<<5|0x1)
};

void _draw_fireworks(uint16_t x, uint16_t y, uint16_t color, uint16_t step);

void fw_do_step()
{
    step = (step + 1) % max_step;
		uint16_t offset;
		for(int i=0; i < fw_cnt; i++){
			offset = (step + fw_offsets[i])% max_step;
			if(0==offset){
				fw_px[i]=rand()%40+12;
				fw_py[i]=rand()%40+12;
			}
			_draw_fireworks(fw_px[i],fw_py[i],fw_colors[i],offset);
		}
}

void _draw_fireworks(uint16_t x, uint16_t y, uint16_t color, uint16_t step)
{
    uint16_t line_len = 4;
    uint32_t line_offset = step > line_len ? step - line_len: 0;

    gl_draw_line(x, y - line_offset, x, y - step, color);
    gl_draw_line(x, y + line_offset, x, y + step, color);
    gl_draw_line(x - line_offset, y, x - step, y, color);
    gl_draw_line(x + line_offset, y, x + step, y, color);

    line_offset = line_offset * 1000 * 0.75 / 1000;
    uint32_t line_offset_end = step * 1000 * 0.75 / 1000;

    gl_draw_line(x - line_offset, y - line_offset, x - line_offset_end, y - line_offset_end, color);
    gl_draw_line(x + line_offset, y + line_offset, x + line_offset_end, y + line_offset_end, color);
    gl_draw_line(x - line_offset, y + line_offset, x - line_offset_end, y + line_offset_end, color);
    gl_draw_line(x + line_offset, y - line_offset, x + line_offset_end, y - line_offset_end, color);
}