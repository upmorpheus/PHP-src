--TEST--
Test fseek(), ftell() & rewind() functions : usage variations - all r & a modes, SEEK_CUR
--FILE--
<?php
/* Prototype: int fseek ( resource $handle, int $offset [, int $whence] );
   Description: Seeks on a file pointer

   Prototype: bool rewind ( resource $handle );
   Description: Rewind the position of a file pointer

   Prototype: int ftell ( resource $handle );
   Description: Tells file pointer read/write position
*/

// include the file.inc for common functions for test
include ("file.inc");

/* Testing fseek(),ftell(),rewind() functions
     1. All read and append modes
     2. Testing fseek() with whence = SEEK_CUR
*/
echo "*** Testing fseek(), ftell(), rewind() : whence = SEEK_CUR & all r and a modes ***\n";

$file_modes = array( "r","rb","rt","r+","r+b","r+t",
                     "a","ab","at","a+","a+b","a+t");
$file_content_types = array( "text_with_new_line","alphanumeric");

$offset = array(-1, 0, 1, 512, 600);// different offsets

$filename = __DIR__."/fseek_ftell_rewind_variation5.tmp"; // this is name of the file created by create_files()

/* open the file using $files_modes and perform fseek(),ftell() and rewind() on it */
foreach($file_content_types as $file_content_type){
  echo "-- File having data of type ". $file_content_type ." --\n";

  foreach($file_modes as $file_mode) {
    echo "-- File opened in mode ".$file_mode." --\n";
    create_files ( __DIR__, 1, $file_content_type, 0755, 512, "w", "fseek_ftell_rewind_variation"
                      ,5,"bytes",".tmp"); //create a file with 512 bytes size
    $file_handle = fopen($filename, $file_mode);
    if (!$file_handle) {
      echo "Error: failed to fopen() file: $filename!";
      exit();
    }
    rewind($file_handle);
    foreach($offset as $count){
      var_dump( fseek($file_handle,$count,SEEK_CUR) );
      var_dump( ftell($file_handle) ); // confirm the file pointer position
      var_dump( feof($file_handle) ); //ensure that file pointer is not at end
    } //end of offset loop

    //close the file and check the size
    fclose($file_handle);
    var_dump( filesize($filename) );

    delete_file($filename); // delete file with name
  } //end of file_mode loop
} //end of file_content_types loop

echo "Done\n";
?>
--EXPECT--
*** Testing fseek(), ftell(), rewind() : whence = SEEK_CUR & all r and a modes ***
-- File having data of type text_with_new_line --
-- File opened in mode r --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode rb --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode rt --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+ --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+b --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+t --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode ab --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode at --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+ --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+b --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+t --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File having data of type alphanumeric --
-- File opened in mode r --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode rb --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode rt --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+ --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+b --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode r+t --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode ab --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode at --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+ --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+b --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
-- File opened in mode a+t --
int(-1)
int(0)
bool(false)
int(0)
int(0)
bool(false)
int(0)
int(1)
bool(false)
int(0)
int(513)
bool(false)
int(0)
int(1113)
bool(false)
int(512)
Done
