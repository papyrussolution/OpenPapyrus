// KABQ-MAIN.CPP
// Copyright (c) A.Sobolev 2024
//
#include <pp.h>
#pragma hdrstop
#include <imgui-support.h>
#include <..\OSF\ImGuiFileDialog\ImGuiFileDialog.h>
#include <..\SLib\gumbo\gumbo.h>

static ImGuiRuntimeBlock ImgRtb;
//static const ImVec2 ButtonSize_Std(64.0f, 24.0f);
//static const ImVec2 ButtonSize_Double(128.0f, 24.0f);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // Forward declare message handler from imgui_impl_win32.cpp

class SingleFilePaneBlock {
public:
	SingleFilePaneBlock() : P(), State(0), CurrentIdx(0)
	{
	}
	struct Param {
		Param() : Columns(SFile::propNamExt|SFile::propSize|SFile::propModTime), SortModifier(0)
		{
		}
		uint8  ReserveStart[64];
		uint32 Columns; // flags SFile::propXXX
		uint32 SortModifier; // Модификатор порядка сортировки элементов
		uint8  Reserve[64];
		LongArray SortOrder; // Список значений SFile::propXXX, определяющий порядок сортировки
		SString Path; // Текущий путь, отображаемый на панеле
	};

	bool Run(const char * pPath)
	{
		return ProcessPath(pPath);
	}
	bool SetCurrentIdx(uint idx/*[1..]*/)
	{
		if(idx > 0 && idx <= Fep.GetCount()) {
			CurrentIdx = idx;
			return true;
		}
		else
			return false;
	}
	uint GetCurrentIdx() const
	{
		return (CurrentIdx > 0 && CurrentIdx <= Fep.GetCount()) ? CurrentIdx : 0;
	}
	bool ProcessPath(const char * pPath)
	{
		bool   ok = false;
		SString temp_buf;
		if(!pPath && State & stInited) {
			Fep.GetInitPath(temp_buf);
			if(!temp_buf.IsEmpty()) {
				//ok = LOGIC(Fep.Scan(temp_buf, "*.*", SFileEntryPool::scanfKeepCase|SFileEntryPool::scanfReckonDirs|SFileEntryPool::scanfReckonUpFolder));
				ok = true;
				if(ok) {
					//Fep.Sort(SFileEntryPool::scByName);
					State |= stInited;
				}
			}
		}
		else if(SFile::IsDir(pPath)) {
			SFsPath::NormalizePath(pPath, SFsPath::npfCompensateDotDot, temp_buf);
			ok = LOGIC(Fep.Scan(temp_buf, "*.*", SFileEntryPool::scanfKeepCase|SFileEntryPool::scanfReckonDirs|SFileEntryPool::scanfReckonUpFolder));
			if(ok) {
				Fep.Sort(SFileEntryPool::scByName);
				CurrentIdx = 0;
				State |= stInited;
			}
		}
		return ok;
	}

	Param  P; // persistent-блок параметров
	// Далее следуют факторы состояния //
	enum {
		stInited = 0x0001
	};
	uint   State;
	SFileEntryPool Fep;
	uint CurrentIdx; // [1..] (0 - undef)
	LongArray SelectedIdxList;
};

struct FilePaneBlock {
	FilePaneBlock() : P()
	{
		// В списке должен быть хотя бы один элемент
		SingleFilePaneBlock * p_entry = L.CreateNewItem();
	}
	struct Param { // persistent-блок параметров
		Param() : MpMode(mpmodeSingle)
		{
		}
		//
		// Режим отображения панелей
		//
		enum {
			mpmodeSingle = 0, // Единственная панель
			mpmodeDouble,     // Двойная панель
			mpmodeTabbed      // Табулированный список панелей
		};
		uint   MpMode; // mpmodeXXX
	};
	Param  P;
	TSCollection <SingleFilePaneBlock> L; 
};

struct HtmlParserBlock {
	HtmlParserBlock()
	{
	}
	~HtmlParserBlock()
	{
	}
	SString Url;
};

class Kabq_ImGuiSceneBlock : public ImGuiSceneBase {
public:
	enum {
		loidMainFrame                 = 1,
		loidWorkFrame                 = 10001, // Workframe
		loidTopicbar                  = 10002, // 
		loidContentFrame              = 10003,
		loidSearchbar                 = 10004,
		loidContentArea               = 10005,
		loidToolbar                   = 10027, // Область панели инструментов в верхней части окна
		loidStatusbar                 = 10028,
		loidContentAreaTwoPane        = 10029, // Область отображения контента в виде 2 панелей. Динамически вставляется в loidContentArea
		loidContentAreaTwoPane_Left   = 10030, // Левая часть 2-панельной области отображения контента
		loidContentAreaTwoPane_Right  = 10031, // Правая часть 2-панельной области отображения контента
	};
	enum {
		topicUndef = 0,
		topicComputer = 1,
		topicProcesses = 2,
		topicSystem = 3,
		topicNotes = 4,
		topicFinance = 5,
		topicScores = 6,
		topicPasswordVault = 7,
		topicHtmlParser = 8
	};
	class State {
	public:
		State() : Topic(topicUndef), FpBlk()/*, DirBlk(*FpParam.L.at(0))*/
		{
		}
		struct SearchInputBlock {
			SearchInputBlock()
			{
				Buf[0] = 0;
			}
			void ProcessInput()
			{
				if(PreserveInput != Buf) {
					Tr.Run(reinterpret_cast<const uchar *>(Buf), sstrleni(Buf), Nta.Z(), 0);
					PreserveInput = Buf;
				}
			}
			char    Buf[2048];
			SNaturalTokenArray Nta;
		private:
			STokenRecognizer Tr;
			SString PreserveInput;
		};
		SearchInputBlock SiBlk;
		FilePaneBlock FpBlk;
		int    Topic;
	};
	class InitializedConstData {
	public:
		InitializedConstData(const UiDescription * pUid) : ButtonStd_Height(0.0f),
			ButtonStd_Width(0.0f), ButtonDouble_Width(0.0f)
		{
			int v;
			ButtonStd_Height = (pUid && pUid->VList.Get(UiValueList::vButtonStdHeight, v) && v > 0) ? (float)v : 24.0f;
			ButtonStd_Width = (pUid && pUid->VList.Get(UiValueList::vButtonStdWidth, v) && v > 0) ? (float)v : 64.0f;
			ButtonDouble_Width = (pUid && pUid->VList.Get(UiValueList::vButtonDoubleWidth, v) && v > 0) ? (float)v : 128.0f;
			ButtonSize_Std.x = ButtonStd_Width;
			ButtonSize_Std.y = ButtonStd_Height;
			ButtonSize_Double.x = ButtonDouble_Width;
			ButtonSize_Double.y = ButtonStd_Height;
		}
		float ButtonStd_Height;
		float ButtonStd_Width;
		float ButtonDouble_Width;
		ImVec2 ButtonSize_Std;
		ImVec2 ButtonSize_Double;
	};
	Kabq_ImGuiSceneBlock() : ImGuiSceneBase(), P_ICD(0)
	{
	}
	~Kabq_ImGuiSceneBlock()
	{
		ZDELETE(P_ICD);
	}
	virtual int Init(ImGuiIO & rIo);
	void BuildScene();
private:
	static int CbInput_Search(ImGuiInputTextCallbackData * pInputData);
	void    ProcessSearchInput(ImGuiContext * pImCtx);
	void    Build_FsDir(SUiLayout * pParentLayout, int loId, const char * pWindowIdent);
	void    Build_FsDir2(uint filePaneIdx);
	void    Build_HtmlParser(SUiLayout * pParentLayout, int loId, const char * pWindowIdent);
	State St;
	const InitializedConstData * P_ICD;
};

/*virtual*/int Kabq_ImGuiSceneBlock::Init(ImGuiIO & rIo)
{
	int    ok = 1;
	SString temp_buf;
	LoadUiDescription();
	const UiDescription * p_uid = SLS.GetUiDescription();
	P_ICD = new InitializedConstData(p_uid);
	{
		const bool use_ui_descripton = true;
		ImGuiStyle * p_dest_style = 0;
		ImGuiStyle * style = p_dest_style ? p_dest_style : &ImGui::GetStyle();
		ImVec4 * colors = style->Colors;
		colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]               = SColor(0x2B, 0x30, 0x38);//ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
		colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]                = SColor(SClrBlack, 0.95f); //ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
		colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
		colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
		colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);// Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);// Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		if(use_ui_descripton && p_uid) {
			const SColorSet * p_cs = p_uid->GetColorSetC("imgui-style");	
			if(p_cs) {
				for(uint i = 0; i < SIZEOFARRAY(style->Colors); i++) {
					const char * p_color_name = ImGui::GetStyleColorName(i);
					if(!isempty(p_color_name)) {
						SColor c;
						if(p_uid->GetColor(p_cs, p_color_name, c)) {
							style->Colors[i] = c;
						}
					}
				}
			}
		}
	}
	if(p_uid) {
		const SColorSet * p_cs = p_uid->GetColorSetC("imgui-style");	
		{
			ImFontConfig f;
			SColor primary_font_color = p_uid->GetColorR(p_cs, "TextPrimary", SColor(SClrWhite));
			SColor secondary_font_color = p_uid->GetColorR(p_cs, "TextSecondary", SColor(SClrSilver));
			SColor substrat_color = p_uid->GetColorR(p_cs, "Substrat", SColor(0x1E, 0x22, 0x28));
			ClearColor = substrat_color;
			{
				//const char * p_font_face_list[] = { "Roboto", "DroidSans", "Cousine", "Karla", "ProggyClean", "ProggyTiny" };
				{
					const SFontSource * p_fs = p_uid->GetFontSourceC("Roboto");
					if(p_fs) {
						PPGetPath(PPPATH_BIN, temp_buf);
						temp_buf.SetLastSlash().Cat("..").SetLastSlash().Cat(p_fs->Src);
						if(fileExists(temp_buf))
							CreateFontEntry(rIo, "FontSecondary", temp_buf, 16.0f, 0, &secondary_font_color);
					}
				}
				{
					const SFontSource * p_fs = p_uid->GetFontSourceC("DroidSans");
					if(p_fs) {
						PPGetPath(PPPATH_BIN, temp_buf);
						temp_buf.SetLastSlash().Cat("..").SetLastSlash().Cat(p_fs->Src);
						if(fileExists(temp_buf))
							CreateFontEntry(rIo, "FontPrimary", temp_buf, 18.0f, 0, &primary_font_color);
					}
				}
			}
		}
	}
	return ok;
}

void Kabq_ImGuiSceneBlock::ProcessSearchInput(ImGuiContext * pImCtx)
{
	//Tr.Run(reinterpret_cast<const uchar *>(St.SiBlk.Buf), sstrleni(St.SiBlk.Buf), St.SiBlk.Nta.Z(), 0);
}

/*static*/int Kabq_ImGuiSceneBlock::CbInput_Search(ImGuiInputTextCallbackData * pInputData)
{
	bool debug_mark = false;
	if(pInputData) {
		Kabq_ImGuiSceneBlock * p_this = static_cast<Kabq_ImGuiSceneBlock *>(pInputData->UserData);
		if(p_this) {
			p_this->ProcessSearchInput(pInputData->Ctx);
		}
	}
	return 0;
}

void Kabq_ImGuiSceneBlock::Build_FsDir2(uint filePaneIdx)
{
	SingleFilePaneBlock * p_sfp_blk = St.FpBlk.L.at(filePaneIdx);
	if(p_sfp_blk) {
		SString temp_buf;
		if(!(p_sfp_blk->State & SingleFilePaneBlock::stInited)) {
			if(!GetKnownFolderPath(UED_FSKNOWNFOLDER_COMPUTER, temp_buf))
				if(!GetKnownFolderPath(UED_FSKNOWNFOLDER_STARTUP, temp_buf))
					if(!GetKnownFolderPath(UED_FSKNOWNFOLDER_DESKTOP, temp_buf))
						temp_buf = "c:\\";
			p_sfp_blk->Run(temp_buf);
		}
		else {
			p_sfp_blk->Run(0);
		}
		{
			SFileEntryPool::Entry fe;
			if(ImGui::BeginTable("view_directory", 3, ImGuiTableFlags_Resizable|ImGuiTableFlags_NoSavedSettings|ImGuiTableFlags_Borders|ImGuiTableFlags_ScrollY)) {
				ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupScrollFreeze(1, 1);
				ImGui::TableHeadersRow();
				ImGuiListClipper clipper;
				bool    enter_pressed_idx = false;
				clipper.Begin(p_sfp_blk->Fep.GetCount());
				while(clipper.Step()) {
					//for(uint i = 0; i < St.DirBlk.Fep.GetCount(); i++) {
					for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
						p_sfp_blk->Fep.Get(i, &fe, 0);
						ImGui::TableNextRow();
						{
							ImGui::TableSetColumnIndex(0);
							//ImGui::Text(fe.Name);
							bool is_selected = false;//St.DirBlk.GetCurrentIdx() == static_cast<uint>(i+1);
							if(ImGui::Selectable(fe.Name, is_selected, ImGuiSelectableFlags_SpanAllColumns|ImGuiSelectableFlags_AllowDoubleClick)) {
								if(ImGui::IsMouseDoubleClicked(0/*left*/)) {
									enter_pressed_idx = true;
									p_sfp_blk->SetCurrentIdx(i+1);
								}
							}
						}
						{
							ImGui::TableSetColumnIndex(1);
							temp_buf.Z().Cat(fe.Size);
							if(false) {
								const auto right_edge = ImGui::GetCursorPosX() + ImGui::GetColumnWidth();
								const auto text_width = ImGui::CalcTextSize(temp_buf).x + 2 * ImGui::GetStyle().ItemSpacing.x;
								const auto pos_x = (right_edge - text_width - ImGui::GetScrollX());
								ImGui::SetCursorPosX(pos_x);
								ImGui::Text(temp_buf);
							}
							else {
								ImGui::Text(temp_buf);
							}
						}
						{
							LDATETIME dtm_mod;
							dtm_mod.SetNs100(fe.ModTm_);
							ImGui::TableSetColumnIndex(2);
							ImGui::Text(temp_buf.Z().Cat(dtm_mod, DATF_ISO8601CENT, 0));
						}
					}
				}
				ImGui::EndTable();
				if(enter_pressed_idx) {
					const uint cur_idx = p_sfp_blk->GetCurrentIdx();
					if(cur_idx && p_sfp_blk->Fep.Get(cur_idx-1, &fe, &temp_buf)) {
						if(fe.IsFolder()) {
							p_sfp_blk->Run(temp_buf);
						}
					}
				}
			}
		}
	}
}

void Kabq_ImGuiSceneBlock::Build_FsDir(SUiLayout * pParentLayout, int loId, const char * pWindowIdent)
{
	if(pParentLayout) {
		uint mpmode = FilePaneBlock::Param::mpmodeDouble;//St.FpBlk.P.MpMode;
		const int view_flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoDecoration;
		if(mpmode == FilePaneBlock::Param::mpmodeSingle) {
			ImGuiWindowByLayout wbl(pParentLayout, /*loidContentArea*/loId, /*"##ContentArea"*/pWindowIdent, view_flags);
			if(wbl.IsValid()) {
				Build_FsDir2(0/*filePaneIdx*/);
			}
		}
		else if(mpmode == FilePaneBlock::Param::mpmodeDouble) {
			SUiLayout * p_lo = pParentLayout->FindById(loId);
			if(p_lo) {
				FRect r = p_lo->GetFrameAdjustedToParent();
				const int double_pane_lo_id = loidContentAreaTwoPane;
				const SUiLayout * p_lo_double_pane = Cache_Layout.Get(&double_pane_lo_id, sizeof(double_pane_lo_id));
				if(p_lo_double_pane) {
					SUiLayout lo_double_pane(*p_lo_double_pane);
					SUiLayout::Param evp;
					evp.ForceSize = r.GetSize();
					lo_double_pane.Evaluate(&evp);
					//
					{
						const SUiLayout * p_lo_left = lo_double_pane.FindByIdC(loidContentAreaTwoPane_Left);
						ImGuiWindowByLayout wbl(p_lo_left, r.a, /*"##ContentArea"*/"##ContentAreaTwoPane_Left", view_flags);
						if(wbl.IsValid()) {
							if(St.FpBlk.L.getCount() < 1)
								St.FpBlk.L.CreateNewItem();
							assert(St.FpBlk.L.getCount() >= 1);
							Build_FsDir2(0/*filePaneIdx*/);
						}
					}
					{
						const SUiLayout * p_lo_right = lo_double_pane.FindByIdC(loidContentAreaTwoPane_Right);
						ImGuiWindowByLayout wbl(p_lo_right, r.a, /*"##ContentArea"*/"##ContentAreaTwoPane_Right", view_flags);
						if(wbl.IsValid()) {
							if(St.FpBlk.L.getCount() < 2)
								St.FpBlk.L.CreateNewItem();
							assert(St.FpBlk.L.getCount() >= 2);
							Build_FsDir2(1/*filePaneIdx*/);
						}
					}
				}
			}
		}
	}
}

void Kabq_ImGuiSceneBlock::Build_HtmlParser(SUiLayout * pParentLayout, int loId, const char * pWindowIdent)
{
}

void Kabq_ImGuiSceneBlock::BuildScene()
{
	BuildSceneProlog(); // Start the Dear ImGui frame
	const int view_flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoDecoration;
	ImGuiViewport * p_vp = ImGui::GetMainViewport();
	if(p_vp) {
		SString temp_buf;
		ImVec2 sz = p_vp->Size;
		SUiLayout::Param evp;
		evp.ForceSize.x = sz.x;
		evp.ForceSize.y = sz.y;
		int   loid = loidMainFrame;
		const SUiLayout * p_tl_org = Cache_Layout.Get(&loid, sizeof(loid));
		if(p_tl_org) {
			SUiLayout tl(*p_tl_org); // Мы будем работать с копией оригинального лейаута поскольку нам придется ее менять в зависимости от текущего состояния //
			tl.Evaluate(&evp);
			{
				ImGuiWindowByLayout wbl(&tl, loidToolbar, "##Toolbar", view_flags);
				if(wbl.IsValid()) {
					;
				}
			}
			{
				ImGuiWindowByLayout wbl(&tl, loidWorkFrame, "##WorkFrame", view_flags);
				if(wbl.IsValid()) {
					;
				}
			}
			{
				ImGuiWindowByLayout wbl(&tl, loidTopicbar, "##Topicbar", view_flags);
				if(wbl.IsValid()) {
					if(ImGui::Button2("Computer", P_ICD->ButtonSize_Double)) {
						St.Topic = topicComputer;
					}
					if(ImGui::Button2("Notes", P_ICD->ButtonSize_Double)) {
						St.Topic = topicNotes;
					}
					if(ImGui::Button2("Finances", P_ICD->ButtonSize_Double)) {
						St.Topic = topicFinance;
					}
					if(ImGui::Button2("Scores", P_ICD->ButtonSize_Double)) {
						St.Topic = topicScores;
					}
					if(ImGui::Button2("Password Vault", P_ICD->ButtonSize_Double)) {
						St.Topic = topicPasswordVault;
					}
					if(ImGui::Button2("Html parser", P_ICD->ButtonSize_Double)) {
						St.Topic = topicHtmlParser;
					}
					if(ImGui::Button2("(debug) File Dialog", P_ICD->ButtonSize_Double)) {
						IGFD::FileDialogConfig config;
						config.path = ".";
						config.sidePane = 0;
						config.sidePaneWidth = 128.0f;
						ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", "*.*", config);
					}
					if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
						if(ImGuiFileDialog::Instance()->IsOk()) { // action if OK
							std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
							std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
							// action
						}
    					// close
						ImGuiFileDialog::Instance()->Close();
					}
				}
			}
			{
				ImGuiWindowByLayout wbl(&tl, loidContentFrame, "##ContentFrame", view_flags);
				if(wbl.IsValid()) {
					;
				}
			}
			{
				ImGuiWindowByLayout wbl(&tl, loidSearchbar, "##Searchbar", view_flags);
				if(wbl.IsValid()) {
					ImGui::InputText("##SearchInput", St.SiBlk.Buf, sizeof(St.SiBlk.Buf), ImGuiInputTextFlags_CallbackEdit, CbInput_Search, this);
					St.SiBlk.ProcessInput();
					for(uint i = 0; i < St.SiBlk.Nta.getCount(); i++) {
						const SNaturalToken & r_nt = St.SiBlk.Nta.at(i);
						r_nt.GetSymb(temp_buf);
						if(temp_buf.NotEmpty()) {
							if(i)
								ImGui::SameLine(0.0f, 4.0f);
							ImGui::Text(temp_buf);
						}
					}
				}
			}
			{
				if(St.Topic == topicComputer) {
					Build_FsDir(&tl, loidContentArea, "##ContentArea");
				}
				else if(St.Topic == topicHtmlParser) {
					Build_HtmlParser(&tl, loidContentArea, "##ContentArea");
				}
				else {
					ImGuiWindowByLayout wbl(&tl, loidContentArea, "##ContentArea", view_flags);
					if(wbl.IsValid()) {
						ImGui::Text("Under construction...");
					}
				}
			}
			{
				ImGuiWindowByLayout wbl(&tl, loidStatusbar, "##Statusbar", view_flags);
				if(wbl.IsValid()) {
					;
				}
			}
		}
	}
	Render(ImgRtb); // Rendering
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
	switch(msg) {
		case WM_SIZE:
			ImgRtb.OnWindowResize(wParam, lParam);
		    return 0;
		case WM_SYSCOMMAND:
		    if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			    return 0;
		    break;
		case WM_DESTROY:
		    ::PostQuitMessage(0);
		    return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main(int, char **)
{
	int    result = 0;
	DS.Init(PPSession::fWsCtlApp|PPSession::fInitPaths, 0, /*pUiDescriptionFileName*/"uid-kabq.json");
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"WSCTL_WCLS", nullptr };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"KAB-Q", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
	if(!ImgRtb.CreateDeviceD3D(hwnd)) {
		result = 1;
	}
	else {
		// Show the window
		::ShowWindow(hwnd, SW_SHOWDEFAULT);
		::UpdateWindow(hwnd);
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO & io = ImGui::GetIO(); 
		//(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();
		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(ImgRtb.g_pd3dDevice, ImgRtb.g_pd3dDeviceContext);
		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
		// - Read 'docs/FONTS.md' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
		//assert(font != nullptr);
		// Our state
		//bool show_demo_window = false; // @sobolev true-->false
		//bool show_another_window = false;
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		
		Kabq_ImGuiSceneBlock scene_blk;
		scene_blk.Init(io);
		// Main loop
		bool done = false;
		do {
			// Poll and handle messages (inputs, window resize, etc.)
			// See the WndProc() function below for our to dispatch events to the Win32 backend.
			MSG msg;
			while(::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
				if(msg.message == WM_QUIT)
					done = true;
			}
			if(!done) {
				//scene_blk.EmitEvents();
				scene_blk.BuildScene();
			}
		} while(!done);
		// Cleanup
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		//ImgRtb.CleanupDeviceD3D();
		::DestroyWindow(hwnd);
		//::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		result = 0;
	}
	ImgRtb.CleanupDeviceD3D();
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
	return result;
}
