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

//#ifdef __cplusplus
//extern "C" {
//#endif
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
xmlAutomata * xmlNewAutomata();
void xmlFreeAutomata(xmlAutomata * am);
xmlAutomataState * xmlAutomataGetInitState(xmlAutomata * am);
int xmlAutomataSetFinalState(xmlAutomata * am, xmlAutomataState * state);
xmlAutomataState * xmlAutomataNewState(xmlAutomata * am);
xmlAutomataState * xmlAutomataNewTransition(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, void * data);
xmlAutomataState * xmlAutomataNewTransition2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
xmlAutomataState * xmlAutomataNewNegTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, void * data);
xmlAutomataState * xmlAutomataNewCountTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
xmlAutomataState * xmlAutomataNewCountTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
xmlAutomataState * xmlAutomataNewOnceTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, int min, int max, void * data);
xmlAutomataState * xmlAutomataNewOnceTrans2(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, const xmlChar * token, const xmlChar * token2, int min, int max, void * data);
xmlAutomataState * xmlAutomataNewAllTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int lax);
xmlAutomataState * FASTCALL xmlAutomataNewEpsilon(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to);
xmlAutomataState * FASTCALL xmlAutomataNewCountedTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
xmlAutomataState * xmlAutomataNewCounterTrans(xmlAutomata * am, xmlAutomataState * from, xmlAutomataState * to, int counter);
int xmlAutomataNewCounter(xmlAutomata * am, int min, int max);
xmlRegexp * xmlAutomataCompile(xmlAutomata * am);
int xmlAutomataIsDeterminist(xmlAutomata * am);

//#ifdef __cplusplus
//}
//#endif
#endif /* LIBXML_AUTOMATA_ENABLED */
#endif /* LIBXML_REGEXP_ENABLED */
#endif /* __XML_AUTOMATA_H__ */
