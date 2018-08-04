/*
 * Copyright (c) Christos Zoulas 2003.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "file.h"

#ifndef	lint
FILE_RCSID("@(#)$File: funcs.c,v 1.94 2017/11/02 20:25:39 christos Exp $")
#endif	/* lint */

#include "magic.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(HAVE_WCHAR_H)
#include <wchar.h>
#endif
#if defined(HAVE_WCTYPE_H)
#include <wctype.h>
#endif
#if defined(HAVE_LOCALE_H)
#include <locale.h>
#endif

#ifndef SIZE_MAX
#define SIZE_MAX	((size_t)~0)
#endif

#include "php.h"
#include "main/php_network.h"

#ifndef PREG_OFFSET_CAPTURE
# define PREG_OFFSET_CAPTURE                 (1<<8)
#endif

extern public void convert_libmagic_pattern(zval *pattern, char *val, size_t len, uint32_t options);

protected int
file_printf(struct magic_set *ms, const char *fmt, ...)
{
	va_list ap;
	size_t len;
	char *buf = NULL, *newstr;

	va_start(ap, fmt);
	len = vspprintf(&buf, 0, fmt, ap);
	va_end(ap);

	if (ms->o.buf != NULL) {
		len = spprintf(&newstr, 0, "%s%s", ms->o.buf, (buf ? buf : ""));
		if (buf) {
			efree(buf);
		}
		efree(ms->o.buf);
		ms->o.buf = newstr;
	} else {
		ms->o.buf = buf;
	}
	return 0;
}

/*
 * error - print best error message possible
 */
/*VARARGS*/
private void
file_error_core(struct magic_set *ms, int error, const char *f, va_list va,
    size_t lineno)
{
	char *buf = NULL;

	/* Only the first error is ok */
	if (ms->event_flags & EVENT_HAD_ERR)
		return;
	if (lineno != 0) {
		efree(ms->o.buf);
		ms->o.buf = NULL;
		file_printf(ms, "line %" SIZE_T_FORMAT "u:", lineno);
	}

	vspprintf(&buf, 0, f, va);
	va_end(va);

	if (error > 0) {
		file_printf(ms, "%s (%s)", (*buf ? buf : ""), strerror(error));
	} else if (*buf) {
		file_printf(ms, "%s", buf);
	}

	if (buf) {
		efree(buf);
	}

	ms->event_flags |= EVENT_HAD_ERR;
	ms->error = error;
}

/*VARARGS*/
protected void
file_error(struct magic_set *ms, int error, const char *f, ...)
{
	va_list va;
	va_start(va, f);
	file_error_core(ms, error, f, va, 0);
	va_end(va);
}

/*
 * Print an error with magic line number.
 */
/*VARARGS*/
protected void
file_magerror(struct magic_set *ms, const char *f, ...)
{
	va_list va;
	va_start(va, f);
	file_error_core(ms, 0, f, va, ms->line);
	va_end(va);
}

protected void
file_oomem(struct magic_set *ms, size_t len)
{
	file_error(ms, errno, "cannot allocate %" SIZE_T_FORMAT "u bytes",
	    len);
}

protected void
file_badseek(struct magic_set *ms)
{
	file_error(ms, errno, "error seeking");
}

protected void
file_badread(struct magic_set *ms)
{
	file_error(ms, errno, "error reading");
}


static int
checkdone(struct magic_set *ms, int *rv)
{
	if ((ms->flags & MAGIC_CONTINUE) == 0)
		return 1;
	if (file_printf(ms, "\n- ") == -1)
		*rv = -1;
	return 0;
}

/*ARGSUSED*/
protected int
file_buffer(struct magic_set *ms, php_stream *stream, const char *inname, const void *buf,
    size_t nb)
{
	int m = 0, rv = 0, looks_text = 0;
	const char *code = NULL;
	const char *code_mime = "binary";
	const char *type = "application/octet-stream";
	const char *def = "data";
	const char *ftype = NULL;
	struct buffer b;
	int fd = -1;
	
	buffer_init(&b, fd, buf, nb);

	if (nb == 0) {
		def = "empty";
		type = "application/x-empty";
		goto simple;
	} else if (nb == 1) {
		def = "very short file (no magic)";
		goto simple;
	}

	if ((ms->flags & MAGIC_NO_CHECK_ENCODING) == 0) {
		looks_text = file_encoding(ms, &b, NULL, 0,
		    &code, &code_mime, &ftype);
	}

#ifdef __EMX__
	if ((ms->flags & MAGIC_NO_CHECK_APPTYPE) == 0 && inname) {
		m = file_os2_apptype(ms, inname, &b);
		if ((ms->flags & MAGIC_DEBUG) != 0)
			(void)fprintf(stderr, "[try os2_apptype %d]\n", m);
		switch (m) {
		case -1:
			return -1;
		case 0:
			break;
		default:
			return 1;
		}
	}
#endif

#if PHP_FILEINFO_UNCOMPRESS
	if ((ms->flags & MAGIC_NO_CHECK_COMPRESS) == 0) {
		m = file_zmagic(ms, &b, inname);
		if ((ms->flags & MAGIC_DEBUG) != 0)
			(void)fprintf(stderr, "[try zmagic %d]\n", m);
		if (m) {
			goto done_encoding;
		}
	}
#endif
	/* Check if we have a tar file */
	if ((ms->flags & MAGIC_NO_CHECK_TAR) == 0) {
		m = file_is_tar(ms, &b);
		if ((ms->flags & MAGIC_DEBUG) != 0)
			(void)fprintf(stderr, "[try tar %d]\n", m);
		if (m) {
			if (checkdone(ms, &rv))
				goto done;
		}
	}

	/* Check if we have a CDF file */
	if ((ms->flags & MAGIC_NO_CHECK_CDF) == 0 && stream) {
#ifdef _WIN64
		php_socket_t _fd = fd;
		int _ret = php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&_fd, 0);
		fd = (int)_fd;
#else
		int _ret = php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, 0);
#endif
		if (SUCCESS == _ret) {
			m = file_trycdf(ms, &b);
			if ((ms->flags & MAGIC_DEBUG) != 0)
				(void)fprintf(stderr, "[try cdf %d]\n", m);
			if (m) {
				if (checkdone(ms, &rv))
					goto done;
			}
		}
	}

	/* try soft magic tests */
	if ((ms->flags & MAGIC_NO_CHECK_SOFT) == 0) {
		m = file_softmagic(ms, &b, NULL, NULL, BINTEST, looks_text);
		if ((ms->flags & MAGIC_DEBUG) != 0)
			(void)fprintf(stderr, "[try softmagic %d]\n", m);
		if (m) {
#ifdef BUILTIN_ELF
			if ((ms->flags & MAGIC_NO_CHECK_ELF) == 0 && m == 1 &&
			    nb > 5 && fd != -1) {
				/*
				 * We matched something in the file, so this
				 * *might* be an ELF file, and the file is at
				 * least 5 bytes long, so if it's an ELF file
				 * it has at least one byte past the ELF magic
				 * number - try extracting information from the
				 * ELF headers that cannot easily * be
				 * extracted with rules in the magic file.
				 */
				m = file_tryelf(ms, &b);
				if ((ms->flags & MAGIC_DEBUG) != 0)
					(void)fprintf(stderr, "[try elf %d]\n",
					    m);
			}
#endif
			if (checkdone(ms, &rv))
				goto done;
		}
	}

	/* try text properties */
	if ((ms->flags & MAGIC_NO_CHECK_TEXT) == 0) {

		m = file_ascmagic(ms, &b, looks_text);
		if ((ms->flags & MAGIC_DEBUG) != 0)
			(void)fprintf(stderr, "[try ascmagic %d]\n", m);
		if (m) {
			if (checkdone(ms, &rv))
				goto done;
		}
	}

simple:
	/* give up */
	m = 1;
	if (ms->flags & MAGIC_MIME) {
		if ((ms->flags & MAGIC_MIME_TYPE) &&
		    file_printf(ms, "%s", type) == -1)
			rv = -1;
	} else if (ms->flags & MAGIC_APPLE) {
		if (file_printf(ms, "UNKNUNKN") == -1)
			rv = -1;
	} else if (ms->flags & MAGIC_EXTENSION) {
		if (file_printf(ms, "???") == -1)
			rv = -1;
	} else {
		if (file_printf(ms, "%s", def) == -1)
			rv = -1;
	}
 done:
	if ((ms->flags & MAGIC_MIME_ENCODING) != 0) {
		if (ms->flags & MAGIC_MIME_TYPE)
			if (file_printf(ms, "; charset=") == -1)
				rv = -1;
		if (file_printf(ms, "%s", code_mime) == -1)
			rv = -1;
	}
#if PHP_FILEINFO_UNCOMPRESS
 done_encoding:
#endif
	buffer_fini(&b);
	if (rv)
		return rv;

	return m;
}

protected int
file_reset(struct magic_set *ms, int checkloaded)
{
	if (checkloaded && ms->mlist[0] == NULL) {
		file_error(ms, 0, "no magic files loaded");
		return -1;
	}
	if (ms->o.buf) {
		efree(ms->o.buf);
		ms->o.buf = NULL;
	}
	if (ms->o.pbuf) {
		efree(ms->o.pbuf);
		ms->o.pbuf = NULL;
	}
	ms->event_flags &= ~EVENT_HAD_ERR;
	ms->error = -1;
	return 0;
}

#define OCTALIFY(n, o)	\
	/*LINTED*/ \
	(void)(*(n)++ = '\\', \
	*(n)++ = (((uint32_t)*(o) >> 6) & 3) + '0', \
	*(n)++ = (((uint32_t)*(o) >> 3) & 7) + '0', \
	*(n)++ = (((uint32_t)*(o) >> 0) & 7) + '0', \
	(o)++)

protected const char *
file_getbuffer(struct magic_set *ms)
{
	char *pbuf, *op, *np;
	size_t psize, len;

	if (ms->event_flags & EVENT_HAD_ERR)
		return NULL;

	if (ms->flags & MAGIC_RAW)
		return ms->o.buf;

	if (ms->o.buf == NULL)
		return NULL;

	/* * 4 is for octal representation, + 1 is for NUL */
	len = strlen(ms->o.buf);
	if (len > (SIZE_MAX - 1) / 4) {
		file_oomem(ms, len);
		return NULL;
	}
	psize = len * 4 + 1;
	if ((pbuf = CAST(char *, erealloc(ms->o.pbuf, psize))) == NULL) {
		file_oomem(ms, psize);
		return NULL;
	}
	ms->o.pbuf = pbuf;

#if defined(HAVE_WCHAR_H) && defined(HAVE_MBRTOWC) && defined(HAVE_WCWIDTH)
	{
		mbstate_t state;
		wchar_t nextchar;
		int mb_conv = 1;
		size_t bytesconsumed;
		char *eop;
		(void)memset(&state, 0, sizeof(mbstate_t));

		np = ms->o.pbuf;
		op = ms->o.buf;
		eop = op + len;

		while (op < eop) {
			bytesconsumed = mbrtowc(&nextchar, op,
			    (size_t)(eop - op), &state);
			if (bytesconsumed == (size_t)(-1) ||
			    bytesconsumed == (size_t)(-2)) {
				mb_conv = 0;
				break;
			}

			if (iswprint(nextchar)) {
				(void)memcpy(np, op, bytesconsumed);
				op += bytesconsumed;
				np += bytesconsumed;
			} else {
				while (bytesconsumed-- > 0)
					OCTALIFY(np, op);
			}
		}
		*np = '\0';

		/* Parsing succeeded as a multi-byte sequence */
		if (mb_conv != 0)
			return ms->o.pbuf;
	}
#endif

	for (np = ms->o.pbuf, op = ms->o.buf; *op;) {
		if (isprint((unsigned char)*op)) {
			*np++ = *op++;
		} else {
			OCTALIFY(np, op);
		}
	}
	*np = '\0';
	return ms->o.pbuf;
}

protected int
file_check_mem(struct magic_set *ms, unsigned int level)
{
	size_t len;

	if (level >= ms->c.len) {
		len = (ms->c.len = 20 + level) * sizeof(*ms->c.li);
		ms->c.li = CAST(struct level_info *, (ms->c.li == NULL) ?
		    emalloc(len) :
		    erealloc(ms->c.li, len));
		if (ms->c.li == NULL) {
			file_oomem(ms, len);
			return -1;
		}
	}
	ms->c.li[level].got_match = 0;
#ifdef ENABLE_CONDITIONALS
	ms->c.li[level].last_match = 0;
	ms->c.li[level].last_cond = COND_NONE;
#endif /* ENABLE_CONDITIONALS */
	return 0;
}

protected size_t
file_printedlen(const struct magic_set *ms)
{
	return ms->o.buf == NULL ? 0 : strlen(ms->o.buf);
}

protected int
file_replace(struct magic_set *ms, const char *pat, const char *rep)
{
	zval patt;
	uint32_t opts = 0;
	pcre_cache_entry *pce;
	zend_string *res;
	zend_string *repl;
	size_t rep_cnt = 0;

	(void)setlocale(LC_CTYPE, "C");

	opts |= PCRE2_MULTILINE;
	convert_libmagic_pattern(&patt, (char*)pat, strlen(pat), opts);
	if ((pce = pcre_get_compiled_regex_cache(Z_STR(patt))) == NULL) {
		zval_ptr_dtor(&patt);
		rep_cnt = -1;
		goto out;
	}
	zval_ptr_dtor(&patt);

	repl = zend_string_init(rep, strlen(rep), 0);
	res = php_pcre_replace_impl(pce, NULL, ms->o.buf, strlen(ms->o.buf), repl, -1, &rep_cnt);

	zend_string_release_ex(repl, 0);
	if (NULL == res) {
		rep_cnt = -1;
		goto out;
	}

	strncpy(ms->o.buf, ZSTR_VAL(res), ZSTR_LEN(res));
	ms->o.buf[ZSTR_LEN(res)] = '\0';

	zend_string_release_ex(res, 0);

out:
	(void)setlocale(LC_CTYPE, "");
	return rep_cnt;
}

protected file_pushbuf_t *
file_push_buffer(struct magic_set *ms)
{
	file_pushbuf_t *pb;

	if (ms->event_flags & EVENT_HAD_ERR)
		return NULL;

	if ((pb = (CAST(file_pushbuf_t *, emalloc(sizeof(*pb))))) == NULL)
		return NULL;

	pb->buf = ms->o.buf;
	pb->offset = ms->offset;

	ms->o.buf = NULL;
	ms->offset = 0;

	return pb;
}

protected char *
file_pop_buffer(struct magic_set *ms, file_pushbuf_t *pb)
{
	char *rbuf;

	if (ms->event_flags & EVENT_HAD_ERR) {
		efree(pb->buf);
		efree(pb);
		return NULL;
	}

	rbuf = ms->o.buf;

	ms->o.buf = pb->buf;
	ms->offset = pb->offset;

	efree(pb);
	return rbuf;
}

/*
 * convert string to ascii printable format.
 */
protected char *
file_printable(char *buf, size_t bufsiz, const char *str)
{
	char *ptr, *eptr;
	const unsigned char *s = (const unsigned char *)str;

	for (ptr = buf, eptr = ptr + bufsiz - 1; ptr < eptr && *s; s++) {
		if (isprint(*s)) {
			*ptr++ = *s;
			continue;
		}
		if (ptr >= eptr - 3)
			break;
		*ptr++ = '\\';
		*ptr++ = ((CAST(unsigned int, *s) >> 6) & 7) + '0';
		*ptr++ = ((CAST(unsigned int, *s) >> 3) & 7) + '0';
		*ptr++ = ((CAST(unsigned int, *s) >> 0) & 7) + '0';
	}
	*ptr = '\0';
	return buf;
}
