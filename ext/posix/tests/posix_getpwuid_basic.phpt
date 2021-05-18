--TEST--
Test posix_getpwuid() function : basic functionality
--SKIPIF--
<?php
    if (!extension_loaded('posix')) die('skip - POSIX extension not loaded');
?>
--FILE--
<?php
  echo "Basic test of POSIX getpwuid\n";


  $pwuid = posix_getpwuid(posix_getuid());

  print_r($pwuid);

?>
===DONE====
--EXPECTF--
Basic test of POSIX getpwuid
Array
(
    [name] => %s
    [passwd] => %S
    [uid] => %d
    [gid] => %d
    [gecos] => %S
    [dir] => %s
    [shell] => %s
)
===DONE====
