/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@zend.com>                             |
  +----------------------------------------------------------------------+
*/
/* $Id$ */

#include <time.h>

#include "php_soap.h"

/* zval type decode */
static zval *to_zval_double(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_long(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_ulong(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_bool(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_string(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_stringr(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_stringc(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_map(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_null(encodeTypePtr type, xmlNodePtr data);

static xmlNodePtr to_xml_long(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_ulong(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_double(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_bool(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

/* String encode */
static xmlNodePtr to_xml_string(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_stringl(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

/* Null encode */
static xmlNodePtr to_xml_null(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

/* Array encode */
static xmlNodePtr guess_array_map(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_map(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

static xmlNodePtr to_xml_list(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_list1(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent);

/* Datetime encode/decode */
static xmlNodePtr to_xml_datetime_ex(encodeTypePtr type, zval *data, char *format, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_datetime(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_time(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_date(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_gyearmonth(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_gyear(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_gmonthday(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_gday(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_gmonth(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_duration(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

static zval *to_zval_object(encodeTypePtr type, xmlNodePtr data);
static zval *to_zval_array(encodeTypePtr type, xmlNodePtr data);

static xmlNodePtr to_xml_object(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);
static xmlNodePtr to_xml_array(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

/* Try and guess for non-wsdl clients and servers */
static zval *guess_zval_convert(encodeTypePtr type, xmlNodePtr data);
static xmlNodePtr guess_xml_convert(encodeTypePtr type, zval *data, int style, xmlNodePtr parent);

static int is_map(zval *array);
static void get_array_type(xmlNodePtr node, zval *array, smart_str *out_type TSRMLS_DC);

static xmlNodePtr check_and_resolve_href(xmlNodePtr data);

static encodePtr get_conversion(int encode);

static void get_type_str(xmlNodePtr node, const char* ns, const char* type, smart_str* ret);
static void set_ns_and_type_ex(xmlNodePtr node, char *ns, char *type);

static void set_ns_and_type(xmlNodePtr node, encodeTypePtr type);

#define FIND_XML_NULL(xml,zval) \
	{ \
		xmlAttrPtr null; \
		if (!xml) { \
			ZVAL_NULL(zval); \
			return zval; \
		} \
		if (xml->properties) { \
			null = get_attribute(xml->properties, "nil"); \
			if (null) { \
				ZVAL_NULL(zval); \
				return zval; \
			} \
		} \
	}

#define FIND_ZVAL_NULL(zval, xml, style) \
{ \
	if (!zval || Z_TYPE_P(zval) == IS_NULL) { \
	  if (style == SOAP_ENCODED) {\
			xmlSetProp(xml, "xsi:nil", "1"); \
		} \
		return xml; \
	} \
}

encode defaultEncoding[] = {
	{{UNKNOWN_TYPE, NULL, NULL, NULL}, guess_zval_convert, guess_xml_convert},

	{{IS_NULL, "nil", XSI_NAMESPACE, NULL}, to_zval_null, to_xml_null},
	{{IS_STRING, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{IS_LONG, XSD_INT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{IS_DOUBLE, XSD_FLOAT_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{IS_BOOL, XSD_BOOLEAN_STRING, XSD_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{IS_CONSTANT, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{IS_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_1_ENC_NAMESPACE, NULL}, to_zval_array, guess_array_map},
	{{IS_CONSTANT_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_1_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},
	{{IS_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_1_1_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},
	{{IS_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_2_ENC_NAMESPACE, NULL}, to_zval_array, guess_array_map},
	{{IS_CONSTANT_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_2_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},
	{{IS_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_1_2_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},

	{{XSD_STRING, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_BOOLEAN, XSD_BOOLEAN_STRING, XSD_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{XSD_DECIMAL, XSD_DECIMAL_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_FLOAT, XSD_FLOAT_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{XSD_DOUBLE, XSD_DOUBLE_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},

	{{XSD_DATETIME, XSD_DATETIME_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_datetime},
	{{XSD_TIME, XSD_TIME_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_time},
	{{XSD_DATE, XSD_DATE_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_date},
	{{XSD_GYEARMONTH, XSD_GYEARMONTH_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_gyearmonth},
	{{XSD_GYEAR, XSD_GYEAR_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_gyear},
	{{XSD_GMONTHDAY, XSD_GMONTHDAY_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_gmonthday},
	{{XSD_GDAY, XSD_GDAY_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_gday},
	{{XSD_GMONTH, XSD_GMONTH_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_gmonth},
	{{XSD_DURATION, XSD_DURATION_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_duration},

	{{XSD_HEXBINARY, XSD_HEXBINARY_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_stringl},
	{{XSD_BASE64BINARY, XSD_BASE64BINARY_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_stringl},

	{{XSD_LONG, XSD_LONG_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_INT, XSD_INT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_SHORT, XSD_SHORT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_BYTE, XSD_BYTE_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_NONPOSITIVEINTEGER, XSD_NONPOSITIVEINTEGER_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_POSITIVEINTEGER, XSD_POSITIVEINTEGER_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_NONNEGATIVEINTEGER, XSD_NONNEGATIVEINTEGER_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_NEGATIVEINTEGER, XSD_NEGATIVEINTEGER_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_UNSIGNEDBYTE, XSD_UNSIGNEDBYTE_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_UNSIGNEDSHORT, XSD_UNSIGNEDSHORT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_UNSIGNEDINT, XSD_UNSIGNEDINT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_UNSIGNEDLONG, XSD_UNSIGNEDLONG_STRING, XSD_NAMESPACE, NULL}, to_zval_ulong, to_xml_ulong},

	{{XSD_ANYTYPE, XSD_ANYTYPE_STRING, XSD_NAMESPACE, NULL}, guess_zval_convert, guess_xml_convert},
	{{XSD_UR_TYPE, XSD_UR_TYPE_STRING, XSD_NAMESPACE, NULL}, guess_zval_convert, guess_xml_convert},
	{{XSD_ANYURI, XSD_ANYURI_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_QNAME, XSD_QNAME_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_NOTATION, XSD_NOTATION_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_NORMALIZEDSTRING, XSD_NORMALIZEDSTRING_STRING, XSD_NAMESPACE, NULL}, to_zval_stringr, to_xml_string},
	{{XSD_TOKEN, XSD_TOKEN_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_LANGUAGE, XSD_LANGUAGE_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_NMTOKEN, XSD_NMTOKEN_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_NMTOKENS, XSD_NMTOKENS_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_list1},
	{{XSD_NAME, XSD_NAME_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_NCNAME, XSD_NCNAME_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_ID, XSD_ID_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_IDREF, XSD_IDREF_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_IDREFS, XSD_IDREFS_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_list1},
	{{XSD_ENTITY, XSD_ENTITY_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_ENTITIES, XSD_ENTITIES_STRING, XSD_NAMESPACE, NULL}, to_zval_stringc, to_xml_list1},

	{{APACHE_MAP, APACHE_MAP_STRING, APACHE_NAMESPACE, NULL}, to_zval_map, to_xml_map},

	{{SOAP_ENC_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_1_1_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},
	{{SOAP_ENC_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_1_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},
	{{SOAP_ENC_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_1_2_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},
	{{SOAP_ENC_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_1_2_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},

	/* support some of the 1999 data types */
	{{XSD_STRING, XSD_STRING_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_BOOLEAN, XSD_BOOLEAN_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{XSD_DECIMAL, XSD_DECIMAL_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},
	{{XSD_FLOAT, XSD_FLOAT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{XSD_DOUBLE, XSD_DOUBLE_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_double, to_xml_double},

	{{XSD_LONG, XSD_LONG_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_INT, XSD_INT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_SHORT, XSD_SHORT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_BYTE, XSD_BYTE_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_1999_TIMEINSTANT, XSD_1999_TIMEINSTANT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_stringc, to_xml_string},

	{{END_KNOWN_TYPES, NULL, NULL, NULL}, guess_zval_convert, guess_xml_convert}
};

void whiteSpace_replace(char* str)
{
	while (*str != '\0') {
		if (*str == '\x9' || *str == '\xA' || *str == '\xD') {
			*str = ' ';
		}
		str++;
	}
}

void whiteSpace_collapse(char* str)
{
	char *pos;
	char old;

	pos = str;
	whiteSpace_replace(str);
	while (*str == ' ') {
		str++;
	}
	old = '\0';
	while (*str != '\0') {
		if (*str != ' ' || old != ' ') {
			*pos = *str;
			pos++;
		}
		old = *str;
		str++;
	}
	if (old == ' ') {
	 	--pos;
	}
	*pos = '\0';
}

xmlNodePtr master_to_xml(encodePtr encode, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr node = NULL;
	TSRMLS_FETCH();

	/* Special handling of class SoapVar */
	if (data &&
	    Z_TYPE_P(data) == IS_OBJECT &&
	    Z_OBJCE_P(data) == soap_var_class_entry) {
		zval **ztype, **zdata, **zns, **zstype, **zname, **znamens;
		encodePtr enc;
		HashTable *ht = Z_OBJPROP_P(data);

		if (zend_hash_find(ht, "enc_type", sizeof("enc_type"), (void **)&ztype) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: SoapVar hasn't 'enc_type' propery");
		}

		enc = get_conversion(Z_LVAL_P(*ztype));

		if (zend_hash_find(ht, "enc_value", sizeof("enc_value"), (void **)&zdata) == FAILURE) {
			node = master_to_xml(enc, NULL, style, parent);
		} else {
			node = master_to_xml(enc, *zdata, style, parent);
		}

		if (style == SOAP_ENCODED) {
			if (zend_hash_find(ht, "enc_stype", sizeof("enc_stype"), (void **)&zstype) == SUCCESS) {
				if (zend_hash_find(ht, "enc_ns", sizeof("enc_ns"), (void **)&zns) == SUCCESS) {
					set_ns_and_type_ex(node, Z_STRVAL_PP(zns), Z_STRVAL_PP(zstype));
				} else {
					set_ns_and_type_ex(node, NULL, Z_STRVAL_PP(zstype));
				}
			}
		}

		if (zend_hash_find(ht, "enc_name", sizeof("enc_name"), (void **)&zname) == SUCCESS) {
			xmlNodeSetName(node, Z_STRVAL_PP(zname));
		}
		if (zend_hash_find(ht, "enc_namens", sizeof("enc_namens"), (void **)&znamens) == SUCCESS) {
			xmlNsPtr nsp = encode_add_ns(node, Z_STRVAL_PP(znamens));
			xmlSetNs(node, nsp);
		}
	} else {
		if (encode == NULL) {
			encode = get_conversion(UNKNOWN_TYPE);
		}
		if (encode->to_xml_before) {
			data = encode->to_xml_before(&encode->details, data);
		}
		if (encode->to_xml) {
			node = encode->to_xml(&encode->details, data, style, parent);
		}
		if (encode->to_xml_after) {
			node = encode->to_xml_after(&encode->details, node, style);
		}
	}
	return node;
}

zval *master_to_zval(encodePtr encode, xmlNodePtr data)
{
	zval *ret = NULL;
	TSRMLS_FETCH();

	if (encode == NULL) {
		encode = get_conversion(UNKNOWN_TYPE);
	}
	data = check_and_resolve_href(data);
	if (encode->to_zval_before) {
		data = encode->to_zval_before(&encode->details, data, 0);
	}
	if (encode->to_zval) {
		ret = encode->to_zval(&encode->details, data);
	}
	if (encode->to_zval_after) {
		ret = encode->to_zval_after(&encode->details, ret);
	}
	return ret;
}

#ifdef HAVE_PHP_DOMXML
zval *to_xml_before_user(encodeTypePtr type, zval *data)
{
	TSRMLS_FETCH();

	if (type.map->map_functions.to_xml_before) {
		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml_before, data, 1, &data  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_xml_before");
		}
	}
	return data;
}

xmlNodePtr to_xml_user(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	zval *ret, **addr;
	xmlNodePtr node;
	TSRMLS_FETCH();

	if (type.map->map_functions.to_xml) {
		MAKE_STD_ZVAL(ret);
		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml, ret, 1, &data  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_xml");
		}

		if (Z_TYPE_P(ret) != IS_OBJECT) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error serializing object from to_xml_user");
		}

		if (zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS) {
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			node = xmlCopyNode(node, 1);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
	}
	return node;
}

xmlNodePtr to_xml_after_user(encodeTypePtr type, xmlNodePtr node, int style)
{
	zval *ret, *param, **addr;
	int found;
	TSRMLS_FETCH();

	if (type.map->map_functions.to_xml_after) {
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml_after, ret, 1, &param  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_xml_after");
		}
		if (zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS) {
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&param);
	}
	return node;
}

xmlNodePtr to_zval_before_user(encodeTypePtr type, xmlNodePtr node, int style)
{
	zval *ret, *param, **addr;
	int found;
	TSRMLS_FETCH();

	if (type.map->map_functions.to_zval_before) {
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval_before, ret, 1, &param  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_zval_before");
		}
		if (zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS) {
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&param);
	}
	return node;
}

zval *to_zval_user(encodeTypePtr type, xmlNodePtr node)
{
	zval *ret, *param;
	int found;
	TSRMLS_FETCH();

	if (type.map->map_functions.to_zval) {
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval, ret, 1, &param  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_zval");
		}
		zval_ptr_dtor(&param);
		efree(param);
	}
	return ret;
}

zval *to_zval_after_user(encodeTypePtr type, zval *data)
{
	TSRMLS_FETCH();

	if (type.map->map_functions.to_zval_after) {
		if (call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval_after, data, 1, &data  TSRMLS_CC) == FAILURE) {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Error calling to_zval_after");
		}
	}
	return data;
}
#endif

/* TODO: get rid of "bogus".. ither by passing in the already created xmlnode or passing in the node name */
/* String encode/decode */
static zval *to_zval_string(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			ZVAL_STRING(ret, data->children->content, 1);
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_EMPTY_STRING(ret);
	}
	return ret;
}

static zval *to_zval_stringr(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_replace(data->children->content);
			ZVAL_STRING(ret, data->children->content, 1);
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_EMPTY_STRING(ret);
	}
	return ret;
}

static zval *to_zval_stringc(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_collapse(data->children->content);
			ZVAL_STRING(ret, data->children->content, 1);
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_EMPTY_STRING(ret);
	}
	return ret;
}

static xmlNodePtr to_xml_string(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;
	char *str;
	int new_len;
	TSRMLS_FETCH();

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	if (Z_TYPE_P(data) == IS_STRING) {
		str = php_escape_html_entities(Z_STRVAL_P(data), Z_STRLEN_P(data), &new_len, 0, 0, NULL TSRMLS_CC);
	} else {
		zval tmp = *data;

		zval_copy_ctor(&tmp);
		convert_to_string(&tmp);
		str = php_escape_html_entities(Z_STRVAL(tmp), Z_STRLEN(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
		zval_dtor(&tmp);
	}

	xmlNodeSetContentLen(ret, str, new_len);
	efree(str);

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

static xmlNodePtr to_xml_stringl(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	if (Z_TYPE_P(data) == IS_STRING) {
		xmlNodeSetContentLen(ret, Z_STRVAL_P(data), Z_STRLEN_P(data));
	} else {
		zval tmp = *data;

		zval_copy_ctor(&tmp);
		convert_to_string(&tmp);
		xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
		zval_dtor(&tmp);
	}

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

static zval *to_zval_double(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_collapse(data->children->content);
			ZVAL_DOUBLE(ret, atof(data->children->content));
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

static zval *to_zval_long(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_collapse(data->children->content);
			ZVAL_LONG(ret, atol(data->children->content));
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

static zval *to_zval_ulong(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_collapse(data->children->content);
			errno = 0;
			ret->value.lval = strtol(data->children->content, NULL, 0);
			if (errno == ERANGE) { /* overflow */
				ret->value.dval = strtod(data->children->content, NULL);
				ret->type = IS_DOUBLE;
			} else {
				ret->type = IS_LONG;
			}
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

static xmlNodePtr to_xml_long(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;
	zval tmp;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	tmp = *data;
	zval_copy_ctor(&tmp);
	if (Z_TYPE(tmp) != IS_LONG) {
		convert_to_long(&tmp);
	}
	convert_to_string(&tmp);
	xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
	zval_dtor(&tmp);

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

static xmlNodePtr to_xml_ulong(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	if (Z_TYPE_P(data) == IS_DOUBLE) {
		char s[16];
		sprintf(s, "%0.0F",Z_DVAL_P(data));
		xmlNodeSetContent(ret, s);
	} else {
		zval tmp = *data;

		zval_copy_ctor(&tmp);
		if (Z_TYPE(tmp) != IS_LONG) {
			convert_to_long(&tmp);
		}
		convert_to_string(&tmp);
		xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
		zval_dtor(&tmp);
	}

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

static xmlNodePtr to_xml_double(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;
	zval tmp;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	tmp = *data;
	zval_copy_ctor(&tmp);
	if (Z_TYPE(tmp) != IS_DOUBLE) {
		convert_to_double(&tmp);
	}
	convert_to_string(&tmp);
	xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
	zval_dtor(&tmp);

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

static zval *to_zval_bool(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		if (data->children->type == XML_TEXT_NODE && data->children->next == NULL) {
			whiteSpace_collapse(data->children->content);
			if (stricmp(data->children->content,"true") == 0 ||
				stricmp(data->children->content,"t") == 0 ||
				strcmp(data->children->content,"1") == 0) {
				ZVAL_BOOL(ret, 1);
			} else {
				ZVAL_BOOL(ret, 0);
			}
		} else {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
		}
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

static xmlNodePtr to_xml_bool(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;
	zval tmp;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);

	if (Z_TYPE_P(data) != IS_BOOL) {
		tmp = *data;
		zval_copy_ctor(&tmp);
		convert_to_boolean(data);
		data = &tmp;
	}

	if (data->value.lval == 1) {
		xmlNodeSetContent(ret, "1");
	} else {
		xmlNodeSetContent(ret, "0");
	}

	if (data == &tmp) {
		zval_dtor(&tmp);
	}

	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, type);
	}
	return ret;
}

/* Null encode/decode */
static zval *to_zval_null(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	ZVAL_NULL(ret);
	return ret;
}

static xmlNodePtr to_xml_null(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr ret;

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	if (style == SOAP_ENCODED) {
		xmlSetProp(ret, "xsi:nil", "1");
	}
	return ret;
}

static void model_to_zval_object(zval *ret, sdlContentModelPtr model, xmlNodePtr data, sdlPtr sdl TSRMLS_DC)
{
	switch (model->kind) {
		case XSD_CONTENT_ELEMENT:
		  if (model->u.element->name) {
		  	xmlNodePtr node = get_node(data->children, model->u.element->name);
		  	if (node) {
					xmlAttrPtr typeAttr = get_attribute(node->properties,"type");
			  	encodePtr  enc = NULL;
			  	zval *val;

					if (typeAttr != NULL && typeAttr->children && typeAttr->children->content) {
						enc = get_encoder_from_prefix(sdl, node, typeAttr->children->content);
					}
					if (enc == NULL) {
						enc = model->u.element->encode;
					}
					val = master_to_zval(enc, node);
					if ((node = get_node(node->next, model->u.element->name)) != NULL) {
						zval *array;

						MAKE_STD_ZVAL(array);
						array_init(array);
						add_next_index_zval(array, val);
						do {
							typeAttr = get_attribute(node->properties,"type");
							enc = NULL;
							if (typeAttr != NULL && typeAttr->children && typeAttr->children->content) {
								enc = get_encoder_from_prefix(sdl, node, typeAttr->children->content);
							}
							if (enc == NULL) {
								enc = model->u.element->encode;
							}
							val = master_to_zval(enc, node);
							add_next_index_zval(array, val);
						} while ((node = get_node(node->next, model->u.element->name)) != NULL);
						val = array;
					}
#ifdef ZEND_ENGINE_2
					val->refcount--;
#endif
					add_property_zval(ret, model->u.element->name, val);
				}
			}
			break;
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL:
		case XSD_CONTENT_CHOICE: {
			sdlContentModelPtr *tmp;

			zend_hash_internal_pointer_reset(model->u.content);
			while (zend_hash_get_current_data(model->u.content, (void**)&tmp) == SUCCESS) {
				model_to_zval_object(ret, *tmp, data, sdl TSRMLS_CC);
				zend_hash_move_forward(model->u.content);
			}
			break;
		}
		case XSD_CONTENT_GROUP:
			model_to_zval_object(ret, model->u.group->model, data, sdl TSRMLS_CC);
			break;
		default:
		  break;
	}
}

/* Struct encode/decode */
static zval *to_zval_object(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	xmlNodePtr trav;
	sdlPtr sdl;
	sdlTypePtr sdlType = type->sdl_type;
	TSRMLS_FETCH();

	sdl = SOAP_GLOBAL(sdl);
	if (sdlType) {
		if (sdlType->kind == XSD_TYPEKIND_RESTRICTION &&
		    sdlType->encode && type != &sdlType->encode->details) {
			encodePtr enc;

			enc = sdlType->encode;
			while (enc && enc->details.sdl_type &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_SIMPLE &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_LIST &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_UNION) {
				enc = enc->details.sdl_type->encode;
			}
			if (enc) {
				zval *base;

				MAKE_STD_ZVAL(ret);

				object_init(ret);
				base = master_to_zval(enc, data);
#ifdef ZEND_ENGINE_2
				base->refcount--;
#endif
				add_property_zval(ret, "_", base);
			} else {
				MAKE_STD_ZVAL(ret);
				FIND_XML_NULL(data, ret);
				object_init(ret);
			}
		} else if (sdlType->kind == XSD_TYPEKIND_EXTENSION &&
		           sdlType->encode &&
		           type != &sdlType->encode->details) {
			if (sdlType->encode->details.sdl_type &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_SIMPLE &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_LIST &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_UNION) {
				ret = master_to_zval(sdlType->encode, data);
				FIND_XML_NULL(data, ret);
			} else {
				zval *base;

				MAKE_STD_ZVAL(ret);

				object_init(ret);
				base = master_to_zval(sdlType->encode, data);
#ifdef ZEND_ENGINE_2
				base->refcount--;
#endif
				add_property_zval(ret, "_", base);
			}
		} else {
			MAKE_STD_ZVAL(ret);
			FIND_XML_NULL(data, ret);
			object_init(ret);
		}
		if (sdlType->model) {
			model_to_zval_object(ret, sdlType->model, data, sdl TSRMLS_CC);
		}
		if (sdlType->attributes) {
			sdlAttributePtr *attr;

			zend_hash_internal_pointer_reset(sdlType->attributes);
			while (zend_hash_get_current_data(sdlType->attributes, (void**)&attr) == SUCCESS) {
				if ((*attr)->name) {
					xmlAttrPtr val = get_attribute(data->properties, (*attr)->name);
					xmlChar *str_val = NULL;

					if (val && val->children && val->children->content) {
						str_val = val->children->content;
						if ((*attr)->fixed && strcmp((*attr)->fixed,str_val) != 0) {
							php_error(E_ERROR,"SOAP-ERROR: Encoding: Attribute '%s' has fixed value '%s' (value '%s' is not allowed)",(*attr)->name,(*attr)->fixed,str_val);
						}
					} else if ((*attr)->def) {
						str_val = (*attr)->def;
					}
					if (str_val) {
						xmlNodePtr dummy;
						zval *data;

						dummy = xmlNewNode(NULL, "BOGUS");
						xmlNodeSetContent(dummy, str_val);
						data = master_to_zval((*attr)->encode, dummy);
						xmlFreeNode(dummy);
#ifdef ZEND_ENGINE_2
						data->refcount--;
#endif
						add_property_zval(ret, (*attr)->name, data);
					}
				}
				zend_hash_move_forward(sdlType->attributes);
			}
		}
	} else {

		MAKE_STD_ZVAL(ret);
		FIND_XML_NULL(data, ret);
		object_init(ret);

		trav = data->children;

		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				encodePtr enc = NULL;
				zval *tmpVal;

				xmlAttrPtr typeAttr = get_attribute(trav->properties,"type");
				if (typeAttr != NULL && typeAttr->children && typeAttr->children->content) {
					enc = get_encoder_from_prefix(sdl, trav, typeAttr->children->content);
				}
				tmpVal = master_to_zval(enc, trav);
#ifdef ZEND_ENGINE_2
				tmpVal->refcount--;
#endif
				add_property_zval(ret, (char *)trav->name, tmpVal);
			}
			trav = trav->next;
		}
	}
	return ret;
}

static int model_to_xml_object(xmlNodePtr node, sdlContentModelPtr model, HashTable *prop, int style, int strict)
{
	switch (model->kind) {
		case XSD_CONTENT_ELEMENT: {
			zval **data;
			xmlNodePtr property;
			encodePtr enc;

			if (zend_hash_find(prop, model->u.element->name, strlen(model->u.element->name)+1, (void**)&data) == SUCCESS) {
				enc = model->u.element->encode;
				if ((model->max_occurs == -1 || model->max_occurs > 1) && Z_TYPE_PP(data) == IS_ARRAY) {
					HashTable *ht = Z_ARRVAL_PP(data);
					zval **val;

					zend_hash_internal_pointer_reset(ht);
					while (zend_hash_get_current_data(ht,(void**)&val) == SUCCESS) {
						if (Z_TYPE_PP(val) == IS_NULL && model->u.element->nillable) {
							property = xmlNewNode(NULL,"BOGUS");
							xmlAddChild(node, property);
							if (style == SOAP_ENCODED) {
								xmlSetProp(property, "xsi:nil", "1");
							} else {
							  xmlNsPtr xsi = encode_add_ns(property,XSI_NAMESPACE);
								xmlSetNsProp(property, xsi, "nil", "1");
							}
						} else {
							property = master_to_xml(enc, *val, style, node);
						}
						xmlNodeSetName(property, model->u.element->name);
						zend_hash_move_forward(ht);
					}
				} else {
					if (Z_TYPE_PP(data) == IS_NULL && model->u.element->nillable) {
						property = xmlNewNode(NULL,"BOGUS");
						xmlAddChild(node, property);
						if (style == SOAP_ENCODED) {
							xmlSetProp(property, "xsi:nil", "1");
						} else {
						  xmlNsPtr xsi = encode_add_ns(property,XSI_NAMESPACE);
							xmlSetNsProp(property, xsi, "nil", "1");
						}
					} else {
						property = master_to_xml(enc, *data, style, node);
					}
					xmlNodeSetName(property, model->u.element->name);
				}
				return 1;
			} else if (model->min_occurs == 0) {
				return 1;
			} else {
				if (strict) {
					php_error(E_ERROR, "SOAP-ERROR: Encoding: object hasn't '%s' property",model->u.element->name);
				}
				return 0;
			}
			break;
		}
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL: {
			sdlContentModelPtr *tmp;

			zend_hash_internal_pointer_reset(model->u.content);
			while (zend_hash_get_current_data(model->u.content, (void**)&tmp) == SUCCESS) {
				if (!model_to_xml_object(node, *tmp, prop, style, model->min_occurs > 0)) {
					return 0;
				}
				zend_hash_move_forward(model->u.content);
			}
			return 1;
		}
		case XSD_CONTENT_CHOICE: {
			sdlContentModelPtr *tmp;

			zend_hash_internal_pointer_reset(model->u.content);
			while (zend_hash_get_current_data(model->u.content, (void**)&tmp) == SUCCESS) {
				if (model_to_xml_object(node, *tmp, prop, style, 0)) {
					return 1;
				}
				zend_hash_move_forward(model->u.content);
			}
			return 0;
		}
		case XSD_CONTENT_GROUP: {
			return model_to_xml_object(node, model->u.group->model, prop, style, model->min_occurs > 0);
		}
		default:
		  break;
	}
	return 1;
}

static sdlTypePtr model_array_element(sdlContentModelPtr model)
{
	switch (model->kind) {
		case XSD_CONTENT_ELEMENT: {
			if (model->max_occurs == -1 || model->max_occurs > 1) {
			  return model->u.element;
			} else {
			  return NULL;
			}
		}
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL:
		case XSD_CONTENT_CHOICE: {
			sdlContentModelPtr *tmp;

			if (zend_hash_num_elements(model->u.content) != 1) {
			  return NULL;
			}
			zend_hash_internal_pointer_reset(model->u.content);
			zend_hash_get_current_data(model->u.content, (void**)&tmp);
			return model_array_element(*tmp);
		}
		case XSD_CONTENT_GROUP: {
			return model_array_element(model->u.group->model);
		}
		default:
		  break;
	}
	return NULL;
}

static xmlNodePtr to_xml_object(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr xmlParam;
	HashTable *prop;
	int i;
	sdlTypePtr sdlType = type->sdl_type;
	TSRMLS_FETCH();

	if (!data || Z_TYPE_P(data) == IS_NULL) {
		xmlParam = xmlNewNode(NULL,"BOGUS");
		xmlAddChild(parent, xmlParam);
	  if (style == SOAP_ENCODED) {
			xmlSetProp(xmlParam, "xsi:nil", "1");
		}
		return xmlParam;
	}

	if (sdlType) {
		prop = NULL;
		if (Z_TYPE_P(data) == IS_OBJECT) {
			prop = Z_OBJPROP_P(data);
		} else if (Z_TYPE_P(data) == IS_ARRAY) {
			prop = Z_ARRVAL_P(data);
		}
		if (sdlType->kind == XSD_TYPEKIND_RESTRICTION &&
		    sdlType->encode && type != &sdlType->encode->details) {
			encodePtr enc;

			enc = sdlType->encode;
			while (enc && enc->details.sdl_type &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_SIMPLE &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_LIST &&
			       enc->details.sdl_type->kind != XSD_TYPEKIND_UNION) {
				enc = enc->details.sdl_type->encode;
			}
			if (enc) {
				zval **tmp;
				if (prop && zend_hash_find(prop, "_", sizeof("_"), (void**)&tmp) == SUCCESS) {
					xmlParam = master_to_xml(enc, *tmp, style, parent);
				} else if (prop == NULL) {
					xmlParam = master_to_xml(enc, data, style, parent);
				} else {
					xmlParam = xmlNewNode(NULL,"BOGUS");
					xmlAddChild(parent, xmlParam);
				}
			} else {
				xmlParam = xmlNewNode(NULL,"BOGUS");
				xmlAddChild(parent, xmlParam);
			}
		} else if (sdlType->kind == XSD_TYPEKIND_EXTENSION &&
		           sdlType->encode && type != &sdlType->encode->details) {
			if (sdlType->encode->details.sdl_type &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_SIMPLE &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_LIST &&
			    sdlType->encode->details.sdl_type->kind != XSD_TYPEKIND_UNION) {
				xmlParam = master_to_xml(sdlType->encode, data, style, parent);
			} else {
				zval **tmp;

				if (prop && zend_hash_find(prop, "_", sizeof("_"), (void**)&tmp) == SUCCESS) {
					xmlParam = master_to_xml(sdlType->encode, *tmp, style, parent);
				} else if (prop == NULL) {
					xmlParam = master_to_xml(sdlType->encode, data, style, parent);
				} else {
					xmlParam = xmlNewNode(NULL,"BOGUS");
					xmlAddChild(parent, xmlParam);
				}
			}
		} else {
			xmlParam = xmlNewNode(NULL,"BOGUS");
			xmlAddChild(parent, xmlParam);
		}
		FIND_ZVAL_NULL(data, xmlParam, style);

		if (prop != NULL) {
		  sdlTypePtr array_el;

		  if (Z_TYPE_P(data) == IS_ARRAY && 
		      !is_map(data) && 
		      sdlType->attributes == NULL &&
		      sdlType->model != NULL &&
		      (array_el = model_array_element(sdlType->model)) != NULL) {
				zval **val;

				zend_hash_internal_pointer_reset(prop);
				while (zend_hash_get_current_data(prop,(void**)&val) == SUCCESS) {
					xmlNodePtr property;
					if (Z_TYPE_PP(val) == IS_NULL && array_el->nillable) {
						property = xmlNewNode(NULL,"BOGUS");
						xmlAddChild(xmlParam, property);
						if (style == SOAP_ENCODED) {
							xmlSetProp(property, "xsi:nil", "1");
						} else {
						  xmlNsPtr xsi = encode_add_ns(property,XSI_NAMESPACE);
							xmlSetNsProp(property, xsi, "nil", "1");
						}
					} else {
						property = master_to_xml(array_el->encode, *val, style, xmlParam);
					}
					xmlNodeSetName(property, array_el->name);
					zend_hash_move_forward(prop);
				}
			} else if (sdlType->model) {
				model_to_xml_object(xmlParam, sdlType->model, prop, style, 1);
			}
			if (sdlType->attributes) {
				sdlAttributePtr *attr;
				zval **data;

				zend_hash_internal_pointer_reset(sdlType->attributes);
				while (zend_hash_get_current_data(sdlType->attributes, (void**)&attr) == SUCCESS) {
					if ((*attr)->name) {
						if (zend_hash_find(prop, (*attr)->name, strlen((*attr)->name)+1, (void**)&data) == SUCCESS) {
							xmlNodePtr dummy;

							dummy = master_to_xml((*attr)->encode, *data, SOAP_LITERAL, xmlParam);
							if (dummy->children && dummy->children->content) {
								if ((*attr)->fixed && strcmp((*attr)->fixed,dummy->children->content) != 0) {
									php_error(E_ERROR,"SOAP-ERROR: Encoding: Attribute '%s' has fixed value '%s' (value '%s' is not allowed)",(*attr)->name,(*attr)->fixed,dummy->children->content);
								}
								xmlSetProp(xmlParam, (*attr)->name, dummy->children->content);
							}
							xmlUnlinkNode(dummy);
							xmlFreeNode(dummy);
						}
					}
					zend_hash_move_forward(sdlType->attributes);
				}
			}
		}
		if (style == SOAP_ENCODED) {
			set_ns_and_type(xmlParam, type);
		}
	} else {
		xmlParam = xmlNewNode(NULL,"BOGUS");
		xmlAddChild(parent, xmlParam);
		FIND_ZVAL_NULL(data, xmlParam, style);

		prop = NULL;
		if (Z_TYPE_P(data) == IS_OBJECT) {
			prop = Z_OBJPROP_P(data);
		} else if (Z_TYPE_P(data) == IS_ARRAY) {
			prop = Z_ARRVAL_P(data);
		}
		if (prop != NULL) {
			i = zend_hash_num_elements(prop);
			zend_hash_internal_pointer_reset(prop);

			for (;i > 0;i--) {
				xmlNodePtr property;
				zval **zprop;
				char *str_key;
				ulong index;
				int key_type;

				key_type = zend_hash_get_current_key(prop, &str_key, &index, FALSE);
				zend_hash_get_current_data(prop, (void **)&zprop);

				property = master_to_xml(get_conversion((*zprop)->type), (*zprop), style, xmlParam);

				if (key_type == HASH_KEY_IS_STRING) {
					xmlNodeSetName(property, str_key);
				}
				zend_hash_move_forward(prop);
			}
		}
		if (style == SOAP_ENCODED) {
			set_ns_and_type(xmlParam, type);
		}
	}
	return xmlParam;
}

/* Array encode/decode */
static xmlNodePtr guess_array_map(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	encodePtr enc = NULL;
	TSRMLS_FETCH();

	if (data && Z_TYPE_P(data) == IS_ARRAY) {
		if (is_map(data)) {
			enc = get_conversion(APACHE_MAP);
		} else {
			enc = get_conversion(SOAP_ENC_ARRAY);
		}
	}
	if (!enc) {
		enc = get_conversion(IS_NULL);
	}

	return master_to_xml(enc, data, style, parent);
}

static int calc_dimension_12(const char* str)
{
	int i = 0, flag = 0;
	while (*str != '\0' && (*str < '0' || *str > '9') && (*str != '*')) {
		str++;
	}
	if (*str == '*') {
		i++;
		str++;
	}
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			if (flag == 0) {
	   		i++;
	   		flag = 1;
	   	}
	  } else if (*str == '*') {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: '*' may only be first arraySize value in list");
		} else {
			flag = 0;
		}
		str++;
	}
	return i;
}

static int* get_position_12(int dimension, const char* str)
{
	int *pos;
	int i = -1, flag = 0;

	pos = emalloc(sizeof(int)*dimension);
	memset(pos,0,sizeof(int)*dimension);
	while (*str != '\0' && (*str < '0' || *str > '9') && (*str != '*')) {
		str++;
	}
	if (*str == '*') {
		str++;
		i++;
	}
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			if (flag == 0) {
				i++;
				flag = 1;
			}
			pos[i] = (pos[i]*10)+(*str-'0');
		} else if (*str == '*') {
			php_error(E_ERROR,"SOAP-ERROR: Encoding: '*' may only be first arraySize value in list");
		} else {
		  flag = 0;
		}
		str++;
	}
	return pos;
}

static int calc_dimension(const char* str)
{
	int i = 1;
	while (*str != ']' && *str != '\0') {
		if (*str == ',') {
    		i++;
		}
		str++;
	}
	return i;
}

static void get_position_ex(int dimension, const char* str, int** pos)
{
	int i = 0;

	memset(*pos,0,sizeof(int)*dimension);
	while (*str != ']' && *str != '\0' && i < dimension) {
		if (*str >= '0' && *str <= '9') {
			(*pos)[i] = ((*pos)[i]*10)+(*str-'0');
		} else if (*str == ',') {
			i++;
		}
		str++;
	}
}

static int* get_position(int dimension, const char* str)
{
	int *pos;

	pos = emalloc(sizeof(int)*dimension);
	get_position_ex(dimension, str, &pos);
	return pos;
}

static void add_xml_array_elements(xmlNodePtr xmlParam,
                                   sdlTypePtr type,
                                   encodePtr enc,
                                   xmlNsPtr ns,
                                   int dimension ,
                                   int* dims,
                                   zval* data,
                                   int style)
{
	int j;

	if (data && Z_TYPE_P(data) == IS_ARRAY) {
	 	zend_hash_internal_pointer_reset(data->value.ht);
		for (j=0; j<dims[0]; j++) {
 			zval **zdata;

 			if (zend_hash_get_current_data(data->value.ht, (void **)&zdata) != SUCCESS) {
 				zdata = NULL;
 			}
 			if (dimension == 1) {
	 			xmlNodePtr xparam;

	 			if (zdata) {
	 				if (enc == NULL) {
						TSRMLS_FETCH();
 						xparam = master_to_xml(get_conversion((*zdata)->type), (*zdata), style, xmlParam);
 					} else {
 						xparam = master_to_xml(enc, (*zdata), style, xmlParam);
		 			}
		 		} else {
					xparam = xmlNewNode(NULL,"BOGUS");
					xmlAddChild(xmlParam, xparam);
		 		}

	 			if (type) {
 					xmlNodeSetName(xparam, type->name);
 				} else if (style == SOAP_LITERAL && enc && enc->details.type_str) {
					xmlNodeSetName(xparam, enc->details.type_str);
					xmlSetNs(xparam, ns);
				} else {
 					xmlNodeSetName(xparam, "item");
 				}
 			} else {
 				if (zdata) {
	 			  add_xml_array_elements(xmlParam, type, enc, ns, dimension-1, dims+1, *zdata, style);
	 			} else {
	 			  add_xml_array_elements(xmlParam, type, enc, ns, dimension-1, dims+1, NULL, style);
	 			}
 			}
 			zend_hash_move_forward(data->value.ht);
 		}
 	} else {
		for (j=0; j<dims[0]; j++) {
 			if (dimension == 1) {
	 			xmlNodePtr xparam;

				xparam = xmlNewNode(NULL,"BOGUS");
				xmlAddChild(xmlParam, xparam);
	 			if (type) {
 					xmlNodeSetName(xparam, type->name);
 				} else if (style == SOAP_LITERAL && enc && enc->details.type_str) {
					xmlNodeSetName(xparam, enc->details.type_str);
					xmlSetNs(xparam, ns);
				} else {
 					xmlNodeSetName(xparam, "item");
 				}
 			} else {
 			  add_xml_array_elements(xmlParam, type, enc, ns, dimension-1, dims+1, NULL, style);
 			}
		}
 	}
}

static inline int array_num_elements(HashTable* ht)
{
	if (ht->pListTail && ht->pListTail->nKeyLength == 0) {
		return ht->pListTail->h-1;
	}
	return 0;
}

static xmlNodePtr to_xml_array(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	sdlTypePtr sdl_type = type->sdl_type;
	sdlTypePtr element_type = NULL;
	smart_str array_type = {0}, array_size = {0};
	int i;
	xmlNodePtr xmlParam;
	encodePtr enc = NULL;
	int dimension = 1;
	int* dims;
	int soap_version;

	TSRMLS_FETCH();

	soap_version = SOAP_GLOBAL(soap_version);

	xmlParam = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, xmlParam);

	FIND_ZVAL_NULL(data, xmlParam, style);

	if (Z_TYPE_P(data) == IS_ARRAY) {
		sdlAttributePtr *arrayType;
		sdlExtraAttributePtr *ext;
		sdlTypePtr elementType;

		i = zend_hash_num_elements(Z_ARRVAL_P(data));

		if (sdl_type &&
		    sdl_type->attributes &&
		    zend_hash_find(sdl_type->attributes, SOAP_1_1_ENC_NAMESPACE":arrayType",
		      sizeof(SOAP_1_1_ENC_NAMESPACE":arrayType"),
		      (void **)&arrayType) == SUCCESS &&
		    zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arrayType", sizeof(WSDL_NAMESPACE":arrayType"), (void **)&ext) == SUCCESS) {

			char *value, *end;
			zval** el;

			value = estrdup((*ext)->val);
			end = strrchr(value,'[');
			if (end) {
				*end = '\0';
				end++;
				dimension = calc_dimension(end);
			}
			if ((*ext)->ns != NULL) {
				enc = get_encoder(SOAP_GLOBAL(sdl), (*ext)->ns, value);
				get_type_str(xmlParam, (*ext)->ns, value, &array_type);
			} else {
				smart_str_appends(&array_type, value);
			}

			dims = emalloc(sizeof(int)*dimension);
			dims[0] = i;
			el = &data;
			for (i = 1; i < dimension; i++) {
				if (el != NULL && Z_TYPE_PP(el) == IS_ARRAY && Z_ARRVAL_PP(el)->pListHead) {
					el = (zval**)Z_ARRVAL_PP(el)->pListHead->pData;
					if (Z_TYPE_PP(el) == IS_ARRAY) {
						dims[i] = zend_hash_num_elements(Z_ARRVAL_PP(el));
					} else {
						dims[i] = 0;
					}
				}
			}

			smart_str_append_long(&array_size, dims[0]);
			for (i=1; i<dimension; i++) {
				smart_str_appendc(&array_size, ',');
				smart_str_append_long(&array_size, dims[i]);
			}

			efree(value);

		} else if (sdl_type &&
		           sdl_type->attributes &&
		           zend_hash_find(sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":itemType",
		             sizeof(SOAP_1_2_ENC_NAMESPACE":itemType"),
		             (void **)&arrayType) == SUCCESS &&
		           zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":itemType", sizeof(WSDL_NAMESPACE":itemType"), (void **)&ext) == SUCCESS) {
			if ((*ext)->ns != NULL) {
				enc = get_encoder(SOAP_GLOBAL(sdl), (*ext)->ns, (*ext)->val);
				get_type_str(xmlParam, (*ext)->ns, (*ext)->val, &array_type);
			} else {
				smart_str_appends(&array_type, (*ext)->val);
			}
			if (zend_hash_find(sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":arraySize",
			                   sizeof(SOAP_1_2_ENC_NAMESPACE":arraySize"),
			                   (void **)&arrayType) == SUCCESS &&
			    zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arraySize", sizeof(WSDL_NAMESPACE":arraysize"), (void **)&ext) == SUCCESS) {
				dimension = calc_dimension_12((*ext)->val);
				dims = get_position_12(dimension, (*ext)->val);
				if (dims[0] == 0) {dims[0] = i;}

				smart_str_append_long(&array_size, dims[0]);
				for (i=1; i<dimension; i++) {
					smart_str_appendc(&array_size, ',');
					smart_str_append_long(&array_size, dims[i]);
				}
			} else {
				dims = emalloc(sizeof(int));
				*dims = 0;
				smart_str_append_long(&array_size, i);
			}
		} else if (sdl_type &&
		           sdl_type->attributes &&
		           zend_hash_find(sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":arraySize",
		             sizeof(SOAP_1_2_ENC_NAMESPACE":arraySize"),
		             (void **)&arrayType) == SUCCESS &&
		           zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arraySize", sizeof(WSDL_NAMESPACE":arraySize"), (void **)&ext) == SUCCESS) {
			dimension = calc_dimension_12((*ext)->val);
			dims = get_position_12(dimension, (*ext)->val);
			if (dims[0] == 0) {dims[0] = i;}

			smart_str_append_long(&array_size, dims[0]);
			for (i=1; i<dimension; i++) {
				smart_str_appendc(&array_size, ',');
				smart_str_append_long(&array_size, dims[i]);
			}

			if (sdl_type && sdl_type->elements &&
			    zend_hash_num_elements(sdl_type->elements) == 1 &&
			    (elementType = *(sdlTypePtr*)sdl_type->elements->pListHead->pData) != NULL &&
			     elementType->encode && elementType->encode->details.type_str) {
				element_type = elementType;
				enc = elementType->encode;
				get_type_str(xmlParam, elementType->encode->details.ns, elementType->encode->details.type_str, &array_type);
			} else {
				get_array_type(xmlParam, data, &array_type TSRMLS_CC);
				enc = get_encoder_ex(SOAP_GLOBAL(sdl), array_type.c, array_type.len);
			}
		} else if (sdl_type && sdl_type->elements &&
		           zend_hash_num_elements(sdl_type->elements) == 1 &&
		           (elementType = *(sdlTypePtr*)sdl_type->elements->pListHead->pData) != NULL &&
		           elementType->encode && elementType->encode->details.type_str) {

			element_type = elementType;
			enc = elementType->encode;
			get_type_str(xmlParam, elementType->encode->details.ns, elementType->encode->details.type_str, &array_type);

			smart_str_append_long(&array_size, i);

			dims = emalloc(sizeof(int)*dimension);
			dims[0] = i;
		} else {

			get_array_type(xmlParam, data, &array_type TSRMLS_CC);
			enc = get_encoder_ex(SOAP_GLOBAL(sdl), array_type.c, array_type.len);
			smart_str_append_long(&array_size, i);
			dims = emalloc(sizeof(int)*dimension);
			dims[0] = i;
		}

		if (style == SOAP_ENCODED) {
			if (soap_version == SOAP_1_1) {
				smart_str_0(&array_type);
				if (strcmp(array_type.c,"xsd:anyType") == 0) {
					smart_str_free(&array_type);
					smart_str_appendl(&array_type,"xsd:ur-type",sizeof("xsd:ur-type")-1);
				}
				smart_str_appendc(&array_type, '[');
				smart_str_append(&array_type, &array_size);
				smart_str_appendc(&array_type, ']');
				smart_str_0(&array_type);
				xmlSetProp(xmlParam, SOAP_1_1_ENC_NS_PREFIX":arrayType", array_type.c);
			} else {
				int i = 0;
				while (i < array_size.len) {
					if (array_size.c[i] == ',') {array_size.c[i] = ' ';}
					++i;
				}
				smart_str_0(&array_type);
				smart_str_0(&array_size);
				xmlSetProp(xmlParam, SOAP_1_2_ENC_NS_PREFIX":itemType", array_type.c);
				xmlSetProp(xmlParam, SOAP_1_2_ENC_NS_PREFIX":arraySize", array_size.c);
			}
		}
		smart_str_free(&array_type);
		smart_str_free(&array_size);

		add_xml_array_elements(xmlParam, element_type, enc, enc?encode_add_ns(xmlParam,enc->details.ns):NULL, dimension, dims, data, style);
		efree(dims);
	}
	if (style == SOAP_ENCODED) {
		set_ns_and_type(xmlParam, type);
	}
	return xmlParam;
}

static zval *to_zval_array(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret;
	xmlNodePtr trav;
	encodePtr enc = NULL;
	int dimension = 1;
	int* dims = NULL;
	int* pos = NULL;
	xmlAttrPtr attr;
	sdlPtr sdl;
	sdlAttributePtr *arrayType;
	sdlExtraAttributePtr *ext;
	sdlTypePtr elementType;

	TSRMLS_FETCH();

	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	sdl = SOAP_GLOBAL(sdl);

	if (data &&
	    (attr = get_attribute(data->properties,"arrayType")) &&
	    attr->children && attr->children->content) {
		char *type, *end, *ns;
		xmlNsPtr nsptr;

		parse_namespace(attr->children->content, &type, &ns);
		nsptr = xmlSearchNs(attr->doc, attr->parent, ns);

		end = strrchr(type,'[');
		if (end) {
			*end = '\0';
			dimension = calc_dimension(end+1);
			dims = get_position(dimension, end+1);
		}
		if (nsptr != NULL) {
			enc = get_encoder(SOAP_GLOBAL(sdl), nsptr->href, type);
		}
		efree(type);
		if (ns) {efree(ns);}

	} else if ((attr = get_attribute(data->properties,"itemType")) &&
	    attr->children &&
	    attr->children->content) {
		char *type, *ns;
		xmlNsPtr nsptr;

		parse_namespace(attr->children->content, &type, &ns);
		nsptr = xmlSearchNs(attr->doc, attr->parent, ns);
		if (nsptr != NULL) {
			enc = get_encoder(SOAP_GLOBAL(sdl), nsptr->href, type);
		}
		efree(type);
		if (ns) {efree(ns);}

		if ((attr = get_attribute(data->properties,"arraySize")) &&
		    attr->children && attr->children->content) {
			dimension = calc_dimension_12(attr->children->content);
			dims = get_position_12(dimension, attr->children->content);
		} else {
			dims = emalloc(sizeof(int));
			*dims = 0;
		}

	} else if ((attr = get_attribute(data->properties,"arraySize")) &&
	    attr->children && attr->children->content) {

		dimension = calc_dimension_12(attr->children->content);
		dims = get_position_12(dimension, attr->children->content);

	} else if (type->sdl_type != NULL &&
	           type->sdl_type->attributes != NULL &&
	           zend_hash_find(type->sdl_type->attributes, SOAP_1_1_ENC_NAMESPACE":arrayType",
	                          sizeof(SOAP_1_1_ENC_NAMESPACE":arrayType"),
	                          (void **)&arrayType) == SUCCESS &&
	           zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arrayType", sizeof(WSDL_NAMESPACE":arrayType"), (void **)&ext) == SUCCESS) {
		char *type, *end;

		type = estrdup((*ext)->val);
		end = strrchr(type,'[');
		if (end) {
			*end = '\0';
		}
		if ((*ext)->ns != NULL) {
			enc = get_encoder(SOAP_GLOBAL(sdl), (*ext)->ns, type);
		}
		efree(type);

		dims = emalloc(sizeof(int));
		*dims = 0;

	} else if (type->sdl_type != NULL &&
	           type->sdl_type->attributes != NULL &&
	           zend_hash_find(type->sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":itemType",
	                          sizeof(SOAP_1_2_ENC_NAMESPACE":itemType"),
	                          (void **)&arrayType) == SUCCESS &&
	           zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":itemType", sizeof(WSDL_NAMESPACE":itemType"), (void **)&ext) == SUCCESS) {

		if ((*ext)->ns != NULL) {
			enc = get_encoder(SOAP_GLOBAL(sdl), (*ext)->ns, (*ext)->val);
		}

		if (zend_hash_find(type->sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":arraySize",
		                   sizeof(SOAP_1_2_ENC_NAMESPACE":arraySize"),
		                   (void **)&arrayType) == SUCCESS &&
		    zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arraySize", sizeof(WSDL_NAMESPACE":arraysize"), (void **)&ext) == SUCCESS) {
			dimension = calc_dimension_12((*ext)->val);
			dims = get_position_12(dimension, (*ext)->val);
		} else {
			dims = emalloc(sizeof(int));
			*dims = 0;
		}
	} else if (type->sdl_type != NULL &&
	           type->sdl_type->attributes != NULL &&
	           zend_hash_find(type->sdl_type->attributes, SOAP_1_2_ENC_NAMESPACE":arraySize",
	                          sizeof(SOAP_1_2_ENC_NAMESPACE":arraySize"),
	                          (void **)&arrayType) == SUCCESS &&
	           zend_hash_find((*arrayType)->extraAttributes, WSDL_NAMESPACE":arraySize", sizeof(WSDL_NAMESPACE":arraysize"), (void **)&ext) == SUCCESS) {

		dimension = calc_dimension_12((*ext)->val);
		dims = get_position_12(dimension, (*ext)->val);
		if (type->sdl_type && type->sdl_type->elements &&
		    zend_hash_num_elements(type->sdl_type->elements) == 1 &&
		    (elementType = *(sdlTypePtr*)type->sdl_type->elements->pListHead->pData) != NULL &&
		    elementType->encode) {
			enc = elementType->encode;
		}
	} else if (type->sdl_type && type->sdl_type->elements &&
	           zend_hash_num_elements(type->sdl_type->elements) == 1 &&
	           (elementType = *(sdlTypePtr*)type->sdl_type->elements->pListHead->pData) != NULL &&
	           elementType->encode) {
		enc = elementType->encode;
	}
	if (dims == NULL) {
		dimension = 1;
		dims = emalloc(sizeof(int));
		*dims = 0;
	}
	pos = emalloc(sizeof(int)*dimension);
	memset(pos,0,sizeof(int)*dimension);
	if (data &&
	    (attr = get_attribute(data->properties,"offset")) &&
	     attr->children && attr->children->content) {
		char* tmp = strrchr(attr->children->content,'[');

		if (tmp == NULL) {
			tmp = attr->children->content;
		}
		get_position_ex(dimension, tmp, &pos);
	}

	array_init(ret);
	trav = data->children;
	while (trav) {
		if (trav->type == XML_ELEMENT_NODE) {
			int i;
			zval *tmpVal, *ar;
			encodePtr typeEnc = NULL;
			xmlAttrPtr type = get_attribute(trav->properties,"type");
			xmlAttrPtr position = get_attribute(trav->properties,"position");
			if (type != NULL && type->children && type->children->content) {
				typeEnc = get_encoder_from_prefix(sdl, trav, type->children->content);
			}
			if (typeEnc) {
				tmpVal = master_to_zval(typeEnc, trav);
			} else {
				tmpVal = master_to_zval(enc, trav);
			}
			if (position != NULL && position->children && position->children->content) {
				char* tmp = strrchr(position->children->content,'[');
				if (tmp == NULL) {
					tmp = position->children->content;
				}
				get_position_ex(dimension, tmp, &pos);
			}

			/* Get/Create intermediate arrays for multidimensional arrays */
			i = 0;
			ar = ret;
			while (i < dimension-1) {
				zval** ar2;
				if (zend_hash_index_find(Z_ARRVAL_P(ar), pos[i], (void**)&ar2) == SUCCESS) {
					ar = *ar2;
				} else {
					zval *tmpAr;
					MAKE_STD_ZVAL(tmpAr);
					array_init(tmpAr);
					zend_hash_index_update(Z_ARRVAL_P(ar), pos[i], &tmpAr, sizeof(zval*), (void**)&ar2);
					ar = *ar2;
				}
				i++;
			}
			zend_hash_index_update(Z_ARRVAL_P(ar), pos[i], &tmpVal, sizeof(zval *), NULL);

			/* Increment position */
			i = dimension;
			while (i > 0) {
			  i--;
			  pos[i]++;
				if (pos[i] >= dims[i]) {
					if (i > 0) {
						pos[i] = 0;
					} else {
						/* TODO: Array index overflow */
					}
				} else {
				  break;
				}
			}
		}
		trav = trav->next;
	}
	efree(dims);
	efree(pos);
	return ret;
}

/* Map encode/decode */
static xmlNodePtr to_xml_map(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	xmlNodePtr xmlParam;
	int i;
	TSRMLS_FETCH();

	xmlParam = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, xmlParam);
	FIND_ZVAL_NULL(data, xmlParam, style);

	if (Z_TYPE_P(data) == IS_ARRAY) {
		i = zend_hash_num_elements(Z_ARRVAL_P(data));
		zend_hash_internal_pointer_reset(data->value.ht);
		for (;i > 0;i--) {
			xmlNodePtr xparam, item;
			xmlNodePtr key;
			zval **temp_data;
			char *key_val;
			int int_val;

			zend_hash_get_current_data(data->value.ht, (void **)&temp_data);
			if (Z_TYPE_PP(temp_data) != IS_NULL) {
				item = xmlNewNode(NULL,"item");
				xmlAddChild(xmlParam, item);
				key = xmlNewNode(NULL,"key");
				xmlAddChild(item,key);
				if (zend_hash_get_current_key(data->value.ht, &key_val, (long *)&int_val, FALSE) == HASH_KEY_IS_STRING) {
					if (style == SOAP_ENCODED) {
						xmlSetProp(key, "xsi:type", "xsd:string");
					}
					xmlNodeSetContent(key, key_val);
				} else {
					smart_str tmp = {0};
					smart_str_append_long(&tmp, int_val);
					smart_str_0(&tmp);

					if (style == SOAP_ENCODED) {
						xmlSetProp(key, "xsi:type", "xsd:int");
					}
					xmlNodeSetContentLen(key, tmp.c, tmp.len);

					smart_str_free(&tmp);
				}

				xparam = master_to_xml(get_conversion((*temp_data)->type), (*temp_data), style, item);

				xmlNodeSetName(xparam, "value");
			}
			zend_hash_move_forward(data->value.ht);
		}
	}
	if (style == SOAP_ENCODED) {
		set_ns_and_type(xmlParam, type);
	}

	return xmlParam;
}

static zval *to_zval_map(encodeTypePtr type, xmlNodePtr data)
{
	zval *ret, *key, *value;
	xmlNodePtr trav, item, xmlKey, xmlValue;
	TSRMLS_FETCH();

	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		array_init(ret);
		trav = data->children;

		trav = data->children;
		FOREACHNODE(trav, "item", item) {
			xmlKey = get_node(item->children, "key");
			if (!xmlKey) {
				php_error(E_ERROR, "SOAP-ERROR: Encoding: Can't decode apache map, missing key");
			}

			xmlValue = get_node(item->children, "value");
			if (!xmlKey) {
				php_error(E_ERROR, "SOAP-ERROR: Encoding: Can't decode apache map, missing value");
			}

			key = master_to_zval(NULL, xmlKey);
			value = master_to_zval(NULL, xmlValue);

			if (Z_TYPE_P(key) == IS_STRING) {
				zend_hash_update(Z_ARRVAL_P(ret), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &value, sizeof(zval *), NULL);
			} else if (Z_TYPE_P(key) == IS_LONG) {
				zend_hash_index_update(Z_ARRVAL_P(ret), Z_LVAL_P(key), &value, sizeof(zval *), NULL);
			} else {
				php_error(E_ERROR, "SOAP-ERROR: Encoding: Can't decode apache map, only Strings or Longs are allowd as keys");
			}
			zval_ptr_dtor(&key);
		}
		ENDFOREACH(trav);
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

/* Unknown encode/decode */
static xmlNodePtr guess_xml_convert(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	encodePtr enc;
	TSRMLS_FETCH();

	if (data) {
		enc = get_conversion(data->type);
	} else {
		enc = get_conversion(IS_NULL);
	}
	return master_to_xml(enc, data, style, parent);
}

static zval *guess_zval_convert(encodeTypePtr type, xmlNodePtr data)
{
	encodePtr enc = NULL;
	xmlAttrPtr tmpattr;
	TSRMLS_FETCH();

	data = check_and_resolve_href(data);

	if (data == NULL) {
		enc = get_conversion(IS_NULL);
	} else if (data->properties && get_attribute(data->properties, "nil")) {
		enc = get_conversion(IS_NULL);
	} else {
		tmpattr = get_attribute(data->properties,"type");
		if (tmpattr != NULL) {
			enc = get_encoder_from_prefix(SOAP_GLOBAL(sdl), data, tmpattr->children->content);
			if (enc != NULL && enc->details.sdl_type != NULL) {
				enc = NULL;
			}
		}

		if (enc == NULL) {
			/* Didn't have a type, totally guess here */
			/* Logic: has children = IS_OBJECT else IS_STRING */
			xmlNodePtr trav;

			if (get_attribute(data->properties, "arrayType") ||
			    get_attribute(data->properties, "itemType") ||
			    get_attribute(data->properties, "arraySize")) {
				enc = get_conversion(SOAP_ENC_ARRAY);
			} else {
				enc = get_conversion(XSD_STRING);
				trav = data->children;
				while (trav != NULL) {
					if (trav->type == XML_ELEMENT_NODE) {
						enc = get_conversion(SOAP_ENC_OBJECT);
						break;
					}
					trav = trav->next;
				}
			}
		}
	}
	return master_to_zval(enc, data);
}

/* Time encode/decode */
static xmlNodePtr to_xml_datetime_ex(encodeTypePtr type, zval *data, char *format, int style, xmlNodePtr parent)
{
	/* logic hacked from ext/standard/datetime.c */
	struct tm *ta, tmbuf;
	time_t timestamp;
	int max_reallocs = 5;
	size_t buf_len=64, real_len;
	char *buf;
	char tzbuf[6];

	xmlNodePtr xmlParam;

	xmlParam = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, xmlParam);
	FIND_ZVAL_NULL(data, xmlParam, style);

	if (Z_TYPE_P(data) == IS_LONG) {
		timestamp = Z_LVAL_P(data);
		ta = php_localtime_r(&timestamp, &tmbuf);
		/*ta = php_gmtime_r(&timestamp, &tmbuf);*/

		buf = (char *) emalloc(buf_len);
		while ((real_len = strftime(buf, buf_len, format, ta)) == buf_len || real_len == 0) {
			buf_len *= 2;
			buf = (char *) erealloc(buf, buf_len);
			if (!--max_reallocs) break;
		}

		/* Time zone support */
#ifdef HAVE_TM_GMTOFF
		sprintf(tzbuf, "%c%02d%02d", (ta->tm_gmtoff < 0) ? '-' : '+', abs(ta->tm_gmtoff / 3600), abs( (ta->tm_gmtoff % 3600) / 60 ));
#else
# ifdef ZEND_WIN32
		sprintf(tzbuf, "%c%02d%02d", ((ta->tm_isdst ? timezone - 3600:timezone)>0)?'-':'+', abs((ta->tm_isdst ? timezone - 3600 : timezone) / 3600), abs(((ta->tm_isdst ? timezone - 3600 : timezone) % 3600) / 60));
# else
		sprintf(tzbuf, "%c%02d%02d", ((ta->tm_isdst ? tzone - 3600:tzone)>0)?'-':'+', abs((ta->tm_isdst ? tzone - 3600 : tzone) / 3600), abs(((ta->tm_isdst ? tzone - 3600 : tzone) % 3600) / 60));
# endif
#endif
		if (strcmp(tzbuf,"+0000") == 0) {
		  strcpy(tzbuf,"Z");
		  real_len++;
		} else {
			real_len += 5;
		}
		if (real_len >= buf_len) {
			buf = (char *) erealloc(buf, real_len+1);
		}
		strcat(buf, tzbuf);

		xmlNodeSetContent(xmlParam, buf);
		efree(buf);
	} else if (Z_TYPE_P(data) == IS_STRING) {
		xmlNodeSetContentLen(xmlParam, Z_STRVAL_P(data), Z_STRLEN_P(data));
	}

	if (style == SOAP_ENCODED) {
		set_ns_and_type(xmlParam, type);
	}
	return xmlParam;
}

static xmlNodePtr to_xml_duration(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	/* TODO: '-'?P([0-9]+Y)?([0-9]+M)?([0-9]+D)?T([0-9]+H)?([0-9]+M)?([0-9]+S)? */
	return to_xml_string(type, data, style, parent);
}

static xmlNodePtr to_xml_datetime(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "%Y-%m-%dT%H:%M:%S", style, parent);
}

static xmlNodePtr to_xml_time(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	/* TODO: microsecconds */
	return to_xml_datetime_ex(type, data, "%H:%M:%S", style, parent);
}

static xmlNodePtr to_xml_date(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "%Y-%m-%d", style, parent);
}

static xmlNodePtr to_xml_gyearmonth(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "%Y-%m", style, parent);
}

static xmlNodePtr to_xml_gyear(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "%Y", style, parent);
}

static xmlNodePtr to_xml_gmonthday(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "--%m-%d", style, parent);
}

static xmlNodePtr to_xml_gday(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "---%d", style, parent);
}

static xmlNodePtr to_xml_gmonth(encodeTypePtr type, zval *data, int style, xmlNodePtr parent)
{
	return to_xml_datetime_ex(type, data, "--%m--", style, parent);
}

static zval* to_zval_list(encodeTypePtr enc, xmlNodePtr data) {
	/*FIXME*/
	return to_zval_stringc(enc, data);
}

static xmlNodePtr to_xml_list(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent) {
	xmlNodePtr ret;
	encodePtr list_enc = NULL;

	if (enc->sdl_type && enc->sdl_type->kind == XSD_TYPEKIND_LIST && enc->sdl_type->elements) {
		sdlTypePtr *type;

		zend_hash_internal_pointer_reset(enc->sdl_type->elements);
		if (zend_hash_get_current_data(enc->sdl_type->elements, (void**)&type) == SUCCESS) {
			list_enc = (*type)->encode;
		}
	}

	ret = xmlNewNode(NULL,"BOGUS");
	xmlAddChild(parent, ret);
	FIND_ZVAL_NULL(data, ret, style);
	if (Z_TYPE_P(data) == IS_ARRAY) {
		zval **tmp;
		smart_str list = {0};
		HashTable *ht = Z_ARRVAL_P(data);

		zend_hash_internal_pointer_reset(ht);
		while (zend_hash_get_current_data(ht, (void**)&tmp) == SUCCESS) {
			xmlNodePtr dummy = master_to_xml(list_enc, *tmp, SOAP_LITERAL, ret);
			if (dummy && dummy->children && dummy->children->content) {
				if (list.len != 0) {
					smart_str_appendc(&list, ' ');
				}
				smart_str_appends(&list, dummy->children->content);
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
			}
			xmlUnlinkNode(dummy);
			xmlFreeNode(dummy);
			zend_hash_move_forward(ht);
		}
		smart_str_0(&list);
		xmlNodeSetContentLen(ret, list.c, list.len);
		smart_str_free(&list);
	} else {
		zval tmp = *data;
		char *str, *start, *next;
		smart_str list = {0};

		if (Z_TYPE_P(data) != IS_STRING) {
			zval_copy_ctor(&tmp);
			convert_to_string(&tmp);
			data = &tmp;
		}
		str = estrndup(Z_STRVAL_P(data), Z_STRLEN_P(data));
		whiteSpace_collapse(str);
		start = str;
		while (start != NULL && *start != '\0') {
			xmlNodePtr dummy;
			zval dummy_zval;

			next = strchr(start,' ');
			if (next != NULL) {
			  *next = '\0';
			  next++;
			}
			ZVAL_STRING(&dummy_zval, start, 0);
			dummy = master_to_xml(list_enc, &dummy_zval, SOAP_LITERAL, ret);
			if (dummy && dummy->children && dummy->children->content) {
				if (list.len != 0) {
					smart_str_appendc(&list, ' ');
				}
				smart_str_appends(&list, dummy->children->content);
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of encoding rules");
			}
			xmlUnlinkNode(dummy);
			xmlFreeNode(dummy);

			start = next;
		}
		smart_str_0(&list);
		xmlNodeSetContentLen(ret, list.c, list.len);
		smart_str_free(&list);
		efree(str);
		if (data == &tmp) {zval_dtor(&tmp);}
	}
	return ret;
}

static xmlNodePtr to_xml_list1(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent) {
	/*FIXME: minLength=1 */
	return to_xml_list(enc,data,style, parent);
}

static zval* to_zval_union(encodeTypePtr enc, xmlNodePtr data) {
	/*FIXME*/
	return to_zval_list(enc, data);
}

static xmlNodePtr to_xml_union(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent) {
	/*FIXME*/
	return to_xml_list(enc,data,style, parent);
}

zval *sdl_guess_convert_zval(encodeTypePtr enc, xmlNodePtr data)
{
	sdlTypePtr type;

	type = enc->sdl_type;
/*FIXME: restriction support
	if (type && type->restrictions &&
	    data &&  data->children && data->children->content) {
		if (type->restrictions->whiteSpace && type->restrictions->whiteSpace->value) {
			if (strcmp(type->restrictions->whiteSpace->value,"replace") == 0) {
				whiteSpace_replace(data->children->content);
			} else if (strcmp(type->restrictions->whiteSpace->value,"collapse") == 0) {
				whiteSpace_collapse(data->children->content);
			}
		}
		if (type->restrictions->enumeration) {
			if (!zend_hash_exists(type->restrictions->enumeration,data->children->content,strlen(data->children->content)+1)) {
				php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: invalid enumeration value \"%s\"",data->children->content);
			}
		}
		if (type->restrictions->minLength &&
		    strlen(data->children->content) < type->restrictions->minLength->value) {
		  php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length less then 'minLength'");
		}
		if (type->restrictions->maxLength &&
		    strlen(data->children->content) > type->restrictions->maxLength->value) {
		  php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length greater then 'maxLength'");
		}
		if (type->restrictions->length &&
		    strlen(data->children->content) != type->restrictions->length->value) {
		  php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length is not equal to 'length'");
		}
	}
*/
	switch (type->kind) {
		case XSD_TYPEKIND_SIMPLE:
			if (type->encode && enc != &type->encode->details) {
				return master_to_zval(type->encode, data);
			}
			break;
		case XSD_TYPEKIND_LIST:
			return to_zval_list(enc, data);
		case XSD_TYPEKIND_UNION:
			return to_zval_union(enc, data);
		case XSD_TYPEKIND_COMPLEX:
		case XSD_TYPEKIND_RESTRICTION:
		case XSD_TYPEKIND_EXTENSION:
			if (type->encode &&
			    (type->encode->details.type == IS_ARRAY ||
			     type->encode->details.type == SOAP_ENC_ARRAY)) {
				return to_zval_array(enc, data);
			}
			return to_zval_object(enc, data);
		default:
			break;
	}
	return guess_zval_convert(enc, data);
}

xmlNodePtr sdl_guess_convert_xml(encodeTypePtr enc, zval *data, int style, xmlNodePtr parent)
{
	sdlTypePtr type;
	xmlNodePtr ret = NULL;

	type = enc->sdl_type;

/*FIXME: restriction support
	if (type) {
		if (type->restrictions && Z_TYPE_P(data) == IS_STRING) {
			if (type->restrictions->enumeration) {
				if (!zend_hash_exists(type->restrictions->enumeration,Z_STRVAL_P(data),Z_STRLEN_P(data)+1)) {
					php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: invalid enumeration value \"%s\".",Z_STRVAL_P(data));
				}
			}
			if (type->restrictions->minLength &&
			    Z_STRLEN_P(data) < type->restrictions->minLength->value) {
		  	php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length less then 'minLength'");
			}
			if (type->restrictions->maxLength &&
			    Z_STRLEN_P(data) > type->restrictions->maxLength->value) {
		  	php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length greater then 'maxLength'");
			}
			if (type->restrictions->length &&
			    Z_STRLEN_P(data) != type->restrictions->length->value) {
		  	php_error(E_WARNING,"SOAP-ERROR: Encoding: Restriction: length is not equal to 'length'");
			}
		}
	}
*/
	switch(type->kind) {
		case XSD_TYPEKIND_SIMPLE:
			if (type->encode && enc != &type->encode->details) {
				ret = master_to_xml(type->encode, data, style, parent);
			}
			break;
		case XSD_TYPEKIND_LIST:
			ret = to_xml_list(enc, data, style, parent);
			break;
		case XSD_TYPEKIND_UNION:
			ret = to_xml_union(enc, data, style, parent);
			break;
		case XSD_TYPEKIND_COMPLEX:
		case XSD_TYPEKIND_RESTRICTION:
		case XSD_TYPEKIND_EXTENSION:
			if (type->encode &&
			    (type->encode->details.type == IS_ARRAY ||
			     type->encode->details.type == SOAP_ENC_ARRAY)) {
				ret = to_xml_array(enc, data, style, parent);
			} else {
				ret = to_xml_object(enc, data, style, parent);
			}
			break;
		default:
			break;
	}
	if (ret == NULL) {
		ret = guess_xml_convert(enc, data, style, parent);
	}
	if (style == SOAP_ENCODED) {
		set_ns_and_type(ret, enc);
	}
	return ret;
}

static xmlNodePtr check_and_resolve_href(xmlNodePtr data)
{
	if (data && data->properties) {
		xmlAttrPtr href;

		href = data->properties;
		while (1) {
			href = get_attribute(href, "href");
			if (href == NULL || href->ns == NULL) {break;}
			href = href->next;
		}
		if (href) {
			/*  Internal href try and find node */
			if (href->children->content[0] == '#') {
				xmlNodePtr ret = get_node_with_attribute_recursive(data->doc->children, NULL, "id", &href->children->content[1]);
				if (!ret) {
					php_error(E_ERROR,"SOAP-ERROR: Encoding: Unresolved reference '%s'",href->children->content);
				}
				return ret;
			} else {
				/*  TODO: External href....? */
				php_error(E_ERROR,"SOAP-ERROR: Encoding: External reference '%s'",href->children->content);
			}
		}
		/* SOAP 1.2 enc:id enc:ref */
		href = get_attribute_ex(data->properties, "ref", SOAP_1_2_ENC_NAMESPACE);
		if (href) {
			char* id;
			xmlNodePtr ret;

			if (href->children->content[0] == '#') {
				id = href->children->content+1;
			} else {
				id = href->children->content;
			}
			ret = get_node_with_attribute_recursive_ex(data->doc->children, NULL, NULL, "id", id, SOAP_1_2_ENC_NAMESPACE);
			if (!ret) {
				php_error(E_ERROR,"SOAP-ERROR: Encoding: Unresolved reference '%s'",href->children->content);
			} else if (ret == data) {
				php_error(E_ERROR,"SOAP-ERROR: Encoding: Violation of id and ref information items '%s'",href->children->content);
			}
			return ret;
		}
	}
	return data;
}

static void set_ns_and_type(xmlNodePtr node, encodeTypePtr type)
{
	set_ns_and_type_ex(node, type->ns, type->type_str);
}

static void set_ns_and_type_ex(xmlNodePtr node, char *ns, char *type)
{
	smart_str nstype = {0};
	get_type_str(node, ns, type, &nstype);
	xmlSetProp(node, "xsi:type", nstype.c);
	smart_str_free(&nstype);
}

xmlNsPtr encode_add_ns(xmlNodePtr node, const char* ns)
{
	xmlNsPtr xmlns;

	if (ns == NULL) {
	  return NULL;
	}

	xmlns = xmlSearchNsByHref(node->doc,node,ns);
	if (xmlns == NULL) {
		char* prefix;
		TSRMLS_FETCH();

		if (zend_hash_find(&SOAP_GLOBAL(defEncNs), (char*)ns, strlen(ns) + 1, (void **)&prefix) == SUCCESS) {
			xmlns = xmlNewNs(node->doc->children,ns,prefix);
		} else {
			smart_str prefix = {0};
			int num = ++SOAP_GLOBAL(cur_uniq_ns);

			smart_str_appendl(&prefix, "ns", 2);
			smart_str_append_long(&prefix, num);
			smart_str_0(&prefix);
			xmlns = xmlNewNs(node->doc->children,ns,prefix.c);
			smart_str_free(&prefix);
		}
	}
	return xmlns;
}

void encode_reset_ns()
{
	TSRMLS_FETCH();
	SOAP_GLOBAL(cur_uniq_ns) = 0;
}

static encodePtr get_conversion(int encode)
{
	encodePtr *enc = NULL;
	TSRMLS_FETCH();

	if (zend_hash_index_find(&SOAP_GLOBAL(defEncIndex), encode, (void **)&enc) == FAILURE) {
		if (SOAP_GLOBAL(overrides)) {
			smart_str nscat = {0};

			smart_str_appendl(&nscat, (*enc)->details.ns, strlen((*enc)->details.ns));
			smart_str_appendc(&nscat, ':');
			smart_str_appendl(&nscat, (*enc)->details.type_str, strlen((*enc)->details.type_str));
			smart_str_0(&nscat);

			if (zend_hash_find(SOAP_GLOBAL(overrides), nscat.c, nscat.len + 1, (void **)&enc) == FAILURE) {
				smart_str_free(&nscat);
				php_error(E_ERROR, "SOAP-ERROR: Encoding: Cannot find encoding");
				return NULL;
			} else {
				smart_str_free(&nscat);
				return *enc;
			}
		} else {
			php_error(E_ERROR, "SOAP-ERROR: Encoding: Cannot find encoding");
			return NULL;
		}
	} else {
		return *enc;
	}
}

static int is_map(zval *array)
{
	int i, count = zend_hash_num_elements(Z_ARRVAL_P(array));

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(array));
	for (i = 0;i < count;i++) {
		if (zend_hash_get_current_key_type(Z_ARRVAL_P(array)) == HASH_KEY_IS_STRING) {
			return TRUE;
		}
		zend_hash_move_forward(Z_ARRVAL_P(array));
	}
	return FALSE;
}

static void get_array_type(xmlNodePtr node, zval *array, smart_str *type TSRMLS_DC)
{
	HashTable *ht = HASH_OF(array);
	int i, count, cur_type, prev_type, different;
	zval **tmp;
	char *prev_stype = NULL, *cur_stype = NULL, *prev_ns = NULL, *cur_ns = NULL;

	if (!array || Z_TYPE_P(array) != IS_ARRAY) {
		smart_str_appendl(type, "xsd:anyType", 11);
	}

	different = FALSE;
	cur_type = prev_type = 0;
	count = zend_hash_num_elements(ht);

	zend_hash_internal_pointer_reset(ht);
	for (i = 0;i < count;i++) {
		zend_hash_get_current_data(ht, (void **)&tmp);

		if (Z_TYPE_PP(tmp) == IS_OBJECT &&
		    Z_OBJCE_PP(tmp) == soap_var_class_entry) {
			zval **ztype;

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_type", sizeof("enc_type"), (void **)&ztype) == FAILURE) {
				php_error(E_ERROR, "SOAP-ERROR: Encoding: SoapVar hasn't 'enc_type' property");
			}
			cur_type = Z_LVAL_PP(ztype);

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_stype", sizeof("enc_stype"), (void **)&ztype) == SUCCESS) {
				cur_stype = Z_STRVAL_PP(ztype);
			} else {
				cur_stype = NULL;
			}

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_ns", sizeof("enc_ns"), (void **)&ztype) == SUCCESS) {
				cur_ns = Z_STRVAL_PP(ztype);
			} else {
				cur_ns = NULL;
			}

		} else if (Z_TYPE_PP(tmp) == IS_ARRAY && is_map(*tmp)) {
			cur_type = APACHE_MAP;
			cur_stype = NULL;
			cur_ns = NULL;
		} else {
			cur_type = Z_TYPE_PP(tmp);
			cur_stype = NULL;
			cur_ns = NULL;
		}

		if (i > 0) {
			if ((cur_type != prev_type) ||
			    (cur_stype != NULL && prev_stype != NULL && strcmp(cur_stype,prev_stype) != 0) ||
			    (cur_stype == NULL && cur_stype != prev_stype) ||
			    (cur_ns != NULL && prev_ns != NULL && strcmp(cur_ns,prev_ns) != 0) ||
			    (cur_ns == NULL && cur_ns != prev_ns)) {
				different = TRUE;
				break;
			}
		}

		prev_type = cur_type;
		prev_stype = cur_stype;
		prev_ns = cur_ns;
		zend_hash_move_forward(ht);
	}

	if (different || count == 0) {
		smart_str_appendl(type, "xsd:anyType", 11);
	} else {
		if (cur_stype != NULL) {
			if (cur_ns) {
				xmlNsPtr ns = encode_add_ns(node,cur_ns);
				smart_str_appends(type,ns->prefix);
				smart_str_appendc(type,':');
			}
			smart_str_appends(type,cur_stype);
			smart_str_0(type);
		} else {
			encodePtr enc;

			enc = get_conversion(cur_type);
			get_type_str(node, enc->details.ns, enc->details.type_str, type);
		}
	}
}

static void get_type_str(xmlNodePtr node, const char* ns, const char* type, smart_str* ret)
{
	TSRMLS_FETCH();

	if (ns) {
		xmlNsPtr xmlns;
		if (SOAP_GLOBAL(soap_version) == SOAP_1_2 &&
		    strcmp(ns,SOAP_1_1_ENC_NAMESPACE) == 0) {
			ns = SOAP_1_2_ENC_NAMESPACE;
		} else if (SOAP_GLOBAL(soap_version) == SOAP_1_1 &&
		           strcmp(ns,SOAP_1_2_ENC_NAMESPACE) == 0) {
			ns = SOAP_1_1_ENC_NAMESPACE;
		}
		xmlns = encode_add_ns(node,ns);
		smart_str_appends(ret, xmlns->prefix);
		smart_str_appendc(ret, ':');
	}
	smart_str_appendl(ret, type, strlen(type));
	smart_str_0(ret);
}

static void delete_mapping(void *data)
{
	soapMappingPtr map = (soapMappingPtr)data;

	if (map->ns) {
		efree(map->ns);
	}
	if (map->ctype) {
		efree(map->ctype);
	}

	if (map->type == SOAP_MAP_FUNCTION) {
		if (map->map_functions.to_xml_before) {
			zval_ptr_dtor(&map->map_functions.to_xml_before);
		}
		if (map->map_functions.to_xml) {
			zval_ptr_dtor(&map->map_functions.to_xml);
		}
		if (map->map_functions.to_xml_after) {
			zval_ptr_dtor(&map->map_functions.to_xml_after);
		}
		if (map->map_functions.to_zval_before) {
			zval_ptr_dtor(&map->map_functions.to_zval_before);
		}
		if (map->map_functions.to_zval) {
			zval_ptr_dtor(&map->map_functions.to_zval);
		}
		if (map->map_functions.to_zval_after) {
			zval_ptr_dtor(&map->map_functions.to_zval_after);
		}
	}
	efree(map);
}

void delete_encoder(void *encode)
{
	encodePtr t = *((encodePtr*)encode);
	if (t->details.ns) {
		efree(t->details.ns);
	}
	if (t->details.type_str) {
		efree(t->details.type_str);
	}
	if (t->details.map) {
		delete_mapping(t->details.map);
	}
	efree(t);
}
