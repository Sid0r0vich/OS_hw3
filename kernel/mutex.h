
struct mutex {
	uint locked;
	struct spinlock splock;
	struct sleeplock sllock;

	int is_used;
	int pid;
	int nlink;
};
