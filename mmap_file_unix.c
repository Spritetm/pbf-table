#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#define DATA_DIR "../../../data"

int mmap_file(const char *filename, void **mem) {
	char path[256];
	sprintf(path, "%s/%s", DATA_DIR, filename);
	int f=open(path, O_RDONLY);
	if (!f) {
		perror(path);
		return 0;
	}
	struct stat statbuf;
	fstat(f, &statbuf);
	
	*mem=mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, f, 0);
	close(f);
	if (mem==MAP_FAILED) {
		perror(filename);
		return 0;
	}
	return statbuf.st_size;
}

void munmap_file(void *ptr) {
	munmap(ptr, 1024*1024);
}
