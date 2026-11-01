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

class PPOptiModule {
public:
	struct Config {
		Config();
		uint   TimeLimitMSec; // Максимальное время решения (секунды)
		uint   ThreadCount;   // Количество параллельных потоков
	};
	class CuttingStock2D {
	public:
		enum {
			objvUndef     = 0, // не определено (на усмотрение модуля)
			objvMinSheets = 1, // минимизация числа использованных листов
			objvMinWaste  = 2, // минимизация отходов
			objvMinCost   = 3, // минимизация стоимости			
		};
		struct Config {
			Config();
			enum {
				fGuillotineCut    = 0x0001, // Гильотинный рез: каждый разрез проходит через весь лист/полосу
				fTexturedMaterial = 0x0002, // Материал с текстурой: запрещает поворот деталей на 90°
				fAllowPartial     = 0x0004, // Допускать ли частичное выполнение заказа
			};
			uint   Flags;
			uint   GuillotineLevels; // Уровень гильотинности (2 = двухуровневый, 3 = трёхуровневый, 0 = неограничен)
			uint   KerfWidth;        // Ширина реза (пропил), мм
			uint   Spacing;          // Зазор между деталями (spacing)
			uint   MinUsefulWaste;   // Минимальный размер отхода, который считается полезным обрезком
			int    Objv;             // objvXXX целевая функция оптимизации
		};
		struct RawSheet {
			RawSheet();
			int    Id;
			SPoint2I Size;
			uint   Qtty;
			double Cost;
		};
		struct BomItem {
			BomItem();
			int   Id;
			SPoint2I Size;
			uint  Qtty;
		};
		//
		// Descr: Размещение одной детали на листе
		//
		struct Placement { // @flat
			Placement();
			bool   IsRotated() const { return LOGIC(Flags & fRotated); }
			void   SetRotated(bool v) { SETFLAG(Flags, fRotated, v); }
			enum {
				fRotated = 0x0001 // была ли деталь повёрнута
			};
			int    ItemId;
			uint   ItemInstance;  // какой экземпляр (если quantity > 1)
			int    SheetId;
			uint   SheetInstance; // какой экземпляр листа
			SPoint2I Pt; // координаты левого нижнего угла
			SPoint2I Sz; // фактические ширина и высота (с учётом поворота) 
			uint   Flags; // fXXX
		};
		//
		// Descr: Информация об использованном листе
		//
		struct UsedSheet {
			UsedSheet();
			double GetUtilization() const { return fdivui(UsedArea, TotalArea) * 100.0; }
			int    SheetId;
			uint   SheetInstance;
			uint   TotalArea;
			uint   UsedArea;
			// (не нужно - проще метод сделать если будет важно) double Utilization;      // процент использования
			//std::vector<Placement> placements;
			TSVector <Placement> ItemList;
		};
		//
		// Descr: Результат решения
		//
		enum ResultStatus {
			stError      = 0,
			stOptimal    = 1,
			stFeasible   = 2,
			stInfeasible = 3,
			stTimeout    = 4,
		};

		struct Result {
			Result();
			Result & Z();
			bool   IsSuccess() const { return oneof2(Status, stOptimal, stFeasible); }
			double GetTotalUtilization() const;
			int    Status;
			int    TotalSheetsUsed;
			int    TotalPiecesPlaced;
			int    TotalPiecesRequested;
			uint   TotalWasteArea;
			double TotalCost;
			uint   SolveTimeMSec;
			TSVector <UsedSheet> Sheets;
			TSVector <Placement> AllPlacements;
		};

		CuttingStock2D();
		void   SetConfig(const Config & rCfg);
		void   SetModuleConfig(const PPOptiModule::Config & rCfg);
		void   AddSheet(const RawSheet & rSheet);
		void   AddBomItem(const BomItem & rItem);
		CuttingStock2D & Z();
		bool Solve(Result & rR);
		//
		// Descr: Генерация SVG-визуализации результата раскроя.
		//   Для каждого использованного листа рисуется прямоугольник листа и 
		//   размещённые на нём детали. Детали одного типа (ItemId) окрашиваются 
		//   одним цветом. Повёрнутые детали помечаются индикатором.
		// ARG(rResult IN): результат раскроя
		// ARG(scale IN): масштабный коэффициент (пикселей на единицу размера). 
		//   Если 0.0 - вычисляется автоматически (чтобы ширина полотна была ~1200px).
		// ARG(rSvgText OUT): текст SVG-документа
		// Returns: true если результат содержит хотя бы один использованный лист
		//
		bool GenerateSvg(const Result & rResult, double scale, SString & rSvgText) const;
		//
		static int InitialTestCase();
	private:
		//
		// Descr: Находит размеры листа по его Id в оригинальном списке SheetList
		//
		void GetSheetSize(int sheetId, int & rW, int & rH) const;
		//
		// Descr: Детерминированно выбирает цвет для детали по её Id.
		//   Одинаковые Id - одинаковый цвет.
		//
		static void PickColor(int itemId, SString & rColor);
		//
		// Проверка гильотинности (постобработка)
		//
		struct IRect {
			int    x1;
			int    y1;
			int    x2;
			int    y2;
		};

		static bool IsGuillotineLayout(const TSVector<Placement> & rPlacements, int sheetW, int sheetH, int maxLevels);
		static bool GuillotineCheck(const std::vector<IRect> & pieces, const IRect& area, int level, int maxLevels);
		//
		// Внутренние развёрнутые экземпляры
		//
		struct PieceInstance {
			bool   IsSquare() const { return Width == Height; }
			int    GetMinDim() const { return smin(Width, Height); }
			int    GetMaxDim() const { return smax(Width, Height); }
			//
			int    BomIdx;       // индекс в массиве BomList
			uint   Instance;     // 0..Qtty-1
			int    Width;
			int    Height;
			int    OriginalId;
		};

		struct SheetInstance {
			int    SheetIdx;
			uint   Instance;
			int    Width;
			int    Height;
			double Cost;
			int    OriginalId;
		};
		//
		// Переменные CP-SAT для одного потенциального размещения
		//
		struct PlacementVars {
			int    PieceInstIdx;
			int    SheetInstIdx;
			BoolVar Assigned;
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
		//
		// Данные задачи
		//
		Config Cfg;
		PPOptiModule::Config ModCfg;
		TSVector <RawSheet>  SheetList;
		TSVector <BomItem>   BomList;

		// Развёрнутые экземпляры (заполняются перед решением)
		std::vector <PieceInstance> PieceInst;
		std::vector <SheetInstance> SheetInst;
		//
		void ExpandInstances();
		bool SolveInternal_(Result & rR);
		bool SolveFreePlacementInternal(Result & rR);
		bool SolveGuillotineInternal(Result & rR);
		void ExtractResult(const CpSolverResponse& response, const std::vector<std::vector<PlacementVars>>& pvars, const std::vector<BoolVar>& sheetUsed, uint solveMs, Result & rR);
	};
};

PPOptiModule::Config::Config() : TimeLimitMSec(60000), ThreadCount(4)
{
}

PPOptiModule::CuttingStock2D::Config::Config() : Flags(0), GuillotineLevels(0), KerfWidth(0), Spacing(0), MinUsefulWaste(0), Objv(objvUndef)
{
}

PPOptiModule::CuttingStock2D::RawSheet::RawSheet() : Id(0), Qtty(0), Cost(0.0)
{
}

PPOptiModule::CuttingStock2D::BomItem::BomItem() : Id(0), Qtty(0)
{
}

PPOptiModule::CuttingStock2D::Placement::Placement() : ItemId(0), ItemInstance(0), SheetId(0), SheetInstance(0), Flags()
{
}

PPOptiModule::CuttingStock2D::UsedSheet::UsedSheet() : SheetId(0), SheetInstance(0), TotalArea(0), UsedArea(0)
{
}

PPOptiModule::CuttingStock2D::Result::Result() : Status(stError), TotalSheetsUsed(0), TotalPiecesPlaced(0), TotalPiecesRequested(0), TotalWasteArea(0), TotalCost(0.0), SolveTimeMSec(0) 
{
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
	Sheets.clear();
	AllPlacements.clear();
	return *this;
}
			
double PPOptiModule::CuttingStock2D::Result::GetTotalUtilization() const
{
	uint total_area = 0;
	uint used_area = 0;
	for(uint i = 0; i < Sheets.getCount(); i++) {
		const UsedSheet & r_s = Sheets.at(i);
		total_area += r_s.TotalArea;
		used_area  += r_s.UsedArea;
	}
	return fdivui(used_area, total_area) * 100.0;
}

PPOptiModule::CuttingStock2D::CuttingStock2D()
{
}
		
void PPOptiModule::CuttingStock2D::SetConfig(const PPOptiModule::CuttingStock2D::Config & rCfg) 
{ 
	Cfg = rCfg; 
}
		
void PPOptiModule::CuttingStock2D::SetModuleConfig(const PPOptiModule::Config & rCfg) 
{ 
	ModCfg = rCfg; 
}
		
void PPOptiModule::CuttingStock2D::AddSheet(const PPOptiModule::CuttingStock2D::RawSheet & rSheet) 
{ 
	SheetList.insert(&rSheet); 
}

void PPOptiModule::CuttingStock2D::AddBomItem(const PPOptiModule::CuttingStock2D::BomItem & rItem) 
{ 
	BomList.insert(&rItem); 
}
		
PPOptiModule::CuttingStock2D & PPOptiModule::CuttingStock2D::Z()
{
	SheetList.clear();
	BomList.clear();
	PieceInst.clear();
	SheetInst.clear();
	Cfg = Config();
	return *this;
}

/*static*/bool PPOptiModule::CuttingStock2D::IsGuillotineLayout(const TSVector<Placement> & rPlacements, int sheetW, int sheetH, int maxLevels)
{
	bool   ok = true;
	if(rPlacements.getCount() > 1) {
		std::vector<IRect> rects;
		rects.reserve(rPlacements.getCount());
		for(uint i = 0; i < rPlacements.getCount(); i++) {
			const Placement & pl = rPlacements.at(i);
			rects.push_back({pl.Pt.x, pl.Pt.y, pl.Pt.x + pl.Sz.x, pl.Pt.y + pl.Sz.y});
		}
		IRect area = {0, 0, sheetW, sheetH};
		ok = GuillotineCheck(rects, area, 0, maxLevels);
	}
	return ok;
}

/*static*/bool PPOptiModule::CuttingStock2D::GuillotineCheck(const std::vector<IRect> & pieces, const IRect& area, int level, int maxLevels)
{
	if(pieces.size() <= 1)
		return true;
	if(maxLevels > 0 && level >= maxLevels)
		return false;
	// Вертикальные разрезы: кандидаты — правые границы деталей
	{
		std::set <int> cands;
		for(auto & p : pieces) {
			if(p.x2 > area.x1 && p.x2 < area.x2)
				cands.insert(p.x2);
		}
		for(int xc : cands) {
			bool crosses = false;
			for(auto& p : pieces) {
				if(p.x1 < xc && p.x2 > xc) { 
					crosses = true; 
					break; 
				}
			}
			if(crosses)
				continue;
			std::vector<IRect> left, right;
			for(auto& p : pieces) {
				if(p.x2 <= xc)      
					left.push_back(p);
				else if(p.x1 >= xc) 
					right.push_back(p);
			}
			if(!left.empty() && !right.empty()) {
				IRect la = {area.x1, area.y1, xc, area.y2};
				IRect ra = {xc, area.y1, area.x2, area.y2};
				if(GuillotineCheck(left, la, level+1, maxLevels) && GuillotineCheck(right, ra, level+1, maxLevels)) { // @recursion
					return true;
				}
			}
		}
	}
	// Горизонтальные разрезы: кандидаты — верхние границы деталей
	{
		std::set<int> cands;
		for(auto& p : pieces) {
			if(p.y2 > area.y1 && p.y2 < area.y2)
				cands.insert(p.y2);
		}
		for(int yc : cands) {
			bool crosses = false;
			for(auto& p : pieces) {
				if(p.y1 < yc && p.y2 > yc) { 
					crosses = true; 
					break; 
				}
			}
			if(crosses)
				continue;
			std::vector<IRect> bottom, top;
			for(auto& p : pieces) {
				if(p.y2 <= yc)      
					bottom.push_back(p);
				else if(p.y1 >= yc) 
					top.push_back(p);
			}
			if(!bottom.empty() && !top.empty()) {
				IRect ba = {area.x1, area.y1, area.x2, yc};
				IRect ta = {area.x1, yc, area.x2, area.y2};
				if(GuillotineCheck(bottom, ba, level+1, maxLevels) && GuillotineCheck(top, ta, level+1, maxLevels)) { // @recursion
					return true;
				}
			}
		}
	}
	return false;
}

void PPOptiModule::CuttingStock2D::ExpandInstances()
{
	PieceInst.clear();
	SheetInst.clear();
	for(uint i = 0; i < BomList.getCount(); i++) {
		const BomItem & bi = BomList.at(i);
		for(uint q = 0; q < bi.Qtty; q++) {
			PieceInstance pi;
			pi.BomIdx     = static_cast<int>(i);
			pi.Instance   = q;
			pi.Width      = bi.Size.x;
			pi.Height     = bi.Size.y;
			pi.OriginalId = bi.Id;
			PieceInst.push_back(pi);
		}
	}
	for(uint i = 0; i < SheetList.getCount(); i++) {
		const RawSheet & rs = SheetList.at(i);
		for(uint q = 0; q < rs.Qtty; q++) {
			SheetInstance si;
			si.SheetIdx   = static_cast<int>(i);
			si.Instance   = q;
			si.Width      = rs.Size.x;
			si.Height     = rs.Size.y;
			si.Cost       = rs.Cost;
			si.OriginalId = rs.Id;
			SheetInst.push_back(si);
		}
	}
}

bool PPOptiModule::CuttingStock2D::SolveInternal_(Result & rR)
{
    rR.Z();
    ExpandInstances();
    rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
    
    if(PieceInst.empty()) {
        rR.Status = stFeasible;
        return true;
    }
    if(SheetInst.empty()) {
        rR.Status = stInfeasible;
        return false;
    }

    auto t_start = std::chrono::steady_clock::now();

    const int numP = static_cast<int>(PieceInst.size());
    const int numS = static_cast<int>(SheetInst.size());
    const int kerf = static_cast<int>(Cfg.KerfWidth);
    const int spacing = static_cast<int>(Cfg.Spacing);
    const int extra = kerf + spacing;
    const bool allow_rotation = !(Cfg.Flags & Config::fTexturedMaterial);
    const bool guillotine     = !!(Cfg.Flags & Config::fGuillotineCut);
    const bool allow_partial  = !!(Cfg.Flags & Config::fAllowPartial);

    CpModelBuilder model;

    // ================================================================
    // Определяем максимальное число полос на одном листе.
    // Для двухуровневого гильотинного раскроя: лист делится на 
    // горизонтальные полосы (1-й рез), внутри полосы детали идут 
    // последовательно слева направо (2-й рез).
    // ================================================================
    
    // Найдём минимальную высоту детали — это нижняя граница высоты полосы
    int minPieceH = INT_MAX;
    int maxPieceH = 0;
    for(int p = 0; p < numP; p++) {
        int h = PieceInst[p].Height;
        int w = PieceInst[p].Width;
        int minh = allow_rotation ? smin(w, h) : h;
        int maxh = allow_rotation ? smax(w, h) : h;
        if(minh < minPieceH) minPieceH = minh;
        if(maxh > maxPieceH) maxPieceH = maxh;
    }
    
    // Максимальная высота листа
    int maxSheetH = 0;
    int maxSheetW = 0;
    for(int s = 0; s < numS; s++) {
        if(SheetInst[s].Height > maxSheetH) maxSheetH = SheetInst[s].Height;
        if(SheetInst[s].Width > maxSheetW)  maxSheetW = SheetInst[s].Width;
    }
    
    // Максимальное число полос на листе
    const int maxStrips = (minPieceH > 0) ? (maxSheetH / minPieceH) : numP;
    // Ограничим разумным числом
    const int maxStripsPerSheet = smin(maxStrips, smin(numP, 32));

    // ================================================================
    // Переменные
    // ================================================================
    
    // sheetUsed[s] — используется ли лист s
    std::vector<BoolVar> sheetUsed(numS);
    for(int s = 0; s < numS; s++) {
        sheetUsed[s] = model.NewBoolVar();
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
        BoolVar assigned;     // деталь p назначена на полосу strip листа s
        BoolVar rotated;
        IntVar  actualW;
        IntVar  actualH;
        IntVar  xPos;         // позиция по X внутри полосы (вычисляется из порядка)
    };

    // strips[s][k] — полоса k листа s
    std::vector<std::vector<StripVars>> strips(numS);
    // pieceOnStrip[s][k] — детали, которые могут быть на полосе k листа s
    std::vector<std::vector<std::vector<PieceOnStrip>>> pieceOnStrip(numS);
    
    // assignment[p] — все BoolVar для детали p (по всем листам и полосам)
    std::vector<std::vector<BoolVar>> allAssignments(numP);

    // Для хранения данных, необходимых ExtractResult
    // Мы будем собирать PlacementVars-совместимую структуру
    std::vector<std::vector<PlacementVars>> pvars(numS);

    for(int s = 0; s < numS; s++) {
        const SheetInstance & si = SheetInst[s];
        const int sheetW = si.Width;
        const int sheetH = si.Height;
        
        int numStripsHere = smin(maxStripsPerSheet, sheetH / (minPieceH > 0 ? minPieceH : 1));
        if(numStripsHere < 1) numStripsHere = 1;
        
        strips[s].resize(numStripsHere);
        pieceOnStrip[s].resize(numStripsHere);
        
        for(int k = 0; k < numStripsHere; k++) {
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
        for(int k = 0; k + 1 < numStripsHere; k++) {
            // yEnd[k] + kerf <= yStart[k+1]  (если обе активны)
            // Упрощённо: yEnd[k] <= yStart[k+1] (kerf учтём в высоте полосы)
            model.AddLessOrEqual(strips[s][k].yEnd, strips[s][k+1].yStart);
            
            // Symmetry breaking: полосы используются по порядку
            model.AddLessOrEqual(strips[s][k+1].active, strips[s][k].active);
        }
        
        // Связь sheetUsed[s] с активностью полос
        {
            std::vector<BoolVar> stripActives;
            for(int k = 0; k < numStripsHere; k++)
                stripActives.push_back(strips[s][k].active);
            for(auto & a : stripActives)
                model.AddGreaterOrEqual(sheetUsed[s], a);
            model.AddLessOrEqual(sheetUsed[s], LinearExpr::Sum(stripActives));
        }
        
        // --- Детали на полосах ---
        for(int k = 0; k < numStripsHere; k++) {
            pieceOnStrip[s][k].reserve(numP);
            
            // Собираем все назначения деталей на эту полосу
            std::vector<BoolVar> assignedHere;
            std::vector<IntVar>  widthsHere;  // actual_w + extra для каждой детали
            std::vector<IntVar>  heightsHere; // actual_h для каждой детали
            
            for(int p = 0; p < numP; p++) {
                const PieceInstance & pi = PieceInst[p];
                
                // Проверяем, может ли деталь в принципе поместиться на этом листе
                const bool fitNormal  = (pi.Width <= sheetW && pi.Height <= sheetH);
                const bool fitRotated = allow_rotation && (pi.Height <= sheetW && pi.Width <= sheetH);
                
                if(!fitNormal && !fitRotated)
                    continue;
                
                PieceOnStrip pos;
                pos.pieceIdx = p;
                pos.sheetIdx = s;
                pos.stripIdx = k;
                pos.assigned = model.NewBoolVar();
                
                allAssignments[p].push_back(pos.assigned);
                assignedHere.push_back(pos.assigned);
                
                // Поворот
                if(allow_rotation && !pi.IsSquare() && fitNormal && fitRotated) {
                    pos.rotated = model.NewBoolVar();
                }
                else if(!fitNormal && fitRotated) {
                    pos.rotated = model.TrueVar();
                }
                else {
                    pos.rotated = model.FalseVar();
                }
                
                // Фактические размеры
                const int maxDim = pi.GetMaxDim();
                const int minDim = pi.GetMinDim();
                
                if(pi.IsSquare() || !allow_rotation) {
                    pos.actualW = model.NewConstant(pi.Width);
                    pos.actualH = model.NewConstant(pi.Height);
                }
                else {
                    pos.actualW = model.NewIntVar(Domain(minDim, maxDim));
                    pos.actualH = model.NewIntVar(Domain(minDim, maxDim));
                    model.AddEquality(pos.actualW, pi.Width).OnlyEnforceIf(pos.rotated.Not());
                    model.AddEquality(pos.actualH, pi.Height).OnlyEnforceIf(pos.rotated.Not());
                    model.AddEquality(pos.actualW, pi.Height).OnlyEnforceIf(pos.rotated);
                    model.AddEquality(pos.actualH, pi.Width).OnlyEnforceIf(pos.rotated);
                }
                
                // Высота детали <= высоте полосы (если деталь назначена на полосу)
                model.AddLessOrEqual(pos.actualH, strips[s][k].height)
                     .OnlyEnforceIf(pos.assigned);
                
                // X-позиция будет определяться ниже
                pos.xPos = model.NewIntVar(Domain(0, sheetW));
                
                widthsHere.push_back(pos.actualW);
                heightsHere.push_back(pos.actualH);
                
                pieceOnStrip[s][k].push_back(pos);
            }
            
            // Если нет деталей на полосе — продолжаем
            if(assignedHere.empty())
                continue;
            
            // Связь: если ни одна деталь не назначена, полоса неактивна
            model.AddLessOrEqual(strips[s][k].active, LinearExpr::Sum(assignedHere));
            for(auto & a : assignedHere)
                model.AddGreaterOrEqual(strips[s][k].active, a);
            
            // ============================================================
            // ГИЛЬОТИННЫЙ РЕЗ: детали внутри полосы не перекрываются по X.
            // Используем 1D NoOverlap (интервалы по X).
            // Это гарантирует, что внутри полосы детали можно отрезать 
            // вертикальными резами.
            // ============================================================
            
            if(guillotine) {
                // 1D NoOverlap по X внутри полосы
                std::vector<IntervalVar> xIntervals;
                
                for(uint pidx = 0; pidx < pieceOnStrip[s][k].size(); pidx++) {
                    PieceOnStrip & pos = pieceOnStrip[s][k][pidx];
                    
                    IntVar xSize;
                    if(extra > 0) {
                        xSize = model.NewIntVar(Domain(0, sheetW + extra));
                        model.AddEquality(xSize, LinearExpr(pos.actualW) + extra)
                             .OnlyEnforceIf(pos.assigned);
                        model.AddEquality(xSize, 0).OnlyEnforceIf(pos.assigned.Not());
                    }
                    else {
                        xSize = pos.actualW;
                    }
                    
                    IntVar xEnd = model.NewIntVar(Domain(0, sheetW + extra));
                    IntervalVar xIval = model.NewOptionalIntervalVar(
                        pos.xPos, xSize, xEnd, pos.assigned);
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
                        model.AddGreaterOrEqual(strips[s][k].height, 
                            LinearExpr(pos.actualH) + kerf)
                            .OnlyEnforceIf(pos.assigned);
                    }
                    else {
                        model.AddGreaterOrEqual(strips[s][k].height, pos.actualH)
                            .OnlyEnforceIf(pos.assigned);
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
                        IntVar numAssigned = model.NewIntVar(Domain(0, numP));
                        model.AddEquality(numAssigned, LinearExpr::Sum(assignedHere));
                        // usedWidth >= sumW + (numAssigned - 1) * extra
                        // Упрощённо: usedWidth >= sumW (для целевой функции)
                    }
                    model.AddGreaterOrEqual(strips[s][k].usedWidth, sumW);
                }
            }
            else {
                // ============================================================
                // Без гильотины: используем 2D NoOverlap как раньше, 
                // но внутри полосы
                // ============================================================
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
                    model.AddLessOrEqual(yEnd, strips[s][k].height)
                         .OnlyEnforceIf(pos.assigned);
                    
                    IntervalVar xIval = model.NewOptionalIntervalVar(
                        pos.xPos, xSize, xEnd, pos.assigned);
                    IntervalVar yIval = model.NewOptionalIntervalVar(
                        yPos, ySize, yEnd, pos.assigned);
                    
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

    // ================================================================
    // Каждая деталь — ровно на один лист/полосу
    // ================================================================
    for(int p = 0; p < numP; p++) {
        if(allAssignments[p].empty()) {
            if(!allow_partial) {
                // Деталь не помещается ни на один лист => infeasible
                rR.Status = stInfeasible;
                return false;
            }
            continue;
        }
        if(allow_partial)
            model.AddAtMostOne(allAssignments[p]);
        else
            model.AddExactlyOne(allAssignments[p]);
    }

    // ================================================================
    // Symmetry breaking: одинаковые листы по порядку
    // ================================================================
    for(int s = 1; s < numS; s++) {
        if(SheetInst[s].SheetIdx == SheetInst[s-1].SheetIdx) {
            model.AddLessOrEqual(sheetUsed[s], sheetUsed[s-1]);
        }
    }

    // ================================================================
    // Целевая функция
    // ================================================================
    // Основная цель
    int objv = Cfg.Objv;
    if(objv == objvUndef)
        objv = objvMinWaste;

    // Для компактности: добавляем вторичную цель — минимизировать 
    // суммарную "использованную площадь полос" (height * sheetW)
    // Это стимулирует минимизацию высот полос и, как следствие,
    // оставляет большой цельный остаток внизу/наверху листа.
    
    // Масштабирующий коэффициент: основная цель должна доминировать
    const int64_t PRIMARY_WEIGHT = 1000000LL;
    const int64_t COMPACT_WEIGHT = 1LL;

    LinearExpr objective;
    
    switch(objv) {
        case objvMinSheets:
            objective = LinearExpr::Sum(sheetUsed) * PRIMARY_WEIGHT;
            break;
        case objvMinCost:
            {
                LinearExpr costExpr;
                for(int s = 0; s < numS; s++) {
                    int costInt = static_cast<int>(SheetInst[s].Cost * 1000);
                    costExpr += LinearExpr(sheetUsed[s]) * costInt;
                }
                objective = costExpr * PRIMARY_WEIGHT;
            }
            break;
        case objvMinWaste:
        default:
            {
                LinearExpr areaExpr;
                for(int s = 0; s < numS; s++) {
                    int area = SheetInst[s].Width * SheetInst[s].Height;
                    areaExpr += LinearExpr(sheetUsed[s]) * area;
                }
                objective = areaExpr * PRIMARY_WEIGHT;
            }
            break;
    }
    
    // Вторичная цель: минимизировать суммарную высоту полос
    // (стимулирует компактность по вертикали)
    {
        LinearExpr compactExpr;
        for(int s = 0; s < numS; s++) {
            for(uint k = 0; k < strips[s].size(); k++) {
                // Минимизируем высоту каждой полосы * ширину листа
                // Это эффективно минимизирует "занятую площадь"
                compactExpr += LinearExpr(strips[s][k].height) * SheetInst[s].Width;
            }
        }
        objective += compactExpr * COMPACT_WEIGHT;
    }
    
    // Третичная цель: минимизировать разброс Y-координат
    // (стимулирует прижатие полос к началу листа)
    {
        LinearExpr yExpr;
        for(int s = 0; s < numS; s++) {
            for(uint k = 0; k < strips[s].size(); k++) {
                yExpr += strips[s][k].yEnd;
            }
        }
        // Ещё меньший вес
        // objective += yExpr;  // можно раскомментировать при необходимости
    }
    
    model.Minimize(objective);

    // ================================================================
    // Параметры солвера
    // ================================================================
    SatParameters satParams;
    satParams.set_max_time_in_seconds(static_cast<double>(ModCfg.TimeLimitMSec) / 1000.0);
    satParams.set_num_workers(static_cast<int>(ModCfg.ThreadCount));
    satParams.set_log_search_progress(true);
    satParams.set_linearization_level(2);
    
    // Подсказки для поиска: попробовать сначала более "жадную" стратегию
    // satParams.set_search_branching(SatParameters::FIXED_SEARCH);

    // ================================================================
    // Решение
    // ================================================================
    CpSolverResponse response = SolveWithParameters(model.Build(), satParams);
    
    auto t_end = std::chrono::steady_clock::now();
    uint solveMs = static_cast<uint>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());

    // ================================================================
    // Извлечение результата
    // ================================================================
    rR.SolveTimeMSec = solveMs;
    auto cpStatus = response.status();
    
    if(cpStatus == CpSolverStatus::INFEASIBLE) {
        rR.Status = stInfeasible;
    }
    else if(oneof2(cpStatus, CpSolverStatus::OPTIMAL, CpSolverStatus::FEASIBLE)) {
        rR.Status = (cpStatus == CpSolverStatus::OPTIMAL) ? stOptimal : stFeasible;
        
        for(int s = 0; s < numS; s++) {
            if(!SolutionBooleanValue(response, sheetUsed[s]))
                continue;
            
            UsedSheet us;
            us.SheetId       = SheetInst[s].OriginalId;
            us.SheetInstance  = SheetInst[s].Instance;
            us.TotalArea      = static_cast<uint>(SheetInst[s].Width * SheetInst[s].Height);
            us.UsedArea       = 0;
            
            for(uint k = 0; k < strips[s].size(); k++) {
                if(!SolutionBooleanValue(response, strips[s][k].active))
                    continue;
                
                int stripY = static_cast<int>(
                    SolutionIntegerValue(response, strips[s][k].yStart));
                int stripH = static_cast<int>(
                    SolutionIntegerValue(response, strips[s][k].height));
                
                for(const auto & pos : pieceOnStrip[s][k]) {
                    if(!SolutionBooleanValue(response, pos.assigned))
                        continue;
                    
                    Placement pl;
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
                    
                    us.UsedArea += static_cast<uint>(aw * ah);
                    us.ItemList.insert(&pl);
                    rR.AllPlacements.insert(&pl);
                }
            }
            
            rR.Sheets.insert(&us);
            rR.TotalCost += SheetInst[s].Cost;
        }
        
        rR.TotalSheetsUsed   = static_cast<int>(rR.Sheets.getCount());
        rR.TotalPiecesPlaced = static_cast<int>(rR.AllPlacements.getCount());
        
        uint totalArea = 0, usedArea = 0;
        for(uint i = 0; i < rR.Sheets.getCount(); i++) {
            totalArea += rR.Sheets.at(i).TotalArea;
            usedArea  += rR.Sheets.at(i).UsedArea;
        }
        rR.TotalWasteArea = totalArea - usedArea;
    }
    else {
        rR.Status = stTimeout;
    }
    return rR.IsSuccess();
}

bool PPOptiModule::CuttingStock2D::SolveGuillotineInternal(Result & rR)
{
	rR.Z();
	bool   ok = false;
	ExpandInstances();
	rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
	if(PieceInst.empty()) {
		rR.Status = stFeasible;
	}
	else if(SheetInst.empty()) {
		rR.Status = stInfeasible;
	}
	else {
		auto t_start = std::chrono::steady_clock::now();
		// @todo
		auto t_end = std::chrono::steady_clock::now();
	}
	return ok;
}

bool PPOptiModule::CuttingStock2D::SolveFreePlacementInternal(Result & rR)
{
	rR.Z();
	bool   ok = false;
	ExpandInstances();
	rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
	if(PieceInst.empty()) {
		rR.Status = stFeasible;
	}
	else if(SheetInst.empty()) {
		rR.Status = stInfeasible;
	}
	else {
		auto t_start = std::chrono::steady_clock::now();
		const int piece_count = static_cast<int>(PieceInst.size());
		const int sheet_count = static_cast<int>(SheetInst.size());
		const int extra = static_cast<int>(Cfg.KerfWidth + Cfg.Spacing);
		const bool allow_rotation = !LOGIC(Cfg.Flags & Config::fTexturedMaterial);
		// (это ограничение отрабатывается в SolveGuillotineInternal) const bool guillotine     = LOGIC(Cfg.Flags & Config::fGuillotineCut);
		const bool allow_partial  = LOGIC(Cfg.Flags & Config::fAllowPartial);

		CpModelBuilder model;

		// --- Булевы переменные: используется ли лист s ---
		std::vector<BoolVar> sheetUsed(sheet_count);
		for(int s = 0; s < sheet_count; s++) {
			sheetUsed[s] = model.NewBoolVar();
		}
		// --- Переменные размещения ---
		// pvars[s] — вектор PlacementVars для деталей, потенциально назначенных на лист s
		std::vector<std::vector<PlacementVars>> pvars(sheet_count);

		// assignment[p][s] — BoolVar: деталь p назначена на лист s
		std::vector<std::vector<BoolVar>> assignment(piece_count);
		for(int p = 0; p < piece_count; p++) {
			const PieceInstance & r_pi = PieceInst[p];
			assignment[p].resize(sheet_count);
			for(int s = 0; s < sheet_count; s++) {
				const SheetInstance & r_si = SheetInst[s];
				const bool fitNormal  = (r_pi.Width <= r_si.Width && r_pi.Height <= r_si.Height);
				const bool fitRotated = allow_rotation && (r_pi.Height <= r_si.Width && r_pi.Width <= r_si.Height);
				if(!fitNormal && !fitRotated) {
					assignment[p][s] = model.FalseVar();
					continue;
				}
				PlacementVars pv;
				pv.PieceInstIdx = p;
				pv.SheetInstIdx = s;
				pv.Assigned = model.NewBoolVar();
				assignment[p][s] = pv.Assigned;
				// --- Поворот ---
				if(allow_rotation && !r_pi.IsSquare() && fitNormal && fitRotated) {
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
				const int max_dim = r_pi.GetMaxDim();
				const int min_dim = r_pi.GetMinDim();
				if(r_pi.IsSquare() || !allow_rotation) {
					pv.ActualWidth  = model.NewConstant(r_pi.Width);
					pv.ActualHeight = model.NewConstant(r_pi.Height);
				}
				else {
					pv.ActualWidth  = model.NewIntVar(Domain(min_dim, max_dim));
					pv.ActualHeight = model.NewIntVar(Domain(min_dim, max_dim));
					// rotated=0 => W=pi.Width,  H=pi.Height
					// rotated=1 => W=pi.Height, H=pi.Width
					model.AddEquality(pv.ActualWidth,  r_pi.Width).OnlyEnforceIf(pv.Rotated.Not());
					model.AddEquality(pv.ActualHeight, r_pi.Height).OnlyEnforceIf(pv.Rotated.Not());
					model.AddEquality(pv.ActualWidth,  r_pi.Height).OnlyEnforceIf(pv.Rotated);
					model.AddEquality(pv.ActualHeight, r_pi.Width).OnlyEnforceIf(pv.Rotated);
				}
				// --- Координаты ---
				pv.XStart = model.NewIntVar(Domain(0, r_si.Width));
				pv.YStart = model.NewIntVar(Domain(0, r_si.Height));
				pv.XEnd   = model.NewIntVar(Domain(0, r_si.Width));
				pv.YEnd   = model.NewIntVar(Domain(0, r_si.Height));
				// x_end = x_start + actual_width
				// y_end = y_start + actual_height
				model.AddEquality(pv.XEnd, LinearExpr(pv.XStart) + LinearExpr(pv.ActualWidth));
				model.AddEquality(pv.YEnd, LinearExpr(pv.YStart) + LinearExpr(pv.ActualHeight));
				// Деталь не выходит за границы листа
				model.AddLessOrEqual(pv.XEnd, r_si.Width).OnlyEnforceIf(pv.Assigned);
				model.AddLessOrEqual(pv.YEnd, r_si.Height).OnlyEnforceIf(pv.Assigned);
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
				IntVar xIvalEnd = model.NewIntVar(Domain(0, r_si.Width + extra));
				IntVar yIvalEnd = model.NewIntVar(Domain(0, r_si.Height + extra));
				pv.XIval = model.NewOptionalIntervalVar(pv.XStart, xSize, xIvalEnd, pv.Assigned);
				pv.YIval = model.NewOptionalIntervalVar(pv.YStart, ySize, yIvalEnd, pv.Assigned);
				pvars[s].push_back(pv);
			}
			// Каждая деталь — ровно на один лист (или не более одного)
			if(allow_partial)
				model.AddAtMostOne(assignment[p]);
			else
				model.AddExactlyOne(assignment[p]);
		}

		// --- NoOverlap2D для каждого листа ---
		for(int s = 0; s < sheet_count; s++) {
			if(pvars[s].empty())
				continue;
			NoOverlap2DConstraint no2d = model.AddNoOverlap2D();
			for(auto & pv : pvars[s])
				no2d.AddRectangle(pv.XIval, pv.YIval);
			// Связь: sheet_used[s] = OR(assigned для всех деталей на s)
			std::vector<BoolVar> onSheet;
			onSheet.reserve(pvars[s].size());
			for(auto & pv : pvars[s])
				onSheet.push_back(pv.Assigned);
			// sheet_used >= each
			for(auto & b : onSheet)
				model.AddGreaterOrEqual(sheetUsed[s], b);
			// sheet_used <= sum
			model.AddLessOrEqual(sheetUsed[s], LinearExpr::Sum(onSheet));
		}
		/* (это ограничение отрабатывается в SolveGuillotineInternal)
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
			if(SheetInst[s].SheetIdx == SheetInst[s-1].SheetIdx) {
				// Если лист s-1 не используется, то и лист s не используется
				model.AddLessOrEqual(sheetUsed[s], sheetUsed[s-1]);
			}
		}
		// --- Symmetry breaking: одинаковые детали — лексикографический порядок ---
		// Одинаковые экземпляры одной BomItem: упорядочить по (sheet, x, y)
		{
			std::map<int, std::vector<int>> bomGroups; // BomIdx -> [pieceInstIdx...]
			for(int p = 0; p < piece_count; p++) {
				bomGroups[PieceInst[p].BomIdx].push_back(p);
			}
			for(auto & [bomIdx, group] : bomGroups) {
				if(group.size() <= 1)
					continue;
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
		// --- Целевая функция ---
		int objv = Cfg.Objv;
		if(objv == objvUndef)
			objv = objvMinWaste;
		switch(objv) {
			case objvMinSheets:
				model.Minimize(LinearExpr::Sum(sheetUsed));
				break;
			case objvMinCost:
				{
					LinearExpr cost_expr;
					for(int s = 0; s < sheet_count; s++) {
						const  int cost_int = static_cast<int>(SheetInst[s].Cost * 1000);
						cost_expr += LinearExpr(sheetUsed[s]) * cost_int;
					}
					model.Minimize(cost_expr);
				}
				break;
			case objvMinWaste:
			default:
				{
					// Минимизация суммарной площади используемых листов (площадь деталей фиксирована => эквивалентно минимизации отходов)
					LinearExpr area_expr;
					for(int s = 0; s < sheet_count; s++) {
						int area = SheetInst[s].Width * SheetInst[s].Height;
						area_expr += LinearExpr(sheetUsed[s]) * area;
					}
					model.Minimize(area_expr);
				}
				break;
		}

		// --- Параметры солвера ---
		SatParameters sat_params;
		sat_params.set_max_time_in_seconds(static_cast<double>(ModCfg.TimeLimitMSec) / 1000.0);
		sat_params.set_num_workers(static_cast<int>(ModCfg.ThreadCount));
		sat_params.set_log_search_progress(true);
		sat_params.set_linearization_level(2);
		// --- Решение ---
		CpSolverResponse response = SolveWithParameters(model.Build(), sat_params);
		auto t_end = std::chrono::steady_clock::now();
		uint solveMs = static_cast<uint>(std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
		// --- Извлечение результата ---
		ExtractResult(response, pvars, sheetUsed, solveMs, rR);
	}
	ok = rR.IsSuccess();
	return ok;
}
		
void PPOptiModule::CuttingStock2D::ExtractResult(const CpSolverResponse & response, 
	const std::vector<std::vector<PlacementVars>> & pvars, const std::vector<BoolVar> & rSheetUsed, uint solveMs, Result & rR)
{
	rR.SolveTimeMSec = solveMs;
	rR.TotalPiecesRequested = static_cast<int>(PieceInst.size());
	auto cpStatus = response.status();
	if(cpStatus == CpSolverStatus::INFEASIBLE) {
		rR.Status = stInfeasible;
		//return result;
	}
	else if(oneof2(cpStatus, CpSolverStatus::OPTIMAL, CpSolverStatus::FEASIBLE)) {
		if(cpStatus == CpSolverStatus::OPTIMAL)
			rR.Status = stOptimal;
		else if(cpStatus == CpSolverStatus::FEASIBLE)
			rR.Status = stFeasible;
		const int sheet_count = static_cast<int>(SheetInst.size());
		// Собрать использованные листы
		for(int s = 0; s < sheet_count; s++) {
			if(!SolutionBooleanValue(response, rSheetUsed[s]))
				continue;
			UsedSheet us;
			us.SheetId = SheetInst[s].OriginalId;
			us.SheetInstance = SheetInst[s].Instance;
			us.TotalArea = static_cast<uint>(SheetInst[s].Width * SheetInst[s].Height);
			us.UsedArea = 0;
			for(const auto & pv : pvars[s]) {
				if(!SolutionBooleanValue(response, pv.Assigned))
					continue;
				Placement pl;
				pl.ItemId       = PieceInst[pv.PieceInstIdx].OriginalId;
				pl.ItemInstance  = PieceInst[pv.PieceInstIdx].Instance;
				pl.SheetId      = SheetInst[s].OriginalId;
				pl.SheetInstance = SheetInst[s].Instance;
				pl.Pt.Set(static_cast<int>(SolutionIntegerValue(response, pv.XStart)), static_cast<int>(SolutionIntegerValue(response, pv.YStart)));
				pl.Sz.Set(static_cast<int>(SolutionIntegerValue(response, pv.ActualWidth)), static_cast<int>(SolutionIntegerValue(response, pv.ActualHeight)));
				pl.SetRotated(SolutionBooleanValue(response, pv.Rotated));
				us.UsedArea += static_cast<uint>(pl.Sz.x * pl.Sz.y);
				us.ItemList.insert(&pl);
				rR.AllPlacements.insert(&pl);
			}
			rR.Sheets.insert(&us);
			rR.TotalCost += SheetInst[s].Cost;
		}
		rR.TotalSheetsUsed  = static_cast<int>(rR.Sheets.getCount());
		rR.TotalPiecesPlaced = static_cast<int>(rR.AllPlacements.getCount());
		// Подсчёт отходов
		uint totalArea = 0, usedArea = 0;
		for(uint i = 0; i < rR.Sheets.getCount(); i++) {
			totalArea += rR.Sheets.at(i).TotalArea;
			usedArea  += rR.Sheets.at(i).UsedArea;
		}
		rR.TotalWasteArea = totalArea - usedArea;
		// Постпроверка гильотинности
		if((Cfg.Flags & Config::fGuillotineCut) && rR.IsSuccess()) {
			for(uint i = 0; i < rR.Sheets.getCount(); i++) {
				const UsedSheet & us = rR.Sheets.at(i);
				// Найти размеры листа
				int sw = 0;
				int sh = 0;
				for(uint j = 0; j < SheetList.getCount(); j++) {
					if(SheetList.at(j).Id == us.SheetId) {
						sw = SheetList.at(j).Size.x;
						sh = SheetList.at(j).Size.y;
						break;
					}
				}
				if(sw > 0 && sh > 0) {
					/*bool ok =*/ IsGuillotineLayout(us.ItemList, sw, sh, static_cast<int>(Cfg.GuillotineLevels));
					// @todo При необходимости пометить лист, если проверка не пройдена
				}
			}
		}
		//return result;
	}
	else {
		rR.Status = stTimeout;
		//return result;
	}
}

bool PPOptiModule::CuttingStock2D::Solve(PPOptiModule::CuttingStock2D::Result & rR)
{
	bool   ok = false;
	ok = SolveInternal_(rR);
	/*
	if(Cfg.Flags & Config::fGuillotineCut) {
		ok = SolveGuillotineInternal(rR);
	}
	else {
		ok = SolveFreePlacementInternal(rR);
	}
	*/
	return ok;
}

void PPOptiModule::CuttingStock2D::GetSheetSize(int sheetId, int & rW, int & rH) const
{
	rW = 0;
	rH = 0;
	for(uint i = 0; i < SheetList.getCount(); i++) {
		const RawSheet & r_s = SheetList.at(i);
		if(r_s.Id == sheetId) {
			rW = r_s.Size.x;
			rH = r_s.Size.y;
			break;
		}
	}
}

/*static*/void PPOptiModule::CuttingStock2D::PickColor(int itemId, SString & rColor)
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
	uint idx = static_cast<uint>(itemId >= 0 ? itemId : -itemId) % n;
	rColor = p_palette[idx];
}

bool PPOptiModule::CuttingStock2D::GenerateSvg(const PPOptiModule::CuttingStock2D::Result & rResult, double scale, SString & rSvgText) const
{
	rSvgText.Z();
	bool   ok = true;
	if(!rResult.IsSuccess() || rResult.Sheets.getCount() == 0)
		ok = false;
	else {
		// --- Автомасштаб ---
		if(scale <= 0.0) {
			// Найдём максимальную ширину листа и подберём scale 
			// так, чтобы один лист был ~1000px
			int max_sheet_w = 0;
			for(uint i = 0; i < rResult.Sheets.getCount(); i++) {
				int sw = 0;
				int sh = 0;
				GetSheetSize(rResult.Sheets.at(i).SheetId, sw, sh);
				if(sw > max_sheet_w)
					max_sheet_w = sw;
			}
			scale = (max_sheet_w > 0) ? (1000.0 / max_sheet_w) : 1.0;
		}

		// --- Вычисление размеров полотна ---
		const int gap_px      = 20;    // расстояние между листами
		const int header_px   = 50;    // высота заголовка над каждым листом
		const int margin_px   = 20;    // внешний отступ
		const int footer_px   = 40;    // высота итогового блока
		const int max_per_row = 2;     // листов в ряду

		int row_count = (static_cast<int>(rResult.Sheets.getCount()) + max_per_row - 1) / max_per_row;

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
			for(uint i = 0; i < rResult.Sheets.getCount(); i++) {
				int sw = 0;
				int sh = 0;
				GetSheetSize(rResult.Sheets.at(i).SheetId, sw, sh);
				const int w_px = static_cast<int>(sw * scale) + gap_px;
				const int h_px = static_cast<int>(sh * scale) + header_px + gap_px;
				row_w += w_px;
				if(h_px > row_h)
					row_h = h_px;
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

		for(uint i = 0; i < rResult.Sheets.getCount(); i++) {
			const UsedSheet & r_us = rResult.Sheets.at(i);
			int sw = 0;
			int sh = 0;
			GetSheetSize(r_us.SheetId, sw, sh);
			const int sheet_w_px = static_cast<int>(sw * scale);
			const int sheet_h_px = static_cast<int>(sh * scale);

			// --- Заголовок листа ---
			r_svg.Cat("<text class=\"title\" x=\"").Cat(cur_x).Cat("\" y=\"").Cat(cur_y + 14).Cat("\">")
					.Cat("Sheet ").Cat(r_us.SheetId).Cat(".").Cat(static_cast<uint>(r_us.SheetInstance))
					.Cat(" (").Cat(sw).Cat("x").Cat(sh).Cat(")</text>").CR();
        
			temp_buf.Z().Cat(r_us.GetUtilization(), MKSFMTD(0, 1, 0));
			r_svg.Cat("<text class=\"subtitle\" x=\"").Cat(cur_x).Cat("\" y=\"").Cat(cur_y + 30).Cat("\">")
					.Cat("Pieces: ").Cat(static_cast<uint>(r_us.ItemList.getCount()))
					.Cat("  Util: ").Cat(temp_buf).Cat("%")
					.Cat("  Used: ").Cat(r_us.UsedArea).Cat("/").Cat(r_us.TotalArea).Cat("</text>").CR();

			// --- Прямоугольник листа ---
			const int sheet_x = cur_x;
			const int sheet_y = cur_y + header_px;
			r_svg.Cat("<rect class=\"sheet\" x=\"").Cat(sheet_x).Cat("\" y=\"").Cat(sheet_y).Cat("\" ")
					.Cat("width=\"").Cat(sheet_w_px).Cat("\" height=\"").Cat(sheet_h_px).Cat("\"/>").CR();

			// --- Детали ---
			// Координаты в задаче: (0,0) - левый НИЖНИЙ угол листа. 
			// В SVG: (0,0) - левый ВЕРХНИЙ. Нужна инверсия Y.
			for(uint j = 0; j < r_us.ItemList.getCount(); j++) {
				const Placement & r_pl = r_us.ItemList.at(j);
				const int pw_px = static_cast<int>(r_pl.Sz.x * scale);
				const int ph_px = static_cast<int>(r_pl.Sz.y * scale);
				// Инверсия Y: svg_y = sheet_top + (sheet_h - piece_y - piece_h)
				const int px = sheet_x + static_cast<int>(r_pl.Pt.x * scale);
				const int py = sheet_y + sheet_h_px - static_cast<int>((r_pl.Pt.y + r_pl.Sz.y) * scale);

				SString color;
				PickColor(r_pl.ItemId, color);

				r_svg.Cat("<rect class=\"piece\" x=\"").Cat(px).Cat("\" y=\"").Cat(py).Cat("\" ")
						.Cat("width=\"").Cat(pw_px).Cat("\" height=\"").Cat(ph_px).Cat("\" ")
						.Cat("fill=\"").Cat(color).Cat("\">")
						.Cat("<title>Item ").Cat(r_pl.ItemId).Cat(" [").Cat(static_cast<uint>(r_pl.ItemInstance)).Cat("]")
						.Cat("  size=").Cat(r_pl.Sz.x).Cat("x").Cat(r_pl.Sz.y)
						.Cat("  pos=(").Cat(r_pl.Pt.x).Cat(",").Cat(r_pl.Pt.y).Cat(")")
						.Cat(r_pl.IsRotated() ? "  [ROTATED]" : "")
						.Cat("</title></rect>").CR();

				// --- Подпись детали ---
				if(pw_px >= 30 && ph_px >= 15) {
					const int cx = px + pw_px / 2;
					const int cy = py + ph_px / 2;
					r_svg.Cat("<text class=\"label\" x=\"").Cat(cx).Cat("\" y=\"").Cat(cy).Cat("\">")
							.Cat(r_pl.ItemId);
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
			if(this_row_h > row_max_h)
				row_max_h = this_row_h;
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

/*static*/int PPOptiModule::CuttingStock2D::InitialTestCase()
{
	int    ok = 0;
	PPOptiModule::Config modCfg;
	modCfg.TimeLimitMSec = 60000;
	modCfg.ThreadCount   = 8;

	PPOptiModule::CuttingStock2D cs;
	cs.SetModuleConfig(modCfg);
	{
		PPOptiModule::CuttingStock2D::Config cfg;
		cfg.Flags = PPOptiModule::CuttingStock2D::Config::fGuillotineCut|PPOptiModule::CuttingStock2D::Config::fTexturedMaterial;
		cfg.GuillotineLevels = 3;
		cfg.KerfWidth        = 4;
		cfg.Objv = PPOptiModule::CuttingStock2D::objvMinWaste;
		cs.SetConfig(cfg);
	}
	// Листы ДСП
	{
		PPOptiModule::CuttingStock2D::RawSheet sh;
		sh.Id   = 1;
		sh.Size.Set(2440, 1220);
		sh.Qtty = 5;
		sh.Cost = 1500.0;
		cs.AddSheet(sh);
	}
	{
		PPOptiModule::CuttingStock2D::RawSheet sh;
		sh.Id   = 2;
		sh.Size.Set(1830, 1220);
		sh.Qtty = 3;
		sh.Cost = 1100.0;
		cs.AddSheet(sh);
	}
	// Детали
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 1; 
		bi.Size.Set(600, 400); 
		bi.Qtty = 4;
		cs.AddBomItem(bi);
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 2; 
		bi.Size.Set(800, 500); 
		bi.Qtty = 2;
		cs.AddBomItem(bi);
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 3; 
		bi.Size.Set(300, 200); 
		bi.Qtty = 8;
		cs.AddBomItem(bi);
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 4; 
		bi.Size.Set(1000, 600); 
		bi.Qtty = 2;
		cs.AddBomItem(bi);
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 5; 
		bi.Size.Set(450, 350); 
		bi.Qtty = 6;
		cs.AddBomItem(bi);
	}
	{
		PPOptiModule::CuttingStock2D::BomItem bi;
		bi.Id = 6; 
		bi.Size.Set(700, 300); 
		bi.Qtty = 3;
		cs.AddBomItem(bi);
	}
	PPOptiModule::CuttingStock2D::Result result;
	// Вывод результатов (для отладки)
	if(cs.Solve(result)) {
		assert(result.IsSuccess());
		printf("Status: %s\n", (result.Status == PPOptiModule::CuttingStock2D::stOptimal) ? "OPTIMAL" : "FEASIBLE");
		printf("Solve time: %u ms\n", result.SolveTimeMSec);
		printf("Sheets used: %d\n", result.TotalSheetsUsed);
		printf("Pieces placed: %d / %d\n", result.TotalPiecesPlaced, result.TotalPiecesRequested);
		printf("Utilization: %.1f%%\n", result.GetTotalUtilization());
		printf("Waste area: %u\n", result.TotalWasteArea);
		printf("Total cost: %.2f\n", result.TotalCost);
		for(uint i = 0; i < result.Sheets.getCount(); i++) {
			const auto & us = result.Sheets.at(i);
			printf("\n  Sheet %d (inst %u): util=%.1f%%\n", us.SheetId, us.SheetInstance, us.GetUtilization());
			for(uint j = 0; j < us.ItemList.getCount(); j++) {
				const auto & pl = us.ItemList.at(j);
				printf("    Item %d[%u]: pos=(%d,%d) size=%dx%d%s\n",
					pl.ItemId, pl.ItemInstance, pl.Pt.x, pl.Pt.y, pl.Sz.x, pl.Sz.y, pl.IsRotated() ? " [ROT]" : "");
			}
		}
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

int main(int argc, char * argv[])
{
	SLS.Init("ortools-support", 0);
	int    result = 0;
	foo();
	PPOptiModule::CuttingStock2D::InitialTestCase();
	return result;
}

