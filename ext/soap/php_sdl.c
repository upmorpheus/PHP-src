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

#include "php_soap.h"
#include "libxml/uri.h"

#include "ext/standard/md5.h"
#include "tsrm_virtual_cwd.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

static void delete_binding(void *binding);
static void delete_function(void *function);
static void delete_parameter(void *paramater);
static void delete_header(void *header);
static void delete_document(void *doc_ptr);

encodePtr get_encoder_from_prefix(sdlPtr sdl, xmlNodePtr data, const char *type)
{
	encodePtr enc = NULL;
	TSRMLS_FETCH();

	enc = get_conversion_from_type(data, type);
	if (enc == NULL && sdl) {
		enc = get_conversion_from_type_ex(sdl->encoders, data, type);
	}
	return enc;
}

static sdlTypePtr get_element(sdlPtr sdl, xmlNodePtr node, const char *type)
{
	sdlTypePtr ret = NULL;
	TSRMLS_FETCH();

	if (sdl->elements) {
		xmlNsPtr nsptr;
		char *ns, *cptype;
		sdlTypePtr *sdl_type;

		parse_namespace(type, &cptype, &ns);
		nsptr = xmlSearchNs(node->doc, node, ns);
		if (nsptr != NULL) {
			smart_str nscat = {0};

			smart_str_appends(&nscat, nsptr->href);
			smart_str_appendc(&nscat, ':');
			smart_str_appends(&nscat, cptype);
			smart_str_0(&nscat);

			if (zend_hash_find(sdl->elements, nscat.c, nscat.len + 1, (void **)&sdl_type) == SUCCESS) {
				ret = *sdl_type;
			} else if (zend_hash_find(sdl->elements, (char*)type, strlen(type) + 1, (void **)&sdl_type) == SUCCESS) {
				ret = *sdl_type;
			}
			smart_str_free(&nscat);
		} else {
			if (zend_hash_find(sdl->elements, (char*)type, strlen(type) + 1, (void **)&sdl_type) == SUCCESS) {
				ret = *sdl_type;
			}
		}

		efree(cptype);
		if (ns) {efree(ns);}
	}
	return ret;
}

encodePtr get_encoder(sdlPtr sdl, const char *ns, const char *type)
{
	encodePtr enc = NULL;
	char *nscat;

	nscat = emalloc(strlen(ns) + strlen(type) + 2);
	sprintf(nscat, "%s:%s", ns, type);

	enc = get_encoder_ex(sdl, nscat);

	efree(nscat);
	return enc;
}

encodePtr get_encoder_ex(sdlPtr sdl, const char *nscat)
{
	encodePtr enc = NULL;
	TSRMLS_FETCH();

	enc = get_conversion_from_href_type(nscat);
	if (enc == NULL && sdl) {
		enc = get_conversion_from_href_type_ex(sdl->encoders, nscat, strlen(nscat));
	}
	return enc;
}

sdlBindingPtr get_binding_from_type(sdlPtr sdl, int type)
{
	sdlBindingPtr *binding;

	if (sdl == NULL) {
		return NULL;
	}

	for (zend_hash_internal_pointer_reset(sdl->bindings);
		zend_hash_get_current_data(sdl->bindings, (void **) &binding) == SUCCESS;
		zend_hash_move_forward(sdl->bindings)) {
		if ((*binding)->bindingType == type) {
			return *binding;
		}
	}
	return NULL;
}

sdlBindingPtr get_binding_from_name(sdlPtr sdl, char *name, char *ns)
{
	sdlBindingPtr binding = NULL;
	smart_str key = {0};

	smart_str_appends(&key, ns);
	smart_str_appendc(&key, ':');
	smart_str_appends(&key, name);
	smart_str_0(&key);

	zend_hash_find(sdl->bindings, key.c, key.len, (void **)&binding);

	smart_str_free(&key);
	return binding;
}

static void load_wsdl_ex(char *struri, sdlCtx *ctx, int include)
{
	sdlPtr tmpsdl = ctx->sdl;
	xmlDocPtr wsdl;
	xmlNodePtr root, definitions, trav;
	xmlAttrPtr targetNamespace;

	if (zend_hash_exists(&ctx->docs, struri, strlen(struri)+1)) {
	  return;
	}

	wsdl = soap_xmlParseFile(struri);

	if (!wsdl) {
		php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Couldn't load from '%s'", struri);
	}

	zend_hash_add(&ctx->docs, struri, strlen(struri)+1, (void**)&wsdl, sizeof(xmlDocPtr), NULL);

	root = wsdl->children;
	definitions = get_node(root, "definitions");
	if (!definitions) {
		if (include) {
			xmlNodePtr schema = get_node(root, "schema");
			if (schema) {
				load_schema(ctx, schema);
				return;
			}
		}
		php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Couldn't find <definitions> in '%s'", struri);
	}

	if (!include) {
		targetNamespace = get_attribute(definitions->properties, "targetNamespace");
		if (targetNamespace) {
			tmpsdl->target_ns = sdl_strdup(targetNamespace->children->content);
		}
	}

	trav = definitions->children;
	while (trav != NULL) {
		if (node_is_equal(trav,"types")) {
			/* TODO: Only one "types" is allowed */
			xmlNodePtr trav2 = trav->children;
			xmlNodePtr schema;

			FOREACHNODE(trav2, "schema", schema) {
				load_schema(ctx, schema);
			}
			ENDFOREACH(trav2);

		} else if (node_is_equal(trav,"import")) {
			/* TODO: namespace ??? */
			xmlAttrPtr tmp = get_attribute(trav->properties, "location");
			if (tmp) {
			  xmlChar *uri;
				xmlChar *base = xmlNodeGetBase(trav->doc, trav);

				if (base == NULL) {
			    uri = xmlBuildURI(tmp->children->content, trav->doc->URL);
				} else {
    			uri = xmlBuildURI(tmp->children->content, base);
			    xmlFree(base);
				}
				load_wsdl_ex(uri, ctx, 1);
		    xmlFree(uri);
			}

		} else if (node_is_equal(trav,"message")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_add(&ctx->messages, name->children->content, strlen(name->children->content)+1,&trav, sizeof(xmlNodePtr), NULL) != SUCCESS) {
					php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <message> '%s' already defined",name->children->content);
				}
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <message> hasn't name attribute");
			}

		} else if (node_is_equal(trav,"portType")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_add(&ctx->portTypes, name->children->content, strlen(name->children->content)+1,&trav, sizeof(xmlNodePtr), NULL) != SUCCESS) {
					php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <portType> '%s' already defined",name->children->content);
				}
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <portType> hasn't name attribute");
			}

		} else if (node_is_equal(trav,"binding")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_add(&ctx->bindings, name->children->content, strlen(name->children->content)+1,&trav, sizeof(xmlNodePtr), NULL) != SUCCESS) {
					php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <binding> '%s' already defined",name->children->content);
				}
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <binding> hasn't name attribute");
			}

		} else if (node_is_equal(trav,"service")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_add(&ctx->services, name->children->content, strlen(name->children->content)+1,&trav, sizeof(xmlNodePtr), NULL) != SUCCESS) {
					php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <service> '%s' already defined",name->children->content);
				}
			} else {
				php_error(E_ERROR,"SOAP-ERROR: Parsing WSDL: <service> hasn't name attribute");
			}
		}
		trav = trav->next;
	}
}

static void wsdl_soap_binding_body(sdlCtx* ctx, xmlNodePtr node, char* wsdl_soap_namespace, sdlSoapBindingFunctionBody *binding)
{
	xmlNodePtr body, trav, header;
	xmlAttrPtr tmp;

	body = get_node_ex(node->children, "body", wsdl_soap_namespace);
	if (body) {
		tmp = get_attribute(body->properties, "use");
		if (tmp && !strncmp(tmp->children->content, "literal", sizeof("literal"))) {
			binding->use = SOAP_LITERAL;
		} else {
			binding->use = SOAP_ENCODED;
		}

		tmp = get_attribute(body->properties, "namespace");
		if (tmp) {
			binding->ns = sdl_strdup(tmp->children->content);
		}

		tmp = get_attribute(body->properties, "parts");
		if (tmp) {
			binding->parts = sdl_strdup(tmp->children->content);
		}

		if (binding->use == SOAP_ENCODED) {
			tmp = get_attribute(body->properties, "encodingStyle");
			if (tmp &&
			    strncmp(tmp->children->content,SOAP_1_1_ENC_NAMESPACE,sizeof(SOAP_1_1_ENC_NAMESPACE)) != 0 &&
		  	  strncmp(tmp->children->content,SOAP_1_2_ENC_NAMESPACE,sizeof(SOAP_1_2_ENC_NAMESPACE)) != 0) {
				php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Unknown encodingStyle '%s'",tmp->children->content);
			} else if (tmp == NULL) {
				php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Unspecified encodingStyle");
			} else {
				binding->encodingStyle = sdl_strdup(tmp->children->content);
			}
		}
	}

	/* Process <soap:header> elements */
	trav = node->children;
	FOREACHNODEEX(trav, "header", wsdl_soap_namespace, header) {
		xmlAttrPtr tmp;
		xmlNodePtr *message, part;
		char *ctype;
		sdlSoapBindingFunctionHeaderPtr h;
		smart_str key = {0};

		tmp = get_attribute(header->properties, "message");
		if (!tmp) {
			php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing message attribute for <header>");
		}

		ctype = strrchr(tmp->children->content,':');
		if (ctype == NULL) {
			ctype = tmp->children->content;
		} else {
		  ++ctype;
		}
		if (zend_hash_find(&ctx->messages, ctype, strlen(ctype)+1, (void**)&message) != SUCCESS) {
			php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing <message> with name '%s'", tmp->children->content);
		}

		tmp = get_attribute(header->properties, "part");
		if (!tmp) {
			php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing part attribute for <header>");
		}
		part = get_node_with_attribute((*message)->children, "part", "name", tmp->children->content);
		if (!part) {
			php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing part '%s' in <message>",tmp->children->content);
		}

		h = sdl_malloc(sizeof(sdlSoapBindingFunctionHeader));
		memset(h, 0, sizeof(sdlSoapBindingFunctionHeader));
		h->name = sdl_strdup(tmp->children->content);

		tmp = get_attribute(part->properties, "type");
		if (tmp != NULL) {
			h->encode = get_encoder_from_prefix(ctx->sdl, part, tmp->children->content);
		} else {
			tmp = get_attribute(part->properties, "element");
			if (tmp != NULL) {
				h->element = get_element(ctx->sdl, part, tmp->children->content);
				if (h->element) {
					h->encode = h->element->encode;
				}
			}
		}

		tmp = get_attribute(header->properties, "use");
		if (tmp && !strncmp(tmp->children->content, "encoded", sizeof("encoded"))) {
			h->use = SOAP_ENCODED;
		} else {
			h->use = SOAP_LITERAL;
		}

		tmp = get_attribute(header->properties, "namespace");
		if (tmp) {
			h->ns = sdl_strdup(tmp->children->content);
		}

		if (h->use == SOAP_ENCODED) {
			tmp = get_attribute(header->properties, "encodingStyle");
			if (tmp &&
			    strncmp(tmp->children->content,SOAP_1_1_ENC_NAMESPACE,sizeof(SOAP_1_1_ENC_NAMESPACE)) != 0 &&
			    strncmp(tmp->children->content,SOAP_1_2_ENC_NAMESPACE,sizeof(SOAP_1_2_ENC_NAMESPACE)) != 0) {
				php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Unknown encodingStyle '%s'",tmp->children->content);
			} else if (tmp == NULL) {
				php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Unspecified encodingStyle");
			} else {
				h->encodingStyle = sdl_strdup(tmp->children->content);
			}
		}

		if (binding->headers == NULL) {
			binding->headers = sdl_malloc(sizeof(HashTable));
			zend_hash_init(binding->headers, 0, NULL, delete_header, SDL_PERSISTENT);
		}

		if (h->ns) {
			smart_str_appends(&key,h->ns);
			smart_str_appendc(&key,':');
		}
		smart_str_appends(&key,h->name);
		smart_str_0(&key);
		if (zend_hash_add(binding->headers, key.c, key.len+1, (void**)&h, sizeof(sdlSoapBindingFunctionHeaderPtr), NULL) != SUCCESS) {
			delete_header((void**)&h);
		}
		smart_str_free(&key);

	}
	ENDFOREACH(trav);
}

static HashTable* wsdl_message(sdlCtx *ctx, char* message_name)
{
	xmlNodePtr trav, part, message = NULL, *tmp;
	HashTable* parameters = NULL;
	char *ctype;

	ctype = strrchr(message_name,':');
	if (ctype == NULL) {
		ctype = message_name;
	} else {
	  ++ctype;
	}
	if (zend_hash_find(&ctx->messages, ctype, strlen(ctype)+1, (void**)&tmp) != SUCCESS) {
		php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing <message> with name '%s'", message->children->content);
	}
	message = *tmp;

	parameters = sdl_malloc(sizeof(HashTable));
	zend_hash_init(parameters, 0, NULL, delete_parameter, SDL_PERSISTENT);

	trav = message->children;
	FOREACHNODE(trav, "part", part) {
		xmlAttrPtr element, type, name;
		sdlParamPtr param;

		param = sdl_malloc(sizeof(sdlParam));
		memset(param,0,sizeof(sdlParam));
		param->order = 0;

		name = get_attribute(part->properties, "name");
		if (name == NULL) {
			php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: No name associated with <part> '%s'", message->name);
		}

		param->paramName = sdl_strdup(name->children->content);

		type = get_attribute(part->properties, "type");
		if (type != NULL) {
			param->encode = get_encoder_from_prefix(ctx->sdl, part, type->children->content);
		} else {
			element = get_attribute(part->properties, "element");
			if (element != NULL) {
				param->element = get_element(ctx->sdl, part, element->children->content);
				if (param->element) {
					param->encode = param->element->encode;
				}
			}
		}

		zend_hash_next_index_insert(parameters, &param, sizeof(sdlParamPtr), NULL);
	}
	ENDFOREACH(trav);
	return parameters;
}

static sdlPtr load_wsdl(char *struri)
{
	sdlCtx ctx;
	int i,n;

	memset(&ctx,0,sizeof(ctx));
	ctx.sdl = sdl_malloc(sizeof(sdl));
	memset(ctx.sdl, 0, sizeof(sdl));
	ctx.sdl->source = sdl_strdup(struri);
	zend_hash_init(&ctx.sdl->functions, 0, NULL, delete_function, SDL_PERSISTENT);

	zend_hash_init(&ctx.docs, 0, NULL, delete_document, 0);
	zend_hash_init(&ctx.messages, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.bindings, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.portTypes, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.services,  0, NULL, NULL, 0);

	load_wsdl_ex(struri,&ctx, 0);
	schema_pass2(&ctx);

	n = zend_hash_num_elements(&ctx.services);
	if (n > 0) {
		zend_hash_internal_pointer_reset(&ctx.services);
		for (i = 0; i < n; i++) {
			xmlNodePtr *tmp, service;
			xmlNodePtr trav, port;

			zend_hash_get_current_data(&ctx.services, (void **)&tmp);
			service = *tmp;

			trav = service->children;
			FOREACHNODE(trav, "port", port) {
				xmlAttrPtr type, name, bindingAttr, location;
				xmlNodePtr portType, operation;
				xmlNodePtr address, binding, trav2;
				char *ctype;
				sdlBindingPtr tmpbinding;
				char *wsdl_soap_namespace = NULL;

				tmpbinding = sdl_malloc(sizeof(sdlBinding));
				memset(tmpbinding, 0, sizeof(sdlBinding));

				bindingAttr = get_attribute(port->properties, "binding");
				if (bindingAttr == NULL) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: No binding associated with <port>");
				}

				/* find address and figure out binding type */
				address = get_node(port->children, "address");
				if (!address) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: No address associated with <port>");
				}

				location = get_attribute(address->properties, "location");
				if (!location) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: No location associated with <port>");
				}

				tmpbinding->location = sdl_strdup(location->children->content);

				if (address->ns) {
					if (!strncmp(address->ns->href, WSDL_SOAP11_NAMESPACE, sizeof(WSDL_SOAP11_NAMESPACE))) {
						wsdl_soap_namespace = WSDL_SOAP11_NAMESPACE;
						tmpbinding->bindingType = BINDING_SOAP;
					} else if (!strncmp(address->ns->href, WSDL_SOAP12_NAMESPACE, sizeof(WSDL_SOAP12_NAMESPACE))) {
						wsdl_soap_namespace = WSDL_SOAP12_NAMESPACE;
						tmpbinding->bindingType = BINDING_SOAP;
					} else if (!strncmp(address->ns->href, RPC_SOAP12_NAMESPACE, sizeof(RPC_SOAP12_NAMESPACE))) {
						wsdl_soap_namespace = RPC_SOAP12_NAMESPACE;
						tmpbinding->bindingType = BINDING_SOAP;
					} else if (!strncmp(address->ns->href, WSDL_HTTP11_NAMESPACE, sizeof(WSDL_HTTP11_NAMESPACE))) {
						tmpbinding->bindingType = BINDING_HTTP;
					} else if (!strncmp(address->ns->href, WSDL_HTTP12_NAMESPACE, sizeof(WSDL_HTTP12_NAMESPACE))) {
						tmpbinding->bindingType = BINDING_HTTP;
					} else {
						php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: PHP-SOAP doesn't support binding '%s'",address->ns->href);
					}
				} else {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Unknown binding type");
				}

				ctype = strrchr(bindingAttr->children->content,':');
				if (ctype == NULL) {
					ctype = bindingAttr->children->content;
				} else {
				  ++ctype;
				}
				if (zend_hash_find(&ctx.bindings, ctype, strlen(ctype)+1, (void*)&tmp) != SUCCESS) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: No <binding> element with name '%s'", ctype);
				}
				binding = *tmp;

				if (tmpbinding->bindingType == BINDING_SOAP) {
					sdlSoapBindingPtr soapBinding;
					xmlNodePtr soapBindingNode;
					xmlAttrPtr tmp;

					soapBinding = sdl_malloc(sizeof(sdlSoapBinding));
					memset(soapBinding, 0, sizeof(sdlSoapBinding));
					soapBinding->style = SOAP_DOCUMENT;

					soapBindingNode = get_node_ex(binding->children, "binding", wsdl_soap_namespace);
					if (soapBindingNode) {
						tmp = get_attribute(soapBindingNode->properties, "style");
						if (tmp && !strncmp(tmp->children->content, "rpc", sizeof("rpc"))) {
							soapBinding->style = SOAP_RPC;
						}

						tmp = get_attribute(soapBindingNode->properties, "transport");
						if (tmp) {
							if (strncmp(tmp->children->content, WSDL_HTTP_TRANSPORT, sizeof(WSDL_HTTP_TRANSPORT))) {
								php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: PHP-SOAP doesn't support transport '%s'", tmp->children->content);
							}
							soapBinding->transport = sdl_strdup(tmp->children->content);
						}
					}
					tmpbinding->bindingAttributes = (void *)soapBinding;
				}

				name = get_attribute(binding->properties, "name");
				if (name == NULL) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing 'name' attribute for <binding>");
				}
				tmpbinding->name = sdl_strdup(name->children->content);

				type = get_attribute(binding->properties, "type");
				if (type == NULL) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing 'type' attribute for <binding>");
				}

				ctype = strrchr(type->children->content,':');
				if (ctype == NULL) {
					ctype = type->children->content;
				} else {
				  ++ctype;
				}
				if (zend_hash_find(&ctx.portTypes, ctype, strlen(ctype)+1, (void**)&tmp) != SUCCESS) {
					php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing <portType> with name '%s'", name->children->content);
				}
				portType = *tmp;

				trav2 = binding->children;
				FOREACHNODE(trav2, "operation", operation) {
					sdlFunctionPtr function;
					xmlNodePtr input, output, fault, portTypeOperation;
					xmlAttrPtr op_name, paramOrder;

					op_name = get_attribute(operation->properties, "name");
					if (op_name == NULL) {
						php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing 'name' attribute for <operation>");
					}

					portTypeOperation = get_node_with_attribute(portType->children, "operation", "name", op_name->children->content);
					if (portTypeOperation == NULL) {
						php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing <portType>/<operation> with name '%s'", op_name->children->content);
					}

					function = sdl_malloc(sizeof(sdlFunction));
					function->functionName = sdl_strdup(op_name->children->content);
					function->requestParameters = NULL;
					function->responseParameters = NULL;
					function->responseName = NULL;
					function->requestName = NULL;
					function->bindingAttributes = NULL;

					if (tmpbinding->bindingType == BINDING_SOAP) {
						sdlSoapBindingFunctionPtr soapFunctionBinding;
						sdlSoapBindingPtr soapBinding;
						xmlNodePtr soapOperation;
						xmlAttrPtr tmp;

						soapFunctionBinding = sdl_malloc(sizeof(sdlSoapBindingFunction));
						memset(soapFunctionBinding, 0, sizeof(sdlSoapBindingFunction));
						soapBinding = (sdlSoapBindingPtr)tmpbinding->bindingAttributes;
						soapFunctionBinding->style = soapBinding->style;

						soapOperation = get_node_ex(operation->children, "operation", wsdl_soap_namespace);
						if (soapOperation) {
							tmp = get_attribute(soapOperation->properties, "soapAction");
							if (tmp) {
								soapFunctionBinding->soapAction = sdl_strdup(tmp->children->content);
							}

							tmp = get_attribute(soapOperation->properties, "style");
							if (tmp) {
								if (!strncmp(tmp->children->content, "rpc", sizeof("rpc"))) {
									soapFunctionBinding->style = SOAP_RPC;
								} else {
									soapFunctionBinding->style = SOAP_DOCUMENT;
								}
							} else {
								soapFunctionBinding->style = soapBinding->style;
							}
						}

						function->bindingAttributes = (void *)soapFunctionBinding;
					}

					input = get_node(portTypeOperation->children, "input");
					if (input != NULL) {
						xmlAttrPtr message, name;

						message = get_attribute(input->properties, "message");
						if (message == NULL) {
							php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing name for <input> of '%s'", op_name->children->content);
						}
						function->requestParameters = wsdl_message(&ctx, message->children->content);

						name = get_attribute(input->properties, "name");
						if (name != NULL) {
							function->requestName = sdl_strdup(name->children->content);
						} else {
							function->requestName = sdl_strdup(function->functionName);
						}

						if (tmpbinding->bindingType == BINDING_SOAP) {
							input = get_node(operation->children, "input");
							if (input != NULL) {
								sdlSoapBindingFunctionPtr soapFunctionBinding = function->bindingAttributes;
								wsdl_soap_binding_body(&ctx, input, wsdl_soap_namespace,&soapFunctionBinding->input);
							}
						}
					}

					output = get_node(portTypeOperation->children, "output");
					if (output != NULL) {
						xmlAttrPtr message, name;

						message = get_attribute(output->properties, "message");
						if (message == NULL) {
							php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Missing name for <output> of '%s'", op_name->children->content);
						}
						function->responseParameters = wsdl_message(&ctx, message->children->content);

						name = get_attribute(output->properties, "name");
						if (name != NULL) {
							function->responseName = sdl_strdup(name->children->content);
						} else if (input == NULL) {
							function->responseName = sdl_strdup(function->functionName);
						} else {
							int len = strlen(function->functionName);
							function->responseName = sdl_malloc(len + sizeof("Response"));
							memcpy(function->responseName, function->functionName, len);
							memcpy(function->responseName+len, "Response", sizeof("Response"));
						}

						if (tmpbinding->bindingType == BINDING_SOAP) {
							output = get_node(operation->children, "output");
							if (output != NULL) {
								sdlSoapBindingFunctionPtr soapFunctionBinding = function->bindingAttributes;
								wsdl_soap_binding_body(&ctx, output, wsdl_soap_namespace, &soapFunctionBinding->output);
							}
						}
					}

					paramOrder = get_attribute(portTypeOperation->properties, "parameterOrder");
					if (paramOrder) {
						/* FIXME: */
					}

					fault = get_node(operation->children, "fault");
					if (!fault) {
						/* FIXME: */
					}

					function->binding = tmpbinding;

					{
						char *tmp = estrdup(function->functionName);
						int  len = strlen(tmp);

						zend_hash_add(&ctx.sdl->functions, php_strtolower(tmp, len), len+1, &function, sizeof(sdlFunctionPtr), NULL);
						efree(tmp);
						if (function->requestName != NULL && strcmp(function->requestName,function->functionName) != 0) {
							if (ctx.sdl->requests == NULL) {
								ctx.sdl->requests = sdl_malloc(sizeof(HashTable));
								zend_hash_init(ctx.sdl->requests, 0, NULL, NULL, SDL_PERSISTENT);
							}
							tmp = estrdup(function->requestName);
							len = strlen(tmp);
							zend_hash_add(ctx.sdl->requests, php_strtolower(tmp, len), len+1, &function, sizeof(sdlFunctionPtr), NULL);
							efree(tmp);
						}
					}
				}
				ENDFOREACH(trav2);

				if (!ctx.sdl->bindings) {
					ctx.sdl->bindings = sdl_malloc(sizeof(HashTable));
					zend_hash_init(ctx.sdl->bindings, 0, NULL, delete_binding, SDL_PERSISTENT);
				}

				zend_hash_add(ctx.sdl->bindings, tmpbinding->name, strlen(tmpbinding->name), &tmpbinding, sizeof(sdlBindingPtr), NULL);
			}
			ENDFOREACH(trav);

			zend_hash_move_forward(&ctx.services);
		}
	} else {
		php_error(E_ERROR, "SOAP-ERROR: Parsing WSDL: Couldn't bind to service");
	}

	zend_hash_destroy(&ctx.messages);
	zend_hash_destroy(&ctx.bindings);
	zend_hash_destroy(&ctx.portTypes);
	zend_hash_destroy(&ctx.services);
	zend_hash_destroy(&ctx.docs);

	return ctx.sdl;
}

#define WSDL_CACHE_VERSION 01

#define WSDL_CACHE_GET(ret,type,buf)   memcpy(&ret,*buf,sizeof(type)); *buf += sizeof(type);
#define WSDL_CACHE_GET_INT(ret,buf)    ret = ((int)(*buf)[0])+((int)(*buf)[1]<<8)+((int)(*buf)[2]<<16)+((int)(*buf)[3]<<24); *buf += 4;
#define WSDL_CACHE_GET_1(ret,type,buf) ret = (type)(**buf); (*buf)++;
#define WSDL_CACHE_GET_N(ret,n,buf)    memcpy(ret,*buf,n); *buf += n;
#define WSDL_CACHE_SKIP(n,buf)         *buf += n;

#define WSDL_CACHE_PUT_INT(val,buf)    smart_str_appendc(buf,val & 0xff); \
                                       smart_str_appendc(buf,(val >> 8) & 0xff); \
                                       smart_str_appendc(buf,(val >> 16) & 0xff); \
                                       smart_str_appendc(buf,(val >> 24) & 0xff);
#define WSDL_CACHE_PUT_1(val,buf)      smart_str_appendc(buf,val);
#define WSDL_CACHE_PUT_N(val,n,buf)    smart_str_appendl(buf,(char*)val,n);

static char* sdl_deserialize_string(char **in)
{
	char *s;
	int len;

	WSDL_CACHE_GET_INT(len, in);
	if (len == 0) {
		return NULL;
	} else {
		s = sdl_malloc(len+1);
		WSDL_CACHE_GET_N(s, len, in);
		s[len] = '\0';
		return s;
	}
}

static void sdl_deserialize_key(HashTable* ht, void* data, char **in)
{
	int len;

	WSDL_CACHE_GET_INT(len, in);
	if (len == 0) {
		zend_hash_next_index_insert(ht, &data, sizeof(void*), NULL);
	} else {
		zend_hash_add(ht, *in, len, &data, sizeof(void*), NULL);
		WSDL_CACHE_SKIP(len, in);
	}
}

static void sdl_deserialize_attribute(sdlAttributePtr attr, encodePtr *encoders, char **in)
{
	int i;

	attr->name = sdl_deserialize_string(in);
	attr->ref = sdl_deserialize_string(in);
	attr->def = sdl_deserialize_string(in);
	attr->fixed = sdl_deserialize_string(in);
	WSDL_CACHE_GET_1(attr->form, sdlForm, in);
	WSDL_CACHE_GET_1(attr->use, sdlUse, in);
	WSDL_CACHE_GET_INT(i, in);
	attr->encode = encoders[i];
	WSDL_CACHE_GET_INT(i, in);
	if (i > 0) {
		attr->extraAttributes = sdl_malloc(sizeof(HashTable));
		zend_hash_init(attr->extraAttributes, i, NULL, delete_extra_attribute, SDL_PERSISTENT);
		while (i > 0) {
			sdlExtraAttributePtr x = sdl_malloc(sizeof(sdlExtraAttribute));
			sdl_deserialize_key(attr->extraAttributes, x, in);
			x->ns = sdl_deserialize_string(in);
			x->val = sdl_deserialize_string(in);
			--i;
		}
	}
}

static sdlRestrictionIntPtr sdl_deserialize_resriction_int(char **in)
{
	if (**in == 1) {
		sdlRestrictionIntPtr x = sdl_malloc(sizeof(sdlRestrictionInt));
		WSDL_CACHE_SKIP(1, in);
		WSDL_CACHE_GET_INT(x->value, in);
		WSDL_CACHE_GET_1(x->fixed, char, in);
		return x;
	} else {
		WSDL_CACHE_SKIP(1, in);
		return NULL;
	}
}

static sdlRestrictionCharPtr sdl_deserialize_resriction_char(char **in)
{
	if (**in == 1) {
		sdlRestrictionCharPtr x = sdl_malloc(sizeof(sdlRestrictionChar));
		WSDL_CACHE_SKIP(1, in);
		x->value = sdl_deserialize_string(in);
		WSDL_CACHE_GET_1(x->fixed, char, in);
		return x;
	} else {
		WSDL_CACHE_SKIP(1, in);
		return NULL;
	}
}

static sdlContentModelPtr sdl_deserialize_model(sdlTypePtr *types, sdlTypePtr *elements, char **in)
{
	int i;
	sdlContentModelPtr model = sdl_malloc(sizeof(sdlContentModel));

	WSDL_CACHE_GET_1(model->kind, sdlContentKind, in);
	WSDL_CACHE_GET_INT(model->min_occurs, in);
	WSDL_CACHE_GET_INT(model->max_occurs, in);
	switch (model->kind) {
		case XSD_CONTENT_ELEMENT:
			WSDL_CACHE_GET_INT(i, in);
			model->u.element = elements[i];
			break;
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL:
		case XSD_CONTENT_CHOICE:
			WSDL_CACHE_GET_INT(i, in);
			if (i > 0) {
				model->u.content = sdl_malloc(sizeof(HashTable));
				zend_hash_init(model->u.content, i, NULL, delete_model, SDL_PERSISTENT);
				while (i > 0) {
					sdlContentModelPtr x = sdl_deserialize_model(types, elements, in);
					zend_hash_next_index_insert(model->u.content,&x,sizeof(sdlContentModelPtr),NULL);
					i--;
				}
			}
			break;
		case XSD_CONTENT_GROUP_REF:
			model->u.group_ref = sdl_deserialize_string(in);
			break;
		case XSD_CONTENT_GROUP:
			WSDL_CACHE_GET_INT(i, in);
			model->u.group = types[i];
			break;
		default:
			break;
	}
	return model;
}

static void sdl_deserialize_type(sdlTypePtr type, sdlTypePtr *types, encodePtr *encoders, char **in)
{
	int i;
	sdlTypePtr *elements = NULL;

	WSDL_CACHE_GET_1(type->kind, sdlTypeKind, in);
	type->name = sdl_deserialize_string(in);
	type->namens = sdl_deserialize_string(in);
	type->def = sdl_deserialize_string(in);
	type->fixed = sdl_deserialize_string(in);
	type->ref = sdl_deserialize_string(in);
	WSDL_CACHE_GET_1(type->nillable, char, in);

	WSDL_CACHE_GET_INT(i, in);
	type->encode = encoders[i];

	if (**in == 1) {
		WSDL_CACHE_SKIP(1, in);
		type->restrictions = sdl_malloc(sizeof(sdlRestrictions));
		/*memset(type->restrictions, 0, sizeof(sdlRestrictions));*/
		type->restrictions->minExclusive = sdl_deserialize_resriction_int(in);
		type->restrictions->minInclusive = sdl_deserialize_resriction_int(in);
		type->restrictions->maxExclusive = sdl_deserialize_resriction_int(in);
		type->restrictions->maxInclusive = sdl_deserialize_resriction_int(in);
		type->restrictions->totalDigits = sdl_deserialize_resriction_int(in);
		type->restrictions->fractionDigits = sdl_deserialize_resriction_int(in);
		type->restrictions->length = sdl_deserialize_resriction_int(in);
		type->restrictions->minLength = sdl_deserialize_resriction_int(in);
		type->restrictions->maxLength = sdl_deserialize_resriction_int(in);
		type->restrictions->whiteSpace = sdl_deserialize_resriction_char(in);
		type->restrictions->pattern = sdl_deserialize_resriction_char(in);
		WSDL_CACHE_GET_INT(i, in);
		if (i > 0) {
			type->restrictions->enumeration = sdl_malloc(sizeof(HashTable));
			zend_hash_init(type->restrictions->enumeration, i, NULL, delete_restriction_var_char, SDL_PERSISTENT);
			while (i > 0) {
				sdlRestrictionCharPtr x = sdl_deserialize_resriction_char(in);
				sdl_deserialize_key(type->restrictions->enumeration, x, in);
				--i;
			}
		} else {
			type->restrictions->enumeration = NULL;
		}
	} else {
		WSDL_CACHE_SKIP(1, in);
	}

	WSDL_CACHE_GET_INT(i, in);
	if (i > 0) {
		elements = do_alloca((i+1) * sizeof(sdlTypePtr));
		elements[0] = NULL;
		type->elements = sdl_malloc(sizeof(HashTable));
		zend_hash_init(type->elements, i, NULL, delete_type, SDL_PERSISTENT);
		while (i > 0) {
			sdlTypePtr t = sdl_malloc(sizeof(sdlType));
			memset(t, 0, sizeof(sdlType));
			sdl_deserialize_key(type->elements, t, in);
			sdl_deserialize_type(t, types, encoders, in);
			elements[i] = t;
			--i;
		}
	}

	WSDL_CACHE_GET_INT(i, in);
	if (i > 0) {
		type->attributes = sdl_malloc(sizeof(HashTable));
		zend_hash_init(type->attributes, i, NULL, delete_attribute, SDL_PERSISTENT);
		while (i > 0) {
			sdlAttributePtr attr = sdl_malloc(sizeof(sdlAttribute));
			memset(attr, 0, sizeof(sdlAttribute));
			sdl_deserialize_key(type->attributes, attr, in);
			sdl_deserialize_attribute(attr, encoders, in);
			--i;
		}
	}

	if (**in != 0) {
		WSDL_CACHE_SKIP(1, in);
		type->model = sdl_deserialize_model(types, elements, in);
	} else {
		WSDL_CACHE_SKIP(1, in);
	}
	if (elements != NULL) {
		free_alloca(elements);
	}
}

static void sdl_deserialize_encoder(encodePtr enc, sdlTypePtr *types, char **in)
{
	int i;

	WSDL_CACHE_GET_INT(enc->details.type, in);
	enc->details.type_str = sdl_deserialize_string(in);
	enc->details.ns = sdl_deserialize_string(in);
	WSDL_CACHE_GET_INT(i, in);
	enc->details.sdl_type = types[i];
	enc->to_xml = sdl_guess_convert_xml;
	enc->to_zval = sdl_guess_convert_zval;
}

static void sdl_deserialize_soap_body(sdlSoapBindingFunctionBodyPtr body, encodePtr *encoders, sdlTypePtr *types, char **in)
{
	int i, n;

	WSDL_CACHE_GET_1(body->use, sdlEncodingUse, in);
	body->ns = sdl_deserialize_string(in);
	body->parts = sdl_deserialize_string(in);
	body->encodingStyle = sdl_deserialize_string(in);
	WSDL_CACHE_GET_INT(i, in);
	if (i > 0) {
		body->headers = sdl_malloc(sizeof(HashTable));
		zend_hash_init(body->headers, i, NULL, delete_header, SDL_PERSISTENT);
		while (i > 0) {
			sdlSoapBindingFunctionHeaderPtr tmp = sdl_malloc(sizeof(sdlSoapBindingFunctionHeader));
			sdl_deserialize_key(body->headers, tmp, in);
			WSDL_CACHE_GET_1(tmp->use, sdlEncodingUse, in);
			tmp->name = sdl_deserialize_string(in);
			tmp->ns = sdl_deserialize_string(in);
			tmp->encodingStyle = sdl_deserialize_string(in);
			WSDL_CACHE_GET_INT(n, in);
			tmp->encode = encoders[n];
			WSDL_CACHE_GET_INT(n, in);
			tmp->element = types[n];
			--i;
		}
	}
}

static HashTable* sdl_deserialize_parameters(encodePtr *encoders, sdlTypePtr *types, char **in)
{
	int i, n;
	HashTable *ht;

	WSDL_CACHE_GET_INT(i, in);
	if (i == 0) {return NULL;}
	ht = sdl_malloc(sizeof(HashTable));
	zend_hash_init(ht, i, NULL, delete_parameter, SDL_PERSISTENT);
	while (i > 0) {
		sdlParamPtr param = sdl_malloc(sizeof(sdlParam));
		sdl_deserialize_key(ht, param, in);
		param->paramName = sdl_deserialize_string(in);
		WSDL_CACHE_GET_INT(param->order, in);
		WSDL_CACHE_GET_INT(n, in);
		param->encode = encoders[n];
		WSDL_CACHE_GET_INT(n, in);
		param->element = types[n];
		--i;
	}
	return ht;
}

static sdlPtr get_sdl_from_cache(const char *fn, const char *uri, time_t t)
{
	sdlPtr sdl;
	time_t old_t;
	int  i, num_groups, num_types, num_elements, num_encoders, num_bindings, num_func;
	sdlFunctionPtr *functions;
	sdlBindingPtr *bindings;
	sdlTypePtr *types;
	encodePtr *encoders;
	encodePtr enc;

	int f;
	struct stat st;
	char *in, *buf;

	f = open(fn, O_RDONLY|O_BINARY);
	if (f < 0) {
		return NULL;
	}
	if (fstat(f, &st) != 0) {
		close(f);
		return NULL;
	}
	buf = in = do_alloca(st.st_size);
	if (read(f, in, st.st_size) != st.st_size) {
		close(f);
		free_alloca(in);
		return NULL;
	}
	close(f);

	if (strncmp(in,"wsdl",4) != 0 || in[4] != WSDL_CACHE_VERSION || in[5] != '\0') {
		unlink(fn);
		free_alloca(buf);
		return NULL;
	}
	in += 6;

	WSDL_CACHE_GET(old_t, time_t, &in);
	if (old_t < t) {
		unlink(fn);
		free_alloca(buf);
		return NULL;
	}

	WSDL_CACHE_GET_INT(i, &in);
	if (i == 0 && strncmp(in, uri, i) != 0) {
		unlink(fn);
		free_alloca(buf);
		return NULL;
	}
	WSDL_CACHE_SKIP(i, &in);

	sdl = sdl_malloc(sizeof(*sdl));
	memset(sdl, 0, sizeof(*sdl));

	sdl->source = sdl_deserialize_string(&in);
	sdl->target_ns = sdl_deserialize_string(&in);

	WSDL_CACHE_GET_INT(num_groups, &in);
	WSDL_CACHE_GET_INT(num_types, &in);
	WSDL_CACHE_GET_INT(num_elements, &in);
	WSDL_CACHE_GET_INT(num_encoders, &in);

	i = num_groups+num_types+num_elements;
	types = do_alloca((i+1)*sizeof(sdlTypePtr));
	types[0] = NULL;
	while (i > 0) {
		types[i] = sdl_malloc(sizeof(sdlType));
		memset(types[i], 0, sizeof(sdlType));
		i--;
	}

	i = num_encoders;
	enc = defaultEncoding;
	while (enc->details.type != END_KNOWN_TYPES) {
		i++; enc++;
	}
	encoders = do_alloca((i+1)*sizeof(encodePtr));
	i = num_encoders;
	encoders[0] = NULL;
	while (i > 0) {
		encoders[i] = sdl_malloc(sizeof(encode));
		memset(encoders[i], 0, sizeof(encode));
		i--;
	}
	i = num_encoders;
	enc = defaultEncoding;
	while (enc->details.type != END_KNOWN_TYPES) {
		encoders[++i] = enc++;
	}

	i = 1;
	if (num_groups > 0) {
		sdl->groups = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->groups, num_groups, NULL, delete_type, SDL_PERSISTENT);
		while (i < num_groups+1) {
			sdl_deserialize_key(sdl->groups, types[i], &in);
			sdl_deserialize_type(types[i], types, encoders, &in);
			i++;
		}
	}

	if (num_types > 0) {
		sdl->types = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->types, num_types, NULL, delete_type, SDL_PERSISTENT);
		while (i < num_groups+num_types+1) {
			sdl_deserialize_key(sdl->types, types[i], &in);
			sdl_deserialize_type(types[i], types, encoders, &in);
			i++;
		}
	}

	if (num_elements > 0) {
		sdl->elements = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->elements, num_elements, NULL, delete_type, SDL_PERSISTENT);
		while (i < num_groups+num_types+num_elements+1) {
			sdl_deserialize_key(sdl->elements, types[i], &in);
			sdl_deserialize_type(types[i], types, encoders, &in);
			i++;
		}
	}

	i = 1;
	if (num_encoders > 0) {
		sdl->encoders = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->encoders, num_encoders, NULL, delete_encoder, SDL_PERSISTENT);
		while (i < num_encoders+1) {
			sdl_deserialize_key(sdl->encoders, encoders[i], &in);
			sdl_deserialize_encoder(encoders[i], types, &in);
			i++;
		}
	}

	/* deserialize bindings */
	WSDL_CACHE_GET_INT(num_bindings, &in);
	bindings = do_alloca(num_bindings*sizeof(sdlBindingPtr));
	if (num_bindings > 0) {
		sdl->bindings = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->bindings, num_bindings, NULL, delete_binding, SDL_PERSISTENT);
		for (i = 0; i < num_bindings; i++) {
			sdlBindingPtr binding = sdl_malloc(sizeof(sdlBinding));
			memset(binding, 0, sizeof(sdlBinding));
			sdl_deserialize_key(sdl->bindings, binding, &in);
			binding->name = sdl_deserialize_string(&in);
			binding->location = sdl_deserialize_string(&in);
			WSDL_CACHE_GET_1(binding->bindingType,sdlBindingType,&in);
			if (binding->bindingType == BINDING_SOAP) {
				if (*in != 0) {
				  sdlSoapBindingPtr soap_binding = binding->bindingAttributes = sdl_malloc(sizeof(sdlSoapBinding));
					WSDL_CACHE_GET_1(soap_binding->style,sdlEncodingStyle,&in);
					soap_binding->transport = sdl_deserialize_string(&in);
				} else {
					WSDL_CACHE_SKIP(1,&in);
				}
			}
			bindings[i] = binding;
		}
	}

	/* deserialize functions */
	WSDL_CACHE_GET_INT(num_func, &in);
	zend_hash_init(&sdl->functions, num_func, NULL, delete_function, SDL_PERSISTENT);
	functions = do_alloca(num_func*sizeof(sdlFunctionPtr));
	for (i = 0; i < num_func; i++) {
		int binding_num;
		sdlFunctionPtr func = sdl_malloc(sizeof(sdlFunction));
		sdl_deserialize_key(&sdl->functions, func, &in);
		func->functionName = sdl_deserialize_string(&in);
		func->requestName = sdl_deserialize_string(&in);
		func->responseName = sdl_deserialize_string(&in);

		WSDL_CACHE_GET_INT(binding_num, &in);
		if (binding_num == 0) {
			func->binding = NULL;
		} else {
			func->binding = bindings[binding_num-1];
		}
		if (func->binding && func->binding->bindingType == BINDING_SOAP) {
			if (*in != 0) {
				sdlSoapBindingFunctionPtr binding = func->bindingAttributes = sdl_malloc(sizeof(sdlSoapBindingFunction));
				memset(binding, 0, sizeof(sdlSoapBindingFunction));
				WSDL_CACHE_GET_1(binding->style,sdlEncodingStyle,&in);
				binding->soapAction = sdl_deserialize_string(&in);
				sdl_deserialize_soap_body(&binding->input, encoders, types, &in);
				sdl_deserialize_soap_body(&binding->output, encoders, types, &in);
				/*sdl_deserialize_soap_body(&binding->fault, encoders, types, &in);*/
			} else {
				WSDL_CACHE_SKIP(1, &in);
			}
		}

		func->requestParameters = sdl_deserialize_parameters(encoders, types, &in);
		func->responseParameters = sdl_deserialize_parameters(encoders, types, &in);
		functions[i] = func;
	}

	/* deserialize requests */
	WSDL_CACHE_GET_INT(i, &in);
	if (i > 0) {
		sdl->requests = sdl_malloc(sizeof(HashTable));
		zend_hash_init(sdl->requests, i, NULL, NULL, SDL_PERSISTENT);
		while (i > 0) {
			int function_num;

			WSDL_CACHE_GET_INT(function_num, &in);
			sdl_deserialize_key(sdl->requests, functions[function_num], &in);
			i--;
		}
	}

	free_alloca(functions);
	free_alloca(bindings);
	free_alloca(encoders);
	free_alloca(types);
	free_alloca(buf);
	return sdl;
}

static void sdl_serialize_string(const char *str, smart_str *out)
{
	int i;

	if (str) {
		i = strlen(str);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		WSDL_CACHE_PUT_N(str, i, out);
	}
}

static void sdl_serialize_key(HashTable *ht, smart_str *out)
{
	char *key;
	uint  key_len;
	ulong index;

	if (zend_hash_get_current_key_ex(ht, &key, &key_len, &index, 0, NULL) == HASH_KEY_IS_STRING) {
		WSDL_CACHE_PUT_INT(key_len, out);
		WSDL_CACHE_PUT_N(key, key_len, out);
	} else {
		WSDL_CACHE_PUT_INT(0, out);
	}
}

static void sdl_serialize_encoder_ref(encodePtr enc, HashTable *tmp_encoders, smart_str *out) {
	if (enc) {
		int *encoder_num;
		if (zend_hash_find(tmp_encoders, (char*)&enc, sizeof(enc), (void**)&encoder_num) == SUCCESS) {
			WSDL_CACHE_PUT_INT(*encoder_num, out);
		} else {
			WSDL_CACHE_PUT_INT(0, out);
		}
	} else {
		WSDL_CACHE_PUT_INT(0, out);
	}
}

static void sdl_serialize_type_ref(sdlTypePtr type, HashTable *tmp_types, smart_str *out) {
	if (type) {
		int *type_num;
		if (zend_hash_find(tmp_types, (char*)&type, sizeof(type), (void**)&type_num) == SUCCESS) {
			WSDL_CACHE_PUT_INT(*type_num, out);
		} else {
			WSDL_CACHE_PUT_INT(0, out);
		}
	} else {
		WSDL_CACHE_PUT_INT(0,out);
	}
}

static void sdl_serialize_attribute(sdlAttributePtr attr, HashTable *tmp_encoders, smart_str *out)
{
	int i;

	sdl_serialize_string(attr->name, out);
	sdl_serialize_string(attr->ref, out);
	sdl_serialize_string(attr->def, out);
	sdl_serialize_string(attr->fixed, out);
	WSDL_CACHE_PUT_1(attr->form, out);
	WSDL_CACHE_PUT_1(attr->use, out);
	sdl_serialize_encoder_ref(attr->encode, tmp_encoders, out);
	if (attr->extraAttributes) {
		i = zend_hash_num_elements(attr->extraAttributes);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlExtraAttributePtr *tmp;
		zend_hash_internal_pointer_reset(attr->extraAttributes);
		while (zend_hash_get_current_data(attr->extraAttributes, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(attr->extraAttributes, out);
			sdl_serialize_string((*tmp)->ns, out);
			sdl_serialize_string((*tmp)->val, out);
			zend_hash_move_forward(attr->extraAttributes);
		}
	}
}

static void sdl_serialize_model(sdlContentModelPtr model, HashTable *tmp_types, HashTable *tmp_elements, smart_str *out)
{
	WSDL_CACHE_PUT_1(model->kind, out);
	WSDL_CACHE_PUT_INT(model->min_occurs, out);
	WSDL_CACHE_PUT_INT(model->max_occurs, out);
	switch (model->kind) {
		case XSD_CONTENT_ELEMENT:
			sdl_serialize_type_ref(model->u.element, tmp_elements, out);
			break;
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL:
		case XSD_CONTENT_CHOICE: {
				sdlContentModelPtr *tmp;
				int i = zend_hash_num_elements(model->u.content);

				WSDL_CACHE_PUT_INT(i, out);
				zend_hash_internal_pointer_reset(model->u.content);
				while (zend_hash_get_current_data(model->u.content, (void**)&tmp) == SUCCESS) {
					sdl_serialize_model(*tmp, tmp_types, tmp_elements, out);
					zend_hash_move_forward(model->u.content);
				}
			}
			break;
		case XSD_CONTENT_GROUP_REF:
			sdl_serialize_string(model->u.group_ref,out);
			break;
		case XSD_CONTENT_GROUP:
			sdl_serialize_type_ref(model->u.group, tmp_types, out);
			break;
		default:
			break;
	}
}

static void sdl_serialize_resriction_int(sdlRestrictionIntPtr x, smart_str *out)
{
	if (x) {
		WSDL_CACHE_PUT_1(1, out);
		WSDL_CACHE_PUT_INT(x->value, out);
		WSDL_CACHE_PUT_1(x->fixed, out);
	} else {
		WSDL_CACHE_PUT_1(0, out);
	}
}

static void sdl_serialize_resriction_char(sdlRestrictionCharPtr x, smart_str *out)
{
	if (x) {
		WSDL_CACHE_PUT_1(1, out);
		sdl_serialize_string(x->value, out);
		WSDL_CACHE_PUT_1(x->fixed, out);
	} else {
		WSDL_CACHE_PUT_1(0, out);
	}
}

static void sdl_serialize_type(sdlTypePtr type, HashTable *tmp_encoders, HashTable *tmp_types, smart_str *out)
{
	int i;
	HashTable *tmp_elements = NULL;

	WSDL_CACHE_PUT_1(type->kind, out);
	sdl_serialize_string(type->name, out);
	sdl_serialize_string(type->namens, out);
	sdl_serialize_string(type->def, out);
	sdl_serialize_string(type->fixed, out);
	sdl_serialize_string(type->ref, out);
	WSDL_CACHE_PUT_1(type->nillable, out);
	sdl_serialize_encoder_ref(type->encode, tmp_encoders, out);

	if (type->restrictions) {
		WSDL_CACHE_PUT_1(1, out);
		sdl_serialize_resriction_int(type->restrictions->minExclusive,out);
		sdl_serialize_resriction_int(type->restrictions->minInclusive,out);
		sdl_serialize_resriction_int(type->restrictions->maxExclusive,out);
		sdl_serialize_resriction_int(type->restrictions->maxInclusive,out);
		sdl_serialize_resriction_int(type->restrictions->totalDigits,out);
		sdl_serialize_resriction_int(type->restrictions->fractionDigits,out);
		sdl_serialize_resriction_int(type->restrictions->length,out);
		sdl_serialize_resriction_int(type->restrictions->minLength,out);
		sdl_serialize_resriction_int(type->restrictions->maxLength,out);
		sdl_serialize_resriction_char(type->restrictions->whiteSpace,out);
		sdl_serialize_resriction_char(type->restrictions->pattern,out);
		if (type->restrictions->enumeration) {
			i = zend_hash_num_elements(type->restrictions->enumeration);
		} else {
			i = 0;
		}
		WSDL_CACHE_PUT_INT(i, out);
		if (i > 0) {
			sdlRestrictionCharPtr *tmp;

			zend_hash_internal_pointer_reset(type->restrictions->enumeration);
			while (zend_hash_get_current_data(type->restrictions->enumeration, (void**)&tmp) == SUCCESS) {
				sdl_serialize_resriction_char(*tmp, out);
				sdl_serialize_key(type->restrictions->enumeration, out);
				zend_hash_move_forward(type->restrictions->enumeration);
			}
		}
	} else {
		WSDL_CACHE_PUT_1(0, out);
	}
	if (type->elements) {
		i = zend_hash_num_elements(type->elements);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlTypePtr *tmp;

	  tmp_elements = emalloc(sizeof(HashTable));
	  zend_hash_init(tmp_elements, 0, NULL, NULL, 0);

		zend_hash_internal_pointer_reset(type->elements);
		while (zend_hash_get_current_data(type->elements, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(type->elements, out);
			sdl_serialize_type(*tmp, tmp_encoders, tmp_types, out);
			zend_hash_add(tmp_elements, (char*)tmp, sizeof(*tmp), &i, sizeof(int), NULL);
			i--;
			zend_hash_move_forward(type->elements);
		}
	}

	if (type->attributes) {
		i = zend_hash_num_elements(type->attributes);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlAttributePtr *tmp;
		zend_hash_internal_pointer_reset(type->attributes);
		while (zend_hash_get_current_data(type->attributes, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(type->attributes, out);
			sdl_serialize_attribute(*tmp, tmp_encoders, out);
			zend_hash_move_forward(type->attributes);
		}
	}
	if (type->model) {
		WSDL_CACHE_PUT_1(1, out);
		sdl_serialize_model(type->model, tmp_types, tmp_elements, out);
	} else {
		WSDL_CACHE_PUT_1(0, out);
	}
	if (tmp_elements != NULL) {
		zend_hash_destroy(tmp_elements);
		efree(tmp_elements);
	}
}

static void sdl_serialize_encoder(encodePtr enc, HashTable *tmp_types, smart_str *out)
{
	WSDL_CACHE_PUT_INT(enc->details.type, out);
	sdl_serialize_string(enc->details.type_str, out);
	sdl_serialize_string(enc->details.ns, out);
	sdl_serialize_type_ref(enc->details.sdl_type, tmp_types, out);
}

static void sdl_serialize_parameters(HashTable *ht, HashTable *tmp_encoders, HashTable *tmp_types, smart_str *out)
{
	int i;

	if (ht) {
		i = zend_hash_num_elements(ht);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlParamPtr *tmp;

		zend_hash_internal_pointer_reset(ht);
		while (zend_hash_get_current_data(ht, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(ht, out);
			sdl_serialize_string((*tmp)->paramName, out);
			WSDL_CACHE_PUT_INT((*tmp)->order, out);
			sdl_serialize_encoder_ref((*tmp)->encode, tmp_encoders, out);
			sdl_serialize_type_ref((*tmp)->element, tmp_types, out);
			zend_hash_move_forward(ht);
		}
	}
}

static void sdl_serialize_soap_body(sdlSoapBindingFunctionBodyPtr body, HashTable *tmp_encoders, HashTable *tmp_types, smart_str *out)
{
	int i;

	WSDL_CACHE_PUT_1(body->use, out);
	sdl_serialize_string(body->ns, out);
	sdl_serialize_string(body->parts, out);
	sdl_serialize_string(body->encodingStyle, out);
	if (body->headers) {
		i = zend_hash_num_elements(body->headers);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlSoapBindingFunctionHeaderPtr *tmp;
		zend_hash_internal_pointer_reset(body->headers);
		while (zend_hash_get_current_data(body->headers, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(body->headers, out);
			WSDL_CACHE_PUT_1((*tmp)->use, out);
			sdl_serialize_string((*tmp)->name, out);
			sdl_serialize_string((*tmp)->ns, out);
			sdl_serialize_string((*tmp)->encodingStyle, out);
			sdl_serialize_encoder_ref((*tmp)->encode, tmp_encoders, out);
			sdl_serialize_type_ref((*tmp)->element, tmp_types, out);
			zend_hash_move_forward(body->headers);
		}
	}
}

static void add_sdl_to_cache(const char *fn, const char *uri, time_t t, sdlPtr sdl)
{
	smart_str buf = {0};
	smart_str *out = &buf;
	int i;
	int type_num = 1;
	int encoder_num = 1;
	int f;
	encodePtr enc;
	HashTable tmp_types;
	HashTable tmp_encoders;
	HashTable tmp_bindings;
	HashTable tmp_functions;

	f = open(fn,O_CREAT|O_WRONLY|O_EXCL|O_BINARY,S_IREAD|S_IWRITE);
	if (f < 0) {return;}

	zend_hash_init(&tmp_types, 0, NULL, NULL, 0);
	zend_hash_init(&tmp_encoders, 0, NULL, NULL, 0);
	zend_hash_init(&tmp_bindings, 0, NULL, NULL, 0);
	zend_hash_init(&tmp_functions, 0, NULL, NULL, 0);

	WSDL_CACHE_PUT_N("wsdl", 4, out);
	WSDL_CACHE_PUT_1(WSDL_CACHE_VERSION,out);
	WSDL_CACHE_PUT_1(0,out);
	WSDL_CACHE_PUT_N(&t, sizeof(t), out);

	sdl_serialize_string(uri, out);
	sdl_serialize_string(sdl->source, out);
	sdl_serialize_string(sdl->target_ns, out);

	if (sdl->groups) {
		i = zend_hash_num_elements(sdl->groups);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlTypePtr *tmp;

		zend_hash_internal_pointer_reset(sdl->groups);
		while (zend_hash_get_current_data(sdl->groups, (void**)&tmp) == SUCCESS) {
			zend_hash_add(&tmp_types, (char*)tmp, sizeof(*tmp), (void**)&type_num, sizeof(type_num), NULL);
			++type_num;
			zend_hash_move_forward(sdl->groups);
		}
	}

	if (sdl->types) {
		i = zend_hash_num_elements(sdl->types);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlTypePtr *tmp;

		zend_hash_internal_pointer_reset(sdl->types);
		while (zend_hash_get_current_data(sdl->types, (void**)&tmp) == SUCCESS) {
			zend_hash_add(&tmp_types, (char*)tmp, sizeof(*tmp), (void**)&type_num, sizeof(type_num), NULL);
			++type_num;
			zend_hash_move_forward(sdl->types);
		}
	}

	if (sdl->elements) {
		i = zend_hash_num_elements(sdl->elements);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlTypePtr *tmp;

		zend_hash_internal_pointer_reset(sdl->elements);
		while (zend_hash_get_current_data(sdl->elements, (void**)&tmp) == SUCCESS) {
			zend_hash_add(&tmp_types, (char*)tmp, sizeof(*tmp), (void**)&type_num, sizeof(type_num), NULL);
			++type_num;
			zend_hash_move_forward(sdl->elements);
		}
	}

	if (sdl->encoders) {
		i = zend_hash_num_elements(sdl->encoders);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		encodePtr *tmp;

		zend_hash_internal_pointer_reset(sdl->encoders);
		while (zend_hash_get_current_data(sdl->encoders, (void**)&tmp) == SUCCESS) {
			zend_hash_add(&tmp_encoders, (char*)tmp, sizeof(*tmp), (void**)&encoder_num, sizeof(encoder_num), NULL);
			++encoder_num;
			zend_hash_move_forward(sdl->encoders);
		}
	}
	enc = defaultEncoding;
	while (enc->details.type != END_KNOWN_TYPES) {
		zend_hash_add(&tmp_encoders, (char*)&enc, sizeof(encodePtr), (void**)&encoder_num, sizeof(encoder_num), NULL);
		enc++;
		++encoder_num;
	}

	if (sdl->groups) {
		sdlTypePtr *tmp;
		zend_hash_internal_pointer_reset(sdl->groups);
		while (zend_hash_get_current_data(sdl->groups, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(sdl->groups, out);
			sdl_serialize_type(*tmp, &tmp_encoders, &tmp_types, out);
			zend_hash_move_forward(sdl->groups);
		}
	}

	if (sdl->types) {
		sdlTypePtr *tmp;
		zend_hash_internal_pointer_reset(sdl->types);
		while (zend_hash_get_current_data(sdl->types, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(sdl->types, out);
			sdl_serialize_type(*tmp, &tmp_encoders, &tmp_types, out);
			zend_hash_move_forward(sdl->types);
		}
	}

	if (sdl->elements) {
		sdlTypePtr *tmp;
		zend_hash_internal_pointer_reset(sdl->elements);
		while (zend_hash_get_current_data(sdl->elements, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(sdl->elements, out);
			sdl_serialize_type(*tmp, &tmp_encoders, &tmp_types, out);
			zend_hash_move_forward(sdl->elements);
		}
	}

	if (sdl->encoders) {
		encodePtr *tmp;
		zend_hash_internal_pointer_reset(sdl->encoders);
		while (zend_hash_get_current_data(sdl->encoders, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(sdl->encoders, out);
			sdl_serialize_encoder(*tmp, &tmp_types, out);
			zend_hash_move_forward(sdl->encoders);
		}
	}

	/* serialize bindings */
	if (sdl->bindings) {
		i = zend_hash_num_elements(sdl->bindings);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlBindingPtr *tmp;
		int binding_num = 1;

		zend_hash_internal_pointer_reset(sdl->bindings);
		while (zend_hash_get_current_data(sdl->bindings, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(sdl->bindings, out);
			sdl_serialize_string((*tmp)->name, out);
			sdl_serialize_string((*tmp)->location, out);
			WSDL_CACHE_PUT_1((*tmp)->bindingType,out);
			if ((*tmp)->bindingType == BINDING_SOAP && (*tmp)->bindingAttributes != NULL) {
				sdlSoapBindingPtr binding = (sdlSoapBindingPtr)(*tmp)->bindingAttributes;
				WSDL_CACHE_PUT_1(binding->style, out);
				sdl_serialize_string(binding->transport, out);
			} else {
				WSDL_CACHE_PUT_1(0,out);
			}

			zend_hash_add(&tmp_bindings, (char*)tmp, sizeof(*tmp), (void**)&binding_num, sizeof(binding_num), NULL);
			binding_num++;
			zend_hash_move_forward(sdl->bindings);
		}
	}

	/* serialize functions */
	i = zend_hash_num_elements(&sdl->functions);
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlFunctionPtr *tmp;
		int *binding_num;
		int function_num = 1;

		zend_hash_internal_pointer_reset(&sdl->functions);
		while (zend_hash_get_current_data(&sdl->functions, (void**)&tmp) == SUCCESS) {
			sdl_serialize_key(&sdl->functions, out);
			sdl_serialize_string((*tmp)->functionName, out);
			sdl_serialize_string((*tmp)->requestName, out);
			sdl_serialize_string((*tmp)->responseName, out);

			if ((*tmp)->binding == NULL ||
			    zend_hash_find(&tmp_bindings,(char*)&(*tmp)->binding,sizeof((*tmp)->binding), (void**)&binding_num) != SUCCESS) {
			}
			WSDL_CACHE_PUT_INT(*binding_num, out);
			if (binding_num >= 0) {
				if ((*tmp)->binding->bindingType == BINDING_SOAP && (*tmp)->bindingAttributes != NULL) {
					sdlSoapBindingFunctionPtr binding = (sdlSoapBindingFunctionPtr)(*tmp)->bindingAttributes;
					WSDL_CACHE_PUT_1(binding->style, out);
					sdl_serialize_string(binding->soapAction, out);
					sdl_serialize_soap_body(&binding->input, &tmp_encoders, &tmp_types, out);
					sdl_serialize_soap_body(&binding->output, &tmp_encoders, &tmp_types, out);
					/*sdl_serialize_soap_body(&binding->fault, &tmp_encoders, &tmp_types, out);*/
				} else {
					WSDL_CACHE_PUT_1(0,out);
				}
			}
			sdl_serialize_parameters((*tmp)->requestParameters, &tmp_encoders, &tmp_types, out);
			sdl_serialize_parameters((*tmp)->responseParameters, &tmp_encoders, &tmp_types, out);

			zend_hash_add(&tmp_functions, (char*)tmp, sizeof(*tmp), (void**)&function_num, sizeof(function_num), NULL);
			function_num++;
			zend_hash_move_forward(&sdl->functions);
		}
	}

	/* serialize requests */
	if (sdl->requests) {
		i = zend_hash_num_elements(sdl->requests);
	} else {
		i = 0;
	}
	WSDL_CACHE_PUT_INT(i, out);
	if (i > 0) {
		sdlFunctionPtr *tmp;
		int *function_num;

		zend_hash_internal_pointer_reset(sdl->requests);
		while (zend_hash_get_current_data(sdl->requests, (void**)&tmp) == SUCCESS) {
			if (zend_hash_find(&tmp_functions, (char*)tmp, sizeof(*tmp), (void**)&function_num) != SUCCESS) {
			}
			WSDL_CACHE_PUT_INT(*function_num, out);
			sdl_serialize_key(sdl->requests, out);
			zend_hash_move_forward(sdl->requests);
		}
	}

	write(f, buf.c, buf.len);
	close(f);
	smart_str_free(&buf);
	zend_hash_destroy(&tmp_functions);
	zend_hash_destroy(&tmp_bindings);
	zend_hash_destroy(&tmp_encoders);
	zend_hash_destroy(&tmp_types);
}

sdlPtr get_sdl(char *uri TSRMLS_DC)
{
	sdlPtr sdl = NULL;
	char* old_error_code = SOAP_GLOBAL(error_code);
#ifdef SDL_CACHE
	sdlPtr *hndl;

	SOAP_GLOBAL(error_code) = "WSDL";
	if (zend_hash_find(SOAP_GLOBAL(sdls), uri, strlen(uri), (void **)&hndl) == FAILURE) {
		sdl = load_wsdl(uri);
		zend_hash_add(SOAP_GLOBAL(sdls), uri, strlen(uri), &sdl, sizeof(sdlPtr), NULL);
	} else {
		sdl = *hndl;
	}
#else
	SOAP_GLOBAL(error_code) = "WSDL";
	if (SOAP_GLOBAL(cache_enabled)) {
		char  fn[MAXPATHLEN];

	  if (strchr(uri,':') != NULL || IS_ABSOLUTE_PATH(uri,strlen(uri))) {
		  strcpy(fn, uri);
		} else if (VCWD_REALPATH(uri, fn) == NULL) {
			sdl = load_wsdl(uri);
		}
		if (sdl == NULL) {
			char* key;
			time_t t = time(0);
		  char md5str[33];
	  	PHP_MD5_CTX context;
		  unsigned char digest[16];
		  int len = strlen(SOAP_GLOBAL(cache_dir));

		  md5str[0] = '\0';
		  PHP_MD5Init(&context);
		  PHP_MD5Update(&context, fn, strlen(fn));
		  PHP_MD5Final(digest, &context);
		  make_digest(md5str, digest);
		  key = do_alloca(len+sizeof("/wsdl-")-1+sizeof(md5str));
		  memcpy(key,SOAP_GLOBAL(cache_dir),len);
		  memcpy(key+len,"/wsdl-",sizeof("/wsdl-")-1);
		  memcpy(key+len+sizeof("/wsdl-")-1,md5str,sizeof(md5str));

			if ((sdl = get_sdl_from_cache(key, fn, t-SOAP_GLOBAL(cache_ttl))) == NULL) {
				sdl = load_wsdl(fn);
				if (sdl != NULL) {
					add_sdl_to_cache(key, fn, t, sdl);
				}
			}
			free_alloca(key);
		}
	} else {
		sdl = load_wsdl(uri);
	}
#endif
	SOAP_GLOBAL(error_code) = old_error_code;
	return sdl;
}

/* Deletes */
void delete_sdl(void *handle)
{
	sdlPtr tmp = (sdlPtr)handle;

	zend_hash_destroy(&tmp->functions);
	if (tmp->source) {
		sdl_free(tmp->source);
	}
	if (tmp->target_ns) {
		sdl_free(tmp->target_ns);
	}
	if (tmp->elements) {
		zend_hash_destroy(tmp->elements);
		sdl_free(tmp->elements);
	}
	if (tmp->encoders) {
		zend_hash_destroy(tmp->encoders);
		sdl_free(tmp->encoders);
	}
	if (tmp->types) {
		zend_hash_destroy(tmp->types);
		sdl_free(tmp->types);
	}
	if (tmp->groups) {
		zend_hash_destroy(tmp->groups);
		sdl_free(tmp->groups);
	}
	if (tmp->bindings) {
		zend_hash_destroy(tmp->bindings);
		sdl_free(tmp->bindings);
	}
	if (tmp->requests) {
		zend_hash_destroy(tmp->requests);
		sdl_free(tmp->requests);
	}
	sdl_free(tmp);
}

void delete_sdl_ptr(void *handle)
{
	delete_sdl((sdlPtr*)handle);
}

static void delete_binding(void *data)
{
	sdlBindingPtr binding = *((sdlBindingPtr*)data);

	if (binding->location) {
		sdl_free(binding->location);
	}
	if (binding->name) {
		sdl_free(binding->name);
	}

	if (binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingPtr soapBind = binding->bindingAttributes;
		if (soapBind && soapBind->transport) {
			sdl_free(soapBind->transport);
		}
		sdl_free(soapBind);
	}
	sdl_free(binding);
}

static void delete_sdl_soap_binding_function_body(sdlSoapBindingFunctionBody body)
{
	if (body.ns) {
		sdl_free(body.ns);
	}
	if (body.parts) {
		sdl_free(body.parts);
	}
	if (body.encodingStyle) {
		sdl_free(body.encodingStyle);
	}
	if (body.headers) {
		zend_hash_destroy(body.headers);
		sdl_free(body.headers);
	}
}

static void delete_function(void *data)
{
	sdlFunctionPtr function = *((sdlFunctionPtr*)data);

	if (function->functionName) {
		sdl_free(function->functionName);
	}
	if (function->requestName) {
		sdl_free(function->requestName);
	}
	if (function->responseName) {
		sdl_free(function->responseName);
	}
	if (function->requestParameters) {
		zend_hash_destroy(function->requestParameters);
		sdl_free(function->requestParameters);
	}
	if (function->responseParameters) {
		zend_hash_destroy(function->responseParameters);
		sdl_free(function->responseParameters);
	}

	if (function->bindingAttributes &&
	    function->binding && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr soapFunction = function->bindingAttributes;
		if (soapFunction->soapAction) {
			sdl_free(soapFunction->soapAction);
		}
		delete_sdl_soap_binding_function_body(soapFunction->input);
		delete_sdl_soap_binding_function_body(soapFunction->output);
		delete_sdl_soap_binding_function_body(soapFunction->fault);
		sdl_free(soapFunction);
	}
	sdl_free(function);
}

static void delete_parameter(void *data)
{
	sdlParamPtr param = *((sdlParamPtr*)data);
	if (param->paramName) {
		sdl_free(param->paramName);
	}
	sdl_free(param);
}

static void delete_header(void *data)
{
	sdlSoapBindingFunctionHeaderPtr hdr = *((sdlSoapBindingFunctionHeaderPtr*)data);
	if (hdr->name) {
		sdl_free(hdr->name);
	}
	if (hdr->ns) {
		sdl_free(hdr->ns);
	}
	if (hdr->encodingStyle) {
		sdl_free(hdr->encodingStyle);
	}
	sdl_free(hdr);
}

static void delete_document(void *doc_ptr)
{
	xmlDocPtr doc = *((xmlDocPtr*)doc_ptr);
	xmlFreeDoc(doc);
}
