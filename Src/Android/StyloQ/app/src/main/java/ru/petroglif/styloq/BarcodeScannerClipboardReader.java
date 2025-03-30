// BarcodeScannerClipboardListener.java
// Copyright (c) A.Sobolev 2025
//
package ru.petroglif.styloq;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;

public class BarcodeScannerClipboardReader {
	private final Context Ctx;
	private SLib.EventHandler Handler;
	private ClipboardManager CbMgr;
	private ClipboardManager.OnPrimaryClipChangedListener Listener;

	BarcodeScannerClipboardReader(Context ctx, SLib.EventHandler handler)
	{
		Ctx = ctx;
		Handler = handler;
		Listener = null;
		if(Ctx != null) {
			CbMgr = (ClipboardManager)Ctx.getSystemService(Context.CLIPBOARD_SERVICE);
		}
	}
	public BusinessEntity.PreprocessBarcodeResult GetBarcodeFromClipboard()
	{
		BusinessEntity.PreprocessBarcodeResult result = null;
		if(CbMgr != null && CbMgr.hasPrimaryClip()) {
			ClipData.Item item = CbMgr.getPrimaryClip().getItemAt(0);
			if(item != null) {
				CharSequence cb_content = item.getText();
				if(cb_content != null && cb_content.length() > 0) {
					String code_potential = cb_content.toString();
					result = BusinessEntity.PreprocessBarcode(code_potential, null, null);
				}
			}
		}
		return result;
	}
	public void RegisterListener()
	{
		UnregisterListener();
		if(Handler != null) {
			if(CbMgr != null) {
				CbMgr.clearPrimaryClip();
				Listener = new ClipboardManager.OnPrimaryClipChangedListener() {
					@Override
					public void onPrimaryClipChanged()
					{
						if(Handler != null && CbMgr != null) {
							BusinessEntity.PreprocessBarcodeResult bcr = GetBarcodeFromClipboard();
							if(bcr != null) {
								CbMgr.clearPrimaryClip();
								// @v12.2.12 Handler.HandleEvent(SLib.EV_BARCODERECEIVED, "clipboard", bcr);
								// @v12.2.12 {
								((SLib.SlActivity)Handler).runOnUiThread(new Runnable() {
									@Override public void run()
									{
										Handler.HandleEvent(SLib.EV_BARCODERECEIVED, "clipboard", bcr);
									}
								});
								// } @v12.2.12
								//ClipDescription cd = CbMgr.getPrimaryClipDescription();
							}
						}
					}
				};
				CbMgr.addPrimaryClipChangedListener(Listener);
			}
		}
	}
	public void UnregisterListener()
	{
		if(Listener != null) {
			if(CbMgr != null) {
				CbMgr.removePrimaryClipChangedListener(Listener);
			}
			Listener = null;
		}
	}
}
