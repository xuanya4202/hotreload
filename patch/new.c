#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <dlfcn.h>

print_v2(void)
{
		printf("Goodbye\n");
}

static void __attribute__((constructor)) init(void)
{
		int numpages;
		void *old_func_entry, *new_func_entry;
		printf("new lib exec!\n");

		old_func_entry = dlsym(NULL, "print");
		new_func_entry = dlsym(NULL, "print_v2");

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE-1))

		//numpages = (PAGE_SIZE - (old_func_entry & ~PAGE_MASK) >= size) ? 1 : 2;
		//mprotect((void *)(old_func_entry & PAGE_MASK), numpages * PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
		mprotect((void *)((size_t)old_func_entry & PAGE_MASK), 4096, PROT_READ|PROT_WRITE|PROT_EXEC);
		memset(old_func_entry, 0x48, 1);
		memset(old_func_entry + 1, 0xb8, 1);
		memcpy(old_func_entry + 2, &new_func_entry, 8);
		memset(old_func_entry + 10, 0xff, 1);
		memset(old_func_entry + 11, 0xe0, 1);
}
