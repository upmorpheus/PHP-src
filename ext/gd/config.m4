shared=no
AC_MSG_CHECKING(whether to include GD support)
AC_ARG_WITH(gd,
[  --without-gd            Disable GD support.
  --with-gd[=DIR]         Include GD support (DIR is GD's install dir).
                          Set DIR to "shared" to build as a dl, or 
                          "shared,DIR" to build as a dl and still specify DIR.],
[
  PHP_WITH_SHARED

  case "$withval" in
    no)
      AC_MSG_RESULT(no) ;;
    yes)
      AC_DEFINE(HAVE_LIBGD)
      if test "$shared" = "yes"; then
        AC_MSG_RESULT(yes (shared))
        GD_LIBS="-lgd"
      else
        AC_MSG_RESULT(yes (static))
        AC_ADD_LIBRARY(gd)
      fi
        old_LDFLAGS=$LDFLAGS
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13) ])
		LIBS="$LIBS -lpng -lz"
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1)])
dnl Some versions of GD support both PNG and GIF. Check for both.
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1)])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1)])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(png)
          AC_ADD_LIBRARY(z)
        fi
        ac_cv_lib_gd_gdImageLine=yes
      ;;
    *)
dnl A whole whack of possible places where this might be
      test -f $withval/include/gd1.3/gd.h && GD_INCLUDE="$withval/include/gd1.3"
      test -f $withval/include/gd/gd.h && GD_INCLUDE="$withval/include/gd"
      test -f $withval/include/gd.h && GD_INCLUDE="$withval/include"
      test -f $withval/gd1.3/gd.h && GD_INCLUDE="$withval/gd1.3"
      test -f $withval/gd/gd.h && GD_INCLUDE="$withval/gd"
      test -f $withval/gd.h && GD_INCLUDE="$withval"

      test -f $withval/lib/libgd.so && GD_LIB="$withval/lib"
      test -f $withval/lib/gd/libgd.so && GD_LIB="$withval/lib/gd"
      test -f $withval/lib/gd1.3/libgd.so && GD_LIB="$withval/lib/gd1.3"
      test -f $withval/libgd.so && GD_LIB="$withval"
      test -f $withval/gd/libgd.so && GD_LIB="$withval/gd"
      test -f $withval/gd1.3/libgd.so && GD_LIB="$withval/gd1.3"

      test -f $withval/lib/libgd.a && GD_LIB="$withval/lib"
      test -f $withval/lib/gd/libgd.a && GD_LIB="$withval/lib/gd"
      test -f $withval/lib/gd1.3/libgd.a && GD_LIB="$withval/lib/gd1.3"
      test -f $withval/libgd.a && GD_LIB="$withval"
      test -f $withval/gd/libgd.a && GD_LIB="$withval/gd"
      test -f $withval/gd1.3/libgd.a && GD_LIB="$withval/gd1.3"
      if test -n "$GD_INCLUDE" && test -n "$GD_LIB" ; then
        AC_DEFINE(HAVE_LIBGD)
        if test "$shared" = "yes"; then
          AC_MSG_RESULT(yes (shared))
          GD_LIBS="-lgd"
          GD_LFLAGS="-L$GD_LIB"
        else
          AC_MSG_RESULT(yes (static))
          AC_ADD_LIBRARY_WITH_PATH(gd, $GD_LIB)
        fi
        old_LDFLAGS=$LDFLAGS
        LDFLAGS="$LDFLAGS -L$GD_LIB"
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13) ])
		LIBS="$LIBS -lpng -lz"
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1)])
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1)])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1)])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(png)
          AC_ADD_LIBRARY(z)
        fi
        ac_cv_lib_gd_gdImageLine=yes
      else
        AC_MSG_ERROR([Unable to find libgd.(a|so) anywhere under $withval])
      fi ;;
  esac
],[
  AC_CHECK_LIB(gd, gdImageLine)
  AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13) ])
  if test "$ac_cv_lib_gd_gdImageLine" = "yes"; then
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13) ])
		LIBS="$LIBS -lpng -lz"
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1)])
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1)])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1)])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(png)
          AC_ADD_LIBRARY(z)
        fi
        ac_cv_lib_gd_gdImageLine=yes
  fi
])
if test "$ac_cv_lib_gd_gdImageLine" = "yes"; then
  CHECK_TTF="yes"
  AC_ARG_WITH(ttf,
  [  --with-ttf[=DIR]        Include Freetype support],[
    if test $withval = "no" ; then
      CHECK_TTF=""
    else
      CHECK_TTF="$withval"
    fi
  ])

  AC_MSG_CHECKING(whether to include ttf support)
  if test -n "$CHECK_TTF" ; then
    for i in /usr /usr/local "$CHECK_TTF" ; do
      if test -f "$i/include/truetype.h" ; then
        FREETYPE_DIR="$i"
      fi
      if test -f "$i/include/freetype.h" ; then
        TTF_DIR="$i"
      fi
    done
    if test -n "$FREETYPE_DIR" ; then
      AC_DEFINE(HAVE_LIBFREETYPE)
      if test "$shared" = "yes"; then
        GD_LIBS="$GD_LIBS -lfreetype"
        GD_LFLAGS="$GD_LFLAGS -L$FREETYPE_DIR/lib"
      else 
        AC_ADD_LIBRARY_WITH_PATH(freetype, $FREETYPE_DIR/lib)
      fi
      AC_ADD_INCLUDE($FREETYPE_DIR/include)
      AC_MSG_RESULT(yes)
    else
      if test -n "$TTF_DIR" ; then
        AC_DEFINE(HAVE_LIBTTF)
        if test "$shared" = "yes"; then
          GD_LIBS="$GD_LIBS -lttf"
          GD_LFLAGS="$GD_LFLAGS -L$TTF_DIR/lib"
        else
          AC_ADD_LIBRARY_WITH_PATH(ttf, $TTF_DIR/lib)
        fi
        AC_ADD_INCLUDE($TTF_DIR/include)
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(no)
      fi
    fi
  else
    AC_MSG_RESULT(no)
  fi
  
  if test -f /usr/pkg/include/gd/gd.h -a -z "$GD_INCLUDE" ; then
    GD_INCLUDE="/usr/pkg/include/gd"
  fi

  AC_MSG_CHECKING(whether to enable 4bit antialias hack with FreeType2)
  AC_ARG_ENABLE(freetype-4bit-antialias-hack,
  [  --enable-freetype-4bit-antialias-hack  
                          Include support for FreeType2 (experimental).],[
  if test "$enableval" = "yes" ; then
    AC_DEFINE(FREETYPE_4BIT_ANTIALIAS_HACK, 1)
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
  ],[
    AC_MSG_RESULT(no)
  ])
  
  AC_EXPAND_PATH($GD_INCLUDE, GD_INCLUDE)
  AC_ADD_INCLUDE($GD_INCLUDE)
  PHP_EXTENSION(gd, $shared)
  if test "$shared" != "yes"; then
    GD_STATIC="libphpext_gd.la"
  else 
    GD_SHARED="gd.la"
  fi
fi

AC_SUBST(GD_LFLAGS)
AC_SUBST(GD_LIBS)
AC_SUBST(GD_STATIC)
AC_SUBST(GD_SHARED)
