#! /usr/local/bin/php -n
<?php

/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* This script lists extension-, class- and method names that contain any
   underscores. It omits magic names (e.g. anything that starts with two
   underscores but no more).
 */
$cnt = 0;
$err = 0;

$classes = array_merge(get_declared_classes(), get_declared_interfaces());

$extensions = array();

foreach(get_loaded_extensions() as $ext) {
	$cnt++;
	if (strpos($ext, "_") !== false) {
		$err++;
		$extensions[$ext] = array();
	}
}

$cnt += count($classes);

foreach($classes as $c) {
	if (strpos($c, "_") !== false) {
		$err++;
		$ref = new ReflectionClass($c);
		if (!($ext = $ref->getExtensionName())) {;
			$ext = $ref->isInternal() ? "<internal>" : "<user>";
		}
		if (!array_key_exists($ext, $extensions)) {
			$extensions[$ext] = array();
		}
		$extensions[$ext][$c] = array();
		foreach(get_class_methods($c) as $method) {
			$cnt++;
			if (strpos(substr($method, substr($method, 0, 2) != "__"  ? 0 : 2), "_") !== false) {
				$err++;
				$extensions[$ext][$c][] = $method;
			}
		}
	}
	else
	{
		$cnt += count(get_class_methods($c));
	}
}

ksort($extensions);
foreach($extensions as $ext => &$classes) {
	echo "Extension: $ext\n";
	ksort($classes);
	foreach($classes as $classname => &$methods) {
		echo "  Class: $classname\n";
		ksort($methods);
		foreach($methods as $method) {
			echo "    Method: $method\n";
		}
	}
}

printf("\n");
printf("Names:  %5d\n", $cnt);
printf("Errors: %5d (%.1f%%)\n", $err, round($err * 100 / $cnt, 1));
printf("\n");

?>