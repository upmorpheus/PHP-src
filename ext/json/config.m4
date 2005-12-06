dnl
dnl $Id$
dnl

AC_DEFUN([PHP_JSON_ADD_SOURCES], [
  PHP_JSON_SOURCES="$PHP_JSON_SOURCES $1"
])

AC_DEFUN([PHP_JSON_ADD_BASE_SOURCES], [
  PHP_JSON_BASE_SOURCES="$PHP_JSON_BASE_SOURCES $1"
])

AC_DEFUN([PHP_JSON_ADD_BUILD_DIR], [
  PHP_JSON_EXTRA_BUILD_DIRS="$PHP_JSON_EXTRA_BUILD_DIRS $1"
])

AC_DEFUN([PHP_JSON_ADD_INCLUDE], [
  PHP_JSON_EXTRA_INCLUDES="$PHP_JSON_EXTRA_INCLUDES $1"
])

AC_DEFUN([PHP_JSON_ADD_CONFIG_HEADER], [
  PHP_JSON_EXTRA_CONFIG_HEADERS="$PHP_JSON_EXTRA_CONFIG_HEADERS $1"
])

AC_DEFUN([PHP_JSON_ADD_CFLAG], [
  PHP_JSON_CFLAGS="$PHP_JSON_CFLAGS $1"
])

AC_DEFUN([PHP_JSON_EXTENSION], [
  PHP_NEW_EXTENSION(json, $PHP_JSON_SOURCES, $ext_shared,, $PHP_JSON_CFLAGS)
  PHP_SUBST(JSON_SHARED_LIBADD)

  for dir in $PHP_JSON_EXTRA_BUILD_DIRS; do
    PHP_ADD_BUILD_DIR([$ext_builddir/$dir], 1)
  done
  
  for dir in $PHP_JSON_EXTRA_INCLUDES; do
    PHP_ADD_INCLUDE([$ext_srcdir/$dir])
    PHP_ADD_INCLUDE([$ext_builddir/$dir])
  done

  if test "$ext_shared" = "no"; then
    PHP_ADD_SOURCES(PHP_EXT_DIR(json), $PHP_JSON_BASE_SOURCES,$PHP_JSON_CFLAGS)
    out="php_config.h"
  else
    PHP_ADD_SOURCES_X(PHP_EXT_DIR(json),$PHP_JSON_BASE_SOURCES,$PHP_JSON_CFLAGS,shared_objects_json,yes)
    if test -f "$ext_builddir/config.h.in"; then
      out="$abs_builddir/config.h"
    else
      out="php_config.h"
    fi
  fi
  
  for cfg in $PHP_JSON_EXTRA_CONFIG_HEADERS; do
    cat > $ext_builddir/$cfg <<EOF
#include "$out"
EOF
  done
])

AC_DEFUN([PHP_JSON_SETUP_JSON_C], [
  dnl json-c is required and can not be disabled
  dnl
  dnl Bundled json-c
  dnl

  PHP_JSON_ADD_BUILD_DIR([json_c])
  PHP_JSON_ADD_INCLUDE([json_c])
  PHP_JSON_ADD_CONFIG_HEADER([json_c/config.h])

  PHP_JSON_ADD_SOURCES([
    json_c/ConvertUTF.c
    json_c/debug.c
    json_c/linkhash.c
    json_c/printbuf.c
    json_c/arraylist.c
    json_c/json_object.c
    json_c/json_tokener.c
    json_c/ossupport.c
  ])
  PHP_JSON_ADD_CFLAG([-DHAVE_CONFIG_H])
])

dnl
dnl Main config
dnl

PHP_ARG_WITH(json, whether to enable JavaScript Object Serialization support,
[  --with-json       Enable JavaScript Object Serialization support])

if test "$PHP_JSON" != "no"; then  
  AC_DEFINE([HAVE_JSON],1,[whether to have JavaScript Object Serialization support])
  AC_HEADER_STDC
  AC_CHECK_FUNCS([strndup vsnprintf vasprintf strncasecmp])
  AC_CHECK_HEADERS([stdarg.h])

  PHP_JSON_ADD_BASE_SOURCES([json.c])

  dnl json_c is required
  PHP_JSON_SETUP_JSON_C
  PHP_JSON_EXTENSION
  dnl PHP_INSTALL_HEADERS([ext/json], [json_c])
fi

# vim600: sts=2 sw=2 et
