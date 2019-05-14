all:
	make user server
user: user user.c constants.h log.c sope.h types.h log_writing.c utils.c
	gcc -g -D_REENTRANT -pthread -Wall -o user user.c constants.h log.c sope.h types.h log_writing.c utils.c
server: server server.c constants.h log.c sope.h types.h log_writing.c utils.c
	gcc -g -D_REENTRANT -pthread -Wall -o server server.c constants.h log.c sope.h types.h log_writing.c utils.c
clean:
	rm -f user server
