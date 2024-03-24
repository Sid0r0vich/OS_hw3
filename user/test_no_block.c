

int main() {
	int p[2];
	int res = pipe(p);
	if (res < 0) {
		fprintf("pipe error!\n");
		exit(1);
	}

	int pid = fork();
	if (pid > 0) {
		
	} else if (pid == 0) {
		
	} else {
		fprintf(2, "fork error!\n");
		exit(1);
	}
}
