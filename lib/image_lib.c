#include <graph_lib.h>

#define IMAGE "image.bmp"

extern char *framebuffer;
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;

BMP_HEADER bmp_header;
BITMAPINFOHEADER bit_map_info_header;
uint8_t *bmp_file_data;

static void dump_bmp_header(BMP_HEADER *hd) {
	if (NULL == hd)
		return;

	printf(
	"Identification: %c %c\n"
	"Bmp size: %u (bytes)\n"
	"Data offset: %u (bytes)\n",
	hd->ident[0], hd->ident[1],
	hd->bmp_size, hd->data_offset
	);

	putchar('\n');

	return;
}

static void dump_bmp_info_header(BITMAPINFOHEADER *bmpinfo)
{
	if (NULL == bmpinfo)
		return;
	
	printf(
	"Bmp info header size: %u (bytes)\n"
	"Bmp width: %d (pixels)\n"
	"Bmp height: %d (pixels)\n"
	"Number of color plane: %u\n"
	"Bits per pixel: %u\n"
	"Compression method: %u\n"
	"Image size: %u (bytes)\n"
	"Horizontal resolution: %u\n"	
	"Vertival resolution: %u\n"
	"Number of palette colors: %u\n"
	"Is important color used: %u\n",
	bmpinfo->size_of_header,
	bmpinfo->bmp_width_in_pixel,
	bmpinfo->bmp_height_in_pixel,
	(uint32_t)bmpinfo->color_plane_num,
	(uint32_t)bmpinfo->bits_per_pixel,
	bmpinfo->compre_method,
	bmpinfo->image_size,
	bmpinfo->hori_resolution,
	bmpinfo->vert_resolution,
	bmpinfo->palette_colors_num,
	bmpinfo->important_color_used
	);

	putchar('\n');

	return;
}

#define INT32_ABS(x) ((x) > 0 ? (x) : (-x))
void draw_image32() {
	uint32_t w = MIN(INT32_ABS(bit_map_info_header.bmp_width_in_pixel),
		vinfo.xres);
	uint32_t h = MIN(INT32_ABS(bit_map_info_header.bmp_height_in_pixel),
		vinfo.yres);
	uint32_t x, y;
	uint32_t pixel;
	uint32_t stride = finfo.line_length / 4;
	uint8_t *data = bmp_file_data + bmp_header.data_offset;

	printf("w = %u, h = %u\n", w, h);

	if (bit_map_info_header.bmp_height_in_pixel < 0) {
		for(y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				pixel = *((uint32_t *)data + y * 
					bit_map_info_header.bmp_width_in_pixel + x);
				*((uint32_t *)framebuffer + stride * (y + vinfo.yoffset) +
					x + vinfo.xoffset) = pixel;
			}
		}
	} else {
		for(y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				pixel = *((uint32_t *)data + y * 
					bit_map_info_header.bmp_width_in_pixel + x);
				*((uint32_t *)framebuffer + stride * (h - 1 - y + vinfo.yoffset) +
					x + vinfo.xoffset) = pixel;
			}
		}
	}

	return;
}

int main(int argc, char **argv) {
	int fd_bmp;
	struct stat statbuf;

	init_palette(argc, argv);

	if ((fd_bmp = open(IMAGE, O_RDONLY)) < 0)
		err_sys("open %s error", IMAGE);
	
	if (fstat(fd_bmp, &statbuf) < 0)
		err_sys("fstat %s error", IMAGE);
	bmp_file_data = mmap(bmp_file_data, statbuf.st_size,
		PROT_READ, MAP_SHARED, fd_bmp, 0);

	if (MAP_FAILED == bmp_file_data)
		err_sys("mmap %s error", IMAGE);

	memcpy((void *)&bmp_header, (void *)bmp_file_data,
		sizeof(BMP_HEADER));
	
	memcpy((void *)&bit_map_info_header, 
		(void *)(bmp_file_data + 14), sizeof(BITMAPINFOHEADER));
	
	dump_bmp_header(&bmp_header);
	dump_bmp_info_header(&bit_map_info_header);

	draw_image32();

	close(fd_bmp);
	munmap(bmp_file_data, statbuf.st_size);

	return (0);
}
