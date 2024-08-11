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


#define THERE_IS_NO_INCLUDE_GUARD_FOR_A_REASON

//meta data
DECLARE_PVD_COMM_STREAM_EVENT( StringHandleEvent )
DECLARE_PVD_COMM_STREAM_EVENT( CreateClass )
DECLARE_PVD_COMM_STREAM_EVENT( DeriveClass )
DECLARE_PVD_COMM_STREAM_EVENT( CreateProperty )
DECLARE_PVD_COMM_STREAM_EVENT( CreatePropertyMessage )


//instance editing
DECLARE_PVD_COMM_STREAM_EVENT( CreateInstance )
DECLARE_PVD_COMM_STREAM_EVENT( SetPropertyValue )

DECLARE_PVD_COMM_STREAM_EVENT( BeginSetPropertyValue )
DECLARE_PVD_COMM_STREAM_EVENT( AppendPropertyValueData )
DECLARE_PVD_COMM_STREAM_EVENT( EndSetPropertyValue )

DECLARE_PVD_COMM_STREAM_EVENT( SetPropertyMessage )

DECLARE_PVD_COMM_STREAM_EVENT( BeginPropertyMessageGroup )
DECLARE_PVD_COMM_STREAM_EVENT( SendPropertyMessageFromGroup )
DECLARE_PVD_COMM_STREAM_EVENT( EndPropertyMessageGroup )

DECLARE_PVD_COMM_STREAM_EVENT( DestroyInstance )

DECLARE_PVD_COMM_STREAM_EVENT( PushBackObjectRef )
DECLARE_PVD_COMM_STREAM_EVENT( RemoveObjectRef )

//section system
DECLARE_PVD_COMM_STREAM_EVENT( BeginSection )
DECLARE_PVD_COMM_STREAM_EVENT( EndSection )

//Instance meta data
DECLARE_PVD_COMM_STREAM_EVENT( SetPickable )
DECLARE_PVD_COMM_STREAM_EVENT( SetColor )
DECLARE_PVD_COMM_STREAM_EVENT( SetIsTopLevel )

//Cameras
DECLARE_PVD_COMM_STREAM_EVENT( SetCamera )

DECLARE_PVD_COMM_STREAM_EVENT( AddProfileZone )
DECLARE_PVD_COMM_STREAM_EVENT( AddProfileZoneEvent )

//System
DECLARE_PVD_COMM_STREAM_EVENT( StreamEndEvent )

//ErrorStream
DECLARE_PVD_COMM_STREAM_EVENT( ErrorMessage )
//Origin Shifting
DECLARE_PVD_COMM_STREAM_EVENT( OriginShift )

#undef THERE_IS_NO_INCLUDE_GUARD_FOR_A_REASON
