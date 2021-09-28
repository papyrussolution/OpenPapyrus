// SARTRE_IMP.CPP
// Copyright (c) A.Sobolev 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <locale.h>

static int FASTCALL RechargeTransaction(BDbTransaction * pTa, uint & rItemsPerTx, const uint maxItemsPerTx)
{
	int    ok = -1;
	if(rItemsPerTx >= maxItemsPerTx) {
		if(pTa) {
			THROW_DB(pTa->Commit(1));
			THROW_DB(pTa->Start(1));
		}
		rItemsPerTx = 0;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

static int ReadAncodeDescrLine_Ru(const char * pLine, SString & rAncode, SrWordForm & rForm)
{
	int    ok = 1;
	rAncode.Z();

	SString line_buf;
	const char * p = pLine;
	while(*p && *p != '\n' && !(p[0] == '/' && p[1] == '/')) {
		line_buf.CatChar(*p++);
	}
	if(line_buf.NotEmptyS()) {
		SString temp_buf;
		const char * p = line_buf;
		//
		temp_buf.Z();
		while(*p != ' ' && *p != '\t' && *p != 0)
			temp_buf.CatChar(*p++);
		rAncode = temp_buf;
		//
		while(oneof2(*p, ' ', '\t'))
			p++;
		temp_buf.Z();
		while(*p != ' ' && *p != '\t' && *p != 0)
			temp_buf.CatChar(*p++);
		//
		int    prev_case = 0;  // Предыдущий токен обозначал падеж
		int    prev_compr = 0; // Предыдущий токен - сравнительная степень прилагательного SRADJCMP_COMPARATIVE
		do {
			while(oneof3(*p, ' ', '\t', ','))
				p++;
			temp_buf.Z();
			while(*p != ' ' && *p != '\t' && *p != ',' && *p != 0)
				temp_buf.CatChar(*p++);
			if(temp_buf.NotEmpty()) {
				temp_buf.Transf(CTRANSF_OUTER_TO_UTF8); // @v10.4.5 (module has been translated to utf-8)
				if(temp_buf == "сравн")
					rForm.SetTag(SRWG_ADJCMP, prev_compr = SRADJCMP_COMPARATIVE);
				else {
					if(temp_buf == "им")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_NOMINATIVE);
					else if(temp_buf == "рд")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_GENITIVE);
					else if(temp_buf == "дт")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_DATIVE);
					else if(temp_buf == "вн")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_ACCUSATIVE);
					else if(temp_buf == "тв")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_INSTRUMENT);
					else if(temp_buf == "пр")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_PREPOSITIONAL);
					else if(temp_buf == "зв")
						rForm.SetTag(SRWG_CASE, prev_case = SRCASE_VOCATIVE);
					else if(temp_buf == "2") {
						if(prev_case == SRCASE_GENITIVE)
							rForm.SetTag(SRWG_CASE, SRCASE_GENITIVE2);
						else if(prev_case == SRCASE_DATIVE)
							rForm.SetTag(SRWG_CASE, SRCASE_DATIVE2);
						else if(prev_case == SRCASE_ACCUSATIVE)
							rForm.SetTag(SRWG_CASE, SRCASE_ACCUSATIVE2);
						else if(prev_case == SRCASE_PREPOSITIONAL)
							rForm.SetTag(SRWG_CASE, SRCASE_PREPOSITIONAL2);
						else if(prev_compr == SRADJCMP_COMPARATIVE)
							rForm.SetTag(SRWG_ADJCMP, SRADJCMP_COMPARATIVE2); // 2-я стравнительная степень прилагательного
						else {
							ok = 0; // Неверное определение "второго" падежа или 2-й сравнительной степени прилагательного
							break;
						}
						prev_case = 0;
						prev_compr = 0;
					}
					else {
						prev_case = 0;
						if(temp_buf == "П")
							rForm.SetTag(SRWG_CLASS, SRWC_ADJECTIVE);
						else if(temp_buf == "КР_ПРИЛ") {
							rForm.SetTag(SRWG_CLASS, SRWC_ADJECTIVE);
							rForm.SetTag(SRWG_SHORT, SRSHORT_BREV);
						}
						else if(temp_buf == "С")
							rForm.SetTag(SRWG_CLASS, SRWC_NOUN);
						else if(temp_buf == "Г")
							rForm.SetTag(SRWG_CLASS, SRWC_VERB);
						else if(temp_buf == "МС")
							rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
						else if(temp_buf == "МС-П")
							rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
						else if(temp_buf == "МС-П")
							rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
						else if(temp_buf == "МС-ПРЕДК")
							rForm.SetTag(SRWG_CLASS, SRWC_PRAEDICPRO); // Местоимение-предикатив
						else if(temp_buf == "ЧИСЛ")
							rForm.SetTag(SRWG_CLASS, SRWC_NUMERAL);
						else if(temp_buf == "ЧИСЛ-П")
							rForm.SetTag(SRWG_CLASS, SRWC_NUMERALORD);
						else if(temp_buf == "Н")
							rForm.SetTag(SRWG_CLASS, SRWC_ADVERB);
						else if(temp_buf == "ПРЕДК")
							rForm.SetTag(SRWG_CLASS, SRWC_PRAEDIC);
						else if(temp_buf == "ПРЕДЛ")
							rForm.SetTag(SRWG_CLASS, SRWC_PREPOSITION);
						else if(temp_buf == "ПОСЛ")
							rForm.SetTag(SRWG_CLASS, SRWC_POSTPOSITION);
						else if(temp_buf == "СОЮЗ")
							rForm.SetTag(SRWG_CLASS, SRWC_CONJUNCTION);
						else if(temp_buf == "МЕЖД")
							rForm.SetTag(SRWG_CLASS, SRWC_INTERJECTION);
						else if(temp_buf == "ВВОДН")
							rForm.SetTag(SRWG_CLASS, SRWC_PARENTH);
						else if(temp_buf == "ЧАСТ")
							rForm.SetTag(SRWG_CLASS, SRWC_PARTICLE);
						else if(temp_buf == "РАЗРЫВ_СОЮЗ")
							rForm.SetTag(SRWG_CLASS, SRWC_CONJUNCTION);
						else if(temp_buf == "прч")
							rForm.SetTag(SRWG_CLASS, SRWC_PARTICIPLE);
						else if(temp_buf == "ПРИЧАСТИЕ")
							rForm.SetTag(SRWG_CLASS, SRWC_PARTICIPLE);
						else if(temp_buf == "КР_ПРИЧАСТИЕ") {
							rForm.SetTag(SRWG_CLASS, SRWC_PARTICIPLE);
							rForm.SetTag(SRWG_SHORT, SRSHORT_BREV);
						}
						else if(temp_buf == "дпр")
							rForm.SetTag(SRWG_CLASS, SRWC_GPARTICIPLE);
						else if(temp_buf == "ДЕЕПРИЧАСТИЕ")
							rForm.SetTag(SRWG_CLASS, SRWC_GPARTICIPLE);
						else if(temp_buf == "ФРАЗ")
							rForm.SetTag(SRWG_CLASS, SRWC_PHRAS);
						else if(temp_buf == "ед")
							rForm.SetTag(SRWG_COUNT, SRCNT_SINGULAR);
						else if(temp_buf == "мн") {
							int c = rForm.GetTag(SRWG_COUNT);
							if(c == SRCNT_PLURAL) {
								//
								// Если дескриптор множественного числа повторяется дважды, то это означает,
								// что существительное pluralia tantum (всегда во множественном числе).
								//
								rForm.RemoveTag(SRWG_COUNT);
								rForm.SetTag(SRWG_TANTUM, SRTANTUM_PLURAL);
							}
							else if(c) {
								ok = 0; // @err Дважды повторяется дескриптом числа словоформы
								break;
							}
							else
								rForm.SetTag(SRWG_COUNT, SRCNT_PLURAL);
						}
						else if(temp_buf == "дфст")
							rForm.SetTag(SRWG_TANTUM, SRTANTUM_SINGULAR);
						else if(temp_buf == "мр")
							rForm.SetTag(SRWG_GENDER, SRGENDER_MASCULINE);
						else if(temp_buf == "жр")
							rForm.SetTag(SRWG_GENDER, SRGENDER_FEMININE);
						else if(temp_buf == "ср")
							rForm.SetTag(SRWG_GENDER, SRGENDER_NEUTER);
						else if(temp_buf == "мр-жр")
							rForm.SetTag(SRWG_GENDER, SRGENDER_COMMON);
						else if(temp_buf == "фам")
							rForm.SetTag(SRWG_PROPERNAME, SRPROPN_FAMILYNAME);
						else if(temp_buf == "имя")
							rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PERSONNAME);
						else if(temp_buf == "отч")
							rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PATRONYMIC);
						else if(temp_buf == "орг")
							rForm.SetTag(SRWG_PROPERNAME, SRPROPN_ORG);
						else if(temp_buf == "пвл")
							rForm.SetTag(SRWG_MOOD, SRMOOD_IMPERATIVE);
						else if(temp_buf == "инф")
							rForm.SetTag(SRWG_ASPECT, SRASPECT_INFINITIVE);
						else if(temp_buf == "ИНФИНИТИВ")
							rForm.SetTag(SRWG_ASPECT, SRASPECT_INFINITIVE);
						else if(temp_buf == "1л")
							rForm.SetTag(SRWG_PERSON, SRPERSON_FIRST);
						else if(temp_buf == "2л")
							rForm.SetTag(SRWG_PERSON, SRPERSON_SECOND);
						else if(temp_buf == "3л")
							rForm.SetTag(SRWG_PERSON, SRPERSON_THIRD);
						else if(temp_buf == "кр")
							rForm.SetTag(SRWG_SHORT, SRSHORT_BREV);
						else if(temp_buf == "нст")
							rForm.SetTag(SRWG_TENSE, SRTENSE_PRESENT);
						else if(temp_buf == "буд")
							rForm.SetTag(SRWG_TENSE, SRTENSE_FUTURE);
						else if(temp_buf == "прш")
							rForm.SetTag(SRWG_TENSE, SRTENSE_PAST);
						else if(temp_buf == "дст")
							rForm.SetTag(SRWG_VOICE, SRVOICE_ACTIVE);
						else if(temp_buf == "стр")
							rForm.SetTag(SRWG_VOICE, SRVOICE_PASSIVE);
						else if(temp_buf == "од") {
							//
							// В исходном файле может быть одновременно указаны тэги SRANIM_ANIMATE и SRANIM_INANIMATE.
							// Наша техника определения словоформ предполагает, что одушевленность в этом случае
							// не определена. Т.е. форма подходит как к одушевленным, так и неодушевленным вариантам.
							//
							if(rForm.GetTag(SRWG_ANIMATE) == SRANIM_INANIMATE)
								rForm.RemoveTag(SRWG_ANIMATE);
							else
								rForm.SetTag(SRWG_ANIMATE, SRANIM_ANIMATE);
						}
						else if(temp_buf == "но") {
							//
							// See comment above
							//
							if(rForm.GetTag(SRWG_ANIMATE) == SRANIM_ANIMATE)
								rForm.RemoveTag(SRWG_ANIMATE);
							else
								rForm.SetTag(SRWG_ANIMATE, SRANIM_INANIMATE);
						}
						else if(temp_buf == "св")
							rForm.SetTag(SRWG_ASPECT, SRASPECT_PERFECTIVE);
						else if(temp_buf == "нс")
							rForm.SetTag(SRWG_ASPECT, SRASPECT_IMPERFECTIVE);
						else if(temp_buf == "безл")
							rForm.SetTag(SRWG_VALENCY, SRVALENCY_AVALENT);
						else if(temp_buf == "пе")
							rForm.SetTag(SRWG_VALENCY, SRVALENCY_TRANSITIVE);
						else if(temp_buf == "нп")
							rForm.SetTag(SRWG_VALENCY, SRVALENCY_INTRANSITIVE);
						else if(temp_buf == "арх")
							rForm.SetTag(SRWG_USAGE, SRWU_ARCHAIC);
						else if(temp_buf == "жарг")
							rForm.SetTag(SRWG_USAGE, SRWU_VULGAR);
						else if(temp_buf == "разг")
							rForm.SetTag(SRWG_USAGE, SRWU_SPOKEN);
						else if(temp_buf == "кач")
							rForm.SetTag(SRWG_ADJCAT, SRADJCAT_QUALIT);
						else if(temp_buf == "притяж")
							rForm.SetTag(SRWG_ADJCAT, SRADJCAT_POSSESSIVE);
						else if(temp_buf == "прев")
							rForm.SetTag(SRWG_ADJCMP, SRADJCMP_SUPERLATIVE);
						else if(temp_buf == "вопр")
							rForm.SetTag(SRWG_ADVERBCAT, SRADVCAT_INTERROGATIVE);
						else if(temp_buf == "относ")
							rForm.SetTag(SRWG_ADVERBCAT, SRADVCAT_RELATIVE);
						else if(temp_buf == "указат")
							rForm.SetTag(SRWG_ADVERBCAT, SRADVCAT_POINTING);
						else if(temp_buf == "0")
							rForm.SetTag(SRWG_INVARIABLE, 1);
						else if(temp_buf == "лок")
							rForm.SetTag(SRWG_LOCAT, 1);
						else if(temp_buf == "опч")
							rForm.SetTag(SRWG_ERROR, 1);
						else if(temp_buf == "аббр")
							rForm.SetTag(SRWG_ABBR, SRABBR_ABBR);
						else if(temp_buf == "*") {
							//
							// Общая словообразовательная граммема.
							//
						}
						else {
							ok = 0;
							break;
						}
					}
					prev_compr = 0;
				}
			}
		} while(*p);
		rForm.SetTag(SRWG_LANGUAGE, slangRU);
		rForm.Normalize();
	}
	else
		ok = -1;
	return ok;
}

static int ReadAncodeDescrLine_En(const char * pLine, SString & rAncode, SrWordForm & rForm)
{
	int    ok = 1;
	rAncode.Z();

	SString line_buf;
	const char * p = pLine;
	while(*p && *p != '\n' && !(p[0] == '/' && p[1] == '/')) {
		line_buf.CatChar(*p++);
	}
	if(line_buf.NotEmptyS()) {
		/*
			aa 1 ADJECTIVE
			ab 1 ADJECTIVE comp
			ac 1 ADJECTIVE sup
			// many, more  most
			xi 1 NUMERAL
			cb 1 NUMERAL comp
			cc 1 NUMERAL sup
			//  for adjectives like "English", "Russian"
			ad 1 ADJECTIVE prop
			ba 1 ADVERB
			bb 1 ADVERB comp
			bc 1 ADVERB sup
			va 1 VERB inf
			vb 1 VERB prsa,sg,3
			vc 1 VERB pasa
			vd 1 VERB pp
			ve 1 VERB ing
			vf 1 MOD inf
			vh 1 MOD pasa
			ta 1 VBE inf
			tb 1 VBE prsa,sg,1
			td 1 VBE prsa,sg,3
			te 1 VBE prsa,pl
			tf 1 VBE ing
			tg 1 VBE pasa,sg
			ti 1 VBE pasa,pl
			tj 1 VBE pp
			tk 1 VBE fut,1,sg
			tl 1 VBE fut,sg,pl,1,2,3
			tm 1 VBE if,sg,1,2
			tn 1 VBE if,sg,3
			to 1 VBE if,pl
			pa 1 PN pers,nom
			pb 1 PN pers,obj
			pc 1 PN pers,nom,sg,1
			pd 1 PN pers,obj,sg,1
			pe 1 PN pers,nom,2
			pf 1 PN pers,obj,2
			pg 1 PN pers,nom,sg,3
			ph 1 PN pers,obj,sg,3
			pi 1 PN pers,nom,pl,1
			pk 1 PN pers,obj,pl,1
			pl 1 PN pers,nom,pl,3
			pm 1 PN pers,obj,pl,3
			da 1 PN ref,sg
			db 1 PN ref,pl
			ea 1 PN_ADJ poss
			eb 1 PN_ADJ poss,pred
			ec 1 PN_ADJ dem,sg
			ed 1 PN_ADJ dem,pl
			ee 1 PN_ADJ
			ef 1 PRON
			// "table", "town"
			na 1 NOUN narr,sg
			nb 1 NOUN narr,pl
			//  analytical possessive
			fa 1 NOUN narr,poss
			//  nouns which can be mass  and uncount
			// "silk", "clay"
			nc 1 NOUN narr,mass,uncount,sg
			//  analytical possessive
			fb 1 NOUN narr,mass,uncount,poss
			//  mass nouns
			// "water", "butter"
			ne 1 NOUN narr,mass,sg
			ng 1 NOUN narr,mass,pl
			//  analytical possessive
			fc 1 NOUN narr,mass,poss
			//  uncount nouns
			// "acceleration", "activism"
			ni 1 NOUN narr,uncount,sg
			// "John", "James"
			oa 1 NOUN prop,m,sg
			ob 1 NOUN prop,m,pl
			//  analytical possessive
			fd 1 NOUN prop,m,poss
			// "Mary", "Jane"
			oc 1 NOUN prop,f,sg
			od 1 NOUN prop,f,pl
			//  analytical possessive
			fe 1 NOUN prop,f,poss
			// "Glen" "Lee" "Jerry"
			oe 1 NOUN prop,m,f,sg
			of 1 NOUN prop,m,f,pl
			//  analytical possessive
			ff 1 NOUN prop,m,f,poss
			// general geographical names
			ga 1 NOUN prop
			//  analytical possessive
			fg 1 NOUN prop,poss

			xa 1 CONJ
			xb 1 INT
			xc 1 PREP
			xd 1 PART
			xf 1 ARTICLE
			xi 1 NUMERAL
			xp 1 ORDNUM
			yc 1 POSS plsq
			yd 1 POSS plsgs
				//Специальное существительное заглушка, номер кода используется!
			xx 1 NOUN prop sg pl

			// type ancodes
			za 1 * geo
			zb 1 * name
			zc 1 * org
		*/
		SString temp_buf;
		const char * p = line_buf;
		//
		temp_buf.Z();
		while(*p != ' ' && *p != '\t' && *p != 0)
			temp_buf.CatChar(*p++);
		rAncode = temp_buf;
		if(rAncode == "ga") { // Географическое наименование
			rForm.SetTag(SRWG_CLASS, SRWC_NOUN);
			rForm.SetTag(SRWG_PROPERNAME, SRPROPN_GEO);
		}
		else if(rAncode == "ad") { // Прилагательное обозначающее принадлежность к национальности
			rForm.SetTag(SRWG_CLASS, SRWC_ADJECTIVE);
			rForm.SetTag(SRWG_ADJCAT, SRADJCAT_NATION);
		}
		else if(rAncode == "da") { // Возвратное местоимение ед.ч.
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
			rForm.SetTag(SRWG_PRONOUN, SRPRON_REFL);
			rForm.SetTag(SRWG_COUNT, SRCNT_SINGULAR);
		}
		else if(rAncode == "db") { // Возвратное местоимение мн.ч.
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
			rForm.SetTag(SRWG_PRONOUN, SRPRON_REFL);
			rForm.SetTag(SRWG_COUNT, SRCNT_PLURAL);
		}
		else if(rAncode == "ea") {
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
			rForm.SetTag(SRWG_POSSESSIVE, 1);
		}
		else if(rAncode == "eb") {
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
			rForm.SetTag(SRWG_POSSESSIVE,  1);
			rForm.SetTag(SRWG_PREDICATIVE, 1);
		}
		else if(rAncode == "ec") { // Указательное местоимение ед.ч.
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
			rForm.SetTag(SRWG_PRONOUN, SRPRON_DEMONSTR);
			rForm.SetTag(SRWG_COUNT, SRCNT_SINGULAR);
		}
		else if(rAncode == "ed") { // Указательное местоимение мн.ч.
			rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
			rForm.SetTag(SRWG_PRONOUN, SRPRON_DEMONSTR);
			rForm.SetTag(SRWG_COUNT, SRCNT_PLURAL);
		}
		else if(rAncode == "yc") { // possessive group (POSS plsq)
			rForm.SetTag(SRWG_CLASS, SRWC_POSSESSIVEGROUP);
		}
		else if(rAncode == "yd") { // possessive group (POSS plsgs)
			rForm.SetTag(SRWG_CLASS, SRWC_POSSESSIVEGROUP);
		}
		else if(rAncode == "ve") { // ing-форма глагола
			rForm.SetTag(SRWG_CLASS, SRWC_GERUND);
		}
		else if(rAncode == "tf") { // ing-форма глагола to be
			rForm.SetTag(SRWG_CLASS, SRWC_GERUND);
			rForm.SetTag(SRWG_TOBE, 1);
		}
		else {
			//
			while(*p == ' ' || *p == '\t') p++;
			temp_buf.Z();
			while(*p != ' ' && *p != '\t' && *p != 0)
				temp_buf.CatChar(*p++);
			//
			int    prev_case = 0;  // Предыдущий токен обозначал падеж
			int    prev_compr = 0; // Предыдущий токен - сравнительная степень прилагательного SRADJCMP_COMPARATIVE
			do {
				while(*p == ' ' || *p == '\t' || *p == ',') p++;
				temp_buf.Z();
				while(*p != ' ' && *p != '\t' && *p != ',' && *p != 0)
					temp_buf.CatChar(*p++);
				if(temp_buf.NotEmpty()) {
					if(temp_buf == "ADJECTIVE")
						rForm.SetTag(SRWG_CLASS, SRWC_ADJECTIVE);
					else if(temp_buf == "ADVERB")
						rForm.SetTag(SRWG_CLASS, SRWC_ADVERB);
					else if(temp_buf == "ARTICLE")
						rForm.SetTag(SRWG_CLASS, SRWC_ARTICLE);
					else if(temp_buf == "NOUN")
						rForm.SetTag(SRWG_CLASS, SRWC_NOUN);
					else if(temp_buf == "NUMERAL")
						rForm.SetTag(SRWG_CLASS, SRWC_NUMERAL);
					else if(temp_buf == "ORDNUM")
						rForm.SetTag(SRWG_CLASS, SRWC_NUMERALORD);
					else if(temp_buf == "CONJ")
						rForm.SetTag(SRWG_CLASS, SRWC_CONJUNCTION);
					else if(temp_buf == "PN")
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
					else if(temp_buf == "PN_ADJ")
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
					else if(temp_buf == "INT")
						rForm.SetTag(SRWG_CLASS, SRWC_INTERJECTION);
					else if(temp_buf == "MOD") {
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
						rForm.SetTag(SRWG_MODAL, 1);
					}
					else if(temp_buf == "PART") // Частица
						rForm.SetTag(SRWG_CLASS, SRWC_PARTICLE);
					else if(temp_buf == "PREP")
						rForm.SetTag(SRWG_CLASS, SRWC_PREPOSITION);
					else if(temp_buf == "PRON") // Не изменяемые местоимения-существительные (all, anybody, etc)
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
					else if(temp_buf == "VBE") { // Специальный случай - глагол "to be"
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
						rForm.SetTag(SRWG_TOBE, 1);
					}
					else if(temp_buf == "VERB")
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
					else if(temp_buf == "comp")
						rForm.SetTag(SRWG_ADJCMP, SRADJCMP_COMPARATIVE);
					else if(temp_buf == "f")
						rForm.SetTag(SRWG_GENDER, SRGENDER_FEMININE);
					else if(temp_buf == "fut")
						rForm.SetTag(SRWG_TENSE, SRTENSE_FUTURE); // Будущая форма (вероятно, только для to be)
					else if(temp_buf == "if") // Вопросительная форма глагола to be
						rForm.SetTag(SRWG_QUEST, 1);
					else if(temp_buf == "inf")
						rForm.SetTag(SRWG_ASPECT, SRASPECT_INFINITIVE);
					else if(temp_buf == "m")
						rForm.SetTag(SRWG_GENDER, SRGENDER_MASCULINE);
					else if(temp_buf == "geo")
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_GEO);
					else if(temp_buf == "name")
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PERSONNAME);
					else if(temp_buf == "org")
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_ORG);
					else if(temp_buf == "narr") { // Нарицательное существительное
						// (не учитываем - считаем все не собственные существительные нарицательными)
					}
					else if(temp_buf == "nom")
						rForm.SetTag(SRWG_CASE, SRCASE_NOMINATIVE); // Именительный падеж
					else if(temp_buf == "obj")
						rForm.SetTag(SRWG_CASE, SRCASE_OBJECTIVE); // Объектный падеж местоимений
					else if(temp_buf == "pasa")
						rForm.SetTag(SRWG_TENSE, SRTENSE_PAST); // Past Indefinite (2-я форма глагола)
					else if(temp_buf == "pers")
						rForm.SetTag(SRWG_PRONOUN, SRPRON_PERSONAL);
					else if(temp_buf == "pl")
						rForm.SetTag(SRWG_COUNT, SRCNT_PLURAL);
					else if(temp_buf == "pp")
						rForm.SetTag(SRWG_TENSE, SRTENSE_PASTPARTICIPLE); // Past Participle (3-я форма глагола)
					else if(temp_buf == "poss")
						rForm.SetTag(SRWG_POSSESSIVE, 1);
					else if(temp_buf == "pred") // предикатив (форма притяжательных местоимений, напр.:yours)
						rForm.SetTag(SRWG_PREDICATIVE, 1);
					else if(temp_buf == "attr") { // @unused атрибутив (форма притяжательных местоимений, напр.:your);
					}
					else if(temp_buf == "prop")
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PERSONNAME);
					else if(temp_buf == "prsa")
						rForm.SetTag(SRWG_TENSE, SRTENSE_PRESENT); // Present (1-я форма глагола)
					else if(temp_buf == "sg")
						rForm.SetTag(SRWG_COUNT, SRCNT_SINGULAR);
					else if(temp_buf == "sup")
						rForm.SetTag(SRWG_ADJCMP, SRADJCMP_SUPERLATIVE);
					else if(temp_buf == "uncount")
						rForm.SetTag(SRWG_COUNTAB, SRCTB_UNCOUNTABLE);
					else if(temp_buf == "mass")
						rForm.SetTag(SRWG_COUNTAB, SRCTB_MASS);
					else if(temp_buf == "1")
						rForm.SetTag(SRWG_PERSON, SRPERSON_FIRST);
					else if(temp_buf == "2")
						rForm.SetTag(SRWG_PERSON, SRPERSON_SECOND);
					else if(temp_buf == "3")
						rForm.SetTag(SRWG_PERSON, SRPERSON_THIRD);
					/* покрывается случаями "yc", "yd" обработанными выше
					else if(temp_buf == "POSS") {
					}
					else if(temp_buf == "plsgs") {
					}
					else if(temp_buf == "plsq") {
					}
					*/
					/* покрывается случаями "ve", "tf" обработанными выше
					else if(temp_buf == "ing") {
					}
					*/
					/* покрывается случаями "da", "db" обработанными выше
					else if(temp_buf == "ref") {
					}
					*/
					/* покрывается случаями "ec", "ed" обработанными выше
					else if(temp_buf == "dem") {
					}
					*/
				}
			} while(*p);
		}
		rForm.SetTag(SRWG_LANGUAGE, slangEN);
		rForm.Normalize();
	}
	else
		ok = -1;
	return ok;
}

int SrDatabase::ImportFlexiaModel(const SrImportParam & rParam)
{
	assert(oneof2(rParam.LangID, slangRU, slangEN));
	int    ok = 1, r;
	if(oneof2(rParam.LangID, slangRU, slangEN)) {
		SymbHashTable anc_tab(4096, 0);
		LAssocArray fm_assoc; // Список ассоциаций индекса модели с идентификатором модели в базе данных
		LAssocArray pfx_assoc; // Список ассоциаций индекса приставки с идентификатором приставки в базе данных
		SString line_buf, ancode, temp_buf;
		SString item_buf, afx_buf, anc_buf, pfx_buf, word_buf;
		int32  base_skeleton_wf_id = 0;
		SFile  anc_file;
		SFile  fm_file;
		SFile  test_out_file;
		SStrScan scan;
		THROW(rParam.GetField(rParam.fldAncodeFileName, temp_buf) > 0);
		THROW(anc_file.Open(temp_buf, SFile::mRead));
		PPWaitMsg((line_buf = "ImportFlexiaModel").Space().Cat(temp_buf));
		//
		THROW(rParam.GetField(rParam.fldFlexiaModelFileName, temp_buf) > 0);
		THROW(fm_file.Open(temp_buf, SFile::mRead));
		if(rParam.Flags & rParam.fTest) {
			SPathStruc::ReplaceExt(temp_buf, "out", 1);
			test_out_file.Open(temp_buf, SFile::mWrite);
		}
		//
		{
			BDbTransaction tra(P_Db, 1);
			THROW(tra);
			{
				//
				// Импортируем список словоформ
				//
				SrWordForm form;
				while(anc_file.ReadLine(line_buf)) {
					form.Clear();
					int   anc_r = 0;
					if(rParam.LangID == slangRU)
						anc_r = ReadAncodeDescrLine_Ru(line_buf.Chomp(), ancode, form);
					else if(rParam.LangID == slangEN)
						anc_r = ReadAncodeDescrLine_En(line_buf.Chomp(), ancode, form);
					if(anc_r > 0) {
						int32 wf_id = 0;
						THROW(r = P_GrT->Search(&form, &wf_id));
						if(r < 0) {
							THROW(P_GrT->Add(&form, &(wf_id = 0)));
							if(rParam.Flags & rParam.fTest) {
								int32 id2 = 0;
								int r2 = P_GrT->Search(&form, &id2);
								THROW(r2 > 0);
								THROW(id2 == wf_id);
							}
						}
						anc_tab.Add(ancode, wf_id);
					}
				}
				//
				// Находим или создаем дескриптор базовой словоформы для основы слова
				//
				{
					form.Clear();
					form.SetTag(SRWG_LANGUAGE, rParam.LangID);
					form.SetTag(SRWG_CLASS, SRWC_SKELETON);
					form.Normalize();
					THROW(r = P_GrT->Search(&form, &base_skeleton_wf_id));
					if(r < 0) {
						THROW(P_GrT->Add(&form, &(base_skeleton_wf_id = 0)));
						if(rParam.Flags & rParam.fTest) {
							int32 id2 = 0;
							int r2 = P_GrT->Search(&form, &id2);
							THROW(r2 > 0);
							THROW(id2 == base_skeleton_wf_id);
						}
					}
				}
			}
			//
			{
				//
				// Импортируем список моделей окончаний (FlexiaModel)
				//
				if(fm_file.ReadLine(line_buf)) {
					const long fm_count = line_buf.ToLong();
					SrFlexiaModel model, model_test;
					THROW(fm_count > 0);
					//
					// В следующем цикле индекс i выступает одновременно в качестве идентификатора, используемого
					// для идентификации модели при описании леммы слова.
					//
					for(long i = 0; i < fm_count; i++) {
						THROW(fm_file.ReadLine(line_buf));
						line_buf.Chomp();
						scan.Set(line_buf, 0);
						if(scan.Skip().SearchChar('%')) {
							model.Clear();
							scan.IncrLen(1);
							for(int eol = 0; !eol;) {
								if(scan.SearchChar('%')) {
									scan.Get(item_buf);
									scan.IncrLen(1);
								}
								else {
									scan.Len = (line_buf.Len() - scan.Offs);
									scan.Get(item_buf);
									eol = 1;
								}
								if(item_buf.NotEmptyS() && item_buf.Divide('*', afx_buf, anc_buf) > 0) {
									(temp_buf = anc_buf).Divide('*', anc_buf, pfx_buf);
									if(anc_buf.NotEmptyS()) {
										uint   anc_id = 0;
										SrFlexiaModel::Item item;
										if(anc_tab.Search(anc_buf, &anc_id, 0) > 0) {
											if(afx_buf.NotEmptyS()) {
												afx_buf.ToUtf8().Utf8ToLower();
												THROW(P_WdT->AddSpecial(SrWordTbl::spcAffix, afx_buf, &item.AffixID));
											}
											if(pfx_buf.NotEmptyS()) {
												pfx_buf.ToUtf8().Utf8ToLower();
												THROW(P_WdT->AddSpecial(SrWordTbl::spcPrefix, pfx_buf, &item.PrefixID));
											}
											item.WordFormID = (int32)anc_id;
											THROW(model.Add(item));
										}
										else {
											; // @warn "Ancode '%s' not found"
										}
									}
									else {
										; // @warn "Ancode not defined"
									}
								}
							}
							{
								int32  model_id = 0;
								size_t prev_len = model.GetLength(); // @debug
								THROW(model.Normalize());
								assert(prev_len == model.GetLength()); // @debug
								THROW(r = P_GrT->Search(&model, &model_id));
								if(r < 0) {
									THROW(P_GrT->Add(&model, &(model_id = 0)));
								}
								else if(rParam.Flags & rParam.fTest) {
									THROW(r = P_GrT->Search(model_id, &model_test));
									THROW(model.IsEqual(model_test));
								}
								fm_assoc.Add(i, model_id, 0);
							}
						}
					}
				}
			}
			{
				//
				// Импортируем список моделей ударений (AccentModel)
				//
				if(fm_file.ReadLine(line_buf)) {
					long   am_count = line_buf.ToLong();
					THROW(am_count > 0);
					for(long i = 0; i < am_count; i++) {
						//
						// Пока просто сканируем строки с моделями, но не акцептируем их в базу данных
						//
						THROW(fm_file.ReadLine(line_buf));
					}
				}
			}
			{
				//
				// Пропускаем журнал сессий
				//
				if(fm_file.ReadLine(line_buf)) {
					long   sess_count = line_buf.ToLong();
					THROW(sess_count > 0);
					for(long i = 0; i < sess_count; i++) {
						//
						// Пока просто сканируем строки с моделями, но не акцептируем их в базу данных
						//
						THROW(fm_file.ReadLine(line_buf));
					}
				}
			}
			{
				//
				// Импортируем список приставок
				//
				if(fm_file.ReadLine(line_buf)) {
					line_buf.Chomp().Strip();
					long   pfx_count = line_buf.ToLong();
					THROW(pfx_count > 0 || line_buf == "0");
					for(long i = 0; i < pfx_count; i++) {
						LEXID  pfx_id = 0;
						THROW(fm_file.ReadLine(line_buf));
						(pfx_buf = line_buf.Chomp().Strip()).ToUtf8().Utf8ToLower();
						THROW(P_WdT->AddSpecial(SrWordTbl::spcPrefix, pfx_buf, &pfx_id));
						pfx_assoc.Add(i, pfx_id, 0);
					}
				}
			}
			{
				// АВИАПРОМ 32 20 6 Уэ -
				// 1 - основа слова, 2 - индекс FlexiaModel, 3 - индекс AccentModel, 4 - индекс сессии, 5 - базовый ancode, 6 - индекс приставки

				//
				// Наконец, импортируем основы слов с правилами преобразования //
				//
				if(fm_file.ReadLine(line_buf)) {
					TSVector <SrWordAssoc> test_wa_list; // @v9.8.4 TSArray-->TSVector
					long   lm_count = line_buf.ToLong();
					THROW(lm_count > 0);
					for(long i = 0; i < lm_count; i++) {
						long   temp_val;
						int32  wa_id = 0;
						SrWordAssoc wa;
						THROW(fm_file.ReadLine(line_buf));
						line_buf.Chomp().Strip();
						scan.Set(line_buf, 0);
						if(scan.Skip().SearchChar(' ')) {
							scan.Get(item_buf);
							scan.IncrLen(1);
							word_buf = item_buf;
							item_buf.ToUtf8().Utf8ToLower();
							if(item_buf == "#") {
								THROW(P_WdT->AddSpecial(SrWordTbl::spcEmpty, item_buf, &wa.WordID));
							}
							else {
								THROW(P_WdT->Add(item_buf, &wa.WordID));
							}
							if(scan.Skip().SearchChar(' ')) {
								scan.Get(item_buf);
								scan.IncrLen(1);
								temp_val = item_buf.ToLong();
								if(temp_val >= 0) {
									fm_assoc.Search(temp_val, &wa.FlexiaModelID, 0);
								}
								if(scan.Skip().SearchChar(' ')) {
									scan.Get(item_buf);
									scan.IncrLen(1);
									// accent_model_id - пока пропускаем
									if(scan.Skip().SearchChar(' ')) {
										scan.Get(item_buf);
										scan.IncrLen(1);
										// индекс сессии - пропускаем
										if(scan.Skip().SearchChar(' ')) {
											scan.Get(item_buf);
											scan.IncrLen(1);
											item_buf.Strip(); // базовый ancode
											uint   anc_id = 0;
											if(!(item_buf == "-") && anc_tab.Search(item_buf, &anc_id, 0) > 0)
												wa.BaseFormID = anc_id;
											SETIFZ(wa.BaseFormID, base_skeleton_wf_id);
											if(scan.Skip().SearchChar(' ')) {
												scan.Get(item_buf);
												scan.IncrLen(1);
												if(!(item_buf == "-")) {
													temp_val = item_buf.ToLong();
													if(temp_val >= 0)
														pfx_assoc.Search(temp_val, &wa.PrefixID, 0);
												}
											}
										}
									}
								}
							}
						}
						if(wa.FlexiaModelID || wa.BaseFormID) {
							THROW(P_WaT->Add(&wa.Normalize(), &wa_id));
							if(rParam.Flags & rParam.fTest) {
								P_WaT->Search(wa.WordID, test_wa_list);
								for(uint j = 0; j < test_wa_list.getCount(); j++) {
									const SrWordAssoc & r_wa = test_wa_list.at(j);
									line_buf.Z().Cat(word_buf).Tab(2).Cat(r_wa.ToStr(temp_buf)).CR();
									test_out_file.WriteLine(line_buf);
								}
							}
						}
					}
				}
			}
			THROW(tra.Commit(1));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int TestImport_Words_MySpell()
{
	int    ok = 1;
	SString line_buf;
	BDbDatabase bdb("/PAPYRUS/PPY/BIN/SARTRDB");
	THROW(bdb);
	{
		SFile  in_file("\\PAPYRUS\\Src\\SARTR\\data\\RU_RU-W.DIC", SFile::mRead);
		THROW(in_file.IsValid());
		if(in_file.ReadLine(line_buf)) { // Первая строка содержит количество строк в словаре
			long   _count = line_buf.ToLong();
			long   line_no = 0;
			SString word, sfx_idx, temp_buf;
			if(_count > 0) {
				StrAssocArray test_list;
				LEXID  id = 0;
				{
					SrWordTbl tbl_words(&bdb);
					THROW(tbl_words);
					while(in_file.ReadLine(line_buf)) {
						line_no++;
						line_buf.Chomp().Divide('/', word, sfx_idx);
						(temp_buf = word).ToUtf8();
						int r = tbl_words.Add(temp_buf, &id);
						THROW(r);
						if(r > 0)
							test_list.Add(id, word);
					}
				}
				//
				//
				//
				{
					const uint cc = test_list.getCount();
					uint i;
					LongArray pos_list;
					SrWordTbl tbl_words(&bdb);
					THROW(tbl_words);
					for(i = 0; i < cc; i++) {
						pos_list.add(i);
					}
					pos_list.shuffle();
					assert(pos_list.getCount() == cc);
					for(i = 0; i < cc; i++) {
						StrAssocArray::Item item = test_list.Get(pos_list.get(i));
						int r = tbl_words.Search((word = item.Txt).ToUtf8(), &id);
						if(r <= 0 || id != item.Id) {
							// error
							sfx_idx = "error";
						}
					}
				}
				{
					const uint cc = test_list.getCount();
					uint i;
					LongArray pos_list;
					SrWordTbl tbl_words(&bdb);
					THROW(tbl_words);
					for(i = 0; i < cc; i++) {
						pos_list.add(i);
					}
					pos_list.shuffle();
					assert(pos_list.getCount() == cc);
					for(i = 0; i < cc; i++) {
						StrAssocArray::Item item = test_list.Get(pos_list.get(i));
						int r = tbl_words.Search(item.Id, word);
						word.Utf8ToChar();
						if(r <= 0 || word != item.Txt) {
							// error
							sfx_idx = "error";
						}
					}
				}

			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class SrConceptParser {
public:
	class Operator {
	public:
		explicit Operator(SrConceptParser & rMaster);
		Operator & Z();
		int    IsEmpty() const;
		int    Close(int ifNeeded);
		int    IsClosed() const;
		//
		// Descr: Извлекает идентификатор языка либо из данного элемента, либо, если здесь он не определен - из родительского.
		//
		int    GetLangID() const;
		Operator * CreateNext();
		Operator * CreateChild();
		Operator * FindParent();
		void   ResetAbbr()
		{
			Flags &= ~(fAbbr|fAbbrDot);
		}
		void   SetAbbr(int dot)
		{
			if(dot) {
				Flags |= fAbbrDot;
				Flags &= ~fAbbr;
			}
			else {
				Flags |= fAbbr;
				Flags &= ~fAbbrDot;
			}
		}

		enum {
			fClosed   = 0x0001, // Оператор закрыт (то есть считана полная конструкция оператора).
			fAccepted = 0x0002, // Оператор учтен в базе данных вызовом SrConceptParser::PostprocessOpList(Operator *)
			fAbbr     = 0x0004, // this - оператор аббревиатуры
			fAbbrDot  = 0x0008  // this - оператор аббревиатуры с опциональной точкой в конце
		};
		SrConceptParser & R_Master; // @anchor
		CONCEPTID  CID;
		CONCEPTID  InstanceOf; // Концепция this является экземпляром концепции SubclassOf
		CONCEPTID  SubclassOf; // Концепция this является подклассом концепции SubclassOf
		NGID   AbbrOf;         // Лексема this является аббревиатурой AbbrOf
		int    CrType;         // Концепция this имеет тип CrType
		LEXID  CLexID;
		int32  WaIdOfCLex;     // Ид ассоциации словоформ для CLexID, созданный или найденный при идентификации.
		NGID   NgID__;
		int16  LangID;
		int16  Flags;
		Int64Array EqToList;
		SrCPropDeclList Pdl;
		Operator * P_Parent;
		Operator * P_Child;
		Operator * P_Next;
		Operator * P_Prev;
	};
	enum {
		tokUnkn = -1,
		tokEnd = 0,
		tokSpace = 1,
		tokComment,
		tokColon,           // :
		tokColon2,          // ::
		tokEq,              // =
		tokEqColon,         // =:
		tokLBrace,          // {
		tokRBrace,          // }
		tokLBracket,        // [
		tokRBracket,        // ]
		tokLPar,            // (
		tokRPar,            // )
		tokSemicol,         // ;
		tokDot,             // .
		tokComma,           // ,
		tokSharp,           // #
		tokSlash,           // / @v9.8.12
		tokBracketsDescr,   // [abc]
		tokConcept,         // :concept
		tokSubclassOf,      // ::concept
		tokExprOf,          // =:concept
		tokPropDeclStart,   // +(
		tokNumber,
		tokWord,
		tokTypeInt,         // #int
		tokTypeReal,        // #real
		tokTypeStr,         // #str
		//tokTypeHDate,       // #hdate
		tokTypeUniTime,     // #unitime
		tokTypeHPeriod,     // #hperiod
		tokEqAbbrev,        // @v9.8.12 =$ кг=$килограмм
		tokEqAbbrevDot,     // @v9.8.12 =. ул=.улица    ул или ул.
	};

	static int FASTCALL _IsWordbreakChar(int c);

	explicit SrConceptParser(SrDatabase & rDb);
	~SrConceptParser();
	int    Run(const char * pFileName);
	int    _ReadLine();
	int    FASTCALL _GetToken(SString & rExtBuf);
	int    _SkipSpaces(int * pToken, SString & rExtBuf);
	int    FASTCALL _IsTypeToken(int token) const;
	int    FASTCALL _GetTypeByToken(int token) const;
	int    FASTCALL _IsIdent(const char * pText) const;
	//LEXID  FASTCALL RecognizeWord(const char *);
	//NGID   FASTCALL RecognizeNGram(const LongArray & rWordList);
	int    GetWordOrNgConstruct(int & rTok, SString & rWordBuf, SString & rW, StringSet & rNg);
	//CONCEPTID FASTCALL RecognizeConcept(const char *);
	int    PostprocessOpList(Operator * pRoot);
	int    ApplyConceptPropList(const StrAssocArray & rTokList, CONCEPTID cid);
	int OpenInput(const char * pFileName)
	{
		CloseInput();
		LineBuf.Z();
		LineNo = 0;
		if(F.Open(pFileName, SFile::mRead))
			return 1;
		else
			return 0;
	}
	void CloseInput()
	{
		F.Close();
	}
	SrCPropList PropList;
	SFile  F;
	SStrScan Scan;
	SString LineBuf;
	uint   LineNo;
	long   ReH_BrDescr;
	long   ReH_Concept;
	long   ReH_SubclsOf;
	long   ReH_ExprOf;
	SrDatabase & R_Db;
};

SrConceptParser::Operator::Operator(SrConceptParser & rMaster) : R_Master(rMaster)
{
	Z();
}

SrConceptParser::Operator & SrConceptParser::Operator::Z()
{
	CID = 0;
	InstanceOf = 0;
	SubclassOf = 0;
	AbbrOf = 0;
	CrType = 0;
	CLexID = 0;
	WaIdOfCLex = 0;
	NgID__ = 0;
	LangID = 0;
	Flags = 0;
	P_Parent = 0;
	P_Child = 0;
	P_Next = 0;
	P_Prev = 0;
	Pdl.Z();
	EqToList.clear();
	return *this;
}

int SrConceptParser::Operator::IsEmpty() const
	{ return BIN(!CID && !CLexID && !NgID__ && !LangID && !P_Child); }
int SrConceptParser::Operator::IsClosed() const
	{ return BIN(Flags & fClosed); }

int SrConceptParser::Operator::Close(int ifNeeded)
{
	if(!ifNeeded || (!IsClosed() && !IsEmpty())) {
		assert(!(Flags & fClosed));
		Flags |= fClosed;
		return 1;
	}
	else
		return -1;
}

int SrConceptParser::Operator::GetLangID() const
{
	int    lid = 0;
	if(LangID)
		lid = LangID;
	else if(P_Parent)
		lid = P_Parent->LangID; // Здесь рекурсии нет - если для родителя язык не определен, то все!
	else if(P_Prev)
		lid = P_Prev->GetLangID(); // @recursion
	return lid;
}

SrConceptParser::Operator * SrConceptParser::Operator::CreateNext()
{
	assert(P_Next == 0);
	Operator * p_next = new Operator(R_Master);
	if(p_next) {
		P_Next = p_next;
		p_next->P_Prev = this;
	}
	else
		PPSetErrorNoMem();
	return p_next;
}

SrConceptParser::Operator * SrConceptParser::Operator::CreateChild()
{
	assert(P_Child == 0);
	Operator * p_child = new Operator(R_Master);
	if(p_child) {
		P_Child = p_child;
		p_child->P_Parent = this;
	}
	else
		PPSetErrorNoMem();
	return p_child;
}

SrConceptParser::Operator * SrConceptParser::Operator::FindParent()
{
	Operator * p_cur = this;
	Operator * p_par = p_cur->P_Parent;
	while(p_par == 0) {
		p_cur = p_cur->P_Prev;
		if(p_cur)
			p_par = p_cur->P_Parent;
		else
			break;
	}
	return p_par;
}
//
//
//
SrConceptParser::SrConceptParser(SrDatabase & rDb) : R_Db(rDb), LineNo(0), ReH_BrDescr(0), ReH_Concept(0), ReH_SubclsOf(0), ReH_ExprOf(0)
{
	Scan.RegisterRe("^\\[.*\\]", &ReH_BrDescr);
	Scan.RegisterRe("^\\:[_a-zA-Z][_0-9a-zA-Z]*", &ReH_Concept);
	Scan.RegisterRe("^\\:\\:[_a-zA-Z][_0-9a-zA-Z]*", &ReH_SubclsOf);
	Scan.RegisterRe("^\\=\\:[_a-zA-Z][_0-9a-zA-Z]*", &ReH_ExprOf);
}

SrConceptParser::~SrConceptParser()
{
}

int SrConceptParser::_ReadLine()
{
	int    ok = 1;
	if(F.ReadLine(LineBuf)) {
		LineNo++;
		Scan.Set(LineBuf.Chomp().Strip(), 0);
		if(Scan.Search("//")) {
			SString temp_buf;
			Scan.Get(temp_buf);
			Scan.Set((LineBuf = temp_buf).Strip(), 0);
		}
	}
	else
		ok = 0;
	return ok;
}

/*static*/int FASTCALL SrConceptParser::_IsWordbreakChar(int c)
{
	static const char * p_wb = " \t{}[]()<>,.;:!@#%^&*=+`~'?№/|\\";
	return (c == 0 || sstrchr(p_wb, c));
}

int SrConceptParser::_SkipSpaces(int * pToken, SString & rExtBuf)
{
	int    ret = -1;
	int    tok = 0;
	do {
		tok = _GetToken(rExtBuf);
		if(oneof2(tok, tokSpace, tokEnd))
			ret = 1;
		while(ret && tok == tokEnd) {
			if(_ReadLine())
				tok = _GetToken(rExtBuf);
			else {
				tok = tokEnd;
				ret = 0;
			}
		}
	} while(ret && tok == tokSpace);
	ASSIGN_PTR(pToken, tok);
	return ret;
}

int FASTCALL SrConceptParser::_IsIdent(const char * pText) const
{
// @v10.9.8 #define _ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
	char c = *pText++;
	if(isasciialpha(c) || c == '_') {
		do {
			c = *pText++;
			if(c && !(isasciialnum(c) || c == '_'))
				return 0;
		} while(c);
		return 1;
	}
	else
		return 0;
// @v10.9.8 #undef _ISALPHA
}

struct SrCTypeSymb {
	int    Type;
	int    Token;
	const char * P_Symb;
};

static const SrCTypeSymb _CTypeSymbList[] = {
	{ SRPROPT_INT,     SrConceptParser::tokTypeInt,     "int" },
	{ SRPROPT_STRING,  SrConceptParser::tokTypeStr,     "str" },
	{ SRPROPT_REAL,    SrConceptParser::tokTypeReal,    "real" },
	{ SRPROPT_UNITIME,   SrConceptParser::tokTypeUniTime,   /*"hdate"*/"unitime" },
	{ SRPROPT_HPERIOD, SrConceptParser::tokTypeHPeriod, "hperiod" }
};

int FASTCALL SrConceptParser::_IsTypeToken(int token) const
{
	for(uint i = 0; i < SIZEOFARRAY(_CTypeSymbList); i++) {
		if(token == _CTypeSymbList[i].Token)
			return 1;
	}
	return 0;
}

int FASTCALL SrConceptParser::_GetTypeByToken(int token) const
{
	for(uint i = 0; i < SIZEOFARRAY(_CTypeSymbList); i++) {
		if(token == _CTypeSymbList[i].Token)
			return _CTypeSymbList[i].Type;
	}
	return PPSetError(PPERR_SR_TYPEBYTOKNFOUND, token);
}

int FASTCALL SrConceptParser::_GetToken(SString & rExtBuf)
{
	rExtBuf.Z();
	int    tok = 0;
	char   c = Scan[0];
	switch(c) {
		case 0: tok = tokEnd; break;
		case '{': tok = tokLBrace; Scan.Incr(); break;
		case '}': tok = tokRBrace; Scan.Incr(); break;
		case '(': tok = tokLPar; Scan.Incr(); break;
		case ')': tok = tokRPar; Scan.Incr(); break;
		case ';': tok = tokSemicol; Scan.Incr(); break;
		case '.': tok = tokDot; Scan.Incr(); break;
		case ',': tok = tokComma; Scan.Incr(); break;
		case ' ':
		case '\t':
			do {
				Scan.Incr();
				c = Scan[0];
			} while(oneof2(c, ' ', '\t'));
			tok = tokSpace;
			break;
		case '#':
			{
				Scan.Incr();
				for(uint i = 0; i < SIZEOFARRAY(_CTypeSymbList); i++) {
					if(Scan.Is(_CTypeSymbList[i].P_Symb)) {
						tok = _CTypeSymbList[i].Token;
						Scan.Incr(sstrlen(_CTypeSymbList[i].P_Symb));
						break;
					}
				}
				if(!tok)
					tok = tokSharp;
			}
			break;
		case '\"':
			{
				Scan.Incr();
				SString temp_buf;
				int    uc = Scan.GetUtf8(temp_buf);
				while(uc != 0 && !(uc == 1 && temp_buf.Last() == '\"')) {
					//if(uc == 1)
					rExtBuf.Cat(temp_buf);
					uc = Scan.GetUtf8(temp_buf);
				}
				if(uc == 0) {
					tok = tokUnkn; // @error Не завершенное, обрамленное кавычками, слово.
				}
				else if(rExtBuf.NotEmpty()) {
					tok = tokWord;
				}
				else {
					tok = tokUnkn; // @error Пустое слово, обрамленное кавычками
				}
			}
			break;
		default:
			if(c == '+' && Scan[1] == '(') {
				tok = tokPropDeclStart;
				Scan.Incr(2);
			}
			else if(Scan.GetRe(ReH_BrDescr, rExtBuf)) {
				rExtBuf.TrimRightChr(']').ShiftLeftChr('[');
				tok = tokBracketsDescr;
			}
			else if(Scan.GetRe(ReH_SubclsOf, rExtBuf)) {
				rExtBuf.ShiftLeftChr(':').ShiftLeftChr(':');
				tok = tokSubclassOf;
			}
			else if(Scan.GetRe(ReH_Concept, rExtBuf)) {
				rExtBuf.ShiftLeftChr(':');
				tok = tokConcept;
			}
			else if(Scan.GetRe(ReH_ExprOf, rExtBuf)) {
				rExtBuf.ShiftLeftChr('=').ShiftLeftChr(':');
				tok = tokExprOf;
			}
			else if(Scan.GetNumber(rExtBuf)) {
				tok = tokNumber;
			}
			else {
				switch(c) {
					case ':':
						if(Scan[1] == ':') {
							tok = tokColon2;
							Scan.Incr(2);
						}
						else {
							tok = tokColon;
							Scan.Incr();
						}
						break;
					case '=':
						if(Scan[1] == ':') { // =:
							tok = tokEqColon;
							Scan.Incr(2);
						}
						else if(Scan[1] == '$') { // =$
							tok = tokEqAbbrev;
							Scan.Incr(2);
						}
						else if(Scan[1] == '.') { // =.
							tok = tokEqAbbrevDot;
							Scan.Incr(2);
						}
						else {
							tok = tokEq;
							Scan.Incr();
						}
						break;
					case '[': tok = tokLBracket; Scan.Incr(); break;
					case ']': tok = tokRBracket; Scan.Incr(); break;
					case '/': tok = tokSlash; Scan.Incr(); break;
					default:
						{
							SString temp_buf;
							int    uc = Scan.GetUtf8(temp_buf);
							while(uc != 0 && !(uc == 1 && _IsWordbreakChar(temp_buf.Last()))) {
								rExtBuf.Cat(temp_buf);
								uc = Scan.GetUtf8(temp_buf);
							}
							if(uc == 1) {
								Scan.Offs -= uc; // Откатываем указатель Scan на разделитель слов.
							}
							if(rExtBuf.NotEmpty())
								tok = tokWord;
							else
								tok = tokUnkn;
						}
						break;
				}
			}
	}
	return tok;
}

int SrConceptParser::PostprocessOpList(Operator * pRoot)
{
	int    ok = 1;
	SString temp_buf;
	TSVector <SrWordAssoc> ex_wa_list;
	for(Operator * p_current = pRoot; p_current; p_current = p_current->P_Next) {
		if(p_current->Flags & p_current->fClosed && !(p_current->Flags & p_current->fAccepted)) {
			if(p_current->AbbrOf) {
				assert(p_current->Flags & (Operator::fAbbr|Operator::fAbbrDot));
				assert(p_current->CLexID);
				assert(!p_current->CID);
				int    _done = 0;
				if(p_current->WaIdOfCLex) {
					//
					// Если при разборе оператора мы создали упрощенный набор тегов лексемы, то в нем
					// и попытаемся указать что речь идет об аббревиатуре.
					//
					SrWordAssoc ex_wa;
					THROW(R_Db.P_WaT->Search(p_current->WaIdOfCLex, &ex_wa) > 0);
					if(ex_wa.AbbrExpID) {
						if(ex_wa.AbbrExpID != p_current->AbbrOf) {
							SrWordAssoc wa = ex_wa;
							assert(wa.WordID == p_current->CLexID); // @paranoic
							wa.AbbrExpID = p_current->AbbrOf;
							wa.Flags |= SrWordAssoc::fHasAbbrExp;
							if(p_current->Flags & Operator::fAbbrDot)
								wa.Flags |= SrWordAssoc::fAbbrDotOption;
							int32   wa_id = 0;
							THROW(R_Db.P_WaT->Add(&wa.Normalize(), &wa_id));
						}
						_done = 1;
					}
					else {
						assert(ex_wa.WordID == p_current->CLexID);
						ex_wa.AbbrExpID = p_current->AbbrOf;
						ex_wa.Flags |= SrWordAssoc::fHasAbbrExp;
						if(p_current->Flags & Operator::fAbbrDot)
							ex_wa.Flags |= SrWordAssoc::fAbbrDotOption;
						THROW(R_Db.P_WaT->Update(ex_wa.Normalize()));
						_done = 1;
					}
				}
				else {
					//
					// Пытаемся выяснить, не является ли лексема уже той аббревиатурой, которую мы хотим установить
					//
					R_Db.P_WaT->Search(p_current->CLexID, ex_wa_list);
					for(uint i = 0; i < ex_wa_list.getCount(); i++) {
						const SrWordAssoc & r_ex_wa = ex_wa_list.at(i);
						if(r_ex_wa.AbbrExpID && r_ex_wa.AbbrExpID == p_current->AbbrOf)
							_done = 1;
					}
				}
				if(!_done) {
					//
					// После всех стараний выяснилось, что лексема не определена как аббревиатура нашей ngram'ы.
					// Что ж - создаем SrWordAssoc в котором укажем сей факт.
					//
					SrWordAssoc wa;
					wa.WordID = p_current->CLexID;
					wa.AbbrExpID = p_current->AbbrOf;
					wa.Flags = SrWordAssoc::fHasAbbrExp;
					if(p_current->Flags & Operator::fAbbrDot)
						wa.Flags |= SrWordAssoc::fAbbrDotOption;
					int32   wa_id = 0;
					THROW(R_Db.P_WaT->Add(&wa.Normalize(), &wa_id));
				}
			}
			else {
				assert(!(p_current->Flags & (Operator::fAbbr|Operator::fAbbrDot)));
				if(p_current->CID) {
					if(p_current->Pdl.GetCount()) {
						SrConcept cp;
						THROW(R_Db.P_CT->SearchByID(p_current->CID, &cp) > 0);
						THROW(cp.Pdl.Merge(p_current->Pdl));
						THROW(R_Db.P_CT->Update(cp));
					}
					if(p_current->InstanceOf) {
						CONCEPTID prop_instance = R_Db.GetReservedConcept(R_Db.rcInstance);
						THROW(prop_instance);
						THROW(R_Db.SetConceptProp(p_current->CID, prop_instance, 0, p_current->InstanceOf));
					}
					if(p_current->SubclassOf) {
						CONCEPTID prop_subclass = R_Db.GetReservedConcept(R_Db.rcSubclass);
						THROW(prop_subclass);
						THROW(R_Db.SetConceptProp(p_current->CID, prop_subclass, 0, p_current->SubclassOf));
					}
					if(p_current->CrType) {
						CONCEPTID prop_crtype = R_Db.GetReservedConcept(R_Db.rcType);
						THROW(prop_crtype);
						THROW(R_Db.SetConceptProp(p_current->CID, prop_crtype, 0, p_current->CrType));
					}
				}
			}
			if(p_current->EqToList.getCount()) {
				if(p_current->NgID__) {
					for(uint i = 0; i < p_current->EqToList.getCount(); i++) {
						CONCEPTID cid = p_current->EqToList.get(i);
						if(cid) {
							THROW(R_Db.P_CNgT->Set(cid, p_current->NgID__));
						}
					}
				}
			}
			p_current->Flags |= p_current->fAccepted;
		}
		if(p_current->P_Child) {
			THROW(PostprocessOpList(p_current->P_Child)); // @recursion
		}
	}
	CATCHZOK
	return ok;
}

int SrConceptParser::ApplyConceptPropList(const StrAssocArray & rTokList, CONCEPTID cid)
{
	int    ok = 1;
	SString temp_buf, ident_buf;
	SrCPropDeclList pdl;
	LongArray ngram;
	THROW(R_Db.GetPropDeclList(cid, pdl));
	uint   i = 0;
	for(int do_get_next_prop = 1; do_get_next_prop;) {
		StrAssocArray::Item titem = rTokList.at_WithoutParent(i++);
		int    tok = titem.Id;
		temp_buf = titem.Txt;
		//
		// (свойство, свойство, ..., свойство)
		//
		// Варианты свойства:
		// prop_symb = :concept               // #1
		// prop_symb =:concept                // #2
		// prop_symb = word1 word2 ... wordN  // #3
		// :concept                           // #4
		// word1 word2 ... wordN              // #5
		// prop_symb = number                 // #6
		// #type                              // #7
		//
		do_get_next_prop = 0;
		SrCProp prop(cid, 0);
		if(tok == tokConcept) { // #4
			THROW(R_Db.MakeConceptPropC(pdl, 0, prop, temp_buf));
			THROW(R_Db.P_CpT->Set(prop));
		}
		else if(_IsTypeToken(tok)) {
			int    type = _GetTypeByToken(tok);
			THROW(type);
			CONCEPTID prop_crtype = R_Db.GetReservedConcept(R_Db.rcType);
			THROW(prop_crtype);
			prop.PropID = prop_crtype;
			prop = type;
			THROW(R_Db.P_CpT->Set(prop));
		}
		else if(tok == tokWord) {
			LEXID  symb_id = 0;
			THROW_PP_S(_IsIdent(temp_buf), PPERR_SR_C_PROPIDEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
			ident_buf = temp_buf;
			{
				THROW_PP(i < rTokList.getCount(), PPERR_SR_C_ENEXPECTEDEOF); // Неожиданное завершение файла
				titem = rTokList.at_WithoutParent(i++);
				tok = titem.Id;
				temp_buf = titem.Txt;
			}
			if(tok == tokEq) {
				if(R_Db.SearchSpecialWord(SrWordTbl::spcCPropSymb, ident_buf, &symb_id) > 0) {
					{
						THROW_PP(i < rTokList.getCount(), PPERR_SR_C_ENEXPECTEDEOF);
						titem = rTokList.at_WithoutParent(i++);
						tok = titem.Id;
						temp_buf = titem.Txt;
					}
					if(tok == tokConcept) { // #1
						THROW(R_Db.MakeConceptPropC(pdl, ident_buf, prop, temp_buf));
						THROW(R_Db.P_CpT->Set(prop));
					}
					else if(tok == tokWord) {
						ngram.clear();
						do {
							LEXID word_id = 0;
							THROW(R_Db.ResolveWord(temp_buf, &word_id));
							assert(word_id);
							ngram.add(word_id);
							{
								THROW_PP(i < rTokList.getCount(), PPERR_SR_C_ENEXPECTEDEOF);
								titem = rTokList.at_WithoutParent(i++);
								tok = titem.Id;
								temp_buf = titem.Txt;
							}
							if(tok == tokSpace) {
								THROW_PP(i < rTokList.getCount(), PPERR_SR_C_ENEXPECTEDEOF);
								titem = rTokList.at_WithoutParent(i++);
								tok = titem.Id;
								temp_buf = titem.Txt;
							}
						} while(tok == tokWord);
						THROW(R_Db.MakeConceptPropNg(pdl, ident_buf, prop, ngram));
						THROW(R_Db.P_CpT->Set(prop));
					}
					else if(tok == tokNumber) {
						THROW(R_Db.MakeConceptPropN(pdl, ident_buf, prop, temp_buf.ToReal()));
						THROW(R_Db.P_CpT->Set(prop));
					}
				}
				else {
					; // @error Символ 'ident_buf'  не является идентификатором свойства
				}
			}
			else if(tok == tokExprOf) { // #2
				THROW(R_Db.MakeConceptPropC(pdl, ident_buf, prop, temp_buf));
				THROW(R_Db.P_CpT->Set(prop));
			}
		}
		else {
			CALLEXCEPT_PP_S(PPERR_SR_C_PROPIDORCONCEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
		}
		{
			THROW_PP(i < rTokList.getCount(), PPERR_SR_C_ENEXPECTEDEOF);
			titem = rTokList.at_WithoutParent(i++);
			tok = titem.Id;
			temp_buf = titem.Txt;
		}
		if(tok == tokRPar)
			do_get_next_prop = 0;
		else if(tok == tokComma) {
			do_get_next_prop = 1;
		}
		else {
			CALLEXCEPT_PP(PPERR_SR_C_RPARORCOMEXPECTED);
		}
	}
	CATCHZOK
	return ok;
}

int SrConceptParser::Run(const char * pFileName)
{
	int    ok = 1;
	int    finish = 0;
	int    lang_id = 0;
	TSVector <SrWordInfo> word_info; // @v9.8.4 TSArray-->TSVector
	SString temp_buf, ident_buf;
	SString msg_buf;
	SrWordForm wf;
	StrAssocArray temp_token_list;
	Operator root(*this);
	Operator * p_current = &root;
	THROW(OpenInput(pFileName));
	{
		BDbTransaction tra((BDbDatabase *)R_Db, 1);
		THROW(tra);
		if(_ReadLine()) {
			int    tok = 0;
			int    prev_tok = 0; // Токен, непосредственно предшествующий текущему
			int    lbrace_count = 0;
			LongArray ngram;
			LongArray walist_of_ngram; // @#{walist_of_ngram.getCount() == ngram.getCount()} Список ассоциаций словоформ, соответствующих списку ngram
			Int64Array _clist;
			Int64Array _hlist;
			do {
				{
					(msg_buf = "ImportConcept").Space().Cat(pFileName).CatChar('(').Cat(LineNo).CatChar(')');
					PPWaitMsg(msg_buf);
				}
				prev_tok = tok;
				tok = _GetToken(temp_buf);
				int    can_use_ngram = 0;
				if(tok == tokWord) {
					ngram.clear();
					walist_of_ngram.clear();
					NGID   ngram_id = 0;
					do {
						LEXID word_id = 0;
						int32  wa_id = 0;
						{
							THROW(R_Db.ResolveWord(temp_buf, &word_id));
							assert(word_id);
							if(!temp_buf.IsDigit()) { // @v10.2.0
								const int lang_id = p_current->GetLangID();
								if(lang_id) {
									wf.Clear();
									wf.SetTag(SRWG_LANGUAGE, lang_id);
									THROW(R_Db.SetSimpleWordFlexiaModel(word_id, wf, &wa_id));
								}
							}
						}
						ngram.add(word_id);
						walist_of_ngram.add(wa_id);
						prev_tok = tok;
						tok = _GetToken(temp_buf);
						if(tok == tokSpace) {
							prev_tok = tok;
							tok = _GetToken(temp_buf);
						}
					} while(tok == tokWord);
					assert(ngram.getCount() == walist_of_ngram.getCount());
					//
					// Если ngram содержит единственное слово и current->CLexID не занят предыдущей частью оператора,
					// то присваиваем p_current->CLexID ид этого слова, а p_current->WaIdOfCLex - ассоциированную SrWordAssoc.
					// Все это нужно для определения аббревиатур
					//
					if(ngram.getCount() == 1 && !p_current->CLexID) {
						p_current->CLexID = ngram.get(0);
						p_current->WaIdOfCLex = walist_of_ngram.get(0);
					}
					//
					// Так как преобразование ngram в p_current->NgID__ нужно не всегда, отложим
					// принятие решения об этом до момента, когда это выяснится (обработаем по сигналу can_use_ngram)
					//
					can_use_ngram = 1;
				}
				if(oneof2(tok, tokEqAbbrev, tokEqAbbrevDot)) {
					// LEX=$NGRAM
					// кг=$килограмм
					// "с/м"=$свеже-мороженый
					THROW_PP(p_current->CLexID, PPERR_SR_C_NWORDBEFOREABBR);
					//// До следующего токена сохраним текущее значение p_current->CLexID в p_current->AbbrOf
					//// после получения очередной NG поменяем все местами.
					//p_current->AbbrOf = p_current->CLexID;
					//p_current->CLexID = 0;
					p_current->SetAbbr(tok == tokEqAbbrevDot);
				}
				else {
					if(can_use_ngram) {
						THROW(R_Db.ResolveNGram(ngram, &p_current->NgID__));
						assert(p_current->NgID__);
					}
					if(p_current->Flags & (Operator::fAbbr|Operator::fAbbrDot)) {
						THROW_PP(p_current->NgID__, PPERR_SR_C_NNGAFTERABBR);
						//p_current->CLexID = (LEXID)p_current->AbbrOf;
						p_current->AbbrOf = p_current->NgID__;
					}
					switch(tok) {
						case tokEnd:
							if(p_current->Close(1) > 0) {
								THROW(p_current = p_current->CreateNext());
							}
							if(!_ReadLine()) {
								finish = 1;
							}
							break;
						case tokSpace:
							if(oneof2(prev_tok, tokWord, tokConcept)) {
								if(p_current->Close(1) > 0) {
									THROW(p_current = p_current->CreateNext());
								}
							}
							break;
						case tokSemicol:
							if(p_current->Close(1) > 0) {
								THROW(p_current = p_current->CreateNext());
							}
							break;
						case tokLBrace:
							THROW(p_current = p_current->CreateChild());
							break;
						case tokRBrace:
							{
								Operator * p_par = p_current->FindParent();
								THROW_PP(p_par, PPERR_SR_C_PAROPNFOUND);
								p_current = p_par;
								if(p_current->LangID) {
									for(Operator * p_child = p_current->P_Child; p_child; p_child = p_child->P_Next) {
										SETIFZ(p_child->LangID, p_current->LangID);
									}
								}
							}
							break;
						case tokBracketsDescr:
							{
								if(p_current->Close(1) > 0) {
									THROW(p_current = p_current->CreateNext());
								}
								THROW_SL(p_current->LangID = RecognizeLinguaSymb(temp_buf, 1));
							}
							break;
						case tokConcept:
							{
								CONCEPTID cid = 0;
								THROW(R_Db.ResolveConcept(temp_buf, &cid));
								assert(cid);
								if(prev_tok == tokConcept) {
									p_current->InstanceOf = cid;
								}
								else if(prev_tok == tokWord) {
									if(p_current->NgID__ && !p_current->CID) {
										_clist.clear();
										int   _cisdone = 0; // Концепция уже является членом иерархии, в которую входит cid
										if(R_Db.GetNgConceptList(p_current->NgID__, R_Db.ngclAnonymOnly, _clist) > 0) {
											assert(_clist.getCount());
											for(uint ci = 0; !_cisdone && ci < _clist.getCount(); ci++) {
												if(R_Db.GetConceptHier(_clist.get(ci), _hlist) > 0 && _hlist.lsearch(cid)) {
													p_current->CID = _clist.get(0);
													_cisdone = 1;
												}
											}
											p_current->CID = _clist.get(0);
										}
										if(!_cisdone) {
											THROW(R_Db.CreateAnonymConcept(&p_current->CID));
											THROW(R_Db.P_CNgT->Set(p_current->CID, p_current->NgID__));
											p_current->InstanceOf = cid;
										}
									}
								}
								else if(prev_tok == tokRBrace) {
									_clist.clear();
									for(Operator * p_child = p_current->P_Child; p_child; p_child = p_child->P_Next) {
										if(!p_child->InstanceOf) {
											if(p_child->NgID__ && !p_child->CID) {
												_clist.clear();
												if(R_Db.GetNgConceptList(p_child->NgID__, R_Db.ngclAnonymOnly, _clist) > 0) {
													assert(_clist.getCount());
													p_child->CID = _clist.get(0);
												}
												else {
													THROW(R_Db.CreateAnonymConcept(&p_child->CID));
													THROW(R_Db.P_CNgT->Set(p_child->CID, p_child->NgID__));
												}
											}
											p_child->InstanceOf = cid;
										}
									}
								}
								else {
									if(p_current->Close(1) > 0) {
										THROW(p_current = p_current->CreateNext());
									}
									p_current->CID = cid;
								}
							}
							break;
						case tokSubclassOf:
							{
								CONCEPTID cid = 0;
								THROW_PP(!p_current->IsClosed(), PPERR_SR_C_CLOSEDCUR_2CC);
								THROW_PP(!p_current->SubclassOf, PPERR_SR_C_HASSUBCLS_2CC);
								THROW(R_Db.ResolveConcept(temp_buf, &cid));
								assert(cid);
								if(prev_tok == tokConcept) {
									p_current->SubclassOf = cid;
								}
								else if(prev_tok == tokWord) {
									if(p_current->NgID__ && !p_current->CID) {
										_clist.clear();
										int   _cisdone = 0; // Концепция уже является членом иерархии, в которую входит cid
										if(R_Db.GetNgConceptList(p_current->NgID__, R_Db.ngclAnonymOnly, _clist) > 0) {
											assert(_clist.getCount());
											for(uint ci = 0; !_cisdone && ci < _clist.getCount(); ci++) {
												if(R_Db.GetConceptHier(_clist.get(ci), _hlist) > 0 && _hlist.lsearch(cid)) {
													p_current->CID = _clist.get(0);
													_cisdone = 1;
												}
											}
										}
										if(!_cisdone) {
											THROW(R_Db.CreateAnonymConcept(&p_current->CID));
											THROW(R_Db.P_CNgT->Set(p_current->CID, p_current->NgID__));
											p_current->SubclassOf = cid;
										}
									}
								}
								else {
									if(p_current->Close(1) > 0) {
										THROW(p_current = p_current->CreateNext());
									}
								}
							}
							break;
						case tokLPar:
							{
								temp_token_list.Z();
								for(int do_get_next_prop = 1; do_get_next_prop;) {
									//
									// (свойство, свойство, ..., свойство)
									//
									// Варианты свойства:
									// prop_symb = :concept               // #1
									// prop_symb =:concept                // #2
									// prop_symb = word1 word2 ... wordN  // #3
									// :concept                           // #4
									// word1 word2 ... wordN              // #5
									// prop_symb = number                 // #6
									// #type                              // #7
									//
									do_get_next_prop = 0;
									THROW(_SkipSpaces(&tok, temp_buf)); // @error Неожиданное завершение файла
									temp_token_list.Add(tok, temp_buf, -1);
									if(tok == tokConcept) { // #4
										// @construction temp_token_list.Add(tok, temp_buf, -1);
									}
									else if(_IsTypeToken(tok)) { // #7
										// @v9.9.0 @fix temp_token_list.Add(tok, temp_buf, -1);
									}
									else if(tok == tokWord) {
										THROW_PP_S(_IsIdent(temp_buf), PPERR_SR_C_PROPIDEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
										ident_buf = temp_buf;
										THROW_PP(_SkipSpaces(&tok, temp_buf), PPERR_SR_C_ENEXPECTEDEOF);
										temp_token_list.Add(tok, temp_buf, -1);
										if(tok == tokEq) {
											THROW_PP(_SkipSpaces(&tok, temp_buf), PPERR_SR_C_ENEXPECTEDEOF);
											temp_token_list.Add(tok, temp_buf, -1);
											if(tok == tokConcept) { // #1
											}
											else if(tok == tokWord) {
												do {
													tok = _GetToken(temp_buf);
													temp_token_list.Add(tok, temp_buf, -1);
													if(tok == tokSpace) {
														tok = _GetToken(temp_buf);
														temp_token_list.Add(tok, temp_buf, -1);
													}
												} while(tok == tokWord);
											}
										}
										else if(tok == tokExprOf) { // #2
										}
									}
									else {
										CALLEXCEPT_PP_S(PPERR_SR_C_PROPIDORCONCEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
									}
									THROW_PP(_SkipSpaces(&tok, temp_buf), PPERR_SR_C_ENEXPECTEDEOF);
									temp_token_list.Add(tok, temp_buf, -1);
									if(tok == tokRPar)
										do_get_next_prop = 0;
									else if(tok == tokComma) {
										do_get_next_prop = 1;
									}
									else {
										CALLEXCEPT_PP(PPERR_SR_C_RPARORCOMEXPECTED);
									}
								}
								{
									//
									// Так как присвоение свойств требует правильной идентификации иерархии объектов
									// придется акцептировать разобранные компоненты в базу данных, предварительно
									// закрыв оператор.
									//
									Operator * p_pc = p_current;
									if(p_current->Close(1) > 0) {
										THROW(p_current = p_current->CreateNext());
									}
									THROW(PostprocessOpList(&root));
									//
									if(p_pc->CID) {
										THROW(ApplyConceptPropList(temp_token_list, p_pc->CID));
									}
									else {
										for(Operator * p_child = p_pc->P_Child; p_child; p_child = p_child->P_Next) {
											if(p_child->CID) {
												THROW(ApplyConceptPropList(temp_token_list, p_child->CID));
											}
										}
									}
								}
							}
							break;
						case tokPropDeclStart:
							{
								SrCPropDeclList pdl;
								THROW_PP(p_current->CID, PPERR_SR_C_UNDEFCONCEPT_PROP);
								for(int do_get_next_decl = 1; do_get_next_decl;) {
									int    ss = 0;
									do_get_next_decl = 0;
									prev_tok = tok;
									ss = _SkipSpaces(&tok, temp_buf);
									THROW_PP(tok == tokConcept, PPERR_SR_C_CCONCEXPECTED);
									{
										SrCPropDecl pd;
										THROW(R_Db.ResolveConcept(temp_buf, &pd.PropID));
										prev_tok = tok;
										ss = _SkipSpaces(&tok, temp_buf);
										if(ss) {
											if(tok == tokWord) {
												THROW_PP_S(_IsIdent(temp_buf), PPERR_SR_C_PROPIDEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
												THROW(R_Db.ResolveCPropSymb(temp_buf, &pd.SymbID));
												THROW(pdl.Add(pd));
												ss = _SkipSpaces(&tok, temp_buf);
												if(tok == tokComma) {
													do_get_next_decl = 1;
												}
												else if(tok == tokRPar) {
												}
												else {
													CALLEXCEPT_PP(PPERR_SR_C_RPARORCOMEXPECTED);
												}
											}
											else if(tok == tokComma) {
												THROW(pdl.Add(pd));
												do_get_next_decl = 1;
											}
											else if(tok == tokRPar) {
												THROW(pdl.Add(pd));
											}
											else {
												; // @error
											}
										}
									}
								}
								THROW(R_Db.P_CT->SetPropDeclList(p_current->CID, &pdl));
							}
							break;
						case tokExprOf:
							{
								CONCEPTID cid = 0;
								THROW_PP(!p_current->IsClosed(), PPERR_SR_C_CLOSEDCUR_2EC);
								THROW(R_Db.ResolveConcept(temp_buf, &cid));
								assert(cid);
								if(prev_tok == tokWord)
									p_current->EqToList.add(cid);
								else if(prev_tok == tokConcept)
									p_current->EqToList.add(cid);
								else if(prev_tok == tokRBrace) {
									for(Operator * p_child = p_current->P_Child; p_child; p_child = p_child->P_Next) {
										p_child->EqToList.add(cid);
									}
								}
								else if(prev_tok == tokSpace) {
									Operator * p_parent = p_current->FindParent();
									if(p_parent && (p_parent->NgID__ || p_parent->CID))
										p_parent->EqToList.add(cid);
								}
								else {
									CALLEXCEPT_PP(PPERR_SR_C_UNEXPAFTER_2EC);
								}
							}
							break;
					}
				}
			} while(!finish);
			THROW(PostprocessOpList(&root));
		}
		THROW(tra.Commit(1));
	}
	CATCHZOK
	CloseInput();
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(PrcssrSartre); PrcssrSartreFilt::PrcssrSartreFilt() : PPBaseFilt(PPFILT_PRCSSRSARTREPARAM, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrSartreFilt, ReserveStart),
		offsetof(PrcssrSartreFilt, SrcPath)-offsetof(PrcssrSartreFilt, ReserveStart));
	SetBranchSString(offsetof(PrcssrSartreFilt, SrcPath));
	Init(1, 0);
}

PrcssrSartreFilt & FASTCALL PrcssrSartreFilt::operator = (const PrcssrSartreFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

int PrcssrSartreFilt::IsEmpty() const
{
	return BIN(!Flags && SrcPath.IsEmpty());
}

class PrcssrSartreFiltDialog : public TDialog {
	DECL_DIALOG_DATA(PrcssrSartreFilt);
public:
	PrcssrSartreFiltDialog() : TDialog(DLG_PRCRSARTR)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 0, Data.fImportFlexia);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 1, Data.fImportConcepts);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 2, Data.fImportHumNames);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 3, Data.fImportBioTaxonomy); // @v10.0.0
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 4, Data.fImportTickers); // @v10.0.08
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 5, Data.fTestFlexia);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 6, Data.fTestConcepts);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 7, Data.fTestSyntaxParser);
		SetClusterData(CTL_PRCRSARTR_FLAGS, Data.Flags);
		// @v10.9.5 {
		AddClusterAssoc(CTL_PRCRSARTR_UEDFLAGS, 0, Data.fImport_UED_Llcc);
		SetClusterData(CTL_PRCRSARTR_UEDFLAGS, Data.Flags);
		// } @v10.9.5 
		FileBrowseCtrlGroup::Setup(this, CTLBRW_PRCRSARTR_SRCPATH, CTL_PRCRSARTR_SRCPATH, 1, 0,
			0, FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		setCtrlString(CTL_PRCRSARTR_SRCPATH, Data.SrcPath);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_PRCRSARTR_FLAGS, &Data.Flags);
		GetClusterData(CTL_PRCRSARTR_UEDFLAGS, &Data.Flags); // @v10.9.5
		getCtrlString(sel = CTL_PRCRSARTR_SRCPATH, Data.SrcPath);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

PrcssrSartre::PrcssrSartre(const char * pDbPath)
{
}

PrcssrSartre::~PrcssrSartre()
{
}

int PrcssrSartre::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrSartreFilt * p_filt = static_cast<PrcssrSartreFilt *>(pBaseFilt);
		if(p_filt->IsEmpty()) {
		}
	}
	else
		ok = 0;
	return ok;
}

int PrcssrSartre::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *static_cast<const PrcssrSartreFilt *>(pBaseFilt);
	CATCHZOK
	return ok;
}

int PrcssrSartre::EditParam(PPBaseFilt * pBaseFilt)
{
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrSartreFilt * p_filt = static_cast<PrcssrSartreFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(PrcssrSartreFiltDialog, p_filt);
}

class SrImpHumanNameList : public SStrGroup {
public:
	struct Entry {
		SString Name;
		int   Gender;
	};
	SrImpHumanNameList() : SStrGroup()
	{
	}
	uint   GetCount() const { return L.getCount(); }
	uint   GetItem(uint p, Entry & rItem) const { return Helper_GetItem(p, rItem); }
	int    GetGender(uint p) const { return (p < L.getCount()) ? L.at(p).Gender : 0; }
	int    SetGender(uint p, int gender)
	{
		if(p < L.getCount()) {
			L.at(p).Gender = gender;
			return 1;
		}
		else
			return 0;
	}
	int    AddItem(Entry & rItem)
	{
		int    ok = 1;
		InnerEntry int_item(rItem.Gender);
		AddS(rItem.Name, &int_item.NameP);
		L.insert(&int_item);
		return ok;
	}
	void   Sort() { L.sort(PTR_CMPCFUNC(SrImpHumanNameEntry), this); }
private:
	struct InnerEntry { // @flat
		explicit InnerEntry(uint gender) : NameP(0), Gender(gender)
		{
		}
		uint   NameP;
		int    Gender;
	};
	static IMPL_CMPCFUNC(SrImpHumanNameEntry, i1, i2)
	{
		const InnerEntry * p1 = static_cast<const InnerEntry *>(i1);
		const InnerEntry * p2 = static_cast<const InnerEntry *>(i2);
		SString n1, n2;
		static_cast<const SrImpHumanNameList *>(pExtraData)->GetS(p1->NameP, n1);
		static_cast<const SrImpHumanNameList *>(pExtraData)->GetS(p2->NameP, n2);
		SStringU nu1, nu2;
		nu1.CopyFromUtf8(n1);
		nu2.CopyFromUtf8(n2);
		return nu1.Cmp(nu2);
	}
	int    Helper_GetItem(uint pos, Entry & rItem) const
	{
		int    ok = 0;
		if(pos < L.getCount()) {
			const InnerEntry & r_int_item = L.at(pos);
			rItem.Gender = r_int_item.Gender;
			GetS(r_int_item.NameP, rItem.Name);
			ok = 1;
		}
		return ok;
	}
	TSVector <InnerEntry> L; // @v9.8.6 TSArray-->TSVector
};

int PrcssrSartre::ImportHumanNames(SrDatabase & rDb, const char * pSrcFileName, const char * pLinguaSymb, int properNameType, int specialProcessing)
{
	int    ok = 1;
	const  uint max_items_per_tx = 512; // @v10.0.08 128-->512
	BDbTransaction * p_ta = 0;
	SString temp_buf;
	SString msg_buf;
	SString src_file_name;
	SString line_buf;
	SStringU uname, uname_prev;
	int   lang_id = RecognizeLinguaSymb(pLinguaSymb, 1);
	const  char * p_parent_concept = 0;
	if(properNameType == SRPROPN_PERSONNAME)
		p_parent_concept = "hum_fname";
	else if(properNameType == SRPROPN_FAMILYNAME)
		p_parent_concept = "hum_sname";
	else if(properNameType == SRPROPN_PATRONYMIC)
		p_parent_concept = "hum_pname";
	if(p_parent_concept) {
		SrCPropList cpl;
		SrCProp cp;
		SrCProp cp_gender;
		Int64Array _clist;
		SrCPropDeclList pdl;
		SrWordForm wf;
		CONCEPTID parent_cid = 0;
		CONCEPTID gender_cid = 0;
		CONCEPTID gender_fem_cid = 0;
		CONCEPTID gender_mas_cid = 0;
		const CONCEPTID prop_subclass = rDb.GetReservedConcept(rDb.rcSubclass);
		const CONCEPTID prop_instance = rDb.GetReservedConcept(rDb.rcInstance);
		THROW(prop_subclass);
		THROW(prop_instance);
		THROW(rDb.ResolveConcept(p_parent_concept, &parent_cid));
		THROW(rDb.ResolveConcept("gender", &gender_cid));
		THROW(rDb.ResolveConcept("gender_mas", &gender_mas_cid));
		THROW(rDb.ResolveConcept("gender_fem", &gender_fem_cid));
		assert(parent_cid);
		assert(gender_cid);
		assert(gender_mas_cid);
		assert(gender_fem_cid);
		{
			(src_file_name = P.SrcPath).SetLastSlash().Cat(pSrcFileName); // utf-8
			StringSet ss(";");
			StringSet name_ss;
			LongArray ngram;
			SFile f_in(src_file_name, SFile::mRead);
			if(f_in.IsValid()) {
				PPWaitMsg((msg_buf = "ImportHumanName").Space().Cat(src_file_name));
				SrImpHumanNameList list;
				SrImpHumanNameList::Entry entry;
				uint   line_no = 0;
				while(f_in.ReadLine(line_buf)) {
					line_no++;
					if(line_no > 1 && line_buf.Chomp().NotEmptyS()) {
						line_buf.Transf(CTRANSF_UTF8_TO_INNER);
						ss.setBuf(line_buf);
						uint   fld_no = 0;
						double freq = 0.0;
						entry.Name.Z();
						entry.Gender = 0;
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							fld_no++;
							if(fld_no == 1) {
								entry.Name = temp_buf.Strip().ToLower().Transf(CTRANSF_INNER_TO_UTF8);
							}
							else if(fld_no == 2) {
								temp_buf.Strip().ToLower();
								if(temp_buf == "male")
									entry.Gender = SRGENDER_MASCULINE;
								else if(temp_buf == "female")
									entry.Gender = SRGENDER_FEMININE;
							}
							else if(fld_no == 3) {
								freq = temp_buf.ToReal();
							}
						}
						if(entry.Name.NotEmpty()) {
							list.AddItem(entry);
						}
					}
				}
				list.Sort();
				{
					uint   items_per_tx = 0;
					//int    coupled_gender = 0;
					if(specialProcessing) {
						//
						// Специальная обработка файла русских фамилий.
						// Так как в исходном файле пол фамилии часто не указан, то полагаемся на
						// правило, что если идут одна за другой две почти одинаковых фамилии, отличающиеся
						// только тем, что у второй на конце русская 'а', то первая фамилия мужская, вторая - женская.
						// Например:
						//   Иванов
						//   Иванова
						//
						SStringU this_name_u;
						SStringU prev_name_u;
						uint   prev_name_pos = 0;
						for(uint i = 0; i < list.GetCount(); i++) {
							if(list.GetItem(i, entry)) {
								this_name_u.CopyFromUtf8(entry.Name);
								if(prev_name_u.Len() && this_name_u.CmpPrefix(prev_name_u) == 0) {
									if((prev_name_u.Len()+1) == this_name_u.Len()) {
										if(this_name_u.Last() == L'а') { // русская 'а' unicode
											if(!list.GetGender(prev_name_pos))
												list.SetGender(prev_name_pos, SRGENDER_MASCULINE);
											if(!entry.Gender)
												list.SetGender(i, SRGENDER_FEMININE);
										}
									}
								}
								prev_name_u = this_name_u;
								prev_name_pos = i;
							}
						}
					}
					THROW_MEM(p_ta = new BDbTransaction(rDb, 1));
					THROW_DB(*p_ta);
					for(uint i = 0; i < list.GetCount(); i++) {
						if(list.GetItem(i, entry)) {
							NGID   ngram_id = 0;
							ngram.clear();
							name_ss.clear();
							entry.Name.Tokenize(" ", name_ss);
							for(uint nssp = 0; name_ss.get(&nssp, temp_buf);) {
								LEXID  word_id = 0;
								THROW(rDb.ResolveWord(temp_buf, &word_id));
								temp_buf.Transf(CTRANSF_UTF8_TO_OUTER); // @debug
								assert(word_id);
								ngram.add(word_id);
							}
							if(lang_id) {
								if(ngram.getCount() == 1) {
									wf.Clear();
									wf.SetTag(SRWG_LANGUAGE, lang_id);
									wf.SetTag(SRWG_CLASS, SRWC_NOUN);
									if(properNameType)
										wf.SetTag(SRWG_PROPERNAME, properNameType);
									if(entry.Gender)
										wf.SetTag(SRWG_GENDER, entry.Gender);
									THROW(rDb.SetSimpleWordFlexiaModel(ngram.get(0), wf, 0));
								}
								else if(ngram.getCount() > 1) {
									for(uint j = 0; j < ngram.getCount(); j++) {
										wf.Clear();
										wf.SetTag(SRWG_LANGUAGE, lang_id);
										THROW(rDb.SetSimpleWordFlexiaModel(ngram.get(j), wf, 0));
									}
								}
							}
							THROW(rDb.ResolveNGram(ngram, &ngram_id));
							if(ngram_id) {
								CONCEPTID cid = 0;
								_clist.clear();
								pdl.Z();
								cpl.Z();
								cp.Z();
								cp_gender.Z();
								if(rDb.GetNgConceptList(ngram_id, rDb.ngclAnonymOnly, _clist) > 0) {
									assert(_clist.getCount());
									for(uint cidx = 0; !cid && cidx < _clist.getCount(); cidx++) {
										CONCEPTID _c = _clist.get(cidx);
										if(rDb.GetConceptPropList(_c, cpl) > 0 && cpl.Get(_c, prop_subclass, cp)) {
											CONCEPTID _val = 0;
											if(cp.Get(_val) && _val == parent_cid) {
												if(cpl.Get(_c, gender_cid, cp_gender) && cp_gender.Get(_val)) {
													if(!entry.Gender)
														cid = _c;
													else if(entry.Gender == SRGENDER_MASCULINE && _val == gender_mas_cid)
														cid = _c;
													else if(entry.Gender == SRGENDER_FEMININE && _val == gender_fem_cid)
														cid = _c;
													else
														cid = 0;
												}
												else {
													cid = _c;
													if(entry.Gender == SRGENDER_MASCULINE) {
														THROW(rDb.SetConceptProp(cid, gender_cid, 0, gender_mas_cid));
													}
													else if(entry.Gender == SRGENDER_FEMININE) {
														THROW(rDb.SetConceptProp(cid, gender_cid, 0, gender_fem_cid));
													}
												}
											}
										}
									}
								}
								if(!cid) {
									THROW(rDb.CreateAnonymConcept(&cid));
									THROW(rDb.P_CNgT->Set(cid, ngram_id));
									THROW(rDb.SetConceptProp(cid, /*prop_subclass*/prop_instance, 0, parent_cid));
									if(entry.Gender == SRGENDER_MASCULINE) {
										THROW(rDb.SetConceptProp(cid, gender_cid, 0, gender_mas_cid));
									}
									else if(entry.Gender == SRGENDER_FEMININE) {
										THROW(rDb.SetConceptProp(cid, gender_cid, 0, gender_fem_cid));
									}
								}
							}
							THROW(RechargeTransaction(p_ta, ++items_per_tx, max_items_per_tx));
						}
						{
							(msg_buf = "ImportHumanName").Space().Cat(src_file_name);
							PPWaitPercent(i+1, list.GetCount(), msg_buf);
						}
					}
					if(p_ta) {
						THROW_DB(p_ta->Commit(1));
						ZDELETE(p_ta);
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_ta;
	return ok;
}

struct BioTaxonomyCPropEntry {
	CONCEPTID CId;
	int64  Value;
	uint8  PropVariant; // 1 - prop_instance, 2 - prop_subclass
};

int PrcssrSartre::ImportBioTaxonomy(SrDatabase & rDb, const char * pFileName)
{
	struct InnerMethods {
		static int FASTCALL AddTaxonomyFactorToSymbHash(const SString & rSymb, SymbHashTable & rSht, LongArray & rIdList, uint & rLastSymbId)
		{
			int    ok = -1;
			uint   sht_id = 0;
			if(rSymb.NotEmpty() && !rSht.Search(rSymb, &sht_id, 0))
				rSht.Add(rSymb, (sht_id = ++rLastSymbId));
			if(sht_id) {
				rIdList.add((long)sht_id);
				ok = 1;
			}
			return ok;
		}
		static int FASTCALL WriteTaxonomyFactorList(SFile & rF, const char * pTitle, SymbHashTable & rSht, const LongArray & rIdList, SString & rLineBuf)
		{
			int    ok = 1;
			SString temp_buf;
			rLineBuf.Z();
			for(uint i = 0; i < rIdList.getCount(); i++) {
				rLineBuf.Z();
				rSht.GetByAssoc(rIdList.get(i), temp_buf.Z());
				rF.WriteLine(rLineBuf.Cat(pTitle).Tab().Cat(temp_buf).CR());
			}
			return ok;
		}
	};
	int    ok = 1;
	const  uint max_items_per_tx = 512;
	uint   items_per_tx = 0;
	uint   items_per_tx_total = 0;
	BDbTransaction * p_ta = 0;
	SString line_buf;
	SString temp_buf;
	StringSet ss("\t");
	SymbHashTable sht(SKILOBYTE(1024), 0);
	LongArray taxonomic_status_list;
	LongArray taxon_rank_list;
	LongArray kingdom_list;
	LongArray phylum_list;
	LongArray class_list;
	LongArray order_list;
	LongArray superfamily_list;
	LongArray family_list;
	LongArray generic_name_list;
	LongArray genus_list;
	TSVector <BioTaxonomyCPropEntry> cprop_list; // Мы сначала построим список свойств концепций а потом, на отдельной фазе, занесем его в БД
	enum {
		taxstatusUnkn = 0,
		taxstatusAcceptedName, // "accepted name"
		taxstatusSynonym,      // "synonym"
		taxstatusMisappliedName, // "misapplied name"
		taxstatusAmbiguousSynonym, // "ambiguous synonym"
		taxstatusProvisionallyAcceptedName // provisionally accepted name
	};
	struct BioTaxonomyEntry {
		int32  TaxonID;
		S_GUID Identifier;
		int32  DatasetID;
		SString DatasetName;
		int32  AcceptedNameUsageID;
		int32  ParentNameUsageID;
		SString TaxonomicStatusText;
		int32  TaxonomicStatusCode;
		SString TaxonRank;
		SString VerbatimTaxonRank;
		SString ScientificName;
		SString Kingdom;
		SString Phylum;
		SString Class;
		SString Order;
		SString Superfamily;
		SString Family;
		SString GenericName;
		SString Genus;
		SString Subgenus;
		SString SpecificEpithet;
		SString InfraspecificEpithet;
		SString ScientificNameAuthorship;
		SString Source;
		SString NamePublishedIn;
		SString NameAccordingTo;
		SString Modified;
		SString Description;
		SString TaxonConceptID;
		SString ScientificNameID;
		SString References;
		int   IsExtinct;
	};
	uint   last_symb_id = 0;
	uint   total_line_count = 0; // Рассчитывается на фазе phasePreprocess
	SString name_buf;
	SString concept_symb_buf;
	SString parent_concept_symb_buf;
	SString synon_concept_symb_buf;
	StringSet name_ss;
	LongArray ngram;
	LongArray parent_ref_list;
	LongArray acceptedname_ref_list;
	BioTaxonomyEntry entry;

	StringSet words_to_append;
	LongArray taxon_symb_to_append; // gbif-идентификаторы таксонов, которые должны быть добавлены в БД

	CONCEPTID cid_biotaxonomy_category = 0;
	CONCEPTID cid_biotaxonomy_kingdom = 0;
	CONCEPTID cid_biotaxonomy_phylum = 0;
	CONCEPTID cid_biotaxonomy_class = 0;
	CONCEPTID cid_biotaxonomy_order = 0;
	CONCEPTID cid_biotaxonomy_superfamily = 0;
	CONCEPTID cid_biotaxonomy_family = 0;
	CONCEPTID cid_biotaxonomy_genus = 0;
	CONCEPTID cid_biotaxonomy_subgenus = 0;
	CONCEPTID cid_biotaxonomy_species = 0;
	CONCEPTID cid_biotaxonomy_infraspecies = 0;
	long   wordform_id = 0;
	PPWaitStart();
	SFile f_in(pFileName, SFile::mRead);
	THROW_SL(f_in.IsValid());

	THROW(rDb.SearchConcept("biotaxonomy_category", &cid_biotaxonomy_category));
	THROW(rDb.SearchConcept("biotaxonomy_kingdom", &cid_biotaxonomy_kingdom));
	THROW(rDb.SearchConcept("biotaxonomy_phylum", &cid_biotaxonomy_phylum));
	THROW(rDb.SearchConcept("biotaxonomy_class", &cid_biotaxonomy_class));
	THROW(rDb.SearchConcept("biotaxonomy_order", &cid_biotaxonomy_order));
	THROW(rDb.SearchConcept("biotaxonomy_superfamily", &cid_biotaxonomy_superfamily));
	THROW(rDb.SearchConcept("biotaxonomy_family", &cid_biotaxonomy_family));
	THROW(rDb.SearchConcept("biotaxonomy_genus", &cid_biotaxonomy_genus));
	THROW(rDb.SearchConcept("biotaxonomy_subgenus", &cid_biotaxonomy_subgenus));
	THROW(rDb.SearchConcept("biotaxonomy_species", &cid_biotaxonomy_species));
	THROW(rDb.SearchConcept("biotaxonomy_infraspecies", &cid_biotaxonomy_infraspecies));
	{
		SrWordForm wordform;
		BDbTransaction local_tra(rDb, 1);
		wordform.SetTag(SRWG_LANGUAGE, slangLA); // Вся импортируемая био-таксономия на латинском языке
		THROW(rDb.ResolveWordForm(wordform, &wordform_id));
		THROW_DB(local_tra.Commit(1));
	}
	const CONCEPTID prop_instance = rDb.GetReservedConcept(rDb.rcInstance);
	const CONCEPTID prop_subclass = rDb.GetReservedConcept(rDb.rcSubclass);
	enum {
		phasePreprocess,
		phase1,
		phase2
	};
	const  int phase_list[] = { phase1, phase2 };
	for(uint phase_idx = 0; phase_idx < SIZEOFARRAY(phase_list); phase_idx++) {
		const int _phase = phase_list[phase_idx];
		if(_phase == phase2) {
			if(!p_ta) {
				THROW_MEM(p_ta = new BDbTransaction(rDb, 1));
				THROW_DB(*p_ta);
			}
			if(!p_ta->IsStarted()) {
				THROW_DB(p_ta->Start(1));
			}
		}
		f_in.Seek(0);
		for(uint line_no = 1; f_in.ReadLine(line_buf); line_no++) {
			line_buf.Chomp();
			ss.clear();
			ss.setBuf(line_buf);
			if(line_no == 1) { // Строка заголовков
				THROW(ss.getCount() >= 31); // Проверка на то, что это - наш файл
			}
			else {
				const char * p_notassigned_text = "not assigned";
				for(uint ssp = 0, fld_no = 1; ss.get(&ssp, temp_buf); fld_no++) {
					temp_buf.Strip().Utf8ToLower();
					switch(fld_no) {
						case 1: entry.TaxonID = temp_buf.ToLong(); break;
						case 2: entry.Identifier.FromStr(temp_buf); break;
						case 3: entry.DatasetID = temp_buf.ToLong(); break;
						case 4: entry.DatasetName = temp_buf; break;
						case 5: entry.AcceptedNameUsageID = temp_buf.ToLong(); break;
						case 6: entry.ParentNameUsageID = temp_buf.ToLong(); break;
						case 7:
							entry.TaxonomicStatusText = temp_buf;
							if(temp_buf.IsEqiAscii("accepted name"))
								entry.TaxonomicStatusCode = taxstatusAcceptedName;
							else if(temp_buf.IsEqiAscii("provisionally accepted name"))
								entry.TaxonomicStatusCode = taxstatusProvisionallyAcceptedName;
							else if(temp_buf.IsEqiAscii("synonym"))
								entry.TaxonomicStatusCode = taxstatusSynonym;
							else if(temp_buf.IsEqiAscii("ambiguous synonym"))
								entry.TaxonomicStatusCode = taxstatusAmbiguousSynonym;
							else if(temp_buf.IsEqiAscii("misapplied name"))
								entry.TaxonomicStatusCode = taxstatusMisappliedName;
							else
								entry.TaxonomicStatusCode = taxstatusUnkn;
							break;
						case 8: entry.TaxonRank = temp_buf; break;
						case 9: entry.VerbatimTaxonRank = temp_buf; break;
						case 10: entry.ScientificName = temp_buf; break;
						case 11: entry.Kingdom = temp_buf; break; // !
						case 12: entry.Phylum = temp_buf; break; // !
						case 13: entry.Class = temp_buf; break; // !
						case 14: entry.Order = temp_buf; break; // !
						case 15: entry.Superfamily = temp_buf; break; // !
						case 16: entry.Family = temp_buf; break; // !
						case 17: entry.GenericName = temp_buf; break;
						case 18: entry.Genus = temp_buf; break; // !
						case 19: entry.Subgenus = temp_buf; break; // !
						case 20: entry.SpecificEpithet = temp_buf; break; // !
						case 21: entry.InfraspecificEpithet = temp_buf; break; // !
						case 22: entry.ScientificNameAuthorship = temp_buf; break;
						case 23: entry.Source = temp_buf; break;
						case 24: entry.NamePublishedIn = temp_buf; break;
						case 25: entry.NameAccordingTo = temp_buf; break;
						case 26: entry.Modified = temp_buf; break;
						case 27: entry.Description = temp_buf; break;
						case 28: entry.TaxonConceptID = temp_buf; break;
						case 29: entry.ScientificNameID = temp_buf; break;
						case 30: entry.References = temp_buf; break;
						case 31: entry.IsExtinct = BIN(temp_buf.IsEqiAscii("true")); break;
					}
				}
				CONCEPTID cid_instance_of = 0;
				name_buf.Z();
				SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcGBIF, &entry.TaxonID, sizeof(entry.TaxonID), concept_symb_buf);
				if(entry.TaxonRank == "species") {
					cid_instance_of = cid_biotaxonomy_species;
					(name_buf = entry.GenericName).Space().Cat(entry.SpecificEpithet);
					if(oneof2(entry.TaxonomicStatusCode, taxstatusAcceptedName, taxstatusProvisionallyAcceptedName)) {

					}
				}
				else if(entry.TaxonRank == "infraspecies") {
					cid_instance_of = cid_biotaxonomy_infraspecies;
					(name_buf = entry.GenericName).Space().Cat(entry.SpecificEpithet);
					if(entry.InfraspecificEpithet.NotEmpty())
						name_buf.Space().Cat(entry.InfraspecificEpithet);
				}
				else if(entry.TaxonRank == "genus") {
					cid_instance_of = cid_biotaxonomy_genus;
					name_buf = entry.GenericName.NotEmpty() ? entry.GenericName : entry.ScientificName;
				}
				else if(entry.TaxonRank == "family") {
					cid_instance_of = cid_biotaxonomy_family;
					name_buf = entry.ScientificName;
				}
				else if(entry.TaxonRank == "superfamily") {
					cid_instance_of = cid_biotaxonomy_superfamily;
					name_buf = entry.ScientificName;
				}
				else if(entry.TaxonRank == "order") {
					cid_instance_of = cid_biotaxonomy_order;
					name_buf = entry.ScientificName;
				}
				else if(entry.TaxonRank == "class") {
					cid_instance_of = cid_biotaxonomy_class;
					name_buf = entry.ScientificName;
				}
				else if(entry.TaxonRank == "phylum") {
					cid_instance_of = cid_biotaxonomy_phylum;
					name_buf = entry.ScientificName;
				}
				else if(entry.TaxonRank == "kingdom") {
					cid_instance_of = cid_biotaxonomy_kingdom;
					name_buf = entry.ScientificName;
				}
				else
					temp_buf = "undefinstance";
				if(_phase == phasePreprocess) {
					total_line_count++;
					if(entry.TaxonID) {
						acceptedname_ref_list.addnz(entry.AcceptedNameUsageID);
						parent_ref_list.addnz(entry.ParentNameUsageID);
					}
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.TaxonomicStatusText, sht, taxonomic_status_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.TaxonRank,       sht, taxon_rank_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Kingdom,         sht, kingdom_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Phylum,          sht, phylum_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Class,           sht, class_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Order,           sht, order_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Superfamily,     sht, superfamily_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Family,          sht, family_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.GenericName,     sht, generic_name_list, last_symb_id);
					InnerMethods::AddTaxonomyFactorToSymbHash(entry.Genus,           sht, genus_list, last_symb_id);
					if((line_no % 1000) == 0) {
						acceptedname_ref_list.sortAndUndup();
						parent_ref_list.sortAndUndup();

						taxonomic_status_list.sortAndUndup();
						taxon_rank_list.sortAndUndup();
						kingdom_list.sortAndUndup();
						phylum_list.sortAndUndup();
						class_list.sortAndUndup();
						order_list.sortAndUndup();
						superfamily_list.sortAndUndup();
						family_list.sortAndUndup();
						generic_name_list.sortAndUndup();
						genus_list.sortAndUndup();
					}
				}
				else if(_phase == phase1) {
					if(cid_instance_of) {
						LEXID  clex_id = 0;
						name_ss.clear();
						name_buf.Tokenize(" ", name_ss);
						for(uint nssp = 0; name_ss.get(&nssp, temp_buf);) {
							LEXID  word_id = 0;
							if(rDb.FetchWord(temp_buf, &word_id) > 0) {
							}
							else {
								uint   sht_id = 0;
								if(!sht.Search(temp_buf, &sht_id, 0)) {
									words_to_append.add(temp_buf);
									sht.Add(temp_buf, (sht_id = ++last_symb_id));
								}
							}
						}
						if(rDb.SearchSpecialWord(SrWordTbl::spcConcept, concept_symb_buf, &clex_id) > 0) {
						}
						else {
							taxon_symb_to_append.add(entry.TaxonID);
						}
					}
				}
				else if(_phase == phase2) {
					/*
						TaxonomicStatus: "synonym" "accepted name" "ambiguous synonym" "misapplied name" "provisionally accepted name"
						TaxonRank: species infraspecies kingdom phylum class order family superfamily genus
					*/
					if(cid_instance_of) {
						CONCEPTID cid = 0;
						CONCEPTID parent_cid = 0;
						CONCEPTID synon_cid = 0;
						NGID   ngram_id = 0;
						SrCPropList cpl;
						SrCProp cp;
						if(entry.ParentNameUsageID) {
							SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcGBIF, &entry.ParentNameUsageID, sizeof(entry.ParentNameUsageID), parent_concept_symb_buf);
							const int rcr = rDb.SearchConcept(parent_concept_symb_buf, &parent_cid);
							assert(rcr > 0); // Все концепции уже были созданы на фазе 1
						}
						else
							parent_concept_symb_buf.Z();
						ngram.clear();
						name_ss.clear();
						name_buf.Tokenize(" ", name_ss);
						for(uint nssp = 0; name_ss.get(&nssp, temp_buf);) {
							LEXID  word_id = 0;
							if(rDb.FetchWord(temp_buf, &word_id) > 0) {
							}
							else {
								// Все слова созданы на фазе 1. Так что здесь - ошибка
							}
							assert(word_id);
							ngram.add(word_id);
						}
						{
							const int rngr = rDb.ResolveNGram(ngram, &ngram_id);
							THROW(rngr);
							/*if(rngr == 2)*/ {
								//items_per_tx++;
								items_per_tx += 4;
								THROW(RechargeTransaction(p_ta, items_per_tx, max_items_per_tx));
							}
						}
						{
							int    skip_instance = 0;
							int    skip_subclass = 0;
							const int rcr = rDb.SearchConcept(concept_symb_buf, &cid);
							assert(rcr > 0); // Все концепции уже были созданы на фазе 1
							THROW(rDb.P_CNgT->Set(cid, ngram_id));
							items_per_tx++;
							THROW(RechargeTransaction(p_ta, items_per_tx, max_items_per_tx));
							// если rcr == 1, то концепция существовала до вызова ResolveConcept
							/*if(rDb.GetConceptPropList(cid, cpl) > 0) {
								for(uint pidx = 0; pidx < cpl.GetCount(); pidx++) {
									if(cpl.GetByPos(pidx, cp)) {
										CONCEPTID _val = 0;
										if(cp.PropID == prop_instance) {
											if(cp.Get(_val)) {
												if(_val == cid_instance_of)
													skip_instance = 1;
												else {
													; // @todo сообщение (мы нашли таксон, который по имени соответствует более чем одной категории таксонов)
												}
											}
										}
										else if(cp.PropID == prop_subclass) {
											if(parent_cid && cp.Get(_val)) {
												if(_val == parent_cid)
													skip_subclass = 1;
												else {
													; // @todo сообщение (мы нашли таксон, который по имени соответствует более чем одному родительскому таксону)
												}
											}
										}
									}
								}
							}*/
							if(!skip_instance) {
								BioTaxonomyCPropEntry cpe;
								cpe.CId = cid;
								cpe.Value = cid_instance_of;
								cpe.PropVariant = 1;
								THROW_SL(cprop_list.insert(&cpe));
								//THROW(rDb.SetConceptProp(cid, prop_instance, 0, cid_instance_of));
								//THROW(RechargeTransaction(p_ta, ++items_per_tx, max_items_per_tx));
							}
							if(parent_cid && !skip_subclass) {
								BioTaxonomyCPropEntry cpe;
								cpe.CId = cid;
								cpe.Value = parent_cid;
								cpe.PropVariant = 2;
								THROW_SL(cprop_list.insert(&cpe));
								//THROW(rDb.SetConceptProp(cid, prop_subclass, 0, parent_cid));
								//THROW(RechargeTransaction(p_ta, ++items_per_tx, max_items_per_tx));
							}
						}
					}
				}
			}
			if(_phase != phasePreprocess || total_line_count == 0) {
				PPWaitMsg(temp_buf.Z().Cat(line_no));
			}
			else {
				PPWaitPercent(line_no, total_line_count);
			}
		}
		if(_phase == phase1) {
			{
				if(!p_ta) {
					THROW_MEM(p_ta = new BDbTransaction(rDb, 1));
					THROW_DB(*p_ta);
				}
				if(!p_ta->IsStarted()) {
					THROW_DB(p_ta->Start(1));
				}
				{
					uint  items_per_tx = 0;
					const uint ssc = words_to_append.getCount();
					for(uint ssp = 0, ssi = 0; words_to_append.get(&ssp, temp_buf); ssi++) {
						LEXID  word_id = 0;
						const  int rwr = rDb.ResolveWord(temp_buf, &word_id);
						assert(oneof2(rwr, 2, 0));
						THROW(rwr);
						assert(word_id);
						if(rwr == 2) { // Было создано новое слово - добавим к нему известные нам признаки (пока только язык)
							if(!temp_buf.IsDigit()) { // @v10.2.0
								THROW(rDb.SetSimpleWordFlexiaModel_Express(word_id, wordform_id, 0));
								items_per_tx++;
							}
							THROW(RechargeTransaction(p_ta, items_per_tx, /*max_items_per_tx*/512));
						}
						PPWaitPercent(ssi+1, ssc, "phase1 accepting (words)");
					}
				}
				THROW_DB(p_ta->Commit(1));
			}
			{
				LongArray symb_list; // Список ид символов концепций в том же порядке, что и taxon_symb_to_append
				const uint tsc = taxon_symb_to_append.getCount();
				{
					uint  items_per_tx = 0;
					if(!p_ta->IsStarted()) {
						THROW_DB(p_ta->Start(1));
					}
					for(uint item_idx = 0; item_idx < tsc; item_idx++) {
						const long taxon_id = taxon_symb_to_append.get(item_idx);
						SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcGBIF, &taxon_id, sizeof(taxon_id), concept_symb_buf);
						LEXID lex_id = 0;
						THROW(rDb.P_WdT->AddSpecial(SrWordTbl::spcConcept, concept_symb_buf, &lex_id));
						symb_list.add(lex_id);
						items_per_tx++;
						THROW(RechargeTransaction(p_ta, items_per_tx, /*max_items_per_tx*/200));
						PPWaitPercent(item_idx+1, tsc, "phase1 accepting (concepts symb)");
					}
					THROW_DB(p_ta->Commit(1));
				}
				{
					assert(symb_list.getCount() == tsc);
					uint  items_per_tx = 0;
					if(!p_ta->IsStarted()) {
						THROW_DB(p_ta->Start(1));
					}
					for(uint item_idx = 0; item_idx < tsc; item_idx++) {
						const LEXID lex_id = symb_list.get(item_idx);
						SrConcept c;
						c.ID = 0;
						c.SymbID  = lex_id;
						THROW(rDb.P_CT->Add(c));
						THROW(RechargeTransaction(p_ta, ++items_per_tx, /*max_items_per_tx*/512+256));
						PPWaitPercent(item_idx+1, tsc, "phase1 accepting (concepts)");
					}
					THROW_DB(p_ta->Commit(1));
				}
			}
		}
		else if(_phase == phasePreprocess) {
			taxonomic_status_list.sortAndUndup();
			taxon_rank_list.sortAndUndup();
			kingdom_list.sortAndUndup();
			phylum_list.sortAndUndup();
			class_list.sortAndUndup();
			order_list.sortAndUndup();
			superfamily_list.sortAndUndup();
			family_list.sortAndUndup();
			generic_name_list.sortAndUndup();
			genus_list.sortAndUndup();
			{
				sht.BuildAssoc();
				SPathStruc ps(pFileName);
				ps.Nam.CatChar('-').Cat("stat");
				ps.Merge(temp_buf);
				SFile f_out(temp_buf, SFile::mWrite);
				//
				InnerMethods::WriteTaxonomyFactorList(f_out, "TaxonomicStatus", sht, taxonomic_status_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "TaxonRank", sht, taxon_rank_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Kingdom", sht, kingdom_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Phylum", sht, phylum_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Class", sht, class_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Order", sht, order_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Superfamily", sht, superfamily_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Family", sht, family_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "GenericName", sht, generic_name_list, line_buf);
				InnerMethods::WriteTaxonomyFactorList(f_out, "Genus", sht, genus_list, line_buf);
			}
		}
	}
	if(p_ta) {
		THROW_DB(p_ta->Commit(1));
		ZDELETE(p_ta);
	}
	if(cprop_list.getCount()) {
		items_per_tx = 0;
		BDbTransaction tra(rDb, 1);
		for(uint i = 0; i < cprop_list.getCount(); i++) {
			BioTaxonomyCPropEntry & r_cpe = cprop_list.at(i);
			if(r_cpe.PropVariant == 1) {
				THROW(rDb.SetConceptProp(r_cpe.CId, prop_instance, 0, r_cpe.Value));
			}
			else if(r_cpe.PropVariant == 2) {
				THROW(rDb.SetConceptProp(r_cpe.CId, prop_subclass, 0, r_cpe.Value));
			}
			THROW(RechargeTransaction(&tra, ++items_per_tx, 128));
			PPWaitPercent(i+1, cprop_list.getCount(), "Storing concept props");
		}
		THROW(tra.Commit(1));
	}
	CATCHZOK
	delete p_ta;
	PPWaitStop();
	return ok;
}
//
//
//
int PrcssrSartre::ImportTickers(SrDatabase & rDb, const char * pExchangeSymb, const char * pFileName)
{
	struct InnerMethods {
		static int FASTCALL AddFactorToSymbHash(const SString & rSymb, SymbHashTable & rSht, LongArray & rIdList, uint & rLastSymbId)
		{
			int    ok = -1;
			uint   sht_id = 0;
			if(rSymb.NotEmpty() && !rSht.Search(rSymb, &sht_id, 0))
				rSht.Add(rSymb, (sht_id = ++rLastSymbId));
			if(sht_id) {
				rIdList.add((long)sht_id);
				ok = 1;
			}
			return ok;
		}
		static int FASTCALL WriteFactorList(SFile & rF, const char * pTitle, SymbHashTable & rSht, const LongArray & rIdList, SString & rLineBuf)
		{
			int    ok = 1;
			SString temp_buf;
			rLineBuf.Z();
			for(uint i = 0; i < rIdList.getCount(); i++) {
				rLineBuf.Z();
				rSht.GetByAssoc(rIdList.get(i), temp_buf.Z());
				rF.WriteLine(rLineBuf.Cat(pTitle).Tab().Cat(temp_buf).CR());
			}
			return ok;
		}
	};
	int    ok = 1;
	uint   last_symb_id = 0;
	SymbHashTable sht(SKILOBYTE(1024), 0);
	LongArray sector_list;
	LongArray industry_list;
	struct Entry {
		Entry & Z()
		{
			Ticker.Z();
			Name.Z();
			LastSaleQuote = 0.0;
			MarketCap = 0.0;
			IPOYear = 0;
			Sector.Z();
			Industry.Z();
			return *this;
		}
		SString Ticker;
		SString Name;
		double LastSaleQuote;
		double MarketCap;
		int    IPOYear;
		SString Sector;
		SString Industry;
	};
	// "Symbol","Name","LastSale","MarketCap","ADR TSO","IPOyear","Sector","Industry","Summary Quote",
	SString temp_buf;
	SString line_buf;
	SString line_out_buf;
	Entry entry;
	SString src_file_name;
	(src_file_name = P.SrcPath).SetLastSlash().Cat(pFileName); // utf-8
	//
	SPathStruc ps(src_file_name);
	ps.Nam.CatChar('-').Cat(pExchangeSymb);
	ps.Ext = "txt";
	ps.Merge(temp_buf);
	SFile f_debug_out(temp_buf, SFile::mWrite);
	SFile f_in(src_file_name, SFile::mRead);
	THROW_SL(f_in.IsValid());
	{
		SString exchange_symb;
		uint   line_count = 0;
		CONCEPTID cid_genusrerum = 0; // форма собственности предприятия
		CONCEPTID cid_sector = 0;     //:negotiumtaxonomy_sector::negotiumtaxonomy_category    // сектор бизнеса
		CONCEPTID cid_indust = 0;    //:negotiumtaxonomy_industria::negotiumtaxonomy_category // сектор индустрии
		CONCEPTID cid_ripae = 0; // класс "банк" (::negotiumtaxonomy_coeptis)
		CONCEPTID cid_commutationem = 0; // класс "биржа" (::negotium_coeptis_commutationem)
		CONCEPTID cid_coeptis = 0; // класс "предприятие"
		CONCEPTID cid_ticker = 0;
		CONCEPTID cid_exchange = 0;
		const CONCEPTID prop_instance = rDb.GetReservedConcept(rDb.rcInstance);
		const CONCEPTID prop_subclass = rDb.GetReservedConcept(rDb.rcSubclass);
		SrNGram ng;
		SrWordForm wf_pattern_en;
		SrCPropList prop_list;
		Int64Array _clist;
		STokenizer::Item titem;
		STokenizer::ResultPosition rp;
		STokenizer::ResultPosition last_rp;
		STokenizer tknz(STokenizer::Param(STokenizer::fEachDelim|STokenizer::fDivAlNum, cpUTF8, " \t\n\r(){}[]<>,.:;\\/&$#@!?*^\"+=%")); // "-" здесь не является разделителем
		THROW(rDb.SearchConcept("negotiumtaxonomy_genusrerum", &cid_genusrerum));
		THROW(rDb.SearchConcept("negotiumtaxonomy_sector", &cid_sector));
		THROW(rDb.SearchConcept("negotiumtaxonomy_industria", &cid_indust));
		THROW(rDb.SearchConcept("negotiumtaxonomy_coeptis", &cid_coeptis));
		THROW(rDb.SearchConcept("negotium_coeptis_ripae", &cid_ripae));
		THROW(rDb.SearchConcept("negotium_coeptis_commutationem", &cid_commutationem));
		THROW(rDb.SearchConcept("stockticker", &cid_ticker));
		cid_exchange = rDb.TryOneWordForConcept((exchange_symb = pExchangeSymb).Utf8ToLower(), cid_commutationem, rDb.tryconceptGeneric);
		THROW(cid_exchange);
		wf_pattern_en.SetTag(SRWG_LANGUAGE, slangEN);
		{
			for(uint line_no = 1; f_in.ReadLine(line_buf); line_no++) {
				line_count++;
			}
			f_in.Seek(0);
		}
		{
			BDbTransaction tra(rDb.P_Db, 1);
			THROW(tra);
			for(uint line_no = 1; f_in.ReadLine(line_buf); line_no++) {
				if(line_no > 1) {
					line_buf.Chomp().Strip();
					SStrScan scan(line_buf);
					uint   fld_no = 0;
					CONCEPTID inst_sector_cid = 0;
					CONCEPTID inst_indust_cid = 0;
					entry.Z();
					while(scan.GetQuotedString(temp_buf)) {
						fld_no++;
						temp_buf.Strip();
						if(temp_buf.IsEqiAscii("n/a"))
							temp_buf.Z();
						switch(fld_no) {
							case 1: entry.Ticker = temp_buf; break;
							case 2: entry.Name = temp_buf; break;
							case 3: entry.LastSaleQuote = temp_buf.ToReal(); break;
							case 4: entry.MarketCap = temp_buf.ToReal(); break;
							case 5: break; // adr tso
							case 6: entry.IPOYear = temp_buf.ToLong(); break;
							case 7: entry.Sector = temp_buf; break;
							case 8: entry.Industry = temp_buf; break;
							case 9: break; // summary quote
						}
						if(!scan.IncrChr(','))
							break;
					}
					entry.Ticker.Utf8ToLower();
					entry.Sector.Utf8ToLower();
					entry.Industry.Utf8ToLower();
					InnerMethods::AddFactorToSymbHash(entry.Sector, sht, sector_list, last_symb_id);
					InnerMethods::AddFactorToSymbHash(entry.Industry, sht, industry_list, last_symb_id);
					uint   idx_first = 0;
					uint   idx_count = 0;
					if(entry.Sector.NotEmpty()) {
						idx_first = 0;
						idx_count = 0;
						tknz.Reset(0).RunSString(0, 0, entry.Sector, &idx_first, &idx_count);
						const int gngftr = rDb.ResolveNgFromTokenizer(tknz, idx_first, idx_count, &wf_pattern_en, 0, ng);
						THROW(gngftr);
						if(gngftr > 0) {
							assert(ng.ID != 0);
							inst_sector_cid = rDb.TryNgForConcept(ng.ID, cid_sector, rDb.tryconceptInstance);
							if(!inst_sector_cid) {
								THROW(rDb.CreateAnonymConcept(&inst_sector_cid));
								THROW(rDb.P_CNgT->Set(inst_sector_cid, ng.ID));
								THROW(rDb.SetConceptProp(inst_sector_cid, prop_instance, 0, cid_sector));
							}
						}
					}
					if(entry.Industry.NotEmpty()) {
						idx_first = 0;
						idx_count = 0;
						tknz.Reset(0).RunSString(0, 0, entry.Industry, &idx_first, &idx_count);
						const int gngftr = rDb.ResolveNgFromTokenizer(tknz, idx_first, idx_count, &wf_pattern_en, 0, ng);
						THROW(gngftr);
						if(gngftr > 0) {
							assert(ng.ID != 0);
							inst_indust_cid = rDb.TryNgForConcept(ng.ID, cid_indust, rDb.tryconceptInstance);
							if(!inst_indust_cid) {
								CONCEPTID indust_to_sect_cid = 0;
								THROW(rDb.CreateAnonymConcept(&inst_indust_cid));
								THROW(rDb.P_CNgT->Set(inst_indust_cid, ng.ID));
								THROW(rDb.SetConceptProp(inst_indust_cid, prop_instance, 0, cid_indust));
								if(inst_sector_cid) {
									if(rDb.GetConceptPropList(inst_indust_cid, prop_list) > 0) {
										SrCProp cp;
										if(prop_list.Get(inst_indust_cid, prop_subclass, cp)) {
											if(cp.Get(indust_to_sect_cid)) {
												if(indust_to_sect_cid == inst_sector_cid) {
													; // ok
												}
												else {
													; // ambiguity
												}
											}
										}
									}
									if(!indust_to_sect_cid) {
										THROW(rDb.SetConceptProp(inst_indust_cid, prop_subclass, 0, inst_sector_cid));
									}
								}
							}
						}
					}
					{
						line_out_buf.Z().Cat(entry.Ticker);
						entry.Name.Utf8ToLower();
						entry.Name.Decode_XMLENT(temp_buf);
						if(temp_buf.CmpSuffix("(the)", 1) == 0) {
							(entry.Name = "the").Space().Cat(temp_buf.Trim(temp_buf.Len()-5).Strip());
						}
						else if(temp_buf.CmpSuffix("(th", 1) == 0) { // иногда встречается такая ошибка в исходном тексте
							(entry.Name = "the").Space().Cat(temp_buf.Trim(temp_buf.Len()-3).Strip());
						}
						else
							entry.Name = temp_buf;
						idx_first = 0;
						idx_count = 0;
						tknz.Reset(0).RunSString(0, 0, entry.Name, &idx_first, &idx_count);
						line_out_buf.Tab();
						uint   out_tok_n = 0;
						for(uint tidx = 0; tidx < idx_count; tidx++) {
							if(tknz.Get(idx_first+tidx, titem)) {
								if(titem.Token == tknz.tokWord) {
									if(out_tok_n)
										line_out_buf.CatChar('|');
									line_out_buf.Cat(titem.Text);
									out_tok_n++;
								}
								else if(titem.Token == tknz.tokDelim) {
									if(titem.Text != " " && titem.Text != "\t") {
										if(out_tok_n)
											line_out_buf.CatChar('|');
										line_out_buf.Cat(titem.Text);
										out_tok_n++;
									}
								}
							}
						}
						//
						{
							CONCEPTID ent_cid = 0; // ид концепции, соответствующей предприятию
							CONCEPTID instance_of_cid = 0;
							CONCEPTID last_fc = 0;
							CONCEPTID fc = 0;
							uint   srch_idx_first = idx_first;
							uint   srch_idx_count = idx_count;
							do {
								fc = rDb.SearchConceptInTokenizer(cid_genusrerum, tknz, srch_idx_first, srch_idx_count, SrDatabase::tryconceptGeneric, rp);
								if(fc) {
									line_out_buf.Cat(" --- ");
									for(uint tj = rp.Start; tj < rp.Start+rp.Count; tj++) {
										if(tknz.Get(tj, titem)) {
											line_out_buf.Cat(titem.Text);
										}
									}
									line_out_buf.CatChar('(').Cat(fc).CatChar(')');
									last_fc = fc;
									last_rp = rp;
									if((rp.Start+rp.Count) == (idx_first+idx_count)) // искомая форма собственности - в конце строки: можно заканчивать.
										fc = 0;
									else {
										srch_idx_count = idx_count - (srch_idx_first-idx_first+1);
										srch_idx_first = rp.Start+1;
									}
								}
							} while(fc);
							{
								uint final_idx_first = idx_first;
								uint final_idx_count = idx_count;
								if(last_fc) {
									final_idx_count = (last_rp.Start - idx_first);
									while(final_idx_count && tknz.Get(idx_first+final_idx_count-1, titem)) {
										if(titem.Text == "," || titem.Text == " " || titem.Text == "\t")
											final_idx_count--;
										else
											break;
									}
								}
								line_out_buf.Cat(" !final[");
								if(final_idx_count) {
									for(uint tj = final_idx_first; tj < final_idx_first+final_idx_count; tj++) {
										if(tknz.Get(tj, titem)) {
											line_out_buf.Cat(titem.Text);
										}
									}
									if(entry.Industry.IsEqiAscii("banks") || entry.Industry.IsEqiAscii("commercial banks") || entry.Industry.IsEqiAscii("major banks"))
										instance_of_cid = cid_ripae;
									else
										instance_of_cid = cid_coeptis;
									if(instance_of_cid) {
										const int gngftr = rDb.ResolveNgFromTokenizer(tknz, final_idx_first, final_idx_count, 0, rDb.rngftoSkipDotAfterWord, ng);
										THROW(gngftr);
										if(gngftr > 0) {
											assert(ng.ID != 0);
											ent_cid = rDb.TryNgForConcept(ng.ID, instance_of_cid, rDb.tryconceptGeneric);
											if(!ent_cid) {
												THROW(rDb.CreateAnonymConcept(&ent_cid));
												THROW(rDb.P_CNgT->Set(ent_cid, ng.ID));
												THROW(rDb.SetConceptProp(ent_cid, prop_instance, 0, instance_of_cid));
												if(last_fc) {
													THROW(rDb.SetConceptProp(ent_cid, cid_genusrerum, 0, last_fc));
												}
												if(inst_indust_cid)
													THROW(rDb.SetConceptProp(ent_cid, cid_indust, 0, inst_indust_cid));
											}
										}
									}
								}
								line_out_buf.Cat("]");
							}
							if(ent_cid) {
								SString concept_symb_buf;
								temp_buf.Z().Cat(exchange_symb).CatChar('_').Cat(entry.Ticker);
								SrConcept::MakeSurrogateSymb(SrConcept::surrsymbsrcTICKER, temp_buf.cptr(), temp_buf.Len(), concept_symb_buf);
								CONCEPTID tk_cid_ = rDb.TryOneWordForConcept(entry.Ticker, cid_ticker, rDb.tryconceptInstance);
								CONCEPTID tk_cid_by_symb = 0;
								const int rcr = rDb.ResolveConcept(concept_symb_buf, &tk_cid_by_symb);
								THROW(rcr);
								assert(tk_cid_by_symb);
								if(tk_cid_ && tk_cid_ != tk_cid_by_symb) {
									; // something went wrong!
								}
								if(rcr > 1) {
									NGID   ng_id = 0;
									LEXID  tk_word_id = 0;
									THROW(rDb.ResolveWord(entry.Ticker, &tk_word_id));
									assert(tk_word_id);
									ng.Z().WordIdList.add(tk_word_id);
									THROW(rDb.ResolveNGram(ng.WordIdList, &ng_id));
									assert(ng_id);
									THROW(rDb.P_CNgT->Set(tk_cid_by_symb, ng_id));
									//
									THROW(rDb.SetConceptProp(tk_cid_by_symb, prop_instance, 0, cid_ticker));
									THROW(rDb.SetConceptProp(tk_cid_by_symb, cid_commutationem, 0, cid_exchange));
									THROW(rDb.SetConceptProp(tk_cid_by_symb, cid_coeptis, 0, ent_cid));
								}
							}
						}
						f_debug_out.WriteLine(line_out_buf.CR());
					}
				}
				PPWaitPercent(line_no+1, line_count, (temp_buf = "Tickers").CatParStr(exchange_symb));
			}
			THROW(tra.Commit(1));
			{
				sector_list.sortAndUndup();
				industry_list.sortAndUndup();
				{
					sht.BuildAssoc();
					SPathStruc ps(src_file_name);
					ps.Nam.CatChar('-').Cat("stat");
					ps.Merge(temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					//
					InnerMethods::WriteFactorList(f_out, "Sector", sht, sector_list, line_buf);
					InnerMethods::WriteFactorList(f_out, "Industry", sht, industry_list, line_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}


#if 0 // {
//
// Descr: Функция одноразового использования для формирования файлов
//   country-intl.csv currency-intl.csv locale-intl.csv language-intl.csv
//   содержащих наименования государств, валют, локалей и языков на очень многих языках.
//   Формируется из репозитория https://github.com/umpirsky/country-list.git
//   в котором добрый человек сложил все необходимые для этого данные.
//
int PrcssrSartre::PreprocessCountryNames(const char * pBaseSrcPath)
{
	int    ok = 1;
    SString result_file_name;
    SString temp_buf;
    SString line_buf;
    (result_file_name = pBaseSrcPath).SetLastSlash().Cat("all.csv");
    SFile f_out(result_file_name, SFile::mWrite);
	line_buf.Z().Cat("lang;id;value").CR();
	f_out.WriteLine(line_buf);
    {
    	PPWaitStart();
    	SString lang_code;
		SString code_buf, text_buf;
    	(temp_buf = pBaseSrcPath).SetLastSlash().Cat("*.*");
    	SDirEntry sde;
		for(SDirec sd(temp_buf, 1); sd.Next(&sde) > 0;) {
            if(sde.IsFolder() && !sde.IsSelf() && !sde.IsUpFolder()) {
                temp_buf.Z().Cat(pBaseSrcPath).SetLastSlash().Cat(sde.FileName).SetLastSlash().Cat("country.csv");
                if(fileExists(temp_buf)) {
					PPWaitMsg(temp_buf);
                    SFile f_in(temp_buf, SFile::mRead);
                    if(f_in.IsValid()) {
						for(uint line_count = 0; f_in.ReadLine(temp_buf); line_count++) {
                            if(line_count) { // Первая строка - заголовок полей
								temp_buf.Chomp().Strip();
								if(temp_buf.Divide(',', code_buf, text_buf) > 0) {
									code_buf.Strip().StripQuotes().Strip();
									text_buf.Strip().StripQuotes().Strip();
									line_buf.Z().Cat(sde.FileName).Semicol().Cat(code_buf).Semicol().Cat(text_buf);
									f_out.WriteLine(line_buf.CR());
								}
                            }
						}
                    }
                }
            }
		}
    }
    PPWaitStop();
	return ok;
}

int PrcssrSartre::PreprocessCurrencyNames(const char * pBaseSrcPath)
{
	int    ok = 1;
    SString result_file_name;
    SString temp_buf;
    SString line_buf;
    (result_file_name = pBaseSrcPath).SetLastSlash().Cat("all.csv");
    SFile f_out(result_file_name, SFile::mWrite);
	line_buf.Z().Cat("lang;id;value").CR();
	f_out.WriteLine(line_buf);
    {
    	PPWaitStart();
    	SString lang_code;
		SString code_buf, text_buf;
    	(temp_buf = pBaseSrcPath).SetLastSlash().Cat("*.*");
    	SDirEntry sde;
		for(SDirec sd(temp_buf, 1); sd.Next(&sde) > 0;) {
            if(sde.IsFolder() && !sde.IsSelf() && !sde.IsUpFolder()) {
                temp_buf.Z().Cat(pBaseSrcPath).SetLastSlash().Cat(sde.FileName).SetLastSlash().Cat("currency.csv");
                if(fileExists(temp_buf)) {
					PPWaitMsg(temp_buf);
                    SFile f_in(temp_buf, SFile::mRead);
                    if(f_in.IsValid()) {
						for(uint line_count = 0; f_in.ReadLine(temp_buf); line_count++) {
                            if(line_count) { // Первая строка - заголовок полей
								temp_buf.Chomp().Strip();
								if(temp_buf.Divide(',', code_buf, text_buf) > 0) {
									code_buf.Strip().StripQuotes().Strip();
									text_buf.Strip().StripQuotes().Strip();
									line_buf.Z().Cat(sde.FileName).Semicol().Cat(code_buf).Semicol().Cat(text_buf);
									f_out.WriteLine(line_buf.CR());
								}
                            }
						}
                    }
                }
            }
		}
    }
    PPWaitStop();
	return ok;
}

int PrcssrSartre::PreprocessLocaleNames(const char * pBaseSrcPath)
{
	int    ok = 1;
    SString result_file_name;
    SString temp_buf;
    SString line_buf;
    (result_file_name = pBaseSrcPath).SetLastSlash().Cat("all.csv");
    SFile f_out(result_file_name, SFile::mWrite);
	line_buf.Z().Cat("lang;id;value").CR();
	f_out.WriteLine(line_buf);
    {
    	PPWaitStart();
    	SString lang_code;
		SString code_buf, text_buf;
    	(temp_buf = pBaseSrcPath).SetLastSlash().Cat("*.*");
    	SDirEntry sde;
		for(SDirec sd(temp_buf, 1); sd.Next(&sde) > 0;) {
            if(sde.IsFolder() && !sde.IsSelf() && !sde.IsUpFolder()) {
                temp_buf.Z().Cat(pBaseSrcPath).SetLastSlash().Cat(sde.FileName).SetLastSlash().Cat("locales.csv");
                if(fileExists(temp_buf)) {
					PPWaitMsg(temp_buf);
                    SFile f_in(temp_buf, SFile::mRead);
                    if(f_in.IsValid()) {
						for(uint line_count = 0; f_in.ReadLine(temp_buf); line_count++) {
                            if(line_count) { // Первая строка - заголовок полей
								temp_buf.Chomp().Strip();
								if(temp_buf.Divide(',', code_buf, text_buf) > 0) {
									code_buf.Strip().StripQuotes().Strip();
									text_buf.Strip().StripQuotes().Strip();
									line_buf.Z().Cat(sde.FileName).Semicol().Cat(code_buf).Semicol().Cat(text_buf);
									f_out.WriteLine(line_buf.CR());
								}
                            }
						}
                    }
                }
            }
		}
    }
    PPWaitStop();
	return ok;
}

int PrcssrSartre::PreprocessLanguageNames(const char * pBaseSrcPath)
{
	int    ok = 1;
    SString result_file_name;
    SString temp_buf;
    SString line_buf;
    (result_file_name = pBaseSrcPath).SetLastSlash().Cat("all.csv");
    SFile f_out(result_file_name, SFile::mWrite);
	line_buf.Z().Cat("lang;id;value").CR();
	f_out.WriteLine(line_buf);
    {
    	PPWaitStart();
    	SString lang_code;
		SString code_buf, text_buf;
    	(temp_buf = pBaseSrcPath).SetLastSlash().Cat("*.*");
    	SDirEntry sde;
		for(SDirec sd(temp_buf, 1); sd.Next(&sde) > 0;) {
            if(sde.IsFolder() && !sde.IsSelf() && !sde.IsUpFolder()) {
                temp_buf.Z().Cat(pBaseSrcPath).SetLastSlash().Cat(sde.FileName).SetLastSlash().Cat("language.csv");
                if(fileExists(temp_buf)) {
					PPWaitMsg(temp_buf);
                    SFile f_in(temp_buf, SFile::mRead);
                    if(f_in.IsValid()) {
						for(uint line_count = 0; f_in.ReadLine(temp_buf); line_count++) {
                            if(line_count) { // Первая строка - заголовок полей
								temp_buf.Chomp().Strip();
								if(temp_buf.Divide(',', code_buf, text_buf) > 0) {
									code_buf.Strip().StripQuotes().Strip();
									text_buf.Strip().StripQuotes().Strip();
									line_buf.Z().Cat(sde.FileName).Semicol().Cat(code_buf).Semicol().Cat(text_buf);
									f_out.WriteLine(line_buf.CR());
								}
                            }
						}
                    }
                }
            }
		}
    }
    PPWaitStop();
	return ok;
}
#endif // } 0

int PrcssrSartre::Run()
{
	int    ok = 1;
	SString temp_buf;
	SString msg_buf;
	SString src_file_name;
	PPLogger logger;
	PPWaitStart();
	THROW_SL(pathValid(P.SrcPath, 1));
	{
		char * p_loc = setlocale(LC_CTYPE, "rus_rus.1251");
		//
		// Извлекает из текстовых файлов базы данных geonames концепции и записывает их в результирующий файл
		// Process_geonames("/PAPYRUS/Universe-HTT/DATA/GEO/geonames.org", "\\PAPYRUS\\Src\\SARTR\\data\\concept-geonames.txt");
		/*{
			BDbDatabase * p_rcv_db = new BDbDatabase(p_db_path, 0, BDbDatabase::oRecover);
			ZDELETE(p_rcv_db);
		}*/
		if(P.Flags & P.fImportFlexia) {
			SrDatabase db;
			THROW(db.Open(0, SrDatabase::oWriteStatOnClose)); // @todo Режим открытия
			{
				SrImportParam impp;
				impp.InputKind = impp.inpFlexiaModel;
				impp.LangID = slangEN;
				impp.CpID = cp1251;
				impp.Flags |= impp.fTest;
				(src_file_name = P.SrcPath).SetLastSlash().Cat("gramtab-en.tab");
				impp.SetField(impp.fldAncodeFileName, src_file_name);
				(src_file_name = P.SrcPath).SetLastSlash().Cat("morphs-en.mrd");
				impp.SetField(impp.fldFlexiaModelFileName, src_file_name);
				if(!db.ImportFlexiaModel(impp))
					logger.LogLastError();
			}
			{
				SrImportParam impp;
				impp.InputKind = impp.inpFlexiaModel;
				impp.LangID = slangRU;
				impp.CpID = cp1251;
				impp.Flags |= impp.fTest;
				(src_file_name = P.SrcPath).SetLastSlash().Cat("gramtab-ru.tab");
				impp.SetField(impp.fldAncodeFileName, src_file_name);
				(src_file_name = P.SrcPath).SetLastSlash().Cat("morphs-ru.mrd");
				impp.SetField(impp.fldFlexiaModelFileName, src_file_name);
				if(!db.ImportFlexiaModel(impp))
					logger.LogLastError();
			}
		}
		if(P.Flags & P.fImportConcepts) {
			SrDatabase db;
			THROW(db.Open(0, SrDatabase::oWriteStatOnClose)); // @todo Режим открытия
			{
				SrConceptParser parser(db);
				(src_file_name = P.SrcPath).SetLastSlash().Cat("concept.txt");
				if(!parser.Run(src_file_name)) {
					//PPGetMessage(mfError, )
					PPGetLastErrorMessage(1, temp_buf);
					(msg_buf = src_file_name).CatChar('(').Cat(parser.LineNo).CatChar(')').Space().Cat(temp_buf);
					logger.Log(msg_buf);
				}
			}
		}
		if(P.Flags & P.fImportHumNames) {
			//
			// Одноразовый вызов для формирования файла country-intl.csv содержащего наименования государств на многих языках.
			// PreprocessCountryNames("D:/DEV/Resource/Data/Geo/REPO-GIT/country-list/data");
			// PreprocessCurrencyNames("D:/DEV/Resource/Data/Geo/REPO-GIT/currency-list/data");
			// PreprocessLocaleNames("D:/DEV/Resource/Data/Geo/REPO-GIT/locale-list/data");
			// PreprocessLanguageNames("D:/DEV/Resource/Data/Geo/REPO-GIT/language-list/data");
			//
			SrDatabase db;
			THROW(db.Open(0, SrDatabase::oWriteStatOnClose));
			if(!ImportHumanNames(db, "name-firstname-ru.csv", "ru", SRPROPN_PERSONNAME, 0))
				logger.LogLastError();
			if(!ImportHumanNames(db, "name-surname-ru.csv", "ru", SRPROPN_FAMILYNAME, 1))
				logger.LogLastError();
			if(!ImportHumanNames(db, "name-firstname-en.csv", "en", SRPROPN_PERSONNAME, 0))
				logger.LogLastError();
			if(!ImportHumanNames(db, "name-firstname-es.csv", "es", SRPROPN_PERSONNAME, 0)) // @v10.0.06
				logger.LogLastError();
		}
		if(P.Flags & P.fImportBioTaxonomy) {
			SrDatabase db;
			const char * p_file_name = "D:/DEV/Resource/Data/Bio/Taxonomy/GBIF_registration-archive-complete/taxa.txt";
			THROW(db.Open(0, SrDatabase::oWriteStatOnClose|SrDatabase::oExclusive)); // @v10.0.12 SrDatabase::oExclusive
			if(!PrcssrSartre::ImportBioTaxonomy(db, p_file_name)) {
				logger.LogLastError();
			}
		}
		if(P.Flags & P.fImportTickers) {
			SrDatabase db;
			THROW(db.Open(0, SrDatabase::oWriteStatOnClose));
			if(!ImportTickers(db, "nyse", "companylist-nyse.csv"))
				logger.LogLastError();
			if(!ImportTickers(db, "nasdaq", "companylist-nasdaq.csv"))
				logger.LogLastError();
			if(!ImportTickers(db, "amex", "companylist-amex.csv"))
				logger.LogLastError();
		}
		if(P.Flags & P.fTestFlexia) {
			TestSearchWords();
		}
		if(P.Flags & P.fTestConcepts) {
			TestConcept();
		}
		if(P.Flags & P.fTestSyntaxParser) {
			TestSyntax();
		}
		// @v10.9.5 {
		if(P.Flags & P.fImport_UED_Llcc) {
			UED_Import_Lingua_LinguaLocus_Country_Currency(/*llccBaseListOnly*/0);
		}
		// } @v10.9.5 
		/*if(!TestImport_Words_MySpell())
			ret = -1;*/
		/*if(!TestImport_AncodeCollection())
			ret = -1;*/
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	PPWaitStop();
	return ok;
}

int PrcssrSartre::TestSearchWords()
{
	int    ok = 1;
	SString line_buf, temp_buf;
	SString word_buf;
	TSVector <SrWordInfo> info_list; // @v9.8.4 TSArray-->TSVector
	SrDatabase * p_db = DS.GetTLA().GetSrDatabase();
	if(p_db) {
		PPGetPath(PPPATH_TESTROOT, temp_buf);
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("sartre_test_words.txt");
		SFile in_file(temp_buf, SFile::mRead); // Данные в файле в кодировке UTF-8
		PPGetFilePath(PPPATH_OUT, "Sartr_TestSearchWords.txt", temp_buf);
		SFile out_file(temp_buf, SFile::mWrite);
		{
			while(in_file.ReadLine(word_buf)) {
				word_buf.Chomp().Strip();
				if(word_buf.NotEmpty()) {
					temp_buf = word_buf;
					info_list.clear();
					if(p_db->GetWordInfo(temp_buf, 0, info_list) > 0) {
						for(uint j = 0; j < info_list.getCount(); j++) {
							p_db->WordInfoToStr(info_list.at(j), temp_buf);
							(line_buf = word_buf).Cat("\t-->\t").Cat(temp_buf).CR();
							out_file.WriteLine(line_buf);
						}
					}
					else {
						(line_buf = word_buf).Cat("\t-->\t").Cat("not found").CR();
						out_file.WriteLine(line_buf);
					}
				}
			}
		}
		out_file.WriteLine(0);
		{
			const char * p_word = "Я"; /*"ТЕХНОЛОГИЧЕСКАЯ";*/ /*"КОНСТАНТИНОВИЧУ";*/
			SrWordForm wf;
			(temp_buf = p_word)/* @v10.4.5 .ToUtf8()*/;
			info_list.clear();
			wf.SetTag(SRWG_CASE, SRCASE_DATIVE);
			wf.SetTag(SRWG_COUNT, SRCNT_PLURAL);
			if(p_db->Transform_(temp_buf, wf, info_list) > 0) {
				for(uint j = 0; j < info_list.getCount(); j++) {
					p_db->WordInfoToStr(info_list.at(j), temp_buf);
					(line_buf = p_word).Transf(CTRANSF_OUTER_TO_UTF8);
					line_buf.Cat("\t-->\t").Cat(temp_buf).CR();
					out_file.WriteLine(line_buf);
				}
			}
		}
	}
	return ok;
}

int PrcssrSartre::TestConcept()
{
	int    ok = 1;
	SString line_buf, temp_buf, symb;
	SrDatabase * p_db = DS.GetTLA().GetSrDatabase();
	if(p_db) {
		PPGetPath(PPPATH_TESTROOT, temp_buf);
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("sartre_test_concepts.txt");
		SFile in_file(temp_buf, SFile::mRead); // Данные в файле в кодировке UTF-8
		PPGetFilePath(PPPATH_OUT, "Sartr_TestConcept.txt", temp_buf);
		SFile out_file(temp_buf, SFile::mWrite);
		StringSet tok_list;
		TSVector <SrWordInfo> info_list;
		Int64Array abbr_concept_list; // Список концепций, сопоставленных с аббревиатурой
		Int64Array clist, hlist;
		LongArray ng;
		SrNGram ng_abbr; // N-gram сопоставленная с аббревиатурой
		while(in_file.ReadLine(line_buf)) {
			line_buf.Chomp().Strip();
			if(line_buf.NotEmpty()) {
				temp_buf = line_buf;
				line_buf.Z().Cat(temp_buf).CR();
				tok_list.clear();
				temp_buf.Tokenize(0, tok_list);
				int    unkn_word = 0;
				ng.clear();
				info_list.clear();
				abbr_concept_list.clear();
				const uint tok_count = tok_list.getCount();
				for(uint sp = 0; !unkn_word && tok_list.get(&sp, temp_buf);) {
					if(tok_count == 1 && p_db->GetWordInfo(temp_buf, 0, info_list) > 0) {
						for(uint i = 0; i < info_list.getCount(); i++) {
							const SrWordInfo & r_wi = info_list.at(i);
							if(r_wi.AbbrExpID && p_db->P_NgT->Search(r_wi.AbbrExpID, &ng_abbr.Z()) > 0)
								p_db->GetNgConceptList(ng_abbr.ID, 0, abbr_concept_list);
						}
					}
					LEXID word_id = 0;
					if(p_db->FetchWord(temp_buf, &word_id) > 0)
						ng.add(word_id);
					else
						unkn_word = 1;
				}
				if(!unkn_word) {
					clist.clear();
					hlist.clear();
					NGID  ng_id = 0;
					if(p_db->SearchNGram(ng, &ng_id) > 0) {
						p_db->GetNgConceptList(ng_id, 0, clist);
					}
					clist.add(&abbr_concept_list);
					clist.sortAndUndup();
					for(uint j = 0; j < clist.getCount(); j++) {
						CONCEPTID cid = clist.get(j);
						SrCPropList cpl;
						SrCProp cp;
						p_db->GetConceptSymb(cid, symb);
						line_buf.Tab().Cat((temp_buf = symb).Utf8ToChar());
						line_buf.CatChar('(');
						if(p_db->GetConceptPropList(cid, cpl) > 0) {
							for(uint k = 0; k < cpl.GetCount(); k++) {
								if(cpl.GetByPos(k, cp)) {
									p_db->FormatProp(cp, 0, temp_buf);
									if(k)
										line_buf.CatDiv(',', 2);
									line_buf.Cat(temp_buf);
								}
							}
						}
						line_buf.CatChar(')');
						if(p_db->GetConceptHier(cid, hlist) > 0 && hlist.getCount()) {
							line_buf.Space().CatDiv(':', 2);
							for(uint k = 0; k < hlist.getCount(); k++) {
								CONCEPTID hcid = hlist.get(k);
								p_db->GetConceptSymb(hcid, symb);
								if(k)
									line_buf.CatDiv(',', 2);
								line_buf.Cat((temp_buf = symb).Utf8ToChar());
							}
						}
						line_buf.CR();
					}
				}
				line_buf.CR();
				out_file.WriteLine(line_buf);
			}
		}
	}
	return ok;
}

int DoProcessSartre(PrcssrSartreFilt * pFilt)
{
	int    ok = -1;
	PrcssrSartre prcssr(0);
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		PrcssrSartreFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}

#if 0 // {
//
//
//
struct GeoName {
	GeoName & Reset();
	long   ID;
	SString Name;
	double Latitude;
	double Longitude;
	char   FeatureClass[4]; // class
	char   FeatureCode[64]; // code
	char   CountryCode[16];
};

struct GeoFeatureCode {
	GeoFeatureCode & Reset();
	SString Code;
	SString Text;
	SString Descr;
};

struct GeoNameAlt {
	GeoNameAlt & Reset();
	long   ID;
	char   LinguaCode[16];
	char   Text[512];
};

GeoName & GeoName::Reset()
{
	ID = 0;
	Name = 0;
	Latitude = 0.0;
	Longitude = 0.0;
	FeatureCode[0] = 0;
	CountryCode[0] = 0;
	return *this;
}

GeoFeatureCode & GeoFeatureCode::Reset()
{
	Code = 0;
	Text = 0;
	Descr = 0;
	return *this;
}

GeoNameAlt & GeoNameAlt::Reset()
{
	ID = 0;
	LinguaCode[0] = 0;
	Text[0] = 0;
	return *this;
}

static SString & FASTCALL _CatSartrEntityPrefix(const char * pEntity, SString & rBuf)
{
	return rBuf.Colon().Cat(pEntity).CatChar('_');
}

int Process_geonames(const char * pPath, const char * pOutFileName)
{
	// featureCodes_en.txt
	// allCountries.txt
	// alternateNames.txt

	int    ok = 1;
	SString in_file_name, out_file_name, temp_buf, line_buf, out_buf;
	SStringU temp_ubuf, dest_ubuf;
	StringSet ss("\t");
	SFile outf(pOutFileName, SFile::mWrite);
	THROW(outf.IsValid());
	{
		GeoFeatureCode entry;
		{
			(in_file_name = pPath).SetLastSlash().Cat("featureCodes_en.txt");
			SFile inf(in_file_name, SFile::mRead);
			THROW(inf.IsValid());
			outf.WriteLine(out_buf.Z().CR().CatChar('{').CR());
			while(inf.ReadLine(line_buf)) {
				line_buf.Chomp().Strip();
				ss.setBuf(line_buf);
				entry.Reset();
				for(uint pos = 0, i = 0; ss.get(&pos, temp_buf); i++) {
					temp_buf.Strip().Utf8ToLower();
					switch(i) {
						case 0: entry.Code = temp_buf; break;
						case 1: entry.Text = temp_buf; break;
						case 2: entry.Descr = temp_buf; break;
					}
				}
				entry.Code.ReplaceChar('.', '_');
				out_buf.Z().Tab().Colon().Cat("geoloct").CatChar('_').Cat(entry.Code).CR();
				outf.WriteLine(out_buf);
			}
			outf.WriteLine(out_buf.Z().CatChar('}').Colon().Cat("geoloctype").CR());
		}
		{
			LongArray ling_list;
			GetLinguaList(ling_list);
			for(uint i = 0; i < ling_list.getCount(); i++) {
				GetLinguaCode(ling_list.get(i), temp_buf);
				temp_buf.ToUtf8().ToLower();
				(in_file_name = pPath).SetLastSlash().Cat("featureCodes").CatChar('_').Cat(temp_buf).DotCat("txt");
				if(fileExists(in_file_name)) {
					SFile inf(in_file_name, SFile::mRead);
					THROW(inf.IsValid());

					out_buf.Z().CR().CatBrackStr(temp_buf).CatChar('{').CR();
					outf.WriteLine(out_buf);

					while(inf.ReadLine(line_buf)) {
						line_buf.Chomp().Strip();
						ss.setBuf(line_buf);
						entry.Reset();
						for(uint pos = 0, i = 0; ss.get(&pos, temp_buf); i++) {
							temp_buf.Strip().Utf8ToLower();
							switch(i) {
								case 0: entry.Code = temp_buf; break;
								case 1: entry.Text = temp_buf; break;
								case 2: entry.Descr = temp_buf; break;
							}
						}
						entry.Code.ReplaceChar('.', '_');
						out_buf.Z().Tab().Cat(entry.Text).Cat("=:").Cat("geoloct").CatChar('_').Cat(entry.Code).CR();
						outf.WriteLine(out_buf);
					}
					out_buf.Z().CatChar('}').CR();
					outf.WriteLine(out_buf);
				}
			}
		}

	}
	{
		GeoName entry;
		(in_file_name = pPath).SetLastSlash().Cat("allCountries.txt");
		SFile inf(in_file_name, SFile::mRead);
		THROW(inf.IsValid());
		outf.WriteLine(out_buf.Z().CR());
		while(inf.ReadLine(line_buf)) {
			//
			// #0  geonameid         : integer id of record in geonames database
			// #1  name              : name of geographical point (utf8) varchar(200)
			// #2  asciiname         : name of geographical point in plain ascii characters, varchar(200)
			// #3  alternatenames    : alternatenames, comma separated varchar(5000)
			// #4  latitude          : latitude in decimal degrees (wgs84)
			// #5  longitude         : longitude in decimal degrees (wgs84)
			// #6  feature class     : see http://www.geonames.org/export/codes.html, char(1)
			// #7  feature code      : see http://www.geonames.org/export/codes.html, varchar(10)
			// #8  country code      : ISO-3166 2-letter country code, 2 characters
			// #9  cc2               : alternate country codes, comma separated, ISO-3166 2-letter country code, 60 characters
			// #10 admin1 code       : fipscode (subject to change to iso code), see exceptions below, see file admin1Codes.txt for display names of this code; varchar(20)
			// #11 admin2 code       : code for the second administrative division, a county in the US, see file admin2Codes.txt; varchar(80)
			// #12 admin3 code       : code for third level administrative division, varchar(20)
			// #13 admin4 code       : code for fourth level administrative division, varchar(20)
			// #14 population        : bigint (8 byte int)
			// #15 elevation         : in meters, integer
			// #16 dem               : digital elevation model, srtm3 or gtopo30, average elevation of 3''x3'' (ca 90mx90m) or 30''x30'' (ca 900mx900m) area in meters, integer. srtm processed by cgiar/ciat.
			// #17 timezone          : the timezone id (see file timeZone.txt) varchar(40)
			// #18 modification date : date of last modification in yyyy-MM-dd format
			//
			line_buf.Chomp().Strip();
			ss.setBuf(line_buf);
			entry.Reset();
			for(uint pos = 0, i = 0; ss.get(&pos, temp_buf); i++) {
				switch(i) {
					case 0: entry.ID = temp_buf.Strip().ToLong(); break;
					case 2: entry.Name = temp_buf.Strip().Utf8ToLower(); break;
					case 4: entry.Latitude = temp_buf.Strip().ToReal(); break;
					case 5: entry.Longitude = temp_buf.Strip().ToReal(); break;
					case 6: temp_buf.Strip().Utf8ToLower().CopyTo(entry.FeatureClass, sizeof(entry.FeatureClass)); break;
					case 7: temp_buf.Strip().Utf8ToLower().CopyTo(entry.FeatureCode, sizeof(entry.FeatureCode)); break;
					case 8: temp_buf.Strip().Utf8ToLower().CopyTo(entry.CountryCode, sizeof(entry.CountryCode)); break;
				}
			}
			_CatSartrEntityPrefix("geoloc", out_buf.Z()).Cat(entry.ID);
			out_buf.Colon().Cat("geoloc");
			out_buf.CatChar('(');
				if(entry.FeatureClass[0] && entry.FeatureCode[0]) {
					_CatSartrEntityPrefix("geoloct", out_buf).Cat(entry.FeatureClass).CatChar('_').Cat(entry.FeatureCode).CatDiv(',', 2);
				}
				if(entry.CountryCode[0] && entry.CountryCode[2] == 0) { // Код государства должен быть двузначным
					_CatSartrEntityPrefix("statu", out_buf).Cat(entry.CountryCode).CatDiv(',', 2);
				}
				out_buf.CatEq("latitude", entry.Latitude).CatDiv(',', 2).CatEq("longitude", entry.Longitude);
			out_buf.CatChar(')').CR();
			outf.WriteLine(out_buf);
		}
	}
	{
		// alternateNames.txt
		(in_file_name = pPath).SetLastSlash().Cat("alternateNames.txt");
		if(fileExists(in_file_name)) {
			int   sorted = 1;
			long  rec_count = 0;
			int64 last_id = 0;
			int64 id = 0;
			SFile inf(in_file_name, SFile::mRead);
			THROW(inf.IsValid());
			//
			// Прежде всего проверим, что файл отсортирован по идентификатору geoname (2-е поле)
			//
			while(sorted && inf.ReadLine(line_buf)) {
				rec_count++;
				line_buf.Chomp().Strip();
				ss.setBuf(line_buf);
				for(uint pos = 0, i = 0; ss.get(&pos, temp_buf); i++) {
					temp_buf.Strip().Utf8ToLower();
					switch(i) {
						case 1:
							id = temp_buf.ToInt64();
							break;
					}
				}
				if(id < last_id) {
					sorted = 0;
				}
				else {
					last_id = id;
				}
			}
			if(sorted) {
				long   rec_no = 0;
				last_id = 0;
				id = 0;
				GeoNameAlt entry;
				TSArray <GeoNameAlt> temp_list;
				inf.Seek(0);
				outf.WriteLine(out_buf.Z().CR());
				while(inf.ReadLine(line_buf)) {
					rec_no++;
					line_buf.Chomp().Strip();
					ss.setBuf(line_buf);
					entry.Reset();
					for(uint pos = 0, i = 0; ss.get(&pos, temp_buf); i++) {
						temp_buf.Strip().Utf8ToLower();
						switch(i) {
							case 1:
								entry.ID = temp_buf.ToLong();
								id = entry.ID;
								break;
							case 2:
								temp_buf.CopyTo(entry.LinguaCode, sizeof(entry.LinguaCode));
								break;
							case 3:
								temp_buf.CopyTo(entry.Text, sizeof(entry.Text));
								break;
						}
					}
					if(id != last_id && last_id) {
						//slangEN
						int    was_en = 0;
						uint   i;
						for(i = 0; i < temp_list.getCount(); i++) {
							const GeoNameAlt & r_entry = temp_list.at(i);
							if(r_entry.LinguaCode[0]) {
								int lc = RecognizeLinguaSymb(r_entry.LinguaCode, 1);
								if(lc) {
									if(lc == slangEN) {
										was_en = 1;
									}
									out_buf.Z().CatChar('[').Cat(r_entry.LinguaCode).CatChar(']').Cat(r_entry.Text).Cat("=:").Cat("geoloc").CatChar('_').Cat(r_entry.ID).CR();
									outf.WriteLine(out_buf);
								}
							}
							else {
							}
						}
						if(!was_en) {
							// ’ 2018, 2019
							for(i = 0; i < temp_list.getCount(); i++) {
								const GeoNameAlt & r_entry = temp_list.at(i);
								if(r_entry.LinguaCode[0] == 0) {
									temp_buf = r_entry.Text;
									temp_ubuf.Z().CopyFromUtf8Strict(temp_buf, temp_buf.Len());
									dest_ubuf = 0;
									int    is_ascii = 1;
									for(uint j = 0; is_ascii && j < temp_ubuf.Len(); j++) {
										const wchar_t wc = temp_ubuf.C(j);
										if(wc == 2018)
											dest_ubuf.CatChar(L'\'');
										else if(wc == 2019)
											dest_ubuf.CatChar(L'\'');
										else if(wc == L'\"')
											dest_ubuf.CatChar(L'\'');
										else if(wc == L'`')
											dest_ubuf.CatChar(L'\'');
										else if(wc > 0 && wc <= 127)
											dest_ubuf.CatChar(wc);
										else
											is_ascii = 0;
									}
									if(is_ascii) {
										dest_ubuf.CopyToUtf8(temp_buf.Z(), 1);
										out_buf.Z().CatBrackStr("en").Cat(temp_buf).Cat("=:").Cat("geoloc").CatChar('_').Cat(r_entry.ID).CR();
										outf.WriteLine(out_buf);
									}
								}
							}
						}
						temp_list.clear();
					}
					temp_list.insert(&entry);
					last_id = id;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
#endif // } 0
#if 0 // {
int ImportSartre()
{
	int    ok = 1;
	//const  char * p_db_path = "/PAPYRUS/PPY/BIN/SARTRDB";
	//PPIniFile ini_file;
	const char * p_src_data_path = "\\PAPYRUS\\Src\\SARTR\\data";
	//SString sartre_db_path;
	SString temp_buf;
	PPWaitStart();
	//PPGetPath(PPPATH_SPII, temp_buf);
	//PPGetPath(PPPATH_SARTREDB, sartre_db_path);
	//THROW_PP(sartre_db_path.NotEmpty() && pathValid(sartre_db_path, 1), PPERR_SARTREDBUNDEF);
	THROW_SL(pathValid(p_src_data_path, 1));
	{
		char * p_loc = setlocale(LC_CTYPE, "rus_rus.1251");
		//
		// Извлекает из текстовых файлов базы данных geonames концепции и записывает их в результирующий файл
		// Process_geonames("/PAPYRUS/Universe-HTT/DATA/GEO/geonames.org", "\\PAPYRUS\\Src\\SARTR\\data\\concept-geonames.txt");
		/*{
			BDbDatabase * p_rcv_db = new BDbDatabase(p_db_path, 0, BDbDatabase::oRecover);
			ZDELETE(p_rcv_db);
		}*/
		//ImportFlexiaModel(p_db_path);
		//static int ImportFlexiaModel(const char * pDbPath, const char * pSrcDataPath)
		{
			SString src_file_name;
			SrDatabase db;
			THROW(db.Open(/*sartre_db_path*/0));
			{
				SrImportParam impp;
				impp.InputKind = impp.inpFlexiaModel;
				impp.LangID = slangEN;
				impp.CpID = cp1251;
				impp.Flags |= impp.fTest;
				(src_file_name = p_src_data_path).SetLastSlash().Cat("gramtab-en.tab");
				impp.SetField(impp.fldAncodeFileName, src_file_name);
				(src_file_name = p_src_data_path).SetLastSlash().Cat("morphs-en.mrd");
				impp.SetField(impp.fldFlexiaModelFileName, src_file_name);
				THROW(db.ImportFlexiaModel(impp));
			}
			{
				SrImportParam impp;
				impp.InputKind = impp.inpFlexiaModel;
				impp.LangID = slangRU;
				impp.CpID = cp1251;
				impp.Flags |= impp.fTest;
				(src_file_name = p_src_data_path).SetLastSlash().Cat("gramtab-ru.tab");
				impp.SetField(impp.fldAncodeFileName, src_file_name);
				(src_file_name = p_src_data_path).SetLastSlash().Cat("morphs-ru.mrd");
				impp.SetField(impp.fldFlexiaModelFileName, src_file_name);
				THROW(db.ImportFlexiaModel(impp));
			}
		}
		TestSearchWords(/*sartre_db_path*/0);
		//TestImportConceptFile(p_db_path, "\\PAPYRUS\\Src\\SARTR\\data\\concept.txt");
		//TestConcept(p_db_path);
		/*if(!TestImport_Words_MySpell())
			ret = -1;*/
		/*if(!TestImport_AncodeCollection())
			ret = -1;*/
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}
#endif // }
#if 0 // {

int main(int argc, char * argv[])
{
	int    ret = 0;
	const  char * p_db_path = "/PAPYRUS/PPY/BIN/SARTRDB";
	SLS.Init("SARTR");
	char * p_loc = setlocale(LC_CTYPE, "rus_rus.1251");
	//
	// Извлекает из текстовых файлов базы данных geonames концепции и записывает их в результирующий файл
	// Process_geonames("/PAPYRUS/Universe-HTT/DATA/GEO/geonames.org", "\\PAPYRUS\\Src\\SARTR\\data\\concept-geonames.txt");
	/*{
		BDbDatabase * p_rcv_db = new BDbDatabase(p_db_path, 0, BDbDatabase::oRecover);
		ZDELETE(p_rcv_db);
	}*/
	//ImportFlexiaModel(p_db_path);
	//TestSearchWords(p_db_path);
	TestImportConceptFile(p_db_path, "\\PAPYRUS\\Src\\SARTR\\data\\concept.txt");
	TestConcept(p_db_path);
	/*if(!TestImport_Words_MySpell())
		ret = -1;*/
	/*if(!TestImport_AncodeCollection())
		ret = -1;*/

	return ret;
}

#endif // } 0

SrSyntaxRuleSet::MatchEntry::MatchEntry(uint textIdxStart, uint textIdxEnd, const SrSyntaxRuleSet::Rule * pRule, uint stkP) :
	TextIdxStart(textIdxStart), TextIdxEnd(textIdxEnd), P_Rule(pRule), StackP(stkP), ConceptId(0)
{
}

//
// Syntax rule
//
/*
	rule ::= rule_name '=' expr ';'
	expr ::= expr '|' expr | expr expr | '(' expr ')'
	expr ::= concept | morph | rule_name | LITERAL
	concept ::= ':' SYMB
	morph ::= '[' morph_symb_list ']'
	morph_symb_list ::= EMPTY | MORPH_SYMB morph_symb_list
*/
SrSyntaxRuleSet::ResolveRuleBlock::ResolveRuleBlock(SrDatabase & rDb, const STokenizer & rT, const SrSyntaxRuleSet::Rule * pRule) :
	R_Db(rDb), R_T(rT), TextIdx(0)
{
	SetupRule(pRule);
}

int SrSyntaxRuleSet::MatchListToStr(const TSVector <MatchEntry> & rML, const STokenizer & rT, SString & rBuf) const
{
	STokenizer::Item titem;
	for(uint i = 0; i < rML.getCount(); i++) {
		const MatchEntry & r_entry = rML.at(i);
		if(i)
			rBuf.Space();
		if(r_entry.P_Rule) {
			if(r_entry.StackP < r_entry.P_Rule->ES.getCount()) {
				ExprItemTextToStr(*(ExprItem *)r_entry.P_Rule->ES.at(r_entry.StackP), rBuf);
			}
			else
				rBuf.Cat("invalid_stack_item").CatParStr(r_entry.StackP);
		}
		else
			rBuf.Cat("zero_rule");
		rBuf.CatChar('(');
		for(uint j = r_entry.TextIdxStart; j <= r_entry.TextIdxEnd; j++) {
			if(rT.Get(j, titem))
				rBuf.Cat(titem.Text);
			else
				rBuf.Cat("???");
		}
		rBuf.CatChar(')');
	}
	return 1;
}


/*int SrSyntaxRuleSet::ResolveRuleBlock::MatchListToStr(const STokenizer & rT, const SrSyntaxRuleSet & rSet, SString & rBuf) const
{
	STokenizer::Item titem;
	for(uint i = 0; i < MatchList.getCount(); i++) {
		const MatchEntry & r_entry = MatchList.at(i);
		if(i)
			rBuf.Space();
		if(r_entry.P_Rule) {
			if(r_entry.StackP < r_entry.P_Rule->ES.getCount()) {
				rSet.ExprItemTextToStr(*(ExprItem *)r_entry.P_Rule->ES.at(r_entry.StackP), rBuf);
			}
			else
				rBuf.Cat("invalid_stack_item").CatParStr(r_entry.StackP);
		}
		else
			rBuf.Cat("zero_rule");
		rBuf.CatChar('(');
		for(uint j = r_entry.TextIdxStart; j <= r_entry.TextIdxEnd; j++) {
			if(rT.Get(j, titem))
				rBuf.Cat(titem.Text);
			else
				rBuf.Cat("???");
		}
		rBuf.CatChar(')');
	}
	return 1;
}*/

void FASTCALL SrSyntaxRuleSet::ResolveRuleBlock::GetTextItemWithAdvance(uint & rTIdx)
{
	while(R_T.Get(rTIdx, TItemBuf) && TItemBuf.Token == STokenizer::tokDelim && (TItemBuf.Text == " " || TItemBuf.Text == "\n"))
		rTIdx++;
}

int SrSyntaxRuleSet::ResolveRuleBlock::PutMatchEntryOnSuccess(uint txtIdxStart, uint txtIdxEnd, CONCEPTID conceptId)
{
	MatchEntry entry(txtIdxStart, txtIdxEnd, P_Rule, StackP);
	entry.ConceptId = conceptId;
	return MatchList.insert(&entry) ? 1 : PPSetErrorSLib();
}

uint SrSyntaxRuleSet::ResolveRuleBlock::GetMatchListPreservedP()
	{ return MatchList.getCount(); }

void FASTCALL SrSyntaxRuleSet::ResolveRuleBlock::TrimMatchListOnFailure(uint preservedP)
{
	uint i = MatchList.getCount();
	if(i > preservedP) do {
		MatchList.atFree(--i);
	} while(i > preservedP);
}

void FASTCALL SrSyntaxRuleSet::ResolveRuleBlock::SetupRule(const SrSyntaxRuleSet::Rule * pRule)
{
	P_Rule = pRule;
	StackP = P_Rule ? P_Rule->ES.getPointer() : 0;
}

void SrSyntaxRuleSet::ResolveRuleBlock::PushInnerState()
{
	InnerStackItem item;
	item.P_Rule = P_Rule;
	item.StackP = StackP;
	item.TextIdx = TextIdx;
	InnerStack.push(item);
}

int FASTCALL SrSyntaxRuleSet::ResolveRuleBlock::PopInnerState(int dontRestoreTextIdx)
{
	InnerStackItem item;
	int    ok = InnerStack.pop(item);
	if(ok) {
		P_Rule = item.P_Rule;
		StackP = item.StackP;
		if(!dontRestoreTextIdx)
			TextIdx = item.TextIdx;
	}
	return ok;
}

SrSyntaxRuleSet::ExprItem::ExprItem(uint16 kind) : K(kind), ArgCount(0), Op(0), RSymb(0), VarP(0), VarItemRef(-1)
{
}

SrSyntaxRuleSet::ExprStack::ExprStack() : TSStack <ExprItem>()
{
}

int FASTCALL SrSyntaxRuleSet::ExprStack::Push(const ExprStack & rS)
{
	int    ok = -1;
    for(uint i = 0; i < rS.getPointer(); i++) {
        push(*(ExprItem *)rS.at(i));
        ok = 1;
    }
    return ok;
}

SrSyntaxRuleSet::Rule::Rule() : Flags(0), NameP(0)
{
}

int SrSyntaxRuleSet::ExprItemTextToStr(const ExprItem & rI, SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	SString var_buf;
	switch(rI.K) {
		case kLiteral:
			GetS(rI.SymbP, temp_buf);
			rBuf.CatQStr(temp_buf);
			break;
		case kConcept:
			if(rI.SymbP)
				GetS(rI.SymbP, temp_buf);
			if(rI.VarP) {
				GetS(rI.VarP, var_buf);
				temp_buf.CatChar('/').Cat(var_buf);
			}
			rBuf.Colon().Cat(temp_buf).Space();
			break;
		case kConceptInstance:
			if(rI.SymbP)
				GetS(rI.SymbP, temp_buf);
			if(rI.VarP) {
				GetS(rI.VarP, var_buf);
				temp_buf.CatChar('/').Cat(var_buf);
			}
			rBuf.Cat("!:").Cat(temp_buf).Space();
			break;
		case kConceptSubclass:
			if(rI.SymbP)
				GetS(rI.SymbP, temp_buf);
			if(rI.VarP) {
				GetS(rI.VarP, var_buf);
				temp_buf.CatChar('/').Cat(var_buf);
			}
			rBuf.Cat("!::").Cat(temp_buf).Space();
			break;
		case kMorph:
			GetS(rI.SymbP, temp_buf);
			rBuf.CatChar('[').Cat(temp_buf).CatChar(']');
			break;
		case kRule:
			GetS(rI.SymbP, temp_buf);
			rBuf.CatChar('#').Cat(temp_buf).Space();
			break;
		default:
			GetS(rI.SymbP, temp_buf);
			rBuf.Cat("???").Cat(temp_buf).Space();
			ok = 0;
			break;
	}
	return ok;
}

int SrSyntaxRuleSet::ExprItemToStr(ExprStack & rS, const ExprItem & rI, SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	if(rI.K == kOp) {
		uint arg_count = rI.ArgCount;
		if(rI.Op == opAnd)
			rBuf.CatDiv('&', 1);
		else if(rI.Op == opOr)
			rBuf.CatDiv('|', 1);
		else if(rI.Op == opConcat)
			rBuf.CatDiv('+', 1);
		else if(rI.Op == opOneof)
			rBuf.Space().Cat("oneof").Space();
		else
			rBuf.CatDiv('?', 1);
		rBuf.CatChar('(');
		for(uint i = 0; i < arg_count; i++) {
			ExprItem operand;
			THROW(rS.pop(operand));
			THROW(ExprItemToStr(rS, operand, rBuf)); // @recursion
		}
		rBuf.CatChar(')');
	}
	else {
		ExprItemTextToStr(rI, rBuf);
	}
	CATCHZOK
	return ok;
}

int SrSyntaxRuleSet::ExprStackToStr(ExprStack & rS, SString & rBuf) const
{
	int    ok = 1;
	ExprItem item;
	if(rS.pop(item)) {
		THROW(ExprItemToStr(rS, item, rBuf));
	}
	rBuf.Semicol();
	if(rS.getPointer()) {
		CALLEXCEPT();
	}
	CATCHZOK
	return ok;
}

int SrSyntaxRuleSet::RuleToStr(const Rule * pR, SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(pR) {
		SString temp_buf;
		GetS(pR->NameP, temp_buf);
		rBuf.Cat(temp_buf).CatChar('=');
		{
			ExprStack temp_es = pR->ES;
			ExprStackToStr(temp_es, rBuf);
		}
	}
	return ok;
}

SrSyntaxRuleSet::SrSyntaxRuleSet() : SStrGroup(), LineNo(0), State(0)
{
}

SrSyntaxRuleSet::~SrSyntaxRuleSet()
{
}

uint SrSyntaxRuleSet::GetRuleCount() const
	{ return RL.getCount(); }
const SrSyntaxRuleSet::Rule * FASTCALL SrSyntaxRuleSet::GetRule(uint pos) const
	{ return (pos < RL.getCount()) ? RL.at(pos) : 0; }

int SrSyntaxRuleSet::GetRuleName(uint pos, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 0;
	if(pos < RL.getCount()) {
		const Rule * p_rule = RL.at(pos);
		if(p_rule)
			ok = GetS(p_rule->NameP, rBuf);
	}
	return ok;
}

const SrSyntaxRuleSet::Rule * FASTCALL SrSyntaxRuleSet::SearchRuleByName(const char * pNameUtf8) const
{
	const SrSyntaxRuleSet::Rule * p_result = 0;
	SString temp_buf;
	for(uint i = 0; !p_result && i < RL.getCount(); i++) {
		const Rule * p_rule = RL.at(i);
		if(p_rule && p_rule->NameP) {
			GetS(p_rule->NameP, temp_buf);
			if(temp_buf == pNameUtf8)
				p_result = p_rule;
		}
	}
	return p_result;
}

void FASTCALL SrSyntaxRuleSet::SkipComment(SStrScan & rScan)
{
	SString temp_buf;
    while(rScan.Get("//", temp_buf)) {
		if(rScan.Search("\x0D\x0A")) {
			rScan.IncrLen();
			rScan.Incr(2);
			LineNo++;
		}
		else if(rScan.Search("\n")) {
			rScan.IncrLen();
			rScan.Incr(1);
			LineNo++;
		}
    }
}

void FASTCALL SrSyntaxRuleSet::ScanSkip(SStrScan & rScan)
{
	uint   line_count = 0;
	rScan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine, &line_count);
	LineNo += line_count;
	SkipComment(rScan);
}

int FASTCALL SrSyntaxRuleSet::IsOperand(SStrScan & rScan, uint * pLen)
{
	int    k = 0;
	uint   len = 0;
	ScanSkip(rScan);
	if(rScan.Is("!::")) {
		k = kConceptSubclass;
		len = 3;
	}
	else if(rScan.Is("!:")) {
		k = kConceptInstance;
		len = 2;
	}
	else if(rScan.Is(':')) { // concept
		k = kConcept;
		len = 1;
	}
	else if(rScan.Is('[')) { // morph
		k = kMorph;
		len = 1;
	}
	else if(rScan.Is('#')) { // rule
		k = kRule;
		len = 1;
	}
	else if(rScan.Is('\"')) { // literal
		k = kLiteral;
		len = 1;
	}
	ASSIGN_PTR(pLen, len);
	return k;
}

int SrSyntaxRuleSet::ParseExpression(SStrScan & rScan, ExprStack & rS, int untilChr)
{
	int    ok = -1;
	uint   seq_count = 0; // Количество последовательных операндов (их соединяем операцией &)
	SString temp_buf;
	ExprStack stack_temp;
	uint   operand_len = 0;
	int    k = IsOperand(rScan, &operand_len);
	if(oneof3(k, kConcept, kConceptInstance, kConceptSubclass)) { // concept
		ExprItem item(k);
		rScan.Incr(operand_len);
		if(rScan[0] == '/') {
			rScan.Incr();
			THROW_PP(rScan.GetIdent(temp_buf), PPERR_SR_S_VARSYMBEXPECTED); // @err После '/' ожидается идент переменной
			AddS(temp_buf, &item.VarP);
		}
		else {
			THROW_PP(rScan.GetIdent(temp_buf), PPERR_SR_S_CONCEPTIDEXPECTED); // @err Ожидается идент концепции
			AddS(temp_buf, &item.SymbP);
			if(rScan[0] == '/') {
				rScan.Incr();
				THROW_PP(rScan.GetIdent(temp_buf), PPERR_SR_S_VARSYMBEXPECTED); // @err После '/' ожидается идент переменной
				AddS(temp_buf, &item.VarP);
			}
		}
		stack_temp.push(item);
	}
	else if(k == kMorph) { // morph
		rScan.Incr(operand_len);
		StringSet ss(" ");
		ScanSkip(rScan);
		while(rScan.GetIdent(temp_buf)) {
			THROW(SrWordForm::StrToToken(temp_buf, 0));
			ss.add(temp_buf.ToLower());
			ScanSkip(rScan);
		}
		THROW_PP(rScan.Is(']'), PPERR_SR_S_RBRACKEXPECTED); // @err Ожидается ]
		rScan.Incr();
		{
			ExprItem item(kMorph);
			temp_buf = ss.getBuf();
			AddS(temp_buf, &item.SymbP);
			stack_temp.push(item);
		}
	}
	else if(k == kRule) { // rule
		rScan.Incr(operand_len);
		THROW_PP(rScan.GetIdent(temp_buf), PPERR_SR_S_RULEIDEXPECTED); // @err Ожидается идент правила
		{
			ExprItem item(kRule);
			AddS(temp_buf, &item.SymbP);
			stack_temp.push(item);
		}
	}
	else if(k == kLiteral) { // literal
		THROW_PP(rScan.GetQuotedString(temp_buf), PPERR_SR_S_LITERAL); // @err Ошибка считывания литерала
		{
			ExprItem item(kLiteral);
			AddS(temp_buf, &item.SymbP);
			stack_temp.push(item);
		}
	}
	else if(rScan.Is("oneof")) {
		rScan.Incr(sstrlen("oneof"));
        ScanSkip(rScan);
        THROW_PP(rScan.GetQuotedString(temp_buf), PPERR_SR_S_LITERALONEOFEXPECTED);
		{
			SStrScan inner_scan(temp_buf);
			SString tok_buf;
			ExprItem op_item(kOp);
			op_item.Op = opOneof;
			TSVector <ExprItem> arg_list; // @v9.8.6 TSArray-->TSVector
			//
			// Чтобы аргументы в стеке были в том же порядке, в каком их перечислили, сначала
			// вставим их во временный массив arg_list а потом из этого массива задом-на-перед
			// затолкаем в стек.
			//
            do {
				inner_scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine);
				tok_buf.Z();
                while(inner_scan[0] && !oneof4(inner_scan[0], ' ', '\t', '\n', '\r')) {
                    tok_buf.CatChar(inner_scan[0]);
                    inner_scan.Incr();
                }
				if(tok_buf.NotEmptyS()) {
					ExprItem item(kLiteral);
					AddS(tok_buf, &item.SymbP);
					THROW_SL(arg_list.insert(&item));
					op_item.ArgCount++;
				}
            } while(inner_scan[0]);
			THROW_PP(op_item.ArgCount, PPERR_SR_S_ONEOFEMPY); // @err
			assert(op_item.ArgCount == arg_list.getCount());
			{
				uint _c = arg_list.getCount();
				if(_c) do {
					stack_temp.push(arg_list.at(--_c));
				} while(_c);
			}
			stack_temp.push(op_item);
		}
	}
	else if(rScan.Is('(')) { // parents - inner expression
		int    r;
		rScan.Incr();
		THROW(r = ParseExpression(rScan, stack_temp, ')')); // @recursion
	}
	if(ok < 0) {
		int   next_k = IsOperand(rScan, 0);
		int   op = 0;
		if(next_k)
			op = opAnd;
		else if(rScan.Is('(')) { // Далее: сложный операнд () после неявной операции AND
			// Символ не "съедаем" - он нужен рекурентному вызову ParseExpression следующему далее
			op = opAnd;
		}
		else if(rScan.Is('&')) {
			rScan.Incr();
			op = opAnd;
		}
		else if(rScan.Is('|')) {
			rScan.Incr();
			op = opOr;
		}
		else if(rScan.Is('+')) {
			rScan.Incr();
			op = opConcat;
		}
		else if(rScan.Is(')')) { // end of inner expression
			THROW_PP(untilChr == ')', PPERR_SR_S_RPAREXPECTED); // @err unexpected ')'
			if(stack_temp.getPointer()) {
				rS.Push(stack_temp);
			}
			rScan.Incr();
			ok = 1;
		}
		else if(rScan.Is(';')) { // end of expression
			THROW_PP(untilChr == ';', PPERR_SR_S_SEMICOLEXPECTED); // @err unexpected ';'
			if(stack_temp.getPointer()) {
				rS.Push(stack_temp);
			}
			rScan.Incr();
			ok = 1;
		}
		else {
			ok = 0; // @err invalid construction rScan
		}
		if(ok < 0) {
			THROW_PP(op, PPERR_SR_S_OPOROPDEXPECTED); // @err expected operator or operand
			THROW_PP(stack_temp.getPointer(), PPERR_SR_S_UNEXPOPERATOR); // @err Unexpected operator (left operand is empty)
			{
				ExprItem op_item(kOp);
				op_item.Op = op;
				op_item.ArgCount = 2;
				//
				ExprStack stack_right;
				THROW(ParseExpression(rScan, stack_right, untilChr)); // @recursion
				rS.Push(stack_right);
				rS.Push(stack_temp);
				rS.push(op_item);
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrSyntaxRuleSet::Parse(const SString & rS)
{
	int    ok = 1;
	SString temp_buf;
	LineNo = 0;
	State = 0;
	SStrScan scan(rS);
	do {
		int    non_terminal_rule = 0;
		SkipComment(scan);
		ScanSkip(scan);
		if(scan.Is('#')) {
			scan.Incr();
			non_terminal_rule = 1;
		}
		THROW_PP(scan.GetIdent(temp_buf), PPERR_SR_S_RULEIDEXPECTED); // @err Ожидается идент правила
		ScanSkip(scan);
		THROW_PP(scan.Is("="), PPERR_SR_S_EQEXPECTED); // @err Ожидается '='
		scan.Incr();
		THROW_PP(SearchRuleByName(temp_buf) == 0, PPERR_SR_RULENAMEEXISTS);
		ScanSkip(scan);
		{
			Rule * p_rule = RL.CreateNewItem();
			THROW_SL(p_rule);
			if(non_terminal_rule)
				p_rule->Flags |= Rule::fNonTerminal;
			THROW_SL(AddS(temp_buf, &p_rule->NameP));
			THROW(ParseExpression(scan, p_rule->ES, ';'));
		}
		ScanSkip(scan);
	} while(scan[0] != 0);
    CATCHZOK
	return ok;
}

int SrSyntaxRuleSet::ResolveSyntaxRules(SrDatabase & rDb)
{
	int    ok = 1;
	CONCEPTID cid = 0;
	SString temp_buf;
	SString var_buf;
	for(uint i = 0; i < RL.getCount(); i++) {
		SrSyntaxRuleSet::Rule * p_rule = RL.at(i);
		if(p_rule) {
			for(uint j = 0; j < p_rule->ES.getPointer(); j++) {
				SrSyntaxRuleSet::ExprItem & r_ei = *(SrSyntaxRuleSet::ExprItem *)p_rule->ES.at(j);
				switch(r_ei.K) {
					case SrSyntaxRuleSet::kConceptInstance:
					case SrSyntaxRuleSet::kConceptSubclass:
					case SrSyntaxRuleSet::kConcept:
						if(r_ei.SymbP) {
							if(!r_ei.RSymb) {
								if(GetS(r_ei.SymbP, temp_buf)) {
									if(rDb.SearchConcept(temp_buf, &cid))
										r_ei.RSymb = cid;
								}
							}
						}
						else if(r_ei.VarP && r_ei.VarItemRef < 0) {
							if(GetS(r_ei.VarP, var_buf) && var_buf.Len()) {
								for(uint n = 0; n < p_rule->ES.getPointer(); n++) {
									if(n != j) {
										SrSyntaxRuleSet::ExprItem & r_ei_inner = *(SrSyntaxRuleSet::ExprItem *)p_rule->ES.at(n);
										if(r_ei_inner.VarP && r_ei_inner.SymbP) {
											if(oneof3(r_ei_inner.K, SrSyntaxRuleSet::kConcept, SrSyntaxRuleSet::kConceptInstance, SrSyntaxRuleSet::kConceptSubclass)) {
												if(GetS(r_ei_inner.VarP, temp_buf) && temp_buf == var_buf) {
													if(r_ei.VarItemRef < 0)
														r_ei.VarItemRef = (int)n;
													else {
														// Не однозначность!
													}
												}
											}
										}
									}
								}
							}
						}
						break;
					case SrSyntaxRuleSet::kMorph:
						if(!r_ei.RSymb) {
							if(GetS(r_ei.SymbP, temp_buf)) {
								if(temp_buf.NotEmptyS()) {
									uint   wf_pos = 0;
									SrWordForm * p_new_wf = WfList.CreateNewItem(&wf_pos);
									THROW_SL(p_new_wf);
									p_new_wf->FromStr(temp_buf);
									r_ei.RSymb = (wf_pos + 1);
								}
							}
						}
						break;
					case SrSyntaxRuleSet::kRule:
						if(!r_ei.RSymb) {
							if(GetS(r_ei.SymbP, temp_buf)) {
								const SrSyntaxRuleSet::Rule * p_inner_rule = SearchRuleByName(temp_buf);
								if(p_inner_rule) {
									r_ei.RSymb = (uint64)p_inner_rule;
								}
							}
						}
						break;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrSyntaxRuleSet::__ResolveExprRule(ResolveRuleBlock & rB, int unrollStackOnly) const
{
	int    ok = -1;
	int    r;
	const  uint preserve_tidx = rB.TextIdx;
	const  uint preserve_match_p = rB.GetMatchListPreservedP();
	const  SrSyntaxRuleSet::ExprStack & r_st = rB.P_Rule->ES;
	const  SrSyntaxRuleSet::ExprItem * p_sti = (const SrSyntaxRuleSet::ExprItem *)r_st.at(--rB.StackP);
	switch(p_sti->K) {
		case SrSyntaxRuleSet::kOp:
			if(p_sti->Op == SrSyntaxRuleSet::opAnd) {
				assert(p_sti->ArgCount == 2);
				if(unrollStackOnly) {
					THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
					THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
				}
				else {
					THROW(r = __ResolveExprRule(rB, 0)); // @recursion
					if(r > 0) {
						THROW(r = __ResolveExprRule(rB, 0)); // @recursion
					}
					else {
						THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
					}
					if(r > 0)
						ok = 1;
				}
			}
			else if(p_sti->Op == SrSyntaxRuleSet::opOr) {
				assert(p_sti->ArgCount == 2);
				if(unrollStackOnly) {
					THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
					THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
				}
				else {
					THROW(r = __ResolveExprRule(rB, 0)); // @recursion
					if(r < 0) {
						THROW(r = __ResolveExprRule(rB, 0)); // @recursion
					}
					else {
						THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
					}
					if(r > 0) {
						ok = 1;
					}
				}
			}
			else if(p_sti->Op == SrSyntaxRuleSet::opOneof) {
                if(unrollStackOnly) {
                    for(uint i = 0; i < p_sti->ArgCount; i++) {
						THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
                    }
                }
                else {
					for(uint i = 0; i < p_sti->ArgCount; i++) {
                        THROW(r = __ResolveExprRule(rB, BIN(ok > 0))); // @recursion (unroll stack on failure)
						if(r > 0)
							ok = 1;
					}
                }
			}
			else if(p_sti->Op == SrSyntaxRuleSet::opConcat) {
				assert(p_sti->ArgCount == 2);
				// @notimplemented
				THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
				THROW(__ResolveExprRule(rB, 1)); // @recursion (unroll stack)
			}
			break;
		case SrSyntaxRuleSet::kRule:
			if(!unrollStackOnly) {
				if(p_sti->RSymb) {
					const uint preserve_tidx = rB.TextIdx;
					rB.PushInnerState();
					rB.SetupRule((SrSyntaxRuleSet::Rule *)p_sti->RSymb);
					ok = __ResolveExprRule(rB, 0); // @recursion
					//
					// Мы не будем (пока) учитывать в списке успешных разборов внутреннии вызовы вложенных выражений.
					// То есть, мы при успешном сопоставлении очищаем список rB.MatchList (при неуспешном он будет
					// секвестирован в конце функции).
					//
					if(ok > 0) {
						rB.PopInnerState(1 /* dont't restore text idx */); // При успешном разрешении восстанавливать индекс текста не надо - это наш результат.
						assert(rB.TextIdx > 0);
						CONCEPTID inh_concept_id = 0;
						for(uint i = preserve_match_p; i < rB.GetMatchList().getCount(); i++) {
							const MatchEntry & r_me = rB.GetMatchList().at(i);
							if(r_me.ConceptId) {
								if(!inh_concept_id)
									inh_concept_id = r_me.ConceptId;
								else {
									inh_concept_id = 0; // Неоднозначность - не наследуем концепцию
									break;
								}
							}
						}
						rB.TrimMatchListOnFailure(preserve_match_p);
						rB.PutMatchEntryOnSuccess(preserve_tidx, rB.TextIdx-1, inh_concept_id);
					}
					else {
						rB.PopInnerState(0 /* dont't restore text idx */);
					}
					THROW(ok);
				}
			}
			break;
		case SrSyntaxRuleSet::kLiteral:
			if(!unrollStackOnly) {
				GetS(p_sti->SymbP, rB.TempBuf);
				uint   tidx = rB.TextIdx;
				rB.GetTextItemWithAdvance(tidx);
				if(rB.TItemBuf.Text == rB.TempBuf) {
					rB.PutMatchEntryOnSuccess(rB.TextIdx, tidx, 0);
					rB.TextIdx = tidx+1;
					ok = 1;
				}
			}
			break;
		case SrSyntaxRuleSet::kMorph:
			if(!unrollStackOnly) {
				if(p_sti->RSymb > 0 && p_sti->RSymb <= WfList.getCount()) {
					uint   tidx = rB.TextIdx;
					rB.GetTextItemWithAdvance(tidx);
					if(rB.TItemBuf.Token == STokenizer::tokWord) {
						const SrWordForm * p_wf = WfList.at((uint)(p_sti->RSymb-1));
						if(rB.R_Db.IsWordInForm(rB.TItemBuf.Text, *p_wf) > 0) {
							rB.PutMatchEntryOnSuccess(rB.TextIdx, tidx, 0);
							rB.TextIdx = tidx+1;
							ok = 1;
						}
					}
				}
			}
			break;
		case SrSyntaxRuleSet::kConcept:
		case SrSyntaxRuleSet::kConceptInstance:
		case SrSyntaxRuleSet::kConceptSubclass:
			if(!unrollStackOnly) {
				CONCEPTID  target_cid = p_sti->RSymb;
				if(target_cid == 0 && p_sti->VarItemRef >= 0) { // Ссылка на другой операнд
					assert(p_sti->VarItemRef < (int)r_st.getCount());
					for(uint mi = 0; mi < rB.GetMatchList().getCount(); mi++) {
						const MatchEntry & r_me = rB.GetMatchList().at(mi);
						if((int)r_me.StackP == p_sti->VarItemRef && r_me.ConceptId) {
							target_cid = r_me.ConceptId;
							break;
						}
					}
				}
				if(target_cid > 0) {
					SrNGram sng;
					SrNGram sng_local;
					uint   tidx = rB.TextIdx;
					TSVector <NGID> ng_list; // @v9.8.4 TSArray-->TSVect
					Int64Array concept_list; // Список концепций, соответствующих отдельной N-грамме
					Int64Array concept_hier_list; // Иерархия отдельной концепции
					LEXID  word_id = 0;
					rB.GetTextItemWithAdvance(tidx);
					if(rB.TItemBuf.Token == STokenizer::tokWord && rB.R_Db.FetchWord(rB.TItemBuf.Text, &word_id) > 0) {
						sng.WordIdList.add(word_id);
						TSVector <SrWordAssoc> wa_list;
						TSVector <SrWordInfo> wi_list;
						TSVector <NGID> abbr_ng_list; // Список N-грамм, соответствующих аббревиатуре word_id (если это-таки аббревиатура)
						rB.R_Db.GetBaseWordInfo(word_id, 0, 0, wa_list, wi_list);
						for(uint wiidx = 0; wiidx < wi_list.getCount(); wiidx++) {
							const SrWordInfo & r_wi = wi_list.at(wiidx);
							if(r_wi.AbbrExpID && rB.R_Db.P_NgT->Search(r_wi.AbbrExpID, 0) > 0) {
								if(TryNgForConcept(rB, r_wi.AbbrExpID, p_sti, target_cid, tidx) > 0) {
									ok = 1;
									// Мы нашли соответствие по аббревиатуре. Пропускаем последующую точку, если она есть.
									// Здесь неточность: точку надо пропускать только если аббревиатура это предполагает, но пока так.
									if(rB.R_T.Get(rB.TextIdx, rB.TItemBuf) && rB.TItemBuf.Text == ".") {
										rB.TextIdx++;
									}
								}
							}
						}
						if(ok < 0 && rB.R_Db.P_NgT->SearchByPrefix(sng, ng_list) > 0) {
							SrNGramCollection sng_list;
							for(uint i = 0; i < ng_list.getCount(); i++) {
								SrNGram * p_ng = sng_list.CreateNewItem();
								THROW_SL(p_ng);
								THROW(rB.R_Db.P_NgT->Search(ng_list.at(i), p_ng) > 0); // Не может такого быть, что этой n-граммы не было (мы только что ее нашли)
							}
							sng_list.SortByLength();
							const  uint local_preserve_tidx = tidx;
							uint   _c = sng_list.getCount();
							if(_c) do {
								SrNGram * p_ng = sng_list.at(--_c);
								THROW(p_ng->WordIdList.get(0) == word_id);
								int    is_match = 1;
								tidx = local_preserve_tidx; // @v9.9.11
								for(uint j = 1 /*! 0-й элемент уже проверен*/; is_match && j < p_ng->WordIdList.getCount(); j++) {
									const LEXID ng_word_id = p_ng->WordIdList.get(j);
									if(rB.R_T.Get(++tidx, rB.TItemBuf)) {
										if(rB.TItemBuf.Token == STokenizer::tokWord) {
											LEXID next_word_id = 0;
											if(rB.R_Db.FetchWord(rB.TItemBuf.Text, &next_word_id) > 0 && next_word_id == ng_word_id) {
												; // ok
											}
											else
												is_match = 0; // break loop
										}
										else if(rB.TItemBuf.Token == STokenizer::tokDelim) {
											if(rB.TItemBuf.Text == " ")
												continue;
											else
												is_match = 0; // break loop
										}
									}
								}
								if(is_match) {
									ok = TryNgForConcept(rB, p_ng->ID, p_sti, target_cid, tidx);
								}
							} while(ok < 0 && _c);
						}
					}
				}
			}
			break;
	}
	CATCHZOK
	if(ok <= 0) {
		rB.TextIdx = preserve_tidx;
		rB.TrimMatchListOnFailure(preserve_match_p);
	}
	return ok;
}

int SrSyntaxRuleSet::TryNgForConcept(ResolveRuleBlock & rB, NGID ngID, const SrSyntaxRuleSet::ExprItem * pSti, CONCEPTID targetCID, uint tidx) const
{
	int    ok = -1;
	long   tryconcept_option = -1;
	switch(pSti->K) {
		case SrSyntaxRuleSet::kConcept: tryconcept_option = SrDatabase::tryconceptGeneric; break;
		case SrSyntaxRuleSet::kConceptInstance: tryconcept_option = SrDatabase::tryconceptInstance; break;
		case SrSyntaxRuleSet::kConceptSubclass: tryconcept_option = SrDatabase::tryconceptSubclass; break;
	}
	const CONCEPTID cid = rB.R_Db.TryNgForConcept(ngID, targetCID, tryconcept_option);
	if(cid) {
		rB.PutMatchEntryOnSuccess(rB.TextIdx, tidx, cid);
		rB.TextIdx = tidx+1;
		ok = 1;
	}
	return ok;
}
//
//
//
SrSyntaxRuleTokenizer::SrSyntaxRuleTokenizer() : STokenizer(Param(STokenizer::fDivAlNum|STokenizer::fEachDelim, cpUTF8, " \t\n\r(){}[]<>,.:;-\\/&$#@!?*^\"+=%"))
{
}

int SrSyntaxRuleTokenizer::ProcessString(const char *pResource, const SString & rTextUtf8, uint * pIdxFirst, uint * pIdxCount)
{
	return RunSString(pResource, 0, rTextUtf8, pIdxFirst, pIdxCount);
}
//
//
//
int SrSyntaxRuleSet::ProcessText(SrDatabase & rDb, SrSyntaxRuleTokenizer & rT, uint tidxFirst, uint tidxCount, TSCollection <Result> & rResultList) const
{
	int    ok = -1;
	rResultList.freeAll();
	if(tidxCount) {
		uint   tidx = tidxFirst; // text index
		const SrSyntaxRuleSet::Rule * p_rule = 0;
		const uint rcount = GetRuleCount();
		for(uint tidx = 0; tidx < tidxCount;) {
			uint next_tidx_max = tidx+1;
			for(uint ridx = 0; ridx < rcount; ridx++) {
				p_rule = GetRule(ridx);
				if(p_rule && !(p_rule->Flags & SrSyntaxRuleSet::Rule::fNonTerminal)) {
					uint   stack_p = p_rule->ES.getPointer();
					SrSyntaxRuleSet::ResolveRuleBlock blk(rDb, rT, p_rule);
					blk.TextIdx = tidx;
					int r = __ResolveExprRule(blk, 0);
					if(r > 0) {
						Result * p_result_item = rResultList.CreateNewItem();
						p_result_item->RuleIdx = ridx;
						p_result_item->TIdxFirst = tidx;
						p_result_item->TIdxNext = blk.TextIdx;
						p_result_item->MatchList = blk.GetMatchList();

						SETMAX(next_tidx_max, blk.TextIdx);
						ok = 1;
					}
				}
			}
			tidx = next_tidx_max;
		}
	}
	return ok;
}

int SrSyntaxRuleSet::__ProcessText2(SrDatabase & rDb, const char * pResource, const SString & rTextUtf8, const char * pOutFileName) const
{
	int    ok = 1;
	SFile * p_f_out = 0;
	SString line_buf;
	SString temp_buf;
	SrSyntaxRuleTokenizer t;
	uint   idx_first = 0;
	uint   idx_count = 0;
	STokenizer::Item item_;
	t.ProcessString(pResource, rTextUtf8, &idx_first, &idx_count);
	if(idx_count) {
		//
		const SrSyntaxRuleSet::Rule * p_rule = 0;
		if(pOutFileName) {
			THROW_MEM(p_f_out = new SFile(pOutFileName, SFile::mWrite));
			if(!p_f_out->IsValid()) {
				ZDELETE(p_f_out);
			}
		}
		//
		TSCollection <Result> result_list;
		if(ProcessText(rDb, t, idx_first, idx_count, result_list) > 0) {
			for(uint residx = 0; residx < result_list.getCount(); residx++) {
				const Result * p_result = result_list.at(residx);
				if(p_result) {
					p_rule = GetRule(p_result->RuleIdx);
					if(p_f_out) {
						line_buf.Z();
						for(uint i = p_result->TIdxFirst; i < p_result->TIdxNext; i++) {
							t.Get(i, item_);
							line_buf.Cat(item_.Text);
						}
						GetS(p_rule->NameP, temp_buf);
						line_buf.CatDiv(':', 1).Cat(temp_buf).CR();
						p_f_out->WriteLine(line_buf);
						//
						//blk.MatchListToStr(t, *this, line_buf.Z());
						MatchListToStr(p_result->MatchList, t, line_buf.Z());
						p_f_out->WriteLine(line_buf.CR());
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_f_out;
	return ok;
}

int PrcssrSartre::TestSyntax()
{
	int    ok = 1;
	PPLogger logger;
	SString src_file_name;
	SString out_file_name;
	SString line_buf;
	SString src_buf;
	SString temp_buf;
	(src_file_name = P.SrcPath).SetLastSlash().Cat(/*"syntax-test.txt"*/"syntax.sr");
	SFile f_in(src_file_name, SFile::mRead);
	THROW_SL(f_in.IsValid());
	while(f_in.ReadLine(line_buf)) {
		src_buf.Cat(line_buf);
	}
	{
		(out_file_name = P.SrcPath).SetLastSlash().Cat("syntax-test.out");
		SFile f_out(out_file_name, SFile::mWrite);
		THROW_SL(f_out.IsValid());
		{
			SrSyntaxRuleSet srs;
			int r = srs.Parse(src_buf);
			if(!r) {
				PPGetLastErrorMessage(1, temp_buf);
				line_buf.Z().Cat(src_file_name).CatParStr(srs.LineNo).CatDiv(':', 2).Cat(temp_buf);
				logger.Log(line_buf);
				CALLEXCEPT();
			}
			for(uint i = 0; i < srs.RL.getCount(); i++) {
				const SrSyntaxRuleSet::Rule * p_rule = srs.RL.at(i);
				if(p_rule) {
					srs.RuleToStr(p_rule, line_buf);
					line_buf.CR();
					f_out.WriteLine(line_buf);
				}
			}
			f_out.Flush();
			{
				SrDatabase * p_db = DS.GetTLA().GetSrDatabase();
				if(p_db) {
					(src_file_name = P.SrcPath).SetLastSlash().Cat("syntax-test-text.txt");
					SFile f_text(src_file_name, SFile::mRead);
					src_buf.Z();
					while(f_text.ReadLine(line_buf)) {
						src_buf.Cat(line_buf);
					}
					src_buf.Utf8ToLower();
					THROW(srs.ResolveSyntaxRules(*p_db));
					{
						(src_file_name = P.SrcPath).SetLastSlash().Cat("syntax-test-text.out");
						srs.__ProcessText2(*p_db, "test-text", src_buf, src_file_name);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
//country-intl.csv 

struct LlccUglyEntry {
	char   Id[20];
	char   Lang[20];
	char   Text[256];
};

static IMPL_CMPFUNC(LlccUglyEntry, p1, p2)
{
	const LlccUglyEntry * p_i1 = static_cast<const LlccUglyEntry *>(p1);
	const LlccUglyEntry * p_i2 = static_cast<const LlccUglyEntry *>(p2);
	int    si = strcmp(p_i1->Id, p_i2->Id);
	if(si == 0)
		si = strcmp(p_i1->Lang, p_i2->Lang);
	return si;
}

int PrcssrSartre::UED_Import_Lingua_LinguaLocus_Country_Currency(uint llccFlags)
{
	int    ok = 1;
	const  char * p_base_path = "/papyrus/src/rsrc/data/sartre";
	enum {
		_idLingua   = 0x0033,
		_idLocus    = 0x0034,
		_idStatu    = 0x003f,
		_idCurrency = 0x0041,
	};
	SIntToSymbTabEntry entries[] = {
		{ _idLingua, "language-intl.csv" },
		{ _idLocus, "locale-intl.csv" },
		{ _idStatu, "country-intl.csv" },
		{ _idCurrency, "currency-intl.csv" },
	};
	SString path;
	SString line_buf;
	SString lang_buf;
	SString id_buf;
	SString value_buf;
	SString temp_buf;
	StringSet ss(";");
	//StringSet base_list;
	StringSet lingua_base_list;
	StringSet locus_base_list;
	StringSet statu_base_list;
	StringSet currency_base_list;
	StrAssocArray lingua_id_list;
	StrAssocArray locus_id_list;
	StrAssocArray statu_id_list;
	StrAssocArray currency_id_list;
	//
	// Так как все элементы в этих наборах данных помечаются идентификатором языка,
	// то в сете base_list_lang соберем список всех таких идентификаторов.
	//
	StringSet base_list_lang; 
	(path = p_base_path).SetLastDSlash();
	if(llccFlags & llccBaseListOnly) {
		path.Cat("ued_llcc_baselist.txt");
	}
	else {
		path.Cat("ued_llcc.ued");
	}
	SFile f_out(path, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	{
		for(uint i = 0; i < SIZEOFARRAY(entries); i++) {
			const SIntToSymbTabEntry & r_entry = entries[i];
			(path = p_base_path).SetLastDSlash().Cat(r_entry.P_Symb);
			SFile f_in(path, SFile::mRead);
			THROW_SL(f_in.IsValid())
			{
				StringSet * p_base_list = 0;
				switch(r_entry.Id) {
					case _idLingua: p_base_list = &lingua_base_list; break;
					case _idLocus: p_base_list = &locus_base_list; break;
					case _idStatu: p_base_list = &statu_base_list; break;
					case _idCurrency: p_base_list = &currency_base_list; break;
					default: assert(0); break;
				}
				if(p_base_list) {
					for(uint ln = 0; f_in.ReadLine(line_buf); ln++) {
						line_buf.Chomp().Strip();
						if(ln > 0) {
							// lang;id;value
							ss.clear();
							ss.setBuf(line_buf);
							for(uint fldidx = 1, ssp = 0; ss.get(&ssp, temp_buf); fldidx++) {
								temp_buf.Strip();
								switch(fldidx) {
									case 1: lang_buf = temp_buf; break;
									case 2: id_buf = temp_buf; break;
									case 3: value_buf = temp_buf; break;
								}
							}
							if(!base_list_lang.searchNcAscii(lang_buf, 0))
								base_list_lang.add(lang_buf);
							if(!p_base_list->searchNcAscii(id_buf, 0))
								p_base_list->add(id_buf);
						}
					}
					p_base_list->sort();
					if(llccFlags & llccBaseListOnly) {
						temp_buf.Z();
						switch(r_entry.Id) {
							case _idLingua: temp_buf.Cat("//").Space().Cat("lingua"); break;
							case _idLocus: temp_buf.Cat("//").Space().Cat("locus"); break;
							case _idStatu: temp_buf.Cat("//").Space().Cat("statu"); break;
							case _idCurrency: temp_buf.Cat("//").Space().Cat("currency"); break;
							default: assert(0); break;
						}
						f_out.WriteLine(temp_buf.CR());
						for(uint blp = 0; p_base_list->get(&blp, temp_buf);) {
							f_out.WriteLine(temp_buf.CR());
						}
					}
				}
			}
		}
		base_list_lang.sort();
	}
	if(llccFlags & llccBaseListOnly) {
		temp_buf.Z();
		temp_buf.Cat("//").Space().Cat("lingua full list");
		f_out.WriteLine(temp_buf.CR());
		for(uint blp = 0; base_list_lang.get(&blp, temp_buf);)
			f_out.WriteLine(temp_buf.CR());
	}
	if(!(llccFlags & llccBaseListOnly)) {
		{
			//
			// На этой фазе присваиваем символам идентификаторы
			//
			for(uint i = 0; i < SIZEOFARRAY(entries); i++) {
				const SIntToSymbTabEntry & r_entry = entries[i];
				StrAssocArray * p_id_list = 0;
				StringSet * p_base_list = 0;
				switch(r_entry.Id) {
					case _idLingua: 
						p_id_list = &lingua_id_list; 
						p_base_list = &lingua_base_list;
						break;
					case _idLocus: 
						p_id_list = &locus_id_list; 
						p_base_list = &locus_base_list;
						break;
					case _idStatu: 
						p_id_list = &statu_id_list; 
						p_base_list = &statu_base_list;
						break;
					case _idCurrency: 
						p_id_list = &currency_id_list; 
						p_base_list = &currency_base_list;
						break;
					default: assert(0); break;
				}
				if(p_id_list) {
					{
						assert(p_id_list->getCount() == 0);
						if(r_entry.Id == _idLocus) {
							assert(lingua_id_list.getCount());
							//
							// Локали обрабатываем специальным образом: 
							// -- сначала ищем полное сопоставления среди языков и, если находим, то ид локали будет равен ид соответствующего языка
							// -- для оставшихся локалей присваиваем идентификаторы, начинающиеся с максимального значения, полученного на
							//    предыдущем шаге плюс 100 (невростенический байас).
							//
							LongArray reckoned_pos_list;
							{
								for(uint ssp = 0, locus_pos = 0; p_base_list->get(&ssp, temp_buf); locus_pos++) {
									uint   lingua_pos = 0;
									if(lingua_id_list.SearchByText(temp_buf, 1, &lingua_pos)) {
										long local_id = lingua_id_list.at_WithoutParent(lingua_pos).Id;
										p_id_list->Add(local_id, temp_buf);
										reckoned_pos_list.add(static_cast<long>(locus_pos));
									}
								}
							}
							{
								long locus_id;
								p_id_list->GetMaxID(&locus_id);
								locus_id += 100;
								for(uint ssp = 0, locus_pos = 0; p_base_list->get(&ssp, temp_buf); locus_pos++) {
									if(!reckoned_pos_list.lsearch(static_cast<long>(locus_pos))) {
										p_id_list->Add(locus_id++, temp_buf);
									}
								}
							}
						}
						else {
							long nn = 0;
							for(uint ssp = 0; p_base_list->get(&ssp, temp_buf);) {
								p_id_list->Add(++nn, temp_buf);
							}
						}
					}
				}
			}
		}
		{
			//
			// Последняя фаза: выгружаем языковые текстовые ассоциации сущностей
			//
			for(uint i = 0; i < SIZEOFARRAY(entries); i++) {
				const SIntToSymbTabEntry & r_entry = entries[i];
				(path = p_base_path).SetLastDSlash().Cat(r_entry.P_Symb);
				SFile f_in(path, SFile::mRead);
				THROW_SL(f_in.IsValid())
				{
					StrAssocArray * p_id_list = 0;
					StringSet * p_base_list = 0;
					switch(r_entry.Id) {
						case _idLingua: 
							p_id_list = &lingua_id_list; 
							p_base_list = &lingua_base_list;
							break;
						case _idLocus: 
							p_id_list = &locus_id_list; 
							p_base_list = &locus_base_list;
							break;
						case _idStatu: 
							p_id_list = &statu_id_list; 
							p_base_list = &statu_base_list;
							break;
						case _idCurrency: 
							p_id_list = &currency_id_list; 
							p_base_list = &currency_base_list;
							break;
						default: assert(0); break;
					}
					assert(p_id_list && p_base_list);
					if(p_id_list && p_base_list) {
						{
							for(uint j = 0; j < p_id_list->getCount(); j++) {
								StrAssocArray::Item id_item = p_id_list->at_WithoutParent(j);
								uint64 final_id = (static_cast<uint64>(r_entry.Id) << 32) | static_cast<uint32>(id_item.Id);
								temp_buf.Z().CatHex(final_id).Space().CatQStr(id_item.Txt);
								f_out.WriteLine(temp_buf.CR());
							}
						}
						TSVector <LlccUglyEntry> value_list;
						for(uint ln = 0; f_in.ReadLine(line_buf); ln++) {
							line_buf.Chomp().Strip();
							if(ln > 0) {
								// lang;id;value
								ss.clear();
								ss.setBuf(line_buf);
								lang_buf.Z();
								id_buf.Z();
								value_buf.Z();
								for(uint fldidx = 1, ssp = 0; ss.get(&ssp, temp_buf); fldidx++) {
									temp_buf.Strip();
									switch(fldidx) {
										case 1: lang_buf = temp_buf; break;
										case 2: id_buf = temp_buf; break;
										case 3: value_buf = temp_buf; break;
									}
								}
								LlccUglyEntry value;
								MEMSZERO(value);
								STRNSCPY(value.Id, id_buf);
								STRNSCPY(value.Lang, lang_buf);
								STRNSCPY(value.Text, value_buf);
								value_list.insert(&value);
							}
						}
						value_list.sort(PTR_CMPFUNC(LlccUglyEntry));
						for(uint validx = 0; validx < value_list.getCount(); validx++) {
							const LlccUglyEntry & r_le = value_list.at(validx);
							uint id_pos = 0;
							if(p_id_list->SearchByText(r_le.Id, 1, &id_pos)) {
								StrAssocArray::Item id_item = p_id_list->at_WithoutParent(id_pos);
								assert(sstreq(id_item.Txt, r_le.Id));
								int   do_skip_lang_entry = 0; // Если !0 то строчку пропускаем поскольку такое описание уже определено для
									// языкового идентификатора высшего уровня (например: fr_GP-->fr).
								{
									uint revidx = validx;
									if(revidx) {
										temp_buf = r_le.Lang;
										const size_t lang_len = temp_buf.Len();
										do {
											const LlccUglyEntry & r_prev_le = value_list.at(--revidx);
											if(sstreq(r_prev_le.Id, r_le.Id)) {
												if(temp_buf.HasPrefix(r_prev_le.Lang)) {
													const size_t prev_lang_len = sstrlen(r_prev_le.Lang);
													if(prev_lang_len && temp_buf.C(prev_lang_len) == '_' && sstreq(r_prev_le.Text, r_le.Text))
														do_skip_lang_entry = 1;
												}
											}
											else
												break;
										} while(revidx && !do_skip_lang_entry);
									}
								}
								if(!do_skip_lang_entry) {
									uint64 final_id = (static_cast<uint64>(r_entry.Id) << 32) | static_cast<uint32>(id_item.Id);
									temp_buf.Z().CatHex(final_id).Space().Cat(r_le.Lang).Space().CatQStr(r_le.Text);
									f_out.WriteLine(temp_buf.CR());
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
