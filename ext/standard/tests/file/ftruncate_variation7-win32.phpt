--TEST--
Test ftruncate() function : usage variations - truncate when file pointer at EOF
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) != 'WIN') {
    die('skip.. only valid for Windows');
}
?>
--FILE--
<?php
/*
 Prototype: bool ftruncate ( resource $handle, int $size );
 Description: Truncates a file to a given length
*/

/* truncate the file when file pointer is positioned at end of the file */
// include common file related test functions
include ("file.inc");

echo "*** Testing ftruncate() : usage variations ***\n";

/* test ftruncate with file opened in different modes */
$file_modes = array("r", "rb", "rt", "r+", "r+b", "r+t",
                    "w", "wb", "wt", "w+", "w+b", "w+t",
                    "x", "xb", "xt", "x+", "x+b", "x+t",
                    "a", "ab", "at", "a+", "a+b", "a+t");

$file_content_types = array("numeric","text_with_new_line");

foreach($file_content_types as $file_content_type) {
 echo "\n-- Testing ftruncate() with file having data of type ". $file_content_type ." --\n";

 for($mode_counter = 0; $mode_counter < count($file_modes); $mode_counter++) {
  echo "-- Testing ftruncate() with file opening using $file_modes[$mode_counter] mode --\n";

   // create 1 file with some contents
   $filename = dirname(__FILE__)."/ftruncate_variation7.tmp";
   if( strstr($file_modes[$mode_counter], "x") || strstr($file_modes[$mode_counter], "w") ) {
     // fopen the file using the $file_modes
     $file_handle = fopen($filename, $file_modes[$mode_counter]);
     fill_file($file_handle, $file_content_type, 1024);
   } else {
     create_files ( dirname(__FILE__), 1, $file_content_type, 0755, 1, "w", "ftruncate_variation", 7);
     // fopen the file using the $file_modes
     $file_handle = fopen($filename, $file_modes[$mode_counter]);
   }
   if (!$file_handle) {
     echo "Error: failed to open file $filename!\n";
     exit();
   }

   rewind($file_handle); // file pointer to 0

   echo "-- Testing ftruncate(): File pointer at the end --\n";
   /* try to truncate it to while file pointer at the end */
   fseek($file_handle, 0, SEEK_END);
   $new_size = 200;
   var_dump( filesize($filename) );  // current filesize
   var_dump( ftell($file_handle) );
   var_dump( ftruncate($file_handle, $new_size) ); // truncate it
   var_dump( ftell($file_handle) );
   var_dump( feof($file_handle) );
   fclose($file_handle);
   clearstatcache(); // clear previous size value in cache
   var_dump( filesize($filename) );

   //delete all files created
   delete_file($filename);
 }//end of inner for loop
}//end of outer foreach loop
echo "Done\n";
?>
--EXPECTF--
*** Testing ftruncate() : usage variations ***

-- Testing ftruncate() with file having data of type numeric --
-- Testing ftruncate() with file opening using r mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using rb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using rt mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using r+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using r+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using r+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using wb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using wt mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using xb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using xt mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using ab mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using at mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)

-- Testing ftruncate() with file having data of type text_with_new_line --
-- Testing ftruncate() with file opening using r mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using rb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using rt mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(false)
int(1024)
bool(false)
int(1024)
-- Testing ftruncate() with file opening using r+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using r+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using r+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using wb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using wt mode --
-- Testing ftruncate(): File pointer at the end --
int(1137)
int(1137)
bool(true)
int(1137)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using w+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1137)
int(1137)
bool(true)
int(1137)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using xb mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using xt mode --
-- Testing ftruncate(): File pointer at the end --
int(1137)
int(1137)
bool(true)
int(1137)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using x+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1137)
int(1137)
bool(true)
int(1137)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using ab mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using at mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+ mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+b mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
-- Testing ftruncate() with file opening using a+t mode --
-- Testing ftruncate(): File pointer at the end --
int(1024)
int(1024)
bool(true)
int(1024)
bool(false)
int(200)
Done
