#include <stdint.h>

#define GL_WIDTH 64
#define GL_HEIGHT 64

typedef struct gl_font_t
{
	uint8_t width;
	uint8_t height;
	uint8_t *data;
} gl_font_t;

typedef struct gl_img_t
{
	uint8_t width;
	uint8_t height;
	uint16_t *data;
} gl_img_t;

uint16_t* gl_getGRAM();
void gl_draw_begin();
void gl_draw_end();
void gl_fill(uint16_t color);
void gl_draw_point(uint16_t x, uint16_t y, uint16_t color);
void gl_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void gl_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, uint8_t fill);
void gl_draw_font(gl_font_t font, uint16_t index, uint16_t x, uint16_t y, uint16_t front_color, uint16_t bg_color, uint8_t scale);
void gl_draw_image(gl_img_t img, uint16_t x, uint16_t y, uint8_t transparent, uint16_t transparent_color);
void gl_print();