#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include <string.h>
#include "gfx.h"

//Note: this code assumes a little-endian host machine. It messes up when run on a big-endian one.

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


uint8_t ram[0x100000];


#define REG_AX cpu.regs.wordregs[regax]
#define REG_BX cpu.regs.wordregs[regbx]
#define REG_CX cpu.regs.wordregs[regcx]
#define REG_DX cpu.regs.wordregs[regdx]
#define REG_BP cpu.regs.wordregs[regbp]
#define REG_DS cpu.segregs[regds]
#define REG_ES cpu.segregs[reges]

void dump_regs() {
	printf("AX %04X  SI %04X  ES %04X\n", cpu.regs.wordregs[regax], cpu.regs.wordregs[regsi], cpu.segregs[reges]);
	printf("BX %04X  DI %04X  CS %04X\n", cpu.regs.wordregs[regbx], cpu.regs.wordregs[regdi], cpu.segregs[regcs]);
	printf("CX %04X  SP %04X  SS %04X\n", cpu.regs.wordregs[regcx], cpu.regs.wordregs[regsp], cpu.segregs[regss]);
	printf("DX %04X  BP %04X  DS %04X\n", cpu.regs.wordregs[regdx], cpu.regs.wordregs[regbp], cpu.segregs[regds]);
}

void timing() {
}

uint8_t vram[256*1024];
uint32_t pal[255];
int vga_seq_addr;
int vga_mask;
int vga_pal_idx;
int vga_crtc_addr;
int vga_hor=200, vga_ver=320;
int vga_startaddr;

uint8_t portin(uint16_t port) {
}

uint8_t portin16(uint16_t port) {
}

void portout(uint16_t port, uint8_t val) {
	if (port==0x3c4) {
		vga_seq_addr=val&3;
	} else if (port==0x3c5) {
		if (vga_seq_addr==2) {
			vga_mask=val&0xf;
		}
	} else if (port==0x3c8) {
		vga_pal_idx=val*4;
	} else if (port==0x3c9) {
		int col=vga_pal_idx>>2;
		int attr=vga_pal_idx&3;
		val<<=2;  //vga palette is 6 bit per color
		if (attr==0) pal[col]=(pal[col]&0xffff00)|(val<<0);
		if (attr==1) pal[col]=(pal[col]&0xff00ff)|(val<<8);
		if (attr==2) pal[col]=(pal[col]&0x00ffff)|(val<<16);
		if (attr==2) vga_pal_idx+=2; else vga_pal_idx++;
	} else if (port==0x3D4) {
		vga_seq_addr=val&31;
	} else if (port==0x3D5) {
		if (vga_seq_addr==1) {
			vga_hor=(val+1)*4;
		} else if (vga_seq_addr==0xC) {
			vga_startaddr=(vga_startaddr&0xff)|(val<<8);
		} else if (vga_seq_addr==0xD) {
			vga_startaddr=(vga_startaddr&0xff00)|(val);
		} else if (vga_seq_addr==0x13) {
			vga_ver=val*8;
		}
	} else {
		printf("Port 0x%X val 0x%02X\n", port, val);
	}
}

void portout16(uint16_t port, uint16_t val) {
	printf("Port 0x%X val 0x%04X\n", port, val);
	portout(port, val&0xff);
	portout(port+1, val>>8);
}

void write86(uint32_t addr32, uint8_t value) {
	if (addr32<0x10000) {
		printf("Write 0x%02X to 0x%X!\n", value, addr32);
	} else if (addr32>=0xA0000 && addr32<0xB0000) {
		//We need to do de-planing...
		int vaddr=addr32-0xA0000;
		if (vga_mask&8) {
			vram[vaddr*4+0]=(vram[vaddr*4+0]&0xFC)|((value>>0)&3);
			vram[vaddr*4+1]=(vram[vaddr*4+1]&0xFC)|((value>>2)&3);
			vram[vaddr*4+2]=(vram[vaddr*4+2]&0xFC)|((value>>4)&3);
			vram[vaddr*4+3]=(vram[vaddr*4+3]&0xFC)|((value>>6)&3);
		}
		if (vga_mask&4) {
			vram[vaddr*4+0]=(vram[vaddr*4+0]&0xF3)|(((value>>0)&3)<<2);
			vram[vaddr*4+1]=(vram[vaddr*4+1]&0xF3)|(((value>>2)&3)<<2);
			vram[vaddr*4+2]=(vram[vaddr*4+2]&0xF3)|(((value>>4)&3)<<2);
			vram[vaddr*4+3]=(vram[vaddr*4+3]&0xF3)|(((value>>6)&3)<<2);
		}
		if (vga_mask&2) {
			vram[vaddr*4+0]=(vram[vaddr*4+0]&0xCF)|(((value>>0)&3)<<4);
			vram[vaddr*4+1]=(vram[vaddr*4+1]&0xCF)|(((value>>2)&3)<<4);
			vram[vaddr*4+2]=(vram[vaddr*4+2]&0xCF)|(((value>>4)&3)<<4);
			vram[vaddr*4+3]=(vram[vaddr*4+3]&0xCF)|(((value>>6)&3)<<4);
		}
		if (vga_mask&1) {
			vram[vaddr*4+0]=(vram[vaddr*4+0]&0x3F)|(((value>>0)&3)<<6);
			vram[vaddr*4+1]=(vram[vaddr*4+1]&0x3F)|(((value>>2)&3)<<6);
			vram[vaddr*4+2]=(vram[vaddr*4+2]&0x3F)|(((value>>4)&3)<<6);
			vram[vaddr*4+3]=(vram[vaddr*4+3]&0x3F)|(((value>>6)&3)<<6);
		}
	} else {
		ram[addr32]=value;
	}
}
void writew86(uint32_t addr32, uint16_t value) {
	write86(addr32, value&0xff);
	write86(addr32+1, value>>8);
}

int hook_interrupt_call(uint8_t intr);

void init_intr_table() {
	//Set up interrupt table so each interrupt i vectors to address (1024+i)
	for (int i=0; i<256; i++) {
		ram[i*4+2]=0x40;
		ram[i*4+3]=0;
		ram[i*4+0]=i;
		ram[i*4+1]=0;
	}
}

uint8_t read86(uint32_t addr32) {
	if (addr32<1024) {
//		printf("ISR vec read 0x%X -> %X\n", addr32, ram[addr32]);
		return ram[addr32];
	} else if (addr32<1024+256) {
		//special region; the default interrupt table points here. If we read from here, it's because
		//we're trying to execute an interrupt that needs to be high-level emulated.
		//assume read from here because of interrupt
		int intr=addr32-1024;
//		printf("ISR trap ins read 0x%X\n", intr);
		hook_interrupt_call(intr);
		return 0xcf; //IRET
	} else if (addr32<0x10000) {
		printf("Read from 0x%X!\n", addr32);
		return ram[addr32];
	} else {
		return ram[addr32];
	}
}
uint16_t readw86(uint32_t addr32) {
	return read86(addr32)+(read86(addr32+1)*0x100);
}

int cb_raster_seg=0, cb_raster_off=0;
int cb_blank_seg=0, cb_blank_off=0;

int hook_interrupt_call(uint8_t intr) {
//	printf("Intr 0x%X\n", intr);
//	dump_regs();
	if (intr==0x21 && (REG_AX>>8)==0x4A) {
		printf("Resize block\n");
		//resize block; always allow as there's nothing in memory anyway
		REG_AX=0;
		cpu.cf=0;
	} else if (intr==0x21 && (REG_AX>>8)==0x9) {
		printf("Message: ");
		int adr=REG_DS*0x10+REG_DX;
		while (ram[adr]!='$') {
			putchar(ram[adr++]);
		}
		printf("\n");
	} else if (intr==0x21 && (REG_AX>>8)==0xE) {
		printf("Set default drive %c\n", 'A'+(REG_DX&0xff));
		REG_AX=0x0E00+26;
	} else if (intr==0x21 && (REG_AX>>8)==0x3B) {
		printf("Set current dir %s\n", &ram[REG_DS*0x10+REG_DX]);
		cpu.cf=0;
	} else if (intr==0x21 && (REG_AX>>8)==0x3D) {
		printf("Open file %s\n", &ram[REG_DS*0x10+REG_DX]);
		cpu.cf=1;
	} else if (intr==0x10 && (REG_AX>>8)==0) {
		printf("Set video mode %x\n", REG_AX&0xff);
	} else if (intr==0x10 && (REG_AX>>8)==0x13) {
		//Eek: unused; I misread the readout. Remove if still unused at the end
		int addr=REG_ES*0x10+REG_BP;
		int len=REG_CX;
		char *str=malloc(len);
		memcpy(str, &ram[addr], len);
		for (int i=0; i<len; i++) {
			if (str[i]<32) str[i]='.';
		}
		printf("message: %s\n", str);
		free(str);
	} else if (intr==0x65) {
		//internal keyhandler: START.ASM:363
		//ax=0: first_time_ask (ret ah=first_time, bl=bannumber)
		//ax=0xffff: save bl to bannumber
		//ax=0x100: LOKALA TOGGLAREN => RESIDENTA TOGGLAREN
		//ax=0x200: RESIDENTA TOGGLAREN => LOKALA TOGGLAREN
		//ax00x12: text input
		//others: simply return scan code without side effects
		//Returns scan code in ax
		REG_AX=0;
	} else if (intr==0x33) {
		//Mouse interrupt
		REG_AX=0; //no mouse
	} else if (intr==0x66 && REG_AX==8) {
//		printf("audio: fill music buffer\n");
	} else if (intr==0x66 && REG_AX==16) {
		printf("audio: play jingle in bx\n");
	} else if (intr==0x66 && REG_AX==21) {
		printf("audio: get status, AL returns bitfield, bit 3 set if silent? (Maybe channel usage bitmap?)\n");
	} else if (intr==0x66 && REG_AX==22) {
		printf("audio: ?get amount of data in buffers into ax?\n");
	} else if (intr==0x66 && REG_AX==16) {
		printf("audio: force position in bx. ret al=old pos\n");
	} else if (intr==0x66 && REG_AX==17) {
		printf("audio: play sound effect in cl,bl,dl at volume bh\n");
	} else if (intr==0x66 && REG_AX==18) {
		printf("audio: load mod in ds:dx\n");
	} else if (intr==0x66 && REG_AX==4) {
		printf("audio: play module ?idx is in bx? at rate cx (0 if default)\n");
	} else if (intr==0x66 && REG_AX==15) {
		printf("audio: stop play\n");
	} else if (intr==0x66 && REG_AX==6) {
		printf("set vol in cx?\n");
	} else if (intr==0x66 && REG_AX==0) {
		printf("deinit sound device\n");
	} else if (intr==0x66 && REG_AX==11) {
		//note: far call?
		printf("init vblank interrupt to es:dx\n");
		cb_blank_seg=REG_ES;
		cb_blank_off=REG_DX;
	} else if (intr==0x66 && REG_AX==12) {
		//note: far call
		printf("init raster interrupt to es:dx, raster in bl or cx?\n");
		cb_raster_seg=REG_ES;
		cb_raster_off=REG_DX;
	} else if (intr==0x66 && REG_AX==19) {
		printf("init jingle handler to es:dx\n");
	} else {
		printf("Unhandled interrupt 0x%X\n", intr);
		dump_regs();
	}
	return 0;
}

int cpu_hlt_handler() {
	printf("CPU halted!\n");
	exit(1);
}

//Note: Assumes the PSP starts 256 bytes before load_start_addr
//Returns amount of data loaded.
int load_mz(const char *exefile, int load_start_addr) {
	FILE *f=fopen(exefile, "r");
	if (!f) {
		perror(exefile);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	int size=ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *exe=malloc(size);
	fread(exe, 1, size, f);
	fclose(f);
	
	mz_hdr_t *hdr=(mz_hdr_t*)exe;
	if (hdr->signature!=0x5a4d) {
		printf("Not an exe file!\n");
		exit(1);
	}
	int exe_data_start = hdr->header_paragraphs*16;
	printf("mz: data starts at %d\n", exe_data_start);
	memcpy(&ram[load_start_addr], &exe[exe_data_start], size-exe_data_start);
	mz_reloc_t *relocs=(mz_reloc_t*)&exe[hdr->reloc_table_offset];
	for (int i=0; i<hdr->num_relocs; i++) {
		int adr=relocs[i].segment*0x10+relocs[i].offset;
		//possibly need to do this manually if unaligned?
		uint16_t *w=(uint16_t*)&ram[load_start_addr+adr];
		*w=*w+(load_start_addr/0x10);
	}
	cpu.segregs[regds]=(load_start_addr-256)/0x10;
	cpu.segregs[reges]=(load_start_addr-256)/0x10;
	cpu.segregs[regcs]=(load_start_addr/0x10)+hdr->cs;
	cpu.segregs[regss]=(load_start_addr/0x10)+hdr->ss;
	cpu.regs.wordregs[regax]=0; //should be related to psp, but we don't emu that.
	cpu.regs.wordregs[regsp]=hdr->sp;
	cpu.ip=hdr->ip;
	return size-exe_data_start;
}

void force_callback(int seg, int off) {
	struct cpu cpu_backup;
	cpu_backup=cpu;
	cpu.segregs[regcs]=seg;
	cpu.ip=off;
	cpu.regs.wordregs[regsp]=cpu.regs.wordregs[regsp]-2;
	printf("CB\n");
	while (cpu.regs.wordregs[regsp]<cpu_backup.regs.wordregs[regsp]) exec86(1);
	printf("CB done\n");
	cpu=cpu_backup;
}


int main(int argc, char** argv) {
	gfx_init();
	init_intr_table();
	load_mz("../data/TABLE1.PRG", 0x10000);
	while(1) {
		exec86(20000);
		if (cb_raster_seg!=0) force_callback(cb_raster_seg, cb_raster_off);
		exec86(20000);
		if (cb_blank_seg!=0) force_callback(cb_blank_seg, cb_blank_off);
		printf("hor %d ver %d\n", vga_hor, vga_ver);
		gfx_show(vram, pal, vga_ver, vga_hor*2);
//		gfx_show(vram, pal, 700,350);
		gfx_tick();
	}
	return 0;
}