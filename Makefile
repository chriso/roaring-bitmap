CFLAGS = -std=c99 -pedantic -Wall -Wextra -g

rbit.o: rbit.c rbit.h
tests.o: tests.c rbit.h
benchmark.o: benchmark.c rbit.h

tests: rbit.o tests.o

benchmark: CFLAGS += -O3
benchmark: rbit.o benchmark.o

check: tests
	./tests

bench: benchmark
	./benchmark

clean:
	rm -f *.o tests benchmark
