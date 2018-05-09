#include "draw.c"
#include "sort.c"
#include "anim.c"

struct sort_task{
	void (*sort_method)(int block, int * arr, int len);
	int * arr; // 待排序数组
	int block; // 在哪块区域绘制
	int tid;
};

int blocks;
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

void thread_sort_test(){
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
	new_sort_thread(insertion_sort, arr);
	new_sort_thread(shell_sort, arr);
	new_sort_thread(selection_sort, arr);
	new_sort_thread(merge_sort, arr);
	new_sort_thread(quick_sort, arr);
	
	for(i = 0; i < blocks; i++){
		task_wait(tasks[i].tid, NULL);
	}
	
	exit_graphic();
}