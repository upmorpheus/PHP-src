--TEST--
Test variations in usage of cosh()
--INI--
serialize_precision = 10
--FILE--
<?php
/*
 * proto float cosh(float number)
 * Function is implemented in ext/standard/math.c
*/


//Test cosh with a different input values

$values = array(23,
        -23,
        2.345e1,
        -2.345e1,
        0x17,
        027,
        "23",
        "23.45",
        "2.345e1",
        "1000",
        "1000ABC",
        null,
        true,
        false);

for ($i = 0; $i < count($values); $i++) {
    $res = cosh($values[$i]);
    var_dump($res);
}

?>
--EXPECTF--
float(4872401723)
float(4872401723)
float(7641446995)
float(7641446995)
float(4872401723)
float(4872401723)
float(4872401723)
float(7641446995)
float(7641446995)
float(INF)

Notice: A non well formed numeric value encountered in %s on line %d
float(INF)
float(1)
float(1.543080635)
float(1)
