//Lazily allocate address space of the x86 emulator to things
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "cpu_addr_space.h"
#if ESP_PLATFORM
#include "esp_attr.h"
#else
#define IRAM_ATTR
#endif

//1MiB of chunks
mem_chunk_t cpu_addr_space_memory[NO_CHUNKS];

static IRAM_ATTR uint8_t read_ram(int addr, mem_chunk_t *memch) {
//	printf("RAM read %X\n", addr);
	uint8_t *mem=(uint8_t*)memch->usr;
#if TRACK_CHUNK_USE
	memch->hitctr++;
#endif
	return mem[addr&(CHUNK_SIZE-1)];
}

static IRAM_ATTR void write_ram(int addr, uint8_t data, mem_chunk_t *memch) {
//	printf("RAM write %X value %X\n", addr, data);
	uint8_t *mem=(uint8_t*)memch->usr;
	mem[addr&(CHUNK_SIZE-1)]=data;
}

static void write_cow(int addr, uint8_t data, mem_chunk_t *memch) {
	//Move to RAM and make into normal memory
//	printf("cpu_addr_space: mapping in new page for written cow chunk\n");
	uint8_t *mem=malloc(CHUNK_SIZE);
	memcpy(mem, memch->usr, CHUNK_SIZE);
	memch->write_fn=write_ram;
	memch->usr=mem;
	//Do write
	write_ram(addr, data, memch);
}

static void write_not_mapped(int addr, uint8_t data, mem_chunk_t *memch) {
	//Map ram into address and do write.
//	printf("cpu_addr_space: mapping in new page for chunk\n");
	memch->read_fn=read_ram;
	memch->write_fn=write_ram;
	memch->usr=malloc(CHUNK_SIZE);
	write_ram(addr, data, memch);
}

static uint8_t read_not_mapped(int addr, mem_chunk_t *memch) {
	printf("Read from unmapped memory! %X\n", addr);
	return 0;
}

void cpu_addr_space_map_cb(int addr, int len, mem_write8_fn writefn, mem_read8_fn readfn, void *usr) {
	assert((addr%CHUNK_SIZE)==0);
	int ch=addr/CHUNK_SIZE;
	while (len>0) {
//		printf("Mapping chunk %d to cb\n", ch);
		assert(cpu_addr_space_memory[ch].write_fn==write_not_mapped);
		cpu_addr_space_memory[ch].read_fn=readfn;
		cpu_addr_space_memory[ch].write_fn=writefn;
		cpu_addr_space_memory[ch].usr=usr;
		ch++;
		len-=CHUNK_SIZE;
	}
}

void cpu_addr_space_map_cow(const uint8_t *buf, int addr, int len) {
	assert((addr%CHUNK_SIZE)==0);
	int ch=addr/CHUNK_SIZE;
	int off=0;
	while (off<len) {
		assert(cpu_addr_space_memory[ch].write_fn==write_not_mapped);
		cpu_addr_space_memory[ch].read_fn=read_ram;
		cpu_addr_space_memory[ch].write_fn=write_cow;
		cpu_addr_space_memory[ch].usr=(void*)&buf[off];
		ch++;
		off+=CHUNK_SIZE;
	}
}

void cpu_addr_space_init() {
	for (int i=0; i<NO_CHUNKS; i++) {
		cpu_addr_space_memory[i].write_fn=write_not_mapped;
		cpu_addr_space_memory[i].read_fn=read_not_mapped;
		cpu_addr_space_memory[i].usr=0;
#if TRACK_CHUNK_USE
		cpu_addr_space_memory[i].base=i*CHUNK_SIZE;
#endif
	}
}

void cpu_addr_space_dump() {
	int unmapped=0;
	int cow=0;
	int ram=0;
	int cb=0;
	for (int ch=0; ch<NO_CHUNKS; ch++) {
		if (cpu_addr_space_memory[ch].write_fn==write_not_mapped) {
			unmapped++;
		} else if (cpu_addr_space_memory[ch].write_fn==write_ram) {
			ram++;
		} else if (cpu_addr_space_memory[ch].write_fn==write_cow) {
			cow++;
		} else {
			cb++;
		}
	}
	printf("memory: mapping table %dK\n", (NO_CHUNKS*3*4)/1024);
	printf("memory: ram %d (%dK)\n", ram, ram*CHUNK_SIZE/1024);
	printf("memory: cow %d (%dK)\n", cow, cow*CHUNK_SIZE/1024);
	printf("memory: cb %d (%dK)\n", cb, cb*CHUNK_SIZE/1024);
	printf("memory: unmapped %d (%dK)\n", unmapped, unmapped*CHUNK_SIZE/1024);
}

void cpu_addr_dump_file(const char *file, int off, int len) {
	uint8_t *mem=malloc(len);
	for (int i=0; i<len; i++) mem[i]=cpu_addr_space_read8(off+i);
	FILE *f=fopen(file, "w");
	if (!f) {
		perror(file);
		return;
	}
	fwrite(mem, len, 1, f);
	fclose(f);
}

#if TRACK_CHUNK_USE
int hitmem_cmp(const void *a, const void *b) {
	mem_chunk_t *ca=*((mem_chunk_t**)a);
	mem_chunk_t *cb=*((mem_chunk_t**)b);
	if (ca->hitctr>cb->hitctr) return -1;
	if (ca->hitctr<cb->hitctr) return 1;
	return 0;
}

void cpu_addr_dump_hitctr() {
	mem_chunk_t **hitmem=calloc(sizeof(mem_chunk_t*), NO_CHUNKS);
	for (int i=0; i<NO_CHUNKS; i++) {
		hitmem[i]=&cpu_addr_space_memory[i];
	}
	qsort(hitmem, NO_CHUNKS, sizeof(mem_chunk_t*), hitmem_cmp);
	for (int i=0; i<64; i++) {
		printf("Base %04X hits %lld\n", hitmem[i]->base, hitmem[i]->hitctr);
	}
	free(hitmem);
}
#else
void cpu_addr_dump_hitctr() {
	//stub
}
#endif
