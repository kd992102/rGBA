all: main.out

CC = gcc
INCLUDE = .
CFLAGS = -m32 -g

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

cpu.o: cpu.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c cpu.c

main.out: main.o memory.o arm7tdmi.o thumb_instruction.o arm_instruction.o cpu.o
	$(CC) -o main main.o memory.o arm7tdmi.o thumb_instruction.o arm_instruction.o cpu.o

clean:
	del -fR main.o memory.o arm7tdmi.o main.out thumb_instruction.o arm_instruction.o cpu.o