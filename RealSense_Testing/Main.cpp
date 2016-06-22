// main file
// http://www.samontab.com/web/2016/04/interfacing-intel-realsense-f200-with-opencv/

#include <cstdio>
#include <iostream>

using namespace std;

#include <pxcsensemanager.h>
#include <opencv2/opencv.hpp>

PXCSenseManager *pxcSenseManager;


PXCImage * CVMat2PXCImage(cv::Mat cvImage)
{
	PXCImage::ImageInfo iinfo;
	memset(&iinfo, 0, sizeof(iinfo));
	iinfo.width = cvImage.cols;
	iinfo.height = cvImage.rows;

	PXCImage::PixelFormat format;
	int type = cvImage.type();
	if (type == CV_8UC1)
		format = PXCImage::PIXEL_FORMAT_Y8;
	else if (type == CV_8UC3)
		format = PXCImage::PIXEL_FORMAT_RGB24;
	else if (type == CV_32FC1)
		format = PXCImage::PIXEL_FORMAT_DEPTH_F32;

	iinfo.format = format;


	PXCImage *pxcImage = pxcSenseManager->QuerySession()->CreateImage(&iinfo);

	PXCImage::ImageData data;
	pxcImage->AcquireAccess(PXCImage::ACCESS_WRITE, format, &data);

	data.planes[0] = cvImage.data;

	pxcImage->ReleaseAccess(&data);
	return pxcImage;
}


cv::Mat PXCImage2CVMat(PXCImage *pxcImage, PXCImage::PixelFormat format)
{
	PXCImage::ImageData data;
	pxcImage->AcquireAccess(PXCImage::ACCESS_READ, format, &data);

	int width = pxcImage->QueryInfo().width;
	int height = pxcImage->QueryInfo().height;
	if (!format)
		format = pxcImage->QueryInfo().format;

	int type;
	if (format == PXCImage::PIXEL_FORMAT_Y8)
		type = CV_8UC1;
	else if (format == PXCImage::PIXEL_FORMAT_RGB24)
		type = CV_8UC3;
	else if (format == PXCImage::PIXEL_FORMAT_DEPTH_F32)
		type = CV_32FC1;
	else if (format == PXCImage::PIXEL_FORMAT_DEPTH)
		type = CV_16UC1;

	cv::Mat ocvImage = cv::Mat(cv::Size(width, height), type, data.planes[0]);


	pxcImage->ReleaseAccess(&data);
	return ocvImage;
}

int main(int argc, char* argv[])
{
	//Define some parameters for the camera
	cv::Size frameSize = cv::Size(640, 480);
	float frameRate = 60;

	//Create the OpenCV windows and images
	cv::namedWindow("IR", cv::WINDOW_NORMAL);
	cv::namedWindow("Color", cv::WINDOW_NORMAL);
	cv::namedWindow("Depth", cv::WINDOW_NORMAL);
	cv::Mat frameIR = cv::Mat::zeros(frameSize, CV_8UC1);
	cv::Mat frameColor = cv::Mat::zeros(frameSize, CV_8UC3);
	cv::Mat frameDepth = cv::Mat::zeros(frameSize, CV_8UC1);


	//Initialize the RealSense Manager
	pxcSenseManager = PXCSenseManager::CreateInstance();

	//Enable the streams to be used
	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_IR, frameSize.width, frameSize.height, frameRate);
	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_COLOR, frameSize.width, frameSize.height, frameRate);
	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, frameSize.width, frameSize.height, frameRate);

	//Initialize the pipeline
	pxcSenseManager->Init();

	bool keepRunning = true;
	while (keepRunning)
	{
		//Acquire all the frames from the camera
		pxcSenseManager->AcquireFrame();
		PXCCapture::Sample *sample = pxcSenseManager->QuerySample();

		//Convert each frame into an OpenCV image
		frameIR = PXCImage2CVMat(sample->ir, PXCImage::PIXEL_FORMAT_Y8);
		frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
		cv::Mat frameDepth_u16 = PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH);
		frameDepth_u16.convertTo(frameDepth, CV_8UC1);

		cv::Mat frameDisplay;
		cv::equalizeHist(frameDepth, frameDisplay);

		//Display the images
		cv::imshow("IR", frameIR);
		cv::imshow("Color", frameColor);
		cv::imshow("Depth", frameDisplay);

		//Check for user input
		int key = cv::waitKey(1);
		if (key == 27)
			keepRunning = false;

		//Release the memory from the frames
		pxcSenseManager->ReleaseFrame();
	}

	//Release the memory from the RealSense manager
	pxcSenseManager->Release();

	return 0;
}