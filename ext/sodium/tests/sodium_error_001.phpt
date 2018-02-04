--TEST--
TypeErrors will not contain param values in backtrace
--SKIPIF--
<?php if (!extension_loaded("sodium")) die("skip ext/sodium required"); ?>
--FILE--
<?php
declare(strict_types=1);

function do_crypto_shorthash($message, $key) {
  return sodium_crypto_shorthash($message, $key);
}

$m = 12;
$key = random_bytes(SODIUM_CRYPTO_SHORTHASH_KEYBYTES);
$hash = do_crypto_shorthash($m, $key);
?>
--EXPECTF--
Fatal error: Uncaught TypeError: sodium_crypto_shorthash() expects parameter 1 to be string, int given in %s:%d
Stack trace:
#0 %s(%d): sodium_crypto_shorthash()
#1 %s(%d): do_crypto_shorthash()
#2 {main}
  thrown in %s on line %d
