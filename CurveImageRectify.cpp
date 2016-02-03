#include "StdAfx.h"
#include "CurveImageRectify.h"
#include <math.h>
#define   ROUND(X)     (int)(X+0.5);
//#define   PI     (3.14159265);

CurveImageRectify::CurveImageRectify(){
	//顯示器曲面描敘
	CurveR=0;//曲率半徑
	//使用者位置
	UserD=1000;//使用者觀看距離

	//曲面顯示範圍
	//DisplayResolutionH=600;
	//DisplayResolutionW=600;
	DisplayResolutionH=1280;
	DisplayResolutionW=720;
	//ThetaRange=180;//最大可顯示角度(0~180)
	ThetaRange=105.1;//最大可顯示角度
	ThetaStart=ThetaRange/2;
	ThetaInvterval=ThetaRange/(DisplayResolutionH-1);

	CurveImgH=0;
	CurveImgW=0;

	//虛擬影像
	ImgH=0;
	ImgW=0;
	ImgY=0;

	//原始影像內容
	//Img_src= cvCreateImage(cvSize(DisplayResolutionW,DisplayResolutionH),8,3);
	//DisplayWidthStep
	Img_dst= cvCreateImage(cvSize(DisplayResolutionW,DisplayResolutionH),8,3);
	//Img_dst= cvCreateImage(cvSize(640,480),8,3);
	//對應起始點
	ProjectI[0]=0;

	//
	i_start=0;
}



int CurveImageRectify::GeoSet(double CurveR,unsigned short UserD){

	this->CurveR=CurveR;
	this->UserD=UserD;

	double DisplayAngle=(3.14159265/180)*ThetaRange;//專換為徑度
/*
	//檢查是否有視覺死角
    double h=CurveR/(CurveR+double(UserD));
	ViewAngle=2*acos(h);
    if(ViewAngle<DisplayAngle)
		DisplayAngle=ViewAngle;
*/
	//計算影像平面高度
	double a=DisplayAngle/2;
	double L=double(UserD);
	ImgY=CurveR*sin(a);
	ImgY=ImgY*L/(L+CurveR*(1-cos(a)));
	ImgH=2*ImgY;

	//計算投影像素投影至像平面位置
	double Y=0;
	double dY=0;
	for(int i=0; i<DisplayResolutionH;i++){

		a=0.008726646*2*(ThetaStart-ThetaInvterval*i);
		Y=CurveR*sin(a);
		Y=Y*L/(L+CurveR*(1-cos(a)));

		ProjectI[i]=(ImgY-Y)*(DisplayResolutionH/ImgH);

		printf("\n %d %f %f %f",i,a ,Y ,ProjectI[i]);//debug
	}


	return 0;
}

int CurveImageRectify::GeoSet2(double CurveR,unsigned short UserD){

	this->CurveR=CurveR;
	this->UserD=UserD;
	double L=double(UserD);
	double DisplayAngle=(3.14159265/180)*ThetaRange;//轉換為徑度

	//檢查是否有視覺死角
	double Y1=0,Y2=0;
	double a=0;
	i_start=0;
	i_end=DisplayResolutionH-1;
	ViewAngleStart_deg=ThetaStart;
	ViewAngleStart_rad=ThetaStart*(3.14159265/180);
	ViewAngle_deg=ThetaRange;
    ViewAngle_rad=ThetaRange*(3.14159265/180);
	ViewAngleInvterval_rad=ThetaInvterval*(3.14159265/180);
	ViewAngleInvterval_deg=ThetaInvterval;

	for(int i=1; i<DisplayResolutionH*0.5;i++){
		a=0.008726646*2*(ViewAngleStart_deg-ThetaInvterval*i);//ThetaStart被更動了
		Y1=CurveR*sin(a);
		Y2=(L+CurveR*(1-cos(a)))*tan(3.1415/2-a);
		if(Y1<Y2){
			i_start=i;
			i_end=(DisplayResolutionH-1)-i_start;
			ViewAngleStart_rad=a;
			ViewAngleStart_deg=ViewAngleStart_rad*(180/3.14159265);
			ViewAngle_deg=2*ViewAngleStart_deg;//可視角度
            ViewAngle_rad=2*ViewAngleStart_rad;//可視角度
			ViewAngleInvterval_rad=ViewAngle_rad/(DisplayResolutionH-1);
			ViewAngleInvterval_deg=ViewAngle_deg/(DisplayResolutionH-1);
			break;
		}
	}

	//計算影像平面高度
	a=ViewAngleStart_rad;
	ImgY=CurveR*sin(a);
	ImgY=ImgY*L/(L+CurveR*(1-cos(a)));
	ImgH=2*ImgY;

	//計算投影像素投影至像平面位置
	double Y=0;
	double dY=0;
	for(int i=i_start; i<DisplayResolutionH;i++){//這裡要改
		a=0.008726646*2*(ViewAngleStart_deg-ViewAngleInvterval_deg*i);
		Y=CurveR*sin(a);
		Y=Y*L/(L+CurveR*(1-cos(a)));
		ProjectI[i]=(ImgY-Y)*(DisplayResolutionH/ImgH);//<----這裡要改
		printf("\n %d %f %f %f",i,a ,Y ,ProjectI[i]);//debug
	}


	return 0;
}


int CurveImageRectify::update(){

	//開頭與結尾
//*
	for (int i=0; i<i_start;i++){
		for(int j=0;j<DisplayResolutionW;j++){
		Img_dst->imageData[Img_dst->widthStep*i+3*j+0]=0;
		Img_dst->imageData[Img_dst->widthStep*i+3*j+1]=0;
		Img_dst->imageData[Img_dst->widthStep*i+3*j+2]=255;
		}}

	for (int i=i_end; i<DisplayResolutionH;i++){
		for(int j=0;j<DisplayResolutionW;j++){
			Img_dst->imageData[Img_dst->widthStep*i+3*j+0]=0;
			Img_dst->imageData[Img_dst->widthStep*i+3*j+1]=0;
			Img_dst->imageData[Img_dst->widthStep*i+3*j+2]=255;
		}}
	//*/
	//中間
	int i_;
	for(int i=i_start;i<=i_end;i++){
		i_=ROUND(ProjectI[i]);
		if(i_>=DisplayResolutionH)
			i_=DisplayResolutionH-1;
		//i_=ROUND(i/4);
		//i_=i;
		for(int j=0;j<DisplayResolutionW;j++){
			Img_dst->imageData[Img_dst->widthStep*i+3*j+0]=Img_src->imageData[Img_dst->widthStep*i_+3*j+0];
			Img_dst->imageData[Img_dst->widthStep*i+3*j+1]=Img_src->imageData[Img_dst->widthStep*i_+3*j+1];
			Img_dst->imageData[Img_dst->widthStep*i+3*j+2]=Img_src->imageData[Img_dst->widthStep*i_+3*j+2];
		}}

	return 0;
}
