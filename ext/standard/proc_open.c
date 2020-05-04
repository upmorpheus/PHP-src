/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Wez Furlong <wez@thebrainroom.com>                           |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "php_string.h"
#include "ext/standard/head.h"
#include "ext/standard/basic_functions.h"
#include "ext/standard/file.h"
#include "exec.h"
#include "php_globals.h"
#include "SAPI.h"
#include "main/php_network.h"
#include "zend_smart_string.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

/* This symbol is defined in ext/standard/config.m4.
 * Essentially, it is set if you HAVE_FORK || PHP_WIN32
 * Other platforms may modify that configure check and add suitable #ifdefs
 * around the alternate code.
 * */
#ifdef PHP_CAN_SUPPORT_PROC_OPEN

#if 0 && HAVE_PTSNAME && HAVE_GRANTPT && HAVE_UNLOCKPT && HAVE_SYS_IOCTL_H && HAVE_TERMIOS_H
# include <sys/ioctl.h>
# include <termios.h>
# define PHP_CAN_DO_PTS	1
#endif

#include "proc_open.h"

static int le_proc_open;

/* {{{ _php_array_to_envp */
static php_process_env_t _php_array_to_envp(zval *environment)
{
	zval *element;
	php_process_env_t env;
	zend_string *key, *str;
#ifndef PHP_WIN32
	char **ep;
#endif
	char *p;
	size_t cnt, sizeenv = 0;
	HashTable *env_hash;

	memset(&env, 0, sizeof(env));

	if (!environment) {
		return env;
	}

	cnt = zend_hash_num_elements(Z_ARRVAL_P(environment));

	if (cnt < 1) {
#ifndef PHP_WIN32
		env.envarray = (char **) ecalloc(1, sizeof(char *));
#endif
		env.envp = (char *) ecalloc(4, 1);
		return env;
	}

	ALLOC_HASHTABLE(env_hash);
	zend_hash_init(env_hash, cnt, NULL, NULL, 0);

	/* first, we have to get the size of all the elements in the hash */
	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(environment), key, element) {
		str = zval_get_string(element);

		if (ZSTR_LEN(str) == 0) {
			zend_string_release_ex(str, 0);
			continue;
		}

		sizeenv += ZSTR_LEN(str) + 1;

		if (key && ZSTR_LEN(key)) {
			sizeenv += ZSTR_LEN(key) + 1;
			zend_hash_add_ptr(env_hash, key, str);
		} else {
			zend_hash_next_index_insert_ptr(env_hash, str);
		}
	} ZEND_HASH_FOREACH_END();

#ifndef PHP_WIN32
	ep = env.envarray = (char **) ecalloc(cnt + 1, sizeof(char *));
#endif
	p = env.envp = (char *) ecalloc(sizeenv + 4, 1);

	ZEND_HASH_FOREACH_STR_KEY_PTR(env_hash, key, str) {
#ifndef PHP_WIN32
		*ep = p;
		++ep;
#endif

		if (key) {
			memcpy(p, ZSTR_VAL(key), ZSTR_LEN(key));
			p += ZSTR_LEN(key);
			*p++ = '=';
		}

		memcpy(p, ZSTR_VAL(str), ZSTR_LEN(str));
		p += ZSTR_LEN(str);
		*p++ = '\0';
		zend_string_release_ex(str, 0);
	} ZEND_HASH_FOREACH_END();

	assert((uint32_t)(p - env.envp) <= sizeenv);

	zend_hash_destroy(env_hash);
	FREE_HASHTABLE(env_hash);

	return env;
}
/* }}} */

/* {{{ _php_free_envp */
static void _php_free_envp(php_process_env_t env)
{
#ifndef PHP_WIN32
	if (env.envarray) {
		efree(env.envarray);
	}
#endif
	if (env.envp) {
		efree(env.envp);
	}
}
/* }}} */

/* {{{ proc_open_rsrc_dtor */
static void proc_open_rsrc_dtor(zend_resource *rsrc)
{
	struct php_process_handle *proc = (struct php_process_handle*)rsrc->ptr;
#ifdef PHP_WIN32
	DWORD wstatus;
#elif HAVE_SYS_WAIT_H
	int wstatus;
	int waitpid_options = 0;
	pid_t wait_pid;
#endif

	/* Close all handles to avoid a deadlock */
	for (int i = 0; i < proc->npipes; i++) {
		if (proc->pipes[i] != NULL) {
			GC_DELREF(proc->pipes[i]);
			zend_list_close(proc->pipes[i]);
			proc->pipes[i] = NULL;
		}
	}

#ifdef PHP_WIN32
	if (FG(pclose_wait)) {
		WaitForSingleObject(proc->childHandle, INFINITE);
	}
	GetExitCodeProcess(proc->childHandle, &wstatus);
	if (wstatus == STILL_ACTIVE) {
		FG(pclose_ret) = -1;
	} else {
		FG(pclose_ret) = wstatus;
	}
	CloseHandle(proc->childHandle);

#elif HAVE_SYS_WAIT_H

	if (!FG(pclose_wait)) {
		waitpid_options = WNOHANG;
	}
	do {
		wait_pid = waitpid(proc->child, &wstatus, waitpid_options);
	} while (wait_pid == -1 && errno == EINTR);

	if (wait_pid <= 0) {
		FG(pclose_ret) = -1;
	} else {
		if (WIFEXITED(wstatus))
			wstatus = WEXITSTATUS(wstatus);
		FG(pclose_ret) = wstatus;
	}

#else
	FG(pclose_ret) = -1;
#endif
	_php_free_envp(proc->env);
	efree(proc->pipes);
	efree(proc->command);
	efree(proc);

}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(proc_open) */
PHP_MINIT_FUNCTION(proc_open)
{
	le_proc_open = zend_register_list_destructors_ex(proc_open_rsrc_dtor, NULL, "process", module_number);
	return SUCCESS;
}
/* }}} */

/* {{{ proto bool proc_terminate(resource process [, int signal])
   kill a process opened by proc_open */
PHP_FUNCTION(proc_terminate)
{
	zval *zproc;
	struct php_process_handle *proc;
	zend_long sig_no = SIGTERM;

	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_RESOURCE(zproc)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(sig_no)
	ZEND_PARSE_PARAMETERS_END();

	if ((proc = (struct php_process_handle *)zend_fetch_resource(Z_RES_P(zproc), "process", le_proc_open)) == NULL) {
		RETURN_THROWS();
	}

#ifdef PHP_WIN32
	if (TerminateProcess(proc->childHandle, 255)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
#else
	if (kill(proc->child, sig_no) == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
#endif
}
/* }}} */

/* {{{ proto int|false proc_close(resource process)
   close a process opened by proc_open */
PHP_FUNCTION(proc_close)
{
	zval *zproc;
	struct php_process_handle *proc;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zproc)
	ZEND_PARSE_PARAMETERS_END();

	if ((proc = (struct php_process_handle *)zend_fetch_resource(Z_RES_P(zproc), "process", le_proc_open)) == NULL) {
		RETURN_THROWS();
	}

	FG(pclose_wait) = 1;
	zend_list_close(Z_RES_P(zproc));
	FG(pclose_wait) = 0;
	RETURN_LONG(FG(pclose_ret));
}
/* }}} */

/* {{{ proto array|false proc_get_status(resource process)
   get information about a process opened by proc_open */
PHP_FUNCTION(proc_get_status)
{
	zval *zproc;
	struct php_process_handle *proc;
#ifdef PHP_WIN32
	DWORD wstatus;
#elif HAVE_SYS_WAIT_H
	int wstatus;
	pid_t wait_pid;
#endif
	int running = 1, signaled = 0, stopped = 0;
	int exitcode = -1, termsig = 0, stopsig = 0;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zproc)
	ZEND_PARSE_PARAMETERS_END();

	if ((proc = (struct php_process_handle *)zend_fetch_resource(Z_RES_P(zproc), "process", le_proc_open)) == NULL) {
		RETURN_THROWS();
	}

	array_init(return_value);

	add_assoc_string(return_value, "command", proc->command);
	add_assoc_long(return_value, "pid", (zend_long) proc->child);

#ifdef PHP_WIN32

	GetExitCodeProcess(proc->childHandle, &wstatus);

	running = wstatus == STILL_ACTIVE;
	exitcode = running ? -1 : wstatus;

#elif HAVE_SYS_WAIT_H

	errno = 0;
	wait_pid = waitpid(proc->child, &wstatus, WNOHANG|WUNTRACED);

	if (wait_pid == proc->child) {
		if (WIFEXITED(wstatus)) {
			running = 0;
			exitcode = WEXITSTATUS(wstatus);
		}
		if (WIFSIGNALED(wstatus)) {
			running = 0;
			signaled = 1;

			termsig = WTERMSIG(wstatus);
		}
		if (WIFSTOPPED(wstatus)) {
			stopped = 1;
			stopsig = WSTOPSIG(wstatus);
		}
	} else if (wait_pid == -1) {
		running = 0;
	}
#endif

	add_assoc_bool(return_value, "running", running);
	add_assoc_bool(return_value, "signaled", signaled);
	add_assoc_bool(return_value, "stopped", stopped);
	add_assoc_long(return_value, "exitcode", exitcode);
	add_assoc_long(return_value, "termsig", termsig);
	add_assoc_long(return_value, "stopsig", stopsig);
}
/* }}} */

/* {{{ handy definitions for portability/readability */
#ifdef PHP_WIN32

/* we use this to allow child processes to inherit handles */
SECURITY_ATTRIBUTES php_proc_open_security = {
	.nLength = sizeof(SECURITY_ATTRIBUTES),
	.lpSecurityDescriptor = NULL,
	.bInheritHandle = TRUE
};

# define pipe(pair)		(CreatePipe(&pair[0], &pair[1], &php_proc_open_security, 0) ? 0 : -1)

# define COMSPEC_NT	"cmd.exe"

static inline HANDLE dup_handle(HANDLE src, BOOL inherit, BOOL closeorig)
{
	HANDLE copy, self = GetCurrentProcess();

	if (!DuplicateHandle(self, src, self, &copy, 0, inherit, DUPLICATE_SAME_ACCESS |
				(closeorig ? DUPLICATE_CLOSE_SOURCE : 0)))
		return NULL;
	return copy;
}

static inline HANDLE dup_fd_as_handle(int fd)
{
	return dup_handle((HANDLE)_get_osfhandle(fd), TRUE, FALSE);
}

# define close_descriptor(fd)	CloseHandle(fd)
#else
# define close_descriptor(fd)	close(fd)
#endif

struct php_proc_open_descriptor_item {
	int index;                                 /* desired FD number in child process */
	int is_pipe;
	php_file_descriptor_t parentend, childend; /* FDs for pipes in parent/child */
	int mode_flags;                            /* mode flags for opening FDs */
};
/* }}} */

static zend_string *get_valid_arg_string(zval *zv, int elem_num) {
	zend_string *str = zval_get_string(zv);
	if (!str) {
		return NULL;
	}

	if (strlen(ZSTR_VAL(str)) != ZSTR_LEN(str)) {
		zend_value_error("Command array element %d contains a null byte", elem_num);
		zend_string_release(str);
		return NULL;
	}

	return str;
}

#ifdef PHP_WIN32
static void append_backslashes(smart_string *str, size_t num_bs)
{
	for (size_t i = 0; i < num_bs; i++) {
		smart_string_appendc(str, '\\');
	}
}

/* See https://docs.microsoft.com/en-us/cpp/cpp/parsing-cpp-command-line-arguments */
static void append_win_escaped_arg(smart_string *str, char *arg)
{
	char c;
	size_t num_bs = 0;
	smart_string_appendc(str, '"');
	while ((c = *arg)) {
		if (c == '\\') {
			num_bs++;
		} else {
			if (c == '"') {
				/* Backslashes before " need to be doubled. */
				num_bs = num_bs * 2 + 1;
			}
			append_backslashes(str, num_bs);
			smart_string_appendc(str, c);
			num_bs = 0;
		}
		arg++;
	}
	append_backslashes(str, num_bs * 2);
	smart_string_appendc(str, '"');
}

static char *create_win_command_from_args(HashTable *args)
{
	smart_string str = {0};
	zval *arg_zv;
	zend_bool is_prog_name = 1;
	int elem_num = 0;

	ZEND_HASH_FOREACH_VAL(args, arg_zv) {
		zend_string *arg_str = get_valid_arg_string(arg_zv, ++elem_num);
		if (!arg_str) {
			smart_string_free(&str);
			return NULL;
		}

		if (!is_prog_name) {
			smart_string_appendc(&str, ' ');
		}

		append_win_escaped_arg(&str, ZSTR_VAL(arg_str));

		is_prog_name = 0;
		zend_string_release(arg_str);
	} ZEND_HASH_FOREACH_END();
	smart_string_0(&str);
	return str.c;
}

static int get_option(zval *other_options, char *option_name)
{
	zval *item = zend_hash_str_find_deref(Z_ARRVAL_P(other_options), option_name, strlen(option_name));
	if (item != NULL) {
		if (Z_TYPE_P(item) == IS_TRUE || ((Z_TYPE_P(item) == IS_LONG) && Z_LVAL_P(item))) {
			return 1;
		}
	}
	return 0;
}

static void init_startup_info(STARTUPINFOW *si, struct php_proc_open_descriptor_item *descriptors, int ndesc)
{
	memset(si, 0, sizeof(STARTUPINFOW));
	si->cb = sizeof(STARTUPINFOW);
	si->dwFlags = STARTF_USESTDHANDLES;

	si->hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si->hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si->hStdError  = GetStdHandle(STD_ERROR_HANDLE);

	/* redirect stdin/stdout/stderr if requested */
	for (int i = 0; i < ndesc; i++) {
		switch (descriptors[i].index) {
			case 0:
				si->hStdInput = descriptors[i].childend;
				break;
			case 1:
				si->hStdOutput = descriptors[i].childend;
				break;
			case 2:
				si->hStdError = descriptors[i].childend;
				break;
		}
	}
}

static void init_process_info(PROCESS_INFORMATION *pi)
{
	memset(&pi, 0, sizeof(pi));
}

static int convert_command_to_use_shell(wchar_t **cmdw, size_t cmdw_len)
{
	size_t len = sizeof(COMSPEC_NT) + sizeof(" /s /c ") + cmdw_len + 3;
	wchar_t *cmdw_shell = (wchar_t *)malloc(len * sizeof(wchar_t));

	if (cmdw_shell == NULL) {
		php_error_docref(NULL, E_WARNING, "Command conversion failed");
		return FAILURE;
	}

	if (_snwprintf(cmdw_shell, len, L"%hs /s /c \"%s\"", COMSPEC_NT, *cmdw) == -1) {
		free(cmdw_shell);
		php_error_docref(NULL, E_WARNING, "Command conversion failed");
		return FAILURE;
	}

	free(*cmdw);
	*cmdw = cmdw_shell;

	return SUCCESS;
}
#endif

static char* get_command_from_array(zval *array, char ***argv, int num_elems)
{
	zval *arg_zv;
	char *command = NULL;
	int i = 0;

	*argv = safe_emalloc(sizeof(char *), num_elems + 1, 0);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), arg_zv) {
		zend_string *arg_str = get_valid_arg_string(arg_zv, i + 1);
		if (!arg_str) {
			/* terminate argv with NULL so exit_fail code knows how many entries to free */
			(*argv)[i] = NULL;
			if (command != NULL) {
				efree(command);
			}
			return NULL;
		}

		if (i == 0) {
			command = estrdup(ZSTR_VAL(arg_str));
		}

		(*argv)[i++] = estrdup(ZSTR_VAL(arg_str));
		zend_string_release(arg_str);
	} ZEND_HASH_FOREACH_END();

	(*argv)[i] = NULL;
	return command;
}

static struct php_proc_open_descriptor_item* alloc_descriptor_array(zval *descriptorspec)
{
	int ndescriptors = zend_hash_num_elements(Z_ARRVAL_P(descriptorspec));
	return ecalloc(sizeof(struct php_proc_open_descriptor_item), ndescriptors);
}

static zend_string* get_string_parameter(zval *array, int index, char *param_name)
{
	zval *array_item;
	if ((array_item = zend_hash_index_find(Z_ARRVAL_P(array), index)) == NULL) {
		zend_value_error("Missing %s", param_name);
		return NULL;
	}
	return zval_try_get_string(array_item);
}

static int set_proc_descriptor_to_blackhole(struct php_proc_open_descriptor_item *desc)
{
#ifdef PHP_WIN32
	desc->childend = CreateFileA(
		"nul", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, NULL);
	if (desc->childend == NULL) {
		php_error_docref(NULL, E_WARNING, "Failed to open nul");
		return FAILURE;
	}
#else
	desc->childend = open("/dev/null", O_RDWR);
	if (desc->childend < 0) {
		php_error_docref(NULL, E_WARNING, "Failed to open /dev/null - %s", strerror(errno));
		return FAILURE;
	}
#endif
	return SUCCESS;
}

static int set_proc_descriptor_to_pipe(struct php_proc_open_descriptor_item *desc, zend_string *zmode)
{
	php_file_descriptor_t newpipe[2];

	if (pipe(newpipe)) {
		php_error_docref(NULL, E_WARNING, "Unable to create pipe %s", strerror(errno));
		return FAILURE;
	}

	desc->is_pipe = 1;

	if (strncmp(ZSTR_VAL(zmode), "w", 1) != 0) {
		desc->parentend = newpipe[1];
		desc->childend = newpipe[0];
		desc->mode_flags = O_WRONLY;
	} else {
		desc->parentend = newpipe[0];
		desc->childend = newpipe[1];
		desc->mode_flags = O_RDONLY;
	}

#ifdef PHP_WIN32
	/* don't let the child inherit the parent side of the pipe */
	desc->parentend = dup_handle(desc->parentend, FALSE, TRUE);

	if (ZSTR_LEN(zmode) >= 2 && ZSTR_VAL(zmode)[1] == 'b')
		desc->mode_flags |= O_BINARY;
#endif

	return SUCCESS;
}

static int set_proc_descriptor_to_file(struct php_proc_open_descriptor_item *desc, zend_string *zfile, zend_string *zmode)
{
	php_socket_t fd;

	/* try a wrapper */
	php_stream *stream = php_stream_open_wrapper(ZSTR_VAL(zfile), ZSTR_VAL(zmode),
			REPORT_ERRORS|STREAM_WILL_CAST, NULL);
	if (stream == NULL) {
		return FAILURE;
	}

	/* force into an fd */
	if (php_stream_cast(stream, PHP_STREAM_CAST_RELEASE|PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		return FAILURE;
	}

#ifdef PHP_WIN32
	desc->childend = dup_fd_as_handle((int)fd);
	_close((int)fd);

	/* simulate the append mode by fseeking to the end of the file
	this introduces a potential race-condition, but it is the best we can do, though */
	if (strchr(ZSTR_VAL(zmode), 'a')) {
		SetFilePointer(desc->childend, 0, NULL, FILE_END);
	}
#else
	desc->childend = fd;
#endif
	return SUCCESS;
}

static int dup_proc_descriptor(php_file_descriptor_t from, php_file_descriptor_t *to, zend_ulong nindex)
{
#ifdef PHP_WIN32
	*to = dup_handle(from, TRUE, FALSE);
	if (*to == NULL) {
		php_error_docref(NULL, E_WARNING, "Failed to dup() for descriptor " ZEND_LONG_FMT, nindex);
		return FAILURE;
	}
#else
	*to = dup(from);
	if (*to < 0) {
		php_error_docref(NULL, E_WARNING,
			"Failed to dup() for descriptor " ZEND_LONG_FMT " - %s", nindex, strerror(errno));
		return FAILURE;
	}
#endif
	return SUCCESS;
}

static int redirect_proc_descriptor(struct php_proc_open_descriptor_item *desc, int target, struct php_proc_open_descriptor_item *descriptors, int ndesc, int nindex)
{
	php_file_descriptor_t redirect_to = PHP_INVALID_FD;

	for (int i = 0; i < ndesc; i++) {
		if (descriptors[i].index == target) {
			redirect_to = descriptors[i].childend;
			break;
		}
	}

	if (redirect_to == PHP_INVALID_FD) { /* didn't find the index we wanted */
		if (target < 0 || target > 2) {
			php_error_docref(NULL, E_WARNING, "Redirection target %d not found", target);
			return FAILURE;
		}

		/* Support referring to a stdin/stdout/stderr pipe adopted from the parent,
		 * which happens whenever an explicit override is not provided. */
#ifndef PHP_WIN32
		redirect_to = target;
#else
		switch (target) {
			case 0: redirect_to = GetStdHandle(STD_INPUT_HANDLE); break;
			case 1: redirect_to = GetStdHandle(STD_OUTPUT_HANDLE); break;
			case 2: redirect_to = GetStdHandle(STD_ERROR_HANDLE); break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
#endif
	}

	return dup_proc_descriptor(redirect_to, &desc->childend, nindex);
}


int set_proc_descriptor_from_array(
		zval *descitem, struct php_proc_open_descriptor_item *descriptors, int ndesc, int nindex) {
	zend_string *ztype = get_string_parameter(descitem, 0, "handle qualifier");
	if (!ztype) {
		return FAILURE;
	}

	zend_string *zmode = NULL, *zfile = NULL;
	int retval = FAILURE;
	if (zend_string_equals_literal(ztype, "pipe")) {
		if ((zmode = get_string_parameter(descitem, 1, "mode parameter for 'pipe'")) == NULL) {
			goto finish;
		}

		retval = set_proc_descriptor_to_pipe(&descriptors[ndesc], zmode);
	} else if (zend_string_equals_literal(ztype, "file")) {
		if ((zfile = get_string_parameter(descitem, 1, "file name parameter for 'file'")) == NULL) {
			goto finish;
		}
		if ((zmode = get_string_parameter(descitem, 2, "mode parameter for 'file'")) == NULL) {
			goto finish;
		}

		retval = set_proc_descriptor_to_file(&descriptors[ndesc], zfile, zmode);
	} else if (zend_string_equals_literal(ztype, "redirect")) {
		zval *ztarget = zend_hash_index_find_deref(Z_ARRVAL_P(descitem), 1);
		if (!ztarget) {
			zend_value_error("Missing redirection target");
			goto finish;
		}
		if (Z_TYPE_P(ztarget) != IS_LONG) {
			zend_value_error("Redirection target must be of type int, %s given", zend_get_type_by_const(Z_TYPE_P(ztarget)));
			goto finish;
		}

		retval = redirect_proc_descriptor(
			&descriptors[ndesc], Z_LVAL_P(ztarget), descriptors, ndesc, nindex);
	} else if (zend_string_equals_literal(ztype, "null")) {
		retval = set_proc_descriptor_to_blackhole(&descriptors[ndesc]);
	} else if (zend_string_equals_literal(ztype, "pty")) {
#if PHP_CAN_DO_PTS
		if (dev_ptmx == -1) {
			/* open things up */
			dev_ptmx = open("/dev/ptmx", O_RDWR);
			if (dev_ptmx == -1) {
				php_error_docref(NULL, E_WARNING, "Failed to open /dev/ptmx, errno %d", errno);
				goto finish;
			}
			grantpt(dev_ptmx);
			unlockpt(dev_ptmx);
			slave_pty = open(ptsname(dev_ptmx), O_RDWR);

			if (slave_pty == -1) {
				php_error_docref(NULL, E_WARNING, "Failed to open slave pty, errno %d", errno);
				goto finish;
			}
		}
		descriptors[ndesc].is_pipe = 1;
		descriptors[ndesc].childend = dup(slave_pty);
		descriptors[ndesc].parentend = dup(dev_ptmx);
		descriptors[ndesc].mode_flags = O_RDWR;
		retval = SUCCESS;
#else
		php_error_docref(NULL, E_WARNING, "PTY pseudo terminal not supported on this system");
		goto finish;
#endif
	} else {
		php_error_docref(NULL, E_WARNING, "%s is not a valid descriptor spec/mode", ZSTR_VAL(ztype));
		goto finish;
	}

finish:
	if (zmode) zend_string_release(zmode);
	if (zfile) zend_string_release(zfile);
	zend_string_release(ztype);
	return retval;
}

static int close_parent_ends_of_pipes_in_child(struct php_proc_open_descriptor_item *descriptors, int ndesc)
{
	/* we are running in child process
	 * close the 'parent end' of all pipes which were opened before forking/spawning
	 * also, dup() the child end of all pipes as necessary so they will use the FD number
	 *   which the user requested */
	for (int i = 0; i < ndesc; i++) {
		if (descriptors[i].is_pipe) {
			close(descriptors[i].parentend);
		}
		if (descriptors[i].childend != descriptors[i].index) {
			if (dup2(descriptors[i].childend, descriptors[i].index) < 0) {
				php_error_docref(NULL, E_WARNING, "Unable to copy file descriptor %d (for pipe) into file descriptor %d - %s",
					descriptors[i].childend, descriptors[i].index, strerror(errno));
				return FAILURE;
			}
			close(descriptors[i].childend);
		}
	}

	return SUCCESS;
}

static void close_all_descriptors(struct php_proc_open_descriptor_item *descriptors, int ndesc)
{
	for (int i = 0; i < ndesc; i++) {
		close_descriptor(descriptors[i].childend);
		if (descriptors[i].parentend)
			close_descriptor(descriptors[i].parentend);
	}
}

static void efree_argv(char **argv)
{
	if (argv) {
		char **arg = argv;
		while (*arg != NULL) {
			efree(*arg);
			arg++;
		}
		efree(argv);
	}
}

/* {{{ proto resource|false proc_open(string|array command, array descriptorspec, array &pipes [, string cwd [, array env [, array other_options]]])
   Run a process with more control over it's file descriptors */
PHP_FUNCTION(proc_open)
{
	zval *command_zv;
	char *command = NULL, *cwd = NULL;
	size_t cwd_len = 0;
	zval *descriptorspec;
	zval *pipes;
	zval *environment = NULL;
	zval *other_options = NULL;
	php_process_env_t env;
	int ndesc = 0;
	int i;
	zval *descitem = NULL;
	zend_string *str_index;
	zend_ulong nindex;
	struct php_proc_open_descriptor_item *descriptors = NULL;
#ifdef PHP_WIN32
	PROCESS_INFORMATION pi;
	HANDLE childHandle;
	STARTUPINFOW si;
	BOOL newprocok;
	DWORD dwCreateFlags = 0;
	UINT old_error_mode;
	char cur_cwd[MAXPATHLEN];
	wchar_t *cmdw = NULL, *cwdw = NULL, *envpw = NULL;
	size_t cmdw_len;
	int suppress_errors = 0;
	int bypass_shell = 0;
	int blocking_pipes = 0;
	int create_process_group = 0;
	int create_new_console = 0;
#else
	char **argv = NULL;
#endif
	php_process_id_t child;
	struct php_process_handle *proc;

#if PHP_CAN_DO_PTS
	php_file_descriptor_t dev_ptmx = -1;	/* master */
	php_file_descriptor_t slave_pty = -1;
#endif

	ZEND_PARSE_PARAMETERS_START(3, 6)
		Z_PARAM_ZVAL(command_zv)
		Z_PARAM_ARRAY(descriptorspec)
		Z_PARAM_ZVAL(pipes)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING_OR_NULL(cwd, cwd_len)
		Z_PARAM_ARRAY_OR_NULL(environment)
		Z_PARAM_ARRAY_OR_NULL(other_options)
	ZEND_PARSE_PARAMETERS_END();

	memset(&env, 0, sizeof(env));

	if (Z_TYPE_P(command_zv) == IS_ARRAY) {
		uint32_t num_elems = zend_hash_num_elements(Z_ARRVAL_P(command_zv));
		if (num_elems == 0) {
			zend_argument_value_error(1, "must have at least one element");
			RETURN_THROWS();
		}

#ifdef PHP_WIN32
		bypass_shell = 1;
		command = create_win_command_from_args(Z_ARRVAL_P(command_zv));
		if (!command) {
			RETURN_FALSE;
		}
#else
		if ((command = get_command_from_array(command_zv, &argv, num_elems)) == NULL) {
			goto exit_fail;
		}
#endif
	} else {
		convert_to_string(command_zv);
		command = estrdup(Z_STRVAL_P(command_zv));
	}

#ifdef PHP_WIN32
	if (other_options) {
		suppress_errors      = get_option(other_options, "suppress_errors");
		/* TODO Deprecate in favor of array command? */
		bypass_shell         = bypass_shell || get_option(other_options, "bypass_shell");
		blocking_pipes       = get_option(other_options, "blocking_pipes");
		create_process_group = get_option(other_options, "create_process_group");
		create_new_console   = get_option(other_options, "create_new_console");
	}
#endif

	if (environment) {
		env = _php_array_to_envp(environment);
	}

	descriptors = alloc_descriptor_array(descriptorspec);

	/* walk the descriptor spec and set up files/pipes */
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(descriptorspec), nindex, str_index, descitem) {
		if (str_index) {
			zend_argument_value_error(2, "must be an integer indexed array");
			goto exit_fail;
		}

		descriptors[ndesc].index = (int)nindex;

		if (Z_TYPE_P(descitem) == IS_RESOURCE) {
			/* should be a stream - try and dup the descriptor */
			php_stream *stream;
			php_socket_t fd;
			php_file_descriptor_t desc;

			php_stream_from_zval(stream, descitem);

			if (FAILURE == php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS)) {
				goto exit_fail;
			}

#ifdef PHP_WIN32
			desc = (HANDLE)_get_osfhandle(fd);
#else
			desc = fd;
#endif
			if (dup_proc_descriptor(desc, &descriptors[ndesc].childend, nindex) == FAILURE) {
				goto exit_fail;
			}
		} else if (Z_TYPE_P(descitem) == IS_ARRAY) {
			if (set_proc_descriptor_from_array(descitem, descriptors, ndesc, nindex) == FAILURE) {
				goto exit_fail;
			}
		} else {
			zend_argument_value_error(2, "must only contain arrays and File-Handles");
			goto exit_fail;
		}
		ndesc++;
	} ZEND_HASH_FOREACH_END();

#ifdef PHP_WIN32
	if (cwd == NULL) {
		char *getcwd_result = VCWD_GETCWD(cur_cwd, MAXPATHLEN);
		if (!getcwd_result) {
			php_error_docref(NULL, E_WARNING, "Cannot get current directory");
			goto exit_fail;
		}
		cwd = cur_cwd;
	}
	cwdw = php_win32_cp_any_to_w(cwd);
	if (!cwdw) {
		php_error_docref(NULL, E_WARNING, "CWD conversion failed");
		goto exit_fail;
	}

	init_startup_info(&si, descriptors, ndesc);
	init_process_info(&pi);

	if (suppress_errors) {
		old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
	}

	dwCreateFlags = NORMAL_PRIORITY_CLASS;
	if(strcmp(sapi_module.name, "cli") != 0) {
		dwCreateFlags |= CREATE_NO_WINDOW;
	}
	if (create_process_group) {
		dwCreateFlags |= CREATE_NEW_PROCESS_GROUP;
	}
	if (create_new_console) {
		dwCreateFlags |= CREATE_NEW_CONSOLE;
	}
	envpw = php_win32_cp_env_any_to_w(env.envp);
	if (envpw) {
		dwCreateFlags |= CREATE_UNICODE_ENVIRONMENT;
	} else  {
		if (env.envp) {
			php_error_docref(NULL, E_WARNING, "ENV conversion failed");
			goto exit_fail;
		}
	}

	cmdw = php_win32_cp_conv_any_to_w(command, strlen(command), &cmdw_len);
	if (!cmdw) {
		php_error_docref(NULL, E_WARNING, "Command conversion failed");
		goto exit_fail;
	}

	if (!bypass_shell) {
		if (convert_command_to_use_shell(&cmdw, cmdw_len) == FAILURE) {
			goto exit_fail;
		}
	}
	newprocok = CreateProcessW(NULL, cmdw, &php_proc_open_security, &php_proc_open_security,
		TRUE, dwCreateFlags, envpw, cwdw, &si, &pi);

	if (suppress_errors) {
		SetErrorMode(old_error_mode);
	}

	if (FALSE == newprocok) {
		DWORD dw = GetLastError();
		close_all_descriptors(descriptors, ndesc);
		php_error_docref(NULL, E_WARNING, "CreateProcess failed, error code - %u", dw);
		goto exit_fail;
	}

	childHandle = pi.hProcess;
	child       = pi.dwProcessId;
	CloseHandle(pi.hThread);
#elif HAVE_FORK
	/* the unix way */
	child = fork();

	if (child == 0) {
		/* this is the child process */

#if PHP_CAN_DO_PTS
		if (dev_ptmx >= 0) {
			int my_pid = getpid();

#ifdef TIOCNOTTY
			/* detach from original tty. Might only need this if isatty(0) is true */
			ioctl(0,TIOCNOTTY,NULL);
#else
			setsid();
#endif
			/* become process group leader */
			setpgid(my_pid, my_pid);
			tcsetpgrp(0, my_pid);
		}

		if (dev_ptmx >= 0) {
			close(dev_ptmx);
			close(slave_pty);
		}
#endif

		if (close_parent_ends_of_pipes_in_child(descriptors, ndesc) == FAILURE) {
			/* We are already in child process and can't do anything to make
			 *   proc_open() return an error in the parent
			 * All we can do is exit with a non-zero (error) exit code */
			_exit(127);
		}

		if (cwd) {
			php_ignore_value(chdir(cwd));
		}

		if (argv) {
			/* execvpe() is non-portable, use environ instead. */
			if (env.envarray) {
				environ = env.envarray;
			}
			execvp(command, argv);
		} else {
			if (env.envarray) {
				execle("/bin/sh", "sh", "-c", command, NULL, env.envarray);
			} else {
				execl("/bin/sh", "sh", "-c", command, NULL);
			}
		}

		/* If execvp/execle/execl are successful, we will never reach here
		 * Display error and exit with non-zero (error) status code */
		php_error_docref(NULL, E_WARNING, "Exec failed - %s", strerror(errno));
		_exit(127);
	} else if (child < 0) {
		/* failed to fork() */
		close_all_descriptors(descriptors, ndesc);
		php_error_docref(NULL, E_WARNING, "Fork failed - %s", strerror(errno));
		goto exit_fail;
	}
#else
# error You lose (configure should not have let you get here)
#endif

	/* we forked/spawned and this is the parent */

	pipes = zend_try_array_init(pipes);
	if (!pipes) {
		goto exit_fail;
	}

	proc = (struct php_process_handle*) emalloc(sizeof(struct php_process_handle));
	proc->command = command;
	proc->pipes = emalloc(sizeof(zend_resource *) * ndesc);
	proc->npipes = ndesc;
	proc->child = child;
#ifdef PHP_WIN32
	proc->childHandle = childHandle;
#endif
	proc->env = env;

#if PHP_CAN_DO_PTS
	if (dev_ptmx >= 0) {
		close(dev_ptmx);
		close(slave_pty);
	}
#endif

	/* clean up all the child ends and then open streams on the parent
	 * ends, where appropriate */
	for (i = 0; i < ndesc; i++) {
		char *mode_string = NULL;
		php_stream *stream = NULL;

		close_descriptor(descriptors[i].childend);

		if (descriptors[i].is_pipe) {
			switch (descriptors[i].mode_flags) {
#ifdef PHP_WIN32
				case O_WRONLY|O_BINARY:
					mode_string = "wb";
					break;
				case O_RDONLY|O_BINARY:
					mode_string = "rb";
					break;
#endif
				case O_WRONLY:
					mode_string = "w";
					break;
				case O_RDONLY:
					mode_string = "r";
					break;
				case O_RDWR:
					mode_string = "r+";
					break;
			}
#ifdef PHP_WIN32
			stream = php_stream_fopen_from_fd(_open_osfhandle((zend_intptr_t)descriptors[i].parentend,
						descriptors[i].mode_flags), mode_string, NULL);
			php_stream_set_option(stream, PHP_STREAM_OPTION_PIPE_BLOCKING, blocking_pipes, NULL);
#else
			stream = php_stream_fopen_from_fd(descriptors[i].parentend, mode_string, NULL);
# if defined(F_SETFD) && defined(FD_CLOEXEC)
			/* mark the descriptor close-on-exec, so that it won't be inherited by potential other children */
			fcntl(descriptors[i].parentend, F_SETFD, FD_CLOEXEC);
# endif
#endif
			if (stream) {
				zval retfp;

				/* nasty hack; don't copy it */
				stream->flags |= PHP_STREAM_FLAG_NO_SEEK;

				php_stream_to_zval(stream, &retfp);
				add_index_zval(pipes, descriptors[i].index, &retfp);

				proc->pipes[i] = Z_RES(retfp);
				Z_ADDREF(retfp);
			}
		} else {
			proc->pipes[i] = NULL;
		}
	}

#ifdef PHP_WIN32
	free(cwdw);
	free(cmdw);
	free(envpw);
#else
	efree_argv(argv);
#endif

	efree(descriptors);
	ZVAL_RES(return_value, zend_register_resource(proc, le_proc_open));
	return;

exit_fail:
	if (descriptors) {
		efree(descriptors);
	}
	_php_free_envp(env);
	if (command) {
		efree(command);
	}
#ifdef PHP_WIN32
	free(cwdw);
	free(cmdw);
	free(envpw);
#else
	efree_argv(argv);
#endif
#if PHP_CAN_DO_PTS
	if (dev_ptmx >= 0) {
		close(dev_ptmx);
	}
	if (slave_pty >= 0) {
		close(slave_pty);
	}
#endif
	RETURN_FALSE;
}
/* }}} */

#endif /* PHP_CAN_SUPPORT_PROC_OPEN */
