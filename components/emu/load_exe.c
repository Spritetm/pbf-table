#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"
#include "cpu_addr_space.h"
#include "mmap_file.h"

//from http://www.delorie.com/djgpp/doc/exe/
typedef struct {
  uint16_t signature; /* == 0x5a4D */
  uint16_t bytes_in_last_block;
  uint16_t blocks_in_file;
  uint16_t num_relocs;
  uint16_t header_paragraphs;
  uint16_t min_extra_paragraphs;
  uint16_t max_extra_paragraphs;
  uint16_t ss;
  uint16_t sp;
  uint16_t checksum;
  uint16_t ip;
  uint16_t cs;
  uint16_t reloc_table_offset;
  uint16_t overlay_number;
} mz_hdr_t;

typedef struct {
  uint16_t offset;
  uint16_t segment;
} mz_reloc_t;

//Note: Assumes the PSP starts 256 bytes before load_start_addr
//Returns amount of data loaded.
int load_mz(const char *exefile, int load_start_addr) {
	const uint8_t *exe;
	int size=mmap_file(exefile, (const void**)&exe);
	
	mz_hdr_t *hdr=(mz_hdr_t*)exe;
	if (hdr->signature!=0x5a4d) {
		printf("Not an exe file!\n");
		munmap_file((const void*)exe);
		return -1;
	}
	int exe_data_start = hdr->header_paragraphs*16;
	printf("mz: data starts at %d\n", exe_data_start);
	cpu_addr_space_map_cow(&exe[exe_data_start], load_start_addr, size-exe_data_start);

	mz_reloc_t *relocs=(mz_reloc_t*)&exe[hdr->reloc_table_offset];
	for (int i=0; i<hdr->num_relocs; i++) {
		int adr=relocs[i].segment*0x10+relocs[i].offset;
		int w=cpu_addr_space_read8(load_start_addr+adr);
		w|=cpu_addr_space_read8(load_start_addr+adr+1)<<8;
		w=w+(load_start_addr/0x10);
		cpu_addr_space_write8(load_start_addr+adr, w&0xff);
		cpu_addr_space_write8(load_start_addr+adr+1, w>>8);
//		printf("Fixup at %x\n", load_start_addr+adr);
	}
	cpu.segregs[regds]=(load_start_addr-256)/0x10;
	cpu.segregs[reges]=(load_start_addr-256)/0x10;
	cpu.segregs[regcs]=(load_start_addr/0x10)+hdr->cs;
	cpu.segregs[regss]=(load_start_addr/0x10)+hdr->ss;
	cpu.regs.wordregs[regax]=0; //should be related to psp, but we don't emu that.
	cpu.regs.wordregs[regsp]=hdr->sp;
	cpu.ip=hdr->ip;
	printf("Exe load done. CPU is set up to start at %04X:%04X (%06X).\n", cpu.segregs[regcs], cpu.ip, cpu.segregs[regcs]*0x10+cpu.ip);

	return size-exe_data_start;
}
