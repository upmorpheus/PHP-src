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
// +----------------------------------------------------------------------+
//
// $Id$

require_once 'PEAR/Common.php';
require_once 'System.php';

/**
 * Administration class used to make a PEAR release tarball.
 *
 * TODO:
 *  - add an extra param the dir where to place the created package
 *
 * @since PHP 4.0.2
 * @author Stig Bakken <ssb@php.net>
 */
class PEAR_Packager extends PEAR_Common
{
    // {{{ constructor

    function PEAR_Packager()
    {
        parent::PEAR_Common();
    }

    // }}}
    // {{{ destructor

    function _PEAR_Packager()
    {
        parent::_PEAR_Common();
    }

    // }}}

    // {{{ package()

    function package($pkgfile = null, $compress = true)
    {
        // {{{ validate supplied package.xml file
        if (empty($pkgfile)) {
            $pkgfile = 'package.xml';
        }
        // $this->pkginfo gets populated inside
        $pkginfo = $this->infoFromDescriptionFile($pkgfile);
        if (PEAR::isError($pkginfo)) {
            return $this->raiseError($pkginfo);
        }

        $pkgdir = dirname(realpath($pkgfile));
        $pkgfile = basename($pkgfile);

        $errors = $warnings = array();
        $this->validatePackageInfo($pkginfo, $errors, $warnings, $pkgdir);
        foreach ($warnings as $w) {
            $this->log(1, "Warning: $w");
        }
        foreach ($errors as $e) {
            $this->log(0, "Error: $e");
        }
        if (sizeof($errors) > 0) {
            return $this->raiseError('Errors in package');
        }
        // }}}

        $pkgver = $pkginfo['package'] . '-' . $pkginfo['version'];

        // {{{ Create the package file list
        $filelist = array();
        $i = 0;

        foreach ($pkginfo['filelist'] as $fname => $atts) {
            $file = $pkgdir . DIRECTORY_SEPARATOR . $fname;
            if (!file_exists($file)) {
                return $this->raiseError("File does not exist: $fname");
            } else {
                $filelist[$i++] = $file;
                if (empty($pkginfo['filelist'][$fname]['md5sum'])) {
                    $md5sum = md5_file($file);
                    $tpkginfo['filelist'][$fname]['md5sum'] = $md5sum;
                }
                $this->log(2, "Adding file $fname");
            }
        }
        // }}}

        // {{{ regenerate package.xml
        $new_xml = $this->xmlFromInfo($pkginfo);
        if (PEAR::isError($new_xml)) {
            return $this->raiseError($new_xml);
        }
        if (!($tmpdir = System::mktemp(array('-d')))) {
            return $this->raiseError("PEAR_Packager: mktemp failed");
        }
        $newpkgfile = $tmpdir . DIRECTORY_SEPARATOR . 'package.xml';
        $np = @fopen($newpkgfile, 'wb');
        if (!$np) {
            return $this->raiseError("PEAR_Packager: unable to rewrite $pkgfile as $newpkgfile");
        }
        fwrite($np, $new_xml);
        fclose($np);
        // }}}

        // {{{ TAR the Package -------------------------------------------
        $ext = $compress ? '.tgz' : '.tar';
        $dest_package = getcwd() . DIRECTORY_SEPARATOR . $pkgver . $ext;
        $tar =& new Archive_Tar($dest_package, $compress);
        $tar->setErrorHandling(PEAR_ERROR_RETURN); // XXX Don't print errors
        // ----- Creates with the package.xml file
        $ok = $tar->createModify(array($newpkgfile), '', $tmpdir);
        if (PEAR::isError($ok)) {
            return $this->raiseError($ok);
        } elseif (!$ok) {
            return $this->raiseError('PEAR_Packager: tarball creation failed');
        }
        // ----- Add the content of the package
        if (!$tar->addModify($filelist, $pkgver, $pkgdir)) {
            return $this->raiseError('PEAR_Packager: tarball creation failed');
        }
        $this->log(1, "Package $dest_package done");
        if (file_exists("$pkgdir/CVS/Root")) {
            $cvsversion = preg_replace('/[^a-z0-9]/i', '_', $pkginfo['version']);
            $cvstag = "RELEASE_$cvsversion";
            $this->log(1, "Tag the released code with `pear cvstag $pkgfile'");
            $this->log(1, "(or set the CVS tag $cvstag by hand)");
        }
        // }}}

        return $dest_package;
    }

    // }}}
}

// {{{ md5_file() utility function
if (!function_exists('md5_file')) {
    function md5_file($file) {
        if (!$fd = @fopen($file, 'r')) {
            return false;
        }
        $md5 = md5(fread($fd, filesize($file)));
        fclose($fd);
        return $md5;
    }
}
// }}}

?>
