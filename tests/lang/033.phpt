--TEST--
Alternative syntaxes test
--POST--
--GET--
--FILE--
<?php
$a = 1;

echo "If: ";
if ($a) echo 1; else echo 0;
if ($a):
	echo 1;
else:
	echo 0;
endif;

echo "\nWhile: ";
while ($a<5) echo $a++;
while ($a<9):
	echo ++$a;
endwhile;

echo "\nFor: ";
for($a=0;$a<5;$a++) echo $a;
for($a=0;$a<5;$a++):
	echo $a;
endfor;

echo "\nSwitch: ";
switch ($a):
	case 0;
		echo 0;
		break;
	case 5:
		echo 1;
		break;
	default;
		echo 0;
		break;
endswitch;

echo "\nold_function: ";
old_function foo $bar, $baz (
	return sprintf("foo(%s, %s);\n", $bar, $baz);
);
echo foo(1,2);
?>
--EXPECT--
If: 11
While: 12346789
For: 0123401234
Switch: 1
old_function: foo(1, 2);
