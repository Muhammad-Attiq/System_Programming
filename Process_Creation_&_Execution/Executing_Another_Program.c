#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

int main()
{
        pid_t pid = fork();
        if(pid == 0)
        {
                printf("CHILD PROCESS EXECUTING EXECLP \n");
                execlp("ls", "ls", "-l", NULL);

                printf("EXECLP NOT EXECUTED");
                exit(1);

        }
        else if(pid > 0)
        {
                int status;
                wait(&status);

                if(WIFEXITED(status))
                {
                        printf("CHILD EXITED WITH STATUS: %d \n", WEXITSTATUS(status));
                }
        }
       else
        {
                printf("FORK FAILED \n");
                return 1;
        }
        return 0;
}
