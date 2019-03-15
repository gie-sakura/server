
servermake:
	gcc -Ddebug -o servidor servidor.c -lsqlite3 -I.
	./servidor
