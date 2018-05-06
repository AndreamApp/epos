/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include "kernel.h"

#define SEM_MAX_SIZE 100

struct semaphore{
	int id;
	int value;
	struct wait_queue * wq;
};

struct semaphore sems[SEM_MAX_SIZE];

int sys_sem_create(int value)
{
	int i;
	for(i = 1; i < SEM_MAX_SIZE; i++){
		if(sems[i].id == 0){
			break;
		}
	}
	if(i == SEM_MAX_SIZE){
		return -1;
	}
	else{
		sems[i].id = i;
		sems[i].value = value;
		sems[i].wq = NULL;
		return sems[i].id;
	}
}

int sys_sem_destroy(int semid)
{
	if(semid < 1 || semid >= SEM_MAX_SIZE || sems[semid].id != semid){
		return -1;
	}
	sems[semid].id = 0;
	sems[semid].value = 0;
	sems[semid].wq = NULL;
	return 0;
}

int sys_sem_wait(int semid)
{
	if(semid < 1 || semid >= SEM_MAX_SIZE || sems[semid].id != semid){
		return -1;
	}
	uint32_t flags;
	save_flags_cli(flags);
	
	sems[semid].value--;
	if(sems[semid].value < 0){
		sleep_on(&sems[semid].wq);
	}
	
	restore_flags(flags);
    return 0;
}

int sys_sem_signal(int semid)
{
	if(semid < 1 || semid >= SEM_MAX_SIZE || sems[semid].id != semid){
		return -1;
	}
	uint32_t flags;
	save_flags_cli(flags);
	
	sems[semid].value++;
	if(sems[semid].value <= 0){
		wake_up(&sems[semid].wq, 1);
	}
	
	restore_flags(flags);
    return 0;
}

