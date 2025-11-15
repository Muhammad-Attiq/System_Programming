#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;
    char write_buf[] = "Hello linux system ubuntu\n";
    char read_buf[100];
    
    fd = open("testfile.txt", O_CREAT | O_RDWR, 0644);
    if(fd < 0)
    {
        perror("open");
        return 1;
    }
    
    write(fd, write_buf, sizeof(write_buf));
    
    lseek(fd, 0, SEEK_SET);
    
    read(fd, read_buf, sizeof(read_buf));
    
    printf("File content: %s\n", read_buf);
    
    close(fd);
    
    return 0;
}
