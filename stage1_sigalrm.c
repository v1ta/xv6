#include "types.h"
#include "stat.h"
#include "user.h"
#include "signal.h"

volatile int flag = 0;

void handle_signal(siginfo_t info)
{
	printf(1, "Caught signal %d...\n", info.signum);
	if (info.signum == SIGALRM)
		printf(1, "TEST PASSED\n");
	else
		printf(1, "TEST FAILED: wrong signal sent.\n");
	exit();
}

int main(int argc, char *argv[])
{
	int start = uptime();

  signal(-1, (uint) trampoline);
	signal(SIGALRM, handle_signal);

	alarm(5);

	while(!flag && uptime() < start + 1000);

	printf(1, "TEST FAILED: no signal sent.\n");

	exit();
}
