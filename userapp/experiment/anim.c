#include "anim.h"

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
	//exit_anim(b);
}
