--TEST--
Bug #40261 (Extremely slow data handling due to memory fragmentation)
--INI--
memory_limit=128M
--FILE--
<?php
$num = 100000;

$a = Array();
for ($i=0; $i<$num; $i++) {
  $a[$i] = Array(1);
}

for ($i=0; $i<$num; $i++) {
  $b[$i] = $a[$i][0];
}

unset($a);
for ($i=0; $i<$num; $i++) {
  $b[$i] = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
}
echo "ok\n";
?>
--EXPECT--
ok
