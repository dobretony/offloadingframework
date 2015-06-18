#include "mandelbrot.h"
#include<string.h>

void mandelbrot(int iXmax, int iYmax, double zoom, char* result)
{
          /* screen ( integer) coordinate */
        int iX,iY;
        //const int iXmax = 1024; 
        //const int iYmax = 1600;
        /* world ( double) coordinate = parameter plane*/
        double Cx,Cy;
        const double CxMin=-1.5;
        const double CxMax=1.5;
        const double CyMin=-2.0;
        const double CyMax=2.0;

	const double ZOOM = zoom;
        /* */
        double PixelWidth=(CxMax-CxMin)/iXmax;
        double PixelHeight=(CyMax-CyMin)/iYmax;
	static unsigned char color[3];
        /* Z=Zx+Zy*i  ;   Z0 = 0 */
        double Zx, Zy;
        double Zx2, Zy2; /* Zx2=Zx*Zx;  Zy2=Zy*Zy  */
        
	/*  */
        int Iteration;
        const int IterationMax=570;
        
	/* bail-out value , radius of circle ;  */
        const double EscapeRadius=2;
        double ER2=EscapeRadius*EscapeRadius;
        

	/*Create a buffer large enough to hold the data*/
	result = (char*)calloc(iXmax * iYmax, sizeof(char));
	int buffer_pointer = 0;


	/* compute and write image data bytes to the file*/
        for(iY=0; iY<iYmax; iY++)
        {
             //Cy=CyMin + iY*PixelHeight;
	     Cy = (iY - iYmax/2 ) / ZOOM;
             //if (fabs(Cy) < PixelHeight/2) Cy=0.0; /* Main antenna */
             for(iX=0; iX < iXmax;iX++)
             {         
                        //Cx=CxMin + iX*PixelWidth;
			Cx = (iX - iXmax/2 ) / ZOOM;
                        /* initial value of orbit = critical point Z= 0 */
                        Zx = 0.0;
                        Zy = 0.0;
                        Zx2=Zx*Zx;
                        Zy2=Zy*Zy;
                        /* */
                        for (Iteration=0;Iteration<IterationMax && ((Zx2+Zy2)<ER2);Iteration++)
                        {
                            Zy=2*Zx*Zy + Cy;
                            Zx=Zx2-Zy2 + Cx;
                            Zx2=Zx*Zx;
                            Zy2=Zy*Zy;
                        };
                        /* compute  pixel color (24 bit = 3 bytes) */
                        if (Iteration==IterationMax)
                        { /*  interior of Mandelbrot set = black */
                           color[0]=0;
                           color[1]=0;
                           color[2]=0;                           
                        }
                     else 
                        { /* exterior of Mandelbrot set = white */
                             color[0]=0; /* Red*/
                             color[1]=0;  /* Green */ 
                             color[2]= (Iteration | Iteration << 8);/* Blue */
                        };
                        /*write color to the buffer*/
                        //fwrite(color,1,3,fp);
			sprintf(result + buffer_pointer, "%s ", color);
			buffer_pointer += strlen(color);

                }
        }
 

}


