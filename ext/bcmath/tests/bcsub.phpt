--TEST--
bcsub() function
--EXTENSIONS--
bcmath
--INI--
bcmath.scale=0
--FILE--
<?php
require(__DIR__ . "/run_bcmath_tests_function.inc");

$minuends = ["15", "-15", "1", "-9", "14.14", "-16.60", "0.15", "-0.01"];
$subtrahends = array_merge($minuends, [
    "0",
    "0.00",
    "-0",
    "-0.00",
    "15151324141414.412312232141241",
    "-132132245132134.1515123765412",
    "141241241241241248267654747412",
    "-149143276547656984948124912",
    "0.1322135476547459213732911312",
    "-0.123912932193769965476541321",
]);

run_bcmath_tests($minuends, $subtrahends, "-", bcsub(...));

?>
--EXPECT--
Number "15" (scale 0)
15 - 15                             = 0
15 - -15                            = 30
15 - 1                              = 14
15 - -9                             = 24
15 - 14.14                          = 0
15 - -16.60                         = 31
15 - 0.15                           = 14
15 - -0.01                          = 15
15 - 0                              = 15
15 - 0.00                           = 15
15 - -0                             = 15
15 - -0.00                          = 15
15 - 15151324141414.412312232141241 = -15151324141399
15 - -132132245132134.1515123765412 = 132132245132149
15 - 141241241241241248267654747412 = -141241241241241248267654747397
15 - -149143276547656984948124912   = 149143276547656984948124927
15 - 0.1322135476547459213732911312 = 14
15 - -0.123912932193769965476541321 = 15

Number "-15" (scale 0)
-15 - 15                             = -30
-15 - -15                            = 0
-15 - 1                              = -16
-15 - -9                             = -6
-15 - 14.14                          = -29
-15 - -16.60                         = 1
-15 - 0.15                           = -15
-15 - -0.01                          = -14
-15 - 0                              = -15
-15 - 0.00                           = -15
-15 - -0                             = -15
-15 - -0.00                          = -15
-15 - 15151324141414.412312232141241 = -15151324141429
-15 - -132132245132134.1515123765412 = 132132245132119
-15 - 141241241241241248267654747412 = -141241241241241248267654747427
-15 - -149143276547656984948124912   = 149143276547656984948124897
-15 - 0.1322135476547459213732911312 = -15
-15 - -0.123912932193769965476541321 = -14

Number "1" (scale 0)
1 - 15                             = -14
1 - -15                            = 16
1 - 1                              = 0
1 - -9                             = 10
1 - 14.14                          = -13
1 - -16.60                         = 17
1 - 0.15                           = 0
1 - -0.01                          = 1
1 - 0                              = 1
1 - 0.00                           = 1
1 - -0                             = 1
1 - -0.00                          = 1
1 - 15151324141414.412312232141241 = -15151324141413
1 - -132132245132134.1515123765412 = 132132245132135
1 - 141241241241241248267654747412 = -141241241241241248267654747411
1 - -149143276547656984948124912   = 149143276547656984948124913
1 - 0.1322135476547459213732911312 = 0
1 - -0.123912932193769965476541321 = 1

Number "-9" (scale 0)
-9 - 15                             = -24
-9 - -15                            = 6
-9 - 1                              = -10
-9 - -9                             = 0
-9 - 14.14                          = -23
-9 - -16.60                         = 7
-9 - 0.15                           = -9
-9 - -0.01                          = -8
-9 - 0                              = -9
-9 - 0.00                           = -9
-9 - -0                             = -9
-9 - -0.00                          = -9
-9 - 15151324141414.412312232141241 = -15151324141423
-9 - -132132245132134.1515123765412 = 132132245132125
-9 - 141241241241241248267654747412 = -141241241241241248267654747421
-9 - -149143276547656984948124912   = 149143276547656984948124903
-9 - 0.1322135476547459213732911312 = -9
-9 - -0.123912932193769965476541321 = -8

Number "14.14" (scale 0)
14.14 - 15                             = 0
14.14 - -15                            = 29
14.14 - 1                              = 13
14.14 - -9                             = 23
14.14 - 14.14                          = 0
14.14 - -16.60                         = 30
14.14 - 0.15                           = 13
14.14 - -0.01                          = 14
14.14 - 0                              = 14
14.14 - 0.00                           = 14
14.14 - -0                             = 14
14.14 - -0.00                          = 14
14.14 - 15151324141414.412312232141241 = -15151324141400
14.14 - -132132245132134.1515123765412 = 132132245132148
14.14 - 141241241241241248267654747412 = -141241241241241248267654747397
14.14 - -149143276547656984948124912   = 149143276547656984948124926
14.14 - 0.1322135476547459213732911312 = 14
14.14 - -0.123912932193769965476541321 = 14

Number "-16.60" (scale 0)
-16.60 - 15                             = -31
-16.60 - -15                            = -1
-16.60 - 1                              = -17
-16.60 - -9                             = -7
-16.60 - 14.14                          = -30
-16.60 - -16.60                         = 0
-16.60 - 0.15                           = -16
-16.60 - -0.01                          = -16
-16.60 - 0                              = -16
-16.60 - 0.00                           = -16
-16.60 - -0                             = -16
-16.60 - -0.00                          = -16
-16.60 - 15151324141414.412312232141241 = -15151324141431
-16.60 - -132132245132134.1515123765412 = 132132245132117
-16.60 - 141241241241241248267654747412 = -141241241241241248267654747428
-16.60 - -149143276547656984948124912   = 149143276547656984948124895
-16.60 - 0.1322135476547459213732911312 = -16
-16.60 - -0.123912932193769965476541321 = -16

Number "0.15" (scale 0)
0.15 - 15                             = -14
0.15 - -15                            = 15
0.15 - 1                              = 0
0.15 - -9                             = 9
0.15 - 14.14                          = -13
0.15 - -16.60                         = 16
0.15 - 0.15                           = 0
0.15 - -0.01                          = 0
0.15 - 0                              = 0
0.15 - 0.00                           = 0
0.15 - -0                             = 0
0.15 - -0.00                          = 0
0.15 - 15151324141414.412312232141241 = -15151324141414
0.15 - -132132245132134.1515123765412 = 132132245132134
0.15 - 141241241241241248267654747412 = -141241241241241248267654747411
0.15 - -149143276547656984948124912   = 149143276547656984948124912
0.15 - 0.1322135476547459213732911312 = 0
0.15 - -0.123912932193769965476541321 = 0

Number "-0.01" (scale 0)
-0.01 - 15                             = -15
-0.01 - -15                            = 14
-0.01 - 1                              = -1
-0.01 - -9                             = 8
-0.01 - 14.14                          = -14
-0.01 - -16.60                         = 16
-0.01 - 0.15                           = 0
-0.01 - -0.01                          = 0
-0.01 - 0                              = 0
-0.01 - 0.00                           = 0
-0.01 - -0                             = 0
-0.01 - -0.00                          = 0
-0.01 - 15151324141414.412312232141241 = -15151324141414
-0.01 - -132132245132134.1515123765412 = 132132245132134
-0.01 - 141241241241241248267654747412 = -141241241241241248267654747412
-0.01 - -149143276547656984948124912   = 149143276547656984948124911
-0.01 - 0.1322135476547459213732911312 = 0
-0.01 - -0.123912932193769965476541321 = 0

Number "15" (scale 10)
15 - 15                             = 0.0000000000
15 - -15                            = 30.0000000000
15 - 1                              = 14.0000000000
15 - -9                             = 24.0000000000
15 - 14.14                          = 0.8600000000
15 - -16.60                         = 31.6000000000
15 - 0.15                           = 14.8500000000
15 - -0.01                          = 15.0100000000
15 - 0                              = 15.0000000000
15 - 0.00                           = 15.0000000000
15 - -0                             = 15.0000000000
15 - -0.00                          = 15.0000000000
15 - 15151324141414.412312232141241 = -15151324141399.4123122321
15 - -132132245132134.1515123765412 = 132132245132149.1515123765
15 - 141241241241241248267654747412 = -141241241241241248267654747397.0000000000
15 - -149143276547656984948124912   = 149143276547656984948124927.0000000000
15 - 0.1322135476547459213732911312 = 14.8677864523
15 - -0.123912932193769965476541321 = 15.1239129321

Number "-15" (scale 10)
-15 - 15                             = -30.0000000000
-15 - -15                            = 0.0000000000
-15 - 1                              = -16.0000000000
-15 - -9                             = -6.0000000000
-15 - 14.14                          = -29.1400000000
-15 - -16.60                         = 1.6000000000
-15 - 0.15                           = -15.1500000000
-15 - -0.01                          = -14.9900000000
-15 - 0                              = -15.0000000000
-15 - 0.00                           = -15.0000000000
-15 - -0                             = -15.0000000000
-15 - -0.00                          = -15.0000000000
-15 - 15151324141414.412312232141241 = -15151324141429.4123122321
-15 - -132132245132134.1515123765412 = 132132245132119.1515123765
-15 - 141241241241241248267654747412 = -141241241241241248267654747427.0000000000
-15 - -149143276547656984948124912   = 149143276547656984948124897.0000000000
-15 - 0.1322135476547459213732911312 = -15.1322135476
-15 - -0.123912932193769965476541321 = -14.8760870678

Number "1" (scale 10)
1 - 15                             = -14.0000000000
1 - -15                            = 16.0000000000
1 - 1                              = 0.0000000000
1 - -9                             = 10.0000000000
1 - 14.14                          = -13.1400000000
1 - -16.60                         = 17.6000000000
1 - 0.15                           = 0.8500000000
1 - -0.01                          = 1.0100000000
1 - 0                              = 1.0000000000
1 - 0.00                           = 1.0000000000
1 - -0                             = 1.0000000000
1 - -0.00                          = 1.0000000000
1 - 15151324141414.412312232141241 = -15151324141413.4123122321
1 - -132132245132134.1515123765412 = 132132245132135.1515123765
1 - 141241241241241248267654747412 = -141241241241241248267654747411.0000000000
1 - -149143276547656984948124912   = 149143276547656984948124913.0000000000
1 - 0.1322135476547459213732911312 = 0.8677864523
1 - -0.123912932193769965476541321 = 1.1239129321

Number "-9" (scale 10)
-9 - 15                             = -24.0000000000
-9 - -15                            = 6.0000000000
-9 - 1                              = -10.0000000000
-9 - -9                             = 0.0000000000
-9 - 14.14                          = -23.1400000000
-9 - -16.60                         = 7.6000000000
-9 - 0.15                           = -9.1500000000
-9 - -0.01                          = -8.9900000000
-9 - 0                              = -9.0000000000
-9 - 0.00                           = -9.0000000000
-9 - -0                             = -9.0000000000
-9 - -0.00                          = -9.0000000000
-9 - 15151324141414.412312232141241 = -15151324141423.4123122321
-9 - -132132245132134.1515123765412 = 132132245132125.1515123765
-9 - 141241241241241248267654747412 = -141241241241241248267654747421.0000000000
-9 - -149143276547656984948124912   = 149143276547656984948124903.0000000000
-9 - 0.1322135476547459213732911312 = -9.1322135476
-9 - -0.123912932193769965476541321 = -8.8760870678

Number "14.14" (scale 10)
14.14 - 15                             = -0.8600000000
14.14 - -15                            = 29.1400000000
14.14 - 1                              = 13.1400000000
14.14 - -9                             = 23.1400000000
14.14 - 14.14                          = 0.0000000000
14.14 - -16.60                         = 30.7400000000
14.14 - 0.15                           = 13.9900000000
14.14 - -0.01                          = 14.1500000000
14.14 - 0                              = 14.1400000000
14.14 - 0.00                           = 14.1400000000
14.14 - -0                             = 14.1400000000
14.14 - -0.00                          = 14.1400000000
14.14 - 15151324141414.412312232141241 = -15151324141400.2723122321
14.14 - -132132245132134.1515123765412 = 132132245132148.2915123765
14.14 - 141241241241241248267654747412 = -141241241241241248267654747397.8600000000
14.14 - -149143276547656984948124912   = 149143276547656984948124926.1400000000
14.14 - 0.1322135476547459213732911312 = 14.0077864523
14.14 - -0.123912932193769965476541321 = 14.2639129321

Number "-16.60" (scale 10)
-16.60 - 15                             = -31.6000000000
-16.60 - -15                            = -1.6000000000
-16.60 - 1                              = -17.6000000000
-16.60 - -9                             = -7.6000000000
-16.60 - 14.14                          = -30.7400000000
-16.60 - -16.60                         = 0.0000000000
-16.60 - 0.15                           = -16.7500000000
-16.60 - -0.01                          = -16.5900000000
-16.60 - 0                              = -16.6000000000
-16.60 - 0.00                           = -16.6000000000
-16.60 - -0                             = -16.6000000000
-16.60 - -0.00                          = -16.6000000000
-16.60 - 15151324141414.412312232141241 = -15151324141431.0123122321
-16.60 - -132132245132134.1515123765412 = 132132245132117.5515123765
-16.60 - 141241241241241248267654747412 = -141241241241241248267654747428.6000000000
-16.60 - -149143276547656984948124912   = 149143276547656984948124895.4000000000
-16.60 - 0.1322135476547459213732911312 = -16.7322135476
-16.60 - -0.123912932193769965476541321 = -16.4760870678

Number "0.15" (scale 10)
0.15 - 15                             = -14.8500000000
0.15 - -15                            = 15.1500000000
0.15 - 1                              = -0.8500000000
0.15 - -9                             = 9.1500000000
0.15 - 14.14                          = -13.9900000000
0.15 - -16.60                         = 16.7500000000
0.15 - 0.15                           = 0.0000000000
0.15 - -0.01                          = 0.1600000000
0.15 - 0                              = 0.1500000000
0.15 - 0.00                           = 0.1500000000
0.15 - -0                             = 0.1500000000
0.15 - -0.00                          = 0.1500000000
0.15 - 15151324141414.412312232141241 = -15151324141414.2623122321
0.15 - -132132245132134.1515123765412 = 132132245132134.3015123765
0.15 - 141241241241241248267654747412 = -141241241241241248267654747411.8500000000
0.15 - -149143276547656984948124912   = 149143276547656984948124912.1500000000
0.15 - 0.1322135476547459213732911312 = 0.0177864523
0.15 - -0.123912932193769965476541321 = 0.2739129321

Number "-0.01" (scale 10)
-0.01 - 15                             = -15.0100000000
-0.01 - -15                            = 14.9900000000
-0.01 - 1                              = -1.0100000000
-0.01 - -9                             = 8.9900000000
-0.01 - 14.14                          = -14.1500000000
-0.01 - -16.60                         = 16.5900000000
-0.01 - 0.15                           = -0.1600000000
-0.01 - -0.01                          = 0.0000000000
-0.01 - 0                              = -0.0100000000
-0.01 - 0.00                           = -0.0100000000
-0.01 - -0                             = -0.0100000000
-0.01 - -0.00                          = -0.0100000000
-0.01 - 15151324141414.412312232141241 = -15151324141414.4223122321
-0.01 - -132132245132134.1515123765412 = 132132245132134.1415123765
-0.01 - 141241241241241248267654747412 = -141241241241241248267654747412.0100000000
-0.01 - -149143276547656984948124912   = 149143276547656984948124911.9900000000
-0.01 - 0.1322135476547459213732911312 = -0.1422135476
-0.01 - -0.123912932193769965476541321 = 0.1139129321
