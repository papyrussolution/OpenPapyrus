// CSOAP.H
//
#ifndef _CSOAP_H
#define _CSOAP_H // {

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <nanohttp.h>

#define CSOAP_ENABLE_ADMIN          "-CSOAPadmin"
#define SOAP_ERROR_CLIENT_INIT      5001
#define SOAP_ERROR_NO_FILE_ATTACHED 4001
#define SOAP_ERROR_EMPTY_ATTACHMENT 4002
#define MAX_HREF_SIZE               150
/**
   The SOAP envelope object.
*/
struct SoapEnv {
	xmlNodePtr root; /** Pointer to the firts xml element (envelope) */
	xmlNodePtr header;
	xmlNodePtr body;
	xmlNodePtr cur; /** Pointer to the current xml element. (stack) */
};

struct SoapCtx {
	SoapEnv * env;
	char * action;
	hrequest_t * http;
	attachments_t * attachments;
};

enum fault_code_t {
	Fault_VersionMismatch,
	Fault_MustUnderstand,
	Fault_Client,
	Fault_Server
};

typedef herror_t (*SoapServiceFunc)(SoapCtx *, SoapCtx *);

struct SoapService {
	char * urn;
	char * method;
	SoapServiceFunc func;
};

struct SoapServiceNode {
	SoapService * service;
	SoapServiceNode * next;
};
/**
   The router object. A router can store a set of
   services. A service is a C function.
 */
struct SoapRouter {
	SoapServiceNode * service_head;
	SoapServiceNode * service_tail;
	SoapService * default_service;
	httpd_auth auth;
	xmlDocPtr wsdl;
};

struct SoapRouterNode {
	char * context;
	SoapRouter * router;
	SoapRouterNode * next;
};

static const char * const soap_env_ns = "http://schemas.xmlsoap.org/soap/envelope/";
static const char * const soap_env_enc = "http://schemas.xmlsoap.org/soap/encoding/";
static const char * const soap_xsi_ns = "http://www.w3.org/1999/XMLSchema-instance";
static const char * const soap_xsd_ns = "http://www.w3.org/1999/XMLSchema";

typedef int (*soap_xmlnode_callback)(xmlNodePtr, void *);

/**
   Initializes the soap admin HTTP interface with commandline arguments.

   @param argc commandline arg count
   @param argv commandline arg vector

   @returns 1 if success, 0 otherwise
 */
herror_t soap_admin_init_args(int argc, char * argv[]);
/**
        Initializes the client side soap engine
 */
herror_t soap_client_init_args(int argc, char * argv[]);
/**
        Destroy the soap client module
 */
void soap_client_destroy();
/**
   Establish connection to the soap server and send
   the given envelope.

   @param env envelope to send
   @param response  the result envelope
   @param url url to the soap server
   @soap_action value for "SoapAction:" in the
    HTTP request header.

    @returns H_OK if success
 */
herror_t soap_client_invoke(SoapCtx * ctx, SoapCtx ** response, const char * url, const char * soap_action);
/**
        Sets the underlaying socket to use while connecting
        into block mode or not block mode.
        The default mode is always non-blocking mode.
   @param block 1 to creat blocked sockets, 0 to create non
        blocking sockets.
 */
void soap_client_block_socket(int block);
int soap_client_get_blockmode();
SoapCtx * soap_ctx_new(SoapEnv * env);   /* should only be used internally */
/**
        Returns the attached file if any found.
        @param ctx the SoapCtx object which should contain the part
        @param node the xml node which points to a file via the "href" xml attribute

        @returns a part_t object of attachment was found, NULL otherwise.

*/
part_t * soap_ctx_get_file(SoapCtx * ctx, xmlNodePtr node);
/**
        Creates a new soap context object.
*/
herror_t soap_ctx_new_with_method(const char * urn, const char * method, SoapCtx ** out);
/*
	Size of destination dest_href should be MAX_HREF_SIZE
*/
herror_t soap_ctx_add_file(SoapCtx * ctx, const char * filename, const char * content_type, char * dest_href);
/*
        Used internally. Will switch the deleteOnExit flag from the
        given one to the added part.
 */
void soap_ctx_add_files(SoapCtx * ctx, attachments_t * attachments);
void soap_ctx_free(SoapCtx * ctx);
/* -------------------------------------------------------------- */
/*  Envelope creation methods                                     */
/* -------------------------------------------------------------- */

/**
   Creates an envelope with a fault object.

   @param faultcode The fault code @see fault_code_t
   @param faultstring A fault message
   @param faultactor The fault actor (This can be NULL)
   @param detail The detail of the error (This can be NULL)
   @param out the result envelope out parameter like follows
   @returns H_OK if success

   <pre>
   <SOAP-ENV:Envelope xmlns:SOAP-ENV="..." SOAP-ENV:encoding="..."
        xmlns:xsi="..."
        xmlns:xsd="...">
      <SOAP-ENV:Body>

         <Fault>
          <faultcode>...</faultcode>
          <faultstring>...</faultstring>
          <faultactor>...</faultactor>
          <faultdetail>..</faultdetail>
         </Fault>

      </SOAP-ENV:Body>
   </SOAP-ENV:Envelope>

   </pre>

*/
herror_t soap_env_new_with_fault(fault_code_t faultcode, const char * faultstring, const char * faultactor, const char * detail, SoapEnv ** out);
/**
   Creates an envelope with a method to invoke a soap service.
   Use this function to create a client call.

   @param urn The urn of the soap service to invoke
   @param method The method name of the soap service

   @param out the result envelope out parameter like follows
   @returns H_OK if success

   <pre>
   <SOAP-ENV:Envelope xmlns:SOAP-ENV="..." SOAP-ENV:encoding="..."
        xmlns:xsi="..."
        xmlns:xsd="...">
      <SOAP-ENV:Body>

       <m:[method] xmlns:m="[urn]">
       </m:[method]>

      </SOAP-ENV:Body>
   </SOAP-ENV:Envelope>

   </pre>

*/
herror_t soap_env_new_with_method(const char * urn, const char * method, SoapEnv ** out);
/**
   Creates a soap envelope with a response.
   Use this function to create a response envelope object
   for a request. This function is only relevant for soap
   service implementors.

   @see example csoap/simpleserver.c

   @param req The request object. A response object will be created
    to this request.

   @param out the result envelope out paramter like follows
   @returns H_OK if success

   <pre>
   <SOAP-ENV:Envelope xmlns:SOAP-ENV="..." SOAP-ENV:encoding="..."
        xmlns:xsi="..."
        xmlns:xsd="...">
      <SOAP-ENV:Body>

       <m:[req-method]Response xmlns:m="[req-urn]">
       </m:[req-method]>

      </SOAP-ENV:Body>
   </SOAP-ENV:Envelope>

   </pre>


*/
herror_t soap_env_new_with_response(SoapEnv * req, SoapEnv ** out);
/**
   Creates an envelope from a given libxml2 xmlDoc
   pointer.

   @param doc the xml document pointer
   @param out the output envelope object
   @returns H_OK if success

*/
herror_t soap_env_new_from_doc(xmlDocPtr doc, SoapEnv ** out);
/**
   Create an envelop object from a string.
   The string must be in xml format.

   @param buffer The string to parse into a envelope.
   @param out the output envelope object
   @returns H_OK if success
*/
herror_t soap_env_new_from_buffer(const char * buffer, SoapEnv ** out);
/**
   Create an envelope from input stream

   @param in the input stream object to read from
   @param out the output envelope object
   @returns H_OK if success
*/
herror_t soap_env_new_from_stream(http_input_stream_t * in, SoapEnv ** out);
/* --------------------------------------------------- */
/*      XML Serializer functions  and typedefs         */
/* --------------------------------------------------- */
typedef void (*XmlSerializerCallback)(void * obj, const xmlChar * root_element_name, void (* OnStartElement)(const xmlChar * element_name, int attr_count,
	xmlChar ** keys, xmlChar ** values, void * userData), void (* OnCharacters)(const xmlChar * element_name, const xmlChar * chars, void * userData),
	void (* OnEndElement)(const xmlChar * element_name, void * userData), void * userdata);

/* ------------------------------------------------------ */
/*     XML build and stack function                       */
/* ------------------------------------------------------ */

/**
   Adds a new xml node under the current parent.

   <pre>
    <m:[name] type=[type]>[value]</m:[name]>
   </pre>

   @param env The envelope object
   @param type Type of the parameter. Something like "xsd:string" or
    "xsd:int" or custom types.
   @param name Name of the xml node
   @param value Text value of the xml node

   @returns The added xmlNode pointer.

   @see tutorial
 */
xmlNodePtr soap_env_add_item(SoapEnv * env, const char * type, const char * name, const char * value);
/**
   Adds attachment href node to the envelope current parent.

   <pre>
    <m:[name] href=[href]/>
   </pre>

   @param env The envelope object
   @param name Name of the xml node
   @param href href. A CID string filled by
     soap_ctx_add_attachment()

   @returns The added xmlNode pointer.

   @see soap_ctx_add_file tutorial
 */
xmlNodePtr soap_env_add_attachment(SoapEnv * env, const char * name, const char * href);
/**
   Serialize and adds obj to the envelope.
   TODO: Document this function !
   <br>
   <b>Important: </b>

 */
void soap_env_add_custom(SoapEnv * env, void * obj, XmlSerializerCallback cb, const char * type, const char * name);
/**
   Same as soap_env_add_item() with c style arguments
   like in printf(). "value" is the format string.
   <br>
   <b>Important: </b> The totally length of value (incl. args)
    must be lower the 1054.

   @see soap_env_add_item
 */
xmlNodePtr soap_env_add_itemf(SoapEnv * env, const char * type, const char * name, const char * value, ...);
/**
   Push the current xml node in the soap envelope one level
   deeper. Here an example:

   <pre>
   soap_env_push_item(env, "my:custom", "Person");
    soap_env_add_item(env, "xsd:string", "name", "Mickey");
    soap_env_add_item(env, "xsd:string", "lastname", "Mouse");
   soap_env_pop_item(env);
   </pre>

   This will create the xml like follows.

   <pre>
   <Person type="my:custom">
    <name>Mickey</name>
    <lastname>Mouse</lastname>
   </Person>
   </pre>

   @returns The added xmlNode pointer.

   @see tutorial
 */
xmlNodePtr soap_env_push_item(SoapEnv * env, const char * type, const char * name);
/**
   Sets the xml pointer 1 level higher.

   @param env The envelope object
   @see soap_env_push_item
 */
void soap_env_pop_item(SoapEnv * env);
/**
   Free the envelope.

   @param env The envelope object
*/
void soap_env_free(SoapEnv * env);

/* --------------------------------------------------- */
/*      XML node finder functions                      */
/* --------------------------------------------------- */

/**
   Gets the xml node pointing to SOAP Body.
 */
xmlNodePtr soap_env_get_body(SoapEnv * env);
/**
   Get the xml node pointing to SOAP method (call)
 */
xmlNodePtr soap_env_get_method(SoapEnv * env);
/**
   Get the xml node pointing to SOAP Fault
 */
xmlNodePtr soap_env_get_fault(SoapEnv * env);
/**
   Get the xml node pointing to SOAP Header
 */
xmlNodePtr soap_env_get_header(SoapEnv * env);
char * soap_env_find_urn(SoapEnv * env);
char * soap_env_find_methodname(SoapEnv * env);
xmlDocPtr soap_fault_build(fault_code_t faultcode, const char * faultstring, const char * faultactor, const char * detail);
/**
   Creates a new router object. Create a router if
   you are implementing a soap server. Then register
   the services to this router.
   <P>A router points also to http url context.

   @returns Soap router
   @see soap_router_free
 */
SoapRouter * soap_router_new();
/**
   Registers a SOAP service (in this case a C function)
   to the router.

   @param router The router object
   @param func Function to register as a soap service
   @param method Method name to call the function from
    the client side.
   @param urn The urn for this service
*/
void soap_router_register_service(SoapRouter * router, SoapServiceFunc func, const char * method, const char * urn);
void soap_router_register_default_service(SoapRouter * router, SoapServiceFunc func, const char * method, const char * urn);
void soap_router_register_description(SoapRouter * router, xmlDocPtr doc);
void soap_router_register_security(SoapRouter * router, httpd_auth auth);
/**
   Searches for a registered soap service.

   @param router The router object
   @param urn URN of the service
   @param method The name under which the service was registered.

   @return The service if found, NULL otherwise.
*/
SoapService * soap_router_find_service(SoapRouter * router, const char * urn, const char * method);
/**
   Frees the router object.

   @param router The router object to free
*/
void soap_router_free(SoapRouter * router);
/**
   Initializes the soap server with commandline arguments.

   <TABLE border=1>
   <TR><TH>Argument</TH><TH>Description</TH></TR>
   <TR><TD>-NHTTPport [port]</TD><TD>Port to listen (default: 10000)</TD></TR>
   <TR><TD>-NHTTPmaxconn [num]</TD><TD>Maximum thread connections</TD></TR>
   <TR><TD>-NHTTPlog [logfilename]</TD><TD>logfile</TD></TR>
   </TABLE>

   @param argc commandline arg count
   @param argv commandline arg vector

   @returns 1 if success, 0 otherwise
*/
herror_t soap_server_init_args(int argc, char * argv[]);
/**
   Register a router to the soap server.

   <P>http://<I>host</I>:<I>port</I>/<B>[context]</B>


   @param router The router to register
   @param context the url context
   @returns 1 if success, 0 otherwise

   @see soap_router_new
   @see soap_router_register_service

*/
int soap_server_register_router(SoapRouter * router, const char * context);
SoapRouter * soap_server_find_router(const char * context);
SoapRouterNode * soap_server_get_routers();
/**
   Enters the server loop and starts to listen to
   http requests.
 */
herror_t soap_server_run();
int soap_server_get_port();
/**
   Frees the soap server.
 */
void soap_server_destroy();
SoapServiceNode * soap_service_node_new(SoapService * service, SoapServiceNode * next);
SoapService * soap_service_new(const char * urn, const char * method, SoapServiceFunc f);
void soap_service_free(SoapService * service);
xmlNodePtr soap_xml_get_children(xmlNodePtr param);
xmlNodePtr soap_xml_get_next(xmlNodePtr param);
xmlXPathObjectPtr soap_xpath_eval(xmlDocPtr doc, const char * xpath);
int soap_xpath_foreach(xmlDocPtr doc, const char * xpath, soap_xmlnode_callback cb, void * userdata);
void soap_xml_doc_print(xmlDocPtr doc);
char * soap_xml_get_text(xmlNodePtr node);

#endif // } _CSOAP_H
