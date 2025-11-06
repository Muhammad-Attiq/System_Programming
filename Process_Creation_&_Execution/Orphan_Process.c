#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main()
{
	pid_t pid = fork();
	if (pid > 0)
	{
		printf("I AM PARENT PROCESS HERE\n");
		printf("PARENT PROCESS PID: %d\n", getpid());
		exit(0);
	}
	else if(pid == 0)
	{
		sleep(5);
		printf("I AM CHILD PROCESS\n");
		printf("CHILD PROCESS PID: %d, PARENT PROCESS PID: %d", getpid(), getppid());
	}
	else
	{
		printf("FORK FAILED");
		return 1;
	}
	return 0;
}
