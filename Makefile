OBJS=cpu.o main.o gfx.o cpu_addr_space.o ibxm/ibxm.o hexdump.o
TARGET=emu
CFLAGS=-Og -ggdb `pkg-config --cflags sdl2`
LDFLAGS=`pkg-config --libs sdl2`

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 

clean:
	rm -f $(OBJS) $(TARGET)