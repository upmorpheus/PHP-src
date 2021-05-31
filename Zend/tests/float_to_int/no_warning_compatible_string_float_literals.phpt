--TEST--
Implicit string float to int conversions should not warn for literals if float has a fractional part equal to 0
--FILE--
<?php

echo 'Bitwise ops:' . \PHP_EOL;
$var = '1.0'|3;
var_dump($var);
$var = '1.0'&3;
var_dump($var);
$var = '1.0'^3;
var_dump($var);
$var = '1.0' << 3;
var_dump($var);
$var = '1.0' >> 3;
var_dump($var);
$var = 3 << '1.0';
var_dump($var);
$var = 3 >> '1.0';
var_dump($var);

echo 'Modulo:' . \PHP_EOL;
$var = '6.0' % 2;
var_dump($var);
$var = 9 % '2.0';
var_dump($var);

/* Float string array keys are never normalized to an integer value */
/* Strings are handled differently and always warn on non integer keys */

echo 'Function calls:' . \PHP_EOL;
function foo(int $a) {
    return $a;
}
var_dump(foo('1.0'));

var_dump(chr('60.0'));

echo 'Function returns:' . \PHP_EOL;
function bar(): int {
    return '3.0';
}
var_dump(bar());

echo 'Typed property assignment:' . \PHP_EOL;
class Test {
    public int $a;
}

$instance = new Test();
$instance->a = '1.0';
var_dump($instance->a);

?>
--EXPECT--
Bitwise ops:
int(3)
int(1)
int(2)
int(8)
int(0)
int(6)
int(1)
Modulo:
int(0)
int(1)
Function calls:
int(1)
string(1) "<"
Function returns:
int(3)
Typed property assignment:
int(1)
