/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Vadim Savchuk <vsavchuk@productengine.com>                  |
   |          Dmitry Lakhtyuk <dlakhtyuk@productengine.com>               |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_intl.h"
#include "intl_error.h"
#include "common_error.h"

/* {{{ proto int intl_get_error_code()
 * Get code of the last occurred error.
 */
PHP_FUNCTION( intl_get_error_code )
{
	RETURN_LONG( intl_error_get_code( NULL ) );
}
/* }}} */

/* {{{ proto string intl_get_error_message()
 * Get text description of the last occurred error.
 */
PHP_FUNCTION( intl_get_error_message )
{
	RETURN_STR(intl_error_get_message( NULL ));
}
/* }}} */

/* {{{ proto bool intl_is_failure()
 * Check whether the given error code indicates a failure.
 * Returns true if it does, and false if the code
 * indicates success or a warning.
 */
PHP_FUNCTION( intl_is_failure )
{
	zend_long err_code;

	/* Parse parameters. */
	if( zend_parse_parameters( ZEND_NUM_ARGS(), "l",
		&err_code ) == FAILURE )
	{
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			"intl_is_failure: unable to parse input params", 0 );

		RETURN_FALSE;
	}

	RETURN_BOOL( U_FAILURE( err_code ) );
}
/* }}} */

/* {{{ proto string intl_error_name()
 * Return a string for a given error code.
 * The string will be the same as the name of the error code constant.
 */
PHP_FUNCTION( intl_error_name )
{
	zend_long err_code;

	/* Parse parameters. */
	if( zend_parse_parameters( ZEND_NUM_ARGS(), "l",
		&err_code ) == FAILURE )
	{
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			"intl_error_name: unable to parse input params", 0 );

		RETURN_FALSE;
	}

	RETURN_STRING( (char*)u_errorName( err_code ) );
}
/* }}} */

/* {{{ intl_expose_icu_error_codes
 * Expose ICU error codes
 */
void intl_expose_icu_error_codes( INIT_FUNC_ARGS )
{
	#define INTL_EXPOSE_CONST(x) REGISTER_LONG_CONSTANT(#x, x, CONST_PERSISTENT | CONST_CS)

	/* Warnings */
	INTL_EXPOSE_CONST( U_USING_FALLBACK_WARNING );
	INTL_EXPOSE_CONST( U_ERROR_WARNING_START );
	INTL_EXPOSE_CONST( U_USING_DEFAULT_WARNING );
	INTL_EXPOSE_CONST( U_SAFECLONE_ALLOCATED_WARNING );
	INTL_EXPOSE_CONST( U_STATE_OLD_WARNING );
	INTL_EXPOSE_CONST( U_STRING_NOT_TERMINATED_WARNING );
	INTL_EXPOSE_CONST( U_SORT_KEY_TOO_SHORT_WARNING );
	INTL_EXPOSE_CONST( U_AMBIGUOUS_ALIAS_WARNING );
	INTL_EXPOSE_CONST( U_DIFFERENT_UCA_VERSION );
	INTL_EXPOSE_CONST( U_ERROR_WARNING_LIMIT );

	/* Standard errors */
	INTL_EXPOSE_CONST( U_ZERO_ERROR );
	INTL_EXPOSE_CONST( U_ILLEGAL_ARGUMENT_ERROR );
	INTL_EXPOSE_CONST( U_MISSING_RESOURCE_ERROR );
	INTL_EXPOSE_CONST( U_INVALID_FORMAT_ERROR );
	INTL_EXPOSE_CONST( U_FILE_ACCESS_ERROR );
	INTL_EXPOSE_CONST( U_INTERNAL_PROGRAM_ERROR );
	INTL_EXPOSE_CONST( U_MESSAGE_PARSE_ERROR );
	INTL_EXPOSE_CONST( U_MEMORY_ALLOCATION_ERROR );
	INTL_EXPOSE_CONST( U_INDEX_OUTOFBOUNDS_ERROR );
	INTL_EXPOSE_CONST( U_PARSE_ERROR );
	INTL_EXPOSE_CONST( U_INVALID_CHAR_FOUND );
	INTL_EXPOSE_CONST( U_TRUNCATED_CHAR_FOUND );
	INTL_EXPOSE_CONST( U_ILLEGAL_CHAR_FOUND );
	INTL_EXPOSE_CONST( U_INVALID_TABLE_FORMAT );
	INTL_EXPOSE_CONST( U_INVALID_TABLE_FILE );
	INTL_EXPOSE_CONST( U_BUFFER_OVERFLOW_ERROR );
	INTL_EXPOSE_CONST( U_UNSUPPORTED_ERROR );
	INTL_EXPOSE_CONST( U_RESOURCE_TYPE_MISMATCH );
	INTL_EXPOSE_CONST( U_ILLEGAL_ESCAPE_SEQUENCE );
	INTL_EXPOSE_CONST( U_UNSUPPORTED_ESCAPE_SEQUENCE );
	INTL_EXPOSE_CONST( U_NO_SPACE_AVAILABLE );
	INTL_EXPOSE_CONST( U_CE_NOT_FOUND_ERROR );
	INTL_EXPOSE_CONST( U_PRIMARY_TOO_LONG_ERROR );
	INTL_EXPOSE_CONST( U_STATE_TOO_OLD_ERROR );
	INTL_EXPOSE_CONST( U_TOO_MANY_ALIASES_ERROR );
	INTL_EXPOSE_CONST( U_ENUM_OUT_OF_SYNC_ERROR );
	INTL_EXPOSE_CONST( U_INVARIANT_CONVERSION_ERROR );
	INTL_EXPOSE_CONST( U_INVALID_STATE_ERROR );
	INTL_EXPOSE_CONST( U_COLLATOR_VERSION_MISMATCH );
	INTL_EXPOSE_CONST( U_USELESS_COLLATOR_ERROR );
	INTL_EXPOSE_CONST( U_NO_WRITE_PERMISSION );
	INTL_EXPOSE_CONST( U_STANDARD_ERROR_LIMIT );

	/* The error code range 0x10000 0x10100 are reserved for Transliterator */
	INTL_EXPOSE_CONST( U_BAD_VARIABLE_DEFINITION );
	INTL_EXPOSE_CONST( U_PARSE_ERROR_START );
	INTL_EXPOSE_CONST( U_MALFORMED_RULE );
	INTL_EXPOSE_CONST( U_MALFORMED_SET );
	INTL_EXPOSE_CONST( U_MALFORMED_SYMBOL_REFERENCE );
	INTL_EXPOSE_CONST( U_MALFORMED_UNICODE_ESCAPE );
	INTL_EXPOSE_CONST( U_MALFORMED_VARIABLE_DEFINITION );
	INTL_EXPOSE_CONST( U_MALFORMED_VARIABLE_REFERENCE );
	INTL_EXPOSE_CONST( U_MISMATCHED_SEGMENT_DELIMITERS );
	INTL_EXPOSE_CONST( U_MISPLACED_ANCHOR_START );
	INTL_EXPOSE_CONST( U_MISPLACED_CURSOR_OFFSET );
	INTL_EXPOSE_CONST( U_MISPLACED_QUANTIFIER );
	INTL_EXPOSE_CONST( U_MISSING_OPERATOR );
	INTL_EXPOSE_CONST( U_MISSING_SEGMENT_CLOSE );
	INTL_EXPOSE_CONST( U_MULTIPLE_ANTE_CONTEXTS );
	INTL_EXPOSE_CONST( U_MULTIPLE_CURSORS );
	INTL_EXPOSE_CONST( U_MULTIPLE_POST_CONTEXTS );
	INTL_EXPOSE_CONST( U_TRAILING_BACKSLASH );
	INTL_EXPOSE_CONST( U_UNDEFINED_SEGMENT_REFERENCE );
	INTL_EXPOSE_CONST( U_UNDEFINED_VARIABLE );
	INTL_EXPOSE_CONST( U_UNQUOTED_SPECIAL );
	INTL_EXPOSE_CONST( U_UNTERMINATED_QUOTE );
	INTL_EXPOSE_CONST( U_RULE_MASK_ERROR );
	INTL_EXPOSE_CONST( U_MISPLACED_COMPOUND_FILTER );
	INTL_EXPOSE_CONST( U_MULTIPLE_COMPOUND_FILTERS );
	INTL_EXPOSE_CONST( U_INVALID_RBT_SYNTAX );
	INTL_EXPOSE_CONST( U_INVALID_PROPERTY_PATTERN );
	INTL_EXPOSE_CONST( U_MALFORMED_PRAGMA );
	INTL_EXPOSE_CONST( U_UNCLOSED_SEGMENT );
	INTL_EXPOSE_CONST( U_ILLEGAL_CHAR_IN_SEGMENT );
	INTL_EXPOSE_CONST( U_VARIABLE_RANGE_EXHAUSTED );
	INTL_EXPOSE_CONST( U_VARIABLE_RANGE_OVERLAP );
	INTL_EXPOSE_CONST( U_ILLEGAL_CHARACTER );
	INTL_EXPOSE_CONST( U_INTERNAL_TRANSLITERATOR_ERROR );
	INTL_EXPOSE_CONST( U_INVALID_ID );
	INTL_EXPOSE_CONST( U_INVALID_FUNCTION );
	INTL_EXPOSE_CONST( U_PARSE_ERROR_LIMIT );

	/* The error code range 0x10100 0x10200 are reserved for formatting API parsing error */
	INTL_EXPOSE_CONST( U_UNEXPECTED_TOKEN );
	INTL_EXPOSE_CONST( U_FMT_PARSE_ERROR_START );
	INTL_EXPOSE_CONST( U_MULTIPLE_DECIMAL_SEPARATORS );
	INTL_EXPOSE_CONST( U_MULTIPLE_DECIMAL_SEPERATORS );    /* Typo: kept for backward compatibility. Use U_MULTIPLE_DECIMAL_SEPARATORS */
	INTL_EXPOSE_CONST( U_MULTIPLE_EXPONENTIAL_SYMBOLS );
	INTL_EXPOSE_CONST( U_MALFORMED_EXPONENTIAL_PATTERN );
	INTL_EXPOSE_CONST( U_MULTIPLE_PERCENT_SYMBOLS );
	INTL_EXPOSE_CONST( U_MULTIPLE_PERMILL_SYMBOLS );
	INTL_EXPOSE_CONST( U_MULTIPLE_PAD_SPECIFIERS );
	INTL_EXPOSE_CONST( U_PATTERN_SYNTAX_ERROR );
	INTL_EXPOSE_CONST( U_ILLEGAL_PAD_POSITION );
	INTL_EXPOSE_CONST( U_UNMATCHED_BRACES );
	INTL_EXPOSE_CONST( U_UNSUPPORTED_PROPERTY );
	INTL_EXPOSE_CONST( U_UNSUPPORTED_ATTRIBUTE );
	INTL_EXPOSE_CONST( U_FMT_PARSE_ERROR_LIMIT );

	/* The error code range 0x10200 0x102ff are reserved for Break Iterator related error */
	INTL_EXPOSE_CONST( U_BRK_INTERNAL_ERROR );
	INTL_EXPOSE_CONST( U_BRK_ERROR_START );
	INTL_EXPOSE_CONST( U_BRK_HEX_DIGITS_EXPECTED );
	INTL_EXPOSE_CONST( U_BRK_SEMICOLON_EXPECTED );
	INTL_EXPOSE_CONST( U_BRK_RULE_SYNTAX );
	INTL_EXPOSE_CONST( U_BRK_UNCLOSED_SET );
	INTL_EXPOSE_CONST( U_BRK_ASSIGN_ERROR );
	INTL_EXPOSE_CONST( U_BRK_VARIABLE_REDFINITION );
	INTL_EXPOSE_CONST( U_BRK_MISMATCHED_PAREN );
	INTL_EXPOSE_CONST( U_BRK_NEW_LINE_IN_QUOTED_STRING );
	INTL_EXPOSE_CONST( U_BRK_UNDEFINED_VARIABLE );
	INTL_EXPOSE_CONST( U_BRK_INIT_ERROR );
	INTL_EXPOSE_CONST( U_BRK_RULE_EMPTY_SET );
	INTL_EXPOSE_CONST( U_BRK_UNRECOGNIZED_OPTION );
	INTL_EXPOSE_CONST( U_BRK_MALFORMED_RULE_TAG );
	INTL_EXPOSE_CONST( U_BRK_ERROR_LIMIT );

	/* The error codes in the range 0x10300-0x103ff are reserved for regular expression related errrs */
	INTL_EXPOSE_CONST( U_REGEX_INTERNAL_ERROR );
	INTL_EXPOSE_CONST( U_REGEX_ERROR_START );
	INTL_EXPOSE_CONST( U_REGEX_RULE_SYNTAX );
	INTL_EXPOSE_CONST( U_REGEX_INVALID_STATE );
	INTL_EXPOSE_CONST( U_REGEX_BAD_ESCAPE_SEQUENCE );
	INTL_EXPOSE_CONST( U_REGEX_PROPERTY_SYNTAX );
	INTL_EXPOSE_CONST( U_REGEX_UNIMPLEMENTED );
	INTL_EXPOSE_CONST( U_REGEX_MISMATCHED_PAREN );
	INTL_EXPOSE_CONST( U_REGEX_NUMBER_TOO_BIG );
	INTL_EXPOSE_CONST( U_REGEX_BAD_INTERVAL );
	INTL_EXPOSE_CONST( U_REGEX_MAX_LT_MIN );
	INTL_EXPOSE_CONST( U_REGEX_INVALID_BACK_REF );
	INTL_EXPOSE_CONST( U_REGEX_INVALID_FLAG );
	INTL_EXPOSE_CONST( U_REGEX_LOOK_BEHIND_LIMIT );
	INTL_EXPOSE_CONST( U_REGEX_SET_CONTAINS_STRING );
	INTL_EXPOSE_CONST( U_REGEX_ERROR_LIMIT );

	/* The error code in the range 0x10400-0x104ff are reserved for IDNA related error codes */
	INTL_EXPOSE_CONST( U_IDNA_PROHIBITED_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_ERROR_START );
	INTL_EXPOSE_CONST( U_IDNA_UNASSIGNED_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_CHECK_BIDI_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_STD3_ASCII_RULES_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_ACE_PREFIX_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_VERIFICATION_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_LABEL_TOO_LONG_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_ZERO_LENGTH_LABEL_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR );
	INTL_EXPOSE_CONST( U_IDNA_ERROR_LIMIT );

	/* Aliases for StringPrep */
	INTL_EXPOSE_CONST( U_STRINGPREP_PROHIBITED_ERROR );
	INTL_EXPOSE_CONST( U_STRINGPREP_UNASSIGNED_ERROR );
	INTL_EXPOSE_CONST( U_STRINGPREP_CHECK_BIDI_ERROR );

	INTL_EXPOSE_CONST( U_ERROR_LIMIT );

	#undef INTL_EXPOSE_CONST
}
/* }}} */
