#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "cpu_addr_space.h"

#define CHUNK_SIZE (1024) //must be power of two
#define NO_CHUNKS ((1024*1024)/CHUNK_SIZE)

//1MiB of chunks
static mem_chunk_t memory[NO_CHUNKS];

static uint8_t read_ram(int addr, mem_chunk_t *memch) {
//	printf("RAM read %X\n", addr);
	uint8_t *mem=(uint8_t*)memch->usr;
	return mem[addr&(CHUNK_SIZE-1)];
}

static void write_ram(int addr, uint8_t data, mem_chunk_t *memch) {
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
		assert(memory[ch].write_fn==write_not_mapped);
		memory[ch].read_fn=readfn;
		memory[ch].write_fn=writefn;
		memory[ch].usr=usr;
		ch++;
		len-=CHUNK_SIZE;
	}
}

void cpu_addr_space_map_cow(const uint8_t *buf, int addr, int len) {
	assert((addr%CHUNK_SIZE)==0);
	int ch=addr/CHUNK_SIZE;
	int off=0;
	while (off<len) {
		assert(memory[ch].write_fn==write_not_mapped);
		memory[ch].read_fn=read_ram;
		memory[ch].write_fn=write_cow;
		memory[ch].usr=(void*)&buf[off];
		ch++;
		off+=CHUNK_SIZE;
	}
}

void cpu_addr_space_init() {
	for (int i=0; i<NO_CHUNKS; i++) {
		memory[i].write_fn=write_not_mapped;
		memory[i].read_fn=read_not_mapped;
		memory[i].usr=0;
	}
}

uint8_t cpu_addr_space_read8(int addr) {
	int ch=addr/CHUNK_SIZE;
	return memory[ch].read_fn(addr, &memory[ch]);
}

void cpu_addr_space_write8(int addr, uint8_t data) {
	int ch=addr/CHUNK_SIZE;
	memory[ch].write_fn(addr, data, &memory[ch]);
}

void cpu_addr_space_dump() {
	int unmapped=0;
	int cow=0;
	int ram=0;
	int cb=0;
	for (int ch=0; ch<NO_CHUNKS; ch++) {
		if (memory[ch].write_fn==write_not_mapped) {
			unmapped++;
		} else if (memory[ch].write_fn==write_ram) {
			ram++;
		} else if (memory[ch].write_fn==write_cow) {
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


//  Hooks for emu

void write86(uint32_t addr32, uint8_t value) {
	cpu_addr_space_write8(addr32, value);
}

void writew86(uint32_t addr32, uint16_t value) {
	write86(addr32, value&0xff);
	write86(addr32+1, value>>8);
}

uint8_t read86(uint32_t addr32) {
	return cpu_addr_space_read8(addr32);
}

uint16_t readw86(uint32_t addr32) {
	return read86(addr32)+(read86(addr32+1)*0x100);
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
