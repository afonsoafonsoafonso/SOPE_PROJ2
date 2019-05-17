all:
	make user server
user: user.c constants.h log.c log_writing.c queue.c sope.h types.h  utils.c
	gcc -g -D_REENTRANT -pthread -Wall -o user user.c constants.h log.c sope.h types.h log_writing.c utils.c
server: server.c constants.h log.c log_writing.c queue.c sope.h types.h  utils.c
	gcc -g -D_REENTRANT -pthread -Wall -o server server.c constants.h log.c sope.h types.h log_writing.c utils.c queue.c
clean:
	rm -f user server
	make clean_log
clean_log:
	rm -f slog.txt ulog.txt
force:
	make clean
	make all