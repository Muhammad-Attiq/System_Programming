#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main()
{
	FILE *fp;
	int cpu_count=0;
	char line[256];
	char model[256] = "";
	long free_memory, total_memory;
	double load1, load5, load15;
	
	fp = fopen("/proc/cpuinfo", "r");
	if(fp == NULL) return 1;

	while(fgets(line, sizeof(line), fp))
	{
		if(strncmp(line, "processor", 9) == 0)
		{
			cpu_count++;
		}
		else if(strncmp(line, "model name", 10) == 0 && model[0] == '\0')
		{
			sscanf(line, "model name: %[^\\n]", model); 
		}
	}
	fclose(fp);

	fp = fopen("/proc/meminfo", "r");
	if(fp == NULL) return 1;
	while(fgets(line, sizeof(line), fp))
	{
		sscanf(line, "MemTotal: %ld kB", &total_memory);
		sscanf(line, "MemFree: %ld kB", &free_memory);
	}
	fclose(fp);

	fp = fopen("/proc/loadavg", "r");
	if(fp == NULL) return 1;
	fscanf(fp, "%lf %lf %lf", &load1, &load5, &load15);
	fclose(fp);

	printf("CPUs: %d\n", cpu_count);
	printf("CPU Model: %s\n", model);
    printf("Total Memory: %ld kB\n", total_memory);
    printf("Free Memory: %ld kB\n", free_memory);
    printf("Load Average: %.2f %.2f %.2f\n", load1, load5, load15);

	return 0;
}
