/*
  +----------------------------------------------------------------------+
  | PHP version 4.0                                                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Sascha Schumann <sascha@schumann.cx>                        |
  +----------------------------------------------------------------------+
*/

#include "php.h"

#ifdef TRANS_SID

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "php_globals.h"
#define STATE_TAG SOME_OTHER_STATE_TAG
#include "basic_functions.h"
#undef STATE_TAG

#define url_adapt_ext url_adapt_ext_ex
#define url_scanner url_scanner_ex

static inline void smart_str_append(smart_str *dest, smart_str *src)
{
	size_t newlen;

	if (!dest->c)
		dest->len = dest->a = 0;
	
	newlen = dest->len + src->len;
	if (newlen >= dest->a) {
		dest->c = realloc(dest->c, newlen + 101);
		dest->a = newlen + 100;
	}
	memcpy(dest->c + dest->len, src->c, src->len);
	dest->c[dest->len = newlen] = '\0';
}

static inline void smart_str_free(smart_str *s)
{
	if (s->c) {
		free(s->c);
		s->c = NULL;
	}
	s->a = s->len = 0;
}

static inline void smart_str_copyl(smart_str *dest, const char *src, size_t len)
{
	dest->c = realloc(dest->c, len + 1);
	memcpy(dest->c, src, len);
	dest->c[len] = '\0';
	dest->a = dest->len = len;
}

static inline void smart_str_appendl(smart_str *dest, const char *src, size_t len)
{
	smart_str s;

	s.c = (char *) src;
	s.len = len;

	smart_str_append(dest, &s);
}

static inline void smart_str_set(smart_str *dest, smart_str *src)
{
	dest->len = src->len;
	dest->a = src->a;
	dest->c = src->c;
}

static inline void smart_str_setl(smart_str *dest, const char *src, size_t len)
{
	dest->len = len;
	dest->a = len + 1;
	dest->c = (char *) src;
}

#define smart_str_appends(dest, src) smart_str_appendl(dest, src, sizeof(src)-1)

#if 0
static inline void smart_str_copys(smart_str *dest, const char *src)
{
	smart_str_copyl(dest, src, strlen(src));
}
#endif

static inline void smart_str_sets(smart_str *dest, const char *src)
{
	smart_str_setl(dest, src, strlen(src));
}

static inline void attach_url(smart_str *url, smart_str *name, smart_str *val, const char *separator)
{
	register const char *p, *q;
	const char *bash = NULL;
	const char *sep = "?";
	
	q = url->c + url->len;
	
	for (p = url->c; p < q; p++) {
		switch(*p) {
			case ':':
				return;
			case '?':
				sep = separator;
				break;
			case '#':
				bash = p;
				break;
		}
	}

	if (bash) {
		smart_str new = {0};
		
		smart_str_copyl(&new, url->c, bash - url->c);
		smart_str_appendl(&new, sep, 1);
		smart_str_append(&new, name);
		smart_str_appendl(&new, "=", 1);
		smart_str_append(&new, val);
		smart_str_appendl(&new, bash, q - bash);

		smart_str_free(url);
		smart_str_set(url, &new);
	} else {
		smart_str_appendl(url, sep, 1);
		smart_str_append(url, name);
		smart_str_appendl(url, "=", 1);
		smart_str_append(url, val);
	}
}

struct php_tag_arg {
	char *tag;
	int taglen;
	char *arg;
	int arglen;
};

#define TAG_ARG_ENTRY(a,b) {#a,sizeof(#a)-1,#b,sizeof(#b)-1},

static struct php_tag_arg check_tag_arg[] = {
	TAG_ARG_ENTRY(a, href)
	TAG_ARG_ENTRY(area, href)
	TAG_ARG_ENTRY(frame, src)
	TAG_ARG_ENTRY(img, src)
	TAG_ARG_ENTRY(input, src)
	TAG_ARG_ENTRY(form, fake_entry_for_passing_on_form_tag)
	{0}
};

static inline void tag_arg(url_adapt_state_ex_t *ctx PLS_DC)
{
	char f = 0;
	int i;

	for (i = 0; check_tag_arg[i].tag; i++) {
		if (check_tag_arg[i].arglen == ctx->arg.len
				&& check_tag_arg[i].taglen == ctx->tag.len
				&& strncasecmp(ctx->tag.c, check_tag_arg[i].tag, ctx->tag.len) == 0
				&& strncasecmp(ctx->arg.c, check_tag_arg[i].arg, ctx->arg.len) == 0) {
			f = 1;
			break;
		}
	}

	smart_str_appends(&ctx->result, "\"");
	if (f) {
		attach_url(&ctx->val, &ctx->q_name, &ctx->q_value, PG(arg_separator));
	}
	smart_str_append(&ctx->result, &ctx->val);
	smart_str_appends(&ctx->result, "\"");
}

enum {
	STATE_PLAIN,
	STATE_TAG,
	STATE_NEXT_ARG,
	STATE_ARG,
	STATE_BEFORE_VAL,
	STATE_VAL
};

#define YYFILL(n) goto stop
#define YYCTYPE char
#define YYCURSOR xp
#define YYLIMIT end
#define YYMARKER q
#define STATE ctx->state

#define PASSTHRU() {\
	smart_str_appendl(&ctx->result, start, YYCURSOR - start); \
}

#define HANDLE_FORM() {\
	if (ctx->tag.len == 4 && strncasecmp(ctx->tag.c, "form", 4) == 0) {\
		smart_str_appends(&ctx->result, "<INPUT TYPE=HIDDEN NAME=\""); \
		smart_str_append(&ctx->result, &ctx->q_name); \
		smart_str_appends(&ctx->result, "\" VALUE=\""); \
		smart_str_append(&ctx->result, &ctx->q_value); \
		smart_str_appends(&ctx->result, "\">"); \
	} \
}

/*
 *  HANDLE_TAG copies the HTML Tag and checks whether we 
 *  have that tag in our table. If we might modify it,
 *  we continue to scan the tag, otherwise we simply copy the complete
 *  HTML stuff to the result buffer.
 */

#define HANDLE_TAG() {\
	int __ok = 0; \
	int i; \
	smart_str_setl(&ctx->tag, start, YYCURSOR - start); \
	for (i = 0; check_tag_arg[i].tag; i++) { \
		if (ctx->tag.len == check_tag_arg[i].taglen \
				&& strncasecmp(ctx->tag.c, check_tag_arg[i].tag, ctx->tag.len) == 0) { \
			__ok = 1; \
			break; \
		} \
	} \
	STATE = __ok ? STATE_NEXT_ARG : STATE_PLAIN; \
}

#define HANDLE_ARG() {\
	smart_str_setl(&ctx->arg, start, YYCURSOR - start); \
}
#define HANDLE_VAL(quotes) {\
	smart_str_copyl(&ctx->val, start + quotes, YYCURSOR - start - quotes * 2); \
	tag_arg(ctx PLS_CC); \
}

/*
 * Since arg/tag are read-only during the mainloop, we do not need
 * to copy them. We need those variables across multiple calls 
 * to url_adapt() though, but they point to a private buffer. So we
 * copy them before leaving the mainloop() and restore them at
 * the beginning.
 */

#define MOVE_TO_CTX(X) \
	if (ctx->X.c) \
		smart_str_copyl(&ctx->c_##X, ctx->X.c, ctx->X.len); \
	else \
		smart_str_free(&ctx->c_##X)

#define FETCH_FROM_CTX(X) \
	smart_str_setl(&ctx->X, ctx->c_##X.c, ctx->c_##X.len)
	
static inline void mainloop(url_adapt_state_ex_t *ctx, const char *newdata, size_t newlen)
{
	char *end, *q;
	char *xp;
	char *start;
	int rest;
	PLS_FETCH();

	FETCH_FROM_CTX(arg);
	FETCH_FROM_CTX(tag);

	smart_str_appendl(&ctx->buf, newdata, newlen);
	
	YYCURSOR = ctx->buf.c;
	YYLIMIT = ctx->buf.c + ctx->buf.len;

/*!re2c
any = [\000-\377];
alpha = [a-zA-Z];
*/
	
	while(1) {
		start = YYCURSOR;
#ifdef SCANNER_DEBUG
		printf("state %d at %s\n", STATE, YYCURSOR);
#endif
	switch(STATE) {
		
		case STATE_PLAIN:
/*!re2c
  [<]			{ PASSTHRU(); STATE = STATE_TAG; continue; }
  (any\[<])		{ PASSTHRU(); continue; }
*/
			break;
			
		case STATE_TAG:
/*!re2c
  alpha+	{ HANDLE_TAG() /* Sets STATE */; PASSTHRU(); continue; }
  any		{ PASSTHRU(); continue; }
*/
  			break;
			
		case STATE_NEXT_ARG:
/*!re2c
  ">"		{ PASSTHRU(); HANDLE_FORM(); STATE = STATE_PLAIN; continue; }
  [ \n]		{ PASSTHRU(); continue; }
  alpha		{ YYCURSOR--; STATE = STATE_ARG; continue; }
  any		{ PASSTHRU(); continue; }
*/
 	 		break;

		case STATE_ARG:
/*!re2c
  alpha+	{ PASSTHRU(); HANDLE_ARG(); STATE = STATE_BEFORE_VAL; continue; }
  any		{ PASSTHRU(); STATE = STATE_NEXT_ARG; continue; }
*/

		case STATE_BEFORE_VAL:
/*!re2c
  [ ]* "=" [ ]*		{ PASSTHRU(); STATE = STATE_VAL; continue; }
  any				{ YYCURSOR--; STATE = STATE_NEXT_ARG; continue; }
*/
			break;

		case STATE_VAL:
/*!re2c
  ["] (any\[">])* ["]	{ HANDLE_VAL(1); STATE = STATE_NEXT_ARG; continue; }
  (any\[ \n>"])+		{ HANDLE_VAL(0); STATE = STATE_NEXT_ARG; continue; }
  any					{ PASSTHRU(); STATE = STATE_NEXT_ARG; continue; }
*/
			break;
	}
	}

stop:
#ifdef SCANNER_DEBUG
	printf("stopped in state %d at pos %d (%d:%c)\n", STATE, YYCURSOR - ctx->buf.c, *YYCURSOR, *YYCURSOR);
#endif

	MOVE_TO_CTX(tag);
	MOVE_TO_CTX(arg);

	rest = YYLIMIT - start;
		
	memmove(ctx->buf.c, start, rest);
	ctx->buf.c[rest] = '\0';
	ctx->buf.len = rest;
}


char *url_adapt_ext(const char *src, size_t srclen, const char *name, const char *value, size_t *newlen)
{
	char *ret;
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();

	ctx = &BG(url_adapt_state_ex);

	smart_str_sets(&ctx->q_name, name);
	smart_str_sets(&ctx->q_value, value);
	mainloop(ctx, src, srclen);

	*newlen = ctx->result.len;

	if (ctx->result.len == 0) {
		return strdup("");
	}
	ret = ctx->result.c;
	ctx->result.c = NULL;
	ctx->result.len = ctx->result.a = 0;
	return ret;
}

PHP_RINIT_FUNCTION(url_scanner)
{
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);

	memset(ctx, 0, sizeof(*ctx));

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(url_scanner)
{
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);

	smart_str_free(&ctx->result);
	smart_str_free(&ctx->buf);
	smart_str_free(&ctx->c_tag);
	smart_str_free(&ctx->c_arg);
	smart_str_free(&ctx->val);

	return SUCCESS;
}

#endif
