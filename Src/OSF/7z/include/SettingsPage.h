// SettingsPage.h
 
#ifndef __SETTINGS_PAGE_H
#define __SETTINGS_PAGE_H

#include <PropertyPage.h>
#include <Edit.h>

class CSettingsPage: public NWindows::NControl::CPropertyPage
{
  bool _wasChanged;

  bool _largePages_wasChanged;

  // void EnableSubItems();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual LONG OnApply();
};

#endif
