/* Generated by re2c 0.13.5 */
#line 1 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
/*
 * phpdbg_lexer.l
 */

#include "phpdbg.h"
#include "phpdbg_cmd.h"

#include "phpdbg_parser.h"

#define LEX(v) (PHPDBG_G(lexer).v)

#define YYCTYPE unsigned char
#define YYSETCONDITION(x) LEX(state) = x;
#define YYGETCONDITION() LEX(state)
#define YYCURSOR LEX(cursor)
#define YYMARKER LEX(marker)
#define yyleng LEX(len)
#define yytext ((char*) LEX(text))
#undef YYDEBUG
#define YYDEBUG(a, b)
#define YYFILL(n)

#define NORMAL 0
#define PRE_RAW 1
#define RAW 2
#define INITIAL 3

ZEND_EXTERN_MODULE_GLOBALS(phpdbg);

void phpdbg_init_lexer (phpdbg_param_t *stack, char *input TSRMLS_DC) {
	PHPDBG_G(parser_stack) = stack;

	YYSETCONDITION(INITIAL);

	LEX(text) = YYCURSOR = (unsigned char *) input;
	LEX(len) = strlen(input);
}

int phpdbg_lex (phpdbg_param_t* yylval) {
	TSRMLS_FETCH(); /* Slow, but this is not a major problem here. TODO: Use TSRMLS_DC */

restart:
	LEX(text) = YYCURSOR;


#line 49 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
{
	YYCTYPE yych;
	unsigned int yyaccept = 0;
	if (YYGETCONDITION() < 2) {
		if (YYGETCONDITION() < 1) {
			goto yyc_NORMAL;
		} else {
			goto yyc_PRE_RAW;
		}
	} else {
		if (YYGETCONDITION() < 3) {
			goto yyc_RAW;
		} else {
			goto yyc_INITIAL;
		}
	}
/* *********************************** */
yyc_INITIAL:
	{
		static const unsigned char yybm[] = {
			  0,   0,   0,   0,   0,   0,   0,   0, 
			  0, 192,  96,   0,   0, 192,   0,   0, 
			  0,   0,   0,   0,   0,   0,   0,   0, 
			  0,   0,   0,   0,   0,   0,   0,   0, 
			192,   0,   0,   0,   0,   0,   0,   0, 
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

		YYDEBUG(0, *YYCURSOR);
		YYFILL(4);
		yych = *YYCURSOR;
		if (yybm[0+yych] & 32) {
			goto yy4;
		}
		if (yych <= 'E') {
			if (yych <= '\f') {
				if (yych <= 0x00) goto yy7;
				if (yych != '\t') goto yy12;
			} else {
				if (yych <= 0x1F) {
					if (yych >= 0x0E) goto yy12;
				} else {
					if (yych <= ' ') goto yy2;
					if (yych <= 'D') goto yy12;
					goto yy8;
				}
			}
		} else {
			if (yych <= 'd') {
				if (yych <= 'Q') goto yy12;
				if (yych <= 'R') goto yy11;
				if (yych <= 'S') goto yy9;
				goto yy12;
			} else {
				if (yych <= 'q') {
					if (yych <= 'e') goto yy8;
					goto yy12;
				} else {
					if (yych <= 'r') goto yy10;
					if (yych <= 's') goto yy9;
					goto yy12;
				}
			}
		}
yy2:
		YYDEBUG(2, *YYCURSOR);
		++YYCURSOR;
		if ((yych = *YYCURSOR) <= '\f') {
			if (yych <= 0x00) goto yy29;
			if (yych <= 0x08) goto yy3;
			if (yych <= '\n') goto yy29;
		} else {
			if (yych <= '\r') goto yy29;
			if (yych == ' ') goto yy29;
		}
yy3:
		YYDEBUG(3, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 176 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(NORMAL);

	YYCURSOR = LEX(text);
	goto restart;
}
#line 161 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy4:
		YYDEBUG(4, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(5, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy28;
		}
		if (yych <= 0x00) goto yy27;
		if (yych == '\n') goto yy4;
yy6:
		YYDEBUG(6, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 69 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return 0;
}
#line 180 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy7:
		YYDEBUG(7, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy3;
yy8:
		YYDEBUG(8, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'V') goto yy23;
		if (yych == 'v') goto yy23;
		goto yy3;
yy9:
		YYDEBUG(9, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'H') goto yy19;
		if (yych == 'h') goto yy19;
		goto yy3;
yy10:
		YYDEBUG(10, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy16;
		}
		if (yych == 'U') goto yy13;
		if (yych == 'u') goto yy13;
		goto yy3;
yy11:
		YYDEBUG(11, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'U') goto yy13;
		if (yych == 'u') goto yy13;
		goto yy3;
yy12:
		YYDEBUG(12, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy3;
yy13:
		YYDEBUG(13, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych == 'N') goto yy15;
		if (yych == 'n') goto yy15;
yy14:
		YYDEBUG(14, *YYCURSOR);
		YYCURSOR = YYMARKER;
		goto yy3;
yy15:
		YYDEBUG(15, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yybm[0+yych] & 64) {
			goto yy16;
		}
		goto yy14;
yy16:
		YYDEBUG(16, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(17, *YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy16;
		}
		YYDEBUG(18, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 163 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(PRE_RAW);
	phpdbg_init_param(yylval, EMPTY_PARAM);
	return T_RUN;
}
#line 253 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy19:
		YYDEBUG(19, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy14;
			if (yych >= '\v') goto yy14;
		} else {
			if (yych <= '\r') goto yy20;
			if (yych != ' ') goto yy14;
		}
yy20:
		YYDEBUG(20, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(21, *YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy22;
			if (yych <= '\n') goto yy20;
		} else {
			if (yych <= '\r') goto yy20;
			if (yych == ' ') goto yy20;
		}
yy22:
		YYDEBUG(22, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 158 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(PRE_RAW);
	phpdbg_init_param(yylval, EMPTY_PARAM);
	return T_SHELL;
}
#line 286 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy23:
		YYDEBUG(23, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy14;
			if (yych >= '\v') goto yy14;
		} else {
			if (yych <= '\r') goto yy24;
			if (yych != ' ') goto yy14;
		}
yy24:
		YYDEBUG(24, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(25, *YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy26;
			if (yych <= '\n') goto yy24;
		} else {
			if (yych <= '\r') goto yy24;
			if (yych == ' ') goto yy24;
		}
yy26:
		YYDEBUG(26, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 153 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(PRE_RAW);
	phpdbg_init_param(yylval, EMPTY_PARAM);
	return T_EVAL;
}
#line 319 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy27:
		YYDEBUG(27, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy6;
yy28:
		YYDEBUG(28, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
yy29:
		YYDEBUG(29, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy28;
		}
		if (yych <= 0x00) goto yy27;
		if (yych == '\n') goto yy4;
		YYDEBUG(30, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 147 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	/* ignore whitespace */

	goto restart;
}
#line 344 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
	}
/* *********************************** */
yyc_NORMAL:
	{
		static const unsigned char yybm[] = {
			  0,   8,   8,   8,   8,   8,   8,   8, 
			  8,  66,  68,   8,   8,  66,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			 66,   8,   8,   0,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,  24,   8, 
			152, 152, 152, 152, 152, 152, 152, 152, 
			152, 152,   0,   8,   8,   8,   8,   8, 
			  8, 168, 168, 168, 168, 168, 168,  40, 
			 40,  40,  40,  40,  40,  40,  40,  40, 
			 40,  40,  40,  40,  40,  40,  40,  40, 
			 40,  40,  40,   8,   8,   8,   8,   8, 
			  8, 168, 168, 168, 168, 168, 168,  40, 
			 40,  40,  40,  40,  40,  40,  40,  40, 
			 40,  40,  40,  40,  40,  40,  40,  40, 
			 40,  40,  40,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
			  8,   8,   8,   8,   8,   8,   8,   8, 
		};
		YYDEBUG(31, *YYCURSOR);
		YYFILL(11);
		yych = *YYCURSOR;
		if (yybm[0+yych] & 2) {
			goto yy33;
		}
		if (yych <= 'N') {
			if (yych <= '0') {
				if (yych <= '#') {
					if (yych <= '\t') {
						if (yych <= 0x00) goto yy39;
						goto yy43;
					} else {
						if (yych <= '\n') goto yy36;
						if (yych <= '"') goto yy43;
						goto yy58;
					}
				} else {
					if (yych <= '-') {
						if (yych <= ',') goto yy43;
						goto yy40;
					} else {
						if (yych <= '.') goto yy45;
						if (yych <= '/') goto yy43;
						goto yy48;
					}
				}
			} else {
				if (yych <= 'E') {
					if (yych <= ':') {
						if (yych <= '9') goto yy45;
						goto yy60;
					} else {
						if (yych <= 'C') goto yy43;
						if (yych <= 'D') goto yy49;
						goto yy50;
					}
				} else {
					if (yych <= 'H') {
						if (yych <= 'F') goto yy51;
						goto yy43;
					} else {
						if (yych <= 'I') goto yy42;
						if (yych <= 'M') goto yy43;
						goto yy52;
					}
				}
			}
		} else {
			if (yych <= 'f') {
				if (yych <= 'Y') {
					if (yych <= 'S') {
						if (yych <= 'O') goto yy53;
						goto yy43;
					} else {
						if (yych <= 'T') goto yy54;
						if (yych <= 'X') goto yy43;
						goto yy55;
					}
				} else {
					if (yych <= 'c') {
						if (yych <= 'Z') goto yy56;
						goto yy43;
					} else {
						if (yych <= 'd') goto yy49;
						if (yych <= 'e') goto yy50;
						goto yy51;
					}
				}
			} else {
				if (yych <= 'o') {
					if (yych <= 'i') {
						if (yych <= 'h') goto yy43;
						goto yy42;
					} else {
						if (yych <= 'm') goto yy43;
						if (yych <= 'n') goto yy52;
						goto yy53;
					}
				} else {
					if (yych <= 'x') {
						if (yych == 't') goto yy54;
						goto yy43;
					} else {
						if (yych <= 'y') goto yy55;
						if (yych <= 'z') goto yy57;
						goto yy43;
					}
				}
			}
		}
yy33:
		YYDEBUG(33, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(34, *YYCURSOR);
		if (yybm[0+yych] & 2) {
			goto yy33;
		}
		if (yych <= 0x00) goto yy39;
		if (yych == '\n') goto yy36;
		YYDEBUG(35, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 147 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	/* ignore whitespace */

	goto restart;
}
#line 493 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy36:
		YYDEBUG(36, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(37, *YYCURSOR);
		if (yybm[0+yych] & 2) {
			goto yy33;
		}
		if (yych <= 0x00) goto yy39;
		if (yych == '\n') goto yy36;
yy38:
		YYDEBUG(38, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 69 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return 0;
}
#line 512 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy39:
		YYDEBUG(39, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy38;
yy40:
		YYDEBUG(40, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 16) {
			goto yy45;
		}
		if (yych == 'r') goto yy113;
		goto yy44;
yy41:
		YYDEBUG(41, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 133 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, STR_PARAM);
	yylval->str = zend_strndup(yytext, yyleng);
	yylval->len = yyleng;
	return T_ID; 
}
#line 536 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy42:
		YYDEBUG(42, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'F') goto yy109;
		if (yych == 'f') goto yy109;
		goto yy44;
yy43:
		YYDEBUG(43, *YYCURSOR);
		yyaccept = 0;
		YYMARKER = ++YYCURSOR;
		YYFILL(3);
		yych = *YYCURSOR;
yy44:
		YYDEBUG(44, *YYCURSOR);
		if (yybm[0+yych] & 8) {
			goto yy43;
		}
		if (yych <= '9') goto yy41;
		goto yy65;
yy45:
		YYDEBUG(45, *YYCURSOR);
		yyaccept = 1;
		YYMARKER = ++YYCURSOR;
		YYFILL(3);
		yych = *YYCURSOR;
		YYDEBUG(46, *YYCURSOR);
		if (yybm[0+yych] & 16) {
			goto yy45;
		}
		if (yych <= 0x1F) {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy47;
				if (yych <= 0x08) goto yy43;
			} else {
				if (yych != '\r') goto yy43;
			}
		} else {
			if (yych <= '#') {
				if (yych <= ' ') goto yy47;
				if (yych <= '"') goto yy43;
			} else {
				if (yych == ':') goto yy65;
				goto yy43;
			}
		}
yy47:
		YYDEBUG(47, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 114 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, NUMERIC_PARAM);
	yylval->num = atoi(yytext);
	return T_DIGITS;
}
#line 592 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy48:
		YYDEBUG(48, *YYCURSOR);
		yyaccept = 1;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 16) {
			goto yy45;
		}
		if (yych <= 0x1F) {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy47;
				if (yych <= 0x08) goto yy44;
				goto yy47;
			} else {
				if (yych == '\r') goto yy47;
				goto yy44;
			}
		} else {
			if (yych <= '#') {
				if (yych <= ' ') goto yy47;
				if (yych <= '"') goto yy44;
				goto yy47;
			} else {
				if (yych == 'x') goto yy105;
				goto yy44;
			}
		}
yy49:
		YYDEBUG(49, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'I') goto yy99;
		if (yych == 'i') goto yy99;
		goto yy44;
yy50:
		YYDEBUG(50, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'N') goto yy94;
		if (yych == 'n') goto yy94;
		goto yy44;
yy51:
		YYDEBUG(51, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'A') goto yy91;
		if (yych == 'a') goto yy91;
		goto yy44;
yy52:
		YYDEBUG(52, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'O') goto yy87;
		if (yych == 'o') goto yy87;
		goto yy44;
yy53:
		YYDEBUG(53, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych <= 'N') {
			if (yych == 'F') goto yy86;
			if (yych <= 'M') goto yy44;
			goto yy80;
		} else {
			if (yych <= 'f') {
				if (yych <= 'e') goto yy44;
				goto yy86;
			} else {
				if (yych == 'n') goto yy80;
				goto yy44;
			}
		}
yy54:
		YYDEBUG(54, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'R') goto yy84;
		if (yych == 'r') goto yy84;
		goto yy44;
yy55:
		YYDEBUG(55, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy79;
		if (yych == 'e') goto yy79;
		goto yy44;
yy56:
		YYDEBUG(56, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy76;
		goto yy44;
yy57:
		YYDEBUG(57, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'e') goto yy64;
		goto yy44;
yy58:
		YYDEBUG(58, *YYCURSOR);
		++YYCURSOR;
		YYDEBUG(59, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 92 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return T_POUND;
}
#line 699 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy60:
		YYDEBUG(60, *YYCURSOR);
		++YYCURSOR;
		if ((yych = *YYCURSOR) == ':') goto yy62;
		YYDEBUG(61, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 98 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return T_COLON;
}
#line 710 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy62:
		YYDEBUG(62, *YYCURSOR);
		++YYCURSOR;
		YYDEBUG(63, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 95 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return T_DCOLON;
}
#line 720 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy64:
		YYDEBUG(64, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'n') goto yy70;
		goto yy44;
yy65:
		YYDEBUG(65, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych == '/') goto yy67;
yy66:
		YYDEBUG(66, *YYCURSOR);
		YYCURSOR = YYMARKER;
		if (yyaccept <= 2) {
			if (yyaccept <= 1) {
				if (yyaccept <= 0) {
					goto yy41;
				} else {
					goto yy47;
				}
			} else {
				goto yy75;
			}
		} else {
			if (yyaccept <= 3) {
				goto yy108;
			} else {
				goto yy119;
			}
		}
yy67:
		YYDEBUG(67, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych != '/') goto yy66;
		YYDEBUG(68, *YYCURSOR);
		++YYCURSOR;
		YYDEBUG(69, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 86 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, STR_PARAM);
	yylval->str = zend_strndup(yytext, yyleng);
	yylval->len = yyleng;
	return T_PROTO;
}
#line 766 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy70:
		YYDEBUG(70, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych != 'd') goto yy44;
		YYDEBUG(71, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych != '_') goto yy44;
yy72:
		YYDEBUG(72, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 32) {
			goto yy73;
		}
		goto yy44;
yy73:
		YYDEBUG(73, *YYCURSOR);
		yyaccept = 2;
		YYMARKER = ++YYCURSOR;
		YYFILL(3);
		yych = *YYCURSOR;
		YYDEBUG(74, *YYCURSOR);
		if (yybm[0+yych] & 32) {
			goto yy73;
		}
		if (yych <= 0x1F) {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy75;
				if (yych <= 0x08) goto yy43;
			} else {
				if (yych != '\r') goto yy43;
			}
		} else {
			if (yych <= '#') {
				if (yych <= ' ') goto yy75;
				if (yych <= '"') goto yy43;
			} else {
				if (yych == ':') goto yy65;
				goto yy43;
			}
		}
yy75:
		YYDEBUG(75, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 126 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, OP_PARAM);
	yylval->str = zend_strndup(yytext, yyleng);
	yylval->len = yyleng;
	return T_OPCODE;
}
#line 820 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy76:
		YYDEBUG(76, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych != 'N') goto yy44;
		YYDEBUG(77, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych != 'D') goto yy44;
		YYDEBUG(78, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == '_') goto yy72;
		goto yy44;
yy79:
		YYDEBUG(79, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'S') goto yy80;
		if (yych != 's') goto yy44;
yy80:
		YYDEBUG(80, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy81;
		}
		goto yy44;
yy81:
		YYDEBUG(81, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(82, *YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy81;
		}
		YYDEBUG(83, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 102 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, NUMERIC_PARAM);
	yylval->num = 1;		
	return T_TRUTHY;
}
#line 866 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy84:
		YYDEBUG(84, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'U') goto yy85;
		if (yych != 'u') goto yy44;
yy85:
		YYDEBUG(85, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy80;
		if (yych == 'e') goto yy80;
		goto yy44;
yy86:
		YYDEBUG(86, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'F') goto yy87;
		if (yych != 'f') goto yy44;
yy87:
		YYDEBUG(87, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy44;
			if (yych >= '\v') goto yy44;
		} else {
			if (yych <= '\r') goto yy88;
			if (yych != ' ') goto yy44;
		}
yy88:
		YYDEBUG(88, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(89, *YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy90;
			if (yych <= '\n') goto yy88;
		} else {
			if (yych <= '\r') goto yy88;
			if (yych == ' ') goto yy88;
		}
yy90:
		YYDEBUG(90, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 108 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, NUMERIC_PARAM);
	yylval->num = 0;
	return T_FALSY;
}
#line 919 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy91:
		YYDEBUG(91, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'L') goto yy92;
		if (yych != 'l') goto yy44;
yy92:
		YYDEBUG(92, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'S') goto yy93;
		if (yych != 's') goto yy44;
yy93:
		YYDEBUG(93, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy87;
		if (yych == 'e') goto yy87;
		goto yy44;
yy94:
		YYDEBUG(94, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'A') goto yy95;
		if (yych != 'a') goto yy44;
yy95:
		YYDEBUG(95, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'B') goto yy96;
		if (yych != 'b') goto yy44;
yy96:
		YYDEBUG(96, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'L') goto yy97;
		if (yych != 'l') goto yy44;
yy97:
		YYDEBUG(97, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy98;
		if (yych != 'e') goto yy44;
yy98:
		YYDEBUG(98, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'D') goto yy80;
		if (yych == 'd') goto yy80;
		goto yy44;
yy99:
		YYDEBUG(99, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'S') goto yy100;
		if (yych != 's') goto yy44;
yy100:
		YYDEBUG(100, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'A') goto yy101;
		if (yych != 'a') goto yy44;
yy101:
		YYDEBUG(101, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'B') goto yy102;
		if (yych != 'b') goto yy44;
yy102:
		YYDEBUG(102, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'L') goto yy103;
		if (yych != 'l') goto yy44;
yy103:
		YYDEBUG(103, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'E') goto yy104;
		if (yych != 'e') goto yy44;
yy104:
		YYDEBUG(104, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'D') goto yy87;
		if (yych == 'd') goto yy87;
		goto yy44;
yy105:
		YYDEBUG(105, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy106;
		}
		goto yy44;
yy106:
		YYDEBUG(106, *YYCURSOR);
		yyaccept = 3;
		YYMARKER = ++YYCURSOR;
		YYFILL(3);
		yych = *YYCURSOR;
		YYDEBUG(107, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy106;
		}
		if (yych <= 0x1F) {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy108;
				if (yych <= 0x08) goto yy43;
			} else {
				if (yych != '\r') goto yy43;
			}
		} else {
			if (yych <= '#') {
				if (yych <= ' ') goto yy108;
				if (yych <= '"') goto yy43;
			} else {
				if (yych == ':') goto yy65;
				goto yy43;
			}
		}
yy108:
		YYDEBUG(108, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 120 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, ADDR_PARAM);
	yylval->addr = strtoul(yytext, 0, 16);
	return T_ADDR;
}
#line 1050 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy109:
		YYDEBUG(109, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy44;
			if (yych >= '\v') goto yy44;
		} else {
			if (yych <= '\r') goto yy110;
			if (yych != ' ') goto yy44;
		}
yy110:
		YYDEBUG(110, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(111, *YYCURSOR);
		if (yych <= '\f') {
			if (yych <= 0x08) goto yy112;
			if (yych <= '\n') goto yy110;
		} else {
			if (yych <= '\r') goto yy110;
			if (yych == ' ') goto yy110;
		}
yy112:
		YYDEBUG(112, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 80 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(RAW);
	phpdbg_init_param(yylval, EMPTY_PARAM);
	return T_IF;
}
#line 1084 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy113:
		YYDEBUG(113, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych <= ' ') {
			if (yych <= '\f') {
				if (yych <= 0x08) goto yy44;
				if (yych >= '\v') goto yy44;
			} else {
				if (yych <= '\r') goto yy114;
				if (yych <= 0x1F) goto yy44;
			}
		} else {
			if (yych <= '.') {
				if (yych <= ',') goto yy44;
				if (yych <= '-') goto yy116;
				goto yy117;
			} else {
				if (yych <= '/') goto yy44;
				if (yych <= '9') goto yy117;
				goto yy44;
			}
		}
yy114:
		YYDEBUG(114, *YYCURSOR);
		++YYCURSOR;
		YYFILL(2);
		yych = *YYCURSOR;
		YYDEBUG(115, *YYCURSOR);
		if (yych <= ' ') {
			if (yych <= '\f') {
				if (yych <= 0x08) goto yy66;
				if (yych <= '\n') goto yy114;
				goto yy66;
			} else {
				if (yych <= '\r') goto yy114;
				if (yych <= 0x1F) goto yy66;
				goto yy114;
			}
		} else {
			if (yych <= '.') {
				if (yych <= ',') goto yy66;
				if (yych <= '-') goto yy120;
				goto yy121;
			} else {
				if (yych <= '/') goto yy66;
				if (yych <= '9') goto yy121;
				goto yy66;
			}
		}
yy116:
		YYDEBUG(116, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == '.') goto yy117;
		if (yych <= '/') goto yy44;
		if (yych >= ':') goto yy44;
yy117:
		YYDEBUG(117, *YYCURSOR);
		yyaccept = 4;
		YYMARKER = ++YYCURSOR;
		YYFILL(3);
		yych = *YYCURSOR;
		YYDEBUG(118, *YYCURSOR);
		if (yych <= ' ') {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy119;
				if (yych <= 0x08) goto yy43;
			} else {
				if (yych == '\r') goto yy119;
				if (yych <= 0x1F) goto yy43;
			}
		} else {
			if (yych <= '.') {
				if (yych == '#') goto yy119;
				if (yych <= '-') goto yy43;
				goto yy117;
			} else {
				if (yych <= '/') goto yy43;
				if (yych <= '9') goto yy117;
				if (yych <= ':') goto yy65;
				goto yy43;
			}
		}
yy119:
		YYDEBUG(119, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 73 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	char *text = yytext + 2;
	while (*++text < '0');
	yylval->num = atoi(text);
	return T_REQ_ID;
}
#line 1179 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy120:
		YYDEBUG(120, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yych == '.') goto yy121;
		if (yych <= '/') goto yy66;
		if (yych >= ':') goto yy66;
yy121:
		YYDEBUG(121, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(122, *YYCURSOR);
		if (yych == '.') goto yy121;
		if (yych <= '/') goto yy119;
		if (yych <= '9') goto yy121;
		goto yy119;
	}
/* *********************************** */
yyc_PRE_RAW:
	{
		static const unsigned char yybm[] = {
			  0,   0,   0,   0,   0,   0,   0,   0, 
			  0, 160,  48,   0,   0, 160,   0,   0, 
			  0,   0,   0,   0,   0,   0,   0,   0, 
			  0,   0,   0,   0,   0,   0,   0,   0, 
			160,   0,   0,   0,   0,   0,   0,   0, 
			  0,   0,   0,   0,   0,   0,  64,   0, 
			 64,  64,  64,  64,  64,  64,  64,  64, 
			 64,  64,   0,   0,   0,   0,   0,   0, 
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
		YYDEBUG(123, *YYCURSOR);
		YYFILL(2);
		yych = *YYCURSOR;
		if (yybm[0+yych] & 16) {
			goto yy127;
		}
		if (yych <= '\r') {
			if (yych <= 0x08) {
				if (yych <= 0x00) goto yy130;
				goto yy132;
			} else {
				if (yych <= '\t') goto yy125;
				if (yych <= '\f') goto yy132;
			}
		} else {
			if (yych <= ' ') {
				if (yych <= 0x1F) goto yy132;
			} else {
				if (yych == '-') goto yy131;
				goto yy132;
			}
		}
yy125:
		YYDEBUG(125, *YYCURSOR);
		++YYCURSOR;
		if ((yych = *YYCURSOR) <= '\f') {
			if (yych <= 0x00) goto yy142;
			if (yych <= 0x08) goto yy126;
			if (yych <= '\n') goto yy142;
		} else {
			if (yych <= '\r') goto yy142;
			if (yych == ' ') goto yy142;
		}
yy126:
		YYDEBUG(126, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 169 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	YYSETCONDITION(RAW);

	YYCURSOR = LEX(text);
	goto restart;
}
#line 1277 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy127:
		YYDEBUG(127, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(128, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy141;
		}
		if (yych <= 0x00) goto yy140;
		if (yych == '\n') goto yy127;
yy129:
		YYDEBUG(129, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 69 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return 0;
}
#line 1296 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy130:
		YYDEBUG(130, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy126;
yy131:
		YYDEBUG(131, *YYCURSOR);
		yyaccept = 0;
		yych = *(YYMARKER = ++YYCURSOR);
		if (yych == 'r') goto yy133;
		goto yy126;
yy132:
		YYDEBUG(132, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy126;
yy133:
		YYDEBUG(133, *YYCURSOR);
		++YYCURSOR;
		YYFILL(2);
		yych = *YYCURSOR;
		YYDEBUG(134, *YYCURSOR);
		if (yybm[0+yych] & 32) {
			goto yy133;
		}
		if (yych <= '.') {
			if (yych <= ',') goto yy135;
			if (yych <= '-') goto yy136;
			goto yy137;
		} else {
			if (yych <= '/') goto yy135;
			if (yych <= '9') goto yy137;
		}
yy135:
		YYDEBUG(135, *YYCURSOR);
		YYCURSOR = YYMARKER;
		goto yy126;
yy136:
		YYDEBUG(136, *YYCURSOR);
		yych = *++YYCURSOR;
		if (yybm[0+yych] & 64) {
			goto yy137;
		}
		goto yy135;
yy137:
		YYDEBUG(137, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(138, *YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy137;
		}
		YYDEBUG(139, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 73 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	char *text = yytext + 2;
	while (*++text < '0');
	yylval->num = atoi(text);
	return T_REQ_ID;
}
#line 1357 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy140:
		YYDEBUG(140, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy129;
yy141:
		YYDEBUG(141, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
yy142:
		YYDEBUG(142, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy141;
		}
		if (yych <= 0x00) goto yy140;
		if (yych == '\n') goto yy127;
		YYDEBUG(143, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 147 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	/* ignore whitespace */

	goto restart;
}
#line 1382 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
	}
/* *********************************** */
yyc_RAW:
	{
		static const unsigned char yybm[] = {
			  0, 128, 128, 128, 128, 128, 128, 128, 
			128, 160,  64, 128, 128, 160, 128, 128, 
			128, 128, 128, 128, 128, 128, 128, 128, 
			128, 128, 128, 128, 128, 128, 128, 128, 
			160, 128, 128, 128, 128, 128, 128, 128, 
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
			128, 128, 128, 128, 128, 128, 128, 128, 
			128, 128, 128, 128, 128, 128, 128, 128, 
			128, 128, 128, 128, 128, 128, 128, 128, 
		};
		YYDEBUG(144, *YYCURSOR);
		YYFILL(1);
		yych = *YYCURSOR;
		if (yybm[0+yych] & 32) {
			goto yy146;
		}
		if (yych <= 0x00) goto yy152;
		if (yych == '\n') goto yy149;
		goto yy153;
yy146:
		YYDEBUG(146, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(147, *YYCURSOR);
		if (yybm[0+yych] & 32) {
			goto yy146;
		}
		if (yych <= 0x00) goto yy152;
		if (yych == '\n') goto yy149;
		goto yy153;
yy148:
		YYDEBUG(148, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 140 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	phpdbg_init_param(yylval, STR_PARAM);
	yylval->str = zend_strndup(yytext, yyleng);
	yylval->len = yyleng;
	return T_INPUT;
}
#line 1452 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy149:
		YYDEBUG(149, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(150, *YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy149;
		}
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy152;
			if (yych == '\t') goto yy155;
		} else {
			if (yych <= '\r') goto yy155;
			if (yych == ' ') goto yy155;
		}
yy151:
		YYDEBUG(151, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 69 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	return 0;
}
#line 1476 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
yy152:
		YYDEBUG(152, *YYCURSOR);
		yych = *++YYCURSOR;
		goto yy151;
yy153:
		YYDEBUG(153, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(154, *YYCURSOR);
		if (yybm[0+yych] & 128) {
			goto yy153;
		}
		goto yy148;
yy155:
		YYDEBUG(155, *YYCURSOR);
		++YYCURSOR;
		YYFILL(1);
		yych = *YYCURSOR;
		YYDEBUG(156, *YYCURSOR);
		if (yybm[0+yych] & 64) {
			goto yy149;
		}
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy152;
			if (yych == '\t') goto yy155;
		} else {
			if (yych <= '\r') goto yy155;
			if (yych == ' ') goto yy155;
		}
		YYDEBUG(157, *YYCURSOR);
		yyleng = (size_t) YYCURSOR - (size_t) yytext;
#line 147 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"
		{
	/* ignore whitespace */

	goto restart;
}
#line 1515 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.c"
	}
}
#line 183 "/Users/Bob/php-src-5.6/sapi/phpdbg/phpdbg_lexer.l"

}
