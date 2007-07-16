--TEST--
Bug #41867 (getName is broken)
--SKIPIF--
<?php if (!extension_loaded("simplexml")) die("skip"); ?>
--FILE--
<?php

$a = simplexml_load_string("<a><b><c/></b></a>");
echo $a->getName()."\n";
echo $a->b->getName()."\n";
echo $a->b->c->getName();
?>
--EXPECT--
a
b
c
