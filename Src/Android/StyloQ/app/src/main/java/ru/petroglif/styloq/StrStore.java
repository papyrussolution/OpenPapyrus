// StrStore.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class StrStore {
	static final int HEADER_SIZE = 64;

	static class StrgHeader {        // @persistent @size=64
		int Signature;   // Сигнатура файла "SC2B"
		int Crc32;          // CRC всего файла
		int ListOffs;       // Смещение от начала файла до List (StrAssocArray)
		int SListOffs;      // Смещение от начала файла до SignatureList (StrAssocArray)
		int SLang;          // Идентификатор языка
		int Ver;            // Версия файла
		int DescrListOffs;  // Смещение от начала файла до DescrPosList (LAssocArray)
		//byte  Reserve[36];    // @reserve // @v8.9.10 [48]-->[44] // @v9.0.2 [44]-->[40] // @v9.0.8 [40]-->[36]
	}

	class LangStrCollItem {
		LangStrCollItem()
		{
			SLang = 0;
			PositionHash = null;
		}
		String Get(int ident)
		{
			String result = null;
			if(PositionHash != null) { // if(PositionHash != null)
				Integer tpos = PositionHash.get(ident);
				if(tpos != null)
					result = List.GetTextByPos(tpos);
			}
			else
				result = List.GetText(ident);
			return result;
		}
		int SLang;
		String FileName;         // Имя бинарного файла, из которого загружаются данные
		SLib.StrAssocArray List;       // Собственно строки, сопоставленные с идентификаторами
		SLib.StrAssocArray DescrList;  // Список описаний к строкам (сопоставленны с идентификаторами строк)
		/*SLib.LAssoc []*/SLib.LAssocVector DescrPosList; // Список сопоставлений идентификаторов строк с позицией в файле, по которой хранится подробное описание
		/*SLib.LAssoc []*/SLib.LAssocVector HashAssocList; // Список ассоциаций хэширующих групп с идентификаторами строк, в них входящих.
		// Хэширующая группа обеспечивает быстрый поиск любой строки, входящей в нее, по содержанию и возврат кода этой строки.
		// Техника хэширующих групп необходима для разбора входящий потоков и команд.
		Map <Integer, Integer> PositionHash;
	}

	SLib.StrAssocArray SignatureList;
	SLib.StrAssocArray GrpSymbList;
	int[] HashGroupList;
	ArrayList <LangStrCollItem> LangStrColl;
	Map<String, Integer> SignatureHash;

	StrStore()
	{
		LangStrColl = new ArrayList<LangStrCollItem> ();
	}
	public String ExpandString(int lang, String buf)
	{
		if(SLib.GetLen(buf) > 3) {
			int at_idx = buf.indexOf('@', 0);
			if(at_idx >= 0) {
				int ips = buf.indexOf("@{", 0);
				while(ips >= 0) {
					int ipe = buf.indexOf("}", ips+2);
					if(ipe > ips+2) {
						String inner_signature = buf.substring(ips+2, ipe);
						String temp_buf = GetString(lang, inner_signature);
						if(SLib.GetLen(temp_buf) > 0) {
							buf = ((ips > 0) ? buf.substring(0, ips) : "") + temp_buf + buf.substring(ipe+1);
						}
						else
							ips += 2;
						ips = buf.indexOf("@{", ips);
					}
					else
						break;
				}
			}
		}
		return buf;
	}
	String Helper_GetString(int lang, int ident)
	{
		LangStrCollItem lscitem = GetList(lang);
		String result = (lscitem != null) ? lscitem.Get(ident) : null;
		if(result == null && lang != 0) {
			lscitem = GetList(0);
			result = (lscitem != null) ? lscitem.Get(ident) : null;
		}
		return ExpandString(lang, result);
		/*
		const  int lang = SLS.GetUiLanguageId();
		const  LangStrCollItem * p_item = GetList(lang);
		if(p_item && p_item->Get(ident, rBuf))
			ok = 1;
		else if(lang != 0) {
			p_item = GetList(0);
			if(p_item && p_item->Get(ident, rBuf))
				ok = 2;
		}
		if(ok)
			ExpandString(rBuf, 0);
		 */
	}
	public String GetString(int lang, int group, int code)
	{
		//#define MakeLong(low,high)    (static_cast<long>((static_cast<uint16>(low)) | ((static_cast<ulong>(static_cast<uint16>(high))) << 16)))
		int ident = ((short)code) | (group << 16);
		return Helper_GetString(lang, ident);
	}
	public String GetString(int lang, String signature)
	{
		if(SignatureHash != null) { // if(P_SignatureHash)
			Integer hval = SignatureHash.get(signature);
			if(hval != null)
				return Helper_GetString(lang, hval);
		}
		else {
			int idx = SignatureList.SearchByText(signature);
			if(idx > 0) {
				SLib.StrAssocArray.Item item = SignatureList.GetByPos(idx);
				if(item != null)
					return Helper_GetString(lang, item.Id);
			}
		}
		return null;
	}
	LangStrCollItem GetList(int slang)
	{
		if(LangStrColl != null) {
			for(int i = 0; i < LangStrColl.size(); i++) {
				LangStrCollItem item = LangStrColl.get(i);
				if(item != null && item.SLang == slang)
					return item;
			}
		}
		return null;
	}
	LangStrCollItem GetOrConstructList(int slang)
	{
		LangStrCollItem result = null;
		if(LangStrColl == null)
			LangStrColl = new ArrayList<LangStrCollItem> ();
		for(int i = 0; result == null && i < LangStrColl.size(); i++) {
			LangStrCollItem item = LangStrColl.get(i);
			if(item != null && item.SLang == slang) {
				result = item;
			}
		}
		if(result == null) {
			result = new LangStrCollItem();
			result.SLang = slang;
			LangStrColl.add(result);
		}
		return result;
	}
	public void IndexLangStrColl()
	{
		if(LangStrColl != null && LangStrColl.size() > 0) {
			for(int i = 0; i < LangStrColl.size(); i++) {
				LangStrCollItem lscitem = LangStrColl.get(i);
				if(lscitem != null) {
					int lc = lscitem.List.GetCount();
					if(lc > 16) {
						lscitem.PositionHash = new HashMap<Integer, Integer>();
						for(int j = 0; j < lc; j++) {
							SLib.StrAssocArray.Item li = lscitem.List.GetByPos(j);
							int tpos = lscitem.List.GetTextPos(j);
							lscitem.PositionHash.put(li.Id, tpos);
						}
					}
					else
						lscitem.PositionHash = null;
				}
			}
		}
	}
	public boolean Load(InputStream stream) throws StyloQException
	{
		boolean ok = false;
		if(Read(stream)) {
			int slc = SignatureList.GetCount();
			if(slc > 16) {
				SignatureHash = new HashMap<String, Integer>();
				for(int i = 0; i < slc; i++) {
					SLib.StrAssocArray.Item item = SignatureList.GetByPos(i);
					SignatureHash.put(item.Txt, item.Id);
				}
			}
			else { // Если количество сигнатур не больше 16 то нет смысла городить огород с хэшем
				SignatureHash = null;
			}
			//IndexLangStrColl();
			ok = true;
		}
		return ok;
	}
	private boolean Read(InputStream stream) throws StyloQException
	{
		boolean ok = false;
		try {
			if(stream != null) {
				StrgHeader hdr = new StrgHeader();
				int avl = stream.available();
				if(avl >= HEADER_SIZE) {
					stream.mark(0x7fffffff);
					byte [] temp_buf = new byte [HEADER_SIZE];
					int rb = stream.read(temp_buf);
					if(rb == HEADER_SIZE && temp_buf[0] == 'S' && temp_buf[1] == 'C' && temp_buf[2] == '2' && temp_buf[3] == 'B') {
						int crc32 = SLib.BytesToInt(temp_buf, 4);
						int list_offs = SLib.BytesToInt(temp_buf, 8); // Смещение от начала файла до List (StrAssocArray)
						int slist_offs = SLib.BytesToInt(temp_buf, 12); // Смещение от начала файла до SignatureList (StrAssocArray)
						int slang = SLib.BytesToInt(temp_buf, 16); // Идентификатор языка
						int ver = SLib.BytesToInt(temp_buf, 20); // Версия файла
						int descr_list_offs = SLib.BytesToInt(temp_buf, 24); // Смещение от начала файла до DescrPosList (LAssocArray)
						//
						//stream.skip(5);
						//stream.
						LangStrCollItem lsc_item = GetOrConstructList(slang);
						stream.reset();
						stream.mark(0x7fffffff);
						stream.skip(list_offs);
						lsc_item.List = new SLib.StrAssocArray();
						lsc_item.List.Read(stream);
						stream.reset();
						stream.mark(0x7fffffff);
						stream.skip(slist_offs);
						SignatureList = new SLib.StrAssocArray();
						SignatureList.Read(stream);
						GrpSymbList = new SLib.StrAssocArray();
						GrpSymbList.Read(stream);
						HashGroupList = SLib.ReadIntArray(stream);
						//lsc_item.HashAssocList = SLib.ReadLAssocArray(stream);
						lsc_item.HashAssocList = new SLib.LAssocVector();
						lsc_item.HashAssocList.Read(stream);
						stream.reset();
						stream.mark(0x7fffffff);
						stream.skip(descr_list_offs);
						//lsc_item.DescrPosList = SLib.ReadLAssocArray(stream);
						lsc_item.DescrPosList = new SLib.LAssocVector();
						lsc_item.DescrPosList.Read(stream);
						ok = true;
					}
				}
			}
		} catch(IOException exn) {
			exn.printStackTrace();
			ok = false;
		}
		return ok;
	}
}
