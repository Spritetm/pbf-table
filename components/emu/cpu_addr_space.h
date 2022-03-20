#include "stdint.h"

typedef struct mem_chunk_t mem_chunk_t;

typedef void(*mem_write8_fn)(int addr, uint8_t data, mem_chunk_t *ch);
typedef uint8_t(*mem_read8_fn)(int addr, mem_chunk_t *ch);

struct mem_chunk_t {
	mem_write8_fn write_fn;
	mem_read8_fn read_fn;
	void *usr;
};

void cpu_addr_space_map_cb(int addr, int len, mem_write8_fn writefn, mem_read8_fn readfn, void *usr);
void cpu_addr_space_map_cow(const uint8_t *buf, int addr, int len);
void cpu_addr_space_init();
uint8_t cpu_addr_space_read8(int addr);
void cpu_addr_space_write8(int addr, uint8_t data);
void cpu_addr_space_dump();
void cpu_addr_dump_file(const char *file, int off, int len);