CC = gcc -std=gnu99

Server:main.o epoll.o parse.o http.o heap.o timer.o threadpool.o utils.o
	$(CC) *.o -o Server -lpthread

main.o:main.c
	$(CC) -c main.c

utils.o:utils.c utils.h
	$(CC) -c utils.c

epoll.o:epoll.c epoll.h
	$(CC) -c epoll.c

http.o:http.c http.h
	$(CC) -c http.c

parse.o:parse.c parse.h
	$(CC) -c parse.c

heap.o:heap.c heap.h
	$(CC) -c heap.c

timer.o:timer.c timer.h
	$(CC) -c timer.c

threadpool.o:threadpool.c threadpool.h
	$(CC) -c threadpool.c

clean:
	rm *.o