#include <unistd.h>
#include "old.h"
#include <stdio.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/user.h>
#include <dlfcn.h>
int main(){
//	unsigned long Loader_dlopen = 0;
//	Loader_dlopen = (unsigned long)dlsym(NULL, "malloc");
	//fprintf(stderr, "Failed to ptrace_attach: %s\n", strerror(errno));
//	printf("Loader_dlopen=%lx\n", Loader_dlopen);
	for(;;){
		print();
//		((void*(*)(size_t))Loader_dlopen)(123);	
		sleep(1);
	}
}
