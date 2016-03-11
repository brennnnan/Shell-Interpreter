.phony all:
all: p1

p1: p1.c
	gcc p1.c -o p1


.PHONY clean:
clean:
	-rm -rf *.o *.exe
