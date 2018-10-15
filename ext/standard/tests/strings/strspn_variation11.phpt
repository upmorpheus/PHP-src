--TEST--
Test strspn() function : usage variations - with varying start and default len args
--FILE--
<?php
/* Prototype  : proto int strspn(string str, string mask [, int start [, int len]])
 * Description: Finds length of initial segment consisting entirely of characters found in mask.
                If start or/and length is provided works like strspn(substr($s,$start,$len),$good_chars)
 * Source code: ext/standard/string.c
 * Alias to functions: none
*/

/*
* Testing strspn() : with varying start and default len arguments
*/

echo "*** Testing strspn() : with different start and default len values ***\n";

// initialing required variables
// defining different strings
$strings = array(
                   "",
		   '',
		   "\n",
		   '\n',
		   "hello\tworld\nhello\nworld\n",
		   'hello\tworld\nhello\nworld\n',
 		   "1234hello45world\t123",
 		   '1234hello45world\t123',
		   "hello\0world\012",
		   'hello\0world\012',
		   chr(0).chr(0),
		   chr(0)."hello\0world".chr(0),
		   chr(0).'hello\0world'.chr(0),
		   "hello".chr(0)."world",
		   'hello'.chr(0).'world',
		   "hello\0\100\xaaaworld",
		   'hello\0\100\xaaaworld'
                   );

// define the array of mask strings
$mask_array = array(
		    "",
		    '',
		    "f\n\trelshti \l",
		    'f\n\trelsthi \l',
		    "\telh",
		    "t\ ",
		    '\telh',
		    "felh\t\ ",
		    " \t",
                    "fhel\t\i\100\xa"
                   );

// defining the array for start values
$start_array = array(
		    0,
		    1,
                    2,
		    -1,
		    -2,
		    2147483647,  // max positive integer
		    -2147483648,  // min negative integer
                   );


// loop through each element of the arrays for str, mask and start argument
$count = 1;
foreach($strings as $str) {
  echo "\n-- Iteration $count --\n";
  foreach($mask_array as $mask) {
    foreach($start_array as $start) {
      var_dump( strspn($str,$mask,$start) );
    }
  }
  $count++;
}

echo "Done"
?>
--EXPECT--
*** Testing strspn() : with different start and default len values ***

-- Iteration 1 --
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 2 --
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
bool(false)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 3 --
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
int(0)
bool(false)
int(1)
int(1)
bool(false)
int(1)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
int(0)
bool(false)
int(1)
int(1)
bool(false)
int(1)

-- Iteration 4 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(1)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(1)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(1)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(1)
int(1)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(1)
int(1)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(1)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(1)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(1)

-- Iteration 5 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(1)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(1)
int(0)
bool(false)
int(4)

-- Iteration 6 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(1)
int(2)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(1)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)

-- Iteration 7 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 8 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 9 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(1)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(1)
int(0)
bool(false)
int(4)

-- Iteration 10 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(0)
bool(false)
int(4)

-- Iteration 11 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 12 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 13 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(4)
int(3)
int(0)
int(0)
bool(false)
int(0)

-- Iteration 14 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)

-- Iteration 15 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)

-- Iteration 16 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)

-- Iteration 17 --
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
int(0)
int(0)
int(0)
int(0)
int(0)
bool(false)
int(0)
int(4)
int(3)
int(2)
int(0)
int(1)
bool(false)
int(4)
Done
