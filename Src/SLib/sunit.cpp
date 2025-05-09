// SUNIT.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2012, 2016, 2017, 2020, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

struct ClsEntry {
	int8   Cls;
	int16  BaseUnit;
	const char * P_Name;
};

static ClsEntry __Cls[] = {
	{ SUnit::clsLength,             SUOM_METER,      "length" },
	{ SUnit::clsMass,               SUOM_KILOGRAM,   "mass" },
	{ SUnit::clsTime,               SUOM_SECOND,     "time" },
	{ SUnit::clsVolume,             SUOM_LITER,      "volume" },
	{ SUnit::clsItem,               SUOM_ITEM,       "item" },
	{ SUnit::clsPart,               SUOM_ONE,        "part" },
	{ SUnit::clsAngle,              SUOM_RADIAN,     "angle" },

	{ SUnit::clsArea,               0, "area" },
	{ SUnit::clsCurrent,            0, "current" },
	{ SUnit::clsAmount,             0, "amount" },
	{ SUnit::clsSolidAngle,         0, "solidangle" },
	{ SUnit::clsMoney,              0, "money" },
	{ SUnit::clsForce,              0, "force" },
	{ SUnit::clsPressure,           0, "pressure" },
	{ SUnit::clsEnergy,             0, "energy" },
	{ SUnit::clsTemperature,        0, "temperature" },
	{ SUnit::clsCharge,             0, "charge" },
	{ SUnit::clsCapacitance,        0, "capacitance" },
	{ SUnit::clsResistance,         0, "resistance" },
	{ SUnit::clsConductance,        0, "conductance" },
	{ SUnit::clsInductance,         0, "inductance" },
	{ SUnit::clsFrequence,          0, "frequence" },
	{ SUnit::clsVelocity,           0, "velocity" },
	{ SUnit::clsAcceleration,       0, "acceleration" },
	{ SUnit::clsDensity,            0, "density" },
	{ SUnit::clsLinearDensity,      0, "lineardensity" },
	{ SUnit::clsViscosity,          0, "viscosity" },
	{ SUnit::clsKinematicViscosity, 0, "kinematicviscosity" },
	{ SUnit::clsInformation,        0, "information" }, // @v11.9.4 Количество информации
	{ SUnit::clsVoltage,            0, "voltage" }, // @v11.9.4 Электрическое напряжение (разность потенциалов)
};

struct UnitEntry {
	int16  Unit;
	int8   Cls;
	const char * P_Name;
	double ToBase;
};

static UnitEntry __Units[] = {
	{ SUOM_METER,     SUnit::clsLength, "meter", 1.0 },
	{ SUOM_INCH,      SUnit::clsLength, "inch", 2.54e-2 },
	{ SUOM_KILOGRAM,  SUnit::clsMass,   "kilogramm", 1.0 },
	{ SUOM_GRAM,      SUnit::clsMass,   "gramm", 1.0e-3 },
	{ SUOM_LITER,     SUnit::clsVolume, "liter", 1.0 },
	{ SUOM_FLOZ,      SUnit::clsVolume, "floz", 0.029573531 },
	{ SUOM_WATT,      SUnit::clsEnergy, "watt", 1.0 },
	{ SUOM_VOLT,      SUnit::clsVoltage, "volt", 1.0 },
	{ SUOM_DAY,       SUnit::clsTime,   "day", 24.*60.*60. },
	{ SUOM_HOUR,      SUnit::clsTime,   "hour", 60.*60. },
	{ SUOM_SECOND,    SUnit::clsTime,   "second", 1.0 },
	{ SUOM_MINUTE,    SUnit::clsTime,   "minute", 60.0 },
	{ SUOM_ITEM,      SUnit::clsItem,   "item", 1.0 },
	{ SUOM_OZ,        SUnit::clsMass,   "oz", 0.0 },
	{ SUOM_COLOR,     SUnit::clsItem,   "color", 1.0 },
	{ SUOM_CMETER,    SUnit::clsLength, "centimeter", 1e-2 },
	{ SUOM_MMETER,    SUnit::clsLength, "millimeter", 1e-3 },
	{ SUOM_PERCENT,   SUnit::clsPart,   "percent", 1e-2 },
	{ SUOM_GR_PIXEL,  SUnit::clsLength, "pixel", 0.0 },
	{ SUOM_GR_PT,     SUnit::clsLength, "point", 2.54e-2 / 72.0 },
	{ SUOM_GR_PC,     SUnit::clsLength, "pc", 2.54e-2 / 6.0 },
	{ SUOM_GR_EM,     SUnit::clsLength, "em", 0.0 },
	{ SUOM_GR_EX,     SUnit::clsLength, "ex", 0.0 },
	{ SUOM_ONE,       SUnit::clsPart,   "one", 1.0 },
	{ SUOM_RADIAN,    SUnit::clsAngle,  "radian", 1.0 },
	{ SUOM_DEGREE,    SUnit::clsAngle,  "degree", SMathConst::PiDiv180 },
	{ SUOM_GR_DIALOG, SUnit::clsLength, "dlg", 0.0 },
	{ SUOM_TON,       SUnit::clsMass, "ton", 1000.0 }, // @v11.9.4 Тонна 
	{ SUOM_DMETER,    SUnit::clsLength, "dm", 0.1 }, // @v11.9.4 Дециметр
	{ SUOM_SQMETER,   SUnit::clsArea, "sqm", 1.0 }, // @v11.9.4 Квадратный метр
	{ SUOM_SQCMETER,  SUnit::clsArea, "sqcm", 10000.0 }, // @v11.9.4 Квадратный сантиметр
	{ SUOM_SQDMETER,  SUnit::clsArea, "sqdm", 100.0 },// @v11.9.4 Квадратный дециметр
	{ SUOM_QMETER,    SUnit::clsVolume, "qm", 1000.0 }, // @v11.9.4 Кубический метр
	{ SUOM_MLITER,    SUnit::clsVolume, "ml", 0.001 }, // @v11.9.4 Миллилитр
	{ SUOM_KWATTH,    SUnit::clsEnergy, "kwh", 0.0 }, // @v11.9.4 Киловатт в час
	{ SUOM_CALORIE,   SUnit::clsEnergy, "cal", 0.0 }, // @v11.9.4 Калория
	{ SUOM_KCALORIE,  SUnit::clsEnergy, "kcal", 0.0 }, // @v11.9.4 Килокалория
	{ SUOM_GCALORIE,  SUnit::clsEnergy, "gcal", 0.0 }, // @v11.9.4 Гигакалория
	{ SUOM_BIT,       SUnit::clsInformation, "bit", 0.0 }, // @v11.9.4 Бит
	{ SUOM_BYTE,      SUnit::clsInformation, "byte", 0.0 }, // @v11.9.4 Байт
	{ SUOM_KIBIBYTE,  SUnit::clsInformation, "kib", 0.0 }, // @v11.9.4 Кибибайт (1024 байт)
	{ SUOM_MEBIBYTE,  SUnit::clsInformation, "mib", 0.0 }, // @v11.9.4 Мебибайт (1024*1024 байт)
	{ SUOM_GIBIBYTE,  SUnit::clsInformation, "gib", 0.0 }, // @v11.9.4 Гибибайт (1024*1024*1024 байт)
	{ SUOM_TIBIBYTE,  SUnit::clsInformation, "tib", 0.0 }, // @v11.9.4 Тебибайт (1024*1024*1024*1024 байт)
};

/*static*/const void * FASTCALL SUnit::SearchEntry(int unitId)
{
	if(unitId)
		for(uint i = 0; i < SIZEOFARRAY(__Units); i++) {
			if(__Units[i].Unit == unitId) {
				return (__Units+i);
			}
		}
	return 0;
}

/*static*/const void * FASTCALL SUnit::SearchClsEntry(int cls)
{
	if(cls)
		for(uint i = 0; i < SIZEOFARRAY(__Cls); i++) {
			if(__Cls[i].Cls == cls) {
				return (__Cls+i);
			}
		}
	return 0;
}

SUnit::Context::Context()
{
}

SUnit::Context::~Context()
{
}

int SUnit::Context::Describe(int unitId, int dir, int * pCls, double * pToBase, SString * pName) const
{
	const UnitEntry * p_entry = (const UnitEntry *)SUnit::SearchEntry(unitId);
	if(p_entry) {
		ASSIGN_PTR(pCls, p_entry->Cls);
		ASSIGN_PTR(pToBase, p_entry->ToBase);
		ASSIGN_PTR(pName, p_entry->P_Name);
		return 1;
	}
	else {
		ASSIGN_PTR(pCls, 0);
		ASSIGN_PTR(pToBase, 0.0);
		ASSIGN_PTR(pName, 0);
		return 1;
	}
}

/*static*/USize & SUnit::Convert(const USize & rUszFrom, USize & rUszTo, const SUnit::Context * pCtx)
{
	int    ok = 0;
	rUszTo.S = 0.0;
	if(!!rUszFrom) {
		int    from_cls = 0, to_cls = 0;
		double from_base = 0.0, to_base = 0.0;
		const SUnit::Context def_ctx;
		int    rf = NZOR(pCtx, &def_ctx)->Describe(rUszFrom.Unit, 0, &from_cls, &from_base, 0);
		int    tf = NZOR(pCtx, &def_ctx)->Describe(rUszTo.Unit, 0, &to_cls, &to_base, 0);
		if(rf && tf && from_cls == to_cls && from_base != 0.0 && to_base != 0.0) {
			rUszTo.S = rUszFrom.S * from_base / to_base;
			rUszTo.Dir = rUszFrom.Dir;
			ok = 1;
		}
	}
	if(!ok)
		rUszTo.SetInvalid();
	return rUszTo;
}

SUnit::SUnit() : Id(0)
{
}

SUnit::SUnit(int id) : Id(id)
{
}

SUnit::SUnit(const char * pName, const SUnit::Context * pCtx) : Id(0)
{
}

SUnit::operator int () const
{
	return Id;
}

int SUnit::GetCls() const
{
	const UnitEntry * p_entry = (const UnitEntry *)SearchEntry(Id);
	return p_entry ? p_entry->Cls : 0;
}

int SUnit::GetName(long flags, SString & rBuf)
{
	rBuf.Z();
	const UnitEntry * p_entry = (const UnitEntry *)SearchEntry(Id);
	if(p_entry) {
		rBuf = p_entry->P_Name;
		return 1;
	}
	else
		return 0;
}
//
//
//
USize & USize::Set(double s, int unit, int dir)
{
	S = s;
	Unit = (int16)unit;
	Dir = (int16)dir;
	return *this;
}

void USize::SetInvalid()
{
	S = 0.0;
	Unit = 0;
	Dir = -1;
}

bool USize::IsValid() const { return (SIEEE754::IsValid(S) && Dir >= 0); }

int USize::FromStr(const char * pStr, int fmt)
{
	int    ok = 0;
	S = 0.0;
	Unit = 0;
	Dir = 0;
	SString temp_buf;
	SStrScan scan(pStr);
	if(scan.Skip().GetDotPrefixedNumber(temp_buf)) {
		S = temp_buf.ToReal();
		ok = 1;
		// @note: единицы измерения должны следовать после значения без пробелов
		if(scan.GetIdent(temp_buf)) {
			//                              1  2  3  4  5  6  7  8  9
			int    u = temp_buf.OneOf(';', "px;pt;in;cm;mm;pc;em;ex;%", 1);
			switch(u) {
				case 1: Unit = SUOM_GR_PIXEL; break;
				case 2: Unit = SUOM_GR_PT; break;
				case 3: Unit = SUOM_INCH; break;
				case 4: Unit = SUOM_CMETER; break;
				case 5: Unit = SUOM_MMETER; break;
				case 6: Unit = SUOM_GR_PT; break;
				case 7: Unit = SUOM_GR_EM; break;
				case 8: Unit = SUOM_GR_EX; break;
				case 9: Unit = SUOM_PERCENT; break;
			}
			if(Unit)
				ok = 2;
		}
	}
	return ok;
}
