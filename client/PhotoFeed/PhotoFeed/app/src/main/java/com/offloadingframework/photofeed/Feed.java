package com.offloadingframework.photofeed;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import com.offloadingframework.offloadlibrary.OffloadService.OffloadServiceBinder;

import com.offloadingframework.offloadlibrary.OffloadService;

public class Feed extends Activity {


    private ImageView image_view = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_feed);
    }

    @Override
    protected void onResume() {
        super.onResume();

        //Intent intent = new Intent(this, OffloadService.class);
        //startService(intent);
        doBindService();

        ImageView view =  (ImageView)findViewById(R.id.image_view);

        Button button = (Button)findViewById(R.id.button_process_image);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

                //generate image
                MandelbrotImage image = new MandelbrotImage(Feed.this);

            }
        });



    }

    @Override
    protected void onPause() {
        super.onPause();

        //Intent intent = new Intent(this, OffloadService.class);
        //stopService(intent);
        doUnbindService();

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_feed, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }


    private void doBindService(){
        bindService(new Intent(Feed.this, OffloadService.class), mConnection, Context.BIND_AUTO_CREATE);
    }

    private void doUnbindService(){
        if(mBoundService != null){
            unbindService(mConnection);
            //mBoundService = null;
        }
    }

    public static OffloadService mBoundService;

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mBoundService = ((OffloadService.OffloadServiceBinder)service).getService();
            Toast.makeText(Feed.this, "Succesfully binded offloading service.", Toast.LENGTH_LONG);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mBoundService = null;
            Toast.makeText(Feed.this, "Succesfully unbinded offloading service.", Toast.LENGTH_LONG);
        }
    };



}
