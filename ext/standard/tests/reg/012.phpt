--TEST--
nonexisting back reference
--POST--
--GET--
--FILE--
<?php $a="abc123";
  echo ereg_replace("123",'def\1ghi',$a)?>
--EXPECT--
abcdef\1ghi
