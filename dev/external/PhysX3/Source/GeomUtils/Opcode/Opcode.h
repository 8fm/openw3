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

#ifndef __OPCODE_H__
#define __OPCODE_H__

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessor
#ifdef PX_WINDOWS
	#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

	#include "PxBounds3.h"
	#include "CmPhysXCommon.h"
	#include "PsIntrinsics.h"

#define OPC_SUPPORT_VMX128

#if defined(_XBOX)
	#include "xbox360/OPC_IceHook_XBOX.h"
#elif defined(PX_WINDOWS)
	#include "windows/OPC_IceHook_WIN.h"
#elif defined(LINUX) || defined(__APPLE__)
	#include "unix/OPC_IceHook_UNIX.h"
#elif defined(__CELLOS_LV2__)
	#include "ps3/OPC_IceHook_PS3.h"
#elif defined (PX_WIIU)
	#include "wiiu/OPC_IceHook_WIIU.h"
#elif defined (PX_XBOXONE)
	#include "XboxOne/OPC_IceHook_XboxOne.h"
#endif

	#include "OPC_Settings.h"

#endif // __OPCODE_H__
