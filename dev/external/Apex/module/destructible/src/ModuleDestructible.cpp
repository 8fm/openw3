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

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
#include "ModuleDestructible.h"
#include "DestructibleAssetProxy.h"
#include "DestructibleActorProxy.h"
#include "DestructibleActorJointProxy.h"
#include "DestructibleScene.h"
#include "ApexSharedUtils.h"
#include "PxMemoryBuffer.h"

#if NX_SDK_VERSION_MAJOR == 2
#include "NxScene.h"
#include "NxCooking.h"
#include "NxConvexMeshDesc.h"
#include "NxConvexShapeDesc.h"
#elif NX_SDK_VERSION_MAJOR == 3
#include "PxPhysics.h"
#include "PxScene.h"
#endif

#include "ApexStream.h"
#include "ModulePerfScope.h"
using namespace destructible;
#endif

#include "NiApexSDK.h"
#include "PsShare.h"

#if APEX_USE_GRB
#if defined(PX_WINDOWS)
#include <PsWindowsInclude.h>
#include "ModuleUpdateLoader.h"
#endif

#include "cuda.h"

#if NX_SDK_VERSION_MAJOR == 3
#include "PxMaterial.h"
physx::PxMaterial * GrbActorExternal::grbMaterial = 0;
PxU32 GrbActorExternal::refCount = 0;
#endif

#if NX_SDK_VERSION_MAJOR == 2
#include "GrbPhysicsSDK.h"
#include "foundation/PxErrorCallback.h"

/* === Wrappers for GRB's old-style callbacks === */
static PxAllocatorWrapper
sPxAllocatorWrapper;

void*
PxAllocatorWrapper::mallocDEBUG(size_t size, const char* fileName, int line)
{
	return physx::Allocator().allocate(size, fileName, line);
}

void*
PxAllocatorWrapper::malloc(size_t size)
{
	return physx::Allocator().allocate(size, __FILE__, __LINE__);
}

void*
PxAllocatorWrapper::realloc(void* memory, size_t size)
{
	(void)memory;
	(void)size;
	return NULL;
}

void
PxAllocatorWrapper::free(void* memory)
{
	physx::shdfnd::Allocator().deallocate(memory);
}

void
PxUserOutputStreamWrapper::reportError(NxErrorCode code, const char* message, const char* file, int line)
{
	if (m_pxUserOutputStream != NULL)
	{
		physx::PxErrorCode::Enum pxCode = physx::PxErrorCode::eNO_ERROR;
		switch (code)
		{
		case NXE_INVALID_PARAMETER:
			pxCode = physx::PxErrorCode::eINVALID_PARAMETER;
			break;
		case NXE_INVALID_OPERATION:
			pxCode = physx::PxErrorCode::eINVALID_OPERATION;
			break;
		case NXE_OUT_OF_MEMORY:
			pxCode = physx::PxErrorCode::eOUT_OF_MEMORY;
			break;
		case NXE_INTERNAL_ERROR:
			pxCode = physx::PxErrorCode::eINTERNAL_ERROR;
			break;
		case NXE_ASSERTION:
			pxCode = physx::PxErrorCode::eINTERNAL_ERROR;
			break;
		case NXE_DB_INFO:
			pxCode = physx::PxErrorCode::eDEBUG_INFO;
			break;
		case NXE_DB_WARNING:
			pxCode = physx::PxErrorCode::eDEBUG_WARNING;
			break;
		case NXE_DB_PRINT:
			pxCode = physx::PxErrorCode::eNO_ERROR;
			break;
		}
		m_pxUserOutputStream->reportError(pxCode, message, file, line);
	}
};

NxAssertResponse
PxUserOutputStreamWrapper::reportAssertViolation(const char* message, const char* file, int line)
{
	if (m_pxUserOutputStream != NULL)
	{
		m_pxUserOutputStream->reportError(physx::PxErrorCode::eINTERNAL_ERROR, message, file, line);
	}
	return NX_AR_BREAKPOINT;
};

void
PxUserOutputStreamWrapper::print(const char* message)
{
	if (m_pxUserOutputStream != NULL)
	{
		m_pxUserOutputStream->reportError(physx::PxErrorCode::eDEBUG_INFO, message, NULL, 0);
	}
}
#endif
#endif


#if APEX_USE_GRB && NX_SDK_VERSION_MAJOR == 3

physx::PxShape*		GrbActorExternal::createShape(const physx::PxGeometry& geometry, physx::PxMaterial*const* materials, physx::PxU16 materialCount, physx::PxShapeFlags shapeFlags)
{
	PX_UNUSED(materialCount);

	if(!grbMaterial)
	{
		const physx::PxMaterial* pxmaterial = materials[0];

		grbMaterial = scene->getPhysics().createMaterial(pxmaterial->getStaticFriction(), pxmaterial->getDynamicFriction(), pxmaterial->getRestitution());
		grbMaterial->setFrictionCombineMode(pxmaterial->getFrictionCombineMode());
		grbMaterial->setRestitutionCombineMode(pxmaterial->getRestitutionCombineMode());
	}

	physx::PxShape *rshape =actor->createShape(geometry, &grbMaterial, 1, shapeFlags);
	return rshape;
}

void			GrbActorExternal::release()
{
	--refCount;

	if(!refCount && grbMaterial)
	{
		grbMaterial->release();
		grbMaterial = 0;
	}
	SCOPED_PHYSX3_LOCK_WRITE(actor->getScene());
	actor->release();
}

#endif

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

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	gApexSdk = inSdk;
	APEX_INIT_FOUNDATION();
	initModuleProfiling(inSdk, "Destructible");
	destructible::ModuleDestructible* impl = PX_NEW(destructible::ModuleDestructible)(inSdk);
	*niRef  = (NiModule*) impl;
	return (NxModule*) impl;
#else
	if (errorCode != NULL)
	{
		*errorCode = APEX_CE_WRONG_VERSION;
	}

	PX_UNUSED(niRef);
	PX_UNUSED(inSdk);
	return NULL; // Destructible Module cannot use this PhysX version
#endif
}
#else
/* Statically linking entry function */
void instantiateModuleDestructible()
{
#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED
	NiApexSDK* sdk = NiGetApexSDK();
	initModuleProfiling(sdk, "Destructible");
	destructible::ModuleDestructible* impl = PX_NEW(destructible::ModuleDestructible)(sdk);
	sdk->registerExternalModule((NxModule*) impl, (NiModule*) impl);
#endif
}
#endif

namespace destructible
{

#if NX_SDK_VERSION_NUMBER >= MIN_PHYSX_SDK_VERSION_REQUIRED

enum NxModuleDestructibleLODParameters
{
	/**
		Controls the maximum number of dynamic chunk islands allowed
	*/
	NX_DESTRUCTIBLE_LOD_CHUNK_ISLAND_NUM = 0,

	/**
		Controls the maximum number of levels in the chunk hierarchy
	*/
	NX_DESTRUCTIBLE_LOD_CHUNK_DEPTH = 1,

	/**
		Controls the maximum separation (both lifetime and distance) for which a dynamic chunk can exist
	*/
	NX_DESTRUCTIBLE_LOD_CHUNK_SEPARATION = 2
};

NxAuthObjTypeID DestructibleAsset::mAssetTypeID;  // Static class member
#ifdef WITHOUT_APEX_AUTHORING

class DestructibleAssetDummyAuthoring : public NxApexAssetAuthoring, public UserAllocated
{
public:
	DestructibleAssetDummyAuthoring(ModuleDestructible* module, NxResourceList& list, NxParameterized::Interface* params, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(params);
		PX_UNUSED(name);
	}

	DestructibleAssetDummyAuthoring(ModuleDestructible* module, NxResourceList& list, const char* name)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
		PX_UNUSED(name);
	}

	DestructibleAssetDummyAuthoring(ModuleDestructible* module, NxResourceList& list)
	{
		PX_UNUSED(module);
		PX_UNUSED(list);
	}

	virtual ~DestructibleAssetDummyAuthoring() {}

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


#if 0
	/**
	* \brief Save asset configuration to a stream
	*/
	virtual physx::PxFileBuf& serialize(physx::PxFileBuf& stream) const
	{
		PX_ASSERT(0);
		return stream;
	}

	/**
	* \brief Load asset configuration from a stream
	*/
	virtual physx::PxFileBuf& deserialize(physx::PxFileBuf& stream)
	{
		PX_ASSERT(0);
		return stream;
	}
#endif

	const char* getName(void) const
	{
		return NULL;
	}

	/**
	* \brief Returns the name of this APEX authorable object type
	*/
	virtual const char* getObjTypeName() const
	{
		return NX_DESTRUCTIBLE_AUTHORING_TYPE_NAME;
	}

	/**
	 * \brief Prepares a fully authored Asset Authoring object for a specified platform
	 */
	virtual bool prepareForPlatform(physx::apex::NxPlatformTag)
	{
		PX_ASSERT(0);
		return false;
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

typedef ApexAuthorableObject<ModuleDestructible, DestructibleAssetProxy, DestructibleAssetDummyAuthoring> DestructionAO;


#else
typedef ApexAuthorableObject<ModuleDestructible, DestructibleAssetProxy, DestructibleAssetAuthoringProxy> DestructionAO;
#endif


#if APEX_USE_GRB
#if NX_SDK_VERSION_MAJOR == 2
/* Manually fix-up this GRB.dll entry point */
typedef GrbPhysicsSDK* (GrbCreatePhysicsSDK_FUNC)(NxPhysicsSDK* nxPhysicsSDK, NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const GrbPhysicsSDKDesc& desc, NxSDKCreateError* errorCode);
typedef void (GrbReleasePhysicsSDK_FUNC)(GrbPhysicsSDK* physicsSDK);
static GrbReleasePhysicsSDK_FUNC* _GrbReleasePhysicsSDK;
#else
typedef physx::PxPhysics*  (GrbCreatePhysicsSDK_FUNC)(
	physx::PxU32 version,
	physx::PxFoundation & foundation,
	const physx::PxTolerancesScale & scale,
	bool trackOutstandingAllocations,
	physx::PxProfileZoneManager * profileZoneManager,
	physx::PxU32 cudaDeviceOrdinal
	);
#endif
#endif


ModuleDestructible::ModuleDestructible(NiApexSDK* inSdk) :
	m_maxChunkDepthOffset(0),
	m_maxChunkSeparationLOD(0.5f),
	m_maxFracturesProcessedPerFrame(PX_MAX_U32),
	m_maxActorsCreateablePerFrame(PX_MAX_U32),
	m_dynamicActorFIFOMax(0),
	m_chunkFIFOMax(0),
	m_sortByBenefit(false),
	m_chunkReport(NULL),
	m_chunkReportBitMask(0xffffffff),
	m_destructiblePhysXActorReport(NULL),
	m_chunkReportMaxFractureEventDepth(0xffffffff),
	m_chunkCrumbleReport(NULL),
	m_chunkDustReport(NULL),
#if APEX_USE_GRB
	mGrbInitialized(false),
	mGrbMeshCellSize(1.0f),
	mGrbSkinWidth(0.001f),
	mGrbNonPenSolverPosIterCount(18),
	mGrbFrictionSolverPosIterCount(6),
	mGrbFrictionSolverVelIterCount(6),
	mGrbMemTempDataSize(256),
	mGrbMemSceneSize(128),
	mGrbMaxLinAcceleration(FLT_MAX),
	mGrbPhysicsSDK(NULL),
#if  NX_SDK_VERSION_MAJOR == 2
	mPxUserOutputStreamWrapper(inSdk->getErrorCallback()),
#endif
#endif
	m_massScale(1.0f),
	m_scaledMassExponent(0.5f),
	mApexDestructiblePreviewParams(NULL),
	mUseLegacyChunkBoundsTesting(false),
	mUseLegacyDamageRadiusSpread(false)
{
	name = "Destructible";
	mSdk = inSdk;
	mApiProxy = this;
	mModuleParams = NULL;

	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();
	if (traits)
	{
#		define PARAM_CLASS(clas) PARAM_CLASS_REGISTER_FACTORY(traits, clas)
#		include "DestructibleParamClasses.inc"

		mApexDestructiblePreviewParams = traits->createNxParameterized(DestructiblePreviewParam::staticClassName());
	}

	/* Register this module's authorable object types and create their namespaces */
	const char* pName = DestructibleAssetParameters::staticClassName();
	DestructionAO* eAO = PX_NEW(DestructionAO)(this, mAuthorableObjects, pName);
	DestructibleAsset::mAssetTypeID = eAO->getResID();
	//	NX_DESTRUCTIBLE_LOD_CHUNK_ISLAND_NUM = 0
	registerLODParameter("ChunkIslandNum", NxRange<physx::PxU32>(1, 10));
	//	NX_DESTRUCTIBLE_LOD_CHUNK_DEPTH = 1
	registerLODParameter("ChunkDepth", NxRange<physx::PxU32>(1, 10));
	//	NX_DESTRUCTIBLE_LOD_CHUNK_SEPARATION = 2
	registerLODParameter("ChunkSeparation", NxRange<physx::PxU32>(1, 10));

	mCachedData = PX_NEW(DestructibleModuleCachedData)(getModuleID());
	// Set per-platform unit cost.  One unit is one chunk.
#if defined(PX_WINDOWS) || defined( PX_XBOXONE )
	mLodUnitCost = 0.01f;
#elif defined(PX_PS4)
	mLodUnitCost = 0.01f;
#elif defined(PX_X360)
	mLodUnitCost = 0.1f;
#elif defined(PX_PS3)
	mLodUnitCost = 0.1f;
#elif defined(PX_ANDROID)
	mLodUnitCost = 0.1f;
#else
	// Using default value set in Module class
#endif
}

NxAuthObjTypeID ModuleDestructible::getModuleID() const
{
	return DestructibleAsset::mAssetTypeID;
}

ModuleDestructible::~ModuleDestructible()
{
	m_destructibleSceneList.clear();

	// This needs to happen after the scene list is cleared (actors do stuff)
	releaseModuleProfiling();

	PX_DELETE(mCachedData);
	mCachedData = NULL;

#if APEX_USE_GRB
	if (mGrbPhysicsSDK)
	{
#if NX_SDK_VERSION_MAJOR == 2
		_GrbReleasePhysicsSDK(mGrbPhysicsSDK);
#else
		mGrbPhysicsSDK->release();
#endif
		mGrbPhysicsSDK = NULL;
	}
#endif
}

NxParameterized::Interface* ModuleDestructible::getDefaultModuleDesc()
{
	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (!mModuleParams)
	{
		mModuleParams = DYNAMIC_CAST(DestructibleModuleParameters*)
		                (traits->createNxParameterized("DestructibleModuleParameters"));
		PX_ASSERT(mModuleParams);
	}
	else
	{
		mModuleParams->initDefaults();
	}

	return mModuleParams;
}

void ModuleDestructible::init(NxParameterized::Interface& desc)
{
	if (strcmp(desc.className(), DestructibleModuleParameters::staticClassName()) == 0)
	{
		DestructibleModuleParameters* params = DYNAMIC_CAST(DestructibleModuleParameters*)(&desc);
		setValidBoundsPadding(params->validBoundsPadding);
		setMaxDynamicChunkIslandCount(params->maxDynamicChunkIslandCount);
		setSortByBenefit(params->sortFIFOByBenefit);
		setMaxChunkSeparationLOD(params->maxChunkSeparationLOD);
		setMaxActorCreatesPerFrame(params->maxActorCreatesPerFrame);
		setMaxChunkDepthOffset(params->maxChunkDepthOffset);

		if (params->massScale > 0.0f)
		{
			m_massScale = params->massScale;
		}

		if (params->scaledMassExponent > 0.0f && params->scaledMassExponent <= 1.0f)
		{
			m_scaledMassExponent = params->scaledMassExponent;
		}

#if APEX_USE_GRB
		const bool grbDescValid =
		    params->gpuRigidBodySettings.nonPenSolverPosIterCount +
		    params->gpuRigidBodySettings.frictionSolverPosIterCount +
		    params->gpuRigidBodySettings.frictionSolverVelIterCount > 0 &&
		    params->gpuRigidBodySettings.gpuMemSceneSize > 0 &&
			params->gpuRigidBodySettings.gpuMemTempDataSize > 0 &&
		    params->gpuRigidBodySettings.gpuMemSceneSize +
			params->gpuRigidBodySettings.gpuMemTempDataSize < 512;
		if (grbDescValid)
		{

			int ordinal = params->gpuRigidBodySettings.gpuDeviceOrdinal;

			bool disabled = (ordinal == -1);
			const bool silent = ordinal == -3;
			if (ordinal == -2 || ordinal == -3)
			{
				ordinal = mSdk->getSuggestedCudaDeviceOrdinal();
			}

			//if (ordinal >= 0)
			//{
			//	// Disable GRB loading if older than Fermi
			//	cuInit(0);

			//	CUdevice dev;
			//	CUresult res = cuDeviceGet(&dev, ordinal);

			//	if (res == CUDA_SUCCESS)
			//	{
			//		int computeCapMinor, computeCapMajor;
			//		res = cuDeviceComputeCapability(&computeCapMajor, &computeCapMinor, dev);
			//		if (res == CUDA_SUCCESS)
			//		{
			//			if (computeCapMajor < 2)
			//			{
			//				ordinal = -1;	// disable
			//				disabled = true;
			//				APEX_DEBUG_WARNING("GRB usage in APEX Destruction is disabled for compute capability < 2.0.");
			//			}
			//		}
			//	}
			//}

			if (ordinal >= 0)
			{
#if NX_SDK_VERSION_MAJOR == 2
#	if _DEBUG
				ApexSimpleString dllName("GRBDEBUG_1_1_api2");
#	else
#		if PX_CHECKED
				ApexSimpleString dllName("GRBCHECKED_1_1_api2");
#		elif PX_PROFILE
				ApexSimpleString dllName("GRBPROFILE_1_1_api2");
#		else
				ApexSimpleString dllName("GRB_1_1_api2");
#		endif
#	endif
#else
#	if _DEBUG
				ApexSimpleString dllName("GRBDEBUG_1_1_api3");
#	else
#		if PX_CHECKED
				ApexSimpleString dllName("GRBCHECKED_1_1_api3");
#		elif PX_PROFILE
				ApexSimpleString dllName("GRBPROFILE_1_1_api3");
#		else
				ApexSimpleString dllName("GRB_1_1_api3");
#		endif
#	endif
#endif

#if defined(PX_X86)
				dllName += ApexSimpleString("_x86.dll");
#else
				dllName += ApexSimpleString("_x64.dll");
#endif

				HMODULE library = NULL;
				{
					ModuleUpdateLoader moduleLoader(UPDATE_LOADER_DLL_NAME);
					library = moduleLoader.loadModule(dllName.c_str(), mSdk->getAppGuid());
				}

				if (library)
				{
#if NX_SDK_VERSION_MAJOR == 2
					GrbCreatePhysicsSDK_FUNC* _GrbCreatePhysicsSDK = (GrbCreatePhysicsSDK_FUNC*)GetProcAddress(library, "GrbCreatePhysicsSDK");
#else
					GrbCreatePhysicsSDK_FUNC* _GrbCreatePhysicsSDK = (GrbCreatePhysicsSDK_FUNC*)GetProcAddress(library, "GrbCreatePhysics");
#endif
					if (_GrbCreatePhysicsSDK != NULL)
					{
#if NX_SDK_VERSION_MAJOR == 2
						GrbPhysicsSDKDesc desc;
						desc.cudaDeviceOrdinal = ordinal;
						desc.gpuMemSceneSize = params->gpuRigidBodySettings.gpuMemSceneSize;
						desc.gpuMemTempDataSize = params->gpuRigidBodySettings.gpuMemTempDataSize;
						NxSDKCreateError errorCode = NXCE_NO_ERROR;
						mGrbPhysicsSDK = _GrbCreatePhysicsSDK(NiGetApexSDK()->getPhysXSDK(), NX_PHYSICS_SDK_VERSION, &sPxAllocatorWrapper, &mPxUserOutputStreamWrapper, desc, &errorCode);
#else
						mGrbPhysicsSDK = _GrbCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, this->mSdk->getPhysXSDK()->getFoundation(), this->mSdk->getPhysXSDK()->getTolerancesScale(),
							true, this->mSdk->getPhysXSDK()->getProfileZoneManager(), ordinal);
#endif
					}
					else
					{
						APEX_DEBUG_WARNING("GRB createPhysicsSDK failed.");
					}

#if NX_SDK_VERSION_MAJOR == 2
					_GrbReleasePhysicsSDK = (GrbReleasePhysicsSDK_FUNC*)GetProcAddress(library, "GrbReleasePhysicsSDK");
#endif
				}
				else
				{
					ApexSimpleString dllErrorMsg("Failed to load GRB dll: ");
					dllErrorMsg += dllName;
					dllErrorMsg += ApexSimpleString(". Even if the GRB dll is present, the load failure could be caused by a missing dependent dll, such as cudart or physXloader.");

					APEX_DEBUG_WARNING(dllErrorMsg.c_str());
				}
				if (mGrbPhysicsSDK != NULL
#if NX_SDK_VERSION_MAJOR == 2
					&& _GrbReleasePhysicsSDK != NULL
#endif					
					)
				{
					mGrbMeshCellSize = params->gpuRigidBodySettings.meshCellSize;
					mGrbSkinWidth = params->gpuRigidBodySettings.skinWidth;
					mGrbNonPenSolverPosIterCount = params->gpuRigidBodySettings.nonPenSolverPosIterCount;
					mGrbFrictionSolverPosIterCount = params->gpuRigidBodySettings.frictionSolverPosIterCount;
					mGrbFrictionSolverVelIterCount = params->gpuRigidBodySettings.frictionSolverVelIterCount;
					mGrbMemSceneSize = params->gpuRigidBodySettings.gpuMemSceneSize;
					mGrbMemTempDataSize = params->gpuRigidBodySettings.gpuMemTempDataSize;
					mGrbMaxLinAcceleration = params->gpuRigidBodySettings.maxLinAcceleration;
					mGrbInitialized = true;
					for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
					{
						DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
						if (ds->mGrbSimulationEnabled)
						{
							ds->createGRBScene();
						}
					}
				}
			}
			else if (!disabled && !silent)
			{
				APEX_DEBUG_WARNING("GRB not loaded, no CUDA capable GPU found.");
			}
		}
		else
		{
			APEX_DEBUG_WARNING("GRB descriptor parameters not valid. GRB simulation not initialized.");
		}
#endif
	}
	else
	{
		APEX_INVALID_PARAMETER("The NxParameterized::Interface object is the wrong type");
	}
}

NiModuleScene* ModuleDestructible::createNiModuleScene(NiApexScene& scene, NiApexRenderDebug* debugRender)
{
	return PX_NEW(DestructibleScene)(*this, scene, debugRender, m_destructibleSceneList);
}

void ModuleDestructible::releaseNiModuleScene(NiModuleScene& scene)
{
	DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(&scene);
	ds->destroy();
}

physx::PxU32 ModuleDestructible::forceLoadAssets()
{
	physx::PxU32 loadedAssetCount = 0;

	for (physx::PxU32 i = 0; i < mAuthorableObjects.getSize(); i++)
	{
		NiApexAuthorableObject* ao = static_cast<NiApexAuthorableObject*>(mAuthorableObjects.getResource(i));
		loadedAssetCount += ao->forceLoadAssets();
	}
	return loadedAssetCount;
}

DestructibleScene* ModuleDestructible::getDestructibleScene(const NxApexScene& apexScene) const
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			return ds;
		}
	}

	PX_ASSERT(!"Unable to locate an appropriate DestructibleScene");
	return NULL;
}

NxApexRenderableIterator* ModuleDestructible::createRenderableIterator(const NxApexScene& apexScene)
{
	DestructibleScene* ds = getDestructibleScene(apexScene);
	if (ds)
	{
		return ds->createRenderableIterator();
	}

	return NULL;
}

NxDestructibleActorJoint* ModuleDestructible::createDestructibleActorJoint(const NxDestructibleActorJointDesc& destructibleActorJointDesc, NxApexScene& scene)
{
	if (!destructibleActorJointDesc.isValid())
	{
		return NULL;
	}
	DestructibleScene* 	ds = getDestructibleScene(scene);
	if (ds)
	{
		return ds->createDestructibleActorJoint(destructibleActorJointDesc);
	}
	else
	{
		return NULL;
	}
}

bool ModuleDestructible::isDestructibleActorJointActive(const NxDestructibleActorJoint* candidateJoint, NxApexScene& apexScene) const
{
	PX_ASSERT(candidateJoint != NULL);
	PX_ASSERT(&apexScene != NULL);
	DestructibleScene* destructibleScene = NULL;
	destructibleScene = getDestructibleScene(apexScene);
	PX_ASSERT(destructibleScene != NULL);
	bool found = false;
	if (destructibleScene != NULL)
	{
		for (physx::PxU32 index = 0; index < destructibleScene->mDestructibleActorJointList.getSize(); ++index)
		{
			NxDestructibleActorJoint* activeJoint = NULL;
			activeJoint = static_cast<NxDestructibleActorJoint*>(static_cast<DestructibleActorJointProxy*>((destructibleScene->mDestructibleActorJointList.getResource(index))));
			PX_ASSERT(activeJoint != NULL);
			if (activeJoint == candidateJoint)
			{
				found = true;
				break;
			}
		}
	}
	return found;
}

void ModuleDestructible::setMaxDynamicChunkIslandCount(physx::PxU32 maxCount)
{
	m_dynamicActorFIFOMax = maxCount;
}

void ModuleDestructible::setMaxChunkCount(physx::PxU32 maxCount)
{
	m_chunkFIFOMax = maxCount;
}

void ModuleDestructible::setSortByBenefit(bool sortByBenefit)
{
	m_sortByBenefit = sortByBenefit;
}

void ModuleDestructible::setMaxChunkDepthOffset(physx::PxU32 offset)
{
	m_maxChunkDepthOffset = offset;
}

void ModuleDestructible::setMaxChunkSeparationLOD(physx::PxF32 separationLOD)
{
	m_maxChunkSeparationLOD = physx::PxClamp(separationLOD, 0.0f, 1.0f);
}

void ModuleDestructible::setValidBoundsPadding(physx::PxF32 pad)
{
	m_validBoundsPadding = pad;
}

void ModuleDestructible::setChunkReport(NxUserChunkReport* chunkReport)
{
	m_chunkReport = chunkReport;
}

void ModuleDestructible::setChunkReportBitMask(physx::PxU32 chunkReportBitMask)
{
	m_chunkReportBitMask = chunkReportBitMask;
}

void ModuleDestructible::setDestructiblePhysXActorReport(NxUserDestructiblePhysXActorReport* destructiblePhysXActorReport)
{
	m_destructiblePhysXActorReport = destructiblePhysXActorReport;
}

void ModuleDestructible::setChunkReportMaxFractureEventDepth(physx::PxU32 chunkReportMaxFractureEventDepth)
{
	m_chunkReportMaxFractureEventDepth = chunkReportMaxFractureEventDepth;
}

void ModuleDestructible::setChunkCrumbleReport(NxUserChunkParticleReport* chunkCrumbleReport)
{
	m_chunkCrumbleReport = chunkCrumbleReport;
}

void ModuleDestructible::setChunkDustReport(NxUserChunkParticleReport* chunkDustReport)
{
	m_chunkDustReport = chunkDustReport;
}

#if NX_SDK_VERSION_MAJOR == 2
void ModuleDestructible::setWorldSupportPhysXScene(NxApexScene& apexScene, NxScene* physxScene)
#elif NX_SDK_VERSION_MAJOR == 3
void ModuleDestructible::setWorldSupportPhysXScene(NxApexScene& apexScene, PxScene* physxScene)
#endif
{
	DestructibleScene* ds = getDestructibleScene(apexScene);
	if (ds)
	{
		ds->setWorldSupportPhysXScene(physxScene);
	}
}

#if NX_SDK_VERSION_MAJOR == 2
bool ModuleDestructible::owns(const NxActor* actor) const
#elif NX_SDK_VERSION_MAJOR == 3
bool ModuleDestructible::owns(const PxRigidActor* actor) const
#endif
{
	const NiApexPhysXObjectDesc* desc = static_cast<const NiApexPhysXObjectDesc*>(mSdk->getPhysXObjectInfo(actor));
	if (desc != NULL)
	{
		const physx::PxU32 actorCount = desc->mApexActors.size();
		for (physx::PxU32 i = 0; i < actorCount; ++i)
		{
			const NxApexActor* actor = desc->mApexActors[i];
			if (actor != NULL && actor->getOwner()->getObjTypeID() == DestructibleAsset::mAssetTypeID)
			{
				return true;
			}
		}
	}

	return false;
}

#if NX_SDK_VERSION_MAJOR == 3
#if APEX_RUNTIME_FRACTURE
bool ModuleDestructible::isRuntimeFractureShape(const PxShape& shape) const
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		physx::fracture::SimScene* simScene = ds->getDestructibleRTScene(false);
		if (simScene && simScene->owns(shape))
		{
			return true;
		}
	}
	return false;
}
#endif
#endif


NxDestructibleActor* ModuleDestructible::getDestructibleAndChunk(const NxShape* shape, physx::PxI32* chunkIndex) const
{
	const NiApexPhysXObjectDesc* desc = static_cast<const NiApexPhysXObjectDesc*>(mSdk->getPhysXObjectInfo(shape));
	if (desc != NULL)
	{
		const physx::PxU32 actorCount = desc->mApexActors.size();
		PX_ASSERT(actorCount == 1);	// Shapes should only be associated with one chunk
		if (actorCount > 0)
		{
			const DestructibleActorProxy* actorProxy = (DestructibleActorProxy*)desc->mApexActors[0];
			if (actorProxy->getOwner()->getObjTypeID() == DestructibleAsset::mAssetTypeID)
			{
				const DestructibleScene* ds = actorProxy->impl.getDestructibleScene();
				DestructibleStructure::Chunk* chunk = (DestructibleStructure::Chunk*)desc->userData;
				if (chunk == NULL || chunk->destructibleID == DestructibleStructure::InvalidID || chunk->destructibleID > ds->mDestructibles.capacity())
				{
					return NULL;
				}

				DestructibleActor* destructible = ds->mDestructibles.direct(chunk->destructibleID);
				if (destructible == NULL)
				{
					return NULL;
				}

				if (chunkIndex)
				{
					*chunkIndex = (physx::PxI32)chunk->indexInAsset;
				}
				return destructible->getAPI();
			}
		}
	}

	return NULL;
}

void ModuleDestructible::applyRadiusDamage(NxApexScene& scene, physx::PxF32 damage, physx::PxF32 momentum, const physx::PxVec3& position, physx::PxF32 radius, bool falloff)
{
	DestructibleScene* ds = getDestructibleScene(scene);
	if (ds)
	{
		ds->applyRadiusDamage(damage, momentum, position, radius, falloff);
	}
}

void ModuleDestructible::setMaxActorCreatesPerFrame(physx::PxU32 maxActorsPerFrame)
{
	m_maxActorsCreateablePerFrame = maxActorsPerFrame;
}

void ModuleDestructible::setMaxFracturesProcessedPerFrame(physx::PxU32 maxFracturesProcessedPerFrame)
{
	m_maxFracturesProcessedPerFrame = maxFracturesProcessedPerFrame;
}

void ModuleDestructible::setIntValue(physx::PxU32 parameterIndex, physx::PxU32 value)
{
	Module::setIntValue(parameterIndex, value);
	switch (parameterIndex)
	{
	case NX_DESTRUCTIBLE_LOD_CHUNK_ISLAND_NUM:
	{
		physx::PxF32 value = getCurrentValue(NxRange<physx::PxU32>(10, 1000), parameterIndex);
		setMaxDynamicChunkIslandCount((physx::PxU32)(value + 0.5f));
	}
	break;
	case NX_DESTRUCTIBLE_LOD_CHUNK_DEPTH:
	{
		physx::PxF32 value = getCurrentValue(NxRange<physx::PxU32>(2, 0), parameterIndex);
		setMaxChunkDepthOffset((physx::PxU32)(value + 0.5f));
	}
	break;
	case NX_DESTRUCTIBLE_LOD_CHUNK_SEPARATION:
	{
		physx::PxF32 value = getCurrentValue(NxRange<physx::PxU32>(0, 1), parameterIndex);
		setMaxChunkSeparationLOD(value);
	}
	break;
	}
}

void ModuleDestructible::releaseBufferedConvexMeshes()
{
	for (physx::PxU32 i = 0; i < convexMeshKillList.size(); i++)
	{
#if NX_SDK_VERSION_MAJOR == 2
		NxGetApexSDK()->getPhysXSDK()->releaseConvexMesh(*convexMeshKillList[i]);
#elif NX_SDK_VERSION_MAJOR == 3
		convexMeshKillList[i]->release();
#endif
	}
	convexMeshKillList.clear();
}

void ModuleDestructible::destroy()
{
#if APEX_USE_GRB
	for (physx::PxU32 i = twoWayRbList.size(); i--;)
	{
		GrbActorExternal* externalActor = twoWayRbList[i];
		if (m_destructiblePhysXActorReport != NULL)
		{
			m_destructiblePhysXActorReport->onPhysXActorRelease(*externalActor->actor);
		}
#if NX_SDK_VERSION_MAJOR == 2
		externalActor->actor->getScene().releaseActor(*externalActor->actor);
#else		
	externalActor->release();
#endif		
		PX_DELETE(externalActor);
	}
	twoWayRbList.clear();
#endif /* APEX_USE_GRB */

	NxParameterized::Traits* traits = mSdk->getParameterizedTraits();

	if (mModuleParams != NULL)
	{
		mModuleParams->destroy();
		mModuleParams = NULL;
	}

	if (mApexDestructiblePreviewParams != NULL)
	{
		mApexDestructiblePreviewParams->destroy();
		mApexDestructiblePreviewParams = NULL;
	}

	// base class
	Module::destroy();

	releaseBufferedConvexMeshes();

	delete this;

	if (traits)
	{
#		define PARAM_CLASS(clas) PARAM_CLASS_REMOVE_FACTORY(traits, clas)
#		include "DestructibleParamClasses.inc"
	}

}

/*** ModuleDestructible::SyncParams ***/
bool ModuleDestructible::setSyncParams(UserDamageEventHandler * userDamageEventHandler, UserFractureEventHandler * userFractureEventHandler, UserChunkMotionHandler * userChunkMotionHandler)
{
	bool validEntry = false;
	validEntry = ((NULL != userChunkMotionHandler) ? (NULL != userDamageEventHandler || NULL != userFractureEventHandler) : true);
	if(validEntry)
	{
		mSyncParams.userDamageEventHandler		= userDamageEventHandler;
		mSyncParams.userFractureEventHandler	= userFractureEventHandler;
		mSyncParams.userChunkMotionHandler		= userChunkMotionHandler;
	}
	return validEntry;
}

typedef ModuleDestructible::SyncParams SyncParams;

SyncParams::SyncParams()
:userDamageEventHandler(NULL)
,userFractureEventHandler(NULL)
,userChunkMotionHandler(NULL)
{
}

SyncParams::~SyncParams()
{
    userChunkMotionHandler = NULL;
	userFractureEventHandler = NULL;
	userDamageEventHandler = NULL;
}

UserDamageEventHandler * SyncParams::getUserDamageEventHandler() const
{
	return userDamageEventHandler;
}

UserFractureEventHandler * SyncParams::getUserFractureEventHandler() const
{
	return userFractureEventHandler;
}

UserChunkMotionHandler * SyncParams::getUserChunkMotionHandler() const
{
	return userChunkMotionHandler;
}

template<typename T> physx::PxU32 SyncParams::getSize() const
{
	return sizeof(T);
}

template physx::PxU32 SyncParams::getSize<NxApexDamageEventHeader>		() const;
template physx::PxU32 SyncParams::getSize<NxApexDamageEventUnit>		() const;
template physx::PxU32 SyncParams::getSize<NxApexFractureEventHeader>	() const;
template physx::PxU32 SyncParams::getSize<NxApexFractureEventUnit>		() const;
template physx::PxU32 SyncParams::getSize<NxApexChunkTransformHeader>	() const;
template physx::PxU32 SyncParams::getSize<NxApexChunkTransformUnit>		() const;

const SyncParams & ModuleDestructible::getSyncParams() const
{
    return mSyncParams;
}

#if NX_SDK_VERSION_MAJOR == 2

NxActor* ModuleDestructible::createTwoWayRb(const NxActorDesc& desc, NxApexScene& apexScene)
{
	NxActor* actor = NULL;
# if APEX_USE_GRB
	if (isGrbSimulationEnabled(apexScene))
	{
		actor = createGrbActor(desc, apexScene);
		if (actor == NULL)
		{
			NiGetApexSDK()->reportError(physx::PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "ModuleDestructible::createTwoWayRb", "createTwoWayRb failed to create a GRB actor");
			return NULL;
		}
	}
# endif
	if (actor == NULL)
	{
		for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
		{
			DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
			if (ds->mApexScene == &apexScene)
			{
				actor = ds->mPhysXScene->createActor(desc);
				break;
			}
		}
		if (actor == NULL)
		{
			NiGetApexSDK()->reportError(physx::PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "ModuleDestructible::createTwoWayRb", "createTwoWayRb failed to create a PhysX actor");
			return NULL;
		}
	}

	if (m_destructiblePhysXActorReport != NULL)
	{
		m_destructiblePhysXActorReport->onPhysXActorCreate(*actor);
	}

#if APEX_USE_GRB
	GrbActorExternal* actorExternal = PX_NEW(GrbActorExternal)(actor);
	twoWayRbList.pushBack(actorExternal);
	return actorExternal;
#else
	return actor;
#endif
}

bool ModuleDestructible::releaseTwoWayRb(NxActor& actor)
{
#if APEX_USE_GRB
	for (physx::PxU32 i = twoWayRbList.size(); i--;)
	{
		GrbActorExternal* externalActor = twoWayRbList[i];
		if (externalActor == &actor)
		{
			twoWayRbList.replaceWithLast(i);
			if (m_destructiblePhysXActorReport != NULL)
			{
				m_destructiblePhysXActorReport->onPhysXActorRelease(*externalActor->actor);
			}
			externalActor->actor->getScene().releaseActor(*externalActor->actor);
			PX_DELETE(externalActor);

			return true;
		}
	}
#else
	PX_UNUSED(actor);
#endif
	return false;
}
#else

PxRigidDynamic * ModuleDestructible::createTwoWayRb(const physx::PxTransform & transform, NxApexScene& apexScene)
{
	PxRigidDynamic* actor = NULL;
	physx::PxScene* scene = 0;

# if APEX_USE_GRB
	if (isGrbSimulationEnabled(apexScene))
	{
		actor = createGrbActor(transform, apexScene, &scene);
		if (actor == NULL)
		{
			NiGetApexSDK()->reportError(physx::PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "ModuleDestructible::createTwoWayRb", "createTwoWayRb failed to create a GRB actor");
			return NULL;
		}
	}
# endif
	if (actor == NULL)
	{
		for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
		{
			DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
			if (ds->mApexScene == &apexScene)
			{
				scene = ds->mPhysXScene;
				PX_UNUSED(scene);
				actor = ds->mPhysXScene->getPhysics().createRigidDynamic(transform);
				break;
			}
		}
		if (actor == NULL)
		{
			NiGetApexSDK()->reportError(physx::PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "ModuleDestructible::createTwoWayRb", "createTwoWayRb failed to create a PhysX actor");
			return NULL;
		}
	}

	if (m_destructiblePhysXActorReport != NULL)
	{
		m_destructiblePhysXActorReport->onPhysXActorCreate(*actor);
	}

# if APEX_USE_GRB
	GrbActorExternal* actorExternal = PX_NEW(GrbActorExternal)(actor, scene);
	return actorExternal;
#else
	return actor;
#endif

}

physx::PxRigidDynamic * ModuleDestructible::addTwoWayRb(PxRigidDynamic * actorExternal, NxApexScene& apexScene)
{
#if APEX_USE_GRB
	PxRigidDynamic * desc = ((GrbActorExternal *) actorExternal)->actor;
	if (isGrbSimulationEnabled(apexScene))
	{
		addGrbActor(desc, apexScene);
	}
	else
#else
	PxRigidDynamic* desc = actorExternal;
#endif
	{
		bool found = false;

		for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
		{
			DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
			if (ds->mApexScene == &apexScene)
			{
				SCOPED_PHYSX3_LOCK_WRITE(ds->mPhysXScene);
				ds->mPhysXScene->addActor(*desc);
				found = true;
				break;
			}
		}
		if (!found)
		{
			NiGetApexSDK()->reportError(physx::PxErrorCode::eINTERNAL_ERROR, __FILE__, __LINE__, "ModuleDestructible::createTwoWayRb", "createTwoWayRb failed to create a PhysX actor");
			return NULL;
		}
	}

#if APEX_USE_GRB
	twoWayRbList.pushBack((GrbActorExternal *) actorExternal);
#endif

	return desc;
}

bool ModuleDestructible::releaseTwoWayRb(PxRigidDynamic& actor)
{
#if APEX_USE_GRB
	for (physx::PxU32 i = twoWayRbList.size(); i--;)
	{
		GrbActorExternal* externalActor = twoWayRbList[i];
		if (externalActor == &actor)
		{
			twoWayRbList.replaceWithLast(i);
			externalActor->release();
			PX_DELETE(externalActor);

			return true;
		}
	}
#else
	PX_UNUSED(actor);
#endif
	return false;
}

#endif // NX_SDK_VERSION_MAJOR == 2


#if APEX_USE_GRB
void ModuleDestructible::setGrbMeshCellSize(float cellSize)
{
	mGrbMeshCellSize = cellSize;
}

void ModuleDestructible::setGrbMaxLinAcceleration(float maxLinAcceleration)
{
	mGrbMaxLinAcceleration = maxLinAcceleration;
}

bool ModuleDestructible::isGrbSimulationEnabled(const NxApexScene& apexScene) const
{
	if (!mGrbInitialized)
	{
		return false;
	}

	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		const DestructibleScene* ds = DYNAMIC_CAST(const DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			return ds->isGrbSimulationEnabled();
		}
	}

	return false;
}

void ModuleDestructible::setGrbSimulationEnabled(NxApexScene& apexScene, bool enabled)
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			ds->setGrbSimulationEnabled(enabled);
		}
	}
}

#if NX_SDK_VERSION_MAJOR == 2

NxActor* ModuleDestructible::createGrbActor(const NxActorDesc& desc, NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			PX_PROFILER_PERF_SCOPE("GrbScene_createActor");

			NxActorDesc mydesc = desc;
			return ds->mGrbScene->createActor(mydesc);
		}
	}
	PX_ASSERT(!"Unable to locate an appropriate GRBScene");
	return NULL;
}

#else

PxRigidDynamic* ModuleDestructible::createGrbActor(const physx::PxTransform & transform, NxApexScene& apexScene, physx::PxScene** ppScene)
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			if(ppScene)
			{
				*ppScene = ds->mGrbScene;
			}

			PxRigidDynamic* actor = ds->mGrbScene->getPhysics().createRigidDynamic(transform);
			if (actor != NULL && m_destructiblePhysXActorReport != NULL)
			{
				m_destructiblePhysXActorReport->onPhysXActorCreate(*actor);
			}

			return actor;
		}
	}
	PX_ASSERT(!"Unable to locate an appropriate GRBScene");
	return NULL;
}

void ModuleDestructible::addGrbActor(PxRigidDynamic * desc, NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			ds->mGrbScene->addActor(*desc);

			return;
		}
	}
	PX_ASSERT(!"Unable to locate an appropriate GRBScene");

	return;
}

#endif	//NX_SDK_VERSION_MAJOR == 2

/*
void ModuleDestructible::releaseGrbActor(GrbActor& actor, NxApexScene& apexScene)
{
	for (physx::PxU32 i = 0 ; i < m_destructibleSceneList.getSize() ; i++)
	{
		DestructibleScene* ds = DYNAMIC_CAST(DestructibleScene*)(m_destructibleSceneList.getResource(i));
		if (ds->mApexScene == &apexScene)
		{
			ds->deadGRBs.pushBack(&actor);
			return;
		}
	}
}
*/

#endif	// #if APEX_USE_GRB

void ModuleDestructible::setUseLegacyChunkBoundsTesting(bool useLegacyChunkBoundsTesting)
{
	mUseLegacyChunkBoundsTesting = useLegacyChunkBoundsTesting;
}

void ModuleDestructible::setUseLegacyDamageRadiusSpread(bool useLegacyDamageRadiusSpread)
{
	mUseLegacyDamageRadiusSpread = useLegacyDamageRadiusSpread;
}

bool ModuleDestructible::setMassScaling(physx::PxF32 massScale, physx::PxF32 scaledMassExponent, NxApexScene& apexScene)
{
	DestructibleScene* dscene = getDestructibleScene(apexScene);

	if (dscene != NULL)
	{
		return dscene->setMassScaling(massScale, scaledMassExponent);
	}

	return false;
}

/*******************************
* DestructibleModuleCachedData *
*******************************/

DestructibleModuleCachedData::DestructibleModuleCachedData(NxAuthObjTypeID moduleID) :
	mModuleID(moduleID)
{
}

DestructibleModuleCachedData::~DestructibleModuleCachedData()
{
	clear();
}

NxParameterized::Interface* DestructibleModuleCachedData::getCachedDataForAssetAtScale(NxApexAsset& asset, const physx::PxVec3& scale)
{
	DestructibleAsset& dasset = DYNAMIC_CAST(DestructibleAssetProxy*)(&asset)->impl;

	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);
		collisionSet->setDestructibleAssetToCook(&dasset);
		
		physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-
		mAssetCollisionSets.pushBack(collisionSet);
	}

	return collisionSet->getCollisionAtScale(scale);
}

physx::PxFileBuf& DestructibleModuleCachedData::serialize(physx::PxFileBuf& stream) const
{
	stream << (physx::PxU32)Version::Current;

	stream << (physx::PxU32)mModuleID;

	physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-

	stream << mAssetCollisionSets.size();
	for (physx::PxU32 i = 0; i < mAssetCollisionSets.size(); ++i)
	{
		mAssetCollisionSets[i]->serialize(stream);
	}

	return stream;
}

physx::PxFileBuf& DestructibleModuleCachedData::deserialize(physx::PxFileBuf& stream)
{
	clear();

	/*const physx::PxU32 version =*/
	stream.readDword();	// Original version

	mModuleID = stream.readDword();

	const physx::PxU32 dataSetCount = stream.readDword();

	physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-

	mAssetCollisionSets.resize(dataSetCount);
	for (physx::PxU32 i = 0; i < mAssetCollisionSets.size(); ++i)
	{
		mAssetCollisionSets[i] = PX_NEW(DestructibleAssetCollision);
		mAssetCollisionSets[i]->deserialize(stream);
	}

	return stream;
}

physx::PxFileBuf& DestructibleModuleCachedData::serializeSingleAsset(NxApexAsset& asset, physx::PxFileBuf& stream)
{
	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if( collisionSet )
	{
		collisionSet->serialize(stream);
	}

	return stream;
}

physx::PxFileBuf& DestructibleModuleCachedData::deserializeSingleAsset(NxApexAsset& asset, physx::PxFileBuf& stream)
{
	const char * assetName = asset.getName();
	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset.getName());
	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);

		physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-
		mAssetCollisionSets.pushBack(collisionSet);	
	}
	collisionSet->deserialize(stream);
	collisionSet->setAssetName( assetName ); // ctremblay +- HACK override name. should use asset name, not the one in data.

	return stream;
}

void DestructibleModuleCachedData::clear()
{
	physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-

	for (physx::PxU32 i = mAssetCollisionSets.size(); i--;)
	{
		PX_DELETE(mAssetCollisionSets[i]);
	}
	mAssetCollisionSets.reset();
}

void DestructibleModuleCachedData::clearAssetCollisionSet(const DestructibleAsset& asset)
{
	physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-

	for (physx::PxU32 i = mAssetCollisionSets.size(); i--;)
	{
		if (!mAssetCollisionSets[i] || !physx::string::stricmp(mAssetCollisionSets[i]->getAssetName(), asset.getName()))
		{
			PX_DELETE(mAssetCollisionSets[i]);
			mAssetCollisionSets.replaceWithLast(i);
		}
	}
}

physx::Array<NxConvexMesh*>* DestructibleModuleCachedData::getConvexMeshesForActor(const DestructibleActor& destructible)
{
	const DestructibleAsset* asset = destructible.getAsset();
	if (asset == NULL)
	{
		return NULL;
	}

	DestructibleAssetCollision* collisionSet = getAssetCollisionSetForActor(destructible);
	if (collisionSet == NULL)
	{
		return NULL;
	}

	return collisionSet->getConvexMeshesAtScale(destructible.getScale());
}

DestructibleAssetCollision* DestructibleModuleCachedData::getAssetCollisionSetForActor(const DestructibleActor& destructible)
{
	const DestructibleAsset* asset = destructible.getAsset();
	if (asset == NULL)
	{
		return NULL;
	}

	DestructibleAssetCollision* collisionSet = findAssetCollisionSet(asset->getName());

	if (collisionSet == NULL)
	{
		collisionSet = PX_NEW(DestructibleAssetCollision);
		physx::ScopedWriteLock scopedWriteLock( m_lock ); // ctremblay +-
		mAssetCollisionSets.pushBack(collisionSet);
	}
	collisionSet->setDestructibleAssetToCook(const_cast<DestructibleAsset*>(asset));

	return collisionSet;
}

DestructibleAssetCollision* DestructibleModuleCachedData::findAssetCollisionSet(const char* name)
{
	physx::ScopedReadLock scopedReadLock( m_lock ); // ctremblay +-

	for (physx::PxU32 i = 0; i < mAssetCollisionSets.size(); ++i)
	{
		if (mAssetCollisionSets[i] && !physx::string::stricmp(mAssetCollisionSets[i]->getAssetName(), name))
		{
			return mAssetCollisionSets[i];
		}
	}

	return NULL;
}

#endif

}
}
} // end namespace physx::apex
