#include "draw.h"

void init_draw_params(int w, int h, int blocks){
	width = w;
	height = h;
	blocks_size = blocks;
	
	block_width = (width / col_cnt);
	block_height = (int) (height * vertical_weight / (blocks / col_cnt + (blocks % col_cnt != 0)));
	marginTop = (int) (block_height * (1 - vertical_weight));
	line_cnt = (int) (block_height / (margin + line_height));
}

int color_size = 6;
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
	RGB(1, 0, 0),       RGB(0, 1, 0),       RGB(1, 0, 0),       RGB(0, 1, 0),       RGB(1, 0, 0),       RGB(0, 1, 0),       
};

COLORREF get_init_color(int b){
	return init_colors[b % color_size];
}

COLORREF get_active_color(int b){
	return active_colors[b % color_size];
}

COLORREF get_finish_color(int b){
	return finish_colors[b % color_size];
}

COLORREF get_highlight_color(int b){
	return highlight_colors[b % color_size];
}

COLORREF get_intense_color(int b){
	return intense_colors[b % color_size];
}

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
	return color;
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


// 0 <= pro <= 2*NZERO-1
void set_progress(int b, int pro){
	int full_width = width / 2 - 2;
	pro = pro * full_width / 39;
	
	int x = b * full_width;
	int y = 10;
	int i;
	for(i = 0; i < 3; i++){
		line(x, y+i, x+full_width, y+i, RGB(55,55,55));
		line(x, y+i, x+pro, y+i, get_active_color(b));
	}
}
