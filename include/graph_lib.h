#ifndef GRAPH_LIB_H
#define GRAPH_LIB_H

#include "wq.h"
#define ABS_DIFF(x,y) ( ((x) > (y)) ? \
	((x) - (y)) : ((y) - (x)) )

#define MAX(x,y) ( (x) > (y) ? (x) : (y) )
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
void init_palette(int argc, char **argv);
void exit_palette();

typedef struct point {
	uint32_t x, y;
	uint32_t thickness;
}POINT;

typedef struct rect {
	uint32_t x0, y0;
	uint32_t x1, y1;
	uint32_t thickness;
}RECT;

uint32_t get_color(const POINT *point);
void set_background(uint32_t color);
void normal_point(const POINT *point, uint32_t color);
void normal_line(const RECT *rect, uint32_t color);
void straight_line(const RECT *rect, uint32_t color);
void regular_rect(const RECT *rect, uint32_t color);
int over_bround(const POINT *point);

typedef struct bmp_header {
	uint8_t ident[2];
	uint32_t bmp_size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t data_offset;
} __attribute__((packed)) BMP_HEADER;

typedef struct bitmap_info_header { // for indows NT, 3.1x or later
	uint32_t size_of_header;
	int32_t	 bmp_width_in_pixel;
	int32_t  bmp_height_in_pixel;
	uint16_t color_plane_num;
	uint16_t bits_per_pixel;
	uint32_t compre_method;
	uint32_t image_size;
	uint32_t hori_resolution;
	uint32_t vert_resolution;
	uint32_t palette_colors_num;
	uint32_t important_color_used;
}BITMAPINFOHEADER;

#endif
