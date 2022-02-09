/*
  Fake86: A portable, open-source 8086 PC emulator.
  Copyright (C)2010-2013 Mike Chambers
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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
  USA.
*/

/* cpu.c: functions to emulate the 8086/V20 CPU in software. the heart of
 * Fake86. */

#ifndef CPU_INSTRUCTION_FLOW_CACHE

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"

#ifdef CPU_ADDR_MODE_CACHE
struct addrmodecache_s addrcache[0x100000];
uint8_t addrcachevalid[0x100000];
uint32_t addrdatalen, dataisvalid, setvalidptr;
uint64_t cached_access_count = 0, uncached_access_count = 0;
#endif

//static uint64_t curtimer, lasttimer;
//static uint64_t timerfreq;

static uint8_t byteregtable[8] = {regal, regcl, regdl, regbl, regah, regch, regdh, regbh};

static const uint8_t parity[0x100] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};



struct cpu cpu;


uint8_t readonly[RAM_SIZE];
//static uint8_t hltstate = 0;
//static uint8_t /*opcode,*/ cpu.segoverride /*, reptype */;
//uint16_t cpu.segregs[4];
//static uint16_t cpu.savecs, cpu.saveip, ip, cpu.useseg;
//uint8_t cf, zf;
//static uint8_t pf, af, sf, tf, ifl, df, of;
static uint8_t mode, reg, rm;
static uint16_t oper1, oper2, res16, disp16, stacksize, frametemp;
static uint8_t oper1b, oper2b, res8, nestlev, addrbyte;
//static uint8_t disp8;	// this seems not to be used ever, just setting it ...
//static uint8_t temp8;
//static uint32_t *temp1, temp2, temp3, tempaddr32;
static uint32_t ea;
//static uint32_t temp4, temp5, temp32;
//static int32_t result;
uint64_t totalexec;

union _bytewordregs_ regs;

uint8_t running = 1;
static uint8_t /*verbose,*/ didbootstrap = 0;
//static uint8_t debugmode, showcsip, /*verbose,*/ mouseemu;
//uint8_t ethif;

void intcall86(uint8_t intnum);

#define StepIP(x)		cpu.ip += (x)
#define getmem8(x, y)		read86(segbase(x) + (y))
#define getmem16(x, y)		readw86(segbase(x) + (y))
#define putmem8(x, y, z)	write86(segbase(x) + (y), (z))
#define putmem16(x, y, z)	writew86(segbase(x) + (y), (z))
#define signext(value)		(int16_t)(int8_t)(value)
#define signext32(value)	(int32_t)(int16_t)(value)
#define getsegreg(regid)	cpu.segregs[regid]
#define putsegreg(regid, wv)    cpu.segregs[regid] = (wv)
#define segbase(x)		((uint32_t)(x) << 4)

#define getreg16(regid)                 cpu.regs.wordregs[regid]
#define getreg8(regid)                  cpu.regs.byteregs[byteregtable[regid]]
#define setreg16(regid, writeval)       cpu.regs.wordregs[regid] = (writeval)
#define setreg8(regid, writeval)        cpu.regs.byteregs[byteregtable[regid]] = (writeval)


static inline void flag_szp8(uint8_t value) {
	cpu.zf = value ? 0 : 1;
	cpu.sf = value >> 7;
	cpu.pf = parity[value];
}

static inline void flag_szp16(uint16_t value) {
	cpu.zf = value ? 0 : 1;
	cpu.sf = value >> 15;
	cpu.pf = parity[value & 255];
}

static inline void flag_log8(uint8_t value) {
	flag_szp8(value);
	cpu.cf = 0;
	cpu.of = 0; /* bitwise logic ops always clear carry and overflow */
}

static inline void flag_log16(uint16_t value) {
	flag_szp16(value);
	cpu.cf = 0;
	cpu.of = 0; /* bitwise logic ops always clear carry and overflow */
}

static inline void flag_adc8(uint8_t v1, uint8_t v2, uint8_t v3) {

	/* v1 = destination operand, v2 = source operand, v3 = carry flag */
	uint16_t dst;

	dst = (uint16_t)v1 + (uint16_t)v2 + (uint16_t)v3;
	flag_szp8((uint8_t)dst);
	if (((dst ^ v1) & (dst ^ v2) & 0x80) == 0x80) {
		cpu.of = 1;
	} else {
		cpu.of = 0; /* set or clear overflow flag */
	}

	if (dst & 0xFF00) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0; /* set or clear carry flag */
	}

	if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0; /* set or clear auxilliary flag */
	}
}

static inline void flag_adc16(uint16_t v1, uint16_t v2, uint16_t v3) {

	uint32_t dst;

	dst = (uint32_t)v1 + (uint32_t)v2 + (uint32_t)v3;
	flag_szp16((uint16_t)dst);
	if ((((dst ^ v1) & (dst ^ v2)) & 0x8000) == 0x8000) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if (dst & 0xFFFF0000) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static inline void flag_add8(uint8_t v1, uint8_t v2) {
	/* v1 = destination operand, v2 = source operand */
	uint16_t dst;

	dst = (uint16_t)v1 + (uint16_t)v2;
	flag_szp8((uint8_t)dst);
	if (dst & 0xFF00) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if (((dst ^ v1) & (dst ^ v2) & 0x80) == 0x80) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static inline void flag_add16(uint16_t v1, uint16_t v2) {
	/* v1 = destination operand, v2 = source operand */
	uint32_t dst;

	dst = (uint32_t)v1 + (uint32_t)v2;
	flag_szp16((uint16_t)dst);
	if (dst & 0xFFFF0000) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if (((dst ^ v1) & (dst ^ v2) & 0x8000) == 0x8000) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static inline void flag_sbb8(uint8_t v1, uint8_t v2, uint8_t v3) {

	/* v1 = destination operand, v2 = source operand, v3 = carry flag */
	uint16_t dst;

	v2 += v3;
	dst = (uint16_t)v1 - (uint16_t)v2;
	flag_szp8((uint8_t)dst);
	if (dst & 0xFF00) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if ((dst ^ v1) & (v1 ^ v2) & 0x80) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if ((v1 ^ v2 ^ dst) & 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static inline void flag_sbb16(uint16_t v1, uint16_t v2, uint16_t v3) {

	/* v1 = destination operand, v2 = source operand, v3 = carry flag */
	uint32_t dst;

	v2 += v3;
	dst = (uint32_t)v1 - (uint32_t)v2;
	flag_szp16((uint16_t)dst);
	if (dst & 0xFFFF0000) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if ((dst ^ v1) & (v1 ^ v2) & 0x8000) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if ((v1 ^ v2 ^ dst) & 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static void flag_sub8(uint8_t v1, uint8_t v2) {

	/* v1 = destination operand, v2 = source operand */
	uint16_t dst;

	dst = (uint16_t)v1 - (uint16_t)v2;
	flag_szp8((uint8_t)dst);
	if (dst & 0xFF00) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if ((dst ^ v1) & (v1 ^ v2) & 0x80) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if ((v1 ^ v2 ^ dst) & 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static void flag_sub16(uint16_t v1, uint16_t v2) {

	/* v1 = destination operand, v2 = source operand */
	uint32_t dst;

	dst = (uint32_t)v1 - (uint32_t)v2;
	flag_szp16((uint16_t)dst);
	if (dst & 0xFFFF0000) {
		cpu.cf = 1;
	} else {
		cpu.cf = 0;
	}

	if ((dst ^ v1) & (v1 ^ v2) & 0x8000) {
		cpu.of = 1;
	} else {
		cpu.of = 0;
	}

	if ((v1 ^ v2 ^ dst) & 0x10) {
		cpu.af = 1;
	} else {
		cpu.af = 0;
	}
}

static inline void op_adc8(void) {
	res8 = oper1b + oper2b + cpu.cf;
	flag_adc8(oper1b, oper2b, cpu.cf);
}

static inline void op_adc16(void) {
	res16 = oper1 + oper2 + cpu.cf;
	flag_adc16(oper1, oper2, cpu.cf);
}

static inline void op_add8(void) {
	res8 = oper1b + oper2b;
	flag_add8(oper1b, oper2b);
}

static inline void op_add16(void) {
	res16 = oper1 + oper2;
	flag_add16(oper1, oper2);
}

static inline void op_and8(void) {
	res8 = oper1b & oper2b;
	flag_log8(res8);
}

static inline void op_and16(void) {
	res16 = oper1 & oper2;
	flag_log16(res16);
}

static inline void op_or8(void) {
	res8 = oper1b | oper2b;
	flag_log8(res8);
}

static inline void op_or16(void) {
	res16 = oper1 | oper2;
	flag_log16(res16);
}

static inline void op_xor8(void) {
	res8 = oper1b ^ oper2b;
	flag_log8(res8);
}

static inline void op_xor16(void) {
	res16 = oper1 ^ oper2;
	flag_log16(res16);
}

static inline void op_sub8(void) {
	res8 = oper1b - oper2b;
	flag_sub8(oper1b, oper2b);
}

static inline void op_sub16(void) {
	res16 = oper1 - oper2;
	flag_sub16(oper1, oper2);
}

static inline void op_sbb8(void) {
	res8 = oper1b - (oper2b + cpu.cf);
	flag_sbb8(oper1b, oper2b, cpu.cf);
}

static inline void op_sbb16(void) {
	res16 = oper1 - (oper2 + cpu.cf);
	flag_sbb16(oper1, oper2, cpu.cf);
}

static inline void getea(uint8_t rmval) {
	uint32_t tempea;

	tempea = 0;
	switch (mode) {
	case 0:
		switch (rmval) {
		case 0:
			tempea = cpu.regs.wordregs[regbx] + cpu.regs.wordregs[regsi];
			break;
		case 1:
			tempea = cpu.regs.wordregs[regbx] + cpu.regs.wordregs[regdi];
			break;
		case 2:
			tempea = cpu.regs.wordregs[regbp] + cpu.regs.wordregs[regsi];
			break;
		case 3:
			tempea = cpu.regs.wordregs[regbp] + cpu.regs.wordregs[regdi];
			break;
		case 4:
			tempea = cpu.regs.wordregs[regsi];
			break;
		case 5:
			tempea = cpu.regs.wordregs[regdi];
			break;
		case 6:
			tempea = disp16;
			break;
		case 7:
			tempea = cpu.regs.wordregs[regbx];
			break;
		}
		break;

	case 1:
	case 2:
		switch (rmval) {
		case 0:
			tempea = cpu.regs.wordregs[regbx] + cpu.regs.wordregs[regsi] + disp16;
			break;
		case 1:
			tempea = cpu.regs.wordregs[regbx] + cpu.regs.wordregs[regdi] + disp16;
			break;
		case 2:
			tempea = cpu.regs.wordregs[regbp] + cpu.regs.wordregs[regsi] + disp16;
			break;
		case 3:
			tempea = cpu.regs.wordregs[regbp] + cpu.regs.wordregs[regdi] + disp16;
			break;
		case 4:
			tempea = cpu.regs.wordregs[regsi] + disp16;
			break;
		case 5:
			tempea = cpu.regs.wordregs[regdi] + disp16;
			break;
		case 6:
			tempea = cpu.regs.wordregs[regbp] + disp16;
			break;
		case 7:
			tempea = cpu.regs.wordregs[regbx] + disp16;
			break;
		}
		break;
	}

	ea = (tempea & 0xFFFF) + (cpu.useseg << 4);
}

static void push(uint16_t pushval) {
	cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regsp] - 2;
	putmem16(cpu.segregs[regss], cpu.regs.wordregs[regsp], pushval);
}

void cpu_push ( uint16_t pushval )
{
	push(pushval);
}


static uint16_t pop(void) {

	uint16_t tempval;

	tempval = getmem16(cpu.segregs[regss], cpu.regs.wordregs[regsp]);
	cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regsp] + 2;
	return tempval;
}

uint16_t cpu_pop ( void )
{
	return pop();
}


void cpu_IRET ( void )
{
	cpu.ip = pop();
	cpu.segregs[regcs] = pop();
	decodeflagsword(pop());
}


void reset86(void) {
	cpu.segregs[regcs] = 0xFFFF;
	cpu.ip = 0x0000;
	cpu.hltstate = 0;
}

static uint16_t readrm16(uint8_t rmval) {
	if (mode < 3) {
		getea(rmval);
		return read86(ea) | ((uint16_t)read86(ea + 1) << 8);
	} else {
		return getreg16(rmval);
	}
}

static uint8_t readrm8(uint8_t rmval) {
	if (mode < 3) {
		getea(rmval);
		return read86(ea);
	} else {
		return getreg8(rmval);
	}
}

static void writerm16(uint8_t rmval, uint16_t value) {
	if (mode < 3) {
		getea(rmval);
		write86(ea, value & 0xFF);
		write86(ea + 1, value >> 8);
	} else {
		setreg16(rmval, value);
	}
}

static void writerm8(uint8_t rmval, uint8_t value) {
	if (mode < 3) {
		getea(rmval);
		write86(ea, value);
	} else {
		setreg8(rmval, value);
	}
}

static uint8_t op_grp2_8(uint8_t cnt) {

	uint16_t s = oper1b;
#ifdef CPU_LIMIT_SHIFT_COUNT
	cnt &= 0x1F;
#endif
	switch (reg) {
	case 0: /* ROL r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			if (s & 0x80) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = s << 1;
			s = s | cpu.cf;
		}

		if (cnt == 1) {
			// of = cpu.cf ^ ( (s >> 7) & 1);
			if ((s & 0x80) && cpu.cf)
				cpu.of = 1;
			else
				cpu.of = 0;
		} else
			cpu.of = 0;
		break;

	case 1: /* ROR r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			cpu.cf = s & 1;
			s = (s >> 1) | (cpu.cf << 7);
		}

		if (cnt == 1) {
			cpu.of = (s >> 7) ^ ((s >> 6) & 1);
		}
		break;

	case 2: /* RCL r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			int oldcf = cpu.cf;
			if (s & 0x80) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = s << 1;
			s = s | oldcf;
		}

		if (cnt == 1) {
			cpu.of = cpu.cf ^ ((s >> 7) & 1);
		}
		break;

	case 3: /* RCR r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			int oldcf = cpu.cf;
			cpu.cf = s & 1;
			s = (s >> 1) | (oldcf << 7);
		}

		if (cnt == 1) {
			cpu.of = (s >> 7) ^ ((s >> 6) & 1);
		}
		break;

	case 4:
	case 6: /* SHL r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			if (s & 0x80) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = (s << 1) & 0xFF;
		}

		if ((cnt == 1) && (cpu.cf == (s >> 7))) {
			cpu.of = 0;
		} else {
			cpu.of = 1;
		}

		flag_szp8((uint8_t)s);
		break;

	case 5: /* SHR r/m8 */
		if ((cnt == 1) && (s & 0x80)) {
			cpu.of = 1;
		} else {
			cpu.of = 0;
		}

		for (int a = 1; a <= cnt; a++) {
			cpu.cf = s & 1;
			s = s >> 1;
		}

		flag_szp8((uint8_t)s);
		break;

	case 7: /* SAR r/m8 */
		for (int a = 1; a <= cnt; a++) {
			unsigned int msb = s & 0x80;
			cpu.cf = s & 1;
			s = (s >> 1) | msb;
		}

		cpu.of = 0;
		flag_szp8((uint8_t)s);
		break;
	}

	return s & 0xFF;
}

static uint16_t op_grp2_16(uint8_t cnt) {

	uint32_t s = oper1;
#ifdef CPU_LIMIT_SHIFT_COUNT
	cnt &= 0x1F;
#endif
	switch (reg) {
	case 0: /* ROL r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			if (s & 0x8000) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = s << 1;
			s = s | cpu.cf;
		}

		if (cnt == 1) {
			cpu.of = cpu.cf ^ ((s >> 15) & 1);
		}
		break;

	case 1: /* ROR r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			cpu.cf = s & 1;
			s = (s >> 1) | (cpu.cf << 15);
		}

		if (cnt == 1) {
			cpu.of = (s >> 15) ^ ((s >> 14) & 1);
		}
		break;

	case 2: /* RCL r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			int oldcf = cpu.cf;
			if (s & 0x8000) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = s << 1;
			s = s | oldcf;
		}

		if (cnt == 1) {
			cpu.of = cpu.cf ^ ((s >> 15) & 1);
		}
		break;

	case 3: /* RCR r/m8 */
		for (int shift = 1; shift <= cnt; shift++) {
			int oldcf = cpu.cf;
			cpu.cf = s & 1;
			s = (s >> 1) | (oldcf << 15);
		}

		if (cnt == 1) {
			cpu.of = (s >> 15) ^ ((s >> 14) & 1);
		}
		break;

	case 4:
	case 6: /* SHL r/m8 */
		for (unsigned int shift = 1; shift <= cnt; shift++) {
			if (s & 0x8000) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			s = (s << 1) & 0xFFFF;
		}

		if ((cnt == 1) && (cpu.cf == (s >> 15))) {
			cpu.of = 0;
		} else {
			cpu.of = 1;
		}

		flag_szp16((uint16_t)s);
		break;

	case 5: /* SHR r/m8 */
		if ((cnt == 1) && (s & 0x8000)) {
			cpu.of = 1;
		} else {
			cpu.of = 0;
		}

		for (int shift = 1; shift <= cnt; shift++) {
			cpu.cf = s & 1;
			s = s >> 1;
		}

		flag_szp16((uint16_t)s);
		break;

	case 7: /* SAR r/m8 */
		for (int shift = 1, msb; shift <= cnt; shift++) {
			msb = s & 0x8000;
			cpu.cf = s & 1;
			s = (s >> 1) | msb;
		}

		cpu.of = 0;
		flag_szp16((uint16_t)s);
		break;
	}

	return (uint16_t)s & 0xFFFF;
}

static inline void op_div8(uint16_t valdiv, uint8_t divisor) {
	if (divisor == 0) {
		intcall86(0);
		return;
	}

	if ((valdiv / (uint16_t)divisor) > 0xFF) {
		intcall86(0);
		return;
	}

	cpu.regs.byteregs[regah] = valdiv % (uint16_t)divisor;
	cpu.regs.byteregs[regal] = valdiv / (uint16_t)divisor;
}

static inline void op_idiv8(uint16_t valdiv, uint8_t divisor) {

	uint16_t s1;
	uint16_t s2;
	uint16_t d1;
	uint16_t d2;
	int sign;

	if (divisor == 0) {
		intcall86(0);
		return;
	}

	s1 = valdiv;
	s2 = divisor;
	sign = (((s1 ^ s2) & 0x8000) != 0);
	s1 = (s1 < 0x8000) ? s1 : ((~s1 + 1) & 0xffff);
	s2 = (s2 < 0x8000) ? s2 : ((~s2 + 1) & 0xffff);
	d1 = s1 / s2;
	d2 = s1 % s2;
	if (d1 & 0xFF00) {
		intcall86(0);
		return;
	}

	if (sign) {
		d1 = (~d1 + 1) & 0xff;
		d2 = (~d2 + 1) & 0xff;
	}

	cpu.regs.byteregs[regah] = (uint8_t)d2;
	cpu.regs.byteregs[regal] = (uint8_t)d1;
}

static inline void op_grp3_8(void) {
	oper1 = signext(oper1b);
	oper2 = signext(oper2b);
	switch (reg) {
	case 0:
	case 1: /* TEST */
		flag_log8(oper1b & getmem8(cpu.segregs[regcs], cpu.ip));
		StepIP(1);
		break;

	case 2: /* NOT */
		res8 = ~oper1b;
		break;

	case 3: /* NEG */
		res8 = (~oper1b) + 1;
		flag_sub8(0, oper1b);
		if (res8 == 0) {
			cpu.cf = 0;
		} else {
			cpu.cf = 1;
		}
		break;

	case 4: /* MUL */
		{
			uint32_t temp1 = (uint32_t)oper1b * (uint32_t)cpu.regs.byteregs[regal];
			cpu.regs.wordregs[regax] = temp1 & 0xFFFF;
			flag_szp8((uint8_t)temp1);
			if (cpu.regs.byteregs[regah]) {
				cpu.cf = 1;
				cpu.of = 1;
			} else {
				cpu.cf = 0;
				cpu.of = 0;
			}
#ifdef CPU_CLEAR_ZF_ON_MUL
			cpu.zf = 0;
#endif
		}
		break;

	case 5: /* IMUL */
		{
			oper1 = signext(oper1b);
			uint32_t temp1 = signext(cpu.regs.byteregs[regal]);
			uint32_t temp2 = oper1;
			if ((temp1 & 0x80) == 0x80) {
				temp1 = temp1 | 0xFFFFFF00;
			}
			if ((temp2 & 0x80) == 0x80) {
				temp2 = temp2 | 0xFFFFFF00;
			}
			uint32_t temp3 = (temp1 * temp2) & 0xFFFF;
			cpu.regs.wordregs[regax] = temp3 & 0xFFFF;
			if (cpu.regs.byteregs[regah]) {
				cpu.cf = 1;
				cpu.of = 1;
			} else {
				cpu.cf = 0;
				cpu.of = 0;
			}
#ifdef CPU_CLEAR_ZF_ON_MUL
			cpu.zf = 0;
#endif
		}
		break;

	case 6: /* DIV */
		op_div8(cpu.regs.wordregs[regax], oper1b);
		break;

	case 7: /* IDIV */
		op_idiv8(cpu.regs.wordregs[regax], oper1b);
		break;
	}
}

static void op_div16(uint32_t valdiv, uint16_t divisor) {
	if (divisor == 0) {
		intcall86(0);
		return;
	}

	if ((valdiv / (uint32_t)divisor) > 0xFFFF) {
		intcall86(0);
		return;
	}

	cpu.regs.wordregs[regdx] = valdiv % (uint32_t)divisor;
	cpu.regs.wordregs[regax] = valdiv / (uint32_t)divisor;
}

static void op_idiv16(uint32_t valdiv, uint16_t divisor) {

	uint32_t d1;
	uint32_t d2;
	uint32_t s1;
	uint32_t s2;
	int sign;

	if (divisor == 0) {
		intcall86(0);
		return;
	}

	s1 = valdiv;
	s2 = divisor;
	s2 = (s2 & 0x8000) ? (s2 | 0xffff0000) : s2;
	sign = (((s1 ^ s2) & 0x80000000) != 0);
	s1 = (s1 < 0x80000000) ? s1 : ((~s1 + 1) & 0xffffffff);
	s2 = (s2 < 0x80000000) ? s2 : ((~s2 + 1) & 0xffffffff);
	d1 = s1 / s2;
	d2 = s1 % s2;
	if (d1 & 0xFFFF0000) {
		intcall86(0);
		return;
	}

	if (sign) {
		d1 = (~d1 + 1) & 0xffff;
		d2 = (~d2 + 1) & 0xffff;
	}

	cpu.regs.wordregs[regax] = d1;
	cpu.regs.wordregs[regdx] = d2;
}

static inline void op_grp3_16(void) {
	switch (reg) {
	case 0:
	case 1: /* TEST */
		flag_log16(oper1 & getmem16(cpu.segregs[regcs], cpu.ip));
		StepIP(2);
		break;

	case 2: /* NOT */
		res16 = ~oper1;
		break;

	case 3: /* NEG */
		res16 = (~oper1) + 1;
		flag_sub16(0, oper1);
		if (res16) {
			cpu.cf = 1;
		} else {
			cpu.cf = 0;
		}
		break;

	case 4: /* MUL */
		{
			uint32_t temp1 = (uint32_t)oper1 * (uint32_t)cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = temp1 & 0xFFFF;
			cpu.regs.wordregs[regdx] = temp1 >> 16;
			flag_szp16((uint16_t)temp1);
			if (cpu.regs.wordregs[regdx]) {
				cpu.cf = 1;
				cpu.of = 1;
			} else {
				cpu.cf = 0;
				cpu.of = 0;
			}
#ifdef CPU_CLEAR_ZF_ON_MUL
			cpu.zf = 0;
#endif
		}
		break;

	case 5: /* IMUL */
		{
			uint32_t temp1 = cpu.regs.wordregs[regax];
			uint32_t temp2 = oper1;
			if (temp1 & 0x8000) {
				temp1 |= 0xFFFF0000;
			}
			if (temp2 & 0x8000) {
				temp2 |= 0xFFFF0000;
			}
			uint32_t temp3 = temp1 * temp2;
			cpu.regs.wordregs[regax] = temp3 & 0xFFFF; /* into register ax */
			cpu.regs.wordregs[regdx] = temp3 >> 16;    /* into register dx */
			if (cpu.regs.wordregs[regdx]) {
				cpu.cf = 1;
				cpu.of = 1;
			} else {
				cpu.cf = 0;
				cpu.of = 0;
			}
#ifdef CPU_CLEAR_ZF_ON_MUL
			cpu.zf = 0;
#endif
		}
		break;

	case 6: /* DIV */
		op_div16(((uint32_t)cpu.regs.wordregs[regdx] << 16) +
			     cpu.regs.wordregs[regax],
			 oper1);
		break;

	case 7: /* DIV */
		op_idiv16(((uint32_t)cpu.regs.wordregs[regdx] << 16) +
			      cpu.regs.wordregs[regax],
			  oper1);
		break;
	}
}

static inline void op_grp5(void) {
	switch (reg) {
		case 0: /* INC Ev */
			{
				oper2 = 1;
				int tempcf = cpu.cf;
				op_add16();
				cpu.cf = tempcf;
				writerm16(rm, res16);
			}
			break;

		case 1: /* DEC Ev */
			{
				oper2 = 1;
				int tempcf = cpu.cf;
				op_sub16();
				cpu.cf = tempcf;
				writerm16(rm, res16);
			}
			break;

		case 2: /* CALL Ev */
			push(cpu.ip);
			cpu.ip = oper1;
			break;

		case 3: /* CALL Mp */
			push(cpu.segregs[regcs]);
			push(cpu.ip);
			getea(rm);
			cpu.ip = (uint16_t)read86(ea) + (uint16_t)read86(ea + 1) * 256;
			cpu.segregs[regcs] =
			    (uint16_t)read86(ea + 2) + (uint16_t)read86(ea + 3) * 256;
			break;

		case 4: /* JMP Ev */
			cpu.ip = oper1;
			break;

		case 5: /* JMP Mp */
			getea(rm);
			cpu.ip = (uint16_t)read86(ea) + (uint16_t)read86(ea + 1) * 256;
			cpu.segregs[regcs] =
			    (uint16_t)read86(ea + 2) + (uint16_t)read86(ea + 3) * 256;
			break;

		case 6: /* PUSH Ev */
			push(oper1);
			break;
	}
}

//static uint8_t didintr = 0;
//static uint8_t dolog = 0;
//static FILE *logout;
//static uint8_t printops = 0;

uint16_t cpu_last_int_seg, cpu_last_int_ip;

void intcall86(uint8_t intnum) {
	cpu_last_int_seg = cpu.segregs[regcs];
	cpu_last_int_ip  = cpu.saveip;	// LGB

	push(makeflagsword());
	push(cpu.segregs[regcs]);
	push(cpu.ip);
	cpu.segregs[regcs] = getmem16(0, (uint16_t)intnum * 4 + 2);
	cpu.ip = getmem16(0, (uint16_t)intnum * 4);
	cpu.ifl = 0;
	cpu.tf = 0;
}

//static uint64_t frametimer = 0, didwhen = 0, didticks = 0;
uint32_t makeupticks = 0;
//static uint64_t timerticks = 0, realticks = 0;
//static uint64_t counterticks = 10000;
//static uint64_t lastcountertimer = 0;


static void modregrm ( void )
{
#ifdef CPU_ADDR_MODE_CACHE
	tempaddr32 = (((uint32_t)cpu.savecs << 4) + cpu.ip) & 0xFFFFF;
	if (addrcachevalid[tempaddr32]) {
		switch (addrcache[tempaddr32].len) {
			case 0:
				dataisvalid = 1;
				break;
			case 1:
				if (addrcachevalid[tempaddr32+1])
					dataisvalid = 1;
				else
					dataisvalid = 0;
				break;
			case 2:
				if (addrcachevalid[tempaddr32+1] && addrcachevalid[tempaddr32+2])
					dataisvalid = 1;
				else
					dataisvalid = 0;
				break;
		}
	} else
		dataisvalid = 0;
	if (dataisvalid) {
		cached_access_count++;
		disp16 = addrcache[tempaddr32].disp16;
		cpu.segregs[regcs] = addrcache[tempaddr32].exitcs;
		cpu.ip = addrcache[tempaddr32].exitip;
		mode = addrcache[tempaddr32].mode;
		reg = addrcache[tempaddr32].reg;
		rm = addrcache[tempaddr32].rm;
		if ((!cpu.segoverride) && addrcache[tempaddr32].forcess)
			cpu.useseg = cpu.segregs[regss];
	} else {
		uncached_access_count++;
		addrbyte = getmem8(cpu.segregs[regcs], cpu.ip);
		StepIP(1);
		mode = addrbyte >> 6;
		reg = (addrbyte >> 3) & 7;
		rm = addrbyte & 7;
		addrdatalen = 0;
		addrcache[tempaddr32].forcess = 0;
		switch (mode) {
			case 0:
				if(rm == 6) {
					disp16 = getmem16(cpu.segregs[regcs], cpu.ip);
					addrdatalen = 2;
					StepIP(2);
				}
				if ((rm == 2) || (rm == 3)) {
					if (!cpu.segoverride)
						cpu.useseg = cpu.segregs[regss];
					addrcache[tempaddr32].forcess = 1;
				}
				break;
			case 1:
				disp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				addrdatalen = 1;
				StepIP(1);
				if ((rm == 2) || (rm == 3) || (rm == 6)) {
					if (!cpu.segoverride)
						cpu.useseg = cpu.segregs[regss];
					addrcache[tempaddr32].forcess = 1;
				}
				break;
			case 2:
				disp16 = getmem16(cpu.segregs[regcs], cpu.ip);
				addrdatalen = 2;
				StepIP(2);
				if ((rm == 2) || (rm == 3) || (rm == 6)) {
					if (!cpu.segoverride)
						cpu.useseg = cpu.segregs[regss];
					addrcache[tempaddr32].forcess = 1;
				}
				break;
			default:
				disp16 = 0;
				break;
		}
		addrcache[tempaddr32].disp16 = disp16;
		addrcache[tempaddr32].exitcs = cpu.segregs[regcs];
		addrcache[tempaddr32].exitip = cpu.ip;
		addrcache[tempaddr32].mode = mode;
		addrcache[tempaddr32].reg = reg;
		addrcache[tempaddr32].rm = rm;
		addrcache[tempaddr32].len = addrdatalen;
		memset(&addrcachevalid[tempaddr32], 1, addrdatalen+1);
	}
#else
	addrbyte = getmem8(cpu.segregs[regcs], cpu.ip);
	StepIP(1);
	mode = addrbyte >> 6;
	reg = (addrbyte >> 3) & 7;
	rm = addrbyte & 7;
	switch (mode) {
		case 0:
			if (rm == 6) {
				disp16 = getmem16(cpu.segregs[regcs], cpu.ip);
				StepIP(2);
			}
			if (((rm == 2) || (rm == 3)) && !cpu.segoverride) {
				cpu.useseg = cpu.segregs[regss];
			}
			break;
		case 1:
			disp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
			StepIP(1);
			if (((rm == 2) || (rm == 3) || (rm == 6)) && !cpu.segoverride) {
				cpu.useseg = cpu.segregs[regss];
			}
			break;
		case 2:
			disp16 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			if (((rm == 2) || (rm == 3) || (rm == 6)) && !cpu.segoverride) {
				cpu.useseg = cpu.segregs[regss];
			}
			break;
		default:
			/* disp8 = 0; <-- this seems not to be used ever! */
			disp16 = 0;
			break;
	}
#endif
}


void exec86_abort() {
	running=0;
}

void exec86(uint32_t execloops) {

	uint8_t docontinue;
	static uint16_t firstip;
	static uint16_t trap_toggle = 0;
	running=1;

	// This seems not to be used anywhere, so commented out for now.
	//counterticks = (uint64_t)((double)timerfreq / (double)65536.0);

	for (uint32_t loopcount = 0; loopcount < execloops; loopcount++) {

		if ((totalexec & TIMING_INTERVAL) == 0)
			timing();

		if (trap_toggle) {
			intcall86(1);
		}

		if (cpu.tf) {
			trap_toggle = 1;
		} else {
			trap_toggle = 0;
		}

		//todo: intr logic
		//intcall86(nextintr());


		if (cpu.hltstate) {
			puts("CPU: HALTED!!!!!");
			goto skipexecution;
		}

		/*if ((((uint32_t)cpu.segregs[regcs] << 4) + (uint32_t)cpu.ip) ==
		   0xFEC59) {
				//printf("Entered F000:EC59, returning to ");
				cpu.ip = pop();
				cpu.segregs[regcs] = pop();
				decodeflagsword(pop());
				//printf("%04X:%04X\n", cpu.segregs[regcs], cpu.ip);
				diskhandler();
			}*/

		int reptype = 0;
		cpu.segoverride = 0;
		cpu.useseg = cpu.segregs[regds];
		docontinue = 0;
		firstip = cpu.ip;

		if ((cpu.segregs[regcs] == 0xF000) && (cpu.ip == 0xE066))
			didbootstrap =
			    0; // detect if we hit the BIOS entry point to clear
			       // didbootstrap because we've rebooted

		uint8_t opcode;
		while (!docontinue) {
			cpu.segregs[regcs] = cpu.segregs[regcs] & 0xFFFF;
			cpu.ip = cpu.ip & 0xFFFF;
			cpu.savecs = cpu.segregs[regcs];
			cpu.saveip = cpu.ip;
			//printf("EXEC @ %04X:%04X cpu.saveip=%04X\n", cpu.segregs[regcs], cpu.ip, cpu.saveip);
			opcode = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);

			switch (opcode) {
				/* segment prefix check */
			case 0x2E: /* segment cpu.segregs[regcs] */
				cpu.useseg = cpu.segregs[regcs];
				cpu.segoverride = 1;
				break;

			case 0x3E: /* segment cpu.segregs[regds] */
				cpu.useseg = cpu.segregs[regds];
				cpu.segoverride = 1;
				break;

			case 0x26: /* segment cpu.segregs[reges] */
				cpu.useseg = cpu.segregs[reges];
				cpu.segoverride = 1;
				break;

			case 0x36: /* segment cpu.segregs[regss] */
				cpu.useseg = cpu.segregs[regss];
				cpu.segoverride = 1;
				break;

				/* repetition prefix check */
			case 0xF3: /* REP/REPE/REPZ */
				reptype = 1;
				break;

			case 0xF2: /* REPNE/REPNZ */
				reptype = 2;
				break;

			default:
				docontinue = 1;
				break;
			}
		}

		totalexec++;

		switch (opcode) {
		case 0x0: /* 00 ADD Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_add8();
			writerm8(rm, res8);
			break;

		case 0x1: /* 01 ADD Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_add16();
			writerm16(rm, res16);
			break;

		case 0x2: /* 02 ADD Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_add8();
			setreg8(reg, res8);
			break;

		case 0x3: /* 03 ADD Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_add16();
			setreg16(reg, res16);
			break;

		case 0x4: /* 04 ADD cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_add8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x5: /* 05 ADD eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_add16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x6: /* 06 PUSH cpu.segregs[reges] */
			push(cpu.segregs[reges]);
			break;

		case 0x7: /* 07 POP cpu.segregs[reges] */
			cpu.segregs[reges] = pop();
			break;

		case 0x8: /* 08 OR Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_or8();
			writerm8(rm, res8);
			break;

		case 0x9: /* 09 OR Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_or16();
			writerm16(rm, res16);
			break;

		case 0xA: /* 0A OR Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_or8();
			setreg8(reg, res8);
			break;

		case 0xB: /* 0B OR Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_or16();
			if ((oper1 == 0xF802) && (oper2 == 0xF802)) {
				cpu.sf = 0; /* cheap hack to make Wolf 3D think
					   we're a 286 so it plays */
			}

			setreg16(reg, res16);
			break;

		case 0xC: /* 0C OR cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_or8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0xD: /* 0D OR eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_or16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0xE: /* 0E PUSH cpu.segregs[regcs] */
			push(cpu.segregs[regcs]);
			break;

#ifdef CPU_ALLOW_POP_CS	  // only the 8086/8088 does this.
		case 0xF: // 0F POP CS
			cpu.segregs[regcs] = pop();
			break;
#endif

		case 0x10: /* 10 ADC Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_adc8();
			writerm8(rm, res8);
			break;

		case 0x11: /* 11 ADC Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_adc16();
			writerm16(rm, res16);
			break;

		case 0x12: /* 12 ADC Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_adc8();
			setreg8(reg, res8);
			break;

		case 0x13: /* 13 ADC Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_adc16();
			setreg16(reg, res16);
			break;

		case 0x14: /* 14 ADC cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_adc8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x15: /* 15 ADC eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_adc16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x16: /* 16 PUSH cpu.segregs[regss] */
			push(cpu.segregs[regss]);
			break;

		case 0x17: /* 17 POP cpu.segregs[regss] */
			cpu.segregs[regss] = pop();
			break;

		case 0x18: /* 18 SBB Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_sbb8();
			writerm8(rm, res8);
			break;

		case 0x19: /* 19 SBB Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_sbb16();
			writerm16(rm, res16);
			break;

		case 0x1A: /* 1A SBB Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_sbb8();
			setreg8(reg, res8);
			break;

		case 0x1B: /* 1B SBB Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_sbb16();
			setreg16(reg, res16);
			break;

		case 0x1C: /* 1C SBB cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_sbb8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x1D: /* 1D SBB eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_sbb16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x1E: /* 1E PUSH cpu.segregs[regds] */
			push(cpu.segregs[regds]);
			break;

		case 0x1F: /* 1F POP cpu.segregs[regds] */
			cpu.segregs[regds] = pop();
			break;

		case 0x20: /* 20 AND Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_and8();
			writerm8(rm, res8);
			break;

		case 0x21: /* 21 AND Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_and16();
			writerm16(rm, res16);
			break;

		case 0x22: /* 22 AND Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_and8();
			setreg8(reg, res8);
			break;

		case 0x23: /* 23 AND Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_and16();
			setreg16(reg, res16);
			break;

		case 0x24: /* 24 AND cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_and8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x25: /* 25 AND eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_and16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x27: /* 27 DAA */
			if (((cpu.regs.byteregs[regal] & 0xF) > 9) || (cpu.af == 1)) {
				oper1 = cpu.regs.byteregs[regal] + 6;
				cpu.regs.byteregs[regal] = oper1 & 255;
				if (oper1 & 0xFF00) {
					cpu.cf = 1;
				} else {
					cpu.cf = 0;
				}

				cpu.af = 1;
			} else {
				// cpu.af = 0;
			}

			if ((cpu.regs.byteregs[regal] > 0x9F) || (cpu.cf == 1)) {
				cpu.regs.byteregs[regal] =
				    cpu.regs.byteregs[regal] + 0x60;
				cpu.cf = 1;
			} else {
				// cpu.cf = 0;
			}

			cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] & 255;
			flag_szp8(cpu.regs.byteregs[regal]);
			break;

		case 0x28: /* 28 SUB Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_sub8();
			writerm8(rm, res8);
			break;

		case 0x29: /* 29 SUB Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_sub16();
			writerm16(rm, res16);
			break;

		case 0x2A: /* 2A SUB Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_sub8();
			setreg8(reg, res8);
			break;

		case 0x2B: /* 2B SUB Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_sub16();
			setreg16(reg, res16);
			break;

		case 0x2C: /* 2C SUB cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_sub8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x2D: /* 2D SUB eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_sub16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x2F: /* 2F DAS */
			if (((cpu.regs.byteregs[regal] & 15) > 9) || (cpu.af == 1)) {
				oper1 = cpu.regs.byteregs[regal] - 6;
				cpu.regs.byteregs[regal] = oper1 & 255;
				if (oper1 & 0xFF00) {
					cpu.cf = 1;
				} else {
					cpu.cf = 0;
				}

				cpu.af = 1;
			} else {
				cpu.af = 0;
			}

			if (((cpu.regs.byteregs[regal] & 0xF0) > 0x90) ||
			    (cpu.cf == 1)) {
				cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] - 0x60;
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}

			flag_szp8(cpu.regs.byteregs[regal]);
			break;

		case 0x30: /* 30 XOR Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			op_xor8();
			writerm8(rm, res8);
			break;

		case 0x31: /* 31 XOR Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			op_xor16();
			writerm16(rm, res16);
			break;

		case 0x32: /* 32 XOR Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			op_xor8();
			setreg8(reg, res8);
			break;

		case 0x33: /* 33 XOR Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			op_xor16();
			setreg16(reg, res16);
			break;

		case 0x34: /* 34 XOR cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			op_xor8();
			cpu.regs.byteregs[regal] = res8;
			break;

		case 0x35: /* 35 XOR eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			op_xor16();
			cpu.regs.wordregs[regax] = res16;
			break;

		case 0x37: /* 37 AAA ASCII */
			if (((cpu.regs.byteregs[regal] & 0xF) > 9) || (cpu.af == 1)) {
				cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] + 6;
				cpu.regs.byteregs[regah] = cpu.regs.byteregs[regah] + 1;
				cpu.af = 1;
				cpu.cf = 1;
			} else {
				cpu.af = 0;
				cpu.cf = 0;
			}

			cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] & 0xF;
			break;

		case 0x38: /* 38 CMP Eb Gb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getreg8(reg);
			flag_sub8(oper1b, oper2b);
			break;

		case 0x39: /* 39 CMP Ev Gv */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getreg16(reg);
			flag_sub16(oper1, oper2);
			break;

		case 0x3A: /* 3A CMP Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			flag_sub8(oper1b, oper2b);
			break;

		case 0x3B: /* 3B CMP Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			flag_sub16(oper1, oper2);
			break;

		case 0x3C: /* 3C CMP cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			flag_sub8(oper1b, oper2b);
			break;

		case 0x3D: /* 3D CMP eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			flag_sub16(oper1, oper2);
			break;

		case 0x3F: /* 3F AAS ASCII */
			if (((cpu.regs.byteregs[regal] & 0xF) > 9) || (cpu.af == 1)) {
				cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] - 6;
				cpu.regs.byteregs[regah] = cpu.regs.byteregs[regah] - 1;
				cpu.af = 1;
				cpu.cf = 1;
			} else {
				cpu.af = 0;
				cpu.cf = 0;
			}

			cpu.regs.byteregs[regal] = cpu.regs.byteregs[regal] & 0xF;
			break;

		case 0x40: /* 40 INC eAX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regax];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regax] = res16;
			}
			break;

		case 0x41: /* 41 INC eCX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regcx];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regcx] = res16;
			}
			break;

		case 0x42: /* 42 INC eDX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regdx];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regdx] = res16;
			}
			break;

		case 0x43: /* 43 INC eBX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regbx];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regbx] = res16;
			}
			break;

		case 0x44: /* 44 INC eSP */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regsp];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regsp] = res16;
			}
			break;

		case 0x45: /* 45 INC eBP */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regbp];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regbp] = res16;
			}
			break;

		case 0x46: /* 46 INC eSI */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regsi];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regsi] = res16;
			}
			break;

		case 0x47: /* 47 INC eDI */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regdi];
				oper2 = 1;
				op_add16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regdi] = res16;
			}
			break;

		case 0x48: /* 48 DEC eAX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regax];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regax] = res16;
			}
			break;

		case 0x49: /* 49 DEC eCX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regcx];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regcx] = res16;
			}
			break;

		case 0x4A: /* 4A DEC eDX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regdx];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regdx] = res16;
			}
			break;

		case 0x4B: /* 4B DEC eBX */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regbx];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regbx] = res16;
			}
			break;

		case 0x4C: /* 4C DEC eSP */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regsp];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regsp] = res16;
			}
			break;

		case 0x4D: /* 4D DEC eBP */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regbp];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regbp] = res16;
			}
			break;

		case 0x4E: /* 4E DEC eSI */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regsi];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regsi] = res16;
			}
			break;

		case 0x4F: /* 4F DEC eDI */
			{
				int oldcf = cpu.cf;
				oper1 = cpu.regs.wordregs[regdi];
				oper2 = 1;
				op_sub16();
				cpu.cf = oldcf;
				cpu.regs.wordregs[regdi] = res16;
			}
			break;

		case 0x50: /* 50 PUSH eAX */
			push(cpu.regs.wordregs[regax]);
			break;

		case 0x51: /* 51 PUSH eCX */
			push(cpu.regs.wordregs[regcx]);
			break;

		case 0x52: /* 52 PUSH eDX */
			push(cpu.regs.wordregs[regdx]);
			break;

		case 0x53: /* 53 PUSH eBX */
			push(cpu.regs.wordregs[regbx]);
			break;

		case 0x54: /* 54 PUSH eSP */
#ifdef USE_286_STYLE_PUSH_SP
			push(cpu.regs.wordregs[regsp]);
#else
			push(cpu.regs.wordregs[regsp] - 2);
#endif
			break;

		case 0x55: /* 55 PUSH eBP */
			push(cpu.regs.wordregs[regbp]);
			break;

		case 0x56: /* 56 PUSH eSI */
			push(cpu.regs.wordregs[regsi]);
			break;

		case 0x57: /* 57 PUSH eDI */
			push(cpu.regs.wordregs[regdi]);
			break;

		case 0x58: /* 58 POP eAX */
			cpu.regs.wordregs[regax] = pop();
			break;

		case 0x59: /* 59 POP eCX */
			cpu.regs.wordregs[regcx] = pop();
			break;

		case 0x5A: /* 5A POP eDX */
			cpu.regs.wordregs[regdx] = pop();
			break;

		case 0x5B: /* 5B POP eBX */
			cpu.regs.wordregs[regbx] = pop();
			break;

		case 0x5C: /* 5C POP eSP */
			cpu.regs.wordregs[regsp] = pop();
			break;

		case 0x5D: /* 5D POP eBP */
			cpu.regs.wordregs[regbp] = pop();
			break;

		case 0x5E: /* 5E POP eSI */
			cpu.regs.wordregs[regsi] = pop();
			break;

		case 0x5F: /* 5F POP eDI */
			cpu.regs.wordregs[regdi] = pop();
			break;

#ifndef CPU_8086
		case 0x60: /* 60 PUSHA (80186+) */
			{
			uint16_t oldsp = cpu.regs.wordregs[regsp];
			push(cpu.regs.wordregs[regax]);
			push(cpu.regs.wordregs[regcx]);
			push(cpu.regs.wordregs[regdx]);
			push(cpu.regs.wordregs[regbx]);
			push(oldsp);
			push(cpu.regs.wordregs[regbp]);
			push(cpu.regs.wordregs[regsi]);
			push(cpu.regs.wordregs[regdi]);
			}
			break;

		case 0x61: /* 61 POPA (80186+) */
			cpu.regs.wordregs[regdi] = pop();
			cpu.regs.wordregs[regsi] = pop();
			cpu.regs.wordregs[regbp] = pop();
			pop();	// result is not used
			cpu.regs.wordregs[regbx] = pop();
			cpu.regs.wordregs[regdx] = pop();
			cpu.regs.wordregs[regcx] = pop();
			cpu.regs.wordregs[regax] = pop();
			break;

		case 0x62: /* 62 BOUND Gv, Ev (80186+) */
			modregrm();
			getea(rm);
			if (signext32(getreg16(reg)) <
			    signext32(getmem16(ea >> 4, ea & 15))) {
				intcall86(5); // bounds check exception
			} else {
				ea += 2;
				if (signext32(getreg16(reg)) >
				    signext32(getmem16(ea >> 4, ea & 15))) {
					intcall86(5); // bounds check exception
				}
			}
			break;

		case 0x68: /* 68 PUSH Iv (80186+) */
			push(getmem16(cpu.segregs[regcs], cpu.ip));
			StepIP(2);
			break;

		case 0x69: /* 69 IMUL Gv Ev Iv (80186+) */
			{
				modregrm();
				uint32_t temp1 = readrm16(rm);
				uint32_t temp2 = getmem16(cpu.segregs[regcs], cpu.ip);
				StepIP(2);
				if ((temp1 & 0x8000L) == 0x8000L) {
					temp1 = temp1 | 0xFFFF0000L;
				}
				if ((temp2 & 0x8000L) == 0x8000L) {
					temp2 = temp2 | 0xFFFF0000L;
				}
				uint32_t temp3 = temp1 * temp2;
				setreg16(reg, temp3 & 0xFFFFL);
				if (temp3 & 0xFFFF0000L) {
					cpu.cf = 1;
					cpu.of = 1;
				} else {
					cpu.cf = 0;
					cpu.of = 0;
				}
			}
			break;

		case 0x6A: /* 6A PUSH Ib (80186+) */
			push(getmem8(cpu.segregs[regcs], cpu.ip));
			StepIP(1);
			break;

		case 0x6B: /* 6B IMUL Gv Eb Ib (80186+) */
			{
				modregrm();
				uint32_t temp1 = readrm16(rm);
				uint32_t temp2 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if ((temp1 & 0x8000L) == 0x8000L) {
					temp1 = temp1 | 0xFFFF0000L;
				}
				if ((temp2 & 0x8000L) == 0x8000L) {
					temp2 = temp2 | 0xFFFF0000L;
				}
				uint32_t temp3 = temp1 * temp2;
				setreg16(reg, temp3 & 0xFFFFL);
				if (temp3 & 0xFFFF0000L) {
					cpu.cf = 1;
					cpu.of = 1;
				} else {
					cpu.cf = 0;
					cpu.of = 0;
				}
			}
			break;

		case 0x6C: /* 6E INSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem8(cpu.useseg, cpu.regs.wordregs[regsi],
				portin(cpu.regs.wordregs[regdx]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0x6D: /* 6F INSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem16(cpu.useseg, cpu.regs.wordregs[regsi],
				 portin16(cpu.regs.wordregs[regdx]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0x6E: /* 6E OUTSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			portout(cpu.regs.wordregs[regdx],
				getmem8(cpu.useseg, cpu.regs.wordregs[regsi]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0x6F: /* 6F OUTSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			portout16(cpu.regs.wordregs[regdx],
				  getmem16(cpu.useseg, cpu.regs.wordregs[regsi]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;
#endif

		case 0x70: /* 70 JO Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.of)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x71: /* 71 JNO Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.of)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x72: /* 72 JB Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.cf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x73: /* 73 JNB Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.cf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x74: /* 74 JZ Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x75: /* 75 JNZ Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x76: /* 76 JBE Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.cf || cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x77: /* 77 JA Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.cf && !cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x78: /* 78 JS Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.sf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x79: /* 79 JNS Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.sf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7A: /* 7A JPE Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.pf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7B: /* 7B JPO Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.pf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7C: /* 7C JL Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.sf != cpu.of)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7D: /* 7D JGE Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (cpu.sf == cpu.of)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7E: /* 7E JLE Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if ((cpu.sf != cpu.of) || cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x7F: /* 7F JG Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.zf && (cpu.sf == cpu.of))
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0x80:
		case 0x82: /* 80/82 GRP1 Eb Ib */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			switch (reg) {
			case 0:
				op_add8();
				break;
			case 1:
				op_or8();
				break;
			case 2:
				op_adc8();
				break;
			case 3:
				op_sbb8();
				break;
			case 4:
				op_and8();
				break;
			case 5:
				op_sub8();
				break;
			case 6:
				op_xor8();
				break;
			case 7:
				flag_sub8(oper1b, oper2b);
				break;
			default:
				break; /* to avoid compiler warnings */
			}

			if (reg < 7) {
				writerm8(rm, res8);
			}
			break;

		case 0x81: /* 81 GRP1 Ev Iv */
		case 0x83: /* 83 GRP1 Ev Ib */
			modregrm();
			oper1 = readrm16(rm);
			if (opcode == 0x81) {
				oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
				StepIP(2);
			} else {
				oper2 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
			}

			switch (reg) {
			case 0:
				op_add16();
				break;
			case 1:
				op_or16();
				break;
			case 2:
				op_adc16();
				break;
			case 3:
				op_sbb16();
				break;
			case 4:
				op_and16();
				break;
			case 5:
				op_sub16();
				break;
			case 6:
				op_xor16();
				break;
			case 7:
				flag_sub16(oper1, oper2);
				break;
			default:
				break; /* to avoid compiler warnings */
			}

			if (reg < 7) {
				writerm16(rm, res16);
			}
			break;

		case 0x84: /* 84 TEST Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			oper2b = readrm8(rm);
			flag_log8(oper1b & oper2b);
			break;

		case 0x85: /* 85 TEST Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			oper2 = readrm16(rm);
			flag_log16(oper1 & oper2);
			break;

		case 0x86: /* 86 XCHG Gb Eb */
			modregrm();
			oper1b = getreg8(reg);
			setreg8(reg, readrm8(rm));
			writerm8(rm, oper1b);
			break;

		case 0x87: /* 87 XCHG Gv Ev */
			modregrm();
			oper1 = getreg16(reg);
			setreg16(reg, readrm16(rm));
			writerm16(rm, oper1);
			break;

		case 0x88: /* 88 MOV Eb Gb */
			modregrm();
			writerm8(rm, getreg8(reg));
			break;

		case 0x89: /* 89 MOV Ev Gv */
			modregrm();
			writerm16(rm, getreg16(reg));
			break;

		case 0x8A: /* 8A MOV Gb Eb */
			modregrm();
			setreg8(reg, readrm8(rm));
			break;

		case 0x8B: /* 8B MOV Gv Ev */
			modregrm();
			setreg16(reg, readrm16(rm));
			break;

		case 0x8C: /* 8C MOV Ew Sw */
			modregrm();
			writerm16(rm, getsegreg(reg));
			break;

		case 0x8D: /* 8D LEA Gv M */
			modregrm();
			getea(rm);
			setreg16(reg, ea - segbase(cpu.useseg));
			break;

		case 0x8E: /* 8E MOV Sw Ew */
			modregrm();
			putsegreg(reg, readrm16(rm));
			break;

		case 0x8F: /* 8F POP Ev */
			modregrm();
			writerm16(rm, pop());
			break;

		case 0x90: /* 90 NOP */
			break;

		case 0x91: /* 91 XCHG eCX eAX */
			oper1 = cpu.regs.wordregs[regcx];
			cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x92: /* 92 XCHG eDX eAX */
			oper1 = cpu.regs.wordregs[regdx];
			cpu.regs.wordregs[regdx] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x93: /* 93 XCHG eBX eAX */
			oper1 = cpu.regs.wordregs[regbx];
			cpu.regs.wordregs[regbx] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x94: /* 94 XCHG eSP eAX */
			oper1 = cpu.regs.wordregs[regsp];
			cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x95: /* 95 XCHG eBP eAX */
			oper1 = cpu.regs.wordregs[regbp];
			cpu.regs.wordregs[regbp] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x96: /* 96 XCHG eSI eAX */
			oper1 = cpu.regs.wordregs[regsi];
			cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x97: /* 97 XCHG eDI eAX */
			oper1 = cpu.regs.wordregs[regdi];
			cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regax];
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0x98: /* 98 CBW */
			if ((cpu.regs.byteregs[regal] & 0x80) == 0x80) {
				cpu.regs.byteregs[regah] = 0xFF;
			} else {
				cpu.regs.byteregs[regah] = 0;
			}
			break;

		case 0x99: /* 99 CWD */
			if ((cpu.regs.byteregs[regah] & 0x80) == 0x80) {
				cpu.regs.wordregs[regdx] = 0xFFFF;
			} else {
				cpu.regs.wordregs[regdx] = 0;
			}
			break;

		case 0x9A: /* 9A CALL Ap */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			push(cpu.segregs[regcs]);
			push(cpu.ip);
			cpu.ip = oper1;
			cpu.segregs[regcs] = oper2;
			break;

		case 0x9B: /* 9B WAIT */
			break;

		case 0x9C: /* 9C PUSHF */
#ifdef CPU_SET_HIGH_FLAGS
			push(makeflagsword() | 0xF800);
#else
			push(makeflagsword() | 0x0800);
#endif
			break;

		case 0x9D: /* 9D POPF */
			{
				uint16_t temp16 = pop();
				decodeflagsword(temp16);
			}
			break;

		case 0x9E: /* 9E SAHF */
			decodeflagsword((makeflagsword() & 0xFF00) |
					cpu.regs.byteregs[regah]);
			break;

		case 0x9F: /* 9F LAHF */
			cpu.regs.byteregs[regah] = makeflagsword() & 0xFF;
			break;

		case 0xA0: /* A0 MOV cpu.regs.byteregs[regal] Ob */
			cpu.regs.byteregs[regal] =
			    getmem8(cpu.useseg, getmem16(cpu.segregs[regcs], cpu.ip));
			StepIP(2);
			break;

		case 0xA1: /* A1 MOV eAX Ov */
			oper1 = getmem16(cpu.useseg, getmem16(cpu.segregs[regcs], cpu.ip));
			StepIP(2);
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0xA2: /* A2 MOV Ob cpu.regs.byteregs[regal] */
			putmem8(cpu.useseg, getmem16(cpu.segregs[regcs], cpu.ip),
				cpu.regs.byteregs[regal]);
			StepIP(2);
			break;

		case 0xA3: /* A3 MOV Ov eAX */
			putmem16(cpu.useseg, getmem16(cpu.segregs[regcs], cpu.ip),
				 cpu.regs.wordregs[regax]);
			StepIP(2);
			break;

		case 0xA4: /* A4 MOVSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem8(cpu.segregs[reges], cpu.regs.wordregs[regdi],
				getmem8(cpu.useseg, cpu.regs.wordregs[regsi]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xA5: /* A5 MOVSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem16(cpu.segregs[reges], cpu.regs.wordregs[regdi],
				 getmem16(cpu.useseg, cpu.regs.wordregs[regsi]));
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xA6: /* A6 CMPSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			oper1b = getmem8(cpu.useseg, cpu.regs.wordregs[regsi]);
			oper2b = getmem8(cpu.segregs[reges], cpu.regs.wordregs[regdi]);
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 1;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			flag_sub8(oper1b, oper2b);
			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			if ((reptype == 1) && !cpu.zf) {
				break;
			} else if ((reptype == 2) && (cpu.zf == 1)) {
				break;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xA7: /* A7 CMPSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			oper1 = getmem16(cpu.useseg, cpu.regs.wordregs[regsi]);
			oper2 = getmem16(cpu.segregs[reges], cpu.regs.wordregs[regdi]);
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 2;
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			flag_sub16(oper1, oper2);
			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			if ((reptype == 1) && !cpu.zf) {
				break;
			}

			if ((reptype == 2) && (cpu.zf == 1)) {
				break;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xA8: /* A8 TEST cpu.regs.byteregs[regal] Ib */
			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			flag_log8(oper1b & oper2b);
			break;

		case 0xA9: /* A9 TEST eAX Iv */
			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			flag_log16(oper1 & oper2);
			break;

		case 0xAA: /* AA STOSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem8(cpu.segregs[reges], cpu.regs.wordregs[regdi],
				cpu.regs.byteregs[regal]);
			if (cpu.df) {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xAB: /* AB STOSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			putmem16(cpu.segregs[reges], cpu.regs.wordregs[regdi],
				 cpu.regs.wordregs[regax]);
			if (cpu.df) {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xAC: /* AC LODSB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			cpu.regs.byteregs[regal] =
			    getmem8(cpu.useseg, cpu.regs.wordregs[regsi]);
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 1;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xAD: /* AD LODSW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			oper1 = getmem16(cpu.useseg, cpu.regs.wordregs[regsi]);
			cpu.regs.wordregs[regax] = oper1;
			if (cpu.df) {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] - 2;
			} else {
				cpu.regs.wordregs[regsi] = cpu.regs.wordregs[regsi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xAE: /* AE SCASB */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			oper1b = cpu.regs.byteregs[regal];
			oper2b = getmem8(cpu.segregs[reges], cpu.regs.wordregs[regdi]);
			flag_sub8(oper1b, oper2b);
			if (cpu.df) {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 1;
			} else {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 1;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			if ((reptype == 1) && !cpu.zf) {
				break;
			} else if ((reptype == 2) && (cpu.zf == 1)) {
				break;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xAF: /* AF SCASW */
			if (reptype && (cpu.regs.wordregs[regcx] == 0)) {
				break;
			}

			oper1 = cpu.regs.wordregs[regax];
			oper2 = getmem16(cpu.segregs[reges], cpu.regs.wordregs[regdi]);
			flag_sub16(oper1, oper2);
			if (cpu.df) {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] - 2;
			} else {
				cpu.regs.wordregs[regdi] = cpu.regs.wordregs[regdi] + 2;
			}

			if (reptype) {
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
			}

			if ((reptype == 1) && !cpu.zf) {
				break;
			} else if ((reptype == 2) & (cpu.zf == 1)) {
				break;
			}

			totalexec++;
			loopcount++;
			if (!reptype) {
				break;
			}

			cpu.ip = firstip;
			break;

		case 0xB0: /* B0 MOV cpu.regs.byteregs[regal] Ib */
			cpu.regs.byteregs[regal] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB1: /* B1 MOV cpu.regs.byteregs[regcl] Ib */
			cpu.regs.byteregs[regcl] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB2: /* B2 MOV cpu.regs.byteregs[regdl] Ib */
			cpu.regs.byteregs[regdl] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB3: /* B3 MOV cpu.regs.byteregs[regbl] Ib */
			cpu.regs.byteregs[regbl] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB4: /* B4 MOV cpu.regs.byteregs[regah] Ib */
			cpu.regs.byteregs[regah] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB5: /* B5 MOV cpu.regs.byteregs[regch] Ib */
			cpu.regs.byteregs[regch] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB6: /* B6 MOV cpu.regs.byteregs[regdh] Ib */
			cpu.regs.byteregs[regdh] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB7: /* B7 MOV cpu.regs.byteregs[regbh] Ib */
			cpu.regs.byteregs[regbh] = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			break;

		case 0xB8: /* B8 MOV eAX Iv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			cpu.regs.wordregs[regax] = oper1;
			break;

		case 0xB9: /* B9 MOV eCX Iv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			cpu.regs.wordregs[regcx] = oper1;
			break;

		case 0xBA: /* BA MOV eDX Iv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			cpu.regs.wordregs[regdx] = oper1;
			break;

		case 0xBB: /* BB MOV eBX Iv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			cpu.regs.wordregs[regbx] = oper1;
			break;

		case 0xBC: /* BC MOV eSP Iv */
			cpu.regs.wordregs[regsp] = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			break;

		case 0xBD: /* BD MOV eBP Iv */
			cpu.regs.wordregs[regbp] = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			break;

		case 0xBE: /* BE MOV eSI Iv */
			cpu.regs.wordregs[regsi] = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			break;

		case 0xBF: /* BF MOV eDI Iv */
			cpu.regs.wordregs[regdi] = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			break;

		case 0xC0: /* C0 GRP2 byte imm8 (80186+) */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			writerm8(rm, op_grp2_8(oper2b));
			break;

		case 0xC1: /* C1 GRP2 word imm8 (80186+) */
			modregrm();
			oper1 = readrm16(rm);
			oper2 = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			writerm16(rm, op_grp2_16((uint8_t)oper2));
			break;

		case 0xC2: /* C2 RET Iw */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			cpu.ip = pop();
			cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regsp] + oper1;
			break;

		case 0xC3: /* C3 RET */
			cpu.ip = pop();
			break;

		case 0xC4: /* C4 LES Gv Mp */
			modregrm();
			getea(rm);
			setreg16(reg, read86(ea) + read86(ea + 1) * 256);
			cpu.segregs[reges] = read86(ea + 2) + read86(ea + 3) * 256;
			break;

		case 0xC5: /* C5 LDS Gv Mp */
			modregrm();
			getea(rm);
			setreg16(reg, read86(ea) + read86(ea + 1) * 256);
			cpu.segregs[regds] = read86(ea + 2) + read86(ea + 3) * 256;
			break;

		case 0xC6: /* C6 MOV Eb Ib */
			modregrm();
			writerm8(rm, getmem8(cpu.segregs[regcs], cpu.ip));
			StepIP(1);
			break;

		case 0xC7: /* C7 MOV Ev Iv */
			modregrm();
			writerm16(rm, getmem16(cpu.segregs[regcs], cpu.ip));
			StepIP(2);
			break;

		case 0xC8: /* C8 ENTER (80186+) */
			stacksize = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			nestlev = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			push(cpu.regs.wordregs[regbp]);
			frametemp = cpu.regs.wordregs[regsp];
			if (nestlev) {
				for (int a = 1; a < nestlev; a++) {
					cpu.regs.wordregs[regbp] =
					    cpu.regs.wordregs[regbp] - 2;
					push(cpu.regs.wordregs[regbp]);
				}

				push(cpu.regs.wordregs[regsp]);
			}

			cpu.regs.wordregs[regbp] = frametemp;
			cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regbp] - stacksize;

			break;

		case 0xC9: /* C9 LEAVE (80186+) */
			cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regbp];
			cpu.regs.wordregs[regbp] = pop();
			break;

		case 0xCA: /* CA RETF Iw */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			cpu.ip = pop();
			cpu.segregs[regcs] = pop();
			cpu.regs.wordregs[regsp] = cpu.regs.wordregs[regsp] + oper1;
			break;

		case 0xCB: /* CB RETF */
			cpu.ip = pop();
			;
			cpu.segregs[regcs] = pop();
			break;

		case 0xCC: /* CC INT 3 */
			intcall86(3);
			break;

		case 0xCD: /* CD INT Ib */
			oper1b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			intcall86(oper1b);
			break;

		case 0xCE: /* CE INTO */
			if (cpu.of) {
				intcall86(4);
			}
			break;

		case 0xCF: /* CF IRET */
			cpu_IRET();
			/*cpu.ip = pop();
			cpu.segregs[regcs] = pop();
			decodeflagsword(pop());*/

			/*
			 * if (net.enabled) net.canrecv = 1;
			 */
			break;

		case 0xD0: /* D0 GRP2 Eb 1 */
			modregrm();
			oper1b = readrm8(rm);
			writerm8(rm, op_grp2_8(1));
			break;

		case 0xD1: /* D1 GRP2 Ev 1 */
			modregrm();
			oper1 = readrm16(rm);
			writerm16(rm, op_grp2_16(1));
			break;

		case 0xD2: /* D2 GRP2 Eb cpu.regs.byteregs[regcl] */
			modregrm();
			oper1b = readrm8(rm);
			writerm8(rm, op_grp2_8(cpu.regs.byteregs[regcl]));
			break;

		case 0xD3: /* D3 GRP2 Ev cpu.regs.byteregs[regcl] */
			modregrm();
			oper1 = readrm16(rm);
			writerm16(rm, op_grp2_16(cpu.regs.byteregs[regcl]));
			break;

		case 0xD4: /* D4 AAM I0 */
			oper1 = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			if (!oper1) {
				intcall86(0);
				break;
			} /* division by zero */

			cpu.regs.byteregs[regah] =
			    (cpu.regs.byteregs[regal] / oper1) & 255;
			cpu.regs.byteregs[regal] =
			    (cpu.regs.byteregs[regal] % oper1) & 255;
			flag_szp16(cpu.regs.wordregs[regax]);
			break;

		case 0xD5: /* D5 AAD I0 */
			oper1 = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			cpu.regs.byteregs[regal] = (cpu.regs.byteregs[regah] * oper1 +
						cpu.regs.byteregs[regal]) &
					       255;
			cpu.regs.byteregs[regah] = 0;
			flag_szp16(cpu.regs.byteregs[regah] * oper1 +
				   cpu.regs.byteregs[regal]);
			cpu.sf = 0;
			break;

		case 0xD6: /* D6 XLAT on V20/V30, SALC on 8086/8088 */
#ifndef CPU_NO_SALC
			cpu.regs.byteregs[regal] = cpu.cf ? 0xFF : 0x00;
			break;
#endif

		case 0xD7: /* D7 XLAT */
			cpu.regs.byteregs[regal] =
			    read86(cpu.useseg * 16 + (cpu.regs.wordregs[regbx]) +
				   cpu.regs.byteregs[regal]);
			break;

		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDE:
		case 0xDD:
		case 0xDF: /* escape to x87 FPU (unsupported) */
			modregrm();
			break;

		case 0xE0: /* E0 LOOPNZ Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
				if ((cpu.regs.wordregs[regcx]) && !cpu.zf)
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0xE1: /* E1 LOOPZ Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
				if (cpu.regs.wordregs[regcx] && (cpu.zf == 1))
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0xE2: /* E2 LOOP Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				cpu.regs.wordregs[regcx] = cpu.regs.wordregs[regcx] - 1;
				if (cpu.regs.wordregs[regcx])
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0xE3: /* E3 JCXZ Jb */
			{
				uint16_t temp16 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
				StepIP(1);
				if (!cpu.regs.wordregs[regcx])
					cpu.ip = cpu.ip + temp16;
			}
			break;

		case 0xE4: /* E4 IN cpu.regs.byteregs[regal] Ib */
			oper1b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			cpu.regs.byteregs[regal] = (uint8_t)portin(oper1b);
			break;

		case 0xE5: /* E5 IN eAX Ib */
			oper1b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			cpu.regs.wordregs[regax] = portin16(oper1b);
			break;

		case 0xE6: /* E6 OUT Ib cpu.regs.byteregs[regal] */
			oper1b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			portout(oper1b, cpu.regs.byteregs[regal]);
			break;

		case 0xE7: /* E7 OUT Ib eAX */
			oper1b = getmem8(cpu.segregs[regcs], cpu.ip);
			StepIP(1);
			portout16(oper1b, cpu.regs.wordregs[regax]);
			break;

		case 0xE8: /* E8 CALL Jv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			push(cpu.ip);
			cpu.ip = cpu.ip + oper1;
			break;

		case 0xE9: /* E9 JMP Jv */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			cpu.ip = cpu.ip + oper1;
			break;

		case 0xEA: /* EA JMP Ap */
			oper1 = getmem16(cpu.segregs[regcs], cpu.ip);
			StepIP(2);
			oper2 = getmem16(cpu.segregs[regcs], cpu.ip);
			cpu.ip = oper1;
			cpu.segregs[regcs] = oper2;
			break;

		case 0xEB: /* EB JMP Jb */
			oper1 = signext(getmem8(cpu.segregs[regcs], cpu.ip));
			StepIP(1);
			cpu.ip = cpu.ip + oper1;
			break;

		case 0xEC: /* EC IN cpu.regs.byteregs[regal] regdx */
			oper1 = cpu.regs.wordregs[regdx];
			cpu.regs.byteregs[regal] = (uint8_t)portin(oper1);
			break;

		case 0xED: /* ED IN eAX regdx */
			oper1 = cpu.regs.wordregs[regdx];
			cpu.regs.wordregs[regax] = portin16(oper1);
			break;

		case 0xEE: /* EE OUT regdx cpu.regs.byteregs[regal] */
			oper1 = cpu.regs.wordregs[regdx];
			portout(oper1, cpu.regs.byteregs[regal]);
			break;

		case 0xEF: /* EF OUT regdx eAX */
			oper1 = cpu.regs.wordregs[regdx];
			portout16(oper1, cpu.regs.wordregs[regax]);
			break;

		case 0xF0: /* F0 LOCK */
			break;

		case 0xF4: /* F4 HLT */
			// HLT can be used as trap. We call this implementation to tell, if it's really a halt or just a trap
			if (cpu_hlt_handler())
				cpu.hltstate = 1;
			break;

		case 0xF5: /* F5 CMC */
			if (!cpu.cf) {
				cpu.cf = 1;
			} else {
				cpu.cf = 0;
			}
			break;

		case 0xF6: /* F6 GRP3a Eb */
			modregrm();
			oper1b = readrm8(rm);
			op_grp3_8();
			if ((reg > 1) && (reg < 4)) {
				writerm8(rm, res8);
			}
			break;

		case 0xF7: /* F7 GRP3b Ev */
			modregrm();
			oper1 = readrm16(rm);
			op_grp3_16();
			if ((reg > 1) && (reg < 4)) {
				writerm16(rm, res16);
			}
			break;

		case 0xF8: /* F8 CLC */
			cpu.cf = 0;
			break;

		case 0xF9: /* F9 STC */
			cpu.cf = 1;
			break;

		case 0xFA: /* FA CLI */
			cpu.ifl = 0;
			break;

		case 0xFB: /* FB STI */
			cpu.ifl = 1;
			break;

		case 0xFC: /* FC CLD */
			cpu.df = 0;
			break;

		case 0xFD: /* FD STD */
			cpu.df = 1;
			break;

		case 0xFE: /* FE GRP4 Eb */
			modregrm();
			oper1b = readrm8(rm);
			oper2b = 1;
			if (!reg) {
				int tempcf = cpu.cf;
				res8 = oper1b + oper2b;
				flag_add8(oper1b, oper2b);
				cpu.cf = tempcf;
				writerm8(rm, res8);
			} else {
				int tempcf = cpu.cf;
				res8 = oper1b - oper2b;
				flag_sub8(oper1b, oper2b);
				cpu.cf = tempcf;
				writerm8(rm, res8);
			}
			break;

		case 0xFF: /* FF GRP5 Ev */
			modregrm();
			oper1 = readrm16(rm);
			op_grp5();
			break;

		default:
#ifdef CPU_ALLOW_ILLEGAL_OP_EXCEPTION
			intcall86(6); /* trip invalid opcode exception (this
					 occurs on the 80186+, 8086/8088 CPUs
					 treat them as NOPs. */
			/* technically they aren't exactly like NOPs in most
			 * cases, but for our pursoses, that's accurate enough.
			 */
#endif
//			if (verbose) {
				printf("Illegal opcode: %02X %02X /%X @ "
				       "%04X:%04X\n",
				       getmem8(cpu.savecs, cpu.saveip),
				       getmem8(cpu.savecs, cpu.saveip + 1),
				       (getmem8(cpu.savecs, cpu.saveip + 2) >> 3) & 7,
				       cpu.savecs, cpu.saveip);
//			}
			break;
		}

	skipexecution:
		if (!running) {
			return;
		}
	}
}
#endif
