output: token.o
	gcc token.o -o token

token.o: token.c
	gcc -c token.c

sample:
	make
	./token sample_source_file

memcheck:
	make
	valgrind ./token sample_source_file

clean:
	rm *.o token