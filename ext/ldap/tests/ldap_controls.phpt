--TEST--
Test the use of controls
--CREDITS--
Côme Chilliet <mcmic@php.net>
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifbindfailure.inc');
?>
--FILE--
<?php
include "connect.inc";

$link = ldap_connect_and_bind($host, $port, $user, $passwd, $protocol_version);
insert_dummy_data($link, $base);

/* Test assertion control */
var_dump(
	$result = ldap_search($link, "o=test,$base", "objectClass=*", array('o'), 0, 0, 0, LDAP_DEREF_NEVER,
		[['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(objectClass=organization)']]]),
	ldap_get_entries($link, $result),
	$result = ldap_search($link, "o=test,$base", "objectClass=*", array('o'), 0, 0, 0, LDAP_DEREF_NEVER,
		[['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(objectClass=organizationalUnit)']]]),
	ldap_modify($link, "o=test,$base", ['description' => 'desc'],
		[['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(!(description=*))']]]),
	$result = ldap_read($link, "o=test,$base", "objectClass=*", array('description')),
	ldap_get_entries($link, $result),
	ldap_modify($link, "o=test,$base", ['description' => 'desc2'],
		[['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(!(description=*))']]]),
	$result = ldap_read($link, "o=test,$base", "objectClass=*", array('description')),
	ldap_get_entries($link, $result),
	ldap_delete($link, "o=test,$base", [['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(description=desc2)']]]),
	ldap_errno($link),
	ldap_error($link),
	ldap_rename($link, "o=test,$base", "o=test2", "", TRUE, [['oid' => LDAP_CONTROL_ASSERT, 'iscritical' => TRUE, 'value' => ['filter' => '(description=desc2)']]])
);
?>
===DONE===
--CLEAN--
<?php
include "connect.inc";

$link = ldap_connect_and_bind($host, $port, $user, $passwd, $protocol_version);
remove_dummy_data($link, $base);
?>
--EXPECTF--
Warning: ldap_search(): Search: Assertion Failed in %s on line %d

Warning: ldap_modify(): Modify: Assertion Failed in %s on line %d

Warning: ldap_delete(): Delete: Assertion Failed in %s on line %d
resource(%d) of type (ldap result)
array(2) {
  ["count"]=>
  int(1)
  [0]=>
  array(4) {
    ["o"]=>
    array(2) {
      ["count"]=>
      int(1)
      [0]=>
      string(4) "test"
    }
    [0]=>
    string(1) "o"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "o=test,%s"
  }
}
bool(false)
bool(true)
resource(%d) of type (ldap result)
array(2) {
  ["count"]=>
  int(1)
  [0]=>
  array(4) {
    ["description"]=>
    array(2) {
      ["count"]=>
      int(1)
      [0]=>
      string(4) "desc"
    }
    [0]=>
    string(11) "description"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "o=test,%s"
  }
}
bool(false)
resource(%d) of type (ldap result)
array(2) {
  ["count"]=>
  int(1)
  [0]=>
  array(4) {
    ["description"]=>
    array(2) {
      ["count"]=>
      int(1)
      [0]=>
      string(4) "desc"
    }
    [0]=>
    string(11) "description"
    ["count"]=>
    int(1)
    ["dn"]=>
    string(%d) "o=test,%s"
  }
}
bool(false)
int(122)
string(16) "Assertion Failed"
bool(false)
===DONE===
