--TEST--
Test session_set_save_handler() : calling default handler when save_handler=user
--INI--
session.save_handler=user
session.name=PHPSESSID
display_errors=off
--SKIPIF--
<?php
include('skipif.inc');
if (substr(PHP_OS, 0, 3) != 'WIN') {
    die('skip only for Windows');
}
?>
--FILE--
<?php

ob_start();

/* 
 * Prototype : bool session_set_save_handler(SessionHandler $handler [, bool $register_shutdown_function = true])
 * Description : Sets user-level session storage functions
 * Source code : ext/session/session.c 
 */

echo "*** Testing session_set_save_handler() : calling default handler when save_handler=user ***\n";

$oldHandler = ini_get('session.save_handler');
$handler = new SessionHandler;
session_set_save_handler($handler);

session_start();

--EXPECTF--
*** Testing session_set_save_handler() : calling default handler when save_handler=user ***
PHP Recoverable fatal error:  PHP Startup: Cannot set 'user' save handler by ini_set() or session_module_name() in Unknown on line 0

