#ifndef _ANIM_H_
#define _ANIM_H_
#include "draw.h"
#include "user.h"

void init_anim(int b);

float move_interpolator(float p);

float color_interpolator(float p);

void exit_anim(int b);

void finish_anim(int b);

#endif