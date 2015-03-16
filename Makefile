rbit.o: rbit.c rbit.h
tests.o: tests.c rbit.h

tests: rbit.o tests.o

check: tests
	./tests

clean:
	rm -f *.o tests
