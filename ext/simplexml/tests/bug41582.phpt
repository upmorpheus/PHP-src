--TEST--
Bug #41582 (SimpleXML crashes when accessing newly created element)
--SKIPIF--
<?php if (!extension_loaded("simplexml")) print "skip"; ?>
--FILE--
<?php

$xml = new SimpleXMLElement('<?xml version="1.0" standalone="yes"?>
<collection></collection>');

$xml->movie[]->characters->character[0]->name = 'Miss Coder';

echo($xml->asXml());

?>
--EXPECTF--
Fatal error: Cannot use [] for reading in %s on line %d
