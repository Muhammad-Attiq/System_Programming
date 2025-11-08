#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main()
{
	int choice;
	printf("enter your choice: \n");
	printf("1. execl() \n");
	printf("2. execlp() \n");
	printf("3. execv() \n");
	printf("4. execvp() \n");
	scanf("%d", &choice);
	
	switch(choice)
	{
	  case 1:
  		printf("using execl() -> running /bin/ls -l");
  		execl("/bin/ls", "ls", "-l", NULL);
  		break;
	  case 2:
  		printf("using execlp() -> running ls -l (uses path)");
  		execlp("ls", "ls", "-l", NULL);
  		break;
	  case 3:
  		printf("using execv() -> running /bin/ls -l");
  		char *args_v[] = {"ls", "-l", NULL};
  		execv("/bin/ls", args_v);
  		break;
	  case 4:
  		printf("using execvp() -> running ls -l (uses path)");
  		char *args_vp[] = {"ls", "-l", NULL};
  		execvp("ls", args_vp);
  		break;
	  default: 
		  printf("invalid operation");
	}
	perror("exec failed");
	return 1;
}
