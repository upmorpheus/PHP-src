--TEST--
wordwrap() function
--POST--
--GET--
--FILE--
<?php

$tests = <<<TESTS
"12345 12345 12345 12345" === wordwrap("12345 12345 12345 12345")
"12345 12345\\n1234567890\\n1234567890" === wordwrap("12345 12345 1234567890 1234567890",12)
"12345\\n12345\\n12345\\n12345" === wordwrap("12345 12345 12345 12345",0)
"12345ab12345ab12345ab12345" === wordwrap("12345 12345 12345 12345",0,"ab")
"12345 12345ab1234567890ab1234567890" === wordwrap("12345 12345 1234567890 1234567890",12,"ab")
"123ab123ab123" === wordwrap("123ab123ab123", 3, "ab")
"123ab123ab123" === wordwrap("123ab123ab123", 5, "ab")
"123ab 123ab123" === wordwrap("123  123ab123", 3, "ab")
"123ab123ab123" === wordwrap("123 123ab123", 5, "ab")
"123 123ab123" === wordwrap("123 123 123", 10, "ab")

"123ab123ab123" === wordwrap("123ab123ab123", 3, "ab", 1)
"123ab123ab123" === wordwrap("123ab123ab123", 5, "ab", 1)
"123ab 12ab3ab123" === wordwrap("123  123ab123", 3, "ab", 1)
"123  ab123ab123" === wordwrap("123  123ab123", 5, "ab", 1)
"123  123ab 123" === wordwrap("123  123  123", 8, "ab", 1)
"123  123ab45  123" === wordwrap("123  12345  123", 8, "ab", 1)
"1ab2ab3ab4" === wordwrap("1234", 1, "ab", 1)
TESTS;

include('../../../../tests/quicktester.inc');

--EXPECT--
OK
