#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>

#define MAX_LINE_LENGTH 256
#define PROC_PATH "/proc"

void print_usage(const char *program_name) {
    printf("Usage: %s <PID>\n", program_name);
    printf("Displays process status information from /proc/[PID]/status\n");
}

int read_proc_status(const char *pid) {
    char filepath[512];
    FILE *file;
    char line[MAX_LINE_LENGTH]; 
    int found_name = 0, found_state = 0, found_vmrss = 0, found_vmsize = 0;
    int found_ppid = 0, found_threads = 0;
    snprintf(filepath, sizeof(filepath), "%s/%s/status", PROC_PATH, pid);
    file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Cannot open %s - %s\n", filepath, strerror(errno));
        return -1;
    }
    printf("Process Status for PID: %s\n", pid);
    printf("==============================\n");
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "Name:", 5) == 0) {
            printf("Process Name: %s", line + 6); 
            found_name = 1;
        }
        else if (strncmp(line, "State:", 6) == 0) {
            printf("State: %s", line + 7); 
            found_state = 1;
        }
        else if (strncmp(line, "VmSize:", 7) == 0) {
            printf("Virtual Memory Size: %s", line + 8); 
            found_vmsize = 1;
        }
        else if (strncmp(line, "VmRSS:", 6) == 0) {
            printf("Resident Set Size: %s", line + 7); 
            found_vmrss = 1;
        }
        else if (strncmp(line, "PPid:", 5) == 0) {
            printf("Parent PID: %s", line + 6); 
            found_ppid = 1;
        }
        else if (strncmp(line, "Threads:", 8) == 0) {
            printf("Number of Threads: %s", line + 9); 
            found_threads = 1;
        }
        if (found_name && found_state && found_vmrss && found_vmsize && found_ppid && found_threads) {
            break;
        }
    }

    fclose(file);
    
    if (!found_name) {
        printf("Process Name: Not found\n");
    }
    if (!found_state) {
        printf("State: Not found\n");
    }
    if (!found_vmsize) {
        printf("Virtual Memory Size: Not found\n");
    }
    if (!found_vmrss) {
        printf("Resident Set Size: Not found\n");
    }
    if (!found_ppid) {
        printf("Parent PID: Not found\n");
    }
    if (!found_threads) {
        printf("Number of Threads: Not found\n");
    }
    
    return 0;
}

int is_valid_pid(const char *pid_str) {
    for (int i = 0; pid_str[i] != '\0'; i++) {
        if (pid_str[i] < '0' || pid_str[i] > '9') {
            return 0;
        }
    }

    if (atoi(pid_str) == 0) {
        return 0;
    }
    
    return 1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments\n");
        print_usage(argv[0]);
        return 1;
    }
    const char *pid = argv[1];
    if (!is_valid_pid(pid)) {
        fprintf(stderr, "Error: Invalid PID format '%s'. PID must be a positive integer.\n", pid);
        return 1;
    }
    char proc_dir[512];
    snprintf(proc_dir, sizeof(proc_dir), "%s/%s", PROC_PATH, pid);
     if (access(proc_dir, F_OK) != 0) {
        fprintf(stderr, "Error: Process with PID %s does not exist or cannot be accessed\n", pid);
        return 1;
    }
    if (read_proc_status(pid) != 0) {
        return 1;
    }
    
    return 0;
}
