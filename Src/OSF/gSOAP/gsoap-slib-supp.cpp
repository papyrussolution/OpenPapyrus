// GSOAP-SLIB-SUPP.CPP
//
#include <slib.h>
#include <stdsoap2.h>

#ifndef SOAP_TYPE_string
	#define SOAP_TYPE_string (4)
#endif

/*
SOAP_FMAC2 soap_ssl_client_context(struct soap * soap, unsigned short flags, const char * keyfile,
	const char * password, const char * cafile, const char * capath,
	const char * randfile)
*/

#if 0 // {
//
// Образец функции DllMain испольуемой в dll-модулях клиентах SOAP-сервисов
//
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) 
{ 
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
			{
				SString product_name;
				(product_name = "Papyrus SoapModule");
				SLS.Init(product_name, (HINSTANCE)hModule);
			}
			break;
#ifdef _MT
		case DLL_THREAD_ATTACH:
			SLS.InitThread();
			break;
		case DLL_THREAD_DETACH:
			SLS.ReleaseThread();
			break;
#endif
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#endif // } 0

int SLAPI gSoapSslClientContextInit(struct soap * pSoap, const char * pKeyfile)
{
	int    ok = 1;
	soap_ssl_init();
	const SGlobalSecureConfig & r_cfg = SLS.GetGlobalSecureConfig();
	long  soaperr = soap_ssl_client_context(pSoap,
		SOAP_SSL_REQUIRE_CLIENT_AUTHENTICATION|
		SOAP_SSL_REQUIRE_SERVER_AUTHENTICATION|SOAP_SSL_SKIP_HOST_CHECK|SOAP_SSL_ALLOW_EXPIRED_CERTIFICATE,
		0/*pKeyfile*/, /* keyfile: required only when client must authenticate to server (see SSL docs on how to obtain this file) */
		0, /* password to read the key file (not used with GNUTLS) */
		r_cfg.CaFile, /* cacert file to store trusted certificates (needed to verify server) */
		r_cfg.CaPath, /* capath to directory with trusted certificates */
		0 /* if randfile!=NULL: use a file with random data to seed randomness */);
	if(soaperr != SOAP_OK) {
		SString msg_buf;
		(msg_buf = "Error on soap_ssl_client_context").CatDiv(':', 2).CatEq("errcode", soaperr).Space().CatEq("CaFile", r_cfg.CaFile).Space().
			CatEq("CaPath", r_cfg.CaPath).Space().CatEq("CaFileExists", (long)fileExists(r_cfg.CaFile));
		SLS.LogMessage(0, msg_buf);
		ok = 0;
	}
	return ok;
}

int SLAPI gSoapClientInit(struct soap * pSoap, long flags, const char * pKeyfile)
{
	int    ok = 1;
	soap_set_imode(pSoap, SOAP_C_UTFSTRING);
	soap_set_omode(pSoap, SOAP_C_UTFSTRING);
	if(!gSoapSslClientContextInit(pSoap, pKeyfile))
		ok = 0;
	return ok;
}

#if 0 // {

SOAP_FMAC3 char * * SOAP_FMAC4 soap_in_string(struct soap *soap, const char *tag, char **a, const char *type)
{	
	char ** p = soap_instring(soap, tag, a, type, SOAP_TYPE_string, 1, 0, -1);
	return p;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putindependent(struct soap *soap)
{
	int i;
	struct soap_plist *pp;
	if(soap->version == 1 && soap->encodingStyle && !(soap->mode & (SOAP_XML_TREE | SOAP_XML_GRAPH)))
		for(i = 0; i < SOAP_PTRHASH; i++)
			for(pp = soap->pht[i]; pp; pp = pp->next)
				if(pp->mark1 == 2 || pp->mark2 == 2)
					if(soap_putelement(soap, pp->ptr, "id", pp->id, pp->type))
						return soap->error;
	return SOAP_OK;
}

#define soap_new_SOAP_ENV__Header(soap, n) soap_instantiate_SOAP_ENV__Header(soap, n, NULL, NULL, NULL)
#define soap_new_SOAP_ENV__Fault(soap, n)  soap_instantiate_SOAP_ENV__Fault(soap, n, NULL, NULL, NULL)

extern SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Header(struct soap * soap, struct SOAP_ENV__Header * a);
extern SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Header(struct soap * soap, const struct SOAP_ENV__Header * a);
extern SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Header(struct soap * soap, const char * tag, int id, const struct SOAP_ENV__Header * a, const char * type);
extern SOAP_FMAC3 struct SOAP_ENV__Header * SOAP_FMAC4 soap_in_SOAP_ENV__Header(struct soap * soap, const char * tag, struct SOAP_ENV__Header * a, const char * type);
extern SOAP_FMAC1 struct SOAP_ENV__Header * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Header(struct soap *soap, int n, const char *type, const char *arrayType, size_t *size);
extern SOAP_FMAC1 struct SOAP_ENV__Fault * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Fault(struct soap *soap, int n, const char *type, const char *arrayType, size_t *size);

SOAP_FMAC3 void SOAP_FMAC4 soap_default_SOAP_ENV__Fault(struct soap *soap, struct SOAP_ENV__Fault *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_default__QName(soap, &a->faultcode);
	soap_default_string(soap, &a->faultstring);
	soap_default_string(soap, &a->faultactor);
	a->detail = NULL;
	a->SOAP_ENV__Code = NULL;
	a->SOAP_ENV__Reason = NULL;
	soap_default_string(soap, &a->SOAP_ENV__Node);
	soap_default_string(soap, &a->SOAP_ENV__Role);
	a->SOAP_ENV__Detail = NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serialize_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a)
{
	(void)soap; (void)a; /* appease -Wall -Werror */
	soap_serialize__QName(soap, &a->faultcode);
	soap_serialize_string(soap, &a->faultstring);
	soap_serialize_string(soap, &a->faultactor);
	soap_serialize_PointerToSOAP_ENV__Detail(soap, &a->detail);
	soap_serialize_PointerToSOAP_ENV__Code(soap, &a->SOAP_ENV__Code);
	soap_serialize_PointerToSOAP_ENV__Reason(soap, &a->SOAP_ENV__Reason);
	soap_serialize_string(soap, &a->SOAP_ENV__Node);
	soap_serialize_string(soap, &a->SOAP_ENV__Role);
	soap_serialize_PointerToSOAP_ENV__Detail(soap, &a->SOAP_ENV__Detail);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_out_SOAP_ENV__Fault(struct soap *soap, const char *tag, int id, const struct SOAP_ENV__Fault *a, const char *type)
{
	const char *soap_tmp_faultcode = soap_QName2s(soap, a->faultcode);
	(void)soap; (void)tag; (void)id; (void)type;
	if(soap_element_begin_out(soap, tag, soap_embedded_id(soap, id, a, SOAP_TYPE_SOAP_ENV__Fault), type))
		return soap->error;
	if(soap_out__QName(soap, "faultcode", -1, (char*const*)&soap_tmp_faultcode, ""))
		return soap->error;
	if(soap_out_string(soap, "faultstring", -1, &a->faultstring, ""))
		return soap->error;
	if(soap_out_string(soap, "faultactor", -1, &a->faultactor, ""))
		return soap->error;
	if(soap_out_PointerToSOAP_ENV__Detail(soap, "detail", -1, &a->detail, ""))
		return soap->error;
	if(soap_out_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Code", -1, &a->SOAP_ENV__Code, ""))
		return soap->error;
	if(soap_out_PointerToSOAP_ENV__Reason(soap, "SOAP-ENV:Reason", -1, &a->SOAP_ENV__Reason, ""))
		return soap->error;
	if(soap_out_string(soap, "SOAP-ENV:Node", -1, &a->SOAP_ENV__Node, ""))
		return soap->error;
	if(soap_out_string(soap, "SOAP-ENV:Role", -1, &a->SOAP_ENV__Role, ""))
		return soap->error;
	if(soap_out_PointerToSOAP_ENV__Detail(soap, "SOAP-ENV:Detail", -1, &a->SOAP_ENV__Detail, ""))
		return soap->error;
	return soap_element_end_out(soap, tag);
}

SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_in_SOAP_ENV__Fault(struct soap *soap, const char *tag, struct SOAP_ENV__Fault *a, const char *type)
{
	size_t soap_flag_faultcode = 1;
	size_t soap_flag_faultstring = 1;
	size_t soap_flag_faultactor = 1;
	size_t soap_flag_detail = 1;
	size_t soap_flag_SOAP_ENV__Code = 1;
	size_t soap_flag_SOAP_ENV__Reason = 1;
	size_t soap_flag_SOAP_ENV__Node = 1;
	size_t soap_flag_SOAP_ENV__Role = 1;
	size_t soap_flag_SOAP_ENV__Detail = 1;
	if(soap_element_begin_in(soap, tag, 0, type))
		return NULL;
	a = (struct SOAP_ENV__Fault *)soap_id_enter(soap, soap->id, a, SOAP_TYPE_SOAP_ENV__Fault, sizeof(struct SOAP_ENV__Fault), 0, NULL, NULL, NULL);
	if(!a)
		return NULL;
	soap_default_SOAP_ENV__Fault(soap, a);
	if(soap->body && !*soap->href)
	{
		for(;;)
		{	soap->error = SOAP_TAG_MISMATCH;
			if(soap_flag_faultcode && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if(soap_in__QName(soap, "faultcode", &a->faultcode, ""))
				{	soap_flag_faultcode--;
					continue;
				}
			if(soap_flag_faultstring && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if(soap_in_string(soap, "faultstring", &a->faultstring, "xsd:string"))
				{	soap_flag_faultstring--;
					continue;
				}
			if(soap_flag_faultactor && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if(soap_in_string(soap, "faultactor", &a->faultactor, "xsd:string"))
				{	soap_flag_faultactor--;
					continue;
				}
			if(soap_flag_detail && soap->error == SOAP_TAG_MISMATCH)
				if(soap_in_PointerToSOAP_ENV__Detail(soap, "detail", &a->detail, ""))
				{	soap_flag_detail--;
					continue;
				}
			if(soap_flag_SOAP_ENV__Code && soap->error == SOAP_TAG_MISMATCH)
				if(soap_in_PointerToSOAP_ENV__Code(soap, "SOAP-ENV:Code", &a->SOAP_ENV__Code, ""))
				{	soap_flag_SOAP_ENV__Code--;
					continue;
				}
			if(soap_flag_SOAP_ENV__Reason && soap->error == SOAP_TAG_MISMATCH)
				if(soap_in_PointerToSOAP_ENV__Reason(soap, "SOAP-ENV:Reason", &a->SOAP_ENV__Reason, ""))
				{	soap_flag_SOAP_ENV__Reason--;
					continue;
				}
			if(soap_flag_SOAP_ENV__Node && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if(soap_in_string(soap, "SOAP-ENV:Node", &a->SOAP_ENV__Node, "xsd:string"))
				{	soap_flag_SOAP_ENV__Node--;
					continue;
				}
			if(soap_flag_SOAP_ENV__Role && (soap->error == SOAP_TAG_MISMATCH || soap->error == SOAP_NO_TAG))
				if(soap_in_string(soap, "SOAP-ENV:Role", &a->SOAP_ENV__Role, "xsd:string"))
				{	soap_flag_SOAP_ENV__Role--;
					continue;
				}
			if(soap_flag_SOAP_ENV__Detail && soap->error == SOAP_TAG_MISMATCH)
				if(soap_in_PointerToSOAP_ENV__Detail(soap, "SOAP-ENV:Detail", &a->SOAP_ENV__Detail, ""))
				{	soap_flag_SOAP_ENV__Detail--;
					continue;
				}
			if(soap->error == SOAP_TAG_MISMATCH)
				soap->error = soap_ignore_element(soap);
			if(soap->error == SOAP_NO_TAG)
				break;
			if(soap->error)
				return NULL;
		}
		if(soap_element_end_in(soap, tag))
			return NULL;
	}
	else
	{	a = (struct SOAP_ENV__Fault *)soap_id_forward(soap, soap->href, (void*)a, 0, SOAP_TYPE_SOAP_ENV__Fault, 0, sizeof(struct SOAP_ENV__Fault), 0, NULL);
		if(soap->body && soap_element_end_in(soap, tag))
			return NULL;
	}
	return a;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_put_SOAP_ENV__Fault(struct soap *soap, const struct SOAP_ENV__Fault *a, const char *tag, const char *type)
{
	register int id = soap_embed(soap, (void*)a, NULL, 0, tag, SOAP_TYPE_SOAP_ENV__Fault);
	if(soap_out_SOAP_ENV__Fault(soap, tag?tag:"SOAP-ENV:Fault", id, a, type))
		return soap->error;
	return soap_putindependent(soap);
}

SOAP_FMAC3 struct SOAP_ENV__Fault * SOAP_FMAC4 soap_get_SOAP_ENV__Fault(struct soap *soap, struct SOAP_ENV__Fault *p, const char *tag, const char *type)
{
	if((p = soap_in_SOAP_ENV__Fault(soap, tag, p, type)))
		if(soap_getindependent(soap))
			return NULL;
	return p;
}

SOAP_FMAC1 struct SOAP_ENV__Fault * SOAP_FMAC2 soap_instantiate_SOAP_ENV__Fault(struct soap *soap, int n, const char *type, const char *arrayType, size_t *size)
{
	(void)type; (void)arrayType; /* appease -Wall -Werror */
	DBGLOG(TEST, SOAP_MESSAGE(fdebug, "soap_instantiate_SOAP_ENV__Fault(%d, %s, %s)\n", n, type?type:"", arrayType?arrayType:""));
	struct soap_clist *cp = soap_link(soap, NULL, SOAP_TYPE_SOAP_ENV__Fault, n, uhttGoods_fdelete);
	if(!cp)
		return NULL;
	if(n < 0)
	{	cp->ptr = (void*)SOAP_NEW(struct SOAP_ENV__Fault);
		if(size)
			*size = sizeof(struct SOAP_ENV__Fault);
	}
	else
	{	cp->ptr = (void*)SOAP_NEW(struct SOAP_ENV__Fault[n]);
		if(!cp->ptr)
		{	soap->error = SOAP_EOM;
			return NULL;
		}
		if(size)
			*size = n * sizeof(struct SOAP_ENV__Fault);
	}
	DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Instantiated location=%p\n", cp->ptr));
	return (struct SOAP_ENV__Fault*)cp->ptr;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_copy_SOAP_ENV__Fault(struct soap *soap, int st, int tt, void *p, size_t len, const void *q, size_t n)
{
	(void)soap; (void)tt; (void)st; (void)len; (void)n; /* appease -Wall -Werror */
	DBGLOG(TEST, SOAP_MESSAGE(fdebug, "Copying struct SOAP_ENV__Fault %p -> %p\n", q, p));
	*(struct SOAP_ENV__Fault*)p = *(struct SOAP_ENV__Fault*)q;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serializeheader(struct soap * soap)
{
	if(soap->header)
		soap_serialize_SOAP_ENV__Header(soap, soap->header);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putheader(struct soap * soap)
{
	if(soap->header) {
		soap->part = SOAP_IN_HEADER;
		if(soap_out_SOAP_ENV__Header(soap, "SOAP-ENV:Header", 0, soap->header, NULL))
			return soap->error;
		soap->part = SOAP_END_HEADER;
	}
	return SOAP_OK;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_getheader(struct soap * soap)
{
	soap->part = SOAP_IN_HEADER;
	soap->header = soap_in_SOAP_ENV__Header(soap, "SOAP-ENV:Header", NULL, NULL);
	soap->part = SOAP_END_HEADER;
	return soap->header == NULL;
}

SOAP_FMAC3 void SOAP_FMAC4 soap_header(struct soap * soap)
{
	if(!soap->header) {
		if((soap->header = soap_new_SOAP_ENV__Header(soap, -1)))
			soap_default_SOAP_ENV__Header(soap, soap->header);
	}
}

SOAP_FMAC3 void SOAP_FMAC4 soap_fault(struct soap * soap)
{
	if(!soap->fault) {
		soap->fault = soap_new_SOAP_ENV__Fault(soap, -1);
		if(!soap->fault)
			return;
		soap_default_SOAP_ENV__Fault(soap, soap->fault);
	}
	if(soap->version == 2 && !soap->fault->SOAP_ENV__Code) {
		soap->fault->SOAP_ENV__Code = soap_new_SOAP_ENV__Code(soap, -1);
		soap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code);
	}
	if(soap->version == 2 && !soap->fault->SOAP_ENV__Reason) {
		soap->fault->SOAP_ENV__Reason = soap_new_SOAP_ENV__Reason(soap, -1);
		soap_default_SOAP_ENV__Reason(soap, soap->fault->SOAP_ENV__Reason);
	}
}

SOAP_FMAC3 void SOAP_FMAC4 soap_serializefault(struct soap * soap)
{
	soap_fault(soap);
	if(soap->fault)
		soap_serialize_SOAP_ENV__Fault(soap, soap->fault);
}

SOAP_FMAC3 int SOAP_FMAC4 soap_putfault(struct soap * soap)
{
	if(soap->fault)
		return soap_put_SOAP_ENV__Fault(soap, soap->fault, "SOAP-ENV:Fault", NULL);
	return SOAP_OK;
}

SOAP_FMAC3 int SOAP_FMAC4 soap_getfault(struct soap * soap)
{
	return (soap->fault = soap_get_SOAP_ENV__Fault(soap, NULL, "SOAP-ENV:Fault", NULL)) == NULL;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultcode(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 2 && soap->fault->SOAP_ENV__Code)
		return (const char **)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Value;
	return (const char **)&soap->fault->faultcode;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultsubcode(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 2) {
		if(!soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode) {
			soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = soap_new_SOAP_ENV__Code(soap, -1);
			soap_default_SOAP_ENV__Code(soap, soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode);
		}
		return (const char **)&soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;
	}
	return (const char **)&soap->fault->faultcode;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultsubcode(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 2) {
		if(soap->fault->SOAP_ENV__Code && soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode &&
		   soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode)
			return soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode->SOAP_ENV__Value;
		return NULL;
	}
	return soap->fault->faultcode;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultstring(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 2)
		return (const char **)&soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text;
	return (const char **)&soap->fault->faultstring;
}

SOAP_FMAC3 const char ** SOAP_FMAC4 soap_faultdetail(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 1) {
		if(!soap->fault->detail) {
			soap->fault->detail =
			        (struct SOAP_ENV__Detail *)soap_malloc(soap, sizeof(struct SOAP_ENV__Detail));
			soap_default_SOAP_ENV__Detail(soap, soap->fault->detail);
		}
		return (const char **)&soap->fault->detail->__any;
	}
	if(!soap->fault->SOAP_ENV__Detail) {
		soap->fault->SOAP_ENV__Detail = soap_new_SOAP_ENV__Detail(soap, -1);
		soap_default_SOAP_ENV__Detail(soap, soap->fault->SOAP_ENV__Detail);
	}
	return (const char **)&soap->fault->SOAP_ENV__Detail->__any;
}

SOAP_FMAC3 const char * SOAP_FMAC4 soap_check_faultdetail(struct soap * soap)
{
	soap_fault(soap);
	if(soap->version == 2 && soap->fault->SOAP_ENV__Detail)
		return soap->fault->SOAP_ENV__Detail->__any;
	if(soap->fault->detail)
		return soap->fault->detail->__any;
	return NULL;
}

#endif // } 0