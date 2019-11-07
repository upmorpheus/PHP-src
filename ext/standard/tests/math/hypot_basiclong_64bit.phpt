--TEST--
Test hypot function : 64bit long tests
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

$otherVals = array(0, 1, -1, 7, 9, 65, -44, MAX_32Bit, MIN_32Bit, MAX_64Bit, MIN_64Bit);


foreach ($longVals as $longVal) {
   foreach($otherVals as $otherVal) {
	   echo "--- testing: $longVal, $otherVal ---\n";
      var_dump(hypot($longVal, $otherVal));
   }
}

?>
--EXPECT--
--- testing: 9223372036854775807, 0 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 1 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, -1 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 7 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 9 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 65 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, -44 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 2147483647 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, -2147483648 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775807, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: 9223372036854775807, -9223372036854775808 ---
float(1.3043817825333E+19)
--- testing: -9223372036854775808, 0 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 1 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, -1 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 7 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 9 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 65 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, -44 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 2147483647 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, -2147483648 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775808, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: -9223372036854775808, -9223372036854775808 ---
float(1.3043817825333E+19)
--- testing: 2147483647, 0 ---
float(2147483647)
--- testing: 2147483647, 1 ---
float(2147483647)
--- testing: 2147483647, -1 ---
float(2147483647)
--- testing: 2147483647, 7 ---
float(2147483647)
--- testing: 2147483647, 9 ---
float(2147483647)
--- testing: 2147483647, 65 ---
float(2147483647)
--- testing: 2147483647, -44 ---
float(2147483647)
--- testing: 2147483647, 2147483647 ---
float(3037000498.5618)
--- testing: 2147483647, -2147483648 ---
float(3037000499.2689)
--- testing: 2147483647, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 2147483647, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: -2147483648, 0 ---
float(2147483648)
--- testing: -2147483648, 1 ---
float(2147483648)
--- testing: -2147483648, -1 ---
float(2147483648)
--- testing: -2147483648, 7 ---
float(2147483648)
--- testing: -2147483648, 9 ---
float(2147483648)
--- testing: -2147483648, 65 ---
float(2147483648)
--- testing: -2147483648, -44 ---
float(2147483648)
--- testing: -2147483648, 2147483647 ---
float(3037000499.2689)
--- testing: -2147483648, -2147483648 ---
float(3037000499.976)
--- testing: -2147483648, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: -2147483648, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: 9223372034707292160, 0 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 1 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, -1 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 7 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 9 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 65 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, -44 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 2147483647 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, -2147483648 ---
float(9.2233720347073E+18)
--- testing: 9223372034707292160, 9223372036854775807 ---
float(1.3043817823814E+19)
--- testing: 9223372034707292160, -9223372036854775808 ---
float(1.3043817823814E+19)
--- testing: -9223372034707292160, 0 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 1 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, -1 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 7 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 9 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 65 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, -44 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 2147483647 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, -2147483648 ---
float(9.2233720347073E+18)
--- testing: -9223372034707292160, 9223372036854775807 ---
float(1.3043817823814E+19)
--- testing: -9223372034707292160, -9223372036854775808 ---
float(1.3043817823814E+19)
--- testing: 2147483648, 0 ---
float(2147483648)
--- testing: 2147483648, 1 ---
float(2147483648)
--- testing: 2147483648, -1 ---
float(2147483648)
--- testing: 2147483648, 7 ---
float(2147483648)
--- testing: 2147483648, 9 ---
float(2147483648)
--- testing: 2147483648, 65 ---
float(2147483648)
--- testing: 2147483648, -44 ---
float(2147483648)
--- testing: 2147483648, 2147483647 ---
float(3037000499.2689)
--- testing: 2147483648, -2147483648 ---
float(3037000499.976)
--- testing: 2147483648, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 2147483648, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: -2147483649, 0 ---
float(2147483649)
--- testing: -2147483649, 1 ---
float(2147483649)
--- testing: -2147483649, -1 ---
float(2147483649)
--- testing: -2147483649, 7 ---
float(2147483649)
--- testing: -2147483649, 9 ---
float(2147483649)
--- testing: -2147483649, 65 ---
float(2147483649)
--- testing: -2147483649, -44 ---
float(2147483649)
--- testing: -2147483649, 2147483647 ---
float(3037000499.976)
--- testing: -2147483649, -2147483648 ---
float(3037000500.6832)
--- testing: -2147483649, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: -2147483649, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: 4294967294, 0 ---
float(4294967294)
--- testing: 4294967294, 1 ---
float(4294967294)
--- testing: 4294967294, -1 ---
float(4294967294)
--- testing: 4294967294, 7 ---
float(4294967294)
--- testing: 4294967294, 9 ---
float(4294967294)
--- testing: 4294967294, 65 ---
float(4294967294)
--- testing: 4294967294, -44 ---
float(4294967294)
--- testing: 4294967294, 2147483647 ---
float(4801919415.2612)
--- testing: 4294967294, -2147483648 ---
float(4801919415.7084)
--- testing: 4294967294, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 4294967294, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: 4294967295, 0 ---
float(4294967295)
--- testing: 4294967295, 1 ---
float(4294967295)
--- testing: 4294967295, -1 ---
float(4294967295)
--- testing: 4294967295, 7 ---
float(4294967295)
--- testing: 4294967295, 9 ---
float(4294967295)
--- testing: 4294967295, 65 ---
float(4294967295)
--- testing: 4294967295, -44 ---
float(4294967295)
--- testing: 4294967295, 2147483647 ---
float(4801919416.1556)
--- testing: 4294967295, -2147483648 ---
float(4801919416.6028)
--- testing: 4294967295, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 4294967295, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: 4294967293, 0 ---
float(4294967293)
--- testing: 4294967293, 1 ---
float(4294967293)
--- testing: 4294967293, -1 ---
float(4294967293)
--- testing: 4294967293, 7 ---
float(4294967293)
--- testing: 4294967293, 9 ---
float(4294967293)
--- testing: 4294967293, 65 ---
float(4294967293)
--- testing: 4294967293, -44 ---
float(4294967293)
--- testing: 4294967293, 2147483647 ---
float(4801919414.3667)
--- testing: 4294967293, -2147483648 ---
float(4801919414.8139)
--- testing: 4294967293, 9223372036854775807 ---
float(9.2233720368548E+18)
--- testing: 4294967293, -9223372036854775808 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 0 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 1 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, -1 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 7 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 9 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 65 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, -44 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 2147483647 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, -2147483648 ---
float(9.2233720368548E+18)
--- testing: 9223372036854775806, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: 9223372036854775806, -9223372036854775808 ---
float(1.3043817825333E+19)
--- testing: 9.2233720368548E+18, 0 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 1 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, -1 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 7 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 9 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 65 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, -44 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 2147483647 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, -2147483648 ---
float(9.2233720368548E+18)
--- testing: 9.2233720368548E+18, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: 9.2233720368548E+18, -9223372036854775808 ---
float(1.3043817825333E+19)
--- testing: -9223372036854775807, 0 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 1 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, -1 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 7 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 9 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 65 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, -44 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 2147483647 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, -2147483648 ---
float(9.2233720368548E+18)
--- testing: -9223372036854775807, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: -9223372036854775807, -9223372036854775808 ---
float(1.3043817825333E+19)
--- testing: -9.2233720368548E+18, 0 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 1 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, -1 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 7 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 9 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 65 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, -44 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 2147483647 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, -2147483648 ---
float(9.2233720368548E+18)
--- testing: -9.2233720368548E+18, 9223372036854775807 ---
float(1.3043817825333E+19)
--- testing: -9.2233720368548E+18, -9223372036854775808 ---
float(1.3043817825333E+19)
