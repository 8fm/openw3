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

#ifndef __FIELDBOUNDARY_ASSET_H__
#define __FIELDBOUNDARY_ASSET_H__

#include "NxApex.h"
#include "PsShare.h"
#include "NxFieldBoundaryAsset.h"
#include "ApexSDKHelpers.h"
#include "ApexInterface.h"
#include "ApexString.h"
#include "ModuleFieldBoundary.h"
#include "ApexAuthorableObject.h"
#include "ApexAssetAuthoring.h"
#include "FieldBoundaryAssetParameters.h"
#include "FieldBoundaryActorParameters.h"
#include "ShapeBoxParams.h"
#include "ShapeCapsuleParams.h"
#include "ShapeConvexParams.h"
#include "ShapeSphereParams.h"

namespace physx
{
namespace apex
{

class NxFieldBoundaryPreview;

namespace fieldboundary
{

class FieldBoundaryActor;

class ffShapeBox : public physx::UserAllocated
{
public:
	physx::PxMat34Legacy mPose;			//pose of the shape
	physx::PxVec3  mDimensions;	//dimensions of the box.
	physx::PxF32	mScale;			//scale of the box

	ffShapeBox(NxParameterized::Interface* params)
	{

		ShapeBoxParams* myShapeBoxParams = (ShapeBoxParams*) params;
		mPose		= myShapeBoxParams->localPose;
		mDimensions	= myShapeBoxParams->dimensions;
		mScale		= myShapeBoxParams->scale;
	}
};
class ffShapeSphere : public physx::UserAllocated
{
public:
	physx::PxMat34Legacy mPose;		//pose of the shape
	physx::PxF32  mRadius;	//radius of the sphere or capsule.
	physx::PxF32	mScale;		//scale of the sphere

	ffShapeSphere(NxParameterized::Interface* params)
	{

		ShapeSphereParams* myShapeSphereParams = (ShapeSphereParams*) params;
		mPose	= myShapeSphereParams->localPose;
		mRadius	= myShapeSphereParams->radius;
		mScale	= myShapeSphereParams->scale;
	}
};

class ffShapeCapsule : public physx::UserAllocated
{
public:
	physx::PxMat34Legacy mPose;		//pose of the shape
	physx::PxF32  mRadius;	//radius of the sphere or capsule.
	physx::PxF32  mHeight;	//height of the capsule
	physx::PxF32	mScale;		//scale of the capsule

	ffShapeCapsule(NxParameterized::Interface* params)
	{

		ShapeCapsuleParams* myShapeCapsuleParams = (ShapeCapsuleParams*) params;
		mPose	= myShapeCapsuleParams->localPose;
		mRadius	= myShapeCapsuleParams->radius;
		mHeight	= myShapeCapsuleParams->height;
		mScale	= myShapeCapsuleParams->scale;
	}
};

class ffShapeConvex : public physx::UserAllocated
{
public:
	physx::PxMat34Legacy				mPose;		//pose of the shape
	physx::PxF32				mScale;		//scale of the capsule
	physx::Array<physx::PxVec3>	mPoints;	// the points in the mesh

	ffShapeConvex(NxParameterized::Interface* params)
	{

		ShapeConvexParams* myShapeConvexParams = (ShapeConvexParams*) params;
		mPose	= myShapeConvexParams->localPose;
		mScale	= myShapeConvexParams->scale;
		PxU32 num = myShapeConvexParams->points.arraySizes[0];
		for (PxU32 i = 0; i < num; i++)
		{
			physx::PxVec3 myPoint = myShapeConvexParams->points.buf[i];
			mPoints.pushBack(myPoint);
		}
	}
};

struct ConvexMeshDesc : physx::UserAllocated
{
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ConvexMeshDesc()
	{
		setToDefault();
	}

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault()
	{
		flags = 0;
		numPoints = 0;
		pointStrideBytes = sizeof(physx::PxVec3);
		points.clear();
	}

	Array<PxVec3> points;
	PxU32 flags;
	PxU32 numPoints;
	PxU32 pointStrideBytes;
};

class FieldShapeDesc : public NxApexDesc
{
public:
	/**
	\brief constructor sets to default.
	*/
	PX_INLINE FieldShapeDesc()
	{
		setToDefault();
	}

	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault()
	{
		type = NX_APEX_SHAPE_SPHERE;
		pose.id();
		radius = 1.0f;
		dimensions = physx::PxVec3(1.0f);
		height = 1.0f;
		mesh = NULL;
	}

	/**
	Returns true if an object can be created using this descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		if (type == NX_APEX_SHAPE_SPHERE)
		{
			if (!physx::PxIsFinite(radius))
			{
				return false;
			}
			if (radius <= 0.0f)
			{
				return false;
			}
		}
		else if (type == NX_APEX_SHAPE_BOX)
		{
			if (!dimensions.isFinite())
			{
				return false;
			}
			if (dimensions.x < 0.0f)
			{
				return false;
			}
			if (dimensions.y < 0.0f)
			{
				return false;
			}
			if (dimensions.z < 0.0f)
			{
				return false;
			}
		}
		else if (type == NX_APEX_SHAPE_CAPSULE)
		{
			if (!physx::PxIsFinite(radius))
			{
				return false;
			}
			if (radius <= 0.0f)
			{
				return false;
			}
			if (!physx::PxIsFinite(height))
			{
				return false;
			}
			if (height <= 0.0f)
			{
				return false;
			}
		}
		else if (type == NX_APEX_SHAPE_CONVEX)
		{
			if (!mesh)
			{
				return false;
			}
		}
		else
		{
			return false;	//invalid force field shape type
		}

		return NxApexDesc::isValid();
	}

public:
	NxApexFieldShapeType type;
	physx::PxMat34Legacy pose;				//pose of the shape

	physx::PxF32 radius;				//radius of the sphere or capsule.
	physx::PxVec3 dimensions;			//dimensions of the box.
	physx::PxF32 height;				//height of the capsule
	ConvexMeshDesc* mesh;		//references the convex mesh
};

/**
\brief Descriptor used to create a field boundary actor.
*/
class NxFieldBoundaryActorDesc : public NxApexDesc
{
public:
	/**
	\brief The initial relative pose of the actor.
	*/
	physx::PxMat44	initialPose;
	/**
	\brief The scale factor to increase or decrease the actors size.
	*/
	physx::PxVec3	scale;

	ApexSimpleString	boundaryGroupMaskName;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE NxFieldBoundaryActorDesc() : NxApexDesc()
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

	/**
	\brief function to initialize the field boundary actor descriptor.
	*/
	PX_INLINE void init()
	{
		initialPose = PxMat44::createIdentity();
		scale = physx::PxVec3(1.0f);
	}
};

class FieldBoundaryPreviewParameters;

class FieldBoundaryAsset : public NxFieldBoundaryAsset, public NxApexResource, public ApexResource, public ApexAssetAuthoring
{
	friend class FieldBoundaryAssetDummyAuthoring;
public:
	FieldBoundaryAsset(ModuleFieldBoundary*, NxResourceList&, const char*);
	FieldBoundaryAsset(ModuleFieldBoundary* module, NxResourceList& list, NxParameterized::Interface* params,
	                   const char* name);
	void initAllMembers(void);
	void loadAllMembersFromParams(const FieldBoundaryAssetParameters* params);

	~FieldBoundaryAsset();
	PxU32 forceLoadAssets();
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

	/* NxApexAsset */
	const char* 			getName() const
	{
		return mName.c_str();
	}
	NxAuthObjTypeID			getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 			getObjTypeName() const
	{
		return getClassName();
	}

	/* NxApexInterface */
	virtual void			release()
	{
		mModule->mSdk->releaseAsset(*this);
	}

	/* NxApexResource, ApexResource */
	physx::PxU32			getListIndex() const
	{
		return m_listIndex;
	}
	void					setListIndex(class NxResourceList& list, physx::PxU32 index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* NxFieldBoundaryAsset specific methods */
	NxFieldBoundaryActor* 	createFieldBoundaryActor(const NxFieldBoundaryActorDesc&, const NxApexScene&);
	physx::PxVec3			getDefaultScale() const
	{
		return mDefScale;
	}
	void                    destroy();

	NxFieldBoundaryActor*	getActor(physx::PxU32 index) const ;
	physx::PxU32			getActorCount() const
	{
		return mFieldBoundaryActors.getSize();
	}

	NxParameterized::Interface* getDefaultActorDesc();
	NxParameterized::Interface* getDefaultAssetPreviewDesc();

	virtual NxApexActor* createApexActor(const NxParameterized::Interface& parms, NxApexScene& apexScene);
	virtual NxApexAssetPreview* createApexAssetPreview(const NxParameterized::Interface& params, NxApexAssetPreviewScene* previewScene);
	void releaseFieldBoundaryPreview(NxFieldBoundaryPreview&);

	virtual bool isValidForActorCreation(const ::NxParameterized::Interface& /*parms*/, NxApexScene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	static const char* 		getClassName()
	{
		return NX_FIELD_BOUNDARY_AUTHORING_TYPE_NAME;
	}
	static NxAuthObjTypeID	mAssetTypeID;
	ModuleFieldBoundary*       mModule;

	Array<FieldShapeDesc>	mShapes;
	Array<ConvexMeshDesc>	mConvex;
	NxResourceList          mFieldBoundaryActors;

	ApexSimpleString        mName;
	NxResID					mAssetResID;
	NxApexFieldBoundaryFlag mFlag;
	physx::PxVec3					mDefScale;
	FieldBoundaryAssetParameters*	mParams;
	FieldBoundaryActorParameters*	mDefaultActorParams;
	FieldBoundaryPreviewParameters*	mDefaultPreviewParams;

	friend class ModuleFieldBoundary;
	friend class FieldBoundaryActor;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class FieldBoundaryAssetAuthoring : public FieldBoundaryAsset, public NxApexAssetAuthoring
{
public:
	/* NxFieldBoundaryAssetAuthoring */
	FieldBoundaryAssetAuthoring(ModuleFieldBoundary* m, NxResourceList& l) :
		FieldBoundaryAsset(m, l, "FieldBoundaryAssetAuthoring") {}

	FieldBoundaryAssetAuthoring(ModuleFieldBoundary* m, NxResourceList& l, const char* name) :
		FieldBoundaryAsset(m, l, name) {}

	FieldBoundaryAssetAuthoring(ModuleFieldBoundary* m, NxResourceList& l, NxParameterized::Interface* params, const char* name) :
		FieldBoundaryAsset(m, l, params, name) {}

	~FieldBoundaryAssetAuthoring() {}

	void					setDefaultScale(const physx::PxVec3& s)
	{
		mDefScale = s;
	}
	void					setBoundaryFlag(NxApexFieldBoundaryFlag flag)
	{
		mFlag = flag;
	}
	bool					addSphere(const NxApexSphereFieldShapeDesc& desc);
	bool					addBox(const NxApexBoxFieldShapeDesc& desc);
	bool					addCapsule(const NxApexCapsuleFieldShapeDesc& desc);
	bool					addConvex(const NxApexConvexFieldShapeDesc& desc);

	void                    destroy()
	{
		delete this;
	}

	/* NxApexAssetAuthoring */
	const char* 					getName(void) const
	{
		return FieldBoundaryAsset::getName();
	}
	const char* 			getObjTypeName() const
	{
		return FieldBoundaryAsset::getClassName();
	}
	virtual bool					prepareForPlatform(physx::apex::NxPlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void setToolString(const char* toolName, const char* toolVersion, PxU32 toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}
	NxParameterized::Interface* getNxParameterized() const
	{
		return mParams;
	}

	/* NxApexInterface */
	virtual void			release()
	{
		mModule->mSdk->releaseAssetAuthoring(*this);
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
