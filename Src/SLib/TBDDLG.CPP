// TBDDLG.CPP
// ..., 2008
//
#include <slib.h>
#pragma hdrstop
#include <tv.h>

TabbedDialog * TabbedDialog::topTabbedDialog = 0; // @global

TabbedDialog::TabbedDialog(char * aName,TabbedDialogPage *aPage,int aNum)
{
	num   = aNum;
	name  = newStr(aName);
	pages = new TabbedDialogPage[aNum];
	memcpy(pages,aPage,sizeof(TabbedDialogPage)*num);
	for(int i = 0; i < num; i++) {
		pages[i].dialog = 0;
		pages[i].hWnd = 0;
	}
	prevTabbedDialog =topTabbedDialog;
	topTabbedDialog = this;
}

TabbedDialog::~TabbedDialog()
{
	delete pages;
	delete name;
	topTabbedDialog=prevTabbedDialog;
}

ushort TabbedDialog::execute(void)
{
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE *handles=new HPROPSHEETPAGE[num];
	for(int i = 0; i < num; i++) {
		psp.dwSize  = sizeof(psp);
		psp.dwFlags = PSP_USETITLE;
		psp.hInstance   = TProgram::GetInst();
		psp.pszTemplate = MAKEINTRESOURCE(pages[i].id);
		psp.hIcon = 0;
		psp.pszTitle = pages[i].tabName;
		OemToChar(psp.pszTitle,(char *)psp.pszTitle);
		psp.pfnDlgProc = (DLGPROC)PropertySheetDialogProc;
		psp.lParam = (long)i;
		psp.pfnCallback = 0;
		psp.pcRefParent = 0;
		handles[i] = CreatePropertySheetPage(&psp);
	}
	PROPSHEETHEADER psh;
	psh.dwSize  = sizeof(psh);
	psh.dwFlags = PSH_NOAPPLYNOW;
	psh.hwndParent = APPL->H_MainWnd;
	psh.hInstance  = TProgram::GetInst();
	psh.hIcon = 0;
	psh.pszCaption = name;
	OemToChar(psh.pszCaption, (char *)psh.pszCaption);
	psh.nPages = num;
	psh.nStartPage = 0;
	psh.phpage = handles;
	psh.pfnCallback = 0;

	PropertySheet(&psh);
	delete handles;
	return lastCommand;
}

HWND TabbedDialog::setupPage(int page, TDialog * pDlg)
{
	pages[page].dialog = pDlg;
	lastCommand = 0;
	return pages[page].hWnd;
}

void TabbedDialog::clearPage(int page)
{
	pages[page].dialog = 0;
}

int TabbedDialog::findPageByHandle(HWND h)
{
	int     i = 0;
	while(i < num && pages[i].hWnd != h)
		++i;
	return (i == num) ? -1 : i;
}

int TabbedDialog::findPageByID(long id)
{
	int     i = 0;
	while(i < num && pages[i].id != id)
		++i;
	return (i == num) ? -1 : i;
}
