#include <time.h>

#include "php_soap.h"

encode defaultEncoding[] = {
	{{UNKNOWN_TYPE, NULL, NULL, NULL}, guess_zval_convert, guess_xml_convert},

	{{IS_NULL, "nil", XSI_NAMESPACE, NULL}, to_zval_null, to_xml_null},
	{{IS_STRING, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{IS_LONG, XSD_INT_STRING, XSD_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{IS_DOUBLE, XSD_FLOAT_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{IS_BOOL, XSD_BOOLEAN_STRING, XSD_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{IS_CONSTANT, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{IS_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_ENC_NAMESPACE, NULL}, to_zval_array, guess_array_map},
	{{IS_CONSTANT_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},
	{{IS_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},

	{{XSD_STRING, XSD_STRING_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_BOOLEAN, XSD_BOOLEAN_STRING, XSD_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{XSD_DECIMAL, XSD_DECIMAL_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_FLOAT, XSD_FLOAT_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{XSD_DOUBLE, XSD_DOUBLE_STRING, XSD_NAMESPACE, NULL}, to_zval_double, to_xml_double},

	{{XSD_DATETIME, XSD_DATETIME_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_datetime},
	{{XSD_TIME, XSD_TIME_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_time},
	{{XSD_DATE, XSD_DATE_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_date},
	{{XSD_GYEARMONTH, XSD_GYEARMONTH_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_gyearmonth},
	{{XSD_GYEAR, XSD_GYEAR_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_gyear},
	{{XSD_GMONTHDAY, XSD_GMONTHDAY_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_gmonthday},
	{{XSD_GDAY, XSD_GDAY_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_gday},
	{{XSD_GMONTH, XSD_GMONTH_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_gmonth},
	{{XSD_DURATION, XSD_DURATION_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_duration},

	{{XSD_HEXBINARY, XSD_HEXBINARY_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_stringl},
	{{XSD_BASE64BINARY, XSD_BASE64BINARY_STRING, XSD_NAMESPACE, NULL}, to_zval_string, to_xml_stringl},

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

	{{APACHE_MAP, APACHE_MAP_STRING, APACHE_NAMESPACE, NULL}, to_zval_map, to_xml_map},

	{{SOAP_ENC_OBJECT, SOAP_ENC_OBJECT_STRING, SOAP_ENC_NAMESPACE, NULL}, to_zval_object, to_xml_object},
	{{SOAP_ENC_ARRAY, SOAP_ENC_ARRAY_STRING, SOAP_ENC_NAMESPACE, NULL}, to_zval_array, to_xml_array},

	/* support some of the 1999 data types */
	{{XSD_STRING, XSD_STRING_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_BOOLEAN, XSD_BOOLEAN_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_bool, to_xml_bool},
	{{XSD_DECIMAL, XSD_DECIMAL_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_string, to_xml_string},
	{{XSD_FLOAT, XSD_FLOAT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_double, to_xml_double},
	{{XSD_DOUBLE, XSD_DOUBLE_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_double, to_xml_double},

	{{XSD_LONG, XSD_LONG_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_INT, XSD_INT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_SHORT, XSD_SHORT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_BYTE, XSD_BYTE_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_long, to_xml_long},
	{{XSD_1999_TIMEINSTANT, XSD_1999_TIMEINSTANT_STRING, XSD_1999_NAMESPACE, NULL}, to_zval_string, to_xml_string},

	{{END_KNOWN_TYPES, NULL, NULL, NULL}, guess_zval_convert, guess_xml_convert}
};

xmlNodePtr master_to_xml(encodePtr encode, zval *data, int style)
{
	xmlNodePtr node;

	if(encode->to_xml_before)
		data = encode->to_xml_before(encode->details, data);
	if(encode->to_xml)
		node = encode->to_xml(encode->details, data, style);
	if(encode->to_xml_after)
		node = encode->to_xml_after(encode->details, node, style);

	return node;
}

zval *master_to_zval(encodePtr encode, xmlNodePtr data)
{
	zval *ret;
	data = check_and_resolve_href(data);
	if(encode->to_zval_before)
		data = encode->to_zval_before(encode->details, data, 0);
	if(encode->to_zval)
		ret = encode->to_zval(encode->details, data);
	if(encode->to_zval_after)
		ret = encode->to_zval_after(encode->details, ret);

	return ret;
}

#ifdef HAVE_PHP_DOMXML
zval *to_xml_before_user(encodeType type, zval *data)
{
	TSRMLS_FETCH();

	if(type.map->map_functions.to_xml_before)
	{
		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml_before, data, 1, &data  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_xml_before");
	}
	return data;
}

xmlNodePtr to_xml_user(encodeType type, zval *data, int style)
{
	zval *ret, **addr;
	xmlNodePtr node;
	TSRMLS_FETCH();

	if(type.map->map_functions.to_xml)
	{
		MAKE_STD_ZVAL(ret);
		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml, ret, 1, &data  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_xml");

		if(Z_TYPE_P(ret) != IS_OBJECT)
			php_error(E_ERROR, "Error serializing object from to_xml_user");

		if(zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS)
		{
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			node = xmlCopyNode(node, 1);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
	}
	return node;
}

xmlNodePtr to_xml_after_user(encodeType type, xmlNodePtr node, int style)
{
	zval *ret, *param, **addr;
	int found;
	TSRMLS_FETCH();

	if(type.map->map_functions.to_xml_after)
	{
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_xml_after, ret, 1, &param  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_xml_after");
		if(zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS)
		{
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&param);
	}
	return node;
}

xmlNodePtr to_zval_before_user(encodeType type, xmlNodePtr node, int style)
{
	zval *ret, *param, **addr;
	int found;
	TSRMLS_FETCH();

	if(type.map->map_functions.to_zval_before)
	{
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval_before, ret, 1, &param  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_zval_before");
		if(zend_hash_index_find(Z_OBJPROP_P(ret), 1, (void **)&addr) == SUCCESS)
		{
			node = (xmlNodePtr)Z_LVAL_PP(addr);
			set_ns_and_type(node, type);
		}
		zval_ptr_dtor(&ret);
		zval_ptr_dtor(&param);
	}
	return node;
}

zval *to_zval_user(encodeType type, xmlNodePtr node)
{
	zval *ret, *param;
	int found;
	TSRMLS_FETCH();

	if(type.map->map_functions.to_zval)
	{
		MAKE_STD_ZVAL(ret);
		MAKE_STD_ZVAL(param);
		param = php_domobject_new(node, &found, NULL TSRMLS_CC);

		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval, ret, 1, &param  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_zval");
		zval_ptr_dtor(&param);
		efree(param);
	}
	return ret;
}

zval *to_zval_after_user(encodeType type, zval *data)
{
	TSRMLS_FETCH();

	if(type.map->map_functions.to_zval_after)
	{
		if(call_user_function(EG(function_table), NULL, type.map->map_functions.to_zval_after, data, 1, &data  TSRMLS_CC) == FAILURE)
			php_error(E_ERROR, "Error calling to_xml_before");
	}
	return data;
}
#endif

/* TODO: get rid of "bogus".. ither by passing in the already created xmlnode or passing in the node name */
/* String encode/decode */
zval *to_zval_string(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	if(data && data->children && data->children->content) {
		ZVAL_STRING(ret, data->children->content, 1);
	} else {
		ZVAL_EMPTY_STRING(ret);
	}
	return ret;
}

zval *to_zval_stringl(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	if(data && data->children && data->children->content) {
		ZVAL_STRINGL(ret, data->children->content, xmlStrlen(data->children->content), 1);
	} else {
		ZVAL_EMPTY_STRING(ret)
	}
	return ret;
}

xmlNodePtr to_xml_string(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;
	char *str, *pstr;
	int new_len;
	TSRMLS_FETCH();

	ret = xmlNewNode(NULL, "BOGUS");
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

	pstr = malloc(new_len + 1);
	memcpy(pstr, str, new_len);
	pstr[new_len] = '\0';
	efree(str);

	xmlNodeSetContentLen(ret, pstr, new_len);

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

xmlNodePtr to_xml_stringl(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;

	ret = xmlNewNode(NULL, "BOGUS");
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

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

zval *to_zval_double(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children && data->children->content) {
		ZVAL_DOUBLE(ret, atof(data->children->content));
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

zval *to_zval_long(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children && data->children->content) {
		ZVAL_LONG(ret, atol(data->children->content));
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

zval *to_zval_ulong(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children && data->children->content) {
	  /* TODO: long overflow */
		ZVAL_LONG(ret, atol(data->children->content));
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

xmlNodePtr to_xml_long(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;
  zval tmp;

	ret = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, ret, style);

	tmp = *data;
  zval_copy_ctor(&tmp);
  if (Z_TYPE(tmp) != IS_LONG) {
		convert_to_long(&tmp);
	}
	convert_to_string(&tmp);
	xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
	zval_dtor(&tmp);

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

xmlNodePtr to_xml_ulong(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;
  zval tmp;

	ret = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, ret, style);

	/* TODO: long overflow */
	tmp = *data;
  zval_copy_ctor(&tmp);
  if (Z_TYPE(tmp) != IS_LONG) {
		convert_to_long(&tmp);
	}
	convert_to_string(&tmp);
	xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
	zval_dtor(&tmp);

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

xmlNodePtr to_xml_double(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;
  zval tmp;

	ret = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, ret, style);

	tmp = *data;
  zval_copy_ctor(&tmp);
  if (Z_TYPE(tmp) != IS_DOUBLE) {
		convert_to_double(&tmp);
	}
	convert_to_string(&tmp);
	xmlNodeSetContentLen(ret, Z_STRVAL(tmp), Z_STRLEN(tmp));
	zval_dtor(&tmp);

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

zval *to_zval_bool(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children && data->children->content) {
		if(stricmp(data->children->content,"true") == 0 ||
			stricmp(data->children->content,"t") == 0 ||
			strcmp(data->children->content,"1") == 0)
		{
			ZVAL_BOOL(ret, 1);
		}
		else
		{
			ZVAL_BOOL(ret, 0);
		}
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

xmlNodePtr to_xml_bool(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;
	zval tmp;

	ret = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, ret, style);

	if (Z_TYPE_P(data) != IS_BOOL) {
	  tmp = *data;
	  zval_copy_ctor(&tmp);
		convert_to_boolean(data);
	  data = &tmp;
	}

	if(data->value.lval == 1)
		xmlNodeSetContent(ret, "1");
	else
		xmlNodeSetContent(ret, "0");

	if (data == &tmp) {
	  zval_dtor(&tmp);
	}

	if(style == SOAP_ENCODED)
		set_ns_and_type(ret, type);
	return ret;
}

/* Null encode/decode */
zval *to_zval_null(encodeType type, xmlNodePtr data)
{
	zval *ret;
	MAKE_STD_ZVAL(ret);
	ZVAL_NULL(ret);
	return ret;
}

xmlNodePtr to_xml_null(encodeType type, zval *data, int style)
{
	xmlNodePtr ret;

	ret = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, ret, style);

	if(style == SOAP_ENCODED)
		xmlSetProp(ret, "xsi:nil", "1");
	return ret;
}

/* Struct encode/decode */
zval *to_zval_object(encodeType type, xmlNodePtr data)
{
	zval *ret;
	xmlNodePtr trav;
	sdlPtr sdl;
	sdlTypePtr sdlType = NULL;
	TSRMLS_FETCH();

	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	sdl = SOAP_GLOBAL(sdl);
	if (sdl && type.sdl_type) {
	  sdlType = type.sdl_type;
	}

	object_init(ret);
	trav = data->children;

	while (trav != NULL) {
		if(trav->type == XML_ELEMENT_NODE) {
			sdlTypePtr *element;
			encodePtr enc = NULL;
			zval *tmpVal;

			xmlAttrPtr typeAttr = get_attribute(trav->properties,"type");
			if (typeAttr != NULL && typeAttr->children && typeAttr->children->content) {
				enc = get_encoder_from_prefix(sdl, trav, typeAttr->children->content);
			}
			if (enc == NULL && sdlType != NULL && sdlType->elements != NULL && trav->name != NULL &&
			    zend_hash_find(sdlType->elements, (char*)trav->name, strlen(trav->name)+1,(void **)&element) == SUCCESS) {
			  enc = (*element)->encode;
			}
			if (enc == NULL) {
				enc = get_conversion(UNKNOWN_TYPE);
			}

			tmpVal = master_to_zval(enc, trav);
#ifdef ZEND_ENGINE_2
			tmpVal->refcount--;
#endif
			add_property_zval(ret, (char *)trav->name, tmpVal);
		}
	  trav = trav->next;
	}
	return ret;
}

xmlNodePtr to_xml_object(encodeType type, zval *data, int style)
{
	xmlNodePtr xmlParam;
	HashTable *prop;
	int i;
	TSRMLS_FETCH();

	/* Special handling of class SoapVar */
	if(data && Z_TYPE_P(data) == IS_OBJECT && Z_OBJCE_P(data) == soap_var_class_entry)
	{
		zval **ztype, **zdata, **zns, **zstype, **zname, **znamens;
		encodePtr enc;

		if(zend_hash_find(Z_OBJPROP_P(data), "enc_type", sizeof("enc_type"), (void **)&ztype) == FAILURE)
			php_error(E_ERROR, "error encoding SoapVar");

		enc = get_conversion(Z_LVAL_P(*ztype));

		if(zend_hash_find(Z_OBJPROP_P(data), "enc_value", sizeof("enc_value"), (void **)&zdata) == FAILURE) {
			xmlParam = master_to_xml(enc, NULL, style);
		} else {
			xmlParam = master_to_xml(enc, *zdata, style);
		}

		if(zend_hash_find(Z_OBJPROP_P(data), "enc_stype", sizeof("enc_stype"), (void **)&zstype) == SUCCESS)
		{
			if(zend_hash_find(Z_OBJPROP_P(data), "enc_ns", sizeof("enc_ns"), (void **)&zns) == SUCCESS)
				set_ns_and_type_ex(xmlParam, Z_STRVAL_PP(zns), Z_STRVAL_PP(zstype));
			else
				set_ns_and_type_ex(xmlParam, NULL, Z_STRVAL_PP(zstype));
		}

		if(zend_hash_find(Z_OBJPROP_P(data), "enc_name", sizeof("enc_name"), (void **)&zname) == SUCCESS)
			xmlNodeSetName(xmlParam, Z_STRVAL_PP(zname));
		if(zend_hash_find(Z_OBJPROP_P(data), "enc_namens", sizeof("enc_namens"), (void **)&znamens) == SUCCESS)
		{
			smart_str *ns;
			xmlNsPtr nsp;

			ns = encode_new_ns();
			nsp = xmlNewNs(xmlParam, Z_STRVAL_PP(znamens), ns->c);
			xmlSetNs(xmlParam, nsp);
			smart_str_free(ns);
			efree(ns);
		}
	}
	else
	{
		xmlParam = xmlNewNode(NULL, "BOGUS");
		FIND_ZVAL_NULL(data, xmlParam, style);

		if(Z_TYPE_P(data) == IS_OBJECT)
		{
			prop = Z_OBJPROP_P(data);
			i = zend_hash_num_elements(prop);
			zend_hash_internal_pointer_reset(prop);

			for(;i > 0;i--)
			{
				xmlNodePtr property;
				encodePtr enc;
				zval **zprop;
				char *str_key;

				zend_hash_get_current_key(prop, &str_key, NULL, FALSE);
				zend_hash_get_current_data(prop, (void **)&zprop);

				enc = get_conversion((*zprop)->type);
				property = master_to_xml(enc, (*zprop), style);

				xmlNodeSetName(property, str_key);
				xmlAddChild(xmlParam, property);
				zend_hash_move_forward(prop);
			}
		}

		if(style == SOAP_ENCODED)
			set_ns_and_type(xmlParam, type);
	}
	return xmlParam;
}

/* Array encode/decode */
xmlNodePtr guess_array_map(encodeType type, zval *data, int style)
{
	encodePtr enc = NULL;
	TSRMLS_FETCH();

	if(data && Z_TYPE_P(data) == IS_ARRAY)
	{
		if(is_map(data))
			enc = get_conversion(APACHE_MAP);
		else
			enc = get_conversion(SOAP_ENC_ARRAY);
	}
	if(!enc)
		enc = get_conversion(IS_NULL);

	return master_to_xml(enc, data, style);
}

xmlNodePtr to_xml_array(encodeType type, zval *data, int style)
{
	smart_str array_type_and_size = {0}, array_type = {0};
	int i;
	xmlNodePtr xmlParam;
	TSRMLS_FETCH();

	xmlParam = xmlNewNode(NULL,"BOGUS");

	FIND_ZVAL_NULL(data, xmlParam, style);

	if(Z_TYPE_P(data) == IS_ARRAY)
	{
		i = zend_hash_num_elements(Z_ARRVAL_P(data));

		if(i > 0 && style == SOAP_ENCODED)
		{
			get_array_type(data, &array_type TSRMLS_CC);
			smart_str_append(&array_type_and_size, &array_type);
			smart_str_appendc(&array_type_and_size, '[');
			smart_str_append_long(&array_type_and_size, i);
			smart_str_appendc(&array_type_and_size, ']');
			smart_str_0(&array_type_and_size);

			xmlSetProp(xmlParam, "SOAP-ENC:arrayType", array_type_and_size.c);

			smart_str_free(&array_type_and_size);
			smart_str_free(&array_type);
		}

		zend_hash_internal_pointer_reset(data->value.ht);
		for(;i > 0;i--)
		{
			xmlNodePtr xparam;
			zval **zdata;
			encodePtr enc;
			zend_hash_get_current_data(data->value.ht, (void **)&zdata);

			enc = get_conversion((*zdata)->type);
			xparam = master_to_xml(enc, (*zdata), style);

			if(style == SOAP_LITERAL)
				xmlNodeSetName(xparam, enc->details.type_str);
			else
				xmlNodeSetName(xparam, "val");

			xmlAddChild(xmlParam, xparam);
			zend_hash_move_forward(data->value.ht);
		}
	}
	if(style == SOAP_ENCODED)
		set_ns_and_type(xmlParam, type);
	return xmlParam;
}

static xmlNodePtr add_zval_array_elements(zval* ret, 
                                          sdlPtr sdl,
                                          encodePtr enc,
                                          int dimension, 
                                          int* dims, 
                                          xmlNodePtr data)
{
	int i;
	if (dimension == 1) {
		for (i = 0; i < dims[0]; i++) {
			zval *tmpVal;
			encodePtr typeEnc = NULL;
			xmlAttrPtr type = get_attribute(data->properties,"type");
			if (type != NULL && type->children && type->children->content) {
				typeEnc = get_encoder_from_prefix(sdl, data, type->children->content);
			}
			if (typeEnc) {
				tmpVal = master_to_zval(typeEnc, data);
			} else {
				tmpVal = master_to_zval(enc, data);
			}
			zend_hash_next_index_insert(Z_ARRVAL_P(ret), &tmpVal, sizeof(zval*), NULL);	  
			if (data != NULL) {
			  do {
					data = data->next;
				} while (data != NULL && data->type != XML_ELEMENT_NODE);
			}
		}
	} else {
		for (i = 0; i < dims[0]; i++) {
			zval *tmpVal;
			MAKE_STD_ZVAL(tmpVal);
			array_init(tmpVal);
			data = add_zval_array_elements(tmpVal,sdl,enc,dimension-1,dims+1,data);
			zend_hash_next_index_insert(Z_ARRVAL_P(ret), &tmpVal, sizeof(zval*), NULL);	  
		}
	}
	return data;
}

zval *to_zval_array(encodeType type, xmlNodePtr data)
{
	zval *ret;
	xmlNodePtr trav;
	encodePtr enc = NULL;
	int dimension = 1;
	int* dims = NULL;
	xmlAttrPtr arrayTypeAttr;
	sdlPtr sdl;

	TSRMLS_FETCH();

	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);
	sdl = SOAP_GLOBAL(sdl);

	if (data &&
	    (arrayTypeAttr = get_attribute(data->properties,"arrayType")) && 
	    arrayTypeAttr->children && 
	    arrayTypeAttr->children->content) {
		char *type, *end, *ns;
		xmlNsPtr nsptr;

		parse_namespace(arrayTypeAttr->children->content, &type, &ns);
		nsptr = xmlSearchNs(arrayTypeAttr->doc, arrayTypeAttr->parent, ns);

		end = strrchr(type,'[');
		if (end) {
			int i;
		  char *tmp = end+1;
		  *end = '\0';
		  end++;
		  while (*tmp != ']' && *tmp != '\0') {
		    if (*tmp == ',') {
		      dimension++;
		    }
		    tmp++;
		  }
			dims = emalloc(sizeof(int)*dimension);
			memset(dims,0,sizeof(int)*dimension);
			tmp = end; i = 0;
		  while (*tmp != ']' && *tmp != '\0') {
		  	if (*tmp >= '0' && *tmp <= '9') {
		  		dims[i] = (dims[i]*10)+(*tmp-'0');
		    } else if (*tmp == ',') {
		      i++;
		    }
		    tmp++;
		  }
		}
		if(nsptr != NULL) {
			enc = get_encoder(SOAP_GLOBAL(sdl), nsptr->href, type);
		}
		efree(type);
		if (ns) efree(ns);
	}
	if (enc == NULL) {
 		enc = get_conversion(UNKNOWN_TYPE);
 	}

	array_init(ret);
	if (dims == NULL) {
		trav = data->children;
		while(trav) {
			if(trav->type == XML_ELEMENT_NODE) {
				zval *tmpVal;
				encodePtr typeEnc = NULL;
				xmlAttrPtr type = get_attribute(trav->properties,"type");
				if (type != NULL && type->children && type->children->content) {
					typeEnc = get_encoder_from_prefix(sdl, trav, type->children->content);
				}
				if (typeEnc) {
					tmpVal = master_to_zval(typeEnc, trav);
				} else {
					tmpVal = master_to_zval(enc, trav);
				}
				zend_hash_next_index_insert(Z_ARRVAL_P(ret), &tmpVal, sizeof(zval *), NULL);
			}
			trav = trav->next;
		}
	} else {
		trav = data->children;
		while (trav != NULL && trav->type != XML_ELEMENT_NODE) { 
		  trav = trav->next;
		}
		add_zval_array_elements(ret, sdl, enc, dimension, dims, trav);
	}
/*
	if (data) {
		if (dims == NULL && dimension <= 1) {
			trav = data->children;
			while(trav) {
				if(trav->type == XML_ELEMENT_NODE) {
					zval *tmpVal;
					tmpVal = master_to_zval(enc, trav);
					zend_hash_next_index_insert(Z_ARRVAL_P(ret), &tmpVal, sizeof(zval *), NULL);
				}
				trav = trav->next;
			}
		} else {
			trav = data->children;
			while (trav != NULL && trav->type != XML_ELEMENT_NODE) { 
			  trav = trav->next;
			}
			add_zval_array_elements(ret, enc, dimension, dims, trav);
		}
	}
*/
	if (dims) {
	  efree(dims);
	}
	return ret;
}

/* Map encode/decode */
xmlNodePtr to_xml_map(encodeType type, zval *data, int style)
{
	xmlNodePtr xmlParam;
	int i;
	TSRMLS_FETCH();

	xmlParam = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, xmlParam, style);

	if(Z_TYPE_P(data) == IS_ARRAY)
	{
		i = zend_hash_num_elements(Z_ARRVAL_P(data));
		/* TODO: Register namespace...??? */
		xmlSetProp(xmlParam, "xmlns:apache", "http://xml.apache.org/xml-soap");
		zend_hash_internal_pointer_reset(data->value.ht);
		for(;i > 0;i--)
		{
			xmlNodePtr xparam, item;
			xmlNodePtr key;
			zval **temp_data;
			char *key_val;
			int int_val;
			encodePtr enc;

			zend_hash_get_current_data(data->value.ht, (void **)&temp_data);
			if(Z_TYPE_PP(temp_data) != IS_NULL)
			{
				item = xmlNewNode(NULL, "item");
				key = xmlNewNode(NULL, "key");
				if(zend_hash_get_current_key(data->value.ht, &key_val, (long *)&int_val, FALSE) == HASH_KEY_IS_STRING)
				{
					if(style == SOAP_ENCODED)
						xmlSetProp(key, "xsi:type", "xsd:string");
					xmlNodeSetContent(key, key_val);
				}
				else
				{
					smart_str tmp = {0};
					smart_str_append_long(&tmp, int_val);
					smart_str_0(&tmp);

					if(style == SOAP_ENCODED)
						xmlSetProp(key, "xsi:type", "xsd:int");
					xmlNodeSetContentLen(key, tmp.c, tmp.len);

					smart_str_free(&tmp);
				}


				enc = get_conversion((*temp_data)->type);
				xparam = master_to_xml(enc, (*temp_data), style);

				xmlNodeSetName(xparam, "value");
				xmlAddChild(item, key);
				xmlAddChild(item, xparam);
				xmlAddChild(xmlParam, item);
			}
			zend_hash_move_forward(data->value.ht);
		}
	}
	if(style == SOAP_ENCODED)
		set_ns_and_type(xmlParam, type);

	return xmlParam;
}

zval *to_zval_map(encodeType type, xmlNodePtr data)
{
	zval *ret, *key, *value;
	xmlNodePtr trav, item, xmlKey, xmlValue;
	encodePtr enc;
	TSRMLS_FETCH();

	MAKE_STD_ZVAL(ret);
	FIND_XML_NULL(data, ret);

	if (data && data->children) {
		array_init(ret);
		trav = data->children;

		enc = get_conversion(UNKNOWN_TYPE);
		trav = data->children;
		FOREACHNODE(trav, "item", item)
		{
			xmlKey = get_node(item->children, "key");
			if(!xmlKey)
				php_error(E_ERROR, "Error encoding apache map, missing key");

			xmlValue = get_node(item->children, "value");
			if(!xmlKey)
				php_error(E_ERROR, "Error encoding apache map, missing value");

			key = master_to_zval(enc, xmlKey);
			value = master_to_zval(enc, xmlValue);

			if(Z_TYPE_P(key) == IS_STRING)
				zend_hash_update(Z_ARRVAL_P(ret), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &value, sizeof(zval *), NULL);
			else if(Z_TYPE_P(key) == IS_LONG)
				zend_hash_index_update(Z_ARRVAL_P(ret), Z_LVAL_P(key), &value, sizeof(zval *), NULL);
			else
				php_error(E_ERROR, "Error encoding apache map, only Strings or Longs are allowd as keys");
		}
		ENDFOREACH(trav);
	} else {
		ZVAL_NULL(ret);
	}
	return ret;
}

/* Unknown encode/decode */
xmlNodePtr guess_xml_convert(encodeType type, zval *data, int style)
{
	encodePtr enc;
	TSRMLS_FETCH();

	if(data)
		enc = get_conversion(data->type);
	else
		enc = get_conversion(IS_NULL);
	return master_to_xml(enc, data, style);
}

zval *guess_zval_convert(encodeType type, xmlNodePtr data)
{
	encodePtr enc = NULL;
	xmlAttrPtr tmpattr;
	TSRMLS_FETCH();

	data = check_and_resolve_href(data);

	if (data == NULL) {
		enc = get_conversion(IS_NULL);
	} else if (data->properties && get_attribute(data->properties, "nil")) {
		enc = get_conversion(IS_NULL);
	}
	else
	{
		tmpattr = get_attribute(data->properties,"type");
		if(tmpattr != NULL)
		{
		  if (tmpattr->children) {
				enc = get_conversion_from_type(data, tmpattr->children->content);
			} else {
				enc = get_conversion_from_type(data, "");
			}
		/*
			if(enc == NULL)
				php_error(E_ERROR, "Error (Don't know how to encode/decode \"%s\")", tmpattr->children->content);
		*/
		}

		if(enc == NULL)
		{
			/* Didn't have a type, totally guess here */
			/* Logic: has children = IS_OBJECT else IS_STRING */
			xmlNodePtr trav;

			if(get_attribute(data->properties, "arrayType"))
				enc = get_conversion(SOAP_ENC_ARRAY);
			else
			{
				enc = get_conversion(XSD_STRING);
				trav = data->children;
				while (trav != NULL) {
					if(trav->type == XML_ELEMENT_NODE)
					{
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
xmlNodePtr to_xml_datetime_ex(encodeType type, zval *data, char *format, int style)
{
	/* logic hacked from ext/standard/datetime.c */
	struct tm *ta, tmbuf;
	time_t timestamp;
	int max_reallocs = 5;
	size_t buf_len=64, real_len;
	char *buf;
	xmlNodePtr xmlParam;

	xmlParam = xmlNewNode(NULL, "BOGUS");
	FIND_ZVAL_NULL(data, xmlParam, style);

	if (Z_TYPE_P(data) == IS_LONG) {
		timestamp = Z_LVAL_P(data);
		/*time(&timestamp);*/
		ta = php_localtime_r(&timestamp, &tmbuf);

		buf = (char *) emalloc(buf_len);
		while ((real_len = strftime(buf, buf_len, format, ta)) == buf_len || real_len == 0)
		{
			buf_len *= 2;
			buf = (char *) erealloc(buf, buf_len);
			if(!--max_reallocs) break;
		}

		xmlNodeSetContent(xmlParam, buf);
		efree(buf);
	} else if (Z_TYPE_P(data) == IS_STRING) {
		buf = malloc(Z_STRLEN_P(data)+1);
		strcpy(buf, Z_STRVAL_P(data));

		xmlNodeSetContentLen(xmlParam, buf, Z_STRLEN_P(data));
	} else {
		xmlSetProp(xmlParam, "xsi:nil", "1");
	}

	if(style == SOAP_ENCODED)
		set_ns_and_type(xmlParam, type);
	return xmlParam;
}

xmlNodePtr to_xml_duration(encodeType type, zval *data, int style)
{
	/* TODO: '-'?P([0-9]+Y)?([0-9]+M)?([0-9]+D)?T([0-9]+H)?([0-9]+M)?([0-9]+S)? */
	return to_xml_string(type, data, style);
}

xmlNodePtr to_xml_datetime(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "%Y-%m-%dT%H:%M:%S", style);
}

xmlNodePtr to_xml_time(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	/* TODO: microsecconds */
	return to_xml_datetime_ex(type, data, "%H:%M:%S", style);
}

xmlNodePtr to_xml_date(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "%Y-%m-%d", style);
}

xmlNodePtr to_xml_gyearmonth(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "%Y-%m", style);
}

xmlNodePtr to_xml_gyear(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "%Y", style);
}

xmlNodePtr to_xml_gmonthday(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "--%m-%d", style);
}

xmlNodePtr to_xml_gday(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "---%d", style);
}

xmlNodePtr to_xml_gmonth(encodeType type, zval *data, int style)
{
	/* TODO: time zone */
	return to_xml_datetime_ex(type, data, "--%m--", style);
}

void set_ns_and_type(xmlNodePtr node, encodeType type)
{
	set_ns_and_type_ex(node, type.ns, type.type_str);
}

void set_ns_and_type_ex(xmlNodePtr node, char *ns, char *type)
{
	if(ns != NULL)
	{
		char *sprefix;
		smart_str *prefix;
		smart_str xmlns = {0}, nstype = {0};

		TSRMLS_FETCH();

		if(zend_hash_find(SOAP_GLOBAL(defEncNs), ns, strlen(ns) + 1, (void **)&sprefix) == FAILURE)
		{
			prefix = encode_new_ns();
			smart_str_appendl(&xmlns, "xmlns:", 6);
			smart_str_append(&xmlns, prefix);
			smart_str_0(&xmlns);

			xmlSetProp(node, xmlns.c, ns);
		}
		else
		{
			prefix = emalloc(sizeof(smart_str));
			memset(prefix, 0, sizeof(smart_str));
			smart_str_appends(prefix, sprefix);
		}

		smart_str_append(&nstype, prefix);
		smart_str_appendc(&nstype, ':');
		smart_str_appends(&nstype, type);
		smart_str_0(&nstype);
		xmlSetProp(node, "xsi:type", nstype.c);
		smart_str_free(&nstype);
		smart_str_free(&xmlns);
		smart_str_free(prefix);
		efree(prefix);
	}
	else
		xmlSetProp(node, "xsi:type", type);
}

smart_str *encode_new_ns()
{
	int num;
	smart_str *ns = emalloc(sizeof(smart_str));

	TSRMLS_FETCH();

	memset(ns, 0, sizeof(smart_str));
	num = ++SOAP_GLOBAL(cur_uniq_ns);
	smart_str_appendl(ns, "ns", 2);
	smart_str_append_long(ns, num);
	smart_str_0(ns);
	return ns;
}

void encode_reset_ns()
{
	TSRMLS_FETCH();
	SOAP_GLOBAL(cur_uniq_ns) = 0;
}

encodePtr get_conversion_ex(HashTable *encoding, int encode)
{
	encodePtr *enc;
	TSRMLS_FETCH();

	if(zend_hash_index_find(encoding, encode, (void **)&enc) == FAILURE)
		php_error(E_ERROR, "Cannot find encoding");

	if(SOAP_GLOBAL(overrides))
	{
		smart_str nscat = {0};

		smart_str_appendl(&nscat, (*enc)->details.ns, strlen((*enc)->details.ns));
		smart_str_appendc(&nscat, ':');
		smart_str_appendl(&nscat, (*enc)->details.type_str, strlen((*enc)->details.type_str));
		smart_str_0(&nscat);

		zend_hash_find(SOAP_GLOBAL(overrides), nscat.c, nscat.len + 1, (void **)&enc);
		smart_str_free(&nscat);
	}

	return *enc;
}

encodePtr get_conversion_from_href_type_ex(HashTable *encoding, const char *type, int len)
{
	encodePtr *enc = NULL;

	if(encoding == NULL)
		return NULL;

	if(zend_hash_find(encoding, (char*)type, len + 1, (void **)&enc) == FAILURE)
		return NULL;

	return (*enc);
}

encodePtr get_conversion_from_type_ex(HashTable *encoding, xmlNodePtr node, const char *type)
{
	encodePtr *enc = NULL;
	xmlNsPtr nsptr;
	char *ns, *cptype;
	smart_str nscat = {0};

	if(encoding == NULL)
		return NULL;

	parse_namespace(type, &cptype, &ns);
	nsptr = xmlSearchNs(node->doc, node, ns);
	if(nsptr != NULL)
	{
		smart_str_appends(&nscat, nsptr->href);
		smart_str_appendc(&nscat, ':');
		smart_str_appends(&nscat, cptype);
		smart_str_0(&nscat);

		if(zend_hash_find(encoding, nscat.c, nscat.len + 1, (void **)&enc) == FAILURE)
		{
			if(zend_hash_find(encoding, (char*)type, strlen(type) + 1, (void **)&enc) == FAILURE)
				enc = NULL;
		}
		smart_str_free(&nscat);
	}
	else
	{
		if(zend_hash_find(encoding, (char*)type, strlen(type) + 1, (void **)&enc) == FAILURE)
			enc = NULL;
	}

	if(cptype) efree(cptype);
	if(ns) efree(ns);
	if(enc == NULL)
		return NULL;
	else
		return (*enc);
}

int is_map(zval *array)
{
	int i, count = zend_hash_num_elements(Z_ARRVAL_P(array));
	for(i = 0;i < count;i++)
	{
		if(zend_hash_get_current_key_type(Z_ARRVAL_P(array)) == HASH_KEY_IS_STRING)
			return TRUE;
		zend_hash_move_forward(Z_ARRVAL_P(array));
	}
	return FALSE;
}

void get_array_type(zval *array, smart_str *type TSRMLS_DC)
{
	HashTable *ht = HASH_OF(array);
	int i, count, cur_type, prev_type, different;
	zval **tmp;
/*	TSRMLS_FETCH();*/

	if(!array || Z_TYPE_P(array) != IS_ARRAY)
		smart_str_appendl(type, "xsd:anyType", 11);

	different = FALSE;
	cur_type = prev_type = 0;
	count = zend_hash_num_elements(ht);

	zend_hash_internal_pointer_reset(ht);
	for(i = 0;i < count;i++)
	{
		zend_hash_get_current_data(ht, (void **)&tmp);

		if(Z_TYPE_PP(tmp) == IS_OBJECT && Z_OBJCE_PP(tmp) == soap_var_class_entry)
		{
			zval **ztype;

			if(zend_hash_find(Z_OBJPROP_PP(tmp), "enc_type", sizeof("enc_type"), (void **)&ztype) == FAILURE)
				php_error(E_ERROR, "error encoding SoapVar");
			cur_type = Z_LVAL_P(*ztype);
		}
		else if(Z_TYPE_PP(tmp) == IS_ARRAY && is_map(*tmp))
			cur_type = APACHE_MAP;
		else
			cur_type = Z_TYPE_PP(tmp);

		if(i > 0)
		{
			if(cur_type != prev_type)
			{
				different = TRUE;
				break;
			}
		}

		prev_type = cur_type;
		zend_hash_move_forward(ht);
	}

	if(different)
		smart_str_appendl(type, "xsd:anyType", 11);
	else
	{
		encodePtr enc;
		char *prefix;

		enc = get_conversion(cur_type);

		if(enc->details.ns != NULL)
		{
			if(zend_hash_find(SOAP_GLOBAL(defEncNs), enc->details.ns, strlen(enc->details.ns) + 1, (void **)&prefix) == FAILURE)
				php_error(E_ERROR, "fix me");

			smart_str_appendl(type, prefix, strlen(prefix));
			smart_str_appendc(type, ':');
			smart_str_appendl(type, enc->details.type_str, strlen(enc->details.type_str));
			smart_str_0(type);
		}
		else
			smart_str_appendl(type, enc->details.type_str, strlen(enc->details.type_str));
	}
}


smart_str *build_soap_action(zval *this_ptr, char *soapaction)
{
	zval **uri;
	smart_str *tmp;

	tmp = emalloc(sizeof(smart_str));
	memset(tmp, 0, sizeof(smart_str));

	if(zend_hash_find(Z_OBJPROP_P(this_ptr), "uri", sizeof("uri"), (void *)&uri) == FAILURE)
		php_error(E_ERROR, "Error finding uri");

	smart_str_appendl(tmp, Z_STRVAL_PP(uri), Z_STRLEN_PP(uri));
	smart_str_appends(tmp, "#");
	smart_str_appendl(tmp, soapaction, strlen(soapaction));
	smart_str_0(tmp);

	return tmp;
}

void delete_encoder(void *encode)
{
	encodePtr t = *((encodePtr*)encode);
	if(t->details.ns)
		free(t->details.ns);
	if(t->details.type_str)
		free(t->details.type_str);
	if(t->details.map)
		delete_mapping(t->details.map);
	free(t);
}
