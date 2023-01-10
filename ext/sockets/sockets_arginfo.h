/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: add91c303eddf7518566bc7e6c1698d7198c0d4c */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_select, 0, 4, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(1, read, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(1, write, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(1, except, IS_ARRAY, 1)
	ZEND_ARG_TYPE_INFO(0, seconds, IS_LONG, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, microseconds, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_create_listen, 0, 1, Socket, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, backlog, IS_LONG, 0, "128")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_accept, 0, 1, Socket, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_set_nonblock, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
ZEND_END_ARG_INFO()

#define arginfo_socket_set_block arginfo_socket_set_nonblock

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_listen, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, backlog, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_close, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_write, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, length, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_read, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "PHP_BINARY_READ")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_getsockname, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_INFO(1, address)
	ZEND_ARG_INFO_WITH_DEFAULT_VALUE(1, port, "null")
ZEND_END_ARG_INFO()

#define arginfo_socket_getpeername arginfo_socket_getsockname

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_create, 0, 3, Socket, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, domain, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, protocol, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_connect, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_strerror, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, error_code, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_bind, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_recv, 0, 4, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_INFO(1, data)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_send, 0, 4, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_recvfrom, 0, 5, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_INFO(1, data)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
	ZEND_ARG_INFO(1, address)
	ZEND_ARG_INFO_WITH_DEFAULT_VALUE(1, port, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_sendto, 0, 5, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, address, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_get_option, 0, 3, MAY_BE_ARRAY|MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, level, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_socket_getopt arginfo_socket_get_option

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_set_option, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, level, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_socket_setopt arginfo_socket_set_option

#if defined(HAVE_SOCKETPAIR)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_create_pair, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, domain, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, protocol, IS_LONG, 0)
	ZEND_ARG_INFO(1, pair)
ZEND_END_ARG_INFO()
#endif

#if defined(HAVE_SHUTDOWN)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_shutdown, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "2")
ZEND_END_ARG_INFO()
#endif

#if defined(HAVE_SOCKATMARK)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_atmark, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_last_error, 0, 0, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, socket, Socket, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_clear_error, 0, 0, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO_WITH_DEFAULT_VALUE(0, socket, Socket, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_import_stream, 0, 1, Socket, MAY_BE_FALSE)
	ZEND_ARG_INFO(0, stream)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_socket_export_stream, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_sendmsg, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_recvmsg, 0, 2, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(1, message, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_cmsg_space, 0, 2, IS_LONG, 1)
	ZEND_ARG_TYPE_INFO(0, level, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, num, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_addrinfo_lookup, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, service, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, hints, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_addrinfo_connect, 0, 1, Socket, MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, address, AddressInfo, 0)
ZEND_END_ARG_INFO()

#define arginfo_socket_addrinfo_bind arginfo_socket_addrinfo_connect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_addrinfo_explain, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_OBJ_INFO(0, address, AddressInfo, 0)
ZEND_END_ARG_INFO()

#if defined(PHP_WIN32)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_socket_wsaprotocol_info_export, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
	ZEND_ARG_OBJ_INFO(0, socket, Socket, 0)
	ZEND_ARG_TYPE_INFO(0, process_id, IS_LONG, 0)
ZEND_END_ARG_INFO()
#endif

#if defined(PHP_WIN32)
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_socket_wsaprotocol_info_import, 0, 1, Socket, MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, info_id, IS_STRING, 0)
ZEND_END_ARG_INFO()
#endif

#if defined(PHP_WIN32)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_socket_wsaprotocol_info_release, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, info_id, IS_STRING, 0)
ZEND_END_ARG_INFO()
#endif


ZEND_FUNCTION(socket_select);
ZEND_FUNCTION(socket_create_listen);
ZEND_FUNCTION(socket_accept);
ZEND_FUNCTION(socket_set_nonblock);
ZEND_FUNCTION(socket_set_block);
ZEND_FUNCTION(socket_listen);
ZEND_FUNCTION(socket_close);
ZEND_FUNCTION(socket_write);
ZEND_FUNCTION(socket_read);
ZEND_FUNCTION(socket_getsockname);
ZEND_FUNCTION(socket_getpeername);
ZEND_FUNCTION(socket_create);
ZEND_FUNCTION(socket_connect);
ZEND_FUNCTION(socket_strerror);
ZEND_FUNCTION(socket_bind);
ZEND_FUNCTION(socket_recv);
ZEND_FUNCTION(socket_send);
ZEND_FUNCTION(socket_recvfrom);
ZEND_FUNCTION(socket_sendto);
ZEND_FUNCTION(socket_get_option);
ZEND_FUNCTION(socket_set_option);
#if defined(HAVE_SOCKETPAIR)
ZEND_FUNCTION(socket_create_pair);
#endif
#if defined(HAVE_SHUTDOWN)
ZEND_FUNCTION(socket_shutdown);
#endif
#if defined(HAVE_SOCKATMARK)
ZEND_FUNCTION(socket_atmark);
#endif
ZEND_FUNCTION(socket_last_error);
ZEND_FUNCTION(socket_clear_error);
ZEND_FUNCTION(socket_import_stream);
ZEND_FUNCTION(socket_export_stream);
ZEND_FUNCTION(socket_sendmsg);
ZEND_FUNCTION(socket_recvmsg);
ZEND_FUNCTION(socket_cmsg_space);
ZEND_FUNCTION(socket_addrinfo_lookup);
ZEND_FUNCTION(socket_addrinfo_connect);
ZEND_FUNCTION(socket_addrinfo_bind);
ZEND_FUNCTION(socket_addrinfo_explain);
#if defined(PHP_WIN32)
ZEND_FUNCTION(socket_wsaprotocol_info_export);
#endif
#if defined(PHP_WIN32)
ZEND_FUNCTION(socket_wsaprotocol_info_import);
#endif
#if defined(PHP_WIN32)
ZEND_FUNCTION(socket_wsaprotocol_info_release);
#endif


static const zend_function_entry ext_functions[] = {
	ZEND_FE(socket_select, arginfo_socket_select)
	ZEND_FE(socket_create_listen, arginfo_socket_create_listen)
	ZEND_FE(socket_accept, arginfo_socket_accept)
	ZEND_FE(socket_set_nonblock, arginfo_socket_set_nonblock)
	ZEND_FE(socket_set_block, arginfo_socket_set_block)
	ZEND_FE(socket_listen, arginfo_socket_listen)
	ZEND_FE(socket_close, arginfo_socket_close)
	ZEND_FE(socket_write, arginfo_socket_write)
	ZEND_FE(socket_read, arginfo_socket_read)
	ZEND_FE(socket_getsockname, arginfo_socket_getsockname)
	ZEND_FE(socket_getpeername, arginfo_socket_getpeername)
	ZEND_FE(socket_create, arginfo_socket_create)
	ZEND_FE(socket_connect, arginfo_socket_connect)
	ZEND_FE(socket_strerror, arginfo_socket_strerror)
	ZEND_FE(socket_bind, arginfo_socket_bind)
	ZEND_FE(socket_recv, arginfo_socket_recv)
	ZEND_FE(socket_send, arginfo_socket_send)
	ZEND_FE(socket_recvfrom, arginfo_socket_recvfrom)
	ZEND_FE(socket_sendto, arginfo_socket_sendto)
	ZEND_FE(socket_get_option, arginfo_socket_get_option)
	ZEND_FALIAS(socket_getopt, socket_get_option, arginfo_socket_getopt)
	ZEND_FE(socket_set_option, arginfo_socket_set_option)
	ZEND_FALIAS(socket_setopt, socket_set_option, arginfo_socket_setopt)
#if defined(HAVE_SOCKETPAIR)
	ZEND_FE(socket_create_pair, arginfo_socket_create_pair)
#endif
#if defined(HAVE_SHUTDOWN)
	ZEND_FE(socket_shutdown, arginfo_socket_shutdown)
#endif
#if defined(HAVE_SOCKATMARK)
	ZEND_FE(socket_atmark, arginfo_socket_atmark)
#endif
	ZEND_FE(socket_last_error, arginfo_socket_last_error)
	ZEND_FE(socket_clear_error, arginfo_socket_clear_error)
	ZEND_FE(socket_import_stream, arginfo_socket_import_stream)
	ZEND_FE(socket_export_stream, arginfo_socket_export_stream)
	ZEND_FE(socket_sendmsg, arginfo_socket_sendmsg)
	ZEND_FE(socket_recvmsg, arginfo_socket_recvmsg)
	ZEND_FE(socket_cmsg_space, arginfo_socket_cmsg_space)
	ZEND_FE(socket_addrinfo_lookup, arginfo_socket_addrinfo_lookup)
	ZEND_FE(socket_addrinfo_connect, arginfo_socket_addrinfo_connect)
	ZEND_FE(socket_addrinfo_bind, arginfo_socket_addrinfo_bind)
	ZEND_FE(socket_addrinfo_explain, arginfo_socket_addrinfo_explain)
#if defined(PHP_WIN32)
	ZEND_FE(socket_wsaprotocol_info_export, arginfo_socket_wsaprotocol_info_export)
#endif
#if defined(PHP_WIN32)
	ZEND_FE(socket_wsaprotocol_info_import, arginfo_socket_wsaprotocol_info_import)
#endif
#if defined(PHP_WIN32)
	ZEND_FE(socket_wsaprotocol_info_release, arginfo_socket_wsaprotocol_info_release)
#endif
	ZEND_FE_END
};


static const zend_function_entry class_Socket_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_AddressInfo_methods[] = {
	ZEND_FE_END
};

static void register_sockets_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("AF_UNIX", AF_UNIX, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AF_INET", AF_INET, CONST_PERSISTENT);
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("AF_INET6", AF_INET6, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SOCK_STREAM", SOCK_STREAM, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOCK_DGRAM", SOCK_DGRAM, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOCK_RAW", SOCK_RAW, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOCK_SEQPACKET", SOCK_SEQPACKET, CONST_PERSISTENT);
#if defined(SOCK_RDM)
	REGISTER_LONG_CONSTANT("SOCK_RDM", SOCK_RDM, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("MSG_OOB", MSG_OOB, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSG_WAITALL", MSG_WAITALL, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSG_CTRUNC", MSG_CTRUNC, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSG_TRUNC", MSG_TRUNC, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSG_PEEK", MSG_PEEK, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MSG_DONTROUTE", MSG_DONTROUTE, CONST_PERSISTENT);
#if defined(MSG_EOR)
	REGISTER_LONG_CONSTANT("MSG_EOR", MSG_EOR, CONST_PERSISTENT);
#endif
#if defined(MSG_EOF)
	REGISTER_LONG_CONSTANT("MSG_EOF", MSG_EOF, CONST_PERSISTENT);
#endif
#if defined(MSG_CONFIRM)
	REGISTER_LONG_CONSTANT("MSG_CONFIRM", MSG_CONFIRM, CONST_PERSISTENT);
#endif
#if defined(MSG_ERRQUEUE)
	REGISTER_LONG_CONSTANT("MSG_ERRQUEUE", MSG_ERRQUEUE, CONST_PERSISTENT);
#endif
#if defined(MSG_NOSIGNAL)
	REGISTER_LONG_CONSTANT("MSG_NOSIGNAL", MSG_NOSIGNAL, CONST_PERSISTENT);
#endif
#if defined(MSG_DONTWAIT)
	REGISTER_LONG_CONSTANT("MSG_DONTWAIT", MSG_DONTWAIT, CONST_PERSISTENT);
#endif
#if defined(MSG_MORE)
	REGISTER_LONG_CONSTANT("MSG_MORE", MSG_MORE, CONST_PERSISTENT);
#endif
#if defined(MSG_WAITFORONE)
	REGISTER_LONG_CONSTANT("MSG_WAITFORONE", MSG_WAITFORONE, CONST_PERSISTENT);
#endif
#if defined(MSG_CMSG_CLOEXEC)
	REGISTER_LONG_CONSTANT("MSG_CMSG_CLOEXEC", MSG_CMSG_CLOEXEC, CONST_PERSISTENT);
#endif
#if defined(MSG_ZEROCOPY)
	REGISTER_LONG_CONSTANT("MSG_ZEROCOPY", MSG_ZEROCOPY, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SO_DEBUG", SO_DEBUG, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_REUSEADDR", SO_REUSEADDR, CONST_PERSISTENT);
#if defined(SO_REUSEPORT)
	REGISTER_LONG_CONSTANT("SO_REUSEPORT", SO_REUSEPORT, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SO_KEEPALIVE", SO_KEEPALIVE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_DONTROUTE", SO_DONTROUTE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_LINGER", SO_LINGER, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_BROADCAST", SO_BROADCAST, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_OOBINLINE", SO_OOBINLINE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_SNDBUF", SO_SNDBUF, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_RCVBUF", SO_RCVBUF, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_SNDLOWAT", SO_SNDLOWAT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_RCVLOWAT", SO_RCVLOWAT, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_SNDTIMEO", SO_SNDTIMEO, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_RCVTIMEO", SO_RCVTIMEO, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SO_TYPE", SO_TYPE, CONST_PERSISTENT);
#if defined(SO_FAMILY)
	REGISTER_LONG_CONSTANT("SO_FAMILY", SO_FAMILY, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SO_ERROR", SO_ERROR, CONST_PERSISTENT);
#if defined(SO_BINDTODEVICE)
	REGISTER_LONG_CONSTANT("SO_BINDTODEVICE", SO_BINDTODEVICE, CONST_PERSISTENT);
#endif
#if defined(SO_USER_COOKIE)
	REGISTER_LONG_CONSTANT("SO_LABEL", SO_LABEL, CONST_PERSISTENT);
#endif
#if defined(SO_USER_COOKIE)
	REGISTER_LONG_CONSTANT("SO_PEERLABEL", SO_PEERLABEL, CONST_PERSISTENT);
#endif
#if defined(SO_USER_COOKIE)
	REGISTER_LONG_CONSTANT("SO_LISTENQLIMIT", SO_LISTENQLIMIT, CONST_PERSISTENT);
#endif
#if defined(SO_USER_COOKIE)
	REGISTER_LONG_CONSTANT("SO_LISTENQLEN", SO_LISTENQLEN, CONST_PERSISTENT);
#endif
#if defined(SO_USER_COOKIE)
	REGISTER_LONG_CONSTANT("SO_USER_COOKIE", SO_USER_COOKIE, CONST_PERSISTENT);
#endif
#if defined(SO_SETFIB)
	REGISTER_LONG_CONSTANT("SO_SETFIB", SO_SETFIB, CONST_PERSISTENT);
#endif
#if defined(SO_ACCEPTFILTER)
	REGISTER_LONG_CONSTANT("SO_ACCEPTFILTER", SO_ACCEPTFILTER, CONST_PERSISTENT);
#endif
#if defined(SOL_FILTER)
	REGISTER_LONG_CONSTANT("SOL_FILTER", SOL_FILTER, CONST_PERSISTENT);
#endif
#if defined(SOL_FILTER)
	REGISTER_LONG_CONSTANT("FIL_ATTACH", FIL_ATTACH, CONST_PERSISTENT);
#endif
#if defined(SOL_FILTER)
	REGISTER_LONG_CONSTANT("FIL_DETACH", FIL_DETACH, CONST_PERSISTENT);
#endif
#if defined(SO_DONTTRUNC)
	REGISTER_LONG_CONSTANT("SO_DONTTRUNC", SO_DONTTRUNC, CONST_PERSISTENT);
#endif
#if defined(SO_WANTMORE)
	REGISTER_LONG_CONSTANT("SO_WANTMORE", SO_WANTMORE, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SOL_SOCKET", SOL_SOCKET, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOMAXCONN", SOMAXCONN, CONST_PERSISTENT);
#if defined(SO_MARK)
	REGISTER_LONG_CONSTANT("SO_MARK", SO_MARK, CONST_PERSISTENT);
#endif
#if defined(SO_RTABLE)
	REGISTER_LONG_CONSTANT("SO_RTABLE", SO_RTABLE, CONST_PERSISTENT);
#endif
#if defined(SO_INCOMING_CPU)
	REGISTER_LONG_CONSTANT("SO_INCOMING_CPU", SO_INCOMING_CPU, CONST_PERSISTENT);
#endif
#if defined(SO_MEMINFO)
	REGISTER_LONG_CONSTANT("SO_MEMINFO", SO_MEMINFO, CONST_PERSISTENT);
#endif
#if defined(SO_BPF_EXTENSIONS)
	REGISTER_LONG_CONSTANT("SO_BPF_EXTENSIONS", SO_BPF_EXTENSIONS, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_OFF)
	REGISTER_LONG_CONSTANT("SKF_AD_OFF", SKF_AD_OFF, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_PROTOCOL)
	REGISTER_LONG_CONSTANT("SKF_AD_PROTOCOL", SKF_AD_PROTOCOL, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_PKTTYPE)
	REGISTER_LONG_CONSTANT("SKF_AD_PKTTYPE", SKF_AD_PKTTYPE, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_IFINDEX)
	REGISTER_LONG_CONSTANT("SKF_AD_IFINDEX", SKF_AD_IFINDEX, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_NLATTR)
	REGISTER_LONG_CONSTANT("SKF_AD_NLATTR", SKF_AD_NLATTR, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_NLATTR_NEST)
	REGISTER_LONG_CONSTANT("SKF_AD_NLATTR_NEST", SKF_AD_NLATTR_NEST, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_MARK)
	REGISTER_LONG_CONSTANT("SKF_AD_MARK", SKF_AD_MARK, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_QUEUE)
	REGISTER_LONG_CONSTANT("SKF_AD_QUEUE", SKF_AD_QUEUE, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_HATYPE)
	REGISTER_LONG_CONSTANT("SKF_AD_HATYPE", SKF_AD_HATYPE, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_RXHASH)
	REGISTER_LONG_CONSTANT("SKF_AD_RXHASH", SKF_AD_RXHASH, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_CPU)
	REGISTER_LONG_CONSTANT("SKF_AD_CPU", SKF_AD_CPU, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_ALU_XOR_X)
	REGISTER_LONG_CONSTANT("SKF_AD_ALU_XOR_X", SKF_AD_ALU_XOR_X, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_VLAN_TAG)
	REGISTER_LONG_CONSTANT("SKF_AD_VLAN_TAG", SKF_AD_VLAN_TAG, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_VLAN_TAG_PRESENT)
	REGISTER_LONG_CONSTANT("SKF_AD_VLAN_TAG_PRESENT", SKF_AD_VLAN_TAG_PRESENT, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_PAY_OFFSET)
	REGISTER_LONG_CONSTANT("SKF_AD_PAY_OFFSET", SKF_AD_PAY_OFFSET, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_RANDOM)
	REGISTER_LONG_CONSTANT("SKF_AD_RANDOM", SKF_AD_RANDOM, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_VLAN_TPID)
	REGISTER_LONG_CONSTANT("SKF_AD_VLAN_TPID", SKF_AD_VLAN_TPID, CONST_PERSISTENT);
#endif
#if defined(SKF_AD_MAX)
	REGISTER_LONG_CONSTANT("SKF_AD_MAX", SKF_AD_MAX, CONST_PERSISTENT);
#endif
#if defined(TCP_CONGESTION)
	REGISTER_LONG_CONSTANT("TCP_CONGESTION", TCP_CONGESTION, CONST_PERSISTENT);
#endif
#if defined(SO_ZEROCOPY)
	REGISTER_LONG_CONSTANT("SO_ZEROCOPY", SO_ZEROCOPY, CONST_PERSISTENT);
#endif
#if defined(TCP_NODELAY)
	REGISTER_LONG_CONSTANT("TCP_NODELAY", TCP_NODELAY, CONST_PERSISTENT);
#endif
#if defined(TCP_NOTSENT_LOWAT)
	REGISTER_LONG_CONSTANT("TCP_NOTSENT_LOWAT", TCP_NOTSENT_LOWAT, CONST_PERSISTENT);
#endif
#if defined(TCP_DEFER_ACCEPT)
	REGISTER_LONG_CONSTANT("TCP_DEFER_ACCEPT", TCP_DEFER_ACCEPT, CONST_PERSISTENT);
#endif
#if defined(TCP_KEEPALIVE)
	REGISTER_LONG_CONSTANT("TCP_KEEPALIVE", TCP_KEEPALIVE, CONST_PERSISTENT);
#endif
#if defined(TCP_KEEPIDLE)
	REGISTER_LONG_CONSTANT("TCP_KEEPIDLE", TCP_KEEPIDLE, CONST_PERSISTENT);
#endif
#if defined(TCP_KEEPIDLE)
	REGISTER_LONG_CONSTANT("TCP_KEEPINTVL", TCP_KEEPINTVL, CONST_PERSISTENT);
#endif
#if defined(TCP_KEEPIDLE)
	REGISTER_LONG_CONSTANT("TCP_KEEPCNT", TCP_KEEPCNT, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("PHP_NORMAL_READ", PHP_NORMAL_READ, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_BINARY_READ", PHP_BINARY_READ, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MCAST_JOIN_GROUP", PHP_MCAST_JOIN_GROUP, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MCAST_LEAVE_GROUP", PHP_MCAST_LEAVE_GROUP, CONST_PERSISTENT);
#if defined(HAS_MCAST_EXT)
	REGISTER_LONG_CONSTANT("MCAST_BLOCK_SOURCE", PHP_MCAST_BLOCK_SOURCE, CONST_PERSISTENT);
#endif
#if defined(HAS_MCAST_EXT)
	REGISTER_LONG_CONSTANT("MCAST_UNBLOCK_SOURCE", PHP_MCAST_UNBLOCK_SOURCE, CONST_PERSISTENT);
#endif
#if defined(HAS_MCAST_EXT)
	REGISTER_LONG_CONSTANT("MCAST_JOIN_SOURCE_GROUP", PHP_MCAST_JOIN_SOURCE_GROUP, CONST_PERSISTENT);
#endif
#if defined(HAS_MCAST_EXT)
	REGISTER_LONG_CONSTANT("MCAST_LEAVE_SOURCE_GROUP", PHP_MCAST_LEAVE_SOURCE_GROUP, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("IP_MULTICAST_IF", IP_MULTICAST_IF, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IP_MULTICAST_TTL", IP_MULTICAST_TTL, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IP_MULTICAST_LOOP", IP_MULTICAST_LOOP, CONST_PERSISTENT);
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("IPV6_MULTICAST_IF", IPV6_MULTICAST_IF, CONST_PERSISTENT);
#endif
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("IPV6_MULTICAST_HOPS", IPV6_MULTICAST_HOPS, CONST_PERSISTENT);
#endif
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("IPV6_MULTICAST_LOOP", IPV6_MULTICAST_LOOP, CONST_PERSISTENT);
#endif
#if defined(IPV6_V6ONLY)
	REGISTER_LONG_CONSTANT("IPV6_V6ONLY", IPV6_V6ONLY, CONST_PERSISTENT);
#endif
#if defined(EPERM)
	REGISTER_LONG_CONSTANT("SOCKET_EPERM", EPERM, CONST_PERSISTENT);
#endif
#if defined(ENOENT)
	REGISTER_LONG_CONSTANT("SOCKET_ENOENT", ENOENT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EINTR)
	REGISTER_LONG_CONSTANT("SOCKET_EINTR", PHP_SOCKET_EINTR, CONST_PERSISTENT);
#endif
#if defined(EIO)
	REGISTER_LONG_CONSTANT("SOCKET_EIO", EIO, CONST_PERSISTENT);
#endif
#if defined(ENXIO)
	REGISTER_LONG_CONSTANT("SOCKET_ENXIO", ENXIO, CONST_PERSISTENT);
#endif
#if defined(E2BIG)
	REGISTER_LONG_CONSTANT("SOCKET_E2BIG", E2BIG, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EBADF)
	REGISTER_LONG_CONSTANT("SOCKET_EBADF", PHP_SOCKET_EBADF, CONST_PERSISTENT);
#endif
#if defined(EAGAIN)
	REGISTER_LONG_CONSTANT("SOCKET_EAGAIN", EAGAIN, CONST_PERSISTENT);
#endif
#if defined(ENOMEM)
	REGISTER_LONG_CONSTANT("SOCKET_ENOMEM", ENOMEM, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EACCES)
	REGISTER_LONG_CONSTANT("SOCKET_EACCES", PHP_SOCKET_EACCES, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EFAULT)
	REGISTER_LONG_CONSTANT("SOCKET_EFAULT", PHP_SOCKET_EFAULT, CONST_PERSISTENT);
#endif
#if defined(ENOTBLK)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTBLK", ENOTBLK, CONST_PERSISTENT);
#endif
#if defined(EBUSY)
	REGISTER_LONG_CONSTANT("SOCKET_EBUSY", EBUSY, CONST_PERSISTENT);
#endif
#if defined(EEXIST)
	REGISTER_LONG_CONSTANT("SOCKET_EEXIST", EEXIST, CONST_PERSISTENT);
#endif
#if defined(EXDEV)
	REGISTER_LONG_CONSTANT("SOCKET_EXDEV", EXDEV, CONST_PERSISTENT);
#endif
#if defined(ENODEV)
	REGISTER_LONG_CONSTANT("SOCKET_ENODEV", ENODEV, CONST_PERSISTENT);
#endif
#if defined(ENOTDIR)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTDIR", ENOTDIR, CONST_PERSISTENT);
#endif
#if defined(EISDIR)
	REGISTER_LONG_CONSTANT("SOCKET_EISDIR", EISDIR, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EINVAL)
	REGISTER_LONG_CONSTANT("SOCKET_EINVAL", PHP_SOCKET_EINVAL, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENFILE)
	REGISTER_LONG_CONSTANT("SOCKET_ENFILE", PHP_SOCKET_ENFILE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EMFILE)
	REGISTER_LONG_CONSTANT("SOCKET_EMFILE", PHP_SOCKET_EMFILE, CONST_PERSISTENT);
#endif
#if defined(ENOTTY)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTTY", ENOTTY, CONST_PERSISTENT);
#endif
#if defined(ENOSPC)
	REGISTER_LONG_CONSTANT("SOCKET_ENOSPC", ENOSPC, CONST_PERSISTENT);
#endif
#if defined(ESPIPE)
	REGISTER_LONG_CONSTANT("SOCKET_ESPIPE", ESPIPE, CONST_PERSISTENT);
#endif
#if defined(EROFS)
	REGISTER_LONG_CONSTANT("SOCKET_EROFS", EROFS, CONST_PERSISTENT);
#endif
#if defined(EMLINK)
	REGISTER_LONG_CONSTANT("SOCKET_EMLINK", EMLINK, CONST_PERSISTENT);
#endif
#if defined(EPIPE)
	REGISTER_LONG_CONSTANT("SOCKET_EPIPE", EPIPE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENAMETOOLONG)
	REGISTER_LONG_CONSTANT("SOCKET_ENAMETOOLONG", PHP_SOCKET_ENAMETOOLONG, CONST_PERSISTENT);
#endif
#if defined(ENOLCK)
	REGISTER_LONG_CONSTANT("SOCKET_ENOLCK", ENOLCK, CONST_PERSISTENT);
#endif
#if defined(ENOSYS)
	REGISTER_LONG_CONSTANT("SOCKET_ENOSYS", ENOSYS, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENOTEMPTY)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTEMPTY", PHP_SOCKET_ENOTEMPTY, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ELOOP)
	REGISTER_LONG_CONSTANT("SOCKET_ELOOP", PHP_SOCKET_ELOOP, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EWOULDBLOCK)
	REGISTER_LONG_CONSTANT("SOCKET_EWOULDBLOCK", PHP_SOCKET_EWOULDBLOCK, CONST_PERSISTENT);
#endif
#if defined(ENOMSG)
	REGISTER_LONG_CONSTANT("SOCKET_ENOMSG", ENOMSG, CONST_PERSISTENT);
#endif
#if defined(EIDRM)
	REGISTER_LONG_CONSTANT("SOCKET_EIDRM", EIDRM, CONST_PERSISTENT);
#endif
#if defined(ECHRNG)
	REGISTER_LONG_CONSTANT("SOCKET_ECHRNG", ECHRNG, CONST_PERSISTENT);
#endif
#if defined(EL2NSYNC)
	REGISTER_LONG_CONSTANT("SOCKET_EL2NSYNC", EL2NSYNC, CONST_PERSISTENT);
#endif
#if defined(EL3HLT)
	REGISTER_LONG_CONSTANT("SOCKET_EL3HLT", EL3HLT, CONST_PERSISTENT);
#endif
#if defined(EL3RST)
	REGISTER_LONG_CONSTANT("SOCKET_EL3RST", EL3RST, CONST_PERSISTENT);
#endif
#if defined(ELNRNG)
	REGISTER_LONG_CONSTANT("SOCKET_ELNRNG", ELNRNG, CONST_PERSISTENT);
#endif
#if defined(EUNATCH)
	REGISTER_LONG_CONSTANT("SOCKET_EUNATCH", EUNATCH, CONST_PERSISTENT);
#endif
#if defined(ENOCSI)
	REGISTER_LONG_CONSTANT("SOCKET_ENOCSI", ENOCSI, CONST_PERSISTENT);
#endif
#if defined(EL2HLT)
	REGISTER_LONG_CONSTANT("SOCKET_EL2HLT", EL2HLT, CONST_PERSISTENT);
#endif
#if defined(EBADE)
	REGISTER_LONG_CONSTANT("SOCKET_EBADE", EBADE, CONST_PERSISTENT);
#endif
#if defined(EBADR)
	REGISTER_LONG_CONSTANT("SOCKET_EBADR", EBADR, CONST_PERSISTENT);
#endif
#if defined(EXFULL)
	REGISTER_LONG_CONSTANT("SOCKET_EXFULL", EXFULL, CONST_PERSISTENT);
#endif
#if defined(ENOANO)
	REGISTER_LONG_CONSTANT("SOCKET_ENOANO", ENOANO, CONST_PERSISTENT);
#endif
#if defined(EBADRQC)
	REGISTER_LONG_CONSTANT("SOCKET_EBADRQC", EBADRQC, CONST_PERSISTENT);
#endif
#if defined(EBADSLT)
	REGISTER_LONG_CONSTANT("SOCKET_EBADSLT", EBADSLT, CONST_PERSISTENT);
#endif
#if defined(ENOSTR)
	REGISTER_LONG_CONSTANT("SOCKET_ENOSTR", ENOSTR, CONST_PERSISTENT);
#endif
#if defined(ENODATA)
	REGISTER_LONG_CONSTANT("SOCKET_ENODATA", ENODATA, CONST_PERSISTENT);
#endif
#if defined(ETIME)
	REGISTER_LONG_CONSTANT("SOCKET_ETIME", ETIME, CONST_PERSISTENT);
#endif
#if defined(ENOSR)
	REGISTER_LONG_CONSTANT("SOCKET_ENOSR", ENOSR, CONST_PERSISTENT);
#endif
#if defined(ENONET)
	REGISTER_LONG_CONSTANT("SOCKET_ENONET", ENONET, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EREMOTE)
	REGISTER_LONG_CONSTANT("SOCKET_EREMOTE", PHP_SOCKET_EREMOTE, CONST_PERSISTENT);
#endif
#if defined(ENOLINK)
	REGISTER_LONG_CONSTANT("SOCKET_ENOLINK", ENOLINK, CONST_PERSISTENT);
#endif
#if defined(EADV)
	REGISTER_LONG_CONSTANT("SOCKET_EADV", EADV, CONST_PERSISTENT);
#endif
#if defined(ESRMNT)
	REGISTER_LONG_CONSTANT("SOCKET_ESRMNT", ESRMNT, CONST_PERSISTENT);
#endif
#if defined(ECOMM)
	REGISTER_LONG_CONSTANT("SOCKET_ECOMM", ECOMM, CONST_PERSISTENT);
#endif
#if defined(EPROTO)
	REGISTER_LONG_CONSTANT("SOCKET_EPROTO", EPROTO, CONST_PERSISTENT);
#endif
#if defined(EMULTIHOP)
	REGISTER_LONG_CONSTANT("SOCKET_EMULTIHOP", EMULTIHOP, CONST_PERSISTENT);
#endif
#if defined(EBADMSG)
	REGISTER_LONG_CONSTANT("SOCKET_EBADMSG", EBADMSG, CONST_PERSISTENT);
#endif
#if defined(ENOTUNIQ)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTUNIQ", ENOTUNIQ, CONST_PERSISTENT);
#endif
#if defined(EBADFD)
	REGISTER_LONG_CONSTANT("SOCKET_EBADFD", EBADFD, CONST_PERSISTENT);
#endif
#if defined(EREMCHG)
	REGISTER_LONG_CONSTANT("SOCKET_EREMCHG", EREMCHG, CONST_PERSISTENT);
#endif
#if defined(ERESTART)
	REGISTER_LONG_CONSTANT("SOCKET_ERESTART", ERESTART, CONST_PERSISTENT);
#endif
#if defined(ESTRPIPE)
	REGISTER_LONG_CONSTANT("SOCKET_ESTRPIPE", ESTRPIPE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EUSERS)
	REGISTER_LONG_CONSTANT("SOCKET_EUSERS", PHP_SOCKET_EUSERS, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENOTSOCK)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTSOCK", PHP_SOCKET_ENOTSOCK, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EDESTADDRREQ)
	REGISTER_LONG_CONSTANT("SOCKET_EDESTADDRREQ", PHP_SOCKET_EDESTADDRREQ, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EMSGSIZE)
	REGISTER_LONG_CONSTANT("SOCKET_EMSGSIZE", PHP_SOCKET_EMSGSIZE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EPROTOTYPE)
	REGISTER_LONG_CONSTANT("SOCKET_EPROTOTYPE", PHP_SOCKET_EPROTOTYPE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENOPROTOOPT)
	REGISTER_LONG_CONSTANT("SOCKET_ENOPROTOOPT", PHP_SOCKET_ENOPROTOOPT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EPROTONOSUPPORT)
	REGISTER_LONG_CONSTANT("SOCKET_EPROTONOSUPPORT", PHP_SOCKET_EPROTONOSUPPORT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ESOCKTNOSUPPORT)
	REGISTER_LONG_CONSTANT("SOCKET_ESOCKTNOSUPPORT", PHP_SOCKET_ESOCKTNOSUPPORT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EOPNOTSUPP)
	REGISTER_LONG_CONSTANT("SOCKET_EOPNOTSUPP", PHP_SOCKET_EOPNOTSUPP, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EPFNOSUPPORT)
	REGISTER_LONG_CONSTANT("SOCKET_EPFNOSUPPORT", PHP_SOCKET_EPFNOSUPPORT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EAFNOSUPPORT)
	REGISTER_LONG_CONSTANT("SOCKET_EAFNOSUPPORT", PHP_SOCKET_EAFNOSUPPORT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EADDRINUSE)
	REGISTER_LONG_CONSTANT("SOCKET_EADDRINUSE", PHP_SOCKET_EADDRINUSE, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EADDRNOTAVAIL)
	REGISTER_LONG_CONSTANT("SOCKET_EADDRNOTAVAIL", PHP_SOCKET_EADDRNOTAVAIL, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENETDOWN)
	REGISTER_LONG_CONSTANT("SOCKET_ENETDOWN", PHP_SOCKET_ENETDOWN, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENETUNREACH)
	REGISTER_LONG_CONSTANT("SOCKET_ENETUNREACH", PHP_SOCKET_ENETUNREACH, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENETRESET)
	REGISTER_LONG_CONSTANT("SOCKET_ENETRESET", PHP_SOCKET_ENETRESET, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ECONNABORTED)
	REGISTER_LONG_CONSTANT("SOCKET_ECONNABORTED", PHP_SOCKET_ECONNABORTED, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ECONNRESET)
	REGISTER_LONG_CONSTANT("SOCKET_ECONNRESET", PHP_SOCKET_ECONNRESET, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENOBUFS)
	REGISTER_LONG_CONSTANT("SOCKET_ENOBUFS", PHP_SOCKET_ENOBUFS, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EISCONN)
	REGISTER_LONG_CONSTANT("SOCKET_EISCONN", PHP_SOCKET_EISCONN, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ENOTCONN)
	REGISTER_LONG_CONSTANT("SOCKET_ENOTCONN", PHP_SOCKET_ENOTCONN, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ESHUTDOWN)
	REGISTER_LONG_CONSTANT("SOCKET_ESHUTDOWN", PHP_SOCKET_ESHUTDOWN, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ETOOMANYREFS)
	REGISTER_LONG_CONSTANT("SOCKET_ETOOMANYREFS", PHP_SOCKET_ETOOMANYREFS, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ETIMEDOUT)
	REGISTER_LONG_CONSTANT("SOCKET_ETIMEDOUT", PHP_SOCKET_ETIMEDOUT, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_ECONNREFUSED)
	REGISTER_LONG_CONSTANT("SOCKET_ECONNREFUSED", PHP_SOCKET_ECONNREFUSED, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EHOSTDOWN)
	REGISTER_LONG_CONSTANT("SOCKET_EHOSTDOWN", PHP_SOCKET_EHOSTDOWN, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EHOSTUNREACH)
	REGISTER_LONG_CONSTANT("SOCKET_EHOSTUNREACH", PHP_SOCKET_EHOSTUNREACH, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EALREADY)
	REGISTER_LONG_CONSTANT("SOCKET_EALREADY", PHP_SOCKET_EALREADY, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EINPROGRESS)
	REGISTER_LONG_CONSTANT("SOCKET_EINPROGRESS", PHP_SOCKET_EINPROGRESS, CONST_PERSISTENT);
#endif
#if defined(EISNAM)
	REGISTER_LONG_CONSTANT("SOCKET_EISNAM", EISNAM, CONST_PERSISTENT);
#endif
#if defined(EREMOTEIO)
	REGISTER_LONG_CONSTANT("SOCKET_EREMOTEIO", EREMOTEIO, CONST_PERSISTENT);
#endif
#if defined(PHP_SOCKET_EDQUOT)
	REGISTER_LONG_CONSTANT("SOCKET_EDQUOT", PHP_SOCKET_EDQUOT, CONST_PERSISTENT);
#endif
#if defined(ENOMEDIUM)
	REGISTER_LONG_CONSTANT("SOCKET_ENOMEDIUM", ENOMEDIUM, CONST_PERSISTENT);
#endif
#if defined(EMEDIUMTYPE)
	REGISTER_LONG_CONSTANT("SOCKET_EMEDIUMTYPE", EMEDIUMTYPE, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_ESTALE", WSAESTALE, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_EDISCON", WSAEDISCON, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_SYSNOTREADY", WSASYSNOTREADY, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_VERNOTSUPPORTED", WSAVERNOTSUPPORTED, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_NOTINITIALISED", WSANOTINITIALISED, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_HOST_NOT_FOUND", WSAHOST_NOT_FOUND, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_TRY_AGAIN", WSATRY_AGAIN, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_NO_RECOVERY", WSANO_RECOVERY, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_NO_DATA", WSANO_DATA, CONST_PERSISTENT);
#endif
#if defined(WIN32)
	REGISTER_LONG_CONSTANT("SOCKET_NO_ADDRESS", WSANO_ADDRESS, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("IPPROTO_IP", IPPROTO_IP, CONST_PERSISTENT);
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("IPPROTO_IPV6", IPPROTO_IPV6, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("SOL_TCP", IPPROTO_TCP, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOL_UDP", IPPROTO_UDP, CONST_PERSISTENT);
#if HAVE_IPV6
	REGISTER_LONG_CONSTANT("IPV6_UNICAST_HOPS", IPV6_UNICAST_HOPS, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("AI_PASSIVE", AI_PASSIVE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AI_CANONNAME", AI_CANONNAME, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("AI_NUMERICHOST", AI_NUMERICHOST, CONST_PERSISTENT);
#if HAVE_AI_V4MAPPED
	REGISTER_LONG_CONSTANT("AI_V4MAPPED", AI_V4MAPPED, CONST_PERSISTENT);
#endif
#if HAVE_AI_ALL
	REGISTER_LONG_CONSTANT("AI_ALL", AI_ALL, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("AI_ADDRCONFIG", AI_ADDRCONFIG, CONST_PERSISTENT);
#if HAVE_AI_IDN
	REGISTER_LONG_CONSTANT("AI_IDN", AI_IDN, CONST_PERSISTENT);
#endif
#if HAVE_AI_IDN
	REGISTER_LONG_CONSTANT("AI_CANONIDN", AI_CANONIDN, CONST_PERSISTENT);
#endif
#if defined(AI_NUMERICSERV)
	REGISTER_LONG_CONSTANT("AI_NUMERICSERV", AI_NUMERICSERV, CONST_PERSISTENT);
#endif
#if defined(SOL_LOCAL)
	REGISTER_LONG_CONSTANT("SOL_LOCAL", SOL_LOCAL, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVPKTINFO) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_RECVPKTINFO", IPV6_RECVPKTINFO, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVPKTINFO) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_PKTINFO", IPV6_PKTINFO, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVHOPLIMIT) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_RECVHOPLIMIT", IPV6_RECVHOPLIMIT, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVHOPLIMIT) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_HOPLIMIT", IPV6_HOPLIMIT, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVTCLASS) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_RECVTCLASS", IPV6_RECVTCLASS, CONST_PERSISTENT);
#endif
#if (defined(IPV6_RECVTCLASS) && HAVE_IPV6)
	REGISTER_LONG_CONSTANT("IPV6_TCLASS", IPV6_TCLASS, CONST_PERSISTENT);
#endif
#if defined(SCM_RIGHTS)
	REGISTER_LONG_CONSTANT("SCM_RIGHTS", SCM_RIGHTS, CONST_PERSISTENT);
#endif
#if defined(SO_PASSCRED) && defined(SCM_CREDENTIALS)
	REGISTER_LONG_CONSTANT("SCM_CREDENTIALS", SCM_CREDENTIALS, CONST_PERSISTENT);
#endif
#if defined(SO_PASSCRED) && !(defined(SCM_CREDENTIALS))
	REGISTER_LONG_CONSTANT("SCM_CREDS", SCM_CREDS, CONST_PERSISTENT);
#endif
#if defined(SO_PASSCRED)
	REGISTER_LONG_CONSTANT("SO_PASSCRED", SO_PASSCRED, CONST_PERSISTENT);
#endif
#if defined(LOCAL_CREDS_PERSISTENT)
	REGISTER_LONG_CONSTANT("SCM_CREDS2", SCM_CREDS2, CONST_PERSISTENT);
#endif
#if defined(LOCAL_CREDS_PERSISTENT)
	REGISTER_LONG_CONSTANT("LOCAL_CREDS_PERSISTENT", LOCAL_CREDS_PERSISTENT, CONST_PERSISTENT);
#endif
#if (!defined(LOCAL_CREDS_PERSISTENT) && defined(LOCAL_CREDS))
	REGISTER_LONG_CONSTANT("SCM_CREDS", SCM_CREDS, CONST_PERSISTENT);
#endif
#if (!defined(LOCAL_CREDS_PERSISTENT) && defined(LOCAL_CREDS))
	REGISTER_LONG_CONSTANT("LOCAL_CREDS", LOCAL_CREDS, CONST_PERSISTENT);
#endif
#if defined(SO_ATTACH_REUSEPORT_CBPF)
	REGISTER_LONG_CONSTANT("SO_ATTACH_REUSEPORT_CBPF", SO_ATTACH_REUSEPORT_CBPF, CONST_PERSISTENT);
#endif
#if defined(SO_DETACH_FILTER)
	REGISTER_LONG_CONSTANT("SO_DETACH_FILTER", SO_DETACH_FILTER, CONST_PERSISTENT);
#endif
#if defined(SO_DETACH_BPF)
	REGISTER_LONG_CONSTANT("SO_DETACH_BPF", SO_DETACH_BPF, CONST_PERSISTENT);
#endif
#if defined(TCP_QUICKACK)
	REGISTER_LONG_CONSTANT("TCP_QUICKACK", TCP_QUICKACK, CONST_PERSISTENT);
#endif
#if defined(IP_DONTFRAG)
	REGISTER_LONG_CONSTANT("IP_DONTFRAG", IP_DONTFRAG, CONST_PERSISTENT);
#endif
#if defined(IP_MTU_DISCOVER)
	REGISTER_LONG_CONSTANT("IP_MTU_DISCOVER", IP_MTU_DISCOVER, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_DO)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_DO", IP_PMTUDISC_DO, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_DONT)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_DONT", IP_PMTUDISC_DONT, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_WANT)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_WANT", IP_PMTUDISC_WANT, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_PROBE)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_PROBE", IP_PMTUDISC_PROBE, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_INTERFACE)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_INTERFACE", IP_PMTUDISC_INTERFACE, CONST_PERSISTENT);
#endif
#if defined(IP_PMTUDISC_OMIT)
	REGISTER_LONG_CONSTANT("IP_PMTUDISC_OMIT", IP_PMTUDISC_OMIT, CONST_PERSISTENT);
#endif
}

static zend_class_entry *register_class_Socket(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Socket", class_Socket_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;

	return class_entry;
}

static zend_class_entry *register_class_AddressInfo(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "AddressInfo", class_AddressInfo_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;

	return class_entry;
}
