--TEST--
Test / operator : 64bit long tests
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
	   echo "--- testing: $longVal / $otherVal ---\n";
      var_dump($longVal/$otherVal);
   }
}

foreach ($otherVals as $otherVal) {
   foreach($longVals as $longVal) {
	   echo "--- testing: $otherVal / $longVal ---\n";
      var_dump($otherVal/$longVal);
   }
}

?>
===DONE===
--EXPECT--
--- testing: 9223372036854775807 / 0 ---
float(INF)
--- testing: 9223372036854775807 / 1 ---
int(9223372036854775807)
--- testing: 9223372036854775807 / -1 ---
int(-9223372036854775807)
--- testing: 9223372036854775807 / 7 ---
int(1317624576693539401)
--- testing: 9223372036854775807 / 9 ---
float(1.0248191152061E+18)
--- testing: 9223372036854775807 / 65 ---
float(1.4189803133623E+17)
--- testing: 9223372036854775807 / -44 ---
float(-2.096220917467E+17)
--- testing: 9223372036854775807 / 2147483647 ---
float(4294967298)
--- testing: 9223372036854775807 / 9223372036854775807 ---
int(1)
--- testing: -9223372036854775808 / 0 ---
float(-INF)
--- testing: -9223372036854775808 / 1 ---
int(-9223372036854775808)
--- testing: -9223372036854775808 / -1 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808 / 7 ---
float(-1.3176245766935E+18)
--- testing: -9223372036854775808 / 9 ---
float(-1.0248191152061E+18)
--- testing: -9223372036854775808 / 65 ---
float(-1.4189803133623E+17)
--- testing: -9223372036854775808 / -44 ---
float(2.096220917467E+17)
--- testing: -9223372036854775808 / 2147483647 ---
float(-4294967298)
--- testing: -9223372036854775808 / 9223372036854775807 ---
float(-1)
--- testing: 2147483647 / 0 ---
float(INF)
--- testing: 2147483647 / 1 ---
int(2147483647)
--- testing: 2147483647 / -1 ---
int(-2147483647)
--- testing: 2147483647 / 7 ---
float(306783378.14286)
--- testing: 2147483647 / 9 ---
float(238609294.11111)
--- testing: 2147483647 / 65 ---
float(33038209.953846)
--- testing: 2147483647 / -44 ---
float(-48806446.522727)
--- testing: 2147483647 / 2147483647 ---
int(1)
--- testing: 2147483647 / 9223372036854775807 ---
float(2.3283064354545E-10)
--- testing: -2147483648 / 0 ---
float(-INF)
--- testing: -2147483648 / 1 ---
int(-2147483648)
--- testing: -2147483648 / -1 ---
int(2147483648)
--- testing: -2147483648 / 7 ---
float(-306783378.28571)
--- testing: -2147483648 / 9 ---
float(-238609294.22222)
--- testing: -2147483648 / 65 ---
float(-33038209.969231)
--- testing: -2147483648 / -44 ---
float(48806446.545455)
--- testing: -2147483648 / 2147483647 ---
float(-1.0000000004657)
--- testing: -2147483648 / 9223372036854775807 ---
float(-2.3283064365387E-10)
--- testing: 9223372034707292160 / 0 ---
float(INF)
--- testing: 9223372034707292160 / 1 ---
int(9223372034707292160)
--- testing: 9223372034707292160 / -1 ---
int(-9223372034707292160)
--- testing: 9223372034707292160 / 7 ---
float(1.3176245763868E+18)
--- testing: 9223372034707292160 / 9 ---
float(1.0248191149675E+18)
--- testing: 9223372034707292160 / 65 ---
float(1.4189803130319E+17)
--- testing: 9223372034707292160 / -44 ---
float(-2.0962209169789E+17)
--- testing: 9223372034707292160 / 2147483647 ---
float(4294967297)
--- testing: 9223372034707292160 / 9223372036854775807 ---
float(0.99999999976717)
--- testing: -9223372034707292160 / 0 ---
float(-INF)
--- testing: -9223372034707292160 / 1 ---
int(-9223372034707292160)
--- testing: -9223372034707292160 / -1 ---
int(9223372034707292160)
--- testing: -9223372034707292160 / 7 ---
float(-1.3176245763868E+18)
--- testing: -9223372034707292160 / 9 ---
float(-1.0248191149675E+18)
--- testing: -9223372034707292160 / 65 ---
float(-1.4189803130319E+17)
--- testing: -9223372034707292160 / -44 ---
float(2.0962209169789E+17)
--- testing: -9223372034707292160 / 2147483647 ---
float(-4294967297)
--- testing: -9223372034707292160 / 9223372036854775807 ---
float(-0.99999999976717)
--- testing: 2147483648 / 0 ---
float(INF)
--- testing: 2147483648 / 1 ---
int(2147483648)
--- testing: 2147483648 / -1 ---
int(-2147483648)
--- testing: 2147483648 / 7 ---
float(306783378.28571)
--- testing: 2147483648 / 9 ---
float(238609294.22222)
--- testing: 2147483648 / 65 ---
float(33038209.969231)
--- testing: 2147483648 / -44 ---
float(-48806446.545455)
--- testing: 2147483648 / 2147483647 ---
float(1.0000000004657)
--- testing: 2147483648 / 9223372036854775807 ---
float(2.3283064365387E-10)
--- testing: -2147483649 / 0 ---
float(-INF)
--- testing: -2147483649 / 1 ---
int(-2147483649)
--- testing: -2147483649 / -1 ---
int(2147483649)
--- testing: -2147483649 / 7 ---
float(-306783378.42857)
--- testing: -2147483649 / 9 ---
float(-238609294.33333)
--- testing: -2147483649 / 65 ---
float(-33038209.984615)
--- testing: -2147483649 / -44 ---
float(48806446.568182)
--- testing: -2147483649 / 2147483647 ---
float(-1.0000000009313)
--- testing: -2147483649 / 9223372036854775807 ---
float(-2.3283064376229E-10)
--- testing: 4294967294 / 0 ---
float(INF)
--- testing: 4294967294 / 1 ---
int(4294967294)
--- testing: 4294967294 / -1 ---
int(-4294967294)
--- testing: 4294967294 / 7 ---
float(613566756.28571)
--- testing: 4294967294 / 9 ---
float(477218588.22222)
--- testing: 4294967294 / 65 ---
float(66076419.907692)
--- testing: 4294967294 / -44 ---
float(-97612893.045455)
--- testing: 4294967294 / 2147483647 ---
int(2)
--- testing: 4294967294 / 9223372036854775807 ---
float(4.656612870909E-10)
--- testing: 4294967295 / 0 ---
float(INF)
--- testing: 4294967295 / 1 ---
int(4294967295)
--- testing: 4294967295 / -1 ---
int(-4294967295)
--- testing: 4294967295 / 7 ---
float(613566756.42857)
--- testing: 4294967295 / 9 ---
float(477218588.33333)
--- testing: 4294967295 / 65 ---
float(66076419.923077)
--- testing: 4294967295 / -44 ---
float(-97612893.068182)
--- testing: 4294967295 / 2147483647 ---
float(2.0000000004657)
--- testing: 4294967295 / 9223372036854775807 ---
float(4.6566128719932E-10)
--- testing: 4294967293 / 0 ---
float(INF)
--- testing: 4294967293 / 1 ---
int(4294967293)
--- testing: 4294967293 / -1 ---
int(-4294967293)
--- testing: 4294967293 / 7 ---
float(613566756.14286)
--- testing: 4294967293 / 9 ---
float(477218588.11111)
--- testing: 4294967293 / 65 ---
float(66076419.892308)
--- testing: 4294967293 / -44 ---
float(-97612893.022727)
--- testing: 4294967293 / 2147483647 ---
float(1.9999999995343)
--- testing: 4294967293 / 9223372036854775807 ---
float(4.6566128698248E-10)
--- testing: 9223372036854775806 / 0 ---
float(INF)
--- testing: 9223372036854775806 / 1 ---
int(9223372036854775806)
--- testing: 9223372036854775806 / -1 ---
int(-9223372036854775806)
--- testing: 9223372036854775806 / 7 ---
float(1.3176245766935E+18)
--- testing: 9223372036854775806 / 9 ---
float(1.0248191152061E+18)
--- testing: 9223372036854775806 / 65 ---
float(1.4189803133623E+17)
--- testing: 9223372036854775806 / -44 ---
float(-2.096220917467E+17)
--- testing: 9223372036854775806 / 2147483647 ---
int(4294967298)
--- testing: 9223372036854775806 / 9223372036854775807 ---
float(1)
--- testing: 9.2233720368548E+18 / 0 ---
float(INF)
--- testing: 9.2233720368548E+18 / 1 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18 / -1 ---
float(-9.2233720368548E+18)
--- testing: 9.2233720368548E+18 / 7 ---
float(1.3176245766935E+18)
--- testing: 9.2233720368548E+18 / 9 ---
float(1.0248191152061E+18)
--- testing: 9.2233720368548E+18 / 65 ---
float(1.4189803133623E+17)
--- testing: 9.2233720368548E+18 / -44 ---
float(-2.096220917467E+17)
--- testing: 9.2233720368548E+18 / 2147483647 ---
float(4294967298)
--- testing: 9.2233720368548E+18 / 9223372036854775807 ---
float(1)
--- testing: -9223372036854775807 / 0 ---
float(-INF)
--- testing: -9223372036854775807 / 1 ---
int(-9223372036854775807)
--- testing: -9223372036854775807 / -1 ---
int(9223372036854775807)
--- testing: -9223372036854775807 / 7 ---
int(-1317624576693539401)
--- testing: -9223372036854775807 / 9 ---
float(-1.0248191152061E+18)
--- testing: -9223372036854775807 / 65 ---
float(-1.4189803133623E+17)
--- testing: -9223372036854775807 / -44 ---
float(2.096220917467E+17)
--- testing: -9223372036854775807 / 2147483647 ---
float(-4294967298)
--- testing: -9223372036854775807 / 9223372036854775807 ---
int(-1)
--- testing: -9.2233720368548E+18 / 0 ---
float(-INF)
--- testing: -9.2233720368548E+18 / 1 ---
float(-9.2233720368548E+18)
--- testing: -9.2233720368548E+18 / -1 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18 / 7 ---
float(-1.3176245766935E+18)
--- testing: -9.2233720368548E+18 / 9 ---
float(-1.0248191152061E+18)
--- testing: -9.2233720368548E+18 / 65 ---
float(-1.4189803133623E+17)
--- testing: -9.2233720368548E+18 / -44 ---
float(2.096220917467E+17)
--- testing: -9.2233720368548E+18 / 2147483647 ---
float(-4294967298)
--- testing: -9.2233720368548E+18 / 9223372036854775807 ---
float(-1)
--- testing: 0 / 9223372036854775807 ---
int(0)
--- testing: 0 / -9223372036854775808 ---
int(0)
--- testing: 0 / 2147483647 ---
int(0)
--- testing: 0 / -2147483648 ---
int(0)
--- testing: 0 / 9223372034707292160 ---
int(0)
--- testing: 0 / -9223372034707292160 ---
int(0)
--- testing: 0 / 2147483648 ---
int(0)
--- testing: 0 / -2147483649 ---
int(0)
--- testing: 0 / 4294967294 ---
int(0)
--- testing: 0 / 4294967295 ---
int(0)
--- testing: 0 / 4294967293 ---
int(0)
--- testing: 0 / 9223372036854775806 ---
int(0)
--- testing: 0 / 9.2233720368548E+18 ---
float(0)
--- testing: 0 / -9223372036854775807 ---
int(0)
--- testing: 0 / -9.2233720368548E+18 ---
float(-0)
--- testing: 1 / 9223372036854775807 ---
float(1.0842021724855E-19)
--- testing: 1 / -9223372036854775808 ---
float(-1.0842021724855E-19)
--- testing: 1 / 2147483647 ---
float(4.6566128752458E-10)
--- testing: 1 / -2147483648 ---
float(-4.6566128730774E-10)
--- testing: 1 / 9223372034707292160 ---
float(1.0842021727379E-19)
--- testing: 1 / -9223372034707292160 ---
float(-1.0842021727379E-19)
--- testing: 1 / 2147483648 ---
float(4.6566128730774E-10)
--- testing: 1 / -2147483649 ---
float(-4.656612870909E-10)
--- testing: 1 / 4294967294 ---
float(2.3283064376229E-10)
--- testing: 1 / 4294967295 ---
float(2.3283064370808E-10)
--- testing: 1 / 4294967293 ---
float(2.328306438165E-10)
--- testing: 1 / 9223372036854775806 ---
float(1.0842021724855E-19)
--- testing: 1 / 9.2233720368548E+18 ---
float(1.0842021724855E-19)
--- testing: 1 / -9223372036854775807 ---
float(-1.0842021724855E-19)
--- testing: 1 / -9.2233720368548E+18 ---
float(-1.0842021724855E-19)
--- testing: -1 / 9223372036854775807 ---
float(-1.0842021724855E-19)
--- testing: -1 / -9223372036854775808 ---
float(1.0842021724855E-19)
--- testing: -1 / 2147483647 ---
float(-4.6566128752458E-10)
--- testing: -1 / -2147483648 ---
float(4.6566128730774E-10)
--- testing: -1 / 9223372034707292160 ---
float(-1.0842021727379E-19)
--- testing: -1 / -9223372034707292160 ---
float(1.0842021727379E-19)
--- testing: -1 / 2147483648 ---
float(-4.6566128730774E-10)
--- testing: -1 / -2147483649 ---
float(4.656612870909E-10)
--- testing: -1 / 4294967294 ---
float(-2.3283064376229E-10)
--- testing: -1 / 4294967295 ---
float(-2.3283064370808E-10)
--- testing: -1 / 4294967293 ---
float(-2.328306438165E-10)
--- testing: -1 / 9223372036854775806 ---
float(-1.0842021724855E-19)
--- testing: -1 / 9.2233720368548E+18 ---
float(-1.0842021724855E-19)
--- testing: -1 / -9223372036854775807 ---
float(1.0842021724855E-19)
--- testing: -1 / -9.2233720368548E+18 ---
float(1.0842021724855E-19)
--- testing: 7 / 9223372036854775807 ---
float(7.5894152073985E-19)
--- testing: 7 / -9223372036854775808 ---
float(-7.5894152073985E-19)
--- testing: 7 / 2147483647 ---
float(3.2596290126721E-9)
--- testing: 7 / -2147483648 ---
float(-3.2596290111542E-9)
--- testing: 7 / 9223372034707292160 ---
float(7.5894152091656E-19)
--- testing: 7 / -9223372034707292160 ---
float(-7.5894152091656E-19)
--- testing: 7 / 2147483648 ---
float(3.2596290111542E-9)
--- testing: 7 / -2147483649 ---
float(-3.2596290096363E-9)
--- testing: 7 / 4294967294 ---
float(1.629814506336E-9)
--- testing: 7 / 4294967295 ---
float(1.6298145059566E-9)
--- testing: 7 / 4294967293 ---
float(1.6298145067155E-9)
--- testing: 7 / 9223372036854775806 ---
float(7.5894152073985E-19)
--- testing: 7 / 9.2233720368548E+18 ---
float(7.5894152073985E-19)
--- testing: 7 / -9223372036854775807 ---
float(-7.5894152073985E-19)
--- testing: 7 / -9.2233720368548E+18 ---
float(-7.5894152073985E-19)
--- testing: 9 / 9223372036854775807 ---
float(9.7578195523695E-19)
--- testing: 9 / -9223372036854775808 ---
float(-9.7578195523695E-19)
--- testing: 9 / 2147483647 ---
float(4.1909515877212E-9)
--- testing: 9 / -2147483648 ---
float(-4.1909515857697E-9)
--- testing: 9 / 9223372034707292160 ---
float(9.7578195546415E-19)
--- testing: 9 / -9223372034707292160 ---
float(-9.7578195546415E-19)
--- testing: 9 / 2147483648 ---
float(4.1909515857697E-9)
--- testing: 9 / -2147483649 ---
float(-4.1909515838181E-9)
--- testing: 9 / 4294967294 ---
float(2.0954757938606E-9)
--- testing: 9 / 4294967295 ---
float(2.0954757933727E-9)
--- testing: 9 / 4294967293 ---
float(2.0954757943485E-9)
--- testing: 9 / 9223372036854775806 ---
float(9.7578195523695E-19)
--- testing: 9 / 9.2233720368548E+18 ---
float(9.7578195523695E-19)
--- testing: 9 / -9223372036854775807 ---
float(-9.7578195523695E-19)
--- testing: 9 / -9.2233720368548E+18 ---
float(-9.7578195523695E-19)
--- testing: 65 / 9223372036854775807 ---
float(7.0473141211558E-18)
--- testing: 65 / -9223372036854775808 ---
float(-7.0473141211558E-18)
--- testing: 65 / 2147483647 ---
float(3.0267983689098E-8)
--- testing: 65 / -2147483648 ---
float(-3.0267983675003E-8)
--- testing: 65 / 9223372034707292160 ---
float(7.0473141227966E-18)
--- testing: 65 / -9223372034707292160 ---
float(-7.0473141227966E-18)
--- testing: 65 / 2147483648 ---
float(3.0267983675003E-8)
--- testing: 65 / -2147483649 ---
float(-3.0267983660908E-8)
--- testing: 65 / 4294967294 ---
float(1.5133991844549E-8)
--- testing: 65 / 4294967295 ---
float(1.5133991841025E-8)
--- testing: 65 / 4294967293 ---
float(1.5133991848072E-8)
--- testing: 65 / 9223372036854775806 ---
float(7.0473141211558E-18)
--- testing: 65 / 9.2233720368548E+18 ---
float(7.0473141211558E-18)
--- testing: 65 / -9223372036854775807 ---
float(-7.0473141211558E-18)
--- testing: 65 / -9.2233720368548E+18 ---
float(-7.0473141211558E-18)
--- testing: -44 / 9223372036854775807 ---
float(-4.7704895589362E-18)
--- testing: -44 / -9223372036854775808 ---
float(4.7704895589362E-18)
--- testing: -44 / 2147483647 ---
float(-2.0489096651082E-8)
--- testing: -44 / -2147483648 ---
float(2.0489096641541E-8)
--- testing: -44 / 9223372034707292160 ---
float(-4.7704895600469E-18)
--- testing: -44 / -9223372034707292160 ---
float(4.7704895600469E-18)
--- testing: -44 / 2147483648 ---
float(-2.0489096641541E-8)
--- testing: -44 / -2147483649 ---
float(2.0489096632E-8)
--- testing: -44 / 4294967294 ---
float(-1.0244548325541E-8)
--- testing: -44 / 4294967295 ---
float(-1.0244548323156E-8)
--- testing: -44 / 4294967293 ---
float(-1.0244548327926E-8)
--- testing: -44 / 9223372036854775806 ---
float(-4.7704895589362E-18)
--- testing: -44 / 9.2233720368548E+18 ---
float(-4.7704895589362E-18)
--- testing: -44 / -9223372036854775807 ---
float(4.7704895589362E-18)
--- testing: -44 / -9.2233720368548E+18 ---
float(4.7704895589362E-18)
--- testing: 2147483647 / 9223372036854775807 ---
float(2.3283064354545E-10)
--- testing: 2147483647 / -9223372036854775808 ---
float(-2.3283064354545E-10)
--- testing: 2147483647 / 2147483647 ---
int(1)
--- testing: 2147483647 / -2147483648 ---
float(-0.99999999953434)
--- testing: 2147483647 / 9223372034707292160 ---
float(2.3283064359966E-10)
--- testing: 2147483647 / -9223372034707292160 ---
float(-2.3283064359966E-10)
--- testing: 2147483647 / 2147483648 ---
float(0.99999999953434)
--- testing: 2147483647 / -2147483649 ---
float(-0.99999999906868)
--- testing: 2147483647 / 4294967294 ---
float(0.5)
--- testing: 2147483647 / 4294967295 ---
float(0.49999999988358)
--- testing: 2147483647 / 4294967293 ---
float(0.50000000011642)
--- testing: 2147483647 / 9223372036854775806 ---
float(2.3283064354545E-10)
--- testing: 2147483647 / 9.2233720368548E+18 ---
float(2.3283064354545E-10)
--- testing: 2147483647 / -9223372036854775807 ---
float(-2.3283064354545E-10)
--- testing: 2147483647 / -9.2233720368548E+18 ---
float(-2.3283064354545E-10)
--- testing: 9223372036854775807 / 9223372036854775807 ---
int(1)
--- testing: 9223372036854775807 / -9223372036854775808 ---
float(-1)
--- testing: 9223372036854775807 / 2147483647 ---
float(4294967298)
--- testing: 9223372036854775807 / -2147483648 ---
float(-4294967296)
--- testing: 9223372036854775807 / 9223372034707292160 ---
float(1.0000000002328)
--- testing: 9223372036854775807 / -9223372034707292160 ---
float(-1.0000000002328)
--- testing: 9223372036854775807 / 2147483648 ---
float(4294967296)
--- testing: 9223372036854775807 / -2147483649 ---
float(-4294967294)
--- testing: 9223372036854775807 / 4294967294 ---
float(2147483649)
--- testing: 9223372036854775807 / 4294967295 ---
float(2147483648.5)
--- testing: 9223372036854775807 / 4294967293 ---
float(2147483649.5)
--- testing: 9223372036854775807 / 9223372036854775806 ---
float(1)
--- testing: 9223372036854775807 / 9.2233720368548E+18 ---
float(1)
--- testing: 9223372036854775807 / -9223372036854775807 ---
int(-1)
--- testing: 9223372036854775807 / -9.2233720368548E+18 ---
float(-1)
===DONE===
