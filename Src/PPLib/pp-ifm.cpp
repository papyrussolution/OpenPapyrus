// PP-IFM.CPP
// Copyright (c) A.Sobolev 2025, 2026
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <pp-ifm.h>

SString & GetORToolsProxiPipeName(SString & rBuf) { return SFile::MakeNamedPipeName(PPConst::PipeORToolsProxi, rBuf); }; // @v12.8.3

/*static*/const char * PPOptiModule::GetResultStatusSymb(int status)
{
	const char * p_result = 0;
	switch(status) {
		case stError: p_result = "ERROR"; break;
		case stOptimal: p_result = "OPTIMAL"; break;
		case stFeasible: p_result = "FEASIBLE"; break;
		case stInfeasible: p_result = "INFEASIBLE"; break;
		case stTimeout: p_result = "TIMEOUT"; break;
		default: p_result = "Status undefined"; break;
	}
	return p_result;
}

PPOptiModule::BaseExecutionBlock::BaseExecutionBlock(uint32 task) : Signature(0), Task(task)
{
}

bool PPOptiModule::BaseExecutionBlock::SetModuleConfig(const PPOptiModule::Config & rCfg)
{
	bool   ok = true;
	ModuleCfg.TimeLimitMSec = inirangeor(rCfg.TimeLimitMSec, 100U, 3600000U, 30000U);
	ModuleCfg.ThreadCount = inirangeor(rCfg.ThreadCount, 1U, 16U, 1U);
	return ok;
}
		
/*virtual*/int PPOptiModule::BaseExecutionBlock::SerializeV(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Signature, rBuf));
	THROW(pSCtx->Serialize(dir, Task, rBuf));
	THROW(ModuleCfg.Serialize(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::ExecutionBlock::ExecutionBlock() : BaseExecutionBlock(taskCuttingStock2D)
{
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::SetConfig(const Config & rCfg)
{
	bool   ok = true;
	TaskCfg.Flags = rCfg.Flags;
	TaskCfg.GuillotineLevels = oneof3(rCfg.GuillotineLevels, 0, 2, 3) ? rCfg.GuillotineLevels : 2;
	TaskCfg.KerfWidth = rCfg.KerfWidth;
	TaskCfg.Spacing = rCfg.Spacing;
	TaskCfg.MinUsefulWaste = rCfg.MinUsefulWaste;
	TaskCfg.Objv = rCfg.Objv;
	return ok;
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::SearchSheetId(uint64 id, uint * pIdx) const { return SheetList.lsearch(&id, pIdx, CMPF_UINT64); }
bool PPOptiModule::CuttingStock2D::ExecutionBlock::SearchBomItemId(uint64 id, uint * pIdx) const { return BomList.lsearch(&id, pIdx, CMPF_UINT64); }

bool PPOptiModule::CuttingStock2D::ExecutionBlock::GetSheetSize(uint64 sheetId, SPoint2I & rS) const
{
	bool   ok = false;
	uint   idx = 0;
	if(SearchSheetId(sheetId, &idx)) {
		rS = SheetList.at(idx).Size;
		ok = true;
	}
	else {
		rS.Z();
	}
	return ok;
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::AddSheet(const RawSheet & rSheet)
{
	bool   ok = true;
	if(!rSheet.UedID || rSheet.Size.GetMinDim() <= 0 || rSheet.Qtty <= 1.0) {
		ok = false;
	}
	else if(SearchSheetId(rSheet.UedID, 0)) {
		ok = false;
	}
	else if(!SheetList.insert(&rSheet)) {
		ok = false;
	}
	return ok;
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::AddBomItem(const BomItem & rItem)
{
	bool   ok = true;
	if(!rItem.UedID || rItem.Size.GetMinDim() <= 0 || rItem.Qtty <= 1.0) {
		ok = false;
	}
	else if(SearchBomItemId(rItem.UedID, 0)) {
		ok = false;
	}
	else if(!BomList.insert(&rItem)) {
		ok = false;
	}
	return ok;
}

/*virtual*/int PPOptiModule::CuttingStock2D::ExecutionBlock::SerializeV(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(BaseExecutionBlock::SerializeV(dir, rBuf, pSCtx));
	THROW(TaskCfg.Serialize(dir, rBuf, pSCtx));
	THROW(TSVector_Serialize(SheetList, dir, rBuf, pSCtx));
	THROW(TSVector_Serialize(BomList, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

PPOptiModule::BaseExecutionResult::BaseExecutionResult() : Signature(0), Task(0), Status(0), SolveTimeMSec(0)
{
}

/*virtual*/int PPOptiModule::BaseExecutionResult::SerializeV(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Signature, rBuf));
	THROW(pSCtx->Serialize(dir, Task, rBuf));
	THROW(pSCtx->Serialize(dir, Status, rBuf));
	THROW(pSCtx->Serialize(dir, SolveTimeMSec, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::Config::Config() : TimeLimitMSec(60000), ThreadCount(4)
{
}

int PPOptiModule::Config::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, TimeLimitMSec, rBuf));
	THROW(pSCtx->Serialize(dir, ThreadCount, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::Config::Config() : Flags(0), GuillotineLevels(0), KerfWidth(0), Spacing(0), MinUsefulWaste(0), Objv(objvUndef)
{
}

int PPOptiModule::CuttingStock2D::Config::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, GuillotineLevels, rBuf));
	THROW(pSCtx->Serialize(dir, KerfWidth, rBuf));
	THROW(pSCtx->Serialize(dir, Spacing, rBuf));
	THROW(pSCtx->Serialize(dir, MinUsefulWaste, rBuf));
	THROW(pSCtx->Serialize(dir, Objv, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::RawSheet::RawSheet() : Qtty(0), Cost(0.0)
{
}

int PPOptiModule::CuttingStock2D::RawSheet::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, UedID, rBuf));
	THROW(Size.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Qtty, rBuf));
	THROW(pSCtx->Serialize(dir, Cost, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::BomItem::BomItem() : Flags(0), Qtty(0), Cost(0.0)
{
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::GetSheetSizeListInDirection(int dir/*cuttingdirX||cuttingdirY*/, LongArray & rList) const
{
	rList.Z();
	bool   ok = false;
	if(dir == cuttingdirX) {
		for(uint i = 0; i < SheetList.getCount(); i++) {
			const  RawSheet & r_item = SheetList.at(i);
			if(r_item.Size.x > 0) {
				rList.add(r_item.Size.x);
				ok = true;
			}
		}
	}
	else if(dir == cuttingdirY) {
		for(uint i = 0; i < SheetList.getCount(); i++) {
			const  RawSheet & r_item = SheetList.at(i);
			if(r_item.Size.y > 0) {
				rList.add(r_item.Size.y);
				ok = true;
			}
		}
	}
	rList.sortAndUndup();
	return ok;
}

bool PPOptiModule::CuttingStock2D::ExecutionBlock::GetItemSizeListInDirection(int dir/*cuttingdirX*/, LongArray & rList) const
{
	rList.Z();
	bool   ok = false;
	if(dir == cuttingdirX) {
		for(uint i = 0; i < BomList.getCount(); i++) {
			const  BomItem & r_item = BomList.at(i);
			if(r_item.Size.x > 0) {
				rList.add(r_item.Size.x);
				ok = true;
			}
			if(r_item.Flags & BomItem::fCanRotate90 && r_item.Size.y > 0) {
				rList.add(r_item.Size.y);
				ok = true;
			}
		}
	}
	else if(dir == cuttingdirY) {
		for(uint i = 0; i < BomList.getCount(); i++) {
			const  BomItem & r_item = BomList.at(i);
			if(r_item.Size.y > 0) {
				rList.add(r_item.Size.y);
				ok = true;
			}
			if(r_item.Flags & BomItem::fCanRotate90 && r_item.Size.x > 0) {
				rList.add(r_item.Size.x);
				ok = true;
			}
		}
	}
	rList.sortAndUndup();
	return ok;
}

int PPOptiModule::CuttingStock2D::BomItem::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, UedID, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(Size.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Qtty, rBuf));
	THROW(pSCtx->Serialize(dir, Cost, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::Placement::Placement() : ItemInstance(0), SheetInstance(0), Flags()
{
}

int PPOptiModule::CuttingStock2D::Placement::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, ItemUedID, rBuf));
	THROW(pSCtx->Serialize(dir, ItemInstance, rBuf));
	THROW(pSCtx->Serialize(dir, SheetUedID, rBuf));
	THROW(pSCtx->Serialize(dir, SheetInstance, rBuf));
	THROW(Pt.Serialize(dir, rBuf, pSCtx));
	THROW(Sz.Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	CATCHZOK
	return ok;
}

PPOptiModule::CuttingStock2D::Result::Result() : BaseExecutionResult(), TotalSheetsUsed(0), TotalPiecesPlaced(0), TotalPiecesRequested(0), TotalWasteArea(0), TotalCost(0.0)
{
}

/*virtual*/int PPOptiModule::CuttingStock2D::Result::SerializeV(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(BaseExecutionResult::SerializeV(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, TotalSheetsUsed, rBuf));
	THROW(pSCtx->Serialize(dir, TotalPiecesPlaced, rBuf));
	THROW(pSCtx->Serialize(dir, TotalPiecesRequested, rBuf));
	THROW(pSCtx->Serialize(dir, TotalWasteArea, rBuf));
	THROW(pSCtx->Serialize(dir, TotalCost, rBuf));
	THROW(TSCollection_Serialize(SheetList, dir, rBuf, pSCtx));
	THROW(TSVector_Serialize(AllPlacements, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}
			
PPOptiModule::CuttingStock2D::Result & PPOptiModule::CuttingStock2D::Result::Z()
{
	Status = stError;
	TotalSheetsUsed = 0;
	TotalPiecesPlaced = 0;
	TotalPiecesRequested = 0;
	TotalWasteArea = 0;
	TotalCost = 0.0;
	SolveTimeMSec = 0;
	SheetList.freeAll();
	AllPlacements.clear();
	return *this;
}
			
double PPOptiModule::CuttingStock2D::Result::GetTotalUtilization() const
{
	uint total_area = 0;
	uint used_area = 0;
	for(uint i = 0; i < SheetList.getCount(); i++) {
		const UsedSheet * p_s = SheetList.at(i);
		if(p_s) {
			total_area += p_s->TotalArea;
			used_area  += p_s->UsedArea;
		}
	}
	return fdivui(used_area, total_area) * 100.0;
}

PPOptiModule::CuttingStock2D::UsedSheet::UsedSheet() : SheetInstance(0), TotalArea(0), UsedArea(0)
{
}

int PPOptiModule::CuttingStock2D::UsedSheet::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, SheetUedID, rBuf));
	THROW(pSCtx->Serialize(dir, SheetInstance, rBuf));
	THROW(pSCtx->Serialize(dir, TotalArea, rBuf));
	THROW(pSCtx->Serialize(dir, UsedArea, rBuf));
	THROW(TSVector_Serialize(ItemList, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

/*static*/void PPOptiModule::CuttingStock2D::PickColor(uint64 itemId, SString & rColor)
{
	//
	// Палитра из 16 различимых цветов (сбалансированная контрастность)
	//
	static const char * p_palette[] = {
		"#4e79a7", "#f28e2b", "#e15759", "#76b7b2",
		"#59a14f", "#edc948", "#b07aa1", "#ff9da7",
		"#9c755f", "#bab0ac", "#6b9ac4", "#d4a6c8",
		"#8cd17d", "#f1ce63", "#86bcb6", "#fabfd2"
	};
	const uint n = SIZEOFARRAY(p_palette);
	uint idx = (itemId % n);
	rColor = p_palette[idx];
}

/*static*/bool PPOptiModule::CuttingStock2D::GenerateSvg(const ExecutionBlock & rEb, const Result & rResult, double scale, SString & rSvgText)
{
	rSvgText.Z();
	bool   ok = true;
	if(!rResult.IsSuccess() || rResult.SheetList.getCount() == 0)
		ok = false;
	else {
		// --- Автомасштаб ---
		if(scale <= 0.0) {
			// Найдём максимальную ширину листа и подберём scale так, чтобы один лист был ~1000px
			int max_sheet_w = 0;
			for(uint i = 0; i < rResult.SheetList.getCount(); i++) {
				SPoint2I ssz;
				rEb.GetSheetSize(rResult.SheetList.at(i)->SheetUedID, ssz);
				SETMAX(max_sheet_w, ssz.x);
			}
			scale = (max_sheet_w > 0) ? (1000.0 / max_sheet_w) : 1.0;
		}

		// --- Вычисление размеров полотна ---
		const int gap_px      = 20;    // расстояние между листами
		const int header_px   = 50;    // высота заголовка над каждым листом
		const int margin_px   = 20;    // внешний отступ
		const int footer_px   = 40;    // высота итогового блока
		const int max_per_row = 2;     // листов в ряду

		int row_count = (static_cast<int>(rResult.SheetList.getCount()) + max_per_row - 1) / max_per_row;

		// Для простоты полагаем, что в одном ряду листы одинаковой высоты.
		// Полная ширина = max_per_row * (max_sheet_w*scale + gap)
		// Полная высота = sum_по_рядам(max_sheet_h_в_ряду * scale + header + gap)
		int total_w = margin_px * 2;
		int total_h = margin_px * 2 + footer_px;
		{
			int row_w = 0;
			int row_h = 0;
			int in_row = 0;
			int max_row_w = 0;
			for(uint i = 0; i < rResult.SheetList.getCount(); i++) {
				SPoint2I ssz;
				rEb.GetSheetSize(rResult.SheetList.at(i)->SheetUedID, ssz);
				const int w_px = static_cast<int>(ssz.x * scale) + gap_px;
				const int h_px = static_cast<int>(ssz.y * scale) + header_px + gap_px;
				row_w += w_px;
				SETMAX(row_h, h_px);
				in_row++;
				if(in_row >= max_per_row) {
					if(row_w > max_row_w)
						max_row_w = row_w;
					total_h += row_h;
					row_w = 0;
					row_h = 0;
					in_row = 0;
				}
			}
			if(in_row > 0) {
				if(row_w > max_row_w)
					max_row_w = row_w;
				total_h += row_h;
			}
			total_w += max_row_w;
		}

		// --- Заголовок SVG ---
		SString & r_svg = rSvgText;
		SString   temp_buf;

		r_svg.Cat("<?xml version=\"1.0\" encoding=\"UTF-8\"?>").CR();
		r_svg.Cat("<svg xmlns=\"http://www.w3.org/2000/svg\" ")
			.Cat("width=\"").Cat(total_w).Cat("\" ")
			.Cat("height=\"").Cat(total_h).Cat("\" ")
			.Cat("viewBox=\"0 0 ").Cat(total_w).Space().Cat(total_h).Cat("\">").CR();
		// --- Стили ---
		r_svg.Cat("<defs><style type=\"text/css\"><![CDATA[").CR()
			.Cat("  .sheet { fill: #fafafa; stroke: #333; stroke-width: 2; }").CR()
			.Cat("  .piece { stroke: #222; stroke-width: 1; opacity: 0.88; }").CR()
			.Cat("  .label { font-family: Arial, sans-serif; font-size: 11px; fill: #fff; ")
			.Cat("text-anchor: middle; dominant-baseline: central; pointer-events: none; }").CR()
			.Cat("  .label-dark { fill: #222; }").CR()
			.Cat("  .title { font-family: Arial, sans-serif; font-size: 13px; font-weight: bold; fill: #222; }").CR()
			.Cat("  .subtitle { font-family: Arial, sans-serif; font-size: 10px; fill: #666; }").CR()
			.Cat("  .footer { font-family: Arial, sans-serif; font-size: 12px; fill: #222; }").CR()
			.Cat("  .rot-mark { fill: #ff2020; font-family: Arial, sans-serif; font-size: 10px; font-weight: bold; }").CR()
			.Cat("]]></style></defs>").CR();

		// --- Размещение листов на полотне ---
		int cur_x = margin_px;
		int cur_y = margin_px;
		int in_row = 0;
		int row_max_h = 0;

		for(uint i = 0; i < rResult.SheetList.getCount(); i++) {
			const UsedSheet * p_us = rResult.SheetList.at(i);
			SPoint2I ssz;
			rEb.GetSheetSize(p_us->SheetUedID, ssz);
			const int sheet_w_px = static_cast<int>(ssz.x * scale);
			const int sheet_h_px = static_cast<int>(ssz.y * scale);
			// --- Заголовок листа ---
			r_svg.Cat("<text class=\"title\" x=\"").Cat(cur_x).Cat("\" y=\"").Cat(cur_y + 14).Cat("\">")
				.Cat("Sheet ").Cat(p_us->SheetUedID).Cat(".").Cat(static_cast<uint>(p_us->SheetInstance))
				.Cat(" (").Cat(ssz.x).Cat("x").Cat(ssz.y).Cat(")</text>").CR();
        
			temp_buf.Z().Cat(p_us->GetUtilization(), MKSFMTD(0, 1, 0));
			r_svg.Cat("<text class=\"subtitle\" x=\"").Cat(cur_x).Cat("\" y=\"").Cat(cur_y + 30).Cat("\">")
				.Cat("Pieces: ").Cat(static_cast<uint>(p_us->ItemList.getCount()))
				.Cat("  Util: ").Cat(temp_buf).Cat("%")
				.Cat("  Used: ").Cat(p_us->UsedArea).Cat("/").Cat(p_us->TotalArea).Cat("</text>").CR();

			// --- Прямоугольник листа ---
			const int sheet_x = cur_x;
			const int sheet_y = cur_y + header_px;
			r_svg.Cat("<rect class=\"sheet\" x=\"").Cat(sheet_x).Cat("\" y=\"").Cat(sheet_y).Cat("\" ")
				.Cat("width=\"").Cat(sheet_w_px).Cat("\" height=\"").Cat(sheet_h_px).Cat("\"/>").CR();

			// --- Детали ---
			// Координаты в задаче: (0,0) - левый НИЖНИЙ угол листа. 
			// В SVG: (0,0) - левый ВЕРХНИЙ. Нужна инверсия Y.
			for(uint j = 0; j < p_us->ItemList.getCount(); j++) {
				const Placement & r_pl = p_us->ItemList.at(j);
				const int pw_px = static_cast<int>(r_pl.Sz.x * scale);
				const int ph_px = static_cast<int>(r_pl.Sz.y * scale);
				// Инверсия Y: svg_y = sheet_top + (sheet_h - piece_y - piece_h)
				const int px = sheet_x + static_cast<int>(r_pl.Pt.x * scale);
				const int py = sheet_y + sheet_h_px - static_cast<int>((r_pl.Pt.y + r_pl.Sz.y) * scale);

				SString color;
				PickColor(r_pl.ItemUedID, color);

				r_svg.Cat("<rect class=\"piece\" x=\"").Cat(px).Cat("\" y=\"").Cat(py).Cat("\" ")
					.Cat("width=\"").Cat(pw_px).Cat("\" height=\"").Cat(ph_px).Cat("\" ")
					.Cat("fill=\"").Cat(color).Cat("\">")
					.Cat("<title>Item ").Cat(r_pl.ItemUedID).Cat(" [").Cat(static_cast<uint>(r_pl.ItemInstance)).Cat("]")
					.Cat("  size=").Cat(r_pl.Sz.x).Cat("x").Cat(r_pl.Sz.y)
					.Cat("  pos=(").Cat(r_pl.Pt.x).Cat(",").Cat(r_pl.Pt.y).Cat(")")
					.Cat(r_pl.IsRotated() ? "  [ROTATED]" : "")
					.Cat("</title></rect>").CR();

				// --- Подпись детали ---
				if(pw_px >= 30 && ph_px >= 15) {
					const int cx = px + pw_px / 2;
					const int cy = py + ph_px / 2;
					r_svg.Cat("<text class=\"label\" x=\"").Cat(cx).Cat("\" y=\"").Cat(cy).Cat("\">").Cat(r_pl.ItemUedID);
					if(r_pl.IsRotated())
						r_svg.Cat("&#8635;");  // символ "↻" (rotate)
					r_svg.Cat("</text>").CR();
					// Размеры деталью мелким шрифтом (если места хватает)
					if(ph_px >= 40) {
						r_svg.Cat("<text class=\"label\" style=\"font-size:9px;opacity:0.8\" ")
							.Cat("x=\"").Cat(cx).Cat("\" y=\"").Cat(cy + 14).Cat("\">")
							.Cat(r_pl.Sz.x).Cat("x").Cat(r_pl.Sz.y).Cat("</text>").CR();
					}
				}
			}

			// --- Переход к следующему листу ---
			cur_x += sheet_w_px + gap_px;
			in_row++;
			const int this_row_h = sheet_h_px + header_px + gap_px;
			SETMAX(row_max_h, this_row_h);
			if(in_row >= max_per_row) {
				cur_x = margin_px;
				cur_y += row_max_h;
				in_row = 0;
				row_max_h = 0;
			}
		}

		// --- Итоговый блок внизу ---
		const int foot_y = total_h - margin_px - 20;
		temp_buf.Z().Cat(rResult.GetTotalUtilization(), MKSFMTD(0, 1, 0));
		r_svg.Cat("<text class=\"footer\" x=\"").Cat(margin_px).Cat("\" y=\"").Cat(foot_y).Cat("\">")
			.Cat("Sheets: ").Cat(rResult.TotalSheetsUsed)
			.Cat("  Pieces: ").Cat(rResult.TotalPiecesPlaced).Cat("/").Cat(rResult.TotalPiecesRequested)
			.Cat("  Utilization: ").Cat(temp_buf).Cat("%")
			.Cat("  Waste: ").Cat(rResult.TotalWasteArea);
		if(rResult.TotalCost > 0.0) {
			temp_buf.Z().Cat(rResult.TotalCost, MKSFMTD(0, 2, 0));
			r_svg.Cat("  Cost: ").Cat(temp_buf);
		}
		r_svg.Cat("  Time: ").Cat(rResult.SolveTimeMSec).Cat("ms").Cat("</text>").CR();
		r_svg.Cat("</svg>").CR();
	}
	return ok;
}

/*static*/int PPOptiModule::CuttingStock2D::MakeInitialTestCase(ExecutionBlock & rEb)
{
	int    ok = 1;
	{
		PPOptiModule::Config mod_cfg;
		mod_cfg.TimeLimitMSec = 60000;
		mod_cfg.ThreadCount   = 8;
		THROW(rEb.SetModuleConfig(mod_cfg));
	}
	{
		PPOptiModule::CuttingStock2D::Config cfg;
		cfg.Flags = 0|PPOptiModule::CuttingStock2D::Config::fGuillotineCut|PPOptiModule::CuttingStock2D::Config::fTexturedMaterial;
		cfg.GuillotineLevels = 3;
		cfg.KerfWidth        = 4;
		cfg.Objv = PPOptiModule::CuttingStock2D::objvMinWaste;
		THROW(rEb.SetConfig(cfg));
	}
	// Листы ДСП
	{
		PPOptiModule::CuttingStock2D::RawSheet sh;
		sh.UedID = 1;
		sh.Size.Set(2440, 1220);
		sh.Qtty = 5;
		sh.Cost = 1500.0;
		THROW(rEb.AddSheet(sh));
	}
	{
		PPOptiModule::CuttingStock2D::RawSheet sh;
		sh.UedID = 2;
		sh.Size.Set(1830, 1220);
		sh.Qtty = 3;
		sh.Cost = 1100.0;
		THROW(rEb.AddSheet(sh));
	}
	// Детали
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 1; 
		bi.Size.Set(600, 400); 
		bi.Qtty = 4;
		THROW(rEb.AddBomItem(bi));
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 2; 
		bi.Size.Set(800, 500); 
		bi.Qtty = 2;
		THROW(rEb.AddBomItem(bi));
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 3; 
		bi.Size.Set(300, 200); 
		bi.Qtty = 8;
		THROW(rEb.AddBomItem(bi));
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 4; 
		bi.Size.Set(1000, 600); 
		bi.Qtty = 2;
		THROW(rEb.AddBomItem(bi));
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 5; 
		bi.Size.Set(450, 350); 
		bi.Qtty = 6;
		THROW(rEb.AddBomItem(bi));
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.UedID = 6; 
		bi.Size.Set(700, 300); 
		bi.Qtty = 3;
		THROW(rEb.AddBomItem(bi));
	}
	CATCHZOK
	return ok;
}
//
//
//
CcFiscalCorrection::CcFiscalCorrection() : AmtCash(0.0), AmtBank(0.0), AmtPrepay(0.0), AmtPostpay(0.0), AmtReckon(0.0),
	AmtVat22(0.0), AmtVat20(0.0), AmtVat18(0.0), AmtVat10(0.0), AmtVat07(0.0), AmtVat05(0.0), AmtVat00(0.0), AmtNoVat(0.0), VatRate(0.0), Dt(ZERODATE), Flags(0)
{
}

ChZnPmReplyTags::ChZnPmReplyTags() : ReqTimestamp(0LL)
{
}

ChZnPmReplyTags & ChZnPmReplyTags::Z()
{
	ReqTimestamp = 0;
	ReqId.Z();
	LocalModuleInstance.Z();
	LocalModuleDbVer.Z();
	return *this;
}
