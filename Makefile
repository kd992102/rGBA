all: main.out

CC = gcc
INCLUDE = .
CFLAGS = -m32 -g
LDLIBS = C:\msys64\mingw32\i686-w64-mingw32\lib

memory.o: memory.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c memory.c

arm7tdmi.o: arm7tdmi.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c arm7tdmi.c

main.o: main.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c main.c

arm_instruction.o: arm_instruction.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c arm_instruction.c

thumb_instruction.o: thumb_instruction.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c thumb_instruction.c

ppu.o: ppu.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c ppu.c

io.o: io.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c io.c

gba.o: gba.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c gba.c

dma.o: dma.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c dma.c

main.out: main.o memory.o arm7tdmi.o thumb_instruction.o arm_instruction.o io.o ppu.o gba.o dma.o
	$(CC) main.o memory.o arm7tdmi.o thumb_instruction.o arm_instruction.o io.o ppu.o gba.o dma.o -lmingw32 -lSDL2main -lSDL2 -o main

clean:
	del -fR main.o memory.o arm7tdmi.o thumb_instruction.o arm_instruction.o io.o ppu.o gba.o dma.o