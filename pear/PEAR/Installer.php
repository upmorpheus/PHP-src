<?php
//
// +----------------------------------------------------------------------+
// | PHP Version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 3.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at the following url:           |
// | http://www.php.net/license/3_0.txt.                                  |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Stig Bakken <ssb@php.net>                                   |
// |          Tomas V.V.Cox <cox@idecnet.com>                             |
// |          Martin Jansen <mj@php.net>                                  |
// +----------------------------------------------------------------------+
//
// $Id$

require_once 'PEAR/Common.php';
require_once 'PEAR/Registry.php';
require_once 'PEAR/Dependency.php';
require_once 'System.php';

define('PEAR_INSTALLER_OK',       1);
define('PEAR_INSTALLER_FAILED',   0);
define('PEAR_INSTALLER_SKIPPED', -1);
define('PEAR_INSTALLER_ERROR_NO_PREF_STATE', 2);

/**
 * Administration class used to install PEAR packages and maintain the
 * installed package database.
 *
 * TODO:
 *   - Check dependencies break on package uninstall (when no force given)
 *   - add a guessInstallDest() method with the code from _installFile() and
 *     use that method in Registry::_rebuildFileMap() & Command_Registry::doList(),
 *     others..
 *
 * @since PHP 4.0.2
 * @author Stig Bakken <ssb@php.net>
 * @author Martin Jansen <mj@php.net>
 */
class PEAR_Installer extends PEAR_Common
{
    // {{{ properties

    /** name of the package directory, for example Foo-1.0
     * @var string
     */
    var $pkgdir;

    /** directory where PHP code files go
     * @var string
     */
    var $phpdir;

    /** directory where PHP extension files go
     * @var string
     */
    var $extdir;

    /** directory where documentation goes
     * @var string
     */
    var $docdir;

    /** installation root directory (ala PHP's INSTALL_ROOT or
     * automake's DESTDIR
     * @var string
     */
    var $installroot = '';

    /** debug level
     * @var int
     */
    var $debug = 1;

    /** temporary directory
     * @var string
     */
    var $tmpdir;

    /** PEAR_Registry object used by the installer
     * @var object
     */
    var $registry;

    /** List of file transactions queued for an install/upgrade/uninstall.
     *
     *  Format:
     *    array(
     *      0 => array("rename => array("from-file", "to-file")),
     *      1 => array("delete" => array("file-to-delete")),
     *      ...
     *    )
     *
     * @var array
     */
    var $file_operations = array();

    // }}}

    // {{{ constructor

    /**
     * PEAR_Installer constructor.
     *
     * @param object $ui user interface object (instance of PEAR_Frontend_*)
     *
     * @access public
     */
    function PEAR_Installer(&$ui)
    {
        parent::PEAR_Common();
        $this->setFrontendObject($ui);
        $this->debug = $this->config->get('verbose');
        //$this->registry = &new PEAR_Registry($this->config->get('php_dir'));
    }

    // }}}

    // {{{ _deletePackageFiles()

    /**
     * Delete a package's installed files, does not remove empty directories.
     *
     * @param string $package package name
     *
     * @return bool TRUE on success, or a PEAR error on failure
     *
     * @access private
     */
    function _deletePackageFiles($package)
    {
        if (!strlen($package)) {
            return $this->raiseError("No package to uninstall given");
        }
        $filelist = $this->registry->packageInfo($package, 'filelist');
        if ($filelist == null) {
            return $this->raiseError("$package not installed");
        }
        foreach ($filelist as $file => $props) {
            if (empty($props['installed_as'])) {
                continue;
            }
            $path = $this->_prependPath($props['installed_as'], $this->installroot);
            $this->addFileOperation('delete', array($path));
        }
        return true;
    }

    // }}}
    // {{{ _installFile()

    /**
     * @param string filename
     * @param array attributes from <file> tag in package.xml
     * @param string path to install the file in
     * @param array options from command-line
     * @access private
     */
    function _installFile($file, $atts, $tmp_path, $options)
    {
        // {{{ return if this file is meant for another platform
        static $os;
        if (isset($atts['platform'])) {
            if (empty($os)) {
                include_once "OS/Guess.php";
                $os = new OS_Guess();
            }
            if (!$os->matchSignature($atts['platform'])) {
                $this->log(3, "skipped $file (meant for $atts[platform], we are ".$os->getSignature().")");
                return PEAR_INSTALLER_SKIPPED;
            }
        }
        // }}}

        // {{{ assemble the destination paths
        switch ($atts['role']) {
            case 'doc':
            case 'data':
            case 'test':
                $dest_dir = $this->config->get($atts['role'] . '_dir') .
                            DIRECTORY_SEPARATOR . $this->pkginfo['package'];
                unset($atts['baseinstalldir']);
                break;
            case 'ext':
            case 'php':
                $dest_dir = $this->config->get($atts['role'] . '_dir');
                break;
            case 'script':
                $dest_dir = $this->config->get('bin_dir');
                break;
            case 'src':
            case 'extsrc':
                $this->source_files++;
                return;
            default:
                return $this->raiseError("Invalid role `$atts[role]' for file $file");
        }
        $save_destdir = $dest_dir;
        if (!empty($atts['baseinstalldir'])) {
            $dest_dir .= DIRECTORY_SEPARATOR . $atts['baseinstalldir'];
        }
        if (dirname($file) != '.' && empty($atts['install-as'])) {
            $dest_dir .= DIRECTORY_SEPARATOR . dirname($file);
        }
        if (empty($atts['install-as'])) {
            $dest_file = $dest_dir . DIRECTORY_SEPARATOR . basename($file);
        } else {
            $dest_file = $dest_dir . DIRECTORY_SEPARATOR . $atts['install-as'];
        }
        $orig_file = $tmp_path . DIRECTORY_SEPARATOR . $file;

        // Clean up the DIRECTORY_SEPARATOR mess
        $ds2 = DIRECTORY_SEPARATOR . DIRECTORY_SEPARATOR;
        list($dest_file, $orig_file) = preg_replace(array('!\\\\+!', '!/!', "!$ds2+!"),
                                                    DIRECTORY_SEPARATOR,
                                                    array($dest_file, $orig_file));
        $installed_as = $dest_file;
        $final_dest_file = $this->_prependPath($dest_file, $this->installroot);
        $dest_dir = dirname($final_dest_file);
        $dest_file = $dest_dir . DIRECTORY_SEPARATOR . '.tmp' . basename($final_dest_file);
        // }}}

        if (!@is_dir($dest_dir)) {
            if (!$this->mkDirHier($dest_dir)) {
                return $this->raiseError("failed to mkdir $dest_dir",
                                         PEAR_INSTALLER_FAILED);
            }
            $this->log(3, "+ mkdir $dest_dir");
        }
        if (empty($atts['replacements'])) {
            if (!file_exists($orig_file)) {
                return $this->raiseError("file does not exist",
                                         PEAR_INSTALLER_FAILED);
            }
            if (!@copy($orig_file, $dest_file)) {
                return $this->raiseError("failed to write $dest_file",
                                         PEAR_INSTALLER_FAILED);
            }
            $this->log(3, "+ cp $orig_file $dest_file");
            if (isset($atts['md5sum'])) {
                $md5sum = md5_file($dest_file);
            }
        } else {
            // {{{ file with replacements
            if (!file_exists($orig_file)) {
                return $this->raiseError("file does not exist",
                                         PEAR_INSTALLER_FAILED);
            }
            $fp = fopen($orig_file, "r");
            $contents = fread($fp, filesize($orig_file));
            fclose($fp);
            if (isset($atts['md5sum'])) {
                $md5sum = md5($contents);
            }
            $subst_from = $subst_to = array();
            foreach ($atts['replacements'] as $a) {
                $to = '';
                if ($a['type'] == 'php-const') {
                    if (preg_match('/^[a-z0-9_]+$/i', $a['to'])) {
                        eval("\$to = $a[to];");
                    } else {
                        $this->log(0, "invalid php-const replacement: $a[to]");
                        continue;
                    }
                } elseif ($a['type'] == 'pear-config') {
                    $to = $this->config->get($a['to']);
                    if (is_null($to)) {
                        $this->log(0, "invalid pear-config replacement: $a[to]");
                        continue;
                    }
                } elseif ($a['type'] == 'package-info') {
                    if (isset($this->pkginfo[$a['to']]) && is_string($this->pkginfo[$a['to']])) {
                        $to = $this->pkginfo[$a['to']];
                    } else {
                        $this->log(0, "invalid package-info replacement: $a[to]");
                        continue;
                    }
                }
                if (!is_null($to)) {
                    $subst_from[] = $a['from'];
                    $subst_to[] = $to;
                }
            }
            $this->log(3, "doing ".sizeof($subst_from)." substitution(s) for $final_dest_file");
            if (sizeof($subst_from)) {
                $contents = str_replace($subst_from, $subst_to, $contents);
            }
            $wp = @fopen($dest_file, "wb");
            if (!is_resource($wp)) {
                return $this->raiseError("failed to create $dest_file: $php_errormsg",
                                         PEAR_INSTALLER_FAILED);
            }
            if (!fwrite($wp, $contents)) {
                return $this->raiseError("failed writing to $dest_file: $php_errormsg",
                                         PEAR_INSTALLER_FAILED);
            }
            fclose($wp);
            // }}}
        }
        // {{{ check the md5
        if (isset($md5sum)) {
            if (strtolower($md5sum) == strtolower($atts['md5sum'])) {
                $this->log(2, "md5sum ok: $final_dest_file");
            } else {
                if (empty($options['force'])) {
                    // delete the file
                    @unlink($dest_file);
                    return $this->raiseError("bad md5sum for file $final_dest_file",
                                             PEAR_INSTALLER_FAILED);
                } else {
                    $this->log(0, "warning : bad md5sum for file $final_dest_file");
                }
            }
        }
        // }}}
        // {{{ set file permissions
        if (!OS_WINDOWS) {
            if ($atts['role'] == 'script') {
                $mode = 0777 & ~(int)octdec($this->config->get('umask'));
                $this->log(3, "+ chmod +x $dest_file");
            } else {
                $mode = 0666 & ~(int)octdec($this->config->get('umask'));
            }
            $this->addFileOperation("chmod", array($mode, $dest_file));
            if (!@chmod($dest_file, $mode)) {
                $this->log(0, "failed to change mode of $dest_file");
            }
        }
        // }}}
        $this->addFileOperation("rename", array($dest_file, $final_dest_file));
        // Store the full path where the file was installed for easy unistall
        $this->addFileOperation("installed_as", array($file, $installed_as,
                                $save_destdir, dirname(substr($dest_file, strlen($save_destdir)))));

        //$this->log(2, "installed: $dest_file");
        return PEAR_INSTALLER_OK;
    }

    // }}}
    // {{{ addFileOperation()

    /**
     * Add a file operation to the current file transaction.
     *
     * @see startFileTransaction()
     * @var string $type This can be one of:
     *    - rename:  rename a file ($data has 2 values)
     *    - chmod:   change permissions on a file ($data has 2 values)
     *    - delete:  delete a file ($data has 1 value)
     *    - rmdir:   delete a directory if empty ($data has 1 value)
     *    - installed_as: mark a file as installed ($data has 4 values).
     * @var array $data For all file operations, this array must contain the
     *    full path to the file or directory that is being operated on.  For
     *    the rename command, the first parameter must be the file to rename,
     *    the second its new name.
     *
     *    The installed_as operation contains 4 elements in this order:
     *    1. Filename as listed in the filelist element from package.xml
     *    2. Full path to the installed file
     *    3. Full path from the php_dir configuration variable used in this
     *       installation
     *    4. Relative path from the php_dir that this file is installed in
     */
    function addFileOperation($type, $data)
    {
        if (!is_array($data)) {
            return $this->raiseError('Internal Error: $data in addFileOperation'
                . ' must be an array, was ' . gettype($data));
        }
        if ($type == 'chmod') {
            $octmode = decoct($data[0]);
            $this->log(3, "adding to transaction: $type $octmode $data[1]");
        } else {
            $this->log(3, "adding to transaction: $type " . implode(" ", $data));
        }
        $this->file_operations[] = array($type, $data);
    }

    // }}}
    // {{{ startFileTransaction()

    function startFileTransaction($rollback_in_case = false)
    {
        if (count($this->file_operations) && $rollback_in_case) {
            $this->rollbackFileTransaction();
        }
        $this->file_operations = array();
    }

    // }}}
    // {{{ commitFileTransaction()

    function commitFileTransaction()
    {
        $n = count($this->file_operations);
        $this->log(2, "about to commit $n file operations");
        // {{{ first, check permissions and such manually
        $errors = array();
        foreach ($this->file_operations as $tr) {
            list($type, $data) = $tr;
            switch ($type) {
                case 'rename':
                    if (!file_exists($data[0])) {
                        $errors[] = "cannot rename file $data[0], doesn't exist";
                    }
                    // check that dest dir. is writable
                    if (!is_writable(dirname($data[1]))) {
                        $errors[] = "permission denied ($type): $data[1]";
                    }
                    break;
                case 'chmod':
                    // check that file is writable
                    if (!is_writable($data[1])) {
                        $errors[] = "permission denied ($type): $data[1] " . decoct($data[0]);
                    }
                    break;
                case 'delete':
                    if (!file_exists($data[0])) {
                        $this->log(2, "warning: file $data[0] doesn't exist, can't be deleted");
                    }
                    // check that directory is writable
                    if (file_exists($data[0]) && !is_writable(dirname($data[0]))) {
                        $errors[] = "permission denied ($type): $data[0]";
                    }
                    break;
            }

        }
        // }}}
        $m = sizeof($errors);
        if ($m > 0) {
            foreach ($errors as $error) {
                $this->log(1, $error);
            }
            return false;
        }
        // {{{ really commit the transaction
        foreach ($this->file_operations as $tr) {
            list($type, $data) = $tr;
            switch ($type) {
                case 'rename':
                    @unlink($data[1]);
                    @rename($data[0], $data[1]);
                    $this->log(3, "+ mv $data[0] $data[1]");
                    break;
                case 'chmod':
                    @chmod($data[1], $data[0]);
                    $octmode = decoct($data[0]);
                    $this->log(3, "+ chmod $octmode $data[1]");
                    break;
                case 'delete':
                    @unlink($data[0]);
                    $this->log(3, "+ rm $data[0]");
                    break;
                case 'rmdir':
                    @rmdir($data[0]);
                    $this->log(3, "+ rmdir $data[0]");
                    break;
                case 'installed_as':
                    $this->pkginfo['filelist'][$data[0]]['installed_as'] = $data[1];
                    if (!isset($this->pkginfo['filelist']['dirtree'][dirname($data[1])])) {
                        $this->pkginfo['filelist']['dirtree'][dirname($data[1])] = true;
                        while(!empty($data[3]) && $data[3] != '/' && $data[3] != '\\'
                              && $data[3] != '.') {
                            $this->pkginfo['filelist']['dirtree']
                                [$this->_prependPath($data[3], $data[2])] = true;
                            $data[3] = dirname($data[3]);
                        }
                    }
                    break;
            }
        }
        // }}}
        $this->log(2, "successfully committed $n file operations");
        $this->file_operations = array();
        return true;
    }

    // }}}
    // {{{ rollbackFileTransaction()

    function rollbackFileTransaction()
    {
        $n = count($this->file_operations);
        $this->log(2, "rolling back $n file operations");
        foreach ($this->file_operations as $tr) {
            list($type, $data) = $tr;
            switch ($type) {
                case 'rename':
                    @unlink($data[0]);
                    $this->log(3, "+ rm $data[0]");
                    break;
                case 'mkdir':
                    @rmdir($data[0]);
                    $this->log(3, "+ rmdir $data[0]");
                    break;
                case 'chmod':
                    break;
                case 'delete':
                    break;
                case 'installed_as':
                    if (isset($this->pkginfo['filelist'])) {
                        unset($this->pkginfo['filelist'][$data[0]]['installed_as']);
                    }
                    if (isset($this->pkginfo['filelist']['dirtree'][dirname($data[1])])) {
                        unset($this->pkginfo['filelist']['dirtree'][dirname($data[1])]);
                        while(!empty($data[3]) && $data[3] != '/' && $data[3] != '\\'
                              && $data[3] != '.') {
                            unset($this->pkginfo['filelist']['dirtree']
                                [$this->_prependPath($data[3], $data[2])]);
                            $data[3] = dirname($data[3]);
                        }
                    }
                    if (isset($this->pkginfo['filelist']['dirtree'])
                          && !count($this->pkginfo['filelist']['dirtree'])) {
                        unset($this->pkginfo['filelist']['dirtree']);
                    }
                    break;
            }
        }
        $this->file_operations = array();
    }

    // }}}
    // {{{ getPackageDownloadUrl()

    function getPackageDownloadUrl($package, $version = null)
    {
        if ($version) {
            $package .= "-$version";
        }
        if ($this === null || $this->config === null) {
            $package = "http://pear.php.net/get/$package";
        } else {
            $package = "http://" . $this->config->get('master_server') .
                "/get/$package";
        }
        if (!extension_loaded("zlib")) {
            $package .= '?uncompress=yes';
        }
        return $package;
    }

    // }}}
    // {{{ mkDirHier($dir)

    function mkDirHier($dir)
    {
        $this->addFileOperation('mkdir', array($dir));
        return parent::mkDirHier($dir);
    }

    // }}}
    // {{{ _prependPath($path, $prepend)

    function _prependPath($path, $prepend)
    {
        if (strlen($prepend) > 0) {
            if (OS_WINDOWS && preg_match('/^[a-z]:/i', $path)) {
                $path = $prepend . substr($path, 2);
            } else {
                $path = $prepend . $path;
            }
        }
        return $path;
    }

    // }}}
    // {{{ extractDownloadFileName($pkgfile, &$version)

    function extractDownloadFileName($pkgfile, &$version)
    {
        if (@is_file($pkgfile)) {
            return $pkgfile;
        }
        // regex defined in Common.php
        if (preg_match(PEAR_COMMON_PACKAGE_DOWNLOAD_PREG, $pkgfile, $m)) {
            $version = (isset($m[3])) ? $m[3] : null;
            return $m[1];
        }
        $version = null;
        return $pkgfile;
    }

    // }}}
    // {{{ _downloadFile()
    /**
     * @param string filename to download
     * @param PEAR_Config Configuration object
     * @param array options returned from Console_GetOpt
     * @param array empty array to populate with error messages, if any
     * @param string version/state
     * @param string original value passed to command-line
     * @param string preferred state (snapshot/devel/alpha/beta/stable)
     * @return null|PEAR_Error|string
     * @access private
     */
    function _downloadFile($pkgfile, &$config, $options, &$errors, $version,
                           $origpkgfile, $state)
    {
        // {{{ check the package filename, and whether it's already installed
        $need_download = false;
        if (preg_match('#^(http|ftp)://#', $pkgfile)) {
            $need_download = true;
        } elseif (!@is_file($pkgfile)) {
            if ($this->validPackageName($pkgfile)) {
                if ($this->registry->packageExists($pkgfile)) {
                    if (empty($options['upgrade']) && empty($options['force'])) {
                        $errors[] = "$pkgfile already installed";
                        return;
                    }
                }
                $pkgfile = $this->getPackageDownloadUrl($pkgfile, $version);
                $need_download = true;
            } else {
                if (strlen($pkgfile)) {
                    $errors[] = "Could not open the package file: $pkgfile";
                } else {
                    $errors[] = "No package file given";
                }
                return;
            }
        }
        // }}}

        // {{{ Download package -----------------------------------------------
        if ($need_download) {
            $downloaddir = $config->get('download_dir');
            if (empty($downloaddir)) {
                if (PEAR::isError($downloaddir = System::mktemp('-d'))) {
                    return $downloaddir;
                }
                $this->log(3, '+ tmp dir created at ' . $downloaddir);
            }
            $callback = $this->ui ? array(&$this, '_downloadCallback') : null;
            $this->pushErrorHandling(PEAR_ERROR_RETURN);
            $file = $this->downloadHttp($pkgfile, $this->ui, $downloaddir, $callback);
            $this->popErrorHandling();
            if (PEAR::isError($file)) {
                if ($this->validPackageName($origpkgfile)) {
                    include_once 'PEAR/Remote.php';
                    $remote = new PEAR_Remote($config);
                    if (!PEAR::isError($info = $remote->call('package.info',
                          $origpkgfile))) {
                        if (!count($info['releases'])) {
                            return $this->raiseError('Package ' . $origpkgfile .
                            ' has no releases');
                        } else {
                            return $this->raiseError('No releases of preferred state "'
                            . $state . '" exist for package ' . $origpkgfile .
                            '.  Use ' . $origpkgfile . '-state to install another' .
                            ' state (like ' . $origpkgfile .'-beta)',
                            PEAR_INSTALLER_ERROR_NO_PREF_STATE);
                        }
                    } else {
                        return $pkgfile;
                    }
                } else {
                    return $this->raiseError($file);
                }
            }
            $pkgfile = $file;
        }
        // }}}
        return $pkgfile;
    }

    // }}}
    // {{{ download()

    /**
     * Download any files and their dependencies, if necessary
     *
     * @param array a mixed list of package names, local files, or package.xml
     * @param PEAR_Config
     * @param array options from the command line
     * @param array this is the array that will be populated with packages to
     *              install.  Format of each entry:
     *
     * <code>
     * array('pkg' => 'package_name', 'file' => '/path/to/local/file',
     *    'info' => array() // parsed package.xml
     * );
     * </code>
     * @param array this will be populated with any error messages
     * @param false private recursion variable
     * @param false private recursion variable
     * @param false private recursion variable
     */
    function download($packages, $options, &$config, &$installpackages,
                      &$errors, $installed = false, $willinstall = false, $state = false)
    {
        // recognized options:
        // - onlyreqdeps   : install all required dependencies as well
        // - alldeps       : install all dependencies, including optional
        //
        // {{{ determine preferred state, installroot, etc
        if (!$willinstall) {
            $willinstall = array();
        }
        if (!$state) {
            $state = $config->get('preferred_state');
            if (!$state) {
                // don't inadvertantly use a non-set preferred_state
                $state = null;
            }
        }
        $mywillinstall = array();
        $php_dir = $config->get('php_dir');
        if (isset($options['installroot'])) {
            if (substr($options['installroot'], -1) == DIRECTORY_SEPARATOR) {
                $options['installroot'] = substr($options['installroot'], 0, -1);
            }
            $php_dir = $this->_prependPath($php_dir, $options['installroot']);
            $this->installroot = $options['installroot'];
        } else {
            $this->installroot = '';
        }
        // }}}
        $this->registry = &new PEAR_Registry($php_dir);

        // {{{ download files in this list if necessary
        foreach($packages as $pkgfile) {
            if (!is_file($pkgfile)) {
                $origpkgfile = $pkgfile;
                $pkgfile = $this->extractDownloadFileName($pkgfile, $version);
                if (preg_match('#^(http|ftp)://#', $pkgfile)) {
                    $pkgfile = $this->_downloadFile($pkgfile, $config, $options,
                                                    $errors, $version, $origpkgfile,
                                                    $state);
                    if (PEAR::isError($pkgfile)) {
                        return $pkgfile;
                    }
                    $tempinfo = $this->infoFromAny($pkgfile);
                    if (isset($options['alldeps']) || isset($options['onlyreqdeps'])) {
                        // ignore dependencies if there are any errors
                        if (!PEAR::isError($tempinfo)) {
                            $mywillinstall[strtolower($tempinfo['package'])] = @$tempinfo['release_deps'];
                        }
                    }
                    $installpackages[] = array('pkg' => $tempinfo['package'],
                                               'file' => $pkgfile, 'info' => $tempinfo);
                    continue;
                }
                if (!$this->validPackageName($pkgfile)) {
                    return $this->raiseError("Package name '$pkgfile' not valid");
                }
                // ignore packages that are installed unless we are upgrading
                $curinfo = $this->registry->packageInfo($pkgfile);
                if ($this->registry->packageExists($pkgfile) && empty($options['upgrade']) && empty($options['force'])) {
                    $this->log(0, "Package '{$curinfo['package']}' already installed, skipping");
                    continue;
                }
                // Retrieve remote release list
                include_once 'PEAR/Remote.php';
                $curver = $curinfo['version'];
                $remote = &new PEAR_Remote($config);
                $releases = $remote->call('package.info', $pkgfile, 'releases');
                if (!count($releases)) {
                    return $this->raiseError("No releases found for package '$pkgfile'");
                }
                // Want a specific version/state
                if ($version !== null) {
                    // Passed Foo-1.2
                    if ($this->validPackageVersion($version)) {
                        if (!isset($releases[$version])) {
                            return $this->raiseError("No release with version '$version' found for '$pkgfile'");
                        }
                    // Passed Foo-alpha
                    } elseif (in_array($version, $this->getReleaseStates())) {
                        $state = $version;
                        $version = 0;
                        foreach ($releases as $ver => $inf) {
                            if ($inf['state'] == $state && version_compare("$version", "$ver") < 0) {
                                $version = $ver;
                            }
                        }
                        if ($version == 0) {
                            return $this->raiseError("No release with state '$state' found for '$pkgfile'");
                        }
                    // invalid postfix passed
                    } else {
                        return $this->raiseError("Invalid postfix '-$version', be sure to pass a valid PEAR ".
                                                 "version number or release state");
                    }
                // Guess what to download
                } else {
                    $states = $this->betterStates($state, true);
                    $possible = false;
                    $version = 0;
                    foreach ($releases as $ver => $inf) {
                        if (in_array($inf['state'], $states) && version_compare("$version", "$ver") < 0) {
                            $version = $ver;
                        }
                    }
                    if ($version == 0 && !isset($options['force'])) {
                        return $this->raiseError('No release with state equal to: \'' . implode(', ', $states) .
                                                 "' found for '$pkgfile'");
                    } elseif ($version == 0) {
                        $this->log(0, "Warning: $pkgfile is state '$inf[state]' which is less stable " .
                                      "than state '$state'");
                    }
                }
                // Check if we haven't already the version
                if (empty($options['force'])) {
                    if ($curinfo['version'] == $version) {
                        $this->log(0, "Package '{$curinfo['package']}-{$curinfo['version']}' already installed, skipping");
                        continue;
                    } elseif (version_compare("$version", "{$curinfo['version']}") < 0) {
                        $this->log(0, "Already got '{$curinfo['package']}-{$curinfo['version']}' greater than requested '$version', skipping");
                        continue;
                    }
                }
                $pkgfile = $this->_downloadFile($pkgfile, $config, $options,
                                                $errors, $version, $origpkgfile,
                                                $state);
                if (PEAR::isError($pkgfile)) {
                    return $pkgfile;
                }
            } // end is_file()
            $tempinfo = $this->infoFromAny($pkgfile);
            if (isset($options['alldeps']) || isset($options['onlyreqdeps'])) {
                // ignore dependencies if there are any errors
                if (!PEAR::isError($tempinfo)) {
                    $mywillinstall[strtolower($tempinfo['package'])] = @$tempinfo['release_deps'];
                }
            }
            $installpackages[] = array('pkg' => $tempinfo['package'],
                                       'file' => $pkgfile, 'info' => $tempinfo);
        } // end foreach($packages)
        // }}}

        // {{{ extract dependencies from downloaded files and then download
        // them if necessary
        if (isset($options['alldeps']) || isset($options['onlyreqdeps'])) {
            include_once "PEAR/Remote.php";
            $remote = new PEAR_Remote($config);
            if (!$installed) {
                $installed = $this->registry->listPackages();
                array_walk($installed, create_function('&$v,$k','$v = strtolower($v);'));
                $installed = array_flip($installed);
            }
            $deppackages = array();
            // {{{ construct the list of dependencies for each file
            foreach ($mywillinstall as $package => $alldeps) {
                if (!is_array($alldeps)) {
                    continue;
                }
                foreach($alldeps as $info) {
                    if ($info['type'] != 'pkg') {
                        continue;
                    }
                    if (!isset($options['alldeps']) && isset($info['optional']) &&
                          $info['optional'] == 'yes') {
                        // skip optional deps
                        $this->log(0, "skipping Package $package optional dependency $info[name]");
                        continue;
                    }
                    // {{{ get releases
                    $releases = $remote->call('package.info', $info['name'], 'releases');
                    if (PEAR::isError($releases)) {
                        return $releases;
                    }
                    if (!count($releases)) {
                        if (!isset($installed[strtolower($info['name'])])) {
                            $errors[] = "Package $package dependency $info[name] ".
                                        "has no releases";
                        }
                        continue;
                    }
                    $found = false;
                    $save = $releases;
                    while(count($releases) && !$found) {
                        if (!empty($state) && $state != 'any') {
                            list($release_version,$release) = each($releases);
                            if ($state != $release['state'] &&
                                !in_array($release['state'], $this->betterStates($state)))
                            {
                                // drop this release - it ain't stable enough
                                array_shift($releases);
                            } else {
                                $found = true;
                            }
                        } else {
                            $found = true;
                        }
                    }
                    if (!count($releases) && !$found) {
                        $get = array();
                        foreach($save as $release) {
                            $get = array_merge($get,
                                $this->betterStates($release['state'], true));
                        }
                        $savestate = array_shift($get);
                        $errors[] = "Release for $package dependency $info[name] " .
                            "has state '$savestate', requires $state";
                        continue;
                    }
                    if (in_array(strtolower($info['name']), $willinstall) ||
                          isset($mywillinstall[strtolower($info['name'])])) {
                        // skip upgrade check for packages we will install
                        continue;
                    }
                    if (!isset($installed[strtolower($info['name'])])) {
                        // check to see if we can install the specific version required
                        if ($info['rel'] == 'eq') {
                            $deppackages[] = $info['name'] . '-' . $info['version'];
                            continue;
                        }
                        // skip upgrade check for packages we don't have installed
                        $deppackages[] = $info['name'];
                        continue;
                    }
                    // }}}

                    // {{{ see if a dependency must be upgraded
                    $inst_version = $this->registry->packageInfo($info['name'], 'version');
                    if (!isset($info['version'])) {
                        // this is a rel='has' dependency, check against latest
                        if (version_compare($release_version, $inst_version, 'le')) {
                            continue;
                        } else {
                            $deppackages[] = $info['name'];
                            continue;
                        }
                    }
                    if (version_compare($info['version'], $inst_version, 'le')) {
                        // installed version is up-to-date
                        continue;
                    }
                    $deppackages[] = $info['name'];
                    // }}}
                } // foreach($alldeps
            }
            // }}} foreach($willinstall

            if (count($deppackages)) {
                // check dependencies' dependencies
                // combine the list of packages to install
                $temppack = array();
                foreach($installpackages as $p) {
                    $temppack[] = strtolower($p['info']['package']);
                }
                foreach($deppackages as $pack) {
                    $temppack[] = strtolower($pack);
                }
                $willinstall = array_merge($willinstall, $temppack);
                $this->download($deppackages, $options, $config, $installpackages,
                                $errors, $installed, $willinstall, $state);
            }
        } // }}} if --alldeps or --onlyreqdeps
    }

    // }}}
    // {{{ install()

    /**
     * Installs the files within the package file specified.
     *
     * @param string $pkgfile path to the package file
     * @param array $options
     * recognized options:
     * - installroot   : optional prefix directory for installation
     * - force         : force installation
     * - register-only : update registry but don't install files
     * - upgrade       : upgrade existing install
     * - soft          : fail silently
     * - nodeps        : ignore dependency conflicts/missing dependencies
     * - alldeps       : install all dependencies
     * - onlyreqdeps   : install only required dependencies
     *
     * @return array|PEAR_Error package info if successful
     */

    function install($pkgfile, $options = array())
    {
        $php_dir = $this->config->get('php_dir');
        if (isset($options['installroot'])) {
            if (substr($options['installroot'], -1) == DIRECTORY_SEPARATOR) {
                $options['installroot'] = substr($options['installroot'], 0, -1);
            }
            $php_dir = $this->_prependPath($php_dir, $options['installroot']);
            $this->installroot = $options['installroot'];
        } else {
            $this->installroot = '';
        }
        $this->registry = &new PEAR_Registry($php_dir);
        //  ==> XXX should be removed later on
        $flag_old_format = false;

        if (substr($pkgfile, -4) == '.xml') {
            $descfile = $pkgfile;
        } else {
            // {{{ Decompress pack in tmp dir -------------------------------------

            // To allow relative package file names
            $pkgfile = realpath($pkgfile);

            if (PEAR::isError($tmpdir = System::mktemp('-d'))) {
                return $tmpdir;
            }
            $this->log(3, '+ tmp dir created at ' . $tmpdir);

            $tar = new Archive_Tar($pkgfile);
            if (!@$tar->extract($tmpdir)) {
                return $this->raiseError("unable to unpack $pkgfile");
            }

            // {{{ Look for existing package file
            $descfile = $tmpdir . DIRECTORY_SEPARATOR . 'package.xml';

            if (!is_file($descfile)) {
                // ----- Look for old package archive format
                // In this format the package.xml file was inside the
                // Package-n.n directory
                $dp = opendir($tmpdir);
                do {
                    $pkgdir = readdir($dp);
                } while ($pkgdir{0} == '.');

                $descfile = $tmpdir . DIRECTORY_SEPARATOR . $pkgdir . DIRECTORY_SEPARATOR . 'package.xml';
                $flag_old_format = true;
                $this->log(0, "warning : you are using an archive with an old format");
            }
            // }}}
            // <== XXX This part should be removed later on
            // }}}
        }

        if (!is_file($descfile)) {
            return $this->raiseError("no package.xml file after extracting the archive");
        }

        // Parse xml file -----------------------------------------------
        $pkginfo = $this->infoFromDescriptionFile($descfile);
        if (PEAR::isError($pkginfo)) {
            return $pkginfo;
        }
        $this->validatePackageInfo($pkginfo, $errors, $warnings);
        // XXX We allow warnings, do we have to do it?
        if (count($errors)) {
            if (empty($options['force'])) {
                return $this->raiseError("The following errors where found (use force option to install anyway):\n".
                                         implode("\n", $errors));
            } else {
                $this->log(0, "warning : the following errors were found:\n".
                           implode("\n", $errors));
            }
        }

        $pkgname = $pkginfo['package'];

        // {{{ Check dependencies -------------------------------------------
        if (isset($pkginfo['release_deps']) && empty($options['nodeps'])) {
            $dep_errors = '';
            $error = $this->checkDeps($pkginfo, $dep_errors);
            if ($error == true) {
                if (empty($options['soft'])) {
                    $this->log(0, substr($dep_errors, 1));
                }
                return $this->raiseError("$pkgname: Dependencies failed");
            } else if (!empty($dep_errors)) {
                // Print optional dependencies
                if (empty($options['soft'])) {
                    $this->log(0, $dep_errors);
                }
            }
        }
        // }}}

        // {{{ checks to do when not in "force" mode
        if (empty($options['force'])) {
            $test = $this->registry->checkFileMap($pkginfo);
            if (sizeof($test)) {
                $tmp = $test;
                foreach ($tmp as $file => $pkg) {
                    if ($pkg == $pkgname) {
                        unset($test[$file]);
                    }
                }
                if (sizeof($test)) {
                    $msg = "$pkgname: conflicting files found:\n";
                    $longest = max(array_map("strlen", array_keys($test)));
                    $fmt = "%${longest}s (%s)\n";
                    foreach ($test as $file => $pkg) {
                        $msg .= sprintf($fmt, $file, $pkg);
                    }
                    return $this->raiseError($msg);
                }
            }
        }
        // }}}

        $this->startFileTransaction();

        if (empty($options['upgrade'])) {
            // checks to do only when installing new packages
            if (empty($options['force']) && $this->registry->packageExists($pkgname)) {
                return $this->raiseError("$pkgname already installed");
            }
        } else {
            if ($this->registry->packageExists($pkgname)) {
                $v1 = $this->registry->packageInfo($pkgname, 'version');
                $v2 = $pkginfo['version'];
                $cmp = version_compare("$v1", "$v2", 'gt');
                if (empty($options['force']) && !version_compare("$v2", "$v1", 'gt')) {
                    return $this->raiseError("upgrade to a newer version ($v2 is not newer than $v1)");
                }
                if (empty($options['register-only'])) {
                    // when upgrading, remove old release's files first:
                    if (PEAR::isError($err = $this->_deletePackageFiles($pkgname))) {
                        return $this->raiseError($err);
                    }
                }
            }
        }

        // {{{ Copy files to dest dir ---------------------------------------

        // info from the package it self we want to access from _installFile
        $this->pkginfo = &$pkginfo;
        // used to determine whether we should build any C code
        $this->source_files = 0;

        if (empty($options['register-only'])) {
            if (!is_dir($php_dir)) {
                return $this->raiseError("no script destination directory\n",
                                         null, PEAR_ERROR_DIE);
            }

            $tmp_path = dirname($descfile);
            if (substr($pkgfile, -4) != '.xml') {
                $tmp_path .= DIRECTORY_SEPARATOR . $pkgname . '-' . $pkginfo['version'];
            }

            //  ==> XXX This part should be removed later on
            if ($flag_old_format) {
                $tmp_path = dirname($descfile);
            }
            // <== XXX This part should be removed later on

            // {{{ install files
            foreach ($pkginfo['filelist'] as $file => $atts) {
                $this->expectError(PEAR_INSTALLER_FAILED);
                $res = $this->_installFile($file, $atts, $tmp_path, $options);
                $this->popExpect();
                if (PEAR::isError($res)) {
                    if (empty($options['ignore-errors'])) {
                        $this->rollbackFileTransaction();
                        if ($res->getMessage() == "file does not exist") {
                            $this->raiseError("file $file in package.xml does not exist");
                        }
                        return $this->raiseError($res);
                    } else {
                        $this->log(0, "Warning: " . $res->getMessage());
                    }
                }
                if ($res != PEAR_INSTALLER_OK) {
                    // Do not register files that were not installed
                    unset($pkginfo['filelist'][$file]);
                }
            }
            // }}}

            // {{{ compile and install source files
            if ($this->source_files > 0 && empty($options['nobuild'])) {
                $this->log(1, "$this->source_files source files, building");
                $bob = &new PEAR_Builder($this->ui);
                $bob->debug = $this->debug;
                $built = $bob->build($descfile, array(&$this, '_buildCallback'));
                if (PEAR::isError($built)) {
                    $this->rollbackFileTransaction();
                    return $built;
                }
                $this->log(1, "\nBuild process completed successfully");
                foreach ($built as $ext) {
                    $bn = basename($ext['file']);
                    list($_ext_name, ) = explode('.', $bn);
                    if (extension_loaded($_ext_name)) {
                        $this->raiseError("Extension '$_ext_name' already loaded. Please unload it ".
                                          "in your php.ini file prior to install or upgrade it.");
                    }
                    $dest = $this->config->get('ext_dir') . DIRECTORY_SEPARATOR . $bn;
                    $this->log(1, "Installing '$bn' at ext_dir ($dest)");
                    $this->log(3, "+ cp $ext[file] ext_dir ($dest)");
                    $copyto = $this->_prependPath($dest, $this->installroot);
                    if (!@copy($ext['file'], $copyto)) {
                        $this->rollbackFileTransaction();
                        return $this->raiseError("failed to copy $bn to $copyto");
                    }
                    $pkginfo['filelist'][$bn] = array(
                        'role' => 'ext',
                        'installed_as' => $dest,
                        'php_api' => $ext['php_api'],
                        'zend_mod_api' => $ext['zend_mod_api'],
                        'zend_ext_api' => $ext['zend_ext_api'],
                        );
                }
            }
            // }}}
        }

        if (!$this->commitFileTransaction()) {
            $this->rollbackFileTransaction();
            return $this->raiseError("commit failed", PEAR_INSTALLER_FAILED);
        }
        // }}}

        $ret = false;
        // {{{ Register that the package is installed -----------------------
        if (empty($options['upgrade'])) {
            // if 'force' is used, replace the info in registry
            if (!empty($options['force']) && $this->registry->packageExists($pkgname)) {
                $this->registry->deletePackage($pkgname);
            }
            $ret = $this->registry->addPackage($pkgname, $pkginfo);
        } else {
            // new: upgrade installs a package if it isn't installed
            if (!$this->registry->packageExists($pkgname)) {
                $ret = $this->registry->addPackage($pkgname, $pkginfo);
            } else {
                $ret = $this->registry->updatePackage($pkgname, $pkginfo, false);
            }
        }
        if (!$ret) {
            return $this->raiseError("Adding package $pkgname to registry failed");
        }
        // }}}
        return $pkginfo;
    }

    // }}}
    // {{{ uninstall()

    /**
     * Uninstall a package
     *
     * This method removes all files installed by the application, and then
     * removes any empty directories.
     * @param string package name
     * @param array Command-line options.  Possibilities include:
     *
     *              - installroot: base installation dir, if not the default
     */
    function uninstall($package, $options = array())
    {
        $php_dir = $this->config->get('php_dir');
        if (isset($options['installroot'])) {
            if (substr($options['installroot'], -1) == DIRECTORY_SEPARATOR) {
                $options['installroot'] = substr($options['installroot'], 0, -1);
            }
            $this->installroot = $options['installroot'];
            $php_dir = $this->_prependPath($php_dir, $this->installroot);
        } else {
            $this->installroot = '';
        }
        $this->registry = &new PEAR_Registry($php_dir);
        $filelist = $this->registry->packageInfo($package, 'filelist');
        if ($filelist == null) {
            return $this->raiseError("$package not installed");
        }
        if (empty($options['nodeps'])) {
            $depchecker = &new PEAR_Dependency($this->registry);
            $error = $depchecker->checkPackageUninstall($errors, $warning, $package);
            if ($error) {
                return $this->raiseError($errors . 'uninstall failed');
            }
            if ($warning) {
                $this->log(0, $warning);
            }
        }
        // {{{ Delete the files
        $this->startFileTransaction();
        if (PEAR::isError($err = $this->_deletePackageFiles($package))) {
            $this->rollbackFileTransaction();
            return $this->raiseError($err);
        }
        if (!$this->commitFileTransaction()) {
            $this->rollbackFileTransaction();
            return $this->raiseError("uninstall failed");
        } else {
            $this->startFileTransaction();
            if (!isset($filelist['dirtree']) || !count($filelist['dirtree'])) {
                return $this->registry->deletePackage($package);
            }
            // attempt to delete empty directories
            uksort($filelist['dirtree'], array($this, '_sortDirs'));
            foreach($filelist['dirtree'] as $dir => $notused) {
                $this->addFileOperation('rmdir', array($dir));
            }
            if (!$this->commitFileTransaction()) {
                $this->rollbackFileTransaction();
            }
        }
        // }}}

        // Register that the package is no longer installed
        return $this->registry->deletePackage($package);
    }

    // }}}
    // {{{ _sortDirs()
    function _sortDirs($a, $b)
    {
        if (strnatcmp($a, $b) == -1) return 1;
        if (strnatcmp($a, $b) == 1) return -1;
        return 0;
    }

    // }}}
    // {{{ checkDeps()

    /**
     * Check if the package meets all dependencies
     *
     * @param  array   Package information (passed by reference)
     * @param  string  Error message (passed by reference)
     * @return boolean False when no error occured, otherwise true
     */
    function checkDeps(&$pkginfo, &$errors)
    {
        if (empty($this->registry)) {
            $this->registry = &new PEAR_Registry($this->config->get('php_dir'));
        }
        $depchecker = &new PEAR_Dependency($this->registry);
        $error = $errors = '';
        $failed_deps = $optional_deps = array();
        if (is_array($pkginfo['release_deps'])) {
            foreach($pkginfo['release_deps'] as $dep) {
                $code = $depchecker->callCheckMethod($error, $dep);
                if ($code) {
                    if (isset($dep['optional']) && $dep['optional'] == 'yes') {
                        $optional_deps[] = array($dep, $code, $error);
                    } else {
                        $failed_deps[] = array($dep, $code, $error);
                    }
                }
            }
            // {{{ failed dependencies
            $n = count($failed_deps);
            if ($n > 0) {
                for ($i = 0; $i < $n; $i++) {
                    if (isset($failed_deps[$i]['type'])) {
                        $type = $failed_deps[$i]['type'];
                    } else {
                        $type = 'pkg';
                    }
                    switch ($failed_deps[$i][1]) {
                        case PEAR_DEPENDENCY_MISSING:
                            if ($type == 'pkg') {
                                // install
                            }
                            $errors .= "\n" . $failed_deps[$i][2];
                            break;
                        case PEAR_DEPENDENCY_UPGRADE_MINOR:
                            if ($type == 'pkg') {
                                // upgrade
                            }
                            $errors .= "\n" . $failed_deps[$i][2];
                            break;
                        default:
                            $errors .= "\n" . $failed_deps[$i][2];
                            break;
                    }
                }
                return true;
            }
            // }}}

            // {{{ optional dependencies
            $count_optional = count($optional_deps);
            if ($count_optional > 0) {
                $errors = "Optional dependencies:";

                for ($i = 0; $i < $count_optional; $i++) {
                    if (isset($optional_deps[$i]['type'])) {
                        $type = $optional_deps[$i]['type'];
                    } else {
                        $type = 'pkg';
                    }
                    switch ($optional_deps[$i][1]) {
                        case PEAR_DEPENDENCY_MISSING:
                        case PEAR_DEPENDENCY_UPGRADE_MINOR:
                        default:
                            $errors .= "\n" . $optional_deps[$i][2];
                            break;
                    }
                }
                return false;
            }
            // }}}
        }
        return false;
    }

    // }}}
    // {{{ _downloadCallback()

    function _downloadCallback($msg, $params = null)
    {
        switch ($msg) {
            case 'saveas':
                $this->log(1, "downloading $params ...");
                break;
            case 'done':
                $this->log(1, '...done: ' . number_format($params, 0, '', ',') . ' bytes');
                break;
            case 'bytesread':
                static $bytes;
                if (empty($bytes)) {
                    $bytes = 0;
                }
                if (!($bytes % 10240)) {
                    $this->log(1, '.', false);
                }
                $bytes += $params;
                break;
            case 'start':
                $this->log(1, "Starting to download {$params[0]} (".number_format($params[1], 0, '', ',')." bytes)");
                break;
        }
        if (method_exists($this->ui, '_downloadCallback'))
            $this->ui->_downloadCallback($msg, $params);
    }

    // }}}
    // {{{ _buildCallback()

    function _buildCallback($what, $data)
    {
        if (($what == 'cmdoutput' && $this->debug > 1) ||
            ($what == 'output' && $this->debug > 0)) {
            $this->ui->outputData(rtrim($data), 'build');
        }
    }

    // }}}
}

// {{{ md5_file() utility function
if (!function_exists("md5_file")) {
    function md5_file($filename) {
        $fp = fopen($filename, "r");
        if (!$fp) return null;
        $contents = fread($fp, filesize($filename));
        fclose($fp);
        return md5($contents);
    }
}
// }}}

?>
