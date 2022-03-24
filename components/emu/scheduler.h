
typedef void (*schedule_cb_t)();

void schedule_add(schedule_cb_t cb, int delay_us, int repeat);
void schedule_run(int us);
void schedule_adjust_cycles(int extra_cycles);
