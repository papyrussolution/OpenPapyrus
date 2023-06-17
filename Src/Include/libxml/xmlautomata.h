/*
 * Summary: API to build regexp automata
 * Description: the API to build regexp automata
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Daniel Veillard
 */
#ifndef __XML_AUTOMATA_H__
#define __XML_AUTOMATA_H__

#ifdef LIBXML_REGEXP_ENABLED
#ifdef LIBXML_AUTOMATA_ENABLED

#ifdef __cplusplus
extern "C" {
#endif
/**
 * xmlAutomataPtr:
 *
 * A libxml automata description, It can be compiled into a regexp
 */
struct xmlAutomata;
//typedef xmlAutomata * xmlAutomataPtr;
/**
 * xmlAutomataStatePtr:
 *
 * A state int the automata description,
 */
struct xmlAutomataState;
//typedef xmlAutomataState * xmlAutomataStatePtr;
/*
 * Building API
 */
XMLPUBFUN xmlAutomata * xmlNewAutomata();
XMLPUBFUN void xmlFreeAutomata(xmlAutomata * am);
XMLPUBFUN xmlAutomataState * xmlAutomataGetInitState(xmlAutomata * am);
XMLPUBFUN int xmlAutomataSetFinalState(xmlAutomata * am, xmlAutomataState * state);
XMLPUBFUN xmlAutomataState * xmlAutomataNewState(xmlAutomata * am);
XMLPUBFUN xmlAutomataState * xmlAutomataNewTransition(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewTransition2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewNegTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewCountTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewCountTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewOnceTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewOnceTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * xmlAutomataNewAllTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int lax);
XMLPUBFUN xmlAutomataState * FASTCALL xmlAutomataNewEpsilon(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to);
XMLPUBFUN xmlAutomataState * FASTCALL xmlAutomataNewCountedTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
XMLPUBFUN xmlAutomataState * xmlAutomataNewCounterTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
XMLPUBFUN int xmlAutomataNewCounter(xmlAutomata * am, int min, int max);
XMLPUBFUN xmlRegexp * xmlAutomataCompile(xmlAutomata * am);
XMLPUBFUN int xmlAutomataIsDeterminist(xmlAutomata * am);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_AUTOMATA_ENABLED */
#endif /* LIBXML_REGEXP_ENABLED */

#endif /* __XML_AUTOMATA_H__ */
