
all:
	gcc -Wall -Werror -Ddebug -o servidor servidor.c -lsqlite3 -I.
	./servidor

clean:
	rm servidor
