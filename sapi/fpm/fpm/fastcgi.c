/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2015 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/

/* $Id: fastcgi.c 287777 2009-08-26 19:17:32Z pajoye $ */

#include "php.h"
#include "fastcgi.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

#include <php_config.h>
#include "fpm.h"
#include "fpm_request.h"
#include "zlog.h"

#ifdef _WIN32

#include <windows.h>

	struct sockaddr_un {
		short   sun_family;
		char    sun_path[MAXPATHLEN];
	};

	static HANDLE fcgi_accept_mutex = INVALID_HANDLE_VALUE;
	static int is_impersonate = 0;

#define FCGI_LOCK(fd) \
	if (fcgi_accept_mutex != INVALID_HANDLE_VALUE) { \
		DWORD ret; \
		while ((ret = WaitForSingleObject(fcgi_accept_mutex, 1000)) == WAIT_TIMEOUT) { \
			if (in_shutdown) return -1; \
		} \
		if (ret == WAIT_FAILED) { \
			fprintf(stderr, "WaitForSingleObject() failed\n"); \
			return -1; \
		} \
	}

#define FCGI_UNLOCK(fd) \
	if (fcgi_accept_mutex != INVALID_HANDLE_VALUE) { \
		ReleaseMutex(fcgi_accept_mutex); \
	}

#else

# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <signal.h>

# define closesocket(s) close(s)

# if defined(HAVE_SYS_POLL_H) && defined(HAVE_POLL)
#  include <sys/poll.h>
# endif
# if defined(HAVE_SYS_SELECT_H)
#  include <sys/select.h>
# endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

# ifndef HAVE_SOCKLEN_T
	typedef unsigned int socklen_t;
# endif

# ifdef USE_LOCKING
#  define FCGI_LOCK(fd)								\
	do {											\
		struct flock lock;							\
		lock.l_type = F_WRLCK;						\
		lock.l_start = 0;							\
		lock.l_whence = SEEK_SET;					\
		lock.l_len = 0;								\
		if (fcntl(fd, F_SETLKW, &lock) != -1) {		\
			break;									\
		} else if (errno != EINTR || in_shutdown) {	\
			return -1;								\
		}											\
	} while (1)

#  define FCGI_UNLOCK(fd)							\
	do {											\
		int orig_errno = errno;						\
		while (1) {									\
			struct flock lock;						\
			lock.l_type = F_UNLCK;					\
			lock.l_start = 0;						\
			lock.l_whence = SEEK_SET;				\
			lock.l_len = 0;							\
			if (fcntl(fd, F_SETLK, &lock) != -1) {	\
				break;								\
			} else if (errno != EINTR) {			\
				return -1;							\
			}										\
		}											\
		errno = orig_errno;							\
	} while (0)
# else
#  define FCGI_LOCK(fd)
#  define FCGI_UNLOCK(fd)
# endif

#endif

typedef union _sa_t {
	struct sockaddr     sa;
	struct sockaddr_un  sa_unix;
	struct sockaddr_in  sa_inet;
	struct sockaddr_in6 sa_inet6;
} sa_t;

static HashTable fcgi_mgmt_vars;

static int is_initialized = 0;
static int in_shutdown = 0;
static sa_t *allowed_clients = NULL;

static sa_t client_sa;

/* hash table */

#define FCGI_HASH_TABLE_SIZE 128
#define FCGI_HASH_TABLE_MASK (FCGI_HASH_TABLE_SIZE - 1)
#define FCGI_HASH_SEG_SIZE   4096

typedef struct _fcgi_hash_bucket {
	unsigned int              hash_value;
	unsigned int              var_len;
	char                     *var;
	unsigned int              val_len;
	char                     *val;
	struct _fcgi_hash_bucket *next;
	struct _fcgi_hash_bucket *list_next;
} fcgi_hash_bucket;

typedef struct _fcgi_hash_buckets {
	unsigned int	           idx;
	struct _fcgi_hash_buckets *next;
	struct _fcgi_hash_bucket   data[FCGI_HASH_TABLE_SIZE];
} fcgi_hash_buckets;

typedef struct _fcgi_data_seg {
	char                  *pos;
	char                  *end;
	struct _fcgi_data_seg *next;
	char                   data[1];
} fcgi_data_seg;

typedef struct _fcgi_hash {
	fcgi_hash_bucket  *hash_table[FCGI_HASH_TABLE_SIZE];
	fcgi_hash_bucket  *list;
	fcgi_hash_buckets *buckets;
	fcgi_data_seg     *data;
} fcgi_hash;

static void fcgi_hash_init(fcgi_hash *h)
{
	memset(h->hash_table, 0, sizeof(h->hash_table));
	h->list = NULL;
	h->buckets = (fcgi_hash_buckets*)malloc(sizeof(fcgi_hash_buckets));
	h->buckets->idx = 0;
	h->buckets->next = NULL;
	h->data = (fcgi_data_seg*)malloc(sizeof(fcgi_data_seg) - 1 + FCGI_HASH_SEG_SIZE);
	h->data->pos = h->data->data;
	h->data->end = h->data->pos + FCGI_HASH_SEG_SIZE;
	h->data->next = NULL;
}

static void fcgi_hash_destroy(fcgi_hash *h)
{
	fcgi_hash_buckets *b;
	fcgi_data_seg *p;

	b = h->buckets;
	while (b) {
		fcgi_hash_buckets *q = b;
		b = b->next;
		free(q);
	}
	p = h->data;
	while (p) {
		fcgi_data_seg *q = p;
		p = p->next;
		free(q);
	}
}

static void fcgi_hash_clean(fcgi_hash *h)
{
	memset(h->hash_table, 0, sizeof(h->hash_table));
	h->list = NULL;
	/* delete all bucket blocks except the first one */
	while (h->buckets->next) {
		fcgi_hash_buckets *q = h->buckets;

		h->buckets = h->buckets->next;
		free(q);
	}
	h->buckets->idx = 0;
	/* delete all data segments except the first one */
	while (h->data->next) {
		fcgi_data_seg *q = h->data;

		h->data = h->data->next;
		free(q);
	}
	h->data->pos = h->data->data;
}

static inline char* fcgi_hash_strndup(fcgi_hash *h, char *str, unsigned int str_len)
{
	char *ret;

	if (UNEXPECTED(h->data->pos + str_len + 1 >= h->data->end)) {
		unsigned int seg_size = (str_len + 1 > FCGI_HASH_SEG_SIZE) ? str_len + 1 : FCGI_HASH_SEG_SIZE;
		fcgi_data_seg *p = (fcgi_data_seg*)malloc(sizeof(fcgi_data_seg) - 1 + seg_size);

		p->pos = p->data;
		p->end = p->pos + seg_size;
		p->next = h->data;
		h->data = p;
	}
	ret = h->data->pos;
	memcpy(ret, str, str_len);
	ret[str_len] = 0;
	h->data->pos += str_len + 1;
	return ret;
}

static char* fcgi_hash_set(fcgi_hash *h, unsigned int hash_value, char *var, unsigned int var_len, char *val, unsigned int val_len)
{
	unsigned int      idx = hash_value & FCGI_HASH_TABLE_MASK;
	fcgi_hash_bucket *p = h->hash_table[idx];

	while (UNEXPECTED(p != NULL)) {
		if (UNEXPECTED(p->hash_value == hash_value) &&
		    p->var_len == var_len &&
		    memcmp(p->var, var, var_len) == 0) {

			p->val_len = val_len;
			p->val = fcgi_hash_strndup(h, val, val_len);
			return p->val;
		}
		p = p->next;
	}

	if (UNEXPECTED(h->buckets->idx >= FCGI_HASH_TABLE_SIZE)) {
		fcgi_hash_buckets *b = (fcgi_hash_buckets*)malloc(sizeof(fcgi_hash_buckets));
		b->idx = 0;
		b->next = h->buckets;
		h->buckets = b;
	}
	p = h->buckets->data + h->buckets->idx;
	h->buckets->idx++;
	p->next = h->hash_table[idx];
	h->hash_table[idx] = p;
	p->list_next = h->list;
	h->list = p;
	p->hash_value = hash_value;
	p->var_len = var_len;
	p->var = fcgi_hash_strndup(h, var, var_len);
	p->val_len = val_len;
	p->val = fcgi_hash_strndup(h, val, val_len);
	return p->val;
}

static void fcgi_hash_del(fcgi_hash *h, unsigned int hash_value, char *var, unsigned int var_len)
{
	unsigned int      idx = hash_value & FCGI_HASH_TABLE_MASK;
	fcgi_hash_bucket **p = &h->hash_table[idx];

	while (*p != NULL) {
		if ((*p)->hash_value == hash_value &&
		    (*p)->var_len == var_len &&
		    memcmp((*p)->var, var, var_len) == 0) {

		    (*p)->val = NULL; /* NULL value means deleted */
		    (*p)->val_len = 0;
			*p = (*p)->next;
		    return;
		}
		p = &(*p)->next;
	}
}

static char *fcgi_hash_get(fcgi_hash *h, unsigned int hash_value, char *var, unsigned int var_len, unsigned int *val_len)
{
	unsigned int      idx = hash_value & FCGI_HASH_TABLE_MASK;
	fcgi_hash_bucket *p = h->hash_table[idx];

	while (p != NULL) {
		if (p->hash_value == hash_value &&
		    p->var_len == var_len &&
		    memcmp(p->var, var, var_len) == 0) {
		    *val_len = p->val_len;
		    return p->val;
		}
		p = p->next;
	}
	return NULL;
}

static void fcgi_hash_apply(fcgi_hash *h, fcgi_apply_func func, void *arg)
{
	fcgi_hash_bucket *p	= h->list;

	while (p) {
		if (EXPECTED(p->val != NULL)) {
			func(p->var, p->var_len, p->val, p->val_len, arg);
		}
		p = p->list_next;
	}
}

struct _fcgi_request {
	int            listen_socket;
#ifdef _WIN32
	int            tcp;
#endif
	int            fd;
	int            id;
	int            keep;
#ifdef TCP_NODELAY
	int            nodelay;
#endif
	int            closed;

	int            in_len;
	int            in_pad;

	fcgi_header   *out_hdr;
	unsigned char *out_pos;
	unsigned char  out_buf[1024*8];
	unsigned char  reserved[sizeof(fcgi_end_request_rec)];

	int            has_env;
	fcgi_hash      env;
};

#ifdef _WIN32

static DWORD WINAPI fcgi_shutdown_thread(LPVOID arg)
{
	HANDLE shutdown_event = (HANDLE) arg;
	WaitForSingleObject(shutdown_event, INFINITE);
	in_shutdown = 1;
	return 0;
}

#else

static void fcgi_signal_handler(int signo)
{
	if (signo == SIGUSR1 || signo == SIGTERM) {
		in_shutdown = 1;
	}
}

static void fcgi_setup_signals(void)
{
	struct sigaction new_sa, old_sa;

	sigemptyset(&new_sa.sa_mask);
	new_sa.sa_flags = 0;
	new_sa.sa_handler = fcgi_signal_handler;
	sigaction(SIGUSR1, &new_sa, NULL);
	sigaction(SIGTERM, &new_sa, NULL);
	sigaction(SIGPIPE, NULL, &old_sa);
	if (old_sa.sa_handler == SIG_DFL) {
		sigaction(SIGPIPE, &new_sa, NULL);
	}
}
#endif

int fcgi_in_shutdown(void)
{
	return in_shutdown;
}

void fcgi_terminate(void)
{
	in_shutdown = 1;
}

int fcgi_init(void)
{
	if (!is_initialized) {
		zend_hash_init(&fcgi_mgmt_vars, 8, NULL, fcgi_free_mgmt_var_cb, 1);
		fcgi_set_mgmt_var("FCGI_MPXS_CONNS", sizeof("FCGI_MPXS_CONNS") - 1, "0", sizeof("0")-1);

		is_initialized = 1;
#ifdef _WIN32
# if 0
		/* TODO: Support for TCP sockets */
		WSADATA wsaData;

		if (WSAStartup(MAKEWORD(2,0), &wsaData)) {
			fprintf(stderr, "Error starting Windows Sockets.  Error: %d", WSAGetLastError());
			return 0;
		}
# endif
		{
			char *str;
			DWORD pipe_mode = PIPE_READMODE_BYTE | PIPE_WAIT;
			HANDLE pipe = GetStdHandle(STD_INPUT_HANDLE);

			SetNamedPipeHandleState(pipe, &pipe_mode, NULL, NULL);

			str = getenv("_FCGI_SHUTDOWN_EVENT_");
			if (str != NULL) {
				HANDLE shutdown_event = (HANDLE) atoi(str);
				if (!CreateThread(NULL, 0, fcgi_shutdown_thread,
				                  shutdown_event, 0, NULL)) {
					return -1;
				}
			}
			str = getenv("_FCGI_MUTEX_");
			if (str != NULL) {
				fcgi_accept_mutex = (HANDLE) atoi(str);
			}
			return 1;
		}
#else
		fcgi_setup_signals();
		return 1;
#endif
	}
	return 1;
}

void fcgi_set_in_shutdown(int new_value)
{
	in_shutdown = new_value;
}

void fcgi_shutdown(void)
{
	if (is_initialized) {
		zend_hash_destroy(&fcgi_mgmt_vars);
	}
	if (allowed_clients) {
		free(allowed_clients);
	}
}

void fcgi_set_allowed_clients(char *ip)
{
	char *cur, *end;
	int n;

	if (ip) {
		ip = strdup(ip);
		cur = ip;
		n = 0;
		while (*cur) {
			if (*cur == ',') n++;
			cur++;
		}
		if (allowed_clients) free(allowed_clients);
		allowed_clients = malloc(sizeof(sa_t) * (n+2));
		n = 0;
		cur = ip;
		while (cur) {
			end = strchr(cur, ',');
			if (end) {
				*end = 0;
				end++;
			}
			if (inet_pton(AF_INET, cur, &allowed_clients[n].sa_inet.sin_addr)>0) {
				allowed_clients[n].sa.sa_family = AF_INET;
				n++;
			} else if (inet_pton(AF_INET6, cur, &allowed_clients[n].sa_inet6.sin6_addr)>0) {
				allowed_clients[n].sa.sa_family = AF_INET6;
				n++;
			} else {
				zlog(ZLOG_ERROR, "Wrong IP address '%s' in listen.allowed_clients", cur);
			}
			cur = end;
		}
		allowed_clients[n].sa.sa_family = 0;
		free(ip);
		if (!n) {
			zlog(ZLOG_ERROR, "There are no allowed addresses for this pool");
			/* don't clear allowed_clients as it will create an "open for all" security issue */
		}
	}
}

fcgi_request *fcgi_init_request(int listen_socket)
{
	fcgi_request *req = (fcgi_request*)calloc(1, sizeof(fcgi_request));
	req->listen_socket = listen_socket;
	req->fd = -1;
	req->id = -1;

	req->in_len = 0;
	req->in_pad = 0;

	req->out_hdr = NULL;
	req->out_pos = req->out_buf;

#ifdef _WIN32
	req->tcp = !GetNamedPipeInfo((HANDLE)_get_osfhandle(req->listen_socket), NULL, NULL, NULL, NULL);
#endif

#ifdef TCP_NODELAY
	req->nodelay = 0;
#endif

	fcgi_hash_init(&req->env);

	return req;
}

void fcgi_destroy_request(fcgi_request *req)
{
	fcgi_hash_destroy(&req->env);
	free(req);
}

static inline ssize_t safe_write(fcgi_request *req, const void *buf, size_t count)
{
	int    ret;
	size_t n = 0;

	do {
		errno = 0;
#ifdef _WIN32
		if (!req->tcp) {
			ret = write(req->fd, ((char*)buf)+n, count-n);
		} else {
			ret = send(req->fd, ((char*)buf)+n, count-n, 0);
			if (ret <= 0) {
				errno = WSAGetLastError();
			}
		}
#else
		ret = write(req->fd, ((char*)buf)+n, count-n);
#endif
		if (ret > 0) {
			n += ret;
		} else if (ret <= 0 && errno != 0 && errno != EINTR) {
			return ret;
		}
	} while (n != count);
	return n;
}

static inline ssize_t safe_read(fcgi_request *req, const void *buf, size_t count)
{
	int    ret;
	size_t n = 0;

	do {
		errno = 0;
#ifdef _WIN32
		if (!req->tcp) {
			ret = read(req->fd, ((char*)buf)+n, count-n);
		} else {
			ret = recv(req->fd, ((char*)buf)+n, count-n, 0);
			if (ret <= 0) {
				errno = WSAGetLastError();
			}
		}
#else
		ret = read(req->fd, ((char*)buf)+n, count-n);
#endif
		if (ret > 0) {
			n += ret;
		} else if (ret == 0 && errno == 0) {
			return n;
		} else if (ret <= 0 && errno != 0 && errno != EINTR) {
			return ret;
		}
	} while (n != count);
	return n;
}

static inline int fcgi_make_header(fcgi_header *hdr, fcgi_request_type type, int req_id, int len)
{
	int pad = ((len + 7) & ~7) - len;

	hdr->contentLengthB0 = (unsigned char)(len & 0xff);
	hdr->contentLengthB1 = (unsigned char)((len >> 8) & 0xff);
	hdr->paddingLength = (unsigned char)pad;
	hdr->requestIdB0 = (unsigned char)(req_id & 0xff);
	hdr->requestIdB1 = (unsigned char)((req_id >> 8) & 0xff);
	hdr->reserved = 0;
	hdr->type = type;
	hdr->version = FCGI_VERSION_1;
	if (pad) {
		memset(((unsigned char*)hdr) + sizeof(fcgi_header) + len, 0, pad);
	}
	return pad;
}

static int fcgi_get_params(fcgi_request *req, unsigned char *p, unsigned char *end)
{
	int name_len = 0;
	int val_len = 0;

	while (p < end) {
		name_len = *p++;
		if (UNEXPECTED(name_len >= 128)) {
			if (UNEXPECTED(p + 3 >= end)) return 0;
			name_len = ((name_len & 0x7f) << 24);
			name_len |= (*p++ << 16);
			name_len |= (*p++ << 8);
			name_len |= *p++;
		}
		if (UNEXPECTED(p >= end)) return 0;
		val_len = *p++;
		if (UNEXPECTED(val_len >= 128)) {
			if (UNEXPECTED(p + 3 >= end)) return 0;
			val_len = ((val_len & 0x7f) << 24);
			val_len |= (*p++ << 16);
			val_len |= (*p++ << 8);
			val_len |= *p++;
		}
		if (UNEXPECTED(name_len > (INT_MAX - val_len)) || /* would the addition overflow? */
		    UNEXPECTED(name_len + val_len > end - p)) {   /* would we exceed the buffer? */
			/* Malformated request */
			return 0;
		}

		/*
		 * get the effective length of the name in case it's not a valid string
		 * don't do this on the value because it can be binary data
		 */
		if (UNEXPECTED(memchr(p, 0, name_len) != NULL)) {
			/* Malicious request */
			return 0;
		}
		fcgi_hash_set(&req->env, FCGI_HASH_FUNC(p, name_len), (char*)p, name_len, (char*)p + name_len, val_len);
		p += name_len + val_len;
	}
	return 1;
}

static void fcgi_free_var(zval *zv)
{
	efree(Z_PTR_P(zv));
}

static int fcgi_read_request(fcgi_request *req)
{
	fcgi_header hdr;
	int len, padding;
	unsigned char buf[FCGI_MAX_LENGTH+8];

	req->keep = 0;
	req->closed = 0;
	req->in_len = 0;
	req->out_hdr = NULL;
	req->out_pos = req->out_buf;
	req->has_env = 1;

	if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) ||
	    hdr.version < FCGI_VERSION_1) {
		return 0;
	}

	len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
	padding = hdr.paddingLength;

	while (hdr.type == FCGI_STDIN && len == 0) {
		if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) ||
		    hdr.version < FCGI_VERSION_1) {
			return 0;
		}

		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;
	}

	if (len + padding > FCGI_MAX_LENGTH) {
		return 0;
	}

	req->id = (hdr.requestIdB1 << 8) + hdr.requestIdB0;

	if (hdr.type == FCGI_BEGIN_REQUEST && len == sizeof(fcgi_begin_request)) {
		if (safe_read(req, buf, len+padding) != len+padding) {
			return 0;
		}

		req->keep = (((fcgi_begin_request*)buf)->flags & FCGI_KEEP_CONN);
#ifdef TCP_NODELAY
		if (req->keep && req->tcp && !req->nodelay) {
# ifdef _WIN32
			BOOL on = 1;
# else
			int on = 1;
# endif

			setsockopt(req->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
			req->nodelay = 1;
		}
#endif
		switch ((((fcgi_begin_request*)buf)->roleB1 << 8) + ((fcgi_begin_request*)buf)->roleB0) {
			case FCGI_RESPONDER:
				fcgi_hash_set(&req->env, FCGI_HASH_FUNC("FCGI_ROLE", sizeof("FCGI_ROLE")-1), "FCGI_ROLE", sizeof("FCGI_ROLE")-1, "RESPONDER", sizeof("RESPONDER")-1);
				break;
			case FCGI_AUTHORIZER:
				fcgi_hash_set(&req->env, FCGI_HASH_FUNC("FCGI_ROLE", sizeof("FCGI_ROLE")-1), "FCGI_ROLE", sizeof("FCGI_ROLE")-1, "AUTHORIZER", sizeof("AUTHORIZER")-1);
				break;
			case FCGI_FILTER:
				fcgi_hash_set(&req->env, FCGI_HASH_FUNC("FCGI_ROLE", sizeof("FCGI_ROLE")-1), "FCGI_ROLE", sizeof("FCGI_ROLE")-1, "FILTER", sizeof("FILTER")-1);
				break;
			default:
				return 0;
		}

		if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) ||
		    hdr.version < FCGI_VERSION_1) {
			return 0;
		}

		len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
		padding = hdr.paddingLength;

		while (hdr.type == FCGI_PARAMS && len > 0) {
			if (len + padding > FCGI_MAX_LENGTH) {
				return 0;
			}

			if (safe_read(req, buf, len+padding) != len+padding) {
				req->keep = 0;
				return 0;
			}

			if (!fcgi_get_params(req, buf, buf+len)) {
				req->keep = 0;
				return 0;
			}

			if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) ||
			    hdr.version < FCGI_VERSION_1) {
				req->keep = 0;
				return 0;
			}
			len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			padding = hdr.paddingLength;
		}
	} else if (hdr.type == FCGI_GET_VALUES) {
		unsigned char *p = buf + sizeof(fcgi_header);
		zval *value;
		unsigned int zlen;
		fcgi_hash_bucket *q;

		if (safe_read(req, buf, len+padding) != len+padding) {
			req->keep = 0;
			return 0;
		}

		if (!fcgi_get_params(req, buf, buf+len)) {
			req->keep = 0;
			return 0;
		}

		q = req->env.list;
		while (q != NULL) {
			if ((value = zend_hash_str_find(&fcgi_mgmt_vars, q->var, q->var_len)) == NULL) {
				q = q->list_next;
				continue;
			}
			zlen = (unsigned int)Z_STRLEN_P(value);
			if ((p + 4 + 4 + q->var_len + zlen) >= (buf + sizeof(buf))) {
				break;
			}
			if (q->var_len < 0x80) {
				*p++ = q->var_len;
			} else {
				*p++ = ((q->var_len >> 24) & 0xff) | 0x80;
				*p++ = (q->var_len >> 16) & 0xff;
				*p++ = (q->var_len >> 8) & 0xff;
				*p++ = q->var_len & 0xff;
			}
			if (zlen < 0x80) {
				*p++ = zlen;
			} else {
				*p++ = ((zlen >> 24) & 0xff) | 0x80;
				*p++ = (zlen >> 16) & 0xff;
				*p++ = (zlen >> 8) & 0xff;
				*p++ = zlen & 0xff;
			}
			memcpy(p, q->var, q->var_len);
			p += q->var_len;
			memcpy(p, Z_STRVAL_P(value), zlen);
			p += zlen;
			q = q->list_next;
		}
		len = p - buf - sizeof(fcgi_header);
		len += fcgi_make_header((fcgi_header*)buf, FCGI_GET_VALUES_RESULT, 0, len);
		if (safe_write(req, buf, sizeof(fcgi_header)+len) != (int)sizeof(fcgi_header)+len) {
			req->keep = 0;
			return 0;
		}
		return 0;
	} else {
		return 0;
	}

	return 1;
}

int fcgi_read(fcgi_request *req, char *str, int len)
{
	int ret, n, rest;
	fcgi_header hdr;
	unsigned char buf[255];

	n = 0;
	rest = len;
	while (rest > 0) {
		if (req->in_len == 0) {
			if (safe_read(req, &hdr, sizeof(fcgi_header)) != sizeof(fcgi_header) ||
			    hdr.version < FCGI_VERSION_1 ||
			    hdr.type != FCGI_STDIN) {
				req->keep = 0;
				return 0;
			}
			req->in_len = (hdr.contentLengthB1 << 8) | hdr.contentLengthB0;
			req->in_pad = hdr.paddingLength;
			if (req->in_len == 0) {
				return n;
			}
		}

		if (req->in_len >= rest) {
			ret = safe_read(req, str, rest);
		} else {
			ret = safe_read(req, str, req->in_len);
		}
		if (ret < 0) {
			req->keep = 0;
			return ret;
		} else if (ret > 0) {
			req->in_len -= ret;
			rest -= ret;
			n += ret;
			str += ret;
			if (req->in_len == 0) {
				if (req->in_pad) {
					if (safe_read(req, buf, req->in_pad) != req->in_pad) {
						req->keep = 0;
						return ret;
					}
				}
			} else {
				return n;
			}
		} else {
			return n;
		}
	}
	return n;
}

void fcgi_close(fcgi_request *req, int force, int destroy)
{
	if (destroy && req->has_env) {
		fcgi_hash_clean(&req->env);
		req->has_env = 0;
	}

#ifdef _WIN32
	if (is_impersonate && !req->tcp) {
		RevertToSelf();
	}
#endif

	if ((force || !req->keep) && req->fd >= 0) {
#ifdef _WIN32
		if (!req->tcp) {
			HANDLE pipe = (HANDLE)_get_osfhandle(req->fd);

			if (!force) {
				FlushFileBuffers(pipe);
			}
			DisconnectNamedPipe(pipe);
		} else {
			if (!force) {
				char buf[8];

				shutdown(req->fd, 1);
				while (recv(req->fd, buf, sizeof(buf), 0) > 0) {}
			}
			closesocket(req->fd);
		}
#else
		if (!force) {
			char buf[8];

			shutdown(req->fd, 1);
			while (recv(req->fd, buf, sizeof(buf), 0) > 0) {}
		}
		close(req->fd);
#endif
#ifdef TCP_NODELAY
		req->nodelay = 0;
#endif
		req->fd = -1;
		fpm_request_finished();
	}
}

int fcgi_is_closed(fcgi_request *req)
{
	return (req->fd < 0);
}

static int fcgi_is_allowed() {
	int i;

	if (client_sa.sa.sa_family == AF_UNIX) {
		return 1;
	}
	if (!allowed_clients) {
		return 1;
	}
	if (client_sa.sa.sa_family == AF_INET) {
		for (i=0 ; allowed_clients[i].sa.sa_family ; i++) {
			if (allowed_clients[i].sa.sa_family == AF_INET
				&& !memcmp(&client_sa.sa_inet.sin_addr, &allowed_clients[i].sa_inet.sin_addr, 4)) {
				return 1;
			}
		}
	}
	if (client_sa.sa.sa_family == AF_INET6) {
		for (i=0 ; allowed_clients[i].sa.sa_family ; i++) {
			if (allowed_clients[i].sa.sa_family == AF_INET6
				&& !memcmp(&client_sa.sa_inet6.sin6_addr, &allowed_clients[i].sa_inet6.sin6_addr, 12)) {
				return 1;
			}
#ifdef IN6_IS_ADDR_V4MAPPED
			if (allowed_clients[i].sa.sa_family == AF_INET
			    && IN6_IS_ADDR_V4MAPPED(&client_sa.sa_inet6.sin6_addr)
				&& !memcmp(((char *)&client_sa.sa_inet6.sin6_addr)+12, &allowed_clients[i].sa_inet.sin_addr, 4)) {
				return 1;
			}
#endif
		}
	}

	zlog(ZLOG_ERROR, "Connection disallowed: IP address '%s' has been dropped.", fcgi_get_last_client_ip());
	return 0;
}

int fcgi_accept_request(fcgi_request *req)
{
#ifdef _WIN32
	HANDLE pipe;
	OVERLAPPED ov;
#endif

	while (1) {
		if (req->fd < 0) {
			while (1) {
				if (in_shutdown) {
					return -1;
				}
#ifdef _WIN32
				if (!req->tcp) {
					pipe = (HANDLE)_get_osfhandle(req->listen_socket);
					FCGI_LOCK(req->listen_socket);
					ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					if (!ConnectNamedPipe(pipe, &ov)) {
						errno = GetLastError();
						if (errno == ERROR_IO_PENDING) {
							while (WaitForSingleObject(ov.hEvent, 1000) == WAIT_TIMEOUT) {
								if (in_shutdown) {
									CloseHandle(ov.hEvent);
									FCGI_UNLOCK(req->listen_socket);
									return -1;
								}
							}
						} else if (errno != ERROR_PIPE_CONNECTED) {
						}
					}
					CloseHandle(ov.hEvent);
					req->fd = req->listen_socket;
					FCGI_UNLOCK(req->listen_socket);
				} else {
					SOCKET listen_socket = (SOCKET)_get_osfhandle(req->listen_socket);
#else
				{
					int listen_socket = req->listen_socket;
#endif
					sa_t sa;
					socklen_t len = sizeof(sa);

					fpm_request_accepting();

					FCGI_LOCK(req->listen_socket);
					req->fd = accept(listen_socket, (struct sockaddr *)&sa, &len);
					FCGI_UNLOCK(req->listen_socket);

					client_sa = sa;
					if (req->fd >= 0 && !fcgi_is_allowed()) {
						closesocket(req->fd);
						req->fd = -1;
						continue;
					}
				}

#ifdef _WIN32
				if (req->fd < 0 && (in_shutdown || errno != EINTR)) {
#else
				if (req->fd < 0 && (in_shutdown || (errno != EINTR && errno != ECONNABORTED))) {
#endif
					return -1;
				}

#ifdef _WIN32
				break;
#else
				if (req->fd >= 0) {
#if defined(HAVE_SYS_POLL_H) && defined(HAVE_POLL)
					struct pollfd fds;
					int ret;

					fpm_request_reading_headers();

					fds.fd = req->fd;
					fds.events = POLLIN;
					fds.revents = 0;
					do {
						errno = 0;
						ret = poll(&fds, 1, 5000);
					} while (ret < 0 && errno == EINTR);
					if (ret > 0 && (fds.revents & POLLIN)) {
						break;
					}
					fcgi_close(req, 1, 0);
#else
					fpm_request_reading_headers();

					if (req->fd < FD_SETSIZE) {
						struct timeval tv = {5,0};
						fd_set set;
						int ret;

						FD_ZERO(&set);
						FD_SET(req->fd, &set);
						do {
							errno = 0;
							ret = select(req->fd + 1, &set, NULL, NULL, &tv) >= 0;
						} while (ret < 0 && errno == EINTR);
						if (ret > 0 && FD_ISSET(req->fd, &set)) {
							break;
						}
						fcgi_close(req, 1, 0);
					} else {
						zlog(ZLOG_ERROR, "Too many open file descriptors. FD_SETSIZE limit exceeded.");
						fcgi_close(req, 1, 0);
					}
#endif
				}
#endif
			}
		} else if (in_shutdown) {
			return -1;
		}
		if (fcgi_read_request(req)) {
#ifdef _WIN32
			if (is_impersonate && !req->tcp) {
				pipe = (HANDLE)_get_osfhandle(req->fd);
				if (!ImpersonateNamedPipeClient(pipe)) {
					fcgi_close(req, 1, 1);
					continue;
				}
			}
#endif
			return req->fd;
		} else {
			fcgi_close(req, 1, 1);
		}
	}
}

static inline fcgi_header* open_packet(fcgi_request *req, fcgi_request_type type)
{
	req->out_hdr = (fcgi_header*) req->out_pos;
	req->out_hdr->type = type;
	req->out_pos += sizeof(fcgi_header);
	return req->out_hdr;
}

static inline void close_packet(fcgi_request *req)
{
	if (req->out_hdr) {
		int len = req->out_pos - ((unsigned char*)req->out_hdr + sizeof(fcgi_header));

		req->out_pos += fcgi_make_header(req->out_hdr, (fcgi_request_type)req->out_hdr->type, req->id, len);
		req->out_hdr = NULL;
	}
}

int fcgi_flush(fcgi_request *req, int close)
{
	int len;

	close_packet(req);

	len = req->out_pos - req->out_buf;

	if (close) {
		fcgi_end_request_rec *rec = (fcgi_end_request_rec*)(req->out_pos);

		fcgi_make_header(&rec->hdr, FCGI_END_REQUEST, req->id, sizeof(fcgi_end_request));
		rec->body.appStatusB3 = 0;
		rec->body.appStatusB2 = 0;
		rec->body.appStatusB1 = 0;
		rec->body.appStatusB0 = 0;
		rec->body.protocolStatus = FCGI_REQUEST_COMPLETE;
		len += sizeof(fcgi_end_request_rec);
	}

	if (safe_write(req, req->out_buf, len) != len) {
		req->keep = 0;
		req->out_pos = req->out_buf;
		return 0;
	}

	req->out_pos = req->out_buf;
	return 1;
}

ssize_t fcgi_write(fcgi_request *req, fcgi_request_type type, const char *str, int len)
{
	int limit, rest;

	if (len <= 0) {
		return 0;
	}

	if (req->out_hdr && req->out_hdr->type != type) {
		close_packet(req);
	}

	/* Optimized version */
	limit = sizeof(req->out_buf) - (req->out_pos - req->out_buf);
	if (!req->out_hdr) {
		limit -= sizeof(fcgi_header);
		if (limit < 0) limit = 0;
	}

	if (len < limit) {
		if (!req->out_hdr) {
			open_packet(req, type);
		}
		memcpy(req->out_pos, str, len);
		req->out_pos += len;
	} else if (len - limit < sizeof(req->out_buf) - sizeof(fcgi_header)) {
		if (!req->out_hdr) {
			open_packet(req, type);
		}
		if (limit > 0) {
			memcpy(req->out_pos, str, limit);
			req->out_pos += limit;
		}
		if (!fcgi_flush(req, 0)) {
			return -1;
		}
		if (len > limit) {
			open_packet(req, type);
			memcpy(req->out_pos, str + limit, len - limit);
			req->out_pos += len - limit;
		}
	} else {
		int pos = 0;
		int pad;

		close_packet(req);
		while ((len - pos) > 0xffff) {
			open_packet(req, type);
			fcgi_make_header(req->out_hdr, type, req->id, 0xfff8);
			req->out_hdr = NULL;
			if (!fcgi_flush(req, 0)) {
				return -1;
			}
			if (safe_write(req, str + pos, 0xfff8) != 0xfff8) {
				req->keep = 0;
				return -1;
			}
			pos += 0xfff8;
		}

		pad = (((len - pos) + 7) & ~7) - (len - pos);
		rest = pad ? 8 - pad : 0;

		open_packet(req, type);
		fcgi_make_header(req->out_hdr, type, req->id, (len - pos) - rest);
		req->out_hdr = NULL;
		if (!fcgi_flush(req, 0)) {
			return -1;
		}
		if (safe_write(req, str + pos, (len - pos) - rest) != (len - pos) - rest) {
			req->keep = 0;
			return -1;
		}
		if (pad) {
			open_packet(req, type);
			memcpy(req->out_pos, str + len - rest,  rest);
			req->out_pos += rest;
		}
	}

	return len;
}

int fcgi_finish_request(fcgi_request *req, int force_close)
{
	int ret = 1;

	if (req->fd >= 0) {
		if (!req->closed) {
			ret = fcgi_flush(req, 1);
			req->closed = 1;
		}
		fcgi_close(req, force_close, 1);
	}
	return ret;
}

char* fcgi_getenv(fcgi_request *req, const char* var, int var_len)
{
	unsigned int val_len;

	if (!req) return NULL;

	return fcgi_hash_get(&req->env, FCGI_HASH_FUNC(var, var_len), (char*)var, var_len, &val_len);
}

char* fcgi_quick_getenv(fcgi_request *req, const char* var, int var_len, unsigned int hash_value)
{
	unsigned int val_len;

	return fcgi_hash_get(&req->env, hash_value, (char*)var, var_len, &val_len);
}

char* fcgi_putenv(fcgi_request *req, char* var, int var_len, char* val)
{
	if (!req) return NULL;
	if (val == NULL) {
		fcgi_hash_del(&req->env, FCGI_HASH_FUNC(var, var_len), var, var_len);
		return NULL;
	} else {
		return fcgi_hash_set(&req->env, FCGI_HASH_FUNC(var, var_len), var, var_len, val, (unsigned int)strlen(val));
	}
}

char* fcgi_quick_putenv(fcgi_request *req, char* var, int var_len, unsigned int hash_value, char* val)
{
	if (val == NULL) {
		fcgi_hash_del(&req->env, hash_value, var, var_len);
		return NULL;
	} else {
		return fcgi_hash_set(&req->env, hash_value, var, var_len, val, (unsigned int)strlen(val));
	}
}

void fcgi_loadenv(fcgi_request *req, fcgi_apply_func func, zval *array)
{
	fcgi_hash_apply(&req->env, func, array);
}

void fcgi_set_mgmt_var(const char * name, size_t name_len, const char * value, size_t value_len)
{
	zval zvalue;
	ZVAL_NEW_STR(&zvalue, zend_string_init(value, value_len, 1));
	zend_hash_str_add(&fcgi_mgmt_vars, name, name_len, &zvalue);
}

void fcgi_free_mgmt_var_cb(zval *zv)
{
	zend_string_free(Z_STR_P(zv));
}

const char *fcgi_get_last_client_ip() /* {{{ */
{
	static char str[INET6_ADDRSTRLEN];

	/* Ipv4 */
	if (client_sa.sa.sa_family == AF_INET) {
		return inet_ntop(client_sa.sa.sa_family, &client_sa.sa_inet.sin_addr, str, INET6_ADDRSTRLEN);
	}
#ifdef IN6_IS_ADDR_V4MAPPED
	/* Ipv4-Mapped-Ipv6 */
	if (client_sa.sa.sa_family == AF_INET6
		&& IN6_IS_ADDR_V4MAPPED(&client_sa.sa_inet6.sin6_addr)) {
		return inet_ntop(AF_INET, ((char *)&client_sa.sa_inet6.sin6_addr)+12, str, INET6_ADDRSTRLEN);
	}
#endif
	/* Ipv6 */
	if (client_sa.sa.sa_family == AF_INET6) {
		return inet_ntop(client_sa.sa.sa_family, &client_sa.sa_inet6.sin6_addr, str, INET6_ADDRSTRLEN);
	}
	/* Unix socket */
	return NULL;
}
/* }}} */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
