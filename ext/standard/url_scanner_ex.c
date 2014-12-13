/* Generated by re2c 0.13.5 */
#line 1 "ext/standard/url_scanner_ex.re"
/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sascha Schumann <sascha@schumann.cx>                         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "php_ini.h"
#include "php_globals.h"
#define STATE_TAG SOME_OTHER_STATE_TAG
#include "basic_functions.h"
#include "url.h"
#undef STATE_TAG

#define url_scanner url_scanner_ex

#include "zend_smart_str.h"

static void tag_dtor(zval *zv)
{
	free(Z_PTR_P(zv));
}

static PHP_INI_MH(OnUpdateTags)
{
	url_adapt_state_ex_t *ctx;
	char *key;
	char *lasts;
	char *tmp;
	
	ctx = &BG(url_adapt_state_ex);
	
	tmp = estrndup(new_value->val, new_value->len);
	
	if (ctx->tags)
		zend_hash_destroy(ctx->tags);
	else {
		ctx->tags = malloc(sizeof(HashTable));
		if (!ctx->tags) {
			return FAILURE;
		}
	}

	zend_hash_init(ctx->tags, 0, NULL, tag_dtor, 1);
	
	for (key = php_strtok_r(tmp, ",", &lasts);
			key;
			key = php_strtok_r(NULL, ",", &lasts)) {
		char *val;

		val = strchr(key, '=');
		if (val) {
			char *q;
			size_t keylen;
			
			*val++ = '\0';
			for (q = key; *q; q++)
				*q = tolower(*q);
			keylen = q - key;
			/* key is stored withOUT NUL
			   val is stored WITH    NUL */
			zend_hash_str_add_mem(ctx->tags, key, keylen, val, strlen(val)+1);
		}
	}

	efree(tmp);

	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("url_rewriter.tags", "a=href,area=href,frame=src,form=,fieldset=", PHP_INI_ALL, OnUpdateTags, url_adapt_state_ex, php_basic_globals, basic_globals)
PHP_INI_END()

#line 107 "ext/standard/url_scanner_ex.re"


#define YYFILL(n) goto done
#define YYCTYPE unsigned char
#define YYCURSOR p
#define YYLIMIT q
#define YYMARKER r
	
static inline void append_modified_url(smart_str *url, smart_str *dest, smart_str *url_app, const char *separator)
{
	register const char *p, *q;
	const char *bash = NULL;
	const char *sep = "?";
	
	q = (p = url->s->val) + url->s->len;

scan:

#line 123 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128,   0, 128, 128, 128, 128,   0, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
	};

	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yybm[0+yych] & 128) {
		goto yy8;
	}
	if (yych <= '9') goto yy6;
	if (yych >= ';') goto yy4;
	++YYCURSOR;
#line 125 "ext/standard/url_scanner_ex.re"
	{ smart_str_append_smart_str(dest, url); return; }
#line 171 "ext/standard/url_scanner_ex.c"
yy4:
	++YYCURSOR;
#line 126 "ext/standard/url_scanner_ex.re"
	{ sep = separator; goto scan; }
#line 176 "ext/standard/url_scanner_ex.c"
yy6:
	++YYCURSOR;
#line 127 "ext/standard/url_scanner_ex.re"
	{ bash = p - 1; goto done; }
#line 181 "ext/standard/url_scanner_ex.c"
yy8:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yybm[0+yych] & 128) {
		goto yy8;
	}
#line 128 "ext/standard/url_scanner_ex.re"
	{ goto scan; }
#line 191 "ext/standard/url_scanner_ex.c"
}
#line 129 "ext/standard/url_scanner_ex.re"

done:
	
	/* Don't modify URLs of the format "#mark" */
	if (bash && bash - url->s->val == 0) {
		smart_str_append_smart_str(dest, url);
		return;
	}

	if (bash)
		smart_str_appendl(dest, url->s->val, bash - url->s->val);
	else
		smart_str_append_smart_str(dest, url);

	smart_str_appends(dest, sep);
	smart_str_append_smart_str(dest, url_app);

	if (bash)
		smart_str_appendl(dest, bash, q - bash);
}


#undef YYFILL
#undef YYCTYPE
#undef YYCURSOR
#undef YYLIMIT
#undef YYMARKER

static inline void tag_arg(url_adapt_state_ex_t *ctx, char quotes, char type)
{
	char f = 0;

	if (strncasecmp(ctx->arg.s->val, ctx->lookup_data, ctx->arg.s->len) == 0)
		f = 1;

	if (quotes)
		smart_str_appendc(&ctx->result, type);
	if (f) {
		append_modified_url(&ctx->val, &ctx->result, &ctx->url_app, PG(arg_separator).output);
	} else {
		smart_str_append_smart_str(&ctx->result, &ctx->val);
	}
	if (quotes)
		smart_str_appendc(&ctx->result, type);
}

enum {
	STATE_PLAIN = 0,
	STATE_TAG,
	STATE_NEXT_ARG,
	STATE_ARG,
	STATE_BEFORE_VAL,
	STATE_VAL
};

#define YYFILL(n) goto stop
#define YYCTYPE unsigned char
#define YYCURSOR xp
#define YYLIMIT end
#define YYMARKER q
#define STATE ctx->state

#define STD_PARA url_adapt_state_ex_t *ctx, char *start, char *YYCURSOR TSRMLS_DC
#define STD_ARGS ctx, start, xp TSRMLS_CC

#if SCANNER_DEBUG
#define scdebug(x) printf x
#else
#define scdebug(x)
#endif

static inline void passthru(STD_PARA) 
{
	scdebug(("appending %d chars, starting with %c\n", YYCURSOR-start, *start));
	smart_str_appendl(&ctx->result, start, YYCURSOR - start);
}

/*
 * This function appends a hidden input field after a <form> or
 * <fieldset>.  The latter is important for XHTML.
 */

static void handle_form(STD_PARA) 
{
	int doit = 0;

	if (ctx->form_app.s->len > 0) {
		switch (ctx->tag.s->len) {
			case sizeof("form") - 1:
				if (!strncasecmp(ctx->tag.s->val, "form", sizeof("form") - 1)) {
					doit = 1;		
				}
				if (doit && ctx->val.s && ctx->lookup_data && *ctx->lookup_data) {
					char *e, *p = (char *)zend_memnstr(ctx->val.s->val, "://", sizeof("://") - 1, ctx->val.s->val + ctx->val.s->len);
					if (p) {
						e = memchr(p, '/', (ctx->val.s->val + ctx->val.s->len) - p);
						if (!e) {
							e = ctx->val.s->val + ctx->val.s->len;
						}
						if ((e - p) && strncasecmp(p, ctx->lookup_data, (e - p))) {
							doit = 0;
						}
					}
				}
				break;

			case sizeof("fieldset") - 1:
				if (!strncasecmp(ctx->tag.s->val, "fieldset", sizeof("fieldset") - 1)) {
					doit = 1;		
				}
				break;
		}

		if (doit)
			smart_str_append_smart_str(&ctx->result, &ctx->form_app);
	}
}

/*
 *  HANDLE_TAG copies the HTML Tag and checks whether we 
 *  have that tag in our table. If we might modify it,
 *  we continue to scan the tag, otherwise we simply copy the complete
 *  HTML stuff to the result buffer.
 */

static inline void handle_tag(STD_PARA) 
{
	int ok = 0;
	unsigned int i;

	if (ctx->tag.s) {
		ctx->tag.s->len = 0;
	}
	smart_str_appendl(&ctx->tag, start, YYCURSOR - start);
	for (i = 0; i < ctx->tag.s->len; i++)
		ctx->tag.s->val[i] = tolower((int)(unsigned char)ctx->tag.s->val[i]);
    /* intentionally using str_find here, in case the hash value is set, but the string val is changed later */
	if ((ctx->lookup_data = zend_hash_str_find_ptr(ctx->tags, ctx->tag.s->val, ctx->tag.s->len)) != NULL)
		ok = 1;
	STATE = ok ? STATE_NEXT_ARG : STATE_PLAIN;
}

static inline void handle_arg(STD_PARA) 
{
	if (ctx->arg.s) {
		ctx->arg.s->len = 0;
	}
	smart_str_appendl(&ctx->arg, start, YYCURSOR - start);
}

static inline void handle_val(STD_PARA, char quotes, char type) 
{
	smart_str_setl(&ctx->val, start + quotes, YYCURSOR - start - quotes * 2);
	tag_arg(ctx, quotes, type);
}

static inline void xx_mainloop(url_adapt_state_ex_t *ctx, const char *newdata, size_t newlen)
{
	char *end, *q;
	char *xp;
	char *start;
	size_t rest;

	smart_str_appendl(&ctx->buf, newdata, newlen);
	
	YYCURSOR = ctx->buf.s->val;
	YYLIMIT = ctx->buf.s->val + ctx->buf.s->len;

	switch (STATE) {
		case STATE_PLAIN: goto state_plain;
		case STATE_TAG: goto state_tag;
		case STATE_NEXT_ARG: goto state_next_arg;
		case STATE_ARG: goto state_arg;
		case STATE_BEFORE_VAL: goto state_before_val;
		case STATE_VAL: goto state_val;
	}
	

state_plain_begin:
	STATE = STATE_PLAIN;
	
state_plain:
	start = YYCURSOR;

#line 378 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128,   0, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
	};
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yybm[0+yych] & 128) {
		goto yy15;
	}
	++YYCURSOR;
#line 313 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); STATE = STATE_TAG; goto state_tag; }
#line 423 "ext/standard/url_scanner_ex.c"
yy15:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yybm[0+yych] & 128) {
		goto yy15;
	}
#line 314 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); goto state_plain; }
#line 433 "ext/standard/url_scanner_ex.c"
}
#line 315 "ext/standard/url_scanner_ex.re"


state_tag:	
	start = YYCURSOR;

#line 441 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0, 128,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if (yych <= '@') {
		if (yych != ':') goto yy22;
	} else {
		if (yych <= 'Z') goto yy20;
		if (yych <= '`') goto yy22;
		if (yych >= '{') goto yy22;
	}
yy20:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy25;
yy21:
#line 320 "ext/standard/url_scanner_ex.re"
	{ handle_tag(STD_ARGS); /* Sets STATE */; passthru(STD_ARGS); if (STATE == STATE_PLAIN) goto state_plain; else goto state_next_arg; }
#line 494 "ext/standard/url_scanner_ex.c"
yy22:
	++YYCURSOR;
#line 321 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); goto state_plain_begin; }
#line 499 "ext/standard/url_scanner_ex.c"
yy24:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy25:
	if (yybm[0+yych] & 128) {
		goto yy24;
	}
	goto yy21;
}
#line 322 "ext/standard/url_scanner_ex.re"


state_next_arg_begin:
	STATE = STATE_NEXT_ARG;
	
state_next_arg:
	start = YYCURSOR;

#line 519 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128,   0, 128,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		128,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if (yych <= '.') {
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy36;
			if (yych <= '\v') goto yy32;
			goto yy36;
		} else {
			if (yych <= '\r') goto yy32;
			if (yych == ' ') goto yy32;
			goto yy36;
		}
	} else {
		if (yych <= '@') {
			if (yych <= '/') goto yy28;
			if (yych == '>') goto yy30;
			goto yy36;
		} else {
			if (yych <= 'Z') goto yy34;
			if (yych <= '`') goto yy36;
			if (yych <= 'z') goto yy34;
			goto yy36;
		}
	}
yy28:
	++YYCURSOR;
	if ((yych = *YYCURSOR) == '>') goto yy39;
yy29:
#line 333 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); goto state_plain_begin; }
#line 586 "ext/standard/url_scanner_ex.c"
yy30:
	++YYCURSOR;
yy31:
#line 330 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); handle_form(STD_ARGS); goto state_plain_begin; }
#line 592 "ext/standard/url_scanner_ex.c"
yy32:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy38;
yy33:
#line 331 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); goto state_next_arg; }
#line 600 "ext/standard/url_scanner_ex.c"
yy34:
	++YYCURSOR;
#line 332 "ext/standard/url_scanner_ex.re"
	{ --YYCURSOR; STATE = STATE_ARG; goto state_arg; }
#line 605 "ext/standard/url_scanner_ex.c"
yy36:
	yych = *++YYCURSOR;
	goto yy29;
yy37:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy38:
	if (yybm[0+yych] & 128) {
		goto yy37;
	}
	goto yy33;
yy39:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy31;
}
#line 334 "ext/standard/url_scanner_ex.re"


state_arg:
	start = YYCURSOR;

#line 629 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0, 128,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if (yych <= '@') goto yy44;
	if (yych <= 'Z') goto yy42;
	if (yych <= '`') goto yy44;
	if (yych >= '{') goto yy44;
yy42:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy47;
yy43:
#line 339 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); handle_arg(STD_ARGS); STATE = STATE_BEFORE_VAL; goto state_before_val; }
#line 679 "ext/standard/url_scanner_ex.c"
yy44:
	++YYCURSOR;
#line 340 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); STATE = STATE_NEXT_ARG; goto state_next_arg; }
#line 684 "ext/standard/url_scanner_ex.c"
yy46:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy47:
	if (yybm[0+yych] & 128) {
		goto yy46;
	}
	goto yy43;
}
#line 341 "ext/standard/url_scanner_ex.re"


state_before_val:
	start = YYCURSOR;

#line 701 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		128,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if (yych == ' ') goto yy50;
	if (yych == '=') goto yy52;
	goto yy54;
yy50:
	yych = *(YYMARKER = ++YYCURSOR);
	if (yych == ' ') goto yy57;
	if (yych == '=') goto yy55;
yy51:
#line 347 "ext/standard/url_scanner_ex.re"
	{ --YYCURSOR; goto state_next_arg_begin; }
#line 750 "ext/standard/url_scanner_ex.c"
yy52:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy56;
yy53:
#line 346 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); STATE = STATE_VAL; goto state_val; }
#line 758 "ext/standard/url_scanner_ex.c"
yy54:
	yych = *++YYCURSOR;
	goto yy51;
yy55:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy56:
	if (yybm[0+yych] & 128) {
		goto yy55;
	}
	goto yy53;
yy57:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yych == ' ') goto yy57;
	if (yych == '=') goto yy55;
	YYCURSOR = YYMARKER;
	goto yy51;
}
#line 348 "ext/standard/url_scanner_ex.re"



state_val:
	start = YYCURSOR;

#line 787 "ext/standard/url_scanner_ex.c"
{
	YYCTYPE yych;
	static const unsigned char yybm[] = {
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 192, 192, 224, 224, 192, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		192, 224,  64, 224, 224, 224, 224, 128, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224,   0, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 224, 224, 224, 224, 224, 224, 
	};
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if (yych <= ' ') {
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy65;
			if (yych <= '\n') goto yy67;
			goto yy65;
		} else {
			if (yych <= '\r') goto yy67;
			if (yych <= 0x1F) goto yy65;
			goto yy67;
		}
	} else {
		if (yych <= '&') {
			if (yych != '"') goto yy65;
		} else {
			if (yych <= '\'') goto yy64;
			if (yych == '>') goto yy67;
			goto yy65;
		}
	}
	yych = *(YYMARKER = ++YYCURSOR);
	if (yych != '>') goto yy76;
yy63:
#line 357 "ext/standard/url_scanner_ex.re"
	{ passthru(STD_ARGS); goto state_next_arg_begin; }
#line 850 "ext/standard/url_scanner_ex.c"
yy64:
	yych = *(YYMARKER = ++YYCURSOR);
	if (yych == '>') goto yy63;
	goto yy71;
yy65:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy69;
yy66:
#line 356 "ext/standard/url_scanner_ex.re"
	{ handle_val(STD_ARGS, 0, ' '); goto state_next_arg_begin; }
#line 862 "ext/standard/url_scanner_ex.c"
yy67:
	yych = *++YYCURSOR;
	goto yy63;
yy68:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy69:
	if (yybm[0+yych] & 32) {
		goto yy68;
	}
	goto yy66;
yy70:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy71:
	if (yybm[0+yych] & 64) {
		goto yy70;
	}
	if (yych <= '=') goto yy73;
yy72:
	YYCURSOR = YYMARKER;
	goto yy63;
yy73:
	++YYCURSOR;
#line 355 "ext/standard/url_scanner_ex.re"
	{ handle_val(STD_ARGS, 1, '\''); goto state_next_arg_begin; }
#line 891 "ext/standard/url_scanner_ex.c"
yy75:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy76:
	if (yybm[0+yych] & 128) {
		goto yy75;
	}
	if (yych >= '>') goto yy72;
	++YYCURSOR;
#line 354 "ext/standard/url_scanner_ex.re"
	{ handle_val(STD_ARGS, 1, '"'); goto state_next_arg_begin; }
#line 904 "ext/standard/url_scanner_ex.c"
}
#line 358 "ext/standard/url_scanner_ex.re"


stop:
	if (YYLIMIT < start) {
		/* XXX: Crash avoidance. Need to work with reporter to figure out what goes wrong */	
		rest = 0;
	} else {
		rest = YYLIMIT - start;
		scdebug(("stopped in state %d at pos %d (%d:%c) %d\n", STATE, YYCURSOR - ctx->buf.c, *YYCURSOR, *YYCURSOR, rest));
	}
	
	if (rest) memmove(ctx->buf.s->val, start, rest);
	ctx->buf.s->len = rest;
}

char *php_url_scanner_adapt_single_url(const char *url, size_t urllen, const char *name, const char *value, size_t *newlen)
{
	char *result;
	smart_str surl = {0};
	smart_str buf = {0};
	smart_str url_app = {0};

	smart_str_setl(&surl, url, urllen);

	smart_str_appends(&url_app, name);
	smart_str_appendc(&url_app, '=');
	smart_str_appends(&url_app, value);

	append_modified_url(&surl, &buf, &url_app, PG(arg_separator).output);

	smart_str_0(&buf);
	if (newlen) *newlen = buf.s->len;
	result = estrndup(buf.s->val, buf.s->len);

	smart_str_free(&url_app);
	smart_str_free(&buf);

	return result;
}


static char *url_adapt_ext(const char *src, size_t srclen, size_t *newlen, zend_bool do_flush)
{
	url_adapt_state_ex_t *ctx;
	char *retval;

	ctx = &BG(url_adapt_state_ex);

	xx_mainloop(ctx, src, srclen);

	if (!ctx->result.s) {
		smart_str_appendl(&ctx->result, "", 0);
		*newlen = 0;
	} else {
		*newlen = ctx->result.s->len;
	}
	smart_str_0(&ctx->result);
	if (do_flush) {
		smart_str_append(&ctx->result, ctx->buf.s);
		*newlen += ctx->buf.s->len;
		smart_str_free(&ctx->buf);
		smart_str_free(&ctx->val);
	}
	retval = estrndup(ctx->result.s->val, ctx->result.s->len);
	smart_str_free(&ctx->result);
	return retval;
}

static int php_url_scanner_ex_activate(void)
{
	url_adapt_state_ex_t *ctx;
	
	ctx = &BG(url_adapt_state_ex);

	memset(ctx, 0, ((size_t) &((url_adapt_state_ex_t *)0)->tags));

	return SUCCESS;
}

static int php_url_scanner_ex_deactivate(void)
{
	url_adapt_state_ex_t *ctx;
	
	ctx = &BG(url_adapt_state_ex);

	smart_str_free(&ctx->result);
	smart_str_free(&ctx->buf);
	smart_str_free(&ctx->tag);
	smart_str_free(&ctx->arg);

	return SUCCESS;
}

static void php_url_scanner_output_handler(char *output, size_t output_len, char **handled_output, size_t *handled_output_len, int mode)
{
	size_t len;

	if (BG(url_adapt_state_ex).url_app.s->len != 0) {
		*handled_output = url_adapt_ext(output, output_len, &len, (zend_bool) (mode & (PHP_OUTPUT_HANDLER_END | PHP_OUTPUT_HANDLER_CONT | PHP_OUTPUT_HANDLER_FLUSH | PHP_OUTPUT_HANDLER_FINAL) ? 1 : 0));
		if (sizeof(uint) < sizeof(size_t)) {
			if (len > UINT_MAX)
				len = UINT_MAX;
		}
		*handled_output_len = len;
	} else if (BG(url_adapt_state_ex).url_app.s->len == 0) {
		url_adapt_state_ex_t *ctx = &BG(url_adapt_state_ex);
		if (ctx->buf.s && ctx->buf.s->len) {
			smart_str_append(&ctx->result, ctx->buf.s);
			smart_str_appendl(&ctx->result, output, output_len);

			*handled_output = estrndup(ctx->result.s->val, ctx->result.s->len);
			*handled_output_len = ctx->buf.s->len + output_len;

			smart_str_free(&ctx->buf);
			smart_str_free(&ctx->result);
		} else {
			*handled_output = estrndup(output, *handled_output_len = output_len);
		}
	} else {
		*handled_output = NULL;
	}
}

PHPAPI int php_url_scanner_add_var(char *name, size_t name_len, char *value, size_t value_len, int urlencode)
{
	smart_str val = {0};
	zend_string *encoded;
	
	if (!BG(url_adapt_state_ex).active) {
		php_url_scanner_ex_activate();
		php_output_start_internal(ZEND_STRL("URL-Rewriter"), php_url_scanner_output_handler, 0, PHP_OUTPUT_HANDLER_STDFLAGS);
		BG(url_adapt_state_ex).active = 1;
	}


	if (BG(url_adapt_state_ex).url_app.s && BG(url_adapt_state_ex).url_app.s->len != 0) {
		smart_str_appends(&BG(url_adapt_state_ex).url_app, PG(arg_separator).output);
	}

	if (urlencode) {
		encoded = php_url_encode(value, value_len);
		smart_str_setl(&val, encoded->val, encoded->len);
	} else {
		smart_str_setl(&val, value, value_len);
	}
	
	smart_str_appendl(&BG(url_adapt_state_ex).url_app, name, name_len);
	smart_str_appendc(&BG(url_adapt_state_ex).url_app, '=');
	smart_str_append_smart_str(&BG(url_adapt_state_ex).url_app, &val);

	smart_str_appends(&BG(url_adapt_state_ex).form_app, "<input type=\"hidden\" name=\""); 
	smart_str_appendl(&BG(url_adapt_state_ex).form_app, name, name_len);
	smart_str_appends(&BG(url_adapt_state_ex).form_app, "\" value=\"");
	smart_str_append_smart_str(&BG(url_adapt_state_ex).form_app, &val);
	smart_str_appends(&BG(url_adapt_state_ex).form_app, "\" />");

	if (urlencode) {
		zend_string_free(encoded);
	}
	smart_str_free(&val);

	return SUCCESS;
}

PHPAPI int php_url_scanner_reset_vars(void)
{
	if (BG(url_adapt_state_ex).form_app.s) {
		BG(url_adapt_state_ex).form_app.s->len = 0;
	}
	if (BG(url_adapt_state_ex).url_app.s) {
		BG(url_adapt_state_ex).url_app.s->len = 0;
	}

	return SUCCESS;
}

PHP_MINIT_FUNCTION(url_scanner)
{
	BG(url_adapt_state_ex).tags = NULL;

	BG(url_adapt_state_ex).form_app.s = BG(url_adapt_state_ex).url_app.s = NULL;

	REGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(url_scanner)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(url_scanner)
{
	BG(url_adapt_state_ex).active = 0;
	
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(url_scanner)
{
	if (BG(url_adapt_state_ex).active) {
		php_url_scanner_ex_deactivate();
		BG(url_adapt_state_ex).active = 0;
	}

	smart_str_free(&BG(url_adapt_state_ex).form_app);
	smart_str_free(&BG(url_adapt_state_ex).url_app);

	return SUCCESS;
}
