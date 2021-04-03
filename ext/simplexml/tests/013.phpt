--TEST--
SimpleXML: Split text content
--EXTENSIONS--
simplexml
--FILE--
<?php

$xml =<<<EOF
<?xml version="1.0" encoding="ISO-8859-1" ?>
<foo>bar<baz/>bar</foo>
EOF;

$sxe = simplexml_load_string($xml);

var_dump((string)$sxe);

?>
--EXPECT--
string(6) "barbar"
