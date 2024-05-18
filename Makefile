C = $(wildcard *.c) picohttpparser/picohttpparser.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	C += picoev/picoev_epoll.c
endif

ifeq ($(UNAME_S),FreeBSD)
	C += picoev/picoev_kqueue.c
endif

ifdef PROD
	CFLAGS = -O3
else
	CFLAGS = -g
endif

CFLAGS += -fsanitize=address

a.out: $(C)
	$(CC) $(C) $(CFLAGS)
