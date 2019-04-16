#include <graph_lib.h>


extern struct fb_fix_screeninfo finfo;
extern struct fb_var_screeninfo vinfo;
#define CHECK_RANGE(x, y, t) \
	do {	\
		if (x + vinfo.xoffset < t || y + vinfo.yoffset < t \
			||x + t + vinfo.xoffset > vinfo.xres_virtual \
			|| y + t + vinfo.yoffset > vinfo.yres_virtual)	\
		{	\
			fprintf(stderr, "Error: invalid range: %u, %u, %u", \
			x, y, t);	\
			exit (EXIT_FAILURE);	\
		}	\
	} while(0);

extern char *framebuffer;
#define FRAMEBUFFER_CHECK framebuffer_check()
inline void framebuffer_check() {
	if (framebuffer == NULL) {
		fprintf(stdout, "Error: palette not init!\n");
		exit (EXIT_FAILURE);
	}
}

extern uint32_t screensize;
extern int 		event_fd;

void init_palette(int argc, char **argv) {
	int fd;
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <framefile> <eventfile>\n", argv[0]);
		exit (EXIT_FAILURE);
	}
	if ((fd = open(argv[1], O_RDWR)) < 0) {
		fprintf(stderr, "Error: can't open %s: %s\n",
			argv[1], strerror(errno));
		exit (EXIT_FAILURE);
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error: can't get fix screen info");
		exit (EXIT_FAILURE);
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error: can't get var screen info");
		exit(EXIT_FAILURE);
	}

	if ((event_fd = open(argv[2], O_RDWR)) < 0) {
		fprintf(stderr, "Error: can't open %s: %s\n",
			argv[2], strerror(errno));
		exit (EXIT_FAILURE);
	}
	screensize = finfo.smem_len;

	framebuffer = 
	(char *)mmap(0, screensize, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd, 0);
	if (framebuffer == MAP_FAILED) {
		perror ("Error: can't mmap to framebuffer");
		exit (EXIT_FAILURE);
	}
	
	fprintf(stdout, "Palette initialized successfully!\n");
	return;
}

void exit_palette() {

	//FRAMEBUFFER_CHECK;
	
	munmap(framebuffer, screensize);
	fprintf(stdout, "Palette deleted successfully!\n");
	return;
}

static uint32_t get_color_rgb32(const POINT *point) {
	uint32_t bytes_per_pixel = 4;
	uint32_t stride 	     = finfo.line_length / bytes_per_pixel;
	return *((uint32_t*)framebuffer + (point->y + vinfo.yoffset) 
		* stride + point->x + vinfo.xoffset);
}

uint32_t get_color(const POINT *point) {
	switch(vinfo.bits_per_pixel) {
	case 32:
		return get_color_rgb32(point);
		break;
	default:
		fprintf(stderr, "get_color not implmented "
		"for color depth %u\n", vinfo.bits_per_pixel);
		exit(EXIT_FAILURE);
		break;
	}
}

void set_background(uint32_t color) {
	uint32_t i;
//	FRAMEBUFFER_CHECK;
	for (i = 0; i < finfo.smem_len; i++)
		framebuffer[i] = color;
	return;
}

static void normal_point_rgb32(const POINT *point, uint32_t color) {
	uint32_t bytes_per_pixel = 4;
	uint32_t stride 		 = finfo.line_length / bytes_per_pixel;

	uint32_t *dest;
	uint32_t i, j;
	uint32_t point_len = 2 * point->thickness + 1;
	CHECK_RANGE(point->x, point->y, point->thickness)

	dest = (uint32_t*)framebuffer + (vinfo.yoffset + point->y 
		- point->thickness) * stride + point->x + vinfo.xoffset - point->thickness;

	for (i = 0; i < point_len; i++)
		for (j = 0; j < point_len; j++)
			dest[i + j * stride] = color;

	return;
}

void normal_point(const POINT *point, uint32_t color) {
	switch(vinfo.bits_per_pixel) {
	case 32:
		normal_point_rgb32(point, color);
		break;
	default:
		fprintf(stderr, "normal_point not implemented \
		for color depth %u\n", vinfo.bits_per_pixel);
		break;
	}
}

void normal_line_rgb32(const RECT *rect, uint32_t color) {
	const uint32_t bytes_per_pixel = 4;
	const uint32_t stride		   = finfo.line_length / bytes_per_pixel;

	uint32_t *dest;
	uint32_t i, j, length;

	CHECK_RANGE(rect->x0, rect->y0, rect->thickness);
	CHECK_RANGE(rect->x1, rect->y1, rect->thickness);
	if (rect->x0 == rect->x1) {
		dest = (uint32_t *)framebuffer + vinfo.xoffset + rect->x0 + 
			stride * (MIN(rect->y0, rect->y1) + vinfo.yoffset);
		length = ABS_DIFF(rect->y0, rect->y1);
		for (i = 0; i < length; i ++)
			dest[i * stride] = color;
		for (i = 1; i <= rect->thickness; i++) {
			for (j = 0; j < length; j ++) {
				*(dest + j * stride - i) = color;
				*(dest + j * stride + i) = color;
			}
		}
	} else if (rect->y0 == rect->y1) {
		dest = (uint32_t *)framebuffer + vinfo.xoffset + 
			MIN(rect->x0, rect->x1) + stride * (vinfo.yoffset + rect->y0);
		length = ABS_DIFF(rect->x0, rect->x1);
		for (i = 0; i < length; i++)
			dest[i] = color;
		for (i = 1; i <= rect->thickness; i++) {
			for (j = 0; j < length; j++) {
				*(dest - stride * i + j) = color;
				*(dest + stride * i + j) = color;
			}
		}
	} else return;
}

void normal_line(const RECT *rect, uint32_t color) {
	switch(vinfo.bits_per_pixel) {
	case 32:
		normal_line_rgb32(rect, color);
		break;
	default:
		fprintf(stderr, "Warning: normal_line not implemented \
		for color depth: %u\n", vinfo.bits_per_pixel);
		return;
	}
}

// rgb8888
static void straight_line_rgb32(const RECT *rect, uint32_t color) {
	const uint32_t bytes_per_pixel = 4;
	const uint32_t stride		  = finfo.line_length / bytes_per_pixel;

	double	 x, y;
	double	 height 	 = (double)ABS_DIFF(rect->y0, rect->y1);
	double	 width		 = (double)ABS_DIFF(rect->x0, rect->x1);
	double   length      = (double)MAX(height, width);
	double   delt_x		 = (double)(width)  / length;
	double   delt_y		 = (double)(height) / length;
	
	uint32_t start_y, start_x;
	uint32_t *dest;
	uint32_t  slope;		// 0: left	1: right

	if (rect->y0 < rect->y1) {
		start_x = rect->x0;
		start_y = rect->y0;
		if (rect->x0 > rect->x1)
			slope = 0;
		else slope = 1;
	} else {
		start_x = rect->x1;
		start_y = rect->y1;
		if (rect->x1 > rect->x0)
			slope = 0;
		else slope = 1;
	}


	dest = (uint32_t *)(framebuffer) + (start_y + vinfo.yoffset)
		* stride + (start_x + vinfo.xoffset);

	if (0 == slope) {
		for (x = 0, y = 0; x < width
			&& y < height; x += delt_x, y += delt_y)
			dest[stride * (uint32_t)(y + 0.5) - (uint32_t)(x + 0.5)] = color;
	} else {
		for (x = 0, y = 0; x < width 
			&& y < height; x += delt_x, y += delt_y) 
			dest[stride * (uint32_t)(y + 0.5) + (uint32_t)(x + 0.5)] = color;
	}

	return;
}

void straight_line(const RECT *rect, uint32_t color) {
	switch (vinfo.bits_per_pixel) {
	case 32:
		straight_line_rgb32(rect, color);
		break;
	default:
		fprintf(stderr, "Warning: straight_line not implemented \
		for color depth %u\n", vinfo.bits_per_pixel);
		break;
	}	
}

// rgb8888
static void regular_rect_rgb32(const RECT *rect, uint32_t color) {
	const uint32_t bytes_per_pixel = 4;
	const uint32_t stride 		  = finfo.line_length / bytes_per_pixel;

	uint32_t *dest = (uint32_t *)(framebuffer) + (rect->y0 + vinfo.yoffset) 
		* stride + (rect->x0 + vinfo.xoffset);
	
	uint32_t x, y, t;
	uint32_t width, height;
	uint32_t *tmp_dest;
	for (t = 0; t < rect->thickness; t++) {
		width 	 = rect->y1 - rect->y0 + 1 - 2 * t;
		height   = rect->x1 - rect->x0 + 1 - 2 * t;
		tmp_dest = dest + stride * t + t;
		for (x = 0; x < width; x++) {
			tmp_dest[x] = color;
			tmp_dest[x + stride * (height - 1)] = color;
		}
		tmp_dest = dest + t + stride * t;
		for (y = 0; y < height; y++) {
			tmp_dest[y * stride] = color;
			tmp_dest[y * stride + (width - 1)] = color;
		}
	}

}

// rgb565
static void regular_rect_rgb16(const RECT *rect, uint32_t color) {
	const uint32_t 	bytes_per_pixel = 2;
	const uint32_t 	stride  		 = finfo.line_length / bytes_per_pixel;
	const uint32_t 	red 	  		 = (color & 0xff0000) >> (16 + 3);
	const uint32_t 	green   		 = (color & 0x00ff00) >> (8  + 2);
	const uint32_t 	blue 	   		 = (color & 0x0000ff) >> (3);
	const uint16_t  color16 		 = (blue) | (green << 5) | (red << (5 + 6));

	uint16_t *dest = (uint16_t *)(framebuffer) + (rect->y0 + vinfo.yoffset)
		* stride + (rect->x0 + vinfo.xoffset);
	
	uint32_t   x, y, t;
	uint32_t   width, height;
	uint16_t   *tmp_dest; 
	
	for (t = 0; t < rect->thickness; t++) {
		width 	 = rect->y1 - rect->y0 + 1 - 2 * t;
		height   = rect->x1 - rect->x0 + 1 - 2 * t;
		tmp_dest = dest + stride * t + t;
		for (x = 0; x < width; x++) {
			tmp_dest[x] = color16;	
			tmp_dest[x + stride * (height - 1)] = color16;
		}	
		tmp_dest = dest + t + stride * t;
		for (y = 0; y < height; y++) {
			tmp_dest[y * stride] = color16;
			tmp_dest[y * stride + (width - 1)] = color16;
		}
	}
}

// rgb555
static void regular_rect_rgb15(const RECT *rect, uint32_t color) {
	const uint32_t 	bytes_per_pixel = 2;
	const uint32_t 	stride  		 = finfo.line_length / bytes_per_pixel;
	const uint32_t 	red 	  		 = (color & 0xff0000) >> (16 + 3);
	const uint32_t 	green   		 = (color & 0x00ff00) >> (8  + 3);
	const uint32_t 	blue 	   		 = (color & 0x0000ff) >> (3);
	const uint16_t  color15 		 = (blue) | (green << 5) | (red << (5 + 5));

	uint16_t *dest = (uint16_t *)(framebuffer) + (rect->y0 + vinfo.yoffset)
		* stride + (rect->x0 + vinfo.xoffset);
	
	uint32_t   x, y, t;
	uint32_t   width, height;
	uint16_t *tmp_dest; 
	
	for (t = 0; t < rect->thickness; t++) {
		width 	 = rect->y1 - rect->y0 + 1 - 2 * t;
		height   = rect->x1 - rect->x0 + 1 - 2 * t;
		tmp_dest = dest + stride * t + t;
		for (x = 0; x < width; x++) {
			tmp_dest[x] = color15;		
			tmp_dest[x + stride * (height - 1)] = color15;
		}
		tmp_dest = dest + t + stride * t;
		for (y = 0; y < height; y++) {
			tmp_dest[y * stride] = color15;
			tmp_dest[y * stride + (width - 1)] = color15;
		}
	}
}

void regular_rect(const RECT *rect, uint32_t color) {
	switch (vinfo.bits_per_pixel) {
	case 32:
		regular_rect_rgb32(rect, color);
		break;
	case 16:
		regular_rect_rgb16(rect, color);
		break;
	case 15:
		regular_rect_rgb15(rect, color);
		break;
	default:
		fprintf(stderr, "Warning: regular_rect() not \
			implemented for color depth %u\n", vinfo.bits_per_pixel);
		break;
	}
}

int over_bround(const POINT *point) {
	int64_t tmp_x = (int64_t)point->x - (int64_t)point->thickness;
	int64_t tmp_y = (int64_t)point->y - (int64_t)point->thickness;
	return ( point->x + point->thickness >= vinfo.xres 
		|| point->y + point->thickness >= vinfo.yres
		|| (tmp_x < 0 || tmp_y < 0));	
}
