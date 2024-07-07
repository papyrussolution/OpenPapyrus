// NetworkConnectionInfoManager.java
//
package ru.petroglif.styloq;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;

public class NetworkConnectionInfoManager {
	public enum Transport {
		Unavailable, // Сеть недоступна
		GenericAvailable, // Сеть доступна (без детализации)
		Ethernet, // Доступна сеть ethernet
		WIFI, // Доступна сеть wifi
		Cellular, // Доступна сеть посредством сотового оператора
	}
	public static class Status {
		Status()
		{
			Transp = Transport.Unavailable;
			DownloadBandwithKbps = 0;
			UploadBandwithKbps = 0;
			CellularQuality = 0;
		}
		public Transport Transp;
		public int DownloadBandwithKbps;
		public int UploadBandwithKbps;
		public int CellularQuality; // 0 - no info, 1 - unstable, 2 - 2G, 3 - 3G, 4 - 4G
	}
	private Context Ctx;
	private SLib.EventHandler Handler;
	private static ConnectivityManager.NetworkCallback Cb;
	NetworkConnectionInfoManager(Context ctx, SLib.EventHandler handler)
	{
		Ctx = ctx;
		Handler = handler;
		ConnectivityManager cmgr = Ctx.getSystemService(ConnectivityManager.class);
		if(cmgr != null) {
			if(Cb != null) {
				cmgr.unregisterNetworkCallback(Cb);
				Cb = null;
			}
			Network current_network = cmgr.getActiveNetwork();
			Cb = new Callback(Ctx, Handler);
			cmgr.registerDefaultNetworkCallback(Cb);
		}
	}
	void Unregister()
	{
		if(Cb != null) {
			ConnectivityManager cmgr = Ctx.getSystemService(ConnectivityManager.class);
			if(cmgr != null) {
				cmgr.unregisterNetworkCallback(Cb);
				Cb = null;
			}
		}
	}
	private static class Callback extends ConnectivityManager.NetworkCallback {
		private Context Ctx;
		private SLib.EventHandler Handler;
		public Callback(Context ctx, SLib.EventHandler handler)
		{
			Ctx = ctx;
			Handler = handler;
		}
		@Override public void onAvailable(Network network)
		{
			Status st = new Status();
			st.Transp = Transport.GenericAvailable;
			SendData(st);
		}
		@Override public void onLost(Network network)
		{
			Status st = new Status();
			st.Transp = Transport.Unavailable;
			SendData(st);
		}
		@Override public void onCapabilitiesChanged(Network network, NetworkCapabilities networkCapabilities)
		{
			Status st = new Status();
			st.Transp = Transport.GenericAvailable;
			st.DownloadBandwithKbps = networkCapabilities.getLinkDownstreamBandwidthKbps();
			st.UploadBandwithKbps = networkCapabilities.getLinkUpstreamBandwidthKbps();
			if(networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) {
				st.Transp = Transport.Ethernet;
			}
			else if(networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
				st.Transp = Transport.WIFI;
			}
			else if(networkCapabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) {
				st.Transp = Transport.Cellular;
				if(st.DownloadBandwithKbps < 15) {
					st.CellularQuality = 1; // unstable
				}
				else if(st.DownloadBandwithKbps < 300) {
					st.CellularQuality = 2; // 2G
				}
				else if(st.DownloadBandwithKbps < 45000) {
					st.CellularQuality = 3; // 3G
				}
				else /*if(st.DownloadBandwithKbps < 150000)*/{
					st.CellularQuality = 4; // 4G
				}
			}
			SendData(st);
		}
		private void SendData(Status data)
		{
			((SLib.SlActivity)Handler).runOnUiThread(new Runnable() {
				@Override public void run()
				{
					Handler.HandleEvent(SLib.EV_ASYNCSET, "NetworkConnectionStatus", data);
				}
			});
		}
	}
}
