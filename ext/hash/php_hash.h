/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2005 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_HASH_H
#define PHP_HASH_H

#include "php.h"
#include "php_hash_types.h"

#define PHP_HASH_EXTNAME	"hash"
#define PHP_HASH_EXTVER		"0.1"
#define PHP_HASH_RESNAME	"Hash Context"

#define PHP_HASH_HMAC		0x0001

typedef void (*php_hash_init_func_t)(void *context);
typedef void (*php_hash_update_func_t)(void *context, const unsigned char *buf, unsigned int count);
typedef void (*php_hash_final_func_t)(unsigned char *digest, void *context);

typedef struct _php_hash_ops {
	php_hash_init_func_t hash_init;
	php_hash_update_func_t hash_update;
	php_hash_final_func_t hash_final;

	int digest_size;
	int block_size;
	int context_size;
} php_hash_ops;

typedef struct _php_hash_data {
	php_hash_ops *ops;
	void *context;

	long options;
	unsigned char *key;
} php_hash_data;

extern php_hash_ops php_hash_md4_ops;
extern php_hash_ops php_hash_md5_ops;
extern php_hash_ops php_hash_sha1_ops;
extern php_hash_ops php_hash_sha256_ops;
extern php_hash_ops php_hash_sha384_ops;
extern php_hash_ops php_hash_sha512_ops;
extern php_hash_ops php_hash_ripemd128_ops;
extern php_hash_ops php_hash_ripemd160_ops;
extern php_hash_ops php_hash_whirlpool_ops;
extern php_hash_ops php_hash_3tiger128_ops;
extern php_hash_ops php_hash_3tiger160_ops;
extern php_hash_ops php_hash_3tiger192_ops;
extern php_hash_ops php_hash_4tiger128_ops;
extern php_hash_ops php_hash_4tiger160_ops;
extern php_hash_ops php_hash_4tiger192_ops;
extern php_hash_ops php_hash_snefru_ops;
extern php_hash_ops php_hash_gost_ops;
extern php_hash_ops php_hash_adler32_ops;
extern php_hash_ops php_hash_crc32_ops;
extern php_hash_ops php_hash_crc32b_ops;

#define PHP_HASH_HAVAL_OPS(p,b)	extern php_hash_ops php_hash_##p##haval##b##_ops;

PHP_HASH_HAVAL_OPS(3,128)
PHP_HASH_HAVAL_OPS(3,160)
PHP_HASH_HAVAL_OPS(3,192)
PHP_HASH_HAVAL_OPS(3,224)
PHP_HASH_HAVAL_OPS(3,256)

PHP_HASH_HAVAL_OPS(4,128)
PHP_HASH_HAVAL_OPS(4,160)
PHP_HASH_HAVAL_OPS(4,192)
PHP_HASH_HAVAL_OPS(4,224)
PHP_HASH_HAVAL_OPS(4,256)

PHP_HASH_HAVAL_OPS(5,128)
PHP_HASH_HAVAL_OPS(5,160)
PHP_HASH_HAVAL_OPS(5,192)
PHP_HASH_HAVAL_OPS(5,224)
PHP_HASH_HAVAL_OPS(5,256)

extern zend_module_entry hash_module_entry;
#define phpext_hash_ptr &hash_module_entry

#ifdef PHP_WIN32
#define PHP_HASH_API __declspec(dllexport)
#else
#define PHP_HASH_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_HASH_API php_hash_ops *php_hash_fetch_ops(const char *algo, int algo_len);
PHP_HASH_API void php_hash_register_algo(const char *algo, php_hash_ops *ops);

static inline void php_hash_bin2hex(char *out, const unsigned char *in, int in_len)
{
	static const char hexits[16] = "0123456789abcdef";
	int i;

	for(i = 0; i < in_len; i++) {
		out[i * 2]       = hexits[in[i] >> 4];
		out[(i * 2) + 1] = hexits[in[i] &  0x0F];
	}
}

#endif	/* PHP_HASH_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
