--TEST--
Test normal operation of password_hash()
--FILE--
<?php
//-=-=-=-

var_dump(strlen(password_hash("foo", PASSWORD_BCRYPT)));

$hash = password_hash("foo", PASSWORD_BCRYPT);

var_dump($hash === crypt("foo", $hash));

$hash = password_hash('foo', PASSWORD_ARGON2);
var_dump(strlen($hash));
var_dump(password_verify('foo', $hash));

$hash = password_hash('foo', PASSWORD_ARGON2I);
var_dump(strlen($hash));
var_dump(password_verify('foo', $hash));

$hash = password_hash('foo', PASSWORD_ARGON2D);
var_dump(strlen($hash));
var_dump(password_verify('foo', $hash));

echo "OK!";
?>
--EXPECT--
int(60)
bool(true)
int(99)
bool(true)
int(99)
bool(true)
int(99)
bool(true)
OK!
