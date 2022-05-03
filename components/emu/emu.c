#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include <string.h>
#include "gfx.h"
#include "cpu_addr_space.h"
#include "hexdump.h"
#include "trace.h"
#include "scheduler.h"
#include "load_exe.h"
#include "mmap_file.h"
#include "pf_vars.h"
#include <assert.h>
#include "haptic.h"
#include "music.h"
#include "hiscore.h"
#include "initials.h"

#define HBL_FREQ 3146876
#define VBL_FREQ 60

//Note: this code assumes a little-endian host machine. It messes up when run on a big-endian one.


static int key_pressed=0;

#define REG_AX cpu.regs.wordregs[regax]
#define REG_BX cpu.regs.wordregs[regbx]
#define REG_CX cpu.regs.wordregs[regcx]
#define REG_DX cpu.regs.wordregs[regdx]
#define REG_BP cpu.regs.wordregs[regbp]
#define REG_DS cpu.segregs[regds]
#define REG_ES cpu.segregs[reges]

int load_mz(const char *exefile, int load_start_addr);


void dump_regs() {
	printf("AX %04X  SI %04X  ES %04X\n", cpu.regs.wordregs[regax], cpu.regs.wordregs[regsi], cpu.segregs[reges]);
	printf("BX %04X  DI %04X  CS %04X\n", cpu.regs.wordregs[regbx], cpu.regs.wordregs[regdi], cpu.segregs[regcs]);
	printf("CX %04X  SP %04X  SS %04X\n", cpu.regs.wordregs[regcx], cpu.regs.wordregs[regsp], cpu.segregs[regss]);
	printf("DX %04X  BP %04X  DS %04X\n", cpu.regs.wordregs[regdx], cpu.regs.wordregs[regbp], cpu.segregs[regds]);
}

void dump_caller() {
	int sp_addr=cpu.regs.wordregs[regsp]+cpu.segregs[regss]*0x10;
	int seg=cpu_addr_space_read8(sp_addr+2)+(cpu_addr_space_read8(sp_addr+3)<<8);
	int off=cpu_addr_space_read8(sp_addr)+(cpu_addr_space_read8(sp_addr+1)<<8);
	printf("Caller: %04X:%04X\n", seg, off);
}


static uint8_t *vram;
static uint32_t pal[256];
static int vga_seq_addr;
static int vga_mask;
static int vga_pal_idx;
static int vga_crtc_addr;
static int vga_hor=200, vga_ver=320;
static int vga_startaddr;
static uint8_t vga_gcregs[8];
static int vga_gcindex;
static uint8_t vga_latch[4];
static int hiscore_addr=-1;
static uint8_t hiscore_nvs_state[16*4];


static int keycode[]={
	0,
	0x50, //down
	0x2A, //lshift
	0x36, //rshift
	0x39, //space
	0x3b,
	0x3c,
	0x3d,
	0x3e,
	0, //test key
};

uint8_t portin(uint16_t port) {
	if (port==0x60) {
		int kc;
		if (key_pressed&INPUT_RAWSCANCODE) {
			kc=key_pressed&0xff;
			printf("Keycode (from raw) %x\n", kc);
			key_pressed=0;
		} else {
			kc=keycode[key_pressed&0x7F];
			if (key_pressed&INPUT_RELEASE) kc|=0x80;
			key_pressed=0;
			printf("Keycode %x\n", kc);
		}
		return kc;
	} else if (port==0x61) {
		return 0; //nothing wrong
	} else if (port==0x3CF) {
		return vga_gcregs[vga_gcindex];
	} else if (port==0x3C5) {
		//ignore, there's very little we use in there anyway
	} else {
		printf("Port read 0x%X\n", port);
		static int n=0;
		return n++;
	}
	return 0;
}

uint8_t portin16(uint16_t port) {
	printf("16-bit port read 0x%X\n", port);
	return 0;
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
	} else if (port==0x3ce) {
		vga_gcindex=val&7;
	} else if (port==0x3cf) {
//		if (val!=vga_gcregs[vga_gcindex]) {
//			printf("0x3C7 idx %d -> %02X\n", vga_gcindex, val);
//		}
		vga_gcregs[vga_gcindex]=val;
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
		} else {
			printf("port 3d5 idx 0x%X val 0x%X\n", vga_seq_addr, val);
		}
	} else if (port==0x61) {
		//Keyboard port; ignore
	} else {
		printf("Port 0x%X val 0x%02X\n", port, val);
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
	if ((vga_gcregs[5]&3)==0) {
		if (vga_mask&1) vram[vaddr*4+0]=data;
		if (vga_mask&2) vram[vaddr*4+1]=data;
		if (vga_mask&4) vram[vaddr*4+2]=data;
		if (vga_mask&8) vram[vaddr*4+3]=data;
	} else if ((vga_gcregs[5]&3)==1) {
		for (int i=0; i<4; i++) vram[vaddr*4+i]=vga_latch[i];
	}
}

uint8_t vga_mem_read(int addr, mem_chunk_t *ch) {
	int vaddr=addr-0xA0000;
	for (int i=0; i<4; i++) vga_latch[i]=vram[vaddr*4+i];
	//note: what if multiple bits in mask are enabled?
	if (vga_mask&1) return vram[vaddr*4+0];
	if (vga_mask&2) return vram[vaddr*4+1];
	if (vga_mask&4) return vram[vaddr*4+2];
	if (vga_mask&8) return vram[vaddr*4+3];
	return 0;
}

void init_vga_mem() {
	vram=calloc(256*1024, 1);
	cpu_addr_space_map_cb(0xa0000, 0x10000, vga_mem_write, vga_mem_read, NULL);
}

static void hook_interrupt_call(uint8_t intr);

void intr_table_writefn(int addr, uint8_t data, mem_chunk_t *ch) {
	//this is supposed to be rom
	printf("Aiee, write to rom segment? (Adr %05X, data %02X)\n", addr, data);
}

uint8_t intr_table_readfn(int addr, mem_chunk_t *ch) {
	int intr=(addr-0xf0000);
	if (intr<256) hook_interrupt_call(intr);
	return 0xcf; //IRET
}

uint8_t abort_readfn(int addr, mem_chunk_t *ch) {
	exec86_abort();
	return 0x90; //nop
}


void init_intr_table() {
	//Set up interrupt table so each interrupt i vectors to address (F000:i)
	for (int i=0; i<256; i++) {
		cpu_addr_space_write8(i*4+2, 0x00);
		cpu_addr_space_write8(i*4+3, 0xf0);
		cpu_addr_space_write8(i*4+0, i);
		cpu_addr_space_write8(i*4+1, 0);
	}
	//Set up some address space in ROM that the interrupt table vectors point at by default.
	//We take a read from the first 256 addresses as an interrupt request for that irq.
	cpu_addr_space_map_cb(0xf0000, 2048, intr_table_writefn, intr_table_readfn, NULL);
	//For callbacks, we also have some memory that aborts execution.
	cpu_addr_space_map_cb(0xf8000, 2048, intr_table_writefn, abort_readfn, NULL);
}

int cb_raster_int_line=0;
int cb_raster_seg=0, cb_raster_off=0;
int cb_blank_seg=0, cb_blank_off=0;
int cb_jingle_seg=0, cb_jingle_off=0;

int mouse_x=0;
int mouse_y=0;
int mouse_btn=0;
int mouse_down=0;

//this is what's indicated as 'togglaren' in the sources
typedef struct __attribute__((packed)) {
	uint8_t s_balls; //3 (0) or 5 (1). Default is 3
	uint8_t s_angle; //high (0) or low (1). Default is high.
	uint8_t s_scrolling; //medium (1), soft (2), hard (0). Default is medium.
	uint8_t s_im; //ingame music, on (0) or off (1). Default is on.
	uint8_t s_resolution; //reso, normal (0) or high (1). Default is normal.
	uint8_t s_mode; //color mode, color (0) or mono (1). Default is color.
} pref_type_t;


static pref_type_t prefs={
	.s_balls=0,
	.s_angle=0,
	.s_scrolling=1,
	.s_im=0,
	.s_resolution=0,
	.s_mode=0
};

#define FILEHANDLE_HISCORE 10
static char hiscore_file[9]={0};

static void hook_interrupt_call(uint8_t intr) {
//	printf("Intr 0x%X\n", intr);
//	dump_regs();
	if (intr==0x21 && (REG_AX>>8)==0x4A) {
		printf("Resize block\n");
		//resize block; always allow as there's nothing in memory anyway
		REG_AX=0;
		cpu.cf=0;
	} else if (intr==0x21 && (REG_AX>>8)==0x9) {
		printf("Message\n");
	} else if (intr==0x21 && (REG_AX>>8)==0xE) {
		printf("Set default drive %c\n", 'A'+(REG_DX&0xff));
		REG_AX=0x0E00+26;
	} else if (intr==0x21 && (REG_AX>>8)==0x3B) {
		printf("Set current dir\n");
		cpu.cf=0;
	} else if (intr==0x21 && ((REG_AX>>8)==0x3C || (REG_AX>>8)==0x3D)) {
		//we handle open/create the same here
		int adr_name=(REG_DS<<4)+REG_DX;
		char file[256]={0};
		for (int i=0; i<255; i++) {
			file[i]=cpu_addr_space_read8(adr_name+i);
			if (file[i]==0) break;
		}
		printf("Open file %s\n", file);
		cpu.cf=0; //assume success
		if (strlen(file)>3 && strcasecmp(&file[strlen(file)-3], ".hi")==0) {
			REG_AX=FILEHANDLE_HISCORE;
			printf("Is hiscore file.\n");
			strcpy(hiscore_file, &file[strlen(file)-9]);
		} else {
			cpu.cf=1;
			REG_AX=2; //file not found
		}
	} else if (intr==0x21 && (REG_AX>>8)==0x3E) {
		printf("Close file\n");
		cpu.cf=0; //whatever
	} else if (intr==0x21 && (REG_AX>>8)==0x3F) {
		printf("Read from file\n");
		int addr_data=(REG_DS<<4)+REG_DX;
		int len=REG_CX;
		cpu.cf=1;
		if (REG_BX==FILEHANDLE_HISCORE) {
			hiscore_addr=addr_data; //save the address as this stays consistent
			hiscore_get(hiscore_file, hiscore_nvs_state);
			for (int i=0; i<REG_CX; i++) cpu_addr_space_write8(addr_data+i, hiscore_nvs_state[i]);
			cpu.cf=0;
			REG_AX=len;
		}
	} else if (intr==0x21 && (REG_AX>>8)==0x3F) {
		printf("Write to file");
		int adr_data=(REG_DS<<4)+REG_DX;
		int len=REG_CX;
		cpu.cf=1;
		if (REG_BX==FILEHANDLE_HISCORE) {
			uint8_t hiscore[16*4];
			for (int i=0; i<REG_CX; i++) hiscore[i]=cpu_addr_space_read8(adr_data);
			hiscore_put(hiscore_file, hiscore);
			cpu.cf=0;
			REG_AX=len;
		}
	} else if (intr==0x10 && (REG_AX>>8)==0) {
		printf("Set video mode %x\n", REG_AX&0xff);
	} else if (intr==0x65) {
		//internal keyhandler: START.ASM:363
		//ax=0: first_time_ask (ret ah=first_time, bl=bannumber)
		//ax=0xffff: save bl to bannumber
		//ax=0x100: LOKALA TOGGLAREN => RESIDENTA TOGGLAREN
		//ax=0x200: RESIDENTA TOGGLAREN => LOKALA TOGGLAREN
		//(togglaren being the preferences, so save & load prefs
		//from/to es:bx)
		//ax00x12: text input
		//others: simply return scan code without side effects
		//Returns scan code in ax
		printf("Key handler, ax=%04X\n", REG_AX);
		if (REG_AX==0x200) {
			int adr=(REG_ES<<4)+REG_BX;
			uint8_t *p=(uint8_t*)&prefs;
			for (int i=0; i<sizeof(prefs); i++) {
				cpu_addr_space_write8(adr++, *p++);
			}
		}
		REG_AX=0;
		dump_regs();
	} else if (intr==0x33) {
		//Mouse interrupt
		if ((REG_AX&0xff)==0) {
			REG_AX=0xFFFF; //installed
			REG_BX=2; //2 buttons
		} else if ((REG_AX&0xff)==8) {
			printf("Mouse: def vert cursor range %d to %d\n", REG_CX, REG_DX);
		} else if ((REG_AX&0xff)==0xf) {
			printf("Mouse: def mickey/pixel hor %d vert %d\n", REG_CX, REG_DX);
		} else if ((REG_AX&0xff)==3) {
			if (mouse_down) mouse_y-=5;
			REG_BX=mouse_btn;
			REG_CX=mouse_x; //col
			REG_DX=mouse_y; //row
//			printf("read mouse %d,%d\n", mouse_x, mouse_y);
			mouse_btn=0;
		} else if ((REG_AX&0xff)==4) {
			//position cursor cx:dx
			mouse_x=REG_CX;
			mouse_y=REG_DX;
//			printf("pos mouse %d,%d\n", mouse_x, mouse_y);
		}
	} else if (intr==0x66 && (REG_AX&0xff)==8) {
//		printf("audio: fill music buffer\n");
		REG_AX=0xff00;  //Note: this seems to indicate *something*... but I have no clue how this affects the code? Super-odd.
//		dump_caller();
	} else if (intr==0x66 && (REG_AX&0xff)==16) { 
		//AL=FORCE POSITION, BX=THE POSITION ret: AL=OLD POSITION
		printf("audio: play jingle in bx: 0x%X\n", REG_BX);
		int old_pos=music_get_sequence_pos();
		music_set_sequence_pos(REG_BX);
		REG_AX=old_pos;
	} else if (intr==0x66 && (REG_AX&0xff)==21) {
		printf("audio: get driver caps, AL returns bitfield? Nosound.drv returns 0xa.\n");
		//Note: bit 3 is tested for nosound
//		REG_AX=10; //nosound driver returns this
		REG_AX=8;
	} else if (intr==0x66 && (REG_AX&0xff)==22) {
		printf("audio: ?get amount of data in buffers into dx.ax?\n");
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==17) {
		printf("audio: play sound effect in cl,bl,dl at volume bh: effect %d, %d, %d vol %d\n", 
			REG_CX&0xff, REG_BX&0xff, REG_DX&0xff, REG_BX>>8);
			//sample, pitch?, channel?, volume (0-64)
		music_sfxchan_play(REG_CX&0xff, (REG_BX&0xff)+32, REG_BX>>8);
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==18) {
		int adr=(REG_DS*0x10)+REG_DX;
		char name[64]={0};
		for (int i=0; i<64; i++) name[i]=cpu_addr_space_read8(adr+i);
		printf("audio: load mod in ds:dx: %s\n", name);
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==4) {
		printf("audio: play module ?idx is in bx: 0x%X? at rate cx %d (0 if default)\n", REG_BX, REG_CX);
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==15) {
		printf("audio: stop play\n");
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==6) {
		printf("set vol in cx?\n");
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==0) {
		printf("deinit sound device\n");
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==11) {
		//note: far call. BL also is... something.
		printf("init vblank interrupt to es:dx\n");
		cb_blank_seg=REG_ES;
		cb_blank_off=REG_DX;
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==12) {
		//note: far call
		printf("init raster interrupt to es:dx, raster in cx %d?\n", REG_CX);
		cb_raster_seg=REG_ES;
		cb_raster_off=REG_DX;
		cb_raster_int_line=REG_CX;
		REG_AX=0;
	} else if (intr==0x66 && (REG_AX&0xff)==19) {
		printf("init jingle handler to es:dx\n");
		cb_jingle_seg=REG_ES;
		cb_jingle_off=REG_DX;
		REG_AX=0;
	} else {
		printf("Unhandled interrupt 0x%X\n", intr);
		dump_regs();
	}
}

int cpu_hlt_handler() {
	printf("CPU halted!\n");
	return 0;
}

uint16_t force_callback(int seg, int off, int axval) {
	uint16_t ret;
	struct cpu cpu_backup;
	//Save existing registers
	cpu_backup=cpu;
	//set cs:ip to address of callback
	cpu.segregs[regcs]=seg;
	cpu.ip=off;
	cpu.regs.wordregs[regax]=axval;
	//decrease sp by four, as if there was a call that pushed the address of the calling function
	//on the stack
	cpu.regs.wordregs[regsp]=cpu.regs.wordregs[regsp]-4;
	int stack_addr=cpu.segregs[regss]*0x10+cpu.regs.wordregs[regsp];
	//We'll return to a ROM location that aborts execution.
	cpu_addr_space_write8(stack_addr, 0x00);
	cpu_addr_space_write8(stack_addr+1, 0x00); //IP
	cpu_addr_space_write8(stack_addr+2, 0x00);
	cpu_addr_space_write8(stack_addr+3, 0xF8); //CS

	//Run the callback until we can see the return address has been popped and
	//we jumped there; this indicates a ret happened.
	int cycles=0;
	while(1) {
		const int cbcycles=10000;
		int t=trace_run_cpu(cbcycles);
		if (cpu.ip==0) {
			printf("IP %x, SP %x\n", cpu.ip, cpu.segregs[regcs]);
		}
		if (t) {
			cycles+=cbcycles-t;
			break;
		} else {
			cycles+=cbcycles;
		}
	}
	ret=cpu.regs.wordregs[regax];
	//Restore registers
	cpu=cpu_backup;
	schedule_adjust_cycles(cycles);
	return ret;
}


int frame=0;

void midframe_cb() {
	if (!gfx_frame_done()) {
		int c=0;
		while(!gfx_frame_done()) {
			trace_run_cpu(100);
			c++;
		}
	}
	initials_handle_vram(vram);
	gfx_show(vram, pal, vga_ver, 607, vga_startaddr/(320/4));
	if (cb_raster_seg!=0) force_callback(cb_raster_seg, cb_raster_off, 0);
}

void vblank_end_cb() {
	frame++;
	schedule_add(midframe_cb, (1000000/HBL_FREQ)*cb_raster_int_line, 0, "midframe");
}

void vblank_evt_cb() {
	if (cb_blank_seg!=0) force_callback(cb_blank_seg, cb_blank_off, 0);
	schedule_add(vblank_end_cb, (1000000/HBL_FREQ)*20, 0, "vblank_end");
}

int dos_timer=0;
void pit_tick_evt_cb() {
	dos_timer++;
	cpu_addr_space_write8(0x46c, dos_timer);
	cpu_addr_space_write8(0x46c, dos_timer>>8);
	intcall86(8); //pit interrupt
}

//PF is a DOS-game in the sense that it expect the user to exit out of the program if they're
//done with it, and only writes the highscore to disk when that happens. In an embedded device,
//the user is way more likely to simply hard turn off the device. As such, we check every so 
//often to see if the highscores changed and write to NVS if it did.
//We get the address from when the highscores are read; PF does not move that data around so
//we can re-use it to keep track of the highscore state as PF knows it.
void check_hiscore() {
	if (hiscore_addr<0) return;

	hiscore_get(hiscore_file, hiscore_nvs_state);
	int changed=0;
	for (int i=0; i<64; i++) {
		uint8_t b=cpu_addr_space_read8(hiscore_addr+i);
		if (b!=hiscore_nvs_state[i]) {
			hiscore_nvs_state[i]=b;
			changed=1;
		}
	}
	if (changed) hiscore_put(hiscore_file, hiscore_nvs_state);
}


int optimize_segs[]={
	0x59800, 0x5F800, 0x10000, 0x5F400,
	0x59400, 0x21000, 0x5BC00, 0x5EC00,
	0x20C00, 0x60C00, 0x5B800, 0x5F000,
//	0x60800, 0x60400, 0x5FC00, 0x60000,
//	0x20800, 0x5A400, 0x5D000, 0x5E800,
	-1
};

#define ABS(x) (((x)>0)?(x):(-x))

void emu_run(const char *prg, const char *mod) {
	cpu_addr_space_init();
	init_intr_table();
	init_vga_mem();
	int r=music_load(mod);
	assert(r);

	schedule_add(vblank_evt_cb, 1000000/VBL_FREQ, 1, "vblank");
	schedule_add(check_hiscore, 1000000, 1, "hiscore");

	int len=load_mz(prg, 0x10000);
	pf_vars_init(0x10000, len);
	int i=0;
	while (optimize_segs[i]>=0) {
		cpu_addr_space_write8(optimize_segs[i], cpu_addr_space_read8(optimize_segs[i]));
		i++;
	}

	int old_vx=0, old_vy=0;
	while(1) {
		schedule_run(1000000/VBL_FREQ); //1 frame
#if DO_TRACE
		if (frame==59) trace_enable(1);
		if (frame==60) {
			while (cpu.ifl==0) trace_run_cpu(10);
			key_pressed=INPUT_F1;
			intcall86(9);
		}
#endif

//		printf("hor %d ver %d start addr %d\n", vga_hor, vga_ver, vga_startaddr);
//		cpu_addr_space_dump();
		if (music_has_looped() && cb_jingle_seg!=0) {
			printf("Music has looped. Doing jingle callback...\n");
			uint16_t ret=force_callback(cb_jingle_seg, cb_jingle_off, 0);
			if (ret!=0) music_set_sequence_pos(ret&0xff);
			printf("Jingle callback returned 0x%X\n", ret);
		}
		int key;
		while((key=gfx_get_key())>0) {
			if (initials_handle_button(key)) {
				//don't handle key as initials handler already handled it
			} else if (key==INPUT_SPRING) {
				mouse_down=1;
			} else if (key==(INPUT_SPRING|INPUT_RELEASE)) {
				mouse_down=0;
				mouse_btn=1;
			} else if (key==INPUT_TEST) {
#if 1
				FILE *f=fopen("vram.bin", "w");
				fwrite(vram, 256*1024, 1, f);
				fclose(f);
				f=fopen("vram.pal", "w");
				fwrite(pal, 256*4, 1, f);
				fclose(f);
#endif
			} else {
				//wait till interrupts are enabled (not in int)
				while (cpu.ifl==0) trace_run_cpu(10);
				key_pressed=key;
				intcall86(9);
			}
		}
		int sc;
		while ((sc=initials_getscancode())>0) {
			while (cpu.ifl==0) trace_run_cpu(10);
			key_pressed=sc|INPUT_RAWSCANCODE;
			intcall86(9);
		}

		//Figure out if we need to have a haptic event.
		int vx, vy;
		pf_vars_get_ball_speed(&vx, &vy);
		int ax=(old_vx-vx)/20;
		int ay=(old_vy-vy)/20;
		if (ax>127) ax=127;
		if (ax<-127) ax=-127;
		if (ay>127) ay=127;
		if (ay<-127) ay=-127;
		if (ABS(ax)>20 || ABS(ay)>20) {
			haptic_event(HAPTIC_EVT_BALL, ax, ay);
		}
		old_vx=vx; old_vy=vy;
	}
	return;
}