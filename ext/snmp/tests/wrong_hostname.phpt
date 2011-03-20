--TEST--                                 
Generic timeout (wrong community)
--CREDITS--
Boris Lytochkin
--SKIPIF--
<?php
require_once(dirname(__FILE__).'/skipif.inc');
?>
--FILE--
<?php
require_once(dirname(__FILE__).'/snmp_include.inc');

//EXPECTF format is quickprint OFF
snmp_set_quick_print(false);
snmp_set_valueretrieval(SNMP_VALUE_PLAIN);

var_dump(snmpget('192.168..6.1', 'community', '.1.3.6.1.2.1.1.1.0', $timeout, $retries));

?>
--EXPECTF--
Warning: snmpget(): Could not open snmp connection: Unknown host (192.168..6.1) (%s) in %s on line %d
bool(false)
