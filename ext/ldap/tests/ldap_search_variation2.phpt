--TEST--
ldap_search() test
--CREDITS--
Davide Mendolia <idaf1er@gmail.com>
Patrick Allaert <patrickallaert@php.net>
Belgian PHP Testfest 2009
--EXTENSIONS--
ldap
--SKIPIF--
<?php
require_once('skipifbindfailure.inc');
?>
--FILE--
<?php
include "connect.inc";

$link = ldap_connect_and_bind($uri, $user, $passwd, $protocol_version);
insert_dummy_data($link, $base);

var_dump(
    $result = ldap_search($link, "$base", "(objectclass=person)", array('sn'), 1),
    ldap_get_entries($link, $result)
);
?>
--CLEAN--
<?php
include "connect.inc";

$link = ldap_connect_and_bind($uri, $user, $passwd, $protocol_version);
remove_dummy_data($link, $base);
?>
--EXPECTF--
object(LDAP\Result)#%d (0) {
}
array(4) {
  ["count"]=>
  int(3)
  [0]=>
  array(4) {
    ["sn"]=>
    array(1) {
      ["count"]=>
      int(0)
    }
    [0]=>
    string(2) "sn"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "cn=userA,%s"
  }
  [1]=>
  array(4) {
    ["sn"]=>
    array(1) {
      ["count"]=>
      int(0)
    }
    [0]=>
    string(2) "sn"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "cn=userB,%s"
  }
  [2]=>
  array(4) {
    ["sn"]=>
    array(1) {
      ["count"]=>
      int(0)
    }
    [0]=>
    string(2) "sn"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "cn=userC,cn=userB,%s"
  }
}
