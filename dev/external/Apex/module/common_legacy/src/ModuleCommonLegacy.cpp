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
#include "ConvexHullParameters_0p0.h"
#include "ConvexHullParameters_0p1.h"
#include "ConversionConvexHullParameters_0p0_0p1.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{

// AUTO_GENERATED_OBJECTS_BEGIN
static ConvexHullParameters_0p0Factory factory_ConvexHullParameters_0p0;
static ConvexHullParameters_0p1Factory factory_ConvexHullParameters_0p1;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleCommonLegacyObjects[] = {
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_ConvexHullParameters_0p0,
		ConvexHullParameters_0p0::freeParameterDefinitionTable,
		ConversionConvexHullParameters_0p0_0p1::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleCommonLegacy : public ApexLegacyModule
{
public:
	ModuleCommonLegacy( NiApexSDK* sdk );

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleCommonLegacy)

ModuleCommonLegacy::ModuleCommonLegacy( NiApexSDK* inSdk )
{
	name = "Common_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits *t = mSdk->getParameterizedTraits();
	if( !t )
		return;

	// Register auto-generated objects
	registerLegacyObjects(ModuleCommonLegacyObjects);

	// Register custom conversions here
}

void ModuleCommonLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits *t = mSdk->getParameterizedTraits();
	if( !t )
		return;

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleCommonLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
