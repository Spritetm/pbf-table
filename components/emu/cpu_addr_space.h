#include "stdint.h"
#if ESP_PLATFORM
#include "esp_attr.h"
#else
#define IRAM_ATTR
#endif

#pragma once

#define TRACK_CHUNK_USE 0

#define CHUNK_SIZE (1024) //must be power of two
#define NO_CHUNKS ((1024*1024)/CHUNK_SIZE)

typedef struct mem_chunk_t mem_chunk_t;
typedef void(*mem_write8_fn)(int addr, uint8_t data, mem_chunk_t *ch);
typedef uint8_t(*mem_read8_fn)(int addr, mem_chunk_t *ch);
struct mem_chunk_t {
	mem_write8_fn write_fn;
	mem_read8_fn read_fn;
	void *usr;
#if TRACK_CHUNK_USE
	int base;
	uint64_t hitctr;
#endif
};
extern mem_chunk_t cpu_addr_space_memory[NO_CHUNKS];




void cpu_addr_space_map_cb(int addr, int len, mem_write8_fn writefn, mem_read8_fn readfn, void *usr);
void cpu_addr_space_map_cow(const uint8_t *buf, int addr, int len);
void cpu_addr_space_init();
void cpu_addr_space_dump();
void cpu_addr_dump_file(const char *file, int off, int len);
void cpu_addr_dump_hitctr();

static inline uint16_t IRAM_ATTR cpu_addr_space_read16(int addr) {
	int ch=addr/CHUNK_SIZE;
	uint16_t r=cpu_addr_space_memory[ch].read_fn(addr, &cpu_addr_space_memory[ch]);
	ch=(addr+1)/CHUNK_SIZE;
	r|=cpu_addr_space_memory[ch].read_fn(addr+1, &cpu_addr_space_memory[ch])<<8;
	return r;
}

static inline uint8_t IRAM_ATTR cpu_addr_space_read8(int addr) {
	int ch=addr/CHUNK_SIZE;
	return cpu_addr_space_memory[ch].read_fn(addr, &cpu_addr_space_memory[ch]);
}

static inline void IRAM_ATTR cpu_addr_space_write8(int addr, uint8_t data) {
	int ch=addr/CHUNK_SIZE;
	cpu_addr_space_memory[ch].write_fn(addr, data, &cpu_addr_space_memory[ch]);
}

static inline void IRAM_ATTR cpu_addr_space_write16(int addr, uint16_t value) {
	cpu_addr_space_write8(addr, value&0xff);
	cpu_addr_space_write8(addr+1, value>>8);
}

#define write86(x,y) cpu_addr_space_write8(x,y)
#define writew86(x,y) cpu_addr_space_write16(x,y)
#define read86(x) cpu_addr_space_read8(x)
#define readw86(x) cpu_addr_space_read16(x)
