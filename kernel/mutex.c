#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"
#include "mutex.h"

struct mutex mutex[NMUTEX];

void
mutexinit(void) {
	struct mutex* mx;
	
	for (mx = mutex; mx < &mutex[NMUTEX]; ++mx) {
		initlock(&mx->splock, "mspin");
		initsleeplock(&mx->sllock, "msleep");
		mx->nlink = 0;
		mx->pid = 0;
	}
}

uint64
sys_cmutex(void) {
	struct mutex* mx;
	
	for (mx = mutex; mx < &mutex[NMUTEX]; ++mx) {
		acquire(&mx->splock);

		if (!mx->is_used) {
			mx->is_used = 1;
			mx->nlink = 1;
			
			struct proc* p = myproc();
			acquire(&p->lock);
			p->mutex[mx - mutex] = 1;
			release(&p->lock);
			
			release(&mx->splock);
			
			return mx - mutex;
		}
		
		release(&mx->splock);
	}

	return -1;
}

uint64
sys_rmutex(void) {
	int md;
	argint(0, &md);

	return rmutex(md);
}

int
rmutex(int md) {
	acquire(&mutex[md].splock);

	if (mutex[md].locked) {
		release(&mutex[md].splock);
		return -1;
	}
	
	struct proc* p = myproc();
	acquire(&p->lock);
	if (p->mutex[md] == 0) {
		release(&p->lock);
		return -1;
	}
	p->mutex[md] = 0;
	release(&p->lock);

	--mutex[md].nlink;
	
	if (!mutex[md].nlink) mutex[md].is_used = 0;

	release(&mutex[md].splock);
	return 0;
}

uint64
sys_lock(void) {
	int md;
	argint(0, &md);
	
	acquire(&mutex[md].splock);

	if (mutex[md].locked) {
		release(&mutex[md].splock);
		return -1;
	}

	mutex[md].locked = 1;

	struct proc* p = myproc();
	acquire(&p->lock);
	mutex[md].pid = p->pid;
	release(&p->lock);
	
	acquiresleep(&mutex[md].sllock);
	
	release(&mutex[md].splock);
	return 0;
}

uint64
sys_unlock(void) {
	int md;
	argint(0, &md);
	
	acquire(&mutex[md].splock);

	struct proc* p = myproc();
	acquire(&p->lock);
	if (!mutex[md].locked || mutex[md].pid != p->pid) {
		release(&p->lock);
		release(&mutex[md].splock);
		return -1;
	}
	release(&p->lock);

	mutex[md].locked = 0;
	mutex[md].pid = 0;
	releasesleep(&mutex[md].sllock);
	
	release(&mutex[md].splock);
	return 0;
}
