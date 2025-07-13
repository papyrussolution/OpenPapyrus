// V_OUTER.CPP
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Реализация класса(-ов) PPView для данных, полученных из внешних источников
//
#include <pp.h>
#pragma hdrstop

#if 1 // @v12.3.8 @construction {

class PPViewWbPublicGoods : public PPView {
public:
	PPViewWbPublicGoods();
	~PPViewWbPublicGoods();
	virtual int  ProcessCommand(uint ppvCmd, const void *, PPViewBrowser *);
	virtual int  EditBaseFilt(PPBaseFilt *);
	virtual PPBaseFilt * CreateFilt(const void * extraPtr) const;
	virtual int  Init_(const PPBaseFilt * pBaseFilt);
private:
	virtual SArray * CreateBrowserArray(uint * pBrwId, SString * pSubTitle);
	virtual int   OnExecBrowser(PPViewBrowser *);
	virtual void  PreprocessBrowser(PPViewBrowser * pBrw);
	virtual void * GetEditExtraParam();

	MarketplaceGoodsSelectionFilt Filt;
};

PPViewWbPublicGoods::PPViewWbPublicGoods() : PPView(0, &Filt, PPVIEW_WBPUBLICGOODS, implBrowseArray, 0)
{
}

/*virtual*/PPViewWbPublicGoods::~PPViewWbPublicGoods()
{
}

/*virtual*/int PPViewWbPublicGoods::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	return ok;
}

/*virtual*/PPBaseFilt * PPViewWbPublicGoods::CreateFilt(const void * extraPtr) const
{
	return 0;
}

/*virtual*/int PPViewWbPublicGoods::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 0;
	return ok;
}

/*virtual*/SArray * PPViewWbPublicGoods::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = 0;
	//THROW(MakeList(0));
	//p_array = new SArray(*P_DsList);
	CATCH
		ZDELETE(p_array);
		//ZDELETE(P_DsList);
	ENDCATCH
	ASSIGN_PTR(pBrwId, 0/*resourceID*/);
	return p_array;
}

#endif // } @v12.3.8 @construction