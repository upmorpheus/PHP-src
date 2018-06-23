--TEST--
Bug #46304 (defining namespaced constant using define())
--FILE--
<?php

define('NS1\ns2\const1','value1');
define('ns1\ns2\const2','value2');
define('ns1\NS2\coNSt3','value3');
define('NS1\ns2\const4','value4', true);
define('ns1\ns2\const5','value5', true);
define('ns1\NS2\coNSt6','value6', true);

print NS1\ns2\const1 . "\n";
print ns1\ns2\const1 . "\n";
print ns1\NS2\const1 . "\n";

print NS1\ns2\const2 . "\n";
print ns1\ns2\const2 . "\n";
print ns1\NS2\const2 . "\n";

print NS1\ns2\coNSt3 . "\n";
print ns1\ns2\coNSt3 . "\n";
print ns1\ns2\coNSt3 . "\n";

print NS1\ns2\const4 . "\n";
print ns1\ns2\const4 . "\n";
print ns1\NS2\const4 . "\n";
print ns1\ns2\coNSt4 . "\n";

print NS1\ns2\const5 . "\n";
print ns1\ns2\const5 . "\n";
print ns1\NS2\const5 . "\n";
print ns1\ns2\coNSt5 . "\n";

print NS1\ns2\const6 . "\n";
print ns1\ns2\const6 . "\n";
print ns1\NS2\const6 . "\n";
print ns1\ns2\coNSt6 . "\n";

print NS1\ns2\coNSt1 . "\n";
?>
--EXPECTF--
Deprecated: define(): Declaration of case-insensitive constants is deprecated in %s on line 6

Deprecated: define(): Declaration of case-insensitive constants is deprecated in %s on line 7

Deprecated: define(): Declaration of case-insensitive constants is deprecated in %s on line 8
value1
value1
value1
value2
value2
value2
value3
value3
value3
value4
value4
value4

Deprecated: Case-insensitive constants are deprecated. The correct casing for this constant is "NS1\ns2\const4" in %s on line 25
value4
value5
value5
value5

Deprecated: Case-insensitive constants are deprecated. The correct casing for this constant is "ns1\ns2\const5" in %s on line 30
value5

Deprecated: Case-insensitive constants are deprecated. The correct casing for this constant is "ns1\NS2\coNSt6" in %s on line 32
value6

Deprecated: Case-insensitive constants are deprecated. The correct casing for this constant is "ns1\NS2\coNSt6" in %s on line 33
value6

Deprecated: Case-insensitive constants are deprecated. The correct casing for this constant is "ns1\NS2\coNSt6" in %s on line 34
value6
value6

Fatal error: Uncaught Error: Undefined constant 'NS1\ns2\coNSt1' in %sbug46304.php:%d
Stack trace:
#0 {main}
  thrown in %sbug46304.php on line %d
