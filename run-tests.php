<?php
/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2005 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Ilia Alshanetsky <iliaa@php.net>                            |
   |          Preston L. Bannister <pbannister@php.net>                   |
   |          Marcus Boerger <helly@php.net>                              |
   |          Derick Rethans <derick@php.net>                             |
   |          Sander Roobol <sander@php.net>                              |
   | (based on version by: Stig Bakken <ssb@php.net>)                     |
   | (based on the PHP 3 test framework by Rasmus Lerdorf)                |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* Sanity check to ensure that pcre extension needed by this script is available.
 * In the event it is not, print a nice error message indicating that this script will
 * not run without it.
 */
if (!extension_loaded("pcre")) {
	echo <<<NO_PCRE_ERROR

+-----------------------------------------------------------+
|                       ! ERROR !                           |
| The test-suite requires that you have pcre extension      |
| enabled. To enable this extension either compile your PHP |
| with --with-pcre-regex or if you've compiled pcre as a    |
| shared module load it via php.ini.                        |
+-----------------------------------------------------------+

NO_PCRE_ERROR;
exit;
}

if (!function_exists("proc_open")) {
	echo <<<NO_PROC_OPEN_ERROR

+-----------------------------------------------------------+
|                       ! ERROR !                           |
| The test-suite requires that proc_open() is available.    |
| Please check if you disabled it in php.ini.               |
+-----------------------------------------------------------+

NO_PROC_OPEN_ERROR;
exit;
}

// store current directory
$CUR_DIR = getcwd();

// change into the PHP source directory.

if (getenv('TEST_PHP_SRCDIR')) {
	@chdir(getenv('TEST_PHP_SRCDIR'));
}

// Delete some security related environment variables
putenv('SSH_CLIENT=deleted');
putenv('SSH_AUTH_SOCK=deleted');
putenv('SSH_TTY=deleted');
putenv('SSH_CONNECTION=deleted');

$cwd = getcwd();
set_time_limit(0);

// delete as much output buffers as possible
while(@ob_end_clean());
if (ob_get_level()) echo "Not all buffers were deleted.\n";

error_reporting(E_ALL);
ini_set('magic_quotes_runtime',0); // this would break tests by modifying EXPECT sections

if (ini_get('safe_mode')) {
	echo <<< SAFE_MODE_WARNING

+-----------------------------------------------------------+
|                       ! WARNING !                         |
| You are running the test-suite with "safe_mode" ENABLED ! |
|                                                           |
| Chances are high that no test will work at all,           |
| depending on how you configured "safe_mode" !             |
+-----------------------------------------------------------+


SAFE_MODE_WARNING;
}

// Don't ever guess at the PHP executable location.
// Require the explicit specification.
// Otherwise we could end up testing the wrong file!

if (getenv('TEST_PHP_EXECUTABLE')) {
	$php = getenv('TEST_PHP_EXECUTABLE');
	if ($php=='auto') {
		$php = $cwd.'/sapi/cli/php';
		putenv("TEST_PHP_EXECUTABLE=$php");
	}
}

if (empty($php) || !file_exists($php)) {
	error("environment variable TEST_PHP_EXECUTABLE must be set to specify PHP executable!");
}

if (getenv('TEST_PHP_LOG_FORMAT')) {
	$log_format = strtoupper(getenv('TEST_PHP_LOG_FORMAT'));
} else {
	$log_format = 'LEOD';
}

if (function_exists('is_executable') && !@is_executable($php)) {
	error("invalid PHP executable specified by TEST_PHP_EXECUTABLE  = " . $php);
}

// Check whether a detailed log is wanted.
if (getenv('TEST_PHP_DETAILED')) {
	$DETAILED = getenv('TEST_PHP_DETAILED');
} else {
	$DETAILED = 0;
}

// Check whether user test dirs are requested.
if (getenv('TEST_PHP_USER')) {
	$user_tests = explode (',', getenv('TEST_PHP_USER'));
} else {
	$user_tests = array();
}

$ini_overwrites = array(
		'output_handler=',
		'open_basedir=',
		'safe_mode=0',
		'disable_functions=',
		'output_buffering=Off',
		'error_reporting=4095',
		'display_errors=1',
		'log_errors=0',
		'html_errors=0',
		'track_errors=1',
		'report_memleaks=1',
		'report_zend_debug=0',
		'docref_root=',
		'docref_ext=.html',
		'error_prepend_string=',
		'error_append_string=',
		'auto_prepend_file=',
		'auto_append_file=',
		'magic_quotes_runtime=0',
	);

function write_information($show_html)
{
	global $cwd, $php, $php_info, $user_tests, $ini_overwrites, $pass_options;

	// Get info from php
	$info_file = realpath(dirname(__FILE__)) . '/run-test-info.php';
	@unlink($info_file);
	$php_info = '<?php echo "
PHP_SAPI    : " . PHP_SAPI . "
PHP_VERSION : " . phpversion() . "
ZEND_VERSION: " . zend_version() . "
PHP_OS      : " . PHP_OS . " - " . php_uname() . "
INI actual  : " . realpath(get_cfg_var("cfg_file_path")) . "
More .INIs  : " . (function_exists(\'php_ini_scanned_files\') ? str_replace("\n","", php_ini_scanned_files()) : "** not determined **"); ?>';
	save_text($info_file, $php_info);
	$info_params = array();
	settings2array($ini_overwrites,$info_params);
	settings2params($info_params);
	$php_info = `$php $pass_options $info_params $info_file`;
	@unlink($info_file);
	define('TESTED_PHP_VERSION', `$php -r 'echo PHP_VERSION;'`);

	// check for extensions that need special handling and regenerate
	$php_extensions = '<?php echo join(",",get_loaded_extensions()); ?>'; 
	save_text($info_file, $php_extensions);
	$php_extensions = explode(',',`$php $pass_options $info_params $info_file`);
	$info_params_ex = array(
		'session' => array('session.auto_start=0'),
		'zlib' => array('zlib.output_compression=Off'),
		'xdebug' => array('xdebug.default_enable=0'),
	);
	foreach($info_params_ex as $ext => $ini_overwrites_ex) {
		if (in_array($ext, $php_extensions)) {
			$ini_overwrites = array_merge($ini_overwrites, $ini_overwrites_ex);
		}
	}
	@unlink($info_file);

	// Write test context information.
	echo "
=====================================================================
CWD         : $cwd
PHP         : $php $php_info
Extra dirs  : ";
	foreach ($user_tests as $test_dir) {
		echo "{$test_dir}\n              ";
	}
	echo "
=====================================================================
";
}

// Determine the tests to be run.

$test_files = array();
$redir_tests = array();
$test_results = array();
$PHP_FAILED_TESTS = array('BORKED' => array(), 'FAILED' => array(), 'WARNED' => array(), 'LEAKED' => array());

// If parameters given assume they represent selected tests to run.
$failed_tests_file= false;
$pass_option_n = false;
$pass_options = '';

$compression = 0;
$output_file = $CUR_DIR . '/php_test_results_' . date('Ymd_Hi') . '.txt';
if ($compression) {
	$output_file = 'compress.zlib://' . $output_file . '.gz';
}
$just_save_results = false;
$leak_check = false;
$html_output = false;
$html_file = null;
$temp_source = null;
$temp_target = null;
$temp_urlbase = null;

if (getenv('TEST_PHP_ARGS'))
{
	if (!isset($argc) || !$argc || !isset($argv))
	{
		$argv = array(__FILE__);
	}
	$argv = array_merge($argv, split(' ', getenv('TEST_PHP_ARGS')));
	$argc = count($argv);
}

if (isset($argc) && $argc > 1) {
	for ($i=1; $i<$argc; $i++) {
		$is_switch = false;
		$switch = substr($argv[$i],1,1);
		$repeat = substr($argv[$i],0,1) == '-';
		while ($repeat) {
			$repeat = false;
			if (!$is_switch) {
				$switch = substr($argv[$i],1,1);
			}
			$is_switch = true;
			switch($switch) {
				case 'r':
				case 'l':
					$test_list = @file($argv[++$i]);
					if ($test_list) {
						foreach($test_list as $test) {
							$matches = array();
							if (preg_match('/^#.*\[(.*)\]\:\s+(.*)$/', $test, $matches)) {
								$redir_tests[] = array($matches[1], $matches[2]);
							} else if (strlen($test)) {
								$test_files[] = trim($test);
							}
						}
					}
					if ($switch != 'l') {
						break;
					}
					$i--;
					// break left intentionally
				case 'w':
					$failed_tests_file = fopen($argv[++$i], 'w+t');
					break;
				case 'a':
					$failed_tests_file = fopen($argv[++$i], 'a+t');
					break;
				case 'd':
					$ini_overwrites[] = $argv[++$i];
					break;
				//case 'h'
				//case 'l'
				case 'm':
					$leak_check = true;
					break;
				case 'n':
					if (!$pass_option_n) {
						$pass_options .= ' -n';
					}
					$pass_option_n = true;
					break;
				case 'q':
					putenv('NO_INTERACTION=1');
					break;
				//case 'r'
				case 's':
					$output_file = $argv[++$i];
					$just_save_results = true;
					break;
				case '--temp-source':
					$temp_source = $argv[++$i];
					break;
				case '--temp-target':
					$temp_target = $argv[++$i];
					if ($temp_urlbase) {
						$temp_urlbase = $temp_target;
					}
					break;
				case '--temp-urlbase':
					$temp_urlbase = $argv[++$i];
					break;
				case 'v':
				case '--verbose':
					$DETAILED = true;
					break;
				//case 'w'
				case '-':
					// repeat check with full switch
					$switch = $argv[$i];
					if ($switch != '-') {
						$repeat = true;
					}
					break;
				case '--html':
					$html_file = @fopen($argv[++$i], 'wt');
					$html_output = is_resource($html_file);
					break;
				case '--version':
					echo '$Revision$'."\n";
					exit(1);
				default:
					echo "Illegal switch '$switch' specified!\n";
					if ($switch == 'u' || $switch == 'U') {
						break;
					}
					// break
				case 'h':
				case '-help':
				case '--help':
					echo <<<HELP
Synopsis:
    php run-tests.php [options] [files] [directories]

Options:
    -l <file>   Read the testfiles to be executed from <file>. After the test 
                has finished all failed tests are written to the same <file>. 
                If the list is empty and no further test is specified then
                all tests are executed (same as: -r <file> -w <file>).

    -r <file>   Read the testfiles to be executed from <file>.

    -w <file>   Write a list of all failed tests to <file>.

    -a <file>   Same as -w but append rather then truncating <file>.

    -n          Pass -n option to the php binary (Do not use a php.ini).

    -d foo=bar  Pass -d option to the php binary (Define INI entry foo
                with value 'bar').

    -m          Test for memory leaks with Valgrind.
    
    -s <file>   Write output to <file>.

    -q          Quite, no user interaction (same as environment NO_INTERACTION).

    --verbose
    -v          Verbose mode.

    --help
    -h          This Help.

    --html <file> Generate HTML output.
	
    --temp-source <sdir>  --temp-target <tdir> [--temp-urlbase <url>]
                Write temporary files to <tdir> by replacing <sdir> from the 
                filenames to generate with <tdir>. If --html is being used and 
                <url> given then the generated links are relative and prefixed
                with the given url. In general you want to make <sdir> the path
                to your source files and <tdir> some pach in your web page 
                hierarchy with <url> pointing to <tdir>.

HELP;
					exit(1);
			}
		}
		if (!$is_switch) {
			$testfile = realpath($argv[$i]);
			if (is_dir($testfile)) {
				find_files($testfile);
			} else if (preg_match("/\.phpt$/", $testfile)) {
				$test_files[] = $testfile;
			} else {
				die("bogus test name " . $argv[$i] . "\n");
			}
		}
	}
	$test_files = array_unique($test_files);
	$test_files = array_merge($test_files, $redir_tests);

	// Run selected tests.
	$test_cnt = count($test_files);
	if ($test_cnt) {
		write_information($html_output);
		usort($test_files, "test_sort");
		$start_time = time();
		if (!$html_output) {
			echo "Running selected tests.\n";
		} else {
			show_start($start_time);
		}
		$test_idx = 0;
		run_all_tests($test_files);
		$end_time = time();
		if ($html_output) {
			show_end($end_time);
		}
		if ($failed_tests_file) {
			fclose($failed_tests_file);
		}
		if (count($test_files) || count($test_results)) {
			compute_summary();
			if ($html_output) {
				fwrite($html_file, "<hr/>\n" . get_summary(false, true));
			}
			echo "=====================================================================";
			echo get_summary(false, false);
		}
		if ($html_output) {
			fclose($html_file);
		}
		if (getenv('REPORT_EXIT_STATUS') == 1 and ereg('FAILED( |$)', implode(' ', $test_results))) {
			exit(1);
		}
		exit(0);
	}
}

write_information($html_output);

// Compile a list of all test files (*.phpt).
$test_files = array();
$exts_to_test = get_loaded_extensions();
$exts_tested = count($exts_to_test);
$exts_skipped = 0;
$ignored_by_ext = 0;
sort($exts_to_test);
$test_dirs = array('tests', 'ext');
$optionals = array('Zend', 'ZendEngine2');
foreach($optionals as $dir) {
	if (@filetype($dir) == 'dir') {
		$test_dirs[] = $dir;
	}
}

// Convert extension names to lowercase
foreach ($exts_to_test as $key => $val) {
	$exts_to_test[$key] = strtolower($val);
}

foreach ($test_dirs as $dir) {
	find_files("{$cwd}/{$dir}", ($dir == 'ext'));
}

foreach ($user_tests as $dir) {
	find_files($dir, ($dir == 'ext'));
}

function find_files($dir,$is_ext_dir=FALSE,$ignore=FALSE)
{
	global $test_files, $exts_to_test, $ignored_by_ext, $exts_skipped, $exts_tested;

	$o = opendir($dir) or error("cannot open directory: $dir");
	while (($name = readdir($o)) !== FALSE) {
		if (is_dir("{$dir}/{$name}") && !in_array($name, array('.', '..', 'CVS'))) {
			$skip_ext = ($is_ext_dir && !in_array(strtolower($name), $exts_to_test));
			if ($skip_ext) {
				$exts_skipped++;
			}
			find_files("{$dir}/{$name}", FALSE, $ignore || $skip_ext);
		}

		// Cleanup any left-over tmp files from last run.
		if (substr($name, -4) == '.tmp') {
			@unlink("$dir/$name");
			continue;
		}

		// Otherwise we're only interested in *.phpt files.
		if (substr($name, -5) == '.phpt') {
			if ($ignore) {
				$ignored_by_ext++;
			} else {
				$testfile = realpath("{$dir}/{$name}");
				$test_files[] = $testfile;
			}
		}
	}
	closedir($o);
}

function test_name($name)
{
	if (is_array($name)) {
		return $name[0] . ':' . $name[1];
	} else {
		return $name;
	}
}

function test_sort($a, $b)
{
	global $cwd;
	
	$a = test_name($a);
	$b = test_name($b);

	$ta = strpos($a, "{$cwd}/tests")===0 ? 1 + (strpos($a, "{$cwd}/tests/run-test")===0 ? 1 : 0) : 0;
	$tb = strpos($b, "{$cwd}/tests")===0 ? 1 + (strpos($b, "{$cwd}/tests/run-test")===0 ? 1 : 0) : 0;
	if ($ta == $tb) {
		return strcmp($a, $b);
	} else {
		return $tb - $ta;
	}
}

$test_files = array_unique($test_files);
usort($test_files, "test_sort");

$start_time = time();
show_start($start_time);

$test_cnt = count($test_files);
$test_idx = 0;
run_all_tests($test_files);
$end_time = time();
if ($failed_tests_file) {
	fclose($failed_tests_file);
}

// Summarize results

if (0 == count($test_results)) {
	echo "No tests were run.\n";
	return;
}

compute_summary();

show_end($end_time);
show_summary();

if ($html_output) {
	fclose($html_file);
}

define('PHP_QA_EMAIL', 'qa-reports@lists.php.net');
define('QA_SUBMISSION_PAGE', 'http://qa.php.net/buildtest-process.php');

/* We got failed Tests, offer the user to send an e-mail to QA team, unless NO_INTERACTION is set */
if (!getenv('NO_INTERACTION')) {
	$fp = fopen("php://stdin", "r+");
	echo "\nYou may have found a problem in PHP.\nWe would like to send this report automatically to the\n";
	echo "PHP QA team, to give us a better understanding of how\nthe test cases are doing. If you don't want to send it\n";
	echo "immediately, you can choose \"s\" to save the report to\na file that you can send us later.\n";
	echo "Do you want to send this report now? [Yns]: ";
	flush();
	$user_input = fgets($fp, 10);
	$just_save_results = (strtolower($user_input[0]) == 's');
}
if ($just_save_results || !getenv('NO_INTERACTION')) {	
	if ($just_save_results || strlen(trim($user_input)) == 0 || strtolower($user_input[0]) == 'y') {
		/*  
		 * Collect information about the host system for our report
		 * Fetch phpinfo() output so that we can see the PHP enviroment
		 * Make an archive of all the failed tests
		 * Send an email
		 */
		if ($just_save_results)
		{
			$user_input = 's';
		}
		/* Ask the user to provide an email address, so that QA team can contact the user */
		if (!strncasecmp($user_input, 'y', 1) || strlen(trim($user_input)) == 0) {
			echo "\nPlease enter your email address.\n(Your address will be mangled so that it will not go out on any\nmailinglist in plain text): ";
			flush();
			$user_email = trim(fgets($fp, 1024));
			$user_email = str_replace("@", " at ", str_replace(".", " dot ", $user_email));
		}
		
		$failed_tests_data = '';
		$sep = "\n" . str_repeat('=', 80) . "\n";
		
		$failed_tests_data .= $failed_test_summary . "\n";
		$failed_tests_data .= get_summary(true, false) . "\n";

		if ($sum_results['FAILED']) {
			foreach ($PHP_FAILED_TESTS['FAILED'] as $test_info) {
				$failed_tests_data .= $sep . $test_info['name'] . $test_info['info'];
				$failed_tests_data .= $sep . file_get_contents(realpath($test_info['output']));
				$failed_tests_data .= $sep . file_get_contents(realpath($test_info['diff']));
				$failed_tests_data .= $sep . "\n\n";
			}
			$status = "failed";
		} else {
			$status = "success";
		}
		
		$failed_tests_data .= "\n" . $sep . 'BUILD ENVIRONMENT' . $sep;
		$failed_tests_data .= "OS:\n" . PHP_OS . " - " . php_uname() . "\n\n";
		$ldd = $autoconf = $sys_libtool = $libtool = $compiler = 'N/A';

		if (substr(PHP_OS, 0, 3) != "WIN") {
			/* If PHP_AUTOCONF is set, use it; otherwise, use 'autoconf'. */
			if (!empty($_ENV['PHP_AUTOCONF'])) {
				$autoconf = shell_exec($_ENV['PHP_AUTOCONF'] . ' --version');
			} else {
				$autoconf = shell_exec('autoconf --version');
			}

			/* Always use the generated libtool - Mac OSX uses 'glibtool' */
			$libtool = shell_exec($CUR_DIR . '/libtool --version');

			/* Use shtool to find out if there is glibtool present (MacOSX) */
			$sys_libtool_path = shell_exec(dirname(__FILE__) . '/build/shtool path glibtool libtool');
			$sys_libtool = shell_exec(str_replace("\n", "", $sys_libtool_path) . ' --version');

			/* Try the most common flags for 'version' */
			$flags = array('-v', '-V', '--version');
			$cc_status=0;
			foreach($flags AS $flag) {
				system(getenv('CC')." $flag >/dev/null 2>&1", $cc_status);
				if ($cc_status == 0) {
					$compiler = shell_exec(getenv('CC')." $flag 2>&1");
					break;
				}
			}
			$ldd = shell_exec("ldd $php 2>/dev/null");
		}
		$failed_tests_data .= "Autoconf:\n$autoconf\n";
		$failed_tests_data .= "Bundled Libtool:\n$libtool\n";
		$failed_tests_data .= "System Libtool:\n$sys_libtool\n";
		$failed_tests_data .= "Compiler:\n$compiler\n";
		$failed_tests_data .= "Bison:\n". @shell_exec('bison --version'). "\n";
		$failed_tests_data .= "Libraries:\n$ldd\n";
		$failed_tests_data .= "\n";
		
		if (isset($user_email)) {
			$failed_tests_data .= "User's E-mail: ".$user_email."\n\n";
		}	
		
		$failed_tests_data .= $sep . "PHPINFO" . $sep;
		$failed_tests_data .= shell_exec($php.' -dhtml_errors=0 -i');
		
		if ($just_save_results || !mail_qa_team($failed_tests_data, $compression, $status)) {
			file_put_contents($output_file, $failed_tests_data);

			if (!$just_save_results) {
				echo "\nThe test script was unable to automatically send the report to PHP's QA Team\n";
			}

			echo "Please send ".$output_file." to ".PHP_QA_EMAIL." manually, thank you.\n";
		} else {
			fwrite($fp, "\nThank you for helping to make PHP better.\n");
			fclose($fp);
		}	
	}
}
 
if (getenv('REPORT_EXIT_STATUS') == 1 and $sum_results['FAILED']) {
	exit(1);
}
exit(0);

//
// Send Email to QA Team
//

function mail_qa_team($data, $compression, $status = FALSE)
{
	$url_bits = parse_url(QA_SUBMISSION_PAGE);
	if (empty($url_bits['port'])) $url_bits['port'] = 80;
	
	$data = "php_test_data=" . urlencode(base64_encode(preg_replace("/[\\x00]/", "[0x0]", $data)));
	$data_length = strlen($data);
	
	$fs = fsockopen($url_bits['host'], $url_bits['port'], $errno, $errstr, 10);
	if (!$fs) {
		return FALSE;
	}

	$php_version = urlencode(TESTED_PHP_VERSION);

	echo "\nPosting to {$url_bits['host']} {$url_bits['path']}\n";
	fwrite($fs, "POST ".$url_bits['path']."?status=$status&version=$php_version HTTP/1.1\r\n");
	fwrite($fs, "Host: ".$url_bits['host']."\r\n");
	fwrite($fs, "User-Agent: QA Browser 0.1\r\n");
	fwrite($fs, "Content-Type: application/x-www-form-urlencoded\r\n");
	fwrite($fs, "Content-Length: ".$data_length."\r\n\r\n");
	fwrite($fs, $data);
	fwrite($fs, "\r\n\r\n");
	fclose($fs);

	return 1;
} 
 
 
//
//  Write the given text to a temporary file, and return the filename.
//

function save_text($filename, $text, $filename_copy = null)
{
	global $DETAILED;

	if ($filename_copy && $filename_copy != $filename) {
		if (@file_put_contents($filename_copy, $text) === false) {
			error("Cannot open file '" . $filename_copy . "' (save_text)");
		}
	}
	if (@file_put_contents($filename, $text) === false) {
		error("Cannot open file '" . $filename . "' (save_text)");
	}
	if (1 < $DETAILED) echo "
FILE $filename {{{
$text
}}} 
";
}

//
//  Write an error in a format recognizable to Emacs or MSVC.
//

function error_report($testname, $logname, $tested) 
{
	$testname = realpath($testname);
	$logname  = realpath($logname);
	switch (strtoupper(getenv('TEST_PHP_ERROR_STYLE'))) {
	case 'MSVC':
		echo $testname . "(1) : $tested\n";
		echo $logname . "(1) :  $tested\n";
		break;
	case 'EMACS':
		echo $testname . ":1: $tested\n";
		echo $logname . ":1:  $tested\n";
		break;
	}
}

function system_with_timeout($commandline)
{
	global $leak_check;

	$data = "";
	
	$proc = proc_open($commandline, array(
		0 => array('pipe', 'r'),
		1 => array('pipe', 'w'),
		2 => array('pipe', 'w')
		), $pipes, null, null, array("suppress_errors" => true));

	if (!$proc)
		return false;

	fclose($pipes[0]);

	while (true) {
		/* hide errors from interrupted syscalls */
		$r = $pipes;
		$w = null;
		$e = null;
		$n = @stream_select($r, $w, $e, $leak_check ? 300 : 60);

		if ($n === 0) {
			/* timed out */
			$data .= "\n ** ERROR: process timed out **\n";
			proc_terminate($proc);
			return $data;
		} else if ($n > 0) {
			$line = fread($pipes[1], 8192);
			if (strlen($line) == 0) {
				/* EOF */
				break;
			}
			$data .= $line;
		}
	}
	$stat = proc_get_status($proc);
	if ($stat['signaled']) {
		$data .= "\nTermsig=".$stat['stopsig'];
	}
	$code = proc_close($proc);
	return $data;
}

function run_all_tests($test_files, $redir_tested = NULL)
{
	global $test_results, $failed_tests_file, $php, $test_cnt, $test_idx;

	foreach($test_files AS $name)
	{
		$index = is_array($name) ? $name[0] : $name;
		$test_idx++;
		$result = run_test($php, $name);
		if (!is_array($name) && $result != 'REDIR')
		{
			$test_results[$index] = $result;
			if ($failed_tests_file && ($result == 'FAILED' || $result == 'WARNED' || $result == 'LEAKED'))
			{
				if ($redir_tested)
				{
					fwrite($failed_tests_file, "# $redir_tested: $name\n");
				} else {
					fwrite($failed_tests_file, "$name\n");
				}
			}
		}
	}
}

//
//  Run an individual test case.
//
function run_test($php, $file)
{
	global $log_format, $info_params, $ini_overwrites, $cwd, $PHP_FAILED_TESTS, $pass_options, $DETAILED, $IN_REDIRECT, $test_cnt, $test_idx, $leak_check, $temp_source, $temp_target;

	$temp_filenames = null;
	$org_file = $file;
	
	if (is_array($file)) $file = $file[0];

	if ($DETAILED) echo "
=================
TEST $file
";

	// Load the sections of the test file.
	$section_text = array(
		'TEST'   => '',
		'SKIPIF' => '',
		'GET'    => '',
		'POST'   => '',
		'ARGS'   => '',
	);

	$fp = @fopen($file, "rt") or error("Cannot open test file: $file");

	$borked = false;
	$bork_info = '';
	if (!feof($fp)) {
		$line = fgets($fp);
	} else {
		$bork_info = "empty test [$file]";
		$borked = true;
	}
	if (!ereg('^--TEST--',$line,$r)) {
		$bork_info = "tests must start with --TEST-- [$file]";
		$borked = true;
	}
	$section = 'TEST';
	$secfile = false;
	$secdone = false;
	while (!feof($fp)) {
		$line = fgets($fp);

		// Match the beginning of a section.
		if (preg_match('/^--([A-Z]+)--/', $line, $r)) {
			$section = $r[1];
			$section_text[$section] = '';
			$secfile = $section == 'FILE' || $section == 'FILEEOF';
			$secdone = false;
			continue;
		}
		
		// Add to the section text.
		if (!$secdone) {
			$section_text[$section] .= $line;
		}

		// End of actual test?
		if ($secfile && preg_match('/^===DONE===/', $line, $r)) {
			$secdone = true;
		}
	}

	// the redirect section allows a set of tests to be reused outside of
	// a given test dir
	if (@count($section_text['REDIRECTTEST']) == 1) {
		if ($IN_REDIRECT) {
			$borked = true;
			$bork_info = "Can't redirect a test from within a redirected test";
		} else {
			$borked = false;
		}
	} else {
		if (@count($section_text['FILE']) + @count($section_text['FILEEOF']) != 1) {
			$bork_info = "missing section --FILE--";
			$borked = true;
		}
		if (@count($section_text['FILEEOF']) == 1) {
			$section_text['FILE'] = preg_replace("/[\r\n]+$/", '', $section_text['FILEEOF']);
			unset($section_text['FILEEOF']);
		}
		if ((@count($section_text['EXPECT']) + @count($section_text['EXPECTF']) + @count($section_text['EXPECTREGEX'])) != 1) {
			$bork_info = "missing section --EXPECT--, --EXPECTF-- or --EXPECTREGEX--";
			$borked = true;
		}
	}
	fclose($fp);

	if ($borked) {
		show_result("BORK", $bork_info);
		$PHP_FAILED_TESTS['BORKED'][] = array (
								'name' => $file,
								'test_name' => '',
								'output' => '',
								'diff'   => '',
								'info'   => "$bork_info [$file]",
		);
		return 'BORKED';
	}

	$shortname = str_replace($cwd.'/', '', $file);
	$tested = trim($section_text['TEST']);
	$tested_file = $shortname;

 	/* For GET/POST tests, check if cgi sapi is available and if it is, use it. */
 	if ((!empty($section_text['GET']) || !empty($section_text['POST']))) {
 		if (file_exists("./sapi/cgi/php")) {
 			$old_php = $php;
 			$php = realpath("./sapi/cgi/php") . ' -C ';
 		} else {
			show_result("SKIP", $tested, $tested_file, "reason: CGI not available");
 			return 'SKIPPED';
 		}
 	}

	show_test($test_idx, $shortname);

	if (is_array($IN_REDIRECT)) {
		$temp_dir = $test_dir = $IN_REDIRECT['dir'];
	} else {
		$temp_dir = $test_dir = realpath(dirname($file));
	}
	if ($temp_source && $temp_target) {
		$temp_dir = str_replace($temp_source, $temp_target, $temp_dir);
	}

	$diff_filename     = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'diff';
	$log_filename      = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'log';
	$exp_filename      = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'exp';
	$output_filename   = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'out';
	$memcheck_filename = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'mem';
	$temp_file         = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'php';
	$test_file         = $test_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'php';
	$temp_skipif       = $temp_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'skip';
	$test_skipif       = $test_dir . DIRECTORY_SEPARATOR . basename($file,'phpt').'skip';
	$tmp_post          = $temp_dir . DIRECTORY_SEPARATOR . uniqid('/phpt.');
	$tmp_relative_file = str_replace(dirname(__FILE__).DIRECTORY_SEPARATOR, '', $test_file) . 't';

	if ($temp_source && $temp_target) {
		$temp_skipif  .= '.phps';
		$temp_file    .= '.phps';
		$copy_file     = $temp_dir . DIRECTORY_SEPARATOR . basename(is_array($file) ? $file[1] : $file).'.phps';
		if (!is_dir(dirname($copy_file))) {
			@mkdir(dirname($copy_file), 0777, true) or error("Cannot create output directory - " . dirname($copy_file));
		}
		if (isset($section_text['FILE'])) {
			save_text($copy_file, $section_text['FILE']);
		}
		$temp_filenames = array(
			'file' => $copy_file,
			'diff' => $diff_filename, 
			'log'  => $log_filename,
			'exp'  => $exp_filename,
			'out'  => $output_filename,
			'mem'  => $memcheck_filename,
			'php'  => $temp_file,
			'skip' => $temp_skipif);
	}

	if (is_array($IN_REDIRECT)) {
		$tested = $IN_REDIRECT['prefix'] . ' ' . trim($section_text['TEST']);
		$tested_file = $tmp_relative_file;
		$section_text['FILE'] = "# original source file: $shortname\n" . $section_text['FILE'];
	}

	// unlink old test results	
	@unlink($diff_filename);
	@unlink($log_filename);
	@unlink($exp_filename);
	@unlink($output_filename);
	@unlink($memcheck_filename);
	@unlink($temp_file);
	@unlink($test_file);
	@unlink($temp_skipif);
	@unlink($test_skipif);
	@unlink($tmp_post);

	// Reset environment from any previous test.
	putenv("REDIRECT_STATUS=");
	putenv("QUERY_STRING=");
	putenv("PATH_TRANSLATED=");
	putenv("SCRIPT_FILENAME=");
	putenv("REQUEST_METHOD=");
	putenv("CONTENT_TYPE=");
	putenv("CONTENT_LENGTH=");

	// Check if test should be skipped.
	$info = '';
	$warn = false;
	if (array_key_exists('SKIPIF', $section_text)) {
		if (trim($section_text['SKIPIF'])) {
			$skipif_params = array();
			settings2array($ini_overwrites,$skipif_params);
			settings2params($skipif_params);

			save_text($test_skipif, $section_text['SKIPIF'], $temp_skipif);
			$extra = substr(PHP_OS, 0, 3) !== "WIN" ?
				"unset REQUEST_METHOD; unset QUERY_STRING; unset PATH_TRANSLATED; unset SCRIPT_FILENAME; unset REQUEST_METHOD;": "";
			$output = system_with_timeout("$extra $php -q $skipif_params $test_skipif");
			@unlink($test_skipif);
			if (!strncasecmp('skip', trim($output), 4)) {
				$reason = (eregi("^skip[[:space:]]*(.+)\$", trim($output))) ? eregi_replace("^skip[[:space:]]*(.+)\$", "\\1", trim($output)) : FALSE;
				if ($reason) {
					show_result("SKIP", $tested, $tested_file, "reason: $reason", $temp_filenames);
				} else {
					show_result("SKIP", $tested, $tested_file, '', $temp_filenames);
				}
				if (isset($old_php)) {
					$php = $old_php;
				}
				@unlink($test_skipif);
				return 'SKIPPED';
			}
			if (!strncasecmp('info', trim($output), 4)) {
				$reason = (ereg("^info[[:space:]]*(.+)\$", trim($output))) ? ereg_replace("^info[[:space:]]*(.+)\$", "\\1", trim($output)) : FALSE;
				if ($reason) {
					$info = " (info: $reason)";
				}
			}
			if (!strncasecmp('warn', trim($output), 4)) {
				$reason = (ereg("^warn[[:space:]]*(.+)\$", trim($output))) ? ereg_replace("^warn[[:space:]]*(.+)\$", "\\1", trim($output)) : FALSE;
				if ($reason) {
					$warn = true; /* only if there is a reason */
					$info = " (warn: $reason)";
				}
			}
		}
	}

	if (@count($section_text['REDIRECTTEST']) == 1) {
		$test_files = array();

		$IN_REDIRECT = eval($section_text['REDIRECTTEST']);
		$IN_REDIRECT['via'] = "via [$shortname]\n\t";
		$IN_REDIRECT['dir'] = realpath(dirname($file));
		$IN_REDIRECT['prefix'] = trim($section_text['TEST']);

		if (@count($IN_REDIRECT['TESTS']) == 1) {
			if (is_array($org_file)) {
				$test_files[] = $org_file[1];
			} else {
				$GLOBALS['test_files'] = $test_files;
				find_files($IN_REDIRECT['TESTS']);
				$test_files = $GLOBALS['test_files'];
			}
			$test_cnt += count($test_files) - 1;
			$test_idx--;

			show_redirect_start($IN_REDIRECT['TESTS'], $tested, $tested_file);

			// set up environment
			foreach ($IN_REDIRECT['ENV'] as $k => $v) {
				putenv("$k=$v");
			}
			putenv("REDIR_TEST_DIR=" . realpath($IN_REDIRECT['TESTS']) . DIRECTORY_SEPARATOR);
	
			usort($test_files, "test_sort");
			run_all_tests($test_files, $tested);
	
			show_redirect_ends($IN_REDIRECT['TESTS'], $tested, $tested_file);
	
			// clean up environment
			foreach ($IN_REDIRECT['ENV'] as $k => $v) {
				putenv("$k=");
			}
			putenv("REDIR_TEST_DIR=");
	
			// a redirected test never fails
			$IN_REDIRECT = false;
			return 'REDIR';
		}
	}
	if (is_array($org_file) || @count($section_text['REDIRECTTEST']) == 1) {
		if (is_array($org_file)) $file = $org_file[0];
		$bork_info = "Redirected test did not contain redirection info";
		show_result("BORK", $bork_info, '', $temp_filenames);
		$PHP_FAILED_TESTS['BORKED'][] = array (
								'name' => $file,
								'test_name' => '',
								'output' => '',
								'diff'   => '',
								'info'   => "$bork_info [$file]",
		);
		//$test_cnt -= 1;  // Only if is_array($org_file) ?
		//$test_idx--;
		return 'BORKED';
	}
	

	// Default ini settings
	$ini_settings = array();
	// additional ini overwrites
	//$ini_overwrites[] = 'setting=value';
	settings2array($ini_overwrites, $ini_settings);

	// Any special ini settings
	// these may overwrite the test defaults...
	if (array_key_exists('INI', $section_text)) {
		if (strpos($section_text['INI'], '{PWD}') !== false) {
			$section_text['INI'] = str_replace('{PWD}', dirname($file), $section_text['INI']);
		}
		settings2array(preg_split( "/[\n\r]+/", $section_text['INI']), $ini_settings);
	}
	settings2params($ini_settings);

	// We've satisfied the preconditions - run the test!
	save_text($test_file, $section_text['FILE'], $temp_file);
	if (array_key_exists('GET', $section_text)) {
		$query_string = trim($section_text['GET']);
	} else {
		$query_string = '';
	}

	if (!empty($section_text['ENV'])) {
		foreach (explode("\n", $section_text['ENV']) as $env) {
			($env = trim($env)) and putenv($env);
		}
	}

	putenv("REDIRECT_STATUS=1");
	putenv("QUERY_STRING=$query_string");
	putenv("PATH_TRANSLATED=$test_file");
	putenv("SCRIPT_FILENAME=$test_file");

	$args = $section_text['ARGS'] ? ' -- '.$section_text['ARGS'] : '';

	if (array_key_exists('POST', $section_text) && !empty($section_text['POST'])) {

		$post = trim($section_text['POST']);
		save_text($tmp_post, $post);
		$content_length = strlen($post);

		putenv("REQUEST_METHOD=POST");
		putenv("CONTENT_TYPE=application/x-www-form-urlencoded");
		putenv("CONTENT_LENGTH=$content_length");

		$cmd = "$php$pass_options$ini_settings -f \"$test_file\" 2>&1 < $tmp_post";

	} else {

		putenv("REQUEST_METHOD=GET");
		putenv("CONTENT_TYPE=");
		putenv("CONTENT_LENGTH=");

		if (empty($section_text['ENV'])) {
			$cmd = "$php$pass_options$ini_settings -f \"$test_file\" $args 2>&1";
		} else {
			$cmd = "$php$pass_options$ini_settings < \"$test_file\" $args 2>&1";
		}
	}

	if ($leak_check) {
		$cmd = "valgrind -q --tool=memcheck --log-file-exactly=$memcheck_filename $cmd";
	}

	if ($DETAILED) echo "
CONTENT_LENGTH  = " . getenv("CONTENT_LENGTH") . "
CONTENT_TYPE    = " . getenv("CONTENT_TYPE") . "
PATH_TRANSLATED = " . getenv("PATH_TRANSLATED") . "
QUERY_STRING    = " . getenv("QUERY_STRING") . "
REDIRECT_STATUS = " . getenv("REDIRECT_STATUS") . "
REQUEST_METHOD  = " . getenv("REQUEST_METHOD") . "
SCRIPT_FILENAME = " . getenv("SCRIPT_FILENAME") . "
COMMAND $cmd
";

//	$out = `$cmd`;
	$out = system_with_timeout($cmd);

	if (!empty($section_text['ENV'])) {
		foreach (explode("\n", $section_text['ENV']) as $env) {
			$env = explode('=', $env);
			putenv($env[0] .'=');
		}
	}

	@unlink($tmp_post);

	$leaked = false;
	$passed = false;

	if ($leak_check) { // leak check
		$leaked = @filesize($memcheck_filename) > 0;
		if (!$leaked) {
			@unlink($memcheck_filename);
		}
	}

	// Does the output match what is expected?
	$output = str_replace("\r\n", "\n", trim($out));

	/* when using CGI, strip the headers from the output */
	if (isset($old_php) && ($pos = strpos($output, "\n\n")) !== FALSE) {
		$output = substr($output, ($pos + 2));
	}

	if (isset($section_text['EXPECTF']) || isset($section_text['EXPECTREGEX'])) {
		if (isset($section_text['EXPECTF'])) {
			$wanted = trim($section_text['EXPECTF']);
		} else {
			$wanted = trim($section_text['EXPECTREGEX']);
		}
		$wanted_re = preg_replace('/\r\n/',"\n",$wanted);
		if (isset($section_text['EXPECTF'])) {
			$wanted_re = preg_quote($wanted_re, '/');
			// Stick to basics
			$wanted_re = str_replace("%e", '\\' . DIRECTORY_SEPARATOR, $wanted_re);
			$wanted_re = str_replace("%s", ".+?", $wanted_re); //not greedy
			$wanted_re = str_replace("%w", "\s*", $wanted_re);
			$wanted_re = str_replace("%i", "[+\-]?[0-9]+", $wanted_re);
			$wanted_re = str_replace("%d", "[0-9]+", $wanted_re);
			$wanted_re = str_replace("%x", "[0-9a-fA-F]+", $wanted_re);
			$wanted_re = str_replace("%f", "[+\-]?\.?[0-9]+\.?[0-9]*(E-?[0-9]+)?", $wanted_re);
			$wanted_re = str_replace("%c", ".", $wanted_re);
			// %f allows two points "-.0.0" but that is the best *simple* expression
		}
/* DEBUG YOUR REGEX HERE
		var_dump($wanted_re);
		print(str_repeat('=', 80) . "\n");
		var_dump($output);
*/
		if (preg_match("/^$wanted_re\$/s", $output)) {
			$passed = true;
			@unlink($test_file);
			if (isset($old_php)) {
				$php = $old_php;
			}
			if (!$leaked) {
			    show_result("PASS", $tested, $tested_file, '', $temp_filenames);
				return 'PASSED';
			}
		}
	} else {
		$wanted = trim($section_text['EXPECT']);
		$wanted = preg_replace('/\r\n/',"\n",$wanted);
		// compare and leave on success
		if (!strcmp($output, $wanted)) {
			$passed = true;
			@unlink($test_file);
			if (isset($old_php)) {
				$php = $old_php;
			}
			if (!$leaked) {
				show_result("PASS", $tested, $tested_file, '', $temp_filenames);
				return 'PASSED';
			}
		}
		$wanted_re = NULL;
	}

	// Test failed so we need to report details.
	
	if ($leaked) {
		$restype = 'LEAK';
	} else if ($warn) {
		$restype = 'WARN';
	} else {
		$restype = 'FAIL';
	}

	if (!$passed) {
		// write .exp
		if (strpos($log_format,'E') !== FALSE && file_put_contents($exp_filename, $wanted) === FALSE) {
			error("Cannot create expected test output - $exp_filename");
		}
	
		// write .out
		if (strpos($log_format,'O') !== FALSE && file_put_contents($output_filename, $output) === FALSE) {
			error("Cannot create test output - $output_filename");
		}
	
		// write .diff
		if (strpos($log_format,'D') !== FALSE && file_put_contents($diff_filename, generate_diff($wanted,$wanted_re,$output)) === FALSE) {
			error("Cannot create test diff - $diff_filename");
		}
	
		// write .log
		if (strpos($log_format,'L') !== FALSE && file_put_contents($log_filename, "
---- EXPECTED OUTPUT
$wanted
---- ACTUAL OUTPUT
$output
---- FAILED
") === FALSE) {
			error("Cannot create test log - $log_filename");
			error_report($file, $log_filename, $tested);
		}
	}

	show_result($restype, $tested, $tested_file, $info, $temp_filenames);

	$PHP_FAILED_TESTS[$restype.'ED'][] = array (
						'name' => $file,
						'test_name' => (is_array($IN_REDIRECT) ? $IN_REDIRECT['via'] : '') . $tested . " [$tested_file]",
						'output' => $output_filename,
						'diff'   => $diff_filename,
						'info'   => $info
						);

	if (isset($old_php)) {
		$php = $old_php;
	}

	return $restype.'ED';
}

function comp_line($l1,$l2,$is_reg)
{
	if ($is_reg) {
		return preg_match('/^'.$l1.'$/s', $l2);
	} else {
		return !strcmp($l1, $l2);
	}
}

function count_array_diff($ar1,$ar2,$is_reg,$w,$idx1,$idx2,$cnt1,$cnt2,$steps)
{
	$equal = 0;
	while ($idx1 < $cnt1 && $idx2 < $cnt2 && comp_line($ar1[$idx1], $ar2[$idx2], $is_reg)) {
		$idx1++;
		$idx2++;
		$equal++;
		$steps--;
	}
	if (--$steps > 0) {
		$eq1 = 0;
		$st = $steps / 2;
		for ($ofs1 = $idx1+1; $ofs1 < $cnt1 && $st-- > 0; $ofs1++) {
			$eq = count_array_diff($ar1,$ar2,$is_reg,$w,$ofs1,$idx2,$cnt1,$cnt2,$st);
			if ($eq > $eq1) {
				$eq1 = $eq;
			}
		}
		$eq2 = 0;
		$st = $steps;
		for ($ofs2 = $idx2+1; $ofs2 < $cnt2 && $st-- > 0; $ofs2++) {
			$eq = count_array_diff($ar1,$ar2,$is_reg,$w,$idx1,$ofs2,$cnt1,$cnt2,$st);
			if ($eq > $eq2) {
				$eq2 = $eq;
			}
		}
		if ($eq1 > $eq2) {
			$equal += $eq1;
		} else if ($eq2 > 0) {
			$equal += $eq2;
		}
	}
	return $equal;
}

function generate_array_diff($ar1,$ar2,$is_reg,$w)
{
	$idx1 = 0; $ofs1 = 0; $cnt1 = count($ar1);
	$idx2 = 0; $ofs2 = 0; $cnt2 = count($ar2);
	$diff = array();
	$old1 = array();
	$old2 = array();
	
	while ($idx1 < $cnt1 && $idx2 < $cnt2) {
		if (comp_line($ar1[$idx1], $ar2[$idx2], $is_reg)) {
			$idx1++;
			$idx2++;
			continue;
		} else {
			$c1 = count_array_diff($ar1,$ar2,$is_reg,$w,$idx1+1,$idx2,$cnt1,$cnt2,10);
			$c2 = count_array_diff($ar1,$ar2,$is_reg,$w,$idx1,$idx2+1,$cnt1,$cnt2,10);
			if ($c1 > $c2) {
				$old1[$idx1] = sprintf("%03d- ", $idx1+1).$w[$idx1++];
				$last = 1;
			} else if ($c2 > 0) {
				$old2[$idx2] = sprintf("%03d+ ", $idx2+1).$ar2[$idx2++];
				$last = 2;
			} else {
				$old1[$idx1] = sprintf("%03d- ", $idx1+1).$w[$idx1++];
				$old2[$idx2] = sprintf("%03d+ ", $idx2+1).$ar2[$idx2++];
			}
		}
	}
	
	reset($old1); $k1 = key($old1); $l1 = -2;
	reset($old2); $k2 = key($old2); $l2 = -2;  
	while ($k1 !== NULL || $k2 !== NULL) {
		if ($k1 == $l1+1 || $k2 === NULL) {
			$l1 = $k1;
			$diff[] = current($old1);
			$k1 = next($old1) ? key($old1) : NULL;
		} else if ($k2 == $l2+1 || $k1 === NULL) {
			$l2 = $k2;
			$diff[] = current($old2);
			$k2 = next($old2) ? key($old2) : NULL;
		} else if ($k1 < $k2) {
			$l1 = $k1;
			$diff[] = current($old1);
			$k1 = next($old1) ? key($old1) : NULL;
		} else {
			$l2 = $k2;
			$diff[] = current($old2);
			$k2 = next($old2) ? key($old2) : NULL;
		}
	}
	while ($idx1 < $cnt1) {
		$diff[] = sprintf("%03d- ", $idx1+1).$w[$idx1++];
	}
	while ($idx2 < $cnt2) {
		$diff[] = sprintf("%03d+ ", $idx2+1).$ar2[$idx2++];
	}
	return $diff;
}

function generate_diff($wanted,$wanted_re,$output)
{
	$w = explode("\n", $wanted);
	$o = explode("\n", $output);
	$r = is_null($wanted_re) ? $w : explode("\n", $wanted_re);
	$diff = generate_array_diff($r,$o,!is_null($wanted_re),$w);
	return implode("\r\n", $diff);
}

function error($message)
{
	echo "ERROR: {$message}\n";
	exit(1);
}

function settings2array($settings, &$ini_settings)
{
	foreach($settings as $setting) {
		if (strpos($setting, '=')!==false) {
			$setting = explode("=", $setting, 2);
			$name = trim(strtolower($setting[0]));
			$value = trim($setting[1]);
			$ini_settings[$name] = $value;
		}
	}
}

function settings2params(&$ini_settings)
{
	$settings = '';
	foreach($ini_settings as $name => $value) {
		$value = addslashes($value);
		$settings .= " -d \"$name=$value\"";
	}
	$ini_settings = $settings;
}

function compute_summary()
{
	global $n_total, $test_results, $ignored_by_ext, $sum_results, $percent_results;

	$n_total = count($test_results);
	$n_total += $ignored_by_ext;
	$sum_results = array('PASSED'=>0, 'WARNED'=>0, 'SKIPPED'=>0, 'FAILED'=>0, 'BORKED'=>0, 'LEAKED'=>0);
	foreach ($test_results as $v) {
		$sum_results[$v]++;
	}
	$sum_results['SKIPPED'] += $ignored_by_ext;
	$percent_results = array();
	while (list($v,$n) = each($sum_results)) {
		$percent_results[$v] = (100.0 * $n) / $n_total;
	}
}

function get_summary($show_ext_summary, $show_html)
{
	global $exts_skipped, $exts_tested, $n_total, $sum_results, $percent_results, $end_time, $start_time, $failed_test_summary, $PHP_FAILED_TESTS, $leak_check;

	$x_total = $n_total - $sum_results['SKIPPED'] - $sum_results['BORKED'];
	if ($x_total) {
		$x_warned = (100.0 * $sum_results['WARNED']) / $x_total;
		$x_failed = (100.0 * $sum_results['FAILED']) / $x_total;
		$x_leaked = (100.0 * $sum_results['LEAKED']) / $x_total;
		$x_passed = (100.0 * $sum_results['PASSED']) / $x_total;
	} else {
		$x_warned = $x_failed = $x_passed = $x_leaked = 0;
	}

	$summary = "";
	if ($show_html) $summary .= "<pre>\n";
	if ($show_ext_summary) {
		$summary .= "
=====================================================================
TEST RESULT SUMMARY
---------------------------------------------------------------------
Exts skipped    : " . sprintf("%4d",$exts_skipped) . "
Exts tested     : " . sprintf("%4d",$exts_tested) . "
---------------------------------------------------------------------
";
	}
	$summary .= "
Number of tests : " . sprintf("%4d",$n_total). "          " . sprintf("%8d",$x_total);
	if ($sum_results['BORKED']) {
	$summary .= "
Tests borked    : " . sprintf("%4d (%5.1f%%)",$sum_results['BORKED'],$percent_results['BORKED']) . " --------";
	}
	$summary .= "
Tests skipped   : " . sprintf("%4d (%5.1f%%)",$sum_results['SKIPPED'],$percent_results['SKIPPED']) . " --------
Tests warned    : " . sprintf("%4d (%5.1f%%)",$sum_results['WARNED'], $percent_results['WARNED']) . " " . sprintf("(%5.1f%%)",$x_warned) . "
Tests failed    : " . sprintf("%4d (%5.1f%%)",$sum_results['FAILED'], $percent_results['FAILED']) . " " . sprintf("(%5.1f%%)",$x_failed);
	if ($leak_check) {
		$summary .= "
Tests leaked    : " . sprintf("%4d (%5.1f%%)",$sum_results['LEAKED'], $percent_results['LEAKED']) . " " . sprintf("(%5.1f%%)",$x_leaked);
	}
	$summary .= "
Tests passed    : " . sprintf("%4d (%5.1f%%)",$sum_results['PASSED'], $percent_results['PASSED']) . " " . sprintf("(%5.1f%%)",$x_passed) . "
---------------------------------------------------------------------
Time taken      : " . sprintf("%4d seconds", $end_time - $start_time) . "
=====================================================================
";
	$failed_test_summary = '';
	if (count($PHP_FAILED_TESTS['BORKED'])) {
		$failed_test_summary .= "
=====================================================================
BORKED TEST SUMMARY
---------------------------------------------------------------------
";
		foreach ($PHP_FAILED_TESTS['BORKED'] as $failed_test_data) {
			$failed_test_summary .= $failed_test_data['info'] . "\n";
		}
		$failed_test_summary .=  "=====================================================================\n";
	}
	
	if (count($PHP_FAILED_TESTS['FAILED'])) {
		$failed_test_summary .= "
=====================================================================
FAILED TEST SUMMARY
---------------------------------------------------------------------
";
		foreach ($PHP_FAILED_TESTS['FAILED'] as $failed_test_data) {
			$failed_test_summary .= $failed_test_data['test_name'] . $failed_test_data['info'] . "\n";
		}
		$failed_test_summary .=  "=====================================================================\n";
	}
	
	if (count($PHP_FAILED_TESTS['LEAKED'])) {
		$failed_test_summary .= "
=====================================================================
LEAKED TEST SUMMARY
---------------------------------------------------------------------
";
		foreach ($PHP_FAILED_TESTS['LEAKED'] as $failed_test_data) {
			$failed_test_summary .= $failed_test_data['test_name'] . $failed_test_data['info'] . "\n";
		}
		$failed_test_summary .=  "=====================================================================\n";
	}
	
	if ($failed_test_summary && !getenv('NO_PHPTEST_SUMMARY')) {
		$summary .= $failed_test_summary;
	}

	if ($show_html) $summary .= "</pre>";

	return $summary;
}

function show_start($start_time)
{
	global $html_output, $html_file;

	if ($html_output)
	{
		fwrite($html_file, "<h2>Time Start: " . date('Y-m-d H:i:s', $start_time) . "</h2>\n");
		fwrite($html_file, "<table>\n");
	}
	echo "TIME START " . date('Y-m-d H:i:s', $start_time) . "\n=====================================================================\n";
}

function show_end($end_time)
{
	global $html_output, $html_file;

	if ($html_output)
	{
		fwrite($html_file, "</table>\n");
		fwrite($html_file, "<h2>Time End: " . date('Y-m-d H:i:s', $end_time) . "</h2>\n");
	}
	echo "=====================================================================\nTIME END " . date('Y-m-d H:i:s', $end_time) . "\n";
}

function show_summary()
{
	global $html_output, $html_file;

	if ($html_output)
	{
		fwrite($html_file, "<hr/>\n" . get_summary(true, true));
	}
	echo get_summary(true, false);
}

function show_redirect_start($tests, $tested, $tested_file)
{
	global $html_output, $html_file;

	if ($html_output)
	{
		fwrite($html_file, "<tr><td colspan='3'>---&gt; $tests ($tested [$tested_file]) begin</td></tr>\n");
	}
	echo "---> $tests ($tested [$tested_file]) begin\n";
}

function show_redirect_ends($tests, $tested, $tested_file)
{
	global $html_output, $html_file;

	if ($html_output)
	{
		fwrite($html_file, "<tr><td colspan='3'>---&gt; $tests ($tested [$tested_file]) done</td></tr>\n");
	}
	echo "---> $tests ($tested [$tested_file]) done\n";
}

function show_test($test_idx, $shortname)
{
	global $test_cnt;

	echo "TEST $test_idx/$test_cnt [$shortname]\r";
	flush();
}

function show_result($result, $tested, $tested_file, $extra = '', $temp_filenames = null)
{
	global $html_output, $html_file, $temp_target, $temp_urlbase;

	echo "$result $tested [$tested_file] $extra\n";

	if ($html_output)
	{
		if (isset($temp_filenames['file']) && @file_exists($temp_filenames['file'])) {
			$url = str_replace($temp_target, $temp_urlbase, $temp_filenames['file']);
			$tested = "<a href='$url'>$tested</a>";
		}
		if (isset($temp_filenames['skip']) && @file_exists($temp_filenames['skip'])) {
			if (empty($extra)) {
				$extra = "skipif";
			}
			$url = str_replace($temp_target, $temp_urlbase, $temp_filenames['skip']);
			$extra = "<a href='$url'>$extra</a>";
		} else if (empty($extra)) {
			$extra = "&nbsp;";
		}
		if (isset($temp_filenames['diff']) && @file_exists($temp_filenames['diff'])) {
			$url = str_replace($temp_target, $temp_urlbase, $temp_filenames['diff']);
			$diff = "<a href='$url'>diff</a>";
		} else {
			$diff = "&nbsp;";
		}
		if (isset($temp_filenames['mem']) && @file_exists($temp_filenames['mem'])) {
			$url = str_replace($temp_target, $temp_urlbase, $temp_filenames['mem']);
			$mem = "<a href='$url'>leaks</a>";
		} else {
			$mem = "&nbsp;";
		}
		fwrite($html_file, 
			"<tr>" .
			"<td>$result</td>" .
			"<td>$tested</td>" .
			"<td>$extra</td>" .
			"<td>$diff</td>" . 
			"<td>$mem</td>" .
			"</tr>\n");
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: noet sw=4 ts=4
 */
?>
