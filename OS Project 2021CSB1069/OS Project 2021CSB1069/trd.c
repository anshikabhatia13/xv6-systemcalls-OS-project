// We cannot include many header files because definitions will conflict
// Including header files included by other programs eg `cat`
#include "types.h"
#include "stat.h"
#include "user.h"
#include "trace.h"


void forkrun() {
	int fr = fork();
	if (fr == -1) {
		printf(1, "Fork error!\n");
		return;
	} else if (fr == 0) {
		close(open("README", 0));
		// exit the child early
		// Or output would be confusing
		exit();
	} else {
		wait();
	}
}

int main() {
	printf(1, "Process is being traced.\n");
	trace(SYSCALL_TRACE);
	forkrun();
	
	trace(SYSCALL_UNTRACE);
	printf(1, "Processs & forks being traced.\n");
	trace(SYSCALL_TRACE | SYSCALL_ONFORK);
	forkrun();

	trace(SYSCALL_UNTRACE);
	printf(1, "Process not being traced.\n");
	forkrun();

	exit();
}
