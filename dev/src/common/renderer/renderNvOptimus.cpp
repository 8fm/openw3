
#include "build.h"

#ifdef RED_PLATFORM_WINPC

//***************************************************************************************************************//
//***************************************************************************************************************//
// NVIDIA
// 
// 
// Starting with the Release 302 drivers, application developers can direct the Optimus
// driver at runtime to use the High Performance Graphics to render any application–even
// those applications for which there is no existing application profile. They can do this by
// exporting a global variable named NvOptimusEnablement. The Optimus driver looks for
// the existence and value of the export. Only the LSB of the DWORD matters at this time. A
// value of 0x00000001 indicates that rendering should be performed using High
// Performance Graphics. A value of 0x00000000 indicates that this method should be
// ignored. 

extern "C" 
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

//***************************************************************************************************************//
//***************************************************************************************************************//
// AMD
// 
// According to http://devgurus.amd.com/thread/169965
// 
// This will select the high performance GPU as long as no profile exists that assigns the application to another GPU.
// Please make sure to use a 13.35 or newer driver. Older drivers do not support this.

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

//***************************************************************************************************************//
//***************************************************************************************************************//

#endif // RED_PLATFORM_WINPC
