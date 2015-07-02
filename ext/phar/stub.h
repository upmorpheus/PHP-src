/*
  +----------------------------------------------------------------------+
  | phar php single-file executable PHP extension generated stub         |
  +----------------------------------------------------------------------+
  | Copyright (c) 2005-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Gregory Beaver <cellog@php.net>                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

static inline zend_string* phar_get_stub(const char *index_php, const char *web, const int name_len, const int web_len)
{
	static const char newstub0[] = "<?php\n\n$web = '";
	static const char newstub1_0[] = "';\n\nif (in_array('phar', stream_get_wrappers()) && class_exists('Phar', 0)) {\nPhar::interceptFileFuncs();\nset_include_path('phar://' . __FILE__ . PATH_SEPARATOR . get_include_path());\nPhar::webPhar(null, $web);\ninclude 'phar://' . __FILE__ . '/' . Extract_Phar::START;\nreturn;\n}\n\nif (@(isset($_SERVER['REQUEST_URI']) && isset($_SERVER['REQUEST_METHOD']) && ($_SERVER['REQUEST_METHOD'] == 'GET' || $_SERVER['REQUEST_METHOD'] == 'POST'))) {\nExtract_Phar::go(true);\n$mimes = array(\n'phps' => 2,\n'c' => 'text/plain',\n'cc' => 'text/plain',\n'cpp' => 'text/plain',\n'c++' => 'text/plain',\n'dtd' => 'text/plain',\n'h' => 'text/plain',\n'log' => 'text/plain',\n'rng' => 'text/plain',\n'txt' => 'text/plain',\n'xsd' => 'text/plain',\n'php' => 1,\n'inc' => 1,\n'avi' => 'video/avi',\n'bmp' => 'image/bmp',\n'css' => 'text/css',\n'gif' => 'image/gif',\n'htm' => 'text/html',\n'html' => 'text/html',\n'htmls' => 'text/html',\n'ico' => 'image/x-ico',\n'jpe' => 'image/jpeg',\n'jpg' => 'image/jpeg',\n'jpeg' => 'image/jpeg',\n'js' => 'application/x-javascript',\n'midi' => 'audio/midi',\n'mid' => 'audio/midi',\n'mod' => 'audio/mod',\n'mov' => 'movie/quicktime',\n'mp3' => 'audio/mp3',\n'mpg' => 'video/mpeg',\n'mpeg' => 'video/mpeg',\n'pdf' => 'application/pdf',\n'png' => 'image/png',\n'swf' => 'application/shockwave-flash',\n'tif' => 'image/tiff',\n'tiff' => 'image/tiff',\n'wav' => 'audio/wav',\n'xbm' => 'image/xbm',\n'xml' => 'text/xml',\n);\n\nheader(\"Cache-Control: no-cache, must-revalidate\");\nheader(\"Pragma: no-cache\");\n\n$basename = basename(__FILE__);\nif (!strpos($_SERVER['REQUEST_URI'], $basename)) {\nchdir(Extract_Phar::$temp);\ninclude $web;\nreturn;\n}\n$pt = substr($_SERVER['REQUEST_URI'], strpos($_SERVER['REQUEST_URI'], $basename) + strlen($basename));\nif (!$pt || $pt == '/') {\n$pt = $web;\nheader('HTTP/1.1 301 Moved Permanently');\nheader('Location: ' . $_SERVER['REQUEST_URI'] . '/' . $pt);\nexit;\n}\n$a = realpath(Extract_Phar::$temp . DIRECTORY_SEPARATOR . $pt);\nif (!$a || strlen(dirname($a)) < strlen(";
	static const char newstub1_1[] = "Extract_Phar::$temp)) {\nheader('HTTP/1.0 404 Not Found');\necho \"<html>\\n <head>\\n  <title>File Not Found<title>\\n </head>\\n <body>\\n  <h1>404 - File \", $pt, \" Not Found</h1>\\n </body>\\n</html>\";\nexit;\n}\n$b = pathinfo($a);\nif (!isset($b['extension'])) {\nheader('Content-Type: text/plain');\nheader('Content-Length: ' . filesize($a));\nreadfile($a);\nexit;\n}\nif (isset($mimes[$b['extension']])) {\nif ($mimes[$b['extension']] === 1) {\ninclude $a;\nexit;\n}\nif ($mimes[$b['extension']] === 2) {\nhighlight_file($a);\nexit;\n}\nheader('Content-Type: ' .$mimes[$b['extension']]);\nheader('Content-Length: ' . filesize($a));\nreadfile($a);\nexit;\n}\n}\n\nclass Extract_Phar\n{\nstatic $temp;\nstatic $origdir;\nconst GZ = 0x1000;\nconst BZ2 = 0x2000;\nconst MASK = 0x3000;\nconst START = '";
	static const char newstub2[] = "';\nconst LEN = ";
	static const char newstub3_0[] = ";\n\nstatic function go($return = false)\n{\n$fp = fopen(__FILE__, 'rb');\nfseek($fp, self::LEN);\n$L = unpack('V', $a = (binary)fread($fp, 4));\n$m = (binary)'';\n\ndo {\n$read = 8192;\nif ($L[1] - strlen($m) < 8192) {\n$read = $L[1] - strlen($m);\n}\n$last = (binary)fread($fp, $read);\n$m .= $last;\n} while (strlen($last) && strlen($m) < $L[1]);\n\nif (strlen($m) < $L[1]) {\ndie('ERROR: manifest length read was \"' .\nstrlen($m) .'\" should be \"' .\n$L[1] . '\"');\n}\n\n$info = self::_unpack($m);\n$f = $info['c'];\n\nif ($f & self::GZ) {\nif (!function_exists('gzinflate')) {\ndie('Error: zlib extension is not enabled -' .\n' gzinflate() function needed for zlib-compressed .phars');\n}\n}\n\nif ($f & self::BZ2) {\nif (!function_exists('bzdecompress')) {\ndie('Error: bzip2 extension is not enabled -' .\n' bzdecompress() function needed for bz2-compressed .phars');\n}\n}\n\n$temp = self::tmpdir();\n\nif (!$temp || !is_writable($temp)) {\n$sessionpath = session_save_path();\nif (strpos ($sessionpath, \";\") !== false)\n$sessionpath = substr ($sessionpath, strpos ($sessionpath, \";\")+1);\nif (!file_exists($sessionpath) || !is_dir($sessionpath)) {\ndie('Could not locate temporary directory to extract phar');\n}\n$temp = $sessionpath;\n}\n\n$temp .= '/pharextract/'.basename(__FILE__, '.phar');\nself::$temp = $temp;\nself::$origdir = getcwd();\n@mkdir($temp, 0777, true);\n$temp = realpath($temp);\n\nif (!file_exists($temp . DIRECTORY_SEPARATOR . md5_file(__FILE__))) {\nself::_removeTmpFiles($temp, getcwd());\n@mkdir($temp, 0777, true);\n@file_put_contents($temp . '/' . md5_file(__FILE__), '');\n\nforeach ($info['m'] as $path => $file) {\n$a = !file_exists(dirname($temp . '/' . $path));\n@mkdir(dirname($temp . '/' . $path), 0777, true);\nclearstatcache();\n\nif ($path[strlen($path) - 1] == '/') {\n@mkdir($temp . '/' . $path, 0777);\n} else {\nfile_put_contents($temp . '/' . $path, self::extractFile($path, $file, $fp));\n@chmod($temp . '/' . $path, 0666);\n}\n}\n}\n\nchdir($temp);\n\nif (!$return) {\ninclude self::ST";
	static const char newstub3_1[] = "ART;\n}\n}\n\nstatic function tmpdir()\n{\nif (strpos(PHP_OS, 'WIN') !== false) {\nif ($var = getenv('TMP') ? getenv('TMP') : getenv('TEMP')) {\nreturn $var;\n}\nif (is_dir('/temp') || mkdir('/temp')) {\nreturn realpath('/temp');\n}\nreturn false;\n}\nif ($var = getenv('TMPDIR')) {\nreturn $var;\n}\nreturn realpath('/tmp');\n}\n\nstatic function _unpack($m)\n{\n$info = unpack('V', substr($m, 0, 4));\n $l = unpack('V', substr($m, 10, 4));\n$m = substr($m, 14 + $l[1]);\n$s = unpack('V', substr($m, 0, 4));\n$o = 0;\n$start = 4 + $s[1];\n$ret['c'] = 0;\n\nfor ($i = 0; $i < $info[1]; $i++) {\n $len = unpack('V', substr($m, $start, 4));\n$start += 4;\n $savepath = substr($m, $start, $len[1]);\n$start += $len[1];\n   $ret['m'][$savepath] = array_values(unpack('Va/Vb/Vc/Vd/Ve/Vf', substr($m, $start, 24)));\n$ret['m'][$savepath][3] = sprintf('%u', $ret['m'][$savepath][3]\n& 0xffffffff);\n$ret['m'][$savepath][7] = $o;\n$o += $ret['m'][$savepath][2];\n$start += 24 + $ret['m'][$savepath][5];\n$ret['c'] |= $ret['m'][$savepath][4] & self::MASK;\n}\nreturn $ret;\n}\n\nstatic function extractFile($path, $entry, $fp)\n{\n$data = '';\n$c = $entry[2];\n\nwhile ($c) {\nif ($c < 8192) {\n$data .= @fread($fp, $c);\n$c = 0;\n} else {\n$c -= 8192;\n$data .= @fread($fp, 8192);\n}\n}\n\nif ($entry[4] & self::GZ) {\n$data = gzinflate($data);\n} elseif ($entry[4] & self::BZ2) {\n$data = bzdecompress($data);\n}\n\nif (strlen($data) != $entry[0]) {\ndie(\"Invalid internal .phar file (size error \" . strlen($data) . \" != \" .\n$stat[7] . \")\");\n}\n\nif ($entry[3] != sprintf(\"%u\", crc32((binary)$data) & 0xffffffff)) {\ndie(\"Invalid internal .phar file (checksum error)\");\n}\n\nreturn $data;\n}\n\nstatic function _removeTmpFiles($temp, $origdir)\n{\nchdir($temp);\n\nforeach (glob('*') as $f) {\nif (file_exists($f)) {\nis_dir($f) ? @rmdir($f) : @unlink($f);\nif (file_exists($f) && is_dir($f)) {\nself::_removeTmpFiles($f, getcwd());\n}\n}\n}\n\n@rmdir($temp);\nclearstatcache();\nchdir($origdir);\n}\n}\n\nExtract_Phar::go();\n__HALT_COMPIL";
	static const char newstub3_2[] = "ER(); ?>";

	static const int newstub_len = 6665;

	return strpprintf(name_len + web_len + newstub_len, "%s%s%s%s%s%s%d%s%s%s", newstub0, web, newstub1_0, newstub1_1, index_php, newstub2, name_len + web_len + newstub_len, newstub3_0, newstub3_1, newstub3_2);
}
