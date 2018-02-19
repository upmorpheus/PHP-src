--TEST--
Zend signed multiply 64-bit, variation 2
--SKIPIF--
<?php if ((1 << 31) < 0) print "skip Running on 32-bit target"; ?>
--FILE--
<?php
for($c = -16; $c < 0; $c++) {
        var_dump($c, intdiv(PHP_INT_MIN, 10), intdiv(PHP_INT_MIN, 10) * $c);
	echo "-----------\n";
}
for($c = 0; $c <= 16; $c++) {
        var_dump($c, intdiv(PHP_INT_MAX, 10), intdiv(PHP_INT_MAX, 10) * $c);
	echo "-----------\n";
}
?>
--EXPECT--
int(-16)
int(-922337203685477580)
float(1.4757395258968E+19)
-----------
int(-15)
int(-922337203685477580)
float(1.3835058055282E+19)
-----------
int(-14)
int(-922337203685477580)
float(1.2912720851597E+19)
-----------
int(-13)
int(-922337203685477580)
float(1.1990383647911E+19)
-----------
int(-12)
int(-922337203685477580)
float(1.1068046444226E+19)
-----------
int(-11)
int(-922337203685477580)
float(1.014570924054E+19)
-----------
int(-10)
int(-922337203685477580)
int(9223372036854775800)
-----------
int(-9)
int(-922337203685477580)
int(8301034833169298220)
-----------
int(-8)
int(-922337203685477580)
int(7378697629483820640)
-----------
int(-7)
int(-922337203685477580)
int(6456360425798343060)
-----------
int(-6)
int(-922337203685477580)
int(5534023222112865480)
-----------
int(-5)
int(-922337203685477580)
int(4611686018427387900)
-----------
int(-4)
int(-922337203685477580)
int(3689348814741910320)
-----------
int(-3)
int(-922337203685477580)
int(2767011611056432740)
-----------
int(-2)
int(-922337203685477580)
int(1844674407370955160)
-----------
int(-1)
int(-922337203685477580)
int(922337203685477580)
-----------
int(0)
int(922337203685477580)
int(0)
-----------
int(1)
int(922337203685477580)
int(922337203685477580)
-----------
int(2)
int(922337203685477580)
int(1844674407370955160)
-----------
int(3)
int(922337203685477580)
int(2767011611056432740)
-----------
int(4)
int(922337203685477580)
int(3689348814741910320)
-----------
int(5)
int(922337203685477580)
int(4611686018427387900)
-----------
int(6)
int(922337203685477580)
int(5534023222112865480)
-----------
int(7)
int(922337203685477580)
int(6456360425798343060)
-----------
int(8)
int(922337203685477580)
int(7378697629483820640)
-----------
int(9)
int(922337203685477580)
int(8301034833169298220)
-----------
int(10)
int(922337203685477580)
int(9223372036854775800)
-----------
int(11)
int(922337203685477580)
float(1.014570924054E+19)
-----------
int(12)
int(922337203685477580)
float(1.1068046444226E+19)
-----------
int(13)
int(922337203685477580)
float(1.1990383647911E+19)
-----------
int(14)
int(922337203685477580)
float(1.2912720851597E+19)
-----------
int(15)
int(922337203685477580)
float(1.3835058055282E+19)
-----------
int(16)
int(922337203685477580)
float(1.4757395258968E+19)
-----------

