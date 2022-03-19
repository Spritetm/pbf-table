
#define DO_TRACE 0
#define USE_BPS 1

typedef void (trace_bp_cb_t)(void);

void trace_enable(int ena);
void trace_run_cpu(int count);
void trace_set_bp(int cs, int ip, trace_bp_cb_t cb);
