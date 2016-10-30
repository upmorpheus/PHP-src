--TEST--
Bug #73254 (Incorrect indentation generated by json_encode() with JSON_PRETTY_PRINT)
--SKIPIF--
<?php
if (!extension_loaded('json')) die('skip');
?>
--FILE--
<?php

echo json_encode([json_encode([1], JSON_PRETTY_PRINT)]), "\n";

$fp = fopen('php://temp', 'r');
$data = ['a' => $fp];
echo json_encode($data), "\n";
echo json_encode([json_encode([1], JSON_PRETTY_PRINT)]), "\n";

?>
--EXPECT--
["[\n    1\n]"]

["[\n    1\n]"]
