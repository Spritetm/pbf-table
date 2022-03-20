#include <stdio.h>
#include "scheduler.h"
#include "trace.h"

typedef struct {
	int delay_us;
	schedule_cb_t cb;
	int repeat_us;
} schedule_event_t;

#define MAX_SCHED_EVENTS 20

#define GRAN_EVENT_US 10
//Note: given that an 8086 does, like, 4 cycles per clock, an 8MHz one would have 20 ticks/10us.
#define TICKS_PER_GRAN_EVENT 5

static schedule_event_t schedule[MAX_SCHED_EVENTS];

void schedule_add(schedule_cb_t cb, int delay_us, int repeat) {
	for (int i=0; i<MAX_SCHED_EVENTS; i++) {
		if (schedule[i].cb==NULL) {
			schedule[i].cb=cb;
			schedule[i].delay_us=delay_us;
			schedule[i].repeat_us=(repeat?delay_us:0);
			return;
		}
	}
}


void schedule_run(int us) {
	while(us>0) {
		for (int i=0; i<MAX_SCHED_EVENTS; i++) {
			if (schedule[i].cb!=NULL) {
				schedule[i].delay_us-=GRAN_EVENT_US;
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
