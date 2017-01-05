// CSOAP.CPP
//
/******************************************************************
* CSOAP Project:  A SOAP client/server library in C
* Copyright (C) 2003  Ferhat Ayaz
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*
* Email: ayaz@jprogrammer.net
******************************************************************/

#include <slib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#ifdef WIN32
	#define snprintf(buffer, num, s1, s2) sprintf(buffer, s1, s2)
	#define USE_XMLSTRING
#endif
#ifdef USE_XMLSTRING
 #include <libxml/xmlstring.h>
#endif
#include <stdio.h>
#include <nanohttp.h>
#include <csoap.h>

#define SOAP_ADMIN_QUERY_ROUTERS        "routers"
#define SOAP_ADMIN_QUERY_ROUTER         "router"
#define SOAP_ADMIN_QUERY_SERVICES       "services"

xmlNodePtr soap_xml_get_children(xmlNodePtr param)
{
	xmlNodePtr children = NULL;
	if(param == NULL) {
		log_error1("Invalid parameter 'param' (null)");
	}
	else {
		children = param->xmlChildrenNode;
		while(children != NULL) {
			if(children->type != XML_ELEMENT_NODE)
				children = children->next;
			else
				break;
		}
	}
	return children;
}

xmlNodePtr soap_xml_get_next(xmlNodePtr param)
{
	xmlNodePtr node = param->next;
	while(node != NULL) {
		if(node->type != XML_ELEMENT_NODE)
			node = node->next;
		else
			break;
	}
	return node;
}

xmlXPathObjectPtr soap_xpath_eval(xmlDocPtr doc, const char * xpath)
{
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST xpath, context);
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
		/* no result */
		return NULL;
	}
	else {
		xmlXPathFreeContext(context);
		return result;
	}
}

int soap_xpath_foreach(xmlDocPtr doc, const char * xpath, soap_xmlnode_callback cb, void * userdata)
{
	int    i = 0;
	xmlXPathObjectPtr xpathobj = soap_xpath_eval(doc, xpath);
	if(xpathobj) {
		xmlNodeSetPtr nodeset = xpathobj->nodesetval;
		if(nodeset) {
			for(i = 0; i < nodeset->nodeNr; i++) {
				if(!cb(nodeset->nodeTab[i], userdata))
					break;
			}
			xmlXPathFreeObject((xmlXPathObjectPtr)nodeset);
		}
	}
	return i;
}

void soap_xml_doc_print(xmlDocPtr doc)
{
	if(doc == NULL) {
		puts("xmlDocPtr is NULL!");
	}
	else {
		xmlNodePtr root = xmlDocGetRootElement(doc);
		if(root == NULL) {
			puts("Empty document!");
		}
		else {
			xmlBufferPtr buffer = xmlBufferCreate();
			xmlNodeDump(buffer, doc, root, 1, 0);
			puts((const char *)xmlBufferContent(buffer));
			xmlBufferFree(buffer);
		}
	}
}

char * soap_xml_get_text(xmlNodePtr node)
{
	return (char *)xmlNodeListGetString(node->doc, node->xmlChildrenNode, 1);
}

SoapServiceNode * soap_service_node_new(SoapService * service, SoapServiceNode * next)
{
	SoapServiceNode * node = (SoapServiceNode *)malloc(sizeof(SoapServiceNode));
	node->service = service;
	node->next = next;
	return node;
}

SoapService * soap_service_new(const char * urn, const char * method, SoapServiceFunc f)
{
	SoapService * service = (SoapService *)malloc(sizeof(SoapService));
	service->func = f;
	if(urn != NULL) {
		service->urn = (char *)malloc(strlen(urn)+1);
		strcpy(service->urn, urn);
	}
	else {
		log_warn1("urn is NULL");
		service->urn = "";
	}
	if(method != NULL) {
		service->method = (char *)malloc(strlen(method)+1);
		strcpy(service->method, method);
	}
	else {
		log_warn1("method is NULL");
		service->method = "";
	}
	return service;
}

void soap_service_free(SoapService * service)
{
	log_verbose2("enter: service=%p", service);
	if(service) {
		if(strcmp(service->urn, ""))
			free(service->urn);
		if(strcmp(service->method, ""))
			free(service->method);
		free(service);
		log_verbose1("leave with success");
	}
}

static SoapRouterNode * P_Head = NULL;
static SoapRouterNode * P_Tail = NULL;

// static SoapRouter *router_find(const char *context);

static void _soap_server_send_env(http_output_stream_t * out, SoapEnv * env)
{
	if(env && env->root) {
		xmlBufferPtr buffer = xmlBufferCreate();
		xmlNodeDump(buffer, env->root->doc, env->root, 1, 1);
		http_output_stream_write_string(out, (const char *)xmlBufferContent(buffer));
		xmlBufferFree(buffer);
	}
}

static void _soap_server_send_fault(httpd_conn_t * conn, const char * errmsg)
{
	SoapEnv * envres;
	herror_t err;
	char buffer[64];
	hpair_t * header = hpairnode_new(HEADER_CONTENT_TYPE, "text/xml", NULL);
	httpd_set_headers(conn, header);
	if((err = httpd_send_header(conn, 500, "FAILED")) != H_OK) {
		/* WARNING: unhandled exception ! */
		log_error4("%s():%s [%d]", herror_func(err), herror_message(err), herror_code(err));
		herror_release(err);
	}
	else {
		err = soap_env_new_with_fault(Fault_Server, errmsg ? errmsg : "General error", "cSOAP_Server", NULL, &envres);
		if(err != H_OK) {
			log_error1(herror_message(err));
			http_output_stream_write_string(conn->out, "<html><head></head><body>");
			http_output_stream_write_string(conn->out, "<h1>Error</h1><hr/>");
			http_output_stream_write_string(conn->out, "Error while sending fault object:<br>Message: ");
			http_output_stream_write_string(conn->out, herror_message(err));
			http_output_stream_write_string(conn->out, "<br />Function: ");
			http_output_stream_write_string(conn->out, herror_func(err));
			http_output_stream_write_string(conn->out, "<br />Error code: ");
			sprintf(buffer, "%d", herror_code(err));
			http_output_stream_write_string(conn->out, buffer);
			http_output_stream_write_string(conn->out, "</body></html>");
			herror_release(err);
		}
		else {
			_soap_server_send_env(conn->out, envres);
		}
		hpairnode_free(header);
	}
}

static void _soap_server_send_ctx(httpd_conn_t * conn, SoapCtx * ctx)
{
	static int counter = 1;
	xmlBufferPtr buffer;
	char strbuffer[32];
	part_t * part;
	if(ctx->env == NULL || ctx->env->root == NULL || ctx->env->root->doc == NULL)
		return;
	xmlThrDefIndentTreeOutput(1);
/*  xmlKeepBlanksDefault(0);*/

	buffer = xmlBufferCreate();
	xmlNodeDump(buffer, ctx->env->root->doc, ctx->env->root, 1, 1);
	if(ctx->attachments) {
		sprintf(strbuffer, "000128590350940924234%d", counter++);
		httpd_mime_send_header(conn, strbuffer, "", "text/xml", 200, "OK");
		httpd_mime_next(conn, strbuffer, "text/xml", "binary");
		http_output_stream_write_string(conn->out, (const char *)xmlBufferContent(buffer));
		part = ctx->attachments->parts;
		while(part) {
			httpd_mime_send_file(conn, part->id, part->content_type, part->transfer_encoding, part->filename);
			part = part->next;
		}
		httpd_mime_end(conn);
	}
	else {
		char   buflen[100];
		xmlXPathContextPtr xpathCtx = xmlXPathNewContext(ctx->env->root->doc);
		xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar *)"//Fault", xpathCtx);
		snprintf(buflen, 100, "%d", xmlBufferLength(buffer));
		httpd_set_header(conn, HEADER_CONTENT_LENGTH, buflen);
		httpd_set_header(conn, HEADER_CONTENT_TYPE, "text/xml");
		if((xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0) {
			httpd_send_header(conn, 500, "FAILED");
		}
		else {
			httpd_send_header(conn, 200, "OK");
		}
		http_output_stream_write_string(conn->out, (const char *)xmlBufferContent(buffer));
		xmlXPathFreeObject(xpathObj);
		xmlXPathFreeContext(xpathCtx);
	}
	xmlBufferFree(buffer);
}

static void _soap_server_send_description(httpd_conn_t * conn, xmlDocPtr wsdl)
{
	char length[16];
	xmlBufferPtr buf = xmlBufferCreate();
	xmlNodeDump(buf, wsdl, xmlDocGetRootElement(wsdl), 0, 0);
	sprintf(length, "%d", xmlBufferLength(buf));
	httpd_set_header(conn, HEADER_CONTENT_TYPE, "text/xml");
	httpd_set_header(conn, HEADER_CONTENT_LENGTH, length);
	httpd_send_header(conn, 200, "OK");
	http_output_stream_write_string(conn->out, (const char *)xmlBufferContent(buf));
	xmlBufferFree(buf);
}

static SoapRouterNode * router_node_new(SoapRouter * router, const char * context, SoapRouterNode * next)
{
	const char * noname = "/lost_found";
	SoapRouterNode * node;
	if(!(node = (SoapRouterNode *)malloc(sizeof(SoapRouterNode)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return NULL;
	}
	if(context) {
		node->context = strdup(context);
	}
	else {
		log_warn2("context is null. Using '%s'", noname);
		node->context = strdup(noname);
	}
	node->router = router;
	node->next = next;
	return node;
}

SoapRouter * soap_server_find_router(const char * context)
{
	for(SoapRouterNode * node = P_Head; node; node = node->next) {
		if(!strcmp(node->context, context))
			return node->router;
	}
	return NULL;
}

static void soap_server_entry(httpd_conn_t * conn, hrequest_t * req)
{
	char buffer[1054];
	char * urn;
	char * method;
	SoapCtx * ctx, * ctxres;
	SoapRouter * router;
	SoapService * service;
	SoapEnv * env;
	herror_t err;
	if(!(router = soap_server_find_router(req->path))) {
		_soap_server_send_fault(conn, "Cannot find router");
		return;
	}
	else if(req->method == HTTP_REQUEST_GET && router->wsdl) {
		_soap_server_send_description(conn, router->wsdl);
		return;
	}
	if(req->method != HTTP_REQUEST_POST) {
		httpd_send_header(conn, 200, "OK");
		http_output_stream_write_string(conn->out,
			"<html>"
			"<head>"
			"</head>"
			"<body>"
			"<h1>Sorry!</h1>"
			"<hr />"
			"<div>I only speak with 'POST' method </div>"
			"</body>"
			"</html>");
		return;
	}
	if((err = soap_env_new_from_stream(req->in, &env)) != H_OK) {
		_soap_server_send_fault(conn, herror_message(err));
		herror_release(err);
		return;
	}
	if(env == NULL) {
		_soap_server_send_fault(conn, "Can not receive POST data!");
	}
	else {
		ctx = soap_ctx_new(env);
		ctx->action = hpairnode_get_ignore_case(req->header, "SoapAction");
		if(ctx->action)
			ctx->action = strdup(ctx->action);
		ctx->http = req;
		soap_ctx_add_files(ctx, req->attachments);
		if(ctx->env == NULL) {
			_soap_server_send_fault(conn, "Can not parse POST data!");
		}
		else {
			/* soap_xml_doc_print(env->root->doc); */
			if(!(urn = soap_env_find_urn(ctx->env))) {
				_soap_server_send_fault(conn, "No URN found!");
				soap_ctx_free(ctx);
				return;
			}
			else {
				log_verbose2("urn: '%s'", urn);
			}
			if(!(method = soap_env_find_methodname(ctx->env))) {
				_soap_server_send_fault(conn, "No method found!");
				soap_ctx_free(ctx);
				return;
			}
			else {
				log_verbose2("method: '%s'", method);
			}
			service = soap_router_find_service(router, urn, method);
			if(service == NULL) {
				sprintf(buffer, "URN '%s' not found", urn);
				_soap_server_send_fault(conn, buffer);
				soap_ctx_free(ctx);
				return;
			}
			else {
				log_verbose2("func: %p", service->func);
				ctxres = soap_ctx_new(NULL);
				/* ===================================== */
				/* CALL SERVICE FUNCTION */
				/* ===================================== */
				if((err = service->func(ctx, ctxres)) != H_OK) {
					sprintf(buffer, "Service returned following error message: '%s'",
						herror_message(err));
					herror_release(err);
					_soap_server_send_fault(conn, buffer);
					soap_ctx_free(ctx);
					return;
				}
				if(ctxres->env == NULL) {
					sprintf(buffer, "Service '%s' returned no envelope", urn);
					_soap_server_send_fault(conn, buffer);
					soap_ctx_free(ctx);
					return;
				}
				else {
					_soap_server_send_ctx(conn, ctxres);
					soap_ctx_free(ctxres);
				}
			}
		}
		soap_ctx_free(ctx);
	}
}

herror_t soap_server_init_args(int argc, char * argv[])
{
	herror_t err = httpd_init(argc, argv);
	return (err == H_OK) ? soap_admin_init_args(argc, argv) : err;
}

int soap_server_register_router(SoapRouter * router, const char * context)
{
	int    ok = 1;
	if(!httpd_register_secure(context, soap_server_entry, router->auth)) {
		ok = 0;
	}
	else if(P_Tail == NULL) {
		P_Head = P_Tail = router_node_new(router, context, NULL);
	}
	else {
		P_Tail->next = router_node_new(router, context, NULL);
		P_Tail = P_Tail->next;
	}
	return ok;
}

SoapRouterNode * soap_server_get_routers()
{
	return P_Head;
}

herror_t soap_server_run()
{
	return httpd_run();
}

int soap_server_get_port()
{
	return httpd_get_port();
}

const char * soap_server_get_protocol()
{
	return httpd_get_protocol();
}

void soap_server_destroy()
{
	SoapRouterNode * node = P_Head;
	while(node != NULL) {
		SoapRouterNode * tmp = node->next;
		log_verbose2("soap_router_free(%p)", node->router);
		soap_router_free(node->router);
		free(node->context);
		free(node);
		node = tmp;
	}
	httpd_destroy();
}

SoapRouter * soap_router_new()
{
	SoapRouter * router = NULL;
	if(!(router = (SoapRouter *)malloc(sizeof(SoapRouter)))) {
		log_error2("malloc failed (%s)", strerror(errno));
	}
	else
		memset(router, 0, sizeof(SoapRouter));
	return router;
}

void soap_router_register_service(SoapRouter * router, SoapServiceFunc func, const char * method, const char * urn)
{
	SoapService * service = soap_service_new(urn, method, func);
	if(router->service_tail == NULL) {
		router->service_head = router->service_tail = soap_service_node_new(service, NULL);
	}
	else {
		router->service_tail->next = soap_service_node_new(service, NULL);
		router->service_tail = router->service_tail->next;
	}
}

void soap_router_register_security(SoapRouter * router, httpd_auth auth)
{
	router->auth = auth;
}

void soap_router_register_description(SoapRouter * router, xmlDocPtr wsdl)
{
	xmlFreeDoc(router->wsdl);
	router->wsdl = xmlCopyDoc(wsdl, 1);
}

void soap_router_register_default_service(SoapRouter * router, SoapServiceFunc func, const char * method, const char * urn)
{
	SoapService * service = soap_service_new(urn, method, func);
	if(router->service_tail == NULL) {
		router->service_head = router->service_tail = soap_service_node_new(service, NULL);
	}
	else {
		router->service_tail->next = soap_service_node_new(service, NULL);
		router->service_tail = router->service_tail->next;
	}
	router->default_service = service;
}

SoapService * soap_router_find_service(SoapRouter * router, const char * urn, const char * method)
{
	if(router && urn && method) {
		for(SoapServiceNode * node = router->service_head; node; node = node->next) {
			if(node->service && node->service->urn && node->service->method) {
				if(!strcmp(node->service->urn, urn) && !strcmp(node->service->method, method))
					return node->service;
			}
		}
		return router->default_service;
	}
	else
		return NULL;
}

void soap_router_free(SoapRouter * router)
{
	SoapServiceNode * node;
	log_verbose2("enter: router=%p", router);
	if(router) {
		while(router->service_head) {
			node = router->service_head->next;
			/* log_verbose2("soap_service_free(%p)\n",
			router->service_head->service); */
			soap_service_free(router->service_head->service);
			free(router->service_head);
			router->service_head = node;
		}
		xmlFreeDoc(router->wsdl);
		free(router);
		log_verbose1("leave with success");
	}
}
/*
   Parameters:
   1- soap_env_ns
   2- soap_env_enc
   3- xsi_ns
   4- xsd_ns
   5- faultcode
   6- faultstring
   7- faultactor
   8- detail
 */
#define  _SOAP_FAULT_TEMPLATE_ \
        "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"%s\" SOAP-ENV:encoding=\"%s\"" \
        " xmlns:xsi=\"%s\"" \
        " xmlns:xsd=\"%s\">" \
        " <SOAP-ENV:Header />" \
        " <SOAP-ENV:Body>" \
        "  <SOAP-ENV:Fault>" \
        "   <faultcode>%s</faultcode>" \
        "   <faultstring>%s</faultstring>" \
        "   <faultactor>%s</faultactor>" \
        "   <detail>%s</detail>" \
        "  </SOAP-ENV:Fault>" \
        " </SOAP-ENV:Body>" \
        "</SOAP-ENV:Envelope>"

static char * fault_vm = "VersionMismatch";
static char * fault_mu = "MustUnderstand";
static char * fault_client = "Client";
static char * fault_server = "Server";

xmlDocPtr soap_fault_build(fault_code_t fcode, const char * faultstring, const char * faultactor, const char * detail)
{
	/* variables */
	char * faultcode;
	size_t bufferlen = 2000;
	char * buffer;
	xmlDocPtr fault;        /* result */
	log_verbose1("Build fault");
	switch(fcode) {
	    case Fault_VersionMismatch: faultcode = fault_vm; break;
	    case Fault_MustUnderstand: faultcode = fault_mu; break;
	    case Fault_Client: faultcode = fault_client; break;
	    case Fault_Server: faultcode = fault_server; break;
	    default: faultcode = fault_client;
	}
	/* calculate buffer length */
	if(faultstring)
		bufferlen += strlen(faultstring);
	if(faultactor)
		bufferlen += strlen(faultactor);
	if(detail)
		bufferlen += strlen(detail);
	log_verbose2("Creating buffer with %d bytes", (int)bufferlen);
	buffer = (char *)malloc(bufferlen);
	sprintf(buffer, _SOAP_FAULT_TEMPLATE_,
		soap_env_ns, soap_env_enc, soap_xsi_ns,
		soap_xsd_ns, faultcode,
		faultstring ? faultstring : "error",
		faultactor ? faultactor : "", detail ? detail : "");
	fault = xmlParseDoc(BAD_CAST buffer);
	free(buffer);
	if(fault == NULL) {
		log_error1("Can not create xml document!");
		return soap_fault_build(fcode, "Can not create fault object in xml", "soap_fault_build()", NULL);
	}
	log_verbose2("Returning fault (%p)", fault);
	return fault;
}
/*
   Parameters:
   1- soap_env_ns
   2- soap_env_enc
   3- xsi_ns
   4- xsd_ns
   3- method name
   4- uri
   5- method name(again)
 */
#define _SOAP_MSG_TEMPLATE_ \
        "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"%s\" SOAP-ENV:encodingStyle=\"%s\"" \
        " xmlns:xsi=\"%s\"" \
        " xmlns:xsd=\"%s\">" \
        " <SOAP-ENV:Header />" \
        " <SOAP-ENV:Body>" \
        "  <m:%s xmlns:m=\"%s\">" \
        "  </m:%s>" \
        " </SOAP-ENV:Body>" \
        "</SOAP-ENV:Envelope>"

/*
   Parameters:
   1- soap_env_ns
   2- soap_env_enc
   3- xsi_ns
   4- xsd_ns
   3- method name
   4- uri
   5- method name(again)
 */
#define _SOAP_MSG_TEMPLATE_EMPTY_TARGET_ \
        "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"%s\" SOAP-ENV:encodingStyle=\"%s\"" \
        " xmlns:xsi=\"%s\"" \
        " xmlns:xsd=\"%s\">" \
        " <SOAP-ENV:Header />" \
        " <SOAP-ENV:Body>" \
        "  <%s xmlns=\"%s\">" \
        "  </%s>" \
        " </SOAP-ENV:Body>" \
        "</SOAP-ENV:Envelope>"

/* ---------------------------------------------------------------------------- */
/*     XML Serializers (implemented at the and of this document)              */
/* ---------------------------------------------------------------------------- */
struct XmlNodeHolder {
	xmlNodePtr node;
};

static void xmlbuilder_start_element(const xmlChar * element_name, int attr_count, xmlChar ** keys, xmlChar ** values,
	void * userData)
{
	struct XmlNodeHolder * holder = (struct XmlNodeHolder *)userData;
	if(holder && holder->node) {
		xmlNodePtr parent = holder->node;
		if(parent)
			holder->node = xmlNewChild(parent, NULL, element_name, NULL);
	}
}

static void xmlbuilder_characters(const xmlChar * element_name, const xmlChar * chars, void * userData)
{
	struct XmlNodeHolder * holder = (struct XmlNodeHolder *)userData;
	if(holder && holder->node) {
		xmlNodePtr parent = holder->node;
		xmlNewTextChild(parent, NULL, element_name, chars);
	}
}

static void xmlbuilder_end_element(const xmlChar * element_name, void * userData)
{
	struct XmlNodeHolder * holder = (struct XmlNodeHolder *)userData;
	if(holder && holder->node) {
		xmlNodePtr parent = holder->node;
		holder->node = parent->parent;
	}
}

herror_t soap_env_new_from_doc(xmlDocPtr doc, SoapEnv ** out)
{
	xmlNodePtr node;
	SoapEnv * env;
	if(doc == NULL) {
		log_error1("Can not create xml document!");
		return herror_new("soap_env_new_from_doc", GENERAL_INVALID_PARAM, "XML Document (xmlDocPtr) is NULL");
	}
	if(!(node = xmlDocGetRootElement(doc))) {
		log_error1("XML document is empty!");
		return herror_new("soap_env_new_from_doc", XML_ERROR_EMPTY_DOCUMENT, "XML Document is empty!");
	}
	if(!(env = (SoapEnv *)malloc(sizeof(SoapEnv)))) {
		log_error2("malloc failed (%s)", strerror(errno));
		return herror_new("soap_env_from_doc", GENERAL_INVALID_PARAM, "malloc failed");
	}
	env->root = node;
	env->header = soap_env_get_header(env);
	env->body = soap_env_get_body(env);
	env->cur = soap_env_get_method(env);
	*out = env;
	return H_OK;
}

herror_t soap_env_new_from_buffer(const char * buffer, SoapEnv ** out)
{
	xmlDocPtr doc;
	herror_t err;
	if(buffer == NULL)
		return herror_new("soap_env_new_from_buffer", GENERAL_INVALID_PARAM, "buffer (first param) is NULL");
	if(!(doc = xmlParseDoc(BAD_CAST buffer)))
		return herror_new("soap_env_new_from_buffer", XML_ERROR_PARSE, "Can not parse xml");
	if((err = soap_env_new_from_doc(doc, out)) != H_OK) {
		xmlFreeDoc(doc);
	}
	return err;
}

herror_t soap_env_new_with_fault(fault_code_t faultcode, const char * faultstring, const char * faultactor, const char * detail, SoapEnv ** out)
{
	herror_t err;
	xmlDocPtr doc = soap_fault_build(faultcode, faultstring, faultactor, detail);
	if(doc == NULL)
		return herror_new("soap_env_new_with_fault", XML_ERROR_PARSE, "Can not parse fault xml");
	if((err = soap_env_new_from_doc(doc, out)) != H_OK) {
		xmlFreeDoc(doc);
	}
	return err;
}

herror_t soap_env_new_with_response(SoapEnv * request, SoapEnv ** out)
{
	char * method, * res_method;
	herror_t ret;
	char * urn;
	if(request == NULL) {
		return herror_new("soap_env_new_with_response", GENERAL_INVALID_PARAM, "request (first param) is NULL");
	}
	if(request->root == NULL) {
		return herror_new("soap_env_new_with_response",
			GENERAL_INVALID_PARAM, "request (first param) has no xml structure");
	}
	if(!(method = soap_env_find_methodname(request))) {
		return herror_new("soap_env_new_with_response",
			GENERAL_INVALID_PARAM, "Method name '%s' not found in request", SAVE_STR(method));
	}
	if(!(urn = soap_env_find_urn(request))) {

		/* here we have no chance to find out the namespace */
		/* try to continue without namespace (urn) */
		urn = "";
	}
	if(!(res_method = (char *)malloc(strlen(method)+9)))
		return herror_new("soap_env_new_with_response", GENERAL_INVALID_PARAM, "malloc failed");
	sprintf(res_method, "%sResponse", method);
	ret = soap_env_new_with_method(urn, res_method, out);
	free(res_method);
	return ret;
}

herror_t soap_env_new_with_method(const char * urn, const char * method, SoapEnv ** out)
{
	xmlDocPtr env;
	xmlChar buffer[1054];
	log_verbose2("URN = '%s'", urn);
	log_verbose2("Method = '%s'", method);
	if(!strcmp(urn, "")) {
#ifdef USE_XMLSTRING
		xmlStrPrintf(buffer, 1054, BAD_CAST _SOAP_MSG_TEMPLATE_EMPTY_TARGET_,
			soap_env_ns, soap_env_enc, soap_xsi_ns, soap_xsd_ns, BAD_CAST method, BAD_CAST urn, BAD_CAST method);
#else
		sprintf(buffer, _SOAP_MSG_TEMPLATE_EMPTY_TARGET_,
			soap_env_ns, soap_env_enc, soap_xsi_ns, soap_xsd_ns, method, urn, method);
#endif
	}
	else {
#ifdef USE_XMLSTRING
		xmlStrPrintf(buffer, 1054, BAD_CAST _SOAP_MSG_TEMPLATE_,
			soap_env_ns, soap_env_enc, soap_xsi_ns, soap_xsd_ns, BAD_CAST method, BAD_CAST urn, BAD_CAST method);
#else
		sprintf(buffer, _SOAP_MSG_TEMPLATE_, soap_env_ns, soap_env_enc, soap_xsi_ns, soap_xsd_ns, method, urn, method);
#endif
	}
	if(!(env = xmlParseDoc(buffer)))
		return herror_new("soap_env_new_with_method", XML_ERROR_PARSE, "Can not parse xml");
	return soap_env_new_from_doc(env, out);
}

static int _soap_env_xml_io_read(void * ctx, char * buffer, int len)
{
	int readed;
	http_input_stream_t * in = (http_input_stream_t *)ctx;
	if(!http_input_stream_is_ready(in))
		return 0;
	readed = http_input_stream_read(in, (byte_t *)buffer, len);
	if(readed == -1)
		return 0;
	return readed;
}

static int _soap_env_xml_io_close(void * ctx)
{
	/* do nothing */
	return 0;
}

herror_t soap_env_new_from_stream(http_input_stream_t * in, SoapEnv ** out)
{
	xmlDocPtr doc = xmlReadIO(_soap_env_xml_io_read, _soap_env_xml_io_close, in, "", NULL, 0);
	if(in->err != H_OK)
		return in->err;
	if(doc == NULL)
		return herror_new("soap_env_new_from_stream", XML_ERROR_PARSE, "Trying to parse not valid xml");
	return soap_env_new_from_doc(doc, out);
}

xmlNodePtr soap_env_add_item(SoapEnv * call, const char * type, const char * name, const char * value)
{
	xmlNodePtr newnode = xmlNewTextChild(call->cur, NULL, BAD_CAST name, BAD_CAST value);
	if(newnode == NULL) {
		log_error1("Can not create new xml node");
		return NULL;
	}
	if(type) {
		if(!xmlNewProp(newnode, BAD_CAST "xsi:type", BAD_CAST type)) {
			log_error1("Can not create new xml attribute");
			return NULL;
		}
	}
	return newnode;
}

xmlNodePtr soap_env_add_itemf(SoapEnv * call, const char * type, const char * name, const char * format, ...)
{
	va_list ap;
	char buffer[1054];
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);
	return soap_env_add_item(call, type, name, buffer);
}

xmlNodePtr soap_env_add_attachment(SoapEnv * call, const char * name, const char * href)
{
	xmlNodePtr newnode = xmlNewTextChild(call->cur, NULL, BAD_CAST name, BAD_CAST "");
	if(newnode == NULL) {
		log_error1("Can not create new xml node");
		return NULL;
	}
	if(href) {
		if(!xmlNewProp(newnode, BAD_CAST "href", BAD_CAST href)) {
			log_error1("Can not create new xml attribute");
			return NULL;
		}
	}
	return newnode;
}

void soap_env_add_custom(SoapEnv * call, void * obj, XmlSerializerCallback cb, const char * type, const char * name)
{
	struct XmlNodeHolder holder;
	holder.node = soap_env_get_method(call);
	cb(obj, BAD_CAST name, xmlbuilder_start_element, xmlbuilder_characters, xmlbuilder_end_element, &holder);
}

xmlNodePtr soap_env_push_item(SoapEnv * call, const char * type, const char * name)
{
	xmlNodePtr node;
	if((node = soap_env_add_item(call, type, name, "")))
		call->cur = node;
	return node;
}

void soap_env_pop_item(SoapEnv * call)
{
	call->cur = call->cur->parent;
}

void soap_env_free(SoapEnv * env)
{
	if(env) {
		if(env->root) {
			xmlFreeDoc(env->root->doc);
		}
		free(env);
	}
}

xmlNodePtr soap_env_get_body(SoapEnv * env)
{
	xmlNodePtr node;
	if(env == NULL) {
		log_error1("env object is NULL");
		return NULL;
	}
	if(env->root == NULL) {
		log_error1("env has no xml");
		return NULL;
	}
	for(node = soap_xml_get_children(env->root); node; node = soap_xml_get_next(node)) {
		if(!xmlStrcmp(node->name, BAD_CAST "Body") && !xmlStrcmp(node->ns->href, BAD_CAST soap_env_ns))
			return node;
	}
	log_error1("Body tag not found!");
	return NULL;
}

xmlNodePtr soap_env_get_header(SoapEnv * env)
{
	xmlNodePtr node;
	if(!env) {
		log_error1("SoapEnv is NULL");
		return NULL;
	}
	if(!env->root) {
		log_error1("SoapEnv contains no document");
		return NULL;
	}
	for(node = soap_xml_get_children(env->root); node; node = soap_xml_get_next(node)) {
		if(!xmlStrcmp(node->name, BAD_CAST "Header") && !xmlStrcmp(node->ns->href, BAD_CAST soap_env_ns))
			return node;
	}
	return NULL;
}

xmlNodePtr soap_env_get_fault(SoapEnv * env)
{
	xmlNodePtr node = soap_env_get_body(env);
	if(!node)
		return NULL;
	while(node != NULL) {
		if(!xmlStrcmp(node->name, BAD_CAST "Fault"))
			return node;
		node = soap_xml_get_next(node);
	}
/*  log_warn1 ("Node Fault tag found!");*/
	return NULL;
}

xmlNodePtr soap_env_get_method(SoapEnv * env)
{
	xmlNodePtr body;
	if(!(body = soap_env_get_body(env))) {
		log_verbose1("SoapEnv contains no Body");
		return NULL;
	}
	else
		return soap_xml_get_children(body); // method is the first child
}

/* XXX: unused function? */
xmlNodePtr _soap_env_get_body(SoapEnv * env)
{
	xmlNodePtr body;
	xmlNodeSetPtr nodeset;
	xmlXPathObjectPtr xpathobj;
	if(env == NULL) {
		log_error1("SoapEnv is NULL");
		return NULL;
	}
	if(env->root == NULL) {
		log_error1("SoapEnv contains no XML document");
		return NULL;
	}
	/*
	   find <Body> tag find out namespace xpath: //Envelope/Body/ */
	xpathobj = soap_xpath_eval(env->root->doc, "//Envelope/Body");
	if(!xpathobj) {
		log_error1("No Body (xpathobj)!");
		return NULL;
	}
	if(!(nodeset = xpathobj->nodesetval)) {
		log_error1("No Body (nodeset)!");
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}
	if(nodeset->nodeNr < 1) {
		log_error1("No Body (nodeNr)!");
		xmlXPathFreeObject(xpathobj);
		return NULL;
	}
	body = nodeset->nodeTab[0]; /* body is <Body> */
	xmlXPathFreeObject(xpathobj);
	return body;
}

char * soap_env_find_urn(SoapEnv * env)
{
	xmlNsPtr ns;
	xmlNodePtr body, node;
	if(!(body = soap_env_get_body(env))) {
		log_verbose1("body is NULL");
		return 0;
	}
	/* node is the first child */
	if(!(node = soap_xml_get_children(body))) {
		log_error1("No namespace found");
		return 0;
	}
	/* if (node->ns && node->ns->prefix) MRC 1/25/2006 */
	if(node->ns) {
		ns = xmlSearchNs(body->doc, node, node->ns->prefix);
		if(ns != NULL) {
			return (char *)ns->href; /* namespace found! */
		}
	}
	else {
		static char * empty = "";
		log_warn1("No namespace found");
		return empty;
	}
	return 0;
}

char * soap_env_find_methodname(SoapEnv * env)
{
	xmlNodePtr node;
	xmlNodePtr body = soap_env_get_body(env);
	if(body == NULL)
		return 0;
	node = soap_xml_get_children(body); /* node is the first child */
	if(node == NULL) {
		log_error1("No method found");
		return 0;
	}
	if(node->name == NULL) {
		log_error1("No methodname found");
		return 0;
	}
	return (char *)node->name;
}

SoapCtx * soap_ctx_new(SoapEnv * env)     /* should only be used internally */
{
	SoapCtx * ctx = (SoapCtx *)malloc(sizeof(SoapCtx));
	if(!ctx) {
		log_error2("malloc failed (%s)", strerror(errno));
	}
	else {
		ctx->env = env;
		ctx->attachments = NULL;
		ctx->action = NULL;
	}
	return ctx;
}

void soap_ctx_add_files(SoapCtx * ctx, attachments_t * attachments)
{
	char   href[MAX_HREF_SIZE];
	if(attachments) {
		part_t * part = attachments->parts;
		while(part) {
			soap_ctx_add_file(ctx, part->filename, part->content_type, href);
			part = part->next;
		}
	}
}

herror_t soap_ctx_add_file(SoapCtx * ctx, const char * filename, const char * content_type, char * dest_href)
{
	char   cid[250];
	char   id[250];
	part_t * part;
	static int counter = 1;
	FILE * test = fopen(filename, "r");
	if(!test)
		return herror_new("soap_ctx_add_file", FILE_ERROR_OPEN, "Can not open file '%s'", filename);
	fclose(test);
	/* generate an id */
	sprintf(id, "005512345894583%d", counter++);
	sprintf(dest_href, "cid:%s", id);
	sprintf(cid, "<%s>", id);

	/* add part to context */
	part = part_new(cid, filename, content_type, NULL, NULL);
	if(!ctx->attachments)
		ctx->attachments = attachments_new();
	attachments_add_part(ctx->attachments, part);
	return H_OK;
}

part_t * soap_ctx_get_file(SoapCtx * ctx, xmlNodePtr node)
{
	xmlChar * prop;
	char href[MAX_HREF_SIZE];
	char buffer[MAX_HREF_SIZE];
	part_t * part;
	if(!ctx->attachments)
		return NULL;
	prop = xmlGetProp(node, (const xmlChar *)"href");
	if(!prop)
		return NULL;
	strcpy(href, (const char *)prop);
	if(!strncmp(href, "cid:", 4)) {
		for(part = ctx->attachments->parts; part; part = part->next) {
			sprintf(buffer, "<%s>", href+4);
			if(!strcmp(part->id, buffer))
				return part;
		}
	}
	else {
		for(part = ctx->attachments->parts; part; part = part->next) {
			if(!strcmp(part->location, href))
				return part;
		}
	}
	return NULL;
}

void soap_ctx_free(SoapCtx * ctx)
{
	if(ctx) {
		if(ctx->attachments)
			attachments_free(ctx->attachments);
		if(ctx->env)
			soap_env_free(ctx->env);
		if(ctx->action)
			free(ctx->action);
		free(ctx);
	}
}

herror_t soap_ctx_new_with_method(const char * urn, const char * method, SoapCtx ** out)
{
	SoapEnv * env = 0;
	herror_t err = soap_env_new_with_method(urn, method, &env);
	if(err == H_OK)
		*out = soap_ctx_new(env);
	return err;
}

static herror_t _soap_client_build_result(hresponse_t * res, SoapEnv ** env)
{
	log_verbose2("Building result (%p)", res);
	if(res == NULL)
		return herror_new("_soap_client_build_result", GENERAL_INVALID_PARAM, "hresponse_t is NULL");
	if(res->in == NULL)
		return herror_new("_soap_client_build_result", GENERAL_INVALID_PARAM, "Empty response from server");
	if(res->errcode != 200 && res->errcode != 500) // @sobolev 500
		return herror_new("_soap_client_build_result", GENERAL_INVALID_PARAM, "HTTP code is not OK (%i)", res->errcode);
	return soap_env_new_from_stream(res->in, env);
}

herror_t soap_client_init_args(int argc, char * argv[])
{
	return httpc_init(argc, argv);
}

void soap_client_destroy()
{
	httpc_destroy();
}

herror_t soap_client_invoke(SoapCtx * call, SoapCtx ** response, const char * url, const char * soap_action)
{
	/* Status */
	herror_t status;
	/* Result document */
	SoapEnv * res_env;
	/* Buffer variables */
	xmlBufferPtr buffer;
	char * content;
	char tmp[15];
	/* Transport variables */
	httpc_conn_t * conn;
	hresponse_t * res;
	/* multipart/related start id */
	char start_id[150];
	static int counter = 1;
	part_t * part;
	/* for copy attachments */
	char href[MAX_HREF_SIZE];
	/* Create buffer */
	buffer = xmlBufferCreate();
	xmlNodeDump(buffer, call->env->root->doc, call->env->root, 1, 0);
	content = (char *)xmlBufferContent(buffer);
	/* Transport via HTTP */
	if(!(conn = httpc_new())) {
		return herror_new("soap_client_invoke", SOAP_ERROR_CLIENT_INIT, "Unable to create SOAP client!");
	}
	/* Set soap action */
	if(soap_action != NULL)
		httpc_set_header(conn, "SoapAction", soap_action);
	httpc_set_header(conn, HEADER_CONNECTION, "Close");
	/* check for attachments */
	if(!call->attachments) {
		/* content-type is always 'text/xml' */
		httpc_set_header(conn, HEADER_CONTENT_TYPE, "text/xml");
		sprintf(tmp, "%d", (int)strlen(content));
		httpc_set_header(conn, HEADER_CONTENT_LENGTH, tmp);
		if((status = httpc_post_begin(conn, url)) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
		if((status = http_output_stream_write_string(conn->out, content)) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
		if((status = httpc_post_end(conn, &res)) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
	}
	else {
		/* Use chunked transport */
		httpc_set_header(conn, HEADER_TRANSFER_ENCODING, TRANSFER_ENCODING_CHUNKED);
		sprintf(start_id, "289247829121218%d", counter++);
		if((status = httpc_mime_begin(conn, url, start_id, "", "text/xml")) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
		if((status = httpc_mime_next(conn, start_id, "text/xml", "binary")) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
		if((status = http_output_stream_write(conn->out, (const byte_t *)content, (int)strlen(content))) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
		for(part = call->attachments->parts; part; part = part->next) {
			status = httpc_mime_send_file(conn, part->id, part->content_type, part->transfer_encoding, part->filename);
			if(status != H_OK) {
				log_error2("Send file failed. Status:%d", status);
				httpc_close_free(conn);
				xmlBufferFree(buffer);
				return status;
			}
		}
		if((status = httpc_mime_end(conn, &res)) != H_OK) {
			httpc_close_free(conn);
			xmlBufferFree(buffer);
			return status;
		}
	}
	/* Free buffer */
	xmlBufferFree(buffer);
	/* Build result */
	if((status = _soap_client_build_result(res, &res_env)) != H_OK) {
		hresponse_free(res);
		httpc_close_free(conn);
		return status;
	}
	/* Create Context */
	*response = soap_ctx_new(res_env);
/*	soap_ctx_add_files(*response, res->attachments);*/
	if(res->attachments != NULL) {
		part = res->attachments->parts;
		while(part) {
			soap_ctx_add_file(*response, part->filename, part->content_type, href);
			part->deleteOnExit = 0;
			part = part->next;
		}
		part = (*response)->attachments->parts;
		while(part) {
			part->deleteOnExit = 1;
			part = part->next;
		}
	}
	hresponse_free(res);
	httpc_close_free(conn);
	return H_OK;
}

static void _soap_admin_send_title(httpd_conn_t * conn, const char * title)
{
	httpd_send_header(conn, 200, "OK");
	http_output_stream_write_string(conn->out, "<html><head><style>");
	http_output_stream_write_string(conn->out,
		".logo {"
		" color: #005177;"
		" background-color: transparent;"
		" font-family: Calligraphic, arial, sans-serif;"
		" font-size: 36px;"
		"}");
	http_output_stream_write_string(conn->out,
		"</style></head><body><span class=\"logo\">csoap</span> ");
	http_output_stream_write_string(conn->out, title);
	http_output_stream_write_string(conn->out, "<hr />");
}

static void _soap_admin_list_routers(httpd_conn_t * conn)
{
	SoapRouterNode * node;
	char buffer[1024];
	_soap_admin_send_title(conn, "Available routers");
	http_output_stream_write_string(conn->out, "<ul>");
	for(node = soap_server_get_routers(); node; node = node->next) {
		sprintf(buffer, "<li><a href=\"?" SOAP_ADMIN_QUERY_ROUTER "=%s\">%s</a> - <a href=\"%s\">[Service Description]</a></li>",
			node->context, node->context, node->context);
		http_output_stream_write_string(conn->out, buffer);
	}
	http_output_stream_write_string(conn->out, "</ul>");
	http_output_stream_write_string(conn->out, "</body></html>");
}

static void _soap_admin_list_services(httpd_conn_t * conn, const char * routername)
{
	SoapRouter * router;
	SoapServiceNode * node;
	char buffer[1024];
	sprintf(buffer, "Listing Services for Router <b>%s</b>", routername);
	_soap_admin_send_title(conn, buffer);
	router = soap_server_find_router(routername);
	if(!router) {
		http_output_stream_write_string(conn->out, "Router not found!");
		http_output_stream_write_string(conn->out, "</body></html>");
	}
	else {
		node = router->service_head;
		http_output_stream_write_string(conn->out, "<ul>");
		while(node) {
			sprintf(buffer, "<li> [%s] (%s) </li>", node->service->urn, node->service->method);
			http_output_stream_write_string(conn->out, buffer);
			node = node->next;
		}
		http_output_stream_write_string(conn->out, "</ul>");
		http_output_stream_write_string(conn->out, "</body></html>");
	}
}

static void _soap_admin_handle_get(httpd_conn_t * conn, hrequest_t * req)
{
	char * param;
	if((param = hpairnode_get_ignore_case(req->query, SOAP_ADMIN_QUERY_ROUTERS))) {
		_soap_admin_list_routers(conn);
	}
	else if((param = hpairnode_get_ignore_case(req->query, SOAP_ADMIN_QUERY_ROUTER))) {
		_soap_admin_list_services(conn, param);
	}
	else {
		_soap_admin_send_title(conn, "Welcome to the admin site");
		http_output_stream_write_string(conn->out, "<ul>");
		http_output_stream_write_string(conn->out, "<li><a href=\"?" SOAP_ADMIN_QUERY_ROUTERS "\">Routers</a></li>");
		http_output_stream_write_string(conn->out, "</ul>");
		http_output_stream_write_string(conn->out, "</body></html>");
	}
}

static void _soap_admin_entry(httpd_conn_t * conn, hrequest_t * req)
{
	if(req->method == HTTP_REQUEST_GET) {
		_soap_admin_handle_get(conn, req);
	}
	else {
		httpd_send_header(conn, 200, "OK");
		http_output_stream_write_string(conn->out,
			"<html>"
			"<head>"
			"</head>"
			"<body>"
			"<h1>Sorry!</h1>"
			"<hr />"
			"<div>POST Service is not implemented now. Use your browser</div>"
			"</body>"
			"</html>");
	}
}

herror_t soap_admin_init_args(int argc, char ** argv)
{
	for(int i = 0; i < argc; i++) {
		if(!strcmp(argv[i], CSOAP_ENABLE_ADMIN)) {
			httpd_register("/csoap", _soap_admin_entry);
			break;
		}
	}
	return H_OK;
}

