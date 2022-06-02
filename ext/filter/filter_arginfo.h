/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b37b1cee991d10a8198a3088a038b7ceea13d294 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_filter_has_var, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, input_type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_filter_input, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, var_name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, filter, IS_LONG, 0, "FILTER_DEFAULT")
	ZEND_ARG_TYPE_MASK(0, options, MAY_BE_ARRAY|MAY_BE_LONG, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_filter_var, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, filter, IS_LONG, 0, "FILTER_DEFAULT")
	ZEND_ARG_TYPE_MASK(0, options, MAY_BE_ARRAY|MAY_BE_LONG, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_filter_input_array, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO(0, type, IS_LONG, 0)
	ZEND_ARG_TYPE_MASK(0, options, MAY_BE_ARRAY|MAY_BE_LONG, "FILTER_DEFAULT")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, add_empty, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_filter_var_array, 0, 1, MAY_BE_ARRAY|MAY_BE_FALSE|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO(0, array, IS_ARRAY, 0)
	ZEND_ARG_TYPE_MASK(0, options, MAY_BE_ARRAY|MAY_BE_LONG, "FILTER_DEFAULT")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, add_empty, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_filter_list, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_filter_id, 0, 1, MAY_BE_LONG|MAY_BE_FALSE)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()


ZEND_FUNCTION(filter_has_var);
ZEND_FUNCTION(filter_input);
ZEND_FUNCTION(filter_var);
ZEND_FUNCTION(filter_input_array);
ZEND_FUNCTION(filter_var_array);
ZEND_FUNCTION(filter_list);
ZEND_FUNCTION(filter_id);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(filter_has_var, arginfo_filter_has_var)
	ZEND_FE(filter_input, arginfo_filter_input)
	ZEND_FE(filter_var, arginfo_filter_var)
	ZEND_FE(filter_input_array, arginfo_filter_input_array)
	ZEND_FE(filter_var_array, arginfo_filter_var_array)
	ZEND_FE(filter_list, arginfo_filter_list)
	ZEND_FE(filter_id, arginfo_filter_id)
	ZEND_FE_END
};

static void register_filter_symbols(int module_number)
{
    REGISTER_LONG_CONSTANT("INPUT_POST", PARSE_POST, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INPUT_GET", PARSE_GET, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INPUT_COOKIE", PARSE_COOKIE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INPUT_ENV", PARSE_ENV, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("INPUT_SERVER", PARSE_SERVER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_NONE", FILTER_FLAG_NONE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_REQUIRE_SCALAR", FILTER_REQUIRE_SCALAR, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_REQUIRE_ARRAY", FILTER_REQUIRE_ARRAY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FORCE_ARRAY", FILTER_FORCE_ARRAY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_NULL_ON_FAILURE", FILTER_NULL_ON_FAILURE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_INT", FILTER_VALIDATE_INT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_BOOLEAN", FILTER_VALIDATE_BOOL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_BOOL", FILTER_VALIDATE_BOOL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_FLOAT", FILTER_VALIDATE_FLOAT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_REGEXP", FILTER_VALIDATE_REGEXP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_DOMAIN", FILTER_VALIDATE_DOMAIN, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_URL", FILTER_VALIDATE_URL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_EMAIL", FILTER_VALIDATE_EMAIL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_IP", FILTER_VALIDATE_IP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_VALIDATE_MAC", FILTER_VALIDATE_MAC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_DEFAULT", FILTER_DEFAULT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_UNSAFE_RAW", FILTER_UNSAFE_RAW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_STRING", FILTER_SANITIZE_STRING, CONST_CS | CONST_PERSISTENT | CONST_DEPRECATED);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_STRIPPED", FILTER_SANITIZE_STRING, CONST_CS | CONST_PERSISTENT | CONST_DEPRECATED);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_ENCODED", FILTER_SANITIZE_ENCODED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_SPECIAL_CHARS", FILTER_SANITIZE_SPECIAL_CHARS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_FULL_SPECIAL_CHARS", FILTER_SANITIZE_FULL_SPECIAL_CHARS, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_EMAIL", FILTER_SANITIZE_EMAIL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_URL", FILTER_SANITIZE_URL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_NUMBER_INT", FILTER_SANITIZE_NUMBER_INT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_NUMBER_FLOAT", FILTER_SANITIZE_NUMBER_FLOAT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_SANITIZE_ADD_SLASHES", FILTER_SANITIZE_ADD_SLASHES, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_CALLBACK", FILTER_CALLBACK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_OCTAL", FILTER_FLAG_ALLOW_OCTAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_HEX", FILTER_FLAG_ALLOW_HEX, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_STRIP_LOW", FILTER_FLAG_STRIP_LOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_STRIP_HIGH", FILTER_FLAG_STRIP_HIGH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_STRIP_BACKTICK", FILTER_FLAG_STRIP_BACKTICK, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_LOW", FILTER_FLAG_ENCODE_LOW, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_HIGH", FILTER_FLAG_ENCODE_HIGH, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_AMP", FILTER_FLAG_ENCODE_AMP, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_ENCODE_QUOTES", FILTER_FLAG_NO_ENCODE_QUOTES, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_EMPTY_STRING_NULL", FILTER_FLAG_EMPTY_STRING_NULL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_FRACTION", FILTER_FLAG_ALLOW_FRACTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_THOUSAND", FILTER_FLAG_ALLOW_THOUSAND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_SCIENTIFIC", FILTER_FLAG_ALLOW_SCIENTIFIC, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_PATH_REQUIRED", FILTER_FLAG_PATH_REQUIRED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_QUERY_REQUIRED", FILTER_FLAG_QUERY_REQUIRED, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_IPV4", FILTER_FLAG_IPV4, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_IPV6", FILTER_FLAG_IPV6, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_RES_RANGE", FILTER_FLAG_NO_RES_RANGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_PRIV_RANGE", FILTER_FLAG_NO_PRIV_RANGE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_HOSTNAME", FILTER_FLAG_HOSTNAME, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("FILTER_FLAG_EMAIL_UNICODE", FILTER_FLAG_EMAIL_UNICODE, CONST_CS | CONST_PERSISTENT);
}
