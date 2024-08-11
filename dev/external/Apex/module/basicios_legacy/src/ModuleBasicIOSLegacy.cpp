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
#include "BasicIOSAssetParam_0p0.h"
#include "BasicIOSAssetParam_0p1.h"
#include "BasicIOSAssetParam_0p2.h"
#include "ConversionBasicIOSAssetParam_0p1_0p2.h"
#include "BasicIOSAssetParam_0p3.h"
#include "ConversionBasicIOSAssetParam_0p2_0p3.h"
#include "ConversionBasicIOSAssetParam_0p0_0p1.h"
#include "BasicIOSAssetParam_0p4.h"
#include "ConversionBasicIOSAssetParam_0p3_0p4.h"
#include "BasicIOSAssetParam_0p5.h"
#include "ConversionBasicIOSAssetParam_0p4_0p5.h"
#include "BasicIOSAssetParam_0p6.h"
#include "ConversionBasicIOSAssetParam_0p5_0p6.h"
#include "BasicIOSAssetParam_0p7.h"
#include "ConversionBasicIOSAssetParam_0p6_0p7.h"
#include "BasicIOSAssetParam_0p8.h"
#include "ConversionBasicIOSAssetParam_0p7_0p8.h"
#include "BasicIOSAssetParam_0p9.h"
#include "ConversionBasicIOSAssetParam_0p8_0p9.h"
#include "BasicIOSAssetParam_1p0.h"
#include "ConversionBasicIOSAssetParam_0p9_1p0.h"
#include "BasicIOSAssetParam_1p1.h"
#include "ConversionBasicIOSAssetParam_1p0_1p1.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static BasicIOSAssetParam_0p0Factory factory_BasicIOSAssetParam_0p0;
static BasicIOSAssetParam_0p1Factory factory_BasicIOSAssetParam_0p1;
static BasicIOSAssetParam_0p2Factory factory_BasicIOSAssetParam_0p2;
static BasicIOSAssetParam_0p3Factory factory_BasicIOSAssetParam_0p3;
static BasicIOSAssetParam_0p4Factory factory_BasicIOSAssetParam_0p4;
static BasicIOSAssetParam_0p5Factory factory_BasicIOSAssetParam_0p5;
static BasicIOSAssetParam_0p6Factory factory_BasicIOSAssetParam_0p6;
static BasicIOSAssetParam_0p7Factory factory_BasicIOSAssetParam_0p7;
static BasicIOSAssetParam_0p8Factory factory_BasicIOSAssetParam_0p8;
static BasicIOSAssetParam_0p9Factory factory_BasicIOSAssetParam_0p9;
static BasicIOSAssetParam_1p0Factory factory_BasicIOSAssetParam_1p0;
static BasicIOSAssetParam_1p1Factory factory_BasicIOSAssetParam_1p1;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleBasicIOSLegacyObjects[] =
{
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_BasicIOSAssetParam_0p0,
		BasicIOSAssetParam_0p0::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p0_0p1::Create,
		0
	},
	{
		1,
		2,
		&factory_BasicIOSAssetParam_0p1,
		BasicIOSAssetParam_0p1::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p1_0p2::Create,
		0
	},
	{
		2,
		3,
		&factory_BasicIOSAssetParam_0p2,
		BasicIOSAssetParam_0p2::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p2_0p3::Create,
		0
	},
	{
		3,
		4,
		&factory_BasicIOSAssetParam_0p3,
		BasicIOSAssetParam_0p3::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p3_0p4::Create,
		0
	},
	{
		4,
		5,
		&factory_BasicIOSAssetParam_0p4,
		BasicIOSAssetParam_0p4::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p4_0p5::Create,
		0
	},
	{
		5,
		6,
		&factory_BasicIOSAssetParam_0p5,
		BasicIOSAssetParam_0p5::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p5_0p6::Create,
		0
	},
	{
		6,
		7,
		&factory_BasicIOSAssetParam_0p6,
		BasicIOSAssetParam_0p6::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p6_0p7::Create,
		0
	},
	{
		7,
		8,
		&factory_BasicIOSAssetParam_0p7,
		BasicIOSAssetParam_0p7::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p7_0p8::Create,
		0
	},
	{
		8,
		9,
		&factory_BasicIOSAssetParam_0p8,
		BasicIOSAssetParam_0p8::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p8_0p9::Create,
		0
	},
	{
		9,
		65536,
		&factory_BasicIOSAssetParam_0p9,
		BasicIOSAssetParam_0p9::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_0p9_1p0::Create,
		0
	},
	{
		65536,
		65537,
		&factory_BasicIOSAssetParam_1p0,
		BasicIOSAssetParam_1p0::freeParameterDefinitionTable,
		ConversionBasicIOSAssetParam_1p0_1p1::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleBasicIOSLegacy : public ApexLegacyModule
{
public:
	ModuleBasicIOSLegacy(NiApexSDK* sdk);

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleBasicIOSLegacy)

ModuleBasicIOSLegacy::ModuleBasicIOSLegacy(NiApexSDK* inSdk)
{
	name = "BasicIOS_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Register auto-generated objects
	registerLegacyObjects(ModuleBasicIOSLegacyObjects);

	// Register custom conversions here
}

void ModuleBasicIOSLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleBasicIOSLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
