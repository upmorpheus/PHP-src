dnl $Id$
dnl config.m4 for extension pdo

PHP_ARG_ENABLE(pdo, whether to disable PDO support,
[  --disable-pdo            Disable PHP Data Objects support], yes)

if test "$PHP_PDO" != "no"; then
  PHP_NEW_EXTENSION(pdo, pdo.c pdo_dbh.c pdo_stmt.c pdo_sql_parser.c pdo_sqlstate.c, $ext_shared)
  PHP_ADD_MAKEFILE_FRAGMENT
fi
