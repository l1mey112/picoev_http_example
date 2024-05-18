#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "http.h"

int http_req(http_data_t *req, int fd) {
	printf("method: %.*s\n", (int)req->http_method_len, req->http_method);
	printf("path: %.*s\n", (int)req->http_path_len, req->http_path);

	for (size_t i = 0; i < req->http_headers_len; i++) {
		printf("header: %.*s: %.*s\n", (int)req->http_headers[i].name_len, req->http_headers[i].name, (int)req->http_headers[i].value_len, req->http_headers[i].value);
	}

	const char *success = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 42\r\n"
		"\r\n"
		"<html><body><h1>success</h1></body></html>";

	if (write(fd, success, strlen(success)) != strlen(success)) {
		return -1; // close connection
	}

	return 0;
}
