package com.offloadingframework.photofeed;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.widget.ImageView;


/**
 * Created by tony on 18.06.2015.
 */
public class MandelbrotImage {


    private final int MAX_ITER = 570;
    private final double ZOOM = 1000;
    private Bitmap picture;
    private Canvas canvas;
    private ImageView imageView;
    private Activity actContext;
    private double zx, zy, cX, cY, tmp;

    public int getHeight() {
        return height;
    }

    private int height = 1600;

    public int getWidth() {
        return width;
    }

    private int width = 1024;


    public MandelbrotImage(Activity context) {

        //setBounds(100, 100, 800, 600);
        this.imageView = (ImageView)context.findViewById(R.id.image_view);
        this.actContext = context;

        Bitmap.Config conf = Bitmap.Config.ARGB_8888;
        picture = Bitmap.createBitmap(getWidth(), getHeight(), conf);
        picture.setHasAlpha(false);



        Runnable runner = new Runnable() {
            @Override
            public void run() {

                //I = new BufferedImage(getWidth(), getHeight(), BufferedImage.TYPE_INT_RGB);
                for (int y = 0; y < getHeight(); y++) {
                    for (int x = 0; x < getWidth(); x++) {
                        zx = zy = 0;
                        cX = (x - getWidth()/2) / ZOOM;
                        cY = (y - getHeight()/2) / ZOOM;
                        int iter = MAX_ITER;
                        while (zx * zx + zy * zy < 4 && iter > 0) {
                            tmp = zx * zx - zy * zy + cX;
                            zy = 2.0 * zx * zy + cY;
                            zx = tmp;
                            iter--;
                        }

                        picture.setPixel(x, y, Color.argb(0, 0, 0, iter | iter << 8));

                    }
                }

                canvas = new Canvas(picture);
                canvas.drawBitmap(picture, 0, 0, null);
                Log.d("MandelbrotImage", "finished drawing canvas.");


                actContext.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d("MandelbrotImage", "running on UI thread.");
                        imageView.setImageBitmap(picture);
                        imageView.invalidate();
                    }
                });
            }
        };

        Thread thread = new Thread(runner);
        thread.start();
        Log.d("MandelbrotImage", "Started generating.");

    }


    public Bitmap getPicture(){
        return picture;
    }

    public Canvas getCanvas(){
        return canvas;
    }

}
