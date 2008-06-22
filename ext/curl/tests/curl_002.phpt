--TEST--
Test curl_opt() function with CURLOPT_RETURNTRANSFER paremter set to 1
--CREDITS--
Sebastian Deutsch <sebastian.deutsch@9elements.com>
--SKIPIF--
<?php if (!extension_loaded("curl") || false === getenv('PHP_CURL_HTTP_REMOTE_SERVER')) print "skip"; ?>
--FILE--
<?php
/* Prototype  : bool curl_setopt(resource ch, int option, mixed value)
 * Description: Set an option for a cURL transfer
 * Source code: ext/curl/interface.c
 * Alias to functions:
 */

  $host = getenv('PHP_CURL_HTTP_REMOTE_SERVER');

  // start testing
  echo '*** Testing curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1); ***' . "\n";

  $url = "{$host}/get.php?test=get";
  $ch = curl_init();

  ob_start(); // start output buffering
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_URL, $url); //set the url we want to use
  
  $curl_content = curl_exec($ch);
  curl_close($ch);

  var_dump( $curl_content );
?>
===DONE===
--EXPECTF--
*** Testing curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1); ***
string(25) "Hello World!
Hello World!"
===DONE===
