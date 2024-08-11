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

#include "PsShare.h"
#include "NxApex.h"
#include "ApexLegacyModule.h"

// AUTO_GENERATED_INCLUDES_BEGIN
#include "VertexBufferParameters_0p0.h"
#include "VertexBufferParameters_0p1.h"
#include "ConversionVertexBufferParameters_0p0_0p1.h"
#include "BufferF32x4_0p0.h"
#include "BufferF32x4_0p1.h"
#include "ConversionBufferF32x4_0p0_0p1.h"
#include "SubmeshParameters_0p0.h"
#include "SubmeshParameters_0p1.h"
#include "ConversionSubmeshParameters_0p0_0p1.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static VertexBufferParameters_0p0Factory factory_VertexBufferParameters_0p0;
static VertexBufferParameters_0p1Factory factory_VertexBufferParameters_0p1;
static BufferF32x4_0p0Factory factory_BufferF32x4_0p0;
static BufferF32x4_0p1Factory factory_BufferF32x4_0p1;
static SubmeshParameters_0p0Factory factory_SubmeshParameters_0p0;
static SubmeshParameters_0p1Factory factory_SubmeshParameters_0p1;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleFrameworkLegacyObjects[] =
{
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_VertexBufferParameters_0p0,
		VertexBufferParameters_0p0::freeParameterDefinitionTable,
		ConversionVertexBufferParameters_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_BufferF32x4_0p0,
		BufferF32x4_0p0::freeParameterDefinitionTable,
		ConversionBufferF32x4_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_SubmeshParameters_0p0,
		SubmeshParameters_0p0::freeParameterDefinitionTable,
		ConversionSubmeshParameters_0p0_0p1::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleFrameworkLegacy : public ApexLegacyModule
{
public:
	ModuleFrameworkLegacy(NiApexSDK* sdk);

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleFrameworkLegacy)

ModuleFrameworkLegacy::ModuleFrameworkLegacy(NiApexSDK* inSdk)
{
	name = "Framework_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Register auto-generated objects
	registerLegacyObjects(ModuleFrameworkLegacyObjects);

	// Register custom conversions here
}

void ModuleFrameworkLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleFrameworkLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
