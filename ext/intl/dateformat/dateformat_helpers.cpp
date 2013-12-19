/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Gustavo Lopes <cataphract@php.net>                          |
   +----------------------------------------------------------------------+
*/

#include "../intl_cppshims.h"

#include <unicode/calendar.h>
#include <unicode/gregocal.h>

#include "dateformat_helpers.h"

extern "C" {
#include "../php_intl.h"
#include <Zend/zend_operators.h>
#define USE_CALENDAR_POINTER 1
#include "../calendar/calendar_class.h"
}

int datefmt_process_calendar_arg(zval* calendar_zv,
								 Locale const& locale,
								 const char *func_name,
								 intl_error *err,
								 Calendar*& cal,
								 php_int_t& cal_int_type,
								 bool& calendar_owned TSRMLS_DC)
{
	char *msg;
	UErrorCode status = UErrorCode();

	if (calendar_zv == NULL || Z_TYPE_P(calendar_zv) == IS_NULL) {

		// default requested
		cal = new GregorianCalendar(locale, status);
		calendar_owned = true;

		cal_int_type = UCAL_GREGORIAN;

	} else if (Z_TYPE_P(calendar_zv) == IS_INT) {

		php_int_t v = Z_IVAL_P(calendar_zv);
		if (v != (php_int_t)UCAL_TRADITIONAL && v != (php_int_t)UCAL_GREGORIAN) {
			spprintf(&msg, 0, "%s: invalid value for calendar type; it must be "
					"one of IntlDateFormatter::TRADITIONAL (locale's default "
					"calendar) or IntlDateFormatter::GREGORIAN. "
					"Alternatively, it can be an IntlCalendar object",
					func_name);
			intl_errors_set(err, U_ILLEGAL_ARGUMENT_ERROR, msg, 1 TSRMLS_CC);
			efree(msg);
			return FAILURE;
		} else if (v == (php_int_t)UCAL_TRADITIONAL) {
			cal = Calendar::createInstance(locale, status);
		} else { //UCAL_GREGORIAN
			cal = new GregorianCalendar(locale, status);
		}
		calendar_owned = true;

		cal_int_type = Z_IVAL_P(calendar_zv);

	} else if (Z_TYPE_P(calendar_zv) == IS_OBJECT &&
			instanceof_function_ex(Z_OBJCE_P(calendar_zv),
			Calendar_ce_ptr, 0 TSRMLS_CC)) {

		cal = calendar_fetch_native_calendar(calendar_zv TSRMLS_CC);
		if (cal == NULL) {
			spprintf(&msg, 0, "%s: Found unconstructed IntlCalendar object",
					func_name);
			intl_errors_set(err, U_ILLEGAL_ARGUMENT_ERROR, msg, 1 TSRMLS_CC);
			efree(msg);
			return FAILURE;
		}
		calendar_owned = false;

		cal_int_type = -1;

	} else {
		spprintf(&msg, 0, "%s: Invalid calendar argument; should be an integer "
				"or an IntlCalendar instance", func_name);
		intl_errors_set(err, U_ILLEGAL_ARGUMENT_ERROR, msg, 1 TSRMLS_CC);
		efree(msg);
		return FAILURE;
	}

	if (cal == NULL && !U_FAILURE(status)) {
		status = U_MEMORY_ALLOCATION_ERROR;
	}
	if (U_FAILURE(status)) {
		spprintf(&msg, 0, "%s: Failure instantiating calendar", func_name);
		intl_errors_set(err, U_ILLEGAL_ARGUMENT_ERROR, msg, 1 TSRMLS_CC);
		efree(msg);
		return FAILURE;
	}

	return SUCCESS;
}
