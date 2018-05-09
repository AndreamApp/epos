#ifndef _DRAW_H_
#define _DRAW_H_
#include "../graphics.h"
#include "user.h"

int width, height;
int block_width;
int block_height;
int marginTop;

int draw_left_bound_cache[MAX][1000];
int draw_right_bound_cache[MAX][1000];
int hightlight_line[MAX];

void init_draw_params(int w, int h, int blocks);

void init_draw(int debug);

COLORREF get_init_color(int b);

COLORREF get_active_color(int b);

COLORREF get_finish_color(int b);

COLORREF get_highlight_color(int b);

COLORREF get_intense_color(int b);

// for gradient
COLORREF intense(int b, int i, COLORREF color);

COLORREF merge_color(COLORREF c1, COLORREF c2, float k);

// for wave animation
void color_line_with_margin(int b, int i, int mrgn, COLORREF color);

/**
 * 绘制第b个数组第i个值，即arrs[b][i]，指定颜色color
 */
void color_line(int b, int i, COLORREF color);

/**
 * 在排序过程中调用此函数绘制，使用highlight_color
 * 同一时间只会有一个highlight
 */
void draw_highlight_line(int b, int i);


// 0 <= pro <= 2*NZERO-1
void set_progress(int b, int pro);

#endif