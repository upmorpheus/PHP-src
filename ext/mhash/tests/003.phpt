--TEST--
mhash_keygen_s2k() test
--SKIPIF--
<?php
	include "skip.inc";
?>
--FILE--
<?php

$supported_hash_al = array(
"MHASH_MD5",
"MHASH_SHA1",
"MHASH_HAVAL256",
"MHASH_HAVAL192",
"MHASH_HAVAL224",
"MHASH_HAVAL160",
"MHASH_RIPEMD160",
"MHASH_GOST",
"MHASH_TIGER",
"MHASH_CRC32",
"MHASH_CRC32B"
);

	foreach ($supported_hash_al as $alg) {
		$passwd = str_repeat($alg, 10);
		$salt = str_repeat($alg, 2);
		
		var_dump(mhash_keygen_s2k(constant($alg), $passwd, $salt, 100));
	}
?>
--EXPECT--
string(100) "��N��2��4z�P���F�栔Ty�zcg�h���t^W	��-��s���A7Y��:��w�ݲx�d�q�S���^҃��&U�,���: �aǙ��z��S�"
string(100) "�1\pE]S���a�f\����T#$��2�����xc
�=��"IJ�'�NΚԽ�n���J3�<+H0�[ ��[����Y�q�ء��K�mI"
string(100) "�G ����N�Pw�H6W�ل�+����,���
gdZ��U��)oAi�[Na����d�T����2�Z����U�46}IYb,���C��u����{�[x"
string(100) ""��q&<�.�A�/�2�d����2�Xv�p�ű�%�(�C;�+p�z8��N��u��d�d)qه��v_���MIN�����xLt����Ǉ"
string(100) "\J�=�Z�����y�8ha�
���p��g|y��ܫq������)
�˼l��ǌ�GB@kw�º�Q���v52O�P�-0�5 L	m`G-2�I"
string(100) "��������dn��6GX��m�� ���η#���5�M��"m�i��1$�|�X<O)Y;�=�ʰ�y.��`"�`��ٴB�	ԯ��ҵ�k>G�����~�
~"
string(100) "���F��x�
�5��
[�gDXgw��ܡD�&�b�����j��B�����7����]�G��[G`^��K���3C-AB�%�K??�K������"
string(100) "�D�i�~�C�=wƂ��B��}��s�M��d|�4h�Y�"
�I*b@ф�xc@sިy���N%d��	��FD
P���}E��~�:pO��
�pw�R{M�"
string(100) "��!���p+#���#����f6y����!���p+#���#����f6y����!���p+#���#����f6y����!���p+#���#����f6y����!�"
string(100) "H@�&_�Y��oQ���1[Fq"����f��V�8�E��E<����M�:Ⱥ��r��k*�H��u��/}���h3iW��J?vu
i��=�W"���"
string(100) "�l�e�>}�*�]�Fכ6�bC�ˏ.	I��j�Υ0��
<��.Qy疾��ti��xb�$}���ȴ�	�ϬȌD�DK��1uL��6��`��ɏ�|Ec��"
