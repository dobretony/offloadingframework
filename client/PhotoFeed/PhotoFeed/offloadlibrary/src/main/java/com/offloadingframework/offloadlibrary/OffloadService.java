package com.offloadingframework.offloadlibrary;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * Created by tony on 09.06.2015.
 */
@TargetApi(21)
public class OffloadService extends Service {

    private BluetoothAdapter mBluetoothAdapter = null;
    private Context mContext = null;

    private static final String LOG_NAME = "OffloadClient";
    private static UUID[] serverUUIDs = null;

    private Looper mServiceLooper;
    private OffloadServiceHandler mServiceHandler;

    private static long SCAN_THREAD_DELAY = 15000;

    private BluetoothGatt mGatt;
    private BluetoothLeScanner mLEScanner;
    private ScanSettings settings;
    private ArrayList<ScanFilter> filters;

    /**
     * Variable that states if an offload server is nearby.
     */
    private boolean foundServer = false;

    private BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {
                @Override
                public void onLeScan(BluetoothDevice device, int rssi, byte[] scanRecord) {
                    Log.d(LOG_NAME, "Found advertisement: " + rssi + " " + device.getAddress());
                }
            };

    private ScanCallback mScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            Log.d(LOG_NAME, "Found advertisement: " + result.toString());
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {

        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.d(LOG_NAME, "Scan failed.");
        }
    };


    private Runnable scanForServerRunnable = new Runnable() {
        @Override
        public void run() {
            scanForServer();
            mServiceHandler.postDelayed(this, SCAN_THREAD_DELAY);
        }
    };

    public OffloadService(){

        super();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId){

        mContext = getApplicationContext();
        final BluetoothManager bluetoothManager = (BluetoothManager) mContext.getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();

        HandlerThread thread = new HandlerThread("ServiceStartArguments", Process.THREAD_PRIORITY_BACKGROUND);

        mServiceLooper = thread.getLooper();
        mServiceHandler = new OffloadServiceHandler(mServiceLooper);

        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = Constants.START_SCANNING;

        if(Build.VERSION.SDK_INT >= 21) {
            mLEScanner = mBluetoothAdapter.getBluetoothLeScanner();
            settings = new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                    .build();
            filters = new ArrayList<ScanFilter>();
        }


        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    public boolean isOffloadingPossible(PackageManager packageManager){
        //check if device is compatible with Bluetooth LE
        boolean result = packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE);

        //check if bluetooth is enabled
        if(mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Log.i(LOG_NAME, "Bluetooth is not enabled. Enable Bluetooth in your application.");
            result = false;
        }

        return result;
    }

    public void enableOffload(){

    }

    public void scanForServer(){

        if(mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()){
            return;
        }

        mServiceHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if(Build.VERSION.SDK_INT < 21){
                    mBluetoothAdapter.stopLeScan(mLeScanCallback);
                }else {
                    mLEScanner.stopScan(mScanCallback);
                }
            }
        }, 10000);


        if(Build.VERSION.SDK_INT < 21){
            mBluetoothAdapter.startLeScan(mLeScanCallback);
        }else{
            mLEScanner.startScan(mScanCallback);
        }
        
    }


    private final class OffloadServiceHandler extends Handler {

        public OffloadServiceHandler(Looper looper){
            super(looper);
        }


        @Override
        public void handleMessage(Message msg) {

            //this is the callback required by the system
            Runnable callback = (Runnable)msg.obj;

            switch(msg.arg1){
                case Constants.IS_OFFLOADING_POSSIBLE:
                    break;
                case Constants.SEND_OFFLOADING_REQUEST:
                    break;
                case Constants.START_SCANNING:
                    mServiceHandler.post(scanForServerRunnable);
                    break;
                default:
                    break;
            }

        }
    }




}
