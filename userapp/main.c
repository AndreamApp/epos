/*
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
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
#include "graphics.h"

extern void *tlsf_create_with_pool(void* mem, size_t bytes);
extern void *g_heap;

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
    size_t heap_size = 32*1024*1024;
    void  *heap_base = mmap(NULL, heap_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
	g_heap = tlsf_create_with_pool(heap_base, heap_size);
}

/*
Feature:

init animation
finish animation
exit wave animation

multiple color
gradient color

layout configuration
anim configuration
*/

#define MAX 10

//// Configuration
int margin = 1;
int line_height = 3;
float vertical_weight = 0.95f;
int col_cnt = 3; // 每列显示3个排序函数

int init_anim_period = 100;
int finish_anim_period = 100;
int exit_anim_period = 10;
int exit_anim_duration = 1000;

float wave_percent = 0.1f;
int sort_methods_size = 6;
//// Configuration

COLORREF init_colors[MAX] = {
	RGB(214, 188, 255), RGB(220, 247, 161), RGB(255, 120, 120), RGB(186, 212, 170), RGB(172, 208, 206), RGB(247, 223, 131), 
};
COLORREF active_colors[MAX] = {
	RGB(191, 134, 171), RGB(131, 252, 216), RGB(120, 180, 250), RGB(212, 212, 170), RGB(77, 147, 159), RGB(247, 185, 131), 
};
COLORREF finish_colors[MAX] = {
	RGB(218, 106, 135), RGB(129, 146, 214), RGB(86, 163, 108), RGB(124, 99, 84), RGB(59, 62, 71), RGB(247, 147, 131), 
};
COLORREF highlight_colors[MAX] = {
	RGB(218, 106, 135), RGB(129, 146, 214), RGB(46, 104, 170), RGB(124, 99, 84), RGB(59, 62, 71), RGB(247, 147, 131), 
};
COLORREF intense_colors[MAX] = {
	RGB(0, 2, 0),       RGB(2, 0, 0),       RGB(2, 0, 0),      RGB(0, 0, 2),        RGB(4, 0, 0),       RGB(0, 0, 3), 
};
int hightlight_line[MAX];

int width, height;
int block_width;
int block_height;
int marginTop;
int line_cnt;

int blocks;
int * arrs[MAX];

/**
 * 休眠msec毫秒
 */
int msleep(const uint32_t msec)
{
	struct timespec ts={0,1000000*msec};
	return nanosleep(&ts, NULL);
}

//////////////////////////////////////////////
// DRAW API
//////////////////////////////////////////////

// for gradient
COLORREF intense(int b, int i, COLORREF color){
	int R = getRValue(color) + i * getRValue(intense_colors[b]);
	int G = getGValue(color) + i * getGValue(intense_colors[b]);
	int B = getBValue(color) + i * getBValue(intense_colors[b]);
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

// for wave animation
void color_line_with_margin(int b, int i, int mrgn, COLORREF color){
	int x = block_width * (b % col_cnt);
	int y = i * (margin + line_height) + ((b / col_cnt) * (block_height + marginTop) + marginTop);
	color = intense(b, i, color);
	int unit = (block_width - 2);
	int t;
	for(t = 0; t < line_height; t++){
		line(x, y+t, x + unit, y+t, RGB(0, 0, 0));
		line(x + mrgn, y+t, x + min(mrgn + arrs[b][i], unit), y+t, color);
	}
}

/**
 * 绘制第b个数组第i个值，即arrs[b][i]，指定颜色color
 */
void color_line(int b, int i, COLORREF color){
	color_line_with_margin(b, i, 0, color);
}

/**
 * 在排序过程中调用此函数绘制，使用highlight_color
 * 同一时间只会有一个highlight
 */
void draw_highlight_line(int b, int i){
	// disable origin highlight
	color_line(b, hightlight_line[b], active_colors[b]);
	// highlight line i
	hightlight_line[b] = i;
	color_line(b, i, highlight_colors[b]);
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
		color_line(b, i, init_colors[b]);
		msleep(init_anim_period);
	}
}

float interpolator(float p){
	if(p > 1){
		return 1 + sin(3.1415f * (p - 0.5f)) * wave_percent;
	}
	else{
		return sin(0.5f * 3.1415f * p) * (1 + wave_percent);
	}
}

void exit_anim(int b){
	int unit = block_width - 2;
	time_t start = elapsed();
	time_t curr;
	int i, delay, move, margin;
	float percent;
	while((curr = elapsed())){
		for(i = 0; i < line_cnt; i++){
			delay = i * 10;
			percent = max(0, curr - start - delay) / (float)exit_anim_duration;
			move = (unit - arrs[b][i]) / 2;
			margin = (int)(move * interpolator(percent));
			color_line_with_margin(b, i, margin, finish_colors[b]);
			if(i % (line_cnt / 2) == 0) 
				msleep(10);
		}
		msleep(100);
	}
}

void finish_anim(int b){
	int i;
	for(i = line_cnt - 1; i >= 0; i--){
		color_line(b, i, finish_colors[b]);
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
	color_line(b, i, active_colors[b]);
	// highlight the last swap line
	draw_highlight_line(b, j);
	msleep(10);
}

void bubble_sort(void * p){
	int b = 0;
	int * arr = arrs[b];
	int len = line_cnt;
	init_anim(b);
	int i, j;
	for(i = 0; i < len; i++){
		for(j = len-1; j > 0; j--){
			if(arr[j] < arr[j-1]){
				swap(b, j, j-1);
			}
		}
	}
	finish_anim(b);
	task_exit(0);
}

void insertion_sort(void * p){
	int b = 1;
	int * arr = arrs[b];
	int len = line_cnt;
	init_anim(b);
	int i, j;
	for(i = 1; i < len; i++){
		for(j = i; j > 0 && arr[j] < arr[j-1]; j--){
			swap(b, j, j-1);
		}
	}
	finish_anim(b);
	task_exit(0);
}


// 希尔排序
// 用于希尔排序的插入排序，incr为步长
void inssort4shell(int * arr, int b, int n, int incr) {
	int i, j;
	for (i = incr; i<n; i += incr)
		for (j = i; (j >= incr) && (arr[j] < arr[j - incr]); j -= incr)
			swap(b, j, j - incr);
}

void _shell_sort(int b, int n) { // 希尔排序
	int * arr = arrs[b];
	int i, j;
	for (i = n / 2; i >= 2; i /= 2)      // i是步长，将数组分为i个较为分散的子数组
		for (j = 0; j < i; j++)       // 对这i个数组分别排序
			inssort4shell(&arr[j], b, n - j, i);
	// 最后来一次汇总式的插入排序，因为数组已经基本有序，所以复杂度没有O(n^2)那么高
	inssort4shell(arr, b, n, 1);
}

void shell_sort(void *p){
	int b = 2;
	init_anim(b);
	_shell_sort(b, line_cnt);
	finish_anim(b);
	task_exit(0);
}

void selection_sort(void * p){
	int b = 3;
	int * arr = arrs[b];
	int len = line_cnt;
	int i, j;
	init_anim(b);
	for(i = 0; i < len-1; i++){
		int low = i;
		for(j = i+1; j < len; j++){
			// set highlight in selection
			draw_highlight_line(b, j);
			msleep(9);
			if(arr[j] < arr[low]){
				low = j;
				msleep(10);
			}
		}
		swap(b, i, low);
	}
	finish_anim(b);
	task_exit(0);
}

void _merge_sort(int * arr, int * tmp, int left, int right){
	if(left >= right) return;
	int mid = (left+right) / 2;
	_merge_sort(arr, tmp, left, mid);
	_merge_sort(arr, tmp, mid+1, right);
	int i, j, curr;
	for(i = 0; i < line_cnt; i++){
		tmp[i] = arr[i];
	}
	i = left, j = mid+1;
	for(curr = left; curr <= right; curr++){
		if(i == mid+1) arr[curr] = tmp[j++];
		else if(j > right) arr[curr] = tmp[i++];
		else if(tmp[i] < tmp[j]) arr[curr] = tmp[i++];
		else arr[curr] = tmp[j++];
		
		// set highlight
		draw_highlight_line(4, curr);
		msleep(10);
	}
	//printf("end merge %d-%d\n", left, right);
}

void merge_sort(void *p){
	int b = 4;
	int * arr = arrs[b];
	int * tmp = (int *)malloc(line_cnt * (sizeof arr));
	init_anim(b);
	_merge_sort(arr, tmp, 0, line_cnt-1);
	finish_anim(b);
	task_exit(0);
}


// 划分数组，以pivot为比较对象，小的往左边放，大的往右边放
// 返回划分点的下标
int partition(int b, int l, int r, int pivot) {
	int * arr = arrs[b];
	do {             // l 和 r将不断靠近，直到相遇
		while (arr[++l] < pivot);  // 从左向右找到一个比pivot大的数！
		while ((l < r) && pivot < arr[--r]); // 从右向左找到一个比pivot小的数！
		swap(b, l, r);              // 让他们俩交换！
	} while (l < r);              // 重复进行
	return l;      // 最后l==r，也就是划分点
}

// 快速排序核心函数
void _quick_sort(int b, int i, int j) {
	if (j <= i) return; // 只有0个或1个元素的数组是有序的
	int pivotindex = (i+j) / 2;
	int * arr = arrs[b];
	swap(b, pivotindex, j);    // 将支点移到数组尾部
	// 对数组进行划分，结束后满足：[i,k) < pivot  &&  [k,j] > pivot
	int k = partition(b, i - 1, j, arr[j]);
	swap(b, k, j);             // 把支点的值换回来
	_quick_sort(b, i, k - 1); // 对左边继续快排
	_quick_sort(b, k + 1, j); // 对右边继续快排
}

void quick_sort(void *p){
	int b = 5;
	init_anim(b);
	_quick_sort(b, 0, line_cnt-1);
	finish_anim(b);
	task_exit(0);
}

//////////////////////////////////////////////
// SORT API
//////////////////////////////////////////////

void sort_wrapper(void *p){
	void (*sort)(int * arr, int len) = p;
	int b = 5;
	init_anim(b);
	sort(arrs[b], line_cnt);
	finish_anim(b);
	task_exit(0);
}

int new_sort_thread(void (*sort)(void* p), int * arr){
	unsigned char * stack;
	unsigned int size = 1024 * 1024;
	stack = malloc(size);
	// copy array
	int * arrcpy = malloc(line_cnt * (sizeof arr));
	memcpy(arrcpy, arr, line_cnt * (sizeof arr));
	arrs[blocks++] = arrcpy;
	// new thread
	int tid = task_create(stack + size, sort, (void*)(0));
	return tid;
}

void (*sort_methods[MAX])(void *p) = {
	bubble_sort,
	insertion_sort,
	shell_sort,
	selection_sort,
	merge_sort,
	quick_sort,
};
int sort_thread_tids[MAX];

int debug = 0;
/**
 * 第一个运行在用户模式的线程所执行的函数
 */
void main(void *pv)
{
    printf("task #%d: I'm the first andream user task(pv=0x%08x)!\r\n",
            task_getid(), pv);

    //TODO: Your code goes here
	if(debug) {
		width = 100;
		height = 100;
		line_cnt = 100;
		list_graphic_modes();
	}
	else{
		init_graphic(0x143);
		width = g_graphic_dev.XResolution;
		height = g_graphic_dev.YResolution;
		
		int blocks = sort_methods_size;
		block_width = (width / col_cnt);
		block_height = (int) (height * vertical_weight / (blocks / col_cnt + (blocks % col_cnt != 0)));
		marginTop = (int) (block_height * (1 - vertical_weight));
		line_cnt = (int) (block_height / (margin + line_height));
	}
	
	// random array
	srand(time(NULL));
	int i;
	int * arr;
	arr = malloc((line_cnt) * (sizeof i));
	for(i = 0; i < line_cnt; i++){
		arr[i] = rand() % (block_width - 2); // 为了避免图像边缘重叠，长度-2
	}
	
	// new thread
	for(i = 0; i < sort_methods_size; i++){
		sort_thread_tids[i] = new_sort_thread(sort_methods[i], arr);
	}
	
	// wait each of them
	for(i = 0; i < sort_methods_size; i++){
		task_wait(sort_thread_tids[i], NULL);
		free(arrs[i]);
	}
	
	free(arr);
	
	exit_graphic();
    while(1);
    task_exit(0);
}

