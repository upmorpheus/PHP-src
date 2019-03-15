--TEST--
Various pow() tests
--FILE--
<?php

define('LONG_MAX', is_int(5000000000)? 9223372036854775807 : 0x7FFFFFFF);
define('LONG_MIN', -LONG_MAX - 1);
printf("%d,%d,%d,%d\n",is_int(LONG_MIN  ),is_int(LONG_MAX  ),
					   is_int(LONG_MIN-1),is_int(LONG_MAX+1));

$tests = <<<TESTS
 0.25 === pow(-2,-2)
-0.5  === pow(-2,-1)
 1    === pow(-2, 0)
-2    === pow(-2, 1)
 4    === pow(-2, 2)
 1.0  === pow(-1,-2)
-1.0  === pow(-1,-1)
 1    === pow(-1, 0)
-1    === pow(-1, 1)
 1    === pow(-1, 2)
 TRUE === is_infinite(pow(0,-2))
 TRUE === is_infinite(pow(0,-1))
 1    === pow( 0, 0)
 0    === pow( 0, 1)
 0    === pow( 0, 2)
 1.0  === pow( 1,-2)
 1.0  === pow( 1,-1)
 1    === pow( 1, 0)
 1    === pow( 1, 1)
 1    === pow( 1, 2)
 0.25 === pow( 2,-2)
 0.5  === pow( 2,-1)
 1    === pow( 2, 0)
 2    === pow( 2, 1)
 4    === pow( 2, 2)
 0.25 === pow(-2,-2.0)
-0.5  === pow(-2,-1.0)
 1.0  === pow(-2, 0.0)
-2.0  === pow(-2, 1.0)
 4.0  === pow(-2, 2.0)
 1.0  === pow(-1,-2.0)
-1.0  === pow(-1,-1.0)
 1.0  === pow(-1, 0.0)
-1.0  === pow(-1, 1.0)
 1.0  === pow(-1, 2.0)
 TRUE === is_infinite(pow(0,-2.0))
 TRUE === is_infinite(pow(0,-1.0))
 1.0  === pow( 0, 0.0)
 0.0  === pow( 0, 1.0)
 0.0  === pow( 0, 2.0)
 1.0  === pow( 1,-2.0)
 1.0  === pow( 1,-1.0)
 1.0  === pow( 1, 0.0)
 1.0  === pow( 1, 1.0)
 1.0  === pow( 1, 2.0)
 0.25 === pow( 2,-2.0)
 0.5  === pow( 2,-1.0)
 1.0  === pow( 2, 0.0)
 2.0  === pow( 2, 1.0)
 4.0  === pow( 2, 2.0)
 2147483648 === pow(2,31)
-2147483648 ~== pow(-2,31)
 1000000000 === pow(10,9)
 100000000  === pow(-10,8)
 1    === pow(-1,1443279822)
-1    === pow(-1,1443279821)
sqrt(2) ~== pow(2,1/2)
 0.25 === pow(-2.0,-2.0)
-0.5  === pow(-2.0,-1.0)
 1.0  === pow(-2.0, 0.0)
-2.0  === pow(-2.0, 1.0)
 4.0  === pow(-2.0, 2.0)
 1.0  === pow(-1.0,-2.0)
-1.0  === pow(-1.0,-1.0)
 1.0  === pow(-1.0, 0.0)
-1.0  === pow(-1.0, 1.0)
 1.0  === pow(-1.0, 2.0)
 TRUE === is_infinite(pow(0.0,-2.0))
 TRUE === is_infinite(pow(0.0,-1.0))
 1.0  === pow( 0.0, 0.0)
 0.0  === pow( 0.0, 1.0)
 0.0  === pow( 0.0, 2.0)
 1.0  === pow( 1.0,-2.0)
 1.0  === pow( 1.0,-1.0)
 1.0  === pow( 1.0, 0.0)
 1.0  === pow( 1.0, 1.0)
 1.0  === pow( 1.0, 2.0)
 0.25 === pow( 2.0,-2.0)
 0.5  === pow( 2.0,-1.0)
 1.0  === pow( 2.0, 0.0)
 2.0  === pow( 2.0, 1.0)
 4.0  === pow( 2.0, 2.0)
 0.25 === pow(-2.0,-2)
-0.5  === pow(-2.0,-1)
 1.0  === pow(-2.0, 0)
-2.0  === pow(-2.0, 1)
 4.0  === pow(-2.0, 2)
 1.0  === pow(-1.0,-2)
-1.0  === pow(-1.0,-1)
 1.0  === pow(-1.0, 0)
-1.0  === pow(-1.0, 1)
 1.0  === pow(-1.0, 2)
 TRUE === is_infinite(pow( 0.0,-2))
 TRUE === is_infinite(pow( 0.0,-1))
 1.0  === pow( 0.0, 0)
 0.0  === pow( 0.0, 1)
 0.0  === pow( 0.0, 2)
 1.0  === pow( 1.0,-2)
 1.0  === pow( 1.0,-1)
 1.0  === pow( 1.0, 0)
 1.0  === pow( 1.0, 1)
 1.0  === pow( 1.0, 2)
 0.25 === pow( 2.0,-2)
 0.5  === pow( 2.0,-1)
 1.0  === pow( 2.0, 0)
 2.0  === pow( 2.0, 1)
 4.0  === pow( 2.0, 2)
 2.0  === pow(   4, 0.5)
 2.0  === pow( 4.0, 0.5)
 3.0  === pow(  27, 1/3)
 3.0  === pow(27.0, 1/3)
 0.5  === pow(   4, -0.5)
 0.5  === pow( 4.0, -0.5)
LONG_MAX-1 === pow(LONG_MAX-1,1)
LONG_MIN+1 === pow(LONG_MIN+1,1)
(LONG_MAX-1)*(LONG_MAX-1) ~== pow(LONG_MAX-1,2)
(LONG_MIN+1)*(LONG_MIN+1) ~== pow(LONG_MIN+1,2)
(float)(LONG_MAX-1) === pow(LONG_MAX-1,1.0)
(float)(LONG_MIN+1) === pow(LONG_MIN+1,1.0)
(LONG_MAX-1)*(LONG_MAX-1) ~== pow(LONG_MAX-1,2.0)
(LONG_MIN+1)*(LONG_MIN+1) ~== pow(LONG_MIN+1,2.0)
LONG_MAX === pow(LONG_MAX,1)
LONG_MIN === pow(LONG_MIN,1)
LONG_MAX*LONG_MAX ~== pow(LONG_MAX,2)
LONG_MIN*LONG_MIN ~== pow(LONG_MIN,2)
(float)LONG_MAX === pow(LONG_MAX,1.0)
(float)LONG_MIN === pow(LONG_MIN,1.0)
LONG_MAX*LONG_MAX ~== pow(LONG_MAX,2.0)
LONG_MIN*LONG_MIN ~== pow(LONG_MIN,2.0)
TESTS;

 echo "On failure, please mail result to php-dev@lists.php.net\n";
 include(__DIR__ . '/../../../../tests/quicktester.inc');
--EXPECT--
1,1,0,0
On failure, please mail result to php-dev@lists.php.net
OK
