--TEST--
bcadd() function with large numbers
--EXTENSIONS--
bcmath
--INI--
bcmath.scale=0
--FILE--
<?php
require(__DIR__ . "/run_bcmath_tests_function.inc");

$leftSummands = [
    "15151324141414.412312232141241",
    "-132132245132134.1515123765412",
    "141241241241241248267654747412",
    "-149143276547656984948124912",
    "0.1322135476547459213732911312",
    "-0.123912932193769965476541321",
];
$rightSummands = array_merge($leftSummands, [
    "0",
    "0.00",
    "-0",
    "-0.00",
    "15",
    "-15",
    "1",
    "-9",
    "14.14",
    "-16.60",
    "0.15",
    "-0.01",
]);

run_bcmath_tests($leftSummands, $rightSummands, "+", bcadd(...));

?>
--EXPECT--
Number "15151324141414.412312232141241" (scale 0)
15151324141414.412312232141241 + 15151324141414.412312232141241 = 30302648282828
15151324141414.412312232141241 + -132132245132134.1515123765412 = -116980920990719
15151324141414.412312232141241 + 141241241241241248267654747412 = 141241241241241263418978888826
15151324141414.412312232141241 + -149143276547656984948124912   = -149143276547641833623983497
15151324141414.412312232141241 + 0.1322135476547459213732911312 = 15151324141414
15151324141414.412312232141241 + -0.123912932193769965476541321 = 15151324141414
15151324141414.412312232141241 + 0                              = 15151324141414
15151324141414.412312232141241 + 0.00                           = 15151324141414
15151324141414.412312232141241 + -0                             = 15151324141414
15151324141414.412312232141241 + -0.00                          = 15151324141414
15151324141414.412312232141241 + 15                             = 15151324141429
15151324141414.412312232141241 + -15                            = 15151324141399
15151324141414.412312232141241 + 1                              = 15151324141415
15151324141414.412312232141241 + -9                             = 15151324141405
15151324141414.412312232141241 + 14.14                          = 15151324141428
15151324141414.412312232141241 + -16.60                         = 15151324141397
15151324141414.412312232141241 + 0.15                           = 15151324141414
15151324141414.412312232141241 + -0.01                          = 15151324141414

Number "-132132245132134.1515123765412" (scale 0)
-132132245132134.1515123765412 + 15151324141414.412312232141241 = -116980920990719
-132132245132134.1515123765412 + -132132245132134.1515123765412 = -264264490264268
-132132245132134.1515123765412 + 141241241241241248267654747412 = 141241241241241116135409615277
-132132245132134.1515123765412 + -149143276547656984948124912   = -149143276547789117193257046
-132132245132134.1515123765412 + 0.1322135476547459213732911312 = -132132245132134
-132132245132134.1515123765412 + -0.123912932193769965476541321 = -132132245132134
-132132245132134.1515123765412 + 0                              = -132132245132134
-132132245132134.1515123765412 + 0.00                           = -132132245132134
-132132245132134.1515123765412 + -0                             = -132132245132134
-132132245132134.1515123765412 + -0.00                          = -132132245132134
-132132245132134.1515123765412 + 15                             = -132132245132119
-132132245132134.1515123765412 + -15                            = -132132245132149
-132132245132134.1515123765412 + 1                              = -132132245132133
-132132245132134.1515123765412 + -9                             = -132132245132143
-132132245132134.1515123765412 + 14.14                          = -132132245132120
-132132245132134.1515123765412 + -16.60                         = -132132245132150
-132132245132134.1515123765412 + 0.15                           = -132132245132134
-132132245132134.1515123765412 + -0.01                          = -132132245132134

Number "141241241241241248267654747412" (scale 0)
141241241241241248267654747412 + 15151324141414.412312232141241 = 141241241241241263418978888826
141241241241241248267654747412 + -132132245132134.1515123765412 = 141241241241241116135409615277
141241241241241248267654747412 + 141241241241241248267654747412 = 282482482482482496535309494824
141241241241241248267654747412 + -149143276547656984948124912   = 141092097964693591282706622500
141241241241241248267654747412 + 0.1322135476547459213732911312 = 141241241241241248267654747412
141241241241241248267654747412 + -0.123912932193769965476541321 = 141241241241241248267654747411
141241241241241248267654747412 + 0                              = 141241241241241248267654747412
141241241241241248267654747412 + 0.00                           = 141241241241241248267654747412
141241241241241248267654747412 + -0                             = 141241241241241248267654747412
141241241241241248267654747412 + -0.00                          = 141241241241241248267654747412
141241241241241248267654747412 + 15                             = 141241241241241248267654747427
141241241241241248267654747412 + -15                            = 141241241241241248267654747397
141241241241241248267654747412 + 1                              = 141241241241241248267654747413
141241241241241248267654747412 + -9                             = 141241241241241248267654747403
141241241241241248267654747412 + 14.14                          = 141241241241241248267654747426
141241241241241248267654747412 + -16.60                         = 141241241241241248267654747395
141241241241241248267654747412 + 0.15                           = 141241241241241248267654747412
141241241241241248267654747412 + -0.01                          = 141241241241241248267654747411

Number "-149143276547656984948124912" (scale 0)
-149143276547656984948124912 + 15151324141414.412312232141241 = -149143276547641833623983497
-149143276547656984948124912 + -132132245132134.1515123765412 = -149143276547789117193257046
-149143276547656984948124912 + 141241241241241248267654747412 = 141092097964693591282706622500
-149143276547656984948124912 + -149143276547656984948124912   = -298286553095313969896249824
-149143276547656984948124912 + 0.1322135476547459213732911312 = -149143276547656984948124911
-149143276547656984948124912 + -0.123912932193769965476541321 = -149143276547656984948124912
-149143276547656984948124912 + 0                              = -149143276547656984948124912
-149143276547656984948124912 + 0.00                           = -149143276547656984948124912
-149143276547656984948124912 + -0                             = -149143276547656984948124912
-149143276547656984948124912 + -0.00                          = -149143276547656984948124912
-149143276547656984948124912 + 15                             = -149143276547656984948124897
-149143276547656984948124912 + -15                            = -149143276547656984948124927
-149143276547656984948124912 + 1                              = -149143276547656984948124911
-149143276547656984948124912 + -9                             = -149143276547656984948124921
-149143276547656984948124912 + 14.14                          = -149143276547656984948124897
-149143276547656984948124912 + -16.60                         = -149143276547656984948124928
-149143276547656984948124912 + 0.15                           = -149143276547656984948124911
-149143276547656984948124912 + -0.01                          = -149143276547656984948124912

Number "0.1322135476547459213732911312" (scale 0)
0.1322135476547459213732911312 + 15151324141414.412312232141241 = 15151324141414
0.1322135476547459213732911312 + -132132245132134.1515123765412 = -132132245132134
0.1322135476547459213732911312 + 141241241241241248267654747412 = 141241241241241248267654747412
0.1322135476547459213732911312 + -149143276547656984948124912   = -149143276547656984948124911
0.1322135476547459213732911312 + 0.1322135476547459213732911312 = 0
0.1322135476547459213732911312 + -0.123912932193769965476541321 = 0
0.1322135476547459213732911312 + 0                              = 0
0.1322135476547459213732911312 + 0.00                           = 0
0.1322135476547459213732911312 + -0                             = 0
0.1322135476547459213732911312 + -0.00                          = 0
0.1322135476547459213732911312 + 15                             = 15
0.1322135476547459213732911312 + -15                            = -14
0.1322135476547459213732911312 + 1                              = 1
0.1322135476547459213732911312 + -9                             = -8
0.1322135476547459213732911312 + 14.14                          = 14
0.1322135476547459213732911312 + -16.60                         = -16
0.1322135476547459213732911312 + 0.15                           = 0
0.1322135476547459213732911312 + -0.01                          = 0

Number "-0.123912932193769965476541321" (scale 0)
-0.123912932193769965476541321 + 15151324141414.412312232141241 = 15151324141414
-0.123912932193769965476541321 + -132132245132134.1515123765412 = -132132245132134
-0.123912932193769965476541321 + 141241241241241248267654747412 = 141241241241241248267654747411
-0.123912932193769965476541321 + -149143276547656984948124912   = -149143276547656984948124912
-0.123912932193769965476541321 + 0.1322135476547459213732911312 = 0
-0.123912932193769965476541321 + -0.123912932193769965476541321 = 0
-0.123912932193769965476541321 + 0                              = 0
-0.123912932193769965476541321 + 0.00                           = 0
-0.123912932193769965476541321 + -0                             = 0
-0.123912932193769965476541321 + -0.00                          = 0
-0.123912932193769965476541321 + 15                             = 14
-0.123912932193769965476541321 + -15                            = -15
-0.123912932193769965476541321 + 1                              = 0
-0.123912932193769965476541321 + -9                             = -9
-0.123912932193769965476541321 + 14.14                          = 14
-0.123912932193769965476541321 + -16.60                         = -16
-0.123912932193769965476541321 + 0.15                           = 0
-0.123912932193769965476541321 + -0.01                          = 0

Number "15151324141414.412312232141241" (scale 10)
15151324141414.412312232141241 + 15151324141414.412312232141241 = 30302648282828.8246244642
15151324141414.412312232141241 + -132132245132134.1515123765412 = -116980920990719.7392001443
15151324141414.412312232141241 + 141241241241241248267654747412 = 141241241241241263418978888826.4123122321
15151324141414.412312232141241 + -149143276547656984948124912   = -149143276547641833623983497.5876877678
15151324141414.412312232141241 + 0.1322135476547459213732911312 = 15151324141414.5445257797
15151324141414.412312232141241 + -0.123912932193769965476541321 = 15151324141414.2883992999
15151324141414.412312232141241 + 0                              = 15151324141414.4123122321
15151324141414.412312232141241 + 0.00                           = 15151324141414.4123122321
15151324141414.412312232141241 + -0                             = 15151324141414.4123122321
15151324141414.412312232141241 + -0.00                          = 15151324141414.4123122321
15151324141414.412312232141241 + 15                             = 15151324141429.4123122321
15151324141414.412312232141241 + -15                            = 15151324141399.4123122321
15151324141414.412312232141241 + 1                              = 15151324141415.4123122321
15151324141414.412312232141241 + -9                             = 15151324141405.4123122321
15151324141414.412312232141241 + 14.14                          = 15151324141428.5523122321
15151324141414.412312232141241 + -16.60                         = 15151324141397.8123122321
15151324141414.412312232141241 + 0.15                           = 15151324141414.5623122321
15151324141414.412312232141241 + -0.01                          = 15151324141414.4023122321

Number "-132132245132134.1515123765412" (scale 10)
-132132245132134.1515123765412 + 15151324141414.412312232141241 = -116980920990719.7392001443
-132132245132134.1515123765412 + -132132245132134.1515123765412 = -264264490264268.3030247530
-132132245132134.1515123765412 + 141241241241241248267654747412 = 141241241241241116135409615277.8484876234
-132132245132134.1515123765412 + -149143276547656984948124912   = -149143276547789117193257046.1515123765
-132132245132134.1515123765412 + 0.1322135476547459213732911312 = -132132245132134.0192988288
-132132245132134.1515123765412 + -0.123912932193769965476541321 = -132132245132134.2754253087
-132132245132134.1515123765412 + 0                              = -132132245132134.1515123765
-132132245132134.1515123765412 + 0.00                           = -132132245132134.1515123765
-132132245132134.1515123765412 + -0                             = -132132245132134.1515123765
-132132245132134.1515123765412 + -0.00                          = -132132245132134.1515123765
-132132245132134.1515123765412 + 15                             = -132132245132119.1515123765
-132132245132134.1515123765412 + -15                            = -132132245132149.1515123765
-132132245132134.1515123765412 + 1                              = -132132245132133.1515123765
-132132245132134.1515123765412 + -9                             = -132132245132143.1515123765
-132132245132134.1515123765412 + 14.14                          = -132132245132120.0115123765
-132132245132134.1515123765412 + -16.60                         = -132132245132150.7515123765
-132132245132134.1515123765412 + 0.15                           = -132132245132134.0015123765
-132132245132134.1515123765412 + -0.01                          = -132132245132134.1615123765

Number "141241241241241248267654747412" (scale 10)
141241241241241248267654747412 + 15151324141414.412312232141241 = 141241241241241263418978888826.4123122321
141241241241241248267654747412 + -132132245132134.1515123765412 = 141241241241241116135409615277.8484876234
141241241241241248267654747412 + 141241241241241248267654747412 = 282482482482482496535309494824.0000000000
141241241241241248267654747412 + -149143276547656984948124912   = 141092097964693591282706622500.0000000000
141241241241241248267654747412 + 0.1322135476547459213732911312 = 141241241241241248267654747412.1322135476
141241241241241248267654747412 + -0.123912932193769965476541321 = 141241241241241248267654747411.8760870678
141241241241241248267654747412 + 0                              = 141241241241241248267654747412.0000000000
141241241241241248267654747412 + 0.00                           = 141241241241241248267654747412.0000000000
141241241241241248267654747412 + -0                             = 141241241241241248267654747412.0000000000
141241241241241248267654747412 + -0.00                          = 141241241241241248267654747412.0000000000
141241241241241248267654747412 + 15                             = 141241241241241248267654747427.0000000000
141241241241241248267654747412 + -15                            = 141241241241241248267654747397.0000000000
141241241241241248267654747412 + 1                              = 141241241241241248267654747413.0000000000
141241241241241248267654747412 + -9                             = 141241241241241248267654747403.0000000000
141241241241241248267654747412 + 14.14                          = 141241241241241248267654747426.1400000000
141241241241241248267654747412 + -16.60                         = 141241241241241248267654747395.4000000000
141241241241241248267654747412 + 0.15                           = 141241241241241248267654747412.1500000000
141241241241241248267654747412 + -0.01                          = 141241241241241248267654747411.9900000000

Number "-149143276547656984948124912" (scale 10)
-149143276547656984948124912 + 15151324141414.412312232141241 = -149143276547641833623983497.5876877678
-149143276547656984948124912 + -132132245132134.1515123765412 = -149143276547789117193257046.1515123765
-149143276547656984948124912 + 141241241241241248267654747412 = 141092097964693591282706622500.0000000000
-149143276547656984948124912 + -149143276547656984948124912   = -298286553095313969896249824.0000000000
-149143276547656984948124912 + 0.1322135476547459213732911312 = -149143276547656984948124911.8677864523
-149143276547656984948124912 + -0.123912932193769965476541321 = -149143276547656984948124912.1239129321
-149143276547656984948124912 + 0                              = -149143276547656984948124912.0000000000
-149143276547656984948124912 + 0.00                           = -149143276547656984948124912.0000000000
-149143276547656984948124912 + -0                             = -149143276547656984948124912.0000000000
-149143276547656984948124912 + -0.00                          = -149143276547656984948124912.0000000000
-149143276547656984948124912 + 15                             = -149143276547656984948124897.0000000000
-149143276547656984948124912 + -15                            = -149143276547656984948124927.0000000000
-149143276547656984948124912 + 1                              = -149143276547656984948124911.0000000000
-149143276547656984948124912 + -9                             = -149143276547656984948124921.0000000000
-149143276547656984948124912 + 14.14                          = -149143276547656984948124897.8600000000
-149143276547656984948124912 + -16.60                         = -149143276547656984948124928.6000000000
-149143276547656984948124912 + 0.15                           = -149143276547656984948124911.8500000000
-149143276547656984948124912 + -0.01                          = -149143276547656984948124912.0100000000

Number "0.1322135476547459213732911312" (scale 10)
0.1322135476547459213732911312 + 15151324141414.412312232141241 = 15151324141414.5445257797
0.1322135476547459213732911312 + -132132245132134.1515123765412 = -132132245132134.0192988288
0.1322135476547459213732911312 + 141241241241241248267654747412 = 141241241241241248267654747412.1322135476
0.1322135476547459213732911312 + -149143276547656984948124912   = -149143276547656984948124911.8677864523
0.1322135476547459213732911312 + 0.1322135476547459213732911312 = 0.2644270953
0.1322135476547459213732911312 + -0.123912932193769965476541321 = 0.0083006154
0.1322135476547459213732911312 + 0                              = 0.1322135476
0.1322135476547459213732911312 + 0.00                           = 0.1322135476
0.1322135476547459213732911312 + -0                             = 0.1322135476
0.1322135476547459213732911312 + -0.00                          = 0.1322135476
0.1322135476547459213732911312 + 15                             = 15.1322135476
0.1322135476547459213732911312 + -15                            = -14.8677864523
0.1322135476547459213732911312 + 1                              = 1.1322135476
0.1322135476547459213732911312 + -9                             = -8.8677864523
0.1322135476547459213732911312 + 14.14                          = 14.2722135476
0.1322135476547459213732911312 + -16.60                         = -16.4677864523
0.1322135476547459213732911312 + 0.15                           = 0.2822135476
0.1322135476547459213732911312 + -0.01                          = 0.1222135476

Number "-0.123912932193769965476541321" (scale 10)
-0.123912932193769965476541321 + 15151324141414.412312232141241 = 15151324141414.2883992999
-0.123912932193769965476541321 + -132132245132134.1515123765412 = -132132245132134.2754253087
-0.123912932193769965476541321 + 141241241241241248267654747412 = 141241241241241248267654747411.8760870678
-0.123912932193769965476541321 + -149143276547656984948124912   = -149143276547656984948124912.1239129321
-0.123912932193769965476541321 + 0.1322135476547459213732911312 = 0.0083006154
-0.123912932193769965476541321 + -0.123912932193769965476541321 = -0.2478258643
-0.123912932193769965476541321 + 0                              = -0.1239129321
-0.123912932193769965476541321 + 0.00                           = -0.1239129321
-0.123912932193769965476541321 + -0                             = -0.1239129321
-0.123912932193769965476541321 + -0.00                          = -0.1239129321
-0.123912932193769965476541321 + 15                             = 14.8760870678
-0.123912932193769965476541321 + -15                            = -15.1239129321
-0.123912932193769965476541321 + 1                              = 0.8760870678
-0.123912932193769965476541321 + -9                             = -9.1239129321
-0.123912932193769965476541321 + 14.14                          = 14.0160870678
-0.123912932193769965476541321 + -16.60                         = -16.7239129321
-0.123912932193769965476541321 + 0.15                           = 0.0260870678
-0.123912932193769965476541321 + -0.01                          = -0.1339129321
