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

#ifndef __EXPLOSION_ASSET_H__
#define __EXPLOSION_ASSET_H__

#include "NxApex.h"
#include "PsShare.h"
#include "NxExplosionAsset.h"
#include "NxExplosionActor.h"
#include "NxExplosionPreview.h"
#include "NxFieldBoundaryAsset.h"
#include "ApexSDKHelpers.h"
#include "ApexInterface.h"
#include "ModuleExplosion.h"
#include "ApexString.h"
#include "ApexAssetTracker.h"
#include "ApexAssetAuthoring.h"
#include "ApexAuthorableObject.h"
#include "ExplosionAssetParam.h"

namespace physx
{
namespace apex
{
namespace explosion 
{


struct NxExplosionEnvSettings
{
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxExplosionEnvSettings()
	{
		setToDefault();
	}

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault()
	{
#if NX_SDK_VERSION_MAJOR == 2
		group				= 0;
		groupsMask.bits0	= 0;
		groupsMask.bits1	= 0;
		groupsMask.bits2	= 0;
		groupsMask.bits3	= 0;
#endif

		fluidType			= NX_APEX_FF_TYPE_OTHER;
		clothType			= NX_APEX_FF_TYPE_OTHER;
		softBodyType		= NX_APEX_FF_TYPE_OTHER;
		rigidBodyType		= NX_APEX_FF_TYPE_OTHER;
		flags				= 0;
	}

#if NX_SDK_VERSION_MAJOR == 2
	NxCollisionGroup			group;					// Collision group used for collision filtering.
	NxGroupsMask				groupsMask;				// Groups mask used for collision filtering.
#endif
	NxApexForceFieldType		fluidType;				// Force field type for fluids
	NxApexForceFieldType		clothType;				// Force field type for cloth
	NxApexForceFieldType		softBodyType;			// Force field type for soft bodies
	NxApexForceFieldType		rigidBodyType;			// Force field type for rigid bodies
	physx::PxU32				flags;					// Force field flags; @see NxApexForceFieldFlags
};

class NxExplosionActorDesc : public NxApexDesc
{
public:
	physx::PxMat44 initialPose;
	physx::PxF32	 scale;
#if NX_SDK_VERSION_MAJOR == 2
	NxActor* nxActor;
#elif NX_SDK_VERSION_MAJOR == 3
	PxActor* nxActor;
#endif
	const char* actorName;
	ExplosionAssetParam* eParameter;
	NxExplosionEnvSettings	envSetting;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxExplosionActorDesc() : NxApexDesc()
	{
		init();
	}

	/**
	\brief sets members to default values.
	*/
	PX_INLINE void setToDefault()
	{
		NxApexDesc::setToDefault();
		init();
	}

	/**
	\brief checks if this is a valid descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		if (!NxApexDesc::isValid())
		{
			return false;
		}

		return true;
	}

private:
	PX_INLINE void init()
	{
		initialPose = PxMat44::createIdentity();
		scale = 1.0f;
		nxActor = NULL;
		actorName = NULL;
		eParameter = NULL;
		envSetting.setToDefault();
	}
};

/**
\brief Descriptor for a Explosion Asset
*/
class NxExplosionPreviewDesc
{
public:
	NxExplosionPreviewDesc() :
		mPose(physx::PxMat44()),
		mIconScale(1.0f),
		mPreviewDetail(APEX_EXPLOSION::EXPLOSION_DRAW_ICON)
	{
		mPose = PxMat44::createIdentity();
	};

	/**
	\brief The pose that translates from explosion preview coordinates to world coordinates.
	*/
	physx::PxMat44							mPose;
	/**
	\brief The scale of the icon.
	*/
	physx::PxF32							mIconScale;
	/**
	\brief The detail options of the preview drawing
	*/
	physx::PxU32							mPreviewDetail;
};


class ExplosionActor;

//
// There are two ugly macros defined here so that both the Impact and Explosion modules
// may use these "functions".  I couldn't come up with a better way, PX_INLINE didn't work.
//

// params MUST be an ExplosionEnvParameters
#define NxParamToFFType( type, params, name )						\
	{																	\
		NxParameterized::Handle h(*params), c(*params);									\
		const char *enumStr=0;											\
		\
		params->getParameterHandle( name, h );							\
		PX_ASSERT( h.isValid() );										\
		h.getChildHandle( params, "interaction", c );					\
		PX_ASSERT( c.isValid() );										\
		params->getParamEnum( c, enumStr );								\
		if( !strcmp( enumStr, "other" ) )								\
		{																\
			type = NX_APEX_FF_TYPE_OTHER;								\
		}																\
		else if( !strcmp( enumStr, "gravitational" ) )					\
		{																\
			type = NX_APEX_FF_TYPE_GRAVITATIONAL;						\
		}																\
		else if( !strcmp( enumStr, "none" ) )							\
		{																\
			type = NX_APEX_FF_TYPE_NO_INTERACTION;						\
		}																\
		else															\
		{																\
			PX_ASSERT( 0 && "Invalid FF interaction type" );			\
			type = NX_APEX_FF_TYPE_OTHER;								\
		}																\
	}

// input params MUST be an ExplosionEnvParameters
#define NxParamToExplEnvSettings( desc, params )													\
	{																									\
		PX_ASSERT( params );																			\
		const char *className = params->className();													\
		if( strcmp(className, ExplosionEnvParameters::staticClassName() ) == 0 )						\
		{																								\
			const ExplosionEnvParameters* tmpParam = static_cast<const ExplosionEnvParameters*>(params);\
			/* Resolve the authored collision group mask name into the actual ID */						\
			if( tmpParam->collisionGroupName != NULL &&	tmpParam->collisionGroupName[0] != 0 )			\
			{																							\
				NiResourceProvider *nrp = NiGetApexSDK()->getInternalResourceProvider();				\
				NxResID cgmns = NiGetApexSDK()->getCollisionGroupNameSpace();							\
				NxResID cgresid = nrp->createResource( cgmns, tmpParam->collisionGroupName );			\
				desc.group = (NxCollisionGroup) (size_t) nrp->getResource( cgresid );					\
			}																							\
			\
			/* Resolve the authored collision group name into the actual ID */							\
			if( tmpParam->collisionGroupsMask128Name != NULL &&											\
			        tmpParam->collisionGroupsMask128Name[0] != 0 )											\
			{																							\
				NxGroupsMask *raycastGroupsMask = 0;													\
				NiResourceProvider *nrp = NiGetApexSDK()->getInternalResourceProvider();				\
				NxResID cgmns = NiGetApexSDK()->getCollisionGroupNameSpace();							\
				NxResID cgresid = nrp->createResource( cgmns, tmpParam->collisionGroupsMask128Name );	\
				raycastGroupsMask = static_cast<NxGroupsMask*>( nrp->getResource( cgresid ) );			\
				\
				if( raycastGroupsMask )																	\
				{																						\
					desc.groupsMask = *raycastGroupsMask;												\
				}																						\
			}																							\
			\
			NxParamToFFType( desc.fluidType, tmpParam, "fluidInteraction" );							\
			NxParamToFFType( desc.clothType, tmpParam, "clothInteraction" );							\
			NxParamToFFType( desc.softBodyType, tmpParam, "softBodyInteraction" );						\
			NxParamToFFType( desc.rigidBodyType, tmpParam, "rigidBodyInteraction" );					\
		}																								\
	}


class ExplosionAsset : public NxExplosionAsset, public NxApexResource, public ApexResource, public ApexAssetAuthoring
{
	friend class ExplosionAssetDummyAuthoring;
public:
	ExplosionAsset(ModuleExplosion*, NxResourceList&, const char* name);
	ExplosionAsset(ModuleExplosion*, NxResourceList&, NxParameterized::Interface*, const char*);

	~ExplosionAsset();

	/* NxApexAsset */
	const char* 					getName() const
	{
		return mName.c_str();
	}
	NxAuthObjTypeID					getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 					getObjTypeName() const
	{
		return getClassName();
	}
	physx::PxU32							forceLoadAssets();

	/* NxApexInterface */
	virtual void					release()
	{
		mModule->mSdk->releaseAsset(*this);
	}

	/* NxApexResource, ApexResource */
	physx::PxU32							getListIndex() const
	{
		return m_listIndex;
	}
	void							setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxExplosionAsset specific methods */
	void							releaseExplosionActor(NxExplosionActor&);
	const ExplosionAssetParam& 		getExplosionParameters() const
	{
		return *mParams;
	}
	physx::PxF32					getDefaultScale() const
	{
		return mParams->defScale;
	}
	physx::PxU32					getFieldBoundariesCount() const;
	const char*						getFieldBoundariesName(physx::PxU32 boundIndex);
	void							destroy();
	NxExplosionPreview*				createExplosionPreview(const NxExplosionPreviewDesc& desc);
	NxExplosionPreview*				createExplosionPreviewImpl(const NxExplosionPreviewDesc& desc, ExplosionAsset* ExplosionAsset);
	void							releaseExplosionPreview(NxExplosionPreview& preview);

	const NxParameterized::Interface* getAssetNxParameterized() const
	{
		return mParams;
	}
	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
	NxParameterized::Interface* getDefaultActorDesc();
	NxParameterized::Interface* getDefaultAssetPreviewDesc();
	virtual NxApexActor* createApexActor(const NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/);
	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* previewScene);

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	static const char* 				getClassName()
	{
		return NX_EXPLOSION_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID			mAssetTypeID;

	ModuleExplosion*				mModule;

	ApexAssetTracker				mFieldBoundaryAssetTracker;

	NxResourceList					mExplosionActors;
	ApexSimpleString				mName;
	ExplosionAssetParam*				mParams;
	ExplosionActorParameters*		mDefaultActorParams;
	ExplosionAssetPreviewParameters*	mDefaultPreviewParams;
	NxParameterized::Handle			mBoundariesNamesParamHandle;

	void							initializeAssetNameTable();

	friend class ModuleExplosion;
	friend class ExplosionActor;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class ExplosionAssetAuthoring : public ExplosionAsset, public NxExplosionAssetAuthoring
{
public:
	/* NxExplosionAssetAuthoring */
	ExplosionAssetAuthoring(ModuleExplosion* m, NxResourceList& l) :
		ExplosionAsset(m, l, "ExplosionAssetAuthoring") {}

	ExplosionAssetAuthoring(ModuleExplosion* m, NxResourceList& l, const char* name) :
		ExplosionAsset(m, l, name) {}

	ExplosionAssetAuthoring(ModuleExplosion* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		ExplosionAsset(m, l, params, name) {}

	~ExplosionAssetAuthoring() {}

	void							addFieldBoundaryName(const char* n);

	void							destroy()
	{
		ExplosionAsset::destroy();
		//delete this;
	}

	/* NxApexAssetAuthoring */
	const char* 					getName(void) const
	{
		return ExplosionAsset::getName();
	}
	const char* 					getObjTypeName() const
	{
		return ExplosionAsset::getClassName();
	}
	virtual bool					prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}

	/* NxApexInterface */
	virtual void					release()
	{
		mModule->mSdk->releaseAssetAuthoring(*this);
	}

	void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	NxParameterized::Interface* getNxParameterized() const
	{
		return (NxParameterized::Interface*)getAssetNxParameterized();
	}
	/**
	 * \brief Releases the ApexAsset but returns the NxParameterized::Interface and *ownership* to the caller.
	 */
	virtual NxParameterized::Interface* releaseAndReturnNxParameterizedInterface(void)
	{
		NxParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
};
#endif

}
}
} // end namespace physx::apex

#endif
