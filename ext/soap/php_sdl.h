#ifndef PHP_SDL_H
#define PHP_SDL_H

#define XSD_WHITESPACE_COLLAPSE 1
#define XSD_WHITESPACE_PRESERVE 1
#define XSD_WHITESPACE_REPLACE 1

#define BINDING_SOAP 1
#define BINDING_HTTP 2

#define SOAP_RPC 1
#define SOAP_DOCUMENT 2

#define SOAP_ENCODED 1
#define SOAP_LITERAL 2

struct _sdl {
	HashTable  docs;             /* pointer to the parsed xml file */
	HashTable  functions;        /* array of sdlFunction */
	HashTable *types;            /* array of sdlTypesPtr */
	HashTable *elements;         /* array of sdlTypesPtr */
	HashTable *encoders;         /* array of encodePtr */
	HashTable *bindings;         /* array of sdlBindings (key'd by name) */
	HashTable *requests;         /* array of sdlFunction (references) */
	HashTable *attributes;       /* array of sdlAttributePtr */
	HashTable *attributeGroups;  /* array of sdlTypesPtr */
	HashTable *groups;           /* array of sdlTypesPtr */
	char      *target_ns;
	char      *source;
};

struct _sdlBinding {
	char *name;
	char *location;
	int   bindingType;
	void *bindingAttributes; /* sdlSoapBindingPtr */
};

/* Soap Binding Specfic stuff */
struct _sdlSoapBinding {
	char *transport;
	int   style;
};

struct _sdlSoapBindingFunctionBody {
	char *ns;
	int   use;
	char *parts;         /* not implemented yet */
	char *encodingStyle; /* not implemented yet */
};

struct _sdlSoapBindingFunction {
	char *soapAction;
	int   style;

	sdlSoapBindingFunctionBody input;
	sdlSoapBindingFunctionBody output;
	sdlSoapBindingFunctionBody falut;
};

struct _sdlRestrictionInt {
	int   value;
	char  fixed;
};

struct _sdlRestrictionChar {
	char *value;
	char  fixed;
};

struct _sdlRestrictions {
	HashTable *enumeration;              /* array of sdlRestrictionCharPtr */
	sdlRestrictionIntPtr minExclusive;
	sdlRestrictionIntPtr minInclusive;
	sdlRestrictionIntPtr maxExclusive;
	sdlRestrictionIntPtr maxInclusive;
	sdlRestrictionIntPtr totalDigits;
	sdlRestrictionIntPtr fractionDigits;
	sdlRestrictionIntPtr length;
	sdlRestrictionIntPtr minLength;
	sdlRestrictionIntPtr maxLength;
	sdlRestrictionCharPtr whiteSpace;
	sdlRestrictionCharPtr pattern;
};

typedef enum _sdlContentKind {
	XSD_CONTENT_ELEMENT,
	XSD_CONTENT_SEQUENCE,
	XSD_CONTENT_ALL,
	XSD_CONTENT_CHOICE,
	XSD_CONTENT_GROUP_REF,
	XSD_CONTENT_GROUP
} sdlContentKind;


typedef struct _sdlContentModel sdlContentModel, *sdlContentModelPtr;

struct _sdlContentModel {
	sdlContentKind kind;
	int min_occurs;
	int max_occurs;
	union {
		sdlTypePtr          element;      /* pointer to element */
		sdlContentModelPtr  group;        /* pointer to group */
		HashTable          *content;      /* array of sdlContentModel for sequnce,all,choice*/
		char               *group_ref;    /* reference to group */
	} u;
};

typedef enum _sdlTypeKind {
	XSD_TYPEKIND_SIMPLE,
	XSD_TYPEKIND_LIST,
	XSD_TYPEKIND_UNION,
	XSD_TYPEKIND_COMPLEX,
	XSD_TYPEKIND_RESTRICTION,
	XSD_TYPEKIND_EXTENSION
} sdlTypeKind;

struct _sdlType {
	sdlTypeKind         kind;
	char               *name;
	char               *namens;
	int                 nillable;
	HashTable          *elements;             /* array of sdlTypePtr */
	HashTable          *attributes;           /* array of sdlAttributePtr */
	sdlRestrictionsPtr  restrictions;
	encodePtr           encode;
	sdlContentModelPtr  model;
	char               *def;
	char               *fixed;
	char               *ref;
};

struct _sdlParam {
	int        order;
	encodePtr  encode;
	char      *paramName;
};

struct _sdlFunction {
	char               *functionName;
	char               *requestName;
	char               *responseName;
	HashTable          *requestParameters;  /* array of sdlParamPtr */
	HashTable          *responseParameters; /* array of sdlParamPtr (this should only be one) */
	struct _sdlBinding *binding;
	void               *bindingAttributes;  /* sdlSoapBindingFunctionPtr */
};

typedef enum _sdlUse {
	XSD_USE_DEFAULT,
	XSD_USE_OPTIONAL,
	XSD_USE_PROHIBITED,
	XSD_USE_REQUIRED
} sdlUse;

typedef enum _sdlForm {
	XSD_FORM_DEFAULT,
	XSD_FORM_QUALIFIED,
	XSD_FORM_UNQUALIFIED
} sdlForm;


struct _sdlAttribute {
	char      *name;
	char      *ref;
	char      *def;
	char      *fixed;
	sdlForm    form;
	sdlUse     use;
	HashTable *extraAttributes;			/* array of xmlNodePtr */
	encodePtr  encode;
};

sdlPtr get_sdl(char *uri);

encodePtr get_encoder_from_prefix(sdlPtr sdl, xmlNodePtr data, const char *type);
encodePtr get_encoder(sdlPtr sdl, const char *ns, const char *type);
encodePtr get_encoder_ex(sdlPtr sdl, const char *nscat);

sdlBindingPtr get_binding_from_type(sdlPtr sdl, int type);
sdlBindingPtr get_binding_from_name(sdlPtr sdl, char *name, char *ns);

void delete_sdl(void *handle);

#endif
