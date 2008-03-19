--TEST--
Test realpath() with relative paths
--FILE--
<?php
/* Prototype: string realpath ( string $path );
   Description: Returns canonicalized absolute pathname
*/

echo "\n*** Testing basic functions of realpath() with files ***\n";

/* creating directories and files */
$file_path = dirname(__FILE__);
mkdir("$file_path/realpath_basic/home/test/", 0777, true);

$file_handle1 = fopen("$file_path/realpath_basic/home/test/realpath_basic.tmp", "w");
$file_handle2 = fopen("$file_path/realpath_basic/home/realpath_basic.tmp", "w");
$file_handle3 = fopen("$file_path/realpath_basic/realpath_basic.tmp", "w");
fclose($file_handle1);
fclose($file_handle2);
fclose($file_handle3);

echo "\n*** Testing realpath() on filenames ***\n";
$filenames = array (
  /* filenames resulting in valid paths */
  "./realpath_basic/home/realpath_basic.tmp",
  "./realpath_basic/realpath_basic.tmp",
  "./realpath_basic//home/test//../test/./realpath_basic.tmp",
  "./realpath_basic/home//../././realpath_basic.tmp",

  /* filenames with invalid path */
  // checking for binary safe
  "./realpath_basicx000/home/realpath_basic.tmp",

  ".///realpath_basic/home//..//././test//realpath_basic.tmp",
  "./realpath_basic/home/../home/../test/..realpath_basic.tmp"
);

chdir("$file_path/..");
chdir($file_path);

$counter = 1;
/* loop through $files to read the filepath of $file in the above array */
foreach($filenames as $file) {
  echo "\n-- Iteration $counter --\n";
  var_dump( realpath($file) );
  $counter++;
}

echo "Done\n";
?>
--CLEAN--
<?php
$name_prefix = dirname(__FILE__)."/realpath_basic";
unlink("$name_prefix/home/test/realpath_basic.tmp");
unlink("$name_prefix/home/realpath_basic.tmp");
unlink("$name_prefix/realpath_basic.tmp");
rmdir("$name_prefix/home/test/");
rmdir("$name_prefix/home/");
rmdir("$name_prefix/");
?>
--EXPECTF--
*** Testing basic functions of realpath() with files ***

*** Testing realpath() on filenames ***

-- Iteration 1 --
string(%d) "%srealpath_basic%shome%srealpath_basic.tmp"

-- Iteration 2 --
string(%d) "%srealpath_basic%srealpath_basic.tmp"

-- Iteration 3 --
string(%d) "%srealpath_basic%shome%stest%srealpath_basic.tmp"

-- Iteration 4 --
string(%d) "%srealpath_basic%srealpath_basic.tmp"

-- Iteration 5 --
bool(false)

-- Iteration 6 --
bool(false)

-- Iteration 7 --
bool(false)
Done
--UEXPECTF--
*** Testing basic functions of realpath() with files ***

*** Testing realpath() on filenames ***

-- Iteration 1 --
unicode(%d) "%srealpath_basic%shome%srealpath_basic.tmp"

-- Iteration 2 --
unicode(%d) "%srealpath_basic%srealpath_basic.tmp"

-- Iteration 3 --
unicode(%d) "%srealpath_basic%shome%stest%srealpath_basic.tmp"

-- Iteration 4 --
unicode(%d) "%srealpath_basic%srealpath_basic.tmp"

-- Iteration 5 --
bool(false)

-- Iteration 6 --
bool(false)

-- Iteration 7 --
bool(false)
Done