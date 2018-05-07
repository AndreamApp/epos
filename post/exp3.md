# 实验三 - 线程优先级调度

考虑一个单核CPU上运行的多线程操作系统，多线程能够使得CPU得到最大化的利用。

### 多线程是如何使CPU得到最大化利用的呢？

答案是：在线程不需要CPU的时候打断它的执行，切换到另一个线程继续执行。只要还有Ready的线程，就不要让CPU闲着。

### 线程什么时候会被打断呢？

1. 发起IO请求（running -> waiting）

2. 等待某个事件(wait event, eg. task_wait(), sleep(), sem_wait())（runnning -> waiting）

3. 线程终止（running -> terminated）

4. CPU收到中断（running -> ready）

前面三种情况打断线程执行是理所当然的，因为线程已经无事可做，只有等到事件响应把它拉回ready状态才能再执行。

第四种情况，比如IO完成、某个事件发生、timer都会发起中断，中断服务例程里可以决定是否打断该线程（eg. 在epos里设置g_resched为1）。

只在前三种情况下调度的算法称为非抢占式的算法（Nonpreemptive），在第四种情况也强行调度的称为抢占式算法（Preemptive）。

### 选择哪一个线程作为下一次执行的线程？

我们需要一个调度策略来分配有限的CPU资源。下面简单总结了书上提到的四种调度算法：

**1. 排队，先到先执行（First Come First Serve）**

实现很简单，ready的线程插入链表尾部，只调度链表头部的线程执行。

平均等待时间较长。还可能造成护送效应（Convoy effect），即许多短时间的线程等待一个长时间的线程执行。

非抢占式，不会被中断打断。

**2. 执行时间最短的先执行（Shortest Job First）**

可以达到理论上平均等待时间最短。但很难实现，因为没办法在极短的时间里知道线程执行所需的时间。

可以实现为抢占式的或非抢占式的。抢占式的就是在新的线程加入ready队列后，重新评估最短剩余时间，也称为Shortest-Remaining-Time-First调度。

**3. 优先级高的先执行（Priority）**

FCFS和SJF是优先级调度的特殊情况。

FCFS可以看做优先级相等的调度，SJF可以看做以执行时间做优先级的调度。

主要的问题是容易造成无限等待（饥饿），一个办法是让高优先级的线程逐渐老化（aging）。

同样可以实现为抢占式的或非抢占式的。

（实验三就是实现这个调度算法）

**4. 依次轮转执行（Round-Robin）**

常用于分时系统，保证每个线程能得到较均匀的执行时间。

通过设置时间片，当线程时间片用完后强行调度。所以是抢占式的。

时间片的粒度决定了轮转调度的性能。时间片越大，则等待时间越长，上下文切换消耗低；如果太小，会导致频繁的上下文切换。通常设置时间片能执行完80%的CPU Burst。


## 实验步骤

### 一、完成系统调用

给struct tcb添加三个属性：

```C
struct tcb{
	//...
	int nice;
	fixedpt estcpu;
	fixedpt priority;
	//...
}
```

注意导入头文件fixedptc.h

然后完成setpriority和getpriority两个系统调用，和实验一一样，不再赘述。

完成后建议在main.c里测试一下，保证可以正常使用这两个系统调用。

> 坑：
找不到get_task，因为get_task是static的，不能在task.c外部调用。
可以把static改为extern，或者把sys_setpriority和sys_getpriority的实现函数写到task.c里。

### 二、计算所需参数

因为需要实现优先级“老化”的效果，随着时间要使高优先级逐渐降低，需要在定时中断例程isr_timer里计算一些参数。

nready: 表示当前处于ready状态的线程的个数。

g_load_avg: 表示系统当前的负荷，正相关于nready。

tsk->estcpu: 估计该线程最近占用cpu的程度（Estimated CPU），正相关于tsk->nice。

其中g_load_avg是全局变量，nready是局部变量。都是为了计算tsk->estcpu做准备。

每次定时器中断时当前运行的线程g_task_running->estcpu+1，每隔一秒为所有线程重新估计一次tsk->estcpu。

伪码如下：

```C
void isr_timer(){
	if(g_task_running != NULL){
		if(g_task_running->tid == 0){
			// do nothing
		}
		else{
			g_task_running->estcpu++;
			
			if(g_timer_ticks % HZ == 0){ // one second
				// 计算nready
				int nready = 0;
				遍历线程链表 {
					if(READY){
						nready++;
					}
				}
				// 计算g_load_avg
				g_load_avg = (59/60)*g_load_avg+(1/60)*nready
				
				// 重新估计所有estcpu
				遍历线程链表 {
					tsk->estcpu = (2*g_load_avg)/(2*g_load_avg+1) * tsk->estcpu + tsk->nice;
				}
			}
		}
	}
}
```

上面的公式都是写的浮点数，为了方便理解。实际为了效率要换成定点数运算。

### 三、优先级调度

在task.c里已经实现了一个schedule函数，使用的是Round Robin轮转调度。把这个调度算法看懂，可以帮助你理解g_task_head的结构。

它是一个单向线程链表，保存了所有线程（ready, running, waiting），也包含task0。

（看懂之后可以把这个函数注释，然后重新编写schedule()。当出现问题的时候可以恢复注释排除调度器的问题。）

在步骤一中实现了系统调用，用户可以设置线程的nice。

在步骤二中已经为每个线程计算了estcpu。

现在，根据这两个值，我们可以为每个ready的线程计算priority了。

公式如下：

priority = PRI_USER_MAX-(estcpu/4)-(nice*2)

（反相关于nice，反相关于estcpu）

代码并不复杂，需要注意两个点：
1. 调度的是优先级最高的且**READY**的线程
2. 只有当没有其他可运行线程时，才调度task0

如果出现运行之后系统没有反应，可以参考下面的解决办法：
1. 排除调度器的问题（恢复schedule的注释）
2. 如果是调度器的问题，检查一下上面两个点
3. 检查有没有考虑g_task_running为空的情况
4. 检查步骤二有无错误

如果出现ILLEGEL MEMORY ACCESS，查找eip的值确定出错时执行的函数，缩小问题范围。

### 四、键盘控制优先级

完成了步骤三之后，在main.c中创建两个冒泡排序线程（跟实验二一样），然后设置不同的静态优先级，如果nice比较小的线程先排序完，就可以初步验证调度算法OK了。

然后还需要使用键盘控制优先级，在屏幕上画出进度条。

代码比较简单，需要注意两个地方：
1. int key = getchar()，key是int类型，不能是char类型。
2. 把两个排序线程的tid定义为全局变量，这样就可以在控制线程里访问。（如果你有别的方法也可以）
