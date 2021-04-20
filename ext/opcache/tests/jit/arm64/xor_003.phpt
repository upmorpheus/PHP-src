--TEST--
JIT XOR: 003
--INI--
opcache.enable=1
opcache.enable_cli=1
opcache.file_update_protection=0
opcache.jit_buffer_size=32M
;opcache.jit_debug=257
--EXTENSIONS--
opcache
--FILE--
<?php
function foo($a, $b) {
  $res = $a ^ $b;
  var_dump($res);
}
foo("abc", "\001\002\003");
?>
--EXPECT--
string(3) "```"
