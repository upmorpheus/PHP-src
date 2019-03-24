#!/bin/sh
#
#  +----------------------------------------------------------------------+
#  | PHP Version 7                                                        |
#  +----------------------------------------------------------------------+
#  | Copyright (c) The PHP Group                                          |
#  +----------------------------------------------------------------------+
#  | This source file is subject to version 3.01 of the PHP license,      |
#  | that is bundled with this package in the file LICENSE, and is        |
#  | available through the world-wide-web at the following url:           |
#  | http://www.php.net/license/3_01.txt                                  |
#  | If you did not receive a copy of the PHP license and are unable to   |
#  | obtain it through the world-wide-web, please send a note to          |
#  | license@php.net so we can mail you a copy immediately.               |
#  +----------------------------------------------------------------------+
#  | Authors: Stig Bakken <ssb@php.net>                                   |
#  |          Sascha Schumann <sascha@schumann.cx>                        |
#  +----------------------------------------------------------------------+
#
# Check PHP build system tools such as autoconf and their versions.
#
# SYNOPSIS:
#   buildcheck.sh [stampfile]
#
# DESCRIPTION:
#   Optional stampfile is for Makefile to check build system only once.
#
# ENVIRONMENT:
#   The following optional variables are supported:
#
#   PHP_AUTOCONF    Overrides the path to autoconf tool.
#                   PHP_AUTOCONF=/path/to/autoconf buildcheck.sh

echo "buildconf: checking installation..."

stamp=$1

# Allow the autoconf executable to be overridden by $PHP_AUTOCONF.
PHP_AUTOCONF=${PHP_AUTOCONF:-autoconf}

# Go to project root.
cd $(CDPATH= cd -- "$(dirname -- "$0")/../" && pwd -P)

# Get minimum required autoconf version from the configure.ac file.
min_version=$(sed -n 's/AC_PREREQ(\[\(.*\)\])/\1/p' configure.ac)

# Check if autoconf exists.
ac_version=$($PHP_AUTOCONF --version 2>/dev/null|head -n 1|sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//')

if test -z "$ac_version"; then
  echo "buildconf: autoconf not found." >&2
  echo "           You need autoconf version $min_version or newer installed" >&2
  echo "           to build PHP from Git." >&2
  exit 1
fi

# Check autoconf version.
set -f; IFS='.'; set -- $ac_version; set +f; IFS=' '
ac_version_num="$(expr ${1} \* 10000 + ${2} \* 100)"
set -f; IFS='.'; set -- $min_version; set +f; IFS=' '
min_version_num="$(expr ${1} \* 10000 + ${2} \* 100)"

if test "$ac_version_num" -lt "$min_version_num"; then
  echo "buildconf: autoconf version $ac_version found." >&2
  echo "           You need autoconf version $min_version or newer installed" >&2
  echo "           to build PHP from Git." >&2
  exit 1
else
  echo "buildconf: autoconf version $ac_version (ok)"
fi

test -n "$stamp" && touch $stamp
