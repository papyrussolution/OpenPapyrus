// TEST-XML.CPP
// Copyright (c) A.Sobolev 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

struct TestXml_DataEntry {
	TestXml_DataEntry() : A(0), C(0.0)
	{
	}
	void   Generate()
	{
		SRandGenerator & r_rg = SLS.GetTLA().Rg;
		A = r_rg.GetUniformIntPos(1000000);
		{
			char   bin_buf[4096];
			const  uint   len = r_rg.GetUniformIntPos(sizeof(bin_buf));
			assert(len > 0 && len < sizeof(bin_buf));
			r_rg.ObfuscateBuffer(bin_buf, len);
			B.EncodeMime64(bin_buf, len);
		}
		C = r_rg.GetReal();
	}
	int    A;
	SString B;
	double C;
};

SLTEST_R(XML)
{
	const char * p_root_name = "test_xml_data_list";
	SString temp_buf;
	{
		const uint item_count = 1000;
		//
		// Создаем xml-файл с именем, содержащим русские буквы
		//
		TSCollection <TestXml_DataEntry> data_list;
		SString file_name(MakeOutputFilePath("xml-файл-with-non-ascii-символами-в имени.xml"));
		assert(file_name.IsLegalUtf8()); // Исходный файл - в utf8-кодировке!
		{
			xmlTextWriter * p_x = xmlNewTextWriterFilename(file_name, 0);
			THROW(SLCHECK_NZ(p_x));
			{
				SXml::WDoc _doc(p_x, cpUTF8);
				{
					SXml::WNode n_test_data(p_x, p_root_name);
					for(uint i = 0; i < item_count; i++) {
						TestXml_DataEntry * p_entry = data_list.CreateNewItem();
						if(p_entry) {
							p_entry->Generate();
							{
								SXml::WNode n_item(p_x, "item");
								n_item.PutInner("A", temp_buf.Z().Cat(p_entry->A));
								n_item.PutInner("B", temp_buf.Z().Cat(p_entry->B));
								n_item.PutInner("C", temp_buf.Z().Cat(p_entry->C, MKSFMTD(0, 12, NMBF_NOTRAILZ)));
							}
						}
					}
				}
			}
			xmlFreeTextWriter(p_x);
			p_x = 0;
		}
		{
			//
			// Теперь читаем тот же файл и проверяем, что все данные на месте
			//
			xmlParserCtxt * p_ctx = 0;
			xmlDoc * p_doc = 0;
			xmlNode * p_root = 0;
			THROW_SL(SLCHECK_NZ(fileExists(file_name)));
			p_ctx = xmlNewParserCtxt();
			THROW_SL(SLCHECK_NZ(p_ctx));
			p_doc = xmlCtxtReadFile(p_ctx, file_name, 0, XML_PARSE_NOENT);
			THROW_SL(SLCHECK_NZ(p_doc));
			p_root = xmlDocGetRootElement(p_doc);
			THROW_SL(SLCHECK_NZ(p_root));
			THROW_SL(SLCHECK_NZ(SXml::IsName(p_root, p_root_name)));
			{
				for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
					if(SXml::IsName(p_n, "item")) {
						TestXml_DataEntry _entry;
						for(const xmlNode * p_in = p_n->children; p_in; p_in = p_in->next) {
							if(SXml::GetContentByName(p_in, "A", temp_buf) > 0) {
								_entry.A = temp_buf.ToLong();
							}
							else if(SXml::GetContentByName(p_in, "B", temp_buf) > 0) {
								_entry.B = temp_buf;
							}
							else if(SXml::GetContentByName(p_in, "C", temp_buf) > 0) {
								_entry.C = temp_buf.ToReal();
							}
						}
						{
							// Необходимо найти считанный элемент в списке data_list 
							uint   f_count = 0; // Количество найденных элементов (должно быть строго 1)
							for(uint i = 0; i < data_list.getCount(); i++) {
								const TestXml_DataEntry * p_entry = data_list.at(i);
								if(p_entry) {
									if(p_entry->A == _entry.A && p_entry->B.IsEq(_entry.B) && feqeps(p_entry->C, _entry.C, 1E-7)) {
										f_count++;
									}
								}
							}
							SLCHECK_EQ(f_count, 1U);
						}
					}
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}