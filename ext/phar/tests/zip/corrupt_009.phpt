--TEST--
Phar: corrupted zip (extra field way too long)
--EXTENSIONS--
phar
--FILE--
<?php
try {
    new PharData(__DIR__ . '/files/extralen_toolong.zip');
} catch (Exception $e) {
    echo $e->getMessage() . "\n";
}
?>
--EXPECTF--
phar error: Unable to process extra field header for file in central directory in zip-based phar "%sextralen_toolong.zip"
