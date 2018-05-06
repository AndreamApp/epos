#include <inttypes.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <sys/mman.h>
#include <syscall.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../graphics.h"

/*
TODO:

include
comment

Feature:

init animation
finish animation
exit wave animation

multiple color
gradient color

layout configuration
anim configuration
*/

#define MAX 25

//// Configuration
//// Feel free to change these configs
int margin = 1;
int line_height = 3;
float vertical_weight = 0.9f;
int col_cnt = 10; // 每列显示3个排序函数
int swap_period = 10;

int init_anim_period = 10;
int finish_anim_period = 10;
int exit_anim_period = 10;
int exit_anim_duration = 200;

float wave_percent = 0.3f;
int delay_unit = 2;
int blocks_size = 10;
//// Configuration

COLORREF init_colors[MAX] = {
	RGB(172, 208, 206), RGB(247, 223, 131), RGB(172, 208, 206), RGB(247, 223, 131), RGB(172, 208, 206), RGB(247, 223, 131), RGB(172, 208, 206), RGB(247, 223, 131), RGB(172, 208, 206), RGB(247, 223, 131), 
};
COLORREF active_colors[MAX] = {
	RGB(77, 147, 159), RGB(247, 185, 131), RGB(77, 147, 159), RGB(247, 185, 131), RGB(77, 147, 159), RGB(247, 185, 131), RGB(77, 147, 159), RGB(247, 185, 131), RGB(77, 147, 159), RGB(247, 185, 131), 
};
COLORREF finish_colors[MAX] = {
	RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), 
};
COLORREF highlight_colors[MAX] = {
	RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), RGB(59, 62, 71), RGB(247, 147, 131), 
};
COLORREF intense_colors[MAX] = {
	RGB(1, 0, 0),       RGB(0, 1, 0),RGB(1, 0, 0),       RGB(0, 1, 0),RGB(1, 0, 0),       RGB(0, 1, 0),RGB(1, 0, 0),       RGB(0, 1, 0),RGB(1, 0, 0),       RGB(0, 1, 0),
};
COLORREF get_init_color(int b){
	return RGB(172, 208, 206);
}

COLORREF get_active_color(int b){
	return RGB(77, 147, 159);
}

COLORREF get_finish_color(int b){
	return RGB(59, 62, 71);
}

COLORREF get_highlight_color(int b){
	return RGB(59, 62, 71);
}

COLORREF get_intense_color(int b){
	return RGB(0, 0, 0);
}

int hightlight_line[MAX];

int width, height;
int block_width;
int block_height;
int marginTop;
int line_cnt;


int draw_left_bound_cache[MAX][1000];
int draw_right_bound_cache[MAX][1000];

int blocks;
int * arrs[MAX];
struct sort_task{
	void (*sort_method)(int block, int * arr, int len);
	int * arr; // 待排序数组
	int block; // 在哪块区域绘制
	int tid;
};

/**
 * 休眠msec毫秒
 */
int msleep(const uint32_t msec)
{
	//struct timespec ts={0,msec};
	//return nanosleep(&ts, NULL);
	return sleep(msec);
}

//////////////////////////////////////////////
// DRAW API
//////////////////////////////////////////////

// for gradient
COLORREF intense(int b, int i, COLORREF color){
	int R = getRValue(color) + i * getRValue(get_intense_color(b));
	int G = getGValue(color) + i * getGValue(get_intense_color(b));
	int B = getBValue(color) + i * getBValue(get_intense_color(b));
	if((R / 255) & 1){
		R = 255 - (R % 255);
	}
	else{
		R %= 255;
	}
	if((G / 255) & 1){
		G = 255 - (G % 255);
	}
	else{
		G %= 255;
	}
	if((B / 255) & 1){
		B = 255 - (B % 255);
	}
	else{
		B %= 255;
	}
	return RGB(R, G, B);
}

COLORREF merge_color(COLORREF c1, COLORREF c2, float k){
	return RGB(
		k * getRValue(c1) + (1-k) * getRValue(c2),
		k * getGValue(c1) + (1-k) * getGValue(c2),
		k * getBValue(c1) + (1-k) * getBValue(c2)
	);
}

// for wave animation
void color_line_with_margin(int b, int i, int mrgn, COLORREF color){
	int x = block_width * (b % col_cnt);
	int y = i * (margin + line_height) + ((b / col_cnt) * (block_height + marginTop) + marginTop);
	int unit = (block_width - 2);
	
	int l1 = draw_left_bound_cache[b][i];
	int r1 = draw_right_bound_cache[b][i];
	int l2 = x + mrgn, r2 = x + min(mrgn + arrs[b][i], unit);
	
	COLORREF bg = RGB(0, 0, 0);
	
	int t;
	if(l1 != 0 || r1 != 0){
		for(t = 0; t < line_height; t++){
			if(l1 < l2){
				line(l1, y+t, l2, y+t, bg);
			}
			if(r2 < r1){
				line(r2, y+t, r1, y+t, bg);
			}
		}
	}
	for(t = 0; t < line_height; t++){
		line(l2, y+t, r2, y+t, color);
	}
	draw_left_bound_cache[b][i] = l2;
	draw_right_bound_cache[b][i] = r2;
}

/**
 * 绘制第b个数组第i个值，即arrs[b][i]，指定颜色color
 */
void color_line(int b, int i, COLORREF color){
	color = intense(b, i, color);
	color_line_with_margin(b, i, 0, color);
}

/**
 * 在排序过程中调用此函数绘制，使用highlight_color
 * 同一时间只会有一个highlight
 */
void draw_highlight_line(int b, int i){
	// disable origin highlight
	color_line(b, hightlight_line[b], get_active_color(b));
	// highlight line i
	hightlight_line[b] = i;
	color_line(b, i, get_highlight_color(b));
}
//////////////////////////////////////////////
// DRAW API
//////////////////////////////////////////////


//////////////////////////////////////////////
// ANIM API
//////////////////////////////////////////////
void init_anim(int b){
	int i;
	for(i = 0; i < line_cnt; i++){
		color_line(b, i, get_init_color(b));
		msleep(init_anim_period);
	}
}

float move_interpolator(float p){
	if(p > 1){
		return 1 + sin(3.1415f * (p - 0.5f)) * wave_percent;
	}
	else{
		return sin(0.5f * 3.1415f * p) * (1 + wave_percent);
	}
}

float color_interpolator(float p){
	return 0.5f * sin(3.1415f * (p - 0.5)) + 0.5f;
}

void exit_anim(int b){
	int unit = block_width - 2;
	time_t start = elapsed();
	time_t curr;
	int i, delay, move, margin_left;
	float move_percent, color_percent;
	COLORREF base_color;
	while((curr = elapsed())){
		// color is merged by two finish_colors, and the merge weight change by time continuously
		color_percent = color_interpolator((curr - start) / (float)(exit_anim_duration));
		base_color = merge_color(get_finish_color((b + 1) % blocks_size), get_finish_color(b), color_percent);
		for(i = 0; i < line_cnt; i++){
			// make lines move around the middle point periodly
			// the delay makes the trinkle effect
			delay = i * delay_unit;
			if(i & 1){
				delay += delay_unit * delay_unit * sin(3.1415f * (i / (float)line_cnt));
			}
			move = (unit - arrs[b][i]) / 2;
			move_percent = max(0, curr - start - delay) / (float)exit_anim_duration;
			if(move < 0) move = 0;
			if(move > unit) move = unit;
			margin_left = (int)(move * move_interpolator(move_percent));
			
			color_line_with_margin(b, i, margin_left, intense(b, i, base_color));
		}
		msleep(30);
	}
}

void finish_anim(int b){
	int i;
	for(i = line_cnt - 1; i >= 0; i--){
		color_line(b, i, get_finish_color(b));
		msleep(finish_anim_period);
	}
	exit_anim(b);
}
//////////////////////////////////////////////
// ANIM API
//////////////////////////////////////////////


//////////////////////////////////////////////
// SORT API
//////////////////////////////////////////////
void swap(int b, int i, int j){
	int t = arrs[b][i];
	arrs[b][i] = arrs[b][j];
	arrs[b][j] = t;
	color_line(b, i, get_active_color(b));
	// highlight the last swap line
	draw_highlight_line(b, j);
	msleep(swap_period);
}

void bubble_sort(int b, int * arr, int len){
	printf("sort %d\n", b);
	int i, j;
	for(i = 0; i < len; i++){
		for(j = len-1; j > 0; j--){
			if(arr[j] < arr[j-1]){
				swap(b, j, j-1);
			}
		}
	}
}

//////////////////////////////////////////////
// SORT API
//////////////////////////////////////////////

struct sort_task tasks[MAX];

void sort_wrapper(void *p){
	struct sort_task * task = (struct sort_task*) p;
	init_anim(task->block);
	task->sort_method(task->block, task->arr, line_cnt);
	finish_anim(task->block);
	task_exit(0);
}

int new_sort_thread(void (*sort)(int block, int * arr, int len), int * arr){
	unsigned char * stack;
	unsigned int size = 1024 * 1024;
	stack = malloc(size);
	// copy array
	int * arrcpy = malloc(line_cnt * (sizeof arr));
	memcpy(arrcpy, arr, line_cnt * (sizeof arr));
	
	int i = blocks++;
	arrs[i] = arrcpy;
	tasks[i].sort_method = sort;
	tasks[i].arr = arrcpy;
	tasks[i].block = i;
	
	// new thread
	tasks[i].tid = task_create(stack + size, sort_wrapper, (void*)(&tasks[i]));
	return tasks[i].tid;
}

int debug = 0;
void init_params(){
	if(debug) {
		width = 100;
		height = 100;
		//list_graphic_modes();
	}
	else{
		init_graphic(0x143);
		width = g_graphic_dev.XResolution;
		height = g_graphic_dev.YResolution;
	}
	int blocks = blocks_size;
	block_width = (width / col_cnt);
	block_height = (int) (height * vertical_weight / (blocks / col_cnt + (blocks % col_cnt != 0)));
	marginTop = (int) (block_height * (1 - vertical_weight));
	line_cnt = (int) (block_height / (margin + line_height));
}

#define NZERO 20
// 0 <= pro <= 2*NZERO-1
void set_progress(int b, int pro){
	pro = pro * block_width / 39;
	
	int x = b * block_width;
	int y = 10;
	int i;
	for(i = 0; i < 3; i++){
		line(x, y+i, x+block_width, y+i, get_finish_color(0));
		line(x, y+i, x+pro, y+i, get_active_color(b));
	}
}

void control(void *p){
	int p1 = 20, p2 = 20;
	
	// 默认优先级已经是最高优先级
	setpriority(task_getid(), 0);
	//setpriority(tasks[0].tid, p1);
	//setpriority(tasks[1].tid, p2);
	
	//set_progress(0, p1);
	//set_progress(1, p2);
	
	#define UP 0x4800
	#define DOWN 0x5000
	#define LEFT 0x4d00
	#define RIGHT 0x4b00
	int key;
	while(1){
		key = getchar();
		if(UP == key){
			if(p1 < (2 * NZERO - 1)){
				p1++;
				setpriority(tasks[0].tid, p1);
				set_progress(0, p1);
				printf("set priority: task %d(%d)\n", tasks[0].tid, p1);
			}
		}
		else if(DOWN == key){
			if(p1 > 0){
				p1--;
				setpriority(tasks[0].tid, p1);
				set_progress(0, p1);
				printf("set priority: task %d(%d)\n", tasks[0].tid, p1);
			}
		}
		else if(LEFT == key){
			if(p2 < (2 * NZERO - 1)){
				p2++;
				setpriority(tasks[1].tid, p2);
				set_progress(1, p2);
				printf("set priority: task %d(%d)\n", tasks[1].tid, p2);
			}
		}
		else if(RIGHT == key){
			if(p2 > 0){
				p2--;
				setpriority(tasks[1].tid, p2);
				set_progress(1, p2);
				printf("set priority: task %d(%d)\n", tasks[1].tid, p2);
			}
		}
	}
	task_exit(0);
}

void sem_test(){
	init_params();
	
	// random array
	srand(time(NULL));
	int i;
	int * arr;
	arr = malloc((line_cnt) * (sizeof i));
	for(i = 0; i < line_cnt; i++){
		arr[i] = rand() % (block_width - 2); // 为了避免图像边缘重叠，长度-2
	}
	
	// new thread
	for(i = 0; i < blocks_size; i++){
		new_sort_thread(bubble_sort, arr);
	}

	/*
	unsigned char * stack;
	unsigned int size = 1024 * 1024;
	stack = malloc(size);
	int cid = task_create(stack + size, control, (void*)0);
	
	task_wait(cid, NULL);
	free(stack);
	*/
	control((void*)0);
	
	// wait each of them
	for(i = 0; i < blocks; i++){
		task_wait(tasks[i].tid, NULL);
		free(tasks[i].arr);
	}
	
	free(arr);
	
	exit_graphic();
}
