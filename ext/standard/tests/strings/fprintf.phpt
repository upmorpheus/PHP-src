--TEST--
Test fprintf() function
--SKIPIF--
<?php

$data_file = dirname(__FILE__) . '/dump.txt';
if !($fp = fopen($data_file, 'w')) {
  die('skip File dump.txt could not be created');
}
  
?>
--FILE--
<?php

/* Prototype: int fprintf( resource handle, string format[, mixed args [, mixed ...]] )
 * Description: Write a formatted string to a stream
 */

$float_variation = array( "%f","%-f", "%+f", "%7.2f", "%-7.2f", "%07.2f", "%-07.2f", "%'#7.2f" );
$float_numbers = array( 0, 1, -1, 0.32, -0.32, 3.4. -3.4, 2.54, -2.54 );

$int_variation = array( "%d", "%-d", "%+d", "%7.2d", "%-7.2d", "%07.2d", "%-07.2d", "%'#7.2d" );
$int_numbers = array( 0, 1, -1, 2.7, -2.7, 23333333, -23333333, "1234" );

$char_variation = array( 'a', "a", 67, -67, 99 );

$string_variation = array( "%5s", "%-5s", "%05s", "%'#5s" );
$strings = array( NULL, "abc", 'aaa' );

/* creating dumping file */
$data_file = dirname(__FILE__) . '/dump.txt';
if (!($fp = fopen($data_file, 'wt')))
   return;

/* Testing Error Conditions */
echo "*** Testing Error Conditions ***\n";

/* zero argument */
var_dump( fprintf() );

/* scalar argument */
var_dump( fprintf(3) );

/* NULL argument */
var_dump( fprintf(NULL) );

$counter = 1;
/* float type variations */
fprintf($fp, "\n*** Testing fprintf() with floats ***\n");

foreach( $float_variation as $float_var ) {
  fprintf( $fp, "\n-- Iteration %d --\n",$counter);
  foreach( $float_numbers as $float_num ) {
    fprintf( $fp, "\n");
    fprintf( $fp, $float_var, $float_num );
  }
  $counter++;
}

$counter = 1;
/* integer type variations */
fprintf($fp, "\n*** Testing fprintf() with integers ***\n");
foreach( $int_variation as $int_var ) {
  fprintf( $fp, "\n-- Iteration %d --\n",$counter);
  foreach( $int_numbers as $int_num ) {
    fprintf( $fp, "\n");
    fprintf( $fp, $int_var, $int_num );
  }
  $counter++;
}

/* binary type variations */
fprintf($fp, "\n*** Testing fprintf() with binary ***\n");
foreach( $int_numbers as $bin_num ) {
  fprintf( $fp, "\n");
  fprintf( $fp, "%b", $bin_num );
}

/* char type variations */
fprintf($fp, "\n*** Testing fprintf() for chars ***\n");
foreach( $char_variation as $char ) {
  fprintf( $fp, "\n");
  fprintf( $fp,"%c", $char );
}

/* %e type variations */
fprintf($fp, "\n*** Testing fprintf() for scientific type ***\n");
foreach( $int_numbers as $num ) {
  fprintf( $fp, "\n");
  fprintf( $fp, "%e", $num );
}

/* unsigned int type variation */
fprintf($fp, "\n*** Testing fprintf() for unsigned integers ***\n");
foreach( $int_numbers as $unsig_num ) {
  fprintf( $fp, "\n");
  fprintf( $fp, "%u", $unsig_num );
}

/* octal type variations */
fprintf($fp, "\n*** Testing fprintf() for octals ***\n");
foreach( $int_numbers as $octal_num ) {
 fprintf( $fp, "\n");
 fprintf( $fp, "%o", $octal_num );
}

/* hexadecimal type variations */
fprintf($fp, "\n*** Testing fprintf() for hexadecimals ***\n");
foreach( $int_numbers as $hexa_num ) {
 fprintf( $fp, "\n");
 fprintf( $fp, "%x", $hexa_num );
}

$counter = 1;
/* string type variations */
fprintf($fp, "\n*** Testing fprintf() for string types ***\n");
foreach( $string_variation as $string_var ) {
  fprintf( $fp, "\n-- Iteration %d --\n",$counter);
  foreach( $strings as $str ) {
    fprintf( $fp, "\n");
    fprintf( $fp, $string_var, $str );
  }
  $counter++;
}

fclose($fp);

print_r(file_get_contents($data_file));
echo "\nDone";

unlink($data_file);

?>
--EXPECTF--
*** Testing Error Conditions ***

Warning: Wrong parameter count for fprintf() in %s on line %d
NULL

Warning: Wrong parameter count for fprintf() in %s on line %d
NULL

Warning: Wrong parameter count for fprintf() in %s on line %d
NULL

*** Testing fprintf() with floats ***

-- Iteration 1 --

0.000000
1.000000
-1.000000
0.320000
-0.320000
3.400000
2.540000
-2.540000
-- Iteration 2 --

0.000000
1.000000
-1.000000
0.320000
-0.320000
3.400000
2.540000
-2.540000
-- Iteration 3 --

+0.000000
+1.000000
-1.000000
+0.320000
-0.320000
+3.400000
+2.540000
-2.540000
-- Iteration 4 --

   0.00
   1.00
  -1.00
   0.32
  -0.32
   3.40
   2.54
  -2.54
-- Iteration 5 --

0.00   
1.00   
-1.00  
0.32   
-0.32  
3.40   
2.54   
-2.54  
-- Iteration 6 --

0000.00
0001.00
-001.00
0000.32
-000.32
0003.40
0002.54
-002.54
-- Iteration 7 --

0.00000
1.00000
-1.0000
0.32000
-0.3200
3.40000
2.54000
-2.5400
-- Iteration 8 --

###0.00
###1.00
##-1.00
###0.32
##-0.32
###3.40
###2.54
##-2.54
*** Testing fprintf() with integers ***

-- Iteration 1 --

0
1
-1
2
-2
23333333
-23333333
1234
-- Iteration 2 --

0
1
-1
2
-2
23333333
-23333333
1234
-- Iteration 3 --

+0
+1
-1
+2
-2
+23333333
-23333333
+1234
-- Iteration 4 --

      0
      1
     -1
      2
     -2
23333333
-23333333
   1234
-- Iteration 5 --

0      
1      
-1     
2      
-2     
23333333
-23333333
1234   
-- Iteration 6 --

0000000
0000001
-000001
0000002
-000002
23333333
-23333333
0001234
-- Iteration 7 --

0      
1      
-1     
2      
-2     
23333333
-23333333
1234   
-- Iteration 8 --

######0
######1
#####-1
######2
#####-2
23333333
-23333333
###1234
*** Testing fprintf() with binary ***

0
1
11111111111111111111111111111111
10
11111111111111111111111111111110
1011001000000100111010101
11111110100110111111011000101011
10011010010
*** Testing fprintf() for chars ***

 
 
C
�
c
*** Testing fprintf() for scientific type ***

0.000000e+0
1.000000e+0
-1.000000e+0
2.700000e+0
-2.700000e+0
2.333333e+7
-2.333333e+7
1.234000e+3
*** Testing fprintf() for unsigned integers ***

0
1
4294967295
2
4294967294
23333333
4271633963
1234
*** Testing fprintf() for octals ***

0
1
37777777777
2
37777777776
131004725
37646773053
2322
*** Testing fprintf() for hexadecimals ***

0
1
ffffffff
2
fffffffe
16409d5
fe9bf62b
4d2
*** Testing fprintf() for string types ***

-- Iteration 1 --

     
  abc
  aaa
-- Iteration 2 --

     
abc  
aaa  
-- Iteration 3 --

00000
00abc
00aaa
-- Iteration 4 --

#####
##abc
##aaa
Done
