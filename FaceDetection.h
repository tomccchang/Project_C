#include "stdafx.h"
#include "util_pipeline.h"
#include "util_render.h"
#include "face_render.h"

//OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cv.h>
#include <highgui.h>

#include "CurveImageRectify.h"


class FacePipeline: public UtilPipeline {

public:
	//雙眼位置
	int Eye_LeftX;
	int Eye_LeftY;
	int Eye_RightX;
	int Eye_RightY;

	//雙眼距離
	unsigned short EyesZ;
	unsigned short EyesZ_previous;

	//影像指標
	unsigned char *ColorImgBuffer;
	IplImage *ColorImg;

	unsigned char *DepthBuffer;
	IplImage *DepthBufferPointer;
	unsigned short *DepthData;
	unsigned short *DepthDataAlign;
	unsigned short *DepthDataAlignSmooth;
	IplImage *DepthDataPointer16bit;//unsigned short
	IplImage *DepthDataPointer8bit;//unsigned char
	IplImage *DepthDataAlignPointer16bit;//unsigned short
	//IplImage *DepthImgPointer;
	//影像修正
	CurveImageRectify myRectify;

public:
	FacePipeline();


	~FacePipeline() {
		delete depth_render;//?
		if (m_face_render != NULL) delete m_face_render;
	}

protected:
	FaceRender* m_face_render;
	UtilRender* depth_render;
	short* DepthMap;

	//looping
	virtual bool OnNewFrame(void);


	void MapXY(float &x, float &y, PXCImage* depth, PXCImage::ImageInfo *cinfo);

	//消除深度資料的雜訊
	int DepthSmoothFilter();

	//深度影像依據彩色影像對位
	int Depth2ColorAlign(PXCImage* ColorFrame, PXCImage* DepthFrame,int i_center, int j_center, int RangeH, int RangeW);

};
