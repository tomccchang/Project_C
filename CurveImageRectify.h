#include "stdafx.h"

//OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cv.h>
#include <highgui.h>

class  CurveImageRectify{

public:
	//顯示器曲面描敘
	double CurveR;//曲率半徑
	//使用者位置
	unsigned short UserD;//使用者觀看距離

	//曲面顯示範圍
	int DisplayResolutionH;
	int DisplayResolutionW;
	int DisplayWidthStep;
	double ThetaRange;//最大可顯示角度(0~180)
	double ThetaRange_rad;//最大可顯示角度(0~180)
	double ThetaInvterval;
	double ThetaStart;
	double CurveImgH;
	double CurveImgW;

	double ViewAngle_deg;//可視角度
	double ViewAngle_rad;//可視角度
	double ViewAngleStart_rad;
	double ViewAngleStart_deg;
	double ViewAngleInvterval_rad;
	double ViewAngleInvterval_deg;
	int i_start,i_end;

	//虛擬影像
	double ImgH;
	double ImgW;
	double ImgY;

	//原始影像內容
	IplImage *Img_src;

	//校正後影像
	IplImage *Img_dst;

	//投影位置
	double ProjectI[2000];

	//設定初值
	CurveImageRectify();
	//設定曲率
	int CurveSet(double CurveR);
	//設定使用者距離
	int GeoSet(double CurveR,unsigned short UserD);
	int GeoSet2(double CurveR,unsigned short UserD);

	int update();
};
