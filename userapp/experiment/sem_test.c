#include "draw.c"
#include "sort.c"
#include "anim.c"

int blocks;

int pid, cid;

int debug = 0;
void init_params(){
	if(debug) {
		init_draw_params(100, 100, 10);
	}
	else{
		init_graphic(0x143);
		int width = g_graphic_dev.XResolution;
		int height = g_graphic_dev.YResolution;
		init_draw_params(width, height, 10);
	}
}

#define NZERO 20

void control(void *p){
	int p1 = 20, p2 = 20;
	
	// 默认优先级已经是最高优先级
	setpriority(task_getid(), 0);
	setpriority(pid, p1);
	setpriority(cid, p2);
	
	set_progress(0, p1);
	set_progress(1, p2);
	
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
				setpriority(pid, p1);
				set_progress(0, p1);
			}
		}
		else if(DOWN == key){
			if(p1 > 0){
				p1--;
				setpriority(pid, p1);
				set_progress(0, p1);
			}
		}
		else if(LEFT == key){
			if(p2 < (2 * NZERO - 1)){
				p2++;
				setpriority(cid, p2);
				set_progress(1, p2);
			}
		}
		else if(RIGHT == key){
			if(p2 > 0){
				p2--;
				setpriority(cid, p2);
				set_progress(1, p2);
			}
		}
	}
	task_exit(0);
}

int full;
int empty;

void produce(void *p){
	int i;
	for(i = 0; ;i++, i %= blocks_size){
		sem_wait(empty);
		
		if(arrs[i] != NULL){
			free(arrs[i]);
		}
		
		// random array
		srand(time(NULL));
		int j;
		int * arr;
		arr = malloc((line_cnt) * (sizeof i));
		for(j = 0; j < line_cnt; j++){
			arr[j] = rand() % (block_width - 2); // 为了避免图像边缘重叠，长度-2
		}
		arrs[i] = arr;
		
		init_anim(i);
		
		sem_signal(full);
	}
}

void (*sorter[6])(int b, int * arr, int len) = {
	bubble_sort,
	insertion_sort,
	shell_sort,
	selection_sort,
	merge_sort,
	quick_sort,
};

void consume(void *p){
	int i, b;
	for(i = 0; ;i++){
		sem_wait(full);
		
		b = i % blocks_size;
		sorter[i%6](b, arrs[b], line_cnt);
		//finish_anim(i);
		
		sem_signal(empty);
	}
}

void sem_test(){
	init_params();
	
	// 创建两个信号量，有多少个资源就设置empty值为多少
	full = sem_create(0);
	empty = sem_create(blocks_size);
	
	unsigned int size = 1024 * 1024;
	unsigned char * produce_stack = (unsigned char *) malloc(size);
	pid = task_create(produce_stack + size, produce, (void*)0);
	
	unsigned char * consume_stack = (unsigned char *) malloc(size);
	cid = task_create(consume_stack + size, consume, (void*)0);
	
	control((void*)0);
	
	task_wait(pid, NULL);
	task_wait(cid, NULL);
	
	exit_graphic();
}
