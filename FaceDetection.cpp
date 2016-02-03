#include "StdAfx.h"
#include "FaceDetection.h"



FacePipeline::FacePipeline(void):UtilPipeline() {
	m_face_render = NULL;

	// Create a face renderer
	m_face_render = new  FaceRender(L"Face Viewer");

	depth_render=new  UtilRender(L"Depth Stream Window");

	// Enable the face landmark detector
	EnableFaceLandmark();

	//EnableFaceLocation();  // if face detection

	EnableImage(PXCImage::IMAGE_TYPE_DEPTH);

	//影像空間
	ColorImgBuffer=new unsigned char[640*480*3];
	memset (ColorImgBuffer,0,sizeof(unsigned char)*(640*480*3));
	ColorImg = cvCreateImage(cvSize(640,480),8,3);
	ColorImg->imageData=(char*)ColorImgBuffer;
	//ColorImg= cvLoadImage("R.bmp", 1);//debug

	DepthBuffer=new unsigned char[640*480];
	memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
	DepthBufferPointer = cvCreateImage(cvSize(640,480),8,1);
	DepthBufferPointer->imageData=(char*)DepthBuffer;

	DepthDataPointer16bit=cvCreateImage(cvSize(320,240),16,1);
	DepthDataPointer8bit=cvCreateImage(cvSize(320,240),8,1);
	DepthDataAlign=new unsigned short[640*480];
	DepthDataAlignPointer16bit=cvCreateImage(cvSize(640,480),16,1);

	DepthDataAlignSmooth=new unsigned short[640*480];
	memset (DepthDataAlignSmooth,0,sizeof(unsigned short)*(640*480));
	//display
	cvNamedWindow( "ColorImg", CV_WINDOW_AUTOSIZE );

	//cvNamedWindow( "2", CV_WINDOW_NORMAL);
	//cvSetWindowProperty("2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

	//預設觀看者距離
	EyesZ=500;
	EyesZ_previous=500;

	//影像修正
	if( !(myRectify.Img_src= cvLoadImage("ChessboardHD.jpg", 1)) ) 
	{
		printf("\n #Error(Panel_Display):READ_IMAGE_FAIL1"); 
		getchar();
		exit(-1);
	}
	//myRectify.DisplayResolutionH=600;
	//myRectify.DisplayResolutionW=600;
	myRectify.DisplayResolutionH=1280;
	myRectify.DisplayResolutionW=720;

}


void FacePipeline::MapXY(float &x, float &y, PXCImage* depth, PXCImage::ImageInfo *cinfo) {

		PXCImage::ImageInfo dinfo;
		depth->QueryInfo(&dinfo);

		PXCImage::ImageData ddata;
		depth->AcquireAccess(PXCImage::ACCESS_READ,&ddata);

		float *uvmap=(float*)ddata.planes[2];
		int index=((int)y)*dinfo.width+x;

		x=uvmap[index*2]*cinfo->width;
		y=uvmap[index*2+1]*cinfo->height;

		depth->ReleaseAccess(&ddata);
	}


	int FacePipeline::DepthSmoothFilter()
	{
		unsigned short DepthSum=0;
		int count=0;

		for(int i=10;i<470;i++){
			for(int j=10;j<630;j++){
				DepthSum=0;
				count=0;

				int a_min=i-1;
				int a_max=i+1;
				int b_min=j-1;
				int b_max=j+1;
				
				for(int a=a_min;a<=a_max;a++){
					for(int b=b_min;b<=b_max;b++){

						if(DepthDataAlign[640*a+b]>0){
							DepthSum+=DepthDataAlign[640*a+b];
							count++;;
						}//if
					}}//for a b

				if(count>0){
					DepthDataAlignSmooth[640*i+j]=(unsigned short)(DepthSum/count);
				}
				else{
					DepthDataAlignSmooth[640*i+j]=0;
				}
			}}//for ij
		return 0;
};

	bool FacePipeline::OnNewFrame(void) {

		//face 
		PXCFaceAnalysis *faceAnalyzer = QueryFace();
		PXCFaceAnalysis::Landmark *landmark =
			faceAnalyzer->DynamicCast<PXCFaceAnalysis::Landmark>();

		PXCFaceAnalysis::Landmark::LandmarkData LandmarkPt_LeftEye_INNER, LandmarkPt_LeftEye_OUTER
			, LandmarkPt_RightEye_INNER, LandmarkPt_RightEye_OUTER;

		// loop all faces
		m_face_render->ClearData();
		for (int fidx = 0; ; fidx++) {
			pxcUID fid = 0;
			pxcU64 timeStamp = 0;
			pxcStatus sts = faceAnalyzer->QueryFace(fidx, &fid, &timeStamp);
			if (sts < PXC_STATUS_NO_ERROR) break; // no more faces

			m_face_render->SetLandmarkData (landmark, fid);
			//m_face_render->PrintLandmarkData(landmark, fid);//Debug:list data on screen 

			//PXCFaceAnalysis::Landmark::ProfileInfo linfo={0};

			//取得眼睛位置
			if(fidx ==0){
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_INNER_CORNER,&LandmarkPt_LeftEye_INNER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_OUTER_CORNER,&LandmarkPt_LeftEye_OUTER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_INNER_CORNER,&LandmarkPt_RightEye_INNER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_OUTER_CORNER,&LandmarkPt_RightEye_OUTER);
				Eye_LeftX=int(0.5*(LandmarkPt_LeftEye_INNER.position.x+LandmarkPt_LeftEye_OUTER.position.x));
				Eye_LeftY=int(0.5*(LandmarkPt_LeftEye_INNER.position.y+LandmarkPt_LeftEye_OUTER.position.y));
				Eye_RightX=int(0.5*(LandmarkPt_RightEye_INNER.position.x+LandmarkPt_RightEye_OUTER.position.x));
				Eye_RightY=int(0.5*(LandmarkPt_RightEye_INNER.position.y+LandmarkPt_RightEye_OUTER.position.y));
			}
			//theCoords
		}

		//Color Image
		auto colorframe=QueryImage(PXCImage::IMAGE_TYPE_COLOR);

		//rendering
		m_face_render->RenderFrame(colorframe);


		//Depth Map
		PXCImage* DepthImage=QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
		PXCImage::ImageData DepthImage_data;
		DepthImage->AcquireAccess(PXCImage::ACCESS_READ, &DepthImage_data);

		DepthData=(unsigned short*)DepthImage_data.planes[0];
		cvSetData(DepthDataPointer16bit,(short*)DepthData,DepthDataPointer16bit->widthStep);//指向深度資料
		//cvSetData(DepthDataAlignPointer16bit,(short*)DepthDataAlign,DepthDataAlignPointer16bit->widthStep);//指向深度資料
		cvSetData(DepthDataAlignPointer16bit,(short*)DepthDataAlignSmooth,DepthDataAlignPointer16bit->widthStep);//指向深度資料
		//cvConvertScale(DepthDataPointer16bit,DepthDataPointer8bit,0.01,0);

		Depth2ColorAlign(colorframe, DepthImage,int(Eye_LeftY*0.5), int(Eye_LeftX*0.5), 30, 100);
		//Depth2ColorAlign(colorframe, DepthImage,int(Eye_LeftY*0.5), int(Eye_LeftX*0.5), 640, 480);
/*
		//座標maping
		PXCImage::ImageInfo colorInfo={0,0,0};
		colorframe->QueryInfo(&colorInfo);
		///清空影像
		memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
		memset (DepthDataAlign,0,sizeof(unsigned short)*(640*480));
		float A=0,B=0;
		for(float i=0;i<240;i++){
			for(float j=0;j<320;j++){
				B=j;
				A=i;
				//DepthBuffer[640*int(A)+int(B)]=255;
				MapXY(B,A,DepthImage,&colorInfo);
				if(A>=0 && A<480){
					if(B>=0 && B<640){
						//DepthBuffer[640*int(B)+int(A)]=255;
						DepthDataAlign[640*int(A)+int(B)]=DepthData[320*int(i)+int(j)];
					}}//if A B
			}}
*/
		DepthSmoothFilter();

		//User Map (for monitoring tracking result)
		memset (ColorImgBuffer,0,sizeof(unsigned char)*(640*480*3));
		if(Eye_LeftX>=0 && Eye_LeftX<640){
			if(Eye_LeftY>=0 && Eye_LeftY<480){
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+0]=0;
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+1]=255;
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+2]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+0]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+1]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+2]=255;
			}}

		//估算使用者深度
		unsigned short EyeRZ, EyeLZ;
		bool EyeR_flag=0,EyeL_flag=0;

		if(Eye_LeftY>=0 && Eye_LeftY<480){
			if(Eye_LeftX>=0 && Eye_LeftX<640){
				EyeLZ=DepthDataAlign[640*int(Eye_LeftY*0.5)*2+int(Eye_LeftX*0.5)*2];

				if(EyeLZ>0 && EyeLZ<1500)
					EyeL_flag=1;
			}}
		if(Eye_RightY>=0 && Eye_RightY<480){
			if(Eye_RightX>=0 && Eye_RightX<640){
				EyeRZ=DepthDataAlign[640*int(Eye_RightY*0.5)*2+int(Eye_RightX*0.5)*2];

				if(EyeRZ>0 && EyeRZ<1500)
					EyeR_flag=1;
			}}

        //確認深度是否正確	
		if(EyeL_flag==1 && EyeR_flag==1){
			EyesZ=(unsigned short)(0.5*(EyeRZ+EyeLZ));
		}else if(EyeL_flag==1 && EyeR_flag==0){
			EyesZ=EyeLZ;
			EyesZ_previous=EyesZ;
		}else if(EyeL_flag==0 && EyeR_flag==1){
			EyesZ=EyeRZ;
			EyesZ_previous=EyesZ;
		}else{
			EyesZ=EyesZ_previous;
		}



		char TextDisplay[100];
		sprintf(TextDisplay,"Distance: %d",EyesZ);
		cvPutText(ColorImg,TextDisplay,cvPoint(0,20),&cvFont(1,1),CV_RGB(0,255,0));

		cvShowImage( "ColorImg", ColorImg );
		cvShowImage( "DepthBufferPointer", DepthBufferPointer );
		cvShowImage( "DepthDataPointer16bit", DepthDataPointer16bit );
		cvShowImage( "DepthDataAlignPointer16bit", DepthDataAlignPointer16bit );

		int key=cvWaitKey(10);
		if( key == 27 ){
			cvSaveImage("DepthDataAlignPointer16bit.bmp",DepthDataAlignPointer16bit, 0);
			
			return 0;
		}

		//short* ar=(short*)data.planes[0];
		//int h=info.height;
		//int w=info.width;

		depth_render->RenderFrame(DepthImage);
		DepthImage->ReleaseAccess(&DepthImage_data);
		//image->Release();//不可用，應用ReleaseAccess

		//影像修正
		myRectify.GeoSet2(60,EyesZ);
		//myRectify.GeoSet(30,100);
		myRectify.update();
		cvShowImage("1",myRectify.Img_src);
		cvShowImage("2",myRectify.Img_dst);

		//cvMoveWindow("2", 0, 0);
		//HWND win_handle;

		return true;
	}

int  FacePipeline::Depth2ColorAlign(PXCImage* ColorFrame, PXCImage* DepthFrame,int i_center, int j_center, int RangeH, int RangeW){


		PXCImage::ImageInfo colorInfo={0,0,0};
		ColorFrame->QueryInfo(&colorInfo);
		///清空影像
		memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
		memset (DepthDataAlign,0,sizeof(unsigned short)*(640*480));

		int i_min=i_center-RangeH;
		int i_max=i_center+RangeH;
		int j_min=j_center-RangeW;
		int j_max=j_center+RangeW;

		if(i_min<0)
			i_min=0;

		if(i_max>240)
			i_max=240;

		if(j_min<0)
			j_min=0;

		if(j_max>320)
			j_max=320;

		float A=0,B=0;
		for(float i=i_min;i<i_max;i++){
			for(float j=j_min;j<j_max;j++){
				B=j;
				A=i;
				//DepthBuffer[640*int(A)+int(B)]=255;
				MapXY(B,A,DepthFrame,&colorInfo);
				//write
				if(A>=0 && A<480){
					if(B>=0 && B<640){
						//DepthBuffer[640*int(B)+int(A)]=255;
						DepthDataAlign[640*int(A)+int(B)]=DepthData[320*int(i)+int(j)];
					}}//if A B
			}}

		return 0;
}#include "StdAfx.h"
#include "FaceDetection.h"



FacePipeline::FacePipeline(void):UtilPipeline() {
	m_face_render = NULL;

	// Create a face renderer
	m_face_render = new  FaceRender(L"Face Viewer");

	depth_render=new  UtilRender(L"Depth Stream Window");

	// Enable the face landmark detector
	EnableFaceLandmark();

	//EnableFaceLocation();  // if face detection

	EnableImage(PXCImage::IMAGE_TYPE_DEPTH);

	//影像空間
	ColorImgBuffer=new unsigned char[640*480*3];
	memset (ColorImgBuffer,0,sizeof(unsigned char)*(640*480*3));
	ColorImg = cvCreateImage(cvSize(640,480),8,3);
	ColorImg->imageData=(char*)ColorImgBuffer;
	//ColorImg= cvLoadImage("R.bmp", 1);//debug

	DepthBuffer=new unsigned char[640*480];
	memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
	DepthBufferPointer = cvCreateImage(cvSize(640,480),8,1);
	DepthBufferPointer->imageData=(char*)DepthBuffer;

	DepthDataPointer16bit=cvCreateImage(cvSize(320,240),16,1);
	DepthDataPointer8bit=cvCreateImage(cvSize(320,240),8,1);
	DepthDataAlign=new unsigned short[640*480];
	DepthDataAlignPointer16bit=cvCreateImage(cvSize(640,480),16,1);

	DepthDataAlignSmooth=new unsigned short[640*480];
	memset (DepthDataAlignSmooth,0,sizeof(unsigned short)*(640*480));
	//display
	cvNamedWindow( "ColorImg", CV_WINDOW_AUTOSIZE );

	//cvNamedWindow( "2", CV_WINDOW_NORMAL);
	//cvSetWindowProperty("2", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

	//預設觀看者距離
	EyesZ=500;
	EyesZ_previous=500;

	//影像修正
	if( !(myRectify.Img_src= cvLoadImage("ChessboardHD.jpg", 1)) ) 
	{
		printf("\n #Error(Panel_Display):READ_IMAGE_FAIL1"); 
		getchar();
		exit(-1);
	}
	//myRectify.DisplayResolutionH=600;
	//myRectify.DisplayResolutionW=600;
	myRectify.DisplayResolutionH=1280;
	myRectify.DisplayResolutionW=720;

}


void FacePipeline::MapXY(float &x, float &y, PXCImage* depth, PXCImage::ImageInfo *cinfo) {

		PXCImage::ImageInfo dinfo;
		depth->QueryInfo(&dinfo);

		PXCImage::ImageData ddata;
		depth->AcquireAccess(PXCImage::ACCESS_READ,&ddata);

		float *uvmap=(float*)ddata.planes[2];
		int index=((int)y)*dinfo.width+x;

		x=uvmap[index*2]*cinfo->width;
		y=uvmap[index*2+1]*cinfo->height;

		depth->ReleaseAccess(&ddata);
	}


	int FacePipeline::DepthSmoothFilter()
	{
		unsigned short DepthSum=0;
		int count=0;

		for(int i=10;i<470;i++){
			for(int j=10;j<630;j++){
				DepthSum=0;
				count=0;

				int a_min=i-1;
				int a_max=i+1;
				int b_min=j-1;
				int b_max=j+1;
				
				for(int a=a_min;a<=a_max;a++){
					for(int b=b_min;b<=b_max;b++){

						if(DepthDataAlign[640*a+b]>0){
							DepthSum+=DepthDataAlign[640*a+b];
							count++;;
						}//if
					}}//for a b

				if(count>0){
					DepthDataAlignSmooth[640*i+j]=(unsigned short)(DepthSum/count);
				}
				else{
					DepthDataAlignSmooth[640*i+j]=0;
				}
			}}//for ij
		return 0;
};

	bool FacePipeline::OnNewFrame(void) {

		//face 
		PXCFaceAnalysis *faceAnalyzer = QueryFace();
		PXCFaceAnalysis::Landmark *landmark =
			faceAnalyzer->DynamicCast<PXCFaceAnalysis::Landmark>();

		PXCFaceAnalysis::Landmark::LandmarkData LandmarkPt_LeftEye_INNER, LandmarkPt_LeftEye_OUTER
			, LandmarkPt_RightEye_INNER, LandmarkPt_RightEye_OUTER;

		// loop all faces
		m_face_render->ClearData();
		for (int fidx = 0; ; fidx++) {
			pxcUID fid = 0;
			pxcU64 timeStamp = 0;
			pxcStatus sts = faceAnalyzer->QueryFace(fidx, &fid, &timeStamp);
			if (sts < PXC_STATUS_NO_ERROR) break; // no more faces

			m_face_render->SetLandmarkData (landmark, fid);
			//m_face_render->PrintLandmarkData(landmark, fid);//Debug:list data on screen 

			//PXCFaceAnalysis::Landmark::ProfileInfo linfo={0};

			//取得眼睛位置
			if(fidx ==0){
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_INNER_CORNER,&LandmarkPt_LeftEye_INNER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_LEFT_EYE_OUTER_CORNER,&LandmarkPt_LeftEye_OUTER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_INNER_CORNER,&LandmarkPt_RightEye_INNER);
				landmark->QueryLandmarkData(fid,PXCFaceAnalysis::Landmark::LABEL_RIGHT_EYE_OUTER_CORNER,&LandmarkPt_RightEye_OUTER);
				Eye_LeftX=int(0.5*(LandmarkPt_LeftEye_INNER.position.x+LandmarkPt_LeftEye_OUTER.position.x));
				Eye_LeftY=int(0.5*(LandmarkPt_LeftEye_INNER.position.y+LandmarkPt_LeftEye_OUTER.position.y));
				Eye_RightX=int(0.5*(LandmarkPt_RightEye_INNER.position.x+LandmarkPt_RightEye_OUTER.position.x));
				Eye_RightY=int(0.5*(LandmarkPt_RightEye_INNER.position.y+LandmarkPt_RightEye_OUTER.position.y));
			}
			//theCoords
		}

		//Color Image
		auto colorframe=QueryImage(PXCImage::IMAGE_TYPE_COLOR);

		//rendering
		m_face_render->RenderFrame(colorframe);


		//Depth Map
		PXCImage* DepthImage=QueryImage(PXCImage::IMAGE_TYPE_DEPTH);
		PXCImage::ImageData DepthImage_data;
		DepthImage->AcquireAccess(PXCImage::ACCESS_READ, &DepthImage_data);

		DepthData=(unsigned short*)DepthImage_data.planes[0];
		cvSetData(DepthDataPointer16bit,(short*)DepthData,DepthDataPointer16bit->widthStep);//指向深度資料
		//cvSetData(DepthDataAlignPointer16bit,(short*)DepthDataAlign,DepthDataAlignPointer16bit->widthStep);//指向深度資料
		cvSetData(DepthDataAlignPointer16bit,(short*)DepthDataAlignSmooth,DepthDataAlignPointer16bit->widthStep);//指向深度資料
		//cvConvertScale(DepthDataPointer16bit,DepthDataPointer8bit,0.01,0);

		Depth2ColorAlign(colorframe, DepthImage,int(Eye_LeftY*0.5), int(Eye_LeftX*0.5), 30, 100);
		//Depth2ColorAlign(colorframe, DepthImage,int(Eye_LeftY*0.5), int(Eye_LeftX*0.5), 640, 480);
/*
		//座標maping
		PXCImage::ImageInfo colorInfo={0,0,0};
		colorframe->QueryInfo(&colorInfo);
		///清空影像
		memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
		memset (DepthDataAlign,0,sizeof(unsigned short)*(640*480));
		float A=0,B=0;
		for(float i=0;i<240;i++){
			for(float j=0;j<320;j++){
				B=j;
				A=i;
				//DepthBuffer[640*int(A)+int(B)]=255;
				MapXY(B,A,DepthImage,&colorInfo);
				if(A>=0 && A<480){
					if(B>=0 && B<640){
						//DepthBuffer[640*int(B)+int(A)]=255;
						DepthDataAlign[640*int(A)+int(B)]=DepthData[320*int(i)+int(j)];
					}}//if A B
			}}
*/
		DepthSmoothFilter();

		//User Map (for monitoring tracking result)
		memset (ColorImgBuffer,0,sizeof(unsigned char)*(640*480*3));
		if(Eye_LeftX>=0 && Eye_LeftX<640){
			if(Eye_LeftY>=0 && Eye_LeftY<480){
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+0]=0;
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+1]=255;
				ColorImg->imageData[640*3*Eye_LeftY+3*Eye_LeftX+2]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+0]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+1]=0;
				ColorImg->imageData[640*3*Eye_RightY+3*Eye_RightX+2]=255;
			}}

		//估算使用者深度
		unsigned short EyeRZ, EyeLZ;
		bool EyeR_flag=0,EyeL_flag=0;

		if(Eye_LeftY>=0 && Eye_LeftY<480){
			if(Eye_LeftX>=0 && Eye_LeftX<640){
				EyeLZ=DepthDataAlign[640*int(Eye_LeftY*0.5)*2+int(Eye_LeftX*0.5)*2];

				if(EyeLZ>0 && EyeLZ<1500)
					EyeL_flag=1;
			}}
		if(Eye_RightY>=0 && Eye_RightY<480){
			if(Eye_RightX>=0 && Eye_RightX<640){
				EyeRZ=DepthDataAlign[640*int(Eye_RightY*0.5)*2+int(Eye_RightX*0.5)*2];

				if(EyeRZ>0 && EyeRZ<1500)
					EyeR_flag=1;
			}}

        //確認深度是否正確	
		if(EyeL_flag==1 && EyeR_flag==1){
			EyesZ=(unsigned short)(0.5*(EyeRZ+EyeLZ));
		}else if(EyeL_flag==1 && EyeR_flag==0){
			EyesZ=EyeLZ;
			EyesZ_previous=EyesZ;
		}else if(EyeL_flag==0 && EyeR_flag==1){
			EyesZ=EyeRZ;
			EyesZ_previous=EyesZ;
		}else{
			EyesZ=EyesZ_previous;
		}



		char TextDisplay[100];
		sprintf(TextDisplay,"Distance: %d",EyesZ);
		cvPutText(ColorImg,TextDisplay,cvPoint(0,20),&cvFont(1,1),CV_RGB(0,255,0));

		cvShowImage( "ColorImg", ColorImg );
		cvShowImage( "DepthBufferPointer", DepthBufferPointer );
		cvShowImage( "DepthDataPointer16bit", DepthDataPointer16bit );
		cvShowImage( "DepthDataAlignPointer16bit", DepthDataAlignPointer16bit );

		int key=cvWaitKey(10);
		if( key == 27 ){
			cvSaveImage("DepthDataAlignPointer16bit.bmp",DepthDataAlignPointer16bit, 0);
			
			return 0;
		}

		//short* ar=(short*)data.planes[0];
		//int h=info.height;
		//int w=info.width;

		depth_render->RenderFrame(DepthImage);
		DepthImage->ReleaseAccess(&DepthImage_data);
		//image->Release();//不可用，應用ReleaseAccess

		//影像修正
		myRectify.GeoSet2(60,EyesZ);
		//myRectify.GeoSet(30,100);
		myRectify.update();
		cvShowImage("1",myRectify.Img_src);
		cvShowImage("2",myRectify.Img_dst);

		//cvMoveWindow("2", 0, 0);
		//HWND win_handle;

		return true;
	}

int  FacePipeline::Depth2ColorAlign(PXCImage* ColorFrame, PXCImage* DepthFrame,int i_center, int j_center, int RangeH, int RangeW){


		PXCImage::ImageInfo colorInfo={0,0,0};
		ColorFrame->QueryInfo(&colorInfo);
		///清空影像
		memset (DepthBuffer,0,sizeof(unsigned char)*(640*480));
		memset (DepthDataAlign,0,sizeof(unsigned short)*(640*480));

		int i_min=i_center-RangeH;
		int i_max=i_center+RangeH;
		int j_min=j_center-RangeW;
		int j_max=j_center+RangeW;

		if(i_min<0)
			i_min=0;

		if(i_max>240)
			i_max=240;

		if(j_min<0)
			j_min=0;

		if(j_max>320)
			j_max=320;

		float A=0,B=0;
		for(float i=i_min;i<i_max;i++){
			for(float j=j_min;j<j_max;j++){
				B=j;
				A=i;
				//DepthBuffer[640*int(A)+int(B)]=255;
				MapXY(B,A,DepthFrame,&colorInfo);
				//write
				if(A>=0 && A<480){
					if(B>=0 && B<640){
						//DepthBuffer[640*int(B)+int(A)]=255;
						DepthDataAlign[640*int(A)+int(B)]=DepthData[320*int(i)+int(j)];
					}}//if A B
			}}

		return 0;
}
