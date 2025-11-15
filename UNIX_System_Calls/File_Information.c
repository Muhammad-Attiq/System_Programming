#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    struct stat info;
    
    if(argc != 2)
    {
        printf("Usage: ./finfo <filename>\\n");
        return 1;
    }
    
    if(stat(argv[1], &info) == -1)
    {
        perror("stat");
        return 1;
    }
    
    printf("File: %s\\n", argv[1]);
    printf("Size: %ld bytes\\n", info.st_size);
    printf("Permissions: %o\\n", info.st_mode & 0777);
    
    if(S_ISREG(info.st_mode))
    {
        printf("Type: regular file \\n");
    }
    else if(S_ISDIR(info.st_mode))
    {
        printf("Type: directory \\n");
    }
    else if(S_ISLNK(info.st_mode))
    {
        printf("Type: symbolic link\\n");
    }
    else
    {
        printf("Type: other\\n");
    }
    return 0;
}
