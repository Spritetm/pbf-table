//Initialize the interface between Pinball Fantasies and the emulator.
//offset_start and len indicate a range in x86 memory where the structs
//to interface with can be found.
void pf_vars_init(int offset_start, int len);

//Returns 1 if pressing tbe button actually moves a flipper
int pf_vars_get_flip_enabled();

//Returns ball velocity
void pf_vars_get_ball_speed(int *x, int *y);

//Set the spring position [0-32]
void pf_vars_set_springpos(uint8_t springpos);
