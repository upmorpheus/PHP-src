--TEST--
Bug #48770 (call_user_func_array() fails to call parent from inheriting class)
--XFAIL--
See Bug #48770
--FILE--
<?php

class A {
    public function func($str) {
        var_dump(__METHOD__ .': '. $str);
    }
    private function func2($str) {
        var_dump(__METHOD__ .': '. $str);
    }
    protected function func3($str) {
        var_dump(__METHOD__ .': '. $str);
    }
    private function func22($str) {
        var_dump(__METHOD__ .': '. $str);
    }
}

class B extends A {
    public function func($str) {
        call_user_func_array(array($this, 'parent::func2'), array($str));
        call_user_func_array(array($this, 'parent::func3'), array($str));
        
        try {
            call_user_func_array(array($this, 'parent::func22'), array($str));
        } catch (\TypeError $e) {
            echo $e->getMessage() . \PHP_EOL;
        }
        
        try {
            call_user_func_array(array($this, 'parent::inexistent'), array($str));
        } catch (\TypeError $e) {
            echo $e->getMessage() . \PHP_EOL;
        }
    }
    private function func2($str) {
        var_dump(__METHOD__ .': '. $str);
    }
    protected function func3($str) {
        var_dump(__METHOD__ .': '. $str);
    }
}

class C extends B {
    public function func($str) {
        parent::func($str);
    }
}

$c = new C;
$c->func('This should work!');

?>
--EXPECT--
string(27) "A::func2: This should work!"
string(27) "A::func3: This should work!"
call_user_func_array() expects parameter 1 to be a valid callback, cannot access private method A::func22()
call_user_func_array() expects parameter 1 to be a valid callback, class 'A' does not have a method 'inexistent'
