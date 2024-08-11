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
#include "ShapeCapsuleParams_0p0.h"
#include "ShapeCapsuleParams_0p1.h"
#include "ConversionShapeCapsuleParams_0p0_0p1.h"
#include "ShapeBoxParams_0p0.h"
#include "ShapeBoxParams_0p1.h"
#include "ConversionShapeBoxParams_0p0_0p1.h"
#include "ShapeSphereParams_0p0.h"
#include "ShapeSphereParams_0p1.h"
#include "ConversionShapeSphereParams_0p0_0p1.h"
#include "ShapeConvexParams_0p0.h"
#include "ShapeConvexParams_0p1.h"
#include "ConversionShapeConvexParams_0p0_0p1.h"
// AUTO_GENERATED_INCLUDES_END

namespace physx
{
namespace apex
{
namespace legacy
{
// AUTO_GENERATED_OBJECTS_BEGIN
static ShapeCapsuleParams_0p0Factory factory_ShapeCapsuleParams_0p0;
static ShapeCapsuleParams_0p1Factory factory_ShapeCapsuleParams_0p1;
static ShapeBoxParams_0p0Factory factory_ShapeBoxParams_0p0;
static ShapeBoxParams_0p1Factory factory_ShapeBoxParams_0p1;
static ShapeSphereParams_0p0Factory factory_ShapeSphereParams_0p0;
static ShapeSphereParams_0p1Factory factory_ShapeSphereParams_0p1;
static ShapeConvexParams_0p0Factory factory_ShapeConvexParams_0p0;
static ShapeConvexParams_0p1Factory factory_ShapeConvexParams_0p1;
// AUTO_GENERATED_OBJECTS_END

static LegacyClassEntry ModuleFieldBoundaryLegacyObjects[] =
{
	// AUTO_GENERATED_TABLE_BEGIN
	{
		0,
		1,
		&factory_ShapeCapsuleParams_0p0,
		ShapeCapsuleParams_0p0::freeParameterDefinitionTable,
		ConversionShapeCapsuleParams_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_ShapeBoxParams_0p0,
		ShapeBoxParams_0p0::freeParameterDefinitionTable,
		ConversionShapeBoxParams_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_ShapeSphereParams_0p0,
		ShapeSphereParams_0p0::freeParameterDefinitionTable,
		ConversionShapeSphereParams_0p0_0p1::Create,
		0
	},
	{
		0,
		1,
		&factory_ShapeConvexParams_0p0,
		ShapeConvexParams_0p0::freeParameterDefinitionTable,
		ConversionShapeConvexParams_0p0_0p1::Create,
		0
	},
	// AUTO_GENERATED_TABLE_END

	{ 0, 0, 0, 0, 0, 0} // Terminator
};

class ModuleFieldBoundaryLegacy : public ApexLegacyModule
{
public:
	ModuleFieldBoundaryLegacy(NiApexSDK* sdk);

protected:
	void releaseLegacyObjects();

private:

	// Add custom conversions here

};

DEFINE_INSTANTIATE_MODULE(ModuleFieldBoundaryLegacy)

ModuleFieldBoundaryLegacy::ModuleFieldBoundaryLegacy(NiApexSDK* inSdk)
{
	name = "FieldBoundary_Legacy";
	mSdk = inSdk;
	mApiProxy = this;

	// Register legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Register auto-generated objects
	registerLegacyObjects(ModuleFieldBoundaryLegacyObjects);

	// Register custom conversions here
}

void ModuleFieldBoundaryLegacy::releaseLegacyObjects()
{
	//Release legacy stuff

	NxParameterized::Traits* t = mSdk->getParameterizedTraits();
	if (!t)
	{
		return;
	}

	// Unregister auto-generated objects
	unregisterLegacyObjects(ModuleFieldBoundaryLegacyObjects);

	// Unregister custom conversions here
}

}
}
} // end namespace physx::apex
