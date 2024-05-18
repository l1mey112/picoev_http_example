#include <stddef.h>
#include "picohttpparser/picohttpparser.h"

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
