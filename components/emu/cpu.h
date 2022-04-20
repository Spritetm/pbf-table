/*
  Fake86: A portable, open-source 8086 PC emulator.
  Copyright (C)2010-2012 Mike Chambers
            (C)2020      Gabor Lenart "LGB"

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef FAKE86_CPU_H_INCLUDED
#define FAKE86_CPU_H_INCLUDED

#include <stdint.h>


#ifdef CPU_ADDR_MODE_CACHE
extern uint64_t cached_access_count, uncached_access_count;
#endif

#define regax 0
#define regcx 1
#define regdx 2
#define regbx 3
#define regsp 4
#define regbp 5
#define regsi 6
#define regdi 7
#define reges 0
#define regcs 1
#define regss 2
#define regds 3

#ifdef __BIG_ENDIAN__
#define regal 1
#define regah 0
#define regcl 3
#define regch 2
#define regdl 5
#define regdh 4
#define regbl 7
#define regbh 6
#else
#define regal 0
#define regah 1
#define regcl 2
#define regch 3
#define regdl 4
#define regdh 5
#define regbl 6
#define regbh 7
#endif

union _bytewordregs_ {
	uint16_t wordregs[8];
	uint8_t byteregs[8];
};

//extern union _bytewordregs_ regs;

#ifdef CPU_ADDR_MODE_CACHE
struct addrmodecache_s {
	uint16_t exitcs;
	uint16_t exitip;
	uint16_t disp16;
	uint32_t len;
	uint8_t mode;
	uint8_t reg;
	uint8_t rm;
	uint8_t forcess;
	uint8_t valid;
};
#endif


#define RAM_SIZE 0x100000

extern uint8_t readonly[RAM_SIZE];
extern uint8_t running;
extern uint32_t makeupticks;
extern uint64_t totalexec;

extern uint16_t cpu_last_int_seg, cpu_last_int_ip;

struct cpu {
        uint8_t         cf, zf, pf, af, sf, tf, ifl, df, of;
	uint16_t	savecs, saveip, ip, useseg;
        int             hltstate;
        uint16_t        segregs[4];
	uint8_t		segoverride;
        union           _bytewordregs_ regs;
};

extern struct cpu cpu;

static inline uint16_t makeflagsword ( void )
{
	return 2 | (uint16_t) cpu.cf | ((uint16_t) cpu.pf << 2) | ((uint16_t) cpu.af << 4) | ((uint16_t) cpu.zf << 6) | ((uint16_t) cpu.sf << 7) |
		((uint16_t) cpu.tf << 8) | ((uint16_t) cpu.ifl << 9) | ((uint16_t) cpu.df << 10) | ((uint16_t) cpu.of << 11)
	;
}

static inline void decodeflagsword ( uint16_t x )
{
	cpu.cf  =  x        & 1;
	cpu.pf  = (x >>  2) & 1;
	cpu.af  = (x >>  4) & 1;
	cpu.zf  = (x >>  6) & 1;
	cpu.sf  = (x >>  7) & 1;
	cpu.tf  = (x >>  8) & 1;
	cpu.ifl = (x >>  9) & 1;
	cpu.df  = (x >> 10) & 1;
	cpu.of  = (x >> 11) & 1;
}

#define CPU_FL_CF	cpu.cf
#define CPU_FL_PF	cpu.pf
#define CPU_FL_AF	cpu.af
#define CPU_FL_ZF	cpu.zf
#define CPU_FL_SF	cpu.sf
#define CPU_FL_TF	cpu.tf
#define CPU_FL_IFL	cpu.ifl
#define CPU_FL_DF	cpu.df
#define CPU_FL_OF	cpu.of

#define CPU_CS		cpu.segregs[regcs]
#define CPU_DS		cpu.segregs[regds]
#define CPU_ES		cpu.segregs[reges]
#define CPU_SS		cpu.segregs[regss]

#define CPU_AX  	cpu.regs.wordregs[regax]
#define CPU_BX  	cpu.regs.wordregs[regbx]
#define CPU_CX  	cpu.regs.wordregs[regcx]
#define CPU_DX  	cpu.regs.wordregs[regdx]
#define CPU_SI  	cpu.regs.wordregs[regsi]
#define CPU_DI  	cpu.regs.wordregs[regdi]
#define CPU_BP  	cpu.regs.wordregs[regbp]
#define CPU_SP  	cpu.regs.wordregs[regsp]
#define CPU_IP		cpu.ip

#define CPU_AL  	cpu.regs.byteregs[regal]
#define CPU_BL  	cpu.regs.byteregs[regbl]
#define CPU_CL  	cpu.regs.byteregs[regcl]
#define CPU_DL  	cpu.regs.byteregs[regdl]
#define CPU_AH  	cpu.regs.byteregs[regah]
#define CPU_BH  	cpu.regs.byteregs[regbh]
#define CPU_CH  	cpu.regs.byteregs[regch]
#define CPU_DH  	cpu.regs.byteregs[regdh]

extern void     reset86  ( void );
//Returns the amount of loops left (in case of abort) or 0
extern int      exec86   ( uint32_t execloops );
extern void     intcall86(uint8_t intnum);
//extern void     write86  ( uint32_t addr32, uint8_t value );
//extern void     writew86  ( uint32_t addr32, uint16_t value );
//extern uint8_t  read86   ( uint32_t addr32 );
//extern uint16_t readw86   ( uint32_t addr32 );
extern void     cpu_push ( uint16_t pushval );
extern uint16_t cpu_pop  ( void );
extern void     cpu_IRET ( void );
extern int      cpu_hlt_handler ( void );
extern uint8_t portin(uint16_t port);
extern uint8_t portin16(uint16_t port);
extern void portout(uint16_t port, uint8_t val);
extern void portout16(uint16_t port, uint16_t val);

void exec86_abort();

#endif
