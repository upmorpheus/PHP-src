--TEST--
Bug #41175 (addAttribute() fails to add an attribute with an empty value)
--SKIPIF--
<?php if (!extension_loaded("simplexml")) print "skip"; ?>
--FILE--
<?php

$xml = new SimpleXmlElement("<img></img>");
$xml->addAttribute("src", "foo");
$xml->addAttribute("alt", "");
echo $xml->asXML();

?>
===DONE===
--EXPECT--
<?xml version="1.0"?>
<img src="foo" alt=""/>
===DONE===
