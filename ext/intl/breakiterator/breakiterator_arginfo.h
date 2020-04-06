/* This is a generated file, edit the .stub.php file instead. */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_createCharacterInstance, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, locale, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_createCodePointInstance, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_IntlBreakIterator_createLineInstance arginfo_class_IntlBreakIterator_createCharacterInstance

#define arginfo_class_IntlBreakIterator_createSentenceInstance arginfo_class_IntlBreakIterator_createCharacterInstance

#define arginfo_class_IntlBreakIterator_createTitleInstance arginfo_class_IntlBreakIterator_createCharacterInstance

#define arginfo_class_IntlBreakIterator_createWordInstance arginfo_class_IntlBreakIterator_createCharacterInstance

#define arginfo_class_IntlBreakIterator___construct arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlBreakIterator_current arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlBreakIterator_first arginfo_class_IntlBreakIterator_createCodePointInstance

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_following, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_IntlBreakIterator_getErrorCode arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlBreakIterator_getErrorMessage arginfo_class_IntlBreakIterator_createCodePointInstance

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_getLocale, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, locale_type, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_getPartsIterator, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key_type, IS_STRING, 0, "IntlPartsIterator::KEY_SEQUENTIAL")
ZEND_END_ARG_INFO()

#define arginfo_class_IntlBreakIterator_getText arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlBreakIterator_isBoundary arginfo_class_IntlBreakIterator_following

#define arginfo_class_IntlBreakIterator_last arginfo_class_IntlBreakIterator_createCodePointInstance

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_next, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, offset, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_IntlBreakIterator_preceding arginfo_class_IntlBreakIterator_following

#define arginfo_class_IntlBreakIterator_previous arginfo_class_IntlBreakIterator_createCodePointInstance

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlBreakIterator_setText, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_IntlRuleBasedBreakIterator___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, rules, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, areCompiled, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

#define arginfo_class_IntlRuleBasedBreakIterator_getBinaryRules arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlRuleBasedBreakIterator_getRules arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlRuleBasedBreakIterator_getRuleStatus arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlRuleBasedBreakIterator_getRuleStatusVec arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlPartsIterator_getBreakIterator arginfo_class_IntlBreakIterator_createCodePointInstance

#define arginfo_class_IntlCodePointBreakIterator_getLastCodePoint arginfo_class_IntlBreakIterator_createCodePointInstance
