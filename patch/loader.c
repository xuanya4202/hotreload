#include <sys/ptrace.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/user.h>
#include <dlfcn.h>
/*Take a hint and find start addr in /proc/pid/maps */
static unsigned long find_lib_base(pid_t pid, char *so_hint)
{
		FILE *fp;
		char maps[4096], mapbuf[4096], perms[32], libpath[4096];
		char *libname;
		unsigned long start, end, file_offset, inode, dev_major, dev_minor;

		sprintf(maps, "/proc/%d/maps", pid);
		fp = fopen(maps, "rb");
		if (!fp) {
				fprintf(stderr, "Failed to open %s: %s\n", maps, strerror(errno));
				return 0;
		}

		while (fgets(mapbuf, sizeof(mapbuf), fp)) {
				sscanf(mapbuf, "%lx-%lx %s %lx %lx:%lx %lu %s", &start,
								&end, perms, &file_offset, &dev_major, &dev_minor, &inode, libpath);

				libname = strrchr(libpath, '/');
				if (libname)
						libname++;
				else
						continue;

				if (!strncmp(perms, "r-xp", 4) && strstr(libname, so_hint)) {
						fclose(fp);
						printf("start =%lx\n", start);
						return start;
				}
		}
		fclose(fp); 
		return 0;
}

static int ptrace_attach(pid_t pid)
{
		int status;

		if (ptrace(PTRACE_ATTACH, pid, NULL, NULL)) {
				fprintf(stderr, "Failed to ptrace_attach: %s\n", strerror(errno));
				return 1;
		}

		if (waitpid(pid, &status, __WALL) < 0) {
				fprintf(stderr, "Failed to wait for PID %d, %s\n", pid, strerror(errno));
				return 1;
		}
		return 0;
}

//static int ptrace_call(pid_t pid, unsigned long func_addr, unsigned long arg1, unsigned long arg2, unsigned long *func_ret)
//{
//		memset(&saved_regs, 0, sizeof(struct user_regs_struct));
//		ptrace_getregs(pid, &saved_regs);
//
//
//		unsigned long invalid = 0x0;
//		regs.rsp -= sizeof(invalid);
//		ptrace_poketext(pid, regs.rsp, ((void *)&invalid), sizeof(invalid));
//		ptrace_poketext(pid, regs.rsp + 512, filename, strlen(filename) + 1);
//		regs.rip = dlopen_addr;
//		regs.rdi = regs.rsp + 512;
//		regs.rsi = RTLD_NOW;
//		ptrace_setregs(pid, &regs);
//}
static int ptrace_cont(pid_t pid)
{		
		int status;

		if (ptrace(PTRACE_CONT, pid, NULL, 0)) {

				fprintf(stderr, "Failed to ptrace_cont: %s\n", strerror(errno));return 1;

		}

		if (waitpid(pid, &status, __WALL) < 0) {fprintf(stderr, "Failed to wait for PID %d, %s\n", pid, strerror(errno));

				return 1;}
		return 0;
}

static int ptrace_getregs(pid_t pid, struct user_regs_struct *data)
{		
		if (ptrace(PTRACE_GETREGS, pid, 0, data)) {

				fprintf(stderr, "Failed to ptrace_getregs: %s\n", strerror(errno));
				return 1;
		}

		return 0;
}
static int ptrace_setregs(pid_t pid, struct user_regs_struct *data)
{		
		if (ptrace(PTRACE_SETREGS, pid, 0, data)) {

				fprintf(stderr, "Failed to ptrace_setregs: %s\n", strerror(errno));
				return 1;
		}

		return 0;
}
static int ptrace_detach(pid_t pid)
{
		ptrace(PTRACE_DETACH, pid);
		//if (ptrace(PTRACE_DETACH, pid)) {
//
//				fprintf(stderr, "Failed to ptrace_detach: %s\n", strerror(errno));
//				return 1;
//		}

		return 0;
}
//static ptrace_poketext(pid_t pid, unsigned long addr, unsigned char data[], int len)
//{
//    int i = 0;
//	printf("data=%s\n", data);
//	for(i = 0; i < len; i++)
//	{
////		ptrace(PTRACE_PEEKTEXT, pid, addr, data[i]);
////		fprintf(stderr, "Failed to ptrace_peektext: %s\n", strerror(errno));
//		if(ptrace(PTRACE_POKETEXT, pid, addr+1, data[i])){
//				fprintf(stderr, "Failed to ptrace_peektext: %s\n", strerror(errno));
//				return 1;
//		}
//		printf("peektext write i=%d\n", i);
//	}
//	return 0;
//}
const word_size = 8;
void putdate(pid_t child, unsigned long addr, char *str, int len){
	char *laddr;
	int i,j;
	union u{
		long val;
		char chars[word_size];
	}data;
	i = 0;
	j = len/word_size;
	laddr = str;

	while(i < j){
		memcpy(data.chars, laddr, word_size);
		ptrace(PTRACE_POKEDATA, child, addr + i*8, data.val);
		++i;
		laddr += word_size;
	}
	j = len%word_size;
	if(j != 0){
		memcpy(data.chars, laddr, j);
		ptrace(PTRACE_POKEDATA, child, addr + i*8, data.val);
	}
}
int main(int argc, char *argv[])
{
	unsigned long loader_libc = 0;
	unsigned long T_libc = 0;
	unsigned long Loader_dlopen = 0;
	unsigned long T_dlopen = 0;
	pid_t T_pid;
	//step 1 find T dlopen add
	T_pid = atoi(argv[1]);
	loader_libc = find_lib_base(getpid(), "libc-");
	T_libc = find_lib_base(T_pid, "libc-");
	printf("T_libc=%lx\n", T_libc);

	Loader_dlopen = (unsigned long)dlsym(NULL, "__libc_dlopen_mode");
	//Loader_dlopen = (unsigned long)dlsym(NULL, "malloc");
	printf("Loader_dlopen=%lx\n", Loader_dlopen);
	T_dlopen = T_libc + (Loader_dlopen - loader_libc);
	printf("T_dlopen=%lx\n", T_dlopen);
	//step 2 attach T
	ptrace_attach(T_pid);

	//step 3 save T rsp
	struct user_regs_struct saved_regs = {0};
	memset(&saved_regs, 0, sizeof(struct user_regs_struct));
	ptrace_getregs(T_pid, &saved_regs);

	//step 4  dlopen
	struct user_regs_struct set_regs = {0};
	memcpy(&set_regs, &saved_regs, sizeof(struct user_regs_struct));
	unsigned long invalid = 0x0;
	set_regs.rsp -= sizeof(invalid);
	putdate(T_pid, set_regs.rsp, ((void *)&invalid), sizeof(invalid));
	//ptrace_poketext(T_pid, set_regs.rsp, ((void *)&invalid), sizeof(invalid));
	printf("start write libnew.so\n");
	char filename[128] = {0};
	sprintf(filename,"%s","/home/haojinglong/kf/dynamic/patch/libnew.so");
	//ptrace_poketext(T_pid, set_regs.rsp + 512, filename, strlen(filename) + 1);
	putdate(T_pid, set_regs.rsp + 512, filename, strlen(filename) + 1);
	//ptrace_poketext(T_pid, set_regs.rsp + 512, "/home/haojinglong/kf/dynamic/patch/libnew.so", strlen("/home/haojinglong/kf/dynamic/patch/libnew.so") + 1);
	set_regs.rip = T_dlopen+2;
	set_regs.rdi = set_regs.rsp + 512;
//	set_regs.rdi = set_regs.rsp;
	set_regs.rsi = RTLD_NOW;
	ptrace_setregs(T_pid, &set_regs);

	struct user_regs_struct get_rips = {0};
	ptrace_getregs(T_pid, &get_rips);
	printf("T_dlopen =%lx, set_regs.rip=%lx, get_rips=%lx\n", T_dlopen, set_regs.rip, get_rips.rip);

	//step 5
	printf("T continue start \n");
	ptrace_cont(T_pid);
	printf("T continue end \n");
	//step 6 set regs
	struct user_regs_struct get_regs = {0};
	ptrace_getregs(T_pid, &get_regs);
	int dlopen_ret = get_regs.rax;
	ptrace_setregs(T_pid, &saved_regs);
	printf("set ptrace_setregs ok\n");
	sleep(20);
	ptrace_detach(T_pid);
}
