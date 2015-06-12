package com.offloadingframework.offloadlibrary;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanRecord;
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
import android.util.SparseArray;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
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

    private BluetoothSocket offloadSocket = null;
    private PrintWriter socketPrintWriter = null;
    private BufferedReader socketBufferedReader = null;

    private static UUID uuid = UUID.fromString("6686416b-aa45-798f-1747-680bd1c4a59b");

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

    private ScanCallback mScanCallback;

    {
        mScanCallback = new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                Log.d(LOG_NAME, "Found advertisement: " + result.toString());
                if(foundServer) return;

                ScanRecord record = result.getScanRecord();
                SparseArray<byte[]> advPacket = record.getManufacturerSpecificData();

                StringBuilder buffer = new StringBuilder();

                for (int i = 0; i < advPacket.size(); i++) {
                    for (byte b : advPacket.valueAt(i)) {
                        buffer.append(String.format("%02X ", b));
                    }
                }

                if(result.getDevice().getUuids() != null)
                    for(int i = 0; i < result.getDevice().getUuids().length; i++){
                        Log.e(LOG_NAME, "UUID: " + result.getDevice().getUuids()[i]);
                    }

                Log.d(LOG_NAME, "PACKET: " + buffer.toString());
                if (buffer.toString().contains("48 45 4C 4C 4F 57 4F 52 4C")) {

                    BluetoothDevice device = result.getDevice();
                    //handleSocket(device);
                    device.fetchUuidsWithSdp();

                    try {
                        Log.d(LOG_NAME, "Found the device, now trying to connect to it through " + uuid.toString());

                        offloadSocket = device.createInsecureRfcommSocketToServiceRecord(uuid);
                        offloadSocket.connect();
                        socketPrintWriter = new PrintWriter(offloadSocket.getOutputStream(), true);
                        socketBufferedReader = new BufferedReader(new InputStreamReader(offloadSocket.getInputStream()));

                    } catch (IOException e) {
                        //e.printStackTrace();
                        Log.e(LOG_NAME, "Could not connect to server.");
                        foundServer = false;
                        return;

                    }

                    Log.d(LOG_NAME, "Found server and succesfully connected to it.");
                    foundServer = true;
                    mServiceHandler.postDelayed(sanityCheckRunnable, PING_TIME);



                }

            }

            @Override
            public void onBatchScanResults(List<ScanResult> results) {

                for (ScanResult result : results) {

                    Log.d(LOG_NAME, "Found advertisement in batch: " + result.toString());

                    if(foundServer) return;

                    ScanRecord record = result.getScanRecord();
                    SparseArray<byte[]> advPacket = record.getManufacturerSpecificData();

                    StringBuilder buffer = new StringBuilder();

                    for (int i = 0; i < advPacket.size(); i++) {
                        for (byte b : advPacket.valueAt(i)) {
                            buffer.append(String.format("%02X ", b));
                        }
                    }

                    Log.d(LOG_NAME, "PACKET: " + buffer.toString());
                    if (buffer.toString().contains("48 45 4C 4C 4F 57 4F 52 4C")) {
                        handleSocket(result.getDevice());
                        return;
                    }

                }

            }

            @Override
            public void onScanFailed(int errorCode) {
                Log.d(LOG_NAME, "Scan failed.");
                foundServer = false;
            }
        };
    }


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

        Log.d(LOG_NAME, "Started Service.");

        HandlerThread thread = new HandlerThread("ServiceStartArguments", Process.THREAD_PRIORITY_BACKGROUND);
        thread.start();

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

        mServiceHandler.sendMessage(msg);

        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mServiceHandler.removeCallbacks(scanForServerRunnable);
        Log.d(LOG_NAME, "Service is destroyed.");
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
        Log.d(LOG_NAME, "Started Scanning.");

        mServiceHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (Build.VERSION.SDK_INT < 21) {
                    mBluetoothAdapter.stopLeScan(mLeScanCallback);
                } else {
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


    public void handleSocket(BluetoothDevice device){

        try {
            Log.d(LOG_NAME, "Found the device, now trying to connect to it through " + uuid.toString());

            offloadSocket = device.createInsecureRfcommSocketToServiceRecord(uuid);
            offloadSocket.connect();
            socketPrintWriter = new PrintWriter(offloadSocket.getOutputStream(), true);
            socketBufferedReader = new BufferedReader(new InputStreamReader(offloadSocket.getInputStream()));

        } catch (IOException e) {
            //e.printStackTrace();
            Log.e(LOG_NAME, "Could not connect to server.");
            foundServer = false;
            return;

        }

        Log.d(LOG_NAME, "Found server and succesfully connected to it.");
        foundServer = true;
        mServiceHandler.postDelayed(sanityCheckRunnable, PING_TIME);
    }

    public void socketCheckSanity(){

        socketPrintWriter.print("PING");
        socketPrintWriter.flush();

        char[] buffer = new char[100];
        try {

            socketBufferedReader.read(buffer, 0, 100);
            String stringBuffer = new String(buffer);
            if(!stringBuffer.equals("ACK"))
                foundServer = false;

        } catch (IOException e) {
            //e.printStackTrace();
            foundServer = false;
        }

        if(!foundServer){
            mServiceHandler.removeCallbacks(sanityCheckRunnable);
            socketPrintWriter.close();


            try {

                socketBufferedReader.close();
                offloadSocket.close();

            } catch (IOException e) {
                e.printStackTrace();
            }

        }else{
            Log.i(LOG_NAME, "Server ACK received.");
        }

    }

    private static long PING_TIME = 5000;
    private Runnable sanityCheckRunnable = new Runnable() {
        @Override
        public void run() {
            socketCheckSanity();
        }
    };


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

    BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            super.onConnectionStateChange(gatt, status, newState);

            switch(newState){
                case BluetoothProfile.STATE_CONNECTED:
                    Log.d(LOG_NAME, "STATE_CONNECTED.");
                    foundServer = true;
                    BluetoothDevice device = gatt.getDevice();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    Log.d(LOG_NAME, "STATE_DISCONNECTED.");
                    foundServer = false;
                    break;
                default:
                    Log.e(LOG_NAME, "STATE_OTHER");
                    break;
            }

        }
    };


}
