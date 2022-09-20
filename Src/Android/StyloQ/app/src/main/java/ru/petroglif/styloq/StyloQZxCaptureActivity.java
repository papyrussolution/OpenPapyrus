package ru.petroglif.styloq;

import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import androidx.annotation.NonNull;
import com.journeyapps.barcodescanner.CaptureActivity;
import com.journeyapps.barcodescanner.CaptureManager;
import com.journeyapps.barcodescanner.DecoratedBarcodeView;
import com.journeyapps.barcodescanner.ViewfinderView;
import java.util.Random;

public class StyloQZxCaptureActivity extends CaptureActivity implements DecoratedBarcodeView.TorchListener {
	private CaptureManager Capture;
	private DecoratedBarcodeView ViewBarcodeScanner;
	private Button ButtonSwitchFlashlight;
	private ViewfinderView ViewViewfinder;

	@Override protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_zx);
		ViewBarcodeScanner = findViewById(R.id.zxing_barcode_scanner);
		ViewBarcodeScanner.setTorchListener(this);
		ButtonSwitchFlashlight = findViewById(R.id.switch_flashlight);
		ViewViewfinder = findViewById(R.id.zxing_viewfinder_view);
		// if the device does not have flashlight in its camera,
		// then remove the switch flashlight button...
		if(!hasFlash()) {
			ButtonSwitchFlashlight.setVisibility(View.GONE);
		}
		Capture = new CaptureManager(this, ViewBarcodeScanner);
		Capture.initializeFromIntent(getIntent(), savedInstanceState);
		Capture.setShowMissingCameraPermissionDialog(false);
		Capture.decode();
		changeMaskColor(null);
		changeLaserVisibility(true);
	}
	public void OnSwitchFlashlight(View view)
	{
		if(getString(R.string.turn_on_flashlight).equals(ButtonSwitchFlashlight.getText())) {
			ViewBarcodeScanner.setTorchOn();
		}
		else {
			ViewBarcodeScanner.setTorchOff();
		}
	}
	public void OnBack(View view)
	{
		finish();
	}
	@Override protected void onResume()
	{
		super.onResume();
		Capture.onResume();
	}
	@Override protected void onPause()
	{
		super.onPause();
		Capture.onPause();
	}
	@Override protected void onDestroy()
	{
		super.onDestroy();
		Capture.onDestroy();
	}
	@Override protected void onSaveInstanceState(Bundle outState)
	{
		super.onSaveInstanceState(outState);
		Capture.onSaveInstanceState(outState);
	}
	@Override public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		return ViewBarcodeScanner.onKeyDown(keyCode, event) || super.onKeyDown(keyCode, event);
	}
	/**
	 * Check if the device's camera has a Flashlight.
	 * @return true if there is Flashlight, otherwise false.
	 */
	private boolean hasFlash()
	{
		return getApplicationContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_FLASH);
	}
	public void switchFlashlight(View view)
	{
		if(getString(R.string.turn_on_flashlight).equals(ButtonSwitchFlashlight.getText())) {
			ViewBarcodeScanner.setTorchOn();
		}
		else {
			ViewBarcodeScanner.setTorchOff();
		}
	}
	public void changeMaskColor(View view)
	{
		Random rnd = new Random();
		int color = Color.argb(100, rnd.nextInt(256), rnd.nextInt(256), rnd.nextInt(256));
		ViewViewfinder.setMaskColor(color);
	}
	public void changeLaserVisibility(boolean visible) { ViewViewfinder.setLaserVisibility(visible); }
	@Override public void onTorchOn() { ButtonSwitchFlashlight.setText(R.string.turn_off_flashlight); }
	@Override public void onTorchOff() {
		ButtonSwitchFlashlight.setText(R.string.turn_on_flashlight);
	}
	@Override public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
	{
		Capture.onRequestPermissionsResult(requestCode, permissions, grantResults);
	}
}
