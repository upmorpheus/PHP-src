--TEST--
mb_output_handler() (Shift_JIS)
--EXTENSIONS--
mbstring
--INI--
output_handler=mb_output_handler
internal_encoding=Shift_JIS
output_encoding=EUC-JP
--FILE--
<?php
// Shift_JIS
var_dump("テスト用日本語文字列。このモジュールはPHPにマルチバイト関数を提供します。");
?>
--EXPECT--
string(73) "･ﾆ･ｹ･ﾈﾍﾑﾆ�ﾋﾜｸ�ﾊｸｻ�ﾎ�｡｣､ｳ､ﾎ･筵ｸ･蝪ｼ･�､ﾏPHP､ﾋ･ﾞ･�･ﾁ･ﾐ･､･ﾈｴﾘｿ�､�ﾄ�ｶ｡､ｷ､ﾞ､ｹ｡｣"
