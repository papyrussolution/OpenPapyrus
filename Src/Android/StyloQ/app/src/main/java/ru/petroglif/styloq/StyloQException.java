// StyloQException.java
// Copyright (c) A.Sobolev 2021
//
package ru.petroglif.styloq;

import android.content.Context;
import android.database.SQLException;

public class StyloQException extends Exception {
	private static final long serialVersionUID = 1L;
	private String Msg;
	private int Group; // Группа кодирования ошибки. По умолчанию (если 0) ppstr2.PPSTR_ERROR
	private int ErrCode;
	private String AddedInfo;
	public StyloQException()
	{
	}
	public StyloQException(String msg)
	{
		Msg = msg;
		ErrCode = 0;
		AddedInfo = null;
	}
	public StyloQException(int errCode, String addInfo)
	{
		ErrCode = errCode;
		AddedInfo = addInfo;
	}
	public StyloQException(int group, int errCode, String addInfo)
	{
		Group = group;
		ErrCode = errCode;
		AddedInfo = addInfo;
	}
	public StyloQException(Context ctx, int errCode, String addInfo)
	{
		ErrCode = errCode;
		AddedInfo = addInfo;
		StyloQApp.SetLastError(ctx, errCode, null, addInfo);
	}
	public StyloQException(Context ctx, String errMsg, String addInfo)
	{
		Msg = errMsg;
		ErrCode = 0;
		AddedInfo = addInfo;
		StyloQApp.SetLastError(ctx, 0, errMsg, addInfo);
	}
	public StyloQException(Context ctx, int errCode, String errMsg, String addInfo)
	{
		Msg = errMsg;
		ErrCode = errCode;
		AddedInfo = addInfo;
		StyloQApp.SetLastError(ctx, errCode, errMsg, addInfo);
	}
	public int GetErrCode() { return ErrCode; }
	public String GetAddedInfo() { return AddedInfo; }
	public String GetMessage(Context ctx)
	{
		return (ctx != null) ? ((StyloQApp)ctx).GetErrorText(ErrCode, AddedInfo) : "no-context";
	}
	@Override
	public String getMessage()
	{
		return (Msg != null) ? Msg : "";
	}
}
