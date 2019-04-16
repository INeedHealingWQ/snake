#include <wq.h>
#include <char_lib.h>

extern const uint8_t 	 		ascii[ASCII_NUM][16];
extern uint8_t 			 		base;
extern struct fb_fix_screeninfo finfo;
extern struct fb_var_screeninfo vinfo;
extern char 			 		*framebuffer;
extern uint32_t 	 	 		screensize;
int 					 		event_fd;
uint32_t 						bgcolor;

static uint32_t strlen8(const uint8_t *str) {
	const uint8_t *p;
	uint32_t len = 0;

	for (p = str; 0 != *p; p++) {
		len += 1;
	}

	return (len);
}

void show_str0816_32(uint32_t x, uint32_t y, 
	uint32_t color, const uint8_t *str)
{
	uint8_t idx;
	const uint8_t *p;
	uint32_t *dest;
	uint32_t width, height;
	uint32_t i, j;
	uint32_t stride;

	if (NULL == framebuffer)
		err_msg("Error in %s: palette not initialized!\n", __FUNCTION__);
	width = strlen8(str) * 9;
	height = 16;

	if (width + x >= vinfo.xres
		|| height + y >= vinfo.yres)
		err_msg("Error in %s: bad str position, "
		"%u, %u.\n", __FUNCTION__, x, y);

	stride = finfo.line_length / 4;
	dest = (uint32_t *)framebuffer + stride *
		(y + vinfo.yoffset) + (x + vinfo.xoffset);
	
	for (p = str; 0 != *p; p++) {
		idx = *p - base;
		if (idx >= ASCII_NUM)
			err_msg("Error: idx of ascii shound't "
			"larger than %u. (idx = %u)\n", ASCII_NUM, idx);
		
		for (j = 0; j < 16; j++) {
			for (i = 0; i < 8; i++) {
				if (1 & (ascii[idx][j] >> i))
					dest[j * stride + i] = color;
			}
			dest[j * stride + i] = bgcolor;
		}

		dest += (8 + 1);
	}
	
	return;
}
