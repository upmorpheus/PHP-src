--TEST--
Bug #46739 (array returned by curl_getinfo should contain content_type key)
--SKIPIF--
<?php 
if (!extension_loaded("curl")) {
	exit("skip curl extension not loaded");
}
if (false === getenv('PHP_CURL_HTTP_REMOTE_SERVER'))  {
	exit("skip PHP_CURL_HTTP_REMOTE_SERVER env variable is not defined");
}
?>
--FILE--
<?php
$ch = curl_init('http://127.0.0.1:9/');

curl_exec($ch);
$info = curl_getinfo($ch);

echo (array_key_exists('content_type', $info)) ? "set" : "not set";
?>
--EXPECT--
set
