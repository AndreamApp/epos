/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#include <stddef.h>
#include <string.h>
#include "kernel.h"

int g_resched;
struct tcb *g_task_head;
struct tcb *g_task_running;
struct tcb *task0;
struct tcb *g_task_own_fpu;

/**
 * CPU调度器函数，这里只实现了轮转调度算法
 *
 * 注意：该函数的执行不能被中断
 */
#define KEEP_RUNNING_THRESH 5
int keep_running_start_ticks = 0;
int keep_running_curr_ticks = 0;
void schedule(){
    struct tcb *tsk = g_task_head;
    struct tcb *select = NULL;
	while(tsk != NULL){
		tsk->priority = PRI_USER_MAX - 
                 //fixedpt_toint(fixedpt_div(tsk->estcpu, fixedpt_fromint(4))) - 
                 fixedpt_toint(tsk->estcpu) / 4 - 
                 (tsk->nice)*2;
		// 选择除task0和running之外的优先级最高的线程，若没有，则select为空
		if(tsk->tid != 0 && tsk != g_task_running && tsk->state == TASK_STATE_READY && (select == NULL || tsk->priority > select->priority)){
			select = tsk;
		}
		tsk = tsk->next;
	}
	
	keep_running_curr_ticks = g_task_running->ticks;
	int keep_running = 0;
	
	if(select == NULL){
		if(g_task_running->state == TASK_STATE_READY){
			keep_running = 1;
		}
		else{
			select = task0;
		}
	}
	else{
		if(g_task_running->state == TASK_STATE_READY){
			// 不允许一个线程执行时间超过KEEP_RUNNING_THRESH
			if((keep_running_curr_ticks - keep_running_start_ticks) >= KEEP_RUNNING_THRESH){
				keep_running = 0;
			}
			else if(select->priority < g_task_running->priority){
				keep_running = 1;
			}
		}
	}
	
	if(keep_running){
		return;
	}
	else{
		keep_running_start_ticks = select->ticks;
		keep_running_curr_ticks = select->ticks;
	}
	
	/*
	if(g_task_running != select && select->tid == 4){
		printk("0x%d -> 0x%d\r\n", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);
		printk("  prio: %d -> %d", g_task_running->priority, select->priority);
		printk("  nice: %d -> %d\r\n", g_task_running->nice, select->nice);
		printk("  est: %d -> %d", g_task_running->estcpu, select->estcpu);
		printk("  ticks: %d -> %d\r\n", g_task_running->ticks, select->ticks);
	}
	*/

    if(select->signature != TASK_SIGNATURE)
        printk("warning: kernel stack of task #%d overflow!!!", select->tid);

    g_resched = 0;
    switch_to(select);
}
 
void schedule_round_robin()
{
    struct tcb *select = g_task_running;
    do {
        select = select->next;
        if(select == NULL)
            select = g_task_head;
        if(select == g_task_running) // 找不到ready线程
            break;
        if((select->tid != 0) &&
           (select->state == TASK_STATE_READY)) // 找到running之后的第一个ready线程
            break;
    } while(1);

    if(select == g_task_running) {
        if(select->state == TASK_STATE_READY) // 只有一个ready线程，且正在执行，无需调度
            return;
        select = task0; // 没有ready线程，执行task0
    }

    //printk("0x%d -> 0x%d\r\n", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);

    if(select->signature != TASK_SIGNATURE)
        printk("warning: kernel stack of task #%d overflow!!!", select->tid);

    g_resched = 0;
    switch_to(select);
}

/**
 * 把当前线程切换为等待状态，等待在*head队列中
 *
 * 注意：该函数的执行不能被中断
 */
void sleep_on(struct wait_queue **head)
{
    struct wait_queue wait;

    wait.tsk = g_task_running;
    wait.next = *head;
    *head = &wait;

    g_task_running->state = TASK_STATE_WAITING;
    schedule();

    if(*head == &wait)
        *head = wait.next;
    else {
        struct wait_queue *p, *q;
        p = *head;
        do {
            q = p;
            p = p->next;
            if(p == &wait) {
                q->next = p->next;
                break;
            }
        } while(p != NULL);
    }
}

/**
 * 唤醒n个等待在*head队列中的线程。
 * 如果n<0，唤醒队列中的所有线程
 *
 * 注意：该函数的执行不能被中断
 */
void wake_up(struct wait_queue **head, int n)
{
    struct wait_queue *p;

    for(p = *head; (p!=NULL) && n; p = p->next, n--)
        p->tsk->state = TASK_STATE_READY;
}

static
void add_task(struct tcb *tsk)
{
    if(g_task_head == NULL)
        g_task_head = tsk;
    else {
        struct tcb *p, *q;
        p = g_task_head;
        do {
            q = p;
            p = p->next;
        } while(p != NULL);
        q->next = tsk;
    }
}

static
void remove_task(struct tcb *tsk)
{
    if(g_task_head != NULL) {
        if(tsk == g_task_head) {
            g_task_head = g_task_head->next;
        } else {
            struct tcb *p, *q;
            p = g_task_head;
            do {
                q = p;
                p = p->next;
                if(p == tsk)
                    break;
            } while(p != NULL);

            if(p == tsk)
                q->next = p->next;
        }
    }
}

static
struct tcb* get_task(int tid)
{
    struct tcb *tsk;

    tsk = g_task_head;
    while(tsk != NULL) {
        if(tsk->tid == tid)
            break;
        tsk = tsk->next;
    }

    return tsk;
}

/**
 * 系统调用task_create的执行函数
 *
 * 创建一个新的线程，该线程执行func函数，并向新线程传递参数pv
 */
struct tcb *sys_task_create(void *tos,
                            void (*func)(void *pv), void *pv)
{
    static int tid = 0;
    struct tcb *new;
    char *p;
    uint32_t flags;
    uint32_t ustack=(uint32_t)tos;

    if(ustack & 3)
        return NULL;

    p = (char *)kmemalign(PAGE_SIZE, PAGE_SIZE);
    if(p == NULL)
        return NULL;

    new = (struct tcb *)p;

    memset(new, 0, sizeof(struct tcb));

    new->kstack = (uint32_t)(p+PAGE_SIZE);
    new->tid = tid++;
    new->state = TASK_STATE_READY;
    new->timeslice = TASK_TIMESLICE_DEFAULT;
    new->wq_exit = NULL;
    new->next = NULL;
	new->nice = 20;
	new->priority = 0;
	new->estcpu = fixedpt_fromint(0);
	new->ticks = 0;
    new->signature = TASK_SIGNATURE;

    /*XXX - should be elsewhere*/
    new->fpu.cwd = 0x37f;
    new->fpu.twd = 0xffff;

    INIT_TASK_CONTEXT(ustack, new->kstack, func, pv);

    save_flags_cli(flags);
    add_task(new);
    restore_flags(flags);

    return new;
}

/**
 * 系统调用task_exit的执行函数
 *
 * 结束当前线程，code_exit是它的退出代码
 */
void sys_task_exit(int code_exit)
{
    uint32_t flags;

    save_flags_cli(flags);

    wake_up(&g_task_running->wq_exit, -1);

    g_task_running->code_exit = code_exit;
    g_task_running->state = TASK_STATE_ZOMBIE;

    if(g_task_own_fpu == g_task_running)
        g_task_own_fpu = NULL;

    schedule();
}

/**
 * 系统调用task_wait的执行函数
 *
 * 当前线程等待线程tid结束执行。
 * 如果pcode_exit不是NULL，用于保存线程tid的退出代码
 */
int sys_task_wait(int tid, int *pcode_exit)
{
    uint32_t flags;
    struct tcb *tsk;

    if(g_task_running == NULL)
        return -1;

	if(tid == 0)
		return -1;

    save_flags_cli(flags);

    if((tsk = get_task(tid)) == NULL) {
        restore_flags(flags);
        return -1;
    }

    if(tsk->state != TASK_STATE_ZOMBIE)
        sleep_on(&tsk->wq_exit);

    if(pcode_exit != NULL)
        *pcode_exit= tsk->code_exit;

    if(tsk->wq_exit == NULL) {
        remove_task(tsk);
        //printk("%d: Task %d reaped\r\n", sys_task_getid(), tsk->tid);
        restore_flags(flags);

        kfree(tsk);
        return 0;
    }

    restore_flags(flags);
    return 0;
}

/**
 * 系统调用task_getid的执行函数
 *
 * 获取当前线程的tid
 */
int sys_task_getid()
{
    return (g_task_running==NULL)?-1:g_task_running->tid;
}

/**
 * 系统调用task_yield的执行函数
 *
 * 当前线程主动放弃CPU，让调度器调度其他线程运行
 */
void sys_task_yield()
{
    uint32_t flags;
    save_flags_cli(flags);
    schedule();
    restore_flags(flags);
}

int sys_get_priority(int tid){
	disable_irq(0);
	struct tcb * tsk;
	uint32_t flags;
	save_flags_cli(flags);
	if(tid == 0){
		tsk = g_task_running;
	}
	else{
		tsk = get_task(tid);
	}
	restore_flags(flags);
	
	int prio = -1;
	if(tsk != NULL){
		prio = tsk->nice + NZERO;
	}
	enable_irq(0);
	return prio;
}

int sys_set_priority(int tid, int prio){
	disable_irq(0);
	struct tcb * tsk;
	uint32_t flags;
	save_flags_cli(flags);
	if(tid == 0){
		tsk = g_task_running;
	}
	else{
		tsk = get_task(tid);
	}
	restore_flags(flags);
	
	enable_irq(0);
	if(tsk == NULL){
		return -1;
	}
	else{
		tsk->nice = prio - NZERO;
		return 0;
	}
}

/**
 * 初始化多线程子系统
 */
void init_task()
{
    g_resched = 0;
    g_task_running = NULL;
    g_task_head = NULL;
    g_task_own_fpu = NULL;

    /*
     * 创建线程task0，即系统空闲线程
     */
    task0 = sys_task_create(NULL, NULL/*task0执行的函数将由run_as_task0填充*/, NULL);
}
