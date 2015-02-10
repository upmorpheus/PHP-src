--TEST--
Scalar type hint basics
--FILE--
<?php

$errnames = [
    E_NOTICE => 'E_NOTICE',
    E_WARNING => 'E_WARNING',
    E_RECOVERABLE_ERROR => 'E_RECOVERABLE_ERROR'
];
set_error_handler(function (int $errno, string $errmsg, string $file, int $line) use ($errnames) {
    echo "$errnames[$errno]: $errmsg on line $line\n";
    return true;
});

$functions = [
    'int' => function (int $i) { return $i; },
    'float' => function (float $f) { return $f; },
    'string' => function (string $s) { return $s; },
    'bool' => function (bool $b) { return $b; }
];

class Stringable {
    public function __toString() {
        return "foobar";
    }
}

$values = [
    1,
    "1",
    1.0,
    1.5,
    "1a",
    "a",
    "",
    PHP_INT_MAX,
    NAN,
    TRUE,
    FALSE,
    NULL,
    [],
    new StdClass,
    new Stringable,
    fopen("data:text/plain,foobar", "r")
];

foreach ($functions as $type => $function) {
    echo PHP_EOL, "Testing '$type' typehint:", PHP_EOL;
    foreach ($values as $value) {
        echo "*** Trying ";
        var_dump($value);
        var_dump($function($value));
    }
}
--EXPECTF--

Testing 'int' typehint:
*** Trying int(1)
int(1)
*** Trying string(1) "1"
int(1)
*** Trying float(1)
int(1)
*** Trying float(1.5)
int(1)
*** Trying string(2) "1a"
E_NOTICE: A non well formed numeric value encountered on line %d
int(1)
*** Trying string(1) "a"
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, string given, called in %s on line %d and defined on line %d
string(1) "a"
*** Trying string(0) ""
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, string given, called in %s on line %d and defined on line %d
string(0) ""
*** Trying int(%d)
int(%d)
*** Trying float(NAN)
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, float given, called in %s on line %d and defined on line %d
float(NAN)
*** Trying bool(true)
int(1)
*** Trying bool(false)
int(0)
*** Trying NULL
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, null given, called in %s on line %d and defined on line %d
NULL
*** Trying array(0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, array given, called in %s on line %d and defined on line %d
array(0) {
}
*** Trying object(stdClass)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, object given, called in %s on line %d and defined on line %d
object(stdClass)#%s (0) {
}
*** Trying object(Stringable)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, object given, called in %s on line %d and defined on line %d
object(Stringable)#%s (0) {
}
*** Trying resource(%d) of type (stream)
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type integer, resource given, called in %s on line %d and defined on line %d
resource(%d) of type (stream)

Testing 'float' typehint:
*** Trying int(1)
float(1)
*** Trying string(1) "1"
float(1)
*** Trying float(1)
float(1)
*** Trying float(1.5)
float(1.5)
*** Trying string(2) "1a"
E_NOTICE: A non well formed numeric value encountered on line %d
float(1)
*** Trying string(1) "a"
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, string given, called in %s on line %d and defined on line %d
string(1) "a"
*** Trying string(0) ""
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, string given, called in %s on line %d and defined on line %d
string(0) ""
*** Trying int(%d)
float(%s)
*** Trying float(NAN)
float(NAN)
*** Trying bool(true)
float(1)
*** Trying bool(false)
float(0)
*** Trying NULL
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, null given, called in %s on line %d and defined on line %d
NULL
*** Trying array(0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, array given, called in %s on line %d and defined on line %d
array(0) {
}
*** Trying object(stdClass)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, object given, called in %s on line %d and defined on line %d
object(stdClass)#%s (0) {
}
*** Trying object(Stringable)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, object given, called in %s on line %d and defined on line %d
object(Stringable)#%s (0) {
}
*** Trying resource(%d) of type (stream)
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type float, resource given, called in %s on line %d and defined on line %d
resource(%d) of type (stream)

Testing 'string' typehint:
*** Trying int(1)
string(1) "1"
*** Trying string(1) "1"
string(1) "1"
*** Trying float(1)
string(1) "1"
*** Trying float(1.5)
string(3) "1.5"
*** Trying string(2) "1a"
string(2) "1a"
*** Trying string(1) "a"
string(1) "a"
*** Trying string(0) ""
string(0) ""
*** Trying int(%d)
string(%d) "%d"
*** Trying float(NAN)
string(3) "NAN"
*** Trying bool(true)
string(1) "1"
*** Trying bool(false)
string(0) ""
*** Trying NULL
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type string, null given, called in %s on line %d and defined on line %d
NULL
*** Trying array(0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type string, array given, called in %s on line %d and defined on line %d
array(0) {
}
*** Trying object(stdClass)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type string, object given, called in %s on line %d and defined on line %d
object(stdClass)#%s (0) {
}
*** Trying object(Stringable)#%s (0) {
}
string(6) "foobar"
*** Trying resource(%d) of type (stream)
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type string, resource given, called in %s on line %d and defined on line %d
resource(%d) of type (stream)

Testing 'bool' typehint:
*** Trying int(1)
bool(true)
*** Trying string(1) "1"
bool(true)
*** Trying float(1)
bool(true)
*** Trying float(1.5)
bool(true)
*** Trying string(2) "1a"
bool(true)
*** Trying string(1) "a"
bool(true)
*** Trying string(0) ""
bool(false)
*** Trying int(%d)
bool(true)
*** Trying float(NAN)
bool(true)
*** Trying bool(true)
bool(true)
*** Trying bool(false)
bool(false)
*** Trying NULL
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type boolean, null given, called in %s on line %d and defined on line %d
NULL
*** Trying array(0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type boolean, array given, called in %s on line %d and defined on line %d
array(0) {
}
*** Trying object(stdClass)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type boolean, object given, called in %s on line %d and defined on line %d
object(stdClass)#%s (0) {
}
*** Trying object(Stringable)#%s (0) {
}
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type boolean, object given, called in %s on line %d and defined on line %d
object(Stringable)#%s (0) {
}
*** Trying resource(%d) of type (stream)
E_RECOVERABLE_ERROR: Argument 1 passed to {closure}() must be of the type boolean, resource given, called in %s on line %d and defined on line %d
resource(%d) of type (stream)
