// Grab_MultipleCameras.cpp
/*
Note: Before getting started, Basler recommends reading the Programmer's Guide topic
in the pylon C++ API documentation that gets installed with pylon.
If you are upgrading to a higher major version of pylon, Basler also
strongly recommends reading the Migration topic in the pylon C++ API documentation.

This sample illustrates how to grab and process images from multiple cameras
using the CInstantCameraArray class. The CInstantCameraArray class represents
an array of instant camera objects. It provides almost the same interface
as the instant camera for grabbing.
The main purpose of the CInstantCameraArray is to simplify waiting for images and
camera events of multiple cameras in one thread. This is done by providing a single
RetrieveResult method for all cameras in the array.
Alternatively, the grabbing can be started using the internal grab loop threads
of all cameras in the CInstantCameraArray. The grabbed images can then be processed by one or more
image event handlers. Please note that this is not shown in this example.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

//@@dyyin
#include "conio.h"//为了使用_getchar()函数实现不输入回车直接获取在控制台键入的字符
#include <string>//
#include <iostream>
//#include <pthread.h>
#include <windows.h>
#include <process.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10;

// Limits the amount of cameras used for grabbing.
// It is important to manage the available bandwidth when grabbing with multiple cameras.
// This applies, for instance, if two GigE cameras are connected to the same network adapter via a switch.
// To manage the bandwidth, the GevSCPD interpacket delay parameter and the GevSCFTD transmission delay
// parameter can be set for each GigE camera device.
// The "Controlling Packet Transmission Timing with the Interpacket and Frame Transmission Delays on Basler GigE Vision Cameras"
// Application Notes (AW000649xx000)
// provide more information about this topic.
// The bandwidth used by a FireWire camera device can be limited by adjusting the packet size.
static const size_t c_maxCamerasToUse = 2;

// Get all attached devices and exit application if no device is found.
DeviceInfoList_t devices;		// Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
CInstantCameraArray cameras(2);

CPylonImage imgLeft, imgRight;
bool bCapture = false, bCaptured = false;
DWORD WINAPI Thread_DisplayImg2Window(LPVOID lpParam)
{
	// Create an array of image windows.
	CPylonImageWindow imageWindowLeft, imageWindowRight;
	imageWindowLeft.Create(0,
		-700, 0, 640, 512);
	imageWindowRight.Create(1,
		90, 0, 640, 512);
	imageWindowLeft.Show();
	imageWindowRight.Show();

	int c_iCameraNum = 2;//摄像机的数量
	int iLeftCameraIndex = 0, iRightCameraIndex = 0;
	CGrabResultPtr ptrGrabResultL, ptrGrabResultR;
	CDeviceInfo aDeviceInfos[2];
	string strDeviceSerialNumbers[2];
	for (int i = 0; i < c_iCameraNum; i++)
	{
		aDeviceInfos[i] = cameras[i].GetDeviceInfo();
		strDeviceSerialNumbers[i] = aDeviceInfos[i].GetSerialNumber();

		//规定序列号为“21770467”的相机为左侧相机
		if (strDeviceSerialNumbers[i] == "21900819")
			iLeftCameraIndex = i;
		else if (strDeviceSerialNumbers[i] == "21770467")
			iRightCameraIndex = i;
	}

	cameras.StartGrabbing(Pylon::GrabStrategy_LatestImageOnly, Pylon::GrabLoop_ProvidedByUser);
	int cntImagesNum = 0;
	string strFileName = "";
	while (cameras.IsGrabbing())
	{
		cameras[iLeftCameraIndex].RetrieveResult(5000, ptrGrabResultL, TimeoutHandling_ThrowException);
		cameras[iRightCameraIndex].RetrieveResult(5000, ptrGrabResultR, TimeoutHandling_ThrowException);

		// If the image was grabbed successfully.
		if (ptrGrabResultL->GrabSucceeded() && ptrGrabResultR->GrabSucceeded())
		{
			imgLeft.AttachGrabResultBuffer(ptrGrabResultL);
			imgRight.AttachGrabResultBuffer(ptrGrabResultR);
		}
		if (imgLeft.IsValid() && imgRight.IsValid())
		{
			imageWindowLeft.SetImage(imgLeft);
			imageWindowRight.SetImage(imgRight);
		}
		if (bCapture == true)
		{
			cntImagesNum++;

			//左侧相机
			strFileName = "left" + to_string(cntImagesNum) + ".jpg";
			CImagePersistence::Save(ImageFileFormat_Jpeg, strFileName.c_str(), ptrGrabResultL);
			////右侧相机
			strFileName = "right" + to_string(cntImagesNum) + ".jpg";
			CImagePersistence::Save(ImageFileFormat_Jpeg, strFileName.c_str(), ptrGrabResultR);

			cout << "第" << cntImagesNum << "张图像已采集" << endl;
			//获取图像成功，更新标志位
			bCapture = false;
			bCaptured = true;
		}
		::Sleep(20);
	}
	return 1;
}

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Before using any pylon methods, the pylon runtime must be initialized. 
	PylonInitialize();

	try
	{
		// Get the transport layer factory.
		CTlFactory& tlFactory = CTlFactory::GetInstance();

		//// Get all attached devices and exit application if no device is found.
		//DeviceInfoList_t devices;
		if (tlFactory.EnumerateDevices(devices) == 0)
		{
			throw RUNTIME_EXCEPTION("No camera present.");
		}

		//// Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
		//CInstantCameraArray cameras(min(devices.size(), c_maxCamerasToUse));

		// Create and attach all Pylon Devices.
		for (size_t i = 0; i < cameras.GetSize(); ++i)
		{
			cameras[i].Attach(tlFactory.CreateDevice(devices[i]));

			// Print the model name of the camera.
			cout << "Using device " << cameras[i].GetDeviceInfo().GetModelName() << endl;
		}
		//
		//		// This smart pointer will receive the grab result data.
		//		CGrabResultPtr ptrGrabResult;
		//
		//		//// Starts grabbing for all cameras starting with index 0. The grabbing
		//		//// is started for one camera after the other. That's why the images of all
		//		//// cameras are not taken at the same time.
		//		//// However, a hardware trigger setup can be used to cause all cameras to grab images synchronously.
		//		//// According to their default configuration, the cameras are
		//		//// set up for free-running continuous acquisition.
		//		//cameras.StartGrabbing();
		//
		//
		////		// Grab c_countOfImagesToGrab from the cameras.
		////		for (int i = 0; i < c_countOfImagesToGrab && cameras.IsGrabbing(); ++i)
		////		{
		////			cameras.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
		////
		////			// When the cameras in the array are created the camera context value
		////			// is set to the index of the camera in the array.
		////			// The camera context is a user settable value.
		////			// This value is attached to each grab result and can be used
		////			// to determine the camera that produced the grab result.
		////			intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();
		////
		////#ifdef PYLON_WIN_BUILD
		////			// Show the image acquired by each camera in the window related to each camera.
		////			Pylon::DisplayImage(cameraContextValue, ptrGrabResult);
		////#endif
		////
		////			// Print the index and the model name of the camera.
		////			cout << "Camera " << cameraContextValue << ": " << cameras[cameraContextValue].GetDeviceInfo().GetModelName() << endl;
		////
		////			// Now, the image data can be processed.
		////			cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << endl;
		////			cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
		////			cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
		////			const uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
		////			cout << "Gray value of first pixel: " << (uint32_t)pImageBuffer[0] << endl << endl;
		////		}


		//########################################### @@dyyin ###################################################################
		//创建线程，不断更新画面
		CreateThread(
			NULL,              // default security attributes
			0,                 // use default stack size  
			Thread_DisplayImg2Window,        // thread function 
			NULL,             // argument to thread function 
			0,                 // use default creation flags 
			NULL);           // returns the thread identifier 

		//用户每输入一次空格就抓取并保存一次图像
		cout << "Press 'k' to grub Picture." << endl;
		while ((_getch()) == 'k')
		{
			bCapture = true;
			while (bCaptured == false);
		}
		cout << "未检测到用户输入'k'，图像采集结束！";
		//#########################################################################################################################

	}
	catch (const GenericException &e)
	{
		// Error handling
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	// Releases all pylon resources. 
	PylonTerminate();

	return exitCode;
}
