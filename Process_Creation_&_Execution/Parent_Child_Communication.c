#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>

int main()
{
        pid_t pid;
        pid = fork();
if(pid == 0)
{
        printf("\n CHILD HERE \n ");
        sleep(2);
}
else if(pid > 0)
{
        printf("\n PARENT HERE \n ");
        wait(NULL);
}
else
{
        printf("FORK FAILED");
        return 1;
}
return 0;
}

