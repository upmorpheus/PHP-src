--TEST--
Using indirect append on ArrayAccess object
--FILE--
<?php

class AA implements ArrayAccess {
    private $data = [];
    public function &offsetGet($name) {
        if (null === $name) {
            return $this->data[];
        } else {
            return $this->data[$name];
        }
    }
    public function offsetSet($name, $value) {
        $this->data[$name] = $value;
    }
    public function offsetUnset($name) {}
    public function offsetExists($name) {}
}

$aa = new AA;
$aa[3] = 1;
$aa[][][0] = 2;
var_dump($aa);

?>
--EXPECT--
object(AA)#1 (1) {
  ["data":"AA":private]=>
  array(2) {
    [3]=>
    int(1)
    [4]=>
    array(1) {
      [0]=>
      array(1) {
        [0]=>
        int(2)
      }
    }
  }
}
