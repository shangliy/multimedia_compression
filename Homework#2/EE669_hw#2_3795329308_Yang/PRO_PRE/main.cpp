// The programe reads the image data from an image file "~.raw"
// Last updated on 02/20/2010 by Steve Cho

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>



using namespace std;

// Here we assume the image is of size 256*256 and is of raw format
// You will need to make corresponding changes to accommodate images of different sizes and types

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

#define Size 256
#define N 8
#define PACIFIER_COUNT 2047


typedef struct  tagBITMAPFILEHEADER {
	//WORD bfType;//
	DWORD bfSize;//
	WORD bfReserved1;//
	WORD bfReserved2;//
	DWORD bfOffBits;//
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;//
	LONG biWidth;//
	LONG biHeight;//
	WORD biPlanes;//1
	WORD biBitCount;//
	DWORD  biCompression; //
	DWORD  biSizeImage; //
	LONG  biXPelsPerMeter; //
	LONG  biYPelsPerMeter; //
	DWORD  biClrUsed; //
	DWORD  biClrImportant; //
}BITMAPINFOHEADER; //

typedef struct tagRGBQUAD {
	BYTE rgbBlue; 
	BYTE rgbGreen; 
	BYTE rgbRed;
	BYTE rgbReserved; //
}RGBQUAD;//

		 //
typedef struct tagIMAGEDATA
{
	BYTE red;
	BYTE green;
	BYTE blue;
}IMAGEDATA;



BITMAPFILEHEADER strHead;
RGBQUAD strPla[256];//256
BITMAPINFOHEADER strInfo;
IMAGEDATA imagedata[256][256];//
double DCT[N][N];
unsigned char Converdata[Size][Size][3]; //store the datas ready for DCT transform
unsigned char compare_data[Size][Size][3];
double Qe_10[N][N];
double Qe_90[N][N];

int v[10][3]={0};
int v_new[10][3]={0};
double a[4][3]={0};
int QP=0;
double c1=2;
double c2=5;
double c3=8;

double Qe[N][N]={
{ 16,	11,	10,	16,  24,   40,   51,	61},
{ 12,	12,	14,	19,  26,   58,   60,	55},
{ 14,	13,	16,	24,  40,   57,   69,	56},
{ 14,	17,	22,	29,  51,   87,   80,	62},
{ 18,   22,	37,	56,  68,   109,  103,	77},
{ 24,	35,	55,	64,  81,   104,  113,	92},
{ 49,	64,	78,	87,  103,  121,  120,	101},
{ 72,	92,	95,	98,  112,  100,  103,	99}
};


int MAX_FUN(int n,int p)
{
 int temp_max=0;
 int i=0;
 for (i=0;i<n;i++)
 {
   if(v[i][p]>temp_max)
   temp_max=v[i][p];

 }
 return temp_max;
 }

 int MIN_FUN(int n,int p)
{
 int temp_min=256;
 int i=0;
 for (i=0;i<n;i++)
 {
   if(v[i][p]<temp_min)
   temp_min=v[i][p];

 }
 return temp_min;
}

void Blur_9(int i,int j,int bitper)
{

int templates[9]={1,2,1,2,4,2,1,2,1};
int sum[3]= {0};
int k=0;
int a,b;

            for ( int m=i-1; m<i+2; m++)
            {
                for (int n=j-1; n<j+2; n++)
                {
                a=m;
                b=n;
                    if(a<0) a=0;
                    if(a>=Size) a=Size-1;
                    if(b<0) b=0;
                    if(b>=Size) b=Size-1;
                    sum[0]+= Converdata[a][b][0] *templates[k] ;
                     sum[1]+= Converdata[a][b][1] *templates[k] ;
                      sum[2]+= Converdata[a][b][2] *templates[k] ;
                      k++;
                }
            }
            sum[0] /= 16;
            sum[1] /= 16;
            sum[2] /= 16;
            if (sum[0]> 255)
                sum[0] = 255;
            if (sum[1] > 255)
                sum[1] = 255;
            if (sum[2] > 255)
                sum[2] = 255;
            Converdata[i][j][0] = sum[0];
            Converdata[i][j][1] = sum[1];
            Converdata[i][j][2] = sum[2];
            k=0;
}



void Smooth_region(int plane)
{
 int p=0;
 int b[9]={1,1,2,2,4,2,2,1,1};
 int m=0;
 int n=0;
 int sum=0;


 v_new[0][plane]=v[0][plane];
 v_new[9][plane]=v[9][plane];
 for(n=1;n<=8;n++)
 {
    for(m=-4;m<=4;m++)
    {
        if(m+n<1){
          if(abs(v[1][plane]-v[0][plane])<QP) p=v[0][plane];
            else p=v[1][plane];
        }
        if(m+n>8){
          if(abs(v[9][plane]-v[8][plane])<QP) p=v[9][plane];
            else p=v[8][plane];
        }
        if((m+n)<=8&&(m+n)>=1) p=v[m+n][plane];

        sum+=b[m+4]*p;
    }
    sum=sum/16;
    v_new[n][plane]=sum;
    sum=0;
 }

}

void Default_region(int plane)
{
    int d=0;
    int a_31=0;

    if(a[3][1]>0)
    {
      if( (abs(a[3][0])<abs(a[3][1])) && (abs(a[3][0])<abs(a[3][2])) )
         a_31=abs(a[3][0]);
      if( (abs(a[3][1])<abs(a[3][2])) && (abs(a[3][1])<abs(a[3][0])) )
         a_31=abs(a[3][1]);
      if( (abs(a[3][2])<abs(a[3][0])) && (abs(a[3][2])<abs(a[3][1])) )
         a_31=abs(a[3][2]);
    }
    else if (a[3][1]<0)
    {
      if( (abs(a[3][0])<abs(a[3][1])) && (abs(a[3][0])<abs(a[3][2])) )
         a_31=-abs(a[3][0]);
      if( (abs(a[3][1])<abs(a[3][2])) && (abs(a[3][1])<abs(a[3][0])) )
         a_31=-abs(a[3][1]);
      if( (abs(a[3][2])<abs(a[3][0])) && (abs(a[3][2])<abs(a[3][1])) )
         a_31=-abs(a[3][2]);
    }
    else a_31=0;

    d=c2*(a_31-a[3][1])/c3;
    if ((v[4][plane]-v[5][plane])>0)
    {
        if (d<0) d=0;
        else if (d>(v[4][plane]-v[5][plane])/2) d=(v[4][plane]-v[5][plane])/2;
    }
    else if ((v[4][plane]-v[5][plane])<0)
    {
        if (d>0) d=0;
        else if (d<(v[4][plane]-v[5][plane])/2) d=(v[4][plane]-v[5][plane])/2;
    }
    else d=0;
    v_new[4][plane]=v[4][plane]-d;
    v_new[5][plane]=v[5][plane]+d;
}


int main(int argc, char *argv[])

{
// file pointer
	FILE *file;
	FILE *fpi;

	int shift_data[Size][Size];

	double PSNR[6];
	double MSE[6]={0};

	// do some image processing task...
	int i=0;
	int j=0;
	int m=0;
	int k=0;
	int vec=1;
	int hem=1;
	int method=0; //0--mymethod;
	              //1--deblocking_filter;
	              //2--reapplying
    int index=0;

    int BytesPerPixel=1;
    cout<<"Type int the BytesPerPixel"<<endl;
	cin>>BytesPerPixel;


	unsigned char origin_data[Size][Size][BytesPerPixel];
	unsigned char colore_data[Size][Size][BytesPerPixel];
	unsigned char test_data[Size][Size][BytesPerPixel];

	int Ima_origin[Size][Size];
	int Ima_outdata[Size][Size];

    char input_name[30];
	char output_name[30];
	char NEWinput_name[30];
	int factor=100;

	// read image "ride.raw" into image data matrix
	if(BytesPerPixel==1)
	{
        if (!(file=fopen("ORIGIN/clock.raw","rb")))
        {
            cout << "Cannot open file: " << "clock.raw" <<endl;
            exit(1);
        }
        fread(origin_data, sizeof(unsigned char), Size*Size*BytesPerPixel, file);
        fclose(file);


            if (!(fpi=fopen("ORIGIN/clock.bmp","rb")))
                printf("The bmg Input file was not opened\n");
            else
                printf("The bmg Input file was opened\n");

            if (fpi != NULL) {
                //file type
                WORD bfType;
                fread(&bfType, 1, sizeof(WORD), fpi);
                fread(&strHead, 1, sizeof(tagBITMAPFILEHEADER), fpi);
                fread(&strInfo, 1, sizeof(tagBITMAPINFOHEADER), fpi);
                //
                for (int nCounti = 0; nCounti<strInfo.biClrUsed; nCounti++) {
                    //remove rgbReserved
                    fread((char *)&strPla[nCounti].rgbBlue, 1, sizeof(BYTE), fpi);
                    fread((char *)&strPla[nCounti].rgbGreen, 1, sizeof(BYTE), fpi);
                    fread((char *)&strPla[nCounti].rgbRed, 1, sizeof(BYTE), fpi);
                    fread((char *)&strPla[nCounti].rgbReserved, 1, sizeof(BYTE), fpi);
                }
                fread(test_data, sizeof(unsigned char), Size*Size*BytesPerPixel, fpi);
                fclose(fpi);

            }


	}
	else {
        if (!(file=fopen("ORIGIN/pepper.raw","rb")))
        {
            cout << "Cannot open file: " << "pepper.raw" <<endl;
            exit(1);
        }
        fread(origin_data, sizeof(unsigned char), Size*Size*BytesPerPixel, file);
        fclose(file);

         if (!(fpi=fopen("ORIGIN/pepper.bmp","rb")))
                printf("The bmg Input file was not opened\n");
            else
                printf("The bmg Input file was opened\n");

            if (fpi != NULL) {
                
                WORD bfType;
                fread(&bfType, 1, sizeof(WORD), fpi);
                fread(&strHead, 1, sizeof(tagBITMAPFILEHEADER), fpi);
                fread(&strInfo, 1, sizeof(tagBITMAPINFOHEADER), fpi);
                
                for (int nCounti = 0; nCounti<256; nCounti++) {
                    
                    fread((char *)&strPla[nCounti].rgbBlue, 1, sizeof(BYTE), fpi);
                    fread((char *)&strPla[nCounti].rgbGreen, 1, sizeof(BYTE), fpi);
                    fread((char *)&strPla[nCounti].rgbRed, 1, sizeof(BYTE), fpi);
                    //fread((char *)&strPla[nCounti].rgbReserved, 1, sizeof(BYTE), fpi);
                }
                fread(test_data, sizeof(unsigned char), Size*Size*BytesPerPixel, fpi);
                fclose(fpi);
            }
	}



	while (1)
	{
        cout<<"Type int the method to use "<<endl;
        cout<<"0--mymethod"<<endl;
        cout<<"1--deblocking_filte"<<endl;
        cout<<"2--reapplying"<<endl;
        cout<<"3--exit the program"<<endl;
        cin>>method;

        if (method==3) return 0;

        for(index=1;index<=5;index++)
        {

           if(BytesPerPixel==1) sprintf(input_name,"RAW/clock_pro_%d.raw",index);
           else sprintf(input_name,"RAW/pepper_pro_%d.raw",index);
           //if(BytesPerPixel==1) sprintf(input_name,"RAW/clock_pro_%d.raw",index);
            //else sprintf(input_name,"RAWDATA/pepper%d.raw",index);

            if (!(file=fopen(input_name,"rb")))
            {
                cout << "Cannot open file: " << input_name <<endl;
                exit(1);
            }
            fread(compare_data, sizeof(unsigned char), Size*Size*3, file);
            fclose(file);


            for (i=0;i<Size;i++)
                     {
                        for (j=0;j<Size;j++)
                        {
                        if (BytesPerPixel==1) MSE[0]+=(double)(origin_data[i][j][0]-compare_data[i][j][0])*(double)(origin_data[i][j][0]-compare_data[i][j][0]);
                        else MSE[0]+=(double)(origin_data[i][j][0]-compare_data[i][j][0])*(double)(origin_data[i][j][0]-compare_data[i][j][0])
                                    +(double)(origin_data[i][j][1]-compare_data[i][j][1])*(double)(origin_data[i][j][1]-compare_data[i][j][1])
                                    +(double)(origin_data[i][j][2]-compare_data[i][j][2])*(double)(origin_data[i][j][2]-compare_data[i][j][2]);
                        }
                    }

            MSE[0]=MSE[0]/(Size*Size*BytesPerPixel);
            PSNR[0]=10*log10(255*255/MSE[0]);
            MSE[0]=0;
            cout<< PSNR[0]<<endl;

            for(i=0;i<Size;i++)
            {
                for(j=0;j<Size;j++)
                {
                    Converdata[i][j][0]=compare_data[i][j][0];
                    Converdata[i][j][1]=compare_data[i][j][1];
                    Converdata[i][j][2]=compare_data[i][j][2];
                }

            }
            /*My method of using 4 block gussian filter*/
            if (method==0)

            {

                for (int i=7;i<Size-1;i+=N)
                    {
                        for (int j=0;j<Size;j++)
                        {
                          Blur_9(i,j,BytesPerPixel);
                          Blur_9(i+1,j,BytesPerPixel);
                        }

                    }

                for (int j=7;j<Size-1;j+=N)
                    {
                        for (int i=0;i<Size;i++)
                        {
                          Blur_9(i,j,BytesPerPixel);
                          Blur_9(i,j+1,BytesPerPixel);

                        }

                    }

            }
            if (method==1)
            {

                int v_count=0;

                int v_max;
                int v_min;
                int p;

                int F=0;
                int T1=2;
                int T2=6;

                for(p=0;p<BytesPerPixel;p++)
                {
                    v[10][p]={0};
                    v_new[10][p]={0};
                    a[4][3]={0};
                    QP=0;
                    c1=2;
                    c2=5;
                    c3=8;

                    for (i=7;i<Size-1;i+=N)
                    {
                        for (j=0;j<Size;j++)
                        {
                            v[0][p]=Converdata[i-4][j][p];
                            v[1][p]=Converdata[i-3][j][p];
                            v[2][p]=Converdata[i-2][j][p];
                            v[3][p]=Converdata[i-1][j][p];
                            v[4][p]=Converdata[i][j][p];
                            v[5][p]=Converdata[i+1][j][p];
                            v[6][p]=Converdata[i+2][j][p];
                            v[7][p]=Converdata[i+3][j][p];
                            v[8][p]=Converdata[i+4][j][p];
                            v[9][p]=Converdata[i+5][j][p];
                            QP=Qe[(i+1)%8][j%8];
                            v_max= MAX_FUN(10,p);
                            v_min= MIN_FUN(10,p);
                            for (k=0;k<3;k++)
                            {
                                a[3][k]=(c1*v[2*k+1][p]-c2*v[2*k+2][p]+c2*v[2*k+3][p]-c1*v[2*k+4][p])/c3;
                            }
                            for(k=0;k<8;k++)
                            {
                                if (abs(v[k][p]-v[k+1][p])<=T1)
                                {
                                    F++;
                                }
                            }
                            if(F>=T2)
                            {
                               if(abs(v_max-v_min)<2*QP)
                               {

                                Smooth_region(p);
                                Converdata[i-4][j][p]=v_new[0][p];
                                Converdata[i-3][j][p]=v_new[1][p];
                                Converdata[i-2][j][p]=v_new[2][p];
                                Converdata[i-1][j][p]=v_new[3][p];
                                Converdata[i][j][p]=v_new[4][p];
                                Converdata[i+1][j][p]=v_new[5][p];
                                Converdata[i+2][j][p]=v_new[6][p];
                                Converdata[i+3][j][p]=v_new[7][p];
                                Converdata[i+4][j][p]=v_new[8][p];
                                Converdata[i+5][j][p]=v_new[9][p];
                                F=0;
                               }
                            }
                            else
                            {
                               if(a[3][1]<QP)
                               {
                               Default_region(p);
                               Converdata[i][j][p]=v_new[4][p];
                               Converdata[i+1][j][p]=v_new[5][p];
                               }
                               F=0;
                            }
                        }
                    }

                    for (j=7;j<Size-1;j+=N)
                    {
                        for (i=0;i<Size;i++)
                        {
                            v[0][p]=Converdata[i][j-4][p];
                            v[1][p]=Converdata[i][j-3][p];
                            v[2][p]=Converdata[i][j-2][p];
                            v[3][p]=Converdata[i][j-1][p];
                            v[4][p]=Converdata[i][j][p];
                            v[5][p]=Converdata[i][j+1][p];
                            v[6][p]=Converdata[i][j+2][p];
                            v[7][p]=Converdata[i][j+3][p];
                            v[8][p]=Converdata[i][j+4][p];
                            v[9][p]=Converdata[i][j+5][p];
                            QP=Qe[(i)%8][(j+1)%8];
                            v_max= MAX_FUN(10,p);
                            v_min= MIN_FUN(10,p);
                            for (k=0;k<3;k++)
                            {
                                a[3][k]=(c1*v[2*k+1][p]-c2*v[2*k+2][p]+c2*v[2*k+3][p]-c1*v[2*k+4][p])/c3;
                            }
                            for(k=0;k<8;k++)
                            {
                                if (abs(v[k][p]-v[k+1][p])<=T1)
                                {
                                    F++;
                                }
                            }
                            if(F>=T2)
                            {
                               if(abs(v_max-v_min)<2*QP)
                               {
                                Smooth_region(p);
                                Converdata[i][j-4][p]=v_new[0][p];
                                Converdata[i][j-3][p]=v_new[1][p];
                                Converdata[i][j-2][p]=v_new[2][p];
                                Converdata[i][j-1][p]=v_new[3][p];
                                Converdata[i][j][p]=v_new[4][p];
                                Converdata[i][j+1][p]=v_new[5][p];
                                Converdata[i][j+2][p]=v_new[6][p];
                                Converdata[i][j+3][p]=v_new[7][p];
                                Converdata[i][j+4][p]=v_new[8][p];
                                Converdata[i][j+5][p]=v_new[9][p];
                               }
                                F=0;
                            }
                            else
                            {
                               if(a[3][1]<QP)
                               {
                                    Default_region(p);
                                    Converdata[i][j][p]=v_new[4][p];
                                    Converdata[i][j+1][p]=v_new[5][p];
                               }
                               F=0;
                            }
                        }
                    }

                }

            }

            if(method==2)
            {

               if(BytesPerPixel==1) sprintf(NEWinput_name,"NEWPRORAW/clock%d.raw",index);
                else sprintf(NEWinput_name,"NEWPRORAW/pepper%d.raw",index);
             //if(BytesPerPixel==1) sprintf(NEWinput_name,"PROPRORAW/clock%d.raw",index);
               //else sprintf(NEWinput_name,"PROPRORAW/clock%d.raw",index);

                    if (!(file=fopen(NEWinput_name,"rb")))
                    {
                        cout << "Cannot open file: " << input_name <<endl;
                        exit(1);
                    }
                    fread(Converdata, sizeof(unsigned char), Size*Size*3, file);
                    fclose(file);

            }



            for (i=0;i<Size;i++)
                     {
                        for (j=0;j<Size;j++)
                        {
                        if (BytesPerPixel==1) MSE[index]+=(double)(origin_data[i][j][0]-Converdata[i][j][0])*(double)(origin_data[i][j][0]-Converdata[i][j][0]);
                        /*else MSE[index]+=(double)(origin_data[i][j][0]-Converdata[i][j][0])*(double)(origin_data[i][j][0]-Converdata[i][j][0])
                                    +(double)(origin_data[i][j][1]-Converdata[i][j][1])*(double)(origin_data[i][j][1]-Converdata[i][j][1])
                                    +(double)(origin_data[i][j][2]-Converdata[i][j][2])*(double)(origin_data[i][j][2]-Converdata[i][j][2]);*/
                        else MSE[index]+=(double)(origin_data[i][j][0]-Converdata[i][j][2])*(double)(origin_data[i][j][0]-Converdata[i][j][2])
                                    +(double)(origin_data[i][j][1]-Converdata[i][j][1])*(double)(origin_data[i][j][1]-Converdata[i][j][1])
                                    +(double)(origin_data[i][j][2]-Converdata[i][j][0])*(double)(origin_data[i][j][2]-Converdata[i][j][0]);


                        }
                     }

                    MSE[index]=MSE[index]/(Size*Size*BytesPerPixel);
                    PSNR[index]=10*log10(255*255/MSE[index]);
                    MSE[index]=0;
                    cout<< PSNR[index]<<endl;


/*
                    unsigned char tem[Size][Size][3];
                     for (i=0;i<Size;i++)
                            {
                                for(j=0;j<Size;j++)
                                {
                                    tem[i][j][0]=Converdata[Size-1-i][j][2];
                                    tem[i][j][1]=Converdata[Size-1-i][j][1];
                                    tem[i][j][2]=Converdata[Size-1-i][j][0];

                                }

                            }

                    for (i=0;i<Size;i++)
                            {
                                for(j=0;j<Size;j++)
                                {
                                    Converdata[i][j][0]=tem[i][j][0];
                                    Converdata[i][j][1]=tem[i][j][1];
                                    Converdata[i][j][2]=tem[i][j][2];

                                }

                            }*/

                    if(BytesPerPixel==1)
                        {sprintf(output_name, "PROBMP/clock_pro%d.bmp", index);}
                    else
                        {sprintf(output_name, "PROBMP/pepper_pro%d.bmp", index);}

                    if (!(fpi=fopen(output_name,"wb")))
                        printf("The output file was not opened\n");
                    else
                        printf("The output file was opened\n");

                    WORD bfType = 0x4d42;
                    fwrite(&bfType, 1, sizeof(WORD), file);
                    //fpw +=2;
                    fwrite(&strHead, 1, sizeof(tagBITMAPFILEHEADER), file);
                    fwrite(&strInfo, 1, sizeof(tagBITMAPINFOHEADER), file);
                    for (int nCounti = 0; nCounti<strInfo.biClrUsed; nCounti++) {

                        fwrite(&strPla[nCounti].rgbBlue, 1, sizeof(BYTE), file);
                        fwrite(&strPla[nCounti].rgbGreen, 1, sizeof(BYTE), file);
                        fwrite(&strPla[nCounti].rgbRed, 1, sizeof(BYTE), file);
                        //fwrite(&strPla[nCounti].rgbReserved, 1, sizeof(BYTE), file);
                      }
                    fwrite(Converdata, sizeof(unsigned char), Size*Size * 3, file);
                    //fwrite(test_data, sizeof(unsigned char), Size*Size * 3, file);
                    fclose(file);

        }

	}



	return 0;
}







