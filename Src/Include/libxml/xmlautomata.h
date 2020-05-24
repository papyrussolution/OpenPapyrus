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
XMLPUBFUN xmlAutomata * XMLCALL xmlNewAutomata();
XMLPUBFUN void XMLCALL xmlFreeAutomata(xmlAutomata * am);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataGetInitState(xmlAutomata * am);
XMLPUBFUN int XMLCALL xmlAutomataSetFinalState(xmlAutomata * am, xmlAutomataState * state);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewState(xmlAutomata * am);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewTransition(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewTransition2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewNegTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewCountTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewCountTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewOnceTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewOnceTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewAllTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int lax);
XMLPUBFUN xmlAutomataState * /*XMLCALL*/FASTCALL xmlAutomataNewEpsilon(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to);
XMLPUBFUN xmlAutomataState * /*XMLCALL*/FASTCALL xmlAutomataNewCountedTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
XMLPUBFUN xmlAutomataState * XMLCALL xmlAutomataNewCounterTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
XMLPUBFUN int XMLCALL xmlAutomataNewCounter(xmlAutomata * am, int min, int max);
XMLPUBFUN xmlRegexp * XMLCALL xmlAutomataCompile(xmlAutomata * am);
XMLPUBFUN int XMLCALL xmlAutomataIsDeterminist(xmlAutomata * am);

#ifdef __cplusplus
}
#endif

#endif /* LIBXML_AUTOMATA_ENABLED */
#endif /* LIBXML_REGEXP_ENABLED */

#endif /* __XML_AUTOMATA_H__ */
