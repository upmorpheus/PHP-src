--TEST--
The default value is a class constant in the parent class method's signature.
--FILE--
<?php
class MyDateTimeZone extends DateTimeZone
{
    public static function listIdentifiers()
    {
    }
}
--EXPECTF--
Fatal error: Declaration of MyDateTimeZone::listIdentifiers() must be compatible with DateTimeZone::listIdentifiers(int $what = DateTimeZone::ALL, ?string $country = null) in %s on line %d
