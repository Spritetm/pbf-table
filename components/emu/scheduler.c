#include <stdio.h>
#include "scheduler.h"
#include "trace.h"

typedef struct {
	int delay_us;
	schedule_cb_t cb;
	int repeat_us;
	const char *name;
} schedule_event_t;

#define MAX_SCHED_EVENTS 20

#define GRAN_EVENT_US 10
//Note: given that an 8086 does, like, 4 cycles per instruction, an 8MHz one would have 20 ticks/10us.
#define TICKS_PER_GRAN_EVENT 6

static schedule_event_t schedule[MAX_SCHED_EVENTS];

void schedule_add(schedule_cb_t cb, int delay_us, int repeat, const char *name) {
	for (int i=0; i<MAX_SCHED_EVENTS; i++) {
		if (schedule[i].cb==NULL) {
			schedule[i].cb=cb;
			schedule[i].delay_us=delay_us;
			schedule[i].repeat_us=(repeat?delay_us:0);
			schedule[i].name=name;
			return;
		}
	}
}

void schedule_adjust_cycles(int extra_cycles) {
	int us=((extra_cycles*GRAN_EVENT_US)/TICKS_PER_GRAN_EVENT);
	for (int i=0; i<MAX_SCHED_EVENTS; i++) {
		if (schedule[i].cb!=NULL) {
			schedule[i].delay_us-=us;
		}
	}
}

void schedule_run(int us) {
#if 0
	for (int i=0; i<MAX_SCHED_EVENTS; i++) {
		if (schedule[i].delay_us<-TICKS_PER_GRAN_EVENT*2) {
			printf("Schedule: evt %s late by %dus\n", schedule[i].name, schedule[i].delay_us);
		}
	}
#endif

	while(us>0) {
		schedule_adjust_cycles(TICKS_PER_GRAN_EVENT);
		for (int i=0; i<MAX_SCHED_EVENTS; i++) {
			if (schedule[i].cb!=NULL) {
				if (schedule[i].delay_us<0) {
					schedule[i].cb();
					if (schedule[i].repeat_us) {
						schedule[i].delay_us+=schedule[i].repeat_us;
					} else {
						schedule[i].cb=NULL;
					}
				}
			}
		}
		trace_run_cpu(TICKS_PER_GRAN_EVENT);
		us-=GRAN_EVENT_US;
	}
}

