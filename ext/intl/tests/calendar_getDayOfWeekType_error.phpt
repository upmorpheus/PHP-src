--TEST--
IntlCalendar::getDayOfWeekOfType(): bad arguments
--INI--
date.timezone=Atlantic/Azores
--SKIPIF--
<?php
if (!extension_loaded('intl'))
	die('skip intl extension not enabled');
--FILE--
<?php
ini_set("intl.error_level", E_WARNING);

$c = new IntlGregorianCalendar(NULL, 'pt_PT');

var_dump($c->getDayOfWeekType(0));

var_dump(intlcal_get_day_of_week_type(1, 1));
--EXPECTF--
Warning: IntlCalendar::getDayOfWeekType(): intlcal_get_day_of_week_type: invalid day of week in %s on line %d
bool(false)

Fatal error: Uncaught TypeError: intlcal_get_day_of_week_type(): Argument #1 ($calendar) must be of type IntlCalendar, int given in %s:%d
Stack trace:
#0 %s(%d): intlcal_get_day_of_week_type(1, 1)
#1 {main}
  thrown in %s on line %d
