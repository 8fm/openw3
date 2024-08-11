#pragma once

#include "../redSystem/os.h"
#include "../redSystem/architecture.h"

#ifdef USE_ANSEL

//#define USE_ANSEL_CAMERA_LIMIT

	#include "NvCameraSDK.h"

	#define CAMERAWORKS_LIB_PATH "..\\..\\..\\external\\Ansel\\lib\\"

	#pragma comment(lib, CAMERAWORKS_LIB_PATH "NvCameraSDK64.lib")
#endif

