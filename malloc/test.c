#include <stdio.h>
int fun(int a, int b)
{
		printf("fun in!\n");
		sleep(10);
		printf("fun out!\n");
		return a + b;
}
