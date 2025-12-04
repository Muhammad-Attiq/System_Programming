#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define MAX 1024

void to_uppercase(char *str) {
for (int i = 0; str[i]; i++)
str[i] = toupper((unsigned char)str[i]);
}
void reverse_string(char *str) {
int len = strlen(str);
for (int i = 0; i < len/2; i++) {
char temp = str[i];
str[i] = str[len-i-1];
str[len-i-1] = temp;
}
}
int main() {
int pipe1[2], pipe2[2];
if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
perror("pipe");
exit(1);
}
pid_t p1 = fork();
if (p1 < 0) { perror("fork"); exit(1); }

if (p1 == 0) { // Process P1

    close(pipe1[0]); // Close read end
    char line[MAX];
    while (1) {
        printf("Enter a string: ");
        if (!fgets(line, MAX, stdin)) break;
        line[strcspn(line, "\n")] = 0; // Remove newline
        if (strlen(line) == 0) break;
        write(pipe1[1], line, strlen(line)+1);
    }
    close(pipe1[1]);
    exit(0);
}

pid_t p2 = fork();
if (p2 < 0) { perror("fork"); exit(1); }

if (p2 == 0) { // Process P2
    close(pipe1[1]); // Close write end of pipe1
    close(pipe2[0]); // Close read end of pipe2
    char buffer[MAX];
    while (read(pipe1[0], buffer, MAX) > 0) {
        to_uppercase(buffer);
        write(pipe2[1], buffer, strlen(buffer)+1);
    }
    close(pipe1[0]);
    close(pipe2[1]);
    exit(0);
}

// Process P3 (Parent)
close(pipe1[0]);
close(pipe1[1]);
close(pipe2[1]); // Close write end
char result[MAX];
while (read(pipe2[0], result, MAX) > 0) {
    reverse_string(result);
    printf("Processed string: %s\n", result);
}
close(pipe2[0]);

wait(NULL); // Wait for P1
wait(NULL); // Wait for P2
return 0;
}
