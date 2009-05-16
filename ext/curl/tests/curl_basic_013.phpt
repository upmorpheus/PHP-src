--TEST--
Test curl_opt() function with CURLOPT_HTTP_VERSION/CURL_HTTP_VERSION_1_1 
--CREDITS--
TestFest 2009 - AFUP - Xavier Gorse <xgorse@elao.com>      
--SKIPIF--
<?php if (!extension_loaded("curl") || false === getenv(b'PHP_CURL_HTTP_REMOTE_SERVER')) print "skip"; ?>
--FILE--
<?php
/* Prototype  : bool curl_setopt(resource ch, int option, mixed value)
 * Description: Set an option for a cURL transfer
 * Source code: ext/curl/interface.c
 * Alias to functions:
 */

  $host = getenv('PHP_CURL_HTTP_REMOTE_SERVER');

  // start testing
  echo '*** Testing curl with HTTP/1.1 ***' . "\n";

  $url = "{$host}/get.php?test=httpversion";
  $ch = curl_init();

  ob_start(); // start output buffering
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  curl_setopt($ch, CURLOPT_URL, $url); //set the url we want to use
  
  $curl_content = curl_exec($ch);
  curl_close($ch);

  var_dump( $curl_content );
?>
===DONE===
--EXPECTF--
*** Testing curl with HTTP/1.1 ***
string(8) "HTTP/1.1"
===DONE===
                     