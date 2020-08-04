--TEST--
Test hypot() - basic function test hypot()
--INI--
serialize_precision=15
--FILE--
<?php

echo "*** Testing hypot() : basic functionality ***\n";

$valuesy = array(23,
                -23,
                2.345e1,
                -2.345e1,
                0x17,
                027,
                "23",
                "23.45",
                "2.345e1",
                null,
                true,
                false);

$valuesx = array(33,
                -33,
                3.345e1,
                -3.345e1,
                0x27,
                037,
                "33",
                "43.45",
                "1.345e1",
                null,
                true,
                false);

for ($i = 0; $i < count($valuesy); $i++) {
    for ($j = 0; $j < count($valuesx); $j++) {
        echo "\nY:$valuesy[$i] X:$valuesx[$j] ";
        $res = hypot($valuesy[$i], $valuesx[$j]);
        var_dump($res);
    }
}
?>
--EXPECT--
*** Testing hypot() : basic functionality ***

Y:23 X:33 float(40.2243707222375)

Y:23 X:-33 float(40.2243707222375)

Y:23 X:33.45 float(40.5943653725489)

Y:23 X:-33.45 float(40.5943653725489)

Y:23 X:39 float(45.2769256906871)

Y:23 X:31 float(38.6005181312376)

Y:23 X:33 float(40.2243707222375)

Y:23 X:43.45 float(49.1620026036369)

Y:23 X:1.345e1 float(26.6439955712352)

Y:23 X: float(23)

Y:23 X:1 float(23.0217288664427)

Y:23 X: float(23)

Y:-23 X:33 float(40.2243707222375)

Y:-23 X:-33 float(40.2243707222375)

Y:-23 X:33.45 float(40.5943653725489)

Y:-23 X:-33.45 float(40.5943653725489)

Y:-23 X:39 float(45.2769256906871)

Y:-23 X:31 float(38.6005181312376)

Y:-23 X:33 float(40.2243707222375)

Y:-23 X:43.45 float(49.1620026036369)

Y:-23 X:1.345e1 float(26.6439955712352)

Y:-23 X: float(23)

Y:-23 X:1 float(23.0217288664427)

Y:-23 X: float(23)

Y:23.45 X:33 float(40.483360779461)

Y:23.45 X:-33 float(40.483360779461)

Y:23.45 X:33.45 float(40.8510097794412)

Y:23.45 X:-33.45 float(40.8510097794412)

Y:23.45 X:39 float(45.5071697647744)

Y:23.45 X:31 float(38.8703293014093)

Y:23.45 X:33 float(40.483360779461)

Y:23.45 X:43.45 float(49.3741329037787)

Y:23.45 X:1.345e1 float(27.033405260899)

Y:23.45 X: float(23.45)

Y:23.45 X:1 float(23.4713122769052)

Y:23.45 X: float(23.45)

Y:-23.45 X:33 float(40.483360779461)

Y:-23.45 X:-33 float(40.483360779461)

Y:-23.45 X:33.45 float(40.8510097794412)

Y:-23.45 X:-33.45 float(40.8510097794412)

Y:-23.45 X:39 float(45.5071697647744)

Y:-23.45 X:31 float(38.8703293014093)

Y:-23.45 X:33 float(40.483360779461)

Y:-23.45 X:43.45 float(49.3741329037787)

Y:-23.45 X:1.345e1 float(27.033405260899)

Y:-23.45 X: float(23.45)

Y:-23.45 X:1 float(23.4713122769052)

Y:-23.45 X: float(23.45)

Y:23 X:33 float(40.2243707222375)

Y:23 X:-33 float(40.2243707222375)

Y:23 X:33.45 float(40.5943653725489)

Y:23 X:-33.45 float(40.5943653725489)

Y:23 X:39 float(45.2769256906871)

Y:23 X:31 float(38.6005181312376)

Y:23 X:33 float(40.2243707222375)

Y:23 X:43.45 float(49.1620026036369)

Y:23 X:1.345e1 float(26.6439955712352)

Y:23 X: float(23)

Y:23 X:1 float(23.0217288664427)

Y:23 X: float(23)

Y:23 X:33 float(40.2243707222375)

Y:23 X:-33 float(40.2243707222375)

Y:23 X:33.45 float(40.5943653725489)

Y:23 X:-33.45 float(40.5943653725489)

Y:23 X:39 float(45.2769256906871)

Y:23 X:31 float(38.6005181312376)

Y:23 X:33 float(40.2243707222375)

Y:23 X:43.45 float(49.1620026036369)

Y:23 X:1.345e1 float(26.6439955712352)

Y:23 X: float(23)

Y:23 X:1 float(23.0217288664427)

Y:23 X: float(23)

Y:23 X:33 float(40.2243707222375)

Y:23 X:-33 float(40.2243707222375)

Y:23 X:33.45 float(40.5943653725489)

Y:23 X:-33.45 float(40.5943653725489)

Y:23 X:39 float(45.2769256906871)

Y:23 X:31 float(38.6005181312376)

Y:23 X:33 float(40.2243707222375)

Y:23 X:43.45 float(49.1620026036369)

Y:23 X:1.345e1 float(26.6439955712352)

Y:23 X: float(23)

Y:23 X:1 float(23.0217288664427)

Y:23 X: float(23)

Y:23.45 X:33 float(40.483360779461)

Y:23.45 X:-33 float(40.483360779461)

Y:23.45 X:33.45 float(40.8510097794412)

Y:23.45 X:-33.45 float(40.8510097794412)

Y:23.45 X:39 float(45.5071697647744)

Y:23.45 X:31 float(38.8703293014093)

Y:23.45 X:33 float(40.483360779461)

Y:23.45 X:43.45 float(49.3741329037787)

Y:23.45 X:1.345e1 float(27.033405260899)

Y:23.45 X: float(23.45)

Y:23.45 X:1 float(23.4713122769052)

Y:23.45 X: float(23.45)

Y:2.345e1 X:33 float(40.483360779461)

Y:2.345e1 X:-33 float(40.483360779461)

Y:2.345e1 X:33.45 float(40.8510097794412)

Y:2.345e1 X:-33.45 float(40.8510097794412)

Y:2.345e1 X:39 float(45.5071697647744)

Y:2.345e1 X:31 float(38.8703293014093)

Y:2.345e1 X:33 float(40.483360779461)

Y:2.345e1 X:43.45 float(49.3741329037787)

Y:2.345e1 X:1.345e1 float(27.033405260899)

Y:2.345e1 X: float(23.45)

Y:2.345e1 X:1 float(23.4713122769052)

Y:2.345e1 X: float(23.45)

Y: X:33 float(33)

Y: X:-33 float(33)

Y: X:33.45 float(33.45)

Y: X:-33.45 float(33.45)

Y: X:39 float(39)

Y: X:31 float(31)

Y: X:33 float(33)

Y: X:43.45 float(43.45)

Y: X:1.345e1 float(13.45)

Y: X: float(0)

Y: X:1 float(1)

Y: X: float(0)

Y:1 X:33 float(33.0151480384384)

Y:1 X:-33 float(33.0151480384384)

Y:1 X:33.45 float(33.4649443447916)

Y:1 X:-33.45 float(33.4649443447916)

Y:1 X:39 float(39.0128184062623)

Y:1 X:31 float(31.0161248385416)

Y:1 X:33 float(33.0151480384384)

Y:1 X:43.45 float(43.4615059564208)

Y:1 X:1.345e1 float(13.4871234887206)

Y:1 X: float(1)

Y:1 X:1 float(1.4142135623731)

Y:1 X: float(1)

Y: X:33 float(33)

Y: X:-33 float(33)

Y: X:33.45 float(33.45)

Y: X:-33.45 float(33.45)

Y: X:39 float(39)

Y: X:31 float(31)

Y: X:33 float(33)

Y: X:43.45 float(43.45)

Y: X:1.345e1 float(13.45)

Y: X: float(0)

Y: X:1 float(1)

Y: X: float(0)
