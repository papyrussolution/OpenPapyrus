/*
        schema.cpp

        XSD binding schema implementation

   --------------------------------------------------------------------------------
   gSOAP XML Web services tools
   Copyright (C) 2001-2011, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This software is released under one of the following two licenses:
   GPL or Genivia's license for commercial use.
   --------------------------------------------------------------------------------
   GPL license.

   This program is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free Software
   Foundation; either version 2 of the License, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE. See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple
   Place, Suite 330, Boston, MA 02111-1307 USA

   Author contact information:
   engelen@genivia.com / engelen@acm.org
   --------------------------------------------------------------------------------
   A commercial use license is available from Genivia, Inc., contact@genivia.com
   --------------------------------------------------------------------------------

 */
#include <slib.h> // @v9.6.8
#include "wsdlH.h"              // cannot include "schemaH.h"
#pragma hdrstop
#include "includes.h"

extern struct Namespace namespaces[];

extern "C" {
extern int warn_ignore(struct soap*, const char*);
}

extern const char * qname_token(const char*, const char*);
extern int is_builtin_qname(const char*);

////////////////////////////////////////////////////////////////////////////////
//
//	schema
//
////////////////////////////////////////////////////////////////////////////////

xs__schema::xs__schema()
{
	soap = soap_new1(SOAP_XML_TREE | SOAP_C_UTFSTRING);
#ifdef WITH_OPENSSL
	soap_ssl_client_context(soap, SOAP_SSL_NO_AUTHENTICATION, NULL, NULL, NULL, NULL, NULL);
#endif
	soap_set_namespaces(soap, namespaces);
	soap_default(soap);
	soap->fignore = warn_ignore;
	soap->encodingStyle = NULL;
	soap->proxy_host = proxy_host;
	soap->proxy_port = proxy_port;
	soap->proxy_userid = proxy_userid;
	soap->proxy_passwd = proxy_passwd;
	targetNamespace = NULL;
	version = NULL;
	updated = false;
	location = NULL;
	redirs = 0;
}

xs__schema::xs__schema(struct soap * copy)
{
	soap = soap_copy(copy);
	soap->socket = SOAP_INVALID_SOCKET;
	soap->recvfd = 0;
	soap->sendfd = 1;
	soap_default(soap);
	soap->fignore = warn_ignore;
	soap->encodingStyle = NULL;
	targetNamespace = NULL;
	version = NULL;
	updated = false;
	location = NULL;
	redirs = 0;
}

xs__schema::xs__schema(struct soap * copy, const char * cwd, const char * loc)
{
	soap = soap_copy(copy);
	soap->socket = SOAP_INVALID_SOCKET;
	soap->recvfd = 0;
	soap->sendfd = 1;
	/* no longer required, since we keep the host name:
	   strcpy(soap->host, copy->host);
	 */
	soap_default(soap);
	soap->fignore = warn_ignore;
	soap->encodingStyle = NULL;
	targetNamespace = NULL;
	version = NULL;
	updated = false;
	location = NULL;
	redirs = 0;
	read(cwd, loc);
}

xs__schema::~xs__schema()
{
}

int xs__schema::get(struct soap * soap)
{
	return preprocess();
}

int xs__schema::preprocess()
{ // process xs:include recursively
	// NOTE: includes are context sensitive (take context info), so keep including
	for(vector<xs__include>::iterator in = include.begin(); in != include.end(); ++in) {
		(*in).preprocess(*this); // read schema and recurse over <include>
		if((*in).schemaPtr())
			insert(*(*in).schemaPtr());
	}
	for(vector<xs__redefine>::iterator re = redefine.begin(); re != redefine.end(); ++re) {
		(*re).preprocess(*this); // read schema and recurse over <redefine>
		if((*re).schemaPtr())
			insert(*(*re).schemaPtr());
	}
	return SOAP_OK;
}

int xs__schema::insert(xs__schema& schema)
{
	bool found;
	if(targetNamespace && schema.targetNamespace && strcmp(targetNamespace, schema.targetNamespace))
		if(!Wflag)
			slfprintf_stderr("Warning: attempt to include schema with mismatching targetNamespace '%s' in schema '%s'\n", schema.targetNamespace, targetNamespace);
	if(elementFormDefault != schema.elementFormDefault)
		if(!Wflag)
			slfprintf_stderr("Warning: attempt to include schema with mismatching elementFormDefault in schema '%s'\n", targetNamespace ? targetNamespace : "");
	if(attributeFormDefault != schema.attributeFormDefault)
		if(!Wflag)
			slfprintf_stderr("Warning: attempt to include schema with mismatching attributeFormDefault in schema '%s'\n", targetNamespace ? targetNamespace : "");
	// insert imports, but only add imports with new namespace
	for(vector<xs__import>::const_iterator im = schema.import.begin(); im != schema.import.end(); ++im) {
		found = false;
		if((*im).namespace_) {
			for(vector<xs__import>::const_iterator i = import.begin(); i != import.end(); ++i) {
				if((*i).namespace_ && !strcmp((*im).namespace_, (*i).namespace_)) {
					found = true;
					break;
				}
			}
		}
		if(!found)
			import.push_back(*im);
	}
	// insert attributes, but only add attributes with new name (limited conflict check)
	for(vector<xs__attribute>::const_iterator at = schema.attribute.begin(); at != schema.attribute.end(); ++at) {
		found = false;
		if((*at).name) {
			for(vector<xs__attribute>::const_iterator a = attribute.begin(); a != attribute.end(); ++a) {
				if((*a).name && !strcmp((*at).name, (*a).name)) {
					found = true;
					if((*at).type && (*a).type && strcmp((*at).type, (*a).type))
						if(!Wflag)
							slfprintf_stderr("Warning: attempt to redefine attribute '%s' with type '%s' in schema '%s'\n",
							    (*at).name, (*at).type, targetNamespace ? targetNamespace : "");
					break;
				}
			}
		}
		if(!found) {
			attribute.push_back(*at);
			attribute.back().schemaPtr(this);
		}
	}
	// insert elements, but only add elements with new name (limited conflict check)
	for(vector<xs__element>::const_iterator el = schema.element.begin(); el != schema.element.end(); ++el) {
		found = false;
		if((*el).name) {
			for(vector<xs__element>::const_iterator e = element.begin(); e != element.end(); ++e) {
				if((*e).name && !strcmp((*el).name, (*e).name)) {
					found = true;
					if((*el).type && (*e).type && strcmp((*el).type, (*e).type))
						if(!Wflag)
							slfprintf_stderr("Warning: attempt to redefine element '%s' with type '%s' in schema '%s'\n", (*el).name, (*el).type, targetNamespace ? targetNamespace : "");
					break;
				}
			}
		}
		if(!found) {
			element.push_back(*el);
			element.back().schemaPtr(this);
		}
	}
	// insert groups, but only add groups with new name (no conflict check)
	for(vector<xs__group>::const_iterator gp = schema.group.begin(); gp != schema.group.end(); ++gp) {
		found = false;
		if((*gp).name) {
			for(vector<xs__group>::const_iterator g = group.begin(); g != group.end(); ++g) {
				if((*g).name && !strcmp((*gp).name, (*g).name)) {
					found = true;
					break;
				}
			}
		}
		if(!found) {
			group.push_back(*gp);
			group.back().schemaPtr(this);
		}
	}
	// insert attributeGroups, but only add attributeGroups with new name (no conflict check)
	for(vector<xs__attributeGroup>::const_iterator ag = schema.attributeGroup.begin(); ag != schema.attributeGroup.end(); ++ag) {
		found = false;
		if((*ag).name) {
			for(vector<xs__attributeGroup>::const_iterator g = attributeGroup.begin(); g != attributeGroup.end(); ++g) {
				if((*g).name && !strcmp((*ag).name, (*g).name)) {
					found = true;
					break;
				}
			}
		}
		if(!found) {
			attributeGroup.push_back(*ag);
			attributeGroup.back().schemaPtr(this);
		}
	}
	// insert simpleTypes, but only add simpleTypes with new name (no conflict check)
	for(vector<xs__simpleType>::const_iterator st = schema.simpleType.begin(); st != schema.simpleType.end(); ++st) {
		found = false;
		if((*st).name) {
			for(vector<xs__simpleType>::const_iterator s = simpleType.begin(); s != simpleType.end(); ++s) {
				if((*s).name && !strcmp((*st).name, (*s).name)) {
					found = true;
					break;
				}
			}
		}
		if(!found) {
			simpleType.push_back(*st);
			simpleType.back().schemaPtr(this);
		}
	}
	// insert complexTypes, but only add complexTypes with new name (no conflict check)
	for(vector<xs__complexType>::const_iterator ct = schema.complexType.begin(); ct != schema.complexType.end(); ++ct) {
		found = false;
		if((*ct).name) {
			for(vector<xs__complexType>::const_iterator c = complexType.begin(); c != complexType.end(); ++c) {
				if((*c).name && !strcmp((*ct).name, (*c).name)) {
					found = true;
					break;
				}
			}
		}
		if(!found) {
			complexType.push_back(*ct);
			complexType.back().schemaPtr(this);
		}
	}
	return SOAP_OK;
}

int xs__schema::traverse()
{
	if(updated)
		return SOAP_OK;
	if(vflag)
		cerr << "  Analyzing schema '" << (targetNamespace ? targetNamespace : "") << "'" << endl;
	updated = true;
	if(!targetNamespace) {
		if(vflag)
			slfprintf_stderr("Warning: Schema has no targetNamespace\n");
		targetNamespace = soap_strdup(soap, "");
	}
	else if(exturis.find(targetNamespace) != exturis.end()) {
		if(vflag)
			slfprintf_stderr("Warning: Built-in schema '%s' content encountered\n", targetNamespace);
	}
	// process import
	for(vector<xs__import>::iterator im = import.begin(); im != import.end(); ++im)
		(*im).traverse(*this);
	// process attributes
	for(vector<xs__attribute>::iterator at = attribute.begin(); at != attribute.end(); ++at)
		(*at).traverse(*this);
	// process elements
	for(vector<xs__element>::iterator el = element.begin(); el != element.end(); ++el)
		(*el).traverse(*this);
	// process simpleTypes, check conflicts with complexTypes
	for(vector<xs__simpleType>::iterator st = simpleType.begin(); st != simpleType.end(); ++st) {
		(*st).traverse(*this);
		if((*st).name) {
			for(vector<xs__complexType>::iterator ct = complexType.begin(); ct != complexType.end(); ++ct)
				if((*ct).name && !strcmp((*st).name, (*ct).name))
					if(!Wflag)
						fprintf(stderr,
						    "Warning: top-level simpleType name and complexType name '%s' clash in schema '%s'\n",
						    (*st).name,
						    targetNamespace ? targetNamespace : "");
		}
	}
	// process complexTypes
	for(vector<xs__complexType>::iterator ct = complexType.begin(); ct != complexType.end(); ++ct)
		(*ct).traverse(*this);
	// process groups
	for(vector<xs__group>::iterator gp = group.begin(); gp != group.end(); ++gp)
		(*gp).traverse(*this);
	// process attributeGroups
	for(vector<xs__attributeGroup>::iterator ag = attributeGroup.begin(); ag != attributeGroup.end(); ++ag)
		(*ag).traverse(*this);
	if(vflag)
		cerr << "  End of schema '" << (targetNamespace ? targetNamespace : "") << "'" << endl;
	return SOAP_OK;
}

int xs__schema::read(const char * cwd, const char * loc)
{
	const char * cwd_temp;
	if(!cwd)
		cwd = cwd_path;
	if(vflag)
		slfprintf_stderr("\nOpening schema '%s' from '%s'\n", loc ? loc : "", cwd ? cwd : "");
	if(loc) {
#ifdef WITH_OPENSSL
		if(!strncmp(loc, "http://", 7) || !strncmp(loc, "https://", 8))
#else
		if(!strncmp(loc, "https://", 8)) {
			fprintf(
			    stderr,
			    "\nCannot connect to https site: no SSL support, please rebuild with SSL (default) or download the files and rerun wsdl2h\n");
			exit(1);
		}
		else if(!strncmp(loc, "http://", 7))
#endif
		{ 
			slfprintf_stderr("\nConnecting to '%s' to retrieve schema...\n", loc);
		  location = soap_strdup(soap, loc);
		  if(soap_connect_command(soap, SOAP_GET, location, NULL)) {
			  slfprintf_stderr("\nConnection failed\n");
			  exit(1);
		  }
		  slfprintf_stderr("Connected, receiving...\n"); }
		else if(cwd && (!strncmp(cwd, "http://", 7) || !strncmp(cwd, "https://", 8))) {
			char * s;
			location = (char *)soap_malloc(soap, strlen(cwd) + strlen(loc) + 2);
			strcpy(location, cwd);
			s = strrchr(location, '/');
			if(s)
				*s = '\0';
			strcat(location, "/");
			strcat(location, loc);
			slfprintf_stderr("\nConnecting to '%s' to retrieve relative '%s' schema...\n", location, loc);
			if(soap_connect_command(soap, SOAP_GET, location, NULL)) {
				slfprintf_stderr("\nConnection failed\n");
				exit(1);
			}
			slfprintf_stderr("Connected, receiving...\n");
		}
		else {soap->recvfd = open(loc, O_RDONLY, 0);
		     if(soap->recvfd < 0) {
			     if(cwd) {
				     char * s;
				     location = (char *)soap_malloc(soap, strlen(cwd) + strlen(loc) + 2);
				     strcpy(location, cwd);
				     s = strrchr(location, '/');
#ifdef WIN32
				     if(!s)
					     s = strrchr(location, '\\');
#endif
				     if(s)
					     *s = '\0';
				     strcat(location, "/");
				     strcat(location, loc);
				     if(!strncmp(location, "file://", 7))
					     location += 7;
				     soap->recvfd = open(location, O_RDONLY, 0);
			     }
			     if(soap->recvfd < 0 && import_path) {
				     location = (char *)soap_malloc(soap, strlen(import_path) + strlen(loc) + 2);
				     strcpy(location, import_path);
				     strcat(location, "/");
				     strcat(location, loc);
				     if(!strncmp(location, "file://", 7))
					     location += 7;
				     soap->recvfd = open(location, O_RDONLY, 0);
			     }
			     if(soap->recvfd < 0) {
				     slfprintf_stderr("\nCannot open '%s' to retrieve schema\n", loc);
				     exit(1);
			     }
		     }
		     else
			     location = soap_strdup(soap, loc);
		     slfprintf_stderr("\nReading schema file '%s'...\n", location); }
	}
	cwd_temp = cwd_path;
	cwd_path = location;
	if(!soap_begin_recv(soap))
		this->soap_in(soap, "xs:schema", NULL);
	if((soap->error >= 301 && soap->error <= 303) || soap->error == 307) { // HTTP redirect, socket was closed
		int r = SOAP_ERR;
		slfprintf_stderr("Redirected to '%s'...\n", soap->endpoint);
		if(redirs++ < 10)
			r = read(cwd, soap->endpoint);
		else
			slfprintf_stderr("\nMax redirects exceeded\n");
		redirs--;
		return r;
	}
	if(soap->error) {
		slfprintf_stderr("\nAn error occurred while parsing schema from '%s'\n", loc ? loc : "");
		soap_print_fault(soap, stderr);
		soap_print_fault_location(soap, stderr);
		fprintf(
		    stderr,
		    "\nIf this schema namespace is considered \"built-in\", then add\n  namespaceprefix = <namespaceURI>\nto typemap.dat.\n");
		exit(1);
	}
	slfprintf_stderr("Done reading '%s'\n", loc ? loc : "");
	soap_end_recv(soap);
	if(soap->recvfd > 2) {
		close(soap->recvfd);
		soap->recvfd = -1;
	}
	else
		soap_closesock(soap);
	cwd_path = cwd_temp;
	return SOAP_OK;
}

void xs__schema::sourceLocation(const char * loc)
{
	location = soap_strdup(soap, loc);
}

const char * xs__schema::sourceLocation()
{
	return location;
}

int xs__schema::error()
{
	return soap->error;
}

void xs__schema::print_fault()
{
	soap_print_fault(soap, stderr);
	soap_print_fault_location(soap, stderr);
}

void xs__schema::builtinType(const char * type)
{
	builtinTypeSet.insert(type);
}

void xs__schema::builtinElement(const char * element)
{
	builtinElementSet.insert(element);
}

void xs__schema::builtinAttribute(const char * attribute)
{
	builtinAttributeSet.insert(attribute);
}

const SetOfString& xs__schema::builtinTypes() const
{
	return builtinTypeSet;
}

const SetOfString& xs__schema::builtinElements() const
{
	return builtinElementSet;
}

const SetOfString& xs__schema::builtinAttributes() const
{
	return builtinAttributeSet;
}

xs__include::xs__include()
{
	schemaLocation = NULL;
	schemaRef = NULL;
}

int xs__include::preprocess(xs__schema &schema)
{
	if(!schemaRef && schemaLocation) { // only read from include locations not read already, uses static std::map
		static map<const char*, xs__schema*, ltstr> included;
		map<const char*, xs__schema*, ltstr>::iterator i = included.end();
		if(schema.targetNamespace)
			for(i = included.begin(); i != included.end(); ++i)
				if((*i).second->targetNamespace
				    && !strcmp(schemaLocation, (*i).first)
				    && !strcmp(schema.targetNamespace, (*i).second->targetNamespace))
					break;
		if(i == included.end()) {
			if(vflag)
				cerr << "Preprocessing schema include '" << (schemaLocation ? schemaLocation : "") << "' into schema '" <<
				(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
			included[schemaLocation] = schemaRef = new xs__schema(schema.soap);
			schemaRef->read(schema.sourceLocation(), schemaLocation);
			schemaRef->targetNamespace = schema.targetNamespace;
		}
		else {if(vflag)
			     cerr << "Schema '" << (schemaLocation ? schemaLocation : "") << "' already included into schema '" <<
			     (schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		     schemaRef = (*i).second; }
	}
	return SOAP_OK;
}

int xs__include::traverse(xs__schema &schema)
{
	return SOAP_OK;
}

void xs__include::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__include::schemaPtr() const
{
	return schemaRef;
}

xs__redefine::xs__redefine()
{
	schemaLocation = NULL;
	schemaRef = NULL;
}

int xs__redefine::preprocess(xs__schema &schema)
{
	if(vflag)
		cerr << "Preprocessing schema redefine '" << (schemaLocation ? schemaLocation : "") << "' into schema '" <<
		(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
	if(!schemaRef) {
		if(schemaLocation)                 {
			schemaRef = new xs__schema(schema.soap, schema.sourceLocation(), schemaLocation);
			for(vector<xs__group>::iterator gp = schemaRef->group.begin(); gp != schemaRef->group.end(); ++gp) {
				if((*gp).name)
				{
					for(vector<xs__group>::const_iterator g = group.begin(); g != group.end();
					    ++g)
					{
						if((*g).name &&
						    !strcmp((*gp).name,
							    (*g).name))
						{
							*gp = *g;
							break;
						}
					}
				}
			}
			for(vector<xs__attributeGroup>::iterator ag = schemaRef->attributeGroup.begin();
			    ag != schemaRef->attributeGroup.end();
			    ++ag) {
				if((*ag).name)
				{
					for(vector<xs__attributeGroup>::const_iterator g = attributeGroup.begin();
					    g != attributeGroup.end();
					    ++g)
					{
						if((*g).name &&
						    !strcmp((*ag).name,
							    (*g).name))
						{
							*ag = *g;
							break;
						}
					}
				}
			}
			for(vector<xs__simpleType>::iterator st = schemaRef->simpleType.begin(); st != schemaRef->simpleType.end(); ++st) {
				if((*st).name)
				{
					for(vector<xs__simpleType>::const_iterator s = simpleType.begin(); s != simpleType.end();
					    ++s)
					{
						if((*s).name &&
						    !strcmp((*st).name,
							    (*s).name))
						{
							*st = *s;
							break;
						}
					}
				}
			}
			for(vector<xs__complexType>::iterator ct = schemaRef->complexType.begin(); ct != schemaRef->complexType.end();
			    ++ct) {
				if((*ct).name)
				{
					for(vector<xs__complexType>::const_iterator c = complexType.begin(); c != complexType.end();
					    ++c)
					{
						if((*c).name &&
						    !strcmp((*ct).name,
							    (*c).name))
						{
							*ct = *c;
							break;
						}
					}
				}
			}
		}
	}
	return SOAP_OK;
}

int xs__redefine::traverse(xs__schema &schema)
{
	return SOAP_OK;
}

void xs__redefine::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__redefine::schemaPtr() const
{
	return schemaRef;
}

xs__import::xs__import()
{
	namespace_ = NULL;
	schemaLocation = NULL;
	schemaRef = NULL;
}

int xs__import::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema import '" << (namespace_ ? namespace_ : "") << "'" << endl;
	if(!schemaRef) {
		bool found = false;
		if(namespace_) {
			for(SetOfString::const_iterator i = exturis.begin(); i != exturis.end(); ++i)                 {
				if(!soap_tag_cmp(namespace_,
					    *i))
				{
					found = true;
					break;
				}
			}
		}
		else if(!Wflag)
			slfprintf_stderr("Warning: no namespace in <import>\n");
		if(!found && !iflag) { // don't import any of the schemas in the .nsmap table (or when -i option is
		                       // used)
			const char * s = schemaLocation;
			if(!s)
				s = namespace_;
			// only read from import locations not read already, uses static std::map
			static map<const char*, xs__schema*, ltstr> included;
			map<const char*, xs__schema*, ltstr>::iterator i = included.find(s);
			if(i == included.end()) {
				included[s] = schemaRef = new xs__schema(schema.soap);
				schemaRef->read(schema.sourceLocation(), s);
			}
			else
				schemaRef = (*i).second;
			if(schemaRef) {
				if(!schemaRef->targetNamespace || !*schemaRef->targetNamespace)
					schemaRef->targetNamespace = namespace_;
				else if(!namespace_ || strcmp(schemaRef->targetNamespace, namespace_))
					if(!Wflag)
						fprintf(stderr,
						    "Warning: schema import '%s' with schema targetNamespace '%s' mismatch\n",
						    namespace_ ? namespace_ : "",
						    schemaRef->targetNamespace);
			}
		}
	}
	if(schemaRef)
		schemaRef->traverse();
	return SOAP_OK;
}

void xs__import::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__import::schemaPtr() const
{
	return schemaRef;
}

xs__attribute::xs__attribute()
{
	schemaRef = NULL;
	attributeRef = NULL;
	simpleTypeRef = NULL;
}

int xs__attribute::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema attribute '" << (name ? name : "") << "'" << endl;
	schemaRef = &schema;
	const char * token = qname_token(ref, schema.targetNamespace);
	attributeRef = NULL;
	if(token) {
		for(vector<xs__attribute>::iterator i = schema.attribute.begin(); i != schema.attribute.end(); ++i)
			if(!strcmp((*i).name, token)) {
				attributeRef = &(*i);
				if(vflag)
					cerr << "    Found attribute '" << (name ? name : "") << "' ref '" << (token ? token : "") <<
					"'" << endl;
				break;
			}
	}
	if(!attributeRef) {
		for(vector<xs__import>::iterator i = schema.import.begin(); i != schema.import.end(); ++i)                    {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(ref, s->targetNamespace);
				if(token) {
					for(vector<xs__attribute>::iterator j = s->attribute.begin(); j != s->attribute.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							attributeRef = &(*j);
							if(vflag)
								cerr << "    Found attribute '" << (name ? name : "") << "' ref '" <<
								(token ? token : "") << "'" << endl;
							break;
						}
					}
					if(attributeRef)
						break;
				}
			}
		}
	}
	if(simpleType) {
		simpleType->traverse(schema);
		simpleTypeRef = simpleType;
	}
	else {token = qname_token(type, schema.targetNamespace);
	     simpleTypeRef = NULL;
	     if(token) {
		     for(vector<xs__simpleType>::iterator i = schema.simpleType.begin(); i != schema.simpleType.end(); ++i)
			     if(!strcmp((*i).name, token)) {
				     simpleTypeRef = &(*i);
				     if(vflag)
					     cerr << "    Found attribute '" << (name ? name : "") << "' type '" << (token ? token : "") <<
					     "'" << endl;
				     break;
			     }
	     }
	     if(!simpleTypeRef) {
		     for(vector<xs__import>::iterator i = schema.import.begin(); i != schema.import.end(); ++i)                     {
			     xs__schema * s = (*i).schemaPtr();
			     if(s) {
				     token = qname_token(type, s->targetNamespace);
				     if(token) {
					     for(vector<xs__simpleType>::iterator j = s->simpleType.begin(); j != s->simpleType.end();
						    ++j)            {
						     if(!strcmp((*j).name,
								    token))
						     {
							     simpleTypeRef = &(*j);
							     if(vflag)
								     cerr << "    Found attribute '" << (name ? name : "") << "' type '" <<
								     (token ? token : "") << "'" << endl;
							     break;
						     }
					     }
					     if(simpleTypeRef)
						     break;
				     }
			     }
		     }
	     }
	}
	if(!attributeRef && !simpleTypeRef) {
		if(ref)                                      {
			if(is_builtin_qname(ref))
				schema.builtinAttribute(ref);
			else if(!Wflag)
				cerr << "Warning: could not find attribute '" << (name ? name : "") << "' ref '" << ref <<
				"' in schema '" << (schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
		else if(type) {
			if(is_builtin_qname(type))
				schema.builtinType(type);
			else if(!Wflag)
				cerr << "Warning: could not find attribute '" << (name ? name : "") << "' type '" << type <<
				"' in schema '" << (schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
	}
	return SOAP_OK;
}

void xs__attribute::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema* xs__attribute::schemaPtr() const
{
	return schemaRef;
}

void xs__attribute::attributePtr(xs__attribute * attribute)
{
	attributeRef = attribute;
}

void xs__attribute::simpleTypePtr(xs__simpleType * simpleType)
{
	simpleTypeRef = simpleType;
}

xs__attribute * xs__attribute::attributePtr() const
{
	return attributeRef;
}

xs__simpleType * xs__attribute::simpleTypePtr() const
{
	return simpleTypeRef;
}

xs__element::xs__element()
{
	schemaRef = NULL;
	elementRef = NULL;
	simpleTypeRef = NULL;
	complexTypeRef = NULL;
}

int xs__element::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema element '" << (name ? name : "") << "'" << endl;
	schemaRef = &schema;
	const char * token = qname_token(ref, schema.targetNamespace);
	elementRef = NULL;
	if(token) {
		for(vector<xs__element>::iterator i = schema.element.begin(); i != schema.element.end(); ++i)
			if(!strcmp((*i).name, token)) {
				elementRef = &(*i);
				if(vflag)
					cerr << "    Found element '" << (name ? name : "") << "' ref '" << (token ? token : "") << "'" <<
					endl;
				break;
			}
	}
	if(!elementRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                  {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(ref, s->targetNamespace);
				if(token) {
					for(vector<xs__element>::iterator j = s->element.begin(); j != s->element.end(); ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							elementRef = &(*j);
							if(vflag)
								cerr << "    Found element '" << (name ? name : "") << "' ref '" <<
								(token ? token : "") << "'" << endl;
							break;
						}
					}
					if(elementRef)
						break;
				}
			}
		}
	}
	if(simpleType) {
		simpleType->traverse(schema);
		simpleTypeRef = simpleType;
	}
	else {token = qname_token(type, schema.targetNamespace);
	     simpleTypeRef = NULL;
	     if(token) {
		     for(vector<xs__simpleType>::iterator i = schema.simpleType.begin(); i != schema.simpleType.end(); ++i)
			     if(!strcmp((*i).name, token)) {
				     simpleTypeRef = &(*i);
				     if(vflag)
					     cerr << "    Found element '" << (name ? name : "") << "' simpleType '" <<
					     (token ? token : "") << "'" << endl;
				     break;
			     }
	     }
	     if(!simpleTypeRef) {
		     for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                     {
			     xs__schema * s = (*i).schemaPtr();
			     if(s) {
				     token = qname_token(type, s->targetNamespace);
				     if(token) {
					     for(vector<xs__simpleType>::iterator j = s->simpleType.begin(); j != s->simpleType.end();
						    ++j)            {
						     if(!strcmp((*j).name,
								    token))
						     {
							     simpleTypeRef = &(*j);
							     if(vflag)
								     cerr << "    Found element '" << (name ? name : "") <<
								     "' simpleType '" << (token ? token : "") << "'" << endl;
							     break;
						     }
					     }
					     if(simpleTypeRef)
						     break;
				     }
			     }
		     }
	     }
	}
	if(complexType) {
		complexType->traverse(schema);
		complexTypeRef = complexType;
	}
	else {token = qname_token(type, schema.targetNamespace);
	     complexTypeRef = NULL;
	     if(token) {
		     for(vector<xs__complexType>::iterator i = schema.complexType.begin(); i != schema.complexType.end(); ++i)
			     if(!strcmp((*i).name, token)) {
				     complexTypeRef = &(*i);
				     if(vflag)
					     cerr << "    Found element '" << (name ? name : "") << "' complexType '" <<
					     (token ? token : "") << "'" << endl;
				     break;
			     }
	     }
	     if(!complexTypeRef) {
		     for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end();
			    ++i)                      {
			     xs__schema * s = (*i).schemaPtr();
			     if(s) {
				     token = qname_token(type, s->targetNamespace);
				     if(token) {
					     for(vector<xs__complexType>::iterator j = s->complexType.begin();
						    j != s->complexType.end();
						    ++j)            {
						     if(!strcmp((*j).name,
								    token))
						     {
							     complexTypeRef = &(*j);
							     if(vflag)
								     cerr << "    Found element '" << (name ? name : "") <<
								     "' complexType '" << (token ? token : "") << "'" << endl;
							     break;
						     }
					     }
					     if(complexTypeRef)
						     break;
				     }
			     }
		     }
	     }
	}
	token = qname_token(substitutionGroup, schema.targetNamespace);
	if(token) {
		for(vector<xs__element>::iterator i = schema.element.begin(); i != schema.element.end(); ++i)
			if(!strcmp((*i).name, token)) {
				(*i).substitutions.push_back(this);
				if(vflag)
					cerr << "    Found substitutionGroup element '" << (name ? name : "") <<
					"' for abstract element '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i) {
		xs__schema * s = (*i).schemaPtr();
		if(s) {
			token = qname_token(substitutionGroup, s->targetNamespace);
			if(token) {
				for(vector<xs__element>::iterator j = s->element.begin(); j != s->element.end(); ++j)            {
					if(!strcmp((*j).name,
						    token))
					{
						(*j).substitutions.push_back(this);
						if(vflag)
							cerr << "    Found substitutionGroup element '" << (name ? name : "") <<
							"' for abstract element '" << (token ? token : "") << "'" << endl;
						break;
					}
				}
			}
		}
	}
	if(!elementRef && !simpleTypeRef && !complexTypeRef) {
		if(ref)                                                       {
			if(is_builtin_qname(ref))
				schema.builtinElement(ref);
			else if(!Wflag)
				cerr << "Warning: could not find element '" << (name ? name : "") << "' ref '" << ref << "' in schema '" <<
				(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
		else if(type) {
			if(is_builtin_qname(type))
				schema.builtinType(type);
			else if(!Wflag)
				cerr << "Warning: could not find element '" << (name ? name : "") << "' type '" << type <<
				"' in schema '" << (schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
	}
	return SOAP_OK;
}

void xs__element::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema* xs__element::schemaPtr() const
{
	return schemaRef;
}

void xs__element::elementPtr(xs__element * element)
{
	elementRef = element;
}

void xs__element::simpleTypePtr(xs__simpleType * simpleType)
{
	simpleTypeRef = simpleType;
}

void xs__element::complexTypePtr(xs__complexType * complexType)
{
	complexTypeRef = complexType;
}

xs__element * xs__element::elementPtr() const
{
	return elementRef;
}

const std::vector<xs__element*>* xs__element::substitutionsPtr() const
{
	return &substitutions;
}

xs__simpleType * xs__element::simpleTypePtr() const
{
	return simpleTypeRef;
}

xs__complexType * xs__element::complexTypePtr() const
{
	return complexTypeRef;
}

xs__simpleType::xs__simpleType()
{
	schemaRef = NULL;
	level = 0;
}

int xs__simpleType::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema simpleType '" << (name ? name : "") << "'" << endl;
	schemaRef = &schema;
	if(list)
		list->traverse(schema);
	else if(restriction)
		restriction->traverse(schema);
	else if(union_)
		union_->traverse(schema);
	return SOAP_OK;
}

void xs__simpleType::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__simpleType::schemaPtr() const
{
	return schemaRef;
}

int xs__simpleType::baseLevel()
{
	if(!level) {
		if(restriction)             {
			level = -1;
			if(restriction->simpleTypePtr())
				level = restriction->simpleTypePtr()->baseLevel() + 1;
			else
				level = 2;
		}
		else if(list && list->restriction) {
			level = -1;
			if(list->restriction->simpleTypePtr())
				level = list->restriction->simpleTypePtr()->baseLevel() + 1;
			else
				level = 2;
		}
		else
			level = 1;
	}
	else if(level < 0) {
		cerr << "Error: cyclic simpleType restriction/extension base dependency in '" << (name ? name : "") << "'" << endl;
	}
	return level;
}

xs__complexType::xs__complexType()
{
	schemaRef = NULL;
	level = 0;
}

int xs__complexType::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema complexType '" << (name ? name : "") << "'" << endl;
	schemaRef = &schema;
	if(simpleContent)
		simpleContent->traverse(schema);
	else if(complexContent)
		complexContent->traverse(schema);
	else if(all)
		all->traverse(schema);
	else if(choice)
		choice->traverse(schema);
	else if(sequence)
		sequence->traverse(schema);
	else if(any)
		any->traverse(schema);
	for(vector<xs__attribute>::iterator at = attribute.begin(); at != attribute.end(); ++at)
		(*at).traverse(schema);
	for(vector<xs__attributeGroup>::iterator ag = attributeGroup.begin(); ag != attributeGroup.end(); ++ag)
		(*ag).traverse(schema);
	return SOAP_OK;
}

void xs__complexType::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__complexType::schemaPtr() const
{
	return schemaRef;
}

int xs__complexType::baseLevel()
{
	if(!level) {
		if(simpleContent)             {
			if(simpleContent->restriction)                                {
				level = -1;
				if(simpleContent->restriction->simpleTypePtr())
					level = simpleContent->restriction->simpleTypePtr()->baseLevel() + 1;
				else if(simpleContent->restriction->complexTypePtr())
					level = simpleContent->restriction->complexTypePtr()->baseLevel() + 1;
				else
					level = 2;
			}
			else if(simpleContent->extension) {
				level = -1;
				if(simpleContent->extension->simpleTypePtr())
					level = simpleContent->extension->simpleTypePtr()->baseLevel() + 1;
				else if(simpleContent->extension->complexTypePtr())
					level = simpleContent->extension->complexTypePtr()->baseLevel() + 1;
				else
					level = 2;
			}
		}
		else if(complexContent) {
			if(complexContent->restriction)                          {
				level = -1;
				if(complexContent->restriction->simpleTypePtr())
					level = complexContent->restriction->simpleTypePtr()->baseLevel() + 1;
				else if(complexContent->restriction->complexTypePtr())
					level = complexContent->restriction->complexTypePtr()->baseLevel() + 1;
				else
					level = 2;
			}
			else if(complexContent->extension) {
				level = -1;
				if(complexContent->extension->simpleTypePtr())
					level = complexContent->extension->simpleTypePtr()->baseLevel() + 1;
				else if(complexContent->extension->complexTypePtr())
					level = complexContent->extension->complexTypePtr()->baseLevel() + 1;
				else
					level = 2;
			}
		}
		else
			level = 1;
	}
	else if(level < 0) {
		cerr << "Error: cyclic complexType restriction/extension base dependency in '" << (name ? name : "") << "'" << endl;
	}
	return level;
}

int xs__simpleContent::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema simpleContent" << endl;
	if(extension)
		extension->traverse(schema);
	else if(restriction)
		restriction->traverse(schema);
	return SOAP_OK;
}

int xs__complexContent::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema complexContent" << endl;
	if(extension)
		extension->traverse(schema);
	else if(restriction)
		restriction->traverse(schema);
	return SOAP_OK;
}

xs__extension::xs__extension()
{
	simpleTypeRef = NULL;
	complexTypeRef = NULL;
}

int xs__extension::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema extension '" << (base ? base : "") << "'" << endl;
	if(group)
		group->traverse(schema);
	else if(all)
		all->traverse(schema);
	else if(choice)
		choice->traverse(schema);
	else if(sequence)
		sequence->traverse(schema);
	for(vector<xs__attribute>::iterator at = attribute.begin(); at != attribute.end(); ++at)
		(*at).traverse(schema);
	for(vector<xs__attributeGroup>::iterator ag = attributeGroup.begin(); ag != attributeGroup.end(); ++ag)
		(*ag).traverse(schema);
	const char * token = qname_token(base, schema.targetNamespace);
	simpleTypeRef = NULL;
	if(token) {
		for(vector<xs__simpleType>::iterator i = schema.simpleType.begin(); i != schema.simpleType.end(); ++i)
			if(!strcmp((*i).name, token)) {
				simpleTypeRef = &(*i);
				if(vflag)
					cerr << "    Found extension base type '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	if(!simpleTypeRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                     {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(base, s->targetNamespace);
				if(token) {
					for(vector<xs__simpleType>::iterator j = s->simpleType.begin(); j != s->simpleType.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							simpleTypeRef = &(*j);
							if(vflag)
								cerr << "    Found extension base type '" << (token ? token : "") << "'" <<
								endl;
							break;
						}
					}
					if(simpleTypeRef)
						break;
				}
			}
		}
	}
	token = qname_token(base, schema.targetNamespace);
	complexTypeRef = NULL;
	if(token) {
		for(vector<xs__complexType>::iterator i = schema.complexType.begin(); i != schema.complexType.end(); ++i)
			if(!strcmp((*i).name, token)) {
				complexTypeRef = &(*i);
				if(vflag)
					cerr << "    Found extension base type '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	if(!complexTypeRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                      {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(base, s->targetNamespace);
				if(token) {
					for(vector<xs__complexType>::iterator j = s->complexType.begin(); j != s->complexType.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							complexTypeRef = &(*j);
							if(vflag)
								cerr << "    Found extension base type '" << (token ? token : "") << "'" <<
								endl;
							break;
						}
					}
					if(complexTypeRef)
						break;
				}
			}
		}
	}
	if(!simpleTypeRef && !complexTypeRef) {
		if(base)                                        {
			if(is_builtin_qname(base))
				schema.builtinType(base);
			else if(!Wflag)
				cerr << "Warning: could not find extension base type '" << base << "' in schema '" <<
				(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
		else
			cerr << "Extension has no base" << endl;
	}
	return SOAP_OK;
}

void xs__extension::simpleTypePtr(xs__simpleType * simpleType)
{
	simpleTypeRef = simpleType;
}

void xs__extension::complexTypePtr(xs__complexType * complexType)
{
	complexTypeRef = complexType;
}

xs__simpleType * xs__extension::simpleTypePtr() const
{
	return simpleTypeRef;
}

xs__complexType * xs__extension::complexTypePtr() const
{
	return complexTypeRef;
}

xs__restriction::xs__restriction()
{
	simpleTypeRef = NULL;
	complexTypeRef = NULL;
}

int xs__restriction::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema restriction '" << (base ? base : "") << "'" << endl;
	if(attributeGroup)
		attributeGroup->traverse(schema);
	if(group)
		group->traverse(schema);
	else if(all)
		all->traverse(schema);
	else if(choice)
		choice->traverse(schema);
	else if(sequence)
		sequence->traverse(schema);
	else {for(vector<xs__enumeration>::iterator en = enumeration.begin(); en != enumeration.end(); ++en)
		     (*en).traverse(schema);
	     for(vector<xs__pattern>::iterator pn = pattern.begin(); pn != pattern.end(); ++pn)
		     (*pn).traverse(schema); }
	for(vector<xs__attribute>::iterator at = attribute.begin(); at != attribute.end(); ++at)
		(*at).traverse(schema);
	const char * token = qname_token(base, schema.targetNamespace);
	simpleTypeRef = NULL;
	if(token) {
		for(vector<xs__simpleType>::iterator i = schema.simpleType.begin(); i != schema.simpleType.end(); ++i)
			if(!strcmp((*i).name, token)) {
				simpleTypeRef = &(*i);
				if(vflag)
					cerr << "    Found restriction base type '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	if(!simpleTypeRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                     {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(base, s->targetNamespace);
				if(token) {
					for(vector<xs__simpleType>::iterator j = s->simpleType.begin(); j != s->simpleType.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							simpleTypeRef = &(*j);
							if(vflag)
								cerr << "    Found restriction base type '" << (token ? token : "") <<
								"'" << endl;
							break;
						}
					}
					if(simpleTypeRef)
						break;
				}
			}
		}
	}
	token = qname_token(base, schema.targetNamespace);
	complexTypeRef = NULL;
	if(token) {
		for(vector<xs__complexType>::iterator i = schema.complexType.begin(); i != schema.complexType.end(); ++i)
			if(!strcmp((*i).name, token)) {
				complexTypeRef = &(*i);
				if(vflag)
					cerr << "    Found restriction base type '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	if(!complexTypeRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                      {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(base, s->targetNamespace);
				if(token) {
					for(vector<xs__complexType>::iterator j = s->complexType.begin(); j != s->complexType.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							complexTypeRef = &(*j);
							if(vflag)
								cerr << "    Found restriction base type '" << (token ? token : "") <<
								"'" << endl;
							break;
						}
					}
					if(complexTypeRef)
						break;
				}
			}
		}
	}
	if(!simpleTypeRef && !complexTypeRef) {
		if(base)                                        {
			if(is_builtin_qname(base))
				schema.builtinType(base);
			else if(!Wflag)
				cerr << "Warning: could not find restriction base type '" << base << "' in schema '" <<
				(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
		}
		else
			cerr << "Restriction has no base" << endl;
	}
	return SOAP_OK;
}

void xs__restriction::simpleTypePtr(xs__simpleType * simpleType)
{
	simpleTypeRef = simpleType;
}

void xs__restriction::complexTypePtr(xs__complexType * complexType)
{
	complexTypeRef = complexType;
}

xs__simpleType * xs__restriction::simpleTypePtr() const
{
	return simpleTypeRef;
}

xs__complexType * xs__restriction::complexTypePtr() const
{
	return complexTypeRef;
}

xs__list::xs__list()
{
	itemTypeRef = NULL;
}

int xs__list::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema list" << endl;
	if(restriction)
		restriction->traverse(schema);
	for(vector<xs__simpleType>::iterator i = simpleType.begin(); i != simpleType.end(); ++i)
		(*i).traverse(schema);
	itemTypeRef = NULL;
	const char * token = qname_token(itemType, schema.targetNamespace);
	if(token) {
		for(vector<xs__simpleType>::iterator i = schema.simpleType.begin(); i != schema.simpleType.end(); ++i)
			if(!strcmp((*i).name, token)) {
				itemTypeRef = &(*i);
				if(vflag)
					cerr << "    Found list itemType '" << (token ? token : "") << "'" << endl;
				break;
			}
	}
	if(!itemTypeRef) {
		for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                   {
			xs__schema * s = (*i).schemaPtr();
			if(s) {
				token = qname_token(itemType, s->targetNamespace);
				if(token) {
					for(vector<xs__simpleType>::iterator j = s->simpleType.begin(); j != s->simpleType.end();
					    ++j)            {
						if(!strcmp((*j).name,
							    token))
						{
							itemTypeRef = &(*j);
							if(vflag)
								cerr << "    Found list itemType '" << (token ? token : "") << "'" << endl;
							break;
						}
					}
					if(itemTypeRef)
						break;
				}
			}
		}
	}
	if(itemType && !itemTypeRef) {
		if(is_builtin_qname(itemType))
			schema.builtinType(itemType);
		else if(!Wflag)
			cerr << "Warning: could not find list itemType '" << itemType << "' in schema '" <<
			(schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
	}
	return SOAP_OK;
}

void xs__list::itemTypePtr(xs__simpleType * simpleType)
{
	itemTypeRef = simpleType;
}

xs__simpleType * xs__list::itemTypePtr() const
{
	return itemTypeRef;
}

int xs__union::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema union" << endl;
	for(vector<xs__simpleType>::iterator i = simpleType.begin(); i != simpleType.end(); ++i)
		(*i).traverse(schema);
	return SOAP_OK;
}

int xs__all::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema all" << endl;
	for(vector<xs__element>::iterator i = element.begin(); i != element.end(); ++i)
		(*i).traverse(schema);
	return SOAP_OK;
}

int xs__contents::traverse(xs__schema &schema)
{
	switch(__union) { 
		case SOAP_UNION_xs__union_content_element:
			if(__content.element)
				__content.element->traverse(schema);
			break;
		case SOAP_UNION_xs__union_content_group:
			if(__content.group)
				__content.group->traverse(schema);
			break;
		case SOAP_UNION_xs__union_content_choice:
			if(__content.choice)
				__content.choice->traverse(schema);
			break;
		case SOAP_UNION_xs__union_content_sequence:
			if(__content.sequence)
				__content.sequence->traverse(schema);
			break;
		case SOAP_UNION_xs__union_content_any:
			if(__content.any)
				__content.any->traverse(schema);
			break; 
	}
	return SOAP_OK;
}

xs__seqchoice::xs__seqchoice()
{
	schemaRef = NULL;
}

int xs__seqchoice::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema sequence/choice" << endl;
	schemaRef = &schema;
	for(vector<xs__contents>::iterator c = __contents.begin(); c != __contents.end(); ++c)
		(*c).traverse(schema);
	return SOAP_OK;
}

void xs__seqchoice::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema * xs__seqchoice::schemaPtr() const
{
	return schemaRef;
}

xs__attributeGroup::xs__attributeGroup()
{
	schemaRef = NULL;
	attributeGroupRef = NULL;
}

int xs__attributeGroup::traverse(xs__schema& schema)
{
	if(vflag)
		cerr << "   Analyzing schema attributeGroup" << endl;
	schemaRef = &schema;
	for(vector<xs__attribute>::iterator at = attribute.begin(); at != attribute.end(); ++at)
		(*at).traverse(schema);
	for(vector<xs__attributeGroup>::iterator ag = attributeGroup.begin(); ag != attributeGroup.end(); ++ag)
		(*ag).traverse(schema);
	attributeGroupRef = NULL;
	if(ref) {
		const char * token = qname_token(ref, schema.targetNamespace);
		if(token) {
			for(vector<xs__attributeGroup>::iterator i = schema.attributeGroup.begin(); i != schema.attributeGroup.end(); ++i)
				if(!strcmp((*i).name, token)) {
					attributeGroupRef = &(*i);
					if(vflag)
						cerr << "    Found attributeGroup '" << (name ? name : "") << "' ref '" <<
						(token ? token : "") << "'" << endl;
					break;
				}
		}
		if(!attributeGroupRef) {
			for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end();
			    ++i)                         {
				xs__schema * s = (*i).schemaPtr();
				if(s) {
					token = qname_token(ref, s->targetNamespace);
					if(token) {
						for(vector<xs__attributeGroup>::iterator j = s->attributeGroup.begin();
						    j != s->attributeGroup.end();
						    ++j)            {
							if(!strcmp((*j).name,
								    token))
							{
								attributeGroupRef = &(*j);
								if(vflag)
									cerr << "    Found attribute Group '" << (name ? name : "") <<
									"' ref '" << (token ? token : "") << "'" << endl;
								break;
							}
						}
						if(attributeGroupRef)
							break;
					}
				}
			}
		}
		if(!attributeGroupRef)
			if(!Wflag)
				cerr << "Warning: could not find attributeGroup '" << (name ? name : "") << "' ref '" <<
				(ref ? ref : "") << "' in schema '" << (schema.targetNamespace ? schema.targetNamespace : "") << "'" <<
				endl;
	}
	return SOAP_OK;
}

void xs__attributeGroup::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

void xs__attributeGroup::attributeGroupPtr(xs__attributeGroup * attributeGroup)
{
	attributeGroupRef = attributeGroup;
}

xs__schema * xs__attributeGroup::schemaPtr() const
{
	return schemaRef;
}

xs__attributeGroup * xs__attributeGroup::attributeGroupPtr() const
{
	return attributeGroupRef;
}

int xs__any::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema any" << endl;
	for(vector<xs__element>::iterator i = element.begin(); i != element.end(); ++i)
		(*i).traverse(schema);
	return SOAP_OK;
}

xs__group::xs__group()
{
	schemaRef = NULL;
	groupRef = NULL;
}

int xs__group::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema group" << endl;
	schemaRef = &schema;
	if(all)
		all->traverse(schema);
	else if(choice)
		choice->traverse(schema);
	else if(sequence)
		sequence->traverse(schema);
	groupRef = NULL;
	if(ref) {
		const char * token = qname_token(ref, schema.targetNamespace);
		if(token) {
			for(vector<xs__group>::iterator i = schema.group.begin(); i != schema.group.end(); ++i)
				if(!strcmp((*i).name, token)) {
					groupRef = &(*i);
					if(vflag)
						cerr << "    Found group '" << (name ? name : "") << "' ref '" << (token ? token : "") <<
						"'" << endl;
					break;
				}
		}
		if(!groupRef) {
			for(vector<xs__import>::const_iterator i = schema.import.begin(); i != schema.import.end(); ++i)                {
				xs__schema * s = (*i).schemaPtr();
				if(s) {
					token = qname_token(ref, s->targetNamespace);
					if(token) {
						for(vector<xs__group>::iterator j = s->group.begin(); j != s->group.end();
						    ++j)            {
							if(!strcmp((*j).name,
								    token))
							{
								groupRef = &(*j);
								if(vflag)
									cerr << "    Found group '" << (name ? name : "") << "' ref '" <<
									(token ? token : "") << "'" << endl;
								break;
							}
						}
						if(groupRef)
							break;
					}
				}
			}
		}
		if(!groupRef)
			if(!Wflag)
				cerr << "Warning: could not find group '" << (name ? name : "") << "' ref '" << (ref ? ref : "") <<
				"' in schema '" << (schema.targetNamespace ? schema.targetNamespace : "") << "'" << endl;
	}
	return SOAP_OK;
}

void xs__group::schemaPtr(xs__schema * schema)
{
	schemaRef = schema;
}

xs__schema* xs__group::schemaPtr() const
{
	return schemaRef;
}

void xs__group::groupPtr(xs__group * group)
{
	groupRef = group;
}

xs__group* xs__group::groupPtr() const
{
	return groupRef;
}

int xs__enumeration::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema enumeration '" << (value ? value : "") << "'" << endl;
	return SOAP_OK;
}

int xs__pattern::traverse(xs__schema &schema)
{
	if(vflag)
		cerr << "   Analyzing schema pattern" << endl;
	return SOAP_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
//	I/O
//
////////////////////////////////////////////////////////////////////////////////

ostream &operator<<(ostream &o, const xs__schema &e)
{
	if(!e.soap) {
		struct soap soap;

		soap_init2(&soap, SOAP_IO_DEFAULT, SOAP_XML_TREE | SOAP_C_UTFSTRING);
		soap_set_namespaces(&soap, namespaces);
		e.soap_serialize(&soap);
		soap_begin_send(&soap);
		e.soap_out(&soap, "xs:schema", 0, NULL);
		soap_end_send(&soap);
		soap_end(&soap);
		soap_done(&soap);
	}
	else {ostream * os = e.soap->os;
	     e.soap->os = &o;
	     e.soap_serialize(e.soap);
	     soap_begin_send(e.soap);
	     e.soap_out(e.soap, "xs:schema", 0, NULL);
	     soap_end_send(e.soap);
	     e.soap->os = os; }
	return o;
}

istream &operator>>(istream &i, xs__schema &e)
{
	if(!e.soap) {
		e.soap = soap_new();
		soap_set_namespaces(e.soap, namespaces);
	}
	istream * is = e.soap->is;
	e.soap->is = &i;
	if(soap_begin_recv(e.soap)
	    || !e.soap_in(e.soap, "xs:schema", NULL)
	    || soap_end_recv(e.soap)) { // handle error? Note: e.soap->error is set and app should check
	}
	e.soap->is = is;
	return i;
}

