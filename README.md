# epos

forked from hongmingjian

## 实验二 - 使用多线程排序

### Feature

**0. 绘制数组**

把数组绘制到屏幕上，是这个实验的基本功能。可以封装一个绘制数组的函数，然后每次更新数组的时候，重新绘制整个数组。这样就每次数组变化都会体现到屏幕上，就能看到排序的整个过程了。伪代码如下：

```C
COLORREF bg_color;
COLORREF active_color;

void draw_arr(int arr[], int len){
	int i;
	for(i = 0; i < len; i++){
		line(0, i, width, i, bg_color); // 先使用背景色填充
		line(0, i, arr[i], i, active_color); // 绘制一行
	}
}

void sample_sort(int arr[], int len){
	int i, j;
	for(i = 0; i < len; i++){
		for(int j = len-1; j >= 0; j--){
			if(arr[i] > arr[j]){
				swap(arr, i, j);
				draw_arr(arr, len); // 重绘
			}
		}
	}
}
```

这样实现，结果就是屏幕闪烁很严重，因为绘制量太大。可以很容易想到优化的办法——只重绘交换的两行

```C
int width;
COLORREF bg_color;
COLORREF active_color;

void draw_line(int arr[], int i, COLORREF color){
	line(0, i, width, i, bg_color); // 先使用背景色填充
	line(0, i, arr[i], i, color); // 绘制一行
}

void draw_arr(int arr[], int len){
	int i;
	for(i = 0; i < len; i++){
		draw_line(arr, i, active_color);
	}
}

void swap(int arr[], int i, int j){
	int t = arr[i];
	arr[i] = arr[j];
	arr[j] = t;
	draw_line(arr, i, active_color);
	draw_line(arr, j, active_color);
	msleep(10);
}

void sample_sort(int arr[], int len){
	draw_arr(arr, len);
	int i, j;
	for(i = 0; i < len; i++){
		for(int j = len-1; j >= 0; j--){
			if(arr[i] > arr[j]){
				swap(arr, i, j);
			}
		}
	}
}
```

观察上面的代码，`draw_arr()`只在开始调用过一次，之后只在`swap`的时候重新绘制交换的两行。因为`swap`在其他地方多次调用，因此把重绘代码封装到`swap`函数内部。
`draw_arr`中也可以重用`draw_line`。

为了避免排序太快，在`swap`中调用`msleep(10)`。这样修改之后，排序就可以流畅的进行了。

**1. 颜色渐变**

在上面的示例代码中，展示了如何将排序过程绘制出来。但是每个数组只有一个颜色，现在来点骚操作，想加入渐变效果，怎么搞呢？

![Gradient color](https://github.com/AndreamApp/epos/raw/master/screenshots/gradient_color.jpg)

要实现渐变效果，根据不同的行号使用不同的颜色绘制。也就是要实现一个这样的函数：

```C
COLORREF get_gradient_color(COLORREF base_color, int i);
```

然后由于良好的封装，我们只需要修改draw_line函数：

```C
void draw_line(int arr[], int i, COLORREF color){
	line(0, i, width, i, bg_color); // 先使用背景色填充
	line(0, i, arr[i], i, get_gradient_color(color, i)); // 绘制一行
}
```

那`get_gradient_color`具体怎么实现呢？这个可以有很多的实现方法，唯一需要注意的就是要保证函数连续性。还需要一点计算机颜色的基础知识，这里不多说了。这个项目里的实现如下：

```C
COLORREF intense_color;

COLORREF get_gradient_color(COLORREF color, int i){
	int R = getRValue(color) + i * getRValue(intense_color);
	int G = getGValue(color) + i * getGValue(intense_color);
	int B = getBValue(color) + i * getBValue(intense_color);
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
	return RGB(R, G, B);
}
```

**2. 波浪动画**

![Wave animation](https://github.com/AndreamApp/epos/raw/master/screenshots/wave_anim.gif)

上面这个效果还可以哈，我们现在讨论怎么实现。

首先，波浪动画是在排序结束之后才开始的。也就是`arr`（我们的数据源）是固定不变的了。数据没有变，变的是我们的绘制参数。换句话说，动画的关键就是随着时间变化不断改变我们的绘制参数（在本例中可以观察到，改变的就是绘制的x坐标）。在下面的伪码中，可以看到时间在动画里的作用：

```C
void wave_anim(int arr[], int len) {
	time_t start = get_curr_time();
	time_t curr;
	while(1) {
		curr = get_curr_time();
		int x = (curr - start) / 10;
		int i;
		for(i = 0; i < len; i++){
			line(0, i, width, i, bg_color); // 先使用背景色填充
			line(x, i, x + arr[i], i, get_gradient_color(color, i)); // 绘制一行
		}
		msleep(100);
	}
}
```

其中`get_curr_time`用来获取系统时间，单位是毫秒（用秒作单位无法获得流畅的动画）。
因为实验一提供的`time()`系统调用返回的时间单位为秒，所以这里需要另外实现一个以毫秒为单位的系统调用`elapsed()`。具体可以看相关代码。

x = (curr - start) / 10; x是线段绘制的起始坐标，也就是每10毫秒向右移动一个像素。随着时间进行，就可以看到整个数组在向右`匀速`运动。

要实现灵动的动画，就不要用匀速的，要搞点加速度：

```C
void wave_anim(int arr[], int len) {
	time_t start = elapsed();
	time_t curr;
	while(1) {
		curr = elapsed();
		float t = (curr - start) / 1000.0f;
		int x = (int) (100 * t * t);
		int i;
		for(i = 0; i < len; i++){
			line(0, i, width, i, bg_color); // 先使用背景色填充
			line(x, i, x + arr[i], i, get_gradient_color(color, i)); // 绘制一行
		}
		msleep(100);
	}
}
```

上面把get_curr_time换成了elapsed，t代表相对时间，x是位移，而且是匀加速运动。类似的，我们要实现匀减速或者各种花式运动，只要把x计算公式修改了就行。

有灵性的同学可能想到了，这里可以把x的计算公式抽象出来，这样程序就更清晰啦，拓展性也更强啦！

```C
float duration = 1000.0f;

float interpolator(float t){
	return 10 * t * t;
}

void wave_anim(int arr[], int len) {
	time_t start = elapsed();
	time_t curr;
	while(1) {
		curr = elapsed();
		float t = (curr - start) / duration;
		int x = (int) (100 * interpolator(t));
		int i;
		for(i = 0; i < len; i++){
			line(0, i, width, i, bg_color); // 先使用背景色填充
			line(x, i, x + arr[i], i, get_gradient_color(color, i)); // 绘制一行
		}
		msleep(100);
	}
}
```

这里抽象了duration，可以理解为单位时间。最关键的是interpolator，在Android里叫做插值器，波浪动画就是把插值器设置为一个sin函数，这样线条就会周期性地做简谐运动了。

示例图上除了波浪效果，颜色也会变，其实也是使用的同样的思路，可以看到这种动画思路是比较通用的。最后把源代码放上来：

```C
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
		base_color = merge_color(finish_colors[(b + 1) % sort_methods_size], finish_colors[b], color_percent);
		for(i = 0; i < line_cnt; i++){
			// make lines move around the middle point periodly
			// the delay makes the trinkle effect
			delay = i * delay_unit;
			if(i & 1){
				delay += delay_unit * 8;
			}
			move = (unit - arrs[b][i]) / 2;
			move_percent = max(0, curr - start - delay) / (float)exit_anim_duration;
			if(move < 0) move = 0;
			if(move > unit) move = unit;
			margin_left = (int)(move * move_interpolator(move_percent));
			
			color_line_with_margin(b, i, margin_left, intense(b, i, base_color));
			if(i % (line_cnt / 2) == 0) 
				msleep(10);
		}
		msleep(100);
	}
}
```


**3. 高效绘制**


### Configuration

