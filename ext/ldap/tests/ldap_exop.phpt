--TEST--
ldap_exop() and ldap_parse_exop() - EXOP operations
--CREDITS--
Côme Chilliet <mcmic@php.net>
--SKIPIF--
<?php require_once('skipif.inc'); ?>
<?php require_once('skipifbindfailure.inc'); ?>
--FILE--
<?php
require "connect.inc";

$link = ldap_connect_and_bind($host, $port, $user, $passwd, $protocol_version);
insert_dummy_data($link, $base);

function build_reqdata_passwd($user, $oldpw, $newpw)
{
	// This is basic and will only work for small strings
	$hex = '';
	if (!empty($user)) {
		$hex .= '80'.sprintf("%'.02x", strlen($user)).bin2hex($user);
	}
	if (!empty($oldpw)) {
		$hex .= '81'.sprintf("%'.02x", strlen($oldpw)).bin2hex($oldpw);
	}
	if (!empty($newpw)) {
		$hex .= '82'.sprintf("%'.02x", strlen($newpw)).bin2hex($newpw);
	}
	return hex2bin('30'.sprintf("%'.02x", strlen($hex)/2).$hex);
}

function extract_genpw($retdata)
{
	// Only works for small strings as well
	return hex2bin(substr(bin2hex($retdata), 4*2));
}

$userAPassword = "oops";

// ldap_exop(resource link, string reqoid [, string reqdata [, string retoid [, string retdata]]])
// bool ldap_parse_exop(resource link, resource result [, string retoid [, string retdata]])
var_dump(
	ldap_exop($link, LDAP_EXOP_WHO_AM_I, NULL, $retdata, $retoid),
	$retdata,
	$retoid,
	$r = ldap_exop($link, LDAP_EXOP_WHO_AM_I),
	ldap_parse_exop($link, $r, $retdata2),
	$retdata2,
	test_bind($host, $port, "cn=userA,$base", $userAPassword, $protocol_version),
	$r = ldap_exop($link, LDAP_EXOP_MODIFY_PASSWD, build_reqdata_passwd("cn=userA,$base", $userAPassword, "")),
	ldap_parse_exop($link, $r, $retpwdata, $retpwoid),
	$genpw = extract_genpw($retpwdata),
	$retpwoid,
	test_bind($host, $port, "cn=userA,$base", $genpw, $protocol_version)
);
?>
===DONE===
--CLEAN--
<?php
require "connect.inc";

$link = ldap_connect_and_bind($host, $port, $user, $passwd, $protocol_version);

remove_dummy_data($link, $base);
?>
--EXPECTF--
bool(true)
string(%d) "dn:%s"
string(0) ""
resource(%d) of type (ldap result)
bool(true)
string(%d) "dn:%s"
bool(true)
resource(%d) of type (ldap result)
bool(true)
string(%d) "%s"
string(0) ""
bool(true)
===DONE===
