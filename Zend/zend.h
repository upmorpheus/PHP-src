/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_H
#define ZEND_H

#define ZEND_VERSION "2.8.0-dev"

#define ZEND_ENGINE_2

#ifdef __cplusplus
#define BEGIN_EXTERN_C() extern "C" {
#define END_EXTERN_C() }
#else
#define BEGIN_EXTERN_C()
#define END_EXTERN_C()
#endif

/*
 * general definitions
 */

#ifdef ZEND_WIN32
# include "zend_config.w32.h"
# define ZEND_PATHS_SEPARATOR		';'
#elif defined(NETWARE)
# include <zend_config.h>
# define ZEND_PATHS_SEPARATOR		';'
#elif defined(__riscos__)
# include <zend_config.h>
# define ZEND_PATHS_SEPARATOR		';'
#else
# include <zend_config.h>
# define ZEND_PATHS_SEPARATOR		':'
#endif

/* Only use this macro if you know for sure that all of the switches values
   are covered by its case statements */
#if ZEND_DEBUG
# define EMPTY_SWITCH_DEFAULT_CASE() default: ZEND_ASSERT(0); break;
#elif defined(ZEND_WIN32)
# define EMPTY_SWITCH_DEFAULT_CASE() default: __assume(0); break;
#else
# define EMPTY_SWITCH_DEFAULT_CASE()
#endif

/* all HAVE_XXX test have to be after the include of zend_config above */

#include <stdio.h>
#include <assert.h>

#ifdef HAVE_UNIX_H
# include <unix.h>
#endif

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#if defined(HAVE_LIBDL) && !defined(ZEND_WIN32)

# ifndef RTLD_LAZY
#  define RTLD_LAZY 1    /* Solaris 1, FreeBSD's (2.1.7.1 and older) */
# endif

# ifndef RTLD_GLOBAL
#  define RTLD_GLOBAL 0
# endif

# if defined(RTLD_GROUP) && defined(RTLD_WORLD) && defined(RTLD_PARENT)
#  define DL_LOAD(libname)			dlopen(libname, RTLD_LAZY | RTLD_GLOBAL | RTLD_GROUP | RTLD_WORLD | RTLD_PARENT)
# elif defined(RTLD_DEEPBIND)
#  define DL_LOAD(libname)			dlopen(libname, RTLD_LAZY | RTLD_GLOBAL | RTLD_DEEPBIND)
# else
#  define DL_LOAD(libname)			dlopen(libname, RTLD_LAZY | RTLD_GLOBAL)
# endif
# define DL_UNLOAD					dlclose
# if defined(DLSYM_NEEDS_UNDERSCORE)
#  define DL_FETCH_SYMBOL(h,s)		dlsym((h), "_" s)
# else
#  define DL_FETCH_SYMBOL			dlsym
# endif
# define DL_ERROR					dlerror
# define DL_HANDLE					void *
# define ZEND_EXTENSIONS_SUPPORT	1
#elif defined(ZEND_WIN32)
# define DL_LOAD(libname)			LoadLibrary(libname)
# define DL_FETCH_SYMBOL			GetProcAddress
# define DL_UNLOAD					FreeLibrary
# define DL_HANDLE					HMODULE
# define ZEND_EXTENSIONS_SUPPORT	1
#else
# define DL_HANDLE					void *
# define ZEND_EXTENSIONS_SUPPORT	0
#endif

#if HAVE_ALLOCA_H && !defined(_ALLOCA_H)
#  include <alloca.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# ifndef HAVE_ALLOCA_H
#  ifdef _AIX
#pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

/* Compatibility with non-clang compilers */
#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

/* GCC x.y.z supplies __GNUC__ = x and __GNUC_MINOR__ = y */
#ifdef __GNUC__
# define ZEND_GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#else
# define ZEND_GCC_VERSION 0
#endif

#if ZEND_GCC_VERSION >= 2096
# define ZEND_ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
#else
# define ZEND_ATTRIBUTE_MALLOC
#endif

#if ZEND_GCC_VERSION >= 4003 || __has_attribute(alloc_size)
# define ZEND_ATTRIBUTE_ALLOC_SIZE(X) __attribute__ ((alloc_size(X)))
# define ZEND_ATTRIBUTE_ALLOC_SIZE2(X,Y) __attribute__ ((alloc_size(X,Y)))
#else
# define ZEND_ATTRIBUTE_ALLOC_SIZE(X)
# define ZEND_ATTRIBUTE_ALLOC_SIZE2(X,Y)
#endif

/* Format string checks are disabled by default, because we use custom format modifiers (like %p),
 * which cause a large amount of false positives. You can enable format checks by adding
 * -DZEND_CHECK_FORMAT_STRINGS to CFLAGS. */

#if ZEND_GCC_VERSION >= 2007 && defined(ZEND_CHECK_FORMAT_STRINGS)
# define ZEND_ATTRIBUTE_FORMAT(type, idx, first) __attribute__ ((format(type, idx, first)))
#else
# define ZEND_ATTRIBUTE_FORMAT(type, idx, first)
#endif

#if ZEND_GCC_VERSION >= 3001 && !defined(__INTEL_COMPILER) && defined(ZEND_CHECK_FORMAT_STRINGS)
# define ZEND_ATTRIBUTE_PTR_FORMAT(type, idx, first) __attribute__ ((format(type, idx, first)))
#else
# define ZEND_ATTRIBUTE_PTR_FORMAT(type, idx, first)
#endif

#if ZEND_GCC_VERSION >= 3001
# define ZEND_ATTRIBUTE_DEPRECATED  __attribute__((deprecated))
#elif defined(ZEND_WIN32) && defined(_MSC_VER) && _MSC_VER >= 1300
# define ZEND_ATTRIBUTE_DEPRECATED  __declspec(deprecated)
#else
# define ZEND_ATTRIBUTE_DEPRECATED
#endif

#if defined(__GNUC__) && ZEND_GCC_VERSION >= 4003
# define ZEND_ATTRIBUTE_UNUSED __attribute__((unused))
# define ZEND_ATTRIBUTE_UNUSED_LABEL __attribute__((cold, unused));
#else
# define ZEND_ATTRIBUTE_UNUSED
# define ZEND_ATTRIBUTE_UNUSED_LABEL
#endif

#if defined(__GNUC__) && ZEND_GCC_VERSION >= 3004 && defined(__i386__)
# define ZEND_FASTCALL __attribute__((fastcall))
#elif defined(_MSC_VER) && defined(_M_IX86)
# define ZEND_FASTCALL __fastcall
#else
# define ZEND_FASTCALL
#endif

#if defined(__GNUC__) && ZEND_GCC_VERSION >= 3004
#else
# define __restrict__
#endif
#define restrict __restrict__

#if (HAVE_ALLOCA || (defined (__GNUC__) && __GNUC__ >= 2)) && !(defined(ZTS) && defined(NETWARE)) && !(defined(ZTS) && defined(HPUX)) && !defined(DARWIN)
# define ZEND_ALLOCA_MAX_SIZE (32 * 1024)
# define ALLOCA_FLAG(name) \
	zend_bool name;
# define SET_ALLOCA_FLAG(name) \
	name = 1
# define do_alloca_ex(size, limit, use_heap) \
	((use_heap = (UNEXPECTED((size) > (limit)))) ? emalloc(size) : alloca(size))
# define do_alloca(size, use_heap) \
	do_alloca_ex(size, ZEND_ALLOCA_MAX_SIZE, use_heap)
# define free_alloca(p, use_heap) \
	do { if (UNEXPECTED(use_heap)) efree(p); } while (0)
#else
# define ALLOCA_FLAG(name)
# define SET_ALLOCA_FLAG(name)
# define do_alloca(p, use_heap)		emalloc(p)
# define free_alloca(p, use_heap)	efree(p)
#endif

#if ZEND_DEBUG
#define ZEND_FILE_LINE_D				const char *__zend_filename, const uint __zend_lineno
#define ZEND_FILE_LINE_DC				, ZEND_FILE_LINE_D
#define ZEND_FILE_LINE_ORIG_D			const char *__zend_orig_filename, const uint __zend_orig_lineno
#define ZEND_FILE_LINE_ORIG_DC			, ZEND_FILE_LINE_ORIG_D
#define ZEND_FILE_LINE_RELAY_C			__zend_filename, __zend_lineno
#define ZEND_FILE_LINE_RELAY_CC			, ZEND_FILE_LINE_RELAY_C
#define ZEND_FILE_LINE_C				__FILE__, __LINE__
#define ZEND_FILE_LINE_CC				, ZEND_FILE_LINE_C
#define ZEND_FILE_LINE_EMPTY_C			NULL, 0
#define ZEND_FILE_LINE_EMPTY_CC			, ZEND_FILE_LINE_EMPTY_C
#define ZEND_FILE_LINE_ORIG_RELAY_C		__zend_orig_filename, __zend_orig_lineno
#define ZEND_FILE_LINE_ORIG_RELAY_CC	, ZEND_FILE_LINE_ORIG_RELAY_C
#define ZEND_ASSERT(c)					assert(c)
#else
#define ZEND_FILE_LINE_D
#define ZEND_FILE_LINE_DC
#define ZEND_FILE_LINE_ORIG_D
#define ZEND_FILE_LINE_ORIG_DC
#define ZEND_FILE_LINE_RELAY_C
#define ZEND_FILE_LINE_RELAY_CC
#define ZEND_FILE_LINE_C
#define ZEND_FILE_LINE_CC
#define ZEND_FILE_LINE_EMPTY_C
#define ZEND_FILE_LINE_EMPTY_CC
#define ZEND_FILE_LINE_ORIG_RELAY_C
#define ZEND_FILE_LINE_ORIG_RELAY_CC
#define ZEND_ASSERT(c)
#endif	/* ZEND_DEBUG */

#ifdef ZTS
#define ZTS_V 1
#else
#define ZTS_V 0
#endif

#include "zend_errors.h"
#include "zend_alloc.h"

#include "zend_types.h"

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif

#ifndef LONG_MIN
#define LONG_MIN (- LONG_MAX - 1)
#endif

#if SIZEOF_ZEND_LONG == 4
#define MAX_LENGTH_OF_LONG 11
static const char long_min_digits[] = "2147483648";
#elif SIZEOF_ZEND_LONG == 8
#define MAX_LENGTH_OF_LONG 20
static const char long_min_digits[] = "9223372036854775808";
#else
#error "Unknown SIZEOF_ZEND_LONG"
#endif

#define MAX_LENGTH_OF_DOUBLE 32

typedef enum {
  SUCCESS =  0,
  FAILURE = -1,		/* this MUST stay a negative number, or it may affect functions! */
} ZEND_RESULT_CODE;

#include "zend_hash.h"
#include "zend_llist.h"

#define INTERNAL_FUNCTION_PARAMETERS uint32_t param_count, zval *return_value TSRMLS_DC
#define INTERNAL_FUNCTION_PARAM_PASSTHRU param_count, return_value TSRMLS_CC

#define USED_RET() \
	(!EG(current_execute_data) || \
	 !EG(current_execute_data)->prev_execute_data || \
	 !ZEND_USER_CODE(EG(current_execute_data)->prev_execute_data->func->common.type) || \
	 !(EG(current_execute_data)->prev_execute_data->opline->result_type & EXT_TYPE_UNUSED))

#if defined(__GNUC__) && __GNUC__ >= 3 && !defined(__INTEL_COMPILER) && !defined(DARWIN) && !defined(__hpux) && !defined(_AIX) && !defined(__osf__)
#  define ZEND_NORETURN __attribute__((noreturn))
void zend_error_noreturn(int type, const char *format, ...) __attribute__ ((noreturn));
#elif defined(ZEND_WIN32)
#  define ZEND_NORETURN __declspec(noreturn)
ZEND_API ZEND_NORETURN void zend_error_noreturn(int type, const char *format, ...);
#else
#  define ZEND_NORETURN
#  define zend_error_noreturn zend_error
#endif

#include "zend_object_handlers.h"

/* overloaded elements data types */
#define OE_IS_ARRAY					(1<<0)
#define OE_IS_OBJECT				(1<<1)
#define OE_IS_METHOD				(1<<2)

#define Z_REFCOUNT_P(pz)			zval_refcount_p(pz)
#define Z_SET_REFCOUNT_P(pz, rc)	zval_set_refcount_p(pz, rc)
#define Z_ADDREF_P(pz)				zval_addref_p(pz)
#define Z_DELREF_P(pz)				zval_delref_p(pz)

#define Z_REFCOUNT(z)				Z_REFCOUNT_P(&(z))
#define Z_SET_REFCOUNT(z, rc)		Z_SET_REFCOUNT_P(&(z), rc)
#define Z_ADDREF(z)					Z_ADDREF_P(&(z))
#define Z_DELREF(z)					Z_DELREF_P(&(z))

#define Z_TRY_ADDREF_P(pz) do {		\
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_ADDREF_P((pz));			\
	}								\
} while (0)

#define Z_TRY_DELREF_P(pz) do {		\
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_DELREF_P((pz));			\
	}								\
} while (0)

#define Z_TRY_ADDREF(z)				Z_TRY_ADDREF_P(&(z))
#define Z_TRY_DELREF(z)				Z_TRY_DELREF_P(&(z))

#if ZEND_DEBUG
#define zend_always_inline inline
#define zend_never_inline
#else
#if defined(__GNUC__)
#if __GNUC__ >= 3
#define zend_always_inline inline __attribute__((always_inline))
#define zend_never_inline __attribute__((noinline))
#else
#define zend_always_inline inline
#define zend_never_inline
#endif
#elif defined(_MSC_VER)
#define zend_always_inline __forceinline
#define zend_never_inline
#else
#define zend_always_inline inline
#define zend_never_inline
#endif
#endif /* ZEND_DEBUG */

#if (defined (__GNUC__) && __GNUC__ > 2 ) && !defined(DARWIN) && !defined(__hpux) && !defined(_AIX)
# define EXPECTED(condition)   __builtin_expect(!(!(condition)), 1)
# define UNEXPECTED(condition) __builtin_expect(!(!(condition)), 0)
#else
# define EXPECTED(condition)   (condition)
# define UNEXPECTED(condition) (condition)
#endif

#ifndef XtOffsetOf
# if defined(CRAY) || (defined(__ARMCC_VERSION) && !defined(LINUX))
# ifdef __STDC__
# define XtOffset(p_type, field) _Offsetof(p_type, field)
# else
# ifdef CRAY2
# define XtOffset(p_type, field) \
    (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

# else /* !CRAY2 */

# define XtOffset(p_type, field) ((unsigned int)&(((p_type)NULL)->field))

# endif /* !CRAY2 */
# endif /* __STDC__ */
# else /* ! (CRAY || __arm) */

# define XtOffset(p_type, field) \
    ((zend_long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

# endif /* !CRAY */

# ifdef offsetof
# define XtOffsetOf(s_type, field) offsetof(s_type, field)
# else
# define XtOffsetOf(s_type, field) XtOffset(s_type*, field)
# endif

#endif

#include "zend_string.h"
#include "zend_ast.h"

static zend_always_inline uint32_t zval_refcount_p(zval* pz) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz) || Z_IMMUTABLE_P(pz));
	return GC_REFCOUNT(Z_COUNTED_P(pz));
}

static zend_always_inline uint32_t zval_set_refcount_p(zval* pz, uint32_t rc) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return GC_REFCOUNT(Z_COUNTED_P(pz)) = rc;
}

static zend_always_inline uint32_t zval_addref_p(zval* pz) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return ++GC_REFCOUNT(Z_COUNTED_P(pz));
}

static zend_always_inline uint32_t zval_delref_p(zval* pz) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return --GC_REFCOUNT(Z_COUNTED_P(pz));
}

/* excpt.h on Digital Unix 4.0 defines function_table */
#undef function_table

/* A lot of stuff needs shifiting around in order to include zend_compile.h here */
union _zend_function;

#include "zend_iterators.h"

struct _zend_serialize_data;
struct _zend_unserialize_data;

typedef struct _zend_serialize_data zend_serialize_data;
typedef struct _zend_unserialize_data zend_unserialize_data;

struct _zend_trait_method_reference {
	zend_string *method_name;	
	zend_class_entry *ce;
	zend_string *class_name;
};
typedef struct _zend_trait_method_reference	zend_trait_method_reference;

struct _zend_trait_precedence {
	zend_trait_method_reference *trait_method;	
	union {
		zend_class_entry  *ce;
		zend_string       *class_name;
	} *exclude_from_classes;
};
typedef struct _zend_trait_precedence zend_trait_precedence;

struct _zend_trait_alias {
	zend_trait_method_reference *trait_method;
	
	/**
	* name for method to be added
	*/
	zend_string *alias;
	
	/**
	* modifiers to be set on trait method
	*/
	uint32_t modifiers;
};
typedef struct _zend_trait_alias zend_trait_alias;

struct _zend_class_entry {
	char type;
	zend_string *name;
	struct _zend_class_entry *parent;
	int refcount;
	uint32_t ce_flags;

	int default_properties_count;
	int default_static_members_count;
	zval *default_properties_table;
	zval *default_static_members_table;
	zval *static_members_table;
	HashTable function_table;
	HashTable properties_info;
	HashTable constants_table;

	union _zend_function *constructor;
	union _zend_function *destructor;
	union _zend_function *clone;
	union _zend_function *__get;
	union _zend_function *__set;
	union _zend_function *__unset;
	union _zend_function *__isset;
	union _zend_function *__call;
	union _zend_function *__callstatic;
	union _zend_function *__tostring;
	union _zend_function *__debugInfo;
	union _zend_function *serialize_func;
	union _zend_function *unserialize_func;

	zend_class_iterator_funcs iterator_funcs;

	/* handlers */
	zend_object* (*create_object)(zend_class_entry *class_type TSRMLS_DC);
	zend_object_iterator *(*get_iterator)(zend_class_entry *ce, zval *object, int by_ref TSRMLS_DC);
	int (*interface_gets_implemented)(zend_class_entry *iface, zend_class_entry *class_type TSRMLS_DC); /* a class implements this interface */
	union _zend_function *(*get_static_method)(zend_class_entry *ce, zend_string* method TSRMLS_DC);

	/* serializer callbacks */
	int (*serialize)(zval *object, unsigned char **buffer, uint32_t *buf_len, zend_serialize_data *data TSRMLS_DC);
	int (*unserialize)(zval *object, zend_class_entry *ce, const unsigned char *buf, uint32_t buf_len, zend_unserialize_data *data TSRMLS_DC);

	uint32_t num_interfaces;
	uint32_t num_traits;
	zend_class_entry **interfaces;
	
	zend_class_entry **traits;
	zend_trait_alias **trait_aliases;
	zend_trait_precedence **trait_precedences;

	union {
		struct {
			zend_string *filename;
			uint32_t line_start;
			uint32_t line_end;
			zend_string *doc_comment;
		} user;
		struct {
			const struct _zend_function_entry *builtin_functions;
			struct _zend_module_entry *module;
		} internal;
	} info;
};

#include "zend_stream.h"
typedef struct _zend_utility_functions {
	void (*error_function)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 4, 0);
	size_t (*printf_function)(const char *format, ...) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 1, 2);
	size_t (*write_function)(const char *str, size_t str_length);
	FILE *(*fopen_function)(const char *filename, char **opened_path TSRMLS_DC);
	void (*message_handler)(zend_long message, const void *data TSRMLS_DC);
	void (*block_interruptions)(void);
	void (*unblock_interruptions)(void);
	zval *(*get_configuration_directive)(zend_string *name);
	void (*ticks_function)(int ticks TSRMLS_DC);
	void (*on_timeout)(int seconds TSRMLS_DC);
	int (*stream_open_function)(const char *filename, zend_file_handle *handle TSRMLS_DC);
	size_t (*vspprintf_function)(char **pbuf, size_t max_len, const char *format, va_list ap);
	zend_string *(*vstrpprintf_function)(size_t max_len, const char *format, va_list ap);
	char *(*getenv_function)(char *name, size_t name_len TSRMLS_DC);
	char *(*resolve_path_function)(const char *filename, int filename_len TSRMLS_DC);
} zend_utility_functions;

typedef struct _zend_utility_values {
	char *import_use_extension;
	uint import_use_extension_length;
	zend_bool html_errors;
} zend_utility_values;

typedef int (*zend_write_func_t)(const char *str, uint str_length);

#undef MIN
#undef MAX
#define MAX(a, b)  (((a)>(b))?(a):(b))
#define MIN(a, b)  (((a)<(b))?(a):(b))
#define ZEND_STRL(str)		(str), (sizeof(str)-1)
#define ZEND_STRS(str)		(str), (sizeof(str))
#define ZEND_NORMALIZE_BOOL(n)			\
	((n) ? (((n)>0) ? 1 : -1) : 0)
#define ZEND_TRUTH(x)		((x) ? 1 : 0)
#define ZEND_LOG_XOR(a, b)		(ZEND_TRUTH(a) ^ ZEND_TRUTH(b))

int zend_startup(zend_utility_functions *utility_functions, char **extensions TSRMLS_DC);
void zend_shutdown(TSRMLS_D);
void zend_register_standard_ini_entries(TSRMLS_D);
void zend_post_startup(TSRMLS_D);
void zend_set_utility_values(zend_utility_values *utility_values);

BEGIN_EXTERN_C()
ZEND_API void _zend_bailout(char *filename, uint lineno);
END_EXTERN_C()

#define zend_bailout()		_zend_bailout(__FILE__, __LINE__)

#ifdef HAVE_SIGSETJMP
#	define SETJMP(a) sigsetjmp(a, 0)
#	define LONGJMP(a,b) siglongjmp(a, b)
#	define JMP_BUF sigjmp_buf
#else
#	define SETJMP(a) setjmp(a)
#	define LONGJMP(a,b) longjmp(a, b)
#	define JMP_BUF jmp_buf
#endif

#define zend_try												\
	{															\
		JMP_BUF *__orig_bailout = EG(bailout);					\
		JMP_BUF __bailout;										\
																\
		EG(bailout) = &__bailout;								\
		if (SETJMP(__bailout)==0) {
#define zend_catch												\
		} else {												\
			EG(bailout) = __orig_bailout;
#define zend_end_try()											\
		}														\
		EG(bailout) = __orig_bailout;							\
	}
#define zend_first_try		EG(bailout)=NULL;	zend_try

BEGIN_EXTERN_C()
ZEND_API char *get_zend_version(void);
ZEND_API int zend_make_printable_zval(zval *expr, zval *expr_copy TSRMLS_DC);
ZEND_API int zend_print_zval(zval *expr, int indent TSRMLS_DC);
ZEND_API int zend_print_zval_ex(zend_write_func_t write_func, zval *expr, int indent TSRMLS_DC);
ZEND_API void zend_print_zval_r(zval *expr, int indent TSRMLS_DC);
ZEND_API void zend_print_flat_zval_r(zval *expr TSRMLS_DC);
ZEND_API void zend_print_zval_r_ex(zend_write_func_t write_func, zval *expr, int indent TSRMLS_DC);
ZEND_API void zend_output_debug_string(zend_bool trigger_break, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
END_EXTERN_C()

BEGIN_EXTERN_C()
ZEND_API void zend_activate(TSRMLS_D);
ZEND_API void zend_deactivate(TSRMLS_D);
ZEND_API void zend_call_destructors(TSRMLS_D);
ZEND_API void zend_activate_modules(TSRMLS_D);
ZEND_API void zend_deactivate_modules(TSRMLS_D);
ZEND_API void zend_post_deactivate_modules(TSRMLS_D);
END_EXTERN_C()

#if ZEND_DEBUG
#define Z_DBG(expr)		(expr)
#else
#define	Z_DBG(expr)
#endif

BEGIN_EXTERN_C()
ZEND_API void free_estring(char **str_p);
ZEND_API void free_string_zval(zval *zv);
END_EXTERN_C()

/* output support */
#define ZEND_WRITE(str, str_len)		zend_write((str), (str_len))
#define ZEND_WRITE_EX(str, str_len)		write_func((str), (str_len))
#define ZEND_PUTS(str)					zend_write((str), strlen((str)))
#define ZEND_PUTS_EX(str)				write_func((str), strlen((str)))
#define ZEND_PUTC(c)					zend_write(&(c), 1)

BEGIN_EXTERN_C()
extern ZEND_API size_t (*zend_printf)(const char *format, ...) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 1, 2);
extern ZEND_API zend_write_func_t zend_write;
extern ZEND_API FILE *(*zend_fopen)(const char *filename, char **opened_path TSRMLS_DC);
extern ZEND_API void (*zend_block_interruptions)(void);
extern ZEND_API void (*zend_unblock_interruptions)(void);
extern ZEND_API void (*zend_ticks_function)(int ticks TSRMLS_DC);
extern ZEND_API void (*zend_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args) ZEND_ATTRIBUTE_PTR_FORMAT(printf, 4, 0);
extern ZEND_API void (*zend_on_timeout)(int seconds TSRMLS_DC);
extern ZEND_API int (*zend_stream_open_function)(const char *filename, zend_file_handle *handle TSRMLS_DC);
extern size_t (*zend_vspprintf)(char **pbuf, size_t max_len, const char *format, va_list ap);
extern zend_string *(*zend_vstrpprintf)(size_t max_len, const char *format, va_list ap);
extern ZEND_API char *(*zend_getenv)(char *name, size_t name_len TSRMLS_DC);
extern ZEND_API char *(*zend_resolve_path)(const char *filename, int filename_len TSRMLS_DC);

ZEND_API void zend_error(int type, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);

void zenderror(const char *error);

/* The following #define is used for code duality in PHP for Engine 1 & 2 */
#define ZEND_STANDARD_CLASS_DEF_PTR zend_standard_class_def
extern ZEND_API zend_class_entry *zend_standard_class_def;
extern ZEND_API zend_utility_values zend_uv;
extern ZEND_API zval zval_used_for_init;

END_EXTERN_C()

#define ZEND_UV(name) (zend_uv.name)

#ifndef ZEND_SIGNALS
#define HANDLE_BLOCK_INTERRUPTIONS()		if (zend_block_interruptions) { zend_block_interruptions(); }
#define HANDLE_UNBLOCK_INTERRUPTIONS()		if (zend_unblock_interruptions) { zend_unblock_interruptions(); }
#else
#include "zend_signal.h"

#define HANDLE_BLOCK_INTERRUPTIONS()		ZEND_SIGNAL_BLOCK_INTERRUPUTIONS()
#define HANDLE_UNBLOCK_INTERRUPTIONS()		ZEND_SIGNAL_UNBLOCK_INTERRUPTIONS()
#endif

BEGIN_EXTERN_C()
ZEND_API void zend_message_dispatcher(zend_long message, const void *data TSRMLS_DC);

ZEND_API zval *zend_get_configuration_directive(zend_string *name);
END_EXTERN_C()

/* Messages for applications of Zend */
#define ZMSG_FAILED_INCLUDE_FOPEN		1L
#define ZMSG_FAILED_REQUIRE_FOPEN		2L
#define ZMSG_FAILED_HIGHLIGHT_FOPEN		3L
#define ZMSG_MEMORY_LEAK_DETECTED		4L
#define ZMSG_MEMORY_LEAK_REPEATED		5L
#define ZMSG_LOG_SCRIPT_NAME			6L
#define ZMSG_MEMORY_LEAKS_GRAND_TOTAL	7L

#define ZVAL_COPY_VALUE(z, v)							\
	do {												\
		zval *_z1 = (z);								\
		zval *_z2 = (v);								\
		(_z1)->value = (_z2)->value;					\
		Z_TYPE_INFO_P(_z1) = Z_TYPE_INFO_P(_z2);		\
	} while (0)

#define ZVAL_COPY(z, v)									\
	do {												\
		zval *__z1 = (z);								\
		zval *__z2 = (v);								\
		ZVAL_COPY_VALUE(__z1, __z2);					\
		if (Z_OPT_REFCOUNTED_P(__z1)) {					\
			Z_ADDREF_P(__z1);							\
		}												\
	} while (0)

#define ZVAL_DUP(z, v)									\
	do {												\
		zval *__z1 = (z);								\
		zval *__z2 = (v);								\
		ZVAL_COPY_VALUE(__z1, __z2);					\
		zval_opt_copy_ctor(__z1);						\
	} while (0)

#define ZVAL_DEREF(z) do {								\
		if (UNEXPECTED(Z_ISREF_P(z))) {					\
			(z) = Z_REFVAL_P(z);						\
		}												\
	} while (0)

#define ZVAL_MAKE_REF(zv) do {							\
		zval *__zv = (zv);								\
		if (!Z_ISREF_P(__zv)) {							\
			ZVAL_NEW_REF(__zv, __zv);					\
		}												\
	} while (0)

#define ZVAL_UNREF(z) do {								\
		zval *_z = (z);									\
		zend_reference *ref;							\
		ZEND_ASSERT(Z_ISREF_P(_z));						\
		ref = Z_REF_P(_z);								\
		ZVAL_COPY_VALUE(_z, &ref->val);					\
		efree_size(ref, sizeof(zend_reference));		\
	} while (0)

#define SEPARATE_STRING(zv) do {						\
		zval *_zv = (zv);								\
		if (Z_REFCOUNTED_P(_zv) &&						\
		    Z_REFCOUNT_P(_zv) > 1) {					\
			Z_DELREF_P(_zv);							\
			zval_copy_ctor_func(_zv);					\
		}												\
	} while (0)

#define SEPARATE_ARRAY(zv) do {							\
		zval *_zv = (zv);								\
		if (Z_REFCOUNT_P(_zv) > 1) {					\
			if (!Z_IMMUTABLE_P(_zv)) {					\
				Z_DELREF_P(_zv);						\
			}											\
			zval_copy_ctor_func(_zv);					\
		}												\
	} while (0)

#define SEPARATE_ZVAL_NOREF(zv) do {					\
		zval *_zv = (zv);								\
		if (Z_COPYABLE_P(_zv) ||						\
		    Z_IMMUTABLE_P(_zv)) {						\
			if (Z_REFCOUNT_P(_zv) > 1) {				\
				if (!Z_IMMUTABLE_P(_zv)) {				\
					Z_DELREF_P(_zv);					\
				}										\
				zval_copy_ctor_func(_zv);				\
			}											\
		}												\
	} while (0)

#define SEPARATE_ZVAL(zv) do {							\
		zval *_zv = (zv);								\
		if (Z_REFCOUNTED_P(_zv) ||						\
		    Z_IMMUTABLE_P(_zv)) {						\
			if (Z_REFCOUNT_P(_zv) > 1) {				\
				if (Z_COPYABLE_P(_zv) ||				\
				    Z_IMMUTABLE_P(_zv)) {				\
					if (!Z_IMMUTABLE_P(_zv)) {			\
						Z_DELREF_P(_zv);				\
					}									\
					zval_copy_ctor_func(_zv);			\
				} else if (Z_ISREF_P(_zv)) {			\
					Z_DELREF_P(_zv);					\
					ZVAL_DUP(_zv, Z_REFVAL_P(_zv));		\
				}										\
			}											\
		}												\
	} while (0)

#define SEPARATE_ZVAL_IF_NOT_REF(zv) do {				\
		zval *_zv = (zv);								\
		if (Z_COPYABLE_P(_zv) ||                    	\
		    Z_IMMUTABLE_P(_zv)) {						\
			if (Z_REFCOUNT_P(_zv) > 1) {				\
				if (!Z_IMMUTABLE_P(_zv)) {				\
					Z_DELREF_P(_zv);					\
				}										\
				zval_copy_ctor_func(_zv);				\
			}											\
		}												\
	} while (0)

#define SEPARATE_ARG_IF_REF(varptr) do { 				\
		ZVAL_DEREF(varptr);								\
		if (Z_REFCOUNTED_P(varptr)) { 					\
			Z_ADDREF_P(varptr); 						\
		}												\
	} while (0)

#define ZEND_MAX_RESERVED_RESOURCES	4

#include "zend_gc.h"
#include "zend_operators.h"
#include "zend_variables.h"

typedef enum {
	EH_NORMAL = 0,
	EH_SUPPRESS,
	EH_THROW
} zend_error_handling_t;

typedef struct {
	zend_error_handling_t  handling;
	zend_class_entry       *exception;
	zval                   user_handler;
} zend_error_handling;

ZEND_API void zend_save_error_handling(zend_error_handling *current TSRMLS_DC);
ZEND_API void zend_replace_error_handling(zend_error_handling_t error_handling, zend_class_entry *exception_class, zend_error_handling *current TSRMLS_DC);
ZEND_API void zend_restore_error_handling(zend_error_handling *saved TSRMLS_DC);

#define DEBUG_BACKTRACE_PROVIDE_OBJECT (1<<0)
#define DEBUG_BACKTRACE_IGNORE_ARGS    (1<<1)

#endif /* ZEND_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
