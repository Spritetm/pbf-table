#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <esp_partition.h>

typedef struct {
	const char *name;
	int type;
	int subtype;
	int size;
} file_t;

#define NO_FILES 10

const file_t available_files[NO_FILES]={
	{"TABLE1.PRG", 123, 1, 537030},
	{"TABLE2.PRG", 123, 2, 518198},
	{"TABLE3.PRG", 123, 3, 504950},
	{"TABLE4.PRG", 123, 4, 522422},
	{"TABLE1.MOD", 124, 1, 210760},
	{"TABLE2.MOD", 124, 2, 211912},
	{"TABLE3.MOD", 124, 3, 219668},
	{"TABLE4.MOD", 124, 4, 216418},
	{"INTRO.MOD", 124, 5, 317262},
	{"table-screens.bin", 125, 1, 1114112},
};

typedef struct {
	const void *adr;
	spi_flash_mmap_handle_t mmap_handle;
} file_mmap_t;

static file_mmap_t mmaped_files[NO_FILES]={0};

int mmap_file(const char *filename, const void **mem) {
	//First, find file.
	int fnum=0;
	while (fnum<NO_FILES && strcasecmp(filename, available_files[fnum].name)!=0) fnum++;
	if (fnum>=NO_FILES) {
		//Not found.
		*mem=NULL;
		return 0;
	}
	if (mmaped_files[fnum].adr) {
		//mmaped already, return existing map
		*mem=mmaped_files[fnum].adr;
		return available_files[fnum].size;
	}
	//Need to mmap the partition.
	const esp_partition_t *part;
	part=esp_partition_find_first(available_files[fnum].type, available_files[fnum].subtype, NULL);
	esp_partition_mmap(part, 0, available_files[fnum].size, SPI_FLASH_MMAP_DATA, 
						&mmaped_files[fnum].adr, &mmaped_files[fnum].mmap_handle);
	*mem=mmaped_files[fnum].adr;
	return available_files[fnum].size;
}

void munmap_file(void *ptr) {
	for (int i=0; i<NO_FILES; i++) {
		if (ptr==mmaped_files[i].adr) {
			mmaped_files[i].adr=NULL;
			spi_flash_munmap(mmaped_files[i].mmap_handle);
		}
	}
}

