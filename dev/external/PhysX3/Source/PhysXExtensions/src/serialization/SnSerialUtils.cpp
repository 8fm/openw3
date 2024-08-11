// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SnSerialUtils.h"

using namespace physx;

namespace
{

#define SN_NUM_BINARY_PLATFORMS 17
const PxU32 sBinaryPlatformTags[SN_NUM_BINARY_PLATFORMS] =
{
	PX_MAKE_FOURCC('W','_','3','2'),
	PX_MAKE_FOURCC('W','_','6','4'),
	PX_MAKE_FOURCC('L','_','3','2'),
	PX_MAKE_FOURCC('L','_','6','4'),
	PX_MAKE_FOURCC('M','_','3','2'),
	PX_MAKE_FOURCC('M','_','6','4'),
	PX_MAKE_FOURCC('M','O','C','A'),
	PX_MAKE_FOURCC('P','S','_','3'),
	PX_MAKE_FOURCC('X','3','6','0'),
	PX_MAKE_FOURCC('A','N','D','R'),
	PX_MAKE_FOURCC('A','I','O','S'),
	PX_MAKE_FOURCC('P','S','P','2'),
	PX_MAKE_FOURCC('W','A','R','M'),
	PX_MAKE_FOURCC('W','I','I','U'),
	PX_MAKE_FOURCC('W','3','2','M'),
	PX_MAKE_FOURCC('W','6','4','M'),
	PX_MAKE_FOURCC('X','O','N','E')
};

const char* sBinaryPlatformNames[SN_NUM_BINARY_PLATFORMS] =
{
	"win32",
	"win64",
	"linux32",
	"linux64",
	"macOSX32",
	"macOSX64",
	"ps4",
	"ps3",
	"xbox360",
	"android",
	"ios",
	"psp2",
	"win8arm"
	"wiiu",
	"win32modern",
	"win64modern",
	"xboxone"
};

}

namespace physx { namespace Sn {

PxU32 getBinaryPlatformTag()
{
#if defined(PX_WINDOWS) && defined(PX_X86) && !defined(PX_WINMODERN)
	return sBinaryPlatformTags[0];
#elif defined(PX_WINDOWS) && defined(PX_X64) && !defined(PX_WINMODERN)
	return sBinaryPlatformTags[1];
#elif defined(PX_LINUX) && defined(PX_X86)
	return sBinaryPlatformTags[2];
#elif defined(PX_LINUX) && defined(PX_X64)
	return sBinaryPlatformTags[3];
#elif defined(PX_APPLE) && defined(PX_X86)
	return sBinaryPlatformTags[4];
#elif defined(PX_APPLE) && defined(PX_X64)
	return sBinaryPlatformTags[5];
#elif defined(PX_PS4)
	return sBinaryPlatformTags[6];
#elif defined(PX_PS3)
	return sBinaryPlatformTags[7];
#elif defined(PX_X360)
	return sBinaryPlatformTags[8];
#elif defined(PX_ANDROID)
	return sBinaryPlatformTags[9];
#elif defined(PX_APPLE_IOS)
	return sBinaryPlatformTags[10];
#elif defined(PX_PSP2)
	return sBinaryPlatformTags[11];
#elif defined(PX_WINMODERN) && defined(PX_ARM)
	return sBinaryPlatformTags[12];
#elif defined(PX_WIIU)
	return sBinaryPlatformTags[13];
#elif defined(PX_WINMODERN) && defined(PX_X86)
	return sBinaryPlatformTags[14];
#elif defined(PX_WINMODERN) && defined(PX_X64)
	return sBinaryPlatformTags[15];
#elif defined(PX_XBOXONE)
	return sBinaryPlatformTags[16];
#else
	#error Unknown binary platform
#endif
}

bool isBinaryPlatformTagValid(physx::PxU32 platformTag)
{
	PxU32 platformIndex = 0;
	while (platformIndex < SN_NUM_BINARY_PLATFORMS && platformTag != sBinaryPlatformTags[platformIndex]) platformIndex++;
	return platformIndex < SN_NUM_BINARY_PLATFORMS;
}

const char* getBinaryPlatformName(physx::PxU32 platformTag)
{
	PxU32 platformIndex = 0;
	while (platformIndex < SN_NUM_BINARY_PLATFORMS && platformTag != sBinaryPlatformTags[platformIndex]) platformIndex++;
	return (platformIndex == SN_NUM_BINARY_PLATFORMS) ? "unknown" : sBinaryPlatformNames[platformIndex];
}

} // Sn
} // physx

