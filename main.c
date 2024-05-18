#include "picoev/picoev.h"
#include "picohttpparser/picohttpparser.h"
#include "http.h"
#include <asm-generic/socket.h>
#include <assert.h>
#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define TIMEOUT_SECS 10
#define MAX_FDS 1024

static http_data_t sock_data[MAX_FDS];

static void clear_http_data(http_data_t *sd) {
	sd->http_method = NULL;
	sd->http_path = NULL;
	sd->http_path_len = 0;
	sd->http_method_len = 0;
	sd->http_headers_len = 0;
	sd->buf_len = 0;
}

static void setup_sock(int fd) {
	int r;
	int on = 1;

	// NODELAY
	r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	assert(r == 0);

	// freebsd, then linux + macos
	#ifdef __FREEBSD__
		r = fcntl(fd, F_SETFL, SOCK_NONBLOCK);
	#else
		r = fcntl(fd, F_SETFL, O_NONBLOCK);
	#endif

	assert(r == 0);
}

static void close_conn(picoev_loop* loop, int fd) {
	printf("closing: %d\n", fd);
	close(fd);
	picoev_del(loop, fd);
}

static void http_cb(picoev_loop* loop, int fd, int events, void* cb_arg) {
	http_data_t *sd = &sock_data[fd];
	
	if ((events & PICOEV_TIMEOUT) != 0) {
		printf("timeout: %d\n", fd);
		close_conn(loop, fd);
		return;
	}

	if ((events & PICOEV_READ) != 0) {
		//printf("reading: %d\n", fd);
		//picoev_set_timeout(loop, fd, TIMEOUT_SECS);

		ssize_t r = read(fd, sd->buf + sd->buf_len, sizeof(sd->buf) - sd->buf_len);
		switch (r) {
			case 0: {
				// connection closed by peer
				goto error;
			}
			case -1: {
				// error
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// try again later
				} else {
					// fatal error
					goto error;
				}
				return;
			}
		}

		// has data
		size_t prevbuflen = sd->buf_len;
		sd->buf_len += r;

		int _minor_version;
		sd->http_headers_len = sizeof(sd->http_headers) / sizeof(sd->http_headers[0]);
		int pret = phr_parse_request(sd->buf, sd->buf_len, &sd->http_method, &sd->http_method_len, &sd->http_path, &sd->http_path_len, &_minor_version, sd->http_headers, &sd->http_headers_len, prevbuflen);

		if (pret > 0) {
			// success
			if (http_req(sd, fd) == 0) {
				// empty out, begin anew request
				clear_http_data(sd);
			} else {
				// close
				goto error;
			}
		} else if (pret == -1) {
			goto error;
		}
		assert(pret != -2); // request is incomplete

		// TODO: not implementing large requests
		if (sd->buf_len == sizeof(sd->buf)) {
			// request is too long
			goto error;
		}
		return;
	error:
		close_conn(loop, fd);
	}
}

static void accept_cb(picoev_loop* loop, int fd, int events, void* cb_arg) {
	int newfd = accept(fd, NULL, NULL);
	if (newfd != 1) {
		http_data_t *sd = &sock_data[newfd];

		// empty out, just to be sure
		clear_http_data(sd);

		printf("connected: %d\n", newfd);
		setup_sock(newfd);
		picoev_add(loop, newfd, PICOEV_READ, TIMEOUT_SECS, http_cb, NULL);
	}
}

int main(void) {
	picoev_loop* loop;
	int sock_fd;

	int flag = 1;
	assert((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) != 0);
	assert(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == 0);

	// epoll options for linux
	#ifndef __FREEBSD__
		assert(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) == 0);
		assert(setsockopt(sock_fd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag)) == 0);
		assert(setsockopt(sock_fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &flag, sizeof(flag)) == 0);
		const int max_queue = 4096;
		assert(setsockopt(sock_fd, IPPROTO_TCP, TCP_FASTOPEN, &max_queue, sizeof(max_queue)) == 0);
	#endif

	struct sockaddr_in listen_addr;
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(8080);
	listen_addr.sin_addr.s_addr = htonl(0x7f000001); // localhost
	assert(bind(sock_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == 0);

	assert(listen(sock_fd, 10) == 0);
	setup_sock(sock_fd);

	printf("listening http://localhost:8080\n");

	picoev_init(1024); // 1024 max FDs
	loop = picoev_create_loop(60);

	picoev_add(loop, sock_fd, PICOEV_READ, 0, accept_cb, NULL);

	for (;;) {
		picoev_loop_once(loop, 10);
	}

	picoev_destroy_loop(loop);
	picoev_deinit();

	return 0;
}
