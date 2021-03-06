#include "cpu.h"

#define DO_TRACE 0
#define USE_BPS 0

typedef void (trace_bp_cb_t)(void);

void trace_enable(int ena);
void trace_set_bp(int cs, int ip, trace_bp_cb_t cb);
#if DO_TRACE
int trace_run_cpu(int count);
#else
//immediately call emulator
static inline int trace_run_cpu(int count) {
	return exec86(count);
}
#endif
