--TEST--
DOMDocumentType::name with invalid state.
--CREDITS--
Eric Lee Stewart <ericleestewart@gmail.com>
# TestFest Atlanta 2009-05-25
--SKIPIF--
<?php require_once('skipif.inc'); ?>
--FILE--
<?php
$doctype = new DOMDocumentType();
try {
    $doctype->name;
} catch (DOMException $exception) {
    echo $exception->getMessage() . "\n";
}
?>
--EXPECT--
Invalid State Error
