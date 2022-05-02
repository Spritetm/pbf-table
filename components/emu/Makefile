SRCS=cpu.c emu.c unix/gfx_sdl.c cpu_addr_space.c ibxm/ibxm.c hexdump.c scheduler.c trace.c 
SRCS+=load_exe.c unix/mmap_file_unix.c unix/main_unix.c unix/haptic_unix.c pf_vars.c music.c
SRCS+=menu.c
TARGET=emu
CFLAGS=-O0 -ggdb `pkg-config --cflags sdl2` -I.
LDFLAGS=`pkg-config --libs sdl2`


DEPFLAGS = -MT $@ -MMD -MP -MF build/$*.d
OBJS = $(patsubst %.c,build/%.o,$(SRCS))

all: $(TARGET) read_trace

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f $(OBJS) $(TARGET)

read_trace: read_trace.c
	$(CC) -o $@ $^

#Dependency generation stuff
build/%.o : %.c build/%.d | build
	@mkdir -p $(@D)
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

build: ; @mkdir -p build

DEPFILES := $(SRCS:%.c=build/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
