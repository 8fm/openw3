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

#include "NxApex.h"
#include "PsShare.h"
#include "IofxAsset.h"
#include "IofxActor.h"
#include "IofxScene.h"
#include "ModuleIofx.h"
#include "NxModifier.h"
#include "Modifier.h"
#include "NiApexScene.h"
#include "NiIofxManager.h"
//#include "ApexSharedSerialization.h"
#include "CurveImpl.h"

namespace physx
{
namespace apex
{
namespace iofx
{


IofxAsset::IofxAsset(ModuleIofx* module, NxResourceList& list, const char* name) :
	mRenderMeshAssetTracker(module->mSdk, NX_RENDER_MESH_AUTHORING_TYPE_NAME),
	mSpriteMaterialAssetTracker(module->mSdk),
	mModule(module),
	mName(name),
	mSpriteSemanticBitmap(0),
	mMeshSemanticBitmap(0)
{
	using namespace IofxAssetParametersNS;

	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();
	mParams = (IofxAssetParameters*)traits->createNxParameterized(IofxAssetParameters::staticClassName());

	PX_ASSERT(mParams);

	// LRR: Hmmmm, these will never be setup for authors, it's a good thing we can't create actors from authors!
	mRenderMeshList = NULL;
	mSpriteParams = NULL;

	list.add(*this);
}

IofxAsset::IofxAsset(ModuleIofx* module,
                     NxResourceList& list,
                     NxParameterized::Interface* params,
                     const char* name) :
	mRenderMeshAssetTracker(module->mSdk, NX_RENDER_MESH_AUTHORING_TYPE_NAME),
	mSpriteMaterialAssetTracker(module->mSdk),
	mParams((IofxAssetParameters*)params),
	mSpriteParams(NULL),
	mModule(module),
	mName(name),
	mSpriteSemanticBitmap(0),
	mMeshSemanticBitmap(0)
{
	using namespace MeshIofxParametersNS;

	if (mParams->iofxType &&
	        !strcmp(mParams->iofxType->className(), MeshIofxParameters::staticClassName()))
	{
		MeshIofxParameters* mIofxParams = static_cast<MeshIofxParameters*>(mParams->iofxType);

		NxParamDynamicArrayStruct* tmpStructPtr = (NxParamDynamicArrayStruct*) & (mIofxParams->renderMeshList);
		mRenderMeshList = PX_NEW(NxParamArray<meshProperties_Type>)(mIofxParams, "renderMeshList", tmpStructPtr);
	}
	else
	{
		mRenderMeshList = NULL;
	}

	if (mParams->iofxType &&
	        !strcmp(mParams->iofxType->className(), SpriteIofxParameters::staticClassName()))
	{
		mSpriteParams = static_cast<SpriteIofxParameters*>(mParams->iofxType);
	}


	// call this now to "initialize" the asset
	postDeserialize();

	list.add(*this);
}

IofxAsset::~IofxAsset()
{
	for (physx::PxU32 i = mSpawnModifierStack.size(); i--;)
	{
		delete mSpawnModifierStack[i];
	}

	for (physx::PxU32 i = mContinuousModifierStack.size(); i--;)
	{
		delete mContinuousModifierStack[i];
	}
}

void IofxAsset::destroy()
{
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */
	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	if (mRenderMeshList)
	{
		PX_DELETE(mRenderMeshList);
	}

#if IOFX_SLOW_COMPOSITE_MODIFIERS
	// destroy any color vs life params that we may have laying around
	for (physx::PxU32 i = 0; i < mCompositeParams.size(); i++)
	{
		mCompositeParams[i]->destroy();
	}
	mCompositeParams.clear();
#endif

	// this will release all of this asset's IOFX actors, which is necessary, otherwise
	// the IOFX actors will have their APEX Render Meshes ripped out from underneath them
	NxResourceList::clear();

	delete this;
}

void IofxAsset::release()
{
	mModule->mSdk->releaseAsset(*this);
}

void IofxAsset::removeAllActors()
{
	ApexContext::removeAllActors();
}

void IofxAsset::addDependentActor(ApexActor* actor)
{
	if (actor)
	{
		actor->addSelfToContext(*this);
	}
}



void IofxAsset::preSerialize(void* userData)
{
	PX_UNUSED(userData);
}

void IofxAsset::postDeserialize(void* userData)
{
	PX_UNUSED(userData);

	NxParameterized::Traits* traits = NiGetApexSDK()->getParameterizedTraits();

	PX_ASSERT(mParams->iofxType);
	if (!mParams->iofxType)
	{
		return;
	}

	initializeAssetNameTable();

	// destroy the old modifier stacks
	for (physx::PxU32 i = mSpawnModifierStack.size(); i--;)
	{
		delete mSpawnModifierStack[i];
	}
	mSpawnModifierStack.resize(0);

	for (physx::PxU32 i = mContinuousModifierStack.size(); i--;)
	{
		delete mContinuousModifierStack[i];
	}
	mContinuousModifierStack.resize(0);

#if IOFX_SLOW_COMPOSITE_MODIFIERS
	// destroy any color vs life params that we may have laying around
	for (physx::PxU32 i = 0; i < mCompositeParams.size(); i++)
	{
		mCompositeParams[i]->destroy();
	}
	mCompositeParams.clear();
#endif

	const char* handleNameList[] = { "spawnModifierList", "continuousModifierList" };

// 'param' and 'newMod' are defined in the local scope where this macro is used
#define _MODIFIER(Type) \
	else if( !strcmp(param->className(), #Type "ModifierParams") )	\
	{																\
		newMod = PX_NEW( Type##Modifier )( (Type##ModifierParams *)param );	\
	}

	for (physx::PxU32 i = 0; i < 2; i++)
	{
		NxParameterized::Handle h(*mParams->iofxType);
		mParams->iofxType->getParameterHandle(handleNameList[i], h);
		int listSize = 0;
		h.getArraySize(listSize);

		for (int j = 0; j < listSize; j++)
		{
			NxParameterized::Interface* param = 0;
			NxParameterized::Handle ih(*mParams->iofxType);
			NxModifier* newMod = 0;

			h.getChildHandle(j, ih);
			ih.getParamRef(param);
			PX_ASSERT(param);
			if (!param)
			{
				continue;
			}

			if (newMod != 0)  //this'll never happen :-)
				{}

#include "ModifierList.h"

#if IOFX_SLOW_COMPOSITE_MODIFIERS
			// LRR: there is a better way to do this, but I don't have time at the moment to
			// make this any cleaner

			// special case, we don't currently have an initial color modifier, just insert
			// a colorVsLife modifier for each color channel
			else if (!strcmp(param->className(), "InitialColorModifierParams"))
			{
				InitialColorModifierParams* icParams = (InitialColorModifierParams*)param;

				for (PxU32 colorIdx = 0; colorIdx < 4; colorIdx++)
				{
					ColorVsLifeModifierParams* p = (ColorVsLifeModifierParams*)
					                               traits->createNxParameterized(ColorVsLifeModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ColorVsLifeModifier* cvlMod = PX_NEW(ColorVsLifeModifier)(p);
					PX_ASSERT(cvlMod);

					// set the color channel
					cvlMod->setColorChannel((ColorChannel)colorIdx);

					// set the curve
					CurveImpl curve;
					curve.addControlPoint(NxVec2R(0.0f, icParams->color[colorIdx]));
					curve.addControlPoint(NxVec2R(1.0f, icParams->color[colorIdx]));

					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ColorVsLifeCompositeModifierParams"))
			{
				// setup an array so we can get the control points easily
				ColorVsLifeCompositeModifierParams* cParams = (ColorVsLifeCompositeModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 colorIdx = 0; colorIdx < 4; colorIdx++)
				{

					ColorVsLifeModifierParams* p = (ColorVsLifeModifierParams*)
					                               traits->createNxParameterized(ColorVsLifeModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ColorVsLifeModifier* cvlMod = PX_NEW(ColorVsLifeModifier)(p);
					PX_ASSERT(cvlMod);

					// set the color channel
					cvlMod->setColorChannel((ColorChannel)colorIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{

						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].lifeRemaining,
						                              cParams->controlPoints.buf[cpIdx].color[colorIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ColorVsDensityCompositeModifierParams"))
			{
				ColorVsDensityCompositeModifierParams* cParams = (ColorVsDensityCompositeModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 colorIdx = 0; colorIdx < 4; colorIdx++)
				{

					ColorVsDensityModifierParams* p = (ColorVsDensityModifierParams*)
					                                  traits->createNxParameterized(ColorVsDensityModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ColorVsDensityModifier* cvlMod = PX_NEW(ColorVsDensityModifier)(p);
					PX_ASSERT(cvlMod);

					// set the color channel
					cvlMod->setColorChannel((ColorChannel)colorIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].density,
						                              cParams->controlPoints.buf[cpIdx].color[colorIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ColorVsVelocityCompositeModifierParams"))
			{
				// setup an array so we can get the control points easily
				ColorVsVelocityCompositeModifierParams* cParams = (ColorVsVelocityCompositeModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 colorIdx = 0; colorIdx < 4; colorIdx++)
				{

					ColorVsVelocityModifierParams* p = (ColorVsVelocityModifierParams*)
					                               traits->createNxParameterized(ColorVsVelocityModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ColorVsVelocityModifier* cvlMod = PX_NEW(ColorVsVelocityModifier)(p);
					PX_ASSERT(cvlMod);

					cvlMod->setVelocity0(cParams->velocity0);
					cvlMod->setVelocity1(cParams->velocity1);

					// set the color channel
					cvlMod->setColorChannel((ColorChannel)colorIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{

						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].velocity,
						                              cParams->controlPoints.buf[cpIdx].color[colorIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsLife2DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsLife2DModifierParams* cParams = (ScaleVsLife2DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 2; axisIdx++)
				{
					ScaleVsLifeModifierParams* p = (ScaleVsLifeModifierParams*)
					                               traits->createNxParameterized(ScaleVsLifeModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsLifeModifier* cvlMod = PX_NEW(ScaleVsLifeModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].lifeRemaining,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsLife3DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsLife3DModifierParams* cParams = (ScaleVsLife3DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 3; axisIdx++)
				{
					ScaleVsLifeModifierParams* p = (ScaleVsLifeModifierParams*)
					                               traits->createNxParameterized(ScaleVsLifeModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsLifeModifier* cvlMod = PX_NEW(ScaleVsLifeModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].lifeRemaining,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsDensity2DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsDensity2DModifierParams* cParams = (ScaleVsDensity2DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 2; axisIdx++)
				{
					ScaleVsDensityModifierParams* p = (ScaleVsDensityModifierParams*)
					                                  traits->createNxParameterized(ScaleVsDensityModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsDensityModifier* cvlMod = PX_NEW(ScaleVsDensityModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].density,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsDensity3DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsDensity3DModifierParams* cParams = (ScaleVsDensity3DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 3; axisIdx++)
				{
					ScaleVsDensityModifierParams* p = (ScaleVsDensityModifierParams*)
					                                  traits->createNxParameterized(ScaleVsDensityModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsDensityModifier* cvlMod = PX_NEW(ScaleVsDensityModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].density,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsCameraDistance2DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsCameraDistance2DModifierParams* cParams = (ScaleVsCameraDistance2DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 2; axisIdx++)
				{
					ScaleVsCameraDistanceModifierParams* p = (ScaleVsCameraDistanceModifierParams*)
					        traits->createNxParameterized(ScaleVsCameraDistanceModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsCameraDistanceModifier* cvlMod = PX_NEW(ScaleVsCameraDistanceModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].cameraDistance,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}

			else if (!strcmp(param->className(), "ScaleVsCameraDistance3DModifierParams"))
			{
				// setup an array so we can get the control points easily
				ScaleVsCameraDistance3DModifierParams* cParams = (ScaleVsCameraDistance3DModifierParams*)param;

				if (cParams->controlPoints.arraySizes[0] == 0)
				{
					continue;
				}

				for (PxU32 axisIdx = 0; axisIdx < 3; axisIdx++)
				{
					ScaleVsCameraDistanceModifierParams* p = (ScaleVsCameraDistanceModifierParams*)
					        traits->createNxParameterized(ScaleVsCameraDistanceModifierParams::staticClassName());

					// save these parameterized classes off for later, we'll need to destroy them
					mCompositeParams.pushBack(p);

					ScaleVsCameraDistanceModifier* cvlMod = PX_NEW(ScaleVsCameraDistanceModifier)(p);
					PX_ASSERT(cvlMod);

					// set the scale axis
					cvlMod->setScaleAxis((ScaleAxis)axisIdx);

					// set the curve
					CurveImpl curve;
					for (physx::PxI32 cpIdx = 0; cpIdx < cParams->controlPoints.arraySizes[0]; cpIdx++)
					{
						curve.addControlPoint(NxVec2R(cParams->controlPoints.buf[cpIdx].cameraDistance,
						                              cParams->controlPoints.buf[cpIdx].scale[axisIdx]));
					}
					cvlMod->setFunction(&curve);

					// save which sprite render semantics this modifier will update
					setSpriteSemanticsUsed(cvlMod->getModifierSpriteSemantics());
					setMeshSemanticsUsed(cvlMod->getModifierMeshSemantics());
					ModifierStack& activeStack = getModifierStack(i);
					activeStack.pushBack(cvlMod);
				}
			}
#endif /* IOFX_SLOW_COMPOSITE_MODIFIERS */

			if (newMod)
			{
				// save which sprite render semantics this modifier will update
				setSpriteSemanticsUsed(newMod->getModifierSpriteSemantics());
				setMeshSemanticsUsed(newMod->getModifierMeshSemantics());

				ModifierStack& activeStack = getModifierStack(i);
				activeStack.pushBack(newMod);
			}

		}
	}

	bool useFloat4Color = mParams->renderOutput.useFloat4Color;
	if (!useFloat4Color)
	{
		//detect if some Color modifier has a color component value greater than 1
		for (physx::PxU32 modStage = 0; modStage < 2; modStage++)
		{
			const ModifierStack& activeStack = getModifierStack(modStage);

			physx::PxU32 modCount = activeStack.size();
			for (physx::PxU32 modIndex = 0; modIndex < modCount; modIndex++)
			{
				const NxCurve* pColorCurve = 0;

				const NxModifier* pModifier = activeStack[modIndex];
				switch (pModifier->getModifierType())
				{
					case ModifierType_ColorVsLife:
						pColorCurve = static_cast<const NxColorVsLifeModifier*>(pModifier)->getFunction();
						break;
					case ModifierType_ColorVsDensity:
						pColorCurve = static_cast<const NxColorVsDensityModifier*>(pModifier)->getFunction();
						break;
				}

				if (pColorCurve)
				{
					physx::PxU32 pointsCount;
					const NxVec2R* pPoints = pColorCurve->getControlPoints(pointsCount);

					for (physx::PxU32 i = 0; i < pointsCount; i++)
					{
						if (pPoints[i].y > 1)
						{
							useFloat4Color = true;
							break;
						}
					}
				}
				if (useFloat4Color)
				{
					break;
				}
			}
			if (useFloat4Color)
			{
				break;
			}
		}
	}

	if (mParams->renderOutput.useUserSemantic)
	{
		mMeshSemanticBitmap |= (1 << NxRenderInstanceSemantic::USER_DATA);
		mSpriteSemanticBitmap |= (1 << NxRenderSpriteSemantic::USER_DATA);
	}
}

const char* IofxAsset::getSpriteMaterialName() const
{
	if (mSpriteParams &&
	        mSpriteParams->spriteMaterialName &&
	        mSpriteParams->spriteMaterialName->name() &&
	        mSpriteParams->spriteMaterialName->name()[0])
	{
		return mSpriteParams->spriteMaterialName->name();
	}
	else
	{
		return NULL;
	}
}

const char* IofxAsset::getMeshAssetName(physx::PxU32 index) const
{
	if (mRenderMeshList && index < (*mRenderMeshList).size() && (*mRenderMeshList)[index].meshAssetName)
	{
		return (*mRenderMeshList)[index].meshAssetName->name();
	}
	else
	{
		return NULL;
	}
}

physx::PxU32 IofxAsset::getMeshAssetWeight(physx::PxU32 index) const
{
	if (mRenderMeshList && index < (*mRenderMeshList).size())
	{
		return (*mRenderMeshList)[index].weight;
	}
	else
	{
		return 0;
	}
}

bool IofxAsset::isOpaqueMesh(physx::PxU32 index) const
{
	if (mRenderMeshList && index < (*mRenderMeshList).size() && (*mRenderMeshList)[index].meshAssetName)
	{
		return strcmp((*mRenderMeshList)[index].meshAssetName->className(), "ApexOpaqueMesh") == 0;
	}
	else
	{
		return false;
	}
}

physx::PxU32 IofxAsset::forceLoadAssets()
{
	physx::PxU32 assetLoadedCount = 0;

	assetLoadedCount += mRenderMeshAssetTracker.forceLoadAssets();
	assetLoadedCount += mSpriteMaterialAssetTracker.forceLoadAssets();

	return assetLoadedCount;
}

void IofxAsset::initializeAssetNameTable()
{
	// clean up asset tracker list
	mRenderMeshAssetTracker.removeAllAssetNames();
	mSpriteMaterialAssetTracker.removeAllAssetNames();

	if (mRenderMeshList)
	{
		/* initialize the rendermesh asset name to resID tables */
		for (physx::PxU32 i = 0; i < (*mRenderMeshList).size(); i++)
		{
			const char* name = "";
			if ((*mRenderMeshList)[i].meshAssetName)
			{
				name = (*mRenderMeshList)[i].meshAssetName->name();
			}
			mRenderMeshAssetTracker.addAssetName(name, isOpaqueMesh(i));
		}
	}
	else
	{
		/* initialize the sprite material name to resID tables */
		if (getSpriteMaterialName())
		{
			mSpriteMaterialAssetTracker.addAssetName(getSpriteMaterialName(), false);
		}
	}
}

void IofxAsset::setSpriteSemanticsUsed(physx::PxU32 spriteSemanticsBitmap)
{
	mSpriteSemanticBitmap |= spriteSemanticsBitmap;
}
void IofxAsset::setMeshSemanticsUsed(physx::PxU32 meshSemanticsBitmap)
{
	mMeshSemanticBitmap |= meshSemanticsBitmap;
}

PxU32 IofxAsset::getPubStateSize() const
{
	return mSpriteParams ? sizeof(SpritePublicState) : sizeof(MeshPublicState);
}

PxU32 IofxAsset::getPrivStateSize() const
{
	return mSpriteParams ? sizeof(SpritePrivateState) : sizeof(MeshPrivateState);
}

bool IofxAsset::isSortingEnabled() const
{
	const ModifierStack& stack = getModifierStack(ModifierStage_Continuous);
	for (ModifierStack::ConstIterator it = stack.begin(); it != stack.end(); ++it)
	{
		if ((*it)->getModifierType() == ModifierType_ViewDirectionSorting)
		{
			return true;
		}
	}

	return false;
}

#ifndef WITHOUT_APEX_AUTHORING

void IofxAssetAuthoring::release()
{
	delete this;
}

const char* IofxAssetAuthoring::getObjTypeName() const
{
	return IofxAsset::getObjTypeName();
}

#if IOFX_AUTHORING_API_ENABLED

void IofxAssetAuthoring::setMeshAssetCount(const physx::PxU32 meshCount)
{
	(*mRenderMeshList).resize(meshCount);
}

void IofxAssetAuthoring::setSpriteMaterialName(const char* inSpriteMaterialName)
{
	if ((*mRenderMeshList).size())
	{
		APEX_INVALID_OPERATION("Unable to set Sprite Material Name (%s). Systems can be either Mesh or Sprite, but not both. This system is already a Mesh.", getSpriteMaterialName());
		return;
	}

	NxParameterized::Handle h(*mParams);
	NxParameterized::Interface* refParam;

	mParams->getParameterHandle("spriteMaterialName", h);
	mParams->initParamRef(h, NULL, true);
	mParams->getParamRef(h, refParam);

	PX_ASSERT(refParam);
	if (refParam)
	{
		refParam->setName(inSpriteMaterialName);
	}
}

void IofxAssetAuthoring::setMeshAssetName(const char* meshAssetName, physx::PxU32 meshIndex)
{
	if (getSpriteMaterialName())
	{
		APEX_INVALID_OPERATION("Unable to set Mesh Material Name (%s). Systems can be either Mesh or Sprite, but not both. This system is already a Sprite.", meshAssetName);
		return;
	}

	PX_ASSERT(meshIndex < (*mRenderMeshList).size());

	NxParameterized::Handle arrayHandle(*mParams), indexHandle(*mParams), childHandle(*mParams);
	NxParameterized::Interface* refPtr;

	mParams->getParameterHandle("renderMeshList", arrayHandle);
	arrayHandle.getChildHandle(meshIndex, indexHandle);

	indexHandle.getChildHandle(mParams, "meshAssetName", childHandle);
	mParams->initParamRef(childHandle, NULL, true);
	mParams->getParamRef(childHandle, refPtr);
	PX_ASSERT(refPtr);
	if (refPtr)
	{
		refPtr->setName(meshAssetName);
	}
}

void IofxAssetAuthoring::setMeshAssetWeight(const physx::PxU32 weight, physx::PxU32 meshIndex)
{
	if (getSpriteMaterialName())
	{
		APEX_INVALID_OPERATION("Unable to set Mesh Material Weight (%d). Systems can be either Mesh or Sprite, but not both. This system is already a Sprite.", weight);
		return;
	}

	PX_ASSERT(meshIndex < (*mRenderMeshList).size());
	(*mRenderMeshList)[meshIndex].weight = weight;
}


NxModifier* IofxAssetAuthoring::createModifier(physx::PxU32 modStage, physx::PxU32 modType)
{
	// create a modifier to go in this particular modifier stage
	// we'll need a handle to the correct mod stage list, pass that and the Param Obj to CreateModifier,
	// it will resize the array (+1), then pass the correct string to initParamRef()
	// Then it can pass the new ref param to the mod constructor
	NxParameterized::Handle h(*mParams), ih(*mParams);
	int modStackSize = 0;

	getModifierStack(modStage, h);

	// resize the array
	mParams->getArraySize(h, modStackSize);
	mParams->resizeArray(h, ++modStackSize);

	h.getChildHandle(modStackSize - 1, ih);

	NxModifier* retVal = CreateModifier((ModifierTypeEnum)modType, mParams, ih);

	physx::PxU32 assetTarget = getAssetTarget();
	// We don't yet know what type this is, which is invalid.
	if (assetTarget == 0)
	{
		// delete the new param ref and re-resize array
		NxParameterized::Interface* refParam = 0;
		mParams->getParamRef(ih, refParam);
		PX_ASSERT(refParam);
		if (refParam)
		{
			refParam->destroy();
		}
		mParams->resizeArray(h, modStackSize - 1);

		delete retVal;
		retVal = 0;
		APEX_INVALID_OPERATION("Specifying modifiers before specifying the mesh or sprite asset is invalid");
		return retVal;
	}

	// The modifier only supports certain semantics, so let's test those now.
	const physx::PxU32 supportedUsage = retVal->getModifierUsage();

	if ((supportedUsage & assetTarget) != assetTarget)
	{
		// delete the new param ref and re-resize array
		NxParameterized::Interface* refParam = 0;
		mParams->getParamRef(ih, refParam);
		PX_ASSERT(refParam);
		if (refParam)
		{
			refParam->destroy();
		}
		mParams->resizeArray(h, modStackSize - 1);

		delete retVal;
		retVal = 0;
		APEX_INVALID_OPERATION("The specified modifier doesn't work on that system type (e.g. Sprite Modifier on a Mesh System or vice-versa).");
		return retVal;
	}

	physx::PxU32 stageBits = ModifierUsageFromStage(ModifierStage(modStage));
	if ((supportedUsage & stageBits) != stageBits)
	{
		// delete the new param ref and re-resize array
		NxParameterized::Interface* refParam = 0;
		mParams->getParamRef(ih, refParam);
		PX_ASSERT(refParam);
		if (refParam)
		{
			refParam->destroy();
		}
		mParams->resizeArray(h, modStackSize - 1);

		delete retVal;
		retVal = 0;
		APEX_INVALID_OPERATION("The specified modifier doesn't work in that stage.");
		return retVal;
	}

	ModifierStack& activeStack = getModifierStack(modStage);

	activeStack.pushBack(retVal);
	return retVal;
}

void IofxAssetAuthoring::removeModifier(physx::PxU32 modStage, physx::PxU32 position)
{
	if (modStage >= ModifierStage_Count)
	{
		APEX_INVALID_OPERATION("Invalid modifier stage");
		return;
	}

	ModifierStack& activeStack = getModifierStack(modStage);

	if (position >= activeStack.size())
	{
		APEX_INVALID_OPERATION("position %d is greater than modifier stack size: %d", position, activeStack.size());
		return;
	}

	// remove from ModifierStack (runtime)
	activeStack.replaceWithLast(position);

	// remove from NxParameterized data
	NxParameterized::Handle h(*mParams), ih(*mParams);
	NxParameterized::Interface* modParams = 0;
	int modStackSize = 0;

	// 1. get the correct stack
	getModifierStack(modStage, h);

	// 2. get the NxParameterized::Interface* for the position
	mParams->getArraySize(h, modStackSize);
	h.getChildHandle(position, ih);
	mParams->getParamRef(ih, modParams);
	PX_ASSERT(modParams);

	// 3. destroy it
	modParams->destroy();

	// 4. copy the last member of the stack to position
	if (position != (physx::PxU32)modStackSize - 1)
	{
		NxParameterized::Handle lastH(*mParams);
		h.getChildHandle(modStackSize - 1, lastH);
		mParams->getParamRef(lastH, modParams);
		mParams->setParamRef(ih, modParams);
	}

	// 5. resize the stack
	mParams->resizeArray(h, modStackSize - 1);
}

physx::PxU32 IofxAssetAuthoring::getModifierCount(physx::PxU32 modStage) const
{
	if (modStage >= ModifierStage_Count)
	{
		APEX_INVALID_OPERATION("Invalid modifier stage");
		return 0;
	}

	const ModifierStack& activeStack = getModifierStack(modStage);
	return activeStack.size();
}

physx::PxU32 IofxAssetAuthoring::findModifier(physx::PxU32 modStage, NxModifier* modifier)
{
	if (modStage >= ModifierStage_Count)
	{
		APEX_INVALID_OPERATION("Invalid modifier stage");
		return 0;
	}

	const ModifierStack& activeStack = getModifierStack(modStage);

	for (physx::PxU32 i = 0; i < activeStack.size(); i++)
	{
		if (activeStack[i] == modifier)
		{
			return i;
		}
	}

	return 0xffffffff;
}

NxModifier* IofxAssetAuthoring::getModifier(physx::PxU32 modStage, physx::PxU32 position) const
{
	if (modStage >= ModifierStage_Count)
	{
		APEX_INVALID_OPERATION("Invalid modifier stage");
		return 0;
	}

	const ModifierStack& activeStack = getModifierStack(modStage);

	if (position >= activeStack.size())
	{
		APEX_INVALID_OPERATION("position %d is greater than modifier stack size: %d", position, activeStack.size());
		return 0;
	}

	// remove from ModifierStack (runtime)
	return activeStack[ position ];
}

physx::PxU32 IofxAssetAuthoring::getAssetTarget() const
{
	physx::PxU32 retVal = 0;
	if (getSpriteMaterialName())
	{
		retVal |= ModifierUsage_Sprite;
	}
	else if ((*mRenderMeshList).size())
	{
		retVal |= ModifierUsage_Mesh;
	}

	return retVal;
}

#endif /* IOFX_AUTHORING_API_ENABLED */

#endif

}
}
} // namespace physx::apex
