// SARTR.CPP
// Copyright (c) A.Sobolev 2011, 2012, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//#include <db.h>
#include <sartre.h>
#include <locale.h>
#include <BerkeleyDB.h>
//
//
/*
// @stub
int CallbackCompress(long a, long b, const char * c, int stop)
{
	return 1;
}
*/
//
//
//
SrSList::SrSList(int type)
{
	Type = type;
	Len = 0;
	SBaseBuffer::Init();
	Alloc(32);
	ASSIGN_PTR(P_Buf, 0);
}

SrSList::~SrSList()
{
	SBaseBuffer::Destroy();
}

int SrSList::GetType() const
{
	return Type;
}

size_t SrSList::GetLength() const
{
	return Len;
}

int FASTCALL SrSList::Copy(const SrSList & rS)
{
	SBaseBuffer::Copy(rS);
	Type = rS.Type;
	Len = rS.Len;
	return 1;
}

//virtual
void SrSList::Clear()
{
	Len = 0;
	ASSIGN_PTR(P_Buf, 0);
}

//virtual
int SrSList::IsEqual(const SrSList & rS) const
{
	if(Type != rS.Type || Len != rS.Len)
		return 0;
	else if(Len && memcmp(P_Buf, rS.P_Buf, Len) != 0)
		return 0;
	else
		return 1;
}

//virtual
int SrSList::Validate() const
{
	return 1;
}

int SrSList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir < 0)
		Clear();
	THROW_SL(pCtx->Serialize(dir, Type, rBuf));
	THROW_SL(pCtx->Serialize(dir, Len, rBuf));
	if(dir < 0) {
		THROW_SL(Alloc(Len));
	}
	THROW_SL(pCtx->SerializeBlock(dir, Len, P_Buf, rBuf, 0));
	CATCHZOK
	return ok;
}
//
//
//
SrWordForm::SrWordForm() : SrSList(SRGRAMTYP_WORDFORM)
{
}

SrWordForm & FASTCALL SrWordForm::operator = (const SrWordForm & rS)
{
	return Copy(rS);
}

SrWordForm & FASTCALL SrWordForm::Copy(const SrWordForm & rS)
{
	SrSList::Copy(rS);
	return *this;
}

SrWordForm & SrWordForm::Merge(const SrWordForm & rBase, const SrWordForm & rVar, int mode)
{
	SrSList::Copy(rBase);
	for(size_t p = 0; rVar.P_Buf[p]; p = rVar.Step(p))
		SetTag(rVar.Tag(p), rVar.Get(p), mode);
	Normalize();
	return *this;
}

int SrWordForm::Normalize()
{
	int    ok = 1;
	if(P_Buf) {
		LAssocArray list;
		for(size_t p = 0; P_Buf[p]; p = Step(p)) {
			list.Add(Tag(p), Get(p), 0);
		}
		list.Sort();
		Clear();
		for(uint i = 0; i < list.getCount(); i++) {
			SetTag(list.at(i).Key, list.at(i).Val);
		}
	}
	return ok;
}

int FASTCALL SrWordForm::IsEqual(const SrWordForm & rS) const
{
	int    r = 0;
	if(Len == rS.Len) {
		LAssocArray list1, list2;
		size_t p;
		for(p = 0; P_Buf[p]; p = Step(p)) {
			list1.Add(Tag(p), Get(p), 0);
		}
		list1.Sort();

		for(p = 0; rS.P_Buf[p]; p = rS.Step(p)) {
			list2.Add(rS.Tag(p), rS.Get(p), 0);
		}
		list2.Sort();
		r = BIN(list1 == list2);
	}
	return r;
}

double FASTCALL SrWordForm::MatchScore(const SrWordForm & rS) const
{
	double score = 0.0;
	for(size_t p = 0; P_Buf[p]; p = Step(p)) {
		const int tag = Tag(p);
		const int s_val = rS.GetTag(tag);
		if(s_val) {
			const int this_val = Get(p);
			if(s_val == this_val)
				score += 1.0;
			else
				score += 0.01;
		}
		else
			score += 0.0;
	}
	return score;
}

int FASTCALL SrWordForm::RemoveTag(int tag)
{
	int    ok = -1;
	if(P_Buf) {
		for(size_t p = 0; P_Buf[p];) {
			size_t next = Step(p);
			if(Tag(p) == tag) {
				memmove(P_Buf+p, P_Buf+next, Len-next);
				Len -= (next - p);
				ok = 1;
			}
			else
				p = next;
		}
	}
	CalcLength(); // @debug
	return ok;
}

int SrWordForm::SetTag(int tag, int val, int mode)
{
	int    ok = -1;
	int    found = 0;
	size_t p = 0;
	if(P_Buf) {
		for(; P_Buf[p]; p = Step(p)) {
			if(Tag(p) == tag) {
				if(oneof2(mode, 0, 1)) {
					Set(p, val);
					ok = 1;
				}
				found = 1;
			}
		}
	}
	if(!found && oneof2(mode, 0, 2)) {
		size_t new_size = p + (oneof2(tag, SRWG_LANGUAGE, SRWG_CLASS) ? 3 : 2) + 1;
		Alloc(new_size);
		PTR8(P_Buf+p)[0] = (uint8)tag;
		p += Set(p, val);
		PTR8(P_Buf+p)[0] = 0;
		Len = new_size;
		ok = 2;
	}
	CalcLength(); // @debug
	return ok;
}

int FASTCALL SrWordForm::GetTag(int tag) const
{
	int    ret = 0;
	if(P_Buf) {
		for(size_t p = 0; !ret && P_Buf[p]; p = Step(p)) {
			if(Tag(p) == tag)
				ret = Get(p);
		}
	}
	return ret;
}

size_t SrWordForm::CalcLength() const
{
	size_t p = 0;
	if(P_Buf) {
		while(P_Buf[p])
			p = Step(p);
		p++;
	}
	assert(p == Len);
	return p;
}

void SrWordForm::CatTok(SString & rBuf, const char * pTok) const
{
	rBuf.CatDivIfNotEmpty(0, 1).Cat(pTok);
}

size_t SrWordForm::Set(size_t pos, int val)
{
	size_t inc = sizeof(uint8);
	if(oneof2(Tag(pos), SRWG_LANGUAGE, SRWG_CLASS)) {
		PTR16(P_Buf+pos+1)[0] = (uint16)val;
		inc += sizeof(uint16);
	}
	else {
		PTR8(P_Buf+pos+1)[0] = (uint8)val;
		inc += sizeof(uint8);
	}
	return inc;
}

int FASTCALL SrWordForm::Tag(size_t pos) const
{
	return (int)(PTR8(P_Buf+pos)[0]);
}

int FASTCALL SrWordForm::Get(size_t pos) const
{
	return oneof2(Tag(pos), SRWG_LANGUAGE, SRWG_CLASS) ? (int)PTR16(P_Buf+pos+1)[0] : (int)PTR8(P_Buf+pos+1)[0];
}

size_t FASTCALL SrWordForm::Step(size_t pos) const
{
	return pos + sizeof(uint8) + (oneof2(Tag(pos), SRWG_LANGUAGE, SRWG_CLASS) ? sizeof(uint16) : sizeof(uint8));
}

struct SrWfToken {
	int16  Tag;
	int16  Val;
	const char * Tok;
};

static SrWfToken __Tokens[] = {
	{ SRWG_LANGUAGE, slangMeta,          "META" },
	{ SRWG_LANGUAGE, slangLA,            "LAT"  },
	{ SRWG_LANGUAGE, slangEN,            "ENG"  },
	{ SRWG_LANGUAGE, slangRU,            "RUS"  },
	{ SRWG_LANGUAGE, slangDE,            "GER"  },

	{ SRWG_CLASS,  SRWC_ALPHA,           "ALPHA" },
	{ SRWG_CLASS,  SRWC_DIGIT,           "DIGIT" },
	{ SRWG_CLASS,  SRWC_NOUN,            "NOUN" },
	{ SRWG_CLASS,  SRWC_NUMERAL,         "NUM" },
	{ SRWG_CLASS,  SRWC_NUMERALORD,      "ORD" },
	{ SRWG_CLASS,  SRWC_ADJECTIVE,       "ADJ" },
	{ SRWG_CLASS,  SRWC_VERB,            "VERB" },
	// @v9.2.0 { SRWG_CLASS,  SRWC_VERBMODAL,       "VERBMODAL" },
	{ SRWG_CLASS,  SRWC_PRONOUN,         "PRONOUN" },
	{ SRWG_CLASS,  SRWC_PRONOUNPOSS,     "PRONOUNPOSS" },
	{ SRWG_CLASS,  SRWC_PRAEDICPRO,      "PRAEDICPRO"  },
	{ SRWG_CLASS,  SRWC_PRAEDIC,         "PRAEDIC" },
	{ SRWG_CLASS,  SRWC_ADVERB,          "ADVERB" },
	{ SRWG_CLASS,  SRWC_PREPOSITION,     "PREPOS" },
	{ SRWG_CLASS,  SRWC_POSTPOSITION,    "POSTPOS" },
	{ SRWG_CLASS,  SRWC_CONJUNCTION,     "CONJ" },
	{ SRWG_CLASS,  SRWC_INTERJECTION,    "INTERJ" },
	{ SRWG_CLASS,  SRWC_PARENTH,         "PARENTH" },
	{ SRWG_CLASS,  SRWC_ARTICLE,         "ART" },
	{ SRWG_CLASS,  SRWC_PARTICLE,        "PART" },
	{ SRWG_CLASS,  SRWC_PARTICIPLE,         "PARTCP" },
	{ SRWG_CLASS,  SRWC_GERUND,             "GERUND" },
	{ SRWG_CLASS,  SRWC_GPARTICIPLE,        "GPARTCP" },
	{ SRWG_CLASS,  SRWC_PHRAS,              "PHRAS" },
	{ SRWG_CLASS,  SRWC_PREFIX,             "PREFIX" },
	{ SRWG_CLASS,  SRWC_AFFIX,              "AFFIX" },
	{ SRWG_CLASS,  SRWC_SKELETON,           "SKEL" },
	// { SRWG_CLASS,  SRWC_PRONOUNREFL,        "PRONOUNREFL" }, // @v9.2.0 Возвратное местоимение (reflexive pronoun)
	{ SRWG_CLASS,  SRWC_POSSESSIVEGROUP,    "POSSGRP" }, // @v9.2.0 possessive group (english)

	{ SRWG_PROPERNAME, SRPROPN_PERSONNAME,  "PERSN" },
	{ SRWG_PROPERNAME, SRPROPN_FAMILYNAME,  "FAMN"  },
	{ SRWG_PROPERNAME, SRPROPN_PATRONYMIC,  "PATRN" },
	{ SRWG_PROPERNAME, SRPROPN_ZOONAME,     "ZOON"  },
	{ SRWG_PROPERNAME, SRPROPN_ORG,         "ORG"   },
	{ SRWG_PROPERNAME, SRPROPN_GEO,         "GEO"   }, // @v9.2.0

	{ SRWG_ABBR,       SRABBR_ABBR,         "ABBR"  },
	{ SRWG_ABBR,       SRABBR_NARROW,       "NARR"  },

	{ SRWG_USAGE,   SRWU_LITERARY,          "LIT" },
	{ SRWG_USAGE,   SRWU_PRO,               "PRO" },
	{ SRWG_USAGE,   SRWU_ARCHAIC,           "ARC" },
	{ SRWG_USAGE,   SRWU_SPOKEN,            "SPK" },
	{ SRWG_USAGE,   SRWU_VULGAR,            "VUL" },

	{ SRWG_GENDER,  SRGENDER_MASCULINE,     "MASC" },
	{ SRWG_GENDER,  SRGENDER_FEMININE,      "FEM"  },
	{ SRWG_GENDER,  SRGENDER_NEUTER,        "NEU"  },
	{ SRWG_GENDER,  SRGENDER_COMMON,        "GCOM" },

	{ SRWG_TANTUM,  SRTANTUM_SINGULAR,      "SINGT" },
	{ SRWG_TANTUM,  SRTANTUM_PLURAL,        "PLURT" },

	{ SRWG_COUNT,   SRCNT_SINGULAR,         "SING" },
	{ SRWG_COUNT,   SRCNT_PLURAL,           "PLUR" },

	{ SRWG_CASE,    SRCASE_NOMINATIVE,      "NOM"  },
	{ SRWG_CASE,    SRCASE_GENITIVE,        "GEN"  },
	{ SRWG_CASE,    SRCASE_GENITIVE2,       "GEN2" },
	{ SRWG_CASE,    SRCASE_DATIVE,          "DAT"  },
	{ SRWG_CASE,    SRCASE_DATIVE2,         "DAT2" },
	{ SRWG_CASE,    SRCASE_ACCUSATIVE,      "ACC"  },
	{ SRWG_CASE,    SRCASE_ACCUSATIVE2,     "ACC2" },
	{ SRWG_CASE,    SRCASE_INSTRUMENT,      "INS"  },
	{ SRWG_CASE,    SRCASE_PREPOSITIONAL,   "PREP" },
	{ SRWG_CASE,    SRCASE_PREPOSITIONAL2,  "PREP2" },
	{ SRWG_CASE,    SRCASE_VOCATIVE,        "VOC"   },
	{ SRWG_CASE,    SRCASE_ADNUM,           "ADNUM" },
	{ SRWG_CASE,    SRCASE_OBJECTIVE,       "OBJCTV" }, // @v9.2.0 Объектный падеж (в английском)

	{ SRWG_TENSE,   SRTENSE_PRESENT,        "PRES" },
	{ SRWG_TENSE,   SRTENSE_PAST,           "PAST" },
	{ SRWG_TENSE,   SRTENSE_FUTURE,         "FUTU" },
	{ SRWG_TENSE,   SRTENSE_PASTPARTICIPLE, "PASTPART" }, // @v9.2.0 Прошедшее время дополнительная форма (English: Past Participle)

	{ SRWG_PERSON,  SRPERSON_FIRST,         "P1" },
	{ SRWG_PERSON,  SRPERSON_SECOND,        "P2" },
	{ SRWG_PERSON,  SRPERSON_THIRD,         "P3" },

	{ SRWG_ANIMATE, SRANIM_ANIMATE,         "ANIM"   },
	{ SRWG_ANIMATE, SRANIM_INANIMATE,       "INANIM" },

	{ SRWG_VALENCY, SRVALENCY_AVALENT,      "AVALENT" },
	{ SRWG_VALENCY, SRVALENCY_INTRANSITIVE, "INTRANS" },
	{ SRWG_VALENCY, SRVALENCY_TRANSITIVE,   "TRANS"   },
	{ SRWG_VALENCY, SRVALENCY_DITRANSITIVE, "DITRANS" },

	{ SRWG_ASPECT,  SRASPECT_INFINITIVE,    "INF"     },
	{ SRWG_ASPECT,  SRASPECT_PERFECTIVE,    "PERFV"   },
	{ SRWG_ASPECT,  SRASPECT_IMPERFECTIVE,  "IMPERFV" },
	{ SRWG_ASPECT,  SRASPECT_HABITUAL,      "HABIT"   },
	{ SRWG_ASPECT,  SRASPECT_CONTINUOUS,    "CONTS"   },
	{ SRWG_ASPECT,  SRASPECT_STATIVE,       "CSTATV"  },
	{ SRWG_ASPECT,  SRASPECT_PROGRESSIVE,   "CPROGV"  },
	{ SRWG_ASPECT,  SRASPECT_PERFECT,       "PERF"    },

	{ SRWG_MOOD,    SRMOOD_INDICATIVE,      "INDCTV" },
	{ SRWG_MOOD,    SRMOOD_SUBJUNCTIVE,     "SUBJUNCTV" },
	{ SRWG_MOOD,    SRMOOD_CONJUNCTIVE,     "CONJUNCTV" },
	{ SRWG_MOOD,    SRMOOD_OPTATIVE,        "OPTV" },
	{ SRWG_MOOD,    SRMOOD_JUSSIVE,         "JUSSIV" },
	{ SRWG_MOOD,    SRMOOD_POTENTIAL,       "POTENT" },
	{ SRWG_MOOD,    SRMOOD_PROHIBITIVE,     "PROHIBV" },
	{ SRWG_MOOD,    SRMOOD_IMPERATIVE,      "IMPERV" },
	{ SRWG_MOOD,    SRMOOD_INTERROGATIVE,   "INTERRV" },

	{ SRWG_VOICE,   SRVOICE_ACTIVE,         "ACT" },
	{ SRWG_VOICE,   SRVOICE_PASSIVE,        "PASS" },

	{ SRWG_ADJCAT,  SRADJCAT_QUALIT,        "ADJQUAL" },
	{ SRWG_ADJCAT,  SRADJCAT_RELATIVE,      "ADJREL"  },
	{ SRWG_ADJCAT,  SRADJCAT_POSSESSIVE,    "ADJPOSS" },
	{ SRWG_ADJCAT,  SRADJCAT_NATION,        "ADJNAT"  }, // @v9.1.12

	{ SRWG_ADJCMP,  SRADJCMP_COMPARATIVE,   "COMP" },
	{ SRWG_ADJCMP,  SRADJCMP_SUPERLATIVE,   "SUPR" },
	{ SRWG_ADJCMP,  SRADJCMP_COMPARATIVE2,  "COMP2" },

	{ SRWG_SHORT,   SRSHORT_BREV,           "BREV" },
	{ SRWG_SHORT,   SRSHORT_PLEN,           "PLEN" },

	{ SRWG_INVARIABLE,  1,                  "INVAR" },
	{ SRWG_LOCAT,       1,                  "LOC"   },
	{ SRWG_ERROR,       1,                  "ERR"   },
	{ SRWG_TOBE,        1,                  "TOBE"  }, // @v9.2.0
	{ SRWG_QUEST,       1,                  "QUEST" }, // @v9.2.0 (Не знаю в какую категорию занести) Вопросительная форма слова
	{ SRWG_MODAL,       1,                  "MODAL" }, // @v9.2.0 (Не знаю в какую категорию занести) Модальный глагол

	{ SRWG_POSSESSIVE,  1,                  "POSSTAG"  }, // @v9.2.0
	{ SRWG_PREDICATIVE, 1,                  "PREDICAT" }, // @v9.2.0

	{ SRWG_PRONOUN, SRPRON_REFL,            "PNREFL" }, // @v9.2.0 Возвратное местоимение (reflexive pronoun) [self]
	{ SRWG_PRONOUN, SRPRON_DEMONSTR,        "PNDEM"  }, // @v9.2.0 Указательное местоимение (pronomina demonstrativa) [one]
	{ SRWG_PRONOUN, SRPRON_PERSONAL,        "PNPERS" }, // @v9.2.0 Персональное местоимение

	{ SRWG_COUNTAB, SRCTB_UNCOUNTABLE,      "NUNCNT" }, // @v9.2.0 неисчисляемое существительное;
		// (Неисчисляемое существительное обозначает объекты, которые обычно нельзя пересчитать. Неисчисляемые существительные
		// не имеют формы множественного числа. Им не нужны неопределенные артикли, например ...an area of outstanding natural beauty.)
	{ SRWG_COUNTAB, SRCTB_MASS,             "NMASS"  }, // @v9.2.0 mass-существительные соединяют в себе поведение исчисляемых и неисчисляемых
		// существительных. Они используются как неисчисляемые, чтобы обозначить субстанцию, а как исчисляемые - чтобы обозначить марку или тип:
		// Rinse in cold water to remove any remaining detergent...Wash it in hot water with a good detergent ... We used several different
		// detergents in our stain-removal tests. Other examples of mass nouns are: shampoo, butter, bleach. pl - множественное число;
	{ SRWG_ADVERBCAT, SRADVCAT_INTERROGATIVE, "ADVINTR" },
	{ SRWG_ADVERBCAT, SRADVCAT_RELATIVE,      "ADVREL"  },
	{ SRWG_ADVERBCAT, SRADVCAT_POINTING,      "ADVPNT"  }

};

int FASTCALL SrWordForm::ToStr(SString & rBuf) const
{
	rBuf = 0;
	if(P_Buf) {
		for(size_t p = 0; P_Buf[p] != 0; p = Step(p)) {
			const int tag = Tag(p);
			const int val = Get(p);
			if(val) {
				for(uint i = 0; i < SIZEOFARRAY(__Tokens); i++) {
					if(__Tokens[i].Tag == tag && __Tokens[i].Val == val)
						CatTok(rBuf, __Tokens[i].Tok);
				}
			}
		}
	}
	return 1;
}

int FASTCALL SrWordForm::FromStr(const char * pStr)
{
	int    ok = 1;
	Clear();
	if(pStr) {
		SString temp_buf;
		SStrScan scan(pStr);
		while(scan.Skip().GetIdent(temp_buf)) {
			temp_buf.ToUpper();
			for(uint i = 0; i < SIZEOFARRAY(__Tokens); i++) {
				if(temp_buf == __Tokens[i].Tok) {
					SetTag(__Tokens[i].Tag, __Tokens[i].Val);
				}
			}
		}
	}
	return ok;
}
//
//
//
SrFlexiaModel::SrFlexiaModel() : SrSList(SRGRAMTYP_FLEXIAMODEL)
{
}

int SrFlexiaModel::Normalize()
{
	int    ok = 1;
	Item item;
	SArray list(sizeof(Item));
	{
		for(size_t i = 0; GetNext(&i, item) > 0;) {
			THROW(list.insert(&item));
		}
	}
	{
		list.sort(PTR_CMPFUNC(_2long));
		Clear();
		for(uint i = 0; i < list.getCount(); i++) {
			THROW(Add(*(Item *)list.at(i)));
		}
	}
	CATCHZOK
	return ok;
}

int FASTCALL SrFlexiaModel::IsEqual(const SrFlexiaModel & rS) const
{
	int    ok = 0;
	Item item;
	SArray list1(sizeof(Item)), list2(sizeof(Item));
	{
		for(size_t i = 0; GetNext(&i, item) > 0;) {
			THROW(list1.insert(&item));
		}
	}
	{
		for(size_t i = 0; rS.GetNext(&i, item) > 0;) {
			THROW(list2.insert(&item));
		}
	}
	if(list1.IsEqual(list2))
		ok = 1;
	CATCHZOK
	return ok;

}

int SrFlexiaModel::Search(LEXID afxID, LEXID pfxID, LongArray & rWordFormList) const
{
	int    ok = -1;
	Item item;
	for(size_t p = 0; GetNext(&p, item) > 0;) {
		if(item.AffixID == afxID && item.PrefixID == pfxID) {
			rWordFormList.addUnique(item.WordFormID);
			ok = 1;
		}
	}
	return ok;
}

int SrFlexiaModel::GetNext(size_t * pPos, SrFlexiaModel::Item & rItem) const
{
	int    ok = 1;
	MEMSZERO(rItem);
	if(pPos) {
		size_t p = *pPos;
		assert(p <= Len);
		if(p == Len) {
			ok = -1;
		}
		else {
			uint8  ind = *PTR8(P_Buf+p);
			p += sizeof(uint8);
			if(ind & fmiWf16) {
				rItem.WordFormID = (int32)PTR16(P_Buf+p)[0];
				p += sizeof(int16);
			}
			else {
				rItem.WordFormID = (int32)PTR32(P_Buf+p)[0];
				p += sizeof(int32);
			}
			if(!(ind & fmiZeroAffix)) {
				rItem.AffixID = (int32)PTR32(P_Buf+p)[0];
				p += sizeof(int32);
			}
			if(!(ind & fmiZeroPrefix)) {
				rItem.PrefixID = (int32)PTR32(P_Buf+p)[0];
				p += sizeof(int32);
			}
			*pPos = p;
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SrFlexiaModel::Add(const SrFlexiaModel::Item & rItem)
{
	int    ok = 1;
	size_t sz = sizeof(SrFlexiaModel::Item);
	uint8  ind = 0;
	if(rItem.WordFormID < 0x7fff) {
		ind |= fmiWf16;
		sz -= sizeof(int16);
	}
	if(rItem.AffixID == 0) {
		ind |= fmiZeroAffix;
		sz -= sizeof(rItem.AffixID);
	}
	if(rItem.PrefixID == 0) {
		ind |= fmiZeroPrefix;
		sz -= sizeof(rItem.PrefixID);
	}
	sz += sizeof(uint8);
	THROW(Alloc(Len+sz));
	{
		size_t p = Len;
		*PTR8(P_Buf+p) = ind;
		p += sizeof(uint8);
		if(ind & fmiWf16) {
			*PTR16(P_Buf+p) = (uint16)rItem.WordFormID;
			p += sizeof(uint16);
		}
		else {
			*PTR32(P_Buf+p) = (uint32)rItem.WordFormID;
			p += sizeof(uint32);
		}
		if(!(ind & fmiZeroAffix)) {
			*PTR32(P_Buf+p) = (uint32)rItem.AffixID;
			p += sizeof(uint32);
		}
		if(!(ind & fmiZeroPrefix)) {
			*PTR32(P_Buf+p) = (uint32)rItem.PrefixID;
			p += sizeof(uint32);
		}
		assert((p-Len) == sz);
		Len = p;
	}
	CATCHZOK
	return ok;
}
//
//
//
SrGrammarTbl::SrGrammarTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("words.db->gramm", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer temp_buf;
			rData.Get(temp_buf);
			rResult = temp_buf;
			return 0;
		}
	};

	SeqID = 0;
	new BDbTable(BDbTable::Config("words.db->gramm_idx01", BDbTable::idxtypHash, 0, 0, 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(SeqID = P_Db->CreateSequence("seq_gramm_id", 0));
	CATCH
		Close();
	ENDCATCH
}

SrGrammarTbl::~SrGrammarTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrGrammarTbl::Helper_Add(SrSList * pL, long * pID)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	int32  id = 0;
	int64  __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	id = (int32)__id;
	key_buf = id;
	{
		SBuffer l_buf;
		SSerializeContext * p_sctx = GetSCtx();
		THROW_DB(p_sctx);
		THROW(pL->Serialize(+1, l_buf, p_sctx));
		data_buf = l_buf;
	}
	THROW_DB(InsertRec(key_buf, data_buf));
	ASSIGN_PTR(pID, id);
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Helper_Search(SrSList * pL, long * pID)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	{
		SBuffer l_buf;
		SSerializeContext * p_sctx = GetSCtx();
		THROW_DB(p_sctx);
		THROW(pL->Serialize(+1, l_buf, p_sctx));
		key_buf = l_buf;
	}
	data_buf.Alloc(1024);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		key_buf.Get(pID);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Helper_Search(long id, SrSList * pL)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = id;
	data_buf.Alloc(1024);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pL) {
			SBuffer wf_buf;
			SSerializeContext * p_sctx = GetSCtx();
			THROW_DB(p_sctx);
			data_buf.Get(wf_buf);
			THROW(pL->Serialize(-1, wf_buf, p_sctx));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrGrammarTbl::Add(SrWordForm * pWf, long * pID)
{
	return Helper_Add(pWf, pID);
}

int SrGrammarTbl::Search(long id, SrWordForm * pWf)
{
	int    ok = Helper_Search(id, pWf);
	if(ok > 0) {
		assert(pWf->GetType() == SRGRAMTYP_WORDFORM);
	}
	return ok;
}

int SrGrammarTbl::Add(SrFlexiaModel * pFm, long * pID)
{
	return Helper_Add(pFm, pID);
}

int SrGrammarTbl::Search(long id, SrFlexiaModel * pFm)
{
	int    ok = Helper_Search(id, pFm);
	if(ok > 0) {
		assert(pFm->GetType() == SRGRAMTYP_FLEXIAMODEL);
	}
	return ok;
}

int SrGrammarTbl::Search(SrWordForm * pWf, long * pID)
{
	int    ok = Helper_Search(pWf, pID);
	if(ok > 0) {
		assert(pWf->GetType() == SRGRAMTYP_WORDFORM);
	}
	return ok;
}

int SrGrammarTbl::Search(SrFlexiaModel * pFm, long * pID)
{
	int    ok = Helper_Search(pFm, pID);
	if(ok > 0) {
		assert(pFm->GetType() == SRGRAMTYP_FLEXIAMODEL);
	}
	return ok;
}
//
// {id;word-id;grammar-rule-id}
//
class SrWordGrammarTbl : public BDbTable {
public:
	SrWordGrammarTbl(BDbDatabase * pDb);
};

SrWordTbl::SrWordTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("words.db->word", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			LEXID  id = 0;
			rData.Get(&id, sizeof(id));
			rResult.Set(&id, sizeof(id));
			return 0;
		}
	};

	SeqID = 0;
	new BDbTable(BDbTable::Config("words.db->word_idx01", BDbTable::idxtypHash, 0, 0, 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(SeqID = P_Db->CreateSequence("seq_word_id", 0));
	CATCH
		Close();
	ENDCATCH
}

SrWordTbl::~SrWordTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrWordTbl::Add(const char * pWordUtf8, LEXID * pID)
{
	int    ok = 1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = pWordUtf8;
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(&id, sizeof(id));
		ok = -1;
	}
	else {
		int64 __id = 0;
		THROW_DB(P_Db->GetSequence(SeqID, &__id));
		id = (LEXID)__id;
		data_buf.Set(&id, sizeof(id));
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	CATCH
		id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordTbl::MakeSpecial(int spcTag, const char * pWordUtf8, SString & rBuf)
{
	int    ok = 1;
	rBuf = 0;
	switch(spcTag) {
		case spcEmpty: rBuf.Cat("/#"); break;
		case spcPrefix: rBuf.Cat(pWordUtf8).Cat("/-"); break;
		case spcAffix: rBuf.Cat("/-").Cat(pWordUtf8); break;
		case spcConcept: rBuf.Cat("/:").Cat(pWordUtf8); break;
		case spcCPropSymb: rBuf.Cat("/.").Cat(pWordUtf8); break;
		default: 
			ok = PPSetError(PPERR_SR_INVSPCWORDTAG, (long)spcTag);
			break;
	}
	return ok;
}

int SrWordTbl::AddSpecial(int spcTag, const char * pWordUtf8, LEXID * pID)
{
	int    ok = 0;
	SString temp_buf;
	if(MakeSpecial(spcTag, pWordUtf8, temp_buf))
		ok = Add(temp_buf, pID);
	return ok;
}

int SrWordTbl::SearchSpecial(int spcTag, const char * pWordUtf8, LEXID * pID)
{
	int    ok = 0;
	SString temp_buf;
	if(MakeSpecial(spcTag, pWordUtf8, temp_buf))
		ok = Search(temp_buf, pID);
	else {
		ASSIGN_PTR(pID, 0);
	}
	return ok;
}

int SrWordTbl::Search(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = pWordUtf8;
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(&id, sizeof(id));
		ok = 1;
	}
	else {
		SString msg_buf;
		PPSetError(PPERR_SR_WORDNFOUND, (msg_buf = pWordUtf8).Transf(CTRANSF_UTF8_TO_INNER));
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordTbl::Search(LEXID id, SString & rBuf)
{
	int    ok = -1;
	rBuf = 0;
	BDbTable::Buffer key_buf, data_buf;
	key_buf.Set(&id, sizeof(id));
	data_buf.Alloc(32);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		key_buf.Get(rBuf);
		ok = 1;
	}
	else
		PPSetErrorDB();
	return ok;
}
//
//
//
SrNGram::SrNGram()
{
	ID = 0;
	Ver = 0;
}
//
//
//
SrNGramTbl::SrNGramTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("words.db->ngram", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrNGram ng_rec;
			SBuffer rec_buf;
			rData.Get(rec_buf);
			((SrNGramTbl *)P_MainT)->SerializeRecBuf(-1, &ng_rec, rec_buf);
			const LongArray & r_list = ng_rec.WordIdList;
			rResult.Set(r_list.dataPtr(), r_list.getCount() * r_list.getItemSize());
			return 0;
		}
	};
	SeqID = 0;
	new BDbTable(BDbTable::Config("words.db->ngram_idx01", BDbTable::idxtypBTree, 0, 0, 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(SeqID = P_Db->CreateSequence("seq_ngram_id", 0));
	CATCH
		Close();
	ENDCATCH
}

SrNGramTbl::~SrNGramTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrNGramTbl::SerializeRecBuf(int dir, SrNGram * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	//THROW(p_sctx->Serialize(dir, pRec->ID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Ver, rBuf));
	THROW_SL(p_sctx->Serialize(dir, &pRec->WordIdList, rBuf));
	CATCHZOK
	return ok;
}

int SrNGramTbl::Add(SrNGram & rRec)
{
	int    ok = 1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	int64 __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	rRec.ID = __id;
	key_buf = __id;
	THROW(SerializeRecBuf(+1, &rRec, rec_buf));
	data_buf = rec_buf;
	THROW_DB(InsertRec(key_buf, data_buf));
	CATCH
		rRec.ID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SrNGramTbl::Search(NGID id, SrNGram * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = id;
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrNGramTbl::Search(const SrNGram & rKey, NGID * pID)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	{
		const LongArray & r_list = rKey.WordIdList;
		key_buf.Set(r_list.dataPtr(), r_list.getCount() * r_list.getItemSize());
	}
	data_buf.Alloc(512);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		key_buf.Get(pID);
		ok = 1;
	}
	return ok;
}

int SrNGramTbl::SearchByPrefix(const SrNGram & rKey, TSArray <NGID> & rList)
{
	int    ok = -1;
	const  LongArray & r_key_list = rKey.WordIdList;
	const  uint key_count = r_key_list.getCount();
	if(key_count) {
		SrNGram rec;
		SBuffer rec_buf;
		BDbCursor curs(*this, 1);
		BDbTable::Buffer key_buf, data_buf;
		key_buf.Set(r_key_list.dataPtr(), key_count * r_key_list.getItemSize());
		if(curs.Search(key_buf, data_buf, spGe)) {
			do {
				data_buf.Get(rec_buf);
				THROW(SerializeRecBuf(+1, &rec, rec_buf));
				if(rec.WordIdList.getCount() >= key_count) {
					int    eq_prefix = 1;
					for(uint i = 0; eq_prefix && i < key_count; i++) {
						if(rec.WordIdList.get(i) != r_key_list.get(i))
							eq_prefix = 0;
					}
					if(eq_prefix) {
						NGID id;
						key_buf.Get(&id);
						rList.insert(&id);
						ok = 1;
					}
					else
						break;
				}
				else
					break;
			} while(curs.Search(key_buf, data_buf, spNext));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SrCPropDecl::SrCPropDecl()
{
	PropID = 0;
	SymbID = 0;
	Tail.Init();
}

SrCPropDecl::~SrCPropDecl()
{
	Tail.Destroy();
}

int FASTCALL SrCPropDecl::IsEqual(const SrCPropDecl & rS) const
{
	if(PropID != rS.PropID)
		return 0;
	else if(SymbID != rS.SymbID)
		return 0;
	else if(!Tail.IsEqual(rS.Tail))
		return 0;
	else
		return 1;
}
//
//
//
SrCPropDeclList::SrCPropDeclList()
{
	Pool.Init();
	Pool.Alloc(32);
	PoolP = 4;
}

SrCPropDeclList::~SrCPropDeclList()
{
	Pool.Destroy();
}

SrCPropDeclList::SrCPropDeclList(const SrCPropDeclList & rS)
{
	Pool.Init();
	Copy(rS);
}

SrCPropDeclList & FASTCALL SrCPropDeclList::operator = (const SrCPropDeclList & rS)
{
	return Copy(rS);
}

SrCPropDeclList & FASTCALL SrCPropDeclList::Copy(const SrCPropDeclList & rS)
{
	D = rS.D;
	PoolP = rS.PoolP;
	Pool.Copy(rS.Pool);
	return *this;
}

SrCPropDeclList & SrCPropDeclList::Clear()
{
	D.clear();
	PoolP = 4;
	return *this;
}

int FASTCALL SrCPropDeclList::Add(const SrCPropDecl & rP)
{
	int ok = 1;
	Item item;
	MEMSZERO(item);
	item.PropID = rP.PropID;
	item.SymbID = rP.SymbID;
	const size_t added_size = rP.Tail.Size;
	if(added_size) {
		Pool.Alloc(ALIGNSIZE(PoolP + added_size, 4));
		memcpy(Pool.P_Buf+PoolP, rP.Tail.P_Buf, added_size);
		item.TailP = PoolP;
		item.TailS = added_size;
		PoolP += added_size;
	}
	D.insert(&item);
	return ok;
}

int SrCPropDeclList::Replace(uint pos, const SrCPropDecl & rP)
{
	int    ok = 1;
	if(pos < D.getCount()) {
		D.atFree(pos);
		Item item;
		MEMSZERO(item);
		item.PropID = rP.PropID;
		item.SymbID = rP.SymbID;
		const size_t added_size = rP.Tail.Size;
		if(added_size) {
			Pool.Alloc(ALIGNSIZE(PoolP + added_size, 4));
			memcpy(Pool.P_Buf+PoolP, rP.Tail.P_Buf, added_size);
			item.TailP = PoolP;
			item.TailS = added_size;
			PoolP += added_size;
		}
		D.atInsert(pos, &item);
	}
	else
		ok = 0;
	return ok;
}

int SrCPropDeclList::Get(uint idx, SrCPropDecl & rP) const
{
	int    ok = 1;
	if(idx < D.getCount()) {
		const Item & r_item = D.at(idx);
		rP.PropID = r_item.PropID;
		rP.SymbID = r_item.SymbID;
		rP.Tail.Destroy();
		if(r_item.TailS) {
			rP.Tail.Alloc(r_item.TailS);
			memcpy(rP.Tail.P_Buf, Pool.P_Buf+r_item.TailP, r_item.TailS);
		}
	}
	else
		ok = 0;
	return ok;
}

int SrCPropDeclList::GetBySymbID(LEXID id, SrCPropDecl & rP) const
{
	int    ok = 0;
	for(uint i = 0; i < D.getCount(); i++) {
		const Item & r_item = D.at(i);
		if(r_item.SymbID == id) {
			rP.PropID = r_item.PropID;
			rP.SymbID = r_item.SymbID;
			rP.Tail.Destroy();
			if(r_item.TailS) {
				rP.Tail.Alloc(r_item.TailS);
				memcpy(rP.Tail.P_Buf, Pool.P_Buf+r_item.TailP, r_item.TailS);
			}
			ok = 1;
		}
	}
	return ok;
}

int SrCPropDeclList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, &D, rBuf));
	THROW_SL(pCtx->Serialize(dir, PoolP, rBuf));
	if(dir < 0) {
		THROW_SL(Pool.Alloc(ALIGNSIZE(PoolP, 4)));
	}
	THROW_SL(pCtx->SerializeBlock(dir, PoolP, Pool.P_Buf, rBuf, 1));
	CATCHZOK
	return ok;
}

int FASTCALL SrCPropDeclList::IsEqual(const SrCPropDeclList & rS) const
{
	if(D.getCount() != rS.D.getCount())
		return 0;
	else {
		const uint c = D.getCount();
		for(uint i = 0; i < c; i++) {
			const Item & ri = D.at(i);
			const Item & rsi = rS.D.at(i);
			if(ri.PropID != rsi.PropID)
				return 0;
			else if(ri.SymbID != rsi.PropID)
				return 0;
			else if(ri.TailS != rsi.TailS)
				return 0;
			else if(ri.TailS) {
				if(memcmp(Pool.P_Buf+ri.TailP, rS.Pool.P_Buf+rsi.TailP, ri.TailS) != 0)
					return 0;
			}
		}
		return 1;
	}
}

int FASTCALL SrCPropDeclList::Merge(const SrCPropDeclList & rS)
{
	int    ok = -1;
	const uint sc = rS.D.getCount();
	for(uint i = 0; i < sc; i++) {
		const Item & rsi = rS.D.at(i);
		SrCPropDecl temp_pd;
		rS.Get(i, temp_pd);
		int    do_add = 1;
		const uint c = D.getCount();
		for(uint j = 0; j < c; j++) {
			const Item & ri = D.at(j);
			if(ri.SymbID == rsi.SymbID) {
				if(ri.PropID == rsi.PropID) {
					//
					// Пара {Prop; Symb} эквивалентна. Заменяем текущий элемент тем,
					// который находится в rS (правило переопределения более ранней
					// декларации более поздней.
					//
					THROW(Replace(j, temp_pd));
					do_add = 0;
					ok = 1;
				}
				else {
					CALLEXCEPT();
				}
			}
		}
		if(do_add) {
			THROW(Add(temp_pd));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SrConcept::SrConcept()
{
	ID = 0;
	SymbID = 0;
	Ver = 0;
}

int FASTCALL SrConcept::IsEqual(const SrConcept & rS) const
{
	int    ok = 1;
	if(ID != rS.ID)
		ok = 0;
	else if(SymbID != rS.SymbID)
		ok = 0;
	else if(Ver != rS.Ver)
		ok = 0;
	else if(!Pdl.IsEqual(rS.Pdl))
		ok = 0;
	return ok;
}

SrConcept & SrConcept::Clear()
{
	ID = 0;
	SymbID = 0;
	Ver = 0;
	Pdl.Clear();
	return *this;
}

SrConceptTbl::SrConceptTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("concept.db->concept", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConcept rec;
			SBuffer rec_buf;
			rData.Get(rec_buf);
			((SrConceptTbl *)P_MainT)->SerializeRecBuf(-1, &rec, rec_buf);
			rResult = rec.SymbID;
			//
			// Нулевой сидентификатор символа (аноминая концепция) не индексируем
			//
			return (rec.SymbID == 0) ? DB_DONOTINDEX : 0;
		}
	};
	SeqID = 0;
	new BDbTable(BDbTable::Config("concept.db->concept_idx01", BDbTable::idxtypHash, 0, 0, 0), pDb, new Idx01, this);
	if(P_Db)
		THROW(SeqID = P_Db->CreateSequence("seq_concept_id", 0));
	CATCH
		Close();
	ENDCATCH
}

SrConceptTbl::~SrConceptTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrConceptTbl::SerializeRecBuf(int dir, SrConcept * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	//THROW(p_sctx->Serialize(dir, pRec->ID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->SymbID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pRec->Ver, rBuf));
	THROW(pRec->Pdl.Serialize(dir, rBuf, p_sctx));
	CATCHZOK
	return ok;
}

int SrConceptTbl::SearchByID(CONCEPTID id, SrConcept * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = id;
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
			key_buf.Get(&pRec->ID);
			assert(pRec->ID == id);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::SearchBySymb(LEXID symbId, SrConcept * pRec)
{
	int    ok = -1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = symbId;
	data_buf.Alloc(512);
	if(BDbTable::Search(1, key_buf, data_buf)) {
		if(pRec) {
			data_buf.Get(rec_buf);
			THROW(SerializeRecBuf(-1, pRec, rec_buf));
			key_buf.Get(&pRec->ID);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::Add(SrConcept & rRec)
{
	int    ok = 1;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	int64 __id = 0;
	THROW_DB(P_Db->GetSequence(SeqID, &__id));
	rRec.ID = __id;
	key_buf = __id;
	THROW(SerializeRecBuf(+1, &rRec, rec_buf));
	data_buf = rec_buf;
	THROW_DB(InsertRec(key_buf, data_buf));
	CATCH
		rRec.ID = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int SrConceptTbl::Update(SrConcept & rRec)
{
	int    ok = 1;
	SrConcept org_rec;
	SBuffer rec_buf;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = rRec.ID;
	data_buf.Alloc(512);
	THROW_DB(BDbTable::Search(key_buf, data_buf));
	data_buf.Get(rec_buf);
	THROW(SerializeRecBuf(-1, &org_rec, rec_buf));
	org_rec.ID = rRec.ID;
	if(rRec.IsEqual(org_rec))
		ok = -1;
	else {
		rec_buf.Clear();
		THROW(SerializeRecBuf(+1, &rRec, rec_buf));
		data_buf = rec_buf;
		THROW_DB(UpdateRec(key_buf, data_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptTbl::Remove(CONCEPTID id)
{
	int    ok = 1;
	BDbTable::Buffer key_buf;
	key_buf = id;
	THROW(DeleteRec(key_buf));
	CATCHZOK
	return ok;
}

int SrConceptTbl::SetPropDeclList(CONCEPTID id, SrCPropDeclList * pPdl)
{
	int    ok = 1;
	if(pPdl && pPdl->GetCount()) {
		SrConcept rec;
		THROW(SearchByID(id, &rec) > 0);
		rec.Pdl.Merge(*pPdl);
		THROW(Update(rec));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
//
//
SrCProp::SrCProp()
{
	CID = 0;
	PropID = 0;
}

SrCProp::SrCProp(CONCEPTID cID, CONCEPTID propID)
{
	CID = cID;
	PropID = propID;
}

int FASTCALL SrCProp::IsEqual(const SrCProp & rS) const
{
	int    ok = 1;
	if(CID != rS.CID)
		ok = 0;
	else if(PropID != rS.PropID)
		ok = 0;
	else if(!Value.IsEqual(rS.Value))
		ok = 0;
	return ok;
}

SrCProp & SrCProp::Clear()
{
	CID = 0;
	PropID = 0;
	Value.Clear();
	return *this;
}

SrCProp & FASTCALL SrCProp::operator = (int val)
{
	int64 v64 = val;
	Value.Write(v64);
	return *this;
}

SrCProp & FASTCALL SrCProp::operator = (int64 val)
{
	Value.Write(val);
	return *this;
}

SrCProp & FASTCALL SrCProp::operator = (double val)
{
	Value.Write(val);
	return *this;
}

int FASTCALL SrCProp::Get(int64 & rIntVal) const
{
	size_t s = Value.ReadStatic(&rIntVal, sizeof(rIntVal));
	return (s == sizeof(rIntVal)) ? 1 : 0;
}

int FASTCALL SrCProp::Get(double & rRealVal) const
{
	size_t s = Value.ReadStatic(&rRealVal, sizeof(rRealVal));
	return (s == sizeof(rRealVal)) ? 1 : 0;
}
//
//
//
SrCPropList::SrCPropList()
{
	uint32 zero = 0;
	D.Write(zero); // zero offset
}

SrCPropList & SrCPropList::Clear()
{
	L.clear();
	D.Clear();
	uint32 zero = 0;
	D.Write(zero); // zero offset
	return *this;
}

uint SrCPropList::GetCount() const
{
	return L.getCount();
}

int SrCPropList::Search(CONCEPTID cID, CONCEPTID propID, uint * pPos) const
{
	int    ok = 0;
	uint   pos = 0;
	const uint c = L.getCount();
	for(uint i = 0; !ok && i < c; i++) {
		const Item & r_item = L.at(i);
		if(r_item.CID == cID && r_item.PropID == propID) {
			pos = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SrCPropList::GetProp(uint pos, SrCProp & rProp) const
{
	int    ok = 1;
	rProp.Clear();
	if(pos < L.getCount()) {
		const Item & r_item = L.at(pos);
		rProp.CID = r_item.CID;
		rProp.PropID = r_item.PropID;
		if(r_item.P && r_item.S) {
			const void * p_data = D.GetBuf(r_item.P);
			if(p_data)
				rProp.Value.Write(p_data, r_item.S);
		}
	}
	else
		ok = 0;
	return ok;
}

size_t FASTCALL SrCPropList::GetDataLen(uint pos) const
{
	return (pos < L.getCount()) ? L.at(pos).S : 0;
}

const void * SrCPropList::GetDataPtr(uint pos, size_t * pDataLen) const
{
	const void * p = 0;
	size_t sz = 0;
	if(pos < L.getCount()) {
		const Item & r_item = L.at(pos);
		if(r_item.P && r_item.S) {
			sz = r_item.S;
			p = D.GetBuf(r_item.P);
		}
	}
	ASSIGN_PTR(pDataLen, sz);
	return p;
}

size_t SrCPropList::GetData(uint pos, void * pData, size_t bufLen) const
{
	size_t sz = 0;
	const void * p = GetDataPtr(pos, &sz);
	if(p && sz) {
		sz = MIN(sz, bufLen);
		memcpy(pData, p, sz);
	}
	return sz;
}

int SrCPropList::SetData(uint pos, const void * pData, size_t dataLen)
{
	int    ok = 1;
	THROW(pos < L.getCount());
	{
		Item & r_item = L.at(pos);
		if(pData && dataLen) {
			uint32 offs = D.GetWrOffs();
			THROW(D.Write(pData, dataLen));
			r_item.P = offs;
			r_item.S = dataLen;
		}
		else {
			r_item.P = 0;
			r_item.S = 0;
		}
	}
	CATCHZOK
	return ok;
}

int SrCPropList::Set(CONCEPTID cID, CONCEPTID propID, const void * pData, size_t dataLen)
{
	int    ok = 1;
	uint pos = 0;
	if(Search(cID, propID, &pos)) {
		size_t data_len;
		const void * p_data = GetDataPtr(pos, &data_len);
		if(p_data) {
			if(data_len == dataLen && memcmp(p_data, pData, data_len) == 0) {
				ok = -1;
			}
			else {
				THROW(SetData(pos, pData, dataLen));
			}
		}
	}
	else {
		Item new_item;
		new_item.CID = cID;
		new_item.PropID = propID;
		new_item.P = 0;
		new_item.S = 0;
		uint   pos = L.getCount();
		THROW(L.insert(&new_item));
		THROW(SetData(pos, pData, dataLen));
	}
	CATCHZOK
	return ok;
}

//static
int FASTCALL SrConceptPropTbl::EncodePrimeKey(BDbTable::Buffer & rKeyBuf, const SrCProp & rRec)
{
	rKeyBuf.Set(&rRec.CID, sizeof(rRec.CID)+sizeof(rRec.PropID));
	return 1;
}

//static
int FASTCALL SrConceptPropTbl::DecodePrimeKey(const BDbTable::Buffer & rKeyBuf, SrCProp & rRec)
{
	rKeyBuf.Get(&rRec.CID, sizeof(rRec.CID)+sizeof(rRec.PropID));
	return 1;
}

SrConceptPropTbl::SrConceptPropTbl(/*BDbDatabase * pDb*/SrDatabase & rSr) :
	BDbTable(BDbTable::Config("concept.db->conceptprop", BDbTable::idxtypHash, 0, 0, 0), rSr.P_Db),
	R_Sr(rSr)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrCProp rec;
			SrConceptPropTbl::DecodePrimeKey(rKey, rec);
			rResult = rec.CID;
			return 0;
		}
	};
	new BDbTable(BDbTable::Config("concept.db->conceptprop_idx01", BDbTable::idxtypHash, BDbTable::cfDup, 0, 0), rSr.P_Db, new Idx01, this);
}

int SrConceptPropTbl::Search(SrCProp & rRec)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	EncodePrimeKey(key_buf, rRec);
	data_buf.Alloc(512);
	rRec.Value.Clear();
	if(BDbTable::Search(key_buf, data_buf)) {
		SBuffer rec_buf;
		data_buf.Get(rec_buf);
		THROW(SerializeRecBuf(-1, &rRec, rec_buf));
		DecodePrimeKey(key_buf, rRec);
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::GetPropIdList(CONCEPTID cID, Int64Array & rPropIdList)
{
	rPropIdList.clear();

	int    ok = -1;
	SrCProp prop;
	BDbTable::Buffer key_buf, data_buf;
	key_buf = cID;
	data_buf.Alloc(512);
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			DecodePrimeKey(key_buf, prop);
			THROW(rPropIdList.add(prop.PropID));
			ok = 1;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::GetPropList(CONCEPTID cID, SrCPropList & rPropList)
{
	rPropList.Clear();

	int    ok = -1;
	SrCProp prop;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer rec_buf;
	key_buf = cID;
	data_buf.Alloc(512);
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			prop.Clear();
			DecodePrimeKey(key_buf, prop);
			if(prop.CID == cID) {
				data_buf.Get(rec_buf.Clear());
				SerializeRecBuf(-1, &prop, rec_buf);
				rPropList.Set(prop.CID, prop.PropID, prop.Value.GetBuf(prop.Value.GetRdOffs()), prop.Value.GetAvailableSize());
				ok = 1;
			}
			else
				break;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	return ok;
}

int SrConceptPropTbl::Set(SrCProp & rProp)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer rec_buf;
	SrCProp org_rec;
	THROW(rProp.CID);
	THROW(rProp.PropID);
	EncodePrimeKey(key_buf, rProp);
	data_buf.Alloc(512);
	if(BDbTable::Search(key_buf, data_buf)) {
		data_buf.Get(rec_buf);
		org_rec.CID = rProp.CID;
		org_rec.PropID = rProp.PropID;
		THROW(SerializeRecBuf(-1, &org_rec, rec_buf));
		org_rec.CID = rProp.CID;
		org_rec.PropID = rProp.PropID;
		if(!org_rec.IsEqual(rProp)) {
			THROW(SerializeRecBuf(+1, &rProp, rec_buf.Clear()));
			THROW_DB(UpdateRec(key_buf, data_buf = rec_buf));
		}
		else
			ok = -1;
	}
	else {
		EncodePrimeKey(key_buf, rProp);
		THROW(SerializeRecBuf(+1, &rProp, rec_buf.Clear()));
		THROW_DB(InsertRec(key_buf, data_buf = rec_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptPropTbl::SerializeRecBuf(int dir, SrCProp * pRec, SBuffer & rBuf)
{
	int    ok = 1;
	uint32 _c = 0;
	int    type = 0;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	//THROW(p_sctx->Serialize(dir, pRec->CID, rBuf));
	//THROW(p_sctx->Serialize(dir, pRec->PropID, rBuf));
	type = R_Sr.GetPropType(pRec->PropID);
	switch(type) {
		case SRPROPT_INT:
			{
				int64 val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		case SRPROPT_STRING:
			{
				SString val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		case SRPROPT_REAL:
			{
				double val;
				if(dir < 0) {
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
					pRec->Value.Write(val);
				}
				else if(dir > 0) {
					pRec->Value.Read(val);
					THROW_SL(p_sctx->Serialize(dir, val, rBuf));
				}
			}
			break;
		default:
			if(dir < 0) {
				pRec->Value = rBuf;
			}
			else if(dir > 0) {
				rBuf = pRec->Value;
			}
			break;
	}
	CATCHZOK
	return ok;
}
//
//
//
struct SrConceptNg {
	CONCEPTID CID;
	NGID   NGID;
};

SrConceptNgTbl::SrConceptNgTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("concept.db->conceptng", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConceptNg rec;
			rKey.Get(&rec, sizeof(rec));
			rResult = rec.CID;
			return 0;
		}
	};
	class Idx02 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SrConceptNg rec;
			rKey.Get(&rec, sizeof(rec));
			rResult = rec.NGID;
			return 0;
		}
	};
	new BDbTable(BDbTable::Config("concept.db->conceptng_idx01", BDbTable::idxtypHash, BDbTable::cfDup, 0, 0), pDb, new Idx01, this);
	new BDbTable(BDbTable::Config("concept.db->conceptng_idx02", BDbTable::idxtypHash, BDbTable::cfDup, 0, 0), pDb, new Idx02, this);
}

int SrConceptNgTbl::Set(CONCEPTID cID, NGID ngID)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SrConceptNg rec;
	rec.CID = cID;
	rec.NGID = ngID;
	key_buf.Set(&rec, sizeof(rec));
	data_buf.Alloc(32);
	if(BDbTable::Search(key_buf, data_buf)) {
		ok = -1;
	}
	else {
		key_buf.Set(&rec, sizeof(rec));
		data_buf.Reset();
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	CATCHZOK
	return ok;
}

int SrConceptNgTbl::GetConceptList(NGID ngID, Int64Array & rConceptList)
{
	int    ok = -1;
	rConceptList.clear();
	BDbCursor curs(*this, 2);
	BDbTable::Buffer key_buf, data_buf;
	key_buf.Alloc(32);
	data_buf.Alloc(32);
	key_buf = ngID;
	if(curs.Search(key_buf, data_buf, spEq)) do {
		SrConceptNg rec;
		key_buf.Get(&rec, sizeof(rec));
		if(rec.NGID == ngID) {
			rConceptList.add(rec.CID);
			ok = 1;
		}
		else
			break;
	} while(curs.Search(key_buf, data_buf, spNext));
	return ok;
}

int SrConceptNgTbl::GetNgList(CONCEPTID cID, Int64Array & rNgList)
{
	int    ok = -1;
	rNgList.clear();
	BDbCursor curs(*this, 1);
	BDbTable::Buffer key_buf, data_buf;
	key_buf.Alloc(32);
	data_buf.Alloc(32);
	key_buf = cID;
	if(curs.Search(key_buf, data_buf, spEq)) do {
		SrConceptNg rec;
		key_buf.Get(&rec, sizeof(rec));
		if(rec.CID == cID) {
			rNgList.add(rec.NGID);
			ok = 1;
		}
		else
			break;
	} while(curs.Search(key_buf, data_buf, spNext));
	return ok;
}
//
//
//
SrWordInfo::SrWordInfo()
{
	Clear();
}

SrWordInfo & SrWordInfo::Clear()
{
	BaseID = 0;
	PrefixID = 0;
	AffixID = 0;
	BaseFormID = 0;
	FormID = 0;
	WaID = 0;
	Score = 0.0;
	return *this;
}
//
//
//
SrDatabase::SrDatabase()
{
	PropInstance = 0;
	PropSubclass = 0;
	PropType = 0;
	P_Db = 0;
	P_WdT = 0;
	P_GrT = 0;
	P_WaT = 0;
	P_CT = 0;
	P_CpT = 0;
	P_NgT = 0;
	P_CNgT = 0;
	P_GnT = 0;
	P_GwT = 0;
	ZeroWordID = 0;
}

SrDatabase::~SrDatabase()
{
	Close();
}

int SrDatabase::Open(const char * pDbPath)
{
	int    ok = 1;
	BDbDatabase::Config cfg;
	cfg.CacheSize   = 256 * 1024 * 1024;
	cfg.CacheCount  = 1; // @v9.6.4 20-->
	cfg.MaxLockers  = 256*1024; // @v9.6.2 20000-->256*1024
	cfg.MaxLocks    = 128*1024; // @v9.6.4
	cfg.MaxLockObjs = 128*1024; // @v9.6.4
	cfg.LogBufSize  = 8*1024*1024;
	cfg.LogFileSize = 256*1024*1024;
	//cfg.LogSubDir = "LOG";
	cfg.Flags |= (cfg.fLogNoSync|cfg.fLogAutoRemove/*|cfg.fLogInMemory*/); // @v9.6.6
	Close();
	{
		SString db_path = pDbPath;
		if(db_path.Empty()) {
			PPGetPath(PPPATH_SARTREDB, db_path);
		}
		THROW_PP(db_path.NotEmpty() && pathValid(db_path, 1), PPERR_SARTREDBUNDEF);
		THROW_S(P_Db = new BDbDatabase(db_path, &cfg, BDbDatabase::oPrivate/*|BDbDatabase::oRecover*/), SLERR_NOMEM);
		THROW(!!*P_Db);

		THROW_S(P_WdT = new SrWordTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GrT = new SrGrammarTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_WaT = new SrWordAssocTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CT = new SrConceptTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CpT = new SrConceptPropTbl(*this), SLERR_NOMEM);
		THROW_S(P_NgT = new SrNGramTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_CNgT = new SrConceptNgTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GnT = new SrGeoNodeTbl(P_Db), SLERR_NOMEM);
		THROW_S(P_GwT = new SrGeoWayTbl(P_Db), SLERR_NOMEM);
		{
			CONCEPTID prop_instance = GetReservedConcept(rcInstance);
			CONCEPTID prop_subclass = GetReservedConcept(rcSubclass);
			CONCEPTID prop_crtype   = GetReservedConcept(rcType);
			THROW(prop_instance);
			THROW(prop_subclass);
			THROW(prop_crtype);
		}
		{
			SString err_file_name;
			(err_file_name = pDbPath).SetLastSlash().Cat("bdberr.log");
			P_Db->SetupErrLog(err_file_name);
		}
	}
	CATCH
		ok = 0;
		Close();
	ENDCATCH
	return ok;
}

int SrDatabase::CreateAnonymConcept(CONCEPTID * pID)
{
	int    ok = 1;
	SrConcept c;
	THROW(P_CT->Add(c));
	ASSIGN_PTR(pID, c.ID);
	CATCHZOK
	return ok;
}

int SrDatabase::GetPropType(CONCEPTID propID)
{
	int    type = SRPROPT_INT;
	if(PropType) {
		if(oneof3(propID, PropInstance, PropSubclass, PropType))
			type = SRPROPT_INT;
		else if(P_CpT) {
			SrCProp cp_type(propID, PropType);
			if(P_CpT->Search(cp_type) > 0) {
				int64 _t = 0;
				if(cp_type.Get(_t) > 0)
					type = (int)_t;
				else
					type = 0;
			}
		}
		else
			type = 0;
	}
	return type;
}

int SrDatabase::ResolveConcept(const char * pSymbUtf8, CONCEPTID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(P_WdT->SearchSpecial(SrWordTbl::spcConcept, pSymbUtf8, &lex_id) > 0) {
		SrConcept c;
		if(P_CT->SearchBySymb(lex_id, &c) > 0) {
			ASSIGN_PTR(pID, c.ID);
			ok = 1;
		}
		else {
			c.ID = 0;
			c.SymbID  = lex_id;
			THROW(P_CT->Add(c));
			ASSIGN_PTR(pID, c.ID);
			ok = 1;
		}
	}
	else {
		THROW(P_WdT->AddSpecial(SrWordTbl::spcConcept, pSymbUtf8, &lex_id));
		{
			SrConcept c;
			c.ID = 0;
			c.SymbID  = lex_id;
			THROW(P_CT->Add(c));
			ASSIGN_PTR(pID, c.ID);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::ResolveCPropSymb(const char * pSymbUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(P_WdT->SearchSpecial(SrWordTbl::spcCPropSymb, pSymbUtf8, &lex_id) > 0) {
		ok = 1;
	}
	else {
		THROW(P_WdT->AddSpecial(SrWordTbl::spcCPropSymb, pSymbUtf8, &lex_id));
	}
	CATCH
		ok = 0;
		lex_id = 0;
	ENDCATCH
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::ResolveWord(const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(P_WdT->Search(pWordUtf8, &lex_id) > 0) {
		ASSIGN_PTR(pID, lex_id);
		ok = 1;
	}
	else {
		THROW(P_WdT->Add(pWordUtf8, &lex_id));
		ASSIGN_PTR(pID, lex_id);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrDatabase::ResolveNGram(const LongArray & rList, NGID * pID)
{
	int    ok = -1;
	NGID   ng_id = 0;
	SrNGram ng;
	ng.WordIdList = rList;
	if(P_NgT->Search(ng, &ng_id) > 0) {
		ASSIGN_PTR(pID, ng_id);
		ok = 1;
	}
	else {
		ng.ID = 0;
		THROW(P_NgT->Add(ng));
		ASSIGN_PTR(pID, ng.ID);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrDatabase::Close()
{
	PropInstance = 0;
	PropSubclass = 0;
	ZDELETE(P_WdT);
	ZDELETE(P_GrT);
	ZDELETE(P_WaT);
	ZDELETE(P_CT);
	ZDELETE(P_CpT);
	ZDELETE(P_NgT);
	ZDELETE(P_CNgT);
	ZDELETE(P_GnT);
	ZDELETE(P_GwT);
	if(P_Db) {
		P_Db->RemoveUnusedLogs();
	}
	ZDELETE(P_Db);
	return 1;
}

CONCEPTID SrDatabase::GetReservedConcept(int rc)
{
	CONCEPTID prop = 0;
	SString temp_buf;
	if(rc == rcInstance) {
		if(!PropInstance) {
			THROW(ResolveConcept((temp_buf = "crp_instance").ToUtf8(), &PropInstance));
		}
		prop = PropInstance;
	}
	else if(rc == rcSubclass) {
		if(!PropSubclass) {
			THROW(ResolveConcept((temp_buf = "crp_subclass").ToUtf8(), &PropSubclass));
		}
		prop = PropSubclass;
	}
	else if(rc == rcType) {
		if(!PropType) {
			THROW(ResolveConcept((temp_buf = "crp_type").ToUtf8(), &PropType));
		}
		prop = PropType;
	}
	CATCH
		prop = 0;
	ENDCATCH
	return prop;
}

static int ReadAncodeDescrLine_Ru(const char * pLine, SString & rAncode, SrWordForm & rForm)
{
	int    ok = 1;
	rAncode = 0;

	SString line_buf;
	const char * p = pLine;
	while(*p && *p != '\n' && !(p[0] == '/' && p[1] == '/')) {
		line_buf.CatChar(*p++);
	}
	if(line_buf.NotEmptyS()) {
		SString temp_buf;
		const char * p = line_buf;
		//
		temp_buf = 0;
		while(*p != ' ' && *p != '\t' && *p != 0)
			temp_buf.CatChar(*p++);
		rAncode = temp_buf;
		//
		while(*p == ' ' || *p == '\t') p++;
		temp_buf = 0;
		while(*p != ' ' && *p != '\t' && *p != 0)
			temp_buf.CatChar(*p++);
		//
		int    prev_case = 0;  // Предыдущий токен обозначал падеж
		int    prev_compr = 0; // Предыдущий токен - сравнительная степень прилагательного SRADJCMP_COMPARATIVE
		do {
			while(*p == ' ' || *p == '\t' || *p == ',') p++;
			temp_buf = 0;
			while(*p != ' ' && *p != '\t' && *p != ',' && *p != 0)
				temp_buf.CatChar(*p++);
			if(temp_buf.NotEmpty()) {
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
	rAncode = 0;

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
		temp_buf = 0;
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
			temp_buf = 0;
			while(*p != ' ' && *p != '\t' && *p != 0)
				temp_buf.CatChar(*p++);
			//
			int    prev_case = 0;  // Предыдущий токен обозначал падеж
			int    prev_compr = 0; // Предыдущий токен - сравнительная степень прилагательного SRADJCMP_COMPARATIVE
			do {
				while(*p == ' ' || *p == '\t' || *p == ',') p++;
				temp_buf = 0;
				while(*p != ' ' && *p != '\t' && *p != ',' && *p != 0)
					temp_buf.CatChar(*p++);
				if(temp_buf.NotEmpty()) {
					if(temp_buf == "ADJECTIVE") {
						rForm.SetTag(SRWG_CLASS, SRWC_ADJECTIVE);
					}
					else if(temp_buf == "ADVERB") {
						rForm.SetTag(SRWG_CLASS, SRWC_ADVERB);
					}
					else if(temp_buf == "ARTICLE") {
						rForm.SetTag(SRWG_CLASS, SRWC_ARTICLE);
					}
					else if(temp_buf == "NOUN") {
						rForm.SetTag(SRWG_CLASS, SRWC_NOUN);
					}
					else if(temp_buf == "NUMERAL") {
						rForm.SetTag(SRWG_CLASS, SRWC_NUMERAL);
					}
					else if(temp_buf == "ORDNUM") {
						rForm.SetTag(SRWG_CLASS, SRWC_NUMERALORD);
					}
					else if(temp_buf == "CONJ") {
						rForm.SetTag(SRWG_CLASS, SRWC_CONJUNCTION);
					}
					else if(temp_buf == "PN") {
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
					}
					else if(temp_buf == "PN_ADJ") {
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUNPOSS);
					}
					else if(temp_buf == "INT") {
						rForm.SetTag(SRWG_CLASS, SRWC_INTERJECTION);
					}
					else if(temp_buf == "MOD") {
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
						rForm.SetTag(SRWG_MODAL, 1);
					}
					else if(temp_buf == "PART") { // Частица
						rForm.SetTag(SRWG_CLASS, SRWC_PARTICLE);
					}
					else if(temp_buf == "PREP") {
						rForm.SetTag(SRWG_CLASS, SRWC_PREPOSITION);
					}
					else if(temp_buf == "PRON") { // Не изменяемые местоимения-существительные (all, anybody, etc)
						rForm.SetTag(SRWG_CLASS, SRWC_PRONOUN);
					}
					else if(temp_buf == "VBE") { // Специальный случай - глагол "to be"
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
						rForm.SetTag(SRWG_TOBE, 1);
					}
					else if(temp_buf == "VERB") {
						rForm.SetTag(SRWG_CLASS, SRWC_VERB);
					}
					else if(temp_buf == "comp") {
						rForm.SetTag(SRWG_ADJCMP, SRADJCMP_COMPARATIVE);
					}
					else if(temp_buf == "f") {
						rForm.SetTag(SRWG_GENDER, SRGENDER_FEMININE);
					}
					else if(temp_buf == "fut") {
						rForm.SetTag(SRWG_TENSE, SRTENSE_FUTURE); // Будущая форма (вероятно, только для to be)
					}
					else if(temp_buf == "if") { // Вопросительная форма глагола to be
						rForm.SetTag(SRWG_QUEST, 1);
					}
					else if(temp_buf == "inf") {
						rForm.SetTag(SRWG_ASPECT, SRASPECT_INFINITIVE);
					}
					else if(temp_buf == "m") {
						rForm.SetTag(SRWG_GENDER, SRGENDER_MASCULINE);
					}
					else if(temp_buf == "geo") {
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_GEO);
					}
					else if(temp_buf == "name") {
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PERSONNAME);
					}
					else if(temp_buf == "org") {
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_ORG);
					}
					else if(temp_buf == "narr") { // Нарицательное существительное
						// (не учитываем - считаем все не собственные существительные нарицательными)
					}
					else if(temp_buf == "nom") {
						rForm.SetTag(SRWG_CASE, SRCASE_NOMINATIVE); // Именительный падеж
					}
					else if(temp_buf == "obj") {
						rForm.SetTag(SRWG_CASE, SRCASE_OBJECTIVE); // Объектный падеж местоимений
					}
					else if(temp_buf == "pasa") {
						rForm.SetTag(SRWG_TENSE, SRTENSE_PAST); // Past Indefinite (2-я форма глагола)
					}
					else if(temp_buf == "pers") {
						rForm.SetTag(SRWG_PRONOUN, SRPRON_PERSONAL);
					}
					else if(temp_buf == "pl") {
						rForm.SetTag(SRWG_COUNT, SRCNT_PLURAL);
					}
					else if(temp_buf == "pp") {
						rForm.SetTag(SRWG_TENSE, SRTENSE_PASTPARTICIPLE); // Past Participle (3-я форма глагола)
					}
					else if(temp_buf == "poss") {
						rForm.SetTag(SRWG_POSSESSIVE, 1);
					}
					else if(temp_buf == "pred") { // предикатив (форма притяжательных местоимений, напр.:yours)
						rForm.SetTag(SRWG_PREDICATIVE, 1);
					}
					else if(temp_buf == "attr") { // @unused атрибутив (форма притяжательных местоимений, напр.:your);
					}
					else if(temp_buf == "prop") {
						rForm.SetTag(SRWG_PROPERNAME, SRPROPN_PERSONNAME);
					}
					else if(temp_buf == "prsa") {
						rForm.SetTag(SRWG_TENSE, SRTENSE_PRESENT); // Present (1-я форма глагола)
					}
					else if(temp_buf == "sg") {
						rForm.SetTag(SRWG_COUNT, SRCNT_SINGULAR);
					}
					else if(temp_buf == "sup") {
						rForm.SetTag(SRWG_ADJCMP, SRADJCMP_SUPERLATIVE);
					}
					else if(temp_buf == "uncount") {
						rForm.SetTag(SRWG_COUNTAB, SRCTB_UNCOUNTABLE);
					}
					else if(temp_buf == "mass") {
						rForm.SetTag(SRWG_COUNTAB, SRCTB_MASS);
					}
					else if(temp_buf == "1") {
						rForm.SetTag(SRWG_PERSON, SRPERSON_FIRST);
					}
					else if(temp_buf == "2") {
						rForm.SetTag(SRWG_PERSON, SRPERSON_SECOND);
					}
					else if(temp_buf == "3") {
						rForm.SetTag(SRWG_PERSON, SRPERSON_THIRD);
					}
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
//
//
//
SLAPI SrWordAssoc::SrWordAssoc()
{
	ID = 0;
	WordID = 0;
	Flags = 0;
	BaseDescrID = 0;
	FlexiaModelID = 0;
	AccentModelID = 0;
	PrefixID = 0;
	AffixModelID = 0;
}

SrWordAssoc & SLAPI SrWordAssoc::Normalize()
{
	Flags = 0;
	if(FlexiaModelID)
		Flags |= fHasFlexiaModel;
	if(AccentModelID)
		Flags |= fHasAccentModel;
	if(PrefixID)
		Flags |= fHasPrefix;
	if(AffixModelID)
		Flags |= fHasAffixModel;
	return *this;
}

SString & FASTCALL SrWordAssoc::ToStr(SString & rBuf) const
{
	return (rBuf = 0).CatChar('[').Cat(ID).CatDiv(',', 2).Cat(WordID).CatDiv(',', 2).Cat("0x").CatHex(Flags).CatDiv(',', 2).
		Cat(BaseDescrID).CatDiv(',', 2).Cat(FlexiaModelID).CatDiv(',', 2).Cat(AccentModelID).CatDiv(',', 2).
		Cat(PrefixID).CatDiv(',', 2).Cat(AffixModelID).CatChar(']');
}
//
//
//
SrWordAssocTbl::SrWordAssocTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("words.db->wordassoc", BDbTable::idxtypHash, 0, 0, 0), pDb)
{
	//
	// Индекс по идентификатору лексемы. Неуникальный.
	//
	class Idx01 : public SecondaryIndex {
	public:
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer buf;
			rData.Get(buf);
			SrWordAssoc rec;
			((SrWordAssocTbl *)P_MainT)->SerializeRecBuf(-1, &rec, buf);
			rResult = rec.WordID;
			return 0;
		}
	};
	//
	// Индекс по полной записи (без ее идентификатора). Уникальный.
	//
	class Idx02 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			SBuffer buf;
			rData.Get(buf);
			rResult = buf;
			return 0;
		}
	};

	SeqID = 0;
	THROW_SL(new BDbTable(BDbTable::Config("words.db->wordassoc_idx01", BDbTable::idxtypHash, cfDup, 0, 0), pDb, new Idx01, this));
	THROW_SL(new BDbTable(BDbTable::Config("words.db->wordassoc_idx02", BDbTable::idxtypHash, 0, 0, 0), pDb, new Idx02, this));
	if(P_Db)
		THROW_DB(SeqID = P_Db->CreateSequence("seq_wordassoc_id", 0));
	CATCH
		Close();
	ENDCATCH
}

SrWordAssocTbl::~SrWordAssocTbl()
{
	CALLPTRMEMB(P_Db, CloseSequence(SeqID));
}

int SrWordAssocTbl::Add(SrWordAssoc * pWa, int32 * pID)
{
	int    ok = 1;
	LEXID  id = 0;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	SerializeRecBuf(+1, pWa, buf);
	key_buf = buf;
	data_buf.Alloc(128);
	if(BDbTable::Search(2, key_buf, data_buf)) {
		key_buf.Get(&id);
		ok = -1;
	}
	else {
		int64 __id = 0;
		THROW_DB(P_Db->GetSequence(SeqID, &__id));
		id = (LEXID)__id;
		key_buf = id;
		data_buf = buf;
		THROW_DB(InsertRec(key_buf, data_buf));
	}
	pWa->ID = id;
	CATCH
		id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, id);
	return ok;
}

int SrWordAssocTbl::Search(int32 id, SrWordAssoc * pWa)
{
	int    ok = -1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	key_buf = id;
	data_buf.Alloc(1024);
	if(BDbTable::Search(key_buf, data_buf)) {
		if(pWa) {
			buf.Clear();
			data_buf.Get(buf);
			SerializeRecBuf(-1, pWa, buf);
		}
		ok = 1;
	}
	return ok;
}

int SrWordAssocTbl::Search(LEXID wordID, TSArray <SrWordAssoc> & rList)
{
	int    ok = 1;
	BDbTable::Buffer key_buf, data_buf;
	SBuffer buf;
	key_buf = wordID;
	data_buf.Alloc(128);
	rList.clear();
	BDbCursor curs(*this, 1);
	if(curs.Search(key_buf, data_buf, spEq)) {
		do {
			int32  id = 0;
			SrWordAssoc wa;
			buf.Clear();
			data_buf.Get(buf);
			SerializeRecBuf(-1, &wa, buf);
			if(wa.WordID == wordID) {
				key_buf.Get(&id);
				wa.ID = id;
				rList.insert(&wa);
				key_buf = wordID;
			}
			else
				break;
		} while(curs.Search(key_buf, data_buf, spNext));
	}
	else
		ok = -1;
	// CATCHZOK
	return ok;
}

int SrWordAssocTbl::SerializeRecBuf(int dir, SrWordAssoc * pWa, SBuffer & rBuf)
{
	int    ok = 1;
	SSerializeContext * p_sctx = GetSCtx();
	THROW_DB(p_sctx);
	THROW_SL(p_sctx->Serialize(dir, pWa->Flags, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pWa->WordID, rBuf));
	THROW_SL(p_sctx->Serialize(dir, pWa->BaseDescrID, rBuf));
	if(pWa->Flags & SrWordAssoc::fHasFlexiaModel) {
		THROW_SL(p_sctx->Serialize(dir, pWa->FlexiaModelID, rBuf));
	}
	if(pWa->Flags & SrWordAssoc::fHasAccentModel) {
		THROW_SL(p_sctx->Serialize(dir, pWa->AccentModelID, rBuf));
	}
	if(pWa->Flags & SrWordAssoc::fHasPrefix) {
		THROW_SL(p_sctx->Serialize(dir, pWa->PrefixID, rBuf));
	}
	if(pWa->Flags & SrWordAssoc::fHasAffixModel) {
		THROW_SL(p_sctx->Serialize(dir, pWa->AffixModelID, rBuf));
	}
	CATCHZOK
	return ok;
}
//
//
//
SrGeoNodeTbl::SrGeoNodeTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("geomap.db->node", BDbTable::idxtypHash, 0, 2048, 8*1024), pDb)
{
	/*
	class Idx01 : public SecondaryIndex {
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult)
		{
			size_t rec_size = 0;
			size_t key_size = 0;
			PPOsm::Tile tile;
			//PPOsm::NodeCluster clu;
			const void * p_key = rKey.GetPtr(&key_size);
			const void * p_data = rData.GetPtr(&rec_size);
			uint64 outer_id = 0;
			if(p_key)
				if(key_size == 4)
					outer_id = *(uint32 *)p_key;
				else if(key_size == 8)
					outer_id = *(uint64 *)p_key;
			Cb_Clu.SetBuffer(p_data, rec_size);
			Cb_Clu.GetTile(outer_id, &tile);
			rResult.Set(&tile.V, sizeof(tile.V));
			return 0;
		}
		PPOsm::NodeCluster Cb_Clu;
	};
	new BDbTable(BDbTable::Config("geomap.db->node_idx01", BDbTable::idxtypBTree, BDbTable::cfDup, 2048, 4*1024), pDb, new Idx01, this);
	*/
}

SrGeoNodeTbl::~SrGeoNodeTbl()
{
}

IMPL_CMPCFUNC(PPOsm_Node_ByWay, p1, p2)
{
	uint64 id1 = *(uint64 *)p1;
	uint64 id2 = *(uint64 *)p2;
	uint   pos1 = 0;
	uint   pos2 = 0;
	const PPOsm::Way * p_way = pExtraData ? (const PPOsm::Way *)pExtraData : 0;
    if(p_way) {
        if(!p_way->NodeRefs.lsearch(id1, &pos1))
			pos1 = UINT_MAX;
        if(!p_way->NodeRefs.lsearch(id2, &pos2))
			pos2 = UINT_MAX;
    }
	else {
		pos1 = UINT_MAX;
		pos2 = UINT_MAX;
	}
    return CMPSIGN(pos1, pos2);
}

int SLAPI SrGeoNodeTbl::GetWayNodes(const PPOsm::Way & rWay, TSArray <PPOsm::Node> & rNodeList)
{
	int    ok = -1;
	const  uint _c = rWay.NodeRefs.getCount();

	rNodeList.clear();
	PPOsm::NodeCluster clu;
	TSArray <PPOsm::Node> clu_node_list;
	UintHashTable processed_pos_list;
	//
	// Для замкнутого контура последняя точка равна первой - просто добавим ее в список в самом конце функции
	//
	const int is_contur = BIN(_c > 1 && rWay.NodeRefs.get(_c-1) == rWay.NodeRefs.get(0));
	if(is_contur)
		processed_pos_list.Add(_c-1);
    for(uint i = 0; i < _c; i++) {
		if(!processed_pos_list.Has(i)) {
			const uint64 node_id = (uint64)rWay.NodeRefs.get(i);
			uint64 logical_id = 0;
			PPOsm::Node node;
			if(Helper_Search(node_id, &clu, &node, 0, &logical_id) > 0) {
				rNodeList.insert(&node);
				processed_pos_list.Add(i);
				clu.Get(logical_id, clu_node_list, 0);
				for(uint k = 0; k < clu_node_list.getCount(); k++) {
					const PPOsm::Node & r_node = clu_node_list.at(k);
					uint  w_pos = 0;
					if(r_node.ID != node.ID && rWay.NodeRefs.lsearch(r_node.ID, &w_pos) && !processed_pos_list.Has(w_pos)) {
						rNodeList.insert(&r_node);
						processed_pos_list.Add(w_pos);
					}
				}
			}
		}
    }
    rNodeList.sort(PTR_CMPCFUNC(PPOsm_Node_ByWay), (void *)&rWay); // @badcast
	//
	// Последнюю точку контура вставляем после сортировки - иначе сортировка сбойнет (вда одинаковых идентификатора)
	//
	if(is_contur) {
		const PPOsm::Node & r_first_node = clu_node_list.at(0);
		rNodeList.insert(&r_first_node);
	}
    if(rNodeList.getCount() == rWay.NodeRefs.getCount())
		ok = 1;
    return ok;
}

int SLAPI SrGeoNodeTbl::Search(uint64 id, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID)
{
	return Helper_Search(id, 0, pNode, pNrList, pLogicalID);
}

int SLAPI SrGeoNodeTbl::Search(uint64 id, PPOsm::NodeCluster * pCluster, uint64 * pLogicalID)
{
	return Helper_Search(id, pCluster, 0, 0, pLogicalID);
}

int SLAPI SrGeoNodeTbl::Helper_Search(uint64 id, PPOsm::NodeCluster * pCluster, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID)
{
/*
LogicalCount=  1; ClusterCount=64512077; ActualCount=64512077; Size=838657001;
LogicalCount=128; ClusterCount=  285217; ActualCount=29844065; Size=169060869;
LogicalCount=  2; ClusterCount=10242038; ActualCount=20484076; Size=191137226;
LogicalCount= 64; ClusterCount=  369007; ActualCount=19787392; Size=112540879;
LogicalCount= 32; ClusterCount=  694227; ActualCount=19543439; Size=112406576;
LogicalCount=  4; ClusterCount= 5003717; ActualCount=18204549; Size=139336132;
LogicalCount= 16; ClusterCount= 1262121; ActualCount=17685283; Size=106760761;
LogicalCount=  8; ClusterCount= 2389896; ActualCount=17049582; Size=112131954;
*/
	const uchar logical_count_priority[]      = {    1,  128,    2,   64,   32,    4,   16,    8 };
	const uchar logical_count_priority_bits[] = { 0x00, 0x7f, 0x01, 0x3f, 0x1f, 0x03, 0x0f, 0x07 };
	int    ok = -1;
	uint64 ret_logical_id = 0;
	PPOsm::NodeCluster nc;
	TSArray <PPOsm::Node> nc_list;
	PPOsm::NodeRefs nr_list;
	DataBuf.Alloc(1024);
	for(uint i = 0; ok < 0 && i < SIZEOFARRAY(logical_count_priority); /* see end of loop */) {
		const uint64 logical_id = (id & ~logical_count_priority_bits[i]);
		{
			uint8  _key[16];
			size_t _key_sz = sshrinkuint64(logical_id, _key);
			KeyBuf.Set(_key, _key_sz);
		}
		uint   next_i = UINT_MAX;
		if(BDbTable::Search(0, KeyBuf, DataBuf)) {
			{
				size_t dbsz = 0;
				const void * p_dbptr = DataBuf.GetPtr(&dbsz);
				nc.SetBuffer(p_dbptr, dbsz);
			}
			PPOsm::Node ex_head;
			uint   ex_count_logic = 0;
			uint   ex_count_actual = 0;
			THROW(nc.Get(logical_id, nc_list, &nr_list, &ex_head, &ex_count_logic, &ex_count_actual));
			assert(logical_id == ex_head.ID); // @paranoic
			assert(id >= logical_id); // @paranoic
			if(id < (logical_id + ex_count_logic)) {
				for(uint ncidx = 0; ok < 0 && ncidx < nc_list.getCount(); ncidx++) {
					const PPOsm::Node & r_node = nc_list.at(ncidx);
					if(r_node.ID == id) {
						ret_logical_id = logical_id;
						ASSIGN_PTR(pNode, r_node);
						ASSIGN_PTR(pCluster, nc);
						ok = (int)logical_count_priority[i];
					}
				}
				if(ok < 0) {
					ret_logical_id = logical_id;
					const uchar tp = logical_count_priority[i];
					for(uint j = i+1; j < SIZEOFARRAY(logical_count_priority); j++) {
						if(logical_count_priority[j] > tp) {
							next_i = j;
							break;
						}
					}
				}
			}
			else {
				ret_logical_id = 0;
				next_i = i+1;
			}
		}
		else {
			ret_logical_id = 0;
			next_i = i+1;
		}
		i = next_i;
	}
	CATCHZOK
	ASSIGN_PTR(pLogicalID, ret_logical_id);
	return ok;
}

int SLAPI SrGeoNodeTbl::Helper_Set(PPOsm::NodeCluster & rNc, uint64 outerID, int update)
{
	int    ok = 1;
	uint64 hid = 0;
	int    his = 0;
	{
		if(outerID) {
			hid = outerID;
		}
		else {
			THROW(his = rNc.GetHeaderID(&hid));
		}
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(hid, _key);
		KeyBuf.Set(_key, _key_sz);
	}
	{
		size_t buf_size = 0;
		const void * p_buf = rNc.GetBuffer(&buf_size);
		THROW(p_buf);
		THROW(DataBuf.Set(p_buf, buf_size));
	}
	if(update) {
		THROW_DB(UpdateRec(KeyBuf, DataBuf));
	}
	else {
		THROW_DB(InsertRec(KeyBuf, DataBuf));
	}
	CATCHZOK
	return ok;
}

int SLAPI SrGeoNodeTbl::Add(PPOsm::NodeCluster & rNc, uint64 outerID)
{
	return Helper_Set(rNc, outerID, 0);
}

int SLAPI SrGeoNodeTbl::Update(PPOsm::NodeCluster & rNc, uint64 outerID)
{
	return Helper_Set(rNc, outerID, 1);
}
//
//
//
SLAPI SrGeoWayTbl::SrGeoWayTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("geomap.db->way", BDbTable::idxtypHash, 0, 2048, 8*1024), pDb)
{
}

SLAPI SrGeoWayTbl::~SrGeoWayTbl()
{
}

int SLAPI SrGeoWayTbl::Add(PPOsm::Way & rW, PPOsm::WayBuffer * pBuffer)
{
	int    ok = 1;
	{
		uint8  _key[16];
		size_t _key_sz = sshrinkuint64(rW.ID, _key);
		KeyBuf.Set(_key, _key_sz);
	}
	{
		PPOsm::WayBuffer wbuf__;
		SETIFZ(pBuffer, &wbuf__);
		uint64 outer_id = rW.ID;
		THROW(pBuffer->Put(&rW, &outer_id));
		{
			//
			// Test
			//
            PPOsm::Way test_w;
            assert(pBuffer->Get(rW.ID, &test_w));
            assert(test_w == rW);
		}
		size_t buf_size = 0;
		const void * p_buf = pBuffer->GetBuffer(&buf_size);
		THROW(p_buf);
		THROW(DataBuf.Set(p_buf, buf_size));
	}
	THROW_DB(InsertRec(KeyBuf, DataBuf));
	CATCHZOK
	return ok;
}

int SLAPI SrGeoWayTbl::Search(uint64 id, PPOsm::Way * pW)
{
	int    ok = -1;
	DataBuf.Alloc(12*1024);
	{
		{
			uint8  _key[16];
			size_t _key_sz = sshrinkuint64(id, _key);
			KeyBuf.Set(_key, _key_sz);
		}
		if(BDbTable::Search(0, KeyBuf, DataBuf)) {
			PPOsm::WayBuffer wbuf;
			{
				size_t dbsz = 0;
				const void * p_dbptr = DataBuf.GetPtr(&dbsz);
				wbuf.SetBuffer(p_dbptr, dbsz);
			}
			if(pW) {
				THROW(wbuf.Get(id, pW));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SrImportParam::SrImportParam()
{
	InputKind = 0;
	LangID = 0;
	CpID = 0;
	Flags = 0;
}

int SrImportParam::SetField(int fld, const char * pVal)
{
	StrItems.Add(fld, pVal);
	return 1;
}

int SrImportParam::GetField(int fld, SString & rVal) const
{
	return StrItems.Get(fld, rVal);
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
					long   fm_count = line_buf.ToLong();
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
										MEMSZERO(item);
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
					TSArray <SrWordAssoc> test_wa_list;
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
												wa.BaseDescrID = anc_id;
											SETIFZ(wa.BaseDescrID, base_skeleton_wf_id);
											if(scan.Skip().SearchChar(' ')) {
												scan.Get(item_buf);
												scan.IncrLen(1);
												if(!(item_buf == "-")) {
													temp_val = item_buf.ToLong();
													if(temp_val >= 0) {
														pfx_assoc.Search(temp_val, &wa.PrefixID, 0);
													}
												}
											}
										}
									}
								}
							}
						}
						if(wa.FlexiaModelID || wa.BaseDescrID) {
							THROW(P_WaT->Add(&wa.Normalize(), &wa_id));
							if(rParam.Flags & rParam.fTest) {
								P_WaT->Search(wa.WordID, test_wa_list);
								for(uint j = 0; j < test_wa_list.getCount(); j++) {
									const SrWordAssoc & r_wa = test_wa_list.at(j);
									(line_buf = 0).Cat(word_buf).Tab(2).Cat(r_wa.ToStr(temp_buf)).CR();
									test_out_file.WriteLine(line_buf);
								}
							}
						}
					}
				}
			}
			THROW(tra.Commit());
			THROW(P_Db->TransactionCheckPoint()); // @v9.7.8
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::GetWordInfo(const char * pWordUtf8, long flags, TSArray <SrWordInfo> & rInfo)
{
	int    ok = -1;
	SStringU word_buf, base_buf_u, afx_buf_u, pfx_buf_u;
	SString temp_buf;
	word_buf.CopyFromUtf8(pWordUtf8, strlen(pWordUtf8));
	int    tc = word_buf.AnalyzeCase();
	base_buf_u = word_buf.ToLower();
	const  uint len = word_buf.Len();
	StrAssocArray afx_list;
	TSArray <SrWordAssoc> wa_list;
	SrFlexiaModel fm;
	LongArray wf_list;
	for(uint pfx_len = 0; pfx_len < len; pfx_len++) {
		int    inv_pfx = 0; // Если !0, то префикс не допустимый
		LEXID  pfx_id = 0;
		if(pfx_len != 0) {
			word_buf.Sub(0, pfx_len, pfx_buf_u);
			pfx_buf_u.CopyToUtf8(temp_buf, 0);
			if(P_WdT->SearchSpecial(SrWordTbl::spcPrefix, temp_buf, &pfx_id) > 0) {
				;
			}
			else
				inv_pfx = 1;
		}
		else
			pfx_buf_u = 0;
		if(!inv_pfx) { // Если префикс не пустой и не содержится в БД, то перебирать оставшуюся часть слова нет смысла
			const  uint __len = len-pfx_len;
			for(uint afx_len = 0; afx_len <= __len; afx_len++) {
				int    inv_afx = 0; // Если !0, то аффикс не допустимый
				LEXID  base_id = 0, afx_id = 0;
				const  uint base_len = __len-afx_len;
				if(base_len == 0) {
					base_buf_u = 0;
					if(ZeroWordID)
						base_id = ZeroWordID;
					else if(P_WdT->SearchSpecial(SrWordTbl::spcEmpty, temp_buf, &base_id) > 0)
						ZeroWordID = base_id;
				}
				else {
					word_buf.Sub(pfx_len, base_len, base_buf_u);
					base_buf_u.CopyToUtf8(temp_buf, 0);
					if(P_WdT->Search(temp_buf, &base_id) > 0) {
						;
					}
				}
				if(afx_len != 0) {
					word_buf.Sub(pfx_len+base_len, afx_len, afx_buf_u);
					afx_buf_u.CopyToUtf8(temp_buf, 0);
					uint   afx_pos = 0;
					if(afx_list.SearchByText(temp_buf, 0, &afx_pos))
						afx_id = afx_list.at(afx_pos).Id;
					else if(pfx_len == 0 && P_WdT->SearchSpecial(SrWordTbl::spcAffix, temp_buf, &afx_id) > 0) {
						//
						// Для pfx_len > 0 все возможные окончания уже найдены на итерации (pfx == 0)
						//
						afx_list.Add(afx_id, temp_buf);
					}
					else
						inv_afx = 1;
				}
				else
					afx_id = 0;
				if(base_id && !inv_afx) {
					wa_list.clear();
					P_WaT->Search(base_id, wa_list);
					for(uint i = 0; i < wa_list.getCount(); i++) {
						const SrWordAssoc & r_wa = wa_list.at(i);
						if(r_wa.FlexiaModelID) {
							fm.Clear();
							if(P_GrT->Search(r_wa.FlexiaModelID, &fm) > 0) {
								wf_list.clear();
								if(fm.Search(afx_id, pfx_id, wf_list) > 0) {
									for(uint j = 0; j < wf_list.getCount(); j++) {
										SrWordInfo ii;
										ii.BaseID = base_id;
										ii.PrefixID = pfx_id;
										ii.AffixID = afx_id;
										ii.BaseFormID = r_wa.BaseDescrID;
										ii.FormID = wf_list.get(j);
										ii.WaID = r_wa.ID;
										rInfo.insert(&ii);
										ok = 1;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int SrDatabase::Transform(const char * pWordUtf8, const SrWordForm * pDestForm, TSArray <SrWordInfo> & rResult)
{
	int    ok = -1;
	rResult.clear();
	TSArray <SrWordInfo> info_list;
	if(GetWordInfo(pWordUtf8, 0, info_list) > 0) {
		SrWordForm base_wf, wf, test_wf;
		TSArray <SrWordAssoc> wa_list;
		for(uint j = 0; j < info_list.getCount(); j++) {
			const SrWordInfo & r_item = info_list.at(j);
			if(pDestForm) {
				if(r_item.FormID) {
					P_GrT->Search(r_item.FormID, &base_wf);
					test_wf.Merge(base_wf, *pDestForm, 1);
				}
				else if(r_item.BaseFormID) {
					P_GrT->Search(r_item.BaseFormID, &base_wf);
					test_wf.Merge(base_wf, *pDestForm, 1);
				}
				else
					test_wf = *pDestForm;
				if(P_WaT->Search(r_item.BaseID, wa_list) > 0) {
					for(uint i = 0; i < wa_list.getCount(); i++) {
						const SrWordAssoc & r_wa = wa_list.at(i);
						if(r_wa.FlexiaModelID) {
							SrFlexiaModel model;
							if(P_GrT->Search(r_wa.FlexiaModelID, &model) > 0) {
								SrFlexiaModel::Item model_item;
								for(size_t fp = 0; model.GetNext(&fp, model_item) > 0;) {
									if(model_item.WordFormID && P_GrT->Search(model_item.WordFormID, &wf) > 0) {
										double score1 = test_wf.MatchScore(wf);
										double score2 = pDestForm->MatchScore(wf);
										double score = score1+score2;
										if(score > 0.0) {
											uint rc = rResult.getCount();
											int  do_insert = 0;
											if(rc) {
												do {
													SrWordInfo & r_res_item = rResult.at(--rc);
													if(score > r_res_item.Score) {
														// if(r_res_item.Score < 1.0) // @debug
															rResult.atFree(rc);
														do_insert = 1;
													}
													else if(score == r_res_item.Score)
														do_insert = 1;
												} while(rc);
											}
											else
												do_insert = 1;
											if(do_insert) {
												SrWordInfo ii;
												ii.BaseID = r_wa.WordID;
												ii.PrefixID = model_item.PrefixID;
												ii.AffixID = model_item.AffixID;
												ii.BaseFormID = r_wa.BaseDescrID;
												ii.FormID = model_item.WordFormID;
												ii.WaID = r_wa.ID;
												ii.Score = score;
												rResult.insert(&ii);
												ok = 1;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int SrDatabase::WordInfoToStr(const SrWordInfo & rWi, SString & rBuf)
{
	int    ok = 1;
	rBuf = 0;
	SString temp_buf;
	rBuf.CatChar('\'');
	if(rWi.PrefixID) {
		if(P_WdT->Search(rWi.PrefixID, temp_buf) > 0)
			rBuf.Cat(temp_buf);
		else
			rBuf.CatChar('=').Cat(rWi.PrefixID);
	}
	rBuf.CatChar('|');
	if(rWi.BaseID) {
		if(rWi.BaseID != ZeroWordID) {
			if(P_WdT->Search(rWi.BaseID, temp_buf) > 0)
				rBuf.Cat(temp_buf);
			else
				rBuf.CatChar('=').Cat(rWi.BaseID);
		}
	}
	rBuf.CatChar('|');
	if(rWi.AffixID) {
		if(P_WdT->Search(rWi.AffixID, temp_buf) > 0)
			rBuf.Cat(temp_buf);
		else
			rBuf.CatChar('=').Cat(rWi.AffixID);
	}
	rBuf.CatChar('\'');
	rBuf.Space();
	if(rWi.FormID || rWi.BaseFormID) {
		SrWordForm base_wf, var_wf, wf;
		if(rWi.BaseFormID)
			P_GrT->Search(rWi.BaseFormID, &base_wf);
		if(rWi.FormID)
			P_GrT->Search(rWi.FormID, &var_wf);
		wf.Merge(base_wf, var_wf);
		wf.ToStr(temp_buf);
	}
	else
		temp_buf = 0;
	rBuf.CatBrackStr(temp_buf);
	//
	temp_buf = 0;
	if(rWi.WaID) {
		SrWordAssoc wa;
		if(P_WaT->Search(rWi.WaID, &wa) > 0)
			wa.ToStr(temp_buf);
		else
			temp_buf.CatChar('[').CatChar('=').Cat(rWi.WaID).CatChar(']');
	}
	else
		temp_buf.CatChar('[').CatChar(']');
	if(rWi.Score != 0.0) {
		temp_buf.Space().CatEq("Score", rWi.Score, MKSFMTD(0, 1, 0));
	}
	rBuf.Cat(temp_buf);
	return ok;
}

int SrDatabase::SearchWord(int special, const char * pWordUtf8, LEXID * pID)
{
	int    ok = -1;
	LEXID  lex_id = 0;
	if(special)
		ok = P_WdT->SearchSpecial(special, pWordUtf8, &lex_id);
	else if(P_WdT->Search(pWordUtf8, &lex_id) > 0)
		ok = 1;
	ASSIGN_PTR(pID, lex_id);
	return ok;
}

int SrDatabase::SearchNGram(const LongArray & rNg, NGID * pID)
{
	int    ok = -1;
	NGID   ng_id = 0;
	SrNGram ng;
	ng.WordIdList = rNg;
	if(P_NgT->Search(ng, &ng_id) > 0)
		ok = 1;
	ASSIGN_PTR(pID, ng_id);
	return ok;
}

int SrDatabase::GetNgConceptList(NGID ngID, long flags, Int64Array & rConceptList)
{
	int    r = P_CNgT->GetConceptList(ngID, rConceptList);
	if(r > 0 && flags & ngclAnonymOnly) {
		SrConcept c;
		uint i = rConceptList.getCount();
		if(i) do {
			CONCEPTID cid = rConceptList.get(--i);
			if(P_CT->SearchByID(cid, &c.Clear()) > 0) {
				if(c.SymbID != 0) {
					rConceptList.atFree(i);
				}
			}
		} while(i);
		if(!rConceptList.getCount())
			r = -1;
	}
	return r;
}

int SrDatabase::Helper_GetConceptHier(CONCEPTID cID, Int64Array & rConceptHier)
{
	int    ok = -1;
	CONCEPTID prop_instance = GetReservedConcept(rcInstance);
	CONCEPTID prop_subclass = GetReservedConcept(rcSubclass);
	SrCProp prop(cID, prop_instance);
	if(P_CpT->Search(prop) > 0) {
		CONCEPTID cid = 0;
		THROW(prop.Get(cid));
		rConceptHier.add(cid);
		THROW(Helper_GetConceptHier(cid, rConceptHier));
		ok = 2;
	}
	else {
		prop.CID = cID;
		prop.PropID = prop_subclass;
		if(P_CpT->Search(prop) > 0) {
			CONCEPTID cid = 0;
			THROW(prop.Get(cid));
			rConceptHier.add(cid);
			THROW(Helper_GetConceptHier(cid, rConceptHier));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::GetConceptHier(CONCEPTID cID, Int64Array & rConceptHier)
{
	rConceptHier.clear();
	return Helper_GetConceptHier(cID, rConceptHier);
}

int SrDatabase::GetConceptPropList(CONCEPTID cID, SrCPropList & rPl)
{
	return P_CpT->GetPropList(cID, rPl);
}

int SrDatabase::GetPropDeclList(CONCEPTID cID, SrCPropDeclList & rPdl)
{
	int    ok = -1;
	rPdl.Clear();
	Int64Array chier;
	Helper_GetConceptHier(cID, chier);
	uint c = chier.getCount();
	if(c) do {
		CONCEPTID cid = chier.get(--c);
		SrConcept cp;
		THROW(P_CT->SearchByID(cid, &cp) > 0);
		THROW(rPdl.Merge(cp.Pdl));
	} while(c);
	if(rPdl.GetCount())
		ok = 1;
	CATCHZOK
	return ok;
}

int SrDatabase::GetConceptSymb(CONCEPTID cID, SString & rSymbUtf8)
{
	rSymbUtf8 = 0;
	int    ok = -1;
	SrConcept c;
	if(P_CT->SearchByID(cID, &c) > 0) {
		if(c.SymbID) {
			THROW(P_WdT->Search(c.SymbID, rSymbUtf8));
			ok = 1;
		}
		else {
			rSymbUtf8.CatEq("Anonym", cID);
			ok = 2;
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::Helper_MakeConceptProp(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, CONCEPTID cID)
{
	int    ok = 1;
	SrCPropDecl pd;
	SrConcept prop_cp;
	Int64Array chier;
	THROW(GetConceptHier(cID, chier));
	if(!isempty(pPropSymb)) {
		LEXID symb_id = 0;
		THROW(SearchWord(SrWordTbl::spcCPropSymb, pPropSymb, &symb_id) > 0);
		THROW(rPdl.GetBySymbID(symb_id, pd) > 0);
		if(chier.lsearch(pd.PropID)) {
			rProp.PropID = pd.PropID;
			rProp = cID;
		}
		else {
			ok = -1; // Значение не соответствует типу свойства
		}
	}
	else {
		uint   suited_pd_idx = 0;
		CONCEPTID prop_id = 0;
		for(uint i = 0; ok > 0 && i < rPdl.GetCount(); i++) {
			rPdl.Get(i, pd);
			if(chier.lsearch(pd.PropID)) {
				if(!suited_pd_idx) {
					suited_pd_idx = i+1;
					prop_id = pd.PropID;
				}
				else {
					ok = -1; // Неоднозначность в определении свойства концепции по значению
				}
			}
		}
		if(ok > 0) {
			if(suited_pd_idx) {
				rProp.PropID = prop_id;
				rProp = cID;
			}
			else {
				ok = -1; // Не удалось идентифицировать свойство по значению
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropN(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, double value)
{
	int    ok = 1;
	int   cp_type = 0;
	SrCPropDecl pd;
	SrConcept prop_cp;
	if(!isempty(pPropSymb)) {
		LEXID symb_id = 0;
		THROW(SearchWord(SrWordTbl::spcCPropSymb, pPropSymb, &symb_id) > 0);
		THROW(rPdl.GetBySymbID(symb_id, pd) > 0);
		cp_type = GetPropType(pd.PropID);
		if(cp_type == SRPROPT_REAL) {
			rProp.PropID = pd.PropID;
			rProp = value;
		}
		else if(cp_type == SRPROPT_INT) {
			if(ffrac(value) == 0.0) {
				rProp.PropID = pd.PropID;
				rProp = (int64)value;
			}
			else
				ok = -1; // Попытка установить вещественное значение для свойства, имеющего тип #int
		}
		else if(cp_type == 0)
			ok = -1; // Неопределенный тип свойства
		else
			ok = -1; // Значение не соответствует типу свойства
	}
	else {
		uint   suited_pd_idx = 0;
		CONCEPTID prop_id = 0;
		for(uint i = 0; ok > 0 && i < rPdl.GetCount(); i++) {
			rPdl.Get(i, pd);
			cp_type = GetPropType(pd.PropID);
			if(cp_type == SRPROPT_REAL) {
				if(!suited_pd_idx) {
					suited_pd_idx = i+1;
					prop_id = pd.PropID;
				}
				else {
					ok = -1; // Неоднозначность в определении свойства концепции по значению
				}
			}
		}
		if(ok > 0) {
			if(suited_pd_idx) {
				rProp.PropID = prop_id;
				rProp = value;
			}
			else {
				ok = -1; // Не удалось идентифицировать свойство по значению
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropC(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const char * pConceptSymb)
{
	int    ok = 1;
	SrConcept concept;
	LEXID  cs_id = 0;
	THROW(SearchWord(SrWordTbl::spcConcept, pConceptSymb, &cs_id));
	THROW(P_CT->SearchBySymb(cs_id, &concept) > 0);
	THROW(Helper_MakeConceptProp(rPdl, pPropSymb, rProp, concept.ID) > 0);
	CATCHZOK
	return ok;
}

int SrDatabase::MakeConceptPropNg(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const LongArray & rNg)
{
	int    ok = 0;
	NGID   ng_id = 0;
	THROW(SearchNGram(rNg, &ng_id) > 0);
	{
		Int64Array clist, hlist;
		THROW(GetNgConceptList(ng_id, 0, clist) > 0);
		for(uint j = 0; !ok && j < clist.getCount(); j++) {
			int r = Helper_MakeConceptProp(rPdl, pPropSymb, rProp, clist.get(j));
			THROW(r);
			if(r > 0) {
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrDatabase::FormatProp(const SrCProp & rCp, long flags, SString & rBuf)
{
	rBuf = 0;
	if(rCp.PropID) {
		SString symb;
		if(GetConceptSymb(rCp.PropID, symb) > 0)
			symb.Utf8ToChar();
		else
			symb = "#unknprop#";
		rBuf.Cat(symb).CatChar('=');
		int    typ = GetPropType(rCp.PropID);
		if(typ == SRPROPT_INT) {
			int64 iv = 0;
			rCp.Get(iv);
			rBuf.Cat(iv);
		}
		else if(typ == SRPROPT_REAL) {
			double rv = 0.0;
			rCp.Get(rv);
			rBuf.Cat(rv);
		}
		else
			rBuf.Cat("#unkn#");
	}
	else
		rBuf.Cat("#zeroprop");
	return 1;
}

int SrDatabase::StoreGeoNodeList(const TSArray <PPOsm::Node> & rList, const LLAssocArray * pNodeToWayAsscList, int dontCheckExist, TSArray <PPOsm::NodeClusterStatEntry> * pStat)
{
	const  uint max_cluster_per_tx = 1024;
	const  int  use_transaction = 1;
	//const  int  use_outer_id = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		TSCollection <PPOsm::NodeCluster> cluster_list;
		TSArray <uint64> outer_id_list;
		TSArray <PPOsm::Node> test_list;
		PPOsm::NodeCluster ex_cluster;
		TSArray <PPOsm::Node> ex_list;
		uint   next_node_to_way_assc_pos = 0;
		size_t offs = 0;
		{
			while(offs < _count) {
				uint   actual_count_ = 0;
				const  PPOsm::Node * p_node = &rList.at(offs);
                uint64 fault_logical_id = 0;
                PPOsm::Node found_node;
				//int   sr = P_GnT->Search(p_node->ID, &found_node, 0, &fault_logical_id);
				int   sr = dontCheckExist ? -1 : P_GnT->Search(p_node->ID, &ex_cluster, &fault_logical_id);
				THROW(sr);
				if(sr > 0) {
					{
						uint   ex_count_logic = 0;
						uint   ex_count_actual = 0;
						ex_list.clear();
						assert(ex_cluster.Get(fault_logical_id, ex_list, 0, 0, &ex_count_logic, &ex_count_actual));
						for(uint   forward_idx = offs; forward_idx < _count; forward_idx++) {
							uint64 forward_id = rList.at(forward_idx).ID;
							uint   fpos = 0;
							if(ex_list.lsearch(&forward_id, &fpos, CMPF_INT64)) {
								assert(rList.at(forward_idx) == ex_list.at(fpos));
								if(pStat) {
									PPOsm::SetProcessedNodeStat((uint)ex_count_logic, 1, *pStat);
								}
								actual_count_++;
							}
							else {
								assert(actual_count_);
								if(!actual_count_)
									sr = -1;
								break;
							}
						}
					}
				}
				if(sr < 0) {
					assert(fault_logical_id == 0);
					uint64 outer_id = 0;
					PPOsm::NodeCluster::Put__Param  clu_put_param(p_node, _count - offs);
					PPOsm::NodeCluster::Put__Result clu_put_result;
					PPOsm::NodeCluster * p_cluster = cluster_list.CreateNewItem();
					THROW(p_cluster);
					if(pNodeToWayAsscList && next_node_to_way_assc_pos < pNodeToWayAsscList->getCount()) {
						clu_put_param.P_NrWayRefs = &pNodeToWayAsscList->at(next_node_to_way_assc_pos);
						clu_put_param.NrWayRefsCount = pNodeToWayAsscList->getCount() - next_node_to_way_assc_pos;
					}
					THROW(p_cluster->Put__(clu_put_param, &outer_id, &clu_put_result, 0));
					THROW(outer_id_list.insert(&outer_id));
					actual_count_ = clu_put_result.ActualCount;
					assert(outer_id_list.getCount() == cluster_list.getCount());
					next_node_to_way_assc_pos += clu_put_result.NrWayShift;
					if(pStat) {
						PPOsm::SetNodeClusterStat(*p_cluster, *pStat);
					}
					if(0) { // @debug
						test_list.clear();
						p_cluster->Get(outer_id, test_list, 0 /*NodeRefs*/);
						for(uint i = 0; i < test_list.getCount(); i++) {
							assert(test_list.at(i) == p_node[i]);
						}
					}
				}
				offs += actual_count_;
				if(cluster_list.getCount() >= max_cluster_per_tx) {
					assert(outer_id_list.getCount() == cluster_list.getCount());
					PROFILE_START
					BDbTransaction tra(P_Db, use_transaction);
					THROW(tra);
					for(uint i = 0; i < cluster_list.getCount(); i++) {
						PPOsm::NodeCluster * p_item = cluster_list.at(i);
						uint64 local_outer_id = outer_id_list.at(i);
						THROW(P_GnT->Add(*p_item, local_outer_id));
					}
					THROW(tra.Commit());
					PROFILE_END
					PROFILE(cluster_list.clear());
					outer_id_list.clear();
				}
			}
			assert(offs == _count);
			if(cluster_list.getCount()) {
				assert(outer_id_list.getCount() == cluster_list.getCount());
				PROFILE_START
				BDbTransaction tra(P_Db, use_transaction);
				THROW(tra);
				for(uint i = 0; i < cluster_list.getCount(); i++) {
					PPOsm::NodeCluster * p_item = cluster_list.at(i);
					uint64 local_outer_id = outer_id_list.at(i);
					THROW(P_GnT->Add(*p_item, local_outer_id));
				}
				THROW(tra.Commit());
				PROFILE_END
				PROFILE(cluster_list.freeAll());
			}
			PROFILE(THROW(P_Db->TransactionCheckPoint()));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::StoreGeoNodeWayRefList(const LLAssocArray & rList)
{
	const  uint max_entries_per_tx = 1024;
	const  int  use_transaction = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		int64 prev_node_id = 0;
		PPOsm::NodeCluster nc;
		TSArray <PPOsm::Node> node_list;
		TSArray <PPOsm::Node> node_list_test;
		PPOsm::NodeRefs ref_list;
		PPOsm::NodeRefs ref_list_test;
		for(uint _current_pos = 0; _current_pos < _count;) {
			PROFILE_START
			BDbTransaction tra(P_Db, use_transaction);
			THROW(tra);
			const uint _local_finish = MIN(_count, (_current_pos + max_entries_per_tx));
			for(uint i = _current_pos; i < _local_finish;) {
				const LLAssoc & r_assoc = rList.at(i);
				assert(!i || r_assoc.Key >= prev_node_id); // Тест на упорядоченность входящего массива
				uint64 _logical_id = 0;
				ref_list.Clear();
				node_list.clear();
				const  int sr = P_GnT->Search(r_assoc.Key, &nc, &_logical_id);
				THROW(sr);
				i++;
				if(sr > 0) {
					uint   _force_logical_count = 0;
					uint   _count_actual = 0;
					THROW(nc.Get(_logical_id, node_list, &ref_list, 0, &_force_logical_count, &_count_actual));
					THROW(ref_list.AddWayRef(r_assoc.Key, r_assoc.Val));
					while(i < _count) {
						const LLAssoc & r_assoc_next = rList.at(i);
						uint   nl_pos = 0;
						if(node_list.lsearch(&r_assoc_next.Key, &nl_pos, CMPF_INT64)) {
							THROW(ref_list.AddWayRef(r_assoc_next.Key, r_assoc_next.Val));
							i++;
						}
						else
							break;
					}
					{
						uint64 _outer_id = _logical_id;
						PPOsm::NodeCluster::Put__Param  clu_put_param(&node_list.at(0), node_list.getCount());
						PPOsm::NodeCluster::Put__Result clu_put_result;
						ref_list.Sort();
						clu_put_param.P_NrWayRefs = &ref_list.WayRefs.at(0);
						clu_put_param.NrWayRefsCount = ref_list.WayRefs.getCount();
						clu_put_param.P_NrRelRefs = &ref_list.RelRefs.at(0);
						clu_put_param.NrRelRefsCount = ref_list.RelRefs.getCount();
						THROW(nc.Put__(clu_put_param, &_outer_id, &clu_put_result, _force_logical_count));
						assert(_outer_id == _logical_id);
						assert(clu_put_result.ActualCount == node_list.getCount());
					}
					{
						//
						// Test
						//
						ref_list_test.Clear();
						node_list_test.clear();
						THROW(nc.Get(_logical_id, node_list_test, &ref_list_test));
						assert(node_list_test.IsEqual(node_list));
						assert(ref_list_test.IsEqual(ref_list));

					}
					THROW(P_GnT->Update(nc, _logical_id));
				}
				else {
					// @todo log message (точка не найдена)
				}
			}
			_current_pos = _local_finish;
			THROW(tra.Commit());
			PROFILE_END
		}
		PROFILE(THROW(P_Db->TransactionCheckPoint()));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrDatabase::StoreGeoWayList(const TSCollection <PPOsm::Way> & rList, TSArray <PPOsm::WayStatEntry> * pStat)
{
	const  uint max_entries_per_tx = 1024;
	const  int  use_transaction = 1;
	int    ok = 1;
	const  uint _count = rList.getCount();
	if(_count) {
		PPOsm::WayBuffer way_buf;
        for(uint _current_pos = 0; _current_pos < _count;) {
			PROFILE_START
			BDbTransaction tra(P_Db, use_transaction);
			THROW(tra);
			const uint _local_finish = MIN(_count, (_current_pos + max_entries_per_tx));
			for(uint i = _current_pos; i < _local_finish; i++) {
				PPOsm::Way * p_way = rList.at(i);
				assert(p_way);
				if(p_way) {
					PPOsm::Way found_way;
					if(P_GwT->Search(p_way->ID, &found_way) > 0) {
						assert(found_way == *p_way);
						if(pStat) {
							PPOsm::SetProcessedWayStat(found_way.NodeRefs.getCount(), 1, *pStat);
						}
					}
					else {
						THROW(P_GwT->Add(*p_way, &way_buf));
						if(pStat) {
							PPOsm::SetWayStat(way_buf, *pStat);
						}
					}
				}
			}
			_current_pos = _local_finish;
			THROW(tra.Commit());
			PROFILE_END
		}
		PROFILE(THROW(P_Db->TransactionCheckPoint()));
	}
	else
		ok = -1;
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
						StrAssocArray::Item item = test_list.at(pos_list.get(i));
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
						StrAssocArray::Item item = test_list.at(pos_list.get(i));
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

class STextStream {
public:
	enum {
		tUnkn = 0,
		tEtc,        // Любой символ, не относящийся к перечисленным далее типам (#$@/\ etc)
		tSpace,      // Пробел, табуляция, перевод строки //
		tPunct,      // .,;:
		tHyphen,     // -
		tNumber,     // Сплошная последовательность цифр (точка, запятая, минус не включаются и должны быть разрешены на следующем уровне).
			// Полная последовательность хранится в списке NList в виде целого числа по индексу Token::W.
		tUnknCode,   // Сплошная последовательность цифр и букв, которая не может быть распознана как хорошо формализованный код (EAN, UPC, ИНН и т.д.)
		tUnresWord,  // Просто слово, неразрешенное с помощью словаря.
		tWord        // Слово, разрешенное посредством словаря.
	};
	struct Token {
		Token & SetPTW(uint32 p, int16 t, uint32 w)
		{
			P = p;
			T = t;
			Reserve = 0;
			W = w;
			return *this;
		}
		uint32 P; // Позиция во входном потоке
		int16  T; // tXXX
		int16  Reserve; // @alignment
		uint32 W; // Идентификатор символа
	};
	STextStream();
	int    Parse(const char * pTextUtf8);
	int    Parse(const wchar_t * pTextU);
	int    Parse(const char * pMbText, int codePage);
private:
	int    Helper_Parse();
	int    FASTCALL AddTok(Token & rTok);
	int    AddSymb(SStringU & rSymb, uint32 * pPos);

	uint   SymbCounter;
	SymbHashTable H;
	TSArray <Token> List;
	SStringU OrgText;
};

STextStream::STextStream() : H(128*1024, 0)
{
	SymbCounter = 0;
}

int FASTCALL STextStream::AddTok(Token & rTok)
{
	if(rTok.T > 0) {
		List.insert(&rTok);
		MEMSZERO(rTok);
		return 1;
	}
	else
		return -1;
}

int STextStream::AddSymb(SStringU & rSymb, uint32 * pPos)
{
	int    ok = 1;
	SString utf8_buf;
	rSymb.ToLower().CopyToUtf8(utf8_buf, 1);
	uint   val = 0, pos = 0;
	if(!H.Search(utf8_buf, &val, &pos)) {
		val = ++SymbCounter;
		H.Add(utf8_buf, val, &pos);
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int STextStream::Parse(const char * pTextUtf8)
{
	OrgText = 0;
	OrgText.CopyFromUtf8Strict(pTextUtf8, strlen(pTextUtf8));
	return Helper_Parse();
}

int STextStream::Helper_Parse()
{
	int    ok = 1;
	const  uint len = OrgText.Len();
	uint   line_no = 1;
	SStringU temp_buf;
	Token tok;
	MEMSZERO(tok);
	List.clear();
	for(uint i = 0; i < len; i++) {
		wchar_t c = OrgText.C(i);
		switch(c) {
			case L' ':
				break;
			case L'\t':
				break;
			case L'\n':
				line_no++;
				break;
			case L'.':
			case L',':
			case L';':
			case L':':
				AddTok(tok.SetPTW(i, tPunct, c));
				break;
			case L'-':
				AddTok(tok);
				tok.SetPTW(i, tHyphen, c);
				break;
			default:
				{
					temp_buf = 0;
					const  uint start_pos = i;
					if(iswdigit(c)) {
						int    dig_only = 1;
						do {
							temp_buf.CatChar(c);
							c = OrgText.C(++i);
							if(iswalpha(c))
								dig_only = 0;
							else if(!iswdigit(c)) {
								c = OrgText.C(--i); // Возвращаем назад позицию буфера для дальнейшего анализа
								break;
							}
						} while(1);
						tok.P = start_pos;
						tok.T = dig_only ? tNumber : tUnknCode;
						AddSymb(temp_buf, &tok.W);
						AddTok(tok);
					}
					else if(iswalpha(c)) {
						int    alpha_only = 1;
						do {
							temp_buf.CatChar(c);
							c = OrgText.C(++i);
							if(iswdigit(c))
								alpha_only = 0;
							else if(!iswalpha(c)) {
								c = OrgText.C(--i); // Возвращаем назад позицию буфера для дальнейшего анализа
								break;
							}
						} while(1);
						tok.P = start_pos;
						tok.T = alpha_only ? tUnresWord : tUnknCode;
						AddSymb(temp_buf, &tok.W);
						AddTok(tok);
					}
					else {
						tok.SetPTW(start_pos, tEtc, c);
						AddTok(tok);
					}
				}
				break;
		}
	}
	return ok;
}

int SLAPI ParseText(const char * pText)
{
	return 0;
}

/*
int SLAPI ParseIcu(const char * pFileName)
{

}
*/

class SrConceptParser {
public:
	class Operator {
	public:
		Operator(SrConceptParser & rMaster);
		Operator & Clear();
		int    IsEmpty() const;
		int    Close(int ifNeeded);
		int    IsClosed() const;
		Operator * CreateNext();
		Operator * CreateChild();
		Operator * FindParent();

		enum {
			fClosed   = 0x0001, // Оператор закрыт (то есть считана полная конструкция оператора).
			fAccepted = 0x0002  // Оператор учтен в базе данных вызовом SrConceptParser::PostprocessOpList(Operator *)
		};
		SrConceptParser & R_Master; // @anchor
		CONCEPTID  CID;
		CONCEPTID  InstanceOf; // Концепция this является 'кземпляром концепции SubclassOf
		CONCEPTID  SubclassOf; // Концепция this является подклассом концепции SubclassOf
		int    CrType;         // Концепция this имеет тип CrType
		LEXID  CLexID;
		NGID   NgID;
		int16  LangID;
		int16  Flags;
		Int64Array EqToList;
		SrCPropDeclList Pdl;
		//
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
		tokTypeHDate,       // #hdate
		tokTypeHPeriod      // #hperiod
	};

	static int FASTCALL _IsWordbreakChar(int c);

	SrConceptParser(SrDatabase & rDb);
	~SrConceptParser();
	int    Run(const char * pFileName);
	int    _ReadLine();
	int    FASTCALL _GetToken(SString & rExtBuf);
	int    _SkipSpaces(int * pToken, SString & rExtBuf);
	int    FASTCALL _IsTypeToken(int token) const;
	int    FASTCALL _GetTypeByToken(int token) const;
	int    FASTCALL _IsIdent(const char * pText) const;
	LEXID  FASTCALL RecognizeWord(const char *);
	NGID   FASTCALL RecognizeNGram(const LongArray & rWordList);
	CONCEPTID FASTCALL RecognizeConcept(const char *);
	int    PostprocessOpList(Operator * pRoot);
	int    ApplyConceptPropList(const StrAssocArray & rTokList, CONCEPTID cid);

	SrCPropList PropList;

	int OpenInput(const char * pFileName)
	{
		CloseInput();
		LineBuf = 0;
		LineNo = 0;
		if(F.Open(pFileName, SFile::mRead)) {
			return 1;
		}
		else
			return 0;
	}
	void CloseInput()
	{
		F.Close();
		//LineBuf = 0;
		//LineNo = 0;
	}

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
	Clear();
}

SrConceptParser::Operator & SrConceptParser::Operator::Clear()
{
	CID = 0;
	InstanceOf = 0;
	SubclassOf = 0;
	CrType = 0;
	CLexID = 0;
	NgID = 0;
	LangID = 0;
	Flags = 0;
	P_Parent = 0;
	P_Child = 0;
	P_Next = 0;
	P_Prev = 0;
	Pdl.Clear();
	EqToList.clear();
	return *this;
}

int SrConceptParser::Operator::IsEmpty() const
{
	return (!CID && !CLexID && !NgID && !LangID && !P_Child) ? 1 : 0;
}

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

int SrConceptParser::Operator::IsClosed() const
{
	return BIN(Flags & fClosed);
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
SrConceptParser::SrConceptParser(SrDatabase & rDb) : R_Db(rDb)
{
	LineNo = 0;
	ReH_BrDescr = 0;
	ReH_Concept = 0;
	ReH_SubclsOf = 0;
	ReH_ExprOf = 0;
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

//static
int FASTCALL SrConceptParser::_IsWordbreakChar(int c)
{
	const char * p_wb = " \t{}[]()<>,.;:!@#%^&*=+`~'?№/|\\";
	return (c == 0 || strchr(p_wb, c));
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
#define _ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

	char c = *pText++;
	if(_ISALPHA(c) || c == '_') {
		do {
			c = *pText++;
			if(c && !(_ISALPHA(c) || c == '_' || (c >= '0' && c <= '9')))
				return 0;
		} while(c);
		return 1;
	}
	else
		return 0;
#undef _ISALPHA
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
	{ SRPROPT_HDATE,   SrConceptParser::tokTypeHDate,   "hdate" },
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
	return 0;
}

int FASTCALL SrConceptParser::_GetToken(SString & rExtBuf)
{
	rExtBuf = 0;

	int    tok = 0;
	char   c = Scan[0];
	switch(c) {
		case 0:
			tok = tokEnd;
			break;
		case ' ':
		case '\t':
			do {
				Scan.Incr();
				c = Scan[0];
			} while(oneof2(c, ' ', '\t'));
			tok = tokSpace;
			break;
		case '{':
			tok = tokLBrace;
			Scan.Incr();
			break;
		case '}':
			tok = tokRBrace;
			Scan.Incr();
			break;
		case '(':
			tok = tokLPar;
			Scan.Incr();
			break;
		case ')':
			tok = tokRPar;
			Scan.Incr();
			break;
		case ';':
			tok = tokSemicol;
			Scan.Incr();
			break;
		case '.':
			tok = tokDot;
			Scan.Incr();
			break;
		case ',':
			tok = tokComma;
			Scan.Incr();
			break;
		case '#':
			{
				Scan.Incr();
				for(uint i = 0; i < SIZEOFARRAY(_CTypeSymbList); i++) {
					if(Scan.Is(_CTypeSymbList[i].P_Symb)) {
						tok = _CTypeSymbList[i].Token;
						Scan.Incr(strlen(_CTypeSymbList[i].P_Symb));
						break;
					}
				}
				if(!tok)
					tok = tokSharp;
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
						if(Scan[1] == ':') {
							tok = tokEqColon;
							Scan.Incr(2);
						}
						else {
							tok = tokEq;
							Scan.Incr();
						}
						break;
					case '[':
 						tok = tokLBracket;
 						Scan.Incr();
						break;
					case ']':
						tok = tokRBracket;
						Scan.Incr();
						break;
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
	CONCEPTID prop_instance = R_Db.GetReservedConcept(R_Db.rcInstance);
	CONCEPTID prop_subclass = R_Db.GetReservedConcept(R_Db.rcSubclass);
	CONCEPTID prop_crtype   = R_Db.GetReservedConcept(R_Db.rcType);
	THROW(prop_instance);
	THROW(prop_subclass);
	THROW(prop_crtype);
	for(Operator * p_current = pRoot; p_current; p_current = p_current->P_Next) {
		if(p_current->Flags & p_current->fClosed && !(p_current->Flags & p_current->fAccepted)) {
			if(p_current->CID) {
				if(p_current->Pdl.GetCount()) {
					SrConcept cp;
					THROW(R_Db.P_CT->SearchByID(p_current->CID, &cp) > 0);
					THROW(cp.Pdl.Merge(p_current->Pdl));
					THROW(R_Db.P_CT->Update(cp));
				}
				if(p_current->InstanceOf) {
					SrCProp cp(p_current->CID, prop_instance);
					cp = p_current->InstanceOf;
					THROW(R_Db.P_CpT->Set(cp));
				}
				if(p_current->SubclassOf) {
					SrCProp cp(p_current->CID, prop_subclass);
					cp = p_current->SubclassOf;
					THROW(R_Db.P_CpT->Set(cp));
				}
				if(p_current->CrType) {
					SrCProp cp(p_current->CID, prop_crtype);
					cp = p_current->CrType;
					THROW(R_Db.P_CpT->Set(cp));
				}
			}
			if(p_current->EqToList.getCount()) {
				if(p_current->NgID) {
					for(uint i = 0; i < p_current->EqToList.getCount(); i++) {
						CONCEPTID cid = p_current->EqToList.get(i);
						if(cid) {
							THROW(R_Db.P_CNgT->Set(cid, p_current->NgID));
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
			if(_IsIdent(temp_buf)) {
				ident_buf = temp_buf;
				{
					THROW(i < rTokList.getCount()); // Неожиданное завершение файла
					titem = rTokList.at_WithoutParent(i++);
					tok = titem.Id;
					temp_buf = titem.Txt;
				}
				if(tok == tokEq) {
					if(R_Db.SearchWord(SrWordTbl::spcCPropSymb, ident_buf, &symb_id) > 0) {
						{
							THROW(i < rTokList.getCount()); // Неожиданное завершение файла
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
								THROW(word_id);
								ngram.add(word_id);
								{
									THROW(i < rTokList.getCount()); // Неожиданное завершение файла
									titem = rTokList.at_WithoutParent(i++);
									tok = titem.Id;
									temp_buf = titem.Txt;
								}
								if(tok == tokSpace) {
									THROW(i < rTokList.getCount()); // Неожиданное завершение файла
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
				; // @error Ожидается идентификатор свойства
			}
		}
		else {
			; // @error Ожидается идентификатор свойства или концепт
		}
		{
			THROW(i < rTokList.getCount()); // Неожиданное завершение файла
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
			; // @error Ожидается скобка ')' или запятая ','
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
	SString temp_buf, ident_buf;
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
			do {
				ngram.clear();
				prev_tok = tok;
				tok = _GetToken(temp_buf);
				if(tok == tokWord) {
					NGID   ngram_id = 0;
					do {
						LEXID word_id = 0;
						THROW(R_Db.ResolveWord(temp_buf, &word_id));
						if(word_id)
							ngram.add(word_id);
						else {
							; // @error
						}
						prev_tok = tok;
						tok = _GetToken(temp_buf);
						if(tok == tokSpace) {
							prev_tok = tok;
							tok = _GetToken(temp_buf);
						}
					} while(tok == tokWord);
					THROW(R_Db.ResolveNGram(ngram, &ngram_id));
					if(ngram_id) {
						p_current->NgID = ngram_id;
					}
					else {
						; // @error
					}
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
									if(!p_child->LangID)
										p_child->LangID = p_current->LangID;
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
								if(p_current->NgID && !p_current->CID) {
									Int64Array _clist;
									if(R_Db.GetNgConceptList(p_current->NgID, R_Db.ngclAnonymOnly, _clist) > 0) {
										assert(_clist.getCount());
										p_current->CID = _clist.get(0);
									}
									else {
										THROW(R_Db.CreateAnonymConcept(&p_current->CID));
										THROW(R_Db.P_CNgT->Set(p_current->CID, p_current->NgID));
									}
									p_current->InstanceOf = cid;
								}
							}
							else if(prev_tok == tokRBrace) {
								Int64Array _clist;
								for(Operator * p_child = p_current->P_Child; p_child; p_child = p_child->P_Next) {
									if(!p_child->InstanceOf) {
										if(p_child->NgID && !p_child->CID) {
											_clist.clear();
											if(R_Db.GetNgConceptList(p_child->NgID, R_Db.ngclAnonymOnly, _clist) > 0) {
												assert(_clist.getCount());
												p_child->CID = _clist.get(0);
											}
											else {
												THROW(R_Db.CreateAnonymConcept(&p_child->CID));
												THROW(R_Db.P_CNgT->Set(p_child->CID, p_child->NgID));
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
							if(p_current->IsClosed()) {
								; // @error
							}
							else {
								if(p_current->SubclassOf) {
									; // @error
								}
								else {
									THROW(R_Db.ResolveConcept(temp_buf, &p_current->SubclassOf));
									if(!p_current->SubclassOf) {
										; // @error
									}
									p_current->Close(0);
									THROW(p_current = p_current->CreateNext());
								}
							}
						}
						break;
					case tokLPar:
						{
							temp_token_list.Clear();
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
									temp_token_list.Add(tok, temp_buf, -1);
								}
								else if(tok == tokWord) {
									if(_IsIdent(temp_buf)) {
										ident_buf = temp_buf;
										THROW(_SkipSpaces(&tok, temp_buf)); // Неожиданное завершение файла
										temp_token_list.Add(tok, temp_buf, -1);
										if(tok == tokEq) {
											THROW(_SkipSpaces(&tok, temp_buf)); // Неожиданное завершение файла
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
										CALLEXCEPT_PP_S(PPERR_SR_C_PROPIDEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
									}
								}
								else {
									CALLEXCEPT_PP_S(PPERR_SR_C_PROPIDORCONCEXPECTED, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
								}
								THROW(_SkipSpaces(&tok, temp_buf)); // Неожиданное завершение файла
								temp_token_list.Add(tok, temp_buf, -1);
								if(tok == tokRPar)
									do_get_next_prop = 0;
								else if(tok == tokComma) {
									do_get_next_prop = 1;
								}
								else {
									; // @error Ожидается скобка ')' или запятая ','
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
						if(p_current->CID) {
							SrCPropDeclList pdl;
							for(int do_get_next_decl = 1; do_get_next_decl;) {
								int    ss = 0;
								do_get_next_decl = 0;
								prev_tok = tok;
								ss = _SkipSpaces(&tok, temp_buf);
								if(tok == tokConcept) {
									SrCPropDecl pd;
									THROW(R_Db.ResolveConcept(temp_buf, &pd.PropID));
									prev_tok = tok;
									ss = _SkipSpaces(&tok, temp_buf);
									if(ss) {
										if(tok == tokWord) {
											if(_IsIdent(temp_buf)) {
												THROW(R_Db.ResolveCPropSymb(temp_buf, &pd.SymbID));
												THROW(pdl.Add(pd));
												ss = _SkipSpaces(&tok, temp_buf);
												if(tok == tokComma) {
													do_get_next_decl = 1;
												}
												else if(tok == tokRPar) {
												}
												else {
													; // @error
												}
											}
											else {
												; // @error
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
								else {
									; // @error
								}
							}
							THROW(R_Db.P_CT->SetPropDeclList(p_current->CID, &pdl));
						}
						else {
							; // @error
						}
						break;
					case tokExprOf:
						{
							if(p_current->IsClosed()) {
								; // @error
							}
							else {
								CONCEPTID cid = 0;
								THROW(R_Db.ResolveConcept(temp_buf, &cid));
								if(!cid) {
									; // @error
								}
								else {
									if(prev_tok == tokWord) {
										p_current->EqToList.add(cid);
									}
									else if(prev_tok == tokConcept) {
										p_current->EqToList.add(cid);
									}
									else if(prev_tok == tokRBrace) {
										for(Operator * p_child = p_current->P_Child; p_child; p_child = p_child->P_Next) {
											p_child->EqToList.add(cid);
										}
									}
									else if(prev_tok == tokSpace) {
										Operator * p_parent = p_current->FindParent();
										if(p_parent && (p_parent->NgID || p_parent->CID)) {
											p_parent->EqToList.add(cid);
										}
									}
									else {
										; // @error
									}
								}
							}
						}
						break;
				}
			} while(!finish);
			THROW(PostprocessOpList(&root));
		}
		THROW(tra.Commit());
		THROW(R_Db.P_Db->TransactionCheckPoint()); // @v9.7.8
	}
	CATCHZOK
	CloseInput();
	return ok;
}
//
//
//
struct GeoName {
	GeoName & Reset()
	{
		ID = 0;
		Name = 0;
		Latitude = 0.0;
		Longitude = 0.0;
		FeatureCode[0] = 0;
		CountryCode[0] = 0;
		return *this;
	}
	long   ID;
	SString Name;
	double Latitude;
	double Longitude;
	char   FeatureClass[4]; // class
	char   FeatureCode[64]; // code
	char   CountryCode[16];
};

struct GeoFeatureCode {
	GeoFeatureCode & Reset()
	{
		Code = 0;
		Text = 0;
		Descr = 0;
		return *this;
	}
	SString Code;
	SString Text;
	SString Descr;
};

struct GeoNameAlt {
	GeoNameAlt & Reset()
	{
		ID = 0;
		LinguaCode[0] = 0;
		Text[0] = 0;
		return *this;
	}
	long   ID;
	char   LinguaCode[16];
	char   Text[512];
};

static SString & FASTCALL _CatSartrEntityPrefix(const char * pEntity, SString & rBuf)
{
	return rBuf.CatChar(':').Cat(pEntity).CatChar('_');
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
			outf.WriteLine((out_buf = 0).CR().CatChar('{').CR());
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
				(out_buf = 0).Tab().CatChar(':').Cat("geoloct").CatChar('_').Cat(entry.Code).CR();
				outf.WriteLine(out_buf);
			}
			outf.WriteLine((out_buf = 0).CatChar('}').CatChar(':').Cat("geoloctype").CR());
		}
		{
			LongArray ling_list;
			GetLinguaList(ling_list);
			for(uint i = 0; i < ling_list.getCount(); i++) {
				GetLinguaCode(ling_list.get(i), temp_buf);
				temp_buf.ToUtf8().ToLower();
				(in_file_name = pPath).SetLastSlash().Cat("featureCodes").CatChar('_').Cat(temp_buf).CatChar('.').Cat("txt");
				if(fileExists(in_file_name)) {
					SFile inf(in_file_name, SFile::mRead);
					THROW(inf.IsValid());

					(out_buf = 0).CR().CatBrackStr(temp_buf).CatChar('{').CR();
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
						(out_buf = 0).Tab().Cat(entry.Text).Cat("=:").Cat("geoloct").CatChar('_').Cat(entry.Code).CR();
						outf.WriteLine(out_buf);
					}
					(out_buf = 0).CatChar('}').CR();
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
		outf.WriteLine((out_buf = 0).CR());
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
			_CatSartrEntityPrefix("geoloc", out_buf = 0).Cat(entry.ID);
			out_buf.CatChar(':').Cat("geoloc");
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
				outf.WriteLine((out_buf = 0).CR());
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
									(out_buf = 0).CatChar('[').Cat(r_entry.LinguaCode).CatChar(']').Cat(r_entry.Text).Cat("=:").Cat("geoloc").CatChar('_').Cat(r_entry.ID).CR();
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
									(temp_ubuf = 0).CopyFromUtf8Strict(temp_buf, temp_buf.Len());
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
										dest_ubuf.CopyToUtf8(temp_buf = 0, 1);
										(out_buf = 0).CatBrackStr("en").Cat(temp_buf).Cat("=:").Cat("geoloc").CatChar('_').Cat(r_entry.ID).CR();
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

int SrDatabase::ImportNames(const SrImportParam & rParam)
{
    int    ok = 1;
	SString src_file_name;
	rParam.GetField(rParam.fldAncodeFileName, src_file_name);
    return ok;
}

IMPLEMENT_PPFILT_FACTORY(PrcssrSartre); SLAPI PrcssrSartreFilt::PrcssrSartreFilt() : PPBaseFilt(PPFILT_PRCSSRSARTREPARAM, 0, 0)
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

int SLAPI PrcssrSartreFilt::IsEmpty() const
{
	if(Flags)
		return 0;
	else if(SrcPath.NotEmpty())
		return 0;
	else
		return 1;
}

class PrcssrSartreFiltDialog : public TDialog {
public:
	PrcssrSartreFiltDialog() : TDialog(DLG_PRCRSARTR)
	{
	}
	int    setDTS(const PrcssrSartreFilt * pData)
	{
		int    ok = 1;
		Data = *pData;
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 0, Data.fImportFlexia);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 1, Data.fImportConcepts);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 2, Data.fImportHumNames);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 3, Data.fTestFlexia);
		AddClusterAssoc(CTL_PRCRSARTR_FLAGS, 4, Data.fTestConcepts);
		SetClusterData(CTL_PRCRSARTR_FLAGS, Data.Flags);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_PRCRSARTR_SRCPATH, CTL_PRCRSARTR_SRCPATH, 1, 0,
			0, FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		setCtrlString(CTL_PRCRSARTR_SRCPATH, Data.SrcPath);
		return ok;
	}
	int    getDTS(PrcssrSartreFilt * pData)
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_PRCRSARTR_FLAGS, &Data.Flags);
		getCtrlString(sel = CTL_PRCRSARTR_SRCPATH, Data.SrcPath);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	PrcssrSartreFilt Data;
};

SLAPI PrcssrSartre::PrcssrSartre(const char * pDbPath)
{
}

SLAPI PrcssrSartre::~PrcssrSartre()
{
}

int SLAPI PrcssrSartre::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	if(P.IsA(pBaseFilt)) {
		PrcssrSartreFilt * p_filt = (PrcssrSartreFilt *)pBaseFilt;
		if(p_filt->IsEmpty()) {
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PrcssrSartre::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(P.IsA(pBaseFilt));
	P = *(PrcssrSartreFilt *)pBaseFilt;
	CATCHZOK
	return ok;
}

int SLAPI PrcssrSartre::EditParam(PPBaseFilt * pBaseFilt)
{
	if(!P.IsA(pBaseFilt))
		return 0;
	PrcssrSartreFilt * p_filt = (PrcssrSartreFilt *)pBaseFilt;
	DIALOG_PROC_BODY(PrcssrSartreFiltDialog, p_filt);
}

int SLAPI PrcssrSartre::TestSearchWords()
{
	int    ok = 1;
	SString line_buf, temp_buf;
	TSArray <SrWordInfo> info_list;
	SrDatabase db;
	THROW(db.Open(0));
	{
		PPGetFilePath(PPPATH_OUT, "Sartr_TestSearchWords.txt", temp_buf);
		SFile out_file(temp_buf, SFile::mWrite);
		{
			const char * p_words[] = {
				"ЧАЙКА",
				"СЕМЬЮ",
				"ЁЖИК",
				"ПЕТРОПАВЛОВСКАЯ",
				"Петропавловские",
				"МАРКСА",
				"ТЕХНОЛОГИЧЕСКУЮ",
				"ТЕХНОЛОГИЧЕСКИЕ",
				"МОРОЗОУСТОЙЧИВОГО",
				"КАК-БЫ",
				"СОБОЛЕВЫХ",
				"МАКСИМЕ",
				"ПОМЕНЬШЕ",
				"ЗЕЛЕНОГЛАЗУЮ",
				"КРАСИВОГЛАЗАЯ",
				"ПРЕДЛОЖЕНИЕМ",
				"СУПЕРПРЕДЛОЖЕНИЕМ",
				"ПОГОВОРИЛ",
				"РАСТОПЛЮ",
				"ОТЫГРАЕШЬСЯ",
				"ПРОВОНЯЛИ",
				"МЫ",
				"ОНИ",
				"Я",
				"ОПЯТЬ",
				"ТРИДЦАТИ",
				"БЛЯДЯМ",
				"ПЕРВЫЙ",
				"ПЕРВОГО",
				"ВТОРОМ",
				"ТРЕТЬЕГО",
				"СВЕРХПРОВОДИМОСТЬЮ",
				"СВЕРХПРОВОДНИКОВЫМ",
				"ГЕОГИЕВИЧУ",
				"САВВИШНА",
				"ПОДУМАЮ",
				"ХОТЯТ",

				"algorithms",
				"management",
				"going",
				"damn"
			};
			for(uint i = 0; i < SIZEOFARRAY(p_words); i++) {
				temp_buf = p_words[i];
				temp_buf.ToUtf8();
				info_list.clear();
				if(db.GetWordInfo(temp_buf, 0, info_list) > 0) {
					for(uint j = 0; j < info_list.getCount(); j++) {
						db.WordInfoToStr(info_list.at(j), temp_buf);
						temp_buf.Utf8ToChar();
						(line_buf = p_words[i]).Cat("\t-->\t").Cat(temp_buf).CR();
						out_file.WriteLine(line_buf);
					}
				}
				else {
					(line_buf = p_words[i]).Cat("\t-->\t").Cat("not found").CR();
					out_file.WriteLine(line_buf);
				}
			}
		}
		out_file.WriteLine(0);
		{
			const char * p_word = "Я"; /*"ТЕХНОЛОГИЧЕСКАЯ";*/ /*"КОНСТАНТИНОВИЧУ";*/
			SrWordForm wf;
			temp_buf = p_word;
			temp_buf.ToUtf8();
			info_list.clear();
			wf.SetTag(SRWG_CASE, SRCASE_DATIVE);
			wf.SetTag(SRWG_COUNT, SRCNT_PLURAL);
			if(db.Transform(temp_buf, &wf, info_list) > 0) {
				for(uint j = 0; j < info_list.getCount(); j++) {
					db.WordInfoToStr(info_list.at(j), temp_buf);
					temp_buf.Utf8ToChar();
					(line_buf = p_word).Cat("\t-->\t").Cat(temp_buf).CR();
					out_file.WriteLine(line_buf);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrSartre::TestConcept()
{
	int    ok = 1;
	SString line_buf, temp_buf, symb;
	SrDatabase db;
	THROW(db.Open(0));
	{
		const char * p_words[] = {
			"россия",
			"неодим",
			"рубль",
			"москва",
			"вологодская область",
			"аризона",
			"широта",
			"долгота",
			"петрозаводск",
			"дарья",
			"максим"
		};
		PPGetFilePath(PPPATH_OUT, "Sartr_TestConcept.txt", temp_buf);
		SFile out_file(temp_buf, SFile::mWrite);
		StringSet tok_list;
		for(uint i = 0; i < SIZEOFARRAY(p_words); i++) {
			temp_buf = p_words[i];
			(line_buf = 0).Cat(temp_buf).CR();
			tok_list.clear(1);
			temp_buf.Tokenize(0, tok_list);
			int    unkn_word = 0;
			LongArray ng;
			for(uint sp = 0; !unkn_word && tok_list.get(&sp, temp_buf);) {
				temp_buf.ToUtf8();
				LEXID word_id = 0;
				if(db.SearchWord(0, temp_buf, &word_id) > 0)
					ng.add(word_id);
				else
					unkn_word = 1;
			}
			if(!unkn_word) {
				NGID  ng_id = 0;
				if(db.SearchNGram(ng, &ng_id) > 0) {
					Int64Array clist, hlist;
					if(db.GetNgConceptList(ng_id, 0, clist) > 0) {
						for(uint j = 0; j < clist.getCount(); j++) {
							CONCEPTID cid = clist.get(j);
							SrCPropList cpl;
							SrCProp cp;
							db.GetConceptSymb(cid, symb);
							line_buf.Tab().Cat((temp_buf = symb).Utf8ToChar());
							line_buf.CatChar('(');
							if(db.GetConceptPropList(cid, cpl) > 0) {
								for(uint k = 0; k < cpl.GetCount(); k++) {
									if(cpl.GetProp(k, cp)) {
										db.FormatProp(cp, 0, temp_buf);
										if(k)
											line_buf.CatDiv(',', 2);
										line_buf.Cat(temp_buf);
									}
								}
							}
							line_buf.CatChar(')');
							if(db.GetConceptHier(cid, hlist) > 0 && hlist.getCount()) {
								line_buf.Space().CatDiv(':', 2);
								for(uint k = 0; k < hlist.getCount(); k++) {
									CONCEPTID hcid = hlist.get(k);
									db.GetConceptSymb(hcid, symb);
									if(k)
										line_buf.CatDiv(',', 2);
									line_buf.Cat((temp_buf = symb).Utf8ToChar());
								}
							}
							line_buf.CR();
						}
					}
				}
			}
			line_buf.CR();
			out_file.WriteLine(line_buf);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrSartre::Run()
{
	int    ok = 1;
	//const char * p_src_data_path = "\\PAPYRUS\\Src\\SARTR\\data";
	SString temp_buf;
	SString msg_buf;
	SString src_file_name;
	PPLogger logger;
	PPWait(1);
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
			THROW(db.Open(/*sartre_db_path*/0));
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
			THROW(db.Open(/*sartre_db_path*/0));
			SrConceptParser parser(db);
			(src_file_name = P.SrcPath).SetLastSlash().Cat("concept.txt");
			if(!parser.Run(src_file_name)) {
				//PPGetMessage(mfError, )
				PPGetLastErrorMessage(1, temp_buf);
				(msg_buf = src_file_name).CatChar('(').Cat(parser.LineNo).CatChar(')').Space().Cat(temp_buf);
				logger.Log(msg_buf);
			}
		}
		if(P.Flags & P.fImportHumNames) {
			;
		}
		if(P.Flags & P.fTestFlexia) {
			TestSearchWords();
		}
		if(P.Flags & P.fTestConcepts) {
			TestConcept();
		}
		/*if(!TestImport_Words_MySpell())
			ret = -1;*/
		/*if(!TestImport_AncodeCollection())
			ret = -1;*/
	}
	CATCH
		logger.LogLastError();
		ok = 0;
	ENDCATCH
	PPWait(0);
	return ok;
}

int SLAPI DoProcessSartre(PrcssrSartreFilt * pFilt)
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
int SLAPI ImportSartre()
{
	int    ok = 1;
	//const  char * p_db_path = "/PAPYRUS/PPY/BIN/SARTRDB";
	//PPIniFile ini_file;
	const char * p_src_data_path = "\\PAPYRUS\\Src\\SARTR\\data";
	//SString sartre_db_path;
	SString temp_buf;
	PPWait(1);
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
	PPWait(0);
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
