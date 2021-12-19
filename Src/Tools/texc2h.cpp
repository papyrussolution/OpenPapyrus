// TEXC2H.CPP
// Copyright (c) M.Kazakov, 2010, 2017, 2020
//
#include <slib.h>
//
//
#define TAG_OPENING				0
#define TAG_CLOSING				1

#define TTYPE_WITHENDING		0
#define TTYPE_SINGLE			1
#define TTYPE_CONTAINER_ITEM	2
#define TTYPE_CONTAINER			3
#define TTYPE_CONTAINER_TERM	4
#define TTYPE_IMAGE				5
#define TTYPE_SIMAGE			6
#define TTYPE_SYMBOL			7
#define TTYPE_DESCR				8
#define TTYPE_IGNORED			9

#define TEMP_DIR	"temp\\"

struct Texc2h {
	Texc2h();
	~Texc2h();
	int    InclLvl;
	int    BrkCount;
	STab   TagsTab;
	SString CurrTopicName;
	SString PictPrefix;
	SString PrjFileName;
	TSStack <uint> * P_TagsStack;
	TSStack <uint> * P_ContnrsStack;
};

Texc2h::Texc2h() : InclLvl(0), BrkCount(0)
{
	P_TagsStack = new TSStack<uint>();
	P_ContnrsStack = new TSStack<uint>();
}

Texc2h::~Texc2h()
{
	delete P_TagsStack;
	delete P_ContnrsStack;
}

static int LoadTagsList(Texc2h *const pTexc2h)
{
	uint	pos = 0;
	SString	fname = SLS.GetExePath();
	if(fname.Search(".exe", 0, 0, &pos) > 0) {
		fname.Excise(pos, 4);
		fname.Cat(".tgs");
	}
	STabFile tb_file(fname, 0);
	if(tb_file.IsValid())
		tb_file.LoadTab("texc2h_TAGS_TABLE", pTexc2h->TagsTab);
	else {
		printf("File not found: %s\n", fname.cptr());
		exit(1);
	}
	return 0;
}

static int LineIsComment(SString *const pLine)
{
	int i = 0, ch = 0, result = FALSE;
	while((ch = pLine->C(i++))) {
		switch(ch) {
			case '%': result = TRUE; break;
			case ' ':
			case '\t': continue;
			default: result = FALSE;
		}
		break;
	}
	return result;
}

static void LinePrepare(SString *const pLine, int npos)
{
	while(true) {
		switch(pLine->C(npos)) {
			case ' ':
				switch(pLine->C(npos + 1)) {
					case '{':
					case '[':
					case ' ':
					case '\\':
					case '\t':
					case '\n':
						pLine->Excise(npos, 1);
						npos++;
						continue;
					default:
						break;
				}
				break;
			case '\t':
				pLine->Excise(npos, 1);
				pLine->Insert(npos, " ");
				continue;
		}
		break;
	}
}

int AddProjectResource(Texc2h * const pTexc2h, SString const * ss_buf)
{
	int		is_exist = 0;
	int		search_up = 0;
	char	cbuf[MAX_PATH];
	SFile	proj_file(pTexc2h->PrjFileName, SFile::mRead);
	SString	line;
	if(proj_file.IsValid()) {
		while(proj_file.ReadLine(line)) {
			line.Chomp();
			if(line == "[FILES]")
				search_up = 1;
			else if(search_up) {
				if(line == *ss_buf) {
					is_exist = 1;
					break;
				}
			}
		}
		proj_file.Close();
		if(!is_exist) {
			ss_buf->CopyTo(cbuf, ss_buf->Len() + 1);
			cbuf[ss_buf->Len() + 1] = 0;
			WritePrivateProfileSection(_T("FILES"), SUcSwitch(cbuf), SUcSwitch(pTexc2h->PrjFileName));
			printf("Added into project: %s\n", cbuf);
		}
	}
	else {
		puts("HHP project file not found");
		exit(1);
	}
	return 0;
}

static int Translate(Texc2h *const pTexc2h, SString *const pLine)
{
	int    ok = 0;
	int    ch = 0, brk = 0, brc_i = 0, type = 0;
	uint   t_pos = 0;
	size_t pos = 0, npos = 0;
	STab::Row row;
	SString buf, name, sub, tag;
	if(pLine->Search("%topic", 0, 0, &pos)) {
		if(pLine->Search("(", pos, 0, &npos)) {
			pTexc2h->CurrTopicName.Destroy();
			pTexc2h->CurrTopicName = "HELP_";
			while(ch = pLine->C(++npos)) {
				if(ch == ')') {
					pLine->Excise(pos, npos - pos + 1);
					pTexc2h->CurrTopicName.Cat(".HTML");
					AddProjectResource(pTexc2h, &pTexc2h->CurrTopicName);
					break;
				}
				else if(ch == ' ') {
					// nothing
				}
				else {
					pTexc2h->CurrTopicName.CatChar(ch);
				}
			}
		}
	}
	else if(pLine->Search("%endtopic", 0, 0, &pos))
		pTexc2h->CurrTopicName.Destroy();
	
	if(pTexc2h->CurrTopicName.NotEmpty()) {
		if(LineIsComment(pLine)) {
			pLine->Destroy();
		}
		else if(pLine->C(0) == '\n') {
			pTexc2h->BrkCount++;
			if(pTexc2h->BrkCount == 2) {
				pTexc2h->BrkCount = 0;
				pLine->Insert(0, "<P>");
			}
		}
		else {
			if(pTexc2h->BrkCount == 1) {
				pTexc2h->BrkCount = 0;
				pLine->Insert(0, "<BR>");
			}
			while((pTexc2h->TagsTab.GetRow(t_pos, row) > 0)) {
				if(row.Get(0 /* <- первый(нулевой) столбец, имя tex тега */, name) > 0) {
					/* поиск тега в строке */
					if(pLine->Search(name, 0, 0, &pos) > 0) {
						LinePrepare(pLine, pos + name.Len());
						switch(pLine->C(pos + name.Len())) {
							case '{':
							case ' ':
							case '[':
							case '<':
							case '\\':
							case '\t':
							case '\n':
							case '.': ok = 1; break;
							default: ok = 0;
						}
						
						if(ok) {
							if(row.Get(1 /* <- столбец содержит заменитель tex тега */, sub) > 0) {
								row.Get(2 /* <- столбец содержит тип tex тега */, buf);
								type = atol(buf);
								/* вырезаем tex тег */
								pLine->Excise(pos, name.Len());
								/* processing */
								switch(type) {
									case TTYPE_WITHENDING:
									{
										tag.Destroy();
										tag.CatTagBrace(sub, TAG_OPENING);
										/* вставляем открывающий HTML тег */
										pLine->Insert(pos, tag);
										/* поиск '{' */
										if(pLine->Search("{", pos, 0, &pos) > 0) {
											pLine->Excise(pos, 1);
											brc_i++;
											while(!brk && (ch = pLine->C(pos))) {
												switch(ch) {
													case '}':
													{
														brc_i--;
														if(brc_i == 0) {
															/* вырезаем '}' */
															pLine->Excise(pos, 1);
															tag.Destroy();
															tag.CatTagBrace(sub, TAG_CLOSING);
															/* вставляем закрывающий HTML тег */
															pLine->Insert(pos, tag);
															brk = 1;
														}
														else
															pos++;
														break;
													}
													case '{':
														brc_i++;
													default:
														pos++;
												}
											}
										}
										break;
									}
									case TTYPE_SINGLE:
										tag.Destroy();
										tag.CatTagBrace(sub, TAG_OPENING);
										pLine->Insert(pos, tag);
										break;
									case TTYPE_CONTAINER:
									{
										tag.Destroy();
										tag.CatTagBrace(sub, TAG_OPENING);
										/* вставляем открывающий HTML тег */
										pLine->Insert(pos, tag);
										pTexc2h->P_ContnrsStack->push(t_pos);
										/* для тега TABLE */
										if(sub == "TABLE")
											pos += 7;	// 7 - длина строки "<TABLE>"
											pLine->Insert(pos, "<TR><TD>");
										break;
									}
									case TTYPE_CONTAINER_TERM:
									{
										uint line_n;
										/* для тега TABLE */
										if(sub == "TABLE") {
											pLine->Insert(pos, "</TD></TR>");
											pos += 10;	// 10 - длина строки "</TD></TR>"
										}
										tag.Destroy();
										tag.CatTagBrace(sub, TAG_CLOSING);
										/* вставляем закрывающий HTML тег */
										pLine->Insert(pos, tag);
										pTexc2h->P_ContnrsStack->pop(line_n);
										break;
									}
									case TTYPE_CONTAINER_ITEM:
									{
										uint		line_n = pTexc2h->P_ContnrsStack->peek();
										uint		npos = 0, br_pos;
										SString		st, lst, rst, item;
										StringSet	ss('&', sub);
										if(pTexc2h->TagsTab.GetRow(line_n, row) > 0) {
											if(row.Get(1, item) > 0) {
												while(ss.get(&npos, st)) {
													if(st.Divide('|', lst, rst) > 0) {
														if(lst == item) {
															tag.Destroy();
															tag.CatTagBrace(rst, TAG_OPENING);
															/* вставляем HTML тег */
															pLine->Insert(pos, tag);
															/* для тега DL (Description List) */
															if(lst == "DL") {
																if(pLine->Search("[", pos, 0, &br_pos) > 0) {
																	pLine->Excise(br_pos, 1);
																	pLine->Insert(br_pos, "<B>");
																	if(pLine->Search("]", br_pos, 0, &br_pos) > 0) {
																		pLine->Excise(br_pos, 1);
																		pLine->Insert(br_pos, "</B>");
																	}
																}
															}
															break;
														}
													}
												}
											}
										}
										break;
									}
									case TTYPE_IMAGE:
									{
										SString img;
										tag.Destroy();
										if(pLine->Search("{", pos, 0, &pos) > 0) {
											pLine->Excise(pos, 1);
											brc_i++;
											pLine->Insert(pos, "<IMG SRC=");
											pos += 9;	// 9 - длина строки "<IMG SRC="
											img = pTexc2h->PictPrefix;
											while(ch = pLine->C(pos)) {
												if(ch == '}') {
													img.Cat(".png");
													AddProjectResource(pTexc2h, &img);
													brc_i--;
													pLine->Excise(pos, 1);
													pLine->Insert(pos, ".png><BR>");
													if(pLine->Search("{", pos, 0, &pos) > 0) {
														brc_i++;
														pLine->Excise(pos, 1);
														pLine->Insert(pos, "<I>");
														if(pLine->Search("}", pos, 0, &pos) > 0) {
															brc_i--;
															pLine->Excise(pos, 1);
															pLine->Insert(pos, "</I><BR>");
														}
													}
													break;
												}
												else {
													img.CatChar(ch);
												}
												pos++;
											}
										}
										break;
									}
									case TTYPE_SIMAGE:
									{
										uint	npos;
										SString	img;
										tag.Destroy();
										if(pLine->Search("{", pos, 0, &pos) > 0) {
											pLine->Excise(pos, 1);
											brc_i++;
											pLine->Insert(pos, "<IMG SRC=");
											pos += 9;	// 9 - длина строки "<IMG SRC="
											img = pTexc2h->PictPrefix;
											while(ch = pLine->C(pos)) {
												if(ch == '}') {
													img.Cat(".png");
													AddProjectResource(pTexc2h, &img);
													brc_i--;
													pLine->Excise(pos, 1);
													pLine->Insert(pos, ".png><BR>");
													if(pLine->Search("{", pos, 0, &pos) > 0) {
														brc_i++;
														pLine->Excise(pos, 1);
														pLine->Insert(pos, "<I>");
														if(pLine->Search("}", pos, 0, &pos) > 0) {
															brc_i--;
															pLine->Excise(pos, 1);
															pLine->Insert(pos, "</I><BR>");
															if(pLine->Search("{", pos, 0, &pos) > 0) {
																brc_i++;
																pLine->Excise(pos, 1);
																if(pLine->Search("}", pos, 0, &npos) > 0) {
																	brc_i--;
																	pLine->Excise(pos, npos - pos + 1);
																}
															}
														}
													}
												}
												else {
													img.CatChar(ch);
												}
												pos++;
											}
										}
										break;
									}
									case TTYPE_SYMBOL:
									{
										pLine->Insert(pos, sub);
										break;
									}
									case TTYPE_DESCR:
									{
										pLine->Insert(pos, sub);
										if(pLine->Search("{", pos, 0, &pos) > 0) {
											pLine->Excise(pos, 1);
											pLine->Insert(pos, "<U>");
											if(pLine->Search("}", pos, 0, &pos) > 0) {
												pLine->Excise(pos, 1);
												pLine->Insert(pos, "</U>");
											}
										}
										break;
									}
									case TTYPE_IGNORED:
									{
										uint npos;
										if(pLine->Search("{", pos, 0, &pos) > 0) {
											brc_i++;
											pLine->Excise(pos, 1);
											if(pLine->Search("}", pos, 0, &npos) > 0) {
												brc_i--;
												pLine->Excise(pos, npos - pos + 1);
											}
										}	
										break;
									}
									default:
										break;
								}
								
								if(brc_i) {
									if(type == TTYPE_WITHENDING) {
										pTexc2h->P_TagsStack->push(t_pos);
										pTexc2h->InclLvl += brc_i;
									}
								}
							}
						}
						else
							t_pos++;
					}
					else
						t_pos++;
				}
			}
			
			if(pTexc2h->InclLvl != 0) {
				pos = 0;
				while((ch = pLine->C(pos))) {
					switch(ch) {
						case '}':
						{
							pTexc2h->InclLvl--;
								if(pTexc2h->P_TagsStack->pop(t_pos) > 0) {
									if(pTexc2h->TagsTab.GetRow(t_pos, row) > 0) {
										if(row.Get(1, buf) > 0) {
											/* вырезаем '}' */
											pLine->Excise(pos, 1);
											tag.Destroy();
											tag.CatTagBrace(buf, TAG_CLOSING);
											/* вставляем закрывающий HTML тег */
											pLine->Insert(pos, tag);
										}
									}
								}
							pos++;
						}
						case '{':
							pTexc2h->InclLvl++;
						default:
							pos++;
					}
				}
			}
		}
	}

	return ok;
}

static void PrintHTMLHeader(SFile * pFile, const char * pTitle)
{
	if(pFile->IsValid()) {
		pFile->WriteLine("<HTML>\n");
		pFile->WriteLine("<HEAD>\n");
		pFile->WriteLine("<TITLE>");
		pFile->WriteLine(pTitle);
		pFile->WriteLine("</TITLE>\n");
		pFile->WriteLine("<LINK REL=\"stylesheet\" TYPE=\"text/css\" HREF=\"style.css\" />");
		pFile->WriteLine("</HEAD>\n");
		pFile->WriteLine("\n");
		pFile->WriteLine("<BODY>\n");
	}
}

static void PrintHTMLEnding(SFile * pFile)
{
	if(pFile->IsValid()) {
		pFile->WriteLine("\n");
		pFile->WriteLine("</BODY>\n");
		pFile->WriteLine("</HTML>\n");
	}
}

int main(int argc, char * argv[])
{
	int    ret = 0;
	puts("Tex to HTML translator\n===========================");
	if(argc < 5) {
		printf("Usage: texc2h.exe [file.tex] [HHP project file path] [pictures prefix] [output path]");
	}
	else {
		SLS.Init("texc2h", 0);
		Texc2h texc2h;
		SFile  in_file;
		SFile  out_file;
		SString out_dir;
		SString out_fpath;
		SString tmp_out_dir;
		SString tmp_out_fpath;
		SString line;
		uint   pos;
		if(!in_file.Open(argv[1], SFile::mRead)) {
			printf("File not found: %s\n", argv[1]);
			ret = -1;
		}
		else {
			printf("Tex file: %s\n", argv[1]);
			//
			// store hhp project file path
			//
			texc2h.PrjFileName = argv[2];
			//
			// prepare pictures prefix string 
			//
			(texc2h.PictPrefix = argv[3]).RmvLastSlash().Cat("\\");
			//
			// prepare out folder string 
			//
			(out_dir = argv[4]).RmvLastSlash().Cat("\\");
			
			(tmp_out_dir = out_dir).Cat(TEMP_DIR);
			::CreateDirectory(SUcSwitch(tmp_out_dir), NULL);
			//
			// read tags assoc table 
			//
			LoadTagsList(&texc2h);
			puts("Begin translation...");
			while(in_file.ReadLine(line)) {
				if(line.Len() != 0) {
					Translate(&texc2h, &line);
					if(texc2h.CurrTopicName.NotEmpty()) {
						(tmp_out_fpath = tmp_out_dir).Cat(texc2h.CurrTopicName);
						if(!out_file.IsValid()) {
							out_file.Open(tmp_out_fpath, SFile::mWrite);
							PrintHTMLHeader(&out_file, texc2h.CurrTopicName);
							puts(texc2h.CurrTopicName);
							out_file.WriteLine(line);
						}
						else {
							if(out_file.GetName() == tmp_out_fpath)
								out_file.WriteLine(line);
							else {
								out_file.WriteLine(line);
								tmp_out_fpath = out_file.GetName();
								PrintHTMLEnding(&out_file);
								out_file.Close();
								out_fpath = tmp_out_fpath;
								if(out_fpath.Search(TEMP_DIR, 0, 0, &pos))
									out_fpath.Excise(pos, 5);
								if(out_file.Compare(tmp_out_fpath, out_fpath, 0) <= 0)
									copyFileByName(tmp_out_fpath, out_fpath);
								::DeleteFile(SUcSwitch(tmp_out_fpath));
							}
						}
					}
					else {
						if(out_file.IsValid()) {
							tmp_out_fpath = out_file.GetName();
							PrintHTMLEnding(&out_file);
							out_file.Close();
							out_fpath = tmp_out_fpath;
							if(out_fpath.Search(TEMP_DIR, 0, 0, &pos))
								out_fpath.Excise(pos, 5);
							if(out_file.Compare(tmp_out_fpath, out_fpath, 0) <= 0)
								copyFileByName(tmp_out_fpath, out_fpath);
							::DeleteFile(SUcSwitch(tmp_out_fpath));
						}
					}
				}
			}
			in_file.Close();
			out_file.Close();
			RemoveDirectory(SUcSwitch(tmp_out_dir));
			{
				//
				// Создаем пустой файл empty_target.txt для того, чтобы компилятор понял, 
				// что работа сделана и не пытался при каждой сборке запускать этот скрипт.
				//
				(line = out_dir).SetLastSlash().Cat("empty_target.txt");
				SFile f_empty_target(line, SFile::mWrite);
			}
			puts("done");
		}
	}
	return ret;
}