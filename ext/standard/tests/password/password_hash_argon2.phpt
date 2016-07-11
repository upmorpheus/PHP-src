--TEST--
Test normal operation of password_hash() with argon2
--SKIPIF--
<?php
if (!defined('PASSWORD_ARGON2')) die('Skipped: password_get_info not built with Argon2');
--FILE--
<?php

$password = "the password for testing 12345!";

$hash = password_hash($password, PASSWORD_ARGON2);
var_dump(password_verify($password, $hash));

$hash = password_hash($password, PASSWORD_ARGON2I);
var_dump(password_verify($password, $hash));

$hash = password_hash($password, PASSWORD_ARGON2D);
var_dump(password_verify($password, $hash));

echo "OK!";
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
OK!