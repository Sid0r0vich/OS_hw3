#include "kernel/types.h"
#include "user.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/mutex.h"

int main(int argc, char* argv[]) {
	if (argc <= 1) exit(1);
	
	int p1[2];
	int p2[2];
	int res1 = pipe(p1);
	int res2 = pipe(p2);
	if (res1 < 0 || res2 < 0) {
		fprintf(2, "pipe error!\n");
		exit(1);
	}

	int md = cmutex();
	if (md < 0) {
		fprintf(2, "mutex error!\n");
		exit(1);
	}
	
	int pid = fork();
	if (pid > 0) {
		close(p1[0]);
		close(p2[1]);

		int res = write(p1[1], argv[1], strlen(argv[1]));
		if (res < 0) {
			fprintf(2, "write error!");
			exit(1);
		}
		close(p1[1]);

		char c;
		while (1) {
			res = read(p2[0], &c, 1);
			if (res <= 0) break;

			lock(md);
			printf("%d: recieved %c\n", getpid(), c);
			unlock(md);
		}
		close(p2[0]);

		if (res < 0) {
			fprintf(2, "read error!");
		}

		wait((int*)0);
	} 
	else if (pid == 0) {
		close(p1[1]);
		close(p2[0]);

		char c;
		int res;
		while (1) {
			res = read(p1[0], &c, 1);
			if (res <= 0) break;

			lock(md);
			printf("%d: recieved %c\n", getpid(), c);
			unlock(md);

			res = write(p2[1], &c, 1);
			if (res < 0) {
				fprintf(2, "write error!");
				exit(1);
			}
		}
		close(p1[0]);
		close(p2[1]);

		if (res < 0) {
			fprintf(2, "read error!");
		}
	} 
	else {
		fprintf(2, "fork error!\n");
		exit(1);
	}

	return 0;
}
