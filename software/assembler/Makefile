CFLAGS = -ansi -pedantic -Wall -Wextra -Werror

as: asm.o symbols.o i8080.o z80.o
	$(CC) $^ -o $@

#
asm.o: asm.c asm.h
symbols.o: symbols.c asm.h

i8080.o: i8080.c asm.h
z80.o: z80.c asm.h

#
.PHONY: clean

clean:
	$(RM) *.o as

