# $Id$
# config.m4 for servlet sapi

AC_MSG_CHECKING(for Servlet support)
AC_ARG_WITH(servlet,
[  --with-servlet[=DIR]	  Include servlet support. DIR is the base install
			  directory for the JSDK.  This SAPI prereqs the
			  java extension must be built as a shared dl.],
[
  if test "$withval" != "no"; then

    if test "$withval" = "yes"; then
      SERVLET_CLASSPATH=.
    else
      if test -f $withval/lib/jsdk.jar; then
	SERVLET_CLASSPATH=$withval/lib/jsdk.jar
      else
	if test -d $withval/javax; then
	  SERVLET_CLASSPATH=$withval
	else
	  AC_MSG_RESULT(no)
	  AC_MSG_ERROR(unable to find JSDK libraries)
	fi
      fi
    fi

    AC_DEFINE(SAPI_SERVLET)
    enable_thread_safety=yes
    passthru="$passthru --enable-thread-safety"
    PHP_EXTENSION(servlet, "shared")
    PHP_SAPI=servlet
    PHP_BUILD_SHARED
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])

AC_SUBST(SERVLET_CLASSPATH)
