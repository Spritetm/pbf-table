#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include <string.h>
#include "gfx.h"
#include "cpu_addr_space.h"
#include "ibxm/ibxm.h"

//Note: this code assumes a little-endian host machine. It messes up when run on a big-endian one.


#define SAMP_RATE 44100
struct module *music_module;
struct replay *music_replay;
uint8_t *music_mixbuf;
int music_mixbuf_len;
int music_mixbuf_left;


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
		if (attr==0) pal[col]=(pal[col]&0x00ffff)|(val<<16);
		if (attr==1) pal[col]=(pal[col]&0xff00ff)|(val<<8);
		if (attr==2) pal[col]=(pal[col]&0xffff00)|(val<<0);
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
//		printf("Port 0x%X val 0x%02X\n", port, val);
	}
}

void portout16(uint16_t port, uint16_t val) {
//	printf("Port 0x%X val 0x%04X\n", port, val);
	portout(port, val&0xff);
	portout(port+1, val>>8);
}

void vga_mem_write(int addr, uint8_t data, mem_chunk_t *ch) {
	//Pinball Fantasies uses Mode X.
	int vaddr=addr-0xA0000;
	if (vga_mask&1) vram[vaddr*4+0]=data;
	if (vga_mask&2) vram[vaddr*4+1]=data;
	if (vga_mask&4) vram[vaddr*4+2]=data;
	if (vga_mask&8) vram[vaddr*4+3]=data;
}

uint8_t vga_mem_read(int addr, mem_chunk_t *ch) {
	printf("VGA mem read unimplemented!\n");
}

void init_vga_mem() {
	cpu_addr_space_map_cb(0xa0000, 0x10000, vga_mem_write, vga_mem_read, NULL);
}

int hook_interrupt_call(uint8_t intr);

void intr_table_writefn(int addr, uint8_t data, mem_chunk_t *ch) {
	uint8_t *mem=(uint8_t*)ch->usr;
	if (addr<1024) mem[addr]=data;
}

uint8_t intr_table_readfn(int addr, mem_chunk_t *ch) {
	uint8_t *mem=(uint8_t*)ch->usr;
	if (addr<1024) {
		return mem[addr];
	} else {
		int intr=(addr-1024);
		if (intr<256) hook_interrupt_call(intr);
		return 0xcf; //IRET
	}
}

void init_intr_table() {
	//Set up interrupt table so each interrupt i vectors to address (1024+i)
	uint8_t *ram=malloc(256*4);
	for (int i=0; i<256; i++) {
		ram[i*4+2]=0x40;
		ram[i*4+3]=0;
		ram[i*4+0]=i;
		ram[i*4+1]=0;
	}
	cpu_addr_space_map_cb(0, 2048, intr_table_writefn, intr_table_readfn, ram);
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
	} else if (intr==0x66 && (REG_AX&0xff)==8) {
//		printf("audio: fill music buffer\n");
	} else if (intr==0x66 && (REG_AX&0xff)==16) {
		printf("audio: play jingle in bx: 0x%X\n", REG_BX);
		audio_lock();
		replay_set_sequence_pos(music_replay, REG_BX);
		audio_unlock();
	} else if (intr==0x66 && (REG_AX&0xff)==21) {
		printf("audio: get status, AL returns bitfield, bit 3 set if silent? (Maybe channel usage bitmap?)\n");
	} else if (intr==0x66 && (REG_AX&0xff)==22) {
		printf("audio: ?get amount of data in buffers into ax?\n");
	} else if (intr==0x66 && (REG_AX&0xff)==16) {
		printf("audio: force position in bx. ret al=old pos\n");
	} else if (intr==0x66 && (REG_AX&0xff)==17) {
		printf("audio: play sound effect in cl,bl,dl at volume bh\n");
	} else if (intr==0x66 && (REG_AX&0xff)==18) {
		int adr=(REG_DS*0x10)+REG_DX;
		char name[64]={0};
		for (int i=0; i<64; i++) name[i]=cpu_addr_space_read8(adr+i);
		printf("audio: load mod in ds:dx: %s\n", name);
	} else if (intr==0x66 && (REG_AX&0xff)==4) {
		printf("audio: play module ?idx is in bx: 0x%X? at rate cx %d (0 if default)\n", REG_BX, REG_CX);
//		audio_lock();
//		replay_set_sequence_pos(music_replay, REG_BX-1);
//		audio_unlock();
	} else if (intr==0x66 && (REG_AX&0xff)==15) {
		printf("audio: stop play\n");
	} else if (intr==0x66 && (REG_AX&0xff)==6) {
		printf("set vol in cx?\n");
	} else if (intr==0x66 && (REG_AX&0xff)==0) {
		printf("deinit sound device\n");
	} else if (intr==0x66 && (REG_AX&0xff)==11) {
		//note: far call?
		printf("init vblank interrupt to es:dx\n");
		cb_blank_seg=REG_ES;
		cb_blank_off=REG_DX;
	} else if (intr==0x66 && (REG_AX&0xff)==12) {
		//note: far call
		printf("init raster interrupt to es:dx, raster in bl or cx?\n");
		cb_raster_seg=REG_ES;
		cb_raster_off=REG_DX;
	} else if (intr==0x66 && (REG_AX&0xff)==19) {
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
	cpu_addr_space_map_cow(&exe[exe_data_start], load_start_addr, size-exe_data_start);

	mz_reloc_t *relocs=(mz_reloc_t*)&exe[hdr->reloc_table_offset];
	for (int i=0; i<hdr->num_relocs; i++) {
		int adr=relocs[i].segment*0x10+relocs[i].offset;
		int w=cpu_addr_space_read8(load_start_addr+adr);
		w|=cpu_addr_space_read8(load_start_addr+adr+1)<<8;
		w=w+(load_start_addr/0x10);
		cpu_addr_space_write8(load_start_addr+adr, w&0xff);
		cpu_addr_space_write8(load_start_addr+adr+1, w>>8);
		printf("Fixup at %x\n", load_start_addr+adr);
	}
	cpu.segregs[regds]=(load_start_addr-256)/0x10;
	cpu.segregs[reges]=(load_start_addr-256)/0x10;
	cpu.segregs[regcs]=(load_start_addr/0x10)+hdr->cs;
	cpu.segregs[regss]=(load_start_addr/0x10)+hdr->ss;
	cpu.regs.wordregs[regax]=0; //should be related to psp, but we don't emu that.
	cpu.regs.wordregs[regsp]=hdr->sp;
	cpu.ip=hdr->ip;
	printf("Exe load done. CPU is set up to start at %04X:%04X (%06X).\n", cpu.segregs[regcs], cpu.ip, cpu.segregs[regcs]*0x10+cpu.ip);
	for (int i=0; i<32; i++) printf("%02X ", cpu_addr_space_read8(cpu.segregs[regcs]*0x10+cpu.ip+i));
	printf("\n");
	return size-exe_data_start;
}

void force_callback(int seg, int off) {
	struct cpu cpu_backup;
	//Save existing registers
	cpu_backup=cpu;
	//set cs:ip to address of callback
	cpu.segregs[regcs]=seg;
	cpu.ip=off;
	//increase sp by two, as if there was a call that pushed the address of the calling function
	//on the stack
	cpu.regs.wordregs[regsp]=cpu.regs.wordregs[regsp]-2;
	//Run the callback until we can see the return address has been popped; this
	//indicates a ret happened.
	while (cpu.regs.wordregs[regsp]<cpu_backup.regs.wordregs[regsp]) exec86(1);
	//Restore registers
	cpu=cpu_backup;
}

void music_init(const char *fname) {
	FILE *f=fopen(fname, "r");
	if (!f) {
		perror(fname);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	int len=ftell(f);
	fseek(f, 0, SEEK_SET);
	struct data mod_data;
	mod_data.buffer=malloc(len);
	fread(mod_data.buffer, 1, len, f);
	mod_data.length=len;
	fclose(f);
	char msg[64];
	music_module=module_load(&mod_data, msg);
	if (music_module==NULL) {
		printf("%s: %s\n", fname, msg);
		exit(1);
	}
	music_replay=new_replay(music_module, SAMP_RATE, 0);
	music_mixbuf=malloc(calculate_mix_buf_len(SAMP_RATE)*2);
	music_mixbuf_left=0;
	music_mixbuf_len=0;
}


void audio_cb(void* userdata, uint8_t* stream, int len) {
	int pos=0;
	if (len==0) return;
	if (music_mixbuf_left!=0) {
		pos=music_mixbuf_left;
		memcpy(stream, &music_mixbuf[music_mixbuf_len-music_mixbuf_left], music_mixbuf_left);
	}
	music_mixbuf_left=0;
	while (pos!=len) {
		music_mixbuf_len=replay_get_audio(music_replay, (int*)music_mixbuf)*4;
		int cplen=music_mixbuf_len;
		if (pos+cplen>len) {
			cplen=len-pos;
			music_mixbuf_left=music_mixbuf_len-cplen;
		}
		memcpy(&stream[pos], music_mixbuf, cplen);
		pos+=cplen;
	}
}



int main(int argc, char** argv) {
	cpu_addr_space_init();
	init_intr_table();
	init_vga_mem();
	gfx_init();
	music_init("../data/TABLE1.MOD");
	init_audio(SAMP_RATE, audio_cb);
	load_mz("../data/TABLE1.PRG", 0x10000);
	while(1) {
		exec86(20000);
		if (cb_raster_seg!=0) force_callback(cb_raster_seg, cb_raster_off);
		exec86(20000);
		if (cb_blank_seg!=0) force_callback(cb_blank_seg, cb_blank_off);
//		printf("hor %d ver %d\n", vga_hor, vga_ver);
//		cpu_addr_space_dump();
		gfx_show(vram, pal, vga_ver, 640);
		gfx_tick();
	}
	return 0;
}