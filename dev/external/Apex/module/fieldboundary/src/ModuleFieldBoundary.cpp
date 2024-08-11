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

#include "NxApexDefs.h"
#include "MinPhysxSdkVersion.h"

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
#include "NxApex.h"
#include "FieldBoundaryAsset.h"
#include "FieldBoundaryScene.h"
#include "NiApexScene.h"
#include "PxMemoryBuffer.h"
#include "NiModuleFieldSampler.h"
#include "ModuleFieldBoundary.h"
#include "ModulePerfScope.h"
using namespace fieldboundary;
#endif

#include "NiApexSDK.h"
#include "PsShare.h"

namespace physx
{
namespace apex
{


#if defined(_USRDLL)

/* Modules don't have to link against the framework, they keep their own */
NiApexSDK* gApexSdk = 0;
NxApexSDK* NxGetApexSDK()
{
	return gApexSdk;
}
NiApexSDK* NiGetApexSDK()
{
	return gApexSdk;
}

NXAPEX_API NxModule*  NX_CALL_CONV createModule(
    NiApexSDK* inSdk,
    NiModule** niRef,
    physx::PxU32 APEXsdkVersion,
    physx::PxU32 PhysXsdkVersion,
    NxApexCreateError* errorCode)
{
	if (APEXsdkVersion != NX_APEX_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

	if (PhysXsdkVersion != NX_PHYSICS_SDK_VERSION)
	{
		if (errorCode)
		{
			*errorCode = APEX_CE_WRONG_VERSION;
		}
		return NULL;
	}

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "FieldBoundary");
	ModuleFieldBoundary* impl = PX_NEW(ModuleFieldBoundary)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL;
#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
}

#else
/* Statically linking entry function */
void instantiateModuleFieldBoundary()
{
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "FieldBoundary");
	fieldboundary::ModuleFieldBoundary* impl = PX_NEW(fieldboundary::ModuleFieldBoundary)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif // `defined(_USRDLL)
namespace fieldboundary
{
/* === ModuleFieldBoundary Implementation === */
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED && NX_SDK_VERSION_MAJOR == 2

NxAuthObjTypeID FieldBoundaryAsset::mAssetTypeID;  // static class member of FieldBoundaryAsset

#ifdef WITHOUT_APEX_AUTHORING

class FieldBoundaryAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	FieldBoundaryAssetDummyAuthoring(ModuleFieldBoundary* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	FieldBoundaryAssetDummyAuthoring(ModuleFieldBoundary* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	FieldBoundaryAssetDummyAuthoring(ModuleFieldBoundary* module, NxResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual void setToolString(const char* /*toolName*/, const char* /*toolVersion*/, PxU32 /*toolChangelist*/)
	{

	}


	virtual void release()
	{
		destroy();
	}

	// internal
	void destroy()
	{
		delete this;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return FieldBoundaryAsset::getClassName();
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	 */
	virtual bool prepareForPlatform(physx::apex::NxPlatformTag)
	{
		PX_ASSERT(0);
		return false;
	}

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Save asset's NxParameterized interface, may return NULL
	*/
	virtual NxParameterized::Interface* getNxParameterized() const
	{
		PX_ASSERT(0);
		return NULL;
	}

	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		PX_ALWAYS_ASSERT();
		return NULL;
	}



};

typedef ApexAuthorableObject<ModuleFieldBoundary, FieldBoundaryAsset, FieldBoundaryAssetDummyAuthoring> FieldBoundaryAO;

#else
typedef ApexAuthorableObject<ModuleFieldBoundary, FieldBoundaryAsset, FieldBoundaryAssetAuthoring> FieldBoundaryAO;
#endif

ModuleFieldBoundary::ModuleFieldBoundary(NiApexSDK* sdk)
{
	name = "FieldBoundary";
	mSdk = sdk;
	mApiProxy = this;
	mModuleParams = NULL;
	mFieldSamplerModule = NULL;

	/* Register asset type and create a namespace for its assets */
	const char* pName = FieldBoundaryAssetParameters::staticClassName();
	FieldBoundaryAO* eAO = PX_NEW(FieldBoundaryAO)(this, mAuthorableObjects, pName);
	FieldBoundaryAsset::mAssetTypeID = eAO->getResID();

	registerLODParameter("Radius", NxRange<physx::PxU32>(1, 10));

	// register the NxParameterized factory
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
#	define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#	include "FieldboundaryParamClasses.inc"
}

ModuleFieldBoundary::~ModuleFieldBoundary()
{
	releaseModuleProfiling();
}

void ModuleFieldBoundary::destroy()
{
	// release the NxParameterized factory
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	Module::destroy();
	delete this;

	if (traits)
	{
#		define PARAM_CLASS(clas) PARAM_CLASS_REMOVE_FACTORY(traits, clas)
#		include "FieldboundaryParamClasses.inc"
	}
}

PxU32 ModuleFieldBoundary::forceLoadAssets()
{
	PxU32 loadedAssetCount = 0;

	for (PxU32 i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}
	return loadedAssetCount;
}

NxParameterized::Interface* ModuleFieldBoundary::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(FieldBoundaryModuleParameters*)
		                (traits->createNxParameterized("FieldBoundaryModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleFieldBoundary::init(const NxModuleFieldBoundaryDesc& meDesc)
{
	PX_PROFILER_PERF_SCOPE("FieldBoundaryModuleInit");  // profile this function
	mModuleValue = meDesc.moduleValue;
}

NxAuthObjTypeID ModuleFieldBoundary::getFieldBoundaryAssetTypeID() const
{
	return FieldBoundaryAsset::mAssetTypeID;
}
NxAuthObjTypeID ModuleFieldBoundary::getModuleID() const
{
	return FieldBoundaryAsset::mAssetTypeID;
}

/* == FieldBoundary Scene methods == */
NiModuleScene* ModuleFieldBoundary::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* renderDebug)
{
	return PX_NEW(FieldBoundaryScene)(*this, scene, renderDebug, mFieldBoundaryScenes);
}

void ModuleFieldBoundary::releaseNiModuleScene(NiModuleScene& scene)
{
	FieldBoundaryScene* fs = DYNAMIC_CAST(FieldBoundaryScene*)(&scene);
	fs->destroy();
}

FieldBoundaryScene* ModuleFieldBoundary::getFieldBoundaryScene(const NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < mFieldBoundaryScenes.getSize() ; i++)
	{
		FieldBoundaryScene* fs = DYNAMIC_CAST(FieldBoundaryScene*)(mFieldBoundaryScenes.getResource(i));
		if (fs->mApexScene == &apexScene)
		{
			return fs;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate FieldBoundaryScene");
	return NULL;
}

NxApexRenderableIterator* ModuleFieldBoundary::createRenderableIterator(const NxApexScene& apexScene)
{
	FieldBoundaryScene* fs = getFieldBoundaryScene(apexScene);
	if (fs)
	{
		return fs->createRenderableIterator();
	}

	return NULL;
}

NiModuleFieldSampler* ModuleFieldBoundary::getNiModuleFieldSampler()
{
	NxApexCreateError err;
	if (!mFieldSamplerModule && mSdk->createModule("FieldSampler", &err))
	{
		NiModule* nim = mSdk->getNiModuleByName("FieldSampler");
		if (nim)
		{
			mFieldSamplerModule = DYNAMIC_CAST(NiModuleFieldSampler*)(nim);
		}
	}

	return mFieldSamplerModule;
}


#ifndef WITHOUT_APEX_AUTHORING
bool FieldBoundaryAssetAuthoring::addSphere(const NxApexSphereFieldShapeDesc& desc)
{
	FieldShapeDesc fdesc;
	fdesc.type = NX_APEX_SHAPE_SPHERE;
	fdesc.pose = desc.localPose;
	fdesc.radius = desc.radius;

	bool success = fdesc.isValid();
	if (success)
	{
		mShapes.pushBack(fdesc);
	}

	return success;
}

bool FieldBoundaryAssetAuthoring::addBox(const NxApexBoxFieldShapeDesc& desc)
{
	FieldShapeDesc fdesc;
	fdesc.type = NX_APEX_SHAPE_BOX;
	fdesc.pose = desc.localPose;
	fdesc.dimensions = desc.dimensions;

	bool success = fdesc.isValid();
	if (success)
	{
		mShapes.pushBack(fdesc);
	}

	return success;
}

bool FieldBoundaryAssetAuthoring::addCapsule(const NxApexCapsuleFieldShapeDesc& desc)
{
	FieldShapeDesc fdesc;
	fdesc.type = NX_APEX_SHAPE_CAPSULE;
	fdesc.pose = desc.localPose;
	fdesc.radius = desc.radius;
	fdesc.height = desc.height;

	bool success = fdesc.isValid();
	if (success)
	{
		mShapes.pushBack(fdesc);
	}

	return success;
}

bool FieldBoundaryAssetAuthoring::addConvex(const NxApexConvexFieldShapeDesc& desc)
{
	ConvexMeshDesc cdesc;
	cdesc.flags = desc.flags;
	cdesc.numPoints = desc.numVertices;
	cdesc.pointStrideBytes = sizeof(physx::PxVec3);

	physx::PxBounds3 meshBounds;
	meshBounds.setEmpty();

	physx::PxVec3 point;

	const char* curPos = (char*)desc.points;
	for (physx::PxU32 i = 0; i < desc.numVertices; i++)
	{
		physx::PxVec3* p = (physx::PxVec3*) curPos;
		point = * p;
		cdesc.points.pushBack(point);
		curPos += desc.pointStrideBytes;
		meshBounds.include(point);
	}

	FieldShapeDesc fdesc;
	fdesc.type = NX_APEX_SHAPE_CONVEX;
	fdesc.pose = desc.localPose;
	fdesc.dimensions = meshBounds.getDimensions(); //save the mesh bound size in fdesc.dimensions
	fdesc.mesh = &cdesc;

	bool success = fdesc.isValid();
	if (success)
	{
		mShapes.pushBack(fdesc);
		mConvex.pushBack(cdesc);
	}

	return success;
}
#endif

#endif // NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

}
}
} // end namespace physx::apex
