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

#include "ApexAssetAuthoring.h"

#include "P4Info.h"
#include "PsString.h"

#include "NxPhysXSDKVersion.h"
#include "NiApexSDK.h"



namespace physx
{
namespace apex
{


void ApexAssetAuthoring::setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
{
#ifdef APEX_SHIPPING
	PX_UNUSED(toolName);
	PX_UNUSED(toolVersion);
	PX_UNUSED(toolChangelist);
#else
	const PxU32 buflen = 256;
	char buf[buflen];
	string::strcpy_s(buf, buflen, toolName);
	string::strcat_s(buf, buflen, " ");

	if (toolVersion != NULL)
	{
		string::strcat_s(buf, buflen, toolVersion);
		string::strcat_s(buf, buflen, ":");
	}

	if (toolChangelist == 0)
	{
		toolChangelist = P4_TOOLS_CHANGELIST;
	}

	{
		char buf2[14];
		string::sprintf_s(buf2, 14, "CL %d", toolChangelist);
		string::strcat_s(buf, buflen, buf2);
		string::strcat_s(buf, buflen, " ");
	}
	
	{
#ifdef WIN64
		string::strcat_s(buf, buflen, "Win64 ");
#elif defined(WIN32)
		string::strcat_s(buf, buflen, "Win32 ");
#endif
	}

	{
		string::strcat_s(buf, buflen, "(Apex ");
		string::strcat_s(buf, buflen, P4_APEX_VERSION_STRING);
		char buf2[20];
		string::sprintf_s(buf2, 20, ", CL %d, ", P4_CHANGELIST);
		string::strcat_s(buf, buflen, buf2);
#ifdef _DEBUG
		string::strcat_s(buf, buflen, "DEBUG ");
#elif defined(PHYSX_PROFILE_SDK)
		string::strcat_s(buf, buflen, "PROFILE ");
#endif
		string::strcat_s(buf, buflen, P4_APEX_BRANCH);
		string::strcat_s(buf, buflen, ") ");
	}

	{
		string::strcat_s(buf, buflen, "(PhysX ");

		char buf2[10] = { 0 };
#if NX_SDK_VERSION_MAJOR == 2
		union
		{
			physx::PxU32 _unsigned;
			physx::PxU8 _chars[4];
		};
		_unsigned = NX_PHYSICS_SDK_VERSION;

		string::sprintf_s(buf2, 10, "%d.%d.%d) ", _chars[3], _chars[2], _chars[1]);
#elif NX_SDK_VERSION_MAJOR == 3
		string::sprintf_s(buf2, 10, "%d.%d) ", PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR);
#endif
		string::strcat_s(buf, buflen, buf2);
	}


	string::strcat_s(buf, buflen, "Apex Build Time: ");
	string::strcat_s(buf, buflen, P4_BUILD_TIME);

	//PxU32 len = strlen(buf);
	//len = len;

	//"<toolName> <toolVersion>:<toolCL> <platform> (Apex <apexVersion>, CL <apexCL> <apexConfiguration> <apexBranch>) (PhysX <physxVersion>) <toolBuildDate>"

	setToolString(buf);
#endif
}



void ApexAssetAuthoring::setToolString(const char* /*toolString*/)
{
	PX_ALWAYS_ASSERT();
	APEX_INVALID_OPERATION("Not Implemented.");
}

} // namespace apex
} // namespace physx