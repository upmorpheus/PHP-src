#ifndef VARIANT_H
#define VARIANT_H

#if PHP_WIN32

#define ALLOC_VARIANT(v)	(v) = (VARIANT *) emalloc(sizeof(VARIANT))
#define IS_VARIANT			php_VARIANT_get_le_variant()

#endif  /* PHP_WIN32 */

#endif  /* VARIANT_H */
