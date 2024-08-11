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
#include "DestructibleAssetParameters_0p0.h"
#include "DestructibleAssetParameters_0p1.h"
#include "ConversionDestructibleAssetParameters_0p0_0p1.h"
#include "DestructibleActorParam_0p0.h"
#include "DestructibleActorParam_0p1.h"
#include "ConversionDestructibleActorParam_0p0_0p1.h"
#include "DestructibleAssetParameters_0p2.h"
#include "ConversionDestructibleAssetParameters_0p1_0p2.h"
#include "DestructibleActorParam_0p2.h"
#include "DestructibleActorParam_0p3.h"
#include "ConversionDestructibleActorParam_0p2_0p3.h"
#include "DestructibleAssetParameters_0p3.h"
#include "ConversionDestructibleAssetParameters_0p2_0p3.h"
#include "DestructibleAssetParameters_0p4.h"
#include "ConversionDestructibleAssetParameters_0p3_0p4.h"
#include "DestructibleAssetParameters_0p5.h"
#include "ConversionDestructibleAssetParameters_0p4_0p5.h"
#include "DestructibleModuleParameters_0p0.h"
#include "DestructibleModuleParameters_0p1.h"
#include "ConversionDestructibleModuleParameters_0p0_0p1.h"
#include "DestructibleAssetParameters_0p6.h"
#include "ConversionDestructibleAssetParameters_0p5_0p6.h"
#include "DestructibleActorParam_0p4.h"
#include "ConversionDestructibleActorParam_0p3_0p4.h"
#include "SurfaceTraceSetParameters_0p0.h"
#include "SurfaceTraceSetParameters_0p1.h"
#include "ConversionSurfaceTraceSetParameters_0p0_0p1.h"
#include "DestructibleAssetParameters_0p7.h"
#include "ConversionDestructibleAssetParameters_0p6_0p7.h"
#include "DestructibleActorParam_0p5.h"
#include "ConversionDestructibleActorParam_0p4_0p5.h"
#include "DestructibleAssetParameters_0p8.h"
#include "ConversionDestructibleAssetParameters_0p7_0p8.h"
#include "DestructibleAssetParameters_0p9.h"
#include "ConversionDestructibleAssetParameters_0p8_0p9.h"
#include "DestructibleActorParam_0p6.h"
#include "ConversionDestructibleActorParam_0p5_0p6.h"
#include "DestructibleActorParam_0p7.h"
#include "ConversionDestructibleActorParam_0p6_0p7.h"
#include "DestructibleActorParam_0p8.h"
#include "ConversionDestructibleActorParam_0p7_0p8.h"
#include "DestructibleActorParam_0p9.h"
#include "ConversionDestructibleActorParam_0p8_0p9.h"
#include "DestructibleAssetParameters_0p10.h"
#include "ConversionDestructibleAssetParameters_0p9_0p10.h"
#include "DestructibleActorParam_0p10.h"
#include "ConversionDestructibleActorParam_0p9_0p10.h"
#include "DestructibleAssetParameters_0p11.h"
#include "ConversionDestructibleAssetParameters_0p10_0p11.h"
#include "DestructibleActorParam_0p11.h"
#include "ConversionDestructibleActorParam_0p10_0p11.h"
#include "DestructibleActorParam_0p12.h"
#include "ConversionDestructibleActorParam_0p11_0p12.h"
#include "DestructibleAssetParameters_0p12.h"
#include "ConversionDestructibleAssetParameters_0p11_0p12.h"
#include "DestructibleActorParam_0p13.h"
#include "ConversionDestructibleActorParam_0p12_0p13.h"
#include "DestructibleActorParam_0p14.h"
#include "ConversionDestructibleActorParam_0p13_0p14.h"
#include "DestructibleActorState_0p0.h"
#include "DestructibleActorState_0p1.h"
#include "ConversionDestructibleActorState_0p0_0p1.h"
#include "DestructibleAssetParameters_0p13.h"
#include "ConversionDestructibleAssetParameters_0p12_0p13.h"
#include "DestructibleAssetParameters_0p14.h"
#include "ConversionDestructibleAssetParameters_0p13_0p14.h"
#include "DestructibleActorParam_0p15.h"
#include "ConversionDestructibleActorParam_0p14_0p15.h"
#include "DestructibleActorState_0p2.h"
#include "ConversionDestructibleActorState_0p1_0p2.h"
#include "DestructibleActorParam_0p16.h"
#include "ConversionDestructibleActorParam_0p15_0p16.h"
#include "DestructibleAssetParameters_0p15.h"
#include "ConversionDestructibleAssetParameters_0p14_0p15.h"
#include "DestructibleActorParam_0p17.h"
#include "ConversionDestructibleActorParam_0p16_0p17.h"
#include "DestructibleAssetParameters_0p16.h"
#include "ConversionDestructibleAssetParameters_0p15_0p16.h"
#include "DestructibleActorParam_0p18.h"
#include "ConversionDestructibleActorParam_0p17_0p18.h"
#include "DestructibleAssetParameters_0p17.h"
#include "ConversionDestructibleAssetParameters_0p16_0p17.h"
#include "DestructibleActorParam_0p19.h"
#include "ConversionDestructibleActorParam_0p18_0p19.h"
#include "DestructibleModuleParameters_0p2.h"
#include "ConversionDestructibleModuleParameters_0p1_0p2.h"
#include "DestructibleActorParam_0p20.h"
#include "ConversionDestructibleActorParam_0p19_0p20.h"
#include "DestructibleAssetParameters_0p18.h"
#include "ConversionDestructibleAssetParameters_0p17_0p18.h"
#include "DestructibleAssetParameters_0p19.h"
#include "ConversionDestructibleAssetParameters_0p18_0p19.h"
#include "DestructibleActorParam_0p21.h"
#include "ConversionDestructibleActorParam_0p20_0p21.h"
#include "DestructibleAssetParameters_0p20.h"
#include "ConversionDestructibleAssetParameters_0p19_0p20.h"
#include "DestructibleActorParam_0p22.h"
#include "ConversionDestructibleActorParam_0p21_0p22.h"
#include "DestructibleAssetParameters_0p21.h"
#include "ConversionDestructibleAssetParameters_0p20_0p21.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static DestructibleAssetParameters_0p0Factory factory_DestructibleAssetParameters_0p0;
static DestructibleAssetParameters_0p1Factory factory_DestructibleAssetParameters_0p1;
static DestructibleActorParam_0p0Factory factory_DestructibleActorParam_0p0;
static DestructibleActorParam_0p1Factory factory_DestructibleActorParam_0p1;
static DestructibleAssetParameters_0p2Factory factory_DestructibleAssetParameters_0p2;
static DestructibleActorParam_0p2Factory factory_DestructibleActorParam_0p2;
static DestructibleActorParam_0p3Factory factory_DestructibleActorParam_0p3;
static DestructibleAssetParameters_0p3Factory factory_DestructibleAssetParameters_0p3;
static DestructibleAssetParameters_0p4Factory factory_DestructibleAssetParameters_0p4;
static DestructibleAssetParameters_0p5Factory factory_DestructibleAssetParameters_0p5;
static DestructibleModuleParameters_0p0Factory factory_DestructibleModuleParameters_0p0;
static DestructibleModuleParameters_0p1Factory factory_DestructibleModuleParameters_0p1;
static DestructibleAssetParameters_0p6Factory factory_DestructibleAssetParameters_0p6;
static DestructibleActorParam_0p4Factory factory_DestructibleActorParam_0p4;
static SurfaceTraceSetParameters_0p0Factory factory_SurfaceTraceSetParameters_0p0;
static SurfaceTraceSetParameters_0p1Factory factory_SurfaceTraceSetParameters_0p1;
static DestructibleAssetParameters_0p7Factory factory_DestructibleAssetParameters_0p7;
static DestructibleActorParam_0p5Factory factory_DestructibleActorParam_0p5;
static DestructibleAssetParameters_0p8Factory factory_DestructibleAssetParameters_0p8;
static DestructibleAssetParameters_0p9Factory factory_DestructibleAssetParameters_0p9;
static DestructibleActorParam_0p6Factory factory_DestructibleActorParam_0p6;
static DestructibleActorParam_0p7Factory factory_DestructibleActorParam_0p7;
static DestructibleActorParam_0p8Factory factory_DestructibleActorParam_0p8;
static DestructibleActorParam_0p9Factory factory_DestructibleActorParam_0p9;
static DestructibleAssetParameters_0p10Factory factory_DestructibleAssetParameters_0p10;
static DestructibleActorParam_0p10Factory factory_DestructibleActorParam_0p10;
static DestructibleAssetParameters_0p11Factory factory_DestructibleAssetParameters_0p11;
static DestructibleActorParam_0p11Factory factory_DestructibleActorParam_0p11;
static DestructibleActorParam_0p12Factory factory_DestructibleActorParam_0p12;
static DestructibleAssetParameters_0p12Factory factory_DestructibleAssetParameters_0p12;
static DestructibleActorParam_0p13Factory factory_DestructibleActorParam_0p13;
static DestructibleActorParam_0p14Factory factory_DestructibleActorParam_0p14;
static DestructibleActorState_0p0Factory factory_DestructibleActorState_0p0;
static DestructibleActorState_0p1Factory factory_DestructibleActorState_0p1;
static DestructibleAssetParameters_0p13Factory factory_DestructibleAssetParameters_0p13;
static DestructibleAssetParameters_0p14Factory factory_DestructibleAssetParameters_0p14;
static DestructibleActorParam_0p15Factory factory_DestructibleActorParam_0p15;
static DestructibleActorState_0p2Factory factory_DestructibleActorState_0p2;
static DestructibleActorParam_0p16Factory factory_DestructibleActorParam_0p16;
static DestructibleAssetParameters_0p15Factory factory_DestructibleAssetParameters_0p15;
static DestructibleActorParam_0p17Factory factory_DestructibleActorParam_0p17;
static DestructibleAssetParameters_0p16Factory factory_DestructibleAssetParameters_0p16;
static DestructibleActorParam_0p18Factory factory_DestructibleActorParam_0p18;
static DestructibleAssetParameters_0p17Factory factory_DestructibleAssetParameters_0p17;
static DestructibleActorParam_0p19Factory factory_DestructibleActorParam_0p19;
static DestructibleModuleParameters_0p2Factory factory_DestructibleModuleParameters_0p2;
static DestructibleActorParam_0p20Factory factory_DestructibleActorParam_0p20;
static DestructibleAssetParameters_0p18Factory factory_DestructibleAssetParameters_0p18;
static DestructibleAssetParameters_0p19Factory factory_DestructibleAssetParameters_0p19;
static DestructibleActorParam_0p21Factory factory_DestructibleActorParam_0p21;
static DestructibleAssetParameters_0p20Factory factory_DestructibleAssetParameters_0p20;
static DestructibleActorParam_0p22Factory factory_DestructibleActorParam_0p22;
static DestructibleAssetParameters_0p21Factory factory_DestructibleAssetParameters_0p21;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleDestructibleLegacyObjects[] =
{
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_DestructibleAssetParameters_0p0,
		DestructibleAssetParameters_0p0::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_DestructibleActorParam_0p0,
		DestructibleActorParam_0p0::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p0_0p1::Create,
		0
	},
	{
		1,
		2,
		&factory_DestructibleAssetParameters_0p1,
		DestructibleAssetParameters_0p1::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p1_0p2::Create,
		0
	},
	{
		2,
		3,
		&factory_DestructibleActorParam_0p2,
		DestructibleActorParam_0p2::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p2_0p3::Create,
		0
	},
	{
		2,
		3,
		&factory_DestructibleAssetParameters_0p2,
		DestructibleAssetParameters_0p2::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p2_0p3::Create,
		0
	},
	{
		3,
		4,
		&factory_DestructibleAssetParameters_0p3,
		DestructibleAssetParameters_0p3::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p3_0p4::Create,
		0
	},
	{
		4,
		5,
		&factory_DestructibleAssetParameters_0p4,
		DestructibleAssetParameters_0p4::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p4_0p5::Create,
		0
	},
	{
		0,
		1,
		&factory_DestructibleModuleParameters_0p0,
		DestructibleModuleParameters_0p0::freeParameterDefinitionTable,
		ConversionDestructibleModuleParameters_0p0_0p1::Create,
		0
	},
	{
		5,
		6,
		&factory_DestructibleAssetParameters_0p5,
		DestructibleAssetParameters_0p5::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p5_0p6::Create,
		0
	},
	{
		3,
		4,
		&factory_DestructibleActorParam_0p3,
		DestructibleActorParam_0p3::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p3_0p4::Create,
		0
	},
	{
		0,
		1,
		&factory_SurfaceTraceSetParameters_0p0,
		SurfaceTraceSetParameters_0p0::freeParameterDefinitionTable,
		ConversionSurfaceTraceSetParameters_0p0_0p1::Create,
		0
	},
	{
		6,
		7,
		&factory_DestructibleAssetParameters_0p6,
		DestructibleAssetParameters_0p6::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p6_0p7::Create,
		0
	},
	{
		4,
		5,
		&factory_DestructibleActorParam_0p4,
		DestructibleActorParam_0p4::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p4_0p5::Create,
		0
	},
	{
		7,
		8,
		&factory_DestructibleAssetParameters_0p7,
		DestructibleAssetParameters_0p7::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p7_0p8::Create,
		0
	},
	{
		8,
		9,
		&factory_DestructibleAssetParameters_0p8,
		DestructibleAssetParameters_0p8::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p8_0p9::Create,
		0
	},
	{
		5,
		6,
		&factory_DestructibleActorParam_0p5,
		DestructibleActorParam_0p5::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p5_0p6::Create,
		0
	},
	{
		6,
		7,
		&factory_DestructibleActorParam_0p6,
		DestructibleActorParam_0p6::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p6_0p7::Create,
		0
	},
	{
		7,
		8,
		&factory_DestructibleActorParam_0p7,
		DestructibleActorParam_0p7::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p7_0p8::Create,
		0
	},
	{
		8,
		9,
		&factory_DestructibleActorParam_0p8,
		DestructibleActorParam_0p8::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p8_0p9::Create,
		0
	},
	{
		9,
		10,
		&factory_DestructibleAssetParameters_0p9,
		DestructibleAssetParameters_0p9::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p9_0p10::Create,
		0
	},
	{
		9,
		10,
		&factory_DestructibleActorParam_0p9,
		DestructibleActorParam_0p9::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p9_0p10::Create,
		0
	},
	{
		10,
		11,
		&factory_DestructibleAssetParameters_0p10,
		DestructibleAssetParameters_0p10::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p10_0p11::Create,
		0
	},
	{
		10,
		11,
		&factory_DestructibleActorParam_0p10,
		DestructibleActorParam_0p10::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p10_0p11::Create,
		0
	},
	{
		11,
		12,
		&factory_DestructibleActorParam_0p11,
		DestructibleActorParam_0p11::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p11_0p12::Create,
		0
	},
	{
		11,
		12,
		&factory_DestructibleAssetParameters_0p11,
		DestructibleAssetParameters_0p11::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p11_0p12::Create,
		0
	},
	{
		12,
		13,
		&factory_DestructibleActorParam_0p12,
		DestructibleActorParam_0p12::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p12_0p13::Create,
		0
	},
	{
		13,
		14,
		&factory_DestructibleActorParam_0p13,
		DestructibleActorParam_0p13::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p13_0p14::Create,
		0
	},
	{
		0,
		1,
		&factory_DestructibleActorState_0p0,
		DestructibleActorState_0p0::freeParameterDefinitionTable,
		ConversionDestructibleActorState_0p0_0p1::Create,
		0
	},
	{
		12,
		13,
		&factory_DestructibleAssetParameters_0p12,
		DestructibleAssetParameters_0p12::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p12_0p13::Create,
		0
	},
	{
		13,
		14,
		&factory_DestructibleAssetParameters_0p13,
		DestructibleAssetParameters_0p13::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p13_0p14::Create,
		0
	},
	{
		14,
		15,
		&factory_DestructibleActorParam_0p14,
		DestructibleActorParam_0p14::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p14_0p15::Create,
		0
	},
	{
		1,
		2,
		&factory_DestructibleActorState_0p1,
		DestructibleActorState_0p1::freeParameterDefinitionTable,
		ConversionDestructibleActorState_0p1_0p2::Create,
		0
	},
	{
		15,
		16,
		&factory_DestructibleActorParam_0p15,
		DestructibleActorParam_0p15::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p15_0p16::Create,
		0
	},
	{
		14,
		15,
		&factory_DestructibleAssetParameters_0p14,
		DestructibleAssetParameters_0p14::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p14_0p15::Create,
		0
	},
	{
		16,
		17,
		&factory_DestructibleActorParam_0p16,
		DestructibleActorParam_0p16::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p16_0p17::Create,
		0
	},
	{
		15,
		16,
		&factory_DestructibleAssetParameters_0p15,
		DestructibleAssetParameters_0p15::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p15_0p16::Create,
		0
	},
	{
		17,
		18,
		&factory_DestructibleActorParam_0p17,
		DestructibleActorParam_0p17::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p17_0p18::Create,
		0
	},
	{
		16,
		17,
		&factory_DestructibleAssetParameters_0p16,
		DestructibleAssetParameters_0p16::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p16_0p17::Create,
		0
	},
	{
		18,
		19,
		&factory_DestructibleActorParam_0p18,
		DestructibleActorParam_0p18::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p18_0p19::Create,
		0
	},
	{
		1,
		2,
		&factory_DestructibleModuleParameters_0p1,
		DestructibleModuleParameters_0p1::freeParameterDefinitionTable,
		ConversionDestructibleModuleParameters_0p1_0p2::Create,
		0
	},
	{
		19,
		20,
		&factory_DestructibleActorParam_0p19,
		DestructibleActorParam_0p19::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p19_0p20::Create,
		0
	},
	{
		17,
		18,
		&factory_DestructibleAssetParameters_0p17,
		DestructibleAssetParameters_0p17::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p17_0p18::Create,
		0
	},
	{
		18,
		19,
		&factory_DestructibleAssetParameters_0p18,
		DestructibleAssetParameters_0p18::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p18_0p19::Create,
		0
	},
	{
		20,
		21,
		&factory_DestructibleActorParam_0p20,
		DestructibleActorParam_0p20::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p20_0p21::Create,
		0
	},
	{
		19,
		20,
		&factory_DestructibleAssetParameters_0p19,
		DestructibleAssetParameters_0p19::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p19_0p20::Create,
		0
	},
	{
		21,
		22,
		&factory_DestructibleActorParam_0p21,
		DestructibleActorParam_0p21::freeParameterDefinitionTable,
		ConversionDestructibleActorParam_0p21_0p22::Create,
		0
	},
	{
		20,
		21,
		&factory_DestructibleAssetParameters_0p20,
		DestructibleAssetParameters_0p20::freeParameterDefinitionTable,
		ConversionDestructibleAssetParameters_0p20_0p21::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleDestructibleLegacy : public ApexLegacyModule
{
public:
	ModuleDestructibleLegacy(NiApexSDK* sdk);

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleDestructibleLegacy)

ModuleDestructibleLegacy::ModuleDestructibleLegacy(NiApexSDK* inSdk)
{
	name = "Destructible_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Register auto-generated objects
	registerLegacyObjects(ModuleDestructibleLegacyObjects);

	// Register custom conversions here
}

void ModuleDestructibleLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleDestructibleLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
