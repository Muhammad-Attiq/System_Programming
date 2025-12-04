#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    int pipe1[2];
    int pipe2[2];
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        while (1) {
            char buffer[50];
            int bytes = read(pipe1[0], buffer, sizeof(buffer));
            buffer[bytes] = '\0';

            if (buffer[0] == 'q') break;

            double a, b, result;
            char op;
            sscanf(buffer, "%lf %c %lf", &a, &op, &b);


            switch (op) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/':
                    if (b == 0) {
                        write(pipe2[1], "Error: Divide by zero", 22);
                        continue;
                    }
                    result = a / b;
                    break;
                default:
                    write(pipe2[1], "Error: Invalid operator", 23);
                    continue;
            }

            char out[50];
            sprintf(out, "%lf", result);
            write(pipe2[1], out, strlen(out) + 1);
        }

        close(pipe1[0]);
        close(pipe2[1]);
    }

    else {
        close(pipe1[0]);
        close(pipe2[1]);

        while (1) {
            char input[50];
            printf("\nEnter: number operator number (or q to quit): ");
            fgets(input, sizeof(input), stdin);

            if (input[0] == 'q') {
                write(pipe1[1], "q", 2);
                break;
            }

            write(pipe1[1], input, strlen(input) + 1);

            char result[50];
            int bytes = read(pipe2[0], result, sizeof(result));
            result[bytes] = '\0';

            printf("Result: %s\n", result);
        }

        close(pipe1[1]);
        close(pipe2[0]);
    }

    return 0;
}
