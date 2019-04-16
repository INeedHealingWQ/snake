#include <wq.h>

struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo vinfo;
char   	 				 *framebuffer;
uint32_t		 		 screensize;
int 					 event_fd;
uint32_t 				 bgcolor;
