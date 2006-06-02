--TEST--
Date constants
--FILE--
<?php
    date_default_timezone_set("Europe/Oslo");
    $constants = array(
        DATE_ATOM,
        DATE_COOKIE,
        DATE_ISO8601,
        DATE_RFC822,
        DATE_RFC850,
        DATE_RFC1036,
        DATE_RFC1123,
        DATE_RFC2822,
        DATE_RSS,
        DATE_W3C
    );
    
    foreach($constants as $const) {
        var_dump(date($const, strtotime("1 Jul 06 14:27:30 +0200")));
        var_dump(date($const, strtotime("2006-05-30T14:32:13+02:00")));
    }
?>
--EXPECT--
string(25) "2006-07-01T14:27:30+02:00"
string(25) "2006-05-30T14:32:13+02:00"
string(33) "Saturday, 01-Jul-06 14:27:30 CEST"
string(32) "Tuesday, 30-May-06 14:32:13 CEST"
string(24) "2006-07-01T14:27:30+0200"
string(24) "2006-05-30T14:32:13+0200"
string(29) "Sat, 01 Jul 06 14:27:30 +0200"
string(29) "Tue, 30 May 06 14:32:13 +0200"
string(33) "Saturday, 01-Jul-06 14:27:30 CEST"
string(32) "Tuesday, 30-May-06 14:32:13 CEST"
string(29) "Sat, 01 Jul 06 14:27:30 +0200"
string(29) "Tue, 30 May 06 14:32:13 +0200"
string(31) "Sat, 01 Jul 2006 14:27:30 +0200"
string(31) "Tue, 30 May 2006 14:32:13 +0200"
string(31) "Sat, 01 Jul 2006 14:27:30 +0200"
string(31) "Tue, 30 May 2006 14:32:13 +0200"
string(31) "Sat, 01 Jul 2006 14:27:30 +0200"
string(31) "Tue, 30 May 2006 14:32:13 +0200"
string(25) "2006-07-01T14:27:30+02:00"
string(25) "2006-05-30T14:32:13+02:00"
--UEXPECT--
unicode(25) "2006-07-01T14:27:30+02:00"
unicode(25) "2006-05-30T14:32:13+02:00"
unicode(33) "Saturday, 01-Jul-06 14:27:30 CEST"
unicode(32) "Tuesday, 30-May-06 14:32:13 CEST"
unicode(24) "2006-07-01T14:27:30+0200"
unicode(24) "2006-05-30T14:32:13+0200"
unicode(29) "Sat, 01 Jul 06 14:27:30 +0200"
unicode(29) "Tue, 30 May 06 14:32:13 +0200"
unicode(33) "Saturday, 01-Jul-06 14:27:30 CEST"
unicode(32) "Tuesday, 30-May-06 14:32:13 CEST"
unicode(29) "Sat, 01 Jul 06 14:27:30 +0200"
unicode(29) "Tue, 30 May 06 14:32:13 +0200"
unicode(31) "Sat, 01 Jul 2006 14:27:30 +0200"
unicode(31) "Tue, 30 May 2006 14:32:13 +0200"
unicode(31) "Sat, 01 Jul 2006 14:27:30 +0200"
unicode(31) "Tue, 30 May 2006 14:32:13 +0200"
unicode(31) "Sat, 01 Jul 2006 14:27:30 +0200"
unicode(31) "Tue, 30 May 2006 14:32:13 +0200"
unicode(25) "2006-07-01T14:27:30+02:00"
unicode(25) "2006-05-30T14:32:13+02:00"
