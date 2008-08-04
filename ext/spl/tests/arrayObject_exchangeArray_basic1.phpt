--TEST--
SPL: ArrayObject::exchangeArray() and copy-on-write references
--FILE--
<?php
$ao = new ArrayObject();
$swapIn = array();
$cowRef = $swapIn; // create a copy-on-write ref to $swapIn
$ao->exchangeArray($swapIn);

$ao['a'] = 'adding element to $ao';
$swapIn['b'] = 'adding element to $swapIn';
$ao['c'] = 'adding another element to $ao';

echo "\n--> swapIn:  ";
var_dump($swapIn);

echo "\n--> cowRef:  ";
var_dump($cowRef);

echo "\n--> ao:  ";
var_dump($ao);
?>
--EXPECTF--

--> swapIn:  array(1) {
  [u"b"]=>
  unicode(25) "adding element to $swapIn"
}

--> cowRef:  array(0) {
}

--> ao:  object(ArrayObject)#%d (1) {
  [u"storage":u"ArrayObject":private]=>
  array(2) {
    [u"a"]=>
    unicode(21) "adding element to $ao"
    [u"c"]=>
    unicode(29) "adding another element to $ao"
  }
}