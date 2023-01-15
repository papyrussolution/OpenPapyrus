package ru.petroglif.styloq;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

import androidx.core.content.ContextCompat;

//
// Note: Заимствовано из проекта StyloAgent. Необходимы корректировки!
//
public class StyloQLocationListener implements LocationListener {
	public final static int STATE_PROVIDERENABLED = 0x0001;
	public final static int STATE_DBERROR         = 0x0002;
	public final static int STATE_NETWORKPRVDR    = 0x0004;
	public final static int STATE_RESERVEPRVDR    = 0x0008; // Состояние устанавливается у резервного слушателя, следящего за включением
	// сетевого провайдера.
	//
	// Descr: Флаги инициализации StyloLocationListener
	//
	public final static int fInitDatabase       = 0x0001;
	public final static int fSingle             = 0x0002;
	public final static int fDontDbRegister     = 0x0004;
	public final static int fUseNetworkPrvdr    = 0x0008;
	public final static int fAllowDisabledPrvdr = 0x0010;
	public final static int fDisabledPrvdr      = 0x0020;

	//private Location LastLoc;
	private int State;
	private int FixCount; // Количество фиксаций координат

	private int Flags;   // @flags fXXX
	private int ExtObjType;
	private int ExtObjID;
	private StyloQApp AppCtx;
	//private StyloQDatabase.GeoTrackTable T;
	private String PrvdrName;
	//
	// Параметры конфигурации, к которым чувствителен объект (требуется перезапуск при изменении)
	//
	//private int    GeoTrackingMode;
	private int    GeoTrackingCycle;

	private static StyloQLocationListener L;
	private static StyloQLocationListener ReserveL;

	public StyloQLocationListener(StyloQApp appCtx, String prvdrName, int flags, int extObjType, int extObjID)
	{
		AppCtx = appCtx;
		Flags = flags;
		ExtObjType = extObjType;
		ExtObjID = extObjID;
		/*
		StyloQDatabase.ConfigTable.Rec cfg = AppCtx.GetCfg();
		//GeoTrackingMode = cfg.GeoTrackingMode;
		GeoTrackingCycle = cfg.GeoTrackingCycle;
		State = 0;
		if((Flags & fUseNetworkPrvdr) != 0)
			State |= STATE_NETWORKPRVDR;
		PrvdrName = prvdrName;
		StyloQDatabase db = AppCtx.GetDB();
		try {
			if((Flags & fInitDatabase) != 0) {
				db.StartTransaction();
				db.CreateTableOnDb(null, StyloDatabase.GeoTrackTable.TBL_NAME, false);
				db.CommitWork();
			}
			T = (StyloDatabase.GeoTrackTable)db.CreateTable(StyloDatabase.GeoTrackTable.TBL_NAME);
		} catch(Exception e) {
			if((Flags & fInitDatabase) != 0 && db.InTransaction())
				db.RollbackWork();
			State |= STATE_DBERROR;
			AppCtx.SetLastError(0, e.getMessage(), null);
		}
		 */
	}
	public String GetProviderName()
	{
		return PrvdrName;
	}
	public static String GetShortLocationInfo(final Location loc)
	{
		return loc.getProvider()+"=["+loc.getLatitude()+","+loc.getLongitude()+"] alt("+loc.getAltitude()+") speed("+loc.getSpeed()+")";
	}
	public static String GetStatusText()
	{
		String text;
		if(L != null) {
			text = "Provider";
			if((L.State & STATE_NETWORKPRVDR) != 0)
				text = text + " " + "NETWORK";
			else
				text = text + " " + "GPS";
			if((L.State & STATE_PROVIDERENABLED) != 0) {
				text = text + " " + "on";
			}
			else {
				text = text + " " + "off";
			}
			if(L.GeoTrackingCycle > 0) {
				text = text + "\n" + "цикл запросов положения " + L.GeoTrackingCycle + "ms";
			}
			text = text + "\n" + L.FixCount + " positions was fixed";
		}
		else {
			text = "Geo-location is off";
		}
		return text;
	}
	static class SelectProviderBlock {
		public SelectProviderBlock(LocationManager locMgr, int options)
		{
			Flags = 0;
			if((options & fUseNetworkPrvdr) != 0)
				Name = LocationManager.NETWORK_PROVIDER;
			else
				Name = LocationManager.GPS_PROVIDER;
			if(!locMgr.isProviderEnabled(Name)) {
				Flags |= fDisabledPrvdr;
			}
		}
		public String Name;
		public int Flags;
	}
	private static StyloQLocationListener CreateEnabledListener(StyloQApp appCtx, LocationManager locMgr, int flags)
	{
		StyloQLocationListener listener = null;
		SelectProviderBlock spb = new SelectProviderBlock(locMgr, 0);
		if((spb.Flags & fDisabledPrvdr) == 0) {
			listener = new StyloQLocationListener(appCtx, spb.Name, flags & ~fUseNetworkPrvdr, 0, 0);
			listener.State |= STATE_PROVIDERENABLED;
		}
		else {
			spb = new SelectProviderBlock(locMgr, fUseNetworkPrvdr);
			if((spb.Flags & fDisabledPrvdr) == 0) {
				listener = new StyloQLocationListener(appCtx, spb.Name, flags | fUseNetworkPrvdr, 0, 0);
				listener.State |= STATE_PROVIDERENABLED;
			}
		}
		return listener;
	}
	public static Location RegisterSingle(StyloQApp appCtx, int flags, int extObjType, int extObjID, int timeout)
	{
		Location loc = null;
		final LocationManager loc_mgr = (LocationManager)appCtx.getSystemService(Context.LOCATION_SERVICE);
		final StyloQLocationListener listener = CreateEnabledListener(appCtx, loc_mgr, flags);
		if(listener != null) {
			//<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION"/>
			//<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
			//<uses-permission android:name="android.permission.ACCESS_BACKGROUND_LOCATION"/>
			//<uses-permission android:name="android.permission.ACCESS_MOCK_LOCATION" />
			boolean is_fineloc_allowed = false;
			boolean is_coarseloc_allowed = false;
			boolean is_bkgloc_allowed = false;
			if(ContextCompat.checkSelfPermission(appCtx, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
				is_fineloc_allowed = true;
			}
			if(ContextCompat.checkSelfPermission(appCtx, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
				is_coarseloc_allowed = true;
			}
			if(ContextCompat.checkSelfPermission(appCtx, Manifest.permission.ACCESS_BACKGROUND_LOCATION) == PackageManager.PERMISSION_GRANTED) {
				is_bkgloc_allowed = true;
			}
			/*if(!is_fineloc_allowed || !is_coarseloc_allowed) {
				ActivityCompat.requestPermissions(appCtx, new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, PERMISSION_REQUEST);
			}*/
			if(is_fineloc_allowed || is_coarseloc_allowed) {
				loc_mgr.requestSingleUpdate(listener.GetProviderName(), listener, null);
				loc = loc_mgr.getLastKnownLocation(listener.GetProviderName());
				if(loc != null) {
					listener.Helper_RegisterLocation(loc, extObjType, extObjID);
				}
			}
			return loc;
		}
		else
			return null;
	}
	public static int Setup(StyloQApp appCtx, int flags)
	{
		int    ok = 1;
		/*
		StyloQDatabase.ConfigTable.Rec cfg = appCtx.GetCfg();
		LocationManager loc_mgr = (LocationManager)appCtx.getSystemService(Context.LOCATION_SERVICE);
		if(ReserveL != null) {
			loc_mgr.removeUpdates(ReserveL);
			ReserveL = null;
		}
		if(L != null) {
			loc_mgr.removeUpdates(L);
			L = null;
		}
		if((cfg.GeoTrackingMode & StyloQDatabase.ConfigTable.GTMF_AUTO) != 0) {
			int    cycle = (cfg.GeoTrackingCycle > 0) ? cfg.GeoTrackingCycle : 900000;
			L = CreateEnabledListener(appCtx, loc_mgr, flags);
			if(L == null) {
				{
					SelectProviderBlock spb = new SelectProviderBlock(loc_mgr, 0);
					L = new StyloQLocationListener(appCtx, spb.Name, flags & ~fUseNetworkPrvdr, 0, 0);
					L.State &= ~STATE_PROVIDERENABLED;
				}
				{
					SelectProviderBlock spb = new SelectProviderBlock(loc_mgr, fUseNetworkPrvdr);
					ReserveL = new StyloQLocationListener(appCtx, spb.Name, (flags | fUseNetworkPrvdr) & ~fInitDatabase, 0, 0);
					ReserveL.State &= ~STATE_PROVIDERENABLED;
					ReserveL.State |= STATE_RESERVEPRVDR;
					loc_mgr.requestLocationUpdates(ReserveL.GetProviderName(), 1000000000, 10000, ReserveL);
				}
			}
			if(L != null) {
				loc_mgr.requestLocationUpdates(L.GetProviderName(), cycle, 0, L);
			}
		}*/
		return ok;
	}
	public static StyloQLocationListener Get() { return L; }
	private void Helper_RegisterLocation(Location loc, int extObjType, int extObjID)
	{
		/*
		StyloQDatabase db = AppCtx.GetDB();
		if(db != null) {
			StyloQDatabase.GeoTrackTable.Rec rec = T.CreateRec();
			rec.Latitude = loc.getLatitude();
			rec.Longitude = loc.getLongitude();
			rec.Altitude = loc.getAltitude();
			rec.Speed = loc.getSpeed();
			Util.LDATETIME dtm = new Util.LDATETIME();
			dtm.SetTimeUtcMs70(loc.getTime());
			rec.Dt = dtm.d.v;
			rec.Tm = dtm.t.v;
			rec.Flags = 0;
			if((State & STATE_NETWORKPRVDR) != 0)
				rec.Flags |= 0x0001;
			rec.ExtObjType = extObjType;
			rec.ExtObjID = (extObjType != 0) ? extObjID : 0;
			try {
				db.StartTransaction();
				long id = db.InsertRec(T, rec);
				if(id > 0)
					db.CommitWork();
				else
					db.RollbackWork();
			}
			catch(Exception e) {
				State |= STATE_DBERROR;
				AppCtx.SetLastError(0, e.getMessage(), null);
				if(db.InTransaction())
					db.RollbackWork();
			}
		}
		 */
	}
	@Override public void onLocationChanged(Location loc)
	{
		//LastLoc = loc;
		FixCount++;
		if((State & STATE_RESERVEPRVDR) == 0) {
			if((Flags & fDontDbRegister) == 0) {
				if(/*T != null*/true) {
					Helper_RegisterLocation(loc, ExtObjType, ExtObjID);
				}
				else
					State |= STATE_DBERROR;
			}
			if((Flags & fSingle) != 0) {
				LocationManager loc_mgr = (LocationManager)AppCtx.getSystemService(Context.LOCATION_SERVICE);
				loc_mgr.removeUpdates(this);
			}
		}
	}
	@Override public void onProviderDisabled(String provider)
	{
		if(provider.equalsIgnoreCase(PrvdrName) && (State & STATE_PROVIDERENABLED) != 0) {
			StyloQLocationListener.Setup(AppCtx, 0);
		}
	}
	@Override public void onProviderEnabled(String provider)
	{
		if(provider.equalsIgnoreCase(PrvdrName) && (State & STATE_PROVIDERENABLED) == 0) {
			StyloQLocationListener.Setup(AppCtx, 0);
		}
	}
	@Override public void onStatusChanged(String provider, int status, Bundle extras)
	{
	}
}
