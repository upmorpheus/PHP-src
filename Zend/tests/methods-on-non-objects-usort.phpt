--TEST--
usort() in combination with "Call to a member function method() on null"
--FILE--
<?php
set_error_handler(function($code, $message) {
  var_dump($code, $message);
});

$comparator= null;
$list= [1, 4, 2, 3, -1];
usort($list, function($a, $b) use ($comparator) {
  return $comparator->compare($a, $b);
});
var_dump($list);
echo "Alive\n";
?>
--EXPECTF--
int(4096)
string(43) "Call to a member function compare() on null"
int(4096)
string(43) "Call to a member function compare() on null"
int(4096)
string(43) "Call to a member function compare() on null"
int(4096)
string(43) "Call to a member function compare() on null"
int(4096)
string(43) "Call to a member function compare() on null"
int(4096)
string(43) "Call to a member function compare() on null"
array(5) {
  [0]=>
  int(-1)
  [1]=>
  int(3)
  [2]=>
  int(2)
  [3]=>
  int(4)
  [4]=>
  int(1)
}
Alive

