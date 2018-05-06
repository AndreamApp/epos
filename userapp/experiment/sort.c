#include "sort.h"

void swap(int b, int i, int j){
	int t = arrs[b][i];
	arrs[b][i] = arrs[b][j];
	arrs[b][j] = t;
	color_line(b, i, get_active_color(b));
	// highlight the last swap line
	draw_highlight_line(b, j);
	msleep(swap_period);
}

void bubble_sort(int b, int * arr, int len){
	printf("sort %d\n", b);
	int i, j;
	for(i = 0; i < len; i++){
		for(j = len-1; j > 0; j--){
			if(arr[j] < arr[j-1]){
				swap(b, j, j-1);
			}
		}
		msleep(10);
	}
}


void insertion_sort(int b, int * arr, int len){
	int i, j;
	for(i = 1; i < len; i++){
		for(j = i; j > 0 && arr[j] < arr[j-1]; j--){
			swap(b, j, j-1);
		}
		msleep(10);
	}
}


// 希尔排序
// 用于希尔排序的插入排序，incr为步长
void inssort4shell(int * arr, int b, int n, int incr) {
	int i, j;
	for (i = incr; i<n; i += incr)
		for (j = i; (j >= incr) && (arr[j] < arr[j - incr]); j -= incr)
			swap(b, j, j - incr);
}

void shell_sort(int b, int * arr, int n) { // 希尔排序
	int i, j;
	for (i = n / 2; i >= 2; i /= 2)      // i是步长，将数组分为i个较为分散的子数组
		for (j = 0; j < i; j++)       // 对这i个数组分别排序
			inssort4shell(&arr[j], b, n - j, i);
	// 最后来一次汇总式的插入排序，因为数组已经基本有序，所以复杂度没有O(n^2)那么高
	inssort4shell(arr, b, n, 1);
}

void selection_sort(int b, int * arr, int len){
	int i, j;
	for(i = 0; i < len-1; i++){
		int low = i;
		for(j = i+1; j < len; j++){
			// set highlight in selection
			draw_highlight_line(b, j);
			if(arr[j] < arr[low]){
				low = j;
			}
		}
		swap(b, i, low);
		msleep(10);
	}
}

void _merge_sort(int b, int * arr, int * tmp, int left, int right){
	if(left >= right) return;
	int mid = (left+right) / 2;
	_merge_sort(b, arr, tmp, left, mid);
	_merge_sort(b, arr, tmp, mid+1, right);
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
		draw_highlight_line(b, curr);
	}
	msleep(10);
}

void merge_sort(int b, int * arr, int len){
	int * tmp = (int *)malloc(len * (sizeof arr));
	_merge_sort(b, arr, tmp, 0, len-1);
}


// 划分数组，以pivot为比较对象，小的往左边放，大的往右边放
// 返回划分点的下标
int partition(int b, int l, int r, int pivot) {
	int * arr = arrs[b];
	do {             // l 和 r将不断靠近，直到相遇
		while (arr[++l] < pivot);  // 从左向右找到一个比pivot大的数！
		while ((l < r) && pivot < arr[--r]); // 从右向左找到一个比pivot小的数！
		swap(b, l, r);              // 让他们俩交换！
		msleep(10);
	} while (l < r);              // 重复进行
	return l;      // 最后l==r，也就是划分点
}

// 快速排序核心函数
void _quick_sort(int b, int * arr, int i, int j) {
	if (j <= i) return; // 只有0个或1个元素的数组是有序的
	int pivotindex = (i+j) / 2;
	swap(b, pivotindex, j);    // 将支点移到数组尾部
	// 对数组进行划分，结束后满足：[i,k) < pivot  &&  [k,j] > pivot
	int k = partition(b, i - 1, j, arr[j]);
	swap(b, k, j);             // 把支点的值换回来
	_quick_sort(b, arr, i, k - 1); // 对左边继续快排
	_quick_sort(b, arr, k + 1, j); // 对右边继续快排
}

void quick_sort(int b, int * arr, int len){
	_quick_sort(b, arr, 0, len-1);
}

