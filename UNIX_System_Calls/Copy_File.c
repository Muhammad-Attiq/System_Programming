#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int src, dest;
    char buf[1024];
    ssize_t n;
    
    src = open("source.txt", O_RDONLY);
    if(src < 0)
    {
        perror("open source");
        return 1;
    }
    
    dest = open("copy.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(dest < 0)
    {
        perror("open dest");
        close(src);
        return 1;
    }
    
    while((n = read(src, buf, sizeof(buf))) > 0)
    {
        write(dest, buf, n);
    }
    
    return 0;
}
