/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: aae9f04a1ef2d0d2698c2e3a617594f0bf3d4cd5 */



static void register_main_symbols(int module_number)
{
	REGISTER_STRING_CONSTANT("PHP_VERSION", PHP_VERSION, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_MAJOR_VERSION", PHP_MAJOR_VERSION, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_MINOR_VERSION", PHP_MINOR_VERSION, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_RELEASE_VERSION", PHP_RELEASE_VERSION, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_EXTRA_VERSION", PHP_EXTRA_VERSION, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_VERSION_ID", PHP_VERSION_ID, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_ZTS", PHP_ZTS, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_DEBUG", PHP_DEBUG, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_OS", PHP_OS_STR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_OS_FAMILY", PHP_OS_FAMILY, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("DEFAULT_INCLUDE_PATH", PHP_INCLUDE_PATH, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PEAR_INSTALL_DIR", PEAR_INSTALLDIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PEAR_EXTENSION_DIR", PHP_EXTENSION_DIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_EXTENSION_DIR", PHP_EXTENSION_DIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_PREFIX", PHP_PREFIX, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_BINDIR", PHP_BINDIR, CONST_PERSISTENT);
#if !defined(PHP_WIN32)
	REGISTER_STRING_CONSTANT("PHP_MANDIR", PHP_MANDIR, CONST_PERSISTENT);
#endif
	REGISTER_STRING_CONSTANT("PHP_LIBDIR", PHP_LIBDIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_DATADIR", PHP_DATADIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_SYSCONFDIR", PHP_SYSCONFDIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_LOCALSTATEDIR", PHP_LOCALSTATEDIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_CONFIG_FILE_PATH", PHP_CONFIG_FILE_PATH, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_CONFIG_FILE_SCAN_DIR", PHP_CONFIG_FILE_SCAN_DIR, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_SHLIB_SUFFIX", PHP_SHLIB_SUFFIX, CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("PHP_EOL", PHP_EOL, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_MAXPATHLEN", MAXPATHLEN, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_INT_MAX", ZEND_LONG_MAX, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_INT_MIN", ZEND_LONG_MIN, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_INT_SIZE", SIZEOF_ZEND_LONG, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_FD_SETSIZE", FD_SETSIZE, CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("PHP_FLOAT_DIG", DBL_DIG, CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("PHP_FLOAT_EPSILON", DBL_EPSILON, CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("PHP_FLOAT_MAX", DBL_MAX, CONST_PERSISTENT);
	REGISTER_DOUBLE_CONSTANT("PHP_FLOAT_MIN", DBL_MIN, CONST_PERSISTENT);
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_MAJOR", EG(windows_version_info).dwMajorVersion, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_MINOR", EG(windows_version_info).dwMinorVersion, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_BUILD", EG(windows_version_info).dwBuildNumber, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_PLATFORM", EG(windows_version_info).dwPlatformId, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MAJOR", EG(windows_version_info).wServicePackMajor, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_SP_MINOR", EG(windows_version_info).wServicePackMinor, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_SUITEMASK", EG(windows_version_info).wSuiteMask, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_VERSION_PRODUCTTYPE", EG(windows_version_info).wProductType, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_NT_DOMAIN_CONTROLLER", VER_NT_DOMAIN_CONTROLLER, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_NT_SERVER", VER_NT_SERVER, CONST_PERSISTENT);
#endif
#if defined(PHP_WIN32)
	REGISTER_LONG_CONSTANT("PHP_WINDOWS_NT_WORKSTATION", VER_NT_WORKSTATION, CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_START", PHP_OUTPUT_HANDLER_START, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_WRITE", PHP_OUTPUT_HANDLER_WRITE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FLUSH", PHP_OUTPUT_HANDLER_FLUSH, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CLEAN", PHP_OUTPUT_HANDLER_CLEAN, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FINAL", PHP_OUTPUT_HANDLER_FINAL, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CONT", PHP_OUTPUT_HANDLER_WRITE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_END", PHP_OUTPUT_HANDLER_FINAL, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CLEANABLE", PHP_OUTPUT_HANDLER_CLEANABLE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_FLUSHABLE", PHP_OUTPUT_HANDLER_FLUSHABLE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_REMOVABLE", PHP_OUTPUT_HANDLER_REMOVABLE, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_STDFLAGS", PHP_OUTPUT_HANDLER_STDFLAGS, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_STARTED", PHP_OUTPUT_HANDLER_STARTED, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PHP_OUTPUT_HANDLER_DISABLED", PHP_OUTPUT_HANDLER_DISABLED, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_OK", PHP_UPLOAD_ERROR_OK, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_INI_SIZE", PHP_UPLOAD_ERROR_A, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_FORM_SIZE", PHP_UPLOAD_ERROR_B, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_PARTIAL", PHP_UPLOAD_ERROR_C, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_NO_FILE", PHP_UPLOAD_ERROR_D, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_NO_TMP_DIR", PHP_UPLOAD_ERROR_E, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_CANT_WRITE", PHP_UPLOAD_ERROR_F, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("UPLOAD_ERR_EXTENSION", PHP_UPLOAD_ERROR_X, CONST_PERSISTENT);
}
