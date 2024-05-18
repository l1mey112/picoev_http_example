# picoev_http_example

example using picoev and picohttpparser for a highly performant simple HTTP server for small places. **zero dynamic memory allocation.** supports linux + freebsd (and by extension darwin) building on from the setup in [picoev's echo example](https://github.com/kazuho/picoev/blob/ff85d9ef578842a40f7c91d2544b7932cec74b9d/example/picoev_echo.c) and code from [vlang's picoev implementation](https://modules.vlang.io/picoev.html) (some freebsd functionality merged by me a while ago)

```c
#define MAX_FDS 1024

typedef struct http_data_t http_data_t;

struct http_data_t {
	const char *http_method;
	const char *http_path;
	size_t http_path_len;
	size_t http_method_len;

	struct phr_header http_headers[32];
	size_t http_headers_len;

	char buf[4096];
	size_t buf_len;
};

int http_req(http_data_t *req, int fd);

static http_data_t sock_data[MAX_FDS];

// http_data_t *sd = sock_data[fd];
// ... parse
// cb_err = http_req(sd, fd);
```

```c
// see http.c
int http_req(http_data_t *req, int fd) {
	printf("method: %.*s\n", (int)req->http_method_len, req->http_method);
	printf("path: %.*s\n", (int)req->http_path_len, req->http_path);

	for (size_t i = 0; i < req->http_headers_len; i++) {
		printf("header: %.*s: %.*s\n", (int)req->http_headers[i].name_len,
			req->http_headers[i].name, (int)req->http_headers[i].value_len,
			req->http_headers[i].value);
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
```

# build and example

make sure to clone the submodules

```
$ git submodule update --init
...
$ make
cc http.c main.c picohttpparser/picohttpparser.c picoev/picoev_epoll.c -g -fsanitize=address
$ ./a.out
connected: 5
method: GET
path: /
header: Host: localhost:8080
header: User-Agent: curl/8.7.1
header: Accept: */*
closing: 5
```

```
$ curl -v 'http://localhost:8080/'
* Host localhost:8080 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:8080...
* connect to ::1 port 8080 from ::1 port 57138 failed: Connection refused
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080
> GET / HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/8.7.1
> Accept: */*
> 
* Request completely sent off
< HTTP/1.1 200 OK
< Content-Type: text/html
< Content-Length: 42
< 
* Connection #0 to host localhost left intact
<html><body><h1>success</h1></body></html>⏎
```