--TEST--
Bug #71018 (ReflectionProperty::setValue() behavior changed)
--FILE--
<?php
class T1 {
    public static $data;

    public static function getDataBySelf()
    {
        return self::$data;
    }

    public static function getDataByStatic()
    {
        return static::$data;
    }
}

class T2 extends T1 {}

$Prop1 = new ReflectionProperty(T1::class, 'data');
$Prop2 = new ReflectionProperty(T2::class, 'data');

// #1
// prints: hello, hello in PHP5, but world, hello in PHP7 - not OK
$Prop1->setValue(\T1::class, "world");
$Prop2->setValue(\T2::class, 'hello');
var_dump("T2::self = " . T2::getDataBySelf());
var_dump("T2::static = " . T2::getDataByStatic());

// #2
// prints: hello, hello in both PHP5 and PHP7 - OK
T1::$data = "world";
T2::$data = 'hello';

var_dump("T2::self = " . T2::getDataBySelf());
var_dump("T2::static = " . T2::getDataByStatic());
?>
--EXPECT--
string(16) "T2::self = hello"
string(18) "T2::static = hello"
string(16) "T2::self = hello"
string(18) "T2::static = hello"
