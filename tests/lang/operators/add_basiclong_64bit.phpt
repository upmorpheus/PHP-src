--TEST--
Test + operator : 64bit long tests
--SKIPIF--
<?php
if (PHP_INT_SIZE != 8) die("skip this test is for 64bit platform only");
?>
--FILE--
<?php

define("MAX_64Bit", 9223372036854775807);
define("MAX_32Bit", 2147483647);
define("MIN_64Bit", -9223372036854775807 - 1);
define("MIN_32Bit", -2147483647 - 1);

$longVals = array(
    MAX_64Bit, MIN_64Bit, MAX_32Bit, MIN_32Bit, MAX_64Bit - MAX_32Bit, MIN_64Bit - MIN_32Bit,
    MAX_32Bit + 1, MIN_32Bit - 1, MAX_32Bit * 2, (MAX_32Bit * 2) + 1, (MAX_32Bit * 2) - 1,
    MAX_64Bit -1, MAX_64Bit + 1, MIN_64Bit + 1, MIN_64Bit - 1
);

$otherVals = array(0, 1, -1, 7, 9, 65, -44, MAX_32Bit, MAX_64Bit);

error_reporting(E_ERROR);

foreach ($longVals as $longVal) {
   foreach($otherVals as $otherVal) {
       echo "--- testing: $longVal + $otherVal ---\n";
      var_dump($longVal+$otherVal);
   }
}

foreach ($otherVals as $otherVal) {
   foreach($longVals as $longVal) {
       echo "--- testing: $otherVal + $longVal ---\n";
      var_dump($otherVal+$longVal);
   }
}

?>
--EXPECT--
--- testing: 9223372036854775807 + 0 ---
int(9223372036854775807)
--- testing: 9223372036854775807 + 1 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807 + -1 ---
int(9223372036854775806)
--- testing: 9223372036854775807 + 7 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807 + 9 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807 + 65 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807 + -44 ---
int(9223372036854775763)
--- testing: 9223372036854775807 + 2147483647 ---
float(9.2233720390023E+18)
--- testing: 9223372036854775807 + 9223372036854775807 ---
float(1.844674407371E+19)
--- testing: -9223372036854775808 + 0 ---
int(-9223372036854775808)
--- testing: -9223372036854775808 + 1 ---
int(-9223372036854775807)
--- testing: -9223372036854775808 + -1 ---
float(-9.2233720368548E+18)
--- testing: -9223372036854775808 + 7 ---
int(-9223372036854775801)
--- testing: -9223372036854775808 + 9 ---
int(-9223372036854775799)
--- testing: -9223372036854775808 + 65 ---
int(-9223372036854775743)
--- testing: -9223372036854775808 + -44 ---
float(-9.2233720368548E+18)
--- testing: -9223372036854775808 + 2147483647 ---
int(-9223372034707292161)
--- testing: -9223372036854775808 + 9223372036854775807 ---
int(-1)
--- testing: 2147483647 + 0 ---
int(2147483647)
--- testing: 2147483647 + 1 ---
int(2147483648)
--- testing: 2147483647 + -1 ---
int(2147483646)
--- testing: 2147483647 + 7 ---
int(2147483654)
--- testing: 2147483647 + 9 ---
int(2147483656)
--- testing: 2147483647 + 65 ---
int(2147483712)
--- testing: 2147483647 + -44 ---
int(2147483603)
--- testing: 2147483647 + 2147483647 ---
int(4294967294)
--- testing: 2147483647 + 9223372036854775807 ---
float(9.2233720390023E+18)
--- testing: -2147483648 + 0 ---
int(-2147483648)
--- testing: -2147483648 + 1 ---
int(-2147483647)
--- testing: -2147483648 + -1 ---
int(-2147483649)
--- testing: -2147483648 + 7 ---
int(-2147483641)
--- testing: -2147483648 + 9 ---
int(-2147483639)
--- testing: -2147483648 + 65 ---
int(-2147483583)
--- testing: -2147483648 + -44 ---
int(-2147483692)
--- testing: -2147483648 + 2147483647 ---
int(-1)
--- testing: -2147483648 + 9223372036854775807 ---
int(9223372034707292159)
--- testing: 9223372034707292160 + 0 ---
int(9223372034707292160)
--- testing: 9223372034707292160 + 1 ---
int(9223372034707292161)
--- testing: 9223372034707292160 + -1 ---
int(9223372034707292159)
--- testing: 9223372034707292160 + 7 ---
int(9223372034707292167)
--- testing: 9223372034707292160 + 9 ---
int(9223372034707292169)
--- testing: 9223372034707292160 + 65 ---
int(9223372034707292225)
--- testing: 9223372034707292160 + -44 ---
int(9223372034707292116)
--- testing: 9223372034707292160 + 2147483647 ---
int(9223372036854775807)
--- testing: 9223372034707292160 + 9223372036854775807 ---
float(1.8446744071562E+19)
--- testing: -9223372034707292160 + 0 ---
int(-9223372034707292160)
--- testing: -9223372034707292160 + 1 ---
int(-9223372034707292159)
--- testing: -9223372034707292160 + -1 ---
int(-9223372034707292161)
--- testing: -9223372034707292160 + 7 ---
int(-9223372034707292153)
--- testing: -9223372034707292160 + 9 ---
int(-9223372034707292151)
--- testing: -9223372034707292160 + 65 ---
int(-9223372034707292095)
--- testing: -9223372034707292160 + -44 ---
int(-9223372034707292204)
--- testing: -9223372034707292160 + 2147483647 ---
int(-9223372032559808513)
--- testing: -9223372034707292160 + 9223372036854775807 ---
int(2147483647)
--- testing: 2147483648 + 0 ---
int(2147483648)
--- testing: 2147483648 + 1 ---
int(2147483649)
--- testing: 2147483648 + -1 ---
int(2147483647)
--- testing: 2147483648 + 7 ---
int(2147483655)
--- testing: 2147483648 + 9 ---
int(2147483657)
--- testing: 2147483648 + 65 ---
int(2147483713)
--- testing: 2147483648 + -44 ---
int(2147483604)
--- testing: 2147483648 + 2147483647 ---
int(4294967295)
--- testing: 2147483648 + 9223372036854775807 ---
float(9.2233720390023E+18)
--- testing: -2147483649 + 0 ---
int(-2147483649)
--- testing: -2147483649 + 1 ---
int(-2147483648)
--- testing: -2147483649 + -1 ---
int(-2147483650)
--- testing: -2147483649 + 7 ---
int(-2147483642)
--- testing: -2147483649 + 9 ---
int(-2147483640)
--- testing: -2147483649 + 65 ---
int(-2147483584)
--- testing: -2147483649 + -44 ---
int(-2147483693)
--- testing: -2147483649 + 2147483647 ---
int(-2)
--- testing: -2147483649 + 9223372036854775807 ---
int(9223372034707292158)
--- testing: 4294967294 + 0 ---
int(4294967294)
--- testing: 4294967294 + 1 ---
int(4294967295)
--- testing: 4294967294 + -1 ---
int(4294967293)
--- testing: 4294967294 + 7 ---
int(4294967301)
--- testing: 4294967294 + 9 ---
int(4294967303)
--- testing: 4294967294 + 65 ---
int(4294967359)
--- testing: 4294967294 + -44 ---
int(4294967250)
--- testing: 4294967294 + 2147483647 ---
int(6442450941)
--- testing: 4294967294 + 9223372036854775807 ---
float(9.2233720411497E+18)
--- testing: 4294967295 + 0 ---
int(4294967295)
--- testing: 4294967295 + 1 ---
int(4294967296)
--- testing: 4294967295 + -1 ---
int(4294967294)
--- testing: 4294967295 + 7 ---
int(4294967302)
--- testing: 4294967295 + 9 ---
int(4294967304)
--- testing: 4294967295 + 65 ---
int(4294967360)
--- testing: 4294967295 + -44 ---
int(4294967251)
--- testing: 4294967295 + 2147483647 ---
int(6442450942)
--- testing: 4294967295 + 9223372036854775807 ---
float(9.2233720411497E+18)
--- testing: 4294967293 + 0 ---
int(4294967293)
--- testing: 4294967293 + 1 ---
int(4294967294)
--- testing: 4294967293 + -1 ---
int(4294967292)
--- testing: 4294967293 + 7 ---
int(4294967300)
--- testing: 4294967293 + 9 ---
int(4294967302)
--- testing: 4294967293 + 65 ---
int(4294967358)
--- testing: 4294967293 + -44 ---
int(4294967249)
--- testing: 4294967293 + 2147483647 ---
int(6442450940)
--- testing: 4294967293 + 9223372036854775807 ---
float(9.2233720411497E+18)
--- testing: 9223372036854775806 + 0 ---
int(9223372036854775806)
--- testing: 9223372036854775806 + 1 ---
int(9223372036854775807)
--- testing: 9223372036854775806 + -1 ---
int(9223372036854775805)
--- testing: 9223372036854775806 + 7 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806 + 9 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806 + 65 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806 + -44 ---
int(9223372036854775762)
--- testing: 9223372036854775806 + 2147483647 ---
float(9.2233720390023E+18)
--- testing: 9223372036854775806 + 9223372036854775807 ---
float(1.844674407371E+19)
--- testing: 9.2233720368548E+18 + 0 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + 1 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + -1 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + 7 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + 9 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + 65 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + -44 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 + 2147483647 ---
float(9.2233720390023E+18)
--- testing: 9.2233720368548E+18 + 9223372036854775807 ---
float(1.844674407371E+19)
--- testing: -9223372036854775807 + 0 ---
int(-9223372036854775807)
--- testing: -9223372036854775807 + 1 ---
int(-9223372036854775806)
--- testing: -9223372036854775807 + -1 ---
int(-9223372036854775808)
--- testing: -9223372036854775807 + 7 ---
int(-9223372036854775800)
--- testing: -9223372036854775807 + 9 ---
int(-9223372036854775798)
--- testing: -9223372036854775807 + 65 ---
int(-9223372036854775742)
--- testing: -9223372036854775807 + -44 ---
float(-9.2233720368548E+18)
--- testing: -9223372036854775807 + 2147483647 ---
int(-9223372034707292160)
--- testing: -9223372036854775807 + 9223372036854775807 ---
int(0)
--- testing: -9.2233720368548E+18 + 0 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + 1 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + -1 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + 7 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + 9 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + 65 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + -44 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 + 2147483647 ---
float(-9.2233720347073E+18)
--- testing: -9.2233720368548E+18 + 9223372036854775807 ---
float(0)
--- testing: 0 + 9223372036854775807 ---
int(9223372036854775807)
--- testing: 0 + -9223372036854775808 ---
int(-9223372036854775808)
--- testing: 0 + 2147483647 ---
int(2147483647)
--- testing: 0 + -2147483648 ---
int(-2147483648)
--- testing: 0 + 9223372034707292160 ---
int(9223372034707292160)
--- testing: 0 + -9223372034707292160 ---
int(-9223372034707292160)
--- testing: 0 + 2147483648 ---
int(2147483648)
--- testing: 0 + -2147483649 ---
int(-2147483649)
--- testing: 0 + 4294967294 ---
int(4294967294)
--- testing: 0 + 4294967295 ---
int(4294967295)
--- testing: 0 + 4294967293 ---
int(4294967293)
--- testing: 0 + 9223372036854775806 ---
int(9223372036854775806)
--- testing: 0 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: 0 + -9223372036854775807 ---
int(-9223372036854775807)
--- testing: 0 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: 1 + 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 1 + -9223372036854775808 ---
int(-9223372036854775807)
--- testing: 1 + 2147483647 ---
int(2147483648)
--- testing: 1 + -2147483648 ---
int(-2147483647)
--- testing: 1 + 9223372034707292160 ---
int(9223372034707292161)
--- testing: 1 + -9223372034707292160 ---
int(-9223372034707292159)
--- testing: 1 + 2147483648 ---
int(2147483649)
--- testing: 1 + -2147483649 ---
int(-2147483648)
--- testing: 1 + 4294967294 ---
int(4294967295)
--- testing: 1 + 4294967295 ---
int(4294967296)
--- testing: 1 + 4294967293 ---
int(4294967294)
--- testing: 1 + 9223372036854775806 ---
int(9223372036854775807)
--- testing: 1 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: 1 + -9223372036854775807 ---
int(-9223372036854775806)
--- testing: 1 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: -1 + 9223372036854775807 ---
int(9223372036854775806)
--- testing: -1 + -9223372036854775808 ---
float(-9.2233720368548E+18)
--- testing: -1 + 2147483647 ---
int(2147483646)
--- testing: -1 + -2147483648 ---
int(-2147483649)
--- testing: -1 + 9223372034707292160 ---
int(9223372034707292159)
--- testing: -1 + -9223372034707292160 ---
int(-9223372034707292161)
--- testing: -1 + 2147483648 ---
int(2147483647)
--- testing: -1 + -2147483649 ---
int(-2147483650)
--- testing: -1 + 4294967294 ---
int(4294967293)
--- testing: -1 + 4294967295 ---
int(4294967294)
--- testing: -1 + 4294967293 ---
int(4294967292)
--- testing: -1 + 9223372036854775806 ---
int(9223372036854775805)
--- testing: -1 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: -1 + -9223372036854775807 ---
int(-9223372036854775808)
--- testing: -1 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: 7 + 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 7 + -9223372036854775808 ---
int(-9223372036854775801)
--- testing: 7 + 2147483647 ---
int(2147483654)
--- testing: 7 + -2147483648 ---
int(-2147483641)
--- testing: 7 + 9223372034707292160 ---
int(9223372034707292167)
--- testing: 7 + -9223372034707292160 ---
int(-9223372034707292153)
--- testing: 7 + 2147483648 ---
int(2147483655)
--- testing: 7 + -2147483649 ---
int(-2147483642)
--- testing: 7 + 4294967294 ---
int(4294967301)
--- testing: 7 + 4294967295 ---
int(4294967302)
--- testing: 7 + 4294967293 ---
int(4294967300)
--- testing: 7 + 9223372036854775806 ---
float(9.2233720368548E+18)
--- testing: 7 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: 7 + -9223372036854775807 ---
int(-9223372036854775800)
--- testing: 7 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: 9 + 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 9 + -9223372036854775808 ---
int(-9223372036854775799)
--- testing: 9 + 2147483647 ---
int(2147483656)
--- testing: 9 + -2147483648 ---
int(-2147483639)
--- testing: 9 + 9223372034707292160 ---
int(9223372034707292169)
--- testing: 9 + -9223372034707292160 ---
int(-9223372034707292151)
--- testing: 9 + 2147483648 ---
int(2147483657)
--- testing: 9 + -2147483649 ---
int(-2147483640)
--- testing: 9 + 4294967294 ---
int(4294967303)
--- testing: 9 + 4294967295 ---
int(4294967304)
--- testing: 9 + 4294967293 ---
int(4294967302)
--- testing: 9 + 9223372036854775806 ---
float(9.2233720368548E+18)
--- testing: 9 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: 9 + -9223372036854775807 ---
int(-9223372036854775798)
--- testing: 9 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: 65 + 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 65 + -9223372036854775808 ---
int(-9223372036854775743)
--- testing: 65 + 2147483647 ---
int(2147483712)
--- testing: 65 + -2147483648 ---
int(-2147483583)
--- testing: 65 + 9223372034707292160 ---
int(9223372034707292225)
--- testing: 65 + -9223372034707292160 ---
int(-9223372034707292095)
--- testing: 65 + 2147483648 ---
int(2147483713)
--- testing: 65 + -2147483649 ---
int(-2147483584)
--- testing: 65 + 4294967294 ---
int(4294967359)
--- testing: 65 + 4294967295 ---
int(4294967360)
--- testing: 65 + 4294967293 ---
int(4294967358)
--- testing: 65 + 9223372036854775806 ---
float(9.2233720368548E+18)
--- testing: 65 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: 65 + -9223372036854775807 ---
int(-9223372036854775742)
--- testing: 65 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: -44 + 9223372036854775807 ---
int(9223372036854775763)
--- testing: -44 + -9223372036854775808 ---
float(-9.2233720368548E+18)
--- testing: -44 + 2147483647 ---
int(2147483603)
--- testing: -44 + -2147483648 ---
int(-2147483692)
--- testing: -44 + 9223372034707292160 ---
int(9223372034707292116)
--- testing: -44 + -9223372034707292160 ---
int(-9223372034707292204)
--- testing: -44 + 2147483648 ---
int(2147483604)
--- testing: -44 + -2147483649 ---
int(-2147483693)
--- testing: -44 + 4294967294 ---
int(4294967250)
--- testing: -44 + 4294967295 ---
int(4294967251)
--- testing: -44 + 4294967293 ---
int(4294967249)
--- testing: -44 + 9223372036854775806 ---
int(9223372036854775762)
--- testing: -44 + 9.2233720368548E+18 ---
float(9.2233720368548E+18)
--- testing: -44 + -9223372036854775807 ---
float(-9.2233720368548E+18)
--- testing: -44 + -9.2233720368548E+18 ---
float(-9.2233720368548E+18)
--- testing: 2147483647 + 9223372036854775807 ---
float(9.2233720390023E+18)
--- testing: 2147483647 + -9223372036854775808 ---
int(-9223372034707292161)
--- testing: 2147483647 + 2147483647 ---
int(4294967294)
--- testing: 2147483647 + -2147483648 ---
int(-1)
--- testing: 2147483647 + 9223372034707292160 ---
int(9223372036854775807)
--- testing: 2147483647 + -9223372034707292160 ---
int(-9223372032559808513)
--- testing: 2147483647 + 2147483648 ---
int(4294967295)
--- testing: 2147483647 + -2147483649 ---
int(-2)
--- testing: 2147483647 + 4294967294 ---
int(6442450941)
--- testing: 2147483647 + 4294967295 ---
int(6442450942)
--- testing: 2147483647 + 4294967293 ---
int(6442450940)
--- testing: 2147483647 + 9223372036854775806 ---
float(9.2233720390023E+18)
--- testing: 2147483647 + 9.2233720368548E+18 ---
float(9.2233720390023E+18)
--- testing: 2147483647 + -9223372036854775807 ---
int(-9223372034707292160)
--- testing: 2147483647 + -9.2233720368548E+18 ---
float(-9.2233720347073E+18)
--- testing: 9223372036854775807 + 9223372036854775807 ---
float(1.844674407371E+19)
--- testing: 9223372036854775807 + -9223372036854775808 ---
int(-1)
--- testing: 9223372036854775807 + 2147483647 ---
float(9.2233720390023E+18)
--- testing: 9223372036854775807 + -2147483648 ---
int(9223372034707292159)
--- testing: 9223372036854775807 + 9223372034707292160 ---
float(1.8446744071562E+19)
--- testing: 9223372036854775807 + -9223372034707292160 ---
int(2147483647)
--- testing: 9223372036854775807 + 2147483648 ---
float(9.2233720390023E+18)
--- testing: 9223372036854775807 + -2147483649 ---
int(9223372034707292158)
--- testing: 9223372036854775807 + 4294967294 ---
float(9.2233720411497E+18)
--- testing: 9223372036854775807 + 4294967295 ---
float(9.2233720411497E+18)
--- testing: 9223372036854775807 + 4294967293 ---
float(9.2233720411497E+18)
--- testing: 9223372036854775807 + 9223372036854775806 ---
float(1.844674407371E+19)
--- testing: 9223372036854775807 + 9.2233720368548E+18 ---
float(1.844674407371E+19)
--- testing: 9223372036854775807 + -9223372036854775807 ---
int(0)
--- testing: 9223372036854775807 + -9.2233720368548E+18 ---
float(0)
