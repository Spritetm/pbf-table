//Schedule a callback at some future x86 cycle time.

typedef void (*schedule_cb_t)();

//Add a callback. delay_us is in emulated x86 time
void schedule_add(schedule_cb_t cb, int delay_us, int repeat, const char *name);

//Run the emulator and call any callbacks for a certain amount of time. us is
//in microseconds emulated x86 time
void schedule_run(int us);

//Tell the scheduler we ran the x86 ourself for this many cycles
void schedule_adjust_cycles(int extra_cycles);
