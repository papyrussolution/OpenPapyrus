// STokenRecognizer.java
// Copyright (c) A.Sobolev 2023, 2024, 2025
//
package ru.petroglif.styloq;

import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

//
// Descr: перенос реализации модуля STokenRecognizer из SLIB.
// Все символьные константы и их числовые эквиваленты в точности соответствуют аналогам из файла slib.h
//
public class STokenRecognizer {
	//
	// Descr: Идентификаторы распознаваемых натуральных токенов.
	//   @persistent
	//   Символьные обозначения токенов находятся в строке PPTXT_NATURALTOKENID
	//
	public static final int SNTOK_UNKN             =  0;
	public static final int SNTOK_NATURALWORD      =  1;
	public static final int SNTOK_DIGITCODE        =  2;
	public static final int SNTOK_EAN13            =  3;
	public static final int SNTOK_EAN8             =  4;
	public static final int SNTOK_UPCA             =  5;
	public static final int SNTOK_UPCE             =  6;
	public static final int SNTOK_RU_INN           =  7;
	public static final int SNTOK_EGAISWARECODE    =  8; // Код алкогольной продукции ЕГАИС
	public static final int SNTOK_EGAISMARKCODE    =  9; // Код акцизной марки алкогольной продукции ЕГАИС
	public static final int SNTOK_LUHN             = 10; // Цифровая последовательность с контрольной цифрой по алгоритму LUHN в конце
	public static final int SNTOK_DIGLAT           = 11; // Алфавитно-цифровая последовательность (содержащая только цифры и латинские символы)
	public static final int SNTOK_GUID             = 12; // Общепринятое текстовое представление GUID
	public static final int SNTOK_EMAIL            = 13; // Адрес электронной почты
	public static final int SNTOK_PHONE            = 14; // Номер телефона
	public static final int SNTOK_IMEI             = 15; // Код IMEI (с контрольной цифрой по алгоритму LUHN в конце)
	public static final int SNTOK_IP4              = 16; // ip4-address xx.xx.xx.xx
	public static final int SNTOK_IP6              = 17; // @todo ip6-address
	public static final int SNTOK_MACADDR48        = 18; // @todo MAC-address(48) xx-xx-xx-xx-xx-xx or xx:xx:xx:xx:xx:xx
	public static final int SNTOK_DATE             = 19; // date
	public static final int SNTOK_TIME             = 20; // time
	public static final int SNTOK_SOFTWAREVER      = 21; // 9.9.9
	public static final int SNTOK_COLORHEX         = 22; // #hhhhhh
	public static final int SNTOK_REALNUMBER       = 23; // 9.9 || .9 || 9. || 9,9
	public static final int SNTOK_INTNUMBER        = 24; // 9 || -9
	public static final int SNTOK_PECENTAGE        = 25; //
	public static final int SNTOK_NUMERIC_DOT      = 26; // Десятичное число с десятичным разделителем точка и опциональными разделителями триад [,'`\x20]
	// Допускается лидирующий + или -. Экспоненциальная запись не допускается.
	public static final int SNTOK_NUMERIC_COM      = 27; // Десятичное число с десятичным разделителем запятая и опциональными разделителями триад ['`\x20]
	// Допускается лидирующий + или -. Экспоненциальная запись не допускается.
	// Если число не имеет дробной части, то применяется SNTOK_NUMERIC_DOT
	public static final int SNTOK_CHZN_GS1_GTIN    = 28; // честный знак Идентификационный номер GS1 для идентификации товаров regexp: "[0-9]{14}"
	public static final int SNTOK_CHZN_SIGN_SGTIN  = 29; // честный знак Индивидуальный серийный номер вторичной упаковки regexp: "[0-9]{14}[&#x21;-&#x22;&#x25;-&#x2F;&#x30;-&#x39;&#x41;-&#x5A;&#x5F;&#x61;-&#x7A;]{13}"
	public static final int SNTOK_CHZN_SSCC        = 30; // честный знак Индивидуальный серийный номер третичной/транспортной упаковки regexp: "[0-9]{18}"
	public static final int SNTOK_CHZN_CIGITEM     = 31; // честный знак Маркировка пачки сигарет
	public static final int SNTOK_CHZN_CIGBLOCK    = 32; // честный знак Маркировка блока сигарет
	public static final int SNTOK_RU_OKPO          = 33; // @v10.8.1 Российский код ОКПО
	public static final int SNTOK_RU_SNILS         = 34; // @v10.8.1 Российский код СНИЛС
	public static final int SNTOK_RU_BIC           = 35; // @v10.8.1 Российский банковский идентификационный код (БИК)
	public static final int SNTOK_RU_KPP           = 36; // @v10.8.2 Российский код причины постановки на налоговый учет (КПП)
	public static final int SNTOK_LINGUACODE       = 37; // @v11.3.12 Код естественного языка
	public static final int SNTOK_CHZN_SURROGATE_GTIN      = 38; // честный знак Специальная суррогатная конструкция, не являющаяся маркой (02 GTIN14)
	public static final int SNTOK_CHZN_SURROGATE_GTINCOUNT = 39; // честный знак Специальная суррогатная конструкция, не являющаяся маркой (02 GTIN14 37 COUNT)
	public static final int SNTOK_CL_RUT                   = 40; // @v11.6.1 Чилийский налоговый код RUT
	public static final int SNTOK_CHZN_ALTCIGITEM          = 41; // @v11.9.0 честный знак Маркировка альтернативной табачной продукции (розничная единица). Код аналогичен SNTOK_CHZN_CIGITEM, но на месте МРЦ стоит AAAA
	public static final int SNTOK_AR_DNI                   = 42; // @v11.9.4 @construction Аргентинский идентификатор гражданина
	public static final int SNTOK_GENERICTEXT_ASCII        = 43; // @v12.2.12
	public static final int SNTOK_GENERICTEXT_UTF8         = 44; // @v12.2.12
	public static final int SNTOK_GENERICTEXT_CP1251       = 45; // @v12.2.12
	public static final int SNTOK_GENERICTEXT_CP866        = 46; // @v12.2.12
	public static final int SNTOK_JSON                     = 47; // @v12.2.12
	public static final int SNTOK_PLIDENT                  = 48; // @v12.3.0 Идентификатор во многих (но не всех) языках программирования. Все символы - ascii, первый символ - latin or '_', остальные - decimal or latin or '_'
	public static final int SNTOK_HASH_MD5                 = 49; // @v12.3.3 @todo
	public static final int SNTOK_BASE32                   = 50; // @v12.3.3
	public static final int SNTOK_BASE32_CROCKFORD         = 51; // @v12.3.3
	public static final int SNTOK_BASE58                   = 52; // @v12.3.3
	public static final int SNTOK_BASE64                   = 53; // @v12.3.3
	public static final int SNTOK_BASE64_URL               = 54; // @v12.3.3
	public static final int SNTOK_BASE64_WP                = 55; // @v12.3.3 (with padding)
	public static final int SNTOK_BASE64_URL_WP            = 56; // @v12.3.3 (with padding)
	public static final int SNTOK_AU_ABN                   = 57; // @v12.3.4 @todo https://abr.business.gov.au/Help/AbnFormat
	public static final int SNTOK_BG_EGN                   = 58; // @v12.3.4 @todo Болгария Единый Гражданский Код
	public static final int SNTOK_CA_SIN                   = 59; // @v12.3.4 @todo Canada Social Insurance Number
	public static final int SNTOK_CHZN_PALLET_GTIN         = 60; // @v12.4.5 честный знак Специальная конструкция, не являющаяся маркой (00 GTIN14 9999)

	public static final int SNTOKSEQ_DEC        = 0x00000001; // 0-9
	public static final int SNTOKSEQ_LATLWR     = 0x00000002; // a-z
	public static final int SNTOKSEQ_LATUPR     = 0x00000004; // A-Z
	public static final int SNTOKSEQ_LAT        = 0x00000008; // A-Z||a-z
	public static final int SNTOKSEQ_HEX        = 0x00000010; // 0-9||A-F||a-f
	public static final int SNTOKSEQ_ASCII      = 0x00000020; // 1-127
	public static final int SNTOKSEQ_UTF8       = 0x00000040; // Вся последовательность состоит из валидных UTF-8 символов
	public static final int SNTOKSEQ_1251       = 0x00000080; // Вся последовательность состоит из символов, соответствующих кодировке cp1251
	public static final int SNTOKSEQ_866        = 0x00000100; // Вся последовательность состоит из символов, соответствующих кодировке cp866
	public static final int SNTOKSEQ_DECLAT     = 0x00000200; // 0-9||A-F||a-f
	public static final int SNTOKSEQ_HEXHYPHEN  = 0x00000400; // ТОЛЬКО HEX && -
	public static final int SNTOKSEQ_DECHYPHEN  = 0x00000800; // ТОЛЬКО DEC && -
	public static final int SNTOKSEQ_HEXCOLON   = 0x00001000; // ТОЛЬКО HEX && :
	public static final int SNTOKSEQ_DECCOLON   = 0x00002000; // ТОЛЬКО DEC && :
	public static final int SNTOKSEQ_HEXDOT     = 0x00004000; // ТОЛЬКО HEX && .
	public static final int SNTOKSEQ_DECDOT     = 0x00008000; // ТОЛЬКО DEC && .
	public static final int SNTOKSEQ_DECSLASH   = 0x00010000; // ТОЛЬКО DEC && /
	public static final int SNTOKSEQ_LEADSHARP  = 0x00020000; // Лидирующий символ # (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
	public static final int SNTOKSEQ_LEADMINUS  = 0x00040000; // Лидирующий символ - (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
	public static final int SNTOKSEQ_LEADDOLLAR = 0x00080000; // Лидирующий символ $ (если этот символ встречается только в начале строки, то он не отключает остальные флаги.
	public static final int SNTOKSEQ_BACKPCT    = 0x00100000; // Последовательность завершается символом % (без отключения остальных флагов)
	public static final int SNTOKSEQ_NUMERIC    = 0x00200000; // @v10.2.5 Многовариантное представление числа [+\-0-9.,' ]
	// -9,999.99  +99'999,9  9 999.999  999
	public static final int SNTOKSEQ_LATHYPHENORUSCORE  = 0x00400000; // @v11.3.12 ТОЛЬКО LAT && (- || _)
	
	public static class Token {
		int    ID;
		float  Prob;
	}
	public static class Stat {
		public Stat()
		{
			Len = 0;
			Seq = 0;
		}
		Stat Z()
		{
			Len = 0;
			Seq = 0;
			return this;
		}
		int    Len;
		int    Seq;
	}
	public static class TokenArray {
		public ArrayList<Token> L;
		public Stat S;

		TokenArray()
		{
			L = null;
			S = null;
		}
		float Has(int tok)
		{
			if(L != null)
				for(int i = 0; i < L.size(); i++) {
					if(L.get(i).ID == tok)
						return L.get(i).Prob;
				}
			return 0.0f;
		}
		void Add(int tok, float prob)
		{
			if(tok > 0) {
				if(L == null) {
					if(tok == 0 || prob <= 0.0f) {
						;
					}
					else
						L = new ArrayList<Token>();
				}
				if(L != null) {
					Token found_entry = null;
					int   found_idx = -1;
					for(int i = 0; i < L.size(); i++) {
						if(L.get(i).ID == tok) {
							found_idx = i;
							found_entry = L.get(i);
						}
					}
					if(found_entry != null) {
						assert(found_idx >= 0 && found_idx < L.size());
						if(found_entry.Prob != prob) {
							if(prob <= 0.0f)
								L.remove(found_idx);
							else
								found_entry.Prob = prob;
						}
					}
					else if(prob > 0.0f){
						Token new_entry = new Token();
						new_entry.ID = tok;
						new_entry.Prob = prob;
						L.add(new_entry);
					}
				}
			}
		}
	}
	STokenRecognizer()
	{
	}
	private static class ImplementBlock {
		public static final int fUtf8     = 0x0001; // Весь токен состоит из валидных UTF8-символов
		public static final int fPhoneSet = 0x0002;  // Токен по набору символов может быть истолкован как номер телефона (но требуются дополнительные проверки)

		ImplementBlock(final byte [] token, int tokenLen /* (<0) - strlen(pToken) */)
		{
			S = new Stat();
			ChrList = new SLib.LAssocVector();
			S.Len = (tokenLen >= 0) ? tokenLen : token.length;
		}
		long   F;
		int    DecCount; // Количество десятичных цифр
		Stat   S;
		SLib.LAssocVector ChrList;
	};
	private static final char TrailingBytesForUTF8[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x20
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x40
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x60
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x80
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xA0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xC0
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 0xE0
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5  // 0x100
	};
	private static boolean IsLegalUtf8(final byte [] source, int start, int length)
	{
		//final byte * srcptr = pSource+length;
		int  idx = start+length;
		switch(length) {
			default:
				return false;
			// Everything else falls through when "true"...
			case 4:
				{
					idx--;
					if(source[idx] < 0x80 || source[idx] > 0xBF)
						return false;
				}
			case 3:
				{
					idx--;
					if(source[idx] < 0x80 || source[idx] > 0xBF)
						return false;
				}
			case 2:
				{
					idx--;
					if(source[idx] > 0xBF)
						return false;
					switch(source[start]){
						// no fall-through in this inner switch
						case (byte)0xE0:
							if(source[idx] < 0xA0)
								return false;
							break;
						case (byte)0xED:
							if(source[idx] > 0x9F)
								return false;
							break;
						case (byte)0xF0:
							if(source[idx] < 0x90)
								return false;
							break;
						case (byte)0xF4:
							if(source[idx] > 0x8F)
								return false;
							break;
						default:
							if(source[idx] < 0x80)
								return false;
					}
				}
			case 1:
				if(source[start] >= 0x80 && source[start] < 0xC2)
					return false;
		}
		return (source[start] > 0xF4) ? false : true;
	}
	private int IsUtf8(final byte [] p, int start, int restLen)
	{
		final int idx = (p[start] >= 0) ? p[start] : (256+p[start]);
		final int extra = TrailingBytesForUTF8[idx];
		return (extra == 0) ? 1 : ((restLen > extra && IsLegalUtf8(p, start, 2)) ? (extra+1) : 0);
	}
	private boolean _ProbeDate(final byte [] text, int len)
	{
		boolean ok = false;
		if(text != null) {
			final String _stext = new String(text, 0, len);
			SLib.LDATE probe_dt = SLib.strtodate(_stext, SLib.DATF_DMY);
			if(SLib.CheckDate(probe_dt))
				ok = true;
			else {
				probe_dt = SLib.strtodate(_stext, SLib.DATF_MDY);
				if(SLib.CheckDate(probe_dt))
					ok = true;
				else {
					probe_dt = SLib.strtodate(_stext, SLib.DATF_YMD);
					if(SLib.CheckDate(probe_dt))
						ok = true;
				}
			}
		}
		return ok;
	}
	public TokenArray Run(String input)
	{
		return Run(input.getBytes(), -1);
	}
	public TokenArray Run(final byte [] input, int tokenLen)
	{
		TokenArray result = new TokenArray();
		ImplementBlock ib = new ImplementBlock(input, tokenLen);
		int    h = 0;
		final int toklen = ib.S.Len;
		if(toklen > 0) {
			int   i;
			byte  num_potential_frac_delim = 0;
			byte  num_potential_tri_delim = 0;
			h = 0xffffffff & ~(SNTOKSEQ_LEADSHARP|SNTOKSEQ_LEADMINUS|SNTOKSEQ_LEADDOLLAR|SNTOKSEQ_BACKPCT);
			byte the_first_chr = input[0];
			if(toklen >= 5)
				ib.F |= ImplementBlock.fPhoneSet;
			for(i = 0; i < toklen; i++) {
	            byte  c = input[i];
				int   ul = IsUtf8(input, i, toklen-i);
				if(ul > 1) {
					ib.F |= ImplementBlock.fUtf8;
					i += (ul-1);
				}
				else {
					if(ul == 0)
						h &= ~SNTOKSEQ_UTF8;
					int  pos = ib.ChrList.Search(c);
					if(pos >= 0)
						ib.ChrList.at(pos).Value++;
					else {
						ib.ChrList.insert(new SLib.LAssoc(c, 1));
					}
				}
			}
			ib.ChrList.Sort();
			if((ib.F & ImplementBlock.fUtf8) != 0) {
				h &= ~(SNTOKSEQ_DEC|SNTOKSEQ_HEX|SNTOKSEQ_LATLWR|SNTOKSEQ_LATUPR|SNTOKSEQ_LAT|SNTOKSEQ_DECLAT|
						SNTOKSEQ_ASCII|SNTOKSEQ_866|SNTOKSEQ_1251|SNTOKSEQ_HEXHYPHEN|SNTOKSEQ_DECHYPHEN|SNTOKSEQ_HEXCOLON|
						SNTOKSEQ_DECCOLON|SNTOKSEQ_HEXDOT|SNTOKSEQ_DECDOT|SNTOKSEQ_DECSLASH|SNTOKSEQ_NUMERIC);
			}
			else {
				boolean is_lead_plus = false;
				boolean has_dec = false;
				i = 0;
				if(the_first_chr == '#') {
					h |= SNTOKSEQ_LEADSHARP;
					i++;
				}
				else if(the_first_chr == '-')
					h |= SNTOKSEQ_LEADMINUS;
				else if(the_first_chr == '$')
					h |= SNTOKSEQ_LEADDOLLAR;
				else if(the_first_chr == '+')
					is_lead_plus = true;
				final int clc = ib.ChrList.getCount();
				for(; i < clc; i++) {
					final byte c = (byte)ib.ChrList.at(i).Key;
					final int  ccnt = ib.ChrList.at(i).Value;
					if((h & SNTOKSEQ_ASCII) != 0 && !(c >= 1 && c <= 127))
						h &= ~SNTOKSEQ_ASCII;
					else {
						final boolean is_hex_c = SLib.ishex(c);
						final boolean is_dec_c = SLib.isdec(c);
						final boolean is_asciialpha = SLib.isasciialpha(c);
						if(is_dec_c) {
							has_dec = true;
							ib.DecCount += ccnt;
						}
						if((h & SNTOKSEQ_LAT) != 0 && !is_asciialpha) {
							h &= ~SNTOKSEQ_LAT;
						}
						else {
							if((h & SNTOKSEQ_LATLWR) != 0 && !(c >= 'a' && c <= 'z'))
								h &= ~SNTOKSEQ_LATLWR;
							if((h & SNTOKSEQ_LATUPR) != 0 && !(c >= 'A' && c <= 'Z'))
								h &= ~SNTOKSEQ_LATUPR;
						}
						// @v11.3.12 {
						if((h & SNTOKSEQ_LATHYPHENORUSCORE) != 0 && !is_asciialpha && (c != '-' && c != '_'))
							h &= ~SNTOKSEQ_LATHYPHENORUSCORE;
						// } @v11.3.12
						if((h & SNTOKSEQ_HEX) != 0 && !is_hex_c)
							h &= ~SNTOKSEQ_HEX;
						else if((h & SNTOKSEQ_DEC) != 0 && !is_dec_c)
							h &= ~SNTOKSEQ_DEC;
						if((h & SNTOKSEQ_DECHYPHEN) != 0 && !(c == '-' || is_dec_c))
							h &= ~SNTOKSEQ_DECHYPHEN;
						if((h & SNTOKSEQ_HEXHYPHEN) != 0 && !(c == '-' || is_hex_c))
							h &= ~SNTOKSEQ_HEXHYPHEN;
						if((h & SNTOKSEQ_DECLAT) != 0 && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_dec_c))
							h &= ~SNTOKSEQ_DECLAT;
						if((h & SNTOKSEQ_DECCOLON) != 0 && !(c == ':' || is_dec_c))
							h &= ~SNTOKSEQ_DECCOLON;
						if((h & SNTOKSEQ_HEXCOLON) != 0 && !(c == ':' || is_hex_c))
							h &= ~SNTOKSEQ_HEXCOLON;
						if((h & SNTOKSEQ_DECDOT) != 0 && !(c == '.' || is_dec_c))
							h &= ~SNTOKSEQ_DECDOT;
						if((h & SNTOKSEQ_HEXDOT) != 0 && !(c == '.' || is_hex_c))
							h &= ~SNTOKSEQ_HEXDOT;
						if((h & SNTOKSEQ_DECSLASH) != 0 && !(c == '/' || is_dec_c))
							h &= ~SNTOKSEQ_DECSLASH;
						if((h & SNTOKSEQ_NUMERIC) != 0) {
							if(!is_dec_c && (c != ',' && c != '.' && c != '\'' && c != '`' && c != ' ' && c != '+' && c != '-'))
								h &= ~SNTOKSEQ_NUMERIC;
							else if(the_first_chr == ' ')
								h &= ~SNTOKSEQ_NUMERIC;
							else if(c == '+' || c == '-') {
								if(ib.ChrList.at(i).Value > 1)
									h &= ~SNTOKSEQ_NUMERIC;
								else if(c == '+' && !is_lead_plus)
									h &= ~SNTOKSEQ_NUMERIC;
								else if(c == '-' && (h & SNTOKSEQ_LEADMINUS) == 0)
									h &= ~SNTOKSEQ_NUMERIC;
							}
						}
						if((ib.F & ImplementBlock.fPhoneSet) != 0) {
							if(!is_dec_c && (c != '+' && c != '-' && c != '(' && c != ')' && c != ' '))
								ib.F &= ~ImplementBlock.fPhoneSet;
							else if((c == '+' || c == '(' || c == ')') && ccnt > 1)
								ib.F &= ~ImplementBlock.fPhoneSet;
						}
					}
					if((h & SNTOKSEQ_866) != 0 && !SLib.isletter866(c))
						h &= ~SNTOKSEQ_866;
					if((h & SNTOKSEQ_1251) != 0 && !SLib.isletter1251(c))
						h &= ~SNTOKSEQ_1251;
				}
				if((h & SNTOKSEQ_ASCII) == 0) {
					h &= ~(SNTOKSEQ_LAT|SNTOKSEQ_LATUPR|SNTOKSEQ_LATLWR|SNTOKSEQ_HEX|SNTOKSEQ_DEC|SNTOKSEQ_DECLAT|
							SNTOKSEQ_HEXHYPHEN|SNTOKSEQ_DECHYPHEN|SNTOKSEQ_HEXCOLON|SNTOKSEQ_DECCOLON|SNTOKSEQ_HEXDOT|
							SNTOKSEQ_DECDOT|SNTOKSEQ_DECSLASH|SNTOKSEQ_NUMERIC|SNTOKSEQ_LATHYPHENORUSCORE);
				}
				else {
					if((h & SNTOKSEQ_HEX) == 0)
						h &= ~SNTOKSEQ_DEC;
					if((h & SNTOKSEQ_LAT) == 0)
						h &= ~(SNTOKSEQ_LATLWR|SNTOKSEQ_LATUPR);
				}
				{
					final int tf = SNTOKSEQ_HEXHYPHEN;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_HEX) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '-');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == '-'); // '#' < '-'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_DECHYPHEN;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_DEC) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '-');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == '-'); // '#' < '-'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_HEXCOLON;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_HEX) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == ':');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == ':'); // '#' < ':'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_DECCOLON;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_DEC) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == ':');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == ':'); // '#' < ':'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_HEXDOT;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_HEX) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '.');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == '.'); // '#' < '.'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_DECDOT;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_DEC) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '.');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == '.'); // '#' < '.'
							h &= ~tf;
						}
					}
				}
				{
					final int tf = SNTOKSEQ_DECSLASH;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_DEC) != 0)
							h &= ~tf;
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '/');
							h &= ~tf;
						}
						else if(clc == 2 && (h & SNTOKSEQ_LEADSHARP) != 0) {
							assert(ib.ChrList.at(1).Key == '/'); // '#' < '/'
							h &= ~tf;
						}
					}
				}
				// @v11.3.12 @construction {
				{
					final int tf = SNTOKSEQ_LATHYPHENORUSCORE;
					if((h & tf) != 0) {
						if((h & SNTOKSEQ_LAT) != 0) {
							h &= ~tf;
						}
						else if(clc == 1) {
							assert(ib.ChrList.at(0).Key == '-' || ib.ChrList.at(0).Key == '_');
							h &= ~tf;
						}
						else {
							if(toklen >= 2 && toklen <= 7) {
								//RecognizeLinguaSymb(reinterpret_cast<const char *>(pToken), 1);
							}
						}
					}
				}
				// } @v11.3.12
				{
					final int tf = SNTOKSEQ_NUMERIC;
					if((h & tf) != 0) {
						if(!has_dec)
							h &= ~tf;
						else {
							int   comma_chr_pos = ib.ChrList.Search(',');
							final  int comma_count = (comma_chr_pos >= 0) ? ib.ChrList.at(comma_chr_pos).Value : 0;
							int   last_dec_ser = 0;
							int   j = toklen;
							if(j != 0) do {
								final byte lc = input[--j];
								if(SLib.isdec(lc))
									last_dec_ser++;
								else {
									if(lc == '.') {
										if(num_potential_frac_delim == 0)
											num_potential_frac_delim = lc;
										else {
											h &= ~tf;
											break; // Две точки в разбираемом формате невозможны
										}
									}
									else if(lc == ',' && comma_count == 1) {
										// 999,99
										// 999,999 - самый плохой случай.
										if(last_dec_ser == 3) {
											num_potential_tri_delim = lc;
										}
										else if(num_potential_frac_delim != 0) {
											h &= ~tf;
											break; // Роль десятичного разделителя занята, а на роль разделителя разрядов запятая здесь не годится - уходим.
										}
										if(num_potential_frac_delim == 0)
											num_potential_frac_delim = lc;
									}
									else {
										boolean is_potential_divider = false;
										if(comma_count > 1)
											is_potential_divider = (lc == ',' || lc == '\'' || lc == '`' || lc == ' ');
										else
											is_potential_divider = (lc == '\'' || lc == '`' || lc == ' ');
										if(is_potential_divider) {
											if(last_dec_ser != 3) {
												h &= ~tf;
												break; // Потенциальный резделитель должен иметь точно 3 цифры справа. Это не так - уходим.
											}
											else {
												if(num_potential_tri_delim == 0) {
													num_potential_tri_delim = lc;
												}
												else if(num_potential_tri_delim != lc) {
													h &= ~tf;
													break; // Более одного символа могут претендовать на роль разделителя разрядов - уходим
												}
											}
										}
										else if(lc == '+') {
											if(j != 0) {
												h &= ~tf;
												break; // + может быть только в первой позиции
											}
										}
										else if(lc == '-') {
											if(j != 0) {
												h &= ~tf;
												break; // - может быть только в первой позиции
											}
										}
									}
									last_dec_ser = 0;
								}
							} while(j != 0);
						}
					}
				}
			}
			ib.S.Seq = h;
			if((h & SNTOKSEQ_LEADSHARP) != 0) {
				if((h & SNTOKSEQ_HEX) != 0 && toklen == 7) {
					result.Add(SNTOK_COLORHEX, 0.9f);
				}
			}
			/*else if((h & SNTOKSEQ_LEADMINUS) != 0) {
			}*/
			else if((h & SNTOKSEQ_LEADDOLLAR) != 0) {
			}
			else if((h & SNTOKSEQ_BACKPCT) != 0) {
			}
			else {
				if((h & SNTOKSEQ_DEC) != 0) {
					byte last = input[toklen-1];
					int   cd = 0;
					result.Add(SNTOK_DIGITCODE, 1.0f);
					switch(toklen) {
						case 6:
							if(_ProbeDate(input, toklen)) {
								result.Add(SNTOK_DATE, 0.5f);
							}
							break;
						case 8:
							// RU_OKPO
							{
								//
								// Проверка правильности указания ОКПО:
								//
								// Алгоритм проверки ОКПО:
								// 1. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (1,2,3,4,5,6,7).
								// 2. Вычисляется контрольное число(1) как остаток от деления контрольной суммы на 11.
								// 3. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (3,4,5,6,7,8,9).
								// 4. Вычисляется контрольное число(2) как остаток от деления контрольной суммы на 11.
								//   Если остаток от деления равен 10-ти, то контрольному числу(2) присваивается ноль.
								// 5. Если контрольное число(1) больше девяти, то восьмой знак ОКПО сравнивается с контрольным числом(2),
								//   иначе восьмой знак ОКПО сравнивается с контрольным числом(1). В случае их равенства ОКПО считается правильным.
								//
								boolean is_ru_okpo = false;
								final int ru_okpo_w1[] = {1,2,3,4,5,6,7};
								final int ru_okpo_w2[] = {3,4,5,6,7,8,9};
								int  sum1 = 0;
								int  sum2 = 0;
								for(i = 0; i < 7; i++) {
									sum1 += (ru_okpo_w1[i] * (input[i]-'0'));
									sum2 += (ru_okpo_w2[i] * (input[i]-'0'));
								}
								int    cd1 = (sum1 % 11);
								int    cd2 = (sum2 % 11);
								if(cd2 == 10)
									cd2 = 0;
								is_ru_okpo = (cd1 > 9) ? ((last-'0') == cd2) : ((last-'0') == cd1);
								if(is_ru_okpo)
									result.Add(SNTOK_RU_OKPO, 0.95f);
							}
							cd = SLib.SCalcBarcodeCheckDigitL(input, toklen-1);
							if(cd == (last-'0')) {
								if(input[0] == '0')
									result.Add(SNTOK_UPCE, 0.9f);
								else
									result.Add(SNTOK_EAN8, 0.9f);
							}
							if(_ProbeDate(input, toklen)) {
								result.Add(SNTOK_DATE, 0.8f);
							}
							break;
						case 9:
							if(input[0] == '0' && input[1] == '4') {
								result.Add(SNTOK_RU_BIC, 0.6f);
							}
							result.Add(SNTOK_RU_KPP, 0.1f); // @v10.8.2
							break;
						case 10:
							if(SLib.SCalcCheckDigit(SLib.SCHKDIGALG_RUINN|SLib.SCHKDIGALG_TEST, input, toklen) != 0) {
								result.Add(SNTOK_RU_INN, 1.0f);
							}
							break;
						case 11:
							{
								// СНИЛС (страховой номер индивидуального лицевого счета) состоит из 11 цифр:
								//	1-9-я цифры — любые цифры;
								//	10-11-я цифры — контрольное число.
								// Маски ввода
								//	XXXXXXXXXXX — маска ввода без разделителей.
								//	XXX-XXX-XXX-XX — маска ввода с разделителями.
								//	XXX-XXX-XXX XX — маска ввода с разделителями и с отделением контрольного числа.
								// Алгоритм проверки контрольного числа
								//	Вычислить сумму произведений цифр СНИЛС (с 1-й по 9-ю) на следующие коэффициенты — 9, 8, 7, 6, 5, 4, 3, 2, 1 (т.е. номера цифр в обратном порядке).
								//	Вычислить контрольное число от полученной суммы следующим образом:
								//		если она меньше 100, то контрольное число равно этой сумме;
								//		если равна 100, то контрольное число равно 0;
								//		если больше 100, то вычислить остаток от деления на 101 и далее:
								//			если остаток от деления равен 100, то контольное число равно 0;
								//			в противном случае контрольное число равно вычисленному остатку от деления.
								//	Сравнить полученное контрольное число с двумя младшими разрядами СНИЛС. Если они равны, то СНИЛС верный.
								final int ru_snils_w[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
								int    sum = 0;
								int    cn = 0; // check number
								for(i = 0; i < 9; i++) {
									sum += (ru_snils_w[i] * (input[i]-'0'));
								}
								if(sum < 100)
									cn = sum;
								else if(sum == 100)
									cn = 0;
								else {
									cn = sum % 101;
									if(cn == 100)
										cn = 0;
								}
								if(cn == ((input[9]-'0') * 10 + (input[10]-'0'))) {
									result.Add(SNTOK_RU_SNILS, 0.95f);
								}
							}
							break;
						case 12:
							cd = SLib.SCalcBarcodeCheckDigitL(input, toklen-1);
							if(cd == (last-'0')) {
								if(input[0] == '0')
									result.Add(SNTOK_UPCE, 1.0f);
								else
									result.Add(SNTOK_EAN8, 1.0f);
							}
							else if(SLib.SCalcCheckDigit(SLib.SCHKDIGALG_RUINN|SLib.SCHKDIGALG_TEST, input, toklen) != 0) {
								result.Add(SNTOK_RU_INN, 1.0f);
							}
							break;
						case 13:
							cd = SLib.SCalcBarcodeCheckDigitL(input, toklen-1);
							if(cd == (last-'0')) {
								result.Add(SNTOK_EAN13, 1.0f);
							}
							break;
						case 15:
							if(SLib.SCalcCheckDigit(SLib.SCHKDIGALG_LUHN|SLib.SCHKDIGALG_TEST, input, toklen) != 0) {
								result.Add(SNTOK_IMEI, 0.9f);
								result.Add(SNTOK_DIGITCODE, 0.1f);
							}
							break;
						case 19:
							if(SLib.SCalcCheckDigit(SLib.SCHKDIGALG_LUHN|SLib.SCHKDIGALG_TEST, input, toklen) != 0) {
								result.Add(SNTOK_LUHN, 0.9f);
								result.Add(SNTOK_EGAISWARECODE, 0.1f);
							}
							else {
								result.Add(SNTOK_EGAISWARECODE, 1.0f);
							}
							break;
					}
				}
				if((h & SNTOKSEQ_DECLAT) != 0) {
					if(toklen == 68)
						result.Add(SNTOK_EGAISMARKCODE, 1.0f);
					else {
						result.Add(SNTOK_DIGLAT, 1.0f);
						if(toklen == 9) {
							boolean is_ru_kpp = true;
							for(i = 0; is_ru_kpp && i < toklen; i++) {
								if(!SLib.isdec(input[i])) {
									if(!((i == 4 || i == 5) && (input[i] >= 'A' && input[i] <= 'Z'))) // 5, 6 знаки в КПП могут быть прописной латинской буквой
										is_ru_kpp = false;
								}
							}
							if(is_ru_kpp)
								result.Add(SNTOK_RU_KPP, 0.1f);
						}
					}
				}
				if((h & SNTOKSEQ_HEXHYPHEN) != 0) {
					if(toklen == 36) {
						int    pos = ib.ChrList.Search('-');
						long   val = 0;
						if(pos >= 0 && ib.ChrList.at(pos).Value == 4) {
							result.Add(SNTOK_GUID, 1.0f);
						}
					}
				}
				if((h & (SNTOKSEQ_DECHYPHEN|SNTOKSEQ_DECSLASH|SNTOKSEQ_DECDOT)) != 0) {
					// 1-1-1 17-12-2016
					if(toklen >= 5 && toklen <= 10) {
						String temp_str = new String(input, 0, toklen);
						String div = null;
						if((h & SNTOKSEQ_DECHYPHEN) != 0)
							div = "-";
						else if((h & SNTOKSEQ_DECSLASH) != 0)
							div = "/";
						else if((h & SNTOKSEQ_DECDOT) != 0)
							div = ".";
						StringTokenizer tokenizer = new StringTokenizer(temp_str, div);
						int ss_count = tokenizer.countTokens();
						if(ss_count == 3) {
							if(_ProbeDate(input, toklen)) {
								result.Add(SNTOK_DATE, 0.8f);
							}
						}
					}
				}
				if((h & SNTOKSEQ_DECDOT) != 0) {
					// 1.1.1.1 255.255.255.255
					String temp_str = new String(input, 0, toklen);
					StringTokenizer tokenizer = new StringTokenizer(temp_str, ".");
					int ss_count = tokenizer.countTokens();
					if(ss_count == 2) {
						result.Add(SNTOK_REALNUMBER, 0.9f);
					}
					if(toklen >= 3 && toklen <= 15) {
						if(ss_count == 4) {
							boolean is_ip4 = true;
							for(int tidx = 0; tidx < ss_count; tidx++) {
								final String tok = tokenizer.nextToken();
								if(SLib.GetLen(tok) == 0)
									is_ip4 = false;
								else {
									int v = Integer.getInteger(tok, 0);
									if(v < 0 || v > 255)
										is_ip4 = false;
								}
							}
							if(is_ip4) {
								float prob = 0.95f;
								if(input.equals("127.0.0.1"))
									prob = 1.0f;
								result.Add(SNTOK_IP4, prob);
							}
						}
						else if(ss_count == 2 || ss_count == 3) {
							boolean is_ver = true;
							for(int tidx = 0; tidx < ss_count; tidx++) {
								final String tok = tokenizer.nextToken();
								if(SLib.GetLen(tok) == 0)
									is_ver = false;
								else {
									int v = Integer.getInteger(tok, 0);
									if(v < 0 || v > 100)
										is_ver = false;
								}
							}
							if(is_ver) {
								result.Add(SNTOK_SOFTWAREVER, (ss_count == 3) ? 0.5f : 0.1f);
							}
						}
					}
				}
				if((h & SNTOKSEQ_NUMERIC) != 0) {
					if(num_potential_frac_delim != 0 && num_potential_frac_delim == num_potential_tri_delim) {
						result.Add(SNTOK_NUMERIC_COM, 0.6f);
						result.Add(SNTOK_NUMERIC_DOT, 0.6f);
					}
					else if(num_potential_frac_delim == ',') {
						result.Add(SNTOK_NUMERIC_COM, (num_potential_tri_delim != 0) ? 0.7f : 0.95f);
					}
					else {
						result.Add(SNTOK_NUMERIC_DOT, (num_potential_tri_delim != 0) ? 0.8f : 0.99f);
					}
				}
				// @v11.0.3 {
				if((ib.F & ImplementBlock.fPhoneSet) != 0) {
					if(ib.DecCount >= 5 && ib.DecCount <= 14) {
						final String regex_phone = "^([+]?[\\s0-9]+)?(\\d{3}|[(]?[0-9]+[)])?([-]?[\\s]?[0-9])+";
						Pattern rep = Pattern.compile(regex_phone);
						Matcher rem = rep.matcher(new String(input, 0, toklen));
						if(rem.matches() && rem.start() == 0 && rem.end() == toklen)
							result.Add(SNTOK_PHONE, 0.8f);
					}
				}
				// } @v11.0.3
				if((h & SNTOKSEQ_ASCII) != 0) {
					int   pos = ib.ChrList.Search('@');
					if(pos >= 0) {
						int val = ib.ChrList.at(pos).Value;
						if(val == 1) {
							final String regex_email = "^[-A-Za-z0-9!#$%&\'*+/=?^_`{|}~]+(\\.[-A-Za-z0-9!#$%&\'*+/=?^_`{|}~]+)*\\@([A-Za-z0-9][-A-Za-z0-9]*\\.)+[A-Za-z]+";
							Pattern rep = Pattern.compile(regex_email);
							Matcher rem = rep.matcher(new String(input, 0, toklen));
							if(rem.matches() && rem.start() == 0 && rem.end() == toklen)
								result.Add(SNTOK_EMAIL, 1.0f);
						}
					}
					//
					// Проверка на маркировки сигаретных пачек (SNTOK_CHZN_CIGITEM)
					//
					if(toklen == 29) {
						int _offs = 0;
						if(input[_offs++] == '0') {
							boolean is_chzn_cigitem = true;
							while(_offs < 14) {
								if(!SLib.isdec(input[_offs]))
									is_chzn_cigitem = false;
								_offs++;
							}
							if(is_chzn_cigitem)
								result.Add(SNTOK_CHZN_CIGITEM, 0.8f);
						}
					}
					else {
						boolean maybe_cigblock = false;
						if(toklen == 25 || toklen == 35 || toklen == 41 || toklen == 52 || toklen == 55) {
							maybe_cigblock = true;
						}
						else if(toklen == 43) {
							final int _29_idx = ib.ChrList.Search(29); //
							if(_29_idx >= 0 && ib.ChrList.at(_29_idx).Value == 2)
								maybe_cigblock = true;
						}
						if(maybe_cigblock) {
							int    sig_prefix = 0; // 0 - no, 1 - '0', 2 - '(01)'
							int    _offs = 0;
							if(input[_offs] == '0') {
								sig_prefix = 1;
								_offs++;
							}
							else if(input[_offs] == '(' && input[_offs+1] == '0' && input[_offs+2] == '1' && input[_offs+3] == ')') {
								sig_prefix = 2;
								_offs += 4;
							}
							if(sig_prefix != 0) {
								boolean is_chzn_cigblock = true;
								if(sig_prefix == 1) {
									while(_offs < 16) {
										if(!SLib.isdec(input[_offs]))
											is_chzn_cigblock = false;
										_offs++;
									}
								}
								else if(sig_prefix == 2) {
									while(_offs < 18) {
										if(!SLib.isdec(input[_offs]))
											is_chzn_cigblock = false;
										_offs++;
									}
								}
								if(is_chzn_cigblock) {
									assert(_offs == 16 || _offs == 18);
									String _tail = new String(input, _offs, toklen-_offs);
									if(_tail.contains("8005")) // код сигаретного блока может содержать тег цены с префиксом 80005
										result.Add(SNTOK_CHZN_CIGBLOCK, 0.8f);
									else if(toklen == 25)
										result.Add(SNTOK_CHZN_CIGBLOCK, 0.5f);
								}
							}
						}
					}
				}
			}
		}
		result.S = ib.S;
		return result;
	}
}
