package com.example.testscan;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.device.ScanDevice;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;
import java.io.UnsupportedEncodingException;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

public class TestScanActivity extends Activity {
    ScanDevice sm;
    private final static String SCAN_ACTION="scan.rcv.message";
    private String barcodeStr;
    private EditText showScanResult;
    private int startNum=0;
    private int endNum=0;

    private BroadcastReceiver mScanReceiver = new BroadcastReceiver()
    {
        @Override public void onReceive(Context context, Intent intent)
        {
            Log.e("TAG", "onReceive: "+intent.getAction ());
            String action = intent.getAction();
            if(action.equals(SCAN_ACTION)){
                byte[] barocode=intent.getByteArrayExtra ("barocode");
                int barocodelen=intent.getIntExtra ("length", 0);
                byte temp=intent.getByteExtra ("barcodeType", (byte) 0);
                byte[] aimid=intent.getByteArrayExtra ("aimid");
                barcodeStr=new String (barocode, 0, barocodelen);
//                try {
//                    barcodeStr=new String (barocode, "SHIFT-JIS");
//                }catch (UnsupportedEncodingException e){
//
//                }
                showScanResult.append(barcodeStr);
                showScanResult.append("\n");
                sm.stopScan();
                UtilSound.play ();
            }

        }
    };

    @Override protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate (savedInstanceState);
        setContentView (R.layout.testcan_main_activity);
        UtilSound.initSoundPool (this);
        sm = new ScanDevice();
        boolean bool = sm.setOutScanMode (0);//启动就是广播模式
        Log.e ("TAG", "onCreate: "+bool);
        Spinner spinner=(Spinner) findViewById (R.id.spinner);
        String[] arr = {
            getString (R.string.brodcast_mode),
            getString (R.string.input_mode)
        };
        //创建ArrayAdapter对象
        ArrayAdapter<String> adapter=new ArrayAdapter<String> (this, android.R.layout.simple_spinner_item, arr);
        adapter.setDropDownViewResource (android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter (adapter);
        spinner.setOnItemSelectedListener (new AdapterView.OnItemSelectedListener () {
            @Override public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
            {
                if(position == 0) {
                    sm.setOutScanMode (0);
                }
                else if (position == 1) {
                    sm.setOutScanMode (1);
                }
            }
            @Override public void onNothingSelected(AdapterView<?> parent)
            {
            }
        });
        showScanResult = (EditText)findViewById (R.id.editText1);
    }

    public void onClick(View v)
    {
        switch(v.getId()) {
            case R.id.openScanner: sm.openScan(); break;
            case R.id.closeScanner: sm.closeScan(); break;
            case R.id.startDecode: sm.startScan(); break;
            case R.id.stopDecode: sm.stopScan(); break;
            case R.id.start_continue: sm.setScanLaserMode(4); break;
            case R.id.stop_continue: sm.setScanLaserMode(8); break;
            case R.id.close: finish(); break;
            case R.id.clear: showScanResult.setText(""); break;
            default: break;
        }
    }

    @Override protected void onPause()
    {
        super.onPause ();
        unregisterReceiver (mScanReceiver);
    }

    @Override protected void onResume()
    {
        super.onResume ();
        IntentFilter filter=new IntentFilter();
        filter.addAction(SCAN_ACTION);
        registerReceiver (mScanReceiver, filter);
    }

    @Override protected void onDestroy()
    {
        super.onDestroy ();
        if(sm != null) {
            sm.stopScan ();
            sm.setScanLaserMode (8);
            sm.closeScan ();
        }
    }
}