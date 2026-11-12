// ORTOOLS_SUPPORT.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
// Модуль, экранирующий работу с OR-TOOLS. Взаимодействует с материнской системой посредством named-pipes.
//
#define SL_USE_STD_MEMFUNC
#include <pp.h>
#pragma hdrstop
#define PROTOBUF_USE_DLLS
#ifndef OR_PROTO_DLL
	#define OR_PROTO_DLL __declspec(dllimport)
#endif
#include <sl-packing-set_compiler_default.h>
#include <ortools/sat/cp_model.h>
#include <iostream>

using namespace operations_research;
using namespace operations_research::sat;

PPOptiModule::CuttingStock2D::CuttingStock2D()
{
}
		
bool PPOptiModule::CuttingStock2D::SetConfig(const PPOptiModule::CuttingStock2D::Config & rCfg) 
{ 
	return EB.SetConfig(rCfg);
}
		
bool PPOptiModule::CuttingStock2D::SetModuleConfig(const PPOptiModule::Config & rCfg) { return EB.SetModuleConfig(rCfg); }
bool PPOptiModule::CuttingStock2D::AddSheet(const PPOptiModule::CuttingStock2D::RawSheet & rSheet) { return EB.AddSheet(rSheet); }
bool PPOptiModule::CuttingStock2D::AddBomItem(const PPOptiModule::CuttingStock2D::BomItem & rItem) { return EB.AddBomItem(rItem); }
		
PPOptiModule::CuttingStock2D & PPOptiModule::CuttingStock2D::Z()
{
	EB.SheetList.clear();
	EB.BomList.clear();
	EB.TaskCfg = Config();
	return *this;
}
//
// Descr: Внутренний класс, реализующий взаимодействие с OR-Tools
//
class OrToolsSolver_CuttingStock2D {
	PPOptiModule::CuttingStock2D & R_Base;
public:
	OrToolsSolver_CuttingStock2D(PPOptiModule::CuttingStock2D & rBase) : R_Base(rBase)
	{
	}
	OrToolsSolver_CuttingStock2D & Z()
	{
		PieceInst.clear();
		SheetInst.clear();
		return *this;
	}
	//
	// Проверка гильотинности (постобработка)
	//
	static bool IsGuillotineLayout(const TSVector <PPOptiModule::CuttingStock2D::Placement> & rPlacements, int sheetW, int sheetH, int maxLevels);
	static bool GuillotineCheck(const std::vector <SRect2I> & pieces, const SRect2I & area, int level, int maxLevels);
	//
	// Внутренние развёрнутые экземпляры деталей (pieces) и сырьевых плоскостей (sheets)
	//
	struct BaseInstance {
		BaseInstance() : RefIdx(0), OriginalId(0), Instance(0), Cost(0.0)
		{
		}
		int    RefIdx;       // Индекс в массиве оригинальных исходных данных
		int    OriginalId;   // ИД в массиве оригинальных исходных данных
		uint   Instance;     // 0..Qtty-1
		SPoint2I Size;
		double Cost;
	};
	//
	// Переменные CP-SAT для одного потенциального размещения
	//
	struct PlacementVars {
		int    PieceInstIdx; // Индекс детали
		int    SheetInstIdx; // Индекс листа
		BoolVar Assigned;    // true если деталь PieceInstIdx вырезается из листа SheetInstIdx
		IntVar XStart;       
		IntVar YStart;
		IntVar XEnd;
		IntVar YEnd;
		IntVar ActualWidth;
		IntVar ActualHeight;
		BoolVar Rotated;
		IntervalVar XIval;
		IntervalVar YIval;
	};

	// Развёрнутые экземпляры (заполняются перед решением)
	std::vector <BaseInstance> PieceInst;
	std::vector <BaseInstance> SheetInst;
	//
	void ExpandInstances();
	bool SolveInternal_(PPOptiModule::CuttingStock2D::Result & rR);
	bool SolveFreePlacementInternal(PPOptiModule::CuttingStock2D::Result & rR);
	void ExtractResult(const CpSolverResponse& response, const std::vector<std::vector<PlacementVars>>& pvars, const std::vector<BoolVar>& sheet_used, uint solveMs, 
		PPOptiModule::CuttingStock2D::Result & rR);
};

/*static*/bool OrToolsSolver_CuttingStock2D::IsGuillotineLayout(const TSVector <PPOptiModule::CuttingStock2D::Placement> & rPlacements, int sheetW, int sheetH, int maxLevels)
{
	bool   ok = true;
	if(rPlacements.getCount() > 1) {
		std::vector <SRect2I> rects;
		rects.reserve(rPlacements.getCount());
		for(uint i = 0; i < rPlacements.getCount(); i++) {
			const PPOptiModule::CuttingStock2D::Placement & pl = rPlacements.at(i);
			rects.push_back(SRect2I(pl.Pt.x, pl.Pt.y, pl.Pt.x + pl.Sz.x, pl.Pt.y + pl.Sz.y));
		}
		SRect2I area(0, 0, sheetW, sheetH);
		ok = GuillotineCheck(rects, area, 0, maxLevels);
	}
	return ok;
}

/*static*/bool OrToolsSolver_CuttingStock2D::GuillotineCheck(const std::vector<SRect2I> & pieces, const SRect2I & area, int level, int maxLevels)
{
	if(pieces.size() <= 1)
		return true;
	if(maxLevels > 0 && level >= maxLevels)
		return false;
	// Вертикальные разрезы: кандидаты — правые границы деталей
	{
		std::set <int> cands;
		for(auto & p : pieces) {
			if(p.RL.x > area.LU.x && p.RL.x < area.RL.x)
				cands.insert(p.RL.x);
		}
		for(int xc : cands) {
			bool crosses = false;
			for(auto& p : pieces) {
				if(p.LU.x < xc && p.RL.x > xc) { 
					crosses = true; 
					break; 
				}
			}
			if(crosses)
				continue;
			std::vector <SRect2I> left, right;
			for(auto & p : pieces) {
				if(p.RL.x <= xc)      
					left.push_back(p);
				else if(p.LU.x >= xc) 
					right.push_back(p);
			}
			if(!left.empty() && !right.empty()) {
				SRect2I la(area.LU.x, area.LU.y, xc, area.RL.y);
				SRect2I ra(xc, area.LU.y, area.RL.x, area.RL.y);
				if(GuillotineCheck(left, la, level+1, maxLevels) && GuillotineCheck(right, ra, level+1, maxLevels)) { // @recursion
					return true;
				}
			}
		}
	}
	// Горизонтальные разрезы: кандидаты — верхние границы деталей
	{
		std::set<int> cands;
		for(auto & p : pieces) {
			if(p.RL.y > area.LU.y && p.RL.y < area.RL.y)
				cands.insert(p.RL.y);
		}
		for(int yc : cands) {
			bool crosses = false;
			for(auto& p : pieces) {
				if(p.LU.y < yc && p.RL.y > yc) { 
					crosses = true; 
					break; 
				}
			}
			if(crosses)
				continue;
			std::vector <SRect2I> bottom, top;
			for(auto& p : pieces) {
				if(p.RL.y <= yc)      
					bottom.push_back(p);
				else if(p.LU.y >= yc) 
					top.push_back(p);
			}
			if(!bottom.empty() && !top.empty()) {
				SRect2I ba(area.LU.x, area.LU.y, area.RL.x, yc);
				SRect2I ta(area.LU.x, yc, area.RL.x, area.RL.y);
				if(GuillotineCheck(bottom, ba, level+1, maxLevels) && GuillotineCheck(top, ta, level+1, maxLevels)) { // @recursion
					return true;
				}
			}
		}
	}
	return false;
}

void OrToolsSolver_CuttingStock2D::ExpandInstances()
{
	PieceInst.clear();
	SheetInst.clear();
	for(uint i = 0; i < R_Base.EB.BomList.getCount(); i++) {
		const PPOptiModule::CuttingStock2D::BomItem & bi = R_Base.EB.BomList.at(i);
		for(uint q = 0; q < bi.Qtty; q++) {
			BaseInstance pi;
			pi.RefIdx = static_cast<int>(i);
			pi.Instance = q;
			pi.Size = bi.Size;
			pi.OriginalId = bi.Id;
			PieceInst.push_back(pi);
		}
	}
	for(uint i = 0; i < R_Base.EB.SheetList.getCount(); i++) {
		const PPOptiModule::CuttingStock2D::RawSheet & rs = R_Base.EB.SheetList.at(i);
		for(uint q = 0; q < rs.Qtty; q++) {
			BaseInstance si;
			si.RefIdx = static_cast<int>(i);
			si.Instance = q;
			si.Size = rs.Size;
			si.Cost = rs.Cost;
			si.OriginalId = rs.Id;
			SheetInst.push_back(si);
		}
	}
}

bool OrToolsSolver_CuttingStock2D::SolveInternal_(PPOptiModule::CuttingStock2D::Result & rR)
{
    rR.Z();
    ExpandInstances();
    rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
    if(PieceInst.empty()) {
        rR.Status = PPOptiModule::stFeasible;
        return true;
    }
    else if(SheetInst.empty()) {
        rR.Status = PPOptiModule::stInfeasible;
        return false;
    }
	else {
		auto t_start = std::chrono::steady_clock::now();
		const int piece_count = static_cast<int>(PieceInst.size());
		const int sheet_count = static_cast<int>(SheetInst.size());
		const int kerf = static_cast<int>(R_Base.EB.TaskCfg.KerfWidth);
		const int spacing = static_cast<int>(R_Base.EB.TaskCfg.Spacing);
		const int extra = kerf + spacing;
		const bool allow_rotation = !(R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fTexturedMaterial);
		const bool guillotine     = LOGIC(R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fGuillotineCut);
		const bool allow_partial  = LOGIC(R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fAllowPartial);
		CpModelBuilder model;
		//
		// Определяем максимальное число полос на одном листе.
		// Для двухуровневого гильотинного раскроя: лист делится на 
		// горизонтальные полосы (1-й рез), внутри полосы детали идут 
		// последовательно слева направо (2-й рез).
		//
		// Найдём минимальную высоту детали — это нижняя граница высоты полосы
		int minPieceH = INT_MAX;
		int maxPieceH = 0;
		for(int p = 0; p < piece_count; p++) {
			int h = PieceInst[p].Size.y;
			int w = PieceInst[p].Size.x;
			int minh = allow_rotation ? smin(w, h) : h;
			int maxh = allow_rotation ? smax(w, h) : h;
			SETMIN(minPieceH, minh);
			SETMAX(maxPieceH, maxh);
		}
		// Максимальная высота листа
		int maxSheetH = 0;
		int maxSheetW = 0;
		for(int s = 0; s < sheet_count; s++) {
			SETMAX(maxSheetH, SheetInst[s].Size.y);
			SETMAX(maxSheetW, SheetInst[s].Size.x);
		}
		// Максимальное число полос на листе
		const int maxStrips = (minPieceH > 0) ? (maxSheetH / minPieceH) : piece_count;
		// Ограничим разумным числом
		const int maxStripsPerSheet = smin(maxStrips, smin(piece_count, 32));
		//
		// Переменные
		//
		// sheet_used[s] — используется ли лист s
		std::vector<BoolVar> sheet_used(sheet_count);
		for(int s = 0; s < sheet_count; s++) {
			sheet_used[s] = model.NewBoolVar();
		}
		// Для каждого листа и каждой полосы:
		struct StripVars {
			BoolVar active;       // используется ли эта полоса
			IntVar  yStart;       // начало полосы по Y
			IntVar  height;       // высота полосы (определяется самой высокой деталью + kerf)
			IntVar  yEnd;         // конец полосы по Y
			IntVar  usedWidth;    // использованная ширина полосы (для компактности)
		};
		// Для каждого потенциального размещения детали:
		struct PieceOnStrip {
			int    pieceIdx;
			int    sheetIdx;
			int    stripIdx;
			BoolVar assigned;     // деталь pieceIdx назначена на полосу stripIdx листа sheetIdx
			BoolVar rotated;
			IntVar  actualW;
			IntVar  actualH;
			IntVar  xPos;         // позиция по X внутри полосы (вычисляется из порядка)
		};

		std::vector <std::vector <StripVars>> strips(sheet_count); // strips[s][k] — полоса k листа s
		std::vector <std::vector <std::vector<PieceOnStrip>>> pieceOnStrip(sheet_count); // pieceOnStrip[s][k] — детали, которые могут быть на полосе k листа s
		std::vector <std::vector <BoolVar>> allAssignments(piece_count); // assignment[p] — все BoolVar для детали p (по всем листам и полосам)
		//
		// Для хранения данных, необходимых ExtractResult
		// Мы будем собирать PlacementVars-совместимую структуру
		std::vector <std::vector <PlacementVars>> pvars(sheet_count);
		for(int s = 0; s < sheet_count; s++) {
			const BaseInstance & r_si = SheetInst[s];
			const int sheetW = r_si.Size.x;
			const int sheetH = r_si.Size.y;
			const int num_strips_here = smax(smin(maxStripsPerSheet, sheetH / (minPieceH > 0 ? minPieceH : 1)), 1);
			strips[s].resize(num_strips_here);
			pieceOnStrip[s].resize(num_strips_here);
			for(int k = 0; k < num_strips_here; k++) {
				StripVars & sv = strips[s][k];
				sv.active = model.NewBoolVar();
				sv.yStart = model.NewIntVar(Domain(0, sheetH));
				sv.height = model.NewIntVar(Domain(0, sheetH));
				sv.yEnd   = model.NewIntVar(Domain(0, sheetH));
				sv.usedWidth = model.NewIntVar(Domain(0, sheetW));
            
				// yEnd = yStart + height
				model.AddEquality(sv.yEnd, LinearExpr(sv.yStart) + LinearExpr(sv.height));
            
				// Полоса не выходит за лист
				model.AddLessOrEqual(sv.yEnd, sheetH);
            
				// Если полоса неактивна, height = 0
				model.AddEquality(sv.height, 0).OnlyEnforceIf(sv.active.Not());
				model.AddGreaterThan(sv.height, 0).OnlyEnforceIf(sv.active);
            
				// Если полоса неактивна, usedWidth = 0
				model.AddEquality(sv.usedWidth, 0).OnlyEnforceIf(sv.active.Not());
			}
        
			// Полосы упорядочены по Y и не перекрываются
			for(int k = 0; (k + 1) < num_strips_here; k++) {
				// yEnd[k] + kerf <= yStart[k+1]  (если обе активны)
				// Упрощённо: yEnd[k] <= yStart[k+1] (kerf учтём в высоте полосы)
				model.AddLessOrEqual(strips[s][k].yEnd, strips[s][k+1].yStart);
            
				// Symmetry breaking: полосы используются по порядку
				model.AddLessOrEqual(strips[s][k+1].active, strips[s][k].active);
			}
        
			// Связь sheet_used[s] с активностью полос
			{
				std::vector <BoolVar> stripActives;
				for(int k = 0; k < num_strips_here; k++)
					stripActives.push_back(strips[s][k].active);
				for(auto & a : stripActives)
					model.AddGreaterOrEqual(sheet_used[s], a);
				model.AddLessOrEqual(sheet_used[s], LinearExpr::Sum(stripActives));
			}
        
			// --- Детали на полосах ---
			for(int k = 0; k < num_strips_here; k++) {
				pieceOnStrip[s][k].reserve(piece_count);
            
				// Собираем все назначения деталей на эту полосу
				std::vector <BoolVar> assignedHere;
				std::vector <IntVar>  widthsHere;  // actual_w + extra для каждой детали
				std::vector <IntVar>  heightsHere; // actual_h для каждой детали
            
				for(int p = 0; p < piece_count; p++) {
					const BaseInstance & r_pi = PieceInst[p];
					// Проверяем, может ли деталь в принципе поместиться на этом листе
					const bool fitNormal  = (r_pi.Size.x <= sheetW && r_pi.Size.y <= sheetH);
					const bool fitRotated = allow_rotation && (r_pi.Size.y <= sheetW && r_pi.Size.x <= sheetH);
					if(fitNormal || fitRotated) {
						PieceOnStrip pos;
						pos.pieceIdx = p;
						pos.sheetIdx = s;
						pos.stripIdx = k;
						pos.assigned = model.NewBoolVar();
                
						allAssignments[p].push_back(pos.assigned);
						assignedHere.push_back(pos.assigned);
						// Поворот
						if(allow_rotation && !r_pi.Size.IsSquare() && fitNormal && fitRotated) {
							pos.rotated = model.NewBoolVar();
						}
						else if(!fitNormal && fitRotated) {
							pos.rotated = model.TrueVar();
						}
						else {
							pos.rotated = model.FalseVar();
						}
						// Фактические размеры
						const int maxDim = r_pi.Size.GetMaxDim();
						const int minDim = r_pi.Size.GetMinDim();
						if(r_pi.Size.IsSquare() || !allow_rotation) {
							pos.actualW = model.NewConstant(r_pi.Size.x);
							pos.actualH = model.NewConstant(r_pi.Size.y);
						}
						else {
							pos.actualW = model.NewIntVar(Domain(minDim, maxDim));
							pos.actualH = model.NewIntVar(Domain(minDim, maxDim));
							model.AddEquality(pos.actualW, r_pi.Size.x).OnlyEnforceIf(pos.rotated.Not());
							model.AddEquality(pos.actualH, r_pi.Size.y).OnlyEnforceIf(pos.rotated.Not());
							model.AddEquality(pos.actualW, r_pi.Size.y).OnlyEnforceIf(pos.rotated);
							model.AddEquality(pos.actualH, r_pi.Size.x).OnlyEnforceIf(pos.rotated);
						}
						// Высота детали <= высоте полосы (если деталь назначена на полосу)
						model.AddLessOrEqual(pos.actualH, strips[s][k].height).OnlyEnforceIf(pos.assigned);
						// X-позиция будет определяться ниже
						pos.xPos = model.NewIntVar(Domain(0, sheetW));
						widthsHere.push_back(pos.actualW);
						heightsHere.push_back(pos.actualH);
						pieceOnStrip[s][k].push_back(pos);
					}
				}
				// Если нет деталей на полосе — продолжаем
				if(assignedHere.empty())
					continue;
				// Связь: если ни одна деталь не назначена, полоса неактивна
				model.AddLessOrEqual(strips[s][k].active, LinearExpr::Sum(assignedHere));
				for(auto & a : assignedHere) {
					model.AddGreaterOrEqual(strips[s][k].active, a);
				}
				//
				// ГИЛЬОТИННЫЙ РЕЗ: детали внутри полосы не перекрываются по X.
				// Используем 1D NoOverlap (интервалы по X).
				// Это гарантирует, что внутри полосы детали можно отрезать 
				// вертикальными резами.
				//
				if(guillotine) {
					// 1D NoOverlap по X внутри полосы
					std::vector<IntervalVar> xIntervals;
					for(uint pidx = 0; pidx < pieceOnStrip[s][k].size(); pidx++) {
						PieceOnStrip & pos = pieceOnStrip[s][k][pidx];
						IntVar xSize;
						if(extra > 0) {
							xSize = model.NewIntVar(Domain(0, sheetW + extra));
							model.AddEquality(xSize, LinearExpr(pos.actualW) + extra).OnlyEnforceIf(pos.assigned);
							model.AddEquality(xSize, 0).OnlyEnforceIf(pos.assigned.Not());
						}
						else {
							xSize = pos.actualW;
						}
						IntVar xEnd = model.NewIntVar(Domain(0, sheetW + extra));
						IntervalVar xIval = model.NewOptionalIntervalVar(pos.xPos, xSize, xEnd, pos.assigned);
						xIntervals.push_back(xIval);
						// Деталь не выходит за ширину листа
						IntVar realEnd = model.NewIntVar(Domain(0, sheetW));
						model.AddEquality(realEnd, LinearExpr(pos.xPos) + LinearExpr(pos.actualW));
						model.AddLessOrEqual(realEnd, sheetW).OnlyEnforceIf(pos.assigned);
					}
                
					/*NoOverlapConstraint*/Constraint no1d = model.AddNoOverlap(xIntervals);
					(void)no1d; // suppress unused warning
                
					// Высота полосы = max(actualH) среди назначенных деталей + kerf
					// Это обеспечивает, что полоса "обтягивает" самую высокую деталь
					for(uint pidx = 0; pidx < pieceOnStrip[s][k].size(); pidx++) {
						PieceOnStrip & pos = pieceOnStrip[s][k][pidx];
						// height >= actualH + kerf (если деталь назначена)
						if(kerf > 0) {
							model.AddGreaterOrEqual(strips[s][k].height, LinearExpr(pos.actualH) + kerf).OnlyEnforceIf(pos.assigned);
						}
						else {
							model.AddGreaterOrEqual(strips[s][k].height, pos.actualH).OnlyEnforceIf(pos.assigned);
						}
					}
                
					// Суммарная ширина (для компактности)
					{
						LinearExpr sumW;
						for(uint pidx = 0; pidx < pieceOnStrip[s][k].size(); pidx++) {
							PieceOnStrip & pos = pieceOnStrip[s][k][pidx];
							// Создаём переменную: ширина * assigned
							IntVar contrib = model.NewIntVar(Domain(0, sheetW));
							model.AddEquality(contrib, pos.actualW).OnlyEnforceIf(pos.assigned);
							model.AddEquality(contrib, 0).OnlyEnforceIf(pos.assigned.Not());
							sumW += LinearExpr(contrib);
						}
						if(extra > 0) {
							// Учитываем kerf между деталями
							IntVar num_assigned = model.NewIntVar(Domain(0, piece_count));
							model.AddEquality(num_assigned, LinearExpr::Sum(assignedHere));
							// usedWidth >= sumW + (num_assigned - 1) * extra
							// Упрощённо: usedWidth >= sumW (для целевой функции)
						}
						model.AddGreaterOrEqual(strips[s][k].usedWidth, sumW);
					}
				}
				else {
					//
					// Без гильотины: используем 2D NoOverlap как раньше, но внутри полосы
					//
					std::vector<IntervalVar> xIntervals, yIntervals;
					for(uint pidx = 0; pidx < pieceOnStrip[s][k].size(); pidx++) {
						PieceOnStrip & pos = pieceOnStrip[s][k][pidx];
						IntVar xSize = pos.actualW;
						IntVar ySize = pos.actualH;
						IntVar xEnd = model.NewIntVar(Domain(0, sheetW));
						IntVar yEnd = model.NewIntVar(Domain(0, sheetH));
						model.AddEquality(xEnd, LinearExpr(pos.xPos) + LinearExpr(xSize));
						model.AddLessOrEqual(xEnd, sheetW).OnlyEnforceIf(pos.assigned);
						// Y внутри полосы: от 0 до height
						IntVar yPos = model.NewIntVar(Domain(0, sheetH));
						model.AddEquality(yEnd, LinearExpr(yPos) + LinearExpr(ySize));
						model.AddLessOrEqual(yEnd, strips[s][k].height).OnlyEnforceIf(pos.assigned);
						IntervalVar xIval = model.NewOptionalIntervalVar(pos.xPos, xSize, xEnd, pos.assigned);
						IntervalVar yIval = model.NewOptionalIntervalVar(yPos, ySize, yEnd, pos.assigned);
						xIntervals.push_back(xIval);
						yIntervals.push_back(yIval);
					}
					if(!xIntervals.empty()) {
						NoOverlap2DConstraint no2d = model.AddNoOverlap2D();
						for(uint idx = 0; idx < xIntervals.size(); idx++) {
							no2d.AddRectangle(xIntervals[idx], yIntervals[idx]);
						}
					}
				}
			} // for strips
		} // for sheets
		//
		// Каждая деталь — ровно на один лист/полосу
		//
		for(int p = 0; p < piece_count; p++) {
			if(allAssignments[p].empty()) {
				if(!allow_partial) {
					// Деталь не помещается ни на один лист => infeasible
					rR.Status = PPOptiModule::stInfeasible;
					return false;
				}
				continue;
			}
			if(allow_partial)
				model.AddAtMostOne(allAssignments[p]);
			else
				model.AddExactlyOne(allAssignments[p]);
		}
		//
		// Symmetry breaking: одинаковые листы по порядку
		//
		for(int s = 1; s < sheet_count; s++) {
			if(SheetInst[s].RefIdx == SheetInst[s-1].RefIdx) {
				model.AddLessOrEqual(sheet_used[s], sheet_used[s-1]);
			}
		}
		//
		// Целевая функция
		//
		// Основная цель
		const int objv = NZOR(R_Base.EB.TaskCfg.Objv, PPOptiModule::CuttingStock2D::objvMinWaste);
		// Для компактности: добавляем вторичную цель — минимизировать 
		// суммарную "использованную площадь полос" (height * sheetW)
		// Это стимулирует минимизацию высот полос и, как следствие,
		// оставляет большой цельный остаток внизу/наверху листа.
    
		// Масштабирующий коэффициент: основная цель должна доминировать
		const int64_t PRIMARY_WEIGHT = 1000000LL;
		const int64_t COMPACT_WEIGHT = 1LL;
		LinearExpr objective;
		switch(objv) {
			case PPOptiModule::CuttingStock2D::objvMinSheets:
				objective = LinearExpr::Sum(sheet_used) * PRIMARY_WEIGHT;
				break;
			case PPOptiModule::CuttingStock2D::objvMinCost:
				{
					LinearExpr costExpr;
					for(int s = 0; s < sheet_count; s++) {
						int costInt = static_cast<int>(SheetInst[s].Cost * 1000);
						costExpr += LinearExpr(sheet_used[s]) * costInt;
					}
					objective = costExpr * PRIMARY_WEIGHT;
				}
				break;
			case PPOptiModule::CuttingStock2D::objvMinWaste:
			default:
				{
					LinearExpr areaExpr;
					for(int s = 0; s < sheet_count; s++) {
						int area = SheetInst[s].Size.x * SheetInst[s].Size.y;
						areaExpr += LinearExpr(sheet_used[s]) * area;
					}
					objective = areaExpr * PRIMARY_WEIGHT;
				}
				break;
		}
		//
		// Вторичная цель: минимизировать суммарную высоту полос (стимулирует компактность по вертикали)
		//
		{
			LinearExpr compactExpr;
			for(int s = 0; s < sheet_count; s++) {
				for(uint k = 0; k < strips[s].size(); k++) {
					// Минимизируем высоту каждой полосы * ширину листа
					// Это эффективно минимизирует "занятую площадь"
					compactExpr += LinearExpr(strips[s][k].height) * SheetInst[s].Size.x;
				}
			}
			objective += compactExpr * COMPACT_WEIGHT;
		}
		//
		// Третичная цель: минимизировать разброс Y-координат (стимулирует прижатие полос к началу листа)
		//
		{
			LinearExpr yExpr;
			for(int s = 0; s < sheet_count; s++) {
				for(uint k = 0; k < strips[s].size(); k++) {
					yExpr += strips[s][k].yEnd;
				}
			}
			// Ещё меньший вес
			// objective += yExpr;  // можно раскомментировать при необходимости
		}
		model.Minimize(objective);
		//
		// Параметры солвера
		//
		SatParameters satParams;
		satParams.set_max_time_in_seconds(static_cast<double>(R_Base.EB.ModuleCfg.TimeLimitMSec) / 1000.0);
		satParams.set_num_workers(static_cast<int>(R_Base.EB.ModuleCfg.ThreadCount));
		satParams.set_log_search_progress(true);
		satParams.set_linearization_level(2);
		// Подсказки для поиска: попробовать сначала более "жадную" стратегию
		// satParams.set_search_branching(SatParameters::FIXED_SEARCH);
		//
		// Решение
		//
		CpSolverResponse response = SolveWithParameters(model.Build(), satParams);
		auto t_end = std::chrono::steady_clock::now();
		uint solveMs = static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
		//
		// Извлечение результата
		//
		rR.SolveTimeMSec = solveMs;
		auto cpStatus = response.status();
		if(cpStatus == CpSolverStatus::INFEASIBLE) {
			rR.Status = PPOptiModule::stInfeasible;
		}
		else if(oneof2(cpStatus, CpSolverStatus::OPTIMAL, CpSolverStatus::FEASIBLE)) {
			rR.Status = (cpStatus == CpSolverStatus::OPTIMAL) ? PPOptiModule::stOptimal : PPOptiModule::stFeasible;
			for(int s = 0; s < sheet_count; s++) {
				if(SolutionBooleanValue(response, sheet_used[s])) {
					PPOptiModule::CuttingStock2D::UsedSheet * p_us = rR.SheetList.CreateNewItem();
					if(p_us) {
						p_us->SheetId = SheetInst[s].OriginalId;
						p_us->SheetInstance  = SheetInst[s].Instance;
						p_us->TotalArea = static_cast<uint>(SheetInst[s].Size.GetArea());
						p_us->UsedArea  = 0;
						for(uint k = 0; k < strips[s].size(); k++) {
							if(SolutionBooleanValue(response, strips[s][k].active)) {
								int stripY = static_cast<int>(SolutionIntegerValue(response, strips[s][k].yStart));
								int stripH = static_cast<int>(SolutionIntegerValue(response, strips[s][k].height));
								for(const auto & pos : pieceOnStrip[s][k]) {
									if(SolutionBooleanValue(response, pos.assigned)) {
										PPOptiModule::CuttingStock2D::Placement pl;
										pl.ItemId       = PieceInst[pos.pieceIdx].OriginalId;
										pl.ItemInstance  = PieceInst[pos.pieceIdx].Instance;
										pl.SheetId      = SheetInst[s].OriginalId;
										pl.SheetInstance = SheetInst[s].Instance;
                    
										int aw = static_cast<int>(SolutionIntegerValue(response, pos.actualW));
										int ah = static_cast<int>(SolutionIntegerValue(response, pos.actualH));
										int xp = static_cast<int>(SolutionIntegerValue(response, pos.xPos));
                    
										pl.Pt.Set(xp, stripY);
										pl.Sz.Set(aw, ah);
										pl.SetRotated(SolutionBooleanValue(response, pos.rotated));
                    
										p_us->UsedArea += static_cast<uint>(aw * ah);
										p_us->ItemList.insert(&pl);
										rR.AllPlacements.insert(&pl);
									}
								}
							}
						}
						//rR.Sheets.insert(&us);
						rR.TotalCost += SheetInst[s].Cost;
					}
				}
			}
			rR.TotalSheetsUsed   = static_cast<int>(rR.SheetList.getCount());
			rR.TotalPiecesPlaced = static_cast<int>(rR.AllPlacements.getCount());
			uint totalArea = 0;
			uint usedArea = 0;
			for(uint i = 0; i < rR.SheetList.getCount(); i++) {
				const PPOptiModule::CuttingStock2D::UsedSheet * p_s = rR.SheetList.at(i);
				if(p_s) {
					totalArea += p_s->TotalArea;
					usedArea  += p_s->UsedArea;
				}
			}
			rR.TotalWasteArea = totalArea - usedArea;
		}
		else {
			rR.Status = PPOptiModule::stTimeout;
		}
		return rR.IsSuccess();
	}
}

bool OrToolsSolver_CuttingStock2D::SolveFreePlacementInternal(PPOptiModule::CuttingStock2D::Result & rR)
{
	rR.Z();
	bool   ok = false;
	ExpandInstances();
	rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
	if(PieceInst.empty()) {
		rR.Status = PPOptiModule::stFeasible;
	}
	else if(SheetInst.empty()) {
		rR.Status = PPOptiModule::stInfeasible;
	}
	else {
		auto t_start = std::chrono::steady_clock::now();
		const int piece_count = static_cast<int>(PieceInst.size());
		const int sheet_count = static_cast<int>(SheetInst.size());
		const int extra = static_cast<int>(R_Base.EB.TaskCfg.KerfWidth + R_Base.EB.TaskCfg.Spacing);
		const bool allow_rotation = !LOGIC(R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fTexturedMaterial);
		// (это ограничение отрабатывается в SolveInternal_) const bool guillotine     = LOGIC(Cfg.Flags & Config::fGuillotineCut);
		const bool allow_partial  = LOGIC(R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fAllowPartial);

		CpModelBuilder model;

		// --- Булевы переменные: используется ли лист s ---
		std::vector <BoolVar> sheet_used(sheet_count);
		for(int s = 0; s < sheet_count; s++) {
			sheet_used[s] = model.NewBoolVar();
		}
		// --- Переменные размещения ---
		// pvars[s] — вектор PlacementVars для деталей, потенциально назначенных на лист s
		std::vector <std::vector <PlacementVars>> pvars(sheet_count);

		// assignment[p][s] — BoolVar: деталь p назначена на лист s
		std::vector <std::vector <BoolVar>> assignment(piece_count);
		for(int p = 0; p < piece_count; p++) {
			const BaseInstance & r_pi = PieceInst[p];
			assignment[p].resize(sheet_count);
			for(int s = 0; s < sheet_count; s++) {
				const BaseInstance & r_si = SheetInst[s];
				const bool fitNormal  = (r_pi.Size.x <= r_si.Size.x && r_pi.Size.y <= r_si.Size.y);
				const bool fitRotated = allow_rotation && (r_pi.Size.y <= r_si.Size.x && r_pi.Size.x <= r_si.Size.y);
				if(!fitNormal && !fitRotated) {
					assignment[p][s] = model.FalseVar();
				}
				else {
					PlacementVars pv;
					pv.PieceInstIdx = p;
					pv.SheetInstIdx = s;
					pv.Assigned = model.NewBoolVar();
					assignment[p][s] = pv.Assigned;
					// --- Поворот ---
					if(allow_rotation && !r_pi.Size.IsSquare() && fitNormal && fitRotated) {
						pv.Rotated = model.NewBoolVar();
					}
					else if(!fitNormal && fitRotated) {
						// Помещается только повёрнутым
						pv.Rotated = model.TrueVar();
					}
					else {
						// Без поворота (текстура, квадрат, или не помещается повёрнутым)
						pv.Rotated = model.FalseVar();
					}
					// --- Фактические размеры ---
					const int max_dim = r_pi.Size.GetMaxDim();
					const int min_dim = r_pi.Size.GetMinDim();
					if(r_pi.Size.IsSquare() || !allow_rotation) {
						pv.ActualWidth  = model.NewConstant(r_pi.Size.x);
						pv.ActualHeight = model.NewConstant(r_pi.Size.y);
					}
					else {
						pv.ActualWidth  = model.NewIntVar(Domain(min_dim, max_dim));
						pv.ActualHeight = model.NewIntVar(Domain(min_dim, max_dim));
						// rotated=0 => W=pi.Width,  H=pi.Height
						// rotated=1 => W=pi.Height, H=pi.Width
						model.AddEquality(pv.ActualWidth,  r_pi.Size.x).OnlyEnforceIf(pv.Rotated.Not());
						model.AddEquality(pv.ActualHeight, r_pi.Size.y).OnlyEnforceIf(pv.Rotated.Not());
						model.AddEquality(pv.ActualWidth,  r_pi.Size.y).OnlyEnforceIf(pv.Rotated);
						model.AddEquality(pv.ActualHeight, r_pi.Size.x).OnlyEnforceIf(pv.Rotated);
					}
					// --- Координаты ---
					pv.XStart = model.NewIntVar(Domain(0, r_si.Size.x));
					pv.YStart = model.NewIntVar(Domain(0, r_si.Size.y));
					pv.XEnd   = model.NewIntVar(Domain(0, r_si.Size.x));
					pv.YEnd   = model.NewIntVar(Domain(0, r_si.Size.y));
					// x_end = x_start + actual_width
					// y_end = y_start + actual_height
					model.AddEquality(pv.XEnd, LinearExpr(pv.XStart) + LinearExpr(pv.ActualWidth));
					model.AddEquality(pv.YEnd, LinearExpr(pv.YStart) + LinearExpr(pv.ActualHeight));
					// Деталь не выходит за границы листа
					model.AddLessOrEqual(pv.XEnd, r_si.Size.x).OnlyEnforceIf(pv.Assigned);
					model.AddLessOrEqual(pv.YEnd, r_si.Size.y).OnlyEnforceIf(pv.Assigned);
					// --- Интервалы для NoOverlap2D ---
					// Размер интервала = actual_dim + extra (kerf + spacing)
					// чтобы NoOverlap2D обеспечивал зазор между деталями
					IntVar xSize, ySize;
					if(extra > 0) {
						xSize = model.NewIntVar(Domain(min_dim + extra, max_dim + extra));
						ySize = model.NewIntVar(Domain(min_dim + extra, max_dim + extra));
						model.AddEquality(xSize, LinearExpr(pv.ActualWidth) + extra);
						model.AddEquality(ySize, LinearExpr(pv.ActualHeight) + extra);
					}
					else {
						xSize = pv.ActualWidth;
						ySize = pv.ActualHeight;
					}
					IntVar xIvalEnd = model.NewIntVar(Domain(0, r_si.Size.x + extra));
					IntVar yIvalEnd = model.NewIntVar(Domain(0, r_si.Size.y + extra));
					pv.XIval = model.NewOptionalIntervalVar(pv.XStart, xSize, xIvalEnd, pv.Assigned);
					pv.YIval = model.NewOptionalIntervalVar(pv.YStart, ySize, yIvalEnd, pv.Assigned);
					pvars[s].push_back(pv);
				}
			}
			// Каждая деталь — ровно на один лист (или не более одного)
			if(allow_partial)
				model.AddAtMostOne(assignment[p]);
			else
				model.AddExactlyOne(assignment[p]);
		}
		// --- NoOverlap2D для каждого листа ---
		for(int s = 0; s < sheet_count; s++) {
			if(!pvars[s].empty()) {
				NoOverlap2DConstraint no2d = model.AddNoOverlap2D();
				for(auto & pv : pvars[s])
					no2d.AddRectangle(pv.XIval, pv.YIval);
				// Связь: sheet_used[s] = OR(assigned для всех деталей на s)
				std::vector<BoolVar> on_sheet;
				on_sheet.reserve(pvars[s].size());
				for(auto & pv : pvars[s])
					on_sheet.push_back(pv.Assigned);
				// sheet_used >= each
				for(auto & b : on_sheet)
					model.AddGreaterOrEqual(sheet_used[s], b);
				// sheet_used <= sum
				model.AddLessOrEqual(sheet_used[s], LinearExpr::Sum(on_sheet));
			}
		}
		/* (это ограничение отрабатывается в SolveInternal_)
		// --- Гильотинные ограничения (попарные, необходимое условие) ---
		if(guillotine) {
			for(int s = 0; s < sheet_count; s++) {
				const int n = static_cast<int>(pvars[s].size());
				for(int i = 0; i < n; i++) {
					for(int j = i + 1; j < n; j++) {
						// Обе детали присутствуют на листе
						BoolVar both = model.NewBoolVar();
						model.AddBoolAnd({pvars[s][i].Assigned, pvars[s][j].Assigned}).OnlyEnforceIf(both);
						model.AddBoolOr({pvars[s][i].Assigned.Not(), pvars[s][j].Assigned.Not()}).OnlyEnforceIf(both.Not());
						// 4 варианта взаимного расположения
						BoolVar iLeftJ  = model.NewBoolVar();
						BoolVar iRightJ = model.NewBoolVar();
						BoolVar iBelowJ = model.NewBoolVar();
						BoolVar iAboveJ = model.NewBoolVar();
						model.AddBoolOr({iLeftJ, iRightJ, iBelowJ, iAboveJ}).OnlyEnforceIf(both);
						model.AddLessOrEqual(pvars[s][i].XEnd, pvars[s][j].XStart).OnlyEnforceIf(iLeftJ);
						model.AddLessOrEqual(pvars[s][j].XEnd, pvars[s][i].XStart).OnlyEnforceIf(iRightJ);
						model.AddLessOrEqual(pvars[s][i].YEnd, pvars[s][j].YStart).OnlyEnforceIf(iBelowJ);
						model.AddLessOrEqual(pvars[s][j].YEnd, pvars[s][i].YStart).OnlyEnforceIf(iAboveJ);
					}
				}
			}
		}*/
		// --- Symmetry breaking: одинаковые листы использовать по порядку ---
		for(int s = 1; s < sheet_count; s++) {
			if(SheetInst[s].RefIdx == SheetInst[s-1].RefIdx) {
				// Если лист s-1 не используется, то и лист s не используется
				model.AddLessOrEqual(sheet_used[s], sheet_used[s-1]);
			}
		}
		// --- Symmetry breaking: одинаковые детали — лексикографический порядок ---
		// Одинаковые экземпляры одной BomItem: упорядочить по (sheet, x, y)
		{
			std::map <int, std::vector<int>> bomGroups; // BomIdx -> [pieceInstIdx...]
			for(int p = 0; p < piece_count; p++) {
				bomGroups[PieceInst[p].RefIdx].push_back(p);
			}
			for(auto & [bomIdx, group] : bomGroups) {
				if(group.size() > 1) {
					// Для каждой пары последовательных экземпляров:
					// назначенный лист[i] <= назначенный лист[i+1]
					// (упрощённое symmetry breaking)
					// Создаём вспомогательную переменную — индекс листа для каждого экземпляра
					std::vector<IntVar> sheet_idx;
					for(int p : group) {
						const IntVar si = model.NewIntVar(Domain(0, sheet_count));
						sheet_idx.push_back(si);
						// si = sum(s * assignment[p][s])
						LinearExpr expr;
						for(int s = 0; s < sheet_count; s++) {
							expr += LinearExpr(assignment[p][s]) * s;
						}
						// Если allow_partial и деталь не размещена, si = sheet_count
						if(allow_partial) {
							const BoolVar placed = model.NewBoolVar();
							model.AddGreaterOrEqual(LinearExpr::Sum(assignment[p]), 1).OnlyEnforceIf(placed);
							model.AddEquality(LinearExpr::Sum(assignment[p]), 0).OnlyEnforceIf(placed.Not());
							// Если не размещена: si = sheet_count
							model.AddEquality(si, sheet_count).OnlyEnforceIf(placed.Not());
							model.AddEquality(si, expr).OnlyEnforceIf(placed);
						}
						else {
							model.AddEquality(si, expr);
						}
					}
					for(uint k = 0; k + 1 < group.size(); k++) {
						model.AddLessOrEqual(sheet_idx[k], sheet_idx[k + 1]);
					}
				}
			}
		}
		// --- Целевая функция ---
		int objv = R_Base.EB.TaskCfg.Objv;
		if(objv == PPOptiModule::CuttingStock2D::objvUndef)
			objv = PPOptiModule::CuttingStock2D::objvMinWaste;
		switch(objv) {
			case PPOptiModule::CuttingStock2D::objvMinSheets:
				model.Minimize(LinearExpr::Sum(sheet_used));
				break;
			case PPOptiModule::CuttingStock2D::objvMinCost:
				{
					LinearExpr cost_expr;
					for(int s = 0; s < sheet_count; s++) {
						const  int cost_int = static_cast<int>(SheetInst[s].Cost * 1000);
						cost_expr += LinearExpr(sheet_used[s]) * cost_int;
					}
					model.Minimize(cost_expr);
				}
				break;
			case PPOptiModule::CuttingStock2D::objvMinWaste:
			default:
				{
					// Минимизация суммарной площади используемых листов (площадь деталей фиксирована => эквивалентно минимизации отходов)
					LinearExpr area_expr;
					for(int s = 0; s < sheet_count; s++) {
						area_expr += LinearExpr(sheet_used[s]) * SheetInst[s].Size.GetArea();
					}
					model.Minimize(area_expr);
				}
				break;
		}

		// --- Параметры солвера ---
		SatParameters sat_params;
		sat_params.set_max_time_in_seconds(static_cast<double>(R_Base.EB.ModuleCfg.TimeLimitMSec) / 1000.0);
		sat_params.set_num_workers(static_cast<int>(R_Base.EB.ModuleCfg.ThreadCount));
		sat_params.set_log_search_progress(true);
		sat_params.set_linearization_level(2);
		// --- Решение ---
		CpSolverResponse response = SolveWithParameters(model.Build(), sat_params);
		auto t_end = std::chrono::steady_clock::now();
		uint solveMs = static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
		// --- Извлечение результата ---
		ExtractResult(response, pvars, sheet_used, solveMs, rR);
	}
	ok = rR.IsSuccess();
	return ok;
}
		
void OrToolsSolver_CuttingStock2D::ExtractResult(const CpSolverResponse & response, 
	const std::vector<std::vector<PlacementVars>> & pvars, const std::vector<BoolVar> & rSheetUsed, uint solveMs, PPOptiModule::CuttingStock2D::Result & rR)
{
	rR.SolveTimeMSec = solveMs;
	rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
	auto cpStatus = response.status();
	if(cpStatus == CpSolverStatus::INFEASIBLE) {
		rR.Status = PPOptiModule::stInfeasible;
		//return result;
	}
	else if(oneof2(cpStatus, CpSolverStatus::OPTIMAL, CpSolverStatus::FEASIBLE)) {
		if(cpStatus == CpSolverStatus::OPTIMAL)
			rR.Status = PPOptiModule::stOptimal;
		else if(cpStatus == CpSolverStatus::FEASIBLE)
			rR.Status = PPOptiModule::stFeasible;
		const int sheet_count = static_cast<int>(SheetInst.size());
		// Собрать использованные листы
		for(int s = 0; s < sheet_count; s++) {
			if(SolutionBooleanValue(response, rSheetUsed[s])) {
				PPOptiModule::CuttingStock2D::UsedSheet * p_us = rR.SheetList.CreateNewItem();
				if(p_us) {
					p_us->SheetId = SheetInst[s].OriginalId;
					p_us->SheetInstance = SheetInst[s].Instance;
					p_us->TotalArea = static_cast<uint>(SheetInst[s].Size.x * SheetInst[s].Size.y);
					p_us->UsedArea = 0;
					for(const auto & pv : pvars[s]) {
						if(SolutionBooleanValue(response, pv.Assigned)) {
							PPOptiModule::CuttingStock2D::Placement pl;
							pl.ItemId       = PieceInst[pv.PieceInstIdx].OriginalId;
							pl.ItemInstance  = PieceInst[pv.PieceInstIdx].Instance;
							pl.SheetId      = SheetInst[s].OriginalId;
							pl.SheetInstance = SheetInst[s].Instance;
							pl.Pt.Set(static_cast<int>(SolutionIntegerValue(response, pv.XStart)), static_cast<int>(SolutionIntegerValue(response, pv.YStart)));
							pl.Sz.Set(static_cast<int>(SolutionIntegerValue(response, pv.ActualWidth)), static_cast<int>(SolutionIntegerValue(response, pv.ActualHeight)));
							pl.SetRotated(SolutionBooleanValue(response, pv.Rotated));
							p_us->UsedArea += static_cast<uint>(pl.Sz.x * pl.Sz.y);
							p_us->ItemList.insert(&pl);
							rR.AllPlacements.insert(&pl);
						}
					}
					rR.TotalCost += SheetInst[s].Cost;
				}
			}
		}
		rR.TotalSheetsUsed  = static_cast<int>(rR.SheetList.getCount());
		rR.TotalPiecesPlaced = static_cast<int>(rR.AllPlacements.getCount());
		// Подсчёт отходов
		uint totalArea = 0;
		uint usedArea = 0;
		for(uint i = 0; i < rR.SheetList.getCount(); i++) {
			const PPOptiModule::CuttingStock2D::UsedSheet * p_us = rR.SheetList.at(i);
			if(p_us) {
				totalArea += p_us->TotalArea;
				usedArea  += p_us->UsedArea;
			}
		}
		rR.TotalWasteArea = totalArea - usedArea;
		// Постпроверка гильотинности
		if((R_Base.EB.TaskCfg.Flags & PPOptiModule::CuttingStock2D::Config::fGuillotineCut) && rR.IsSuccess()) {
			for(uint i = 0; i < rR.SheetList.getCount(); i++) {
				const PPOptiModule::CuttingStock2D::UsedSheet * p_us = rR.SheetList.at(i);
				if(p_us) {
					// Найти размеры листа
					//int sw = 0;
					//int sh = 0;
					SPoint2I found_sz;
					for(uint j = 0; j < R_Base.EB.SheetList.getCount(); j++) {
						if(R_Base.EB.SheetList.at(j).Id == p_us->SheetId) {
							//sw = R_Base.SheetList.at(j).Size.x;
							//sh = R_Base.SheetList.at(j).Size.y;
							found_sz = R_Base.EB.SheetList.at(j).Size;
							break;
						}
					}
					//if(sw > 0 && sh > 0) {
					if(found_sz.x > 0 && found_sz.y > 0) {
						/*bool ok =*/ //IsGuillotineLayout(p_us->ItemList, found_sz.x, found_sz.y, static_cast<int>(R_Base.Cfg.GuillotineLevels));
						// @todo При необходимости пометить лист, если проверка не пройдена
					}
				}
			}
		}
		//return result;
	}
	else {
		rR.Status = PPOptiModule::stTimeout;
		//return result;
	}
}

bool PPOptiModule::CuttingStock2D::Solve(const ExecutionBlock & rEb, Result & rR)
{
	bool   ok = false;
	EB = rEb;
	OrToolsSolver_CuttingStock2D s_(*this);
	if(EB.TaskCfg.Flags & Config::fGuillotineCut) {
		ok = s_.SolveInternal_(rR);
	}
	else {
		ok = s_.SolveFreePlacementInternal(rR);
	}
	return ok;
}

bool PPOptiModule::CuttingStock2D::Solve(PPOptiModule::CuttingStock2D::Result & rR)
{
	bool   ok = false;
	OrToolsSolver_CuttingStock2D s_(*this);
	if(EB.TaskCfg.Flags & Config::fGuillotineCut) {
		ok = s_.SolveInternal_(rR);
	}
	else {
		ok = s_.SolveFreePlacementInternal(rR);
	}
	return ok;
}

void PPOptiModule::CuttingStock2D::GetSheetSize(int sheetId, int & rW, int & rH) const
{
	rW = 0;
	rH = 0;
	for(uint i = 0; i < EB.SheetList.getCount(); i++) {
		const PPOptiModule::CuttingStock2D::RawSheet & r_s = EB.SheetList.at(i);
		if(r_s.Id == sheetId) {
			rW = r_s.Size.x;
			rH = r_s.Size.y;
			break;
		}
	}
}

bool PPOptiModule::CuttingStock2D::GenerateSvg(const PPOptiModule::CuttingStock2D::Result & rResult, double scale, SString & rSvgText) const
{
	return PPOptiModule::CuttingStock2D::GenerateSvg(EB, rResult, scale, rSvgText);
}

void PPOptiModule::CuttingStock2D::Result::ToStr_Debug(SString & rBuf) const
{
	rBuf.Z();
	rBuf.Cat("Status").CatDiv(':', 2).Cat(GetResultStatusSymb(Status)).CR();
	rBuf.Cat("Solve time").CatDiv(':', 2).Cat(SolveTimeMSec).Cat("ms").CR();
	rBuf.Cat("Sheets used").CatDiv(':', 2).Cat(TotalSheetsUsed).CR();
	rBuf.Cat("Pieces placed").CatDiv(':', 2).Cat(TotalPiecesPlaced).CatChar('/').Cat(TotalPiecesRequested).CR();
	rBuf.Cat("Utilization").CatDiv(':', 2).Cat(GetTotalUtilization(), MKSFMTD(0, 1, 0)).CatChar('%').CR();
	rBuf.Cat("Waste area").CatDiv(':', 2).Cat(TotalWasteArea).CR();
	rBuf.Cat("Total cost").CatDiv(':', 2).Cat(TotalCost).CR();
	for(uint i = 0; i < SheetList.getCount(); i++) {
		const UsedSheet * p_us = SheetList.at(i);
		rBuf.Tab().Cat("Sheet").Space().CatChar('#').Cat(p_us->SheetId).Space().CatChar('(').CatEq("inst", p_us->SheetInstance).CatChar(')').CatDiv(':', 2).
			Cat("utilization").Eq().Cat(p_us->GetUtilization(), MKSFMTD(0, 1, 0)).CatChar('%').CR();
		for(uint j = 0; j < p_us->ItemList.getCount(); j++) {
			const Placement & r_pl = p_us->ItemList.at(j);
			rBuf.Tab_(2).Cat("Item").Space().CatChar('#').Cat(r_pl.ItemId).CatChar('[').Cat(r_pl.ItemInstance).CatChar(']').CatDiv(':', 2).
				Cat("pos").Eq().CatChar('(').Cat(r_pl.Pt.x).Comma().Cat(r_pl.Pt.y).CatChar(')').Space().
				Cat("size").Eq().Cat(r_pl.Sz.x).CatChar('x').Cat(r_pl.Sz.y);
			if(r_pl.IsRotated()) {
				rBuf.Space().Cat("[rotated]");
			}
			rBuf.CR();
		}
	}
}

/*static*/int PPOptiModule::CuttingStock2D::InitialTestCase()
{
	int    ok = 0;
	ExecutionBlock eb;
	if(MakeInitialTestCase(eb)) {
		PPOptiModule::CuttingStock2D cs;
		cs.EB = eb;
		PPOptiModule::CuttingStock2D::Result result;
		// Вывод результатов (для отладки)
		if(cs.Solve(result)) {
			assert(result.IsSuccess());
			SString out_buf;
			/*
			printf("Status: %s\n", (result.Status == PPOptiModule::stOptimal) ? "OPTIMAL" : "FEASIBLE");
			printf("Solve time: %u ms\n", result.SolveTimeMSec);
			printf("Sheets used: %d\n", result.TotalSheetsUsed);
			printf("Pieces placed: %d / %d\n", result.TotalPiecesPlaced, result.TotalPiecesRequested);
			printf("Utilization: %.1f%%\n", result.GetTotalUtilization());
			printf("Waste area: %u\n", result.TotalWasteArea);
			printf("Total cost: %.2f\n", result.TotalCost);
			for(uint i = 0; i < result.SheetList.getCount(); i++) {
				const UsedSheet * p_us = result.SheetList.at(i);
				printf("\n  Sheet %d (inst %u): util=%.1f%%\n", p_us->SheetId, p_us->SheetInstance, p_us->GetUtilization());
				for(uint j = 0; j < p_us->ItemList.getCount(); j++) {
					const Placement & r_pl = p_us->ItemList.at(j);
					printf("    Item %d[%u]: pos=(%d,%d) size=%dx%d%s\n",
						r_pl.ItemId, r_pl.ItemInstance, r_pl.Pt.x, r_pl.Pt.y, r_pl.Sz.x, r_pl.Sz.y, r_pl.IsRotated() ? " [ROT]" : "");
				}
			}
			*/
			result.ToStr_Debug(out_buf);
			puts(out_buf.cptr());
			{
				SString svg_out;
				cs.GenerateSvg(result, 1.0, svg_out);
				if(svg_out.NotEmpty()) {
					const char * p_file_name = "PPOptiModule_CuttingStock2D_InitialTestCase.svg";
					SFile f_out(p_file_name, SFile::mWrite);
					if(f_out.IsValid()) {
						f_out.WriteLine(svg_out);
					}
				}
			}
			ok = 1;
		}
		else {
			assert(!result.IsSuccess());
			printf("Solve failed. Status: %d\n", result.Status);
		}
	}
	return ok;
}

static int foo() 
{
	// 1. Инициализируем модель
	CpModelBuilder cp_model;
	// 2. Создаем целочисленные переменные (от 0 до 10)
	const IntVar x = cp_model.NewIntVar(Domain(0, 10));
	const IntVar y = cp_model.NewIntVar(Domain(0, 10));
	const IntVar z = cp_model.NewIntVar(Domain(0, 10));
	// 3. Добавляем ограничение: x + y + z <= 10
	cp_model.AddLessOrEqual(x + y + z, 10);
	// 4. Задаем целевую функцию: Максимизировать (x + 2*y + 3*z)
	cp_model.Maximize(x + 2 * y + 3 * z);
	// 5. Запускаем решатель
	const CpSolverResponse response = Solve(cp_model.Build());
	// 6. Обрабатываем результат
	if(response.status() == CpSolverStatus::OPTIMAL || response.status() == CpSolverStatus::FEASIBLE) {
		std::cout << "=== Решение найдено ===" << std::endl;
		std::cout << "x = " << SolutionIntegerValue(response, x) << std::endl;
		std::cout << "y = " << SolutionIntegerValue(response, y) << std::endl;
		std::cout << "z = " << SolutionIntegerValue(response, z) << std::endl;
		std::cout << "Максимальное значение функции = " << response.objective_value() << std::endl;
	} 
	else {
		std::cout << "Решение не найдено. Статус: " << static_cast<int>(response.status()) << std::endl;
	}
	return 0;
}
//
// Descr: Класс сервера, коммуницирующего с клиентом по именованному каналу (NamedPipe)
//
class PipeServer_OrToolsSupport : public SlThread_WithStartupSignal {
	static constexpr uint32 BufSize = SKILOBYTE(128);
	SString ExternalPipeName;
public:
	PipeServer_OrToolsSupport(const char * pExternalPipeName) : SlThread_WithStartupSignal(), ExternalPipeName(pExternalPipeName)
	{
	}
	~PipeServer_OrToolsSupport()
	{
	}
	virtual void Run()
	{
		class PipeSession : public SlThread {
			SIntHandle H_Pipe;
		public:
			PipeSession(SIntHandle hPipe) : SlThread(), H_Pipe(hPipe)
			{
			}
			~PipeSession()
			{
			}
			virtual void Run()
			{
				if(!!H_Pipe) {
					SString msg_buf;
					STempBuffer rd_buf(BufSize);
					STempBuffer wr_buf(BufSize);
					bool   do_exit = false;
					bool   is_pipe_ended = false;
					SString temp_buf;
					SString cmd_buf;
					SSerializeContext sctx;
					{
						do {
							DWORD rd_size = 0;
							const boolint rd_ok = ::ReadFile(H_Pipe, rd_buf, rd_buf.GetSize32(), &rd_size, NULL/*not overlapped I/O*/);
							if(rd_ok) {
								temp_buf.Z().CatN(rd_buf.cptr(), rd_size);
								//printf(msg_buf.Z().Cat("Q").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
								SJson * p_js_query = SJson::Parse(temp_buf);
								if(p_js_query) {
									bool   do_reply = true;
									bool   do_ret_sbuf = false; // если true то клиенту возвращается буфер сериализации результата sbuf
									bool   rep_status_ok = true;
									SBuffer sbuf;
									SString rep_info_text;
#if 1 // {
									//int    pr_call_state = 0; // 0 - CrystalReportPrint2_Server не была вызвана; 
										// 1 - была вызвана функция CrystalReportPrint2_Server; 2 - вызов перенаправлен для предпросмотра
									//CrystalReportPrintReply pr_reply;
									//
									cmd_buf.Z();
									const SJson * p_c = 0;
									p_c = p_js_query->FindChildByKey("cmd");
									long cmd_id = 0;
									if(SJson::IsString(p_c)) {
										(cmd_buf = p_c->Text).Unescape();
									}
									else if(SJson::IsNumber(p_c)) {
										cmd_buf = p_c->Text;
										cmd_id = cmd_buf.ToLong();
									}
									if(cmd_buf.IsEqiAscii("ping") || cmd_id == PPOptiModule::taskPingIPC) {
										rep_info_text = "pong";
									}
									else if(cmd_id == PPOptiModule::taskCuttingStock2D) {
										SSerializeContext sctx;
										p_c = p_js_query->FindChildByKey("param");
										if(p_c) {
											(temp_buf = p_c->Text).Unescape();
											const bool quit_after = SJson::IsTrue(p_js_query->FindChildByKey("quitafter"));
											sbuf.Z();
											if(sbuf.AppendMime64(temp_buf)) {
												PPOptiModule::CuttingStock2D::ExecutionBlock eb;
												if(eb.SerializeV(-1, sbuf, &sctx)) {
													{
														PPOptiModule::CuttingStock2D cs;
														PPOptiModule::CuttingStock2D::Result result;
														// Вывод результатов (для отладки)
														if(cs.Solve(eb, result)) {
															assert(result.IsSuccess());
															SString out_buf;
															result.ToStr_Debug(out_buf);
															puts(out_buf.cptr());
															{
																SString svg_out;
																cs.GenerateSvg(result, 1.0, svg_out);
																if(svg_out.NotEmpty()) {
																	const char * p_file_name = "PPOptiModule_CuttingStock2D_InitialTestCase.svg";
																	SFile f_out(p_file_name, SFile::mWrite);
																	if(f_out.IsValid()) {
																		f_out.WriteLine(svg_out);
																	}
																}
															}
														}
														else {
															assert(!result.IsSuccess());
															printf("Solve failed. Status: %d\n", result.Status);
														}
														sbuf.Z();
														result.SerializeV(+1, sbuf, &sctx);
														do_ret_sbuf = true;
													}
												}
											}
											else {
												; // @todo @err
												//js_reply.InsertString("status", "fail");
												rep_status_ok = false;
											}
										}
										else {
											//js_reply.InsertString("status", "fail");
											rep_status_ok = false;
										}
									}
									else {
										//js_reply.InsertString("status", "ok");
										(rep_info_text = "I have got your message").CatDiv(':', 2).Cat(cmd_buf);
										//js_reply.InsertString("info", temp_buf);
									}
									// do make reply
									//memcpy(wr_buf.cptr()
									if(do_reply) {
										SJson js_reply(SJson::tOBJECT);
										js_reply.InsertString("status", rep_status_ok ? "ok" : "fail");
										if(rep_info_text.NotEmpty()) {
											js_reply.InsertString("info", rep_info_text);
										}
										if(do_ret_sbuf) {
											temp_buf.Z().EncodeMime64(sbuf.constptr(), sbuf.GetAvailableSize());
											js_reply.InsertString("result", temp_buf);
										}
										/*if(pr_call_state == 1) {
											if(pr_reply.Code != 0) {
												SJson * p_js_pr_reply = pr_reply.ToJsonObj();
												if(p_js_pr_reply)
													js_reply.Insert("err", p_js_pr_reply);
											}
										}
										else if(pr_call_state == 2) {
											if(rep_info_text.IsEmpty()) {
												js_reply.InsertString("info", "blahblah");
											}
										}*/
										js_reply.ToStr(temp_buf);
										temp_buf.CopyTo(wr_buf, wr_buf.GetSize());
										DWORD reply_size = temp_buf.Len32()+1;
										DWORD wr_size = 0;
										boolint wr_ok = ::WriteFile(H_Pipe, wr_buf, reply_size, &wr_size, NULL/*not overlapped I/O*/);
										if(wr_ok) {
											//printf(msg_buf.Z().Cat("R").CatDiv(':', 2).Cat(temp_buf).CR().cptr()); // @debug
											;
										}
										else {
										}
									}
									if(cmd_buf.IsEqiAscii("quit")) {
										do_exit = true;
									}
#endif // } 0
								}
							}
							else {
								if(GetLastError() == ERROR_BROKEN_PIPE) {
									is_pipe_ended = true;
									do_exit = true;
								}
								else {
									;
								}
							}
						} while(!do_exit);
					}
					if(!is_pipe_ended) {
						FlushFileBuffers(H_Pipe);
						DisconnectNamedPipe(H_Pipe);
						CloseHandle(H_Pipe);
					}
					H_Pipe.Z();
				}
			}
		};

		uint _call_count = 0;
		DWORD last_werr = 0;
		SString pipe_name;
		if(ExternalPipeName.NotEmpty())
			pipe_name = ExternalPipeName;
		else
			GetORToolsProxiPipeName(pipe_name);
		for(bool do_exit = false; !do_exit;) {
			SIntHandle h_pipe = ::CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX,
				PIPE_TYPE_MESSAGE /*message type pipe*/|PIPE_READMODE_MESSAGE/*message-read mode*/|PIPE_WAIT/*blocking mode*/,PIPE_UNLIMITED_INSTANCES/*max. instances*/,
				BufSize /*output buffer size*/, BufSize /*input buffer size*/, 0 /*client time-out*/, NULL /*default security attribute*/);
			if(!h_pipe) {
				// @todo @err
				do_exit = true;
			}
			else {
				const BOOL cnpr = ::ConnectNamedPipe(h_pipe, 0);
				if(cnpr) {
					_call_count++;
					PipeSession * p_sess = new PipeSession(h_pipe);
					if(p_sess)
						p_sess->Start(false);
				}
				else {
					// @todo @err
					last_werr = ::GetLastError();
					do_exit = true;
				}
			}
		}
	}
};
//
//
//
int main(int argc, char * argv[])
{
	SString temp_buf;
	SString external_pipe_name;
	SLS.Init("ortools-support", 0);
	//DS.Init(PPSession::internalappCrr32Support, PPSession::fInitPaths, 0, 0);
	{
		const SString pipe_symb_arg_name("/pipesymb");
		for(int i = 1; i < _argc; i++) {
			temp_buf = _argv[i];
			if(external_pipe_name.IsEmpty() && temp_buf.HasPrefixIAscii(pipe_symb_arg_name)) {
				if(temp_buf.C(pipe_symb_arg_name.Len()) == ':') {
					temp_buf.ShiftLeft(pipe_symb_arg_name.Len()+1);
					if(temp_buf.NotEmptyS()) {
						SFile::MakeNamedPipeName(temp_buf, external_pipe_name);
					}
				}
			}
		}
	}
	int    result = 0;
	/*{
		ExitEventCatcher * p_eec = new ExitEventCatcher(0);
		if(p_eec)
			p_eec->Start(false);
	}*/
	if(true) {
		PipeServer_OrToolsSupport * p_psrv = new PipeServer_OrToolsSupport(external_pipe_name);
		if(p_psrv) {
			p_psrv->Start(true);
			::WaitForSingleObject(*p_psrv, INFINITE);
		}
	}
	else {
		//foo();
		PPOptiModule::CuttingStock2D::InitialTestCase();
	}
	return result;
}

