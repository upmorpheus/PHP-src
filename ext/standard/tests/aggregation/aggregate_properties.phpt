--TEST--
aggregating all default properties
--POST--
--GET--
--FILE--
<?php
include "./ext/standard/tests/aggregation/aggregate.lib.php";

$obj = new simple();
aggregate_properties($obj, 'mixin');
print implode(',', array_keys(get_object_vars($obj)))."\n";
print $obj->simple_prop."\n";
print implode(',', get_class_methods($obj))."\n";
?>
--EXPECT--
I'm alive!
simple_prop,mix
100
simple
