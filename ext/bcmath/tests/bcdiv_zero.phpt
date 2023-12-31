--TEST--
bcdiv() function with number zero
--EXTENSIONS--
bcmath
--INI--
bcmath.scale=0
--FILE--
<?php
require(__DIR__ . "/run_bcmath_tests_function.inc");

$dividends = ["0", "0.00", "-0", "-0.00"];
$divisors = [
    "15",
    "-15",
    "1",
    "-9",
    "14.14",
    "-16.60",
    "0.15",
    "-0.01",
    "15151324141414.412312232141241",
    "-132132245132134.1515123765412",
    "141241241241241248267654747412",
    "-149143276547656984948124912",
    "0.1322135476547459213732911312",
    "-0.123912932193769965476541321",
];

run_bcmath_tests($dividends, $divisors, "/", bcdiv(...));

?>
--EXPECT--
Number "0" (scale 0)
0 / 15                             = 0
0 / -15                            = 0
0 / 1                              = 0
0 / -9                             = 0
0 / 14.14                          = 0
0 / -16.60                         = 0
0 / 0.15                           = 0
0 / -0.01                          = 0
0 / 15151324141414.412312232141241 = 0
0 / -132132245132134.1515123765412 = 0
0 / 141241241241241248267654747412 = 0
0 / -149143276547656984948124912   = 0
0 / 0.1322135476547459213732911312 = 0
0 / -0.123912932193769965476541321 = 0

Number "0.00" (scale 0)
0.00 / 15                             = 0
0.00 / -15                            = 0
0.00 / 1                              = 0
0.00 / -9                             = 0
0.00 / 14.14                          = 0
0.00 / -16.60                         = 0
0.00 / 0.15                           = 0
0.00 / -0.01                          = 0
0.00 / 15151324141414.412312232141241 = 0
0.00 / -132132245132134.1515123765412 = 0
0.00 / 141241241241241248267654747412 = 0
0.00 / -149143276547656984948124912   = 0
0.00 / 0.1322135476547459213732911312 = 0
0.00 / -0.123912932193769965476541321 = 0

Number "-0" (scale 0)
-0 / 15                             = 0
-0 / -15                            = 0
-0 / 1                              = 0
-0 / -9                             = 0
-0 / 14.14                          = 0
-0 / -16.60                         = 0
-0 / 0.15                           = 0
-0 / -0.01                          = 0
-0 / 15151324141414.412312232141241 = 0
-0 / -132132245132134.1515123765412 = 0
-0 / 141241241241241248267654747412 = 0
-0 / -149143276547656984948124912   = 0
-0 / 0.1322135476547459213732911312 = 0
-0 / -0.123912932193769965476541321 = 0

Number "-0.00" (scale 0)
-0.00 / 15                             = 0
-0.00 / -15                            = 0
-0.00 / 1                              = 0
-0.00 / -9                             = 0
-0.00 / 14.14                          = 0
-0.00 / -16.60                         = 0
-0.00 / 0.15                           = 0
-0.00 / -0.01                          = 0
-0.00 / 15151324141414.412312232141241 = 0
-0.00 / -132132245132134.1515123765412 = 0
-0.00 / 141241241241241248267654747412 = 0
-0.00 / -149143276547656984948124912   = 0
-0.00 / 0.1322135476547459213732911312 = 0
-0.00 / -0.123912932193769965476541321 = 0

Number "0" (scale 10)
0 / 15                             = 0.0000000000
0 / -15                            = 0.0000000000
0 / 1                              = 0.0000000000
0 / -9                             = 0.0000000000
0 / 14.14                          = 0.0000000000
0 / -16.60                         = 0.0000000000
0 / 0.15                           = 0.0000000000
0 / -0.01                          = 0.0000000000
0 / 15151324141414.412312232141241 = 0.0000000000
0 / -132132245132134.1515123765412 = 0.0000000000
0 / 141241241241241248267654747412 = 0.0000000000
0 / -149143276547656984948124912   = 0.0000000000
0 / 0.1322135476547459213732911312 = 0.0000000000
0 / -0.123912932193769965476541321 = 0.0000000000

Number "0.00" (scale 10)
0.00 / 15                             = 0.0000000000
0.00 / -15                            = 0.0000000000
0.00 / 1                              = 0.0000000000
0.00 / -9                             = 0.0000000000
0.00 / 14.14                          = 0.0000000000
0.00 / -16.60                         = 0.0000000000
0.00 / 0.15                           = 0.0000000000
0.00 / -0.01                          = 0.0000000000
0.00 / 15151324141414.412312232141241 = 0.0000000000
0.00 / -132132245132134.1515123765412 = 0.0000000000
0.00 / 141241241241241248267654747412 = 0.0000000000
0.00 / -149143276547656984948124912   = 0.0000000000
0.00 / 0.1322135476547459213732911312 = 0.0000000000
0.00 / -0.123912932193769965476541321 = 0.0000000000

Number "-0" (scale 10)
-0 / 15                             = 0.0000000000
-0 / -15                            = 0.0000000000
-0 / 1                              = 0.0000000000
-0 / -9                             = 0.0000000000
-0 / 14.14                          = 0.0000000000
-0 / -16.60                         = 0.0000000000
-0 / 0.15                           = 0.0000000000
-0 / -0.01                          = 0.0000000000
-0 / 15151324141414.412312232141241 = 0.0000000000
-0 / -132132245132134.1515123765412 = 0.0000000000
-0 / 141241241241241248267654747412 = 0.0000000000
-0 / -149143276547656984948124912   = 0.0000000000
-0 / 0.1322135476547459213732911312 = 0.0000000000
-0 / -0.123912932193769965476541321 = 0.0000000000

Number "-0.00" (scale 10)
-0.00 / 15                             = 0.0000000000
-0.00 / -15                            = 0.0000000000
-0.00 / 1                              = 0.0000000000
-0.00 / -9                             = 0.0000000000
-0.00 / 14.14                          = 0.0000000000
-0.00 / -16.60                         = 0.0000000000
-0.00 / 0.15                           = 0.0000000000
-0.00 / -0.01                          = 0.0000000000
-0.00 / 15151324141414.412312232141241 = 0.0000000000
-0.00 / -132132245132134.1515123765412 = 0.0000000000
-0.00 / 141241241241241248267654747412 = 0.0000000000
-0.00 / -149143276547656984948124912   = 0.0000000000
-0.00 / 0.1322135476547459213732911312 = 0.0000000000
-0.00 / -0.123912932193769965476541321 = 0.0000000000
