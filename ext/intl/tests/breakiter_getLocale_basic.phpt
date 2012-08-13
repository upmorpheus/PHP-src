--TEST--
IntlBreakIterator::getLocale(): basic test
--FILE--
<?php
ini_set("intl.error_level", E_WARNING);
ini_set("intl.default_locale", "pt_PT");

$bi = IntlBreakIterator::createSentenceInstance('pt');

var_dump($bi->getLocale(0));
var_dump($bi->getLocale(1));
?>
==DONE==
--EXPECT--
string(4) "root"
string(4) "root"
==DONE==