//Helper code to allow outputting a trace of the instructions being executed.
//Normally disabled.
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "cpu.h"
#include "trace.h"

#define REG_AX cpu.regs.wordregs[regax]
#define REG_BX cpu.regs.wordregs[regbx]
#define REG_CX cpu.regs.wordregs[regcx]
#define REG_DX cpu.regs.wordregs[regdx]
#define REG_BP cpu.regs.wordregs[regbp]
#define REG_DS cpu.segregs[regds]
#define REG_ES cpu.segregs[reges]


#if USE_BPS
//Callback for every single existing memory address.
//Note: this array is pretty large (4M-8M). You don't want this on embedded devices.
static trace_bp_cb_t *cbs[1024*1024]={0};

static int exec86_bp(int count) {
	for (int i=0; i<count; i++) {
		if (exec86(1)!=0) return count-i;
		int adr=(cpu.segregs[regcs]<<4)+cpu.ip;
		if (cbs[adr]) cbs[adr]();
	}
}

void trace_set_bp(int cs, int ip, trace_bp_cb_t cb) {
	cbs[(cs<<4)+ip]=cb;
}

#else
//tracing disabled: stub
void set_bp(int addr, trace_bp_cb_t cb) {
	return;
}
#endif

#if DO_TRACE
typedef struct __attribute__((packed)) {
	uint16_t cs;
	uint16_t ip;
	uint16_t ax;
	uint16_t null;
} trace_t;

#define TRACECT (3*1024*1024*2ULL)
static trace_t *tracemem=NULL;
static int tracepos=0;
static int do_trace=0;

//Run the CPU for x cycles, include tracing
void trace_run_cpu(int count) {
	if (!tracemem) tracemem=calloc(TRACECT, sizeof(trace_t));
	if (!do_trace) {
		return exec86_bp(count);
	}
	for (int i=0; i<count; i++) {
		if (exec86_bp(1)!=0) return count-i;
		tracemem[tracepos].cs=cpu.segregs[regcs];
		tracemem[tracepos].ip=cpu.ip;
		tracemem[tracepos].ax=REG_AX;
		tracemem[tracepos].null=REG_DX;
		tracepos++;
		if (tracepos>=TRACECT) {
			FILE *f=fopen("tracelog.bin", "w");
			if (!f) {
				perror("tracelog");
			} else {
				fwrite(tracemem, sizeof(trace_t), TRACECT, f);
				fclose(f);
			}
			printf("Trace complete.\n");
			exit(0);
		}
		if (!r) return 0;
	}
}

void trace_enable(int ena) {
	do_trace=ena;
}

#else

//stub
void trace_enable(int ena) {
	return;
}

#endif


