/*
        soap.cpp

        WSDL/SOAP binding schema

   --------------------------------------------------------------------------------
   gSOAP XML Web services tools
   Copyright (C) 2001-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
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
#include <slib.h>
#include "wsdlH.h"              // cannot include "schemaH.h"
#pragma hdrstop
#include "includes.h"

extern const char * qname_token(const char*, const char*);
//
//	soap:header
//
int soap__header::traverse(wsdl__definitions& definitions)
{
	if(vflag)
		cerr << "    Analyzing soap header in wsdl namespace '" << (definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	messageRef = NULL;
	partRef = NULL;
	const char * token = qname_token(message, definitions.targetNamespace);
	if(token) {
		for(vector<wsdl__message>::iterator message = definitions.message.begin(); message != definitions.message.end();
		    ++message)            {
			if((*message).name && !strcmp((*message).name, token)) {
				messageRef = &(*message);
				if(vflag)
					cerr << "     Found soap header part '" << (part ? part : "") << "' message '" << (token ? token : "") << "'" << endl;
				break;
			}
		}
	}
	if(!messageRef) {
		for(vector<wsdl__import>::iterator import = definitions.import.begin(); import != definitions.import.end(); ++import) {
			wsdl__definitions * importdefinitions = (*import).definitionsPtr();
			if(importdefinitions) {
				token = qname_token(message, importdefinitions->targetNamespace);
				if(token) {
					for(vector<wsdl__message>::iterator message = importdefinitions->message.begin(); message != importdefinitions->message.end(); ++message) {
						if((*message).name && !strcmp((*message).name, token)) {
							messageRef = &(*message);
							if(vflag)
								cerr << "     Found soap header part '" << (part ? part : "") << "' message '" << (token ? token : "") << "'" << endl;
							break;
						}
					}
				}
			}
		}
	}
	if(messageRef) {
		if(part) {
			for(vector<wsdl__part>::iterator pt = messageRef->part.begin(); pt != messageRef->part.end(); ++pt)
				if((*pt).name && !strcmp((*pt).name, part)) {
					partRef = &(*pt);
					break;
				}
		}
		if(!partRef)
			cerr << "Warning: soap header has no matching part in message '" << (message ? message : "") <<
			"' in wsdl definitions '" << definitions.name << "' namespace '" <<
			(definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	}
	else
		cerr << "Warning: could not find soap header part '" << (part ? part : "") << "' message '" << (message ? message : "") <<
		"' in wsdl definitions '" << definitions.name << "' namespace '" <<
		(definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	for(vector<soap__headerfault>::iterator i = headerfault.begin(); i != headerfault.end(); ++i)
		(*i).traverse(definitions);
	return SOAP_OK;
}

void soap__header::messagePtr(wsdl__message * message)
{
	messageRef = message;
}

wsdl__message * soap__header::messagePtr() const
{
	return messageRef;
}

void soap__header::partPtr(wsdl__part * part)
{
	partRef = part;
}

wsdl__part * soap__header::partPtr() const
{
	return partRef;
}

////////////////////////////////////////////////////////////////////////////////
//
//	soap:headerfault
//
////////////////////////////////////////////////////////////////////////////////

int soap__headerfault::traverse(wsdl__definitions& definitions)
{
	if(vflag)
		cerr << "    Analyzing soap headerfault in wsdl namespace '" << (definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	messageRef = NULL;
	partRef = NULL;
	const char * token = qname_token(message, definitions.targetNamespace);
	if(token) {
		for(vector<wsdl__message>::iterator message = definitions.message.begin(); message != definitions.message.end(); ++message) {
			if((*message).name && !strcmp((*message).name, token)) {
				messageRef = &(*message);
				if(vflag)
					cerr << "     Found soap headerfault part '" << (part ? part : "") << "' message '" << (token ? token : "") << "'" << endl;
				break;
			}
		}
	}
	else {
		for(vector<wsdl__import>::iterator import = definitions.import.begin(); import != definitions.import.end(); ++import) {
		     wsdl__definitions * importdefinitions = (*import).definitionsPtr();
		     if(importdefinitions) {
			     token = qname_token(message, importdefinitions->targetNamespace);
			     if(token) {
				     for(vector<wsdl__message>::iterator message = importdefinitions->message.begin(); message != importdefinitions->message.end(); ++message) {
					     if((*message).name && !strcmp((*message).name, token)) {
						     messageRef = &(*message);
						     if(vflag)
							     cerr << "     Found soap headerfault part '" << (part ? part : "") <<
							     "' message '" << (token ? token : "") << "'" << endl;
						     break;
					     }
				     }
			     }
		     }
	     }
	}
	if(messageRef) {
		if(part) {
			for(vector<wsdl__part>::iterator pt = messageRef->part.begin(); pt != messageRef->part.end(); ++pt)
				if((*pt).name && !strcmp((*pt).name, part)) {
					partRef = &(*pt);
					break;
				}
		}
		if(!partRef)
			cerr << "Warning: soap headerfault has no matching part in message '" << (message ? message : "") <<
			"' in wsdl definitions '" << definitions.name << "' namespace '" <<
			(definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	}
	else
		cerr << "Warning: could not find soap headerfault part '" << (part ? part : "") << "' message '" <<
		(message ? message : "") << "' in wSDL definitions '" << definitions.name << "' namespace '" <<
		(definitions.targetNamespace ? definitions.targetNamespace : "") << "'" << endl;
	return SOAP_OK;
}

void soap__headerfault::messagePtr(wsdl__message * message)
{
	messageRef = message;
}

wsdl__message * soap__headerfault::messagePtr() const
{
	return messageRef;
}

void soap__headerfault::partPtr(wsdl__part * part)
{
	partRef = part;
}

wsdl__part * soap__headerfault::partPtr() const
{
	return partRef;
}

