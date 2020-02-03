--TEST--
Bug #60569 (Nullbyte truncates Exception $message.)
--FILE--
<?php
try {
    $msg = "Some error \x00 message";
    throw new Exception($msg);
} catch(Exception $e) {
    var_dump($e->getMessage(), $msg);
}
?>
--EXPECT--
string(20) "Some error   message"
string(20) "Some error   message"
