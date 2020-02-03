--TEST--
Test strcspn() function : usage variations - with heredoc strings, varying start and default len args
--FILE--
<?php
/* Prototype  : proto int strcspn(string str, string mask [, int start [,int len]])
 * Description: Finds length of initial segment consisting entirely of characters not found in mask.
                If start or/and length is provided works like strcspn(substr($s,$start,$len),$bad_chars)
 * Source code: ext/standard/string.c
 * Alias to functions: none
*/

/*
* Testing strcspn() : with heredoc string, varying start and default len arguments
*/

echo "*** Testing strcspn() : with different start values ***\n";

// initialing required variables
// defining different heredoc strings
$empty_heredoc = <<<EOT
EOT;

$heredoc_with_newline = <<<EOT
\n

EOT;

$heredoc_with_characters = <<<EOT
first line of heredoc string
second line of heredoc string
third line of heredocstring
EOT;

$heredoc_with_newline_and_tabs = <<<EOT
hello\tworld\nhello\nworld\n
EOT;

$heredoc_with_alphanumerics = <<<EOT
hello123world456
1234hello\t1234
EOT;

$heredoc_with_embedded_nulls = <<<EOT
hello\0world\0hello
\0hello\0
EOT;

$heredoc_with_hexa_octal = <<<EOT
hello\0\100\xaaworld\0hello
\0hello\0
EOT;

$heredoc_strings = array(
                   $empty_heredoc,
                   $heredoc_with_newline,
                   $heredoc_with_characters,
                   $heredoc_with_newline_and_tabs,
                   $heredoc_with_alphanumerics,
                   $heredoc_with_embedded_nulls,
           $heredoc_with_hexa_octal
           );


// defining array of mask strings

$mask_array = array(
            "",
            '',
            "\n\trsti \l",
            '\n\trsti \l',
            "\t",
            "t\ ",
            '\t',
            "\t\ ",
            " \t",
                    "\t\i\100\xaa"
                   );

// definig array of start values
$start_array = array(
            0,
            1,
            2,
            -1,
            -2,
            2147483647,  // max positive integer
            -2147483648,  // min negative integer
                   );


// loop through each element of the arrays for str, mask and start arguments

$count = 1;
foreach($heredoc_strings as $str) {
  echo "\n-- Iteration $count --\n";
  foreach($mask_array as $mask) {
    foreach($start_array as $start) {
      var_dump( strcspn($str,$mask,$start) );  // with default len value
    }
  }
  $count++;
}

echo "Done"
?>
--EXPECT--
*** Testing strcspn() : with different start values ***

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
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
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
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)

-- Iteration 3 --
int(86)
int(85)
int(84)
int(1)
int(2)
bool(false)
int(86)
int(86)
int(85)
int(84)
int(1)
int(2)
bool(false)
int(86)
int(1)
int(0)
int(0)
int(1)
int(2)
bool(false)
int(1)
int(1)
int(0)
int(0)
int(1)
int(0)
bool(false)
int(1)
int(86)
int(85)
int(84)
int(1)
int(2)
bool(false)
int(86)
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
int(1)
int(2)
bool(false)
int(4)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)
int(1)
int(0)
int(5)
int(1)
int(2)
bool(false)
int(1)

-- Iteration 4 --
int(24)
int(23)
int(22)
int(1)
int(2)
bool(false)
int(24)
int(24)
int(23)
int(22)
int(1)
int(2)
bool(false)
int(24)
int(2)
int(1)
int(0)
int(0)
int(1)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)
int(24)
int(23)
int(22)
int(1)
int(2)
bool(false)
int(24)
int(24)
int(23)
int(22)
int(1)
int(2)
bool(false)
int(24)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)
int(5)
int(4)
int(3)
int(1)
int(2)
bool(false)
int(5)

-- Iteration 5 --
int(31)
int(30)
int(29)
int(1)
int(2)
bool(false)
int(31)
int(31)
int(30)
int(29)
int(1)
int(2)
bool(false)
int(31)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(26)
int(25)
int(24)
int(1)
int(2)
bool(false)
int(26)
int(31)
int(30)
int(29)
int(1)
int(2)
bool(false)
int(31)
int(31)
int(30)
int(29)
int(1)
int(2)
bool(false)
int(31)
int(26)
int(25)
int(24)
int(1)
int(2)
bool(false)
int(26)
int(26)
int(25)
int(24)
int(1)
int(2)
bool(false)
int(26)
int(26)
int(25)
int(24)
int(1)
int(2)
bool(false)
int(26)

-- Iteration 6 --
int(5)
int(4)
int(3)
int(0)
int(1)
bool(false)
int(5)
int(5)
int(4)
int(3)
int(0)
int(1)
bool(false)
int(5)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)
int(25)
int(24)
int(23)
int(1)
int(2)
bool(false)
int(25)

-- Iteration 7 --
int(5)
int(4)
int(3)
int(0)
int(1)
bool(false)
int(5)
int(5)
int(4)
int(3)
int(0)
int(1)
bool(false)
int(5)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(2)
int(1)
int(0)
int(1)
int(2)
bool(false)
int(2)
int(27)
int(26)
int(25)
int(1)
int(2)
bool(false)
int(27)
int(27)
int(26)
int(25)
int(1)
int(2)
bool(false)
int(27)
int(27)
int(26)
int(25)
int(1)
int(2)
bool(false)
int(27)
int(27)
int(26)
int(25)
int(1)
int(2)
bool(false)
int(27)
int(27)
int(26)
int(25)
int(1)
int(2)
bool(false)
int(27)
int(6)
int(5)
int(4)
int(1)
int(2)
bool(false)
int(6)
Done
