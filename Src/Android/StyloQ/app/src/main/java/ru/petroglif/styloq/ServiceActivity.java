// ServiceActivity.java
// Copyright (c) A.Sobolev 2021
//
package ru.petroglif.styloq;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

public class ServiceActivity extends SLib.SlActivity {
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_service);
				break;
		}
		return result;
	}
}