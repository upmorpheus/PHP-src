--TEST--
ReflectionType possible types
--FILE--
<?php

$functions = [
    function(): int {},
    function(): float {},
    function(): string {},
    function(): bool {},
    function(): array {},
    function(): callable {},
    function(): StdClass {}
];

foreach ($functions as $function) {
    $reflectionFunc = new ReflectionFunction($function);
    $returnType = $reflectionFunc->getReturnType();
    var_dump($returnType->__toString());
}
?>
--EXPECTF--
string(3) "int"
string(5) "float"
string(6) "string"
string(4) "bool"
string(5) "array"
string(8) "callable"
string(8) "StdClass"
