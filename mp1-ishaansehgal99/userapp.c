#include "userapp.h"
#include <stdio.h> // sprintf()
#include <string.h> // memset()
#include <time.h> // time()
#include <stdlib.h> // system()
#include <unistd.h> // getpid()

int RUN_TIME = 12;

void register_process(){
	char cmd[256];
	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "echo %u > /proc/mp1/status", getpid());
	system(cmd);
}

int factorial(int n) {
	if (n <= 1) return n; 
	return n * factorial(n-1);
}
int fibonacci(int n) {
	if (n <= 1) return n;
	return fibonacci(n - 1) + fibonacci(n - 2);
}

int main(int argc, char* argv[])
{
	time_t start = time(NULL);
	register_process(); 

	while(1) {
		// Occupy the CPU
		int f = factorial(12);
		// printf("Factorial 12 %u \n", f);
		if((int)(time(NULL) - start) > RUN_TIME) break;
	}
	return 0;
}
