--TEST--
bzcompress()/bzdecompress() tests
--SKIPIF--
<?php if (!extension_loaded("bz2")) print "skip"; ?>
--FILE--
<?php

$string = "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else";

var_dump(bzcompress());
/* Having newlines confuses EXPECTF */
var_dump(str_replace(array(b"\n", b"\r"), array(b"\\n", b"\\r"), bzcompress(1,1,1)));
var_dump(bzcompress($string, 100));
var_dump(bzcompress($string, 100, -1));
var_dump(bzcompress($string, 100, 1000));
var_dump(bzcompress($string, -1, 1));

$data = bzcompress($string);
$data2 = bzcompress($string, 1, 10);

$data3 = $data2;
$data3[3] = b"0";

var_dump(bzdecompress());
var_dump(bzdecompress(1,1,1));
var_dump(bzdecompress(1,1));
var_dump(bzdecompress($data3));
var_dump(bzdecompress($data3,1));

var_dump(bzdecompress($data, -1));
var_dump(bzdecompress($data, 0));
var_dump(bzdecompress($data, 1000));
var_dump(bzdecompress($data));
var_dump(bzdecompress($data2));

echo "Done\n";
?>
--EXPECTF--	
Warning: bzcompress() expects at least 1 parameter, 0 given in %s on line %d
NULL
string(%d) "BZ%s"
int(-2)
int(-2)
int(-2)
int(-2)

Warning: bzdecompress() expects at least 1 parameter, 0 given in %s on line %d
bool(false)

Warning: bzdecompress() expects at most 2 parameters, 3 given in %s on line %d
bool(false)
int(-5)
int(-5)
int(-5)
bool(false)
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
bool(false)
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
Done
--UEXPECTF--
Warning: bzcompress() expects at least 1 parameter, 0 given in %s on line %d
NULL
unicode(%d) "BZ%s"
int(-2)
int(-2)
int(-2)
int(-2)

Warning: bzdecompress() expects at least 1 parameter, 0 given in %s on line %d
bool(false)

Warning: bzdecompress() expects at most 2 parameters, 3 given in %s on line %d
bool(false)
int(-5)
int(-5)
int(-5)
bool(false)
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
bool(false)
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
string(110) "Life it seems, will fade away
Drifting further everyday
Getting lost within myself
Nothing matters no one else"
Done
