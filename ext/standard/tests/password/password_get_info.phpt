--TEST--
Test normal operation of password_get_info()
--FILE--
<?php
//-=-=-=-
// Test Bcrypt
var_dump(password_get_info('$2y$10$MTIzNDU2Nzg5MDEyMzQ1Nej0NmcAWSLR.oP7XOR9HD/vjUuOj100y'));
// Test Bcrypt Cost
var_dump(password_get_info('$2y$11$MTIzNDU2Nzg5MDEyMzQ1Nej0NmcAWSLR.oP7XOR9HD/vjUuOj100y'));
// Test Bcrypt Invalid Length
var_dump(password_get_info('$2y$11$MTIzNDU2Nzg5MDEyMzQ1Nej0NmcAWSLR.oP7XOR9HD/vjUuOj100'));
// Test Non-Bcrypt
var_dump(password_get_info('$1$rasmusle$rISCgZzpwk3UhDidwXvin0'));
// Test Argon2
//var_dump(password_get_info('$argon2i$v=19$m=65536,t=3,p=1$SWhIcG5MT21Pc01PbWdVZw$WagZELICsz7jlqOR2YzoEVTWb2oOX1tYdnhZYXxptbU'));

echo "OK!";
?>
--EXPECT--
array(3) {
  ["algo"]=>
  int(1)
  ["algoName"]=>
  string(6) "bcrypt"
  ["options"]=>
  array(1) {
    ["cost"]=>
    int(10)
  }
}
array(3) {
  ["algo"]=>
  int(1)
  ["algoName"]=>
  string(6) "bcrypt"
  ["options"]=>
  array(1) {
    ["cost"]=>
    int(11)
  }
}
array(3) {
  ["algo"]=>
  int(0)
  ["algoName"]=>
  string(7) "unknown"
  ["options"]=>
  array(0) {
  }
}
array(3) {
  ["algo"]=>
  int(0)
  ["algoName"]=>
  string(7) "unknown"
  ["options"]=>
  array(0) {
  }
}
array(3) {
  ["algo"]=>
  int(2)
  ["algoName"]=>
  string(7) "argon2i"
  ["options"]=>
  array(3) {
    ["m_cost"]=>
    int(65536)
    ["t_cost"]=>
    int(3)
    ["threads"]=>
    int(1)
  }
}
OK!
