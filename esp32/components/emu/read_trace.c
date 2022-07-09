//Not ESP32 code but executable on the host: read output from trace.c and convert into
//readable text
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct __attribute__((packed)) {
	uint16_t cs;
	uint16_t ip;
	uint16_t ax;
	uint16_t null;
} trace_t;


#define LOOPBUFLEN 500

trace_t loopbuf[LOOPBUFLEN]={0};
int loopbuf_pos;
int loop_insns=0;
int loop_len=0;

//What we're trying to do here:
//For an instruction t, we look if it's part of a loop.
//If the last [p-n...p-1] instructions are the same as the
//last [p-n*2...p-n] instructions, we decide we're part of a loop.
//Note that this only detect loops where each iteration takes the exact
//same path. (ToDo: allow a bit of variation?)
int find_loop(trace_t *t) {
	loopbuf[loopbuf_pos++]=*t;
	if (loopbuf_pos>=LOOPBUFLEN) loopbuf_pos=0;
	for (int n=1; n<LOOPBUFLEN/2; n++) {
		int it1pos=loopbuf_pos-1;
		int it2pos=loopbuf_pos-(n+1);
		int match=1;
		for (int p=0; p<n; p++) {
			if (it1pos<0) it1pos+=LOOPBUFLEN;
			if (it2pos<0) it2pos+=LOOPBUFLEN;
			if (loopbuf[it1pos].cs!=loopbuf[it2pos].cs || loopbuf[it1pos].ip!=loopbuf[it2pos].ip) {
				match=0;
				break;
			}
			it1pos--;
			it2pos--;
		}
		if (match) {
			loop_len=n;
			loop_insns++;
			return 1;
		}
	}
	//If we're here, we're not in a loop.
	if (loop_insns) {
		printf("(Previous %d instructions loop for %d iterations.)\n", loop_len, (loop_insns/loop_len)+2);
		loop_insns=0;
	}
	return 0;
}


int main(int argc, char **argv) {
	FILE *f=fopen(argv[1], "r");
	if (!f) {
		perror(argv[1]);
		exit(1);
	}
	
	trace_t t;
	while(!feof(f)) {
		if (fread(&t, sizeof(t), 1, f)) {
			if (!find_loop(&t)) {
				printf("%04X:%04X ax=%04x unused=%04X\n", t.cs, t.ip, t.ax, t.null);
			}
		}
	}
	//flush out last loop indicator
	memset(&t, 0, sizeof(t));
	find_loop(&t);
	fclose(f);
}

