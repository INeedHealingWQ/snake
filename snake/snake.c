#include <graph_lib.h>

#define GLOBAL_THICK 		2
#define BACK_GROUND_COLOR	0x00000000
#define SNAKE_BODY_COLOR	0xFFFFFFFF
#define NUM_INFL			50
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
extern int event_fd;
useconds_t sec = 100000;
int max_x;
int max_y;

enum {
	stop = 0,
	low_speed,
	mid_speed,
	high_speed,
} speed;

enum dirt {
	none = -1,
	up,
	down,
	left,
	right,
} dirt;

struct infl {
	POINT infl_posi;
	enum dirt dirt;
};

struct snake {
	POINT head;
	POINT tail;
	struct infl *infl;
	uint32_t n_infl;
	uint32_t idx_infl;
	uint32_t speed;
	enum dirt h_dirt;
	enum dirt t_dirt;
	uint32_t length;
	int  state_flag;
	pthread_mutex_t s_lock;
} *psnake;

struct food {
	POINT posi;
} food;

void dump_snake_info(struct snake *sn) {
	fprintf(stdout, "snake info:\n"
	"\thead position: x = %u, y = %u\n"
	"\ttail position: x = %u, y = %u\n",
	sn->head.x, sn->head.y, sn->tail.x, sn->tail.y);
}

void init_snake() {
	struct infl *infl;
	RECT  body;
	psnake = (struct snake *)malloc(sizeof(struct snake));
	if (psnake == NULL) {
		fprintf(stderr, "Error: failed to malloc snake!\n");
		fflush(stderr);
		exit (EXIT_FAILURE);
	}
	infl = (struct infl *) malloc (sizeof(struct infl) * NUM_INFL);
	if (infl == NULL) {
		fprintf(stderr, "Error: failed to malloc POINT!\n");
		fflush(stderr);
		exit (EXIT_FAILURE);
	}
	psnake->head.x 			= (vinfo.xres / (2 * GLOBAL_THICK + 1) / 2) * (2 * GLOBAL_THICK + 1) + GLOBAL_THICK;
	psnake->tail.x 			= psnake->head.x;
	psnake->head.y 			= (vinfo.yres / (2 * GLOBAL_THICK + 1) / 2) * (2 * GLOBAL_THICK + 1) + GLOBAL_THICK;
	psnake->tail.y 			= psnake->head.y + 3 * (2 * GLOBAL_THICK + 1);
	psnake->head.thickness 	= GLOBAL_THICK;
	psnake->tail.thickness 	= GLOBAL_THICK;
 	psnake->infl   			= infl;
	psnake->n_infl  		=  0;
	psnake->idx_infl 		=  0;
	psnake->speed  			= low_speed;
	psnake->h_dirt   		= up;
	psnake->t_dirt			= up;
	psnake->length 			= 30;
	psnake->state_flag		=  0;

	if (pthread_mutex_init(&psnake->s_lock, NULL) != 0)
		goto fail_init_mutex;

	body.x0 	    = psnake->head.x,
	body.y0 	    = psnake->head.y,
	body.x1 	    = psnake->tail.x,
	body.y1 	    = psnake->tail.y,
	body.thickness  = GLOBAL_THICK,

	normal_line(&body, SNAKE_BODY_COLOR);
	fprintf(stdout, "Info: snake successfully created!\n");
	return;
	
fail_init_mutex:
	fprintf(stderr, "failed to init mutex\n");
	free(psnake->infl);
	free(psnake);
	exit(EXIT_FAILURE);
}

void kill_snake(struct snake *sn) {
	free(sn->infl);
	free(sn);
	return;
}

void game_over() {
	fprintf(stdout, "Info: Game Over!\n");
	set_background(BACK_GROUND_COLOR);
	return;
}

int catch_food(const POINT *tmp, const struct food *fd) {
	return (tmp->x == fd->posi.x
	&& tmp->y == fd->posi.y);
}

int crash(const POINT *tmp) {
	fprintf(stdout, "tmp.x = %u, tmp.y = %u, color = 0x%X "
	"BACKG_GROUND_COLOR = 0x%X\n", tmp->x, tmp->y, get_color(tmp), BACK_GROUND_COLOR);
	return ( get_color(tmp) != BACK_GROUND_COLOR );
}


void correct_tail_dirt(struct snake *sn) {
	int i;
	if (sn->n_infl != 0 && 
		(sn->infl[sn->idx_infl].infl_posi.x == sn->tail.x
		&& sn->infl[sn->idx_infl].infl_posi.y == sn->tail.y))
	{
		sn->t_dirt = sn->infl[sn->idx_infl].dirt;
		sn->n_infl --;
		sn->idx_infl ++;
		if (sn->n_infl + 10 <= NUM_INFL) {
			for (i = 0; i < sn->n_infl; i++, sn->idx_infl++)
				sn->infl[i] = sn->infl[sn->idx_infl];
			sn->idx_infl = 0;
		}
	}
}

void rand_food() {
	int f_x, f_y;
	do {
		f_x = rand() % max_x;
		f_y = rand() % max_y;
		food.posi.x = f_x * (2 * GLOBAL_THICK + 1) + GLOBAL_THICK;
		food.posi.y = f_y * (2 * GLOBAL_THICK + 1) + GLOBAL_THICK;
	} while (get_color(&food.posi) != BACK_GROUND_COLOR);
	food.posi.thickness = GLOBAL_THICK;
	normal_point(&food.posi, SNAKE_BODY_COLOR);
}

void step_proc(struct snake *sn) {
	POINT tmp;
	uint32_t fact = 2 * GLOBAL_THICK + 1;
	int grow = 0;
	while (1) {
		usleep(sec);
		pthread_mutex_lock(&sn->s_lock);
		switch(sn->h_dirt) {
		case up:
			tmp.x = sn->head.x;
			tmp.y = sn->head.y - fact;
			tmp.thickness = GLOBAL_THICK;
			dump_snake_info(sn);
			if (catch_food(&tmp, &food))
			{
				sn->head.x = food.posi.x;
				sn->head.y = food.posi.y;
				sn->length++;
				rand_food();
				grow = 1;
				break;
			} else if (sn->head.y < GLOBAL_THICK + fact
				|| crash(&tmp)) {
				game_over();
				return;
			}
			normal_point(&tmp, SNAKE_BODY_COLOR);
			sn->head.x = tmp.x;
			sn->head.y = tmp.y;
			break;
		case down:
			tmp.x = sn->head.x;
			tmp.y = sn->head.y + fact;
			tmp.thickness = GLOBAL_THICK;
			dump_snake_info(sn);
			if (catch_food(&tmp, &food))
			{
				sn->head.x = food.posi.x;
				sn->head.y = food.posi.y;
				sn->length ++;
				rand_food();
				grow = 1;
				break;
			} else if (sn->head.y > vinfo.yres - 1 - 
				GLOBAL_THICK - fact || crash(&tmp)) {
				game_over();
				return;
			}
			normal_point(&tmp, SNAKE_BODY_COLOR);
			sn->head.x = tmp.x;
			sn->head.y = tmp.y;
			break;
		case left:
			tmp.x = sn->head.x - fact;
			tmp.y = sn->head.y;
			tmp.thickness = GLOBAL_THICK;
			dump_snake_info(sn);
			if (catch_food(&tmp, &food))
			{
				sn->head.x = food.posi.x;
				sn->head.y = food.posi.y;
				sn->length ++;
				rand_food();
				grow = 1;
				break;
			} else if (sn->head.x < GLOBAL_THICK + fact
				|| crash(&tmp)) {
				game_over();
				return;
			}
			normal_point(&tmp, SNAKE_BODY_COLOR);
			sn->head.x = tmp.x;
			sn->head.y = tmp.y;
			break;
		case right:
			tmp.x = sn->head.x + fact;
			tmp.y = sn->head.y;
			tmp.thickness = GLOBAL_THICK;
			dump_snake_info(sn);
			if (catch_food(&tmp, &food))
			{
				sn->head.x = food.posi.x;
				sn->head.y = food.posi.y;
				sn->length++;
				rand_food();
				grow = 1;
				break;
			} else if (sn->head.x > vinfo.xres - 1 - 
				GLOBAL_THICK - fact || crash(&tmp)) {
				game_over();
				return;
			}
			normal_point(&tmp, SNAKE_BODY_COLOR);
			sn->head.x = tmp.x;
			sn->head.y = tmp.y;
			break;
		default:
			break;
		}
		if (grow) {
			grow = 0;
			goto next;
		}
		switch(sn->t_dirt) {
		case up:
			fprintf(stdout, "tail up.\n");
			normal_point(&(sn->tail), BACK_GROUND_COLOR);
			sn->tail.y -= fact;
			break;
		case down:
			normal_point(&(sn->tail), BACK_GROUND_COLOR);
			sn->tail.y += fact; 
			break;
		case left:
			normal_point(&(sn->tail), BACK_GROUND_COLOR);
			sn->tail.x -= fact;
			break;
		case right:
			normal_point(&(sn->tail), BACK_GROUND_COLOR);
			sn->tail.x += fact;
			break;
		default:
			break;
		}
		sn->state_flag = 1;
		correct_tail_dirt(sn);
next:
		pthread_mutex_unlock(&sn->s_lock);
	}
	return;
}

static void add_infl(struct snake *sn, struct infl *ifl)
{
	if (sn->n_infl + 10 >= NUM_INFL) {
		sn->infl = (struct infl*)realloc(sn->infl, 
			sizeof(struct infl) * (NUM_INFL + 10));
	}
	memcpy(&sn->infl[sn->n_infl + sn->idx_infl], ifl, sizeof(struct infl));
	sn->n_infl++;
}

void *get_key(void *arg) {
	int rc;
	struct input_event event;
	struct snake *sn = (struct snake *)arg;
	struct infl ifl;
	while ((rc = read(event_fd, &event, 
		sizeof(struct input_event))) > 0) {
		switch (event.type)	{
		case EV_KEY:
			switch (event.code) {
			case KEY_UP:
				if (event.value) {
					fprintf(stdout, "detected key up\n");
					if (sn->h_dirt != up && sn->h_dirt != down
						&& sn->state_flag) {
						pthread_mutex_lock(&sn->s_lock);
						sn->h_dirt = up;
						ifl.infl_posi = sn->head;
						ifl.dirt	  = up;
						add_infl(sn, &ifl);
						pthread_mutex_unlock(&sn->s_lock);
						sn->state_flag = 0;
					}
				}
				break;
			case KEY_DOWN:
				if (event.value) {
					
					fprintf(stdout, "detected key down\n");
					if (sn->h_dirt != up && sn->h_dirt != down
						&& sn->state_flag) {
						pthread_mutex_lock(&sn->s_lock);
						sn->h_dirt = down;
						ifl.infl_posi = sn->head;
						ifl.dirt	  = down;
						add_infl(sn, &ifl);
						pthread_mutex_unlock(&sn->s_lock);
						sn->state_flag = 0;
					}
				}
				break;
			case KEY_LEFT:
				if (event.value) {
					fprintf(stdout, "detected key left\n");	
					if (sn->h_dirt != left && sn->h_dirt != right
						&& sn->state_flag)
					{
						pthread_mutex_lock(&sn->s_lock);
						sn->h_dirt = left;
						ifl.infl_posi = sn->head;
						ifl.dirt	  = left;
						add_infl(sn, &ifl);
						pthread_mutex_unlock(&sn->s_lock);
						sn->state_flag = 0;
					}
				}
				break;
			case KEY_RIGHT:
				if (event.value) {

					fprintf(stdout, "detected key right\n");	
					if (sn->h_dirt != left && sn->h_dirt != right
						&& sn->state_flag)
					{
						pthread_mutex_lock(&sn->s_lock);
						sn->h_dirt = right;
						ifl.infl_posi = sn->head;
						ifl.dirt	  = right;
						add_infl(sn, &ifl);
						pthread_mutex_unlock(&sn->s_lock);
						sn->state_flag = 0;
					}
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	return NULL;
}

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while(0)

int main(int argc, char *argv[]) {
	int s;
	pthread_t th;
	init_palette(argc, argv);
	set_background(BACK_GROUND_COLOR);
	init_snake();
	max_x = vinfo.xres / (2 * GLOBAL_THICK + 1);
	max_y = vinfo.yres / (2 * GLOBAL_THICK + 1);
	srand(time(NULL));
	s = pthread_create(&th, NULL, get_key, (void *)psnake);	
	if (s != 0)
		handle_error_en(s, "pthread create error");
	rand_food();
	normal_point(&food.posi, SNAKE_BODY_COLOR);
	step_proc(psnake);
	kill_snake(psnake);
	exit_palette();
	fflush(stdout);
	return 0;
}
