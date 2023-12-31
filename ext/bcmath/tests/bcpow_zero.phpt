--TEST--
bcpow() function with number zero
--EXTENSIONS--
bcmath
--INI--
bcmath.scale=0
--FILE--
<?php
require(__DIR__ . "/run_bcmath_tests_function.inc");

$exponents = ["0", "-0", "1", "1128321638"];
$baseNumbers = [
    "0.00",
    "-0.00",
    "0.000000000000000000000000",
    "-0.000000000000000000000000",
    "-0",
    "0",
];

run_bcmath_tests($baseNumbers, $exponents, "**", bcpow(...));

?>
--EXPECT--
Number "0.00" (scale 0)
0.00 ** 0                              = 1
0.00 ** -0                             = 1
0.00 ** 1                              = 0
0.00 ** 1128321638                     = 0

Number "-0.00" (scale 0)
-0.00 ** 0                              = 1
-0.00 ** -0                             = 1
-0.00 ** 1                              = 0
-0.00 ** 1128321638                     = 0

Number "0.000000000000000000000000" (scale 0)
0.000000000000000000000000 ** 0                              = 1
0.000000000000000000000000 ** -0                             = 1
0.000000000000000000000000 ** 1                              = 0
0.000000000000000000000000 ** 1128321638                     = 0

Number "-0.000000000000000000000000" (scale 0)
-0.000000000000000000000000 ** 0                              = 1
-0.000000000000000000000000 ** -0                             = 1
-0.000000000000000000000000 ** 1                              = 0
-0.000000000000000000000000 ** 1128321638                     = 0

Number "-0" (scale 0)
-0 ** 0                              = 1
-0 ** -0                             = 1
-0 ** 1                              = 0
-0 ** 1128321638                     = 0

Number "0" (scale 0)
0 ** 0                              = 1
0 ** -0                             = 1
0 ** 1                              = 0
0 ** 1128321638                     = 0

Number "0.00" (scale 10)
0.00 ** 0                              = 1.0000000000
0.00 ** -0                             = 1.0000000000
0.00 ** 1                              = 0.0000000000
0.00 ** 1128321638                     = 0.0000000000

Number "-0.00" (scale 10)
-0.00 ** 0                              = 1.0000000000
-0.00 ** -0                             = 1.0000000000
-0.00 ** 1                              = 0.0000000000
-0.00 ** 1128321638                     = 0.0000000000

Number "0.000000000000000000000000" (scale 10)
0.000000000000000000000000 ** 0                              = 1.0000000000
0.000000000000000000000000 ** -0                             = 1.0000000000
0.000000000000000000000000 ** 1                              = 0.0000000000
0.000000000000000000000000 ** 1128321638                     = 0.0000000000

Number "-0.000000000000000000000000" (scale 10)
-0.000000000000000000000000 ** 0                              = 1.0000000000
-0.000000000000000000000000 ** -0                             = 1.0000000000
-0.000000000000000000000000 ** 1                              = 0.0000000000
-0.000000000000000000000000 ** 1128321638                     = 0.0000000000

Number "-0" (scale 10)
-0 ** 0                              = 1.0000000000
-0 ** -0                             = 1.0000000000
-0 ** 1                              = 0.0000000000
-0 ** 1128321638                     = 0.0000000000

Number "0" (scale 10)
0 ** 0                              = 1.0000000000
0 ** -0                             = 1.0000000000
0 ** 1                              = 0.0000000000
0 ** 1128321638                     = 0.0000000000
