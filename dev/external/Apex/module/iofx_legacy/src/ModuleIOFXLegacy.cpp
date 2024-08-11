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
#include "IofxAssetParameters_0p0.h"
#include "IofxAssetParameters_0p1.h"
#include "ConversionIofxAssetParameters_0p0_0p1.h"
#include "SpriteIofxParameters_0p0.h"
#include "SpriteIofxParameters_0p1.h"
#include "ConversionSpriteIofxParameters_0p0_0p1.h"
#include "MeshIofxParameters_0p0.h"
#include "MeshIofxParameters_0p1.h"
#include "ConversionMeshIofxParameters_0p0_0p1.h"
#include "RotationModifierParams_0p0.h"
#include "RotationModifierParams_0p1.h"
#include "ConversionRotationModifierParams_0p0_0p1.h"
#include "SpriteIofxParameters_0p2.h"
#include "ConversionSpriteIofxParameters_0p1_0p2.h"
#include "MeshIofxParameters_0p2.h"
#include "ConversionMeshIofxParameters_0p1_0p2.h"
#include "IofxAssetParameters_0p2.h"
#include "ConversionIofxAssetParameters_0p1_0p2.h"
#include "SpriteIofxParameters_0p3.h"
#include "ConversionSpriteIofxParameters_0p2_0p3.h"
#include "MeshIofxParameters_0p3.h"
#include "ConversionMeshIofxParameters_0p2_0p3.h"
#include "RotationModifierParams_0p2.h"
#include "ConversionRotationModifierParams_0p1_0p2.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static IofxAssetParameters_0p0Factory factory_IofxAssetParameters_0p0;
static IofxAssetParameters_0p1Factory factory_IofxAssetParameters_0p1;
static SpriteIofxParameters_0p0Factory factory_SpriteIofxParameters_0p0;
static SpriteIofxParameters_0p1Factory factory_SpriteIofxParameters_0p1;
static MeshIofxParameters_0p0Factory factory_MeshIofxParameters_0p0;
static MeshIofxParameters_0p1Factory factory_MeshIofxParameters_0p1;
static RotationModifierParams_0p0Factory factory_RotationModifierParams_0p0;
static RotationModifierParams_0p1Factory factory_RotationModifierParams_0p1;
static SpriteIofxParameters_0p2Factory factory_SpriteIofxParameters_0p2;
static MeshIofxParameters_0p2Factory factory_MeshIofxParameters_0p2;
static IofxAssetParameters_0p2Factory factory_IofxAssetParameters_0p2;
static SpriteIofxParameters_0p3Factory factory_SpriteIofxParameters_0p3;
static MeshIofxParameters_0p3Factory factory_MeshIofxParameters_0p3;
static RotationModifierParams_0p2Factory factory_RotationModifierParams_0p2;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleIOFXLegacyObjects[] =
{
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_IofxAssetParameters_0p0,
		IofxAssetParameters_0p0::freeParameterDefinitionTable,
		ConversionIofxAssetParameters_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_SpriteIofxParameters_0p0,
		SpriteIofxParameters_0p0::freeParameterDefinitionTable,
		ConversionSpriteIofxParameters_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_MeshIofxParameters_0p0,
		MeshIofxParameters_0p0::freeParameterDefinitionTable,
		ConversionMeshIofxParameters_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_RotationModifierParams_0p0,
		RotationModifierParams_0p0::freeParameterDefinitionTable,
		ConversionRotationModifierParams_0p0_0p1::Create,
		0
	},
	{
		1,
		2,
		&factory_SpriteIofxParameters_0p1,
		SpriteIofxParameters_0p1::freeParameterDefinitionTable,
		ConversionSpriteIofxParameters_0p1_0p2::Create,
		0
	},
	{
		1,
		2,
		&factory_MeshIofxParameters_0p1,
		MeshIofxParameters_0p1::freeParameterDefinitionTable,
		ConversionMeshIofxParameters_0p1_0p2::Create,
		0
	},
	{
		1,
		2,
		&factory_IofxAssetParameters_0p1,
		IofxAssetParameters_0p1::freeParameterDefinitionTable,
		ConversionIofxAssetParameters_0p1_0p2::Create,
		0
	},
	{
		2,
		3,
		&factory_SpriteIofxParameters_0p2,
		SpriteIofxParameters_0p2::freeParameterDefinitionTable,
		ConversionSpriteIofxParameters_0p2_0p3::Create,
		0
	},
	{
		2,
		3,
		&factory_MeshIofxParameters_0p2,
		MeshIofxParameters_0p2::freeParameterDefinitionTable,
		ConversionMeshIofxParameters_0p2_0p3::Create,
		0
	},
	{
		1,
		2,
		&factory_RotationModifierParams_0p1,
		RotationModifierParams_0p1::freeParameterDefinitionTable,
		ConversionRotationModifierParams_0p1_0p2::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleIOFXLegacy : public ApexLegacyModule
{
public:
	ModuleIOFXLegacy(NiApexSDK* sdk);

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleIOFXLegacy)

ModuleIOFXLegacy::ModuleIOFXLegacy(NiApexSDK* inSdk)
{
	name = "IOFX_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Register auto-generated objects
	registerLegacyObjects(ModuleIOFXLegacyObjects);

	// Register custom conversions here
}

void ModuleIOFXLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleIOFXLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
