#! /bin/sh
#  +----------------------------------------------------------------------+
#  | PHP Version 5                                                        |
#  +----------------------------------------------------------------------+
#  | Copyright (c) 1997-2004 The PHP Group                                |
#  +----------------------------------------------------------------------+
#  | This source file is subject to version 3.0 of the PHP license,       |
#  | that is bundled with this package in the file LICENSE, and is        |
#  | available through the world-wide-web at the following url:           |
#  | http://www.php.net/license/3_0.txt.                                  |
#  | If you did not receive a copy of the PHP license and are unable to   |
#  | obtain it through the world-wide-web, please send a note to          |
#  | license@php.net so we can mail you a copy immediately.               |
#  +----------------------------------------------------------------------+
#  | Authors: Stig Bakken <ssb@php.net>                                   |
#  |          Sascha Schumann <sascha@schumann.cx>                        |
#  +----------------------------------------------------------------------+
#
# $Id: buildcheck.sh,v 1.34 2005-01-20 01:41:19 sniper Exp $ 
#

echo "buildconf: checking installation..."

stamp=$1

# autoconf 2.13 or newer
ac_version=`autoconf --version 2>/dev/null|head -n 1|sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$ac_version"; then
echo "buildconf: autoconf not found."
echo "           You need autoconf version 2.13 or newer installed"
echo "           to build PHP from CVS."
exit 1
fi
IFS=.; set $ac_version; IFS=' '
if test "$1" = "2" -a "$2" -lt "13" || test "$1" -lt "2"; then
echo "buildconf: autoconf version $ac_version found."
echo "           You need autoconf version 2.13 or newer installed"
echo "           to build PHP from CVS."
exit 1
else
echo "buildconf: autoconf version $ac_version (ok)"
fi

if test "$1" = "2" && test "$2" -ge "50"; then
  echo "buildconf: Your version of autoconf likely contains buggy cache code."
  echo "           Running cvsclean for you."
  echo "           To avoid this, install autoconf-2.13 and automake-1.5."
  ./cvsclean
  stamp=
fi

test -n "$stamp" && touch $stamp

exit 0
