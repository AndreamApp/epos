#include "draw.c"
#include "sort.c"
#include "anim.c"

#define NZERO 20
#define UP 0x4800
#define DOWN 0x5000
#define LEFT 0x4d00
#define RIGHT 0x4b00

int blocks;
struct sort_task{
	void (*sort_method)(int block, int * arr, int len);
	int * arr; // 待排序数组
	int block; // 在哪块区域绘制
	int tid;
};

struct sort_task tasks[MAX];

void sort_wrapper(void *p){
	struct sort_task * task = (struct sort_task*) p;
	init_anim(task->block);
	task->sort_method(task->block, task->arr, line_cnt);
	finish_anim(task->block);
	exit_anim(task->block);
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

void control(void *p){
	int p1 = 20, p2 = 20;
	
	// 默认优先级已经是最高优先级
	setpriority(task_getid(), 0);
	setpriority(tasks[0].tid, p1);
	setpriority(tasks[1].tid, p2);
	
	set_progress(0, p1);
	set_progress(1, p2);
	
	int key;
	while(1){
		key = getchar();
		if(UP == key){
			if(p1 < (2 * NZERO - 1)){
				p1++;
				setpriority(tasks[0].tid, p1);
				set_progress(0, p1);
			}
		}
		else if(DOWN == key){
			if(p1 > 0){
				p1--;
				setpriority(tasks[0].tid, p1);
				set_progress(0, p1);
			}
		}
		else if(LEFT == key){
			if(p2 < (2 * NZERO - 1)){
				p2++;
				setpriority(tasks[1].tid, p2);
				set_progress(1, p2);
			}
		}
		else if(RIGHT == key){
			if(p2 > 0){
				p2--;
				setpriority(tasks[1].tid, p2);
				set_progress(1, p2);
			}
		}
	}
}

void thread_priority_test(){
	init_draw(0);
	
	// random array
	srand(time(NULL));
	int i;
	int * arr;
	arr = malloc((line_cnt) * (sizeof i));
	for(i = 0; i < line_cnt; i++){
		arr[i] = rand() % (block_width - 2); // 为了避免图像边缘重叠，长度-2
	}
	
	// new thread
	new_sort_thread(bubble_sort, arr);
	new_sort_thread(bubble_sort, arr);

	control((void*)0);
	
	// wait each of them
	for(i = 0; i < blocks; i++){
		task_wait(tasks[i].tid, NULL);
		free(tasks[i].arr);
	}
	
	free(arr);
	
	exit_graphic();
}
