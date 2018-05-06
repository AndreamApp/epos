#ifndef _SORT_H_
#define _SORT_H_
#include "draw.h"
#include "user.h"

void swap(int b, int i, int j);

void bubble_sort(int b, int * arr, int len);

void insertion_sort(int b, int * arr, int len);

void shell_sort(int b, int * arr, int len);

void selection_sort(int b, int * arr, int len);

void merge_sort(int b, int * arr, int len);

void quick_sort(int b, int * arr, int len);

#endif