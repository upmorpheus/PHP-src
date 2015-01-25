/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Jason Greene <jason@php.net>                                 |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

/* This file is to be included by sockets.c */

REGISTER_LONG_CONSTANT("SOCKET_EINTR", WSAEINTR, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EBADF", WSAEBADF, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EACCES", WSAEACCES, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EFAULT", WSAEFAULT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EINVAL", WSAEINVAL, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EMFILE", WSAEMFILE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EWOULDBLOCK", WSAEWOULDBLOCK, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EINPROGRESS", WSAEINPROGRESS, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EALREADY", WSAEALREADY, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENOTSOCK", WSAENOTSOCK, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EDESTADDRREQ", WSAEDESTADDRREQ, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EMSGSIZE", WSAEMSGSIZE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EPROTOTYPE", WSAEPROTOTYPE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENOPROTOOPT", WSAENOPROTOOPT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EPROTONOSUPPORT", WSAEPROTONOSUPPORT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EOPNOTSUPP", WSAEOPNOTSUPP, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EPFNOSUPPORT", WSAEPFNOSUPPORT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EAFNOSUPPORT", WSAEAFNOSUPPORT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EADDRINUSE", WSAEADDRINUSE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EADDRNOTAVAIL", WSAEADDRNOTAVAIL, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENETDOWN", WSAENETDOWN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENETUNREACH", WSAENETUNREACH, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENETRESET", WSAENETRESET, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ECONNABORTED", WSAECONNABORTED, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ECONNRESET", WSAECONNRESET, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENOBUFS", WSAENOBUFS, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EISCONN", WSAEISCONN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENOTCONN", WSAENOTCONN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ESHUTDOWN", WSAESHUTDOWN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ETOOMANYREFS", WSAETOOMANYREFS, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ETIMEDOUT", WSAETIMEDOUT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ECONNREFUSED", WSAECONNREFUSED, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ELOOP", WSAELOOP, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENAMETOOLONG", WSAENAMETOOLONG, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EHOSTDOWN", WSAEHOSTDOWN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EHOSTUNREACH", WSAEHOSTUNREACH, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ENOTEMPTY", WSAENOTEMPTY, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EPROCLIM", WSAEPROCLIM, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EUSERS", WSAEUSERS, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EDQUOT", WSAEDQUOT, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_ESTALE", WSAESTALE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EREMOTE", WSAEREMOTE, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_EDISCON", WSAEDISCON, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_SYSNOTREADY", WSASYSNOTREADY, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_VERNOTSUPPORTED", WSAVERNOTSUPPORTED, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_NOTINITIALISED", WSANOTINITIALISED, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_HOST_NOT_FOUND", WSAHOST_NOT_FOUND, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_TRY_AGAIN", WSATRY_AGAIN, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_NO_RECOVERY", WSANO_RECOVERY, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_NO_DATA", WSANO_DATA, CONST_CS | CONST_PERSISTENT);
REGISTER_LONG_CONSTANT("SOCKET_NO_ADDRESS", WSANO_ADDRESS, CONST_CS | CONST_PERSISTENT);
