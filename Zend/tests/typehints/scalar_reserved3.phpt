--TEST--
Scalar type hint names cannot be used as class, trait or interface names (3)
--FILE--
<?php

class float {}
--EXPECTF--
Fatal error: "float" cannot be used as a class name in %s on line %d
