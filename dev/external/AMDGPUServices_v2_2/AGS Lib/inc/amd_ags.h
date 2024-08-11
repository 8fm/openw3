//-----------------------------------------------------------------------------
// File: amd_ags.h
// Copyright (c) 2008 - 2015 Advanced Micro Devices, Inc. All rights reserved.
// AGS v2.2
//-----------------------------------------------------------------------------

#ifndef __AMD_AGS_H
#define __AMD_AGS_H

#include <windows.h>
#include <stdlib.h>

typedef enum _AGSReturnCode
{
	AGS_ERROR_MISSING_DLL = -2,
	AGS_ERROR_LEGACY_DRIVER = -1,
	AGS_FAILURE = 0,
	AGS_SUCCESS = 1
} AGSReturnCode;

typedef struct _AGSSimpleRectStruct {

	int iXOffset;
	int iYOffset;
	int iWidth;
	int iHeight;

} AGSSimpleRectStruct;

typedef struct _AGSAGSEyefinityInfoStruct {

	int iSLSActive; // Indicates if Eyefinity is active for the operating system display
					// index passed into atiEyefinityGetConfigInfo(). 1 if enabled and 0 if disabled.

	int iSLSGridWidth;  // Contains width of the multi-monitor grid that makes up the Eyefinity Single Large Surface.
					    // For example, a 3 display wide by 2 high Eyefinity setup will return 3 for this entry.
	int iSLSGridHeight; // Contains height of the multi-monitor grid that makes up the Eyefinity Single Large Surface.
						// For example, a 3 display wide by 2 high Eyefinity setup will return 2 for this entry.

	int iSLSWidth;  // Contains width in pixels of the multi-monitor Single Large Surface. The value returned is
	                // a function of the width of the SLS grid, of the horizontal resolution of each display, and
	                // of whether or not bezel compensation is enabled.
	int iSLSHeight; // Contains height in pixels of the multi-monitor Single Large Surface. The value returned is
	                // a function of the height of the SLS grid, of the vertical resolution of each display, and
	                // of whether or not bezel compensation is enabled.

	int iBezelCompensatedDisplay; // Indicates if bezel compensation is used for the current SLS display area.
	                              // 1 if enabled, and 0 if disabled.

} AGSEyefinityInfoStruct;

typedef struct _AGSAGSDisplayInfoStruct {

	int iGridXCoord; // Contains horizontal SLS grid coordinate of the display. The value is zero based with
	                 // increasing values from left to right of the overall SLS grid. For example, the left-most
	                 // display of a 3x2 Eyefinity setup will have the value 0, and the right-most will have
	                 // the value 2.
	int iGridYCoord; // Contains vertical SLS grid coordinate of the display. The value is zero based with
	                 // increasing values from top to bottom of the overall SLS grid. For example, the top
	                 // display of a 3x2 Eyefinity setup will have the value 0, and the bottom will have the
	                 // value 1.

	AGSSimpleRectStruct displayRect;     // Contains the base offset and dimensions in pixels of the SLS rendering
	                                     // area associated with this display. If bezel compensation is enabled, this
	                                     // area will be larger than what the display can natively present to account
	                                     // for bezel area. If bezel compensation is disabled, this area will be equal
	                                     // to what the display can support natively.

	AGSSimpleRectStruct displayRectVisible; // Contains the base offset and dimensions in pixels of the SLS rendering area
	                                        // associated with this display that is visible to the end user. If bezel
	                                        // compensation is enabled, this area will be equal to what the display can
	                                        // natively, but smaller that the area described in the displayRect entry. If
	                                        // bezel compensation is disabled, this area will be equal to what the display
	                                        // can support natively and equal to the area described in the displayRect entry.
	                                        // Developers wishing to place UI, HUD, or other game assets on a given display
	                                        // so that it is visible and accessible to end users need to locate them inside
	                                        // of the region defined by this rect.

	int iPreferredDisplay; // Indicates whether or not this display is the preferred one for rendering of
	                       // game HUD and UI elements. Only one display out of the whole SLS grid will have
	                       // this be true if it is the preferred display and 0 otherwise. Developers wishing
	                       // to place specific UI, HUD, or other game assets on a given display so that it
	                       // is visible and accessible to end users need to locate them inside of the region
	                       // defined by this rect.

} AGSDisplayInfoStruct;

typedef struct _AGSDriverVersionInfoStruct {

	char strDriverVersion[256];
	char strCatalystVersion[256];
	char strCatalystWebLink[256];

} AGSDriverVersionInfoStruct;

typedef struct _AGSLatestDriverVersionInfoStruct {

	char strDriverVersion[256];
	char strDriverWebLink[256];

} AGSLatestDriverVersionInfoStruct;


// Description
//   Function used to initialize the AGS library.
//   Must be called prior to any of the subsequent AGS API calls.
//   nb. Initializes static members so take care when using the dll version from multiple threads.
//
// Input params
//   none
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
//   AGS_ERROR_MISSING_DLL - Failed to load the AMD Display Library (ADL)
AGSReturnCode agsInit();

// Description
//   Function used to clean up the AGS library.
//
// Input params
//   none
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsDeInit();

// Description
//   Function used to query the number of GPUs used for Crossfire acceleration.
//	 This may be different from the total number of GPUs present in the system
//	 which you can query using agsGPUGetDeviceCount which reports all GPUs,
// 	 even if they are not configured for Crossfire.
//
// Input params
//   none
//
// Output params
// 	 lpNumGPUs - Number of GPUs used for Crossfire acceleration
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsCrossfireGetGPUCount( int iOSDisplayIndex, int *lpNumGPUs );

// Description
//   Function used to query Eyefinity configuration state information relevant to ISVs. State info returned
//   includes: whether Eyefinity is enabled or not, SLS grid configuration, SLS dimensions, whether bezel
//   compensation is enabled or not, SLS grid coordinate for each display, total rendering area for each
//   display, visible rendering area for each display, and a preferred display flag.
//
// Input params
//   iOSDisplayIndex - Operating system specific display index identifier. The value used should be the
//                     index of the display used for rendering operations. On Windows operating systems,
//                     the value can be queried using the EnumDisplayDevices() API.
//
// Output params
// 	 pEyefinitiInfo       ?This is a pointer to a EyefinitiInfoStruct structure that contains system Eyefinity
//                          configuration information.
//   lpNumDisplaysInfo    ?Pointer to the number of AGSDisplayInfoStruct structures stored in the returned
//                          lppAGSDisplayInfoStruct array. The value returned is equal to the number of displays
//                          used for the Eyefinity setup.
//   lppAGSDisplayInfoStruct ?Pointer to an array of AGSDisplayInfoStruct structures that contains per display
//                          Eyefinity configuration information.
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsEyefinityGetConfigInfo( int iOSDisplayIndex, AGSEyefinityInfoStruct *lpEyefinityInfo, int *lpNumDisplaysInfo, AGSDisplayInfoStruct **lppDisplaysInfo );

// Description
//   Function used to query version info of the currenly installed driver version.
//
// Input params
//   none
//
// Output params
// 	 lpNumGPUs - Number of GPUs used for Crossfire acceleration
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsDriverGetVersionInfo ( AGSDriverVersionInfoStruct *lpDriverVersionInfo );

// Description
//   Function used to query version number and URL of latest driver.
//
// Input params
//   none
//
// Output params
// 	 lpLatestDriverVerInfo - version number string and URL string of latest driver.
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsDriverGetLatestVersionInfo ( AGSLatestDriverVersionInfoStruct* lpLatestDriverVerInfo );

// Description
//   Function used to query the number of GPUs in the system.
//	 This number may be different from agsCrossfireGetGPUCount as it reports
//	 all devices installed in the system, and not only those configured for
//	 Crossfire.
//
// Input params
//   none
//
// Output params
// 	 lpNumGPUs - Number of GPUs in the system.
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsGPUGetDeviceCount ( int* lpDeviceCount );

// Description
//   Function used to query the memory size of a GPU. The number of GPUs should
//	 be obtained using agsGPUGetDeviceCount ()
//
// Input params
//   iDeviceNumber - The device number to query
//
// Output params
// 	 lpNumGPUs - Memory size on the device in bytes
//
// Return code
//   AGS_SUCCESS - Success
//   AGS_FAILURE - Failure
AGSReturnCode agsGPUGetDeviceMemorySize ( int iDeviceNumber, long long* lpSizeInBytes );

typedef AGSReturnCode (*AGSINIT)();
typedef AGSReturnCode (*AGSDEINIT)();
typedef AGSReturnCode (*AGSCROSSFIREGETGPUCOUNT)( int, int* );
typedef AGSReturnCode (*AGSEYEFINITYGETCONFIGINFO) ( int, AGSEyefinityInfoStruct*, int*, AGSDisplayInfoStruct** );
typedef AGSReturnCode (*AGSDRIVERGETVERSIONINFO)( AGSDriverVersionInfoStruct* );
typedef AGSReturnCode (*AGSDRIVERGETLATESTVERSIONINFO)( AGSLatestDriverVersionInfoStruct* );
typedef AGSReturnCode (*AGSGPUGETDEVICECOUNT)( int* );
typedef AGSReturnCode (*AGSGPUGETDEVICEMEMORYSIZE) ( int, long long* );


__inline AGSReturnCode AGSInit()
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSINIT agsInit = (AGSINIT)GetProcAddress(lib, "agsInit");
	if (NULL == agsInit)
	{
		FreeLibrary(lib);
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	iReturnValue = agsInit();

	return iReturnValue;
}


__inline AGSReturnCode AGSDeInit()
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSINIT agsDeInit = (AGSDEINIT)GetProcAddress(lib, "agsDeInit");
	if (NULL == agsDeInit)
	{
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	iReturnValue = agsDeInit();

#ifdef _AMD_AGS_USE_DLL
	// Free the lib
	FreeLibrary(lib);
#endif // _AMD_AGS_USE_DLL

	return iReturnValue;
}


__inline AGSReturnCode AGSGetDefaultDisplayIndex( int *lpOSDisplayIndex )
{
	int iDevNum = 0;
	DISPLAY_DEVICE displayDevice;

	if ( NULL == lpOSDisplayIndex )
	{
		return AGS_FAILURE;
	}

	displayDevice.cb = sizeof(displayDevice);

	while ( EnumDisplayDevices(0, iDevNum, &displayDevice, 0) )
	{
		if (0 != (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) )
		{
			*lpOSDisplayIndex = iDevNum;
			break;
		}
		iDevNum++;
	}

	return AGS_SUCCESS;
}

__inline AGSReturnCode AGSCrossfireGetGPUCount( int iOSDisplayIndex, int *lpNumGPUs )
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

	// Validate params
	if ( NULL == lpNumGPUs )
	{
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSCROSSFIREGETGPUCOUNT agsCrossfireGetGPUCount = NULL;
	agsCrossfireGetGPUCount = (AGSCROSSFIREGETGPUCOUNT)GetProcAddress(lib, "agsCrossfireGetGPUCount");
	if (NULL == agsCrossfireGetGPUCount)
	{
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	// Get the number of GPUs
	iReturnValue = agsCrossfireGetGPUCount( iOSDisplayIndex, lpNumGPUs );

	return iReturnValue;
}

__inline AGSReturnCode AGSEyefinityFillConfigInfo ( AGSEyefinityInfoStruct *lpEyefinityInfo, int *lpNumDisplaysInfo, AGSDisplayInfoStruct **lppDisplaysInfo )
{
	AGSDisplayInfoStruct *lpDisplaysInfo = *lppDisplaysInfo;

	int iCurrentColumn = 0;
	int iCurrentRow = 0;
	int iCurrentDisplay = 0;
	int iPreferredGridIndexX = -1;
	int iPreferredGridIndexY = -1;

	AGSReturnCode iReturnValue = AGS_SUCCESS;

	if ( NULL == lpEyefinityInfo ||
		 NULL == lpNumDisplaysInfo ||
		 NULL == lppDisplaysInfo ||
		 NULL == *lppDisplaysInfo )
	{
		return AGS_FAILURE;
	}

	lpDisplaysInfo = *lppDisplaysInfo;

	// Try to use the center-most column for gaming UI & HUD elements
	switch ( lpEyefinityInfo->iSLSGridWidth )
	{
	case 3:
		iPreferredGridIndexX = 1;
		break;
	case 5:
		iPreferredGridIndexX = 2;
		break;
	}

	// Always default to the bottom row for gaming UI & HUD elements
	if ( lpEyefinityInfo->iSLSGridWidth != 2 )
	{
		iPreferredGridIndexY = lpEyefinityInfo->iSLSGridHeight - 1;
	}

	if ( lpEyefinityInfo->iSLSGridWidth > 1 ||
		 lpEyefinityInfo->iSLSGridHeight > 1 )
	{
		lpEyefinityInfo->iSLSActive = TRUE;
	}
	else
	{
		lpEyefinityInfo->iSLSActive = FALSE;
		lpEyefinityInfo->iBezelCompensatedDisplay = FALSE;
	}

	// For each display...
	for ( iCurrentColumn = 0; iCurrentColumn < lpEyefinityInfo->iSLSGridWidth; iCurrentColumn++ )
	{
		for ( iCurrentRow = 0; iCurrentRow < lpEyefinityInfo->iSLSGridHeight; iCurrentRow++ )
		{
			iCurrentDisplay = iCurrentColumn + ( iCurrentRow * lpEyefinityInfo->iSLSGridWidth );

			// Set the grid coord
			lpDisplaysInfo[iCurrentDisplay].iGridXCoord = iCurrentColumn;
			lpDisplaysInfo[iCurrentDisplay].iGridYCoord = iCurrentRow;

			// Set the display total rect
			lpDisplaysInfo[iCurrentDisplay].displayRect.iWidth = lpEyefinityInfo->iSLSWidth / lpEyefinityInfo->iSLSGridWidth;
			lpDisplaysInfo[iCurrentDisplay].displayRect.iHeight = lpEyefinityInfo->iSLSHeight / lpEyefinityInfo->iSLSGridHeight;
			lpDisplaysInfo[iCurrentDisplay].displayRect.iXOffset = iCurrentColumn * lpDisplaysInfo[iCurrentDisplay].displayRect.iWidth;
			lpDisplaysInfo[iCurrentDisplay].displayRect.iYOffset = iCurrentRow * lpDisplaysInfo[iCurrentDisplay].displayRect.iHeight;

			// Set the display visible rect
			lpDisplaysInfo[iCurrentDisplay].displayRectVisible = lpDisplaysInfo[iCurrentDisplay].displayRect;
			if ( TRUE == lpEyefinityInfo->iBezelCompensatedDisplay )
			{
				if ( lpEyefinityInfo->iSLSGridWidth > 1 )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iWidth = (int)(lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iWidth * 0.98);
				}
				if ( lpEyefinityInfo->iSLSGridHeight > 1 )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iHeight = (int)(lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iHeight * 0.98);
				}

				if ( iCurrentColumn == 0 )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iXOffset = 0;
				}
				else if ( iCurrentColumn == (lpEyefinityInfo->iSLSGridWidth-1) )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iXOffset = lpEyefinityInfo->iSLSWidth - lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iWidth;
				}
				else
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iXOffset = lpDisplaysInfo[iCurrentDisplay].displayRect.iXOffset + (lpDisplaysInfo[iCurrentDisplay].displayRect.iWidth - lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iWidth)/2;
				}

				if ( iCurrentRow == 0 )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iYOffset = 0;
				}
				else if ( iCurrentRow == (lpEyefinityInfo->iSLSGridHeight-1) )
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iYOffset = lpEyefinityInfo->iSLSHeight - lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iHeight;
				}
				else
				{
					lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iYOffset = lpDisplaysInfo[iCurrentDisplay].displayRect.iYOffset + (lpDisplaysInfo[iCurrentDisplay].displayRect.iHeight - lpDisplaysInfo[iCurrentDisplay].displayRectVisible.iHeight)/2;
				}
			}

			// Set the preferred display flag
			if ( iPreferredGridIndexX == lpDisplaysInfo[iCurrentDisplay].iGridXCoord &&
				 iPreferredGridIndexY == lpDisplaysInfo[iCurrentDisplay].iGridYCoord )
			{
				lpDisplaysInfo[iCurrentDisplay].iPreferredDisplay = TRUE;
			}
		}
	}

	return iReturnValue;
}

__inline AGSReturnCode AGSEyefinityDrawBezelBorder ( AGSEyefinityInfoStruct *lpEyefinityInfo, int *lpNumDisplaysInfo, AGSDisplayInfoStruct **lppDisplaysInfo )
{
	// Validate params
	if ( NULL == lpEyefinityInfo ||
		 NULL == lpNumDisplaysInfo ||
		 NULL == lppDisplaysInfo ||
		 NULL != *lppDisplaysInfo )
	{
		return AGS_FAILURE;
	}
}

__inline AGSReturnCode AGSEyefinityGetConfigInfo( int iOSDisplayIndex, AGSEyefinityInfoStruct *lpEyefinityInfo, int *lpNumDisplaysInfo, AGSDisplayInfoStruct **lppDisplaysInfo )
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;
	int bOverrideEyefinityConfigInfo = FALSE;

	// Validate params
	if ( NULL == lpEyefinityInfo ||
		 NULL == lpNumDisplaysInfo ||
		 NULL == lppDisplaysInfo ||
		 NULL != *lppDisplaysInfo )
	{
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSEYEFINITYGETCONFIGINFO agsEyefinityGetConfigInfo = NULL;
	agsEyefinityGetConfigInfo = (AGSEYEFINITYGETCONFIGINFO)GetProcAddress(lib, "agsEyefinityGetConfigInfo");
	if ( NULL == agsEyefinityGetConfigInfo )
	{
		iReturnValue = AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

#ifdef _DEBUG
	if ( lpEyefinityInfo->iSLSGridWidth != 0 &&
		 lpEyefinityInfo->iSLSGridHeight != 0 &&
		 lpEyefinityInfo->iSLSWidth != 0 &&
		 lpEyefinityInfo->iSLSHeight != 0 )
	{
		bOverrideEyefinityConfigInfo = TRUE;
	}
	else
#endif // _DEBUG
	{
		memset ( lpEyefinityInfo,'\0', sizeof(AGSEyefinityInfoStruct) );
	}

	if ( AGS_SUCCESS == iReturnValue )
	{
		if ( TRUE == bOverrideEyefinityConfigInfo )
		{
			*lpNumDisplaysInfo = lpEyefinityInfo->iSLSGridWidth * lpEyefinityInfo->iSLSGridHeight;
		}
		// Get the number of displays
		else
		{
			iReturnValue = agsEyefinityGetConfigInfo( iOSDisplayIndex, NULL, lpNumDisplaysInfo, NULL );
		}
	}

	if ( 0 != *lpNumDisplaysInfo )
	{
		// Allocate array of DisplayInfo structs
		if ( AGS_SUCCESS == iReturnValue )
		{
			*lppDisplaysInfo = (AGSDisplayInfoStruct*)malloc ( sizeof(AGSDisplayInfoStruct) * *lpNumDisplaysInfo );
			if ( NULL == *lppDisplaysInfo )
			{
				iReturnValue = AGS_FAILURE;
			}
			else
			{
				memset ( *lppDisplaysInfo,'\0', sizeof(AGSDisplayInfoStruct) * *lpNumDisplaysInfo );
			}
		}

		// Get the Eyefinity config info
		if ( AGS_SUCCESS == iReturnValue )
		{
			if ( TRUE == bOverrideEyefinityConfigInfo )
			{
				iReturnValue = AGSEyefinityFillConfigInfo ( lpEyefinityInfo, lpNumDisplaysInfo, lppDisplaysInfo );
			}
			else
			{
				iReturnValue = agsEyefinityGetConfigInfo ( iOSDisplayIndex, lpEyefinityInfo, lpNumDisplaysInfo, lppDisplaysInfo );
			}
		}
	}

	return iReturnValue;
}

__inline AGSReturnCode AGSEyefinityReleaseConfigInfo( AGSDisplayInfoStruct **lppDisplaysInfo )
{
	if ( NULL != lppDisplaysInfo &&
		 NULL != *lppDisplaysInfo )
	{
		free(*lppDisplaysInfo);
	}

	return AGS_SUCCESS;
}

__inline AGSReturnCode AGSDriverGetVersionInfo( AGSDriverVersionInfoStruct *lpDriverVersionInfo )
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

	// Validate params
	if ( NULL == lpDriverVersionInfo )
	{
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSDRIVERGETVERSIONINFO agsDriverGetVersionInfo = NULL;
	agsDriverGetVersionInfo = (AGSDRIVERGETVERSIONINFO)GetProcAddress(lib, "agsDriverGetVersionInfo");
	if (NULL == agsDriverGetVersionInfo)
	{
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	// Get the number of GPUs
	iReturnValue = agsDriverGetVersionInfo( lpDriverVersionInfo );

	return iReturnValue;
}

__inline AGSReturnCode AGSDriverGetLatestVersionInfo( AGSLatestDriverVersionInfoStruct* lpLatestDriverVerInfo )
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

	// Validate params
	if ( NULL == lpLatestDriverVerInfo )
	{
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary(TEXT("amd_ags.dll"));
	if (NULL == lib)
	{
		lib = LoadLibrary(TEXT("amd_ags64.dll"));
		if (NULL == lib)
		{
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSDRIVERGETLATESTVERSIONINFO agsDriverGetLatestVersionInfo = NULL;
	agsDriverGetLatestVersionInfo = (AGSDRIVERGETLATESTVERSIONINFO)GetProcAddress(lib, "agsDriverGetLatestVersionInfo");
	if (NULL == agsDriverGetLatestVersionInfo)
	{
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	// Get the number of GPUs
	iReturnValue = agsDriverGetLatestVersionInfo( lpLatestDriverVerInfo );

	return iReturnValue;
}

__inline AGSReturnCode AGSGPUGetDeviceCount (int* count)
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

	// Validate params
	if (NULL == count) {
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary (TEXT ("amd_ags.dll"));
	if (NULL == lib) {
		lib = LoadLibrary (TEXT ("amd_ags64.dll"));
		if (NULL == lib) {
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSGPUGETDEVICECOUNT agsGPUGetDeviceCount = NULL;
	agsGPUGetDeviceCount = (AGSGPUGETDEVICECOUNT)GetProcAddress (lib, "agsGPUGetDeviceCount");
	if (NULL == agsGPUGetDeviceCount) {
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	// Get the number of GPUs
	iReturnValue = agsGPUGetDeviceCount (count);

	return iReturnValue;
}

__inline AGSReturnCode AGSGPUGetDeviceMemorySize (int device, long long* lpSizeInBytes)
{
	AGSReturnCode iReturnValue = AGS_SUCCESS;

	// Validate params
	if (NULL == lpSizeInBytes) {
		return AGS_FAILURE;
	}

#ifdef _AMD_AGS_USE_DLL
	// Load the lib
	HINSTANCE lib = NULL;
	lib = LoadLibrary (TEXT ("amd_ags.dll"));
	if (NULL == lib) {
		lib = LoadLibrary (TEXT ("amd_ags64.dll"));
		if (NULL == lib) {
			return AGS_FAILURE;
		}
	}

	// Get the function pointer
	AGSGPUGETDEVICEMEMORYSIZE agsGPUGetDeviceMemorySize = NULL;
	agsGPUGetDeviceMemorySize = (AGSGPUGETDEVICEMEMORYSIZE)GetProcAddress (lib, "agsGPUGetDeviceMemorySize");
	if (NULL == agsGPUGetDeviceMemorySize) {
		return AGS_FAILURE;
	}
#endif // _AMD_AGS_USE_DLL

	// Get the number of GPUs
	iReturnValue = agsGPUGetDeviceMemorySize (device, lpSizeInBytes);

	return iReturnValue;
}

#endif // __AMD_AGS_H
