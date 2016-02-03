// HelloSenz3d.cpp : 定義主控台應用程式的進入點。
//

#include "stdafx.h"
//Used to render the images in a windows
//#include "util_render.h"
//allows to retrieve the camera stream
//#include "util_pipeline.h"

#include "FaceDetection.h"
//#include "CurveImageRectify.h"

int _tmain(int argc, _TCHAR* argv[])
{
/*
	CurveImageRectify myRectify;

	myRectify.GeoSet(30,1500);

	if( !(myRectify.Img_src= cvLoadImage("chessboard.jpg", 1)) ) 
	{
		printf("\n #Error(Panel_Display):READ_IMAGE_FAIL1"); 
		getchar();
		exit(-1);
	}




	myRectify.update();
	cvShowImage("1",myRectify.Img_src);
	cvShowImage("2",myRectify.Img_dst);
 	cvSaveImage("Img_dst.bmp",myRectify.Img_dst,0);//debug
*/
	FacePipeline* p= new FacePipeline();
	p->LoopFrames();
	delete p; 

/*
	//Structure used to accesss the image
	UtilPipeline pipeline;
	//Tell the framework that you are interested by the depth image
	pipeline.EnableImage(PXCImage::COLOR_FORMAT_DEPTH);
	pipeline.EnableImage(PXCImage::COLOR_FORMAT_RGB32);
	//initialize the pipeline
	pipeline.Init();

	UtilRender depth_render(L"Depth Stream Window");
	UtilRender color_render(L"Color Stream Window");

	//The main loop
	while(true){
		if(!pipeline.AcquireFrame(true)) 
			break;

		PXCImage *depth_image=pipeline.QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
		PXCImage *color_image=pipeline.QueryImage(PXCImage::IMAGE_TYPE_COLOR);

		if(!depth_render.RenderFrame(depth_image)) 
			break;

		if(!color_render.RenderFrame(color_image)) 
			break;

		pipeline.ReleaseFrame();
	}
	//Close the pipeline
	pipeline.Close();
*/
	return 0;
}

