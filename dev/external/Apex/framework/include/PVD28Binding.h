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

#ifndef PVD28_BINDING_H
#define PVD28_BINDING_H

#include "PsShare.h"
#ifndef WITHOUT_PVD

#include "NxApexDefs.h"
#if NX_SDK_VERSION_MAJOR == 2

#include "foundation/Px.h"

#include "PvdBinding.h"
class NxRemoteDebugger;

namespace PVD {

	class Pvd28Binding : public PvdBinding
	{
	protected:
		virtual ~Pvd28Binding(){}
	public:
		//overloads the connect method.

		//Perhaps setup a foundation instance, and attach yourself to this debugger.
		static Pvd28Binding& attach( NxRemoteDebugger& inDebugger, bool inSetupFoundation = true );
	};

}
#endif // NX_SDK_VERSION_MAJOR == 2
#endif // WITHOUT_PVD
#endif // PVD28_BINDING_H
