// ComboDialog.h

#ifndef __COMBO_DIALOG_H
#define __COMBO_DIALOG_H

#include <ComboBox.h>
#include <Dialog.h>
#include "ComboDialogRes.h"

class CComboDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox _comboBox;
  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
public:
  // bool Sorted;
  UString Title;
  UString Static;
  UString Value;
  UStringVector Strings;
  
  // CComboDialog(): Sorted(false) {};
  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_COMBO, parentWindow); }
};

#endif
