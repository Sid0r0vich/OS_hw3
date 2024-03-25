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
			struct proc* p = myproc();
			int i;
			for (i = 0; i < NMUTEXPROC; ++i) {
				acquire(&p->lock);

				if (p->mutex[i] == 0) {
					p->mutex[i] = mx;
					mx->is_used = 1;
					mx->nlink = 1;
					release(&p->lock);
					break;
				}

				release(&p->lock);
			};
			
			release(&mx->splock);

			if (i == NMUTEXPROC) return -1;
			
			return i;
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
	struct proc* p = myproc();
	acquire(&p->lock);
	struct mutex* mx = p->mutex[md];
	release(&p->lock);

	if (mx == 0) {
		return -1;
	}

	acquire(&mx->splock);

	if (mx->locked) {
		release(&mx->splock);
		return -1;
	}

	acquire(&p->lock);
	p->mutex[md] = 0;
	release(&p->lock);

	--mx->nlink;
	
	if (!mx->nlink) mx->is_used = 0;

	release(&mx->splock);
	return 0;
}

uint64
sys_lock(void) {
	int md;
	argint(0, &md);

	struct proc* p = myproc();
	acquire(&p->lock);
	struct mutex* mx = p->mutex[md];
	release(&p->lock);
	
	acquire(&mx->splock);

	mx->locked = 1;

	acquire(&p->lock);
	mx->pid = p->pid;
	release(&p->lock);
	
	release(&mx->splock);
	acquiresleep(&mx->sllock);
	return 0;
}

uint64
sys_unlock(void) {
	int md;
	argint(0, &md);

	struct proc* p = myproc();
	acquire(&p->lock);
	struct mutex* mx = p->mutex[md];
	release(&p->lock);
	
	acquire(&mx->splock);

	mx->locked = 0;
	mx->pid = 0;
	
	release(&mx->splock);
	releasesleep(&mx->sllock);
	return 0;
}
