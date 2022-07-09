//Mmap binding for poxix-y OSses
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#define DATA_DIR "../../../data"

int mmap_file(const char *filename, const void **mem) {
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

void munmap_file(const void *ptr) {
//Somehow, munmap makes the entire thing crash after the menu... something with corrupting video driver
//memory somehow? Obviously, not unmapping is a hack and shouldn't be done in production code... but
//this ain't production code.
//	munmap(ptr, 1024*1024);
}
