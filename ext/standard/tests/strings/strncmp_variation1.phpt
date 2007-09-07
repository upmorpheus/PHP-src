--TEST--
Test strncmp() function: usage variations - different inputs(alphabet characters)
--FILE--
<?php
/* Prototype  : int strncmp ( string $str1, string $str2, int $len );
 * Description: Binary safe case-sensitive string comparison of the first n characters
 * Source code: Zend/zend_builtin_functions.c
*/

/* Test strncmp() function with upper-case and lower-case alphabets as inputs for 'str1' and 'str2' */

echo "*** Test strncmp() function: with chars ***\n";
echo "-- Passing upper-case letters for 'str1' --\n";
for($ASCII = 65; $ASCII <= 90; $ASCII++) {
  var_dump( strncmp( chr($ASCII), chr($ASCII), 1 ) );  //comparing uppercase letters with uppercase letters; exp: int(0)
  var_dump( strncmp( chr($ASCII), chr($ASCII + 32), 1 ) );  //comparing uppercase letters with lowercase letters; exp: int(-1)
}

echo "\n-- Passing lower-case letters for 'str1' --\n";
for($ASCII = 97; $ASCII <= 122; $ASCII++) {
  var_dump( strncmp( chr($ASCII), chr($ASCII), 1 ) );  //comparing lowercase letters with lowercase letters; exp: int(0)
  var_dump( strncmp( chr($ASCII), chr($ASCII - 32), 1 ) );  //comparing lowercase letters with uppercase letters; exp: int(1)
}
echo "*** Done ***\n";
?>
--EXPECTF--	
*** Test strncmp() function: with chars ***
-- Passing upper-case letters for 'str1' --
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)
int(0)
int(-1)

-- Passing lower-case letters for 'str1' --
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
int(0)
int(1)
*** Done ***
