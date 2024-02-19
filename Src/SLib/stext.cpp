// STEXT.CPP
// Copyright (c) A.Sobolev 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
//
// Преобразование символов и строк, и другие текстовые функции
//
#include <slib-internal.h>
#pragma hdrstop
#include <uchardet.h>
#include <unicode/ucsdet.h>

const char * SlTxtOutOfMem = "Out of memory"; // @v11.3.12

static const char * p_dow_en_sh[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
static const char * p_mon_en_sh[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char * p_dow_en[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
static const char * p_asciictrl[] = {
	"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS",  "HT", "LF",  "VT",  "FF", "CR", "SO", "SI",
	"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US", "DEL" // @v11.9.1 "DEL"
};
static const char * p_base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// The Base 64 encoding with an URL and filename safe alphabet, RFC 4648 section 5
static const char * p_base64url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char * p_alphabet_eng_l = "abcdefghijklmnopqrstuvwxyz";
static const char * p_alphabet_eng_u = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*static*/const char * FASTCALL STextConst::Get(int c, uint idx)
{
	switch(c) {
		case cMon_En_Sh: return (idx >= 0 && idx < SIZEOFARRAY(p_mon_en_sh)) ? p_mon_en_sh[idx] : "";
		case cDow_En_Sh: return (idx >= 0 && idx < SIZEOFARRAY(p_dow_en_sh)) ? p_dow_en_sh[idx] : "";
		case cDow_En: return (idx >= 0 && idx < SIZEOFARRAY(p_dow_en)) ? p_dow_en[idx] : "";
		case cAsciiCtrl: return (idx >= 0 && idx < SIZEOFARRAY(p_asciictrl)) ? p_asciictrl[idx] : "";
		case cBasis64: return p_base64;
		case cBasis64Url: return p_base64url;
		case cAlphabetEngL: return p_alphabet_eng_l;
		case cAlphabetEngU: return p_alphabet_eng_u;
		default: return "";
	}
}

/*static*/const char * STextConst::P_Digits = "0123456789";
/*static*/const wchar_t * STextConst::P_DigitsW = L"0123456789";

/*static*/const char * FASTCALL STextConst::GetBool(int b) { return b ? "true" : "false"; }

/*static*/int FASTCALL STextConst::GetIdx(int c, const char * pText)
{
	uint i = 0;
	switch(c) {
		case cMon_En_Sh:
			for(; i < SIZEOFARRAY(p_mon_en_sh); i++)
				if(sstreqi_ascii(pText, p_mon_en_sh[i]))
					return static_cast<int>(i);
			break;
		case cDow_En_Sh:
			for(; i < SIZEOFARRAY(p_dow_en_sh); i++)
				if(sstreqi_ascii(pText, p_dow_en_sh[i]))
					return static_cast<int>(i);
			break;
		case cDow_En:
			for(; i < SIZEOFARRAY(p_dow_en); i++)
				if(sstreqi_ascii(pText, p_dow_en[i]))
					return static_cast<int>(i);
			break;
		case cAsciiCtrl:
			for(; i < SIZEOFARRAY(p_asciictrl); i++)
				if(sstreqi_ascii(pText, p_asciictrl[i]))
					return static_cast<int>(i);
			break;
	}
	return -1;
}
//
//
//
struct LinguaIdent {
	int16  Id;
	uint8  WinLangId;
	const char * P_Code;
};

static const LinguaIdent P_LinguaIdentList[] = {
	{ slangMeta,   0, "meta" }, // meta-language (not standard)
	{ slangAA,     0, "aa"   }, // Afar
	{ slangAB,     0, "ab"   }, // Abkhazian
	{ slangACE,    0, "ace"  }, // Achinese
	{ slangACH,    0, "ach"  }, // Acoli
	{ slangADA,    0, "ada"  }, // Adangme
	{ slangADY,    0, "ady"  }, // Adyghe
	{ slangAE,     0, "ae"   }, // Avestan
	{ slangAF,     0, "af"   }, // Afrikaans
	{ slangAFA,    0, "afa"  }, // Afro-Asiatic Language
	{ slangAFH,    0, "afh"  }, // Afrihili
	{ slangAGQ,    0, "agq"  }, // Aghem
	{ slangAIN,    0, "ain"  }, // Ainu
	{ slangAK,     0, "ak"   }, // Akan
	{ slangAKK,    0, "akk"  }, // Akkadian
	{ slangALE,    0, "ale"  }, // Aleut
	{ slangALG,    0, "alg"  }, // Algonquian Language
	{ slangALT,    0, "alt"  }, // Southern Altai
	{ slangAM,     0, "am"   }, // Amharic
	{ slangAN,     0, "an"   }, // Aragonese
	{ slangANG,    0, "ang"  }, // Old English
	{ slangANP,    0, "anp"  }, // Angika
	{ slangAPA,    0, "apa"  }, // Apache Language
	{ slangAR,     LANG_ARABIC, "ar"   }, // Arabic
	{ slangARC,    0, "arc"  }, // Aramaic
	{ slangARN,    0, "arn"  }, // Araucanian
	{ slangARP,    0, "arp"  }, // Arapaho
	{ slangART,    0, "art"  }, // Artificial Language
	{ slangARW,    0, "arw"  }, // Arawak
	{ slangAS,     0, "as"   }, // Assamese
	{ slangASA,    0, "asa"  }, // Asu
	{ slangAST,    0, "ast"  }, // Asturian
	{ slangATH,    0, "ath"  }, // Athapascan Language
	{ slangAUS,    0, "aus"  }, // Australian Language
	{ slangAV,     0, "av"   }, // Avaric
	{ slangAWA,    0, "awa"  }, // Awadhi
	{ slangAY,     0, "ay"   }, // Aymara
	{ slangAZ,     0, "az"   }, // Azerbaijani
	{ slangBA,     0, "ba"   }, // Bashkir
	{ slangBAD,    0, "bad"  }, // Banda
	{ slangBAI,    0, "bai"  }, // Bamileke Language
	{ slangBAL,    0, "bal"  }, // Baluchi
	{ slangBAN,    0, "ban"  }, // Balinese
	{ slangBAS,    0, "bas"  }, // Basaa
	{ slangBAT,    0, "bat"  }, // Baltic Language
	{ slangBE,     0, "be"   }, // Belarusian
	{ slangBEJ,    0, "bej"  }, // Beja
	{ slangBEM,    0, "bem"  }, // Bemba
	{ slangBER,    0, "ber"  }, // Berber
	{ slangBEZ,    0, "bez"  }, // Bena
	{ slangBG,     0, "bg"   }, // Bulgarian
	{ slangBH,     0, "bh"   }, // Bihari
	{ slangBHO,    0, "bho"  }, // Bhojpuri
	{ slangBI,     0, "bi"   }, // Bislama
	{ slangBIK,    0, "bik"  }, // Bikol
	{ slangBIN,    0, "bin"  }, // Bini
	{ slangBLA,    0, "bla"  }, // Siksika
	{ slangBM,     0, "bm"   }, // Bambara
	{ slangBN,     0, "bn"   }, // Bengali
	{ slangBNT,    0, "bnt"  }, // Bantu
	{ slangBO,     0, "bo"   }, // Tibetan
	{ slangBR,     0, "br"   }, // Breton
	{ slangBRA,    0, "bra"  }, // Braj
	{ slangBRX,    0, "brx"  }, // Bodo
	{ slangBS,     0, "bs"   }, // Bosnian
	{ slangBTK,    0, "btk"  }, // Batak
	{ slangBUA,    0, "bua"  }, // Buriat
	{ slangBUG,    0, "bug"  }, // Buginese
	{ slangBYN,    0, "byn"  }, // Blin
	{ slangCA,     0, "ca"   }, // Catalan
	{ slangCAD,    0, "cad"  }, // Caddo
	{ slangCAI,    0, "cai"  }, // Central American Indian Language
	{ slangCAR,    0, "car"  }, // Carib
	{ slangCAU,    0, "cau"  }, // Caucasian Language
	{ slangCAY,    0, "cay"  }, // Cayuga
	{ slangCCH,    0, "cch"  }, // Atsam
	{ slangCE,     0, "ce"   }, // Chechen
	{ slangCEB,    0, "ceb"  }, // Cebuano
	{ slangCEL,    0, "cel"  }, // Celtic Language
	{ slangCGG,    0, "cgg"  }, // Chiga
	{ slangCH,     0, "ch"   }, // Chamorro
	{ slangCHB,    0, "chb"  }, // Chibcha
	{ slangCHG,    0, "chg"  }, // Chagatai
	{ slangCHK,    0, "chk"  }, // Chuukese
	{ slangCHM,    0, "chm"  }, // Mari
	{ slangCHN,    0, "chn"  }, // Chinook Jargon
	{ slangCHO,    0, "cho"  }, // Choctaw
	{ slangCHP,    0, "chp"  }, // Chipewyan
	{ slangCHR,    0, "chr"  }, // Cherokee
	{ slangCHY,    0, "chy"  }, // Cheyenne
	{ slangCMC,    0, "cmc"  }, // Chamic Language
	{ slangCO,     0, "co"   }, // Corsican
	{ slangCOP,    0, "cop"  }, // Coptic
	{ slangCPE,    0, "cpe"  }, // English-based Creole or Pidgin
	{ slangCPF,    0, "cpf"  }, // French-based Creole or Pidgin
	{ slangCPP,    0, "cpp"  }, // Portuguese-based Creole or Pidgin
	{ slangCR,     0, "cr"   }, // Cree
	{ slangCRH,    0, "crh"  }, // Crimean Turkish
	{ slangCRP,    0, "crp"  }, // Creole or Pidgin
	{ slangCS,     0, "cs"   }, // Czech
	{ slangCSB,    0, "csb"  }, // Kashubian
	{ slangCU,     0, "cu"   }, // Church Slavic
	{ slangCUS,    0, "cus"  }, // Cushitic Language
	{ slangCV,     0, "cv"   }, // Chuvash
	{ slangCY,     0, "cy"   }, // Welsh
	{ slangDA,     0, "da"   }, // Danish
	{ slangDAK,    0, "dak"  }, // Dakota
	{ slangDAR,    0, "dar"  }, // Dargwa
	{ slangDAV,    0, "dav"  }, // Taita
	{ slangDAY,    0, "day"  }, // Dayak
	{ slangDE,     LANG_GERMAN, "de"   }, // German
	{ slangDE_AT,  0, "de-at" }, // Austrian German
	{ slangDE_CH,  0, "de-ch" }, // Swiss High German
	{ slangDEL,    0, "del" }, // Delaware
	{ slangDEN,    0, "den" }, // Slave
	{ slangDGR,    0, "dgr" }, // Dogrib
	{ slangDIN,    0, "din" }, // Dinka
	{ slangDJE,    0, "dje" }, // Zarma
	{ slangDOI,    0, "doi" }, // Dogri
	{ slangDRA,    0, "dra" }, // Dravidian Language
	{ slangDSB,    0, "dsb" }, // Lower Sorbian
	{ slangDUA,    0, "dua" }, // Duala
	{ slangDUM,    0, "dum" }, // Middle Dutch
	{ slangDV,     0, "dv"  }, // Divehi
	{ slangDYO,    0, "dyo" }, // Jola-Fonyi
	{ slangDYU,    0, "dyu" }, // Dyula
	{ slangDZ,     0, "dz"  }, // Dzongkha
	{ slangEBU,    0, "ebu" }, // Embu
	{ slangEE,     0, "ee"  }, // Ewe
	{ slangEFI,    0, "efi" }, // Efik
	{ slangEGY,    0, "egy" }, // Ancient Egyptian
	{ slangEKA,    0, "eka" }, // Ekajuk
	{ slangEL,     LANG_GREEK, "el"  }, // Greek
	{ slangELX,    0, "elx" }, // Elamite
	{ slangEN,     LANG_ENGLISH, "en"  }, // English
	{ slangEN_AU,  0, "en-au" }, // Australian English
	{ slangEN_CA,  0, "en-ca" }, // Canadian English
	{ slangEN_GB,  0, "en-gb" }, // British English
	{ slangEN_US,  0, "en-us" }, // U.S. English
	{ slangENM,    0, "enm" }, // Middle English
	{ slangEO,     0, "eo"  }, // Esperanto
	{ slangES,     LANG_SPANISH, "es"  }, // Spanish
	{ slangES_419, 0, "es-419" }, // Latin American Spanish
	{ slangES_ES,  0, "es-es"  }, // Iberian Spanish
	{ slangET,     0, "et"  }, // Estonian
	{ slangEU,     0, "eu"  }, // Basque
	{ slangEWO,    0, "ewo" }, // Ewondo
	{ slangFA,     0, "fa"  }, // Persian
	{ slangFAN,    0, "fan" }, // Fang
	{ slangFAT,    0, "fat" }, // Fanti
	{ slangFF,     0, "ff"  }, // Fulah
	{ slangFI,     LANG_FINNISH, "fi"  }, // Finnish
	{ slangFIL,    0, "fil" }, // Filipino
	{ slangFIU,    0, "fiu" }, // Finno-Ugrian Language
	{ slangFJ,     0, "fj"  }, // Fijian
	{ slangFO,     0, "fo"  }, // Faroese
	{ slangFON,    0, "fon" }, // Fon
	{ slangFR,     LANG_FRENCH, "fr"  }, // French
	{ slangFR_CA,  0, "fr-ca" }, // Canadian French
	{ slangFR_CH,  0, "fr-ch" }, // Swiss French
	{ slangFRM,    0, "frm" }, // Middle French
	{ slangFRO,    0, "fro" }, // Old French
	{ slangFRR,    0, "frr" }, // Northern Frisian
	{ slangFRS,    0, "frs" }, // Eastern Frisian
	{ slangFUR,    0, "fur" }, // Friulian
	{ slangFY,     0, "fy"  }, // Western Frisian
	{ slangGA,     0, "ga"  }, // Irish
	{ slangGAA,    0, "gaa" }, // Ga
	{ slangGAY,    0, "gay" }, // Gayo
	{ slangGBA,    0, "gba" }, // Gbaya
	{ slangGD,     0, "gd"  }, // Scottish Gaelic
	{ slangGEM,    0, "gem" }, // Germanic Language
	{ slangGEZ,    0, "gez" }, // Geez
	{ slangGIL,    0, "gil" }, // Gilbertese
	{ slangGL,     0, "gl"  }, // Galician
	{ slangGMH,    0, "gmh" }, // Middle High German
	{ slangGN,     0, "gn"  }, // Guarani
	{ slangGOH,    0, "goh" }, // Old High German
	{ slangGON,    0, "gon" }, // Gondi
	{ slangGOR,    0, "gor" }, // Gorontalo
	{ slangGOT,    0, "got" }, // Gothic
	{ slangGRB,    0, "grb" }, // Grebo
	{ slangGRC,    0, "grc" }, // Ancient Greek
	{ slangGSW,    0, "gsw" }, // Swiss German
	{ slangGU,     0, "gu"  }, // Gujarati
	{ slangGUZ,    0, "guz" }, // Gusii
	{ slangGV,     0, "gv"  }, // Manx
	{ slangGWI,    0, "gwi" }, // Gwich?in
	{ slangHA,     0, "ha"  }, // Hausa
	{ slangHAI,    0, "hai" }, // Haida
	{ slangHAW,    0, "haw" }, // Hawaiian
	{ slangHE,     0, "he"  }, // Hebrew
	{ slangHI,     0, "hi"  }, // Hindi
	{ slangHIL,    0, "hil" }, // Hiligaynon
	{ slangHIM,    0, "him" }, // Himachali
	{ slangHIT,    0, "hit" }, // Hittite
	{ slangHMN,    0, "hmn" }, // Hmong
	{ slangHO,     0, "ho"  }, // Hiri Motu
	{ slangHR,     0, "hr"  }, // Croatian
	{ slangHSB,    0, "hsb" }, // Upper Sorbian
	{ slangHT,     0, "ht"  }, // Haitian
	{ slangHU,     LANG_HUNGARIAN, "hu"  }, // Hungarian
	{ slangHUP,    0, "hup" }, // Hupa
	{ slangHY,     0, "hy"  }, // Armenian
	{ slangHZ,     0, "hz"  }, // Herero
	{ slangIA,     0, "ia"  }, // Interlingua
	{ slangIBA,    0, "iba" }, // Iban
	{ slangID,     0, "id"  }, // Indonesian
	{ slangIE,     0, "ie"  }, // Interlingue
	{ slangIG,     0, "ig"  }, // Igbo
	{ slangII,     0, "ii"  }, // Sichuan Yi
	{ slangIJO,    0, "ijo" }, // Ijo
	{ slangIK,     0, "ik"  }, // Inupiaq
	{ slangILO,    0, "ilo" }, // Iloko
	{ slangINC,    0, "inc" }, // Indic Language
	{ slangINE,    0, "ine" }, // Indo-European Language
	{ slangINH,    0, "inh" }, // Ingush
	{ slangIO,     0, "io"  }, // Ido
	{ slangIRA,    0, "ira" }, // Iranian Language
	{ slangIRO,    0, "iro" }, // Iroquoian Language
	{ slangIS,     0, "is"  }, // Icelandic
	{ slangIT,     LANG_ITALIAN, "it"  }, // Italian
	{ slangIU,     0, "iu"  }, // Inuktitut
	{ slangJA,     LANG_JAPANESE, "ja"  }, // Japanese
	{ slangJBO,    0, "jbo" }, // Lojban
	{ slangJMC,    0, "jmc" }, // Machame
	{ slangJPR,    0, "jpr" }, // Judeo-Persian
	{ slangJRB,    0, "jrb" }, // Judeo-Arabic
	{ slangJV,     0, "jv"  }, // Javanese
	{ slangKA,     0, "ka"  }, // Georgian
	{ slangKAA,    0, "kaa" }, // Kara-Kalpak
	{ slangKAB,    0, "kab" }, // Kabyle
	{ slangKAC,    0, "kac" }, // Kachin
	{ slangKAJ,    0, "kaj" }, // Jju
	{ slangKAM,    0, "kam" }, // Kamba
	{ slangKAR,    0, "kar" }, // Karen
	{ slangKAW,    0, "kaw" }, // Kawi
	{ slangKBD,    0, "kbd" }, // Kabardian
	{ slangKCG,    0, "kcg" }, // Tyap
	{ slangKDE,    0, "kde" }, // Makonde
	{ slangKEA,    0, "kea" }, // Kabuverdianu
	{ slangKFO,    0, "kfo" }, // Koro
	{ slangKG,     0, "kg"  }, // Kongo
	{ slangKHA,    0, "kha" }, // Khasi
	{ slangKHI,    0, "khi" }, // Khoisan Language
	{ slangKHO,    0, "kho" }, // Khotanese
	{ slangKHQ,    0, "khq" }, // Koyra Chiini
	{ slangKI,     0, "ki" }, // Kikuyu
	{ slangKJ,     0, "kj" }, // Kuanyama
	{ slangKK,     0, "kk" }, // Kazakh
	{ slangKL,     0, "kl" }, // Kalaallisut
	{ slangKLN,    0, "kln" }, // Kalenjin
	{ slangKM,     0, "km" }, // Khmer
	{ slangKMB,    0, "kmb" }, // Kimbundu
	{ slangKN,     0, "kn" }, // Kannada
	{ slangKO,     0, "ko" }, // Korean
	{ slangKOK,    0, "kok" }, // Konkani
	{ slangKOS,    0, "kos" }, // Kosraean
	{ slangKPE,    0, "kpe" }, // Kpelle
	{ slangKR,     0, "kr" }, // Kanuri
	{ slangKRC,    0, "krc" }, // Karachay-Balkar
	{ slangKRL,    0, "krl" }, // Karelian
	{ slangKRO,    0, "kro" }, // Kru
	{ slangKRU,    0, "kru" }, // Kurukh
	{ slangKS,     0, "ks" }, // Kashmiri
	{ slangKSB,    0, "ksb" }, // Shambala
	{ slangKSF,    0, "ksf" }, // Bafia
	{ slangKSH,    0, "ksh" }, // Colognian
	{ slangKU,     0, "ku" }, // Kurdish
	{ slangKUM,    0, "kum" }, // Kumyk
	{ slangKUT,    0, "kut" }, // Kutenai
	{ slangKV,     0, "kv" }, // Komi
	{ slangKW,     0, "kw" }, // Cornish
	{ slangKY,     0, "ky" }, // Kirghiz
	{ slangLA,     0, "la" }, // Latin
	{ slangLAD,    0, "lad" }, // Ladino
	{ slangLAG,    0, "lag" }, // Langi
	{ slangLAH,    0, "lah" }, // Lahnda
	{ slangLAM,    0, "lam" }, // Lamba
	{ slangLB,     0, "lb" }, // Luxembourgish
	{ slangLEZ,    0, "lez" }, // Lezghian
	{ slangLG,     0, "lg" }, // Ganda
	{ slangLI,     0, "li" }, // Limburgish
	{ slangLN,     0, "ln" }, // Lingala
	{ slangLO,     0, "lo" }, // Lao
	{ slangLOL,    0, "lol" }, // Mongo
	{ slangLOZ,    0, "loz" }, // Lozi
	{ slangLT,     0, "lt" }, // Lithuanian
	{ slangLU,     0, "lu" }, // Luba-Katanga
	{ slangLUA,    0, "lua" }, // Luba-Lulua
	{ slangLUI,    0, "lui" }, // Luiseno
	{ slangLUN,    0, "lun" }, // Lunda
	{ slangLUO,    0, "luo" }, // Luo
	{ slangLUS,    0, "lus" }, // Lushai
	{ slangLUY,    0, "luy" }, // Luyia
	{ slangLV,     0, "lv" }, // Latvian
	{ slangMAD,    0, "mad" }, // Madurese
	{ slangMAG,    0, "mag" }, // Magahi
	{ slangMAI,    0, "mai" }, // Maithili
	{ slangMAK,    0, "mak" }, // Makasar
	{ slangMAN,    0, "man" }, // Mandingo
	{ slangMAP,    0, "map" }, // Austronesian Language
	{ slangMAS,    0, "mas" }, // Masai
	{ slangMDF,    0, "mdf" }, // Moksha
	{ slangMDR,    0, "mdr" }, // Mandar
	{ slangMEN,    0, "men" }, // Mende
	{ slangMER,    0, "mer" }, // Meru
	{ slangMFE,    0, "mfe" }, // Morisyen
	{ slangMG,     0, "mg" }, // Malagasy
	{ slangMGA,    0, "mga" }, // Middle Irish
	{ slangMGH,    0, "mgh" }, // Makhuwa-Meetto
	{ slangMH,     0, "mh" }, // Marshallese
	{ slangMI,     0, "mi" }, // Maori
	{ slangMIC,    0, "mic" }, // Micmac
	{ slangMIN,    0, "min" }, // Minangkabau
	{ slangMIS,    0, "mis" }, // Miscellaneous Language
	{ slangMK,     0, "mk" }, // Macedonian
	{ slangMKH,    0, "mkh" }, // Mon-Khmer Language
	{ slangML,     0, "ml" }, // Malayalam
	{ slangMN,     0, "mn" }, // Mongolian
	{ slangMNC,    0, "mnc" }, // Manchu
	{ slangMNI,    0, "mni" }, // Manipuri
	{ slangMNO,    0, "mno" }, // Manobo Language
	{ slangMO,     0, "mo" }, // Moldavian
	{ slangMOH,    0, "moh" }, // Mohawk
	{ slangMOS,    0, "mos" }, // Mossi
	{ slangMR,     0, "mr" }, // Marathi
	{ slangMS,     0, "ms" }, // Malay
	{ slangMT,     0, "mt" }, // Maltese
	{ slangMUA,    0, "mua" }, // Mundang
	{ slangMUL,    0, "mul" }, // Multiple Languages
	{ slangMUN,    0, "mun" }, // Munda Language
	{ slangMUS,    0, "mus" }, // Creek
	{ slangMWL,    0, "mwl" }, // Mirandese
	{ slangMWR,    0, "mwr" }, // Marwari
	{ slangMY,     0, "my" }, // Burmese
	{ slangMYN,    0, "myn" }, // Mayan Language
	{ slangMYV,    0, "myv" }, // Erzya
	{ slangNA,     0, "na" }, // Nauru
	{ slangNAH,    0, "nah" }, // Nahuatl
	{ slangNAI,    0, "nai" }, // North American Indian Language
	{ slangNAP,    0, "nap" }, // Neapolitan
	{ slangNAQ,    0, "naq" }, // Nama
	{ slangNB,     0, "nb" }, // Norwegian Bokmal
	{ slangND,     0, "nd" }, // North Ndebele
	{ slangNDS,    0, "nds" }, // Low German
	{ slangNE,     0, "ne" }, // Nepali
	{ slangNEW,    0, "new" }, // Newari
	{ slangNG,     0, "ng" }, // Ndonga
	{ slangNIA,    0, "nia" }, // Nias
	{ slangNIC,    0, "nic" }, // Niger-Kordofanian Language
	{ slangNIU,    0, "niu" }, // Niuean
	{ slangNL,     0, "nl" }, // Dutch
	{ slangNL_BE,  0, "nl-be" }, // Flemish
	{ slangNMG,    0, "nmg" }, // Kwasio
	{ slangNN,     0, "nn" }, // Norwegian Nynorsk
	{ slangNO,     0, "no" }, // Norwegian
	{ slangNOG,    0, "nog" }, // Nogai
	{ slangNON,    0, "non" }, // Old Norse
	{ slangNQO,    0, "nqo" }, // N’Ko
	{ slangNR,     0, "nr" }, // South Ndebele
	{ slangNSO,    0, "nso" }, // Northern Sotho
	{ slangNUB,    0, "nub" }, // Nubian Language
	{ slangNUS,    0, "nus" }, // Nuer
	{ slangNV,     0, "nv" }, // Navajo
	{ slangNWC,    0, "nwc" }, // Classical Newari
	{ slangNY,     0, "ny" }, // Nyanja
	{ slangNYM,    0, "nym" }, // Nyamwezi
	{ slangNYN,    0, "nyn" }, // Nyankole
	{ slangNYO,    0, "nyo" }, // Nyoro
	{ slangNZI,    0, "nzi" }, // Nzima
	{ slangOC,     0, "oc" }, // Occitan
	{ slangOJ,     0, "oj" }, // Ojibwa
	{ slangOM,     0, "om" }, // Oromo
	{ slangOR,     0, "or" }, // Oriya
	{ slangOS,     0, "os" }, // Ossetic
	{ slangOSA,    0, "osa" }, // Osage
	{ slangOTA,    0, "ota" }, // Ottoman Turkish
	{ slangOTO,    0, "oto" }, // Otomian Language
	{ slangPA,     0, "pa" }, // Punjabi
	{ slangPAA,    0, "paa" }, // Papuan Language
	{ slangPAG,    0, "pag" }, // Pangasinan
	{ slangPAL,    0, "pal" }, // Pahlavi
	{ slangPAM,    0, "pam" }, // Pampanga
	{ slangPAP,    0, "pap" }, // Papiamento
	{ slangPAU,    0, "pau" }, // Palauan
	{ slangPEO,    0, "peo" }, // Old Persian
	{ slangPHI,    0, "phi" }, // Philippine Language
	{ slangPHN,    0, "phn" }, // Phoenician
	{ slangPI,     0, "pi" }, // Pali
	{ slangPL,     0, "pl" }, // Polish
	{ slangPON,    0, "pon" }, // Pohnpeian
	{ slangPRA,    0, "pra" }, // Prakrit Language
	{ slangPRO,    0, "pro" }, // Old Provencal
	{ slangPS,     0, "ps" }, // Pashto
	{ slangPT,     0, "pt" }, // Portuguese
	{ slangPT_BR,  0, "pt-br" }, // Brazilian Portuguese
	{ slangPT_PT,  0, "pt-pt" }, // Iberian Portuguese
	{ slangQU,     0, "qu" }, // Quechua
	{ slangRAJ,    0, "raj" }, // Rajasthani
	{ slangRAP,    0, "rap" }, // Rapanui
	{ slangRAR,    0, "rar" }, // Rarotongan
	{ slangRM,     0, "rm" }, // Romansh
	{ slangRN,     0, "rn" }, // Rundi
	{ slangRO,     0, "ro" }, // Romanian
	{ slangROA,    0, "roa" }, // Romance Language
	{ slangROF,    0, "rof" }, // Rombo
	{ slangROM,    0, "rom" }, // Romany
	{ slangROOT,   0, "root" }, // Root
	{ slangRU,     LANG_RUSSIAN, "ru" }, // Russian
	{ slangRUP,    0, "rup" }, // Aromanian
	{ slangRW,     0, "rw" }, // Kinyarwanda
	{ slangRWK,    0, "rwk" }, // Rwa
	{ slangSA,     0, "sa" }, // Sanskrit
	{ slangSAD,    0, "sad" }, // Sandawe
	{ slangSAH,    0, "sah" }, // Sakha
	{ slangSAI,    0, "sai" }, // South American Indian Language
	{ slangSAL,    0, "sal" }, // Salishan Language
	{ slangSAM,    0, "sam" }, // Samaritan Aramaic
	{ slangSAQ,    0, "saq" }, // Samburu
	{ slangSAS,    0, "sas" }, // Sasak
	{ slangSAT,    0, "sat" }, // Santali
	{ slangSBP,    0, "sbp" }, // Sangu
	{ slangSC,     0, "sc" }, // Sardinian
	{ slangSCN,    0, "scn" }, // Sicilian
	{ slangSCO,    0, "sco" }, // Scots
	{ slangSD,     0, "sd" }, // Sindhi
	{ slangSE,     0, "se" }, // Northern Sami
	{ slangSEE,    0, "see" }, // Seneca
	{ slangSEH,    0, "seh" }, // Sena
	{ slangSEL,    0, "sel" }, // Selkup
	{ slangSEM,    0, "sem" }, // Semitic Language
	{ slangSES,    0, "ses" }, // Koyraboro Senni
	{ slangSG,     0, "sg" }, // Sango
	{ slangSGA,    0, "sga" }, // Old Irish
	{ slangSGN,    0, "sgn" }, // Sign Language
	{ slangSH,     0, "sh" }, // Serbo-Croatian
	{ slangSHI,    0, "shi" }, // Tachelhit
	{ slangSHN,    0, "shn" }, // Shan
	{ slangSI,     0, "si" }, // Sinhala
	{ slangSID,    0, "sid" }, // Sidamo
	{ slangSIO,    0, "sio" }, // Siouan Language
	{ slangSIT,    0, "sit" }, // Sino-Tibetan Language
	{ slangSK,     0, "sk" }, // Slovak
	{ slangSL,     0, "sl" }, // Slovenian
	{ slangSLA,    0, "sla" }, // Slavic Language
	{ slangSM,     0, "sm" }, // Samoan
	{ slangSMA,    0, "sma" }, // Southern Sami
	{ slangSMI,    0, "smi" }, // Sami Language
	{ slangSMJ,    0, "smj" }, // Lule Sami
	{ slangSMN,    0, "smn" }, // Inari Sami
	{ slangSMS,    0, "sms" }, // Skolt Sami
	{ slangSN,     0, "sn" }, // Shona
	{ slangSNK,    0, "snk" }, // Soninke
	{ slangSO,     0, "so" }, // Somali
	{ slangSOG,    0, "sog" }, // Sogdien
	{ slangSON,    0, "son" }, // Songhai
	{ slangSQ,     0, "sq" }, // Albanian
	{ slangSR,     0, "sr" }, // Serbian
	{ slangSRN,    0, "srn" }, // Sranan Tongo
	{ slangSRR,    0, "srr" }, // Serer
	{ slangSS,     0, "ss" }, // Swati
	{ slangSSA,    0, "ssa" }, // Nilo-Saharan Language
	{ slangSSY,    0, "ssy" }, // Saho
	{ slangST,     0, "st" }, // Southern Sotho
	{ slangSU,     0, "su" }, // Sundanese
	{ slangSUK,    0, "suk" }, // Sukuma
	{ slangSUS,    0, "sus" }, // Susu
	{ slangSUX,    0, "sux" }, // Sumerian
	{ slangSV,     0, "sv" }, // Swedish
	{ slangSW,     0, "sw" }, // Swahili
	{ slangSWB,    0, "swb" }, // Comorian
	{ slangSWC,    0, "swc" }, // Congo Swahili
	{ slangSYC,    0, "syc" }, // Classical Syriac
	{ slangSYR,    0, "syr" }, // Syriac
	{ slangTA,     0, "ta" }, // Tamil
	{ slangTAI,    0, "tai" }, // Tai Language
	{ slangTE,     0, "te" }, // Telugu
	{ slangTEM,    0, "tem" }, // Timne
	{ slangTEO,    0, "teo" }, // Teso
	{ slangTER,    0, "ter" }, // Tereno
	{ slangTET,    0, "tet" }, // Tetum
	{ slangTG,     0, "tg" }, // Tajik
	{ slangTH,     0, "th" }, // Thai
	{ slangTI,     0, "ti" }, // Tigrinya
	{ slangTIG,    0, "tig" }, // Tigre
	{ slangTIV,    0, "tiv" }, // Tiv
	{ slangTK,     0, "tk" }, // Turkmen
	{ slangTKL,    0, "tkl" }, // Tokelau
	{ slangTL,     0, "tl" }, // Tagalog
	{ slangTLH,    0, "tlh" }, // Klingon
	{ slangTLI,    0, "tli" }, // Tlingit
	{ slangTMH,    0, "tmh" }, // Tamashek
	{ slangTN,     0, "tn" }, // Tswana
	{ slangTO,     0, "to" }, // Tongan
	{ slangTOG,    0, "tog" }, // Nyasa Tonga
	{ slangTPI,    0, "tpi" }, // Tok Pisin
	{ slangTR,     0, "tr" }, // Turkish
	{ slangTRV,    0, "trv" }, // Taroko
	{ slangTS,     0, "ts" }, // Tsonga
	{ slangTSI,    0, "tsi" }, // Tsimshian
	{ slangTT,     0, "tt" }, // Tatar
	{ slangTUM,    0, "tum" }, // Tumbuka
	{ slangTUP,    0, "tup" }, // Tupi Language
	{ slangTUT,    0, "tut" }, // Altaic Language
	{ slangTVL,    0, "tvl" }, // Tuvalu
	{ slangTW,     0, "tw" }, // Twi
	{ slangTWQ,    0, "twq" }, // Tasawaq
	{ slangTY,     0, "ty" }, // Tahitian
	{ slangTYV,    0, "tyv" }, // Tuvinian
	{ slangTZM,    0, "tzm" }, // Central Morocco Tamazight
	{ slangUDM,    0, "udm" }, // Udmurt
	{ slangUG,     0, "ug" }, // Uighur
	{ slangUGA,    0, "uga" }, // Ugaritic
	{ slangUK,     LANG_UKRAINIAN, "uk" }, // Ukrainian
	{ slangUMB,    0, "umb" }, // Umbundu
	{ slangUND,    0, "und" }, // Unknown Language
	{ slangUR,     0, "ur" }, // Urdu
	{ slangUZ,     0, "uz" }, // Uzbek
	{ slangVAI,    0, "vai" }, // Vai
	{ slangVE,     0, "ve" }, // Venda
	{ slangVI,     0, "vi" }, // Vietnamese
	{ slangVO,     0, "vo" }, // Volapuk
	{ slangVOT,    0, "vot" }, // Votic
	{ slangVUN,    0, "vun" }, // Vunjo
	{ slangWA,     0, "wa" }, // Walloon
	{ slangWAE,    0, "wae" }, // Walser
	{ slangWAK,    0, "wak" }, // Wakashan Language
	{ slangWAL,    0, "wal" }, // Walamo
	{ slangWAR,    0, "war" }, // Waray
	{ slangWAS,    0, "was" }, // Washo
	{ slangWEN,    0, "wen" }, // Sorbian Language
	{ slangWO,     0, "wo" }, // Wolof
	{ slangXAL,    0, "xal" }, // Kalmyk
	{ slangXH,     0, "xh" }, // Xhosa
	{ slangXOG,    0, "xog" }, // Soga
	{ slangYAO,    0, "yao" }, // Yao
	{ slangYAP,    0, "yap" }, // Yapese
	{ slangYAV,    0, "yav" }, // Yangben
	{ slangYI,     0, "yi" }, // Yiddish
	{ slangYO,     0, "yo" }, // Yoruba
	{ slangYPK,    0, "ypk" }, // Yupik Language
	{ slangYUE,    0, "yue" }, // Cantonese
	{ slangZA,     0, "za" }, // Zhuang
	{ slangZAP,    0, "zap" }, // Zapotec
	{ slangZBL,    0, "zbl" }, // Blissymbols
	{ slangZEN,    0, "zen" }, // Zenaga
	{ slangZH,     0, "zh" }, // Chinese
	{ slangZH_HANS,0, "zh-hans" }, // Simplified Chinese
	{ slangZH_HANT,0, "zh-hant" }, // Traditional Chinese
	{ slangZND,    0, "znd" }, // Zande
	{ slangZU,     0, "zu" }, // Zulu
	{ slangZUN,    0, "zun" }, // Zuni
	{ slangZXX,    0, "zxx" }, // No linguistic content
	{ slangZZA,    0, "zza" }, // Zaza
	{ slangBN_BD,  0, "bn-bd" },
};

void GetLinguaSymbList(StringSet & rSs)
{
	rSs.Z();
	for(uint i = 0; i < SIZEOFARRAY(P_LinguaIdentList); i++) {
		rSs.add(P_LinguaIdentList[i].P_Code);
	}
}

void GetLinguaIdList(LongArray & rList)
{
	rList.Z();
	for(uint i = 0; i < SIZEOFARRAY(P_LinguaIdentList); i++) {
		rList.add(P_LinguaIdentList[i].Id);
	}
}

int FASTCALL RecognizeLinguaSymb(const char * pSymb, int word)
{
	int    ret_ident = 0;
	if(!isempty(pSymb)) {
		uint   i;
		const  size_t max_symb_len = 7;
		size_t max_found_len = 0;
		SString temp_buf;
		for(i = 0; *pSymb && i < max_symb_len; i++)
			temp_buf.CatChar(*pSymb++);
		temp_buf.ToLower();
		temp_buf.ReplaceChar('_', '-');
		temp_buf.ReplaceChar(' ', '-');
		for(i = 0; i < SIZEOFARRAY(P_LinguaIdentList); i++) {
			const char * p_code = P_LinguaIdentList[i].P_Code;
			if(word) {
				if(temp_buf == p_code) {
					ret_ident = P_LinguaIdentList[i].Id;
					break;
				}
			}
			else {
				const size_t code_len = sstrlen(p_code);
				if(temp_buf.HasPrefix(p_code) && code_len > max_found_len) {
					const char nextc = temp_buf.C(code_len);
					//
					// @v9.1.8 Дополнительное ограничение: следующий символ не должен быть латинской буквой.
					//
					if(!((nextc >= 'a' && nextc <= 'z') || (nextc >= 'A' && nextc <= 'Z'))) {
						max_found_len = code_len;
						ret_ident = P_LinguaIdentList[i].Id;
					}
				}
			}
		}
	}
	if(!ret_ident) {
		SLS.SetError(SLERR_LANGSYMBNFOUND, pSymb);
	}
	return ret_ident;
}

int FASTCALL GetLinguaCode(int ident, SString & rCode)
{
	int    ok = 0;
	rCode.Z();
	for(uint i = 0; !ok && i < SIZEOFARRAY(P_LinguaIdentList); i++) {
		if(ident == P_LinguaIdentList[i].Id) {
			rCode = P_LinguaIdentList[i].P_Code;
			ok = 1;
		}
	}
	return ok;
}

uint32 FASTCALL GetLinguaWinIdent(int ident)
{
	uint32 result = 0;
	for(uint i = 0; i < SIZEOFARRAY(P_LinguaIdentList); i++) {
		if(ident == P_LinguaIdentList[i].Id) {
			result = P_LinguaIdentList[i].WinLangId;
			break;
		}
	}
	return result;
}

void FASTCALL GetLinguaList(LongArray & rList)
{
	rList.clear();
	for(uint i = 0; i < SIZEOFARRAY(P_LinguaIdentList); i++)
		rList.add(P_LinguaIdentList[i].Id);
}
//
//
//
struct SNScriptIdent {
	int16  Id;
	const char * P_Code;
};

static const SNScriptIdent P_SNScriptIdentList[] = {
	//{ snscriptUnkn,        "unkn" },
	//{ snscriptMeta,        "meta" },
	{ snscriptLatin,        "latin" },
	{ snscriptGreek,        "greek" },
	{ snscriptCyrillic,     "cyrillic" },
	{ snscriptArmenian,     "armenian" },
	{ snscriptHebrew,       "hebrew" },
    { snscriptArabicMathematical, "arabic mathematical" },
	{ snscriptArabic,       "arabic" },
	{ snscriptSyriac,       "syriac" },
	{ snscriptThaana,       "thaana" },
	{ snscriptNko,          "nko" },
	{ snscriptSamaritan,    "samaritan" },
	{ snscriptMandaic,      "mandaic" },
	{ snscriptDevanagari,   "devanagari" },
	{ snscriptBengali,      "bengali" },
	{ snscriptGurmukhi,     "gurmukhi" },
	{ snscriptGujarati,     "gujarati" },
	{ snscriptOriya,        "oriya" },
	{ snscriptTamil,        "tamil" },
	{ snscriptTelugu,       "telugu" },
	{ snscriptKannada,      "kannada" },
	{ snscriptMalayalam,    "malayalam" },
	{ snscriptSinhala,      "sinhala" },
	{ snscriptThai,         "thai" },
	{ snscriptLao,          "lao" },
	{ snscriptTibetan,      "tibetan" },
	{ snscriptMyanmar,      "myanmar" },
	{ snscriptGeorgian,     "georgian" },
    { snscriptHangul,       "hangul" },
    { snscriptEthiopic,     "ethiopic" },
    { snscriptCherokee,     "cherokee" },
    { snscriptMongolian,    "mongolian" },
    { snscriptKhmer,        "khmer" },
    { snscriptLimbu,        "limbu" },
    { snscriptBuginese,     "buginese" },
    { snscriptBalinese,     "balinese" },
    { snscriptSundanese,    "sundanese" },
    { snscriptBatak,        "batak" },
    { snscriptLepcha,       "lepcha" },
    { snscriptHiragana,     "hiragana" },
    { snscriptKatakana,     "katakana" },
    { snscriptBopomofo,     "bopomofo" },
    { snscriptKharoshthi,   "kharoshthi" },
    { snscriptManichaean,   "manichaean" },
    { snscriptAvestan,      "avestan" },
    { snscriptBrahmi,       "brahmi" },
    { snscriptKaithi,       "kaithi" },
    { snscriptChakma,       "chakma" },
    { snscriptMahajani,     "mahajani" },
    { snscriptSharada,      "sharada" },
    { snscriptKhojki,       "khojki" },
    { snscriptGlagolitic,   "glagolitic" },
    { snscriptCanadianSyllabics, "canadian syllabics" },
    { snscriptTaiLe,        "tai le" },
    { snscriptNewTaiLue,    "new tai lue" },
    { snscriptTaiTham,      "tai tham" },
    { snscriptTaiViet,      "tai viet" },
    { snscriptGothic,       "gothic" },
    { snscriptUgaritic,     "ugaritic" },
    { snscriptMathematical, "mathematical" },
    { snscriptYi,           "yi" },
    { snscriptCJK,          "cjk" },
    { snscriptSquaredCJK,   "squared cjk" },
    { snscriptBoxDrawings,  "box drawings" },
};

int FASTCALL RecognizeSNScriptSymb(const char * pSymb, size_t * pLength)
{
	int    ret_ident = 0;
	size_t max_found_len = 0;
	if(!isempty(pSymb)) {
		uint   i;
		const  size_t max_symb_len = 64;
		SString temp_buf;
		for(i = 0; *pSymb && i < max_symb_len; i++)
			temp_buf.CatChar(*pSymb++);
		temp_buf.ToLower();
		for(i = 0; i < SIZEOFARRAY(P_SNScriptIdentList); i++) {
			const char * p_code = P_SNScriptIdentList[i].P_Code;
			const size_t code_len = sstrlen(p_code);
			if(temp_buf.HasPrefix(p_code) && code_len > max_found_len) {
				const char nextc = temp_buf.C(code_len);
				// @v10.9.6 if(!((nextc >= 'a' && nextc <= 'z') || (nextc >= 'A' && nextc <= 'Z'))) {
				if(!isasciialpha(nextc)) { // @v10.9.6 
					max_found_len = code_len;
					ret_ident = P_SNScriptIdentList[i].Id;
				}
			}
		}
	}
	ASSIGN_PTR(pLength, max_found_len);
	return ret_ident;
}

int FASTCALL GetSNScriptCode(int ident, SString & rCode)
{
	int    ok = 0;
	rCode.Z();
	for(uint i = 0; !ok && i < SIZEOFARRAY(P_SNScriptIdentList); i++) {
		if(ident == P_SNScriptIdentList[i].Id) {
			rCode = P_SNScriptIdentList[i].P_Code;
			ok = 1;
		}
	}
	return ok;
}

/* @v10.6.2 struct SSisIdent {
	int16  Id;
	const char * P_Code;
};*/

static const /*SSisIdent*/SIntToSymbTabEntry P_SisIdentList[] = { // @v10.6.2 SSisIdent-->SIntToSymbTabEntry
	{ SSystem::ssisWindows, "windows" },
	{ SSystem::ssisWindows, "win" },
	{ SSystem::ssisMAC, "mac" },
	{ SSystem::ssisMAC, "imac" },
	{ SSystem::ssisIBM, "ibm" },
	{ SSystem::ssisAIX, "aix" },
	{ SSystem::ssisJava, "java" },
	{ SSystem::ssisSolaris, "solaris" },
	{ SSystem::ssisHPUX, "hpux" },
	{ SSystem::ssisGLibC, "glibc" },
	{ SSystem::ssisBSD, "bsd" }, // @v10.4.5
	{ SSystem::ssisGNU, "gnu" }  // @v10.4.5
};

int FASTCALL RecognizeSisSymb(const char * pSymb)
{
	return SIntToSymbTab_GetId(P_SisIdentList, SIZEOFARRAY(P_SisIdentList), pSymb); // @v10.6.2 
	/* @v10.6.2 int    ret_ident = 0;
	if(!isempty(pSymb)) {
		for(uint i = 0; !ret_ident && i < SIZEOFARRAY(P_SisIdentList); i++) {
			if(sstreqi_ascii(pSymb, P_SisIdentList[i].P_Code))
                ret_ident = P_SisIdentList[i].Id;
		}
	}
	return ret_ident;*/
}

int FASTCALL GetSisCode(int ident, SString & rCode)
{
	return SIntToSymbTab_GetSymb(P_SisIdentList, SIZEOFARRAY(P_SisIdentList), ident, rCode); // @v10.6.2
	/* @v10.6.2 int    ok = 0;
	rCode.Z();
	for(uint i = 0; !ok && i < SIZEOFARRAY(P_SisIdentList); i++) {
		if(ident == P_SisIdentList[i].Id) {
			rCode = P_SisIdentList[i].P_Code;
			ok = 1;
		}
	}
	return ok;*/
}

#if 0 // {

static const struct stringpool_t stringpool_contents =
{
	"850",
	"862",
	"866",
	"ANSI_X3.4-1968",
	"ANSI_X3.4-1986",
	"ARABIC",
	"ARMSCII-8",
	"ASCII",
	"ASMO-708",
	"BIG-5",
	"BIG-FIVE",
	"BIG5",
	"BIG5-HKSCS",
	"BIG5HKSCS",
	"BIGFIVE",
	"C99",
	"CHAR",
	"CHINESE",
	"CN",
	"CN-BIG5",
	"CN-GB",
	"CN-GB-ISOIR165",
	"CP1133",
	"CP1250",
	"CP1251",
	"CP1252",
	"CP1253",
	"CP1254",
	"CP1255",
	"CP1256",
	"CP1257",
	"CP1258",
	"CP1361",
	"CP367",
	"CP819",
	"CP850",
	"CP862",
	"CP866",
	"CP874",
	"CP932",
	"CP936",
	"CP949",
	"CP950",
	"CSASCII",
	"CSBIG5",
	"CSEUCKR",
	"CSEUCPKDFMTJAPANESE"
	"CSEUCTW",
	"CSGB2312",
	"CSHALFWIDTHKATAKANA",
	"CSHPROMAN8",
	"CSIBM866",
	"CSISO14JISC6220RO",
	"CSISO159JISX02121990",
	"CSISO2022CN",
	"CSISO2022JP",
	"CSISO2022JP2",
	"CSISO2022KR",
	"CSISO57GB1988",
	"CSISO58GB231280",
	"CSISO87JISX0208",
	"CSISOLATIN1",
	"CSISOLATIN2",
	"CSISOLATIN3",
	"CSISOLATIN4",
	"CSISOLATIN5",
	"CSISOLATIN6",
	"CSISOLATINARABIC",
	"CSISOLATINCYRILLIC",
	"CSISOLATINGREEK",
	"CSISOLATINHEBREW",
	"CSKOI8R",
	"CSKSC56011987",
	"CSMACINTOSH",
	"CSPC850MULTILINGUAL",
	"CSPC862LATINHEBREW",
	"CSSHIFTJIS",
	"CSUCS4",
	"CSUNICODE",
	"CSUNICODE11",
	"CSUNICODE11UTF7",
	"CSVISCII",
	"CYRILLIC",
	"ECMA-114",
	"ECMA-118",
	"ELOT_928",
	"EUC-CN",
	"EUC-JP",
	"EUC-KR",
	"EUC-TW",
	"EUCCN",
	"EUCJP",
	"EUCKR",
	"EUCTW",
	"EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE",
	"GB18030",
	"GB2312",
	"GBK",
	"GB_1988-80",
	"GB_2312-80",
	"GEORGIAN-ACADEMY",
	"GEORGIAN-PS",
	"GREEK",
	"GREEK8",
	"HEBREW",
	"HP-ROMAN8",
	"HZ",
	"HZ-GB-2312",
	"IBM-CP1133",
	"IBM367",
	"IBM819",
	"IBM850",
	"IBM862",
	"IBM866",
	"ISO-10646-UCS-2",
	"ISO-10646-UCS-4",
	"ISO-2022-CN",
	"ISO-2022-CN-EXT",
	"ISO-2022-JP",
	"ISO-2022-JP-1",
	"ISO-2022-JP-2",
	"ISO-2022-KR",
	"ISO-8859-1",
	"ISO-8859-10",
	"ISO-8859-13",
	"ISO-8859-14",
	"ISO-8859-15",
	"ISO-8859-16",
	"ISO-8859-2",
	"ISO-8859-3",
	"ISO-8859-4",
	"ISO-8859-5",
	"ISO-8859-6",
	"ISO-8859-7",
	"ISO-8859-8",
	"ISO-8859-9",
	"ISO-CELTIC",
	"ISO-IR-100",
	"ISO-IR-101",
	"ISO-IR-109",
	"ISO-IR-110",
	"ISO-IR-126",
	"ISO-IR-127",
	"ISO-IR-138",
	"ISO-IR-14",
	"ISO-IR-144",
	"ISO-IR-148",
	"ISO-IR-149",
	"ISO-IR-157",
	"ISO-IR-159",
	"ISO-IR-165",
	"ISO-IR-166",
	"ISO-IR-179",
	"ISO-IR-199",
	"ISO-IR-203",
	"ISO-IR-226",
	"ISO-IR-57",
	"ISO-IR-58",
	"ISO-IR-6",
	"ISO-IR-87",
	"ISO646-CN",
	"ISO646-JP",
	"ISO646-US",
	"ISO8859-1",
	"ISO8859-10",
	"ISO8859-13",
	"ISO8859-14",
	"ISO8859-15",
	"ISO8859-16",
	"ISO8859-2",
	"ISO8859-3",
	"ISO8859-4",
	"ISO8859-5",
	"ISO8859-6",
	"ISO8859-7",
	"ISO8859-8",
	"ISO8859-9",
	"ISO_646.IRV:1991",
	"ISO_8859-1",
	"ISO_8859-10",
	"ISO_8859-10:1992",
	"ISO_8859-13",
	"ISO_8859-14",
	"ISO_8859-14:1998",
	"ISO_8859-15",
	"ISO_8859-15:1998",
	"ISO_8859-16",
	"ISO_8859-16:2001",
	"ISO_8859-1:1987",
	"ISO_8859-2",
	"ISO_8859-2:1987",
	"ISO_8859-3",
	"ISO_8859-3:1988",
	"ISO_8859-4",
	"ISO_8859-4:1988",
	"ISO_8859-5",
	"ISO_8859-5:1988",
	"ISO_8859-6",
	"ISO_8859-6:1987",
	"ISO_8859-7",
	"ISO_8859-7:1987",
	"ISO_8859-8",
	"ISO_8859-8:1988",
	"ISO_8859-9",
	"ISO_8859-9:1989",
	"JAVA",
	"JIS0208",
	"JISX0201-1976",
	"JIS_C6220-1969-RO",
	"JIS_C6226-1983",
	"JIS_X0201",
	"JIS_X0208",
	"JIS_X0208-1983",
	"JIS_X0208-1990",
	"JIS_X0212",
	"JIS_X0212-1990",
	"JIS_X0212.1990-0",
	"JOHAB",
	"JP",
	"KOI8-R",
	"KOI8-RU",
	"KOI8-T",
	"KOREAN",
	"KSC_5601",
	"KS_C_5601-1987",
	"KS_C_5601-1989",
	"L1",
	"L10",
	"L2",
	"L3",
	"L4",
	"L5",
	"L6",
	"L7",
	"L8",
	"LATIN-9",
	"LATIN1",
	"LATIN10",
	"LATIN2",
	"LATIN3",
	"LATIN4",
	"LATIN5",
	"LATIN6",
	"LATIN7",
	"LATIN8",
	"MAC",
	"MACARABIC",
	"MACCENTRALEUROPE",
	"MACCROATIAN",
	"MACCYRILLIC",
	"MACGREEK",
	"MACHEBREW",
	"MACICELAND",
	"MACINTOSH",
	"MACROMAN",
	"MACROMANIA",
	"MACTHAI",
	"MACTURKISH",
	"MACUKRAINE",
	"MS-ANSI",
	"MS-ARAB",
	"MS-CYRL",
	"MS-EE",
	"MS-GREEK",
	"MS-HEBR",
	"MS-TURK",
	"MS936",
	"MS_KANJI",
	"MULELAO-1",
	"NEXTSTEP",
	"R8",
	"ROMAN8",
	"SHIFT-JIS",
	"SHIFT_JIS",
	"SJIS",
	"TCVN",
	"TCVN-5712",
	"TCVN5712-1",
	"TCVN5712-1:1993",
	"TIS-620",
	"TIS620",
	"TIS620-0",
	"TIS620.2529-1",
	"TIS620.2533-0",
	"TIS620.2533-1",
	"UCS-2",
	"UCS-2-INTERNAL",
	"UCS-2-SWAPPED",
	"UCS-2BE",
	"UCS-2LE",
	"UCS-4",
	"UCS-4-INTERNAL",
	"UCS-4-SWAPPED",
	"UCS-4BE",
	"UCS-4LE",
	"UHC",
	"UNICODE-1-1",
	"UNICODE-1-1-UTF-7",
	"UNICODEBIG",
	"UNICODELITTLE",
	"US",
	"US-ASCII",
	"UTF-16",
	"UTF-16BE",
	"UTF-16LE",
	"UTF-32",
	"UTF-32BE",
	"UTF-32LE",
	"UTF-7",
	"UTF-8",
	"VISCII",
	"VISCII1.1-1",
	"WCHAR_T",
	"WINBALTRIM",
	"WINDOWS-1250",
	"WINDOWS-1251",
	"WINDOWS-1252",
	"WINDOWS-1253",
	"WINDOWS-1254",
	"WINDOWS-1255",
	"WINDOWS-1256",
	"WINDOWS-1257",
	"WINDOWS-1258",
	"WINDOWS-874",
	"WINDOWS-936",
	"X0201",
	"X0208",
	"X0212",
};

#endif // } 0

/*
//"KOI8-R"
//"MAC-CYRILLIC"
//"IBM855"
//"IBM866"
//"TIS-620"
//"VISCII"
//"WINDOWS-1250"
//"WINDOWS-1251"
//"WINDOWS-1252"
//"WINDOWS-1253"
//"WINDOWS-1255"
//"WINDOWS-1256"
//"WINDOWS-1258"
//"ISO-8859-1"
//"ISO-8859-2"
//"ISO-8859-3"
//"ISO-8859-5"
//"ISO-8859-6"
//"ISO-8859-7"
//"ISO-8859-9"
//"ISO-8859-11"
//"ISO-8859-15"

//"HZ-GB-2312"
//"ISO-2022-CN"
//"ISO-2022-JP"
//"ISO-2022-KR"
//"EUC-JP"
//"EUC-KR"
//"EUC-TW"
//"GB18030"
//"SHIFT_JIS"

*/

struct SCpEntry {
	int    Cp;
	const char * P_CLibLocale;
	const char * P_Canonical;
	const char * P_Xml;
	const char * P_Name;
};

static const SCpEntry __SCpL[] = {
	{ cpUndef, "", "", "" },
	{ cpANSI,  "ACP", "ANSI", "", "ANSI" },
	{ cpOEM,   "OCP", "OEM",  "", "OEM"  },

	{ cp437,   "437", "437", "CP437", "437 US MSDOS" },
	{ cp737,   "737", "737", "CP737", "737 Greek MSDOS" },
	{ cp850,   "850", "850", "CP850", "850 International MSDOS" },
	{ cp852,   "852", "852", "CP852", "852 EasernEuropean MSDOS" },
	{ cp855,   "855", "855", "CP855", "855 OEM Cyrillic" },
	{ cp857,   "857", "857", "CP857", "857 Turkish MSDOS" },
	{ cp861,   "861", "861", "CP861", "861 Icelandic MSDOS" },
	{ cp865,   "865", "865", "CP865", "865 Nordic MSDOS" },
	{ cp866,   "866", "866", "CP866", "866 Russian MSDOS" },
	{ cp932,   "932", "932", "CP932", "932 Japanese Windows" },
	{ cp936,   "936", "936", "CP936", "936 Chinese Windows" },
	{ cp950,   "950", "950", "CP950", "950 Chinese Windows" },
	{ cp10007, "10007", "10007", "windows-10007", "x-mac-cyrillic" },
	{ cp1250,  "1250",  "1250",  "windows-1250",  "1250 Eastern European Windows" },
	{ cp1251,  "1251",  "1251",  "windows-1251",  "1251 Russian Windows" },
	{ cp1252,  "1252",  "1252",  "windows-1252",  "1252 Windows ANSI" },
	{ cp1253,  "1253",  "1253",  "windows-1253",  "1253 Greek Windows" },
	{ cp1254,  "1254",  "1254",  "windows-1254",  "1254 Turkish Windows" },
	{ cp1255,  "1255",  "1255",  "windows-1255",  "1255 Hebrew Windows" },
	{ cp1256,  "1256",  "1256",  "windows-1256",  "1256 Arabic Windows" },
	{ cp1257,  "1257",  "1257",  "windows-1257",  "1257 Windows Baltic" },
	{ cp1258,  "1258",  "1258",  "windows-1258",  "1258 Vietnamese Windows" },

	{ cp20866,  "koi8-r",  "koi8-r",  "koi8-r",  "Cyrillic (KOI8-R)" },
	{ cpVISCII, "VISCII",  "VISCII",  "VISCII",  "Vietnamese Standard Code for Information Interchange" },
	{ cpTIS620, "TIS-620", "TIS-620", "TIS-620", "Thai Industrial Standard 620-2533" },

	{ cpUTF7,  "UTF7", "UTF7", "UTF-7", "CP_UTF7" },
	{ cpUTF8,  "UTF8", "UTF8", "UTF-8", "UTF8" },

	{ cp28591, "ISO-8859-1",  "ISO-8859-1",  "ISO-8859-1",  "ISO 8859-1 Latin 1; Western European" },
	{ cp28592, "ISO-8859-2",  "ISO-8859-2",  "ISO-8859-2",  "ISO 8859-2 Central European" },
	{ cp28593, "ISO-8859-3",  "ISO-8859-3",  "ISO-8859-3",  "ISO 8859-3 Latin 3" },
	{ cp28594, "ISO-8859-4",  "ISO-8859-4",  "ISO-8859-4",  "ISO 8859-4 Baltic" },
	{ cp28595, "ISO-8859-5",  "ISO-8859-5",  "ISO-8859-5",  "ISO 8859-5 Cyrillic" },
	{ cp28596, "ISO-8859-6",  "ISO-8859-6",  "ISO-8859-6",  "ISO 8859-6 Arabic" },
	{ cp28597, "ISO-8859-7",  "ISO-8859-7",  "ISO-8859-7",  "ISO 8859-7 Greek" },
	{ cp28598, "ISO-8859-8",  "ISO-8859-8",  "ISO-8859-8",  "ISO 8859-8 Hebrew" },
	{ cp28599, "ISO-8859-9",  "ISO-8859-9",  "ISO-8859-9",  "ISO 8859-9 Turkish" },
	{ cpISO_8859_10, "ISO-8859-10", "ISO-8859-10", "ISO-8859-10", "LATIN-6" },
	{ cpISO_8859_11, "ISO-8859-11", "ISO-8859-11", "ISO-8859-11", "Latin/Thai" },
	// { cpISO_8859_12, "ISO-8859-12", "ISO-8859-12", "ISO-8859-12", "Latin/Devanagari" },
	{ cp28603, "ISO-8859-13", "ISO-8859-13", "ISO-8859-13", "ISO 8859-13 Estonian" },
	{ cpISO_8859_14, "ISO-8859-14", "ISO-8859-14", "ISO-8859-14", "LATIN-8" },
	{ cp28605, "ISO-8859-15", "ISO-8859-15", "ISO-8859-15", "ISO 8859-15 Latin 9" },
	{ cpISO_8859_16, "ISO-8859-16", "ISO-8859-16", "ISO-8859-16", "LATIN-10" },

	{ cp52936, "HZ-GB-2312", "HZ-GB-2312", "HZ-GB-2312", "HZ-GB2312 Simplified Chinese" },
	{ cp50220, "iso-2022-jp", "iso-2022-jp", "iso-2022-jp", "ISO 2022 Japanese with no halfwidth Katakana" },
	{ cp50221, "iso-2022-jp", "iso-2022-jp", "iso-2022-jp", "ISO 2022 Japanese with halfwidth Katakana" },
	{ cp50222, "iso-2022-jp", "iso-2022-jp", "iso-2022-jp", "ISO 2022 Japanese JIS X 0201-1989" },
	{ cp50225, "iso-2022-kr", "iso-2022-kr", "iso-2022-kr", "ISO 2022 Korean" },
	{ cp50229, "iso-2022-cn", "iso-2022-cn", "iso-2022-cn", "ISO 2022 Traditional Chinese" },

	{ cp51932, "euc-jp", "euc-jp", "euc-jp", "EUC Japanese" },
	{ cp51936, "euc-cn", "euc-cn", "euc-cn", "EUC Simplified Chinese" },
	{ cp51949, "euc-kr", "euc-kr", "euc-kr", "EUC Korean" },
	{ cp51950, "euc-tw", "euc-tw", "euc-tw", "EUC Traditional Chinese" },
	{ cp54936, "GB18030", "GB18030", "GB18030", "GB18030 Simplified Chinese" },
	{ cpShiftJIS, "shift_jis", "shift_jis", "shift_jis", "Japanese (Shift-JIS)" },
};

/*static*/uint SCodepageIdent::GetRegisteredCodepageCount()
{
	return SIZEOFARRAY(/*CodepageNames*/__SCpL);
}

/*static*/int SCodepageIdent::GetRegisteredCodepage(uint idx, SCodepage & rCp, SString & rName)
{
	int    ok = 1;
	if(idx < SIZEOFARRAY(/*CodepageNames*/__SCpL)) {
		rCp = /*CodepageNames*/(SCodepage)__SCpL[idx].Cp;
		if(isempty(__SCpL[idx].P_Name))
			rName = __SCpL[idx].P_Canonical;
		else
			rName = /*CodepageNames*/__SCpL[idx].P_Name;
	}
	else
		ok = 0;
	return ok;
}

SCodepageIdent::SCodepageIdent() : Cp(cpANSI)
{
}

SCodepageIdent::SCodepageIdent(int cp) : Cp(cp)
{
}

SCodepageIdent::operator int() const { return static_cast<int>(Cp); }
SCodepageIdent::operator SCodepage() const { return static_cast<SCodepage>(Cp); }
bool FASTCALL SCodepageIdent::operator == (SCodepage cp) const { return (Cp == cp); }
bool FASTCALL SCodepageIdent::operator != (SCodepage cp) const { return (Cp != cp); }

SCodepageIdent & FASTCALL SCodepageIdent::operator = (SCodepage cp)
{
	Cp = static_cast<SCodepage>(cp);
	return *this;
}

int SCodepageIdent::FromStr(const char * pStr)
{
	int    ok = 0;
	SString & r_inp_buf = SLS.AcquireRvlStr(); // @v9.9.4
	(r_inp_buf = pStr).Strip();
	for(uint i = 0; !ok && i < SIZEOFARRAY(__SCpL); i++) {
		const SCpEntry & r_entry = __SCpL[i];
		if(!isempty(r_entry.P_Xml) && r_inp_buf.CmpNC(r_entry.P_Xml) == 0) {
			Cp = r_entry.Cp;
			ok = 1;
		}
		else if(!isempty(r_entry.P_Canonical) && r_inp_buf.CmpNC(r_entry.P_Canonical) == 0) {
			Cp = r_entry.Cp;
			ok = 2;
		}
		else if(!isempty(r_entry.P_CLibLocale) && r_inp_buf.CmpNC(r_entry.P_CLibLocale) == 0) {
			Cp = r_entry.Cp;
			ok = 3;
		}
	}
	return ok;
}

int SCodepageIdent::ToStr(int fmt, SString & rBuf) const
{
	rBuf.Z();
	if(oneof2(fmt, fmtCLibLocale, fmtXML)) {
		for(uint i = 0; i < SIZEOFARRAY(__SCpL); i++) {
			const SCpEntry & r_entry = __SCpL[i];
			if(Cp == r_entry.Cp) {
				if(fmt == fmtCLibLocale) {
					if(r_entry.P_CLibLocale[0]) {
						rBuf.Dot().Cat(r_entry.P_CLibLocale);
					}
				}
				else if(fmt == fmtXML) {
					if(!isempty(r_entry.P_Xml)) {
						rBuf.Cat(r_entry.P_Xml);
					}
				}
				else {
					if(r_entry.P_Canonical[0]) {
						if(Cp >= 437 && Cp <= 1256)
							rBuf.Cat("cp").Cat(r_entry.P_Canonical);
						else
							rBuf = r_entry.P_Canonical;
					}
				}
				break;
			}
		}
	}
	return rBuf.NotEmpty();
}
//
//
//
uint   FASTCALL hex(char c) { return isdec(c) ? (c-'0') : ((c >= 'A' && c <= 'F') ? (c-'A'+10) : ((c >= 'a' && c <= 'f') ? (c-'a'+10) : 0)); }
uint   FASTCALL hexw(wchar_t c) { return (c >= L'0' && c <= L'9') ? (c-L'0') : ((c >= L'A' && c <= L'F') ? (c-L'A'+10) : ((c >= L'a' && c <= L'f') ? (c-L'a'+10) : 0)); }

uint8 FASTCALL hextobyte(const char * pBuf)
{
	return (ishex(pBuf[0]) && ishex(pBuf[1])) ? hex2(pBuf[0], pBuf[1]) : 0; // @v11.9.3
	/* @v11.9.3
	uint8  c0 = pBuf[1];
	uint8  c1 = pBuf[0];
	uint8  r = 0;
	if(c0 >= '0' && c0 <= '9')
		r |= c0 - '0';
	else if(c0 >= 'A' && c0 <= 'F')
		r |= c0 - 'A' + 10;
	else if(c0 >= 'a' && c0 <= 'f')
		r |= c0 - 'a' + 10;
	else
		return 0;
	if(c1 >= '0' && c1 <= '9')
		r |= (c1 - '0') << 4;
	else if(c1 >= 'A' && c1 <= 'F')
		r |= (c1 - 'A' + 10) << 4;
	else if(c1 >= 'a' && c1 <= 'f')
		r |= (c1 - 'a' + 10) << 4;
	else
		return 0;
	return r;
	*/
}

/*static int experimental_isasciialpha(int c) 
{
    return ((uint)(c | 32) - 97) < 26U;
	//unsigned((ch&(~(1<<5))) - 'A') <= ('Z' - 'A')
}*/
// @v11.3.6 (inlined) int FASTCALL isasciialpha(char ch) { return (((uchar)(ch | 32) - 97) < 26U); }
// @v11.3.6 (inlined) int FASTCALL isasciialnum(char ch) { return ((ch >= '0' && ch <= '9') || (((uchar)(ch | 32) - 97) < 26U)); }
// @v10.9.3 (replaced with isasciialpha) int FASTCALL IsLetterASCII(int ch) { return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')); }

int FASTCALL ToLower1251(int alpha)
{
	uint   ch = (alpha & 0x00ff);
	if(ch == 0xa8U)
		ch = 0xb8U;
	else if(ch >= 0xc0U && ch <= 0xdfU)
		ch += 32;
	else if(isalpha(ch) && isupper(ch))
		ch = _tolower(ch);
	return ch;
}

int FASTCALL ToUpper1251(int alpha)
{
	uint   ch = (alpha & 0x00ff);
	if(ch == 0xb8U)
		ch = 0xa8U;
	else if(ch >= 0xe0U && ch <= 0xffU)
		ch -= 32;
	else if(isalpha(ch) && islower(ch))
		ch = _toupper(ch);
	return ch;
}
//
// Descr: переводит символы русского алфавита (cp866) в нижний регистр
//   если символ не является русской буквой, то вызывается стандартная функция tolower()
//
int FASTCALL ToLower866(int alpha)
{
	uint   ch = (alpha & 0x00ff);
	if(IsLetter866(ch)) {
		if(ch >= 0x80 && ch < 0x90)
			ch += 0x20;
		else if(ch >= 0x90 && ch < 0xa0)
			ch += 0x50;
		else if(ch == 0xf0)
			ch += 0x01;
	}
	else if(isalpha(ch) && isupper(ch))
		ch = _tolower(ch);
	return ch;
}
//
// Descr: переводит символы русского алфавита (cp866) в верхний регистр
//   если символ не является русской буквой, то вызывается функция TurboC toupper()
//
int FASTCALL ToUpper866(int alpha)
{
	uint   ch = (alpha & 0x00ff);
	if(IsLetter866(ch)) {
		if(ch >= 0xa0U && ch < 0xb0U)
			ch -= 0x20;
		else if(ch >= 0xe0U && ch < 0xf0U)
			ch -= 0x50;
		else if(ch == 0xf1U) // 0xf1 // @v11.3.2 @fix 0xf0U-->0xf1U
			ch -= 0x01;
	}
	else if(isalpha(ch) && islower(ch))
		ch = _toupper(ch);
	return ch;
}

char * FASTCALL stristr866(const char * s1, const char * s2)
{
	for(size_t p = 0, i = sstrlen(s1), len = sstrlen(s2); (i-p) >= len; p++)
		if(strnicmp866(s1+p, s2, len) == 0)
			return const_cast<char *>(s1+p); // @badcast
	return 0;
}

int FASTCALL stricmp866(const char * s1, const char * s2)
{
	// @v11.3.2 @fix Добавлена специальная обработка пустых строк: (s1 == 0 && s2 != 0 && s2[0] == 0)=>(s1==s2); 
	const size_t len1 = sstrlen(s1);
	const size_t len2 = sstrlen(s2);
	if(len1 != 0 && len2 != 0) { 
		int    c1, c2;
		do {
			c1 = ToUpper866(*s1++);
			c2 = ToUpper866(*s2++);
			if(c1 > c2)
				return 1;
			else if(c1 < c2)
				return -1;
		} while(c1 && c2);
		return 0;
	}
	else if(len1 == 0 && len2 == 0)
		return 0;
	else
		return CMPSIGN(len1, len2);
}

int FASTCALL stricmp1251(const char * s1, const char * s2)
{
	// @v11.3.2 @fix Добавлена специальная обработка пустых строк: (s1 == 0 && s2 != 0 && s2[0] == 0)=>(s1==s2); 
	const size_t len1 = sstrlen(s1);
	const size_t len2 = sstrlen(s2);
	if(len1 != 0 && len2 != 0) { 
		int    c1, c2;
		do {
			c1 = ToUpper1251(*s1++);
			c2 = ToUpper1251(*s2++);
			if(c1 > c2)
				return 1;
			else if(c1 < c2)
				return -1;
		} while(c1 && c2);
		return 0;
	}
	else if(len1 == 0 && len2 == 0)
		return 0;
	else
		return CMPSIGN(len1, len2);
}

int FASTCALL strnicmp866(const char * s1, const char * s2, size_t maxlen)
{
	uchar  c1, c2;
	size_t i = 0;
	if(maxlen)
		do {
			c1 = static_cast<uchar>(ToUpper866(*s1++));
			c2 = static_cast<uchar>(ToUpper866(*s2++));
			if(c1 > c2)
				return 1;
			else if(c1 < c2)
				return -1;
		} while(c1 && c2 && ++i < maxlen);
	return 0;
}

char * FASTCALL strlwr1251(char * str)
{
	if(str) {
		for(char * s = str; *s; s++)
			*s = ToLower1251(*s);
	}
	return str;
}

char * FASTCALL strupr1251(char * str)
{
	if(str) {
		for(char * s = str; *s; s++)
			*s = ToUpper1251(*s);
	}
	return str;
}

char * FASTCALL strlwr866(char * str)
{
	if(str) {
		for(char * s = str; *s; s++)
			*s = ToLower866(*s);
	}
	return str;
}

char * FASTCALL strupr866(char * str)
{
	if(str) {
		for(char * s = str; *s; s++)
			*s = ToUpper866(*s);
	}
	return str;
}
//
// KOI
//
int FASTCALL __866_to_koi7(int c)
{
	static const char koi7tab[] =
		"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
		"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"

		"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F"
		"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F"
		"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
		"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F"
		"\x27\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
		"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x3C\x49\x3E\x5E\x7F"
		"\x61\x62\x77\x67\x64\x65\x76\x7A\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70"
		"\x72\x73\x74\x75\x66\x68\x63\x7E\x7B\x7D\x27\x79\x78\x7C\x60\x71"
		"\x61\x62\x77\x67\x64\x65\x76\x7A\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70"
		"\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F"
		"\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F"
		"\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F"
		"\x72\x73\x74\x75\x66\x68\x63\x7E\x7B\x7D\x27\x79\x78\x7C\x60\x71"
		"\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x3F\x7F";
	return koi7tab[static_cast<uchar>(c)];
}

char * FASTCALL _s_866_to_koi7(char * s)
{
	for(char * p = s; *p; p++)
		*p = static_cast<uchar>(__866_to_koi7(*p));
	return s;
}

int FASTCALL _koi8_to_866(int c)
{
	static const char koi8tab[] =
		"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
		"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
		"\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F"
		"\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F"
		"\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F"
		"\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F"
		"\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F"
		"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F"
		"\xC4\xB3\xDA\xBF\xC0\xD9\xC3\xB4\xC2\xC1\xC5\xDF\xDC\xDB\xDD\xDE"
		"\xB0\xB1\xB2\xF4\xFE\xF9\xFB\xF7\xF3\xF2\xFF\xF5\xF8\xFD\xFA\xF6"
		"\xCD\xBA\xD5\xF1\xD6\xC9\xB8\xB7\xBB\xD4\xD3\xC8\xBE\xBD\xBC\xC6"
		"\xC7\xCC\xB5\xF0\xB6\xB9\xD1\xD2\xCB\xCF\xD0\xCA\xD8\xD7\xCE\xFC"
		"\xEE\xA0\xA1\xE6\xA4\xA5\xE4\xA3\xE5\xA8\xA9\xAA\xAB\xAC\xAD\xAE"
		"\xAF\xEF\xE0\xE1\xE2\xE3\xA6\xA2\xEC\xEB\xA7\xE8\xED\xE9\xE7\xEA"
		"\x9E\x80\x81\x96\x84\x85\x94\x83\x95\x88\x89\x8A\x8B\x8C\x8D\x8E"
		"\x8F\x9F\x90\x91\x92\x93\x86\x82\x9C\x9B\x87\x98\x9D\x99\x97\x9A";
	if(c < 128)
		return koi8tab[static_cast<size_t>(c)];
	else
		for(size_t i = 0; i < 256; i++)
			if(koi8tab[i] == c)
				return koi8tab[i];
	return c;
}
//
// OEM <-> CHAR
//
char * FASTCALL SOemToChar(char * pStr)
{
	if(pStr)
		OemToCharA(pStr, pStr); // @unicodeproblem
	return pStr;
}

char * FASTCALL SCharToOem(char * pStr)
{
	if(pStr)
		CharToOemA(pStr, pStr); // @unicodeproblem
	return pStr;
}
//
//
//
static FORCEINLINE size_t FASTCALL implement_sstrlen(const char * pStr) { return (pStr /*&& pStr[0]*/) ? strlen(pStr) : 0; }
static FORCEINLINE size_t FASTCALL implement_sstrlen(const uchar * pStr) { return (pStr /*&& pStr[0]*/) ? strlen(reinterpret_cast<const char *>(pStr)) : 0; }
static FORCEINLINE size_t FASTCALL implement_sstrlen(const wchar_t * pStr) { return (pStr /*&& pStr[0]*/) ? wcslen(pStr) : 0; }
//
// Descr: копирует сроку from в буфер to и возвращает указатель на
//   завершающий нулевой символ строки to.
//
char * FASTCALL stpcpy(char * to, const char * from)
{
	size_t len = implement_sstrlen(from);
	if(len)
		memcpy(to, from, len+1);
	return (to+len);
}
//
//
//
//bool   FASTCALL isempty(const char * pStr) { return (pStr == 0 || pStr[0] == 0); }
//bool   FASTCALL isempty(const uchar * pStr) { return (pStr == 0 || pStr[0] == 0); }
//bool   FASTCALL isempty(const wchar_t * pStr) { return (pStr == 0 || pStr[0] == 0); }
//size_t FASTCALL sstrlen(const char * pStr) { return implement_sstrlen(pStr); }
//size_t FASTCALL sstrlen(const uchar * pStr) { return implement_sstrlen(pStr); }
//size_t FASTCALL sstrlen(const wchar_t * pStr) { return implement_sstrlen(pStr); }

uint FASTCALL iseol(const char * pStr, SEOLFormat eolf)
{
	return isempty(pStr) ? 0 : implement_iseol(pStr, eolf);
}

size_t FASTCALL sstrnlen(const char * pStr, size_t maxLen)
{
	// @v11.7.0 memchr-->smemchr
	if(pStr) {
		const char * p_end = static_cast<const char *>(smemchr(pStr, 0, maxLen)); 
		return p_end ? (size_t)(p_end - pStr) : maxLen;
	}
	else
		return 0;
}
//
// @v11.7.10 Внесено уточнение во все функции sstrchr: при поиске нулевого символа возвращается (pStr+strlen(pStr))
// это соответствует спецификации strchr согласно которой завершающий символ трактуется как часть строки и может
// быть найден с возвратом адекватного результата, а не нуля.
//
const char * FASTCALL sstrchr(const char * pStr, char c)
{
	const  size_t len = implement_sstrlen(pStr);
	// @v11.7.0 memchr-->smemchr
	const  char * p = c ? (len ? static_cast<const char *>(smemchr(pStr, static_cast<uchar>(c), len)) : 0) : (pStr+len);
	return p;
}

char * FASTCALL sstrchr(char * pStr, char c)
{
	const  size_t len = implement_sstrlen(pStr);
	// @v11.7.0 memchr-->smemchr
	char * p = c ? (len ? static_cast<char *>(const_cast<void *>(smemchr(pStr, static_cast<uchar>(c), len))) : 0) : (pStr+len);
	return p;
}

const wchar_t * FASTCALL sstrchr(const wchar_t * pStr, wchar_t c)
{
	const  size_t len = implement_sstrlen(pStr);
	if(c) {
		for(size_t i = 0; i < len; i++) {
			if(pStr[i] == c)
				return (pStr+i);
		}
		return 0;
	}
	else
		return (pStr+len);
}

wchar_t * FASTCALL sstrchr(wchar_t * pStr, wchar_t c)
{
	const  size_t len = implement_sstrlen(pStr);
	if(c) {
		for(size_t i = 0; i < len; i++) {
			if(pStr[i] == c)
				return (pStr+i);
		}
		return 0;
	}
	else
		return (pStr+len);
}

#if 1 // @v11.8.9 @construction 
//
// Замечания по реализации sstrrch:
// наиболее очевидный вариант реализации - сканирование строки с конца в начало с проверкой каждого символа.
// Но этому подходу есть возражение: современные архитектуры "заточены" на сканирование памяти с меньших адресов
// к большим. В результате, может оказаться, что прямое сканирование, хоть и потребляет больше итераций в номинальном
// выражении, в реальности окажется быстрее.
// Для варианта с однобайтовыми строками strrchr(char *, char)  у меня есть очень быстрая реализация smemchr
// из-за чего я не задумываясь применяю прямое сканирование.
// Для unicode-же (strrchr(wchar_t *, wchar_t)) я реализовал и прямой и обратный подходы с целью 
// сравнить производительность того и другого экспериментально.
// 
// Обращу внимание на то, что поиск нулевого символа (strrchr(text, '0')) трактуется как валидный вызов
// и возвращает указатель на '\0'-терминатор строки (хотя смысла в таком вызове и не много).
//
static FORCEINLINE const char * FASTCALL implement_sstrrchr(const char * pStr, char c)
{
	const char * p_result = 0;
	const size_t len = implement_sstrlen(pStr);
	if(c) {
		if(len) {
			const char * p = pStr;
			do {
				assert(p && static_cast<ssize_t>(len) >= (p - pStr)); // @paranoic
				p = static_cast<const char *>(smemchr(p, c, len - (p - pStr)));
				if(p)
					p_result = p++;
			} while(p && *p);
		}
	}
	else if(pStr)
		p_result = (pStr+len);
	return p_result;
}

const char  * FASTCALL sstrrchr(const char * pStr, char c) { return implement_sstrrchr(pStr, c); }
char  * FASTCALL sstrrchr(char * pStr, char c) { return const_cast<char *>(implement_sstrrchr(pStr, c)); }
//
//
//
FORCEINLINE const wchar_t * FASTCALL implement_sstrrchr_reverse(const wchar_t * pStr, wchar_t c)
{
	const wchar_t * p_result = 0;
	const size_t len = implement_sstrlen(pStr);
	if(c) {
		if(len) {
			for(const wchar_t * p_last = pStr+len-1; !p_result && p_last >= pStr;) {
				if(*p_last == c)
					p_result = p_last;
				else
					--p_last;
			}
		}
	}
	else if(pStr)
		p_result = (pStr+len);
	return p_result;
}

FORCEINLINE const wchar_t * FASTCALL implement_sstrrchr_direct(const wchar_t * pStr, wchar_t c)
{
	const wchar_t * p_result = 0;
	const size_t len = implement_sstrlen(pStr);
	if(c) {
		if(len) {
			const wchar_t * p_end = (pStr + len);
			for(const wchar_t * p_cur = pStr; p_cur < p_end; p_cur++) {
				if(*p_cur == c)
					p_result = p_cur;
			}
		}
	}
	else if(pStr)
		p_result = (pStr+len);
	return p_result;
}

const wchar_t * FASTCALL sstrrchr(const wchar_t * pStr, wchar_t c)
{
	return implement_sstrrchr_reverse(pStr, c);
}

wchar_t * FASTCALL sstrrchr(wchar_t * pStr, wchar_t c)
{
	return const_cast<wchar_t *>(implement_sstrrchr_reverse(pStr, c));
}

#endif // } 0 @v11.8.9 @construction 

char * FASTCALL sstrdup(const char * pStr)
{
	if(pStr) {
		size_t len = implement_sstrlen(pStr) + 1;
		char * p = static_cast<char *>(SAlloc::M(len));
		return p ? static_cast<char *>(memcpy(p, pStr, len)) : 0;
	}
	else
		return 0;
}

uchar * FASTCALL sstrdup(const uchar * pStr)
{
	if(pStr) {
		size_t len = implement_sstrlen(reinterpret_cast<const char *>(pStr)) + 1;
		uchar * p = static_cast<uchar *>(SAlloc::M(len));
		return p ? static_cast<uchar *>(memcpy(p, pStr, len)) : 0;
	}
	else
		return 0;
}

wchar_t * FASTCALL sstrdup(const wchar_t * pStr)
{
	if(pStr) {
		size_t len = implement_sstrlen(pStr) + 1;
		wchar_t * p = static_cast<wchar_t *>(SAlloc::M(len * sizeof(wchar_t)));
		return p ? static_cast<wchar_t *>(memcpy(p, pStr, len * sizeof(wchar_t))) : 0;
	}
	else
		return 0;
}

bool FASTCALL sstreq(const wchar_t * pS1, const wchar_t * pS2)
{
	if(pS1 != pS2) {
		if(pS1) {
			if(!pS2)
				return pS1[0] ? false : true;
			else
				return (pS1[0] == pS2[0]) ? (pS1[0] == 0 || wcscmp(pS1, pS2) == 0) : false;
		}
		else
			return pS2[0] ? false : true;
	}
	else
		return true;
}

bool FASTCALL sstreq(const char * pS1, const char * pS2)
{
	if(pS1 != pS2) {
		if(pS1) {
			if(!pS2)
				return pS1[0] ? false : true;
			else
				return (pS1[0] == pS2[0]) ? (pS1[0] == 0 || strcmp(pS1, pS2) == 0) : false;
		}
		else
			return pS2[0] ? false : true;
	}
	else
		return true;
}

bool FASTCALL sstreq(const uchar * pS1, const uchar * pS2)
{
	if(pS1 != pS2) {
		if(pS1) {
			if(!pS2)
				return pS1[0] ? false : true;
			else
				return (pS1[0] == pS2[0]) ? (pS1[0] == 0 || strcmp(reinterpret_cast<const char *>(pS1), reinterpret_cast<const char *>(pS2)) == 0) : false;
		}
		else
			return pS2[0] ? false : true;
	}
	else
		return true;
}

bool FASTCALL sstreq(const uchar * pS1, const char * pS2)
{
	if(pS1 != reinterpret_cast<const uchar *>(pS2))
		if(pS1) {
			if(!pS2)
				return pS1[0] ? false : true;
			else
				return (pS1[0] == pS2[0]) ? (pS1[0] == 0 || strcmp(reinterpret_cast<const char *>(pS1), pS2) == 0) : false;
		}
		else
			return pS2[0] ? false : true;
	else
		return true;
}

// @construction
bool STDCALL sstrneq(const char * pS1, const char * pS2, uint len)
{
	if(!len)
		return true;
	else if(pS1 != pS2) {
		if(pS1 && pS2) {
			switch(len) {
				case 1: return (*pS1 == *pS2);
				//
				// Строго говоря, следующие 3 case'а - опасные: если строки pS1 или pS2 заканчиваются раньше чем соответствующий
				// байт будет достигнут и плюс к тому данные по указателям обрываются на границе сегмента то мы
				// рискуем получить исключение по доступу на чтение к недоступной области памяти.
				// Однако, я думаю что вероятность такого события может быть проигнорирована.
				// Тем не менее, следует сделать тест на предмет такого развития событий.
				//
				case 2: return (*PTR16C(pS1) == *PTR16C(pS2));
				case 4: return (*PTR32C(pS1) == *PTR32C(pS2));
				case 8: return (*PTR64C(pS1) == *PTR64C(pS2));
				default: 
					{
						uint x = 0;
						if(len >= 4) {
							for(; x < len-4; x+=4) {
								pS1 += 4;
								pS2 += 4;
								uint8 c1 = *(pS1-4);
								if(c1 != *(pS2-4))
									return false;
								else if(c1 == 0)
									return true;
								else {
									c1 = *(pS1-3);
									if(c1 != *(pS2-3))
										return false;
									else if(c1 == 0)
										return true;
									else {
										c1 = *(pS1-2);
										if(c1 != *(pS2-2))
											return false;
										else if(c1 == 0)
											return true;
										else {
											c1 = *(pS1-1);
											if(c1 != *(pS2-1))
												return false;
											else if(c1 == 0)
												return true;
										}
									}
								}
							}
						}
						for(; x < len; x++) {
							if(*pS1 != *pS2)
								return false;
							else if(*pS1 == 0)
								return true;
							else {
								pS1++;
								pS2++;
							}
						}
						return false;
					}
					break;
			}
		}
		else
			return false;
	}
	else
		return true;
}

bool FASTCALL sstreqi_ascii(const char * pS1, const char * pS2)
{
	if(pS1 != pS2) {
        const size_t len = implement_sstrlen(pS1);
        if(len != sstrlen(pS2))
			return false;
		else if(len) {
            for(size_t i = 0; i < len; i++)
				if(!chreqi_ascii(static_cast<int>(pS1[i]), static_cast<int>(pS2[i])))
					return false;
		}
	}
	return true;
}

bool FASTCALL sstreqi_ascii(const uchar * pS1, const uchar * pS2)
{
	if(pS1 != pS2) {
        const size_t len = implement_sstrlen(pS1);
        if(len != implement_sstrlen(pS2))
			return false;
		else if(len) {
            for(size_t i = 0; i < len; i++)
				if(!chreqi_ascii(static_cast<int>(pS1[i]), static_cast<int>(pS2[i])))
					return false;
		}
	}
	return true;
}

bool FASTCALL sstreqi_ascii(const wchar_t * pS1, const wchar_t * pS2)
{
	if(pS1 != pS2) {
        const size_t len = implement_sstrlen(pS1);
        if(len != sstrlen(pS2))
			return false;
		else if(len) {
            for(size_t i = 0; i < len; i++)
				if(!chreqi_ascii(static_cast<int>(pS1[i]), static_cast<int>(pS2[i])))
					return false;
		}
	}
	return true;
}

bool FASTCALL sstreqi_ascii(const wchar_t * pS1, const char * pS2)
{
	if(static_cast<const void *>(pS1) != static_cast<const void *>(pS2)) {
        const size_t len = implement_sstrlen(pS1);
        if(len != implement_sstrlen(pS2))
			return false;
		else if(len) {
            for(size_t i = 0; i < len; i++)
				if(!chreqi_ascii(static_cast<int>(pS1[i]), static_cast<int>(pS2[i])))
					return false;
		}
	}
	return true;
}

bool STDCALL sstreqni_ascii(const char * pS1, const char * pS2, size_t maxlen)
{
	if(static_cast<const void *>(pS1) != static_cast<const void *>(pS2)) {
		const size_t len1 = smin(implement_sstrlen(pS1), maxlen);
		const size_t len2 = smin(implement_sstrlen(pS2), maxlen);
		if(len1 != len2)
			return false;
		else if(len1) {
            for(size_t i = 0; i < len1; i++)
				if(!chreqi_ascii(static_cast<int>(pS1[i]), static_cast<int>(pS2[i])))
					return false;
		}
	}
	return true;
}

int FASTCALL sstrcmpi_ascii(const char * pS1, const char * pS2)
{
	if(pS1 && pS2) {
		int c1;
		int c2;
		do {
			c1 = *pS1++;
			c2 = *pS2++;
			if(c1 != c2) {
				if(c1 >= 'A' && c1 <= 'Z')
					c1 += ('a' - 'A');
				if(c2 >= 'A' && c2 <= 'Z')
					c2 += ('a' - 'A');				
				if(c1 != c2)
					return (c1 - c2);
			}
			else if(c1 == 0)
				return 0;
		} while(true);
	}
	else if(pS1 && !pS2)
		return -1;
	else if(!pS1 && pS2)
		return +1;
	return 0;
}

bool FASTCALL sisascii(const char * pS, size_t len)
{
	bool   yes = true;
	if(pS && len) {
		const size_t oct_count = len / 8;
		size_t p = 0;
		const int8 * _ = reinterpret_cast<const int8 *>(pS);
		for(uint i = 0; yes && i < oct_count; i++) {
			yes = (((_[p] | _[p+1] | _[p+2] | _[p+3] | _[p+4] | _[p+5] | _[p+6] | _[p+7]) & 0x80) == 0);
			p += 8;
		}
		while(yes && p < len) {
			if(_[p++] < 0)
				yes = false;
		}
	}
	return yes;
}

bool FASTCALL sisascii(const wchar_t * pS, size_t len/* length of pS in characters (not bytes)*/)
{
	bool   yes = true;
	if(pS && len) {
		for(uint i = 0; yes && i < len; i++) {
			if(pS[i] < 0 || pS[i] > 127)
				yes = false;
		}
	}
	return yes;
}

char * FASTCALL newStr(const char * s)
{
	if(s) {
		size_t len = implement_sstrlen(s) + 1;
		char * p = new char[len];
		return p ? (char *)memcpy(p, s, len) : 0;
	}
	else
		return 0;
}

char * FASTCALL sstrcpy(char * pDest, const char * pSrc) { return strcpy(pDest, pSrc); }
uchar * FASTCALL sstrcpy(uchar * pDest, const uchar * pSrc) { return reinterpret_cast<uchar *>(strcpy(reinterpret_cast<char *>(pDest), reinterpret_cast<const char *>(pSrc))); }
wchar_t * FASTCALL sstrcpy(wchar_t * pDest, const wchar_t * pSrc) { return wcscpy(pDest, pSrc); }
char * STDCALL strnzcpy(char * dest, const uchar * src, size_t maxlen) { return strnzcpy(dest, reinterpret_cast<const char *>(src), maxlen); }

char * STDCALL strnzcpy(char * dest, const char * src, size_t maxlen)
{
	if(dest) {
		if(src) {
			if(maxlen) {
				//
				// @v11.7.12 Пришлось перестроить код: проблема в том, что smemchr может "посмотреть" чуть дальше чем находится '\0'
				// и таким образом привести к исключению по чтению недуступного адреса.
				// Новый код быстрее (strlen быстрее чем smemchr), но если src не содержит нуля в разумных границах, то
				// исключение все равно будет.
				//
				/* @v11.7.12 const char * p = static_cast<const char *>(smemchr(src, 0, maxlen)); // @v11.7.0 memchr-->smemchr
				if(p)
					memcpy(dest, src, (size_t)(p - src)+1);
				else {
					memcpy(dest, src, maxlen-1);
					dest[maxlen-1] = 0;
				}*/
				// @v11.7.12 {
				const size_t src_len = strlen(src);
				if(src_len < maxlen)
					memcpy(dest, src, src_len+1);
				else {
					memcpy(dest, src, maxlen-1);
					dest[maxlen-1] = 0;					
				}
				// } @v11.7.12 
			}
			else
				strcpy(dest, src);
		}
		else
			dest[0] = 0;
	}
	return dest;
}

char * STDCALL strnzcpy(char * pDest, const SString & rSrc, size_t maxlen)
{
	rSrc.CopyTo(pDest, maxlen);
	return pDest;
}

wchar_t * STDCALL strnzcpy(wchar_t * dest, const wchar_t * src, size_t maxlen)
{
	if(dest) {
		if(src) {
			if(maxlen) {
				const wchar_t * p = static_cast<const wchar_t *>(wmemchr(src, 0, maxlen));
				if(p) {
					memcpy(dest, src, (size_t)(((p - src)+1) << 1));
				}
				else {
					memcpy(dest, src, ((maxlen-1) << 1));
					dest[maxlen-1] = 0;
				}
			}
			else
				wcscpy(dest, src);
		}
		else {
			dest[0] = 0;
		}
	}
	return dest;
}

char * FASTCALL sstrncat(char * pDest, size_t destSize, const char * pSrc) // @v11.0.0
{
	if(pDest && destSize) {
		const size_t src_len = implement_sstrlen(pSrc);
		if(src_len) {
			const size_t dest_len = strlen(pDest);
			if(destSize > (dest_len+1))
				strnzcpy(pDest+dest_len, pSrc, destSize-dest_len);
		}
	}
	return pDest;
}

char * FASTCALL trimleft(char * pStr)
{
	char * p = pStr;
	if(*p == ' ') {
		do {
			p++;
		} while(*p == ' ');
		memmove(pStr, p, implement_sstrlen(p)+1);
	}
	return pStr;
}

char * FASTCALL trimright(char * pStr)
{
	size_t len = implement_sstrlen(pStr);
	if(len) {
		size_t t = len-1;
		while(t && pStr[t] == ' ')
			--t;
		if(t+1 < len)
			pStr[t+1] = 0;
	}
	return pStr;
}

char * FASTCALL strip(char * pStr)
{
	const size_t org_len = implement_sstrlen(pStr);
	if(org_len) {
		char * p = pStr;
		char * q = pStr;
		char * back = pStr + org_len - 1;
		while(*back == ' ' && back >= p)
			back--;
		while(*p == ' ' && p <= back)
			p++;
		if(p != q) {
			while(p <= back)
				*q++ = *p++;
			p = q;
			*p = 0;
		}
		else
			back[1] = 0;
	}
	return pStr;
}

char * FASTCALL chomp(char * s)
{
	if(s) {
		size_t n = implement_sstrlen(s);
		if(s[n-1] == '\n') {
			if(s[n-2] == '\r')
				s[n-2] = 0;
			else
				s[n-1] = 0;
		}
	}
	return s;
}

/* @v10.0.0 const char * skipws(const char * pStr, size_t * pPos)
{
	if(pStr) {
		size_t pos = DEREFPTRORZ(pPos);
		const char * p = pStr+pos;
		while(*p != ' ' && *p != '\t')
			p++;
		if(pPos)
			*pPos = (p - pStr);
		return p;
	}
	else
		return 0;
}*/

/* @v9.6.5 const char * FASTCALL onecstr(char c)
{
#ifdef _WIN32_WCE
	static char buf[2];
	buf[0] = c;
	buf[1] = 0;
	return buf;
#else
	char * p_buf = SLS.GetTLA().OneCStrBuf;
	p_buf[0] = c;
	p_buf[1] = 0;
	return p_buf;
#endif
} */

/* @v9.4.10 (unused) char * quotstr(char * pS, int leftQuotChr, int rightQuotChr)
{
	if(pS) {
		size_t n = strlen(pS);
		memmove(pS+1, pS, n+1);
		pS[0] = leftQuotChr;
		pS[n+1] = rightQuotChr;
		pS[n+2] = 0;
	}
	return pS;
}*/

/* @v9.4.10 (unused) char * catdiv(char * pStr, int div, int addSpaces)
{
	if(addSpaces)
		*pStr++ = ' ';
	*pStr++ = div;
	if(addSpaces)
		*pStr++ = ' ';
	return pStr;
}*/
//
// Предполагается, что под строку выделено
// достаточно места, чтобы вместить набивку
//
char * STDCALL padleft(char * pStr, char pad, size_t n)
{
	memmove(pStr+n, pStr, implement_sstrlen(pStr)+1);
	memset(pStr, pad, n);
	return pStr;
}

char * STDCALL padright(char * pStr, char pad, size_t n)
{
	size_t len = implement_sstrlen(pStr);
	memset(pStr + len, pad, n);
	pStr[len+n] = 0;
	return pStr;
}

char * alignstr(char * pStr, size_t wd, int adj)
{
	if(pStr) {
		size_t len = implement_sstrlen(strip(pStr));
		if(wd > len) {
			size_t n = (wd - len);
			switch(adj) {
				case ADJ_LEFT:
					return padright(pStr, ' ', n);
				case ADJ_RIGHT:
					return padleft(pStr, ' ', n);
				case ADJ_CENTER:
					len = n/2;
					return padright(padleft(pStr, ' ', len), ' ', n-len);
			}
		}
	}
	return pStr;
}

#if 0 // @v9.5.1 {
int getTextHight(char * str, int width)
{
	char   ch = *str;
	int    y, i, pi;
	for(y = 0, i = 0; ch; ++y) {
		for(pi = i; ch && ch != '\n';)
			if(width == 0 || i - pi < width)
				ch = str[++i];
			else
				break;
		if(ch == '\n')
			ch = str[++i];
	}
	return y;
}
#endif // } 0

static int FASTCALL iswordchar(int ch, const char * /*pWordChars*/)
{
	return (ch && (isalnum(ch) || ch == '_' || IsLetter866(ch)));
}

int searchstr(const char * pStr, const SSrchParam & rParam, size_t * pBeg, size_t * pLen)
{
	size_t pos = DEREFPTRORZ(pBeg);
	const  char * s = pStr + pos;
	const  char * p;
	const  char * pat = rParam.P_Pattern;
	const  char * wch = rParam.P_WordChars;
	int    f   = rParam.Flags;
	if(!isempty(pStr) && !isempty(pat)) {
		while(1) {
			if((p = (f & SSPF_NOCASE) ? stristr866(s, pat) : strstr(s, pat)) != 0) {
				size_t len = implement_sstrlen(pat);
				pos += static_cast<uint>(p - s);
				if(f & SSPF_WORDS)
					if(iswordchar(p[len], wch)) {
						s = &pStr[++pos];
						p = 0;
					}
					else if(pos && iswordchar(pStr[pos-1], wch)) {
		   	        	s = &pStr[++pos];
			   	        p = 0;
				   	}
				if(p) {
					ASSIGN_PTR(pBeg, pos);
					ASSIGN_PTR(pLen, len);
					return 1;
				}
			}
			else
				return 0;
		}
	}
	return 0;
}

int hostrtocstr(const char * pInBuf, char * pOutBuf, size_t outBufSize)
{
	int    digit = -1, base = 0;
	uint   prcsd_symbs = 0;
	size_t pos = 0, len = 0;
	size_t in_buf_len = implement_sstrlen(pInBuf);
	SSrchParam ss_p("\\", 0, 0);
	strnzcpy(pOutBuf, pInBuf, outBufSize);
	while(searchstr(pOutBuf, ss_p, &pos, &len) > 0) {
		if((pos+1) < in_buf_len)
			if(oneof2(pOutBuf[pos+1], 'x', 'X'))
				base = 16;
			else if(pOutBuf[pos+1] == '0')
				base = 8;
		if(base) {
			int    i = 0;
			int    pow_d = 0;
			prcsd_symbs = 2;
			for(i = 2; i >= 0; i--) {
				if(pos + i + 2 < in_buf_len) {
					int    symb = pOutBuf[pos + i + 2];
					int    dig  = -1;
					if(isdec(symb)) {
						dig = (symb - '0');
						dig = (base == 8 && dig > 7) ? -1 : dig;
					}
					else if(base == 16) {
						if(symb == 97 || symb == 65)
							dig = 10;
						else if(symb == 98 || symb == 66)
							dig = 11;
						else if(symb == 99 || symb == 67)
							dig = 12;
						else if(symb == 100 || symb == 68)
							dig = 13;
						else if(symb == 101 || symb == 69)
							dig = 14;
						else if(symb == 102 || symb == 70)
							dig = 15;
						else
							dig = -1;
					}
					if(dig >= 0) {
						prcsd_symbs++;
						digit = (digit < 0) ? 0 : digit;
						digit += (int)pow((double)base, pow_d) * dig;
						pow_d++;
					}
					else {
						digit = -1;
						prcsd_symbs = 2;
						pow_d = 0;
					}
				}
			}
			if(digit >= 0 && digit <= 255) {
				len = prcsd_symbs;
				char   srch_buf[8];
				srch_buf[0] = static_cast<char>(digit);
				srch_buf[1] = 0;
				replacestr(pOutBuf, srch_buf, &pos, &len, outBufSize);
			}
		}
		pos += len;
	}
	return 1;
}

#pragma warn -par

int replacestr(char * str, const char * rstr, size_t * pPos, size_t * pLen, uint maxlen)
{
	if(str && pPos && pLen) {
		size_t p = *pPos;
		size_t len = *pLen;
		size_t rl = implement_sstrlen(rstr);
		char * s  = str + p;
		memmove(s + rl, s + len, sstrlen(s+len) + 1);
		if(rl)
			memmove(s, rstr, rl);
		*pLen = rl;
		return 1;
	}
	return 0;
}

#pragma warn .par

int SplitBuf(HDC hdc, SString & aBuf, size_t maxStrSize, size_t maxStrsCount)
{
	if(hdc && maxStrSize > 0 && maxStrsCount > 0 && aBuf.Len()) {
		const  char * p_dots = "...";
		char   ret_buf[1024];
		int    src_pos = 0, dest_pos = 0;
		int    dots_pos = -1;
		size_t dots_size = 0;
		SIZE   size;
		memzero(ret_buf, sizeof(ret_buf));
#ifdef _WIN32_WCE // {
		GetTextExtentPoint32(hdc, (const ushort*)".", 1, &size);
#else
		GetTextExtentPoint32(hdc, _T("."), 1, &size);
#endif // } _WIN32_WCE
		dots_size = size.cx * 3;
		for(size_t strs_count = 0; strs_count < maxStrsCount; strs_count++) {
			int    is_last_str = BIN(strs_count >= maxStrsCount - 1);
			int    src_spc_pos = 0, dest_spc_pos = 0;
			size_t word_size = 0;
			for(; word_size < maxStrSize && aBuf.C(src_pos);) {
				if(aBuf.C(src_pos) == ' ') {
					src_spc_pos = src_pos;
					dest_spc_pos = dest_pos;
				}
				if(is_last_str)
					dots_pos = word_size + (dots_size <= maxStrSize) ? dest_pos : dots_pos;
#ifdef _WIN32_WCE // {
				GetTextExtentPoint32(hdc, &aBuf[src_pos], 1, &size);
#else
				GetTextExtentPoint32(hdc, SUcSwitch(&aBuf[src_pos]), 1, &size); // @unicodeproblem
#endif // } _WIN32_WCE
				word_size += size.cx;
				if(word_size <= maxStrSize) {
					ret_buf[dest_pos] = aBuf.C(src_pos);
					src_pos++;
					dest_pos++;
				}
			}
			char   c = aBuf.C(src_pos);
			if(c) {
				if(is_last_str) {
					if(dots_pos >= 0) {
						ret_buf[dots_pos] = '\0';
						strcat(ret_buf, p_dots);
						dest_pos = dots_pos + sstrleni(p_dots) - 1;
					}
				}
				else
					if(c != ' ' && src_spc_pos) {
						src_pos  = src_spc_pos + 1;
						dest_pos = dest_spc_pos;
					}
			}
			if(!is_last_str)
				ret_buf[dest_pos++] = '\n';
			else
				ret_buf[++dest_pos] = '\0';
		}
		aBuf.CopyFrom(ret_buf);
	}
	return 1;
}
//
//
//
//
// Index into the table below with the first byte of a UTF-8 sequence to
// get the number of trailing bytes that are supposed to follow it.
// Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
// left as-is for anyone who may want to do such conversion, which was
// allowed in earlier algorithms.
//
const char SUtfConst::TrailingBytesForUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x20
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x40
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x60
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x80
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xA0
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xC0
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xE0
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5  // 0x100
};

const uint8 SUtfConst::Utf8EncLen_RFC3629[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const uint8 SUtfConst::Utf8EncLen_[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};
//
// Magic values subtracted from a buffer value during uint8 conversion.
// This table contains as many values as there might be trailing bytes
// in a UTF-8 sequence.
//
const uint32 SUtfConst::OffsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };
//
// Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
// into the first byte, depending on how many bytes follow.  There are
// as many entries in this table as there are UTF-8 sequence types.
// (I.e., one byte sequence, two byte... etc.). Remember that sequencs
// for *legal* UTF-8 will be 4 or fewer bytes total.
//
const uint8 SUtfConst::FirstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

const uint32 SUtfConst::HalfBase = 0x0010000UL;
const uint32 SUtfConst::HalfMask = 0x3FFUL;
const int    SUtfConst::HalfShift = 10; // used for shifting by 10 bits
//
// Функции нечеткого поиска подстроки.
// Позаимствовано у Kevin Atkinson
//
// Descr:
//   Функция ApproxStrSrch осуществляет приблизительный поиск заданного ключевого слова(далее pattern) в заданной строке,
//   наверняка содержащей заданное ключевое слово (далее buffer). В случае нахождения pattern`а где-либо в buffer`е функция //
//   записывает в структуру ApproxStrSrchParam позицию (далее pos) наиболее схожего слова и количество набранных этим словом
//   "очков" (далее score) по сравнению с pattern`ом, в результате возвращая '1'. В случае неудачного поиска(во всем buffer`е
//   нет такого pattern`а) функция возвращает '-1'. В случае ошибки функция возвращает '0'.
//
// Params:
//   const char * pPattern -
//   Ключевая строка поиска.
//   const char * pBuf -
//   Строка, где должно искаться значение.
//   double* mVal -
//   Если в конце поиска максимальное значение score на позиции pos
//   меньше этого значения, то функция считает такой поиск неудачным
//   (см. таблицу возвращаемых значений).
//   struct ApproxStrSrchParam* param -
//   структура ApproxStrSrchParam.
//
// Return values map:
//   -1 : Поиск закончился неудачей.
//   0  : Возникла ошибка
//   Если был возвращен 0, то структура ApproxStrSrchParam должна
//   	 содержать описание ошибки.
//   1  : Поиск был успешным
//   	Если была возвращена 1, то структура ApproxStrSrchParam
//   	должна содержать наиболее подходящюю позицию и количество
//   	набранных баллов.
//   Структура ApproxStrSrchParam.
//
//   Структура ApproxStrSrchParam содержит 2 переменные:
//
//   size_t maxpos - позиция максимального соответствия и
//   double maxscore - значение score в позиции maxpos
//
struct dist_weights {
	dist_weights(double d1, double d2, double sw, double su) : Del1(d1), Del2(d2), Swaps(sw), Subs(su)
	{
	}
	void set(double d1, double d2, double sw, double su)
	{
		Del1 = d1;
		Del2 = d2;
		Swaps = sw;
		Subs = su;
	}
	double Del1;
	double Del2;
	double Swaps;
	double Subs;
};

struct dist_components {
	dist_components(int d1, int d2, int sw, int su) : Del1(d1), Del2(d2), Swaps(sw), Subs(su)
	{
	}
	void set(int d1, int d2, int sw, int su)
	{
		Del1 = d1;
		Del2 = d2;
		Swaps = sw;
		Subs = su;
	}
	int    Del1;
	int    Del2;
	int    Swaps;
	int    Subs;
};

class HighestScore {
public:
	HighestScore(double weight) : Weight(weight), Score(0.0), S1(0), S2(0)
	{
	}
	void Clear()
	{
		Score = 0.0;
	}
	int GetFirst() const { return S1; }
	int GetSecond() const { return S2; }
	double FASTCALL WeighedScore(const char *s1, const char *s2) const
	{
		if(Weight == 1.0) {
			long si = 0;
			while(*s1 && *s2)
				if(*s1++ == *s2++)
					++si;
			return (double)si;
		}
		else {
			double score = 0.0;
			double p = 1.0;
			while(*s1 && *s2) {
				if(*s1++ == *s2++)
					score += p;
				p *= Weight;
			}
			return score;
		}
	}
	void Process(const char * pS1, const char * pS2)
	{
		uint i;
		Score = WeighedScore(pS1, pS2);
		S1 = 0;
		S2 = 0;
		for(i = 1; pS1[i]; ++i) {
			double sc = WeighedScore(pS1+i, pS2);
			if(sc > Score) {
				Score = sc;
				S1 = i;
				S2 = 0;
			}
		}
		for(i = 1; pS2[i]; ++i) {
			double sc = WeighedScore(pS1, pS2+i);
			if(sc > Score) {
				Score = sc;
				S1 = 0;
				S2 = i;
			}
		}
	}
private:
	double Weight; // Коэффициент веса каждого последующего символа
	double Score;
	int    S1;
	int    S2;
};

ApproxStrSrchParam::ApproxStrSrchParam() : umin(0.0), weight(0.0), method(0), no_case(0), maxscore(0.0), maxpos(0)
{
}

inline double ApproxStrComparator::Distance() { return MIN(Del1 + Del2 + Swaps + Subs, MaxSize); }
inline double ApproxStrComparator::Score() { return (MaxSize - Distance()) / MaxSize; }

inline double FASTCALL ApproxStrComparator::Distance(const dist_weights & w)
	{ return MIN(w.Del1 * Del1 + w.Del2 * Del2 + w.Swaps * Swaps + w.Subs * Subs, MaxSize); }
inline double FASTCALL ApproxStrComparator::Score(const dist_weights & w)
	{ return (MaxSize - Distance(w)) / MaxSize; }
inline void FASTCALL ApproxStrComparator::Distance(dist_components & c)
	{ c.set(Del1, Del2, Swaps, Subs); }

ApproxStrComparator::ApproxStrComparator(const char * pPattern, const ApproxStrSrchParam * param) :
	Del1(0), Del2(0), Swaps(0), Subs(0), MaxSize(0), Pattern(pPattern)
{
	P = *param;
	if(P.no_case)
		Pattern.ToLower();
}

double FASTCALL ApproxStrComparator::Next(const char * b2)
{
	double result = 1.0;
	SString & r_temp = SLS.AcquireRvlStr();
	r_temp = b2;
	if(P.no_case) {
		r_temp.ToLower();
		b2 = (const char *)r_temp;
	}
	if(Pattern != r_temp) {
		HighestScore higest(P.weight);
		Del1 = Del2 = Swaps = Subs = 0;
		MaxSize = 0;
		Track1.Clear();
		Track2.Clear();
		for(const char * s1 = Pattern, * s2 = b2; ; s1++, s2++) {
			uint n = 0;
			while(*s1 == *s2 && *s1) {
				++n;
				++s1;
				++s2;
			}
			if(n) {
				Track1.insertN(1, n);
				Track2.insertN(1, n);
			}
			if(!*s1) {
				n = 0;
				while(*s2) {
					++n;
					++s2;
				}
				Track2.insertN(0, n);
				break;
			}
			else if(!*s2) {
				n = 0;
				while(*s1) {
					++n;
					++s1;
				}
				Track1.insertN(0, n);
				break;
			}
			else {
				higest.Process(s1, s2);
				n = higest.GetFirst();
				Track1.insertN(0, n);
				s1 += n;
				n = higest.GetSecond();
				Track2.insertN(0, n);
				s2 += n;
				if(*s1 == *s2) {
					Track1.insert(1);
					Track2.insert(1);
				}
				else {
					Track1.insert(0);
					Track2.insert(0);
				}
			}
		}
		size_t i1 = 0;
		size_t i2 = 0;
		size_t size1 = Track1.getCount();
		size_t size2 = Track2.getCount();
		for(;;) {
			if(i1 == size1) {
				if(P.method == 0)
					Del2 += size2 - i2;
				break;
			}
			if(i2 == size2) {
				Del1 += size1 - i1;
				break;
			}
			const int tr1 = Track1[i1];
			const int tr2 = Track2[i2];
			if(tr1 == tr2) {
				if(tr1 == 0)
					++Subs;
				++i1;
				++i2;
			}
			else if((i1 + 1 != size1) && (i2 + 1 != size2) &&
				tr1 != Track1[i1 + 1] && tr2 != Track2[i2 + 1] && Pattern.C(i1) == b2[i2 + 1] && Pattern.C(i1 + 1) == b2[i2]) {
				++Swaps;
				i1 += 2;
				i2 += 2;
			}
			else if(tr1 == 0) {
				++Del1;
				++i1;
			}
			else if(tr2 == 0) {
				++Del2;
				++i2;
			}
		}
		MaxSize = (P.method == 0) ? ((size1 > size2) ? size1 : size2) : size1;
		result = Score();
	}
	return result;
}

int ApproxStrCmp(const char * pStr1, const char * pStr2, int noCase, double * pScore)
{
	ApproxStrSrchParam param;
	param.method = 1;
	param.weight = 1.0;
	ApproxStrComparator srch(pStr1, &param);
	double s = srch.Next(pStr2);
	ASSIGN_PTR(pScore, s);
	return (s == 1.0) ? 1 : -1;
}

int ApproxStrSrch(const char * pPattern, const char * pBuffer, ApproxStrSrchParam * param)
{
	int    ok = 1;
	size_t buflen = implement_sstrlen(pBuffer);
	size_t patlen = implement_sstrlen(pPattern);
	if(patlen == 0 && buflen == 0)
		ok = 1;
	else if(patlen == 0 || buflen == 0)
		ok = 0;
	else {
		size_t maxpos = static_cast<size_t>(-1);
		double maxscore = 0.0;
		ApproxStrComparator search(pPattern, param);
		for(size_t j = 0; j < buflen; j++) {
			double s = search.Next(pBuffer+j);
			if(s > maxscore) {
				maxpos = j;
				maxscore = s;
			}
		}
		if(maxscore < param->umin)
			ok = -1;
		else {
			param->maxscore = maxscore;
			param->maxpos = maxpos;
		}
	}
	return ok;
}

int FASTCALL ExtStrSrch(const char * pBuffer, const char * pPattern, uint flags)
{
	int    ok = 1;
	int    done = 0;
	// const  char * p_or_div = "€‹€"; // ИЛИ (cp866)
	const  char * p_or_dir = "||";
	const  char * p_and_dir = "&&";
	if(pBuffer && pPattern) {
		SString temp_buf;
		uint   p = 0;
		// div: 1 - OR, 2 - AND
		for(int div = 0; !div && pPattern[p]; p++) {
			const  char c = pPattern[p];
			uint   inc = 0;
			if(c == '|' && pPattern[p+1] == '|') {
				if(p && pPattern[p-1] == '\\') {
					temp_buf.TrimRight().CatCharN('|', 2);
				}
				else {
					inc = 2;
					div = 1;
				}
			}
			else if(c == '&' && pPattern[p+1] == '&') {
				if(p && pPattern[p-1] == '\\') {
					temp_buf.TrimRight().CatCharN('&', 2);
				}
				else {
					inc = 2;
					div = 2;
				}
			}
			if(div) {
				const int r1 = ExtStrSrch(pBuffer, temp_buf, flags); // @recursion
				if(div == 1) { // OR
					if(r1)
						ok = 1;
					else {
						const int r2 = ExtStrSrch(pBuffer, pPattern+p+inc, flags); // @recursion
						ok = BIN(r2);
					}
				}
				else if(div == 2) { // AND
					if(!r1)
						ok = 0;
					else {
						const int r2 = ExtStrSrch(pBuffer, pPattern+p+inc, flags); // @recursion
						ok = BIN(r2);
					}
				}
				else
					ok = 0;
				done = 1;
			}
			else
				temp_buf.CatChar(c);
		}
		if(!done) {
			const char * p_srch_str = temp_buf;
			if(*p_srch_str == '!' && *++p_srch_str != '!') {
				ApproxStrSrchParam sp;
				sp.weight = 1;
				sp.method = 1;
				sp.no_case = 1;
				if(*p_srch_str == '(') {
					int i, j;
					for(i = j = 0, p_srch_str++; i < 2 && *p_srch_str != ')'; i++, p_srch_str++)
						if(isdec(*p_srch_str))
							j = j * 10 + *p_srch_str - '0';
						else {
							ok = 0;
							break;
						}
					if(ok && *p_srch_str == ')') {
						p_srch_str++;
						sp.umin = fdiv100i(j);
					}
					else
						ok = 0;
				}
				else
					sp.umin = 0.75;
				if(ok && ApproxStrSrch(p_srch_str, pBuffer, &sp) != 1)
					ok = 0;
			}
			else {
				if(p_srch_str[0] == '%' && p_srch_str[1] == '^') {
					p_srch_str += 2;
					const size_t prefix_len = strlen(p_srch_str);
					if(prefix_len) {
						if(flags & essfCaseSensitive) {
							ok = BIN(strncmp(pBuffer, p_srch_str, prefix_len) == 0);
						}
						else {
							ok = BIN(strnicmp866(pBuffer, p_srch_str, prefix_len) == 0);
						}
					}
					else
						ok = 1; // Вырожденный паттерн - считаем, что любая строка для него годится //
				}
				else {
					const char * p_local_r = (flags & essfCaseSensitive) ? strstr(pBuffer, p_srch_str) : stristr866(pBuffer, p_srch_str);
					if(!p_local_r)
						ok = 0;
					/*if(!stristr866(pBuffer, p_srch_str))
						ok = 0;*/
				}
			}
		}
	}
	else
		ok = 0;
	return ok;
}

static const uint8 Utf_k_Boms[][3] = {
	{0x00, 0x00, 0x00},  // Unknown
	{0xEF, 0xBB, 0xBF},  // UTF8
	{0xFE, 0xFF, 0x00},  // Big endian
	{0xFF, 0xFE, 0x00},  // Little endian
};

size_t FASTCALL SGetUnicodeModeBomSize(SUnicodeMode m)
{
	switch(m) {
		case suni16BE: return 2;
		case suni16LE: return 2;
		case suniUTF8: return 3;
	}
	return 0;
}

SUnicodeMode FASTCALL SDetermineUtfEncoding(const void * pBuf, size_t bufLen)
{
	// detect UTF-16 big-endian with BOM
	if(bufLen > 1 && PTR8C(pBuf)[0] == Utf_k_Boms[suni16BE][0] && PTR8C(pBuf)[1] == Utf_k_Boms[suni16BE][1])
		return suni16BE;
	else if(bufLen > 1 && PTR8C(pBuf)[0] == Utf_k_Boms[suni16LE][0] && PTR8C(pBuf)[1] == Utf_k_Boms[suni16LE][1]) // detect UTF-16 little-endian with BOM
		return suni16LE;
	else if(bufLen > 2 && PTR8C(pBuf)[0] == Utf_k_Boms[suniUTF8][0] && PTR8C(pBuf)[1] == Utf_k_Boms[suniUTF8][1] && PTR8C(pBuf)[2] == Utf_k_Boms[suniUTF8][2]) // detect UTF-8 with BOM
		return suniUTF8;
	else
		return suni8Bit;
}

SEOLFormat FASTCALL SDetermineEOLFormat(const void * pBuf, size_t bufLen)
{
	for(size_t i = 0 ; i < bufLen; i++) {
		if(PTR8C(pBuf)[i] == '\x0D') {
			if((i+1) < bufLen && PTR8C(pBuf)[i+1] == '\x0A')
				return eolWindows;
			else
				return eolMac;
		}
		else if(PTR8C(pBuf)[i] == '\x0A')
			return eolUnix;
	}
	return eolUndef;
}
//
//
//
STextEncodingStat::STextEncodingStat(long options) : LangID(0)
{
	Init(options);
}

STextEncodingStat::~STextEncodingStat()
{
}

STextEncodingStat & STextEncodingStat::Init(long options)
{
	Flags = fEmpty;
	EncDetectionBuf.Z(); // @v11.6.2
	LangID = 0; // @v11.6.2
	if(options & fUseIcuCharDet)
		Flags |= fUseIcuCharDet;
	else if(options & fUseUCharDet)
		Flags |= fUseUCharDet;
	Cp = cpUndef;
	CpName[0] = 0;
	Eolf = eolUndef;
	MEMSZERO(ChrFreq);
	memzero(Utf8Prefix, sizeof(Utf8Prefix));
	Utf8PrefixLen = 0;
	return *this;
}

int STextEncodingStat::Add(const void * pData, size_t size)
{
	int    ok = 1;
	if(size && pData) {
		if(Flags & fEmpty) {
			Flags &= ~fEmpty;
			Flags |= (fAsciiOnly|fLegalUtf8Only);
		}
		// @v11.6.2 {
		if(Flags & (fUseIcuCharDet|fUseUCharDet)) {
			const size_t enc_det_buf_maxlen = SKILOBYTE(8);
			if(EncDetectionBuf.Len() < enc_det_buf_maxlen) {
				if((size+EncDetectionBuf.Len()) > enc_det_buf_maxlen)
					EncDetectionBuf.CatN(static_cast<const char *>(pData), enc_det_buf_maxlen-EncDetectionBuf.Len());
				else
					EncDetectionBuf.CatN(static_cast<const char *>(pData), size);
			}
			assert(EncDetectionBuf.Len() <= enc_det_buf_maxlen);
		}
		// } @v11.6.2 
		size_t skip_eolf_pos = 0;
		size_t next_utf8_pos = 0;
		const uint8 * p = static_cast<const uint8 *>(pData);
		for(size_t i = 0; i < size; i++) {
			const uint8 c = p[i];
			ChrFreq[c]++;
			if(checkirange(c, static_cast<uint8>(1), static_cast<uint8>(127))) {
				if(c == '\x0D') {
					skip_eolf_pos = i+1;
					if((i+1) < size && p[i+1] == '\x0A') {
						if(Eolf == eolUndef)
							Eolf = eolWindows;
						else if(Eolf != eolWindows)
							Flags |= fMiscEolf;
					}
					else {
						if(Eolf == eolUndef)
							Eolf = eolMac;
						else if(Eolf != eolMac)
							Flags |= fMiscEolf;
					}
				}
				else if(c == '\x0A' && i != skip_eolf_pos) {
					if(Eolf == eolUndef)
						Eolf = eolUnix;
					else if(Eolf != eolUnix)
						Flags |= fMiscEolf;
				}
			}
			else {
				Flags &= ~fAsciiOnly;
			}
			if(i >= next_utf8_pos) {
				if(Utf8PrefixLen) {
					assert(i == 0);
					assert(Utf8PrefixLen <= 8);
					uint8  _sb[16];
					memcpy(_sb, Utf8Prefix, Utf8PrefixLen);
					memcpy(_sb+Utf8PrefixLen, p, MIN(sizeof(_sb) - Utf8PrefixLen, size));
					uint16 extra = SUtfConst::TrailingBytesForUTF8[_sb[0]];
					if(!SUnicode::IsLegalUtf8Char(_sb, extra+1))
						Flags &= ~fLegalUtf8Only;
					else
						next_utf8_pos = extra+1-Utf8PrefixLen;
					Utf8PrefixLen = 0;
					memzero(Utf8Prefix, sizeof(Utf8Prefix));
				}
				else {
					uint16 extra = SUtfConst::TrailingBytesForUTF8[c];
					if((i+extra+1) <= size) {
						if(!SUnicode::IsLegalUtf8Char(p+i, extra+1))
							Flags &= ~fLegalUtf8Only;
						else
							next_utf8_pos = i+extra+1;
					}
					else {
						Utf8PrefixLen = (i+extra+1)-size;
						memcpy(Utf8Prefix, p+i, Utf8PrefixLen);
						next_utf8_pos = size;
					}
				}
			}
		}
	}
	return ok;
}

void STextEncodingStat::Finish()
{
	if(EncDetectionBuf.Len()) {
		if(Flags & fUseIcuCharDet) { // @v11.6.2
			UErrorCode icu_status = U_ZERO_ERROR;
			UCharsetDetector * p_provider = ucsdet_open(&icu_status);
			if(p_provider) {
				UErrorCode icu_status = U_ZERO_ERROR;
				ucsdet_setText(p_provider, EncDetectionBuf.cptr(), EncDetectionBuf.Len(), &icu_status);
				if(!icu_status) {
					const UCharsetMatch * p_ucm = ucsdet_detect(p_provider, &icu_status);
					if(p_ucm) {
						const char * p_cp_symb = ucsdet_getName(p_ucm, &icu_status);
						int32 conf = ucsdet_getConfidence(p_ucm, &icu_status);
						const char * p_lang = ucsdet_getLanguage(p_ucm, &icu_status);
						if(!isempty(p_cp_symb)) {
							STRNSCPY(CpName, p_cp_symb);
							LangID = RecognizeLinguaSymb(p_lang, 0);
							Flags |= fUCharDetWorked;
						}
					}
				}
				ucsdet_close(p_provider);
			}
		}
		else if(Flags & fUseUCharDet) {
			uchardet_t p_provider = uchardet_new();
			if(p_provider) {
				uchardet_handle_data(p_provider, EncDetectionBuf, EncDetectionBuf.Len());
				uchardet_data_end(p_provider);
				const char * p_cp_symb = uchardet_get_charset(p_provider);
				if(p_cp_symb) {
					STRNSCPY(CpName, p_cp_symb);
					Flags |= fUCharDetWorked;
				}
			}
			uchardet_delete(p_provider);
		}
	}
}

SCodepageIdent STextEncodingStat::GetAutodetectedCp() const
{
	SCodepageIdent cp = cpUndef;
	const char * p_cp_name = GetCpName();
	if(sstreqi_ascii(p_cp_name, "IBM866"))
		cp = cp866;
	else if(p_cp_name && strnicmp(p_cp_name, "windows", 7) == 0)
		cp.FromStr(p_cp_name);
	else if(p_cp_name && strnicmp(p_cp_name, "iso", 3) == 0)
		cp.FromStr(p_cp_name);
	return cp;
}
//
// Finds the first occurrence of the sub-string needle in the string haystack.
// Returns NULL if needle was not found.
//
const char * byteshift_strstr(const char * pHayStack, const char * pNeedle)
{
	if(!*pNeedle) // Empty needle.
		return pHayStack;
	const char needle_first  = *pNeedle;
	// Runs strchr() on the first section of the haystack as it has a lower
	// algorithmic complexity for discarding the first non-matching characters.
	pHayStack = sstrchr(pHayStack, needle_first);
	if(!pHayStack) // First character of needle is not in the haystack.
		return NULL;
	// First characters of haystack and needle are the same now. Both are
	// guaranteed to be at least one character long.
	// Now computes the sum of the first needle_len characters of haystack
	// minus the sum of characters values of needle.
	const uchar * i_haystack = (const uchar *)pHayStack + 1;
	const uchar * i_needle   = (const uchar *)pNeedle + 1;
	bool identical = true;
	while(*i_haystack && *i_needle) {
		identical &= *i_haystack++ == *i_needle++;
	}
	// i_haystack now references the (needle_len + 1)-th character.
	if(*i_needle) // haystack is smaller than needle.
		return NULL;
	else if(identical)
		return pHayStack;
	size_t needle_len = i_needle - (const uchar *)pNeedle;
	// Note: needle_len > 1, because we checked that it isn't zero, and if it
	//       is 1 then identical must be true because the first strchr() ensured
	//       that the first characters are identical
	const char * sub_start = pHayStack;
	size_t compare_len;
	ulong last_needle_chars;
	ulong last_haystack_chars;
	ulong mask;
	size_t needle_cmp_len = (needle_len < sizeof(long)) ? needle_len : sizeof(long);
#ifdef SL_BIGENDIAN
	last_needle_chars = MAKE_ULONG_BIGENDIAN(*((ulong *)(i_needle - needle_cmp_len))) >> (8 * (LONG_INT_N_BYTES - needle_cmp_len));
	last_haystack_chars = MAKE_ULONG_BIGENDIAN(*((ulong *)(i_haystack - needle_cmp_len))) >> (8 * (LONG_INT_N_BYTES - needle_cmp_len));
#else
	const uchar * needle_cmp_end = i_needle;
	i_needle -= needle_cmp_len;
	i_haystack -= needle_cmp_len;
	last_needle_chars = 0;
	last_haystack_chars = 0;
	while(i_needle != needle_cmp_end) {
		last_needle_chars <<= 8;
		last_needle_chars ^= *i_needle++;
		last_haystack_chars <<= 8;
		last_haystack_chars ^= *i_haystack++;
	}
#endif
	// At this point:
	// * needle is at least two characters long
	// * haystack is at least needle_len characters long (also at least two)
	// * the first characters of needle and haystack are identical
	if(needle_len > (sizeof(long) + 1)) {
		// we will call memcmp() only once we know that the LONG_INT_N_BYTES
		// last chars are equal, so it will be enough to compare all but the
		// last LONG_INT_N_BYTES characters 
		compare_len = needle_len - sizeof(long);
		// iterate through the remainder of haystack while checking for identity
		// of the last LONG_INT_N_BYTES, and checking the rest with memcmp()
		while(*i_haystack) {
			last_haystack_chars <<= 8;
			last_haystack_chars ^= *i_haystack++;
			sub_start++;
			if(last_haystack_chars == last_needle_chars && memcmp(sub_start, pNeedle, compare_len) == 0) {
				return sub_start;
			}
		}
	}
	else if(needle_len == (sizeof(long) + 1)) {
		// iterate through the remainder of haystack while checking for identity
		// of the last LONG_INT_N_BYTES as well as the single additional
		// character, which is the first one 
		while(*i_haystack) {
			last_haystack_chars <<= 8;
			last_haystack_chars ^= *i_haystack++;
			sub_start++;
			if(last_haystack_chars == last_needle_chars && *sub_start == needle_first) {
				return sub_start;
			}
		}
	}
	else if(needle_len == sizeof(long)) {
		// iterate through the remainder of haystack while checking for identity
		// of the last LONG_INT_N_BYTES characters, which should exactly match
		// the entire needle
		while(*i_haystack) {
			last_haystack_chars <<= 8;
			last_haystack_chars ^= *i_haystack++;
			if(last_haystack_chars == last_needle_chars) {
				return reinterpret_cast<const char *>(i_haystack - needle_len);
			}
		}
	}
	else { /* needle_len < LONG_INT_N_BYTES */
		mask = ((1UL) << (needle_len * 8)) - 1;
		last_needle_chars &= mask;
		// iterate through the remainder of haystack, updating the sums' difference
		// and checking for identity whenever the difference is zero */
		while(*i_haystack) {
			last_haystack_chars <<= 8;
			last_haystack_chars ^= *i_haystack++;
			last_haystack_chars &= mask;
			if(last_haystack_chars == last_needle_chars) {
				return reinterpret_cast<const char *>(i_haystack - needle_len);
			}
		}
	}
	return NULL;
}
//
// Descr: Попытка сформулировать хорошие спецификации для функций преобразования типизированных
//   бинарных данных (int, double, etc) в текст и обратно.
//   Идея заключается в том, чтобы этот класс был бы базовым механизмом преобразования данные<->текст,
//   все остальные функции пректа должны бы обращаться к этим методам за реализацией.
// 
class SStrTransform { // @v11.9.2 @construction
public:
	static bool ToText(int v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(uint v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(int64 v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(uint64 v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(double v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(float v, long fmt, char * pBuf, size_t bufSize);
	static bool ToText(const SUniTime_Internal & rV, long fmt, char * pBuf, size_t bufLen);
	static bool ToText(const S_GUID & rV, long fmt, char * pBuf, size_t bufLen);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, int * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, uint * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, int64 * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, uint64 * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, double * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, float * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, SUniTime_Internal * pV);
	static bool FromText(const char * pText, size_t textLen, long flags, size_t * pScannedCount, S_GUID * pV);
};
//
//
//
#if 0 // {
int main(int argc, char **argv )
{
	const char * input = "<title>Foo</title><p>Foo!";
	TidyBuffer output;
	TidyBuffer errbuf;
	int rc = -1;
	Bool ok;
	TidyDoc tdoc = tidyCreate();                     // Initialize "document"
	tidyBufInit(&output);
	tidyBufInit(&errbuf);
	printf("Tidying:\t\%s\\n", input);

	ok = tidyOptSetBool(tdoc, TidyXhtmlOut, yes);  // Convert to XHTML
	if(ok)
		rc = tidySetErrorBuffer(tdoc, &errbuf);      // Capture diagnostics
	if(rc >= 0)
		rc = tidyParseString(tdoc, input);           // Parse the input
	if(rc >= 0)
		rc = tidyCleanAndRepair(tdoc);               // Tidy it up!
	if(rc >= 0)
		rc = tidyRunDiagnostics(tdoc);               // Kvetch
	if(rc > 1)                                    // If error, force output.
		rc = (tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1);
	if(rc >= 0)
		rc = tidySaveBuffer(tdoc, &output);          // Pretty Print
	if(rc >= 0) {
		if(rc > 0)
			printf("\\nDiagnostics:\\n\\n\%s", errbuf.bp);
		printf("\\nAnd here is the result:\\n\\n\%s", output.bp);
	}
	else
		printf("A severe error (\%d) occurred.\\n", rc);
	tidyBufFree(&output);
	tidyBufFree(&errbuf);
	tidyRelease(tdoc);
	return rc;
}
#endif // } 0
