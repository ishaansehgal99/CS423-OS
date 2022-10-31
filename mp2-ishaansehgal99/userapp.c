#include "userapp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// Constants replaced by runtime parameters
#define PERIOD_SEC 3
#define NUM_JOBS 10

int factorial(int n) {
	if (n <= 1) return n; 
	return n * factorial(n-1);
}
int fibonacci(int n) {
	if (n <= 1) return n;
	return fibonacci(n - 1) + fibonacci(n - 2);
}

void busy_job(){
    int i = 0;
    while(i < 100000000){
        i += 1;
    }
}

void register_process(pid_t pid, char *period, char *run_time){
    FILE *f = fopen("/proc/mp2/status", "w");
    fprintf(f, "R, %d, %s, %s\n", pid, period, run_time);
    fclose(f);
}

void yield(pid_t pid){
    FILE *f = fopen("/proc/mp2/status", "w");
    fprintf(f, "Y, %d\n", pid);
    fclose(f);
}

void deregister_process(pid_t pid){
    FILE *f = fopen("/proc/mp2/status", "w");
    fprintf(f, "D, %d\n", pid);
    fclose(f);
}

int read_status(pid_t pid, char *period, char *run_time){
    FILE *f = fopen("/proc/mp2/status", "r");
    char buf[255]; 
    char tar[255];
    sprintf(tar, "%d: %s, %s\n", pid, period, run_time);
    while(fgets(buf, 255, f)){
        if (!strcmp(buf, tar)){
            fclose(f);
            return 0;
        }

    }
    fclose(f);
    return 1;
}

double calc_time_diff(struct timeval start, struct timeval end){
    double diff = (1000000 * (end.tv_sec - start.tv_sec)) + (end.tv_usec - start.tv_usec); 
    // Convert from micro to milliseconds
    diff *= 0.001; 
    return diff;
}

double get_ms_time(struct timeval start) {
    double time = (1000000 * start.tv_sec + start.tv_usec); 
    // Convert from micro to milliseconds
    time *= 0.001; 
    return time;
}

// Used to calculate an average runtime per job
int get_job_runtime(){
    struct timeval start, end;
    gettimeofday(&start, NULL);
	busy_job();
	busy_job();
	busy_job();
	busy_job();
    busy_job();
    busy_job();
	gettimeofday(&end, NULL);
    return (int)(calc_time_diff(start, end) / 6.0 * (1.2));
}

int main(int argc, char *argv[])
{
    /* Replaced below with command line parsing
     * int period = PERIOD_SEC; 
     * int num_jobs = NUM_JOBS;
    */

    if (argc != 3){
		printf("Incorrect. Correct Usage: ./userapp period(seconds) #jobs\n");
        printf("Ex: ./userapp 3 10\n");
        return 1;
	}

    int period = atoi(argv[1]);
    int num_jobs = atoi(argv[2]);
    int runtime = get_job_runtime();

    char char_runtime[32];
    memset(char_runtime, '\0', sizeof(char_runtime));
    sprintf(char_runtime, "%d", runtime);
    printf("Job Runtime: %sms\n", char_runtime);

    char char_period[32];
    memset(char_period, '\0', sizeof(char_period));
    sprintf(char_period, "%d", period * 1000);
    printf("Job Period: %sms\n", char_period);

    pid_t pid = getpid();
	register_process(pid, char_period, char_runtime);

    if(read_status(pid, char_period, char_runtime)){
        printf("Failed Registration.\n");
        exit(1);
    }
    
    yield(pid);

    struct timeval start, end;
    while(num_jobs-- > 0){
        gettimeofday(&start, NULL);
        printf("Start Time : %fms\n", get_ms_time(start));
		busy_job();
		gettimeofday(&end, NULL);
        printf("End Time : %fms\n", get_ms_time(end));
        printf("Time taken for job is : %dms.\n", (int)calc_time_diff(start, end));

		yield(pid);
        gettimeofday(&end, NULL);
    }

    gettimeofday(&end, NULL);
    printf("End time of the process %ld second\n", end.tv_sec);
    deregister_process(pid);
    return 0;
}

