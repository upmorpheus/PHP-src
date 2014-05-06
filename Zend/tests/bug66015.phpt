--TEST--
Bug #66015 (wrong array indexing in class's static property)
--FILE--
<?php
class Test
{
   const FIRST = 1;
   const SECOND = 2;
   const THIRD = 3;

   protected static $array = [
       self::FIRST => 'first',
       'second',
       'third'
   ];

   public function __construct()
   {
       var_export(self::$array);
   }
}

$test = new Test();
?>
--EXPECTF--
array (
  1 => 'first',
  2 => 'second',
  3 => 'third',
)
