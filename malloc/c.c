#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
void *handle = NULL;
int (*func)(int, int);
void thread1()
{
	while(1){
	sleep(15);
	printf("reload!\n");
	dlclose(handle);
	malloc(123);
	handle = dlopen("./libtest.so", RTLD_NOW|RTLD_GLOBAL);
	if(NULL == handle)
	{
		printf("dlopen failed!\n");
		exit(-1);
	}
	func = dlsym(handle, "fun");

	char *dlsym_error = dlerror();
	if(dlsym_error)
	{
		printf("dlsym_error:%s\n", dlsym_error);
		dlclose(handle);
		exit(-1);
	}
	}
}
int main(int argc, char *argv[])
{
	int a = 2;
	int b = 3;
	pthread_t thid;
	pthread_create(&thid, NULL, (void*)thread1, NULL);
	
	handle = dlopen("./libtest.so", RTLD_NOW|RTLD_GLOBAL);
	if(NULL == handle)
	{
		printf("dlopen failed!\n");
		return -1;
	}
	func = dlsym(handle, "fun");

	char *dlsym_error = dlerror();
	if(dlsym_error)
	{
		printf("dlsym_error:%s\n", dlsym_error);
		dlclose(handle);
		return -1;
	}
	printf("fun = %d\n", (*func)(a, b));

	while(1)
	{
			printf("fun = %d\n", (*func)(a, b));
			sleep(20);
	}
	dlclose(handle);
	return 0;
}
