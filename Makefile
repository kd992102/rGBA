all: main.out

CC = gcc
INCLUDE = .
CFLAGS = -m32 -g
main.o: main.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c main.c

memory.o: memory.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c memory.c

arm7tdmi.o: arm7tdmi.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c arm7tdmi.c

arm_instruction.o: arm_instruction.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c arm_instruction.c

thumb_instruction.o: thumb_instruction.c
	$(CC) -I $(INCLUDE) $(CFLAGS) -c thumb_instruction.c

main.out: main.o memory.o arm7tdmi.o arm_instruction.o thumb_instruction.o
	$(CC) -o main main.o memory.o arm7tdmi.o arm_instruction.o thumb_instruction.o

clean:
	del -fR main.o memory.o arm7tdmi.o arm_instruction.o main.out