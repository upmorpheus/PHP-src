dnl $Id$
dnl config.m4 for extension pdo_odbc
dnl vim:et:sw=2:ts=2:

if test "$PHP_PDO" != "no"; then

define([PDO_ODBC_HELP_TEXT],[[
                            include and lib dirs are looked under 'dir'.
                            'flavour' can be one of:  ibm-db2, unixODBC, generic
                            If ',dir' part is omitted, default for the flavour 
                            you have selected will used. e.g.:
                              --with-pdo-odbc=unixODBC
                            will check for unixODBC under /usr/local. You may attempt 
                            to use an otherwise unsupported driver using the \"generic\" 
                            flavour.  The syntax for generic ODBC support is:
                              --with-pdo-odbc=generic,dir,libname,ldflags,cflags
                            This extension will always be created as a shared extension
                            named pdo_odbc.so]])

PHP_ARG_WITH(pdo-odbc, for ODBC v3 support for PDO,
[  --with-pdo-odbc=flavour,dir
                            PDO: Support for 'flavour' ODBC driver.]PDO_ODBC_HELP_TEXT)

AC_DEFUN([PDO_ODBC_CHECK_HEADERS],[
  php_pdo_have_header=no
  for i in odbc.h \
           odbcsdk.h \
           iodbc.h \
           sqlunix.h \
           sqltypes.h \
           sqlucode.h \
           sql.h \
           isql.h \
           sqlext.h \
           isqlext.h \
           udbcext.h \
           sqlcli1.h \
           LibraryManager.h \
           cli0core.h \
           cli0ext.h \
           cli0cli.h \
           cli0defs.h \
           cli0env.h
  do
    if test -f "$PDO_ODBC_INCDIR/$i"; then
      php_pdo_have_header=yes
      PHP_DEF_HAVE([$i])
    fi
  done
  if test "$php_pdo_have_header" = "no"; then
    AC_MSG_ERROR([Cannot find any header file(s)!])
  fi
])
                                  
if test "$PHP_PDO_ODBC" != "no"; then

  PHP_CHECK_PDO_INCLUDES

  AC_MSG_CHECKING([for selected PDO ODBC flavour])

  pdo_odbc_flavour="`echo $PHP_PDO_ODBC | cut -d, -f1`"
  pdo_odbc_dir="`echo $PHP_PDO_ODBC | cut -d, -f2`"

  if test "$pdo_odbc_dir" = "$PHP_PDO_ODBC" ; then
    pdo_odbc_dir=
  fi

  case $pdo_odbc_flavour in
    ibm-db2)
        pdo_odbc_def_libdir=/home/db2inst1/sqllib/lib
        pdo_odbc_def_incdir=/home/db2inst1/sqllib/include
        pdo_odbc_def_lib=db2
        ;;

    unixODBC|unixodbc)
        pdo_odbc_def_libdir=/usr/local/lib
        pdo_odbc_def_incdir=/usr/local/include
        pdo_odbc_def_lib=odbc
        ;;

    ODBCRouter|odbcrouter)
        pdo_odbc_def_libdir=/usr/lib
        pdo_odbc_def_incdir=/usr/include
        pdo_odbc_def_lib=odbcsdk
        ;;

    generic)
        pdo_odbc_def_lib="`echo $PHP_PDO_ODBC | cut -d, -f3`"
        pdo_odbc_def_ldflags="`echo $PHP_PDO_ODBC | cut -d, -f4`"
        pdo_odbc_def_cflags="`echo $PHP_PDO_ODBC | cut -d, -f5`"
        pdo_odbc_flavour="$pdo_odbc_flavour ($pdo_odbc_def_lib)"
        ;;

      *)
        AC_MSG_ERROR([Unknown ODBC flavour $pdo_odbc_flavour]PDO_ODBC_HELP_TEXT)
        ;;
  esac

  if test -n "$pdo_odbc_dir"; then
    PDO_ODBC_INCDIR="$pdo_odbc_dir/include"
    PDO_ODBC_LIBDIR="$pdo_odbc_dir/lib"
  else
    PDO_ODBC_INCDIR="$pdo_odbc_def_incdir"
    PDO_ODBC_LIBDIR="$pdo_odbc_def_libdir"
  fi
  
  AC_MSG_RESULT([$pdo_odbc_flavour
          libs       $PDO_ODBC_LIBDIR,
          headers    $PDO_ODBC_INCDIR])

  if ! test -d "$PDO_ODBC_LIBDIR" ; then
    AC_MSG_WARN([library dir $PDO_ODBC_LIBDIR does not exist])
  fi

  PDO_ODBC_CHECK_HEADERS

  PDO_ODBC_INCLUDE="$pdo_odbc_def_cflags -I$PDO_ODBC_INCDIR -DPDO_ODBC_TYPE=\\"$pdo_odbc_flavour\\""
  PDO_ODBC_LDFLAGS="$pdo_odbc_def_ldflags -L$PDO_ODBC_LIBDIR -l$pdo_odbc_def_lib"

  PHP_EVAL_LIBLINE([$PDO_ODBC_LDFLAGS], [PDO_ODBC_SHARED_LIBADD])

  dnl Check first for an ODBC 1.0 function to assert that the libraries work
  PHP_CHECK_LIBRARY($pdo_odbc_def_lib, SQLBindCol,
  [
    dnl And now check for an ODBC 3.0 function to assert that they're
    dnl *good* libraries.
    PHP_CHECK_LIBRARY($pdo_odbc_def_lib, SQLAllocHandle,
    [], [
      AC_MSG_ERROR([
Your ODBC library does not appear to be ODBC 3 compatible.
You should consider using unixODBC instead, and loading your
libraries as a driver in that environment; it will emulate the
functions required for PDO support.
])])
  ],[
    AC_MSG_ERROR([Your ODBC library does not exist or there was an error. Check config.log for more information])
  ])

  PHP_NEW_EXTENSION(pdo_odbc, pdo_odbc.c odbc_driver.c odbc_stmt.c, $ext_shared,,-I$pdo_inc_path $PDO_ODBC_INCLUDE)
  PHP_SUBST(PDO_ODBC_SHARED_LIBADD)
  PHP_ADD_EXTENSION_DEP(pdo_odbc, pdo)
fi

fi
