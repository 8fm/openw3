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

#include "FieldBoundaryAssetPreview.h"
#include "FieldBoundaryAsset.h"
#include "NxApexRenderDebug.h"
#if 0
#include "NxParameterized.h"
#endif
#include "NxParamUtils.h"
#include "FieldBoundaryDrawer.h"

namespace physx
{
namespace apex
{
namespace fieldboundary
{

namespace
{
struct FieldBoundaryParamsIndex
{
	enum Index
	{
		kFieldBoundaryTypeIndex = 0,
		kUseAsExcludeShapeIndex,
		kUseLocalPoseAsWorldPoseIndex
	};
#if 0
	enum RefSphere
	{
		kSphereLocalPoseIndex = 0,
		kSphereScaleIndex,
		kSphereRadiusIndex
	};
	enum RefBox
	{
		kBoxLocalPoseIndex = 0,
		kBoxScaleIndex,
		kBoxDimensionIndex
	};
	enum RefCapsule
	{
		kCapsuleLocalPoseIndex = 0,
		kCapsuleScaleIndex,
		kCapsuleRadiusIndex,
		kCapsuleHeightIndex
	};
	enum RefConvex
	{
		kConvexLocalPoseIndex = 0,
		kConvexScaleIndex,
		kConvexPointsIndex
	};
#endif
};

class FieldBoundaryProperty
{
public:
	FieldBoundaryProperty(): mShape(FieldBoundaryProperty::eUndefined) {}
	~FieldBoundaryProperty() {}
	enum ShapeType {eUndefined, eSphere, eBox, eCapsule, eConvex};
	void setShape(FieldBoundaryProperty::ShapeType e)
	{
		this->mShape = e;
	}
	FieldBoundaryProperty::ShapeType getShape() const
	{
		return mShape;
	}
	ShapeSphereParamsNS::ParametersStruct* getSphere()
	{
		PX_ASSERT(FieldBoundaryProperty::eSphere == this->mShape);
		return FieldBoundaryProperty::eSphere == this->mShape ? &(this->uSphere) : NULL;
	}
	ShapeBoxParamsNS::ParametersStruct* getBox()
	{
		PX_ASSERT(FieldBoundaryProperty::eBox == this->mShape);
		return FieldBoundaryProperty::eBox == this->mShape ? &(this->uBox) : NULL;
	}
	ShapeCapsuleParamsNS::ParametersStruct* getCapsule()
	{
		PX_ASSERT(FieldBoundaryProperty::eCapsule == this->mShape);
		return FieldBoundaryProperty::eCapsule == this->mShape ? &(this->uCapsule) : NULL;
	}
	ShapeConvexParamsNS::ParametersStruct* getConvex()
	{
		PX_ASSERT(FieldBoundaryProperty::eConvex == this->mShape);
		return FieldBoundaryProperty::eConvex == this->mShape ? &(this->uConvex) : NULL;
	}
private:
	//union
	//{
	ShapeSphereParamsNS::ParametersStruct uSphere;
	ShapeBoxParamsNS::ParametersStruct uBox;
	ShapeCapsuleParamsNS::ParametersStruct uCapsule;
	ShapeConvexParamsNS::ParametersStruct uConvex;
	//};
	ShapeType mShape;
} currentFieldBoundary;

void paramOK(bool b)
{
	PX_UNUSED(b);
	PX_ASSERT(b);
}
void paramOK(NxParameterized::ErrorType e)
{
	PX_UNUSED(e);
	PX_ASSERT(NxParameterized::ERROR_NONE == e);
}
} //namespace nameless
FieldBoundaryAssetPreview::FieldBoundaryAssetPreview(NxApexSDK& sdk, FieldBoundaryAsset& asset, const FieldBoundaryPreviewParameters& params) :
	mApexRenderDebug(sdk.createApexRenderDebug(false, true)),
	mAsset(asset),
	mPreviewParams(params),
	mDrawIconID(0),
	mDrawIconBoldID(0)
{
	PX_ASSERT(mApexRenderDebug);
	mDrawBoundariesID.clear();
	onInit();
}

FieldBoundaryAssetPreview::~FieldBoundaryAssetPreview()
{
	mApexRenderDebug->release();
}

void FieldBoundaryAssetPreview::release()
{
	mAsset.releaseFieldBoundaryPreview(*this);
}

void FieldBoundaryAssetPreview::lockRenderResources()
{
	ApexRenderable::renderDataLock();
}

void FieldBoundaryAssetPreview::unlockRenderResources()
{
	ApexRenderable::renderDataUnLock();
}

void FieldBoundaryAssetPreview::updateRenderResources(bool, void*)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->updateRenderResources();
	}
}

void FieldBoundaryAssetPreview::dispatchRenderResources(NxUserRenderer& r)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->dispatchRenderResources(r);
	}
}

physx::PxBounds3 FieldBoundaryAssetPreview::getBounds() const
{
	PX_ASSERT(0);
	return physx::PxBounds3::empty();
}

void FieldBoundaryAssetPreview::setPose(const physx::PxMat44& p)
{
	PX_ASSERT(0);
	p.isFinite();
}

const physx::PxMat44 FieldBoundaryAssetPreview::getPose() const
{
	PX_ASSERT(0);
	return physx::PxMat44::createIdentity();
}

void FieldBoundaryAssetPreview::setDetailLevel(physx::PxU32 detailLevel) const
{
	using namespace APEX_FIELD_BOUNDARY;
	PX_ASSERT(detailLevel <= (FIELD_BOUNDARY_DRAW_ICON | FIELD_BOUNDARY_DRAW_ICON_BOLD | FIELD_BOUNDARY_DRAW_BOUNDARIES));
	mApexRenderDebug->setDrawGroupVisible(mDrawIconID, 0 != (FIELD_BOUNDARY_DRAW_ICON & detailLevel));
	mApexRenderDebug->setDrawGroupVisible(mDrawIconBoldID, 0 != (FIELD_BOUNDARY_DRAW_ICON_BOLD & detailLevel));
	for (physx::Array<physx::PxI32>::ConstIterator iter = mDrawBoundariesID.begin(); iter != mDrawBoundariesID.end(); ++iter)
	{
		mApexRenderDebug->setDrawGroupVisible(*iter, 0 != (FIELD_BOUNDARY_DRAW_BOUNDARIES & detailLevel));
	}
}

void FieldBoundaryAssetPreview::destroy()
{
	ApexPreview::destroy();
	PX_DELETE(this);
}

void FieldBoundaryAssetPreview::onInit()
{
	PX_ASSERT(0 == (mDrawIconID | mDrawIconBoldID | mDrawBoundariesID.size()));
	{
		const physx::PxF32 w = 5.0f;
		const physx::PxF32 h = 2.0f;
		const physx::PxVec3 p[] = {physx::PxVec3(w, h, w), physx::PxVec3(-w, h, w), physx::PxVec3(-w, h, -w), physx::PxVec3(w, h, -w)};

		if (0 == mDrawIconID)
		{
			mDrawIconID = mApexRenderDebug->beginDrawGroup(mPreviewParams.previewPose);
			for (physx::PxU32 i = 0; i < 4; ++i)
			{
				mApexRenderDebug->debugLine(p[i], p[(i + 1) % 4]);
			}
			mApexRenderDebug->setCurrentTextScale(2.0f);
			//physx::PxMat44 textPose = physx::PxMat44(physx::PxVec3(1,0,0), physx::PxVec3(0,0,-1), physx::PxVec3(0,1,0), physx::PxVec3(-4.8,h,0)); //lionel: to face up
			physx::PxMat44 textPose = physx::PxMat44(physx::PxVec3(1, 0, 0), physx::PxVec3(0, 1, 0), physx::PxVec3(0, 0, 1), physx::PxVec3(-4.8, h, 0));
			mApexRenderDebug->debugOrientedText(textPose, "Unimaginative Field Boundary Icon!");
			mApexRenderDebug->endDrawGroup();
		}
		if (0 == mDrawIconBoldID)
		{
			mDrawIconBoldID = mApexRenderDebug->beginDrawGroup(mPreviewParams.previewPose);
			mApexRenderDebug->addToCurrentState(DebugRenderState::SolidShaded);
			for (physx::PxU32 i = 0; i < 4; ++i)
			{
				mApexRenderDebug->debugCylinder(p[i], p[(i + 1) % 4], 0.1f);
			}
			mApexRenderDebug->removeFromCurrentState(DebugRenderState::SolidShaded);
			mApexRenderDebug->endDrawGroup();
		}
	}
	if (0 == mDrawBoundariesID.size())
	{
		physx::PxI32 numBoundaries = mPreviewParams.fieldBoundaryContainer.arraySizes[0];
		PX_ASSERT(numBoundaries > 0);
		NxParameterized::Handle objHandle(mPreviewParams);
		NxParameterized::Handle objMemberHandle(mPreviewParams);
		char candidateString[128];

		for (physx::PxI32 arrayIndex = 0; arrayIndex < numBoundaries; ++arrayIndex)
		{
			APEX_SPRINTF_S(candidateString, PX_ARRAY_SIZE(candidateString), "fieldBoundaryContainer[%d]", arrayIndex);
			paramOK(mPreviewParams.getParameterHandle(candidateString, objHandle));
			PX_ASSERT(objHandle.isValid());

			paramOK(objHandle.getChildHandle(FieldBoundaryParamsIndex::kUseAsExcludeShapeIndex, objMemberHandle));
			PX_ASSERT(objMemberHandle.isValid());
			paramOK(objMemberHandle.getParamBool(mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].useAsExcludeShape));
			objMemberHandle.reset();

			paramOK(objHandle.getChildHandle(FieldBoundaryParamsIndex::kUseLocalPoseAsWorldPoseIndex, objMemberHandle));
			PX_ASSERT(objMemberHandle.isValid());
			paramOK(objMemberHandle.getParamBool(mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].useLocalPoseAsWorldPose));
			objMemberHandle.reset();

			objHandle.reset();

			APEX_SPRINTF_S(candidateString, PX_ARRAY_SIZE(candidateString), "fieldBoundaryContainer[%d].fieldBoundaryType", arrayIndex);
			paramOK(mPreviewParams.getParameterHandle(candidateString, objHandle));
			PX_ASSERT(objHandle.isValid());
			paramOK(objHandle.getParamRef(mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType));
			PX_ASSERT(mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType != NULL);
			objHandle.reset();

			physx::PxMat44 adoptedPose = physx::PxMat44::createIdentity();
			APEX_SPRINTF_S(candidateString, PX_ARRAY_SIZE(candidateString), mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType->className());
			if (strcmp(candidateString, ShapeSphereParams::staticClassName()) == 0)
			{
				currentFieldBoundary.setShape(FieldBoundaryProperty::eSphere);
				paramOK(NxParameterized::getParamMat44(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "localPose", currentFieldBoundary.getSphere()->localPose));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "scale", currentFieldBoundary.getSphere()->scale));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "radius", currentFieldBoundary.getSphere()->radius));
				adoptedPose = currentFieldBoundary.getSphere()->localPose;
			}
			else if (strcmp(candidateString, ShapeBoxParams::staticClassName()) == 0)
			{
				currentFieldBoundary.setShape(FieldBoundaryProperty::eBox);
				paramOK(NxParameterized::getParamMat44(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "localPose", currentFieldBoundary.getBox()->localPose));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "scale", currentFieldBoundary.getBox()->scale));
				paramOK(NxParameterized::getParamVec3(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "dimensions", currentFieldBoundary.getBox()->dimensions));
				adoptedPose = currentFieldBoundary.getBox()->localPose;
			}
			else if (strcmp(candidateString, ShapeCapsuleParams::staticClassName()) == 0)
			{
				currentFieldBoundary.setShape(FieldBoundaryProperty::eCapsule);
				paramOK(NxParameterized::getParamMat44(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "localPose", currentFieldBoundary.getCapsule()->localPose));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "scale", currentFieldBoundary.getCapsule()->scale));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "radius", currentFieldBoundary.getCapsule()->radius));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "height", currentFieldBoundary.getCapsule()->height));
				adoptedPose = currentFieldBoundary.getCapsule()->localPose;
			}
			else if (strcmp(candidateString, ShapeConvexParams::staticClassName()) == 0)
			{
				currentFieldBoundary.setShape(FieldBoundaryProperty::eConvex);
				paramOK(NxParameterized::getParamMat44(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "localPose", currentFieldBoundary.getConvex()->localPose));
				paramOK(NxParameterized::getParamF32(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "scale", currentFieldBoundary.getConvex()->scale));
				paramOK(NxParameterized::getParamArraySize(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType, "points", currentFieldBoundary.getConvex()->points.arraySizes[0]));
				NxParameterized::Handle paramRefMemberHandle(*mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType);
				paramOK(mPreviewParams.fieldBoundaryContainer.buf[arrayIndex].fieldBoundaryType->getParameterHandle("points", paramRefMemberHandle));
				PX_ASSERT(paramRefMemberHandle.isValid());
				currentFieldBoundary.getConvex()->points.buf = ::new physx::PxVec3[currentFieldBoundary.getConvex()->points.arraySizes[0]];
				paramOK(paramRefMemberHandle.getParamVec3Array(currentFieldBoundary.getConvex()->points.buf, currentFieldBoundary.getConvex()->points.arraySizes[0], 0));
				paramRefMemberHandle.reset();
				adoptedPose = currentFieldBoundary.getConvex()->localPose;
			}
			else
			{
				currentFieldBoundary.setShape(FieldBoundaryProperty::eUndefined);
				PX_ASSERT(0);
			}

			if (!mPreviewParams.fieldBoundaryContainer.buf->useLocalPoseAsWorldPose)
			{
				adoptedPose.column3 += mPreviewParams.previewPose.column3;
				adoptedPose.column3.w -= 1;

				//lionel: todo: process matrix to be relative to previewPose. tricky for orientation!
				//rotate about a different reference axes == axes of localPose
				//use quarternions about previewPose's right, up, out. Ensure normalized first!
				//if using general rotation matrix, ensure translation == 0 (double check your theory if this is necessary)
			}

			mDrawBoundariesID.pushBack(mApexRenderDebug->beginDrawGroup(adoptedPose));
			mApexRenderDebug->pushRenderState();
			mApexRenderDebug->setCurrentColor(mPreviewParams.fieldBoundaryContainer.buf->useAsExcludeShape ? mApexRenderDebug->getDebugColor(physx::DebugColors::Red) : mApexRenderDebug->getDebugColor(physx::DebugColors::Green));
			switch (currentFieldBoundary.getShape())
			{
			case FieldBoundaryProperty::eSphere :
				mApexRenderDebug->setDrawGroupPose(*(mDrawBoundariesID.end() - 1), physx::PxMat44::createIdentity());
				FieldBoundaryDrawer::drawFieldBoundarySphere(*mApexRenderDebug, adoptedPose.getPosition(), currentFieldBoundary.getSphere()->radius, 16);
				break;
			case FieldBoundaryProperty::eBox :
				mApexRenderDebug->setDrawGroupPose(*(mDrawBoundariesID.end() - 1), physx::PxMat44::createIdentity());
				FieldBoundaryDrawer::drawFieldBoundaryBox(*mApexRenderDebug, adoptedPose.getPosition() - currentFieldBoundary.getBox()->dimensions, adoptedPose.getPosition() + currentFieldBoundary.getBox()->dimensions);
				break;
			case FieldBoundaryProperty::eCapsule :
				FieldBoundaryDrawer::drawFieldBoundaryCapsule(*mApexRenderDebug, currentFieldBoundary.getCapsule()->radius, currentFieldBoundary.getCapsule()->height, 2, adoptedPose);
				break;
			case FieldBoundaryProperty::eConvex :
				mApexRenderDebug->setDrawGroupPose(*(mDrawBoundariesID.end() - 1), physx::PxMat44::createIdentity());
				{
					PX_ASSERT(currentFieldBoundary.getConvex()->points.arraySizes[0] == 4); //lionel: hack
					physx::PxU32 triVertexIndices[] = {0, 1, 2, 1, 0, 3, 3, 2, 1, 2, 3, 0};
					FieldBoundaryDrawer::drawFieldBoundaryConvex(*mApexRenderDebug, adoptedPose.getPosition(), 4, 4, 12, 12, static_cast<void*>(currentFieldBoundary.getConvex()->points.buf), static_cast<void*>(triVertexIndices));
				}
				::delete []currentFieldBoundary.getConvex()->points.buf;
				currentFieldBoundary.getConvex()->points.buf = 0;
				break;
			default:
				PX_ASSERT(0);
				break;
			}
			mApexRenderDebug->popRenderState();
			mApexRenderDebug->endDrawGroup();
		}
	}
	PX_ASSERT(mDrawIconID && mDrawIconBoldID && mDrawBoundariesID.size());
	using namespace APEX_FIELD_BOUNDARY;
	setDetailLevel(0);
	//setDetailLevel(FIELD_BOUNDARY_DRAW_ICON | FIELD_BOUNDARY_DRAW_ICON_BOLD | FIELD_BOUNDARY_DRAW_BOUNDARIES);
}

} //namespace fieldboundary
} //namespace apex
} //namespace physx

#endif