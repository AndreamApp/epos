#ifndef _USER_H_
#define _USER_H_
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

#define MAX 25

// layout config
int margin = 1;
int line_height = 1;
float vertical_weight = 0.9f;
int col_cnt = 10; // 每列显示3个排序函数

// anim config
int swap_period = 0;
int init_anim_period = 10;
int finish_anim_period = 10;
int exit_anim_period = 10;
int exit_anim_duration = 200;

float wave_percent = 0.3f;
int delay_unit = 2;

// basic sharing params
int blocks_size;
int line_cnt;

// sharing data
int * arrs[MAX];


/**
 * 休眠msec毫秒
 */
int msleep(const uint32_t msec)
{
	struct timespec ts={0,1000000*msec};
	return nanosleep(&ts, NULL);
}


#endif