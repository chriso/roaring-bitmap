CFLAGS = -std=c99 -pedantic -Wall -Wextra -g $(EXTCFLAGS)

rset.o: rset.c rset.h
tests.o: tests.c rset.h
benchmark.o: benchmark.c rset.h

tests: rset.o tests.o

benchmark: CFLAGS += -O3
benchmark: rset.o benchmark.o

check: tests
	./tests

bench: benchmark
	./benchmark

clean:
	rm -f *.o tests benchmark
