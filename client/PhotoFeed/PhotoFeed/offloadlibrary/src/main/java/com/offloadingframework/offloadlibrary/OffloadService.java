package com.offloadingframework.offloadlibrary;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
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
import android.os.Binder;
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

    private NotificationManager mNotificationManager;

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
                    handleSocket(device);
                }
            };

    private ScanCallback mScanCallback = new ScanCallback() {
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
                    handleSocket(device);

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
    public void onCreate() {
        super.onCreate();

        mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if(mServiceHandler != null)
            mServiceHandler.removeCallbacks(scanForServerRunnable);
        Log.d(LOG_NAME, "Service is destroyed.");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }


    private void showNotification() {

        CharSequence text = "Offloading is possible.";

        Notification notification = new Notification.Builder(this)
                .setCategory(Notification.CATEGORY_MESSAGE)
                .setContentTitle("Offloading Service.")
                .setContentText("System has detected BLE offloading is possible.")
                .setSmallIcon(R.drawable.notification_template_icon_bg)
                .setAutoCancel(false).build();
        mNotificationManager.notify(1, notification);

    }


    private final IBinder mBinder = new OffloadServiceBinder();

    public boolean isOffloadingPossible(PackageManager packageManager){
        //check if device is compatible with Bluetooth LE
        boolean result = packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE);

        //check if bluetooth is enabled
        if(mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Log.i(LOG_NAME, "Bluetooth is not enabled. Enable Bluetooth in your application.");
            result = false;
        }

        if(offloadSocket == null || !offloadSocket.isConnected()){
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

        //stop scanning if there already is a socket
        if(offloadSocket != null && offloadSocket.isConnected())
            return;

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

        if(offloadSocket != null && offloadSocket.isConnected()){
            return;
        }

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

        Log.d(LOG_NAME, "Found server and succesfully connected to it!!!!!");
        showNotification();
        foundServer = true;
        //mServiceHandler.postDelayed(sanityCheckRunnable, PING_TIME);
        mServiceHandler.removeCallbacks(sanityCheckRunnable);
        mServiceHandler.removeCallbacks(scanForServerRunnable);
        offloadMandelbrot(1600, 1024);
    }


    public synchronized void writeToSocket(char[] buffer){

        if(!offloadSocket.isConnected()){

            foundServer = false;
            mServiceHandler.removeCallbacks(sanityCheckRunnable);

            try {
                socketPrintWriter.close();
                socketBufferedReader.close();
            }catch(IOException e){
                Log.e(LOG_NAME, "");
            }

            return;
        }

        try {
            Log.d(LOG_NAME, "Seding " + buffer + " to server.");
            socketPrintWriter.print(buffer);
            socketPrintWriter.flush();
        }catch(Exception e){
            foundServer = false;
        }

    }

    public synchronized void writeToSocket(String buffer){

        if(offloadSocket == null || !offloadSocket.isConnected()){

            cleanUpSocket();
            return;

        }

        try {
            Log.d(LOG_NAME, "Sending " + buffer + " to server.");
            socketPrintWriter.print(buffer);
            socketPrintWriter.flush();
        }catch(Exception e){
            foundServer = false;
            cleanUpSocket();
        }

    }


    public void cleanUpSocket(){


        mServiceHandler.removeCallbacks(sanityCheckRunnable);
        if(socketPrintWriter != null) socketPrintWriter.close();


        try {

            if(socketBufferedReader != null) socketBufferedReader.close();
            if(offloadSocket != null) offloadSocket.close();

        } catch (IOException e) {
            e.printStackTrace();
        }

        socketBufferedReader = null;
        socketPrintWriter = null;
        offloadSocket = null;

        Message m = mServiceHandler.obtainMessage();
        m.arg1 = Constants.START_SCANNING;
        mServiceHandler.handleMessage(m);


    }

    public void socketCheckSanity(){

        synchronized (offloadSocket) {
            writeToSocket("PING");

            char[] buffer = new char[100];
            try {

                socketBufferedReader.read(buffer, 0, 100);
                String stringBuffer = new String(buffer);
                if (!stringBuffer.equals("ACK"))
                    foundServer = false;

            } catch (Exception e) {
                foundServer = false;
            }

            if (!foundServer) {
                cleanUpSocket();
            } else {
                Log.i(LOG_NAME, "Server ACK received.");
                mServiceHandler.postDelayed(sanityCheckRunnable, PING_TIME);
            }
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
                case Constants.OFFLOAD_MANDELBROT:
                    break;
                default:
                    break;
            }

        }
    }

    //do not comment on the horrible thing that this code is. It is 2 o'clock in the morning and i just need it to work
    public static int widthS, heightS;

    private void offloadMandelbrot(int width, int height) {

        widthS = width;
        heightS  = height;

        RequestFactory.RMIRequest request = (RequestFactory.RMIRequest)RequestFactory.createRequest(RequestType.REQUEST_RMI);

        request.setMethodName("mandelbrot");
        request.addParameter("width", width);
        request.addParameter("height", height);

        request.buildRequest();

        writeToSocket(request.getOutput());

        Runnable waitForMandelbrot = new Runnable() {
            @Override
            public void run() {

                synchronized (offloadSocket) {

                    int size = widthS * heightS;
                    char[] buffer = new char[size];
                    Log.d(LOG_NAME, "Waiting for a buffer of size: " + size);

                    try {

                        socketBufferedReader.read(buffer, 0, size);

                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    Log.d(LOG_NAME, "YUHUUUUUUU.");

                }



            }
        };

        mServiceHandler.post(waitForMandelbrot);

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

    public class OffloadServiceBinder extends Binder {
        OffloadService getService(){
            return OffloadService.this;
        }

    }



}
