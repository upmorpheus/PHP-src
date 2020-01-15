--TEST--
Scripts with flex-incompatible encoding without suitable conversion strategy
--SKIPIF--
<?php
if (extension_loaded("mbstring")) {
  die("skip The mbstring extension cannot be present for this test");
}
?>
--INI--
zend.multibyte=1
--FILE--
��  <   ?   p   h   p   
   e   c   h   o       "   h   e   l   l   o       w   o   r   l   d   "   ;   
   
--EXPECT--
Fatal error: Could not convert the script from the detected encoding "UTF-32LE" to a compatible encoding in Unknown on line 0
