--TEST--
strtotime() function
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) == 'WIN') {
    die('skip Windows does not support dates prior to midnight (00:00:00), January 1, 1970');
}
if (!@putenv("TZ=EST5") || getenv("TZ") != 'EST5') {
	die("skip unable to change TZ enviroment variable\n");
}
?>
--FILE--
<?php
	$dates = array (
		"1999-10-13",
		"Oct 13  1999",
		"2000-01-19",
		"Jan 19  2000",
		"2001-12-21",
		"Dec 21  2001",
		"2001-12-21 12:16",
		"Dec 21 2001 12:16",
		"Dec 21  12:16",
	    "2001-10-22 21:19:58",
	    "2001-10-22 21:19:58-02",
	    "2001-10-22 21:19:58-0213",
	    "2001-10-22 21:19:58+02",
    	"2001-10-22 21:19:58+0213",
		"2001-10-22T21:20:58-03:40",
		"2001-10-22T211958-2",
		"20011022T211958+0213",
		"20011022T21:20+0215",
		"1997W011",
		"2004W101T05:00+0",
	);

	echo "*** GMT0\n";
	putenv ("TZ=GMT0");
	foreach ($dates as $date) {
	    echo date ("Y-m-d H:i:s\n", strtotime ($date));
	}

	echo "*** US/Eastern\n";
	putenv("TZ=US/Eastern");
	if( date("T") == "GMT" ) {
		// POSIX style
		putenv ("TZ=EST5EDT4,M4.1.0,M10.5.0");
	}

	foreach ($dates as $date) {
	    echo date ("Y-m-d H:i:s\n", strtotime ($date));
	}
?>
--EXPECT--
*** GMT0
1999-10-13 00:00:00
1999-10-13 00:00:00
2000-01-19 00:00:00
2000-01-19 00:00:00
2001-12-21 00:00:00
2001-12-21 00:00:00
2001-12-21 12:16:00
2001-12-21 12:16:00
1969-12-31 23:59:59
2001-10-22 21:19:58
2001-10-22 23:19:58
2001-10-22 23:32:58
2001-10-22 19:19:58
2001-10-22 19:06:58
2001-10-23 01:00:58
2001-10-22 23:19:58
2001-10-22 19:06:58
2001-10-22 19:05:00
1996-12-30 00:00:00
2004-03-01 05:00:00
*** US/Eastern
1999-10-13 00:00:00
1999-10-13 00:00:00
2000-01-19 00:00:00
2000-01-19 00:00:00
2001-12-21 00:00:00
2001-12-21 00:00:00
2001-12-21 12:16:00
2001-12-21 12:16:00
1969-12-31 18:59:59
2001-10-22 21:19:58
2001-10-22 19:19:58
2001-10-22 19:32:58
2001-10-22 15:19:58
2001-10-22 15:06:58
2001-10-22 21:00:58
2001-10-22 19:19:58
2001-10-22 15:06:58
2001-10-22 15:05:00
1996-12-30 00:00:00
2004-03-01 00:00:00
