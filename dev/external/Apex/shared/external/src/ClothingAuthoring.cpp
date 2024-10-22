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
#if defined(PX_WINDOWS)

#include "ClothingAuthoring.h"

#include <cstdio>
#include <stdarg.h>
#define NOMINMAX
#include <windows.h>

// for std::sort
#include <algorithm>

// PhysX foundation
#include <PsMathUtils.h>
#include <PsTime.h>

// PhysX
#if NX_SDK_VERSION_MAJOR == 2
#include <NxArray.h> // doh
#include <NxActorDesc.h>
#include <NxPlaneShapeDesc.h>
#include <NxScene.h>
#elif NX_SDK_VERSION_MAJOR == 3
#include "extensions/PxDefaultCpuDispatcher.h"
#include "extensions/PxDefaultSimulationFilterShader.h"
#include <PxPhysics.h>
#include <PxScene.h>
#include <PxRigidStatic.h>
#include "geometry/PxPlaneGeometry.h"
#endif
#include "pxtask/PxCudaContextManager.h"

// APEX Framework
#include <NxApex.h>
#include <NxApexRenderDebug.h>
#include <NxParamUtils.h>
#include <NxApexCustomBufferIterator.h>

// APEX Clothing
#include <NxClothingActor.h>
#include <NxClothingAsset.h>
#include <NxClothingAssetAuthoring.h>
#include <NxClothingPhysicalMesh.h>
#include <NxClothingPreview.h>
#include <NxClothingIsoMesh.h>
#include <NxClothingUserRecompute.h>
#include <NxModuleClothing.h>

// shared::external
#include "BestFit.h"
#include "MeshPainter.h"
#include "SkeletalAnim.h"
#include "TriangleMesh.h"
#include "foundation/PxMath.h"

// shared::general
#include <AutoGeometry.h>

// MeshImport
#include <MeshImport.h>

//PVD
#include "PvdBinding.h"
#include "physxvisualdebuggersdk/PvdConnectionFlags.h"



#ifdef _DEBUG
#define VERIFY_PARAM(_A) PX_ASSERT(_A == NxParameterized::ERROR_NONE)
#else
#define VERIFY_PARAM(_A) _A
#endif


typedef physx::PxU32 PxU32;
typedef physx::PxVec3 PxVec3;
typedef physx::PxBounds3 PxBounds3;

namespace SharedTools
{
const unsigned int SIMPLIFICATOR_MAX_STEPS = 100000;

class ProgressCallback : public physx::apex::IProgressListener
{
public:
	ProgressCallback(int totalWork, physx::apex::IProgressListener* parent) :
		m_work(0), m_subtaskWork(0), m_totalWork(totalWork), m_taskName(NULL), m_parent(parent) {}

	void	setSubtaskWork(int subtaskWork, const char* taskName = NULL)
	{
		if (subtaskWork < 0)
		{
			subtaskWork = m_totalWork - m_work;
		}

		m_subtaskWork = subtaskWork;
		PX_ASSERT(m_work + m_subtaskWork <= m_totalWork);
		m_taskName = taskName;
		setProgress(0, m_taskName);
	}

	void	completeSubtask()
	{
		setProgress(100, m_taskName);
		m_work += m_subtaskWork;
		m_subtaskWork = 0;
	}

	void	setProgress(int progress, const char* taskName = NULL)
	{
		PX_ASSERT(progress >= 0);
		PX_ASSERT(progress <= 100);

		if (taskName == NULL)
		{
			taskName = m_taskName;
		}

		if (m_parent != NULL)
		{
			m_parent->setProgress(m_totalWork > 0 ? (m_work * 100 + m_subtaskWork * progress) / m_totalWork : 100, taskName);
		}
	}

protected:
	int m_work;
	int m_subtaskWork;
	int m_totalWork;
	const char* m_taskName;
	physx::apex::IProgressListener* m_parent;
};


void ClothingAuthoring::Simulation::ClothingAsset::releaseRenderMeshAssets()
{
	for (size_t i = 0; i < renderMeshAssets.size(); ++i)
	{
		if (renderMeshAssets[i] != NULL)
		{
			renderMeshAssets[i]->release();
		}
	}
}


ClothingAuthoring::ClothingAuthoring(physx::apex::NxApexSDK* apexSDK,
                                     physx::apex::NxModuleClothing* moduleClothing,
                                     physx::apex::NxResourceCallback* resourceCallback,
                                     physx::apex::NxUserRenderResourceManager* renderResourceManager,
                                     ClothingAuthoring::ErrorCallback* errorCallback) :

	_mApexSDK(apexSDK),
	_mModuleClothing(moduleClothing),
	_mApexRenderDebug(NULL),
	_mResourceCallback(resourceCallback),
	_mRenderResourceManager(renderResourceManager),
	_mMaterialList(NULL),

	_mErrorCallback(errorCallback),
	mCurrentActorDesc(0)
{
	mSimulation.init();
	mMeshes.init();
	mAuthoringObjects.init();
	resetTempConfiguration();

	initConfiguration();
	prepareConfiguration();

	_mApexRenderDebug = _mApexSDK->createApexRenderDebug();

	physx::apex::NxApexSceneDesc sceneDesc;

#ifdef PX_WINDOWS
	// create cuda context manager
	if (mSimulation.cudaContextManager == NULL)
	{
#if NX_SDK_VERSION_MAJOR == 2
		physx::PxCudaContextManagerDesc cudaContextManagerDesc;
		mSimulation.cudaContextManager = _mApexSDK->createCudaContextManager(cudaContextManagerDesc);
#elif NX_SDK_VERSION_MAJOR == 3
		physx::PxProfileZoneManager* profileZoneManager = _mApexSDK->getPhysXSDK()->getProfileZoneManager();
		physx::PxCudaContextManagerDesc cudaContextManagerDesc;
		if (mSimulation.cudaContextManager == NULL && physx::PxGetSuggestedCudaDeviceOrdinal(_mApexSDK->getPhysXSDK()->getFoundation().getErrorCallback()) != -1)
		{
			mSimulation.cudaContextManager = physx::PxCreateCudaContextManager(_mApexSDK->getPhysXSDK()->getFoundation(), cudaContextManagerDesc, profileZoneManager);
		}
#endif
	}
#endif


#if NX_SDK_VERSION_MAJOR == 2
	mSimulation.cpuDispatcher = _mApexSDK->createCpuDispatcher(2);
	sceneDesc.cpuDispatcher = mSimulation.cpuDispatcher;
	if (mSimulation.cudaContextManager != NULL)
	{
		sceneDesc.gpuDispatcher = mSimulation.cudaContextManager->getGpuDispatcher();
	}
#elif NX_SDK_VERSION_MAJOR == 3
	mSimulation.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
#endif

	mSimulation.apexScene = _mApexSDK->createScene(sceneDesc);


	if (mSimulation.apexScene == NULL)
	{
		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportError("NxApexScene Creation", "Error: Unable to initialize APEX Scene.");
		}
	}
	else
	{
		mSimulation.apexScene->allocViewMatrix(physx::apex::ViewMatrixType::LOOK_AT_RH);
		mSimulation.apexScene->allocProjMatrix(physx::apex::ProjMatrixType::USER_CUSTOMIZED);
	}

}



ClothingAuthoring::~ClothingAuthoring()
{
	releasePhysX();

	if (mSimulation.cudaContextManager != NULL)
	{
		mSimulation.cudaContextManager->release();
		mSimulation.cudaContextManager = NULL;
	}

	resetTempConfiguration();

	mMeshes.clear(_mRenderResourceManager, _mResourceCallback, true);
}


void ClothingAuthoring::addNotifyCallback(NotificationCallback* callback)
{
	_mNotifyCallbacks.push_back(callback);
}



void ClothingAuthoring::setMaterialList(Samples::MaterialList* list)
{
	_mMaterialList = list;
}



void ClothingAuthoring::releasePhysX()
{
	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		for (size_t j = 0; j < mSimulation.actors[i].actors.size(); j++)
		{
			if (mSimulation.actors[i].actors[j] != NULL)
			{
				mSimulation.actors[i].actors[j]->release();
				mSimulation.actors[i].actors[j] = NULL;
			}
		}

		for (size_t j = 0; j < mSimulation.actors[i].previews.size(); j++)
		{
			if (mSimulation.actors[i].previews[j] != NULL)
			{
				mSimulation.actors[i].previews[j]->release();
				mSimulation.actors[i].previews[j] = NULL;
			}
		}

		if (mSimulation.actors[i].triangleMesh != NULL)
		{
			mSimulation.actors[i].triangleMesh->clear(_mRenderResourceManager, _mResourceCallback);
			delete mSimulation.actors[i].triangleMesh;
			mSimulation.actors[i].triangleMesh = NULL;
		}
	}
	mSimulation.actors.clear();

	if (mSimulation.userRecompute != NULL)
	{
		delete mSimulation.userRecompute;
		mSimulation.userRecompute = NULL;
	}

	for (size_t i = 0; i < mSimulation.assets.size(); i++)
	{
		mSimulation.assets[i].releaseRenderMeshAssets();
		mSimulation.assets[i].apexAsset->release();
		mSimulation.assets[i].apexAsset = NULL;
	}
	mSimulation.assets.clear();

	if (mSimulation.apexScene != NULL)
	{
		mSimulation.apexScene->release();
		mSimulation.apexScene = NULL;
	}

	if (mSimulation.cpuDispatcher != NULL)
	{
#if NX_SDK_VERSION_MAJOR == 2
		_mApexSDK->releaseCpuDispatcher(*mSimulation.cpuDispatcher);
#elif NX_SDK_VERSION_MAJOR == 3
		delete mSimulation.cpuDispatcher;
#endif
		mSimulation.cpuDispatcher = NULL;
	}


	if (mSimulation.groundPlane != NULL)
	{
		PX_ASSERT(mSimulation.physxScene != NULL);
#if NX_SDK_VERSION_MAJOR == 2
		mSimulation.physxScene->releaseActor(*mSimulation.groundPlane);
#elif NX_SDK_VERSION_MAJOR == 3
		mSimulation.groundPlane->release();
#endif
		mSimulation.groundPlane = NULL;
	}

	if (mSimulation.physxScene != NULL)
	{
#if NX_SDK_VERSION_MAJOR == 2
		_mApexSDK->getPhysXSDK()->releaseScene(*mSimulation.physxScene);
#elif NX_SDK_VERSION_MAJOR == 3
		mSimulation.physxScene->release();
#endif
		mSimulation.physxScene = NULL;
	}

#if NX_SDK_VERSION_MAJOR == 2
	PX_ASSERT(_mApexSDK->getPhysXSDK()->getNbClothMeshes() == 0);
	PX_ASSERT(_mApexSDK->getPhysXSDK()->getNbSoftBodyMeshes() == 0);
#elif NX_SDK_VERSION_MAJOR == 3
	PX_ASSERT(_mApexSDK->getPhysXSDK()->getNbClothFabrics() == 0);
#endif

	mAuthoringObjects.clear();
	clearMaterialLibraries();

	if (_mApexRenderDebug != NULL)
	{
		_mApexRenderDebug->release();
		_mApexRenderDebug = NULL;
	}

	clearLoadedActorDescs();
}



void ClothingAuthoring::connectPVD(bool toggle)
{

	PVD::PvdBinding* pvd = _mApexSDK->getPvdBinding();
	if (pvd != NULL)
	{
		stopSimulation();
		if (pvd->isConnected())
		{
			if (mState.needsReconnect)
			{
				pvd->disconnect();
			}
			else if (toggle)
			{
				pvd->disconnect();
				return;
			}
		}

		if (!pvd->isConnected())
		{
			physx::debugger::TConnectionFlagsType flags;
			if (mConfig.simulation.pvdDebug)
			{
				flags |= physx::debugger::PvdConnectionType::eDEBUG;
			}
			if (mConfig.simulation.pvdProfile)
			{
				flags |= physx::debugger::PvdConnectionType::ePROFILE;
			}
			if (mConfig.simulation.pvdMemory)
			{
				flags |= physx::debugger::PvdConnectionType::eMEMORY;
			}

			if (flags != physx::debugger::TConnectionFlagsType())
			{
				pvd->connect("localhost", DEFAULT_PVD_BINDING_PORT, DEFAULT_PVD_BINDING_TIMEOUT_MS, flags);
			}
		}
	}
}



int  ClothingAuthoring::getNumParameters() const
{
	return (int)(mFloatConfiguration.size() + mIntConfiguration.size() + mBoolConfiguration.size());
}



bool ClothingAuthoring::getParameter(unsigned int i, std::string& name, std::string& type, std::string& val) const
{
	if (i < mFloatConfiguration.size())
	{
		std::map<std::string, float*>::const_iterator it;
		for (it = mFloatConfiguration.begin(); i > 0 && it != mFloatConfiguration.end(); ++it, --i)
		{
			;
		}

		PX_ASSERT(it != mFloatConfiguration.end());
		name = it->first;
		type = "float";
		char buf[16];
		sprintf_s(buf, 16, "%.9g", *it->second);
		val = buf;
		return true;
	}
	i -= (int)mFloatConfiguration.size();

	if (i < mIntConfiguration.size())
	{
		std::map<std::string, int*>::const_iterator it;
		for (it = mIntConfiguration.begin(); i > 0 && it != mIntConfiguration.end(); ++it, --i)
		{
			;
		}

		PX_ASSERT(it != mIntConfiguration.end());
		name = it->first;
		type = "int";
		char buf[16];
		sprintf_s(buf, 16, "%d", *it->second);
		val = buf;
		return true;
	}
	i -= (int)mIntConfiguration.size();

	if (i < mBoolConfiguration.size())
	{
		std::map<std::string, bool*>::const_iterator it;
		for (it = mBoolConfiguration.begin(); i > 0 && it != mBoolConfiguration.end(); ++it, --i)
		{
			;
		}

		PX_ASSERT(it != mBoolConfiguration.end());
		name = it->first;
		type = "bool";
		val = *it->second ? "true" : "false";
		return true;
	}

	return false;
}



bool ClothingAuthoring::setParameter(std::string& name, std::string& type, std::string& val)
{
	if (type == "float")
	{
		std::map<std::string, float*>::iterator it = mFloatConfiguration.find(name);

		if (it != mFloatConfiguration.end())
		{
			*(it->second) = (float)atof(val.c_str());
			return true;
		}
		else
		{
			it = mFloatConfigurationOld.find(name);
			if (it != mFloatConfigurationOld.end())
			{
				*(it->second) = (float)atof(val.c_str());
				return true;
			}
		}
	}

	if (type == "int")
	{
		std::map<std::string, int*>::iterator it = mIntConfiguration.find(name);

		if (it != mIntConfiguration.end())
		{
			*(it->second) = atoi(val.c_str());

			if (it->first == "mConfig.mesh.textureUvOrigin")
			{
				setTextureUvOrigin((physx::apex::NxTextureUVOrigin::Enum) * (it->second));
			}
			return true;
		}
		else
		{
			it = mIntConfigurationOld.find(name);
			if (it != mIntConfigurationOld.end())
			{
				*(it->second) = atoi(val.c_str());
				return true;
			}
		}
	}

	if (type == "bool")
	{
		std::map<std::string, bool*>::iterator it = mBoolConfiguration.find(name);

		if (it != mBoolConfiguration.end())
		{
			*(it->second) = (val == "true");
			return true;
		}
		else
		{
			it = mBoolConfigurationOld.find(name);
			if (it != mBoolConfigurationOld.end())
			{
				*(it->second) = (val == "true");
				return true;
			}
		}
	}

	return false;
}



bool ClothingAuthoring::createDefaultMaterialLibrary()
{
	if (!mState.materialLibraries.empty())
	{
		CurrentState::tMaterialLibraries::iterator found;

		// Assert that each library has one material!
		for (found = mState.materialLibraries.begin(); found != mState.materialLibraries.end(); ++found)
		{
			NxParameterized::Interface* materialLibrary = found->second;
			PX_ASSERT(materialLibrary != NULL);

			NxParameterized::Handle arrayHandle(materialLibrary);

			NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
			error = arrayHandle.getParameter("materials");
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			int numMaterials = 0;
			error = arrayHandle.getArraySize(numMaterials);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			if (numMaterials == 0)
			{
				numMaterials = 1;
				addMaterialToLibrary(found->first.c_str(), "Default");
			}
		}

		// select one material
		found = mState.materialLibraries.find(mState.selectedMaterialLibrary);
		if (found == mState.materialLibraries.end())
		{
			found = mState.materialLibraries.begin();
			mState.selectedMaterialLibrary = found->first;
		}

		NxParameterized::Interface* materialLibrary = found->second;
		PX_ASSERT(materialLibrary != NULL);

		NxParameterized::Handle arrayHandle(materialLibrary);

		NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
		error = arrayHandle.getParameter("materials");
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		int numMaterials = 0;
		error = arrayHandle.getArraySize(numMaterials);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		const char* firstMaterialName = NULL;
		for (int i = 0; i < numMaterials; i++)
		{
			error = arrayHandle.set(i);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			NxParameterized::Handle nameHandle(materialLibrary);
			error = arrayHandle.getChildHandle(materialLibrary, "materialName", nameHandle);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			const char* materialName = NULL;
			error = nameHandle.getParamString(materialName);

			if (firstMaterialName == NULL)
			{
				firstMaterialName = materialName;
			}

			if (mState.selectedMaterial == materialName)
			{
				return false;    // we found an existing material
			}

			arrayHandle.popIndex();
		}

		PX_ASSERT(firstMaterialName != NULL);
		mState.selectedMaterial = firstMaterialName;

		return false;
	}

	mState.selectedMaterialLibrary = "Default";
	mState.selectedMaterial = "Default";

	addMaterialLibrary("Default", NULL);
	addMaterialToLibrary("Default", "Default");
	addMaterialToLibrary("Default", "HS Limited");
	addMaterialToLibrary("Default", "Low Gravity");

	mDirty.workspace = true;

	PX_ASSERT(mState.materialLibraries.begin()->first == "Default");
	NxParameterized::Interface* materialLibrary = mState.materialLibraries.begin()->second;

	NxParameterized::Handle arrayHandle(materialLibrary);

	NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
	error = arrayHandle.getParameter("materials");
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	int numMaterials = 0;
	error = arrayHandle.getArraySize(numMaterials);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	PX_ASSERT(numMaterials == 3);

	NxParameterized::Handle elementHandle(materialLibrary);

	for (int i = 0; i < numMaterials; i++)
	{
		arrayHandle.set(i);

		if (i == 2)
		{
			error = arrayHandle.getChildHandle(materialLibrary, "gravityScale", elementHandle);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);
			error = elementHandle.setParamF32(0.2f);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);
		}

		if (i == 1)
		{
			error = arrayHandle.getChildHandle(materialLibrary, "hardStretchLimitation", elementHandle);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);
			error = elementHandle.setParamF32(1.03f);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);
		}

		arrayHandle.popIndex();
	}

	return true;
}



bool ClothingAuthoring::addMaterialLibrary(const char* name, NxParameterized::Interface* newInterface)
{
	PX_ASSERT(newInterface == NULL || strcmp(newInterface->className(), "ClothingMaterialLibraryParameters") == 0);

	CurrentState::tMaterialLibraries::iterator found = mState.materialLibraries.find(name);
	if (found != mState.materialLibraries.end())
	{
		if (newInterface == NULL)
		{
			return true;
		}

		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportErrorPrintf("Material Loading", "Material \'%s\' already exists, didn't overwrite", name);
		}
		return false;
	}
	else
	{
		if (newInterface == NULL)
		{
			mState.materialLibraries[name] = _mApexSDK->getParameterizedTraits()->createNxParameterized("ClothingMaterialLibraryParameters");
		}
		else
		{
			mState.materialLibraries[name] = newInterface;
		}
	}
	return true;
}



void ClothingAuthoring::removeMaterialLibrary(const char* name)
{
	CurrentState::tMaterialLibraries::iterator found = mState.materialLibraries.find(name);
	if (found != mState.materialLibraries.end())
	{
		found->second->destroy();
		mState.materialLibraries.erase(found);
	}

	createDefaultMaterialLibrary();
}



void ClothingAuthoring::addMaterialToLibrary(const char* libName, const char* matName)
{
	// here starts the fun
	CurrentState::tMaterialLibraries::iterator found = mState.materialLibraries.find(libName);
	if (found == mState.materialLibraries.end())
	{
		return;
	}

	NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;

	NxParameterized::Interface* materialLibrary = found->second;
	NxParameterized::Handle arrayHandle(materialLibrary);
	error = arrayHandle.getParameter("materials");
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	int oldSize = 0;
	error = arrayHandle.getArraySize(oldSize);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = arrayHandle.resizeArray(oldSize + 1);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.set(oldSize);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	NxParameterized::Handle elementHandle(materialLibrary);
	error = arrayHandle.getChildHandle(materialLibrary, "materialName", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamString(matName);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "verticalStretchingStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "horizontalStretchingStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "bendingStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "shearingStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "zeroStretchStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "tetherStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "tetherLimit", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "orthoBending", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamBool(false);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "verticalStiffnessScaling.compressionRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "verticalStiffnessScaling.stretchRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "verticalStiffnessScaling.scale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "horizontalStiffnessScaling.compressionRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "horizontalStiffnessScaling.stretchRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "horizontalStiffnessScaling.scale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "bendingStiffnessScaling.compressionRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "bendingStiffnessScaling.stretchRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "bendingStiffnessScaling.scale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "shearingStiffnessScaling.compressionRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "shearingStiffnessScaling.stretchRange", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "shearingStiffnessScaling.scale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.5f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "damping", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.1f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "stiffnessFrequency", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(100.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "comDamping", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamBool(false);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "friction", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.25f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "massScale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.25f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "solverIterations", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamU32(5);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "solverFrequency", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(250.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "gravityScale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "inertiaScale", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "hardStretchLimitation", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "maxDistanceBias", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "hierarchicalSolverIterations", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamU32(0);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "selfcollisionThickness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(0.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	error = arrayHandle.getChildHandle(materialLibrary, "selfcollisionStiffness", elementHandle);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
	error = elementHandle.setParamF32(1.0f);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	arrayHandle.popIndex();
}



void ClothingAuthoring::removeMaterialFromLibrary(const char* libName, const char* matName)
{
	// here the real fun starts
	CurrentState::tMaterialLibraries::iterator found = mState.materialLibraries.find(libName);
	if (found == mState.materialLibraries.end())
	{
		return;
	}

	NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;

	NxParameterized::Interface* materialLibrary = found->second;
	NxParameterized::Handle arrayHandle(materialLibrary);
	error = arrayHandle.getParameter("materials");
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	int numMaterials = 0;
	error = arrayHandle.getArraySize(numMaterials);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);

	int deleteIndex = -1;

	for (int i = 0; i < numMaterials; i++)
	{
		error = arrayHandle.set(i);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		NxParameterized::Handle stringHandle(materialLibrary);
		error = arrayHandle.getChildHandle(materialLibrary, "materialName", stringHandle);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
		const char* currentMaterialName = NULL;
		error = stringHandle.getParamString(currentMaterialName);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		if (strcmp(currentMaterialName, matName) == 0)
		{
			PX_ASSERT(deleteIndex == -1); // should only find 1!!
			deleteIndex = i;
		}

		arrayHandle.popIndex();
	}

	if (deleteIndex == -1)
	{
		return;
	}


	// we should remove the deleteIndex item!

#if 1 // replace with last
	if (deleteIndex < numMaterials - 1)
	{
		error = arrayHandle.swapArrayElements(deleteIndex, numMaterials - 1);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
	}
#else // erase
	for (int i = deleteIndex + 1; i < numMaterials; i++)
	{
		error = arrayHandle.swapArrayElements(i - 1, i);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
	}
#endif
	error = arrayHandle.resizeArray(numMaterials - 1);
	PX_ASSERT(error == NxParameterized::ERROR_NONE);
}



void ClothingAuthoring::selectMaterial(const char* libName, const char* matName)
{
	mDirty.workspace |= mState.selectedMaterialLibrary != libName;
	mState.selectedMaterialLibrary = libName;
	mDirty.workspace |= mState.selectedMaterial != matName;
	mState.selectedMaterial = matName;
}



physx::PxBounds3 ClothingAuthoring::getCombinedSimulationBounds() const
{
	physx::PxBounds3 cummulatedBound;
	cummulatedBound.setEmpty();

	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		physx::PxVec3 actorPosition = mSimulation.actors[i].currentPose.getPosition();
		physx::PxBounds3 actorBound = mState.apexBounds;
		actorBound.minimum += actorPosition;
		actorBound.maximum += actorPosition;

		cummulatedBound.include(actorBound);
	}

	return cummulatedBound;
}



void ClothingAuthoring::clearMaterialLibraries()
{
	for (CurrentState::tMaterialLibraries::iterator it = mState.materialLibraries.begin(); it != mState.materialLibraries.end(); ++it)
	{
		if (it->second != NULL)
		{
			it->second->destroy();
		}
	}
	mState.materialLibraries.clear();
}



NxParameterized::Interface* ClothingAuthoring::getMaterialLibrary(unsigned int libraryIndex) const
{
	NxParameterized::Interface* result = NULL;
	if (libraryIndex < mState.materialLibraries.size())
	{
		unsigned int i = 0;
		for (CurrentState::tMaterialLibraries::const_iterator it = mState.materialLibraries.begin(); it != mState.materialLibraries.end(); ++it, ++i)
		{
			if (libraryIndex == i)
			{
				result = it->second;
				break;
			}
		}
	}

	return result;
}



bool ClothingAuthoring::setMaterialLibrary(unsigned int libraryIndex, NxParameterized::Interface* data)
{
	if (::strcmp(data->className(), "ClothingMaterialLibraryParameters") != 0)
	{
		return false;
	}

	if (libraryIndex < mState.materialLibraries.size())
	{
		unsigned int i = 0;
		for (CurrentState::tMaterialLibraries::iterator it = mState.materialLibraries.begin(); it != mState.materialLibraries.end(); ++it, ++i)
		{
			if (libraryIndex == i)
			{
				NxParameterized::Interface* oldData = it->second;
				it->second = data;

				// hopefully this is not used by any asset?
				oldData->destroy();
				return true;
			}
		}
	}
	return false;
}



const char* ClothingAuthoring::getMaterialLibraryName(unsigned int libraryIndex) const
{
	const char* result = NULL;
	if (libraryIndex < mState.materialLibraries.size())
	{
		unsigned int i = 0;
		for (CurrentState::tMaterialLibraries::const_iterator it = mState.materialLibraries.begin(); it != mState.materialLibraries.end(); ++it, ++i)
		{
			if (libraryIndex == i)
			{
				result = it->first.c_str();
				break;
			}
		}
	}

	return result;
}



unsigned int ClothingAuthoring::getNumMaterials(unsigned int libraryIndex) const
{
	NxParameterized::Interface* materialLibrary = getMaterialLibrary(libraryIndex);

	if (materialLibrary != NULL)
	{
		NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
		NxParameterized::Handle handle(materialLibrary);

		error = handle.getParameter("materials");
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		int numMaterials = 0;
		error = handle.getArraySize(numMaterials);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
		PX_ASSERT(numMaterials >= 0);

		return numMaterials;
	}

	return 0;
}



const char* ClothingAuthoring::getMaterialName(unsigned int libraryIndex, unsigned int materialIndex) const
{
	NxParameterized::Interface* materialLibrary = getMaterialLibrary(libraryIndex);

	if (materialLibrary != NULL)
	{
		NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
		NxParameterized::Handle handle(materialLibrary);

		error = handle.getParameter("materials");
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		int numMaterials = 0;
		error = handle.getArraySize(numMaterials);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);
		PX_ASSERT(numMaterials >= 0);

		if (materialIndex < (unsigned int)numMaterials)
		{
			error = handle.set(materialIndex);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			NxParameterized::Handle nameHandle(materialLibrary);
			error = handle.getChildHandle(materialLibrary, "materialName", nameHandle);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			const char* materialName = NULL;
			error = nameHandle.getParamString(materialName);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			return materialName;
		}
	}

	return NULL;
}



const physx::PxMat44& ClothingAuthoring::getTriangleMeshPose(size_t index) const
{
	PX_ASSERT(index < mSimulation.actors.size());
	return mSimulation.actors[index].currentPose;
}



physx::general_shared3::PxMat34Legacy& ClothingAuthoring::getTriangleMeshPoseOldFormat(size_t index)
{
	PX_ASSERT(index < mSimulation.actors.size());
	mSimulation.actors[index].currentPoseOldFormat = mSimulation.actors[index].currentPose;
	return mSimulation.actors[index].currentPoseOldFormat;
}



size_t ClothingAuthoring::getNumSimulationActors() const
{
	size_t count = 0;
	for (size_t actorIndex = 0; actorIndex < mSimulation.actors.size(); ++actorIndex)
	{
		count += mSimulation.actors[actorIndex].actors.size();
	}
	return count;
}



physx::apex::NxClothingActor* ClothingAuthoring::getSimulationActor(size_t index)
{
	size_t actorIndex = index / mSimulation.assets.size();
	size_t assetIndex = index % mSimulation.assets.size();
	if (actorIndex < mSimulation.actors.size())
	{
		if (assetIndex < mSimulation.actors[actorIndex].actors.size())
		{
			return mSimulation.actors[actorIndex].actors[assetIndex];
		}
	}

	return NULL;
}



void ClothingAuthoring::initMeshSkinningData()
{
	for (size_t assetIdx = 0; assetIdx < mSimulation.assets.size(); ++assetIdx)
	{
		if (mSimulation.assets[assetIdx].meshSkinningMaps.size() > 0)
			continue;

		physx::apex::NxClothingAsset* clothingAsset = mSimulation.assets[assetIdx].apexAsset;
		if (clothingAsset != NULL)
		{
			PxU32 numLods = clothingAsset->getNumGraphicalLodLevels();
			mSimulation.assets[assetIdx].meshSkinningMaps.resize(numLods);
			mSimulation.assets[assetIdx].renderMeshAssets.resize(numLods);
			for (PxU32 lod = 0; lod < numLods; ++lod)
			{
				physx::PxU32 numMapEntries = clothingAsset->getMeshSkinningMapSize(lod);

				mSimulation.assets[assetIdx].meshSkinningMaps[lod].resize(numMapEntries);
				if (numMapEntries > 0)
				{
					clothingAsset->getMeshSkinningMap(lod, &mSimulation.assets[assetIdx].meshSkinningMaps[lod][0]);
				}

				const physx::apex::NxRenderMeshAsset* renderMeshAsset = clothingAsset->getRenderMeshAsset(lod);

				if (renderMeshAsset != NULL)
				{
					const NxParameterized::Interface* rmaParams = renderMeshAsset->getAssetNxParameterized();
					NxParameterized::Interface* rmaParamsCopy = NULL;
					rmaParams->clone(rmaParamsCopy);
					char name[20];
					sprintf(name, "rma_%i_%i", mSimulation.assets.size(), lod);
					mSimulation.assets[assetIdx].renderMeshAssets[lod] = (physx::apex::NxRenderMeshAsset*)_mApexSDK->createAsset(rmaParamsCopy, name);
				}
			}
		}
	}
}



physx::apex::ClothingMeshSkinningMap* ClothingAuthoring::getMeshSkinningMap(size_t index, physx::PxU32 lod, physx::PxU32& mapSize, physx::apex::NxRenderMeshAsset*& renderMeshAsset)
{
	size_t assetIndex = index % mSimulation.assets.size();

	physx::apex::ClothingMeshSkinningMap* map = NULL;

	mapSize = (PxU32)mSimulation.assets[assetIndex].meshSkinningMaps[lod].size();
	if (mapSize > 0)
	{
		map = &mSimulation.assets[assetIndex].meshSkinningMaps[lod][0];
	}
	renderMeshAsset = mSimulation.assets[assetIndex].renderMeshAssets[lod];
	return map;
}



physx::apex::NxApexRenderable* ClothingAuthoring::getSimulationRenderable(size_t index)
{
	size_t actorIndex = index / mSimulation.assets.size();
	size_t assetIndex = index % mSimulation.assets.size();
	if (actorIndex < mSimulation.actors.size())
	{
		if (assetIndex < mSimulation.actors[actorIndex].actors.size())
		{
			return mSimulation.actors[actorIndex].actors[assetIndex];
		}
		else if (assetIndex < mSimulation.actors[actorIndex].previews.size())
		{
			return mSimulation.actors[actorIndex].previews[assetIndex];
		}
	}

	return NULL;
}



void ClothingAuthoring::setActorCount(int count, bool addToCommandQueue)
{
	if (addToCommandQueue)
	{
		char buf[64];
		sprintf_s(buf, 64, "setActorCount %d", count);
		addCommand(buf);
	}
	if (mSimulation.assets.empty())
	{
		mSimulation.actorCount = count;
	}
	else if (mSimulation.actorCount != count)
	{
		stopSimulation();
		mSimulation.actorCount = count;

		// create instances
		PX_ASSERT(mSimulation.assets.size() > 0);

		NxParameterized::Interface* actorDesc = mSimulation.assets[0].apexAsset->getDefaultActorDesc();
		PX_ASSERT(actorDesc != NULL);

		NxParameterized::Interface* previewDesc = mSimulation.assets[0].apexAsset->getDefaultAssetPreviewDesc();
		PX_ASSERT(previewDesc != NULL);


		NxParameterized::setParamBool(*actorDesc, "useHardwareCloth", mConfig.simulation.gpuSimulation);
		NxParameterized::setParamBool(*actorDesc, "flags.ParallelCpuSkinning", mConfig.apex.parallelCpuSkinning);
		NxParameterized::setParamBool(*actorDesc, "flags.RecomputeNormals", mConfig.apex.recomputeNormals);
		NxParameterized::setParamBool(*actorDesc, "flags.RecomputeTangents", mConfig.apex.recomputeTangents);
		NxParameterized::setParamBool(*actorDesc, "flags.CorrectSimulationNormals", mConfig.apex.correctSimulationNormals);
		NxParameterized::setParamF32(*actorDesc, "maxDistanceBlendTime", mConfig.simulation.blendTime);
		NxParameterized::setParamF32(*actorDesc, "pressure", mConfig.simulation.pressure);
		NxParameterized::setParamBool(*actorDesc, "fallbackSkinning", mConfig.simulation.fallbackSkinning);
		NxParameterized::setParamBool(*actorDesc, "useInternalBoneOrder", true);
		NxParameterized::setParamBool(*actorDesc, "updateStateWithGlobalMatrices", mConfig.animation.useGlobalPoseMatrices);
		NxParameterized::setParamBool(*actorDesc, "allowAdaptiveTargetFrequency", true);
		NxParameterized::setParamBool(*actorDesc, "localSpaceSim", mConfig.simulation.localSpaceSim);
		NxParameterized::setParamBool(*actorDesc, "multiplyGlobalPoseIntoBones", !mConfig.animation.applyGlobalPoseInApp);

		// LoD
		NxParameterized::setParamF32(*actorDesc, "lodWeights.maxDistance", 0.0f);
		//NxParameterized::setParamF32(*actorDesc, "lodWeights.maxDistance", 30.0f * mState.simulationValuesScale);
		NxParameterized::setParamF32(*actorDesc, "lodWeights.distanceWeight", 1.0f);
		NxParameterized::setParamF32(*actorDesc, "lodWeights.bias", 0.0f);
		NxParameterized::setParamF32(*actorDesc, "lodWeights.benefitsBias", 0.0f);

		// max distance scale
		NxParameterized::setParamF32(*actorDesc, "maxDistanceScale.Scale", mConfig.painting.maxDistanceScale);
		NxParameterized::setParamBool(*actorDesc, "maxDistanceScale.Multipliable", mConfig.painting.maxDistanceScaleMultipliable);

#if NX_SDK_VERSION_MAJOR == 2
		if (mConfig.apex.forceEmbedded)
		{
			NxParameterized::setParamEnum(*actorDesc, "simulationBackend", "ForceEmbedded");
		}
#endif

		if (mConfig.apex.useMorphTargetTest)
		{
			float scale = mState.simulationValuesScale / 10.0f;
			physx::PxVec3 positions[3];
			positions[0] = physx::PxVec3(scale, 0.0f, 0.0f);
			positions[1] = physx::PxVec3(0.0f, scale, 0.0f);
			positions[2] = physx::PxVec3(0.0f, 0.0f, scale);

			NxParameterized::Handle morphDisp(*actorDesc);
			morphDisp.getParameter("morphDisplacements");
			morphDisp.resizeArray(3);

			for (int i = 0; i < 3; i++)
			{
				morphDisp.set(i);
				morphDisp.setParamVec3(positions[i]);
				morphDisp.popIndex();
			}

			for (int i = 0; i < 3; i++)
			{
				positions[i] += mState.apexBounds.getCenter();
			}

			for (size_t i = 0; i < mSimulation.assets.size(); i++)
			{
				mSimulation.assets[i].apexAsset->prepareMorphTargetMapping(positions, 3, 10);
			}
		}

		if (getMeshSkinningInApp())
		{
			for (size_t i = 0; i < mSimulation.assets.size(); i++)
			{
				if (mSimulation.actors.size() == 0)
				{
					initMeshSkinningData();
					mSimulation.assets[i].apexAsset->releaseGraphicalData();
				}
			}
		}

		// preview
		NxParameterized::setParamBool(*previewDesc, "useInternalBoneOrder", true);
		NxParameterized::setParamBool(*previewDesc, "updateStateWithGlobalMatrices", mConfig.animation.useGlobalPoseMatrices);

		const float width = std::max(mState.apexBounds.maximum.x - mState.apexBounds.minimum.x,
		                             std::max(mState.apexBounds.maximum.y - mState.apexBounds.minimum.y, mState.apexBounds.maximum.z - mState.apexBounds.minimum.z));

		const physx::PxMat44* skinningMatrices = NULL;
		unsigned int numSkinningMatrices = 0;

		if (mMeshes.skeleton != NULL)
		{
			if (mConfig.animation.useGlobalPoseMatrices)
			{
				numSkinningMatrices = (int)mMeshes.skeleton->getSkinningMatricesWorld().size();
				skinningMatrices = numSkinningMatrices > 0 ? &mMeshes.skeleton->getSkinningMatricesWorld()[0] : NULL;
			}
			else
			{
				numSkinningMatrices = (int)mMeshes.skeleton->getSkinningMatrices().size();
				skinningMatrices = numSkinningMatrices > 0 ? &mMeshes.skeleton->getSkinningMatrices()[0] : NULL;
			}
		}


		std::vector<physx::PxMat44> scaledSkinningMatrices;
		if (mSimulation.actorCount > (int)mSimulation.actors.size())
		{
			for (int i = (int)mSimulation.actors.size(); i < mSimulation.actorCount; i++)
			{
				Simulation::ClothingActor clothingActor;

				clothingActor.scale = physx::PxPow(mConfig.simulation.scaleFactor, (float)(i + 1));
				NxParameterized::setParamF32(*actorDesc, "actorScale", clothingActor.scale);

				physx::PxVec3 pos(0.0f, 0.0f, 0.0f);

				const float index = i + 1.0f;
				const float squareRoot = physx::PxSqrt(index);
				const float roundedSquareRoot = physx::PxFloor(squareRoot);
				const float square = roundedSquareRoot * roundedSquareRoot;
				if (physx::PxAbs(index - square) < 0.01f)
				{
					pos.x = width * 2.0f * (squareRoot - 1);
					pos.z = -pos.x;
				}
				else if (squareRoot - roundedSquareRoot < 0.5f)
				{
					pos.x = width * 2.0f * roundedSquareRoot;
					pos.z = -(i - square) * width * 2.0f;
				}
				else
				{
					pos.z = -width * 2.0f * roundedSquareRoot;
					pos.x = (i - square - roundedSquareRoot) * width * 2.0f;
				}

				if (mConfig.ui.zAxisUp)
				{
					const float t = pos.y;
					pos.y = pos.z;
					pos.z = t;
				}

				clothingActor.initPose.setPosition(pos);

				physx::PxMat44 pose(physx::PxMat44::createIdentity());
				pose.setPosition(pos);
				clothingActor.currentPose = mSimulation.CCTPose * pose;

				clothingActor.currentPose.scale(physx::PxVec4(clothingActor.scale, clothingActor.scale, clothingActor.scale, 1.0f));
				NxParameterized::setParamMat44(*actorDesc, "globalPose", clothingActor.currentPose);
				NxParameterized::setParamMat44(*previewDesc, "globalPose", clothingActor.currentPose);

				for (size_t j = 0; j < mSimulation.assets.size(); j++)
				{
					std::vector<short>& remapTable = mSimulation.assets[j].remapToSkeleton;
					if (scaledSkinningMatrices.size() < remapTable.size())
					{
						scaledSkinningMatrices.resize(remapTable.size());
					}

					for (size_t k = 0; k < remapTable.size(); k++)
					{
						PX_ASSERT((unsigned int)remapTable[k] < numSkinningMatrices);
						scaledSkinningMatrices[k] = skinningMatrices[remapTable[k]];
					}

					const unsigned int numSkinningMatricesReal = (unsigned int)remapTable.size();
					physx::PxMat44* skinningMatricesReal = numSkinningMatricesReal > 0 ? &scaledSkinningMatrices[0] : NULL;

					if (mConfig.animation.applyGlobalPoseInApp)
					{
						if(numSkinningMatricesReal)
						{
							PX_ASSERT(skinningMatricesReal);
						}
						for (size_t k = 0; k < numSkinningMatricesReal; k++)
						{
							skinningMatricesReal[k] = clothingActor.currentPose * skinningMatricesReal[k];
						}
					}

					if (skinningMatricesReal != NULL)
					{
						NxParameterized::Handle actorHandle(actorDesc);
						VERIFY_PARAM(actorHandle.getParameter("boneMatrices"));
						VERIFY_PARAM(actorHandle.resizeArray(numSkinningMatricesReal));
						VERIFY_PARAM(actorHandle.setParamMat44Array(skinningMatricesReal, numSkinningMatricesReal));

						NxParameterized::Handle previewHandle(previewDesc);
						VERIFY_PARAM(previewHandle.getParameter("boneMatrices"));
						VERIFY_PARAM(previewHandle.resizeArray(numSkinningMatricesReal));
						VERIFY_PARAM(previewHandle.setParamMat44Array(skinningMatricesReal, numSkinningMatricesReal));
					}

					if (mConfig.simulation.usePreview)
					{
						physx::apex::NxApexAssetPreview* apexPreview = mSimulation.assets[j].apexAsset->createApexAssetPreview(*previewDesc, NULL);
						clothingActor.previews.push_back(static_cast<physx::apex::NxClothingPreview*>(apexPreview));
					}
					else
					{
						NxParameterized::Interface* desc = (mLoadedActorDescs.size() > 0) ? mLoadedActorDescs[0] : actorDesc;
						physx::apex::NxApexActor* actor = mSimulation.assets[j].apexAsset->createApexActor(*desc, *mSimulation.apexScene);
						physx::apex::NxClothingActor* cActor = static_cast<physx::apex::NxClothingActor*>(actor);

						cActor->setUserRecompute(mSimulation.userRecompute);
						clothingActor.actors.push_back(cActor);
					}
				}

				mSimulation.actors.push_back(clothingActor);
			}
		}
		else if (mSimulation.actorCount < (int)mSimulation.actors.size())
		{
			while ((int)mSimulation.actors.size() > mSimulation.actorCount)
			{
				Simulation::ClothingActor& clothingActor = mSimulation.actors.back();

				for (size_t j = 0; j < clothingActor.actors.size(); j++)
				{
					if (clothingActor.actors[j] != NULL)
					{
						clothingActor.actors[j]->release();
						clothingActor.actors[j] = NULL;
					}
				}
				for (size_t j = 0; j < clothingActor.previews.size(); j++)
				{
					if (clothingActor.previews[j] != NULL)
					{
						clothingActor.previews[j]->release();
						clothingActor.previews[j] = NULL;
					}
				}

				if (clothingActor.triangleMesh != NULL)
				{
					clothingActor.triangleMesh->clear(_mRenderResourceManager, _mResourceCallback);
					delete clothingActor.triangleMesh;
					clothingActor.triangleMesh = NULL;
				}

				mSimulation.actors.pop_back();
			}
		}
	}
}



float ClothingAuthoring::getActorScale(size_t index)
{
	size_t actorIndex = index / mSimulation.assets.size();
	if (actorIndex < mSimulation.actors.size())
	{
		return mSimulation.actors[actorIndex].scale;
	}

	return 1.0f;
}



void ClothingAuthoring::setMatrices(const physx::PxMat44& viewMatrix, const physx::PxMat44& projectionMatrix)
{
	if (mSimulation.apexScene != NULL)
	{
		mSimulation.apexScene->setViewMatrix(viewMatrix, 0);
		mSimulation.apexScene->setProjMatrix(projectionMatrix, 0);
		mSimulation.apexScene->setUseViewProjMatrix(0, 0);
	}
}



void ClothingAuthoring::startCreateApexScene()
{
	stopSimulation();

	if (mSimulation.apexScene == NULL)
	{
		return;
	}

	mConfig.simulation.budgetPercent = 100;

	for (size_t i = 0; i < mSimulation.assets.size(); i++)
	{
		mSimulation.assets[i].releaseRenderMeshAssets();
		mSimulation.assets[i].apexAsset->release();
		mSimulation.assets[i].apexAsset = NULL;
	}
	mSimulation.assets.clear();
	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		if (mSimulation.actors[i].triangleMesh != NULL)
		{
			mSimulation.actors[i].triangleMesh->clear(_mRenderResourceManager, _mResourceCallback);
			delete mSimulation.actors[i].triangleMesh;
			mSimulation.actors[i].triangleMesh = NULL;
		}
	}
	mSimulation.actors.clear(); // deleted when releasing all the assets :)

	if (mSimulation.physxScene != NULL)
	{
		mSimulation.apexScene->setPhysXScene(NULL);

#if NX_SDK_VERSION_MAJOR == 2
		_mApexSDK->getPhysXSDK()->releaseScene(*mSimulation.physxScene);
#elif NX_SDK_VERSION_MAJOR == 3
		mSimulation.physxScene->release();
#endif

		mSimulation.physxScene = NULL;
		mSimulation.groundPlane = NULL;
	}


	{
		NxParameterized::Interface* debugRendering = mSimulation.apexScene->getDebugRenderParams();

		NxParameterized::setParamBool(*debugRendering, "Enable", true);
		NxParameterized::setParamF32(*debugRendering, "Scale", 1.0f);
	}

	{
#if NX_SDK_VERSION_MAJOR == 2
		NxSceneDesc sceneDesc;
		sceneDesc.maxTimestep = 1.0f / ClothingAuthoring::getSimulationFrequency();

		sceneDesc.flags &= ~NX_SF_SEQUENTIAL_PRIMARY;
		//sceneDesc.flags |= NX_SF_SEQUENTIAL_PRIMARY;
		sceneDesc.gravity = NxVec3(0.0f);
		sceneDesc.gravity[mConfig.ui.zAxisUp ? 2 : 1] = -0.1f; // will be set to the right value later, we just need the right direction for cooking

		mSimulation.physxScene = _mApexSDK->getPhysXSDK()->createScene(sceneDesc);
#elif NX_SDK_VERSION_MAJOR == 3

		physx::PxSceneDesc sceneDesc(_mApexSDK->getPhysXSDK()->getTolerancesScale());

		if (mSimulation.cudaContextManager != NULL)
		{
			sceneDesc.gpuDispatcher = mSimulation.cudaContextManager->getGpuDispatcher();
		}
		sceneDesc.flags|=physx::PxSceneFlag::eREQUIRE_RW_LOCK;
		sceneDesc.cpuDispatcher = mSimulation.cpuDispatcher;
		sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

		sceneDesc.gravity = physx::PxVec3(0.0f);
		sceneDesc.gravity[mConfig.ui.zAxisUp ? 2 : 1] = -0.1f; // will be set to the right value later, we just need the right direction for cooking

		mSimulation.physxScene = _mApexSDK->getPhysXSDK()->createScene(sceneDesc);
#endif
	}


	// turn on debug rendering
#if NX_SDK_VERSION_MAJOR == 2
	_mApexSDK->getPhysXSDK()->setParameter(NX_VISUALIZATION_SCALE, 1.0f);
#elif NX_SDK_VERSION_MAJOR == 3
	mSimulation.physxScene->lockWrite(__FILE__, __LINE__);
	mSimulation.physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f);
	mSimulation.physxScene->unlockWrite();
#endif

	mSimulation.apexScene->setPhysXScene(mSimulation.physxScene);

	// connect PVD
	connectPVD(false);

	if (mMeshes.inputMesh != NULL)
	{
		for (int i = 0; i < (int)mMeshes.inputMesh->getNumSubmeshes(); i++)
		{
			mMeshes.inputMesh->setSubMeshHasPhysics(i, mMeshes.inputMesh->getSubMesh(i)->selected);
		}
	}
}



bool ClothingAuthoring::addAssetToScene(physx::apex::NxClothingAsset* clothingAsset, physx::apex::IProgressListener* progress, unsigned int totalNumAssets)
{
	if (mSimulation.apexScene == NULL)
	{
		return false;
	}

	mSimulation.assets.reserve(totalNumAssets);

	if (clothingAsset != NULL)
	{
		PX_ASSERT(!mSimulation.clearAssets || mSimulation.assets.empty());
		mSimulation.clearAssets = false;
		mSimulation.assets.push_back(clothingAsset);
	}
	else
	{
		PX_ASSERT(mSimulation.assets.empty());
		mSimulation.clearAssets = true;
		physx::apex::NxClothingAssetAuthoring* authoringAsset = getClothingAsset(progress);
		if (authoringAsset != NULL)
		{
#if 1
			physx::apex::NxApexAsset* asset = _mApexSDK->createAsset(*authoringAsset, "TheSimulationAsset");
#else
			static bool flip = true;
			if (flip)
			{
				physx::PxMat44 mat = physx::PxMat44::identity();
				mat(1, 1) = -1;
				authoringAsset->applyTransformation(mat, 1.0f, true, true);
			}
			// PH: test the releaseAndReturnNxParameterizedInterface() method
			PX_ASSERT(mAuthoringObjects.clothingAssetAuthoring == authoringAsset);
			NxParameterized::Interface* authoringInterface = authoringAsset->releaseAndReturnNxParameterizedInterface();

			physx::apex::NxApexAsset* asset = NULL;
			if (authoringInterface != NULL)
			{
				mAuthoringObjects.clothingAssetAuthoring = authoringAsset = NULL;
				asset = _mApexSDK->createAsset(authoringInterface, "TheSimulationAsset");
			}
#endif
			if (asset != NULL)
			{
				if (strcmp(asset->getObjTypeName(), NX_CLOTHING_AUTHORING_TYPE_NAME) == 0)
				{
					clothingAsset = static_cast<physx::apex::NxClothingAsset*>(asset);
					mSimulation.assets.push_back(clothingAsset);
				}
				else
				{
					_mApexSDK->releaseAsset(*asset);
				}
			}
		}
	}

	if (mMeshes.skeleton != NULL && clothingAsset != NULL)
	{
		const unsigned int numBones = clothingAsset->getNumUsedBones();
		std::vector<short> remap;
		remap.resize(numBones, -1);

		const std::vector<Samples::SkeletalBone>& bones = mMeshes.skeleton->getBones();

		for (unsigned int i = 0; i < numBones; i++)
		{
			const char* boneName = clothingAsset->getBoneName(i);
			for (unsigned int j = 0; j < bones.size(); j++)
			{
				if (bones[j].name == boneName)
				{
					remap[i] = (short)j;
					break;
				}
			}
		}

		mSimulation.assets.back().remapToSkeleton.swap(remap);
	}

	if (clothingAsset != NULL)
	{
		PxU32 numLods = clothingAsset->getNumGraphicalLodLevels();
		mSimulation.assets.back().meshSkinningMaps.resize(numLods);
		mSimulation.assets.back().renderMeshAssets.resize(numLods);
		for (PxU32 lod = 0; lod < numLods; ++lod)
		{
			physx::PxU32 numMapEntries = clothingAsset->getMeshSkinningMapSize(lod);

			mSimulation.assets.back().meshSkinningMaps[lod].resize(numMapEntries);
			if (numMapEntries > 0)
			{
				clothingAsset->getMeshSkinningMap(lod, &mSimulation.assets.back().meshSkinningMaps[lod][0]);
			}

			
			const physx::apex::NxRenderMeshAsset* renderMeshAsset = clothingAsset->getRenderMeshAsset(lod);

			if (renderMeshAsset != NULL)
			{
				const NxParameterized::Interface* rmaParams = renderMeshAsset->getAssetNxParameterized();
				NxParameterized::Interface* rmaParamsCopy = NULL;
				rmaParams->clone(rmaParamsCopy);
				char name[20];
				sprintf(name, "rma_%i_%i", mSimulation.assets.size(), lod);
				mSimulation.assets.back().renderMeshAssets[lod] = (physx::apex::NxRenderMeshAsset*)_mApexSDK->createAsset(rmaParamsCopy, name);
			}
		}

#if NX_SDK_VERSION_MAJOR == 2
		mConfig.simulation.localSpaceSim = clothingAsset->getClothSolverMode() == physx::apex::ClothSolverMode::v3;
#else
		mConfig.simulation.localSpaceSim = true;
#endif
	}

	return clothingAsset != NULL;
}


void ClothingAuthoring::handleGravity()
{
	if (getAndClearGravityDirty() && mSimulation.physxScene != NULL)
	{
		const float scale = mState.gravityValueScale != 0 ? mState.gravityValueScale : mState.simulationValuesScale;
#if NX_SDK_VERSION_MAJOR == 2
		if (mConfig.ui.zAxisUp)
		{
			mSimulation.physxScene->setGravity(NxVec3(0.0f, 0.0f, -scale * mConfig.simulation.gravity));
		}
		else
		{
			mSimulation.physxScene->setGravity(NxVec3(0.0f, -scale * mConfig.simulation.gravity, 0.0f));
		}
#elif NX_SDK_VERSION_MAJOR == 3
		physx::PxVec3 gravity(0.0f);
		gravity[mConfig.ui.zAxisUp ? 2 : 1] = -scale * mConfig.simulation.gravity;

		mSimulation.physxScene->lockWrite(__FILE__, __LINE__);
		mSimulation.physxScene->setGravity(gravity);
		mSimulation.physxScene->unlockWrite();
#endif
	}
}


void ClothingAuthoring::finishCreateApexScene(bool recomputeScale)
{
	if (mSimulation.apexScene == NULL)
	{
		return;
	}

	int oldActorCount = std::max(1, mSimulation.actorCount);
	mSimulation.actorCount = 0;

	float time = 0.0f;
	if (mMeshes.skeleton != NULL && !mMeshes.skeleton->getAnimations().empty())
	{
		Samples::SkeletalAnimation* anim = mMeshes.skeleton->getAnimations()[0];
		clampAnimation(time, false, false, anim->minTime, anim->maxTime);
	}
	setAnimationTimes(time);

	updateAnimation();

	mState.apexBounds.setEmpty();

	if (mMeshes.inputMesh != NULL)
	{
		mMeshes.inputMesh->getBounds(mState.apexBounds);
	}

	for (size_t i = 0; i < mSimulation.assets.size(); i++)
	{
		mState.apexBounds.include(mSimulation.assets[i].apexAsset->getBoundingBox());
	}

	if (mState.gravityValueScale != 0.0f)
	{
		// loaded from a .ctp
		mState.simulationValuesScale = mState.gravityValueScale;
	}
	else if (recomputeScale)
	{
		// loaded without a .ctp, directly an .apx or .apb
		float scale = 0.5f * (mState.apexBounds.minimum - mState.apexBounds.maximum).magnitude();
		if (scale >= 0.5f && scale <= 2.0f)
		{
			scale = 1.0f;
		}

		mState.simulationValuesScale = scale;

		if (mState.apexBounds.minimum.y < mState.apexBounds.minimum.z * 10)
		{
			setZAxisUp(true);
		}
		else if (mState.apexBounds.minimum.z < mState.apexBounds.minimum.y * 10)
		{
			setZAxisUp(false);
		}

		for (size_t i = 0; i < _mNotifyCallbacks.size(); i++)
		{
			_mNotifyCallbacks[i]->notifyInputMeshLoaded();
		}
	}

	if (mLoadedActorDescs.size() > 0)
	{
		physx::PxMat44 globalPose;
		if (NxParameterized::getParamMat44(*mLoadedActorDescs[0], "globalPose", globalPose))
		{
			for (size_t i = 0; i < _mNotifyCallbacks.size(); i++)
			{
				_mNotifyCallbacks[i]->notifyMeshPos(mState.apexBounds.getCenter() + globalPose.getPosition());
			}
		}
	}

	mSimulation.CCTPose = physx::PxMat44::createIdentity();
	mSimulation.CCTDirection = physx::PxVec3(0.0f);
	mSimulation.CCTRotationDelta = physx::PxVec3(0.0f);

	mDirty.gravity = true;
	mDirty.groundPlane = true;

	mSimulation.paused = false;
	mSimulation.frameNumber = 0;
	PX_ASSERT(mSimulation.running == false);

	// init gravity before actor creation, in case it's recooking the asset
	handleGravity();

	setActorCount(oldActorCount, false); // creates the actors and computes mApexBounds
}



#define TELEPORT_TEST 0

void ClothingAuthoring::startSimulation()
{
	if (mSimulation.assets.empty() || mSimulation.running)
	{
		return;
	}

	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		for (size_t j = 0; j < mSimulation.actors[i].actors.size(); j++)
		{
			PX_ASSERT(mSimulation.actors[i].actors[j] != NULL);
		}
	}

	if (getAndClearNeedsRestart())
	{
		restartSimulation();    // using setActorCount twice
	}

	if (mSimulation.stepsUntilPause > 0)
	{
		mSimulation.paused = false;
		if (mConfig.animation.selectedAnimation >= 0)
		{
			mSimulation.stepsUntilPause--;

			if (mSimulation.stepsUntilPause == 0)
			{
				setAnimation(-mConfig.animation.selectedAnimation);
				mSimulation.paused = true;
			}
		}
		else
		{
			mSimulation.stepsUntilPause = 0;
		}
	}

	if (mSimulation.paused)
	{
		return;
	}

	float deltaT = 1.0f / mConfig.simulation.frequency;
	if (mConfig.simulation.timingNoise != 0.0f)
	{
		const float randomValue = (::rand() / (float)RAND_MAX);
		deltaT += randomValue * deltaT * mConfig.simulation.timingNoise;
	}

//	const bool updatedCCT = updateCCT(deltaT);

	float maxBudget = 0;

	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		for (size_t j = 0; j < mSimulation.actors[i].actors.size(); j++)
		{
			if (mSimulation.actors[i].actors[j] != NULL)
			{
				maxBudget += mSimulation.actors[i].actors[j]->getMaximumSimulationBudget();
			}
		}
	}
	maxBudget *= 1.01f;

	if (mSimulation.apexScene != NULL)
	{
		mSimulation.apexScene->setLODResourceBudget((maxBudget * mConfig.simulation.budgetPercent) / 100.0f);
	}

	// inter-collision settings
	// removed from 1.3
//	_mModuleClothing->setInterCollisionDistance(mConfig.simulation.interCollisionDistance);
//	_mModuleClothing->setInterCollisionStiffness(mConfig.simulation.interCollisionStiffness);
//	_mModuleClothing->setInterCollisionIterations(mConfig.simulation.interCollisionIterations);

	stepAnimationTimes(mConfig.animation.speed * deltaT / 100.f);

	bool animationJumped = updateAnimation();

	const physx::PxMat44* skinningMatrices = NULL;
	unsigned int numSkinningMatrices = 0;

	if (mMeshes.skeleton != NULL)
	{
		if (mConfig.animation.useGlobalPoseMatrices)
		{
			numSkinningMatrices = (int)mMeshes.skeleton->getSkinningMatricesWorld().size();
			skinningMatrices = numSkinningMatrices > 0 ? &mMeshes.skeleton->getSkinningMatricesWorld()[0] : NULL;
		}
		else
		{
			numSkinningMatrices = (int)mMeshes.skeleton->getSkinningMatrices().size();
			skinningMatrices = numSkinningMatrices > 0 ? &mMeshes.skeleton->getSkinningMatrices()[0] : NULL;
		}
	}

	std::vector<physx::PxMat44> scaledSkinningMatrices;

	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
#if !TELEPORT_TEST
		mSimulation.actors[i].currentPose = mSimulation.CCTPose * mSimulation.actors[i].initPose;
		physx::PxF32 actorScale = mSimulation.actors[i].scale;
		mSimulation.actors[i].currentPose.scale(physx::PxVec4(actorScale, actorScale, actorScale, 1.0f));
#endif

		PX_ASSERT(mSimulation.actors[i].actors.size() == mSimulation.assets.size() || mSimulation.actors[i].previews.size() == mSimulation.assets.size());

		for (size_t j = 0; j < mSimulation.assets.size(); j++)
		{
			std::vector<short>& remapTable = mSimulation.assets[j].remapToSkeleton;
			if (scaledSkinningMatrices.size() < remapTable.size())
			{
				scaledSkinningMatrices.resize(remapTable.size());
			}

			for (size_t k = 0; k < remapTable.size(); k++)
			{
				PX_ASSERT((unsigned int)remapTable[k] < numSkinningMatrices);
				scaledSkinningMatrices[k] = skinningMatrices[remapTable[k]]/* * mSimulation.actors[i].scale*/;
			}

			const unsigned int numSkinningMatricesReal = (unsigned int)remapTable.size();
			physx::PxMat44* skinningMatricesReal = numSkinningMatricesReal > 0 ? &scaledSkinningMatrices[0] : NULL;

			if (mConfig.animation.applyGlobalPoseInApp)
			{
				for (size_t k = 0; k < numSkinningMatricesReal; k++)
				{
					skinningMatricesReal[k] = mSimulation.actors[i].currentPose * skinningMatricesReal[k];
				}
			}

			if (j < mSimulation.actors[i].previews.size())
			{
				mSimulation.actors[i].previews[j]->updateState(mSimulation.actors[i].currentPose, skinningMatricesReal, sizeof(physx::PxMat44), numSkinningMatricesReal);
			}

			if (j < mSimulation.actors[i].actors.size())
			{
				physx::apex::NxClothingActor* actor = mSimulation.actors[i].actors[j];
				PX_ASSERT(actor != NULL);

				actor->setGraphicalLOD(mConfig.simulation.graphicalLod);

				physx::apex::ClothingTeleportMode::Enum teleportMode = !animationJumped ?
				        physx::apex::ClothingTeleportMode::Continuous : physx::apex::ClothingTeleportMode::TeleportAndReset;

#if TELEPORT_TEST
				// Test teleport feature, make sure to also comment line 1633!! (above where the CCT position is set every frame, it ends in " + mSimulation.CCTPosition);")
				static float timeout = 2.0f;
				static int changeIndex = 0;
				if (timeout < 0.0f)
				{
					timeout = 2.0f;

					physx::PxVec3 upAxis(0.0f);
					upAxis[mConfig.ui.zAxisUp ? 2 : 1] = 1.0f;

					physx::PxVec3 pos1(1.0f * mState.simulationValuesScale, 0.0f, 0.0f);
					physx::PxQuat rot1(physx::degToRad( 90.0f), upAxis);
					physx::PxVec3 pos2(5.0f * mState.simulationValuesScale, 0.0f, 0.0f);
					physx::PxQuat rot2(physx::degToRad(-45.0f), upAxis);
					physx::PxMat44 globalPoses[2] =
					{
						physx::PxMat44(physx::PxMat33(rot1) * mSimulation.actors[i].scale, pos1),
						physx::PxMat44(physx::PxMat33(rot2) * mSimulation.actors[i].scale, pos2)
					};

					//mSimulation.stepsUntilPause = 2;
					mSimulation.actors[i].currentPose = globalPoses[changeIndex];
					changeIndex = 1 - changeIndex;

					teleportMode = physx::apex::ClothingTeleportMode::Teleport;
				}
				timeout -= 1.0f / 50.0f;
#endif
				actor->updateState(mSimulation.actors[i].currentPose, skinningMatricesReal,
				                   sizeof(physx::PxMat44), numSkinningMatricesReal, teleportMode);

				NxParameterized::Interface* actorDesc = actor->getActorDesc();
				NxParameterized::Handle handle(actorDesc);

				// update wind
				if (mConfig.simulation.windVelocity <= 0.0f)
				{
					mState.windOrigin = physx::PxVec3(0.0f);
					mState.windTarget = physx::PxVec3(0.0f);
					mState.drawWindTime = 0.0f;

					NxParameterized::setParamVec3(*actorDesc, "windParams.Velocity", physx::PxVec3(0.0f));
					NxParameterized::setParamF32(*actorDesc, "windParams.Adaption", 0.0f);
				}
				else
				{
					if (getZAxisUp())
					{
						mState.windOrigin = physx::PxVec3(mConfig.simulation.windVelocity * mState.simulationValuesScale, 0.0f, 0.0f);
						physx::PxMat33Legacy rotation;
						rotation.setRotY(physx::degToRad((float) - mConfig.simulation.windElevation));
						mState.windOrigin = rotation * mState.windOrigin;
						rotation.setRotZ(physx::degToRad((float)mConfig.simulation.windDirection));
						mState.windOrigin = rotation * mState.windOrigin;
					}
					else
					{
						mState.windOrigin = physx::PxVec3(mConfig.simulation.windVelocity * mState.simulationValuesScale, 0.0f, 0.0f);
						physx::PxMat33Legacy rotation;
						rotation.setRotZ(physx::degToRad((float)mConfig.simulation.windElevation));
						mState.windOrigin = rotation * mState.windOrigin;
						rotation.setRotY(physx::degToRad((float)mConfig.simulation.windDirection));
						mState.windOrigin = rotation * mState.windOrigin;
					}
					mState.windTarget = mSimulation.actors[i].currentPose.getPosition();
					mState.windOrigin += mState.windTarget;

					NxParameterized::setParamVec3(*actorDesc, "windParams.Velocity", mState.windTarget - mState.windOrigin);
					NxParameterized::setParamF32(*actorDesc, "windParams.Adaption", 1.0f);

					// average the target
					mState.windTarget = 0.8f * mState.windTarget + 0.2f * mState.windOrigin;

					mState.drawWindTime = physx::PxMax(0.0f, mState.drawWindTime - deltaT);
				}

				actor->forcePhysicalLod((float)mConfig.simulation.lodOverwrite);

				if (mDirty.maxDistanceScale)
				{
					NxParameterized::setParamF32(*actorDesc, "maxDistanceScale.Scale", mConfig.painting.maxDistanceScale);
					NxParameterized::setParamBool(*actorDesc, "maxDistanceScale.Multipliable", mConfig.painting.maxDistanceScaleMultipliable);
				}

				if (mDirty.clothingActorFlags)
				{
					NxParameterized::setParamBool(*actorDesc, "flags.ParallelCpuSkinning", mConfig.apex.parallelCpuSkinning);
					NxParameterized::setParamBool(*actorDesc, "flags.RecomputeNormals", mConfig.apex.recomputeNormals);
					NxParameterized::setParamBool(*actorDesc, "flags.RecomputeTangents", mConfig.apex.recomputeTangents);
					NxParameterized::setParamBool(*actorDesc, "flags.CorrectSimulationNormals", mConfig.apex.correctSimulationNormals);
				}

				// disable skinning if done externally
				NxParameterized::setParamBool(*actorDesc, "flags.ComputeRenderData", !getMeshSkinningInApp());

				if (mDirty.blendTime)
				{
					NxParameterized::setParamF32(*actorDesc, "maxDistanceBlendTime", mConfig.simulation.blendTime);
				}

				if (mDirty.pressure)
				{
					NxParameterized::setParamF32(*actorDesc, "pressure", mConfig.simulation.pressure);
				}

				if (mCurrentActorDesc < mLoadedActorDescs.size())
				{
					actorDesc->copy(*mLoadedActorDescs[mCurrentActorDesc]);
					if (animationJumped)
					{
						// overwrite loaded setting because of tool navigation
						NxParameterized::setParamI32(*actorDesc, "teleportMode", 2);
					}
				}
			}
		}
	}

	mDirty.maxDistanceScale = mDirty.clothingActorFlags = mDirty.blendTime = mDirty.pressure = false;

	if (getAndClearGroundPlaneDirty() && mSimulation.physxScene != NULL)
	{
		if (mSimulation.groundPlane != NULL)
		{
#if NX_SDK_VERSION_MAJOR == 2
			mSimulation.physxScene->releaseActor(*mSimulation.groundPlane);
#elif NX_SDK_VERSION_MAJOR == 3
			mSimulation.groundPlane->release();
#endif

			mSimulation.groundPlane = NULL;
		}

		if (mConfig.simulation.groundplaneEnabled)
		{
#if NX_SDK_VERSION_MAJOR == 2
			NxPlaneShapeDesc planeShape;
			planeShape.normal = NxVec3(0.0f, mConfig.ui.zAxisUp ? 0.0f : 1.0f, mConfig.ui.zAxisUp ? 1.0f : 0.0f);
			planeShape.d = mConfig.simulation.groundplane * mState.simulationValuesScale * 0.1f;

			NxActorDesc actorDesc;
			actorDesc.shapes.pushBack(&planeShape);

			mSimulation.groundPlane = mSimulation.physxScene->createActor(actorDesc);
#elif NX_SDK_VERSION_MAJOR == 3
			// there's no world collision yet, so no point in making a ground plane
			/*
			mSimulation.physxMaterial = _mApexSDK->getPhysXSDK()->createMaterial(0.5f, 0.5f, 0.1f);     //static friction, dynamic friction, restitution

			physx::PxReal d = 0.0f;
			physx::PxU32 axis = 1;
			PxTransform pose;

			if(axis == 0)
				pose = PxTransform(physx::PxVec3(d, 0.0f, 0.0f));
			else if(axis == 1)
				pose = PxTransform(physx::PxVec3(0.0f, d, 0.0f),PxQuat(PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f)));
			else if(axis == 2)
				pose = PxTransform(physx::PxVec3(0.0f, 0.0f, d), PxQuat(-PxHalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f)));

			mSimulation.groundPlane = _mApexSDK->getPhysXSDK()->createRigidStatic(pose);
			physx::PxShape* shape = mSimulation.groundPlane->createShape(physx::PxPlaneGeometry(), *mSimulation.physxMaterial);
			mSimulation.physxScene->addActor(*mSimulation.groundPlane);
			*/

#endif
		}
	}

	handleGravity();

	if (mState.needsReconnect)
	{
		connectPVD(false);
		mState.needsReconnect = false;
	}


	// advance scene
	if (mSimulation.apexScene != NULL)
	{
#if 1

#if NX_SDK_VERSION_MAJOR == 2
		float maxTimestep = mCurrentActorDesc < mMaxTimesteps.size() ? mMaxTimesteps[mCurrentActorDesc] : deltaT;
		int maxIter = mCurrentActorDesc < mMaxIterations.size() ? mMaxIterations[mCurrentActorDesc] : 1;
		int timestepMethod = mCurrentActorDesc < mTimestepMethods.size() ? mTimestepMethods[mCurrentActorDesc] : NX_TIMESTEP_FIXED;
		mSimulation.physxScene->setTiming(maxTimestep, maxIter, (NxTimeStepMethod)timestepMethod);

#elif NX_SDK_VERSION_MAJOR == 3
		// no substepping for 3.0
#endif
		float dt = deltaT;
		if (mCurrentActorDesc < mDts.size())
		{
			dt = mDts[mCurrentActorDesc];
		}
		if (mCurrentActorDesc < mGravities.size())
		{

#if NX_SDK_VERSION_MAJOR == 2
			mSimulation.physxScene->setGravity(mGravities[mCurrentActorDesc]);
#else
			mSimulation.physxScene->lockWrite(__FILE__, __LINE__);
			mSimulation.physxScene->setGravity(mGravities[mCurrentActorDesc]);
			mSimulation.physxScene->unlockWrite();
#endif
		}
		mSimulation.apexScene->simulate(dt);

#else
		// The UE3 way of stepping
		int NumSubSteps = (int)::ceil(deltaT * mConfig.simulation.frequency);
		NumSubSteps = physx::PxClamp(NumSubSteps, 1, 5);
		float MaxSubstep = physx::PxClamp(deltaT / NumSubSteps, 0.0025f, 1.0f / mConfig.simulation.frequency);
#if NX_SDK_VERSION_MAJOR == 2
		mSimulation.physxScene->setTiming(MaxSubstep, NumSubSteps, NX_TIMESTEP_FIXED);
		mSimulation.apexScene->simulate(deltaT);
#elif NX_SDK_VERSION_MAJOR == 3
		for (int i = 0; i < NumSubSteps - 1; i++)
		{
			mSimulation.apexScene->simulate(MaxSubstep, false);
			mSimulation.apexScene->fetchResults(true, NULL);
		}
		mSimulation.apexScene->simulate(MaxSubstep, true);
#endif

#endif


		mSimulation.running = true;
		mState.currentFrameNumber++;
	}
}



void ClothingAuthoring::stopSimulation()
{
	if (mSimulation.running)
	{
		unsigned int errorState = 0;
		mSimulation.apexScene->fetchResults(true, &errorState);
		PX_ASSERT(errorState == 0);
		mSimulation.running = false;
		mSimulation.frameNumber++;
	}
}



void ClothingAuthoring::restartSimulation()
{
#if 1
	int oldActorCount = mSimulation.actorCount;
	setActorCount(0, false);
	setActorCount(oldActorCount, false);
#else
	if (mSimulation.apexScene != NULL && mSimulation.physxScene != NULL)
	{
		mSimulation.apexScene->setPhysXScene(NULL);
		mSimulation.apexScene->setPhysXScene(mSimulation.physxScene);
	}
#endif

	for (size_t i = 0; i < _mNotifyCallbacks.size(); i++)
	{
		_mNotifyCallbacks[i]->notifyRestart();
	}
}



bool ClothingAuthoring::updateCCT(float deltaT)
{
	bool updated = false;
	if (mConfig.simulation.CCTSpeed > 0.0f)
	{
		const float cctSpeed = mConfig.simulation.CCTSpeed * mState.simulationValuesScale * 3.0f;
		physx::PxVec3 cctTargetSpeedVec(0.0f, 0.0f, 0.0f);
		physx::PxVec3 cctTargetRotation(0.0f, 0.0f, 0.0f);
		if (!mSimulation.CCTDirection.isZero())
		{
			cctTargetSpeedVec = mSimulation.CCTDirection * (cctSpeed / mSimulation.CCTDirection.magnitude());
			updated = true;
		}

		if (!mSimulation.CCTRotationDelta.isZero())
		{
			cctTargetRotation = mSimulation.CCTRotationDelta * physx::PxTwoPi * deltaT  * mConfig.simulation.CCTSpeed;
			updated = true;
		}

		mSimulation.CCTPose.setPosition(mSimulation.CCTPose.getPosition() + cctTargetSpeedVec * deltaT);

		physx::PxF32 rotationAngle = cctTargetRotation.normalize();
		if (rotationAngle > 0)
		{
			// add rotation to CCTPose rotation
			physx::PxMat44 rot(physx::PxQuat(rotationAngle, cctTargetRotation));
			physx::PxMat44 poseRot = mSimulation.CCTPose;
			physx::PxVec3 posePos = mSimulation.CCTPose.getPosition();
			poseRot.setPosition(PxVec3(0.0f));
			mSimulation.CCTPose = rot * poseRot;
			mSimulation.CCTPose.setPosition(posePos);
		}
	}
	return updated;
}



void ClothingAuthoring::stepsUntilPause(int steps)
{
	mSimulation.stepsUntilPause = steps + 1; // always need one more

	if (mConfig.animation.selectedAnimation < 0)
	{
		setAnimation(-mConfig.animation.selectedAnimation);
	}
}



int ClothingAuthoring::getMaxLodValue() const
{
	if (mSimulation.actors.size() > 0)
	{
		if (mSimulation.actors[0].actors.size() > 0)
		{
			if (mSimulation.actors[0].actors[0] != NULL)
			{
				float min, max;
				bool intOnly;
				mSimulation.actors[0].actors[0]->getPhysicalLodRange(min, max, intOnly);

				return (int)max;
			}
		}
	}
	return -1;
}



bool ClothingAuthoring::loadInputMesh(const char* filename, bool allowConversion, bool silentOnError, bool recurseIntoApx)
{
	mMeshes.clear(_mRenderResourceManager, _mResourceCallback, false);

	stopSimulation();
	resetTempConfiguration();

	if (filename == NULL)
	{
		filename = mMeshes.inputMeshFilename.c_str();
	}

	bool OK = false;

	const char* extension = filename;

	const char* lastDir = std::max(::strrchr(filename, '/'), ::strrchr(filename, '\\'));
	if (lastDir != NULL)
	{
		extension = lastDir + 1;
	}

	while (!OK)
	{
		extension = ::strchr(extension, '.'); // first dot in string
		if (extension == NULL)
		{
			OK = false;
			break;
		}

		extension++; // go beyond the '.'

		if (mMeshes.inputMesh == NULL)
		{
			mMeshes.inputMesh = new Samples::TriangleMesh(0);
			mMeshes.inputMesh->setTextureUVOrigin(getTextureUvOrigin());
		}

		mMeshes.inputMesh->clear(_mRenderResourceManager, _mResourceCallback);

		if (::strcmp(extension, "mesh.xml") == 0)
		{
			OK = mMeshes.inputMesh->loadFromXML(filename, true);
			if (OK)
			{
				if (mMeshes.skeleton == NULL)
				{
					mMeshes.skeleton = new Samples::SkeletalAnim();
				}

				// check whether there is a corresponding skeleton
				std::string error;
				std::string skeletonFile = filename;
				if (!mMeshes.skeleton->loadFromXML(skeletonFile.substr(0, skeletonFile.size() - 8) + "skeleton.xml", error))
				{
					// error.empty() means the file couldn't be found, not an error!
					if (!error.empty() && _mErrorCallback != NULL)
					{
						_mErrorCallback->reportErrorPrintf("Skeleton Import Failure", "%s", error.c_str());
					}

					delete mMeshes.skeleton;
					mMeshes.skeleton = NULL;
				}
			}
		}
		else if (::strcmp(extension, "obj") == 0)
		{
			OK = mMeshes.inputMesh->loadFromObjFile(filename, true);
			if (OK)
			{
				mMeshes.inputMesh->moveAboveGround(0.0f);
			}
		}
		else if (::strcmp(extension, "apx") == 0 || ::strcmp(extension, "apb") == 0)
		{
			NxParameterized::Interface* renderMeshAssetInterface = NULL;

			// try loading it as a render mesh asset
			NxParameterized::Serializer::DeserializedData deserializedData;
			loadParameterized(filename, NULL, deserializedData, silentOnError);

			for (unsigned int i = 0; i < deserializedData.size(); i++)
			{
				if (::strcmp(deserializedData[i]->className(), "RenderMeshAssetParameters") == 0 && renderMeshAssetInterface == NULL)
				{
					renderMeshAssetInterface = deserializedData[i];
				}
				else if (renderMeshAssetInterface == NULL && recurseIntoApx)
				{
					NxParameterized::Handle handle(deserializedData[i]);
					renderMeshAssetInterface = extractRMA(handle);

					deserializedData[i]->destroy();
				}
				else
				{
					deserializedData[i]->destroy();
				}
			}

			physx::apex::NxRenderMeshAssetAuthoring* renderMeshAssetAuthoring = NULL;
			if (renderMeshAssetInterface != NULL)
			{
				renderMeshAssetAuthoring = static_cast<physx::apex::NxRenderMeshAssetAuthoring*>(_mApexSDK->createAssetAuthoring(renderMeshAssetInterface, "The Render Mesh"));
			}

			if (renderMeshAssetAuthoring != NULL)
			{
				mMeshes.inputMesh->initFrom(*renderMeshAssetAuthoring, true);

				mMeshes.skeleton = new Samples::SkeletalAnim();
				if (!mMeshes.skeleton->initFrom(*renderMeshAssetAuthoring))
				{
					delete mMeshes.skeleton;
					mMeshes.skeleton = NULL;
				}

				for (size_t i = 0; i < mAuthoringObjects.renderMeshAssets.size(); i++)
				{
					mAuthoringObjects.renderMeshAssets[i]->release();
				}
				mAuthoringObjects.renderMeshAssets.clear();

				mAuthoringObjects.renderMeshAssets.push_back(renderMeshAssetAuthoring);

				OK = true;
			}

		}
		else if (mimp::gMeshImport)
		{
			FILE* fph = fopen(filename, "rb");
			if (fph)
			{
				fseek(fph, 0L, SEEK_END);
				unsigned int len = ftell(fph);
				if (len > 0)
				{
					fseek(fph, 0L, SEEK_SET);
					unsigned char* fileData = new unsigned char[len];
					fread(fileData, len, 1, fph);
					physx::Time timer;
					mimp::MeshSystemContainer* msc = mimp::gMeshImport->createMeshSystemContainer(filename, fileData, len, 0);
					if (msc)
					{
						if (::strcmp(extension, "gr2") == 0)
						{
							mimp::gMeshImport->scale(msc, 10);
						}

						physx::Time::Second elapsed = timer.getElapsedSeconds();
						if (elapsed > 5.0f && allowConversion)
						{
							int seconds = (int)physx::PxFloor((float)elapsed);
							int milliseconds = (int)physx::PxFloor((float)((elapsed - seconds) * 1000.0));

							char temp[512];
							sprintf_s(temp, 512, "Loading of %s took %d.%.3ds\nDo you want to save it to EazyMesh (ezm)?", filename, seconds, milliseconds);
							elapsed = ::MessageBox(NULL, temp, "File Import Conversion", MB_YESNO) == IDYES ? 1.0 : 0.0;
						}
						else
						{
							elapsed = 0.0f;
						}

						if (elapsed == 1.0)
						{
							std::string newFileName = filename;
							mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);
							mimp::MeshSerializeFormat format = mimp::MSF_EZMESH;
							if (::strcmp(extension, "ezm") == 0)
							{
								format = mimp::MSF_PSK;
								newFileName = newFileName.substr(0, newFileName.size() - 4) + "psk";
							}
							else
							{
								newFileName = newFileName.substr(0, newFileName.size() - 4) + "ezm";
							}

							mimp::MeshSerialize data(format);
							bool ok = mimp::gMeshImport->serializeMeshSystem(ms, data);
							if (ok && data.mBaseData)
							{
								FILE* fph = fopen(newFileName.c_str(), "wb");
								if (fph)
								{
									fwrite(data.mBaseData, data.mBaseLen, 1, fph);
									fclose(fph);
								}
							}
						}

						OK = mMeshes.inputMesh->loadFromMeshImport(msc, true);
						if (OK)
						{
							if (mMeshes.skeleton == NULL)
							{
								mMeshes.skeleton = new Samples::SkeletalAnim();
							}

							std::string error;
							if (!mMeshes.skeleton->loadFromMeshImport(msc, error, false))
							{
								PX_ASSERT(!error.empty());
								if (!error.empty() && _mErrorCallback != NULL)
								{
									_mErrorCallback->reportErrorPrintf("SkeletalMesh Import Error", "%s", error.c_str());
								}

								delete mMeshes.skeleton;
								mMeshes.skeleton = NULL;
							}
						}
						mimp::gMeshImport->releaseMeshSystemContainer(msc);
					}
					delete [] fileData;
				}
				fclose(fph);
			}
		}
	}

	if (!OK)
	{
		if (_mErrorCallback != NULL && !silentOnError)
		{
			_mErrorCallback->reportErrorPrintf("Mesh Loading error", "%s unrecognized file type", filename);
		}

		delete mMeshes.inputMesh;
		mMeshes.inputMesh = NULL;

		setAuthoringState(AuthoringState::None, true);

		return false;
	}

	mMeshes.inputMeshFilename = filename;
	mMeshes.inputMesh->loadMaterials(_mResourceCallback, _mRenderResourceManager);

	mMeshes.inputMesh->selectSubMesh((size_t)-1, true); // select all

	if (mMeshes.skeleton != NULL)
	{
		mMeshes.skeletonBehind = new Samples::SkeletalAnim();
		mMeshes.skeletonBehind->loadFromParent(mMeshes.skeleton);
		PX_ASSERT(mMeshes.skeletonRemap.empty());
	}

	physx::PxBounds3 bounds;
	mMeshes.inputMesh->getBounds(bounds);

	// as if the character's height is about 2m
	mState.simulationValuesScale = 0.5f * (bounds.minimum - bounds.maximum).magnitude();

	// round to 1.0 if near
	if (mState.simulationValuesScale >= 0.5f && mState.simulationValuesScale <= 2.0f)
	{
		mState.simulationValuesScale = 1.0f;
	}


	updatePaintingColors();
	updatePainter();

	mMeshes.subdivideHistory.clear();
	mState.needsRedraw = true;
	mDirty.maxDistancePainting = true;

	setAuthoringState(AuthoringState::MeshLoaded, true);

	for (size_t i = 0; i < _mNotifyCallbacks.size(); i++)
	{
		_mNotifyCallbacks[i]->notifyInputMeshLoaded();
	}

	return true;
}



bool ClothingAuthoring::loadAnimation(const char* filename, std::string& error)
{
	bool ret = false;

	if (mimp::gMeshImport)
	{
		FILE* fph = fopen(filename, "rb");
		if (fph)
		{
			fseek(fph, 0L, SEEK_END);
			unsigned int len = ftell(fph);
			if (len > 0)
			{
				fseek(fph, 0L, SEEK_SET);
				unsigned char* fileData = new unsigned char[len];
				fread(fileData, len, 1, fph);
				physx::Time timer;
				mimp::MeshSystemContainer* msc = mimp::gMeshImport->createMeshSystemContainer(filename, fileData, len, 0);
				if (msc)
				{
					if (mMeshes.skeleton != NULL)
					{
						ret = mMeshes.skeleton->loadFromMeshImport(msc, error, true);
					}
					mimp::gMeshImport->releaseMeshSystemContainer(msc);
				}
				delete [] fileData;
			}
			fclose(fph);
		}
	}

	return ret;
}



NxParameterized::Interface* ClothingAuthoring::extractRMA(NxParameterized::Handle& param)
{
	NxParameterized::Interface* result = NULL;
	switch (param.parameterDefinition()->type())
	{
	case NxParameterized::TYPE_REF:
		{
			NxParameterized::Interface* child = NULL;
			param.getParamRef(child);

			if (child != NULL && ::strcmp(child->className(), "RenderMeshAssetParameters") == 0)
			{
				result = child;
				param.setParamRef(NULL, false);

				NxParameterized::setParamBool(*child, "isReferenced", false);
			}
			else
			{
				NxParameterized::Handle childHandle(child);
				result = extractRMA(childHandle);
			}
		}
		break;

	case NxParameterized::TYPE_STRUCT:
	case NxParameterized::TYPE_ARRAY:
		{
			int arraySize = 0;
			if (param.parameterDefinition()->type() == NxParameterized::TYPE_ARRAY)
			{
				param.getArraySize(arraySize, 0);
			}
			else
			{
				arraySize = param.parameterDefinition()->numChildren();
			}

			for (int i = 0; i < arraySize && result == NULL; i++)
			{
				param.set(i);

				result = extractRMA(param);

				param.popIndex();
			}
		}
		break;

	default:
		break;
	}

	return result;
}



bool ClothingAuthoring::saveInputMeshToXml(const char* filename)
{
	if (mMeshes.inputMesh == NULL || mMeshes.inputMesh->getNumVertices() == 0)
	{
		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportErrorPrintf("saveInputMeshToXml Error", "Load a mesh first");
		}

		return false;
	}

//	int l = (int)strlen(filename);
	bool OK = mMeshes.inputMesh->saveToXML(filename);

	if (OK && mMeshes.skeleton != NULL && !mMeshes.skeleton->getAnimations().empty())
	{
		std::string skeletonFile = filename;

		if (strstr(filename, ".mesh.xml") != NULL)
		{
			skeletonFile = skeletonFile.substr(0, skeletonFile.size() - 8) + "skeleton.xml";
		}
		else
		{
			skeletonFile = skeletonFile.substr(0, skeletonFile.size() - 3) + "skeleton.xml";
		}

		OK = mMeshes.skeleton->saveToXML(skeletonFile.c_str());
	}

	if (!OK)
	{
		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportErrorPrintf("SaveMeshToXML Error", "%s save error", filename);
		}

		return false;
	}

	return true;
}



bool ClothingAuthoring::saveInputMeshToEzm(const char* filename)
{
	if (mMeshes.inputMesh == NULL || mMeshes.inputMesh->getNumVertices() == 0)
	{
		return false;
	}

	const char* extension = strrchr(filename, '.');

	mimp::MeshSerializeFormat format = mimp::MSF_LAST;
	if (::strcmp(extension, ".ezm") == 0)
	{
		format = mimp::MSF_EZMESH;
	}
	else if (::strcmp(extension, ".ezb") == 0)
	{
		format = mimp::MSF_EZB;
	}

	bool ok = false;

	if (format != mimp::MSF_LAST)
	{
		mimp::MeshSystemContainer* msc = mimp::gMeshImport->createMeshSystemContainer();

		if (msc != NULL)
		{
			bool ok = mMeshes.inputMesh->saveToMeshImport(msc);

			if (ok && mMeshes.skeleton != NULL)
			{
				ok &= mMeshes.skeleton->saveToMeshImport(msc);
			}

			if (ok)
			{
				mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);

				mimp::MeshSerialize data(format);
				ok = mimp::gMeshImport->serializeMeshSystem(ms, data);
				if (ok && data.mBaseData)
				{
					FILE* fph = fopen(filename, "wb");
					if (fph)
					{
						fwrite(data.mBaseData, data.mBaseLen, 1, fph);
						fclose(fph);
					}
				}

				mimp::gMeshImport->releaseSerializeMemory(data);
			}

			mimp::MeshSystem* ms = mimp::gMeshImport->getMeshSystem(msc);
			for (unsigned int m = 0; m < ms->mMeshCount; m++)
			{
				for (unsigned int sm = 0; sm < ms->mMeshes[m]->mSubMeshCount; sm++)
				{
					::free(ms->mMeshes[m]->mSubMeshes[sm]->mIndices);
					::free((char*)ms->mMeshes[m]->mSubMeshes[sm]->mMaterialName);
					delete ms->mMeshes[m]->mSubMeshes[sm];
				}
				::free(ms->mMeshes[m]->mSubMeshes);

				::free(ms->mMeshes[m]->mVertices);
				
				::free((char*)ms->mMeshes[m]->mName);

				delete ms->mMeshes[m];
			}
			::free(ms->mMeshes);

			for (unsigned int s = 0; s < ms->mSkeletonCount; s++)
			{
				for (int b = 0; b< ms->mSkeletons[s]->mBoneCount; b++)
				{
					::free((char*)ms->mSkeletons[s]->mBones[b].mName);
				}
				::free(ms->mSkeletons[s]->mBones);
				delete ms->mSkeletons[s];
			}
			::free(ms->mSkeletons);

			for (unsigned int a = 0; a < ms->mAnimationCount; a++)
			{
				for (int t = 0; t < ms->mAnimations[a]->mTrackCount; t++)
				{
					mimp::MeshAnimTrack* track = ms->mAnimations[a]->mTracks[t];
					track->mName = NULL; // not allocated

					::free(track->mPose);

					delete ms->mAnimations[a]->mTracks[t];
				}
				::free(ms->mAnimations[a]->mTracks);

				::free((char*)ms->mAnimations[a]->mName);
				delete ms->mAnimations[a];
			}
			::free(ms->mAnimations);

			mimp::gMeshImport->releaseMeshSystemContainer(msc);
		}
	}

	return ok;
}



void ClothingAuthoring::selectSubMesh(int subMeshNr, bool on)
{
	if (mMeshes.inputMesh == NULL)
	{
		return;
	}

	bool dirtyChanged = false;
	if (subMeshNr == -1)
	{
		for (size_t i = 0; i < mMeshes.inputMesh->getNumSubmeshes(); i++)
		{
			const Samples::TriangleSubMesh* submesh = mMeshes.inputMesh->getSubMesh(i);
			if (submesh != NULL)
			{
				dirtyChanged |= submesh->selected != on;
			}
		}
	}
	else
	{
		const Samples::TriangleSubMesh* submesh = mMeshes.inputMesh->getSubMesh(subMeshNr);
		if (submesh != NULL)
		{
			dirtyChanged |= submesh->selected != on;
		}
	}

	mDirty.workspace |= dirtyChanged;
	mDirty.maxDistancePainting |= dirtyChanged;

	mMeshes.inputMesh->selectSubMesh(subMeshNr, on);

	setAuthoringState(AuthoringState::SubmeshSelectionChanged, false);

	// restrict painting:
	setPainterIndexBufferRange();
}



bool ClothingAuthoring::loadCustomPhysicsMesh(const char* filename)
{
	clearCustomPhysicsMesh();

	if (filename == NULL)
	{
		return true;
	}

	if (mMeshes.customPhysicsMesh == NULL)
	{
		mMeshes.customPhysicsMesh = new Samples::TriangleMesh(0);
	}

	const char* extension = filename;

	const char* lastDir = std::max(::strrchr(filename, '/'), ::strrchr(filename, '\\'));
	if (lastDir != NULL)
	{
		extension = lastDir;
	}

	bool OK = false;

	while (!OK)
	{
		extension = ::strchr(extension, '.'); // first dot in string
		if (extension == NULL)
		{
			return false;
		}

		extension++; // go beyond the '.'


		//mCustomPhysicsMeshFilename = filename;

		if (::strcmp(extension, "obj") == 0)
		{
			OK = mMeshes.customPhysicsMesh->loadFromObjFile(filename, false);
		}
		else if (::strcmp(extension, "mesh.xml") == 0)
		{
			OK = mMeshes.customPhysicsMesh->loadFromXML(filename, false);
		}
		else if (mimp::gMeshImport)
		{
			FILE* fph = fopen(filename, "rb");
			if (fph != NULL)
			{
				fseek(fph, 0L, SEEK_END);
				size_t len = ftell(fph);
				if (len > 0)
				{
					fseek(fph, 0L, SEEK_SET);
					unsigned char* data = new unsigned char[len];
					fread(data, len, 1, fph);
					mimp::MeshSystemContainer* msc = mimp::gMeshImport->createMeshSystemContainer(filename, data, (physx::PxU32)len, 0);
					if (msc != NULL)
					{
						if (::strcmp(extension, "gr2") == 0)
						{
							mimp::gMeshImport->scale(msc, 10);
						}

						OK = mMeshes.customPhysicsMesh->loadFromMeshImport(msc, true);
						mimp::gMeshImport->releaseMeshSystemContainer(msc);
					}
				}
				fclose(fph);
			}
		}
	}

	if (!OK || mMeshes.customPhysicsMesh->getVertices().empty())
	{
		mMeshes.customPhysicsMesh->clear(_mRenderResourceManager, _mResourceCallback);
		delete mMeshes.customPhysicsMesh;
		mMeshes.customPhysicsMesh = NULL;

		mMeshes.customPhysicsMeshFilename.clear();
		return false;
	}

	mMeshes.customPhysicsMesh->loadMaterials(_mResourceCallback, _mRenderResourceManager, true);
	mMeshes.customPhysicsMesh->setSubMeshColor((size_t)-1, 0xff00ff);

	mMeshes.customPhysicsMeshFilename = filename;
	return true;
}



void ClothingAuthoring::clearCustomPhysicsMesh()
{
	if (mMeshes.customPhysicsMesh != NULL)
	{
		mMeshes.customPhysicsMesh->clear(_mRenderResourceManager, _mResourceCallback);
		delete mMeshes.customPhysicsMesh;
		mMeshes.customPhysicsMesh = NULL;

		mMeshes.customPhysicsMeshFilename.clear();
	}
}



int ClothingAuthoring::subdivideSubmesh(int subMeshNumber)
{
	if (mMeshes.inputMesh == NULL)
	{
		return 0;
	}

	const size_t oldNumIndices = mMeshes.inputMesh->getNumIndices();
	const Samples::TriangleSubMesh* submesh = mMeshes.inputMesh->getSubMesh(subMeshNumber);
	if (submesh != NULL)
	{
		Meshes::SubdivideHistoryItem item;
		item.submesh = subMeshNumber;
		item.subdivision = mConfig.mesh.originalMeshSubdivision;
		mMeshes.subdivideHistory.push_back(item);

		mMeshes.inputMesh->subdivideSubMesh(item.submesh, item.subdivision, mConfig.mesh.evenOutVertexDegrees);
		mMeshes.inputMesh->updatePaintingColors(Samples::PC_NUM_CHANNELS, mConfig.painting.valueMin, mConfig.painting.valueMax, mConfig.painting.valueFlag, _mApexRenderDebug);

		mDirty.workspace = true;
		updatePainter();
		setPainterIndexBufferRange();
	}
	else
	{
		// subdivide all selected
		for (int i = 0; i < (int)mMeshes.inputMesh->getNumSubmeshes(); i++)
		{
			submesh = mMeshes.inputMesh->getSubMesh(i);
			if (submesh->selected)
			{
				subdivideSubmesh(i);
			}
		}
	}

	return (int)(mMeshes.inputMesh->getNumIndices() - oldNumIndices) / 3;
}



void ClothingAuthoring::renameSubMeshMaterial(size_t submesh, const char* newName)
{
	if (mMeshes.inputMesh != NULL)
	{
		mMeshes.inputMesh->setSubMeshMaterialName(submesh, newName, _mResourceCallback);

		mDirty.workspace = true;
	}
}



void ClothingAuthoring::generateInputTangentSpace()
{
	if (mMeshes.inputMesh != NULL)
	{
		if (mMeshes.inputMesh->generateTangentSpace())
		{
			setAuthoringState(AuthoringState::PaintingChanged, false);
			mMeshes.tangentSpaceGenerated = true;
		}
	}
}



void ClothingAuthoring::paint(const physx::PxVec3& rayOrigin, const physx::PxVec3& rayDirection, bool execute,
                              bool leftButton, bool rightButton)
{
	if (mMeshes.painter == NULL)
	{
		return;
	}

	mState.needsRedraw |= mMeshes.painter->raycastHit(); // hit last frame?

	if (mMeshes.inputMesh != NULL && execute)
	{
		physx::PxBounds3 bounds;
		mMeshes.inputMesh->getBounds(bounds);
		const float boundDiagonal = (bounds.minimum - bounds.maximum).magnitude();

		float scale = 1.0f;
		if (mConfig.painting.channel == Samples::PC_MAX_DISTANCE)
		{
			scale *= getAbsolutePaintingScalingMaxDistance();
		}
		else if (mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE)
		{
			scale *= getAbsolutePaintingScalingCollisionFactor();
		}

		const float paintingRadius = boundDiagonal * 0.002f * mConfig.painting.brushRadius;
		const float paintingColor = mConfig.painting.value / mConfig.painting.valueMax;
		mMeshes.painter->setRayAndRadius(rayOrigin, rayDirection, paintingRadius, mConfig.painting.brushMode,
		                                 mConfig.painting.falloffExponent,
		                                 mConfig.painting.value * scale, paintingColor);

		if ((leftButton || rightButton) && mMeshes.painter->raycastHit())
		{
			if (mConfig.painting.channel == Samples::PC_MAX_DISTANCE ||
			        mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE)
			{
				const float invalidValue = mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE ? -1.1f : -0.01f;
				const float minimum = mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE ? -1.1f : -0.01f;
				const float paintValue = rightButton ? invalidValue : mConfig.painting.value;

				mMeshes.painter->paintFloat(mConfig.painting.channel, minimum, mConfig.painting.valueMax, paintValue);
			}
			else
			{
				unsigned int flag = mConfig.painting.valueFlag;
				if (rightButton)
				{
					flag = ~flag;
				}

				mMeshes.painter->paintFlag(mConfig.painting.channel, flag, rightButton);
			}

			ClothingAuthoring::updatePaintingColors();

			setAuthoringState(AuthoringState::PaintingChanged, false);
			mDirty.maxDistancePainting |= mConfig.painting.channel == Samples::PC_MAX_DISTANCE;
		}

		mState.needsRedraw |= mMeshes.painter->raycastHit(); // hit this frame?
	}
	else
	{
		// turn off brush
		mMeshes.painter->setRayAndRadius(rayOrigin, rayDirection, 0.0f, mConfig.painting.brushMode,
		                                 mConfig.painting.falloffExponent, mState.simulationValuesScale,
		                                 mConfig.painting.value / mConfig.painting.valueMax);
	}
}



void ClothingAuthoring::floodPainting(bool invalid)
{
	if (mMeshes.painter != NULL)
	{
		const float paintingRadius = -1; //flood
		mMeshes.painter->setRayAndRadius(physx::PxVec3(0.0f), physx::PxVec3(0.0f), paintingRadius,
		                                 BrushMode::PaintVolumetric, mConfig.painting.falloffExponent, 0.0f,
		                                 mConfig.painting.value / mConfig.painting.valueMax);

		if (mConfig.painting.channel == Samples::PC_MAX_DISTANCE ||
		        mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE)
		{
			const float invalidValue = mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE ? -1.1f : -0.01f;
			const float minimum = mConfig.painting.channel == Samples::PC_COLLISION_DISTANCE ? -1.1f : -0.01f;
			const float paintValue = invalid ? invalidValue : mConfig.painting.value;

			mMeshes.painter->paintFloat(mConfig.painting.channel, minimum, mConfig.painting.valueMax, paintValue);
		}
		else
		{
			unsigned int flag = invalid ? ~mConfig.painting.valueFlag : mConfig.painting.valueFlag;
			mMeshes.painter->paintFlag(mConfig.painting.channel, flag, invalid);
		}

		updatePaintingColors();

		setAuthoringState(AuthoringState::PaintingChanged, false);
		mDirty.maxDistancePainting |= mConfig.painting.channel == Samples::PC_MAX_DISTANCE;
		mDirty.workspace = true;

		mState.needsRedraw = true;
	}
}



void ClothingAuthoring::smoothPainting(int numIterations)
{
	if (mMeshes.painter != NULL)
	{
		//mMeshes.painter->smoothFloat(mConfig.painting.channel, 0.5f, numIterations);
		mMeshes.painter->smoothFloatFast(mConfig.painting.channel, numIterations);

		updatePaintingColors();

		setAuthoringState(AuthoringState::PaintingChanged, false);
		mDirty.maxDistancePainting |= mConfig.painting.channel == Samples::PC_MAX_DISTANCE;
		mDirty.workspace = true;
	}
}



void ClothingAuthoring::updatePainter()
{
	if (mMeshes.painter == NULL)
	{
		return;
	}

	mMeshes.painter->clear();

	Samples::TriangleMesh* inputMesh = mMeshes.inputMesh;

	if (inputMesh != NULL)
	{
		const std::vector<physx::PxVec3> &verts = inputMesh->getVertices();
		const std::vector<physx::PxU32> &indices = inputMesh->getIndices();

		if (!verts.empty())
		{
			mMeshes.painter->initFrom(&verts[0], (int)verts.size(), sizeof(physx::PxVec3), &indices[0], (int)indices.size(), sizeof(physx::PxU32));
			mMeshes.painter->setFloatBuffer(Samples::PC_MAX_DISTANCE, &inputMesh->getPaintChannel(Samples::PC_MAX_DISTANCE)[0].paintValueF32, sizeof(Samples::PaintedVertex));
			mMeshes.painter->setFloatBuffer(Samples::PC_COLLISION_DISTANCE, &inputMesh->getPaintChannel(Samples::PC_COLLISION_DISTANCE)[0].paintValueF32, sizeof(Samples::PaintedVertex));
			mMeshes.painter->setFlagBuffer(Samples::PC_LATCH_TO_NEAREST_SLAVE, &inputMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_SLAVE)[0].paintValueU32, sizeof(Samples::PaintedVertex));
			mMeshes.painter->setFlagBuffer(Samples::PC_LATCH_TO_NEAREST_MASTER, &inputMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_MASTER)[0].paintValueU32, sizeof(Samples::PaintedVertex));
		}
	}
}



void ClothingAuthoring::setPainterIndexBufferRange()
{
	if (mMeshes.painter == NULL)
	{
		return;
	}

	mMeshes.painter->clearIndexBufferRange();

	Samples::TriangleMesh* inputMesh = mMeshes.inputMesh;

	if (inputMesh != NULL && inputMesh->getNumIndices() > 0)
	{
		for (size_t i = 0; i < inputMesh->getNumSubmeshes(); i++)
		{
			const Samples::TriangleSubMesh* submesh = inputMesh->getSubMesh(i);
			if (submesh->selected)
			{
				mMeshes.painter->addIndexBufferRange(submesh->firstIndex, submesh->firstIndex + submesh->numIndices);
			}
		}
	}
}



void ClothingAuthoring::initGroundMesh(const char* resourceDir)
{
	if (mMeshes.groundMesh == NULL)
	{
		mMeshes.groundMesh = new Samples::TriangleMesh(0);

		mMeshes.groundMesh->initPlane(50.0f, 5.0f, "ClothingToolGround");

		mMeshes.groundMesh->loadMaterials(_mResourceCallback, _mRenderResourceManager, false, resourceDir);
		mMeshes.groundMesh->setCullMode(physx::apex::NxRenderCullMode::CLOCKWISE, -1);
	}
}



void ClothingAuthoring::updatePaintingColors()
{
	if (mMeshes.inputMesh != NULL)
	{
		if (mConfig.painting.channel == Samples::PC_MAX_DISTANCE)
		{
			mMeshes.inputMesh->updatePaintingColors(Samples::PC_MAX_DISTANCE, mConfig.painting.valueMin, mConfig.painting.valueMax, 0, _mApexRenderDebug);
		}
		else
		{
			const float maxDistMax = mMeshes.inputMesh->getMaximalMaxDistance();
			mMeshes.inputMesh->updatePaintingColors(Samples::PC_MAX_DISTANCE, 0, maxDistMax, 0, _mApexRenderDebug);
		}

		mMeshes.inputMesh->updatePaintingColors(Samples::PC_COLLISION_DISTANCE, 0, 0, 0, _mApexRenderDebug);
		mMeshes.inputMesh->updatePaintingColors(Samples::PC_LATCH_TO_NEAREST_SLAVE, 0, 0, mConfig.painting.valueFlag, _mApexRenderDebug);
		mMeshes.inputMesh->updatePaintingColors(Samples::PC_LATCH_TO_NEAREST_MASTER, 0, 0, mConfig.painting.valueFlag, _mApexRenderDebug);

	}
}



bool ClothingAuthoring::getMaxDistancePaintValues(const float*& values, int& numValues, int& byteStride)
{
	if (mMeshes.inputMesh == NULL || mMeshes.inputMesh->getPaintChannel(Samples::PC_MAX_DISTANCE).empty())
	{
		values = NULL;
		numValues = 0;
	}
	else
	{
		values = &mMeshes.inputMesh->getPaintChannel(Samples::PC_MAX_DISTANCE)[0].paintValueF32;
		numValues = (int)mMeshes.inputMesh->getVertices().size();
	}
	byteStride = sizeof(Samples::PaintedVertex);

	return getAndClearMaxDistancePaintingDirty();
}



float ClothingAuthoring::getAbsolutePaintingScalingMaxDistance()
{
	if (mMeshes.inputMesh != NULL)
	{
		physx::PxBounds3 bounds;
		mMeshes.inputMesh->getBounds(bounds);
		return (bounds.minimum - bounds.maximum).magnitude() * mConfig.painting.scalingMaxdistance;
	}

	return 1.0f;
}



float ClothingAuthoring::getAbsolutePaintingScalingCollisionFactor()
{
	if (mMeshes.inputMesh != NULL)
	{
		physx::PxBounds3 bounds;
		mMeshes.inputMesh->getBounds(bounds);
		return (bounds.minimum - bounds.maximum).magnitude() * mConfig.painting.scalingCollisionFactor;
	}

	return 1.0f;
}




void ClothingAuthoring::setPhysicalLodValues(int numValues, const float* values)
{
	mMeshes.physicalLodValues.clear();

	for (int i = 0; i < numValues; i++)
	{
		mMeshes.physicalLodValues.push_back(values[i] * getAbsolutePaintingScalingMaxDistance());
	}
	std::sort(mMeshes.physicalLodValues.begin(), mMeshes.physicalLodValues.end());

	mState.needsRedraw = true;
}



void ClothingAuthoring::setAnimationPose(int position)
{
	if (mMeshes.inputMesh == NULL || mMeshes.skeleton == NULL)
	{
		return;
	}

	setAnimationTime((float)position);
	const int animation = mConfig.animation.selectedAnimation;
	if (animation != 0)
	{
		mMeshes.skeleton->setAnimPose(physx::PxAbs(animation) - 1, mConfig.animation.time, mConfig.animation.lockRootbone);
		skinMeshes(mMeshes.skeleton);
	}
	else
	{
		mMeshes.skeleton->setBindPose();
		skinMeshes(NULL);
	}
	mConfig.animation.showSkinnedPose = animation != 0;
}



void ClothingAuthoring::setBindPose()
{
	if (mConfig.animation.showSkinnedPose)
	{
		if (mMeshes.skeleton != NULL && mMeshes.inputMesh != NULL)
		{
			mMeshes.skeleton->setBindPose();
			skinMeshes(NULL);
		}
		mConfig.animation.showSkinnedPose = false;
	}
}



void ClothingAuthoring::setAnimationTime(float time)
{
	if (mCurrentActorDesc < mLoadedActorDescs.size())
	{
		mCurrentActorDesc = (unsigned int)(((float)(mLoadedActorDescs.size()-1)) / 100.0f * time );
		stepsUntilPause(1); // need to simulate once to update
		mState.manualAnimation = true;
		return;
	}

	const int animation = mConfig.animation.selectedAnimation;

	if (animation == 0 || mMeshes.skeleton == NULL ||  physx::PxAbs(animation) > (int)mMeshes.skeleton->getAnimations().size())
	{
		setAnimationTimes(0.0f);
	}
	else
	{
		Samples::SkeletalAnimation* anim = mMeshes.skeleton->getAnimations()[physx::PxAbs(animation) - 1];

		const float minTime = physx::PxMax(anim->minTime, mConfig.animation.cropMin);
		const float maxTime = physx::PxMin(anim->maxTime, mConfig.animation.cropMax);
		if (maxTime > minTime)
		{
			const float newAnimationTime = minTime + (maxTime - minTime) * time / 100.f;
			mState.manualAnimation = physx::PxAbs(mConfig.animation.time - newAnimationTime) > 0.05f;

			setAnimationTimes(newAnimationTime);

			mConfig.animation.showSkinnedPose = true;
		}
	}
}



float ClothingAuthoring::getAnimationTime() const
{
	if (mCurrentActorDesc < mLoadedActorDescs.size())
	{
		return 100.0f / (mLoadedActorDescs.size()-1) * mCurrentActorDesc;
	}

	const int animation = mConfig.animation.selectedAnimation;

	if (animation == 0 || mMeshes.skeleton == NULL || physx::PxAbs(animation) > (int)mMeshes.skeleton->getAnimations().size())
	{
		return 0.0f;
	}

	Samples::SkeletalAnimation* anim = mMeshes.skeleton->getAnimations()[physx::PxAbs(animation) - 1];

	const float minTime = physx::PxMax(anim->minTime, mConfig.animation.cropMin);
	const float maxTime = physx::PxMin(anim->maxTime, mConfig.animation.cropMax);

	if (minTime >= maxTime)
	{
		return minTime;
	}

	return (mConfig.animation.time - minTime) * 100.f / (maxTime - minTime);
}



bool ClothingAuthoring::updateAnimation()
{
	if (mCurrentActorDesc < mLoadedActorDescs.size())
	{
		bool jumped = getAndClearManualAnimation();
		if (mConfig.animation.selectedAnimation > 0)
		{
			++mCurrentActorDesc;
			if (mCurrentActorDesc >=mLoadedActorDescs.size())
			{
				mCurrentActorDesc = 0;
				if (!mConfig.animation.continuous)
					jumped = true;
			}
		}
		else
		{
			stepsUntilPause(1);
		}
		return jumped;
	}

	if (mMeshes.skeleton == NULL || mMeshes.inputMesh == NULL)
	{
		return false;
	}

	PX_ASSERT(mMeshes.skeletonBehind != NULL);

	bool jumped = false;
	const std::vector<Samples::SkeletalAnimation*> &anims = mMeshes.skeleton->getAnimations();
	const int animation = mConfig.animation.selectedAnimation;
	if (animation == 0 || physx::PxAbs(animation) > (int)anims.size())
	{
		mMeshes.skeleton->setBindPose();
		mMeshes.skeletonBehind->setBindPose();

		skinMeshes(NULL);
	}
	else
	{
		int anim = physx::PxAbs(animation) - 1;

		jumped |= clampAnimation(mConfig.animation.time, true, mConfig.animation.loop, anims[anim]->minTime, anims[anim]->maxTime) && !mConfig.animation.continuous;

		bool lockRootBone = mConfig.animation.lockRootbone;
		mMeshes.skeleton->setAnimPose(anim, mConfig.animation.time, lockRootBone);
		mMeshes.skeletonBehind->setAnimPose(anim, mConfig.animation.time, lockRootBone);

		skinMeshes(mMeshes.skeleton);

		mConfig.animation.showSkinnedPose = true;
	}

	jumped |= getAndClearManualAnimation();
	return jumped;
}



void ClothingAuthoring::skinMeshes(Samples::SkeletalAnim* anim)
{
	if (anim == NULL)
	{
		mMeshes.inputMesh->unskin();
	}
	else
	{
		mMeshes.inputMesh->skin(*anim);
	}

	for (size_t i = 0; i < mSimulation.actors.size(); i++)
	{
		if (mSimulation.actors[i].triangleMesh != NULL)
		{
			if (anim == NULL)
			{
				//mSimulation.actors[i].triangleMesh->unskin();
			}
			else
			{
				mSimulation.actors[i].triangleMesh->skin(*anim/*, mSimulation.actors[i].scale*/);  // scale is on globalPose
			}
		}
	}
}



void ClothingAuthoring::clearCollisionVolumes()
{
	mMeshes.collisionVolumes.clear();
}

unsigned int ClothingAuthoring::generateCollisionVolumes(bool useCapsule, bool commandMode, bool dirtyOnly)
{
	if (mMeshes.skeleton == NULL)
	{
		return 0;
	}

	const std::vector<Samples::SkeletalBone> &bones = mMeshes.skeleton->getBones();
	int boneCount = (int)bones.size();


	for (int i = (int)mMeshes.collisionVolumes.size() - 1; i >= 0; --i)
	{
		const int boneIndex = mMeshes.collisionVolumes[i].boneIndex;
		if ((bones[boneIndex].dirtyParams && dirtyOnly) || (!bones[boneIndex].manualShapes && !dirtyOnly))
		{
			mMeshes.collisionVolumes.erase(mMeshes.collisionVolumes.begin() + i);
			mMeshes.skeleton->clearShapeCount(boneIndex);
		}
	}

	unsigned int dirtyCount = 0;
	for (size_t i = 0; i < bones.size(); i++)
	{
		if ((bones[i].dirtyParams && dirtyOnly) || (!bones[i].manualShapes && !dirtyOnly))
		{
			dirtyCount++;
		}

	}
	if (dirtyCount == 0)
	{
		return 0;
	}

	if (!mMeshes.collisionVolumes.empty() && !commandMode)
	{
		mDirty.workspace = true;
	}

	HACD::AutoGeometry* autoGeometry = createAutoGeometry();

	if (autoGeometry != NULL)
	{
		for (int i = 0; i < boneCount; i++)
		{
			const Samples::SkeletalBone& source = bones[i];
			HACD::SimpleBone dest;
			dest.mOption = static_cast<HACD::BoneOption>(source.boneOption);
			dest.mBoneName = source.name.c_str();
			dest.mParentIndex = source.parent;
			if ((dirtyOnly && !source.dirtyParams) || (!dirtyOnly && source.manualShapes))
			{
				// disable this bone
				dest.mBoneMinimalWeight = 100.0f;
			}
			else
			{
				dest.mBoneMinimalWeight = source.minimalBoneWeight;
				PX_ASSERT(mMeshes.skeleton);
				mMeshes.skeleton->setBoneDirty(i, false);
			}
			source.bindWorldPose.getColumnMajor44(dest.mTransform);
			source.invBindWorldPose.getColumnMajor44(dest.mInverseTransform);
			autoGeometry->addSimpleBone(dest);
		}


		unsigned int geomCount;
		float autoCollapsePercent = 5.0f;
		HACD::SimpleHull** hulls = autoGeometry->createCollisionVolumes(autoCollapsePercent, geomCount);

		if (hulls == NULL)
		{
			geomCount = 0;
		}

		for (unsigned int i = 0; i < geomCount; i++)
		{
			addCollisionVolumeInternal(hulls[i], useCapsule && bones[hulls[i]->mBoneIndex].allowPrimitives);
		}

		HACD::releaseAutoGeometry(autoGeometry);
	}

	if (useCapsule && mMeshes.skeleton != NULL && false)
	{
		for (size_t i = 0; i < mMeshes.collisionVolumes.size(); i++)
		{
			CollisionVolume& volume = mMeshes.collisionVolumes[i];

			if (mMeshes.skeleton->getBones()[volume.boneIndex].allowPrimitives)
			{
				continue;
			}

			physx::PxQuat quat;
			computeBestFitCapsule(volume.vertices.size(), (float*)&volume.vertices[0], sizeof(physx::PxVec3),
			                      volume.capsuleRadius, volume.capsuleHeight, &volume.shapeOffset.t.x, &quat.x, true);
			volume.shapeOffset.M.fromQuat(quat);

			// apply scale
			volume.capsuleRadius *= 100.0f / mState.simulationValuesScale;
			volume.capsuleHeight *= 100.0f / mState.simulationValuesScale;

			// PH: if the capsule radius is != 0, we use capsules, but we keep the convex just in case

			const float capsuleVolume = (volume.capsuleRadius * volume.capsuleRadius * physx::PxPi * volume.capsuleHeight) +
			                            (volume.capsuleRadius * volume.capsuleRadius * volume.capsuleRadius * 4.0f / 3.0f * physx::PxPi);

			// PH: this does not seem like a good idea
			if (capsuleVolume > volume.meshVolume * 1.5f && false)
			{
				// turn off capsule if bad approximation
				volume.capsuleRadius = 0.0f;
			}
		}
	}

	return (unsigned int)mMeshes.collisionVolumes.size();
}



ClothingAuthoring::CollisionVolume* ClothingAuthoring::addCollisionVolume(bool useCapsule, unsigned int boneIndex, bool createFromMesh)
{
	if (mMeshes.skeleton == NULL)
	{
		return NULL;
	}

	const std::vector<Samples::SkeletalBone> &bones = mMeshes.skeleton->getBones();
	const size_t boneCount = bones.size();

	if (!createFromMesh)
	{
		CollisionVolume volume;
		volume.boneIndex = boneIndex;
		volume.boneName = mMeshes.skeleton->getBones()[boneIndex].name;
		mMeshes.collisionVolumes.push_back(volume);
		return &mMeshes.collisionVolumes.back();
	}
	else
	{
		HACD::AutoGeometry* autoGeometry = createAutoGeometry();
		if (autoGeometry != NULL)
		{
			for (size_t i = 0; i < boneCount; i++)
			{
				const Samples::SkeletalBone& source = bones[i];
				HACD::SimpleBone dest;
				dest.mOption = static_cast<HACD::BoneOption>(source.boneOption);
				dest.mBoneName = source.name.c_str();
				dest.mParentIndex = source.parent;
				dest.mBoneMinimalWeight = (i != boneIndex) ? 100.0f : source.minimalBoneWeight;

				source.bindWorldPose.getColumnMajor44(dest.mTransform);
				source.invBindWorldPose.getColumnMajor44(dest.mInverseTransform);

				autoGeometry->addSimpleBone(dest);
			}

			unsigned int geomCount;
			float autoCollapsePercent = 5.0f;
			HACD::SimpleHull** hulls = autoGeometry->createCollisionVolumes(autoCollapsePercent, geomCount);

			PX_ASSERT(geomCount <= 1); // only one bone is turned on!
			for (unsigned int i = 0; i < geomCount; i++)
			{
				addCollisionVolumeInternal(hulls[i], useCapsule && bones[hulls[i]->mBoneIndex].allowPrimitives);
			}

			HACD::releaseAutoGeometry(autoGeometry);

			return geomCount > 0 ? &mMeshes.collisionVolumes.back() : NULL;
		}
	}

	return NULL;
}



ClothingAuthoring::CollisionVolume* ClothingAuthoring::getCollisionVolume(int index)
{
	if (index >= 0 && (size_t)index < mMeshes.collisionVolumes.size())
	{
		return &mMeshes.collisionVolumes[index];
	}

	return NULL;
}



bool ClothingAuthoring::deleteCollisionVolume(int index)
{
	if (index >= 0 && (size_t)index < mMeshes.collisionVolumes.size())
	{
		mMeshes.collisionVolumes.erase(mMeshes.collisionVolumes.begin() + index);
		return true;
	}

	return false;
}



void ClothingAuthoring::drawCollisionVolumes(bool wireframe) const
{
	if (_mApexRenderDebug == NULL || mMeshes.collisionVolumes.empty())
	{
		return;
	}

	_mApexRenderDebug->pushRenderState();
	if (!wireframe)
	{
		_mApexRenderDebug->addToCurrentState(physx::DebugRenderState::SolidShaded);
	}
	_mApexRenderDebug->addToCurrentState(physx::DebugRenderState::CounterClockwise);


	for (size_t i = 0; i < mMeshes.collisionVolumes.size(); i++)
	{
		const CollisionVolume& volume = mMeshes.collisionVolumes[i];

		physx::PxMat34Legacy xform = volume.transform * volume.shapeOffset;

		unsigned int color = 0xdcd44eff;
		float inflation = 0.0f;
		bool selected = false;

		if (mMeshes.skeleton != NULL)
		{
			const std::vector<Samples::SkeletalBone> &bones = mMeshes.skeleton->getBones();
			xform = bones[volume.boneIndex].currentWorldPose * volume.shapeOffset;

			if (!bones[volume.boneIndex].manualShapes)
			{
				inflation = bones[volume.boneIndex].inflateConvex * mState.simulationValuesScale / 100.0f;    // a bit magic, I know
			}
			else if (volume.capsuleRadius <= 0.0f)
			{
				inflation = volume.inflation * mState.simulationValuesScale / 100.0f;
			}

			selected = bones[volume.boneIndex].selected;
		}
		// PH: render each bone with a different color
		if (selected)
		{
			color = 0xefffffff;
		}
		else
		{
			// get a color from the bone name
			const char* str = volume.boneName.c_str();
#if 0
			// djb2
			int c = *str;
			unsigned long hash = 5381 + volume.boneIndex;
			while (c = *str++)
			{
				hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
			}
#else
			// sdbm
			unsigned long hash = volume.boneIndex;
			while (*str)
			{
				int c = *str;
				hash = c + (hash << 6) + (hash << 16) - hash;
				str++;
			}
#endif
			color = hash | 0xff000000;
		}

		_mApexRenderDebug->setCurrentColor(color, color);

		if (volume.capsuleRadius > 0.0f)
		{
			const float height = volume.capsuleHeight * mState.simulationValuesScale / 100.0f;

			physx::PxVec3 p0(0.0f, -height * 0.5f, 0.0f);
			physx::PxVec3 p1(0.0f, height * 0.5f, 0.0f);
			xform.multiply(p0, p0);
			xform.multiply(p1, p1);

			const float radius = volume.capsuleRadius * mState.simulationValuesScale / 100.0f;
			_mApexRenderDebug->debugOrientedCapsule(radius + inflation, height, 2, xform);
		}
		else
		{
			size_t tcount = volume.indices.size() / 3;
			if (tcount > 0)
			{

				physx::PxVec3 center(0.0f, 0.0f, 0.0f);

				if (inflation != 0.0f)
				{
					for (size_t i = 0; i < volume.vertices.size(); i++)
					{
						center += volume.vertices[i];
					}
					center /= (physx::PxF32)volume.vertices.size();
				}

				const unsigned int* indices = &volume.indices[0];
				for (size_t i = 0; i < tcount; i++)
				{
					unsigned int i1 = indices[i * 3 + 0];
					unsigned int i2 = indices[i * 3 + 1];
					unsigned int i3 = indices[i * 3 + 2];
					physx::PxVec3 p1 = volume.vertices[i1];
					physx::PxVec3 p2 = volume.vertices[i2];
					physx::PxVec3 p3 = volume.vertices[i3];
					if (inflation != 0.0f)
					{
						physx::PxVec3 out = p1 - center;
						float dist = out.normalize();
						if (dist + inflation > 0.0f)
						{
							p1 += inflation * out;
						}
						else
						{
							p1 = center;
						}

						out = p2 - center;
						dist = out.normalize();
						if (dist + inflation > 0.0f)
						{
							p2 += inflation * out;
						}
						else
						{
							p2 = center;
						}

						out = p3 - center;
						dist = out.normalize();
						if (dist + inflation > 0.0f)
						{
							p3 += inflation * out;
						}
						else
						{
							p3 = center;
						}
					}
					physx::PxVec3 t1, t2, t3;
					xform.multiply(p1, t1);
					xform.multiply(p2, t2);
					xform.multiply(p3, t3);

					_mApexRenderDebug->debugTri(t1, t2, t3);
				}
			}
		}
	}

	_mApexRenderDebug->popRenderState();
}




physx::apex::NxRenderMeshAssetAuthoring* ClothingAuthoring::getRenderMeshAsset(int index)
{
	createRenderMeshAssets();

	if (index >= mConfig.cloth.numGraphicalLods)
	{
		return NULL;
	}

	return mAuthoringObjects.renderMeshAssets[index];
}



physx::apex::NxClothingPhysicalMesh* ClothingAuthoring::getClothMesh(int index, physx::apex::IProgressListener* progress)
{
	createClothMeshes(progress);

	if (index >= (int)mAuthoringObjects.physicalMeshes.size())
	{
		return NULL;
	}

	return mAuthoringObjects.physicalMeshes[index];
}



void ClothingAuthoring::simplifyClothMesh(float factor)
{
	for (size_t i = 0; i < mAuthoringObjects.physicalMeshes.size(); i++)
	{
		unsigned int maxSteps = (factor == 0.0f) ? SIMPLIFICATOR_MAX_STEPS : (unsigned int)(factor * mAuthoringObjects.physicalMeshes[i]->getNumVertices());
		unsigned int subdivSize = (factor == 0.0f) ? mConfig.cloth.simplify : 1;
		mAuthoringObjects.physicalMeshes[i]->simplify(subdivSize, maxSteps, -1, NULL);
	}
}



int ClothingAuthoring::getNumClothTriangles() const
{
	if (mAuthoringObjects.physicalMeshes.size() > 0 && !mAuthoringObjects.physicalMeshes[0]->isTetrahedralMesh())
	{
		return mAuthoringObjects.physicalMeshes[0]->getNumIndices() / 3;
	}

	return 0;
}



physx::apex::NxClothingIsoMesh* ClothingAuthoring::getIsoMesh(physx::apex::IProgressListener* progress)
{
	createIsoMesh(progress);

	return mAuthoringObjects.isoMesh;
}



void ClothingAuthoring::simplifyIsoMesh(float factor, physx::apex::IProgressListener* progressParent)
{
	ProgressCallback progress(100, progressParent);

	if (mAuthoringObjects.isoMesh != NULL || mState.authoringState < AuthoringState::PhysicalIsoMeshCreated)
	{
		progress.setSubtaskWork(40, "Create Iso Mesh");
		getIsoMesh(&progress);
		progress.completeSubtask();
	}

	PX_ASSERT(mAuthoringObjects.isoMesh != NULL);

	progress.setSubtaskWork(-1, "Simplify Iso Mesh");
	if (factor == 0.0f)
	{
		mAuthoringObjects.isoMesh->simplify(mConfig.softbody.isoSimplifySubdivision, SIMPLIFICATOR_MAX_STEPS, -1, &progress);
	}
	else
	{
		unsigned int numSteps = (physx::PxU32)(factor * mAuthoringObjects.isoMesh->getNumVertices());
		mAuthoringObjects.isoMesh->simplify(0, physx::PxMax(1u, numSteps), 5.0f, NULL);
	}
	progress.completeSubtask();
}




int ClothingAuthoring::getNumIsoSurfaceTriangles() const
{
	if (mAuthoringObjects.isoMesh != NULL)
	{
		return mAuthoringObjects.isoMesh->getNumIndices() / 3;
	}

	return 0;
}



physx::apex::NxClothingPhysicalMesh* ClothingAuthoring::getTetraMesh(physx::apex::IProgressListener* progress)
{
	createTetraMesh(progress);

	if (mAuthoringObjects.physicalMeshes.empty())
	{
		return NULL;
	}

	return mAuthoringObjects.physicalMeshes[0];
}



int ClothingAuthoring::getNumTetrahedra() const
{
	PX_ASSERT(mAuthoringObjects.physicalMeshes.size() <= 1);
	if (mAuthoringObjects.physicalMeshes.size() == 1 && mAuthoringObjects.physicalMeshes[0]->isTetrahedralMesh())
	{
		return mAuthoringObjects.physicalMeshes[0]->getNumIndices() / 4;
	}

	return 0;
}



physx::apex::NxClothingPhysicalMesh* ClothingAuthoring::getPhysicalMesh()
{
	return mAuthoringObjects.physicalMeshes.empty() ? NULL : mAuthoringObjects.physicalMeshes[0];
}



physx::apex::NxClothingAssetAuthoring* ClothingAuthoring::getClothingAsset(physx::apex::IProgressListener* progress)
{
	createClothingAsset(progress);

	if (mAuthoringObjects.clothingAssetAuthoring == NULL)
	{
		return NULL;
	}

	updateDeformableParameters();


	CurrentState::tMaterialLibraries::iterator found = mState.materialLibraries.find(mState.selectedMaterialLibrary);
	if (found != mState.materialLibraries.end())
	{
		// figure out material index
		NxParameterized::Interface* materialLibrary = found->second;
		NxParameterized::Handle arrayHandle(materialLibrary);

		NxParameterized::ErrorType error = NxParameterized::ERROR_NONE;
		error = arrayHandle.getParameter("materials");
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		int numMaterials = 0;
		error = arrayHandle.getArraySize(numMaterials);
		PX_ASSERT(error == NxParameterized::ERROR_NONE);

		int materialIndex = 0;
		for (int i = 0; i < numMaterials; i++)
		{
			error = arrayHandle.set(i);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			NxParameterized::Handle nameHandle(materialLibrary);
			error = arrayHandle.getChildHandle(materialLibrary, "materialName", nameHandle);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			const char* materialName = NULL;
			error = nameHandle.getParamString(materialName);
			PX_ASSERT(error == NxParameterized::ERROR_NONE);

			if (mState.selectedMaterial == materialName)
			{
				materialIndex = i;
				break;
			}

			arrayHandle.popIndex();
		}

		mAuthoringObjects.clothingAssetAuthoring->setMaterialLibrary(found->second, materialIndex, false);
	}


	if (!mMeshes.collisionVolumes.empty())
	{
		PX_ASSERT(mMeshes.skeleton != NULL);

		mAuthoringObjects.clothingAssetAuthoring->clearAllBoneActors();

		PxU32 numCapsules = (PxU32)mMeshes.collisionVolumes.size();
		/*
		std::vector<const char*> boneNames(2*numCapsules);
		std::vector<physx::PxF32> radii(2*numCapsules);
		std::vector<physx::PxVec3> localPositions(2*numCapsules);
		std::vector<physx::PxU16> pairs(2*numCapsules);
		*/
		for (size_t i = 0; i < numCapsules; i++)
		{
			const CollisionVolume& volume = mMeshes.collisionVolumes[i];
			const Samples::SkeletalBone& bone = mMeshes.skeleton->getBones()[volume.boneIndex];

			float inflation = (bone.manualShapes ? volume.inflation : bone.inflateConvex) * mState.simulationValuesScale / 100.0f;

			/*
			boneNames[2*i] = volume.boneName.c_str();
			boneNames[2*i+1] = volume.boneName.c_str();

			radii[2*i] = volume.capsuleRadius * mState.simulationValuesScale / 100.0f + inflation;
			radii[2*i+1] = radii[2*i];

			const float halfheight = 0.5f*volume.capsuleHeight * mState.simulationValuesScale / 100.0f;
			physx::PxVec3 dir = volume.shapeOffset.M.getColumn(1);
			dir.normalize();
			localPositions[2*i] = volume.shapeOffset.t + halfheight * dir;
			localPositions[2*i+1] = volume.shapeOffset.t - halfheight * dir;

			pairs[2*i] = (physx::PxU16)(2*i);
			pairs[2*i+1] = (physx::PxU16)(2*i+1);
			*/

			if (volume.capsuleRadius > 0.0f)
			{
				if (bone.manualShapes)
				{
					inflation = 0.0f;
				}

				const float radius = volume.capsuleRadius * mState.simulationValuesScale / 100.0f;
				const float height = volume.capsuleHeight * mState.simulationValuesScale / 100.0f;
				mAuthoringObjects.clothingAssetAuthoring->addBoneCapsule(volume.boneName.c_str(), radius + inflation, height, volume.shapeOffset);
			}
			else
			{
				if (inflation != 0.0f)
				{

					std::vector<physx::PxVec3> vertexCopy(volume.vertices);
					physx::PxVec3 center(0.0f, 0.0f, 0.0f);
					for (size_t i = 0; i < vertexCopy.size(); i++)
					{
						vertexCopy[i] = volume.shapeOffset * vertexCopy[i];
						center += vertexCopy[i];
					}
					center /= (float)vertexCopy.size();
					for (size_t i = 0; i < vertexCopy.size(); i++)
					{
						physx::PxVec3 out = vertexCopy[i] - center;
						const float dist = out.normalize();
						if (dist + inflation > 0.0f)
						{
							vertexCopy[i] += inflation * out;
						}
					}
					mAuthoringObjects.clothingAssetAuthoring->addBoneConvex(volume.boneName.c_str(), &vertexCopy[0], (unsigned int)volume.vertices.size());
				}
				else
				{
					mAuthoringObjects.clothingAssetAuthoring->addBoneConvex(volume.boneName.c_str(), &volume.vertices[0], (unsigned int)volume.vertices.size());
				}
			}
		}
		
		//mAuthoringObjects.clothingAssetAuthoring->setCollision(&boneNames[0], &radii[0], &localPositions[0], 2*numCapsules, &pairs[0], (physx::PxU32)pairs.size());

	}

	return mAuthoringObjects.clothingAssetAuthoring;
}




void ClothingAuthoring::setZAxisUp(bool z)
{
	mDirty.workspace |= mConfig.ui.zAxisUp != z;
	mConfig.ui.zAxisUp = z;

	setGroundplane(mConfig.simulation.groundplane);
	setGravity(mConfig.simulation.gravity);
}



void ClothingAuthoring::setCullMode(physx::apex::NxRenderCullMode::Enum cullMode)
{
	mDirty.workspace |= mConfig.mesh.cullMode != cullMode;
	mConfig.mesh.cullMode = cullMode;

	if (mMeshes.inputMesh != NULL)
	{
		mMeshes.inputMesh->setCullMode(cullMode, -1);
	}
}



void ClothingAuthoring::setTextureUvOrigin(physx::apex::NxTextureUVOrigin::Enum origin)
{
	mDirty.workspace |= mConfig.mesh.textureUvOrigin != origin;
	mConfig.mesh.textureUvOrigin = origin;

	if (mMeshes.inputMesh != NULL)
	{
		mMeshes.inputMesh->setTextureUVOrigin(origin);
	}
}



void ClothingAuthoring::setPaintingChannel(int channel)
{
	if (mConfig.painting.channel != channel)
	{
		mState.needsRedraw = true;
	}

	mDirty.workspace |= mConfig.painting.channel != channel;
	mConfig.painting.channel = channel;

	switch (mConfig.painting.channel)
	{
	case Samples::PC_MAX_DISTANCE:
		mConfig.painting.value = physx::PxMax(-0.1f, mConfig.painting.value);
		break;
	case Samples::PC_COLLISION_DISTANCE:
		if (physx::PxAbs(mConfig.painting.value) > 1.0f)
		{
			mConfig.painting.value = physx::PxSign(mConfig.painting.value);
		}
		break;
	case Samples::PC_LATCH_TO_NEAREST_SLAVE:
	case Samples::PC_LATCH_TO_NEAREST_MASTER:
		mConfig.painting.falloffExponent = 0.0f;
		break;
	}

	if (mConfig.painting.channel != Samples::PC_NUM_CHANNELS && mMeshes.painter == NULL)
	{
		mMeshes.painter = new SharedTools::MeshPainter();
		updatePainter();
		setPainterIndexBufferRange();
	}
}



void ClothingAuthoring::setPaintingValue(float val, float vmin, float vmax)
{
	PX_ASSERT(val >= 0.0f);
	PX_ASSERT(val <= 1.0f);
	const float vval = val * vmax + (1 - val) * vmin;
	mDirty.workspace |= mConfig.painting.value != vval;
	mConfig.painting.value = vval;

	if (mConfig.painting.valueMin != vmin || mConfig.painting.valueMax != vmax)
	{
		mConfig.painting.valueMin = vmin;
		mConfig.painting.valueMax = vmax;

		mState.needsRedraw = true;
		updatePaintingColors();
	}
}



void ClothingAuthoring::setPaintingValueFlag(unsigned int flags)
{
	if ((unsigned int)mConfig.painting.valueFlag != flags)
	{
		mDirty.workspace = true;
		mConfig.painting.valueFlag = flags;

		updatePaintingColors();
		mState.needsRedraw = true;
	}
}



void ClothingAuthoring::setAnimation(int animation)
{
	if (mConfig.animation.selectedAnimation != animation)
	{
		mDirty.workspace = true;
		mState.manualAnimation = mConfig.animation.selectedAnimation != -animation;
		mConfig.animation.selectedAnimation = animation;

		if (mConfig.animation.selectedAnimation > 0)
		{
			if (!mConfig.animation.loop && mMeshes.skeleton != NULL)
			{
				// set the animation to beginning if at end if looping doesn't take care of it ?!?

				Samples::SkeletalAnimation* animation = mMeshes.skeleton->getAnimations()[mConfig.animation.selectedAnimation - 1];
				if (mConfig.animation.time >= animation->maxTime)
				{
					mConfig.animation.time = animation->minTime;
				}
			}
		}
	}
	else if (mConfig.animation.selectedAnimation == 0 && animation == 0 && !mSimulation.paused)
	{
		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportErrorPrintf("Animation Error", "Bind pose is selected, cannot play animation");
		}
	}
	mSimulation.paused = false;
}



void ClothingAuthoring::stepAnimationTimes(float animStep)
{
	if (mConfig.animation.selectedAnimation > 0)
	{
		mConfig.animation.time += animStep;
	}
}



bool ClothingAuthoring::clampAnimation(float& time, bool stoppable, bool loop, float minTime, float maxTime)
{
	bool jumped = false;

//	const float animLength = maxTime - minTime;

	minTime = physx::PxMax(minTime, mConfig.animation.cropMin);
	maxTime = physx::PxMin(maxTime, mConfig.animation.cropMax);

	if (time < minTime)
	{
		if (loop)
		{
			jumped = true;
			time = maxTime;
		}
		else
		{
			if (stoppable)
			{
				mConfig.animation.selectedAnimation = -mConfig.animation.selectedAnimation;
			}

			time = minTime;
		}
	}
	else if (time > maxTime)
	{
		if (loop)
		{
			jumped = true;
			time = minTime;
		}
		else
		{
			if (stoppable)
			{
				mConfig.animation.selectedAnimation = -mConfig.animation.selectedAnimation;
			}

			time = maxTime;
		}
	}

	return jumped;
}



void  ClothingAuthoring::setAnimationCrop(float min, float max)
{
	PX_ASSERT(min < max);
	mConfig.animation.cropMin = min;
	mConfig.animation.cropMax = max;
}



void ClothingAuthoring::ErrorCallback::reportErrorPrintf(const char* label, const char* fmt, ...)
{
	const size_t stringLength = 512;
	char stringBuffer[stringLength];

	va_list va;
	va_start(va, fmt);
	vsnprintf(stringBuffer, stringLength, fmt, va);
	va_end(va);

	reportError(label, stringBuffer);
}



void ClothingAuthoring::resetTempConfiguration()
{
	stopSimulation();

	mSimulation.clear();

	mMeshes.clear(_mRenderResourceManager, _mResourceCallback, false);
	mAuthoringObjects.clear();
	mState.init();

	mConfig.apex.forceEmbedded = false;

	selectMaterial("Default", "Default");
	//releasePhysX();

	mDirty.init();

	mRecordCommands.clear();
}



void ClothingAuthoring::initConfiguration()
{
	mConfig.init();
}


void ClothingAuthoring::prepareConfiguration()
{
	mFloatConfiguration.clear();
	mIntConfiguration.clear();
	mBoolConfiguration.clear();

#define ADD_PARAM_NEW(_CONFIG, _PARAM) _CONFIG[#_PARAM] = &_PARAM
#define ADD_PARAM_OLD(_CONFIG, _PARAMNAME, _PARAM) _CONFIG##Old[_PARAMNAME] = &_PARAM

	// configuration.UI
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.ui.zAxisUp);
	ADD_PARAM_OLD(mBoolConfiguration, "mZAxisUp", mConfig.ui.zAxisUp);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.ui.spotLight);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.ui.spotLightShadow);

	// configuration.mesh
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.mesh.originalMeshSubdivision);
	ADD_PARAM_OLD(mIntConfiguration, "mOriginalMeshSubdivision", mConfig.mesh.originalMeshSubdivision);
	ADD_PARAM_OLD(mIntConfiguration, "mSubmeshSubdiv", mConfig.mesh.originalMeshSubdivision);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.mesh.evenOutVertexDegrees);
	ADD_PARAM_OLD(mBoolConfiguration, "mEvenOutVertexDegrees", mConfig.mesh.evenOutVertexDegrees);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.mesh.cullMode);
	ADD_PARAM_OLD(mIntConfiguration,  "mCullMode", mConfig.mesh.cullMode);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.mesh.textureUvOrigin);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.mesh.physicalMeshType);

	// configuration.Apex
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.parallelCpuSkinning);
	ADD_PARAM_OLD(mBoolConfiguration, "mParallelCpuSkinning", mConfig.apex.parallelCpuSkinning);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.recomputeNormals);
	ADD_PARAM_OLD(mBoolConfiguration, "mRecomputeNormals", mConfig.apex.recomputeNormals);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.recomputeTangents);
	ADD_PARAM_OLD(mBoolConfiguration, "mRecomputeNormals", mConfig.apex.recomputeTangents);
	//ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.correctSimulationNormals); // not added on purpose
	//ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.useMorphTargetTest); // not added on purpose
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.apex.forceEmbedded);

	// configuration.tempMeshes.Cloth
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.cloth.numGraphicalLods);
	ADD_PARAM_OLD(mIntConfiguration,  "mClothNumGraphicalLods", mConfig.cloth.numGraphicalLods);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.cloth.simplify);
	ADD_PARAM_OLD(mIntConfiguration,  "mClothSimplifySL", mConfig.cloth.simplify);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.cloth.close);
	ADD_PARAM_OLD(mBoolConfiguration, "mCloseCloth", mConfig.cloth.close);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.cloth.subdivide);
	ADD_PARAM_OLD(mBoolConfiguration, "mSlSubdivideCloth", mConfig.cloth.subdivide);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.cloth.subdivision);
	ADD_PARAM_OLD(mIntConfiguration,  "mClothMeshSubdiv", mConfig.cloth.subdivision);

	// configuration.tempMeshes.Softbody
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.softbody.isoSurfaceSubdivision);
	ADD_PARAM_OLD(mIntConfiguration,  "mIsoSurfaceSubdivision", mConfig.softbody.isoSurfaceSubdivision);
	ADD_PARAM_OLD(mIntConfiguration, "mIsoGridSubdiv", mConfig.softbody.isoSurfaceSubdivision);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.softbody.isoSimplifySubdivision);
	ADD_PARAM_OLD(mIntConfiguration,  "mIsoSimplifySubdivision", mConfig.softbody.isoSimplifySubdivision);
	ADD_PARAM_OLD(mIntConfiguration, "mSimplificationSubdiv", mConfig.softbody.isoSimplifySubdivision);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.softbody.keepBiggestMeshes);
	ADD_PARAM_OLD(mIntConfiguration,  "mTetraKeepBiggestMeshes", mConfig.softbody.keepBiggestMeshes);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.softbody.discardInnerMeshes);

	// configuration.collisionVolumes
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.collisionVolumes.usePaintingChannel);
	ADD_PARAM_OLD(mBoolConfiguration, "mCollisionVolumeUsePaintingChannel", mConfig.collisionVolumes.usePaintingChannel);

	// configuration.painting
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.painting.brushMode);
	ADD_PARAM_OLD(mIntConfiguration,   "mBrushMode", mConfig.painting.brushMode);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.falloffExponent);
	ADD_PARAM_OLD(mFloatConfiguration, "mFalloffExponent", mConfig.painting.falloffExponent);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.painting.channel);
	ADD_PARAM_OLD(mIntConfiguration,   "mPaintingChannel", mConfig.painting.channel);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.value);
	ADD_PARAM_OLD(mFloatConfiguration, "mPaintingValue", mConfig.painting.value);
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.valueMin); // not added on purpose
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.valueMax); // not added on purpose
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.painting.valueFlag); // not added on purpose
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.painting.brushRadius);
	ADD_PARAM_OLD(mIntConfiguration,   "mBrushRadius", mConfig.painting.brushRadius);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.scalingMaxdistance);
	ADD_PARAM_OLD(mFloatConfiguration, "mPaintingScalingMaxdistanceNew", mConfig.painting.scalingMaxdistance);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.scalingCollisionFactor);
	ADD_PARAM_OLD(mFloatConfiguration, "mPaintingScalingCollisionFactorNew", mConfig.painting.scalingCollisionFactor);
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.painting.maxDistanceScale);
	//ADD_PARAM_NEW(mBoolConfiguration, mConfig.painting.maxDistanceScaleMultipliable);

	// configuration.setMeshes
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.setMeshes.deriveNormalsFromBones);
	ADD_PARAM_OLD(mBoolConfiguration,  "mDeriveNormalsFromBones", mConfig.setMeshes.deriveNormalsFromBones);

	// configuration.simulation
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.frequency);
	ADD_PARAM_OLD(mFloatConfiguration, "mSimulationFrequency", mConfig.simulation.frequency);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.gravity);
	ADD_PARAM_OLD(mIntConfiguration,   "mGravity", mConfig.simulation.gravity);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.groundplane);
	ADD_PARAM_OLD(mIntConfiguration,   "mGroundplane", mConfig.simulation.groundplane);
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.groundplaneEnabled);
	ADD_PARAM_OLD(mBoolConfiguration,  "mGroundplaneEnabled", mConfig.simulation.groundplaneEnabled);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.budgetPercent);
	ADD_PARAM_OLD(mIntConfiguration,   "mBudgetPercent", mConfig.simulation.budgetPercent);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.interCollisionDistance);
	ADD_PARAM_OLD(mFloatConfiguration, "mInterCollisionDistance", mConfig.simulation.interCollisionDistance);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.interCollisionStiffness);
	ADD_PARAM_OLD(mFloatConfiguration, "mInterCollisionStiffness", mConfig.simulation.interCollisionStiffness);
	ADD_PARAM_NEW(mIntConfiguration, mConfig.simulation.interCollisionIterations);
	ADD_PARAM_OLD(mIntConfiguration, "mInterCollisionIterations", mConfig.simulation.interCollisionIterations);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.blendTime);
	ADD_PARAM_OLD(mFloatConfiguration, "mBlendTime", mConfig.simulation.blendTime);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.pressure);
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.lodOverwrite); // not added on purpose
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.windDirection);
	ADD_PARAM_OLD(mIntConfiguration,   "mWindDirection", mConfig.simulation.windDirection);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.windElevation);
	ADD_PARAM_OLD(mIntConfiguration,   "mWindElevation", mConfig.simulation.windElevation);
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.windVelocity);
	ADD_PARAM_OLD(mIntConfiguration,   "mWindVelocity", mConfig.simulation.windVelocity);
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.gpuSimulation);
	ADD_PARAM_OLD(mBoolConfiguration,  "mGpuSimulation", mConfig.simulation.gpuSimulation);
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.fallbackSkinning);
	ADD_PARAM_OLD(mBoolConfiguration,  "mFallbackSkinning", mConfig.simulation.fallbackSkinning);
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.simulation.CCTSpeed); // Not added on purpose!
	ADD_PARAM_NEW(mIntConfiguration,   mConfig.simulation.graphicalLod);
	//ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.usePreview); // Not added on purpose
	//ADD_PARAM_NEW(mFloatConfiguration,  mConfig.simulation.timingNoise); // Not added on purpose
	//ADD_PARAM_NEW(mFloatConfiguration,  mConfig.simulation.scaleFactor); // Not added on purpose
	//ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.localSpaceSim); // Not added on purpose
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.pvdDebug);
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.pvdProfile);
	ADD_PARAM_NEW(mBoolConfiguration,  mConfig.simulation.pvdMemory);

	// configuration.animation
	//ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.showSkinnedPose); // not done on purpose
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.animation.selectedAnimation);
	ADD_PARAM_OLD(mIntConfiguration,  "mAnimation", mConfig.animation.selectedAnimation);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.animation.speed);
	ADD_PARAM_OLD(mIntConfiguration,  "mAnimationSpeed", mConfig.animation.speed);
	//ADD_PARAM_NEW(mIntConfiguration,  mConfig.animation.times[0]); // not done on purpose
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.loop);
	ADD_PARAM_OLD(mBoolConfiguration, "mLoopAnimation", mConfig.animation.loop);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.lockRootbone);
	ADD_PARAM_OLD(mBoolConfiguration, "mLockRootbone", mConfig.animation.lockRootbone);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.continuous);
	ADD_PARAM_OLD(mBoolConfiguration, "mAnimationContinuous", mConfig.animation.continuous);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.useGlobalPoseMatrices);
	ADD_PARAM_OLD(mBoolConfiguration, "mUseGlobalPoseMatrices", mConfig.animation.useGlobalPoseMatrices);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.animation.applyGlobalPoseInApp);
	//ADD_PARAM_NEW(mFloatConfiguration,mConfig.animation.cropMin); // not added on purpose
	//ADD_PARAM_NEW(mFloatConfiguration,mConfig.animation.cropMax); // not added on purpose

	// configuration.deformable
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.deformable.thickness);
	ADD_PARAM_OLD(mFloatConfiguration, "mDeformableThickness", mConfig.deformable.thickness);
	//ADD_PARAM(mBoolConfiguration, mConfig.deformable.drawThickness); // not added on purpose
	//ADD_PARAM_NEW(mFloatConfiguration, mConfig.deformable.selfcollisionThickness);
	//ADD_PARAM_OLD(mFloatConfiguration, "mDeformableSelfcollisionThickness", mConfig.deformable.selfcollisionThickness);
	//ADD_PARAM(mBoolConfiguration, mConfig.deformable.drawSelfcollisionThickness); // not added on purpose
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.deformable.virtualParticleDensity);
	ADD_PARAM_NEW(mIntConfiguration,  mConfig.deformable.hierarchicalLevels);
	ADD_PARAM_OLD(mIntConfiguration,  "mDeformableHierarchicalLevels", mConfig.deformable.hierarchicalLevels);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.deformable.disableCCD);
	ADD_PARAM_OLD(mBoolConfiguration, "mDeformableDisableCCD", mConfig.deformable.disableCCD);
	//ADD_PARAM_NEW(mBoolConfiguration, mConfig.deformable.selfcollision);
	//ADD_PARAM_OLD(mBoolConfiguration, "mDeformableSelfcollision", mConfig.deformable.selfcollision);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.deformable.twowayInteraction);
	ADD_PARAM_OLD(mBoolConfiguration, "mDeformableTwowayInteraction", mConfig.deformable.twowayInteraction);
	ADD_PARAM_NEW(mBoolConfiguration, mConfig.deformable.untangling);
	ADD_PARAM_OLD(mBoolConfiguration, "mDeformableUntangling", mConfig.deformable.untangling);
	ADD_PARAM_NEW(mFloatConfiguration, mConfig.deformable.restLengthScale);

#undef ADD_PARAM_NEW
#undef ADD_PARAM_OLD
}



void ClothingAuthoring::addCommand(const char* commandString, int frameNumber)
{
	if (frameNumber == -2)
	{
		frameNumber = mState.currentFrameNumber;
	}

	PX_ASSERT(mRecordCommands.empty() || frameNumber >= mRecordCommands.back().frameNumber);

	Command command;
	command.frameNumber = frameNumber;
	command.command = commandString;

	mRecordCommands.push_back(command);
}



void ClothingAuthoring::clearCommands()
{
	mRecordCommands.clear();
}



bool ClothingAuthoring::loadParameterized(const char* filename, physx::PxFileBuf* filebuffer, NxParameterized::Serializer::DeserializedData& deserializedData, bool silent)
{
	bool error = false;
	bool ownsFileBuffer = false;

	if (filebuffer == NULL)
	{
		filebuffer = _mApexSDK->createStream(filename, physx::PxFileBuf::OPEN_READ_ONLY);
		ownsFileBuffer = true;
	}
	else if (filename == NULL)
	{
		filename = "unnamed buffer";
	}

	NxParameterized::Serializer::SerializeType inTypeExt = extensionToType(filename);

	if (filebuffer != NULL)
	{
		if (filebuffer->isOpen())
		{
			NxParameterized::Serializer::SerializeType inTypeFile = _mApexSDK->getSerializeType(*filebuffer);
			PX_ASSERT(inTypeFile != NxParameterized::Serializer::NST_LAST);

			if (inTypeFile == NxParameterized::Serializer::NST_LAST)
			{
				if (_mErrorCallback != NULL)
				{
					_mErrorCallback->reportErrorPrintf("loadParameterized error", "File \'%s\' contains neither xml nor binary data\n", filename);
				}
				error = true;
			}
			else
			{
				if (inTypeExt != NxParameterized::Serializer::NST_LAST && inTypeFile != inTypeExt && _mErrorCallback != NULL)
				{
					const char* realExtension = inTypeFile == NxParameterized::Serializer::NST_XML ? ".apx" : ".apb";
					_mErrorCallback->reportErrorPrintf("loadParameterized error", "File \'%s\' has wrong extension should be %s\n", filename, realExtension);
				}

				NxParameterized::Serializer* serializer = _mApexSDK->createSerializer(inTypeFile);

				NxParameterized::Serializer::ErrorType serError = serializer->deserialize(*filebuffer, deserializedData);

				error = parameterizedError(serError, silent ? NULL : filename);

				serializer->release();
			}
			PX_ASSERT(error || deserializedData.size() > 0);
		}
		if (ownsFileBuffer)
		{
			filebuffer->release();
			filebuffer = NULL;
		}
	}

	if (!error && deserializedData.size() > 0)
	{
		return true;
	}

	return false;
}



bool ClothingAuthoring::saveParameterized(const char* filename, physx::PxFileBuf* filebuffer, const NxParameterized::Interface** pInterfaces, unsigned int numInterfaces)
{
	NxParameterized::Serializer::SerializeType serType = extensionToType(filename);


	if (serType == NxParameterized::Serializer::NST_LAST)
	{
		if (_mErrorCallback != NULL)
		{
			_mErrorCallback->reportErrorPrintf("SaveParameterized Error", "Cannot find serialization for file \'%s\'", filename);
		}
		return false;
	}

	PX_ASSERT(pInterfaces != NULL);
	if (pInterfaces == NULL)
	{
		return false;
	}

	for (unsigned int i = 0; i  < numInterfaces; i++)
	{
		PX_ASSERT(pInterfaces[i] != NULL);
		if (pInterfaces[i] == NULL)
		{
			return false;
		}
	}

	bool error = false;
	bool ownsFileBuffer = false;
	if (filebuffer == NULL)
	{
		filebuffer = _mApexSDK->createStream(filename, physx::PxFileBuf::OPEN_WRITE_ONLY);
		ownsFileBuffer = true;
	}

	if (filebuffer != NULL)
	{
		if (filebuffer->isOpen())
		{
			NxParameterized::Serializer* serializer = _mApexSDK->createSerializer(serType);
			NxParameterized::Serializer::ErrorType serError = serializer->serialize(*filebuffer, pInterfaces, numInterfaces);
			error = parameterizedError(serError, filename);

			serializer->release();
		}

		if (ownsFileBuffer)
		{
			filebuffer->release();
			filebuffer = NULL;
		}
	}


	// that's stupid, on error we still create the empty or half finished file!
	// maybe we should save to a memory stream
	return error;
}



NxParameterized::Serializer::SerializeType ClothingAuthoring::extensionToType(const char* filename) const
{
	const char* lastSlash = std::max(strrchr(filename, '/'), strrchr(filename, '\\'));
	if (lastSlash == NULL)
	{
		lastSlash = filename;
	}

	const char* extension = strchr(lastSlash, '.');

	NxParameterized::Serializer::SerializeType serType = NxParameterized::Serializer::NST_LAST;

	while (serType == NxParameterized::Serializer::NST_LAST && extension != NULL)
	{
		extension++; // move beyond the '.'
		if (_stricmp(extension, "apx") == 0)
		{
			serType = NxParameterized::Serializer::NST_XML;
		}
		else if (_stricmp(extension, "apb") == 0)
		{
			serType = NxParameterized::Serializer::NST_BINARY;
		}

		extension = strchr(extension, '.');
	}

	return serType;
}



bool ClothingAuthoring::parameterizedError(NxParameterized::Serializer::ErrorType errorType, const char* filename)
{
	if (errorType != NxParameterized::Serializer::ERROR_NONE)
	{
		char* errorString = NULL;
		switch (errorType)
		{
#define CASE(_A) case NxParameterized::Serializer::_A: errorString = #_A; break;
			CASE(ERROR_UNKNOWN)
			CASE(ERROR_NOT_IMPLEMENTED)
			CASE(ERROR_INVALID_PLATFORM)
			CASE(ERROR_INVALID_PLATFORM_NAME)
			CASE(ERROR_INVALID_FILE_VERSION)
			CASE(ERROR_INVALID_FILE_FORMAT)
			CASE(ERROR_INVALID_MAGIC)
			CASE(ERROR_STREAM_ERROR)
			CASE(ERROR_MEMORY_ALLOCATION_FAILURE)
			CASE(ERROR_UNALIGNED_MEMORY)
			CASE(ERROR_PRESERIALIZE_FAILED)
			CASE(ERROR_INTERNAL_BUFFER_OVERFLOW)
			CASE(ERROR_OBJECT_CREATION_FAILED)
			CASE(ERROR_CONVERSION_FAILED)
			CASE(ERROR_VAL2STRING_FAILED)
			CASE(ERROR_STRING2VAL_FAILED)
			CASE(ERROR_INVALID_TYPE_ATTRIBUTE)
			CASE(ERROR_UNKNOWN_XML_TAG)
			CASE(ERROR_MISSING_DOCTYPE)
			CASE(ERROR_MISSING_ROOT_ELEMENT)
			CASE(ERROR_INVALID_NESTING)
			CASE(ERROR_INVALID_ATTR)
			CASE(ERROR_INVALID_ARRAY)
			CASE(ERROR_ARRAY_INDEX_OUT_OF_RANGE)
			CASE(ERROR_INVALID_VALUE)
			CASE(ERROR_INVALID_INTERNAL_PTR)
			CASE(ERROR_INVALID_PARAM_HANDLE)
			CASE(ERROR_INVALID_RELOC_TYPE)
			CASE(ERROR_INVALID_DATA_TYPE)

#undef CASE
		default:
			errorString = "un-implemented error";
		}

		if (errorString != NULL && _mErrorCallback != NULL && filename != NULL)
		{
			_mErrorCallback->reportErrorPrintf("Serialization Error", "%s in %s", filename, errorString);
		}

		return true;
	}

	return false;
}



void ClothingAuthoring::setAuthoringState(AuthoringState::Enum authoringState, bool allowAdvance)
{
	if (mState.authoringState > authoringState || allowAdvance)
	{
		mState.authoringState = authoringState;
	}
}



HACD::AutoGeometry* ClothingAuthoring::createAutoGeometry()
{
	Samples::TriangleMesh* inputMesh = mMeshes.inputMesh;
	Samples::SkeletalAnim* skeleton = mMeshes.skeleton;

	if (inputMesh == NULL || skeleton == NULL)
	{
		return NULL;
	}

	const std::vector<Samples::SkeletalBone> &bones = skeleton->getBones();
	unsigned int boneCount = (unsigned int)bones.size();

	const std::vector<physx::PxVec3>& meshVertices = inputMesh->getVertices();
	const std::vector<unsigned short>& boneIndices = inputMesh->getBoneIndices();
	const std::vector<physx::PxVec4>& boneWeights2 = inputMesh->getBoneWeights();

	if (!meshVertices.empty() && (meshVertices.size() * 4) == boneIndices.size() && boneCount > 0)
	{
		HACD::AutoGeometry* autoGeometry = HACD::createAutoGeometry();

		const std::vector<unsigned int> &meshIndices = inputMesh->getIndices();
		const std::vector<Samples::PaintedVertex>& graphicalSlave = inputMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_SLAVE);

		const size_t numSubmeshes = inputMesh->getNumSubmeshes();
		for (size_t i = 0; i < numSubmeshes; i++)
		{
			const Samples::TriangleSubMesh& sub = *inputMesh->getSubMesh(i);

			if (sub.usedForCollision)
			{
				const unsigned int tcount = sub.numIndices / 3;
				for (unsigned int j = 0; j < tcount; j++)
				{
					const unsigned int triangleIndices[3] =
					{
						meshIndices[(j * 3 + 0) + sub.firstIndex ],
						meshIndices[(j * 3 + 1) + sub.firstIndex ],
						meshIndices[(j * 3 + 2) + sub.firstIndex ],
					};

					const bool used1 = graphicalSlave[triangleIndices[0]].paintValueU32 == 0;
					const bool used2 = graphicalSlave[triangleIndices[1]].paintValueU32 == 0;
					const bool used3 = graphicalSlave[triangleIndices[2]].paintValueU32 == 0;
					if (!ClothingAuthoring::getCollisionVolumeUsePaintChannel() || (used1 && used2 && used3))
					{
						HACD::SimpleSkinnedVertex vertices[3];
						for (unsigned int k = 0; k < 3; k++)
						{
							(physx::PxVec3&)vertices[k].mPos[0] = meshVertices[triangleIndices[k]];
							for (unsigned int l = 0; l < 4; l++)
							{
								const unsigned int boneIndex = triangleIndices[k] * 4 + l;
								vertices[k].mBone[l] = boneIndices[boneIndex];
								vertices[k].mWeight[l] = boneWeights2[triangleIndices[k]][l];
							}
						}

						autoGeometry->addSimpleSkinnedTriangle(vertices[0], vertices[1], vertices[2]);
					}
				}
			}
		}

		return autoGeometry;
	}

	return NULL;
}



void ClothingAuthoring::addCollisionVolumeInternal(HACD::SimpleHull* hull, bool useCapsule)
{
	CollisionVolume volume;
	volume.boneIndex = hull->mBoneIndex;
	volume.parentIndex = hull->mParentIndex;
	volume.boneName    = hull->mBoneName;
	volume.meshVolume  = hull->mMeshVolume;
	volume.transform.setColumnMajor44(hull->mTransform);

	for (unsigned int j = 0; j < hull->mVertexCount; j++)
	{
		const float* p = &hull->mVertices[j * 3];
		physx::PxVec3 vp(p[0], p[1], p[2]);
		volume.vertices.push_back(vp);
	}

	for (unsigned int j = 0; j < hull->mTriCount * 3; j++)
	{
		volume.indices.push_back(hull->mIndices[j]);
	}


	if (useCapsule)
	{
		physx::PxQuat quat;
		computeBestFitCapsule(volume.vertices.size(), (float*)&volume.vertices[0], sizeof(physx::PxVec3),
		                      volume.capsuleRadius, volume.capsuleHeight, &volume.shapeOffset.t.x, &quat.x, true);
		volume.shapeOffset.M.fromQuat(quat);

		// apply scale
		volume.capsuleRadius *= 100.0f / mState.simulationValuesScale;
		volume.capsuleHeight *= 100.0f / mState.simulationValuesScale;
	}

	mMeshes.collisionVolumes.push_back(volume);
	mMeshes.skeleton->incShapeCount(volume.boneIndex);
}



struct PaintingValues
{
	float maxDistance;
	float collisionSphereDistance;
	float collisionSphereRadius;
	unsigned int latchToNearestSlave;
	unsigned int latchToNearestMaster;
};



void ClothingAuthoring::createRenderMeshAssets()
{
	if (mMeshes.inputMesh == NULL)
	{
		return;
	}

	if (mState.authoringState >= AuthoringState::RenderMeshAssetCreated && mAuthoringObjects.renderMeshAssets.size() == (unsigned int)mConfig.cloth.numGraphicalLods)
	{
		return;
	}

	for (size_t i = 0; i < mAuthoringObjects.renderMeshAssets.size(); i++)
	{
		mAuthoringObjects.renderMeshAssets[i]->release();
	}
	mAuthoringObjects.renderMeshAssets.clear();

	// find maximum #bones per vertex (only the vertices that are submitted!)
	unsigned int maxBonesPerVertex = 0;
	if (mMeshes.inputMesh->getBoneWeights().size() == mMeshes.inputMesh->getNumVertices())
	{
		const unsigned int* indices = &mMeshes.inputMesh->getIndices()[0];
		const physx::PxVec4* boneWeights2 = &mMeshes.inputMesh->getBoneWeights()[0];
		for (size_t i = 0; i < mMeshes.inputMesh->getNumSubmeshes(); i++)
		{
			const Samples::TriangleSubMesh* submesh = mMeshes.inputMesh->getSubMesh(i);
			if (submesh != NULL && submesh->selected)
			{
				const unsigned int start = submesh->firstIndex;
				const unsigned int end = start + submesh->numIndices;

				for (unsigned int i = start; i < end; i++)
				{
					const unsigned int index = indices[i];
					for (unsigned int j = 0; j < 4; j++)
					{
						if (boneWeights2[index][j] != 0.0f)
						{
							maxBonesPerVertex = physx::PxMax(maxBonesPerVertex, j + 1);
						}
					}
				}
			}
		}
	}


	std::vector<float> distances2;
	if (mConfig.cloth.numGraphicalLods > 1)
	{
		// sort the edge distances for later subdivision
		const size_t numIndices = mMeshes.inputMesh->getNumIndices();
		const unsigned int* indices = &mMeshes.inputMesh->getIndices()[0];
		const physx::PxVec3* positions = &mMeshes.inputMesh->getVertices()[0];
		distances2.reserve(numIndices);

		for (size_t i = 0; i < numIndices; i += 3)
		{
			distances2.push_back((positions[indices[i + 0]] - positions[indices[i + 1]]).magnitudeSquared());
			distances2.push_back((positions[indices[i + 1]] - positions[indices[i + 2]]).magnitudeSquared());
			distances2.push_back((positions[indices[i + 2]] - positions[indices[i + 0]]).magnitudeSquared());
		}

		class Compare
		{
		public:
			bool operator()(const float a, const float b) const
			{
				return a > b;
			}

		};
		std::sort(distances2.begin(), distances2.end(), Compare());
	}


	Samples::TriangleMesh subdividedMesh(0);

	for (int graphicalLod = 0; graphicalLod < mConfig.cloth.numGraphicalLods; graphicalLod++)
	{
		const Samples::TriangleMesh* useThisMesh = mMeshes.inputMesh;

		if (graphicalLod > 0)
		{
			subdividedMesh.clear(NULL, NULL);
			subdividedMesh.copyFrom(*mMeshes.inputMesh);
			physx::PxBounds3 bounds;
			subdividedMesh.getBounds(bounds);
			const float dist2 = distances2[graphicalLod * distances2.size() / mConfig.cloth.numGraphicalLods];
			int subdivision = (int)((bounds.minimum - bounds.maximum).magnitude() / physx::PxSqrt(dist2));
			for (int j = 0; j < (int)subdividedMesh.getNumSubmeshes(); j++)
			{
				const Samples::TriangleSubMesh* submesh = subdividedMesh.getSubMesh(j);
				if (submesh != NULL && submesh->selected)
				{
					subdividedMesh.subdivideSubMesh(j, subdivision, mConfig.mesh.evenOutVertexDegrees);
				}
			}
			useThisMesh = &subdividedMesh;
		}

		physx::Array<physx::apex::NxRenderMeshAssetAuthoring::SubmeshDesc> submeshDescs;
		physx::Array<physx::apex::NxRenderMeshAssetAuthoring::VertexBuffer> vertexBufferDescs;

		physx::Array<PaintingValues> paintingValues((unsigned int)useThisMesh->getNumVertices());
		{
			const std::vector<Samples::PaintedVertex> maxDistances = useThisMesh->getPaintChannel(Samples::PC_MAX_DISTANCE);
			const std::vector<Samples::PaintedVertex> collisionDistances = useThisMesh->getPaintChannel(Samples::PC_COLLISION_DISTANCE);
			const std::vector<Samples::PaintedVertex> latchToNearestSlave = useThisMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_SLAVE);
			const std::vector<Samples::PaintedVertex> latchToNearestMaster = useThisMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_MASTER);

			const float paintScalingMaxDistance = getAbsolutePaintingScalingMaxDistance();
			const float paintScalingCollisionFactor = getAbsolutePaintingScalingCollisionFactor();

			for (unsigned int vertex = 0, numVertices = (unsigned int)useThisMesh->getNumVertices(); vertex < numVertices; ++vertex)
			{
				paintingValues[vertex].maxDistance = physx::PxMax(0.0f, maxDistances[vertex].paintValueF32 * paintScalingMaxDistance);

				const float factor = collisionDistances[vertex].paintValueF32;
				const float maxDistance = maxDistances[vertex].paintValueF32;

				if (physx::PxAbs(factor) > 1.0f || maxDistance <= 0)
				{
					paintingValues[vertex].collisionSphereDistance = 0.0f;
					paintingValues[vertex].collisionSphereRadius = 0.0f;
				}
				else
				{
					paintingValues[vertex].collisionSphereDistance = factor * paintScalingCollisionFactor;
					paintingValues[vertex].collisionSphereRadius = 10.0f * maxDistance * paintScalingMaxDistance;
				}

				paintingValues[vertex].latchToNearestSlave = latchToNearestSlave[vertex].paintValueU32;
				paintingValues[vertex].latchToNearestMaster = latchToNearestMaster[vertex].paintValueU32;
			}
		}

		physx::Array<physx::PxVec4> tangentValues;
		if (useThisMesh->getTangents().size() == useThisMesh->getVertices().size())
		{
			tangentValues.resize((physx::PxU32)useThisMesh->getVertices().size());

			const physx::PxVec3* normals = &useThisMesh->getNormals()[0];
			const physx::PxVec3* tangents = &useThisMesh->getTangents()[0];
			const physx::PxVec3* bitangents = &useThisMesh->getBitangents()[0];

			for (physx::PxU32 i = 0; i < tangentValues.size(); i++)
			{
				const physx::PxF32 w = physx::PxSign(normals[i].cross(tangents[i]).dot(bitangents[i]));
				PX_ASSERT(w != 0.0f);
				tangentValues[i] = physx::PxVec4(tangents[i], w);
			}
		}


		physx::apex::NxRenderMeshAssetAuthoring* renderMeshAsset = NULL;

		unsigned int numSelected = 0;
		for (size_t submeshIndex = 0, numSubmeshes = useThisMesh->getNumSubmeshes(); submeshIndex < numSubmeshes; submeshIndex++)
		{
			const Samples::TriangleSubMesh* submesh = useThisMesh->getSubMesh(submeshIndex);
			if (submesh != NULL && submesh->selected)
			{
				numSelected++;
			}
		}
		// PH: This array must be big enough from the start!
		vertexBufferDescs.reserve(numSelected);
		physx::Array<physx::apex::NxApexRenderSemanticData> customSemanticData;
		customSemanticData.reserve(numSelected * 5);

		for (size_t submeshIndex = 0, numSubmeshes = useThisMesh->getNumSubmeshes(); submeshIndex < numSubmeshes; submeshIndex++)
		{
			const Samples::TriangleSubMesh* submesh = useThisMesh->getSubMesh(submeshIndex);
			if (submesh != NULL && submesh->selected)
			{
				physx::apex::NxRenderMeshAssetAuthoring::SubmeshDesc submeshDesc;

				submeshDesc.m_numVertices = (physx::PxU32)useThisMesh->getVertices().size();

				physx::apex::NxRenderMeshAssetAuthoring::VertexBuffer vertexBufferDesc;

				vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::POSITION,
					&useThisMesh->getVertices()[0],
					sizeof(physx::PxVec3),
					physx::apex::NxRenderDataFormat::FLOAT3);

				if (useThisMesh->getNormals().size() == submeshDesc.m_numVertices)
				{
					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::NORMAL,
						&useThisMesh->getNormals()[0],
						sizeof(physx::PxVec3),
						physx::apex::NxRenderDataFormat::FLOAT3);
				}

				if (useThisMesh->getTangents().size() == submeshDesc.m_numVertices && useThisMesh->getBitangents().size() == submeshDesc.m_numVertices)
				{
#if 0
					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::TANGENT,
						&useThisMesh->getTangents()[0],
						sizeof(physx::PxVec3),
						physx::apex::NxRenderDataFormat::FLOAT3);

					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::BINORMAL,
						&useThisMesh->getBitangents()[0],
						sizeof(physx::PxVec3),
						physx::apex::NxRenderDataFormat::FLOAT3);
#else
					PX_ASSERT(tangentValues.size() == submeshDesc.m_numVertices);
					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::TANGENT,
						&tangentValues[0],
						sizeof(physx::PxVec4),
						physx::apex::NxRenderDataFormat::FLOAT4);
#endif
				}

				for (physx::PxU32 texCoord = 0; texCoord < Samples::TriangleMesh::NUM_TEXCOORDS ; texCoord++)
				{
					if (texCoord < 4 && useThisMesh->getTexCoords(texCoord).size() == submeshDesc.m_numVertices)
					{
						physx::apex::NxRenderVertexSemantic::Enum semantic =
							(physx::apex::NxRenderVertexSemantic::Enum)(physx::apex::NxRenderVertexSemantic::TEXCOORD0 + texCoord);

						vertexBufferDesc.setSemanticData(semantic,
							&useThisMesh->getTexCoords(texCoord)[0],
							sizeof(physx::apex::NxVertexUV),
							physx::apex::NxRenderDataFormat::FLOAT2);
					}
				}

				if (useThisMesh->getBoneWeights().size() == submeshDesc.m_numVertices && useThisMesh->getBoneIndices().size() == submeshDesc.m_numVertices * 4)
				{
					// check how many are actually used
					physx::PxU32 maxBonesPerSubmeshVertex = 0;
					for (physx::PxU32 index = submesh->firstIndex, end = submesh->firstIndex + submesh->numIndices; index < end; index++)
					{
						const physx::PxU32 vertexIndex = useThisMesh->getIndices()[index];
						for (physx::PxU32 j = 0; j < 4; j++)
						{
							if (useThisMesh->getBoneWeights()[vertexIndex][j] != 0.0f)
							{
								maxBonesPerSubmeshVertex = physx::PxMax(maxBonesPerSubmeshVertex, j + 1);
							}
						}
					}

					physx::apex::NxRenderDataFormat::Enum format =
						(physx::apex::NxRenderDataFormat::Enum)(physx::apex::NxRenderDataFormat::USHORT1 + maxBonesPerSubmeshVertex - 1);

					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::BONE_INDEX,
						&useThisMesh->getBoneIndices()[0],
						sizeof(physx::PxU16) * 4,
						format);

					format = (physx::apex::NxRenderDataFormat::Enum)(physx::apex::NxRenderDataFormat::FLOAT1 + maxBonesPerSubmeshVertex - 1);

					vertexBufferDesc.setSemanticData(physx::apex::NxRenderVertexSemantic::BONE_WEIGHT,
						&useThisMesh->getBoneWeights()[0],
						sizeof(physx::PxVec4),
						format);
				}

				const physx::PxU32 startCustom = customSemanticData.size();

				if (useThisMesh->getPaintChannel(Samples::PC_MAX_DISTANCE).size() == useThisMesh->getNumVertices())
				{
					physx::apex::NxApexRenderSemanticData customData;
					customData.data = &paintingValues[0].maxDistance;
					customData.stride = sizeof(PaintingValues);
					customData.format = physx::apex::NxRenderDataFormat::FLOAT1;
					customData.ident = "MAX_DISTANCE";
					customSemanticData.pushBack(customData);
				}

				if (useThisMesh->getPaintChannel(Samples::PC_COLLISION_DISTANCE).size() == useThisMesh->getNumVertices())
				{
					physx::apex::NxApexRenderSemanticData customData;
					customData.data = &paintingValues[0].collisionSphereDistance;
					customData.stride = sizeof(PaintingValues);
					customData.format = physx::apex::NxRenderDataFormat::FLOAT1;
					customData.ident = "COLLISION_SPHERE_DISTANCE";
					customSemanticData.pushBack(customData);

					customData.data = &paintingValues[0].collisionSphereRadius;
					customData.stride = sizeof(PaintingValues);
					customData.format = physx::apex::NxRenderDataFormat::FLOAT1;
					customData.ident = "COLLISION_SPHERE_RADIUS";
					customSemanticData.pushBack(customData);
				}
				if (useThisMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_SLAVE).size() == useThisMesh->getNumVertices())
				{
					physx::apex::NxApexRenderSemanticData customData;
					customData.data = &paintingValues[0].latchToNearestSlave;
					customData.stride = sizeof(PaintingValues);
					customData.format = physx::apex::NxRenderDataFormat::UINT1;
					customData.ident = "LATCH_TO_NEAREST_SLAVE";
					customSemanticData.pushBack(customData);
				}
				if (useThisMesh->getPaintChannel(Samples::PC_LATCH_TO_NEAREST_MASTER).size() == useThisMesh->getNumVertices())
				{
					physx::apex::NxApexRenderSemanticData customData;
					customData.data = &paintingValues[0].latchToNearestMaster;
					customData.stride = sizeof(PaintingValues);
					customData.format = physx::apex::NxRenderDataFormat::UINT1;
					customData.ident = "LATCH_TO_NEAREST_MASTER";
					customSemanticData.pushBack(customData);
				}

				if (startCustom < customSemanticData.size())
				{
					vertexBufferDesc.setCustomSemanticData(&customSemanticData[startCustom], customSemanticData.size() - startCustom);
				}

				vertexBufferDescs.pushBack(vertexBufferDesc);

				submeshDesc.m_materialName = submesh->materialName.c_str();
				submeshDesc.m_vertexBuffers = &vertexBufferDescs.back();
				submeshDesc.m_numVertexBuffers = 1;

				submeshDesc.m_primitive = physx::apex::NxRenderMeshAssetAuthoring::Primitive::TRIANGLE_LIST;
				submeshDesc.m_indexType = physx::apex::NxRenderMeshAssetAuthoring::IndexType::UINT;

				submeshDesc.m_vertexIndices = &useThisMesh->getIndices()[0] + submesh->firstIndex;
				submeshDesc.m_numIndices = submesh->numIndices;
				submeshDesc.m_firstVertex = 0;
				submeshDesc.m_partIndices = NULL;
				submeshDesc.m_numParts = 0;

				submeshDesc.m_cullMode = ClothingAuthoring::getCullMode();

				submeshDescs.pushBack(submeshDesc);
			}
		}

		PX_ASSERT(customSemanticData.capacity() == numSelected * 5);

		physx::apex::NxRenderMeshAssetAuthoring::MeshDesc meshDesc;
		meshDesc.m_numSubmeshes = submeshDescs.size();
		meshDesc.m_submeshes = submeshDescs.begin();
		meshDesc.m_uvOrigin = mMeshes.inputMesh->getTextureUVOrigin();

		char rmaName[30];
		if (graphicalLod == 0)
		{
			sprintf(rmaName, "TheRMA");
		}
		else
		{
			sprintf(rmaName, "TheRMA_%i", graphicalLod);
		}
		renderMeshAsset = static_cast<physx::apex::NxRenderMeshAssetAuthoring*>(_mApexSDK->createAssetAuthoring(NX_RENDER_MESH_AUTHORING_TYPE_NAME, rmaName));

		renderMeshAsset->createRenderMesh(meshDesc, true);


		if (renderMeshAsset != NULL)
		{
			mAuthoringObjects.renderMeshAssets.push_back(renderMeshAsset);
		}
	}

	setAuthoringState(AuthoringState::RenderMeshAssetCreated, true);
}



void ClothingAuthoring::createCustomMesh()
{
	createRenderMeshAssets();

	if (mMeshes.inputMesh != NULL && mMeshes.customPhysicsMesh != NULL)
	{
		if (mAuthoringObjects.physicalMeshes.size() == 0)
		{
			physx::apex::NxClothingPhysicalMesh* physicalMesh = _mModuleClothing->createEmptyPhysicalMesh();
			physicalMesh->setGeometry(false,
			                          (physx::PxU32)mMeshes.customPhysicsMesh->getVertices().size(),
			                          sizeof(physx::PxVec3),
			                          &mMeshes.customPhysicsMesh->getVertices()[0],
			                          (physx::PxU32)mMeshes.customPhysicsMesh->getIndices().size(),
			                          sizeof(physx::PxU32),
			                          &mMeshes.customPhysicsMesh->getIndices()[0]);

			mAuthoringObjects.physicalMeshes.push_back(physicalMesh);

			setAuthoringState(AuthoringState::PhysicalCustomMeshCreated, true);
		}
		PX_ASSERT(mAuthoringObjects.physicalMeshes.size() == 1);
	}
}



void ClothingAuthoring::createClothMeshes(physx::apex::IProgressListener* progressParent)
{
	if (mState.authoringState >= AuthoringState::PhysicalClothMeshCreated)
	{
		return;
	}

	createRenderMeshAssets();

	for (size_t i = 0; i < mAuthoringObjects.physicalMeshes.size(); i++)
	{
		if (mAuthoringObjects.physicalMeshes[i] != NULL)
		{
			mAuthoringObjects.physicalMeshes[i]->release();
		}

		mAuthoringObjects.physicalMeshes[i] = NULL;
	}
	mAuthoringObjects.physicalMeshes.clear();

	ProgressCallback progress(mConfig.cloth.numGraphicalLods, progressParent);

	//mApexPhysicalMeshes.resize(ClothingAuthoring::getNumRenderMeshAssets());
	for (int i = 0; i < mConfig.cloth.numGraphicalLods; i++)
	{
		progress.setSubtaskWork(1, "Create Physical Mesh");
		physx::apex::NxClothingPhysicalMesh* physicalMesh = _mModuleClothing->createSingleLayeredMesh(
		            mAuthoringObjects.renderMeshAssets[i],
		            mConfig.cloth.subdivide ? mConfig.cloth.subdivision : 0,
		            true, // mergeVerticest
		            mConfig.cloth.close,
		            &progress);

		mAuthoringObjects.physicalMeshes.push_back(physicalMesh);

		progress.completeSubtask();
	}

	setAuthoringState(AuthoringState::PhysicalClothMeshCreated, true);
	mConfig.mesh.physicalMeshType = 0;
}



void ClothingAuthoring::createIsoMesh(physx::apex::IProgressListener* progress)
{
	if (mState.authoringState >= AuthoringState::PhysicalIsoMeshCreated)
	{
		return;
	}

	physx::apex::NxRenderMeshAssetAuthoring* renderMeshAsset = getRenderMeshAsset(0);

	if (mAuthoringObjects.isoMesh != NULL)
	{
		mAuthoringObjects.isoMesh->release();
		mAuthoringObjects.isoMesh = NULL;
	}

	mAuthoringObjects.isoMesh = _mModuleClothing->createMultiLayeredMesh(renderMeshAsset, mConfig.softbody.isoSurfaceSubdivision,
	                            mConfig.softbody.keepBiggestMeshes, mConfig.softbody.discardInnerMeshes, progress);

	setAuthoringState(AuthoringState::PhysicalIsoMeshCreated, true);
	mConfig.mesh.physicalMeshType = 1;
}



void ClothingAuthoring::createTetraMesh(physx::apex::IProgressListener* progressParent)
{
	if (mState.authoringState >= AuthoringState::PhysicalTetraMeshcreated)
	{
		return;
	}

	ProgressCallback progress(100, progressParent);

	if (mState.authoringState < AuthoringState::PhysicalIsoMeshCreated)
	{
		progress.setSubtaskWork(50, "Create Iso Mesh");
		createIsoMesh(&progress);
		progress.completeSubtask();
	}

	progress.setSubtaskWork(-1, "Create Tetra Mesh");

	for (size_t i = 0; i < mAuthoringObjects.physicalMeshes.size(); i++)
	{
		if (mAuthoringObjects.physicalMeshes[i] != NULL)
		{
			mAuthoringObjects.physicalMeshes[i]->release();
		}

		mAuthoringObjects.physicalMeshes[i] = NULL;
	}
	mAuthoringObjects.physicalMeshes.clear();

	mAuthoringObjects.physicalMeshes.push_back(mAuthoringObjects.isoMesh->generateSoftbodyMesh(0, &progress));

	progress.completeSubtask();

	setAuthoringState(AuthoringState::PhysicalTetraMeshcreated, true);
	mConfig.mesh.physicalMeshType = 1;
}



void ClothingAuthoring::createClothingAsset(physx::apex::IProgressListener* progressParent)
{
	if (mState.authoringState <= AuthoringState::None || mState.authoringState >= AuthoringState::ClothingAssetCreated)
	{
		return;
	}

	if (mAuthoringObjects.clothingAssetAuthoring != NULL)
	{
		mAuthoringObjects.clothingAssetAuthoring->release();
		mAuthoringObjects.clothingAssetAuthoring = NULL;
	}

	unsigned int totalWork = mConfig.cloth.numGraphicalLods * 2;
	ProgressCallback progress(totalWork, progressParent);

	if (mState.authoringState <= AuthoringState::RenderMeshAssetCreated)
	{
		createRenderMeshAssets();

		progress.setSubtaskWork(mConfig.cloth.numGraphicalLods, "Create Physics Mesh");

		if (mMeshes.customPhysicsMesh != NULL)
		{
			// only 1 lod
			createCustomMesh();
		}
		else if (mConfig.mesh.physicalMeshType == 0) //cloth
		{
			// all lods
			createClothMeshes(&progress);
		}
		else
		{
			// only 1 lod
			createTetraMesh(&progress);
		}

		progress.completeSubtask();
	}

	{
		physx::apex::NxApexAssetAuthoring* assetAuthoring = _mApexSDK->createAssetAuthoring(NX_CLOTHING_AUTHORING_TYPE_NAME, "The Authoring Asset");
		assetAuthoring->setToolString("ClothingTool", NULL, 0);

		PX_ASSERT(assetAuthoring != NULL);
		if (assetAuthoring != NULL)
		{
			mAuthoringObjects.clothingAssetAuthoring = static_cast<physx::apex::NxClothingAssetAuthoring*>(assetAuthoring);

			updateDeformableParameters();

			mAuthoringObjects.clothingAssetAuthoring->setDeriveNormalsFromBones(ClothingAuthoring::getDeriveNormalsFromBones());

			if (mMeshes.skeleton != NULL)
			{
				const std::vector<Samples::SkeletalBone>& bones = mMeshes.skeleton->getBones();
				for (unsigned int i = 0; i < (unsigned int)bones.size(); i++)
				{
					mAuthoringObjects.clothingAssetAuthoring->setBoneInfo(i, bones[i].name.c_str(), bones[i].bindWorldPose, bones[i].parent);
					if (bones[i].isRoot)
					{
						mAuthoringObjects.clothingAssetAuthoring->setRootBone(bones[i].name.c_str());
					}
				}
			}
		}
	}

	PX_ASSERT(!mAuthoringObjects.physicalMeshes.empty());

	for (int i = 0; i < (int)mAuthoringObjects.physicalMeshes.size(); i++)
	{
		progress.setSubtaskWork(1, "combine graphical and physical mesh");

		const int lod = mState.authoringState == AuthoringState::PhysicalClothMeshCreated ? mConfig.cloth.numGraphicalLods - 1 - i : 0;

		int numPhysicalLods = (int)mMeshes.physicalLodValues.size();
		float* physicalLodValues = (mMeshes.physicalLodValues.size() > 0) ? &mMeshes.physicalLodValues[0] : NULL;

		mAuthoringObjects.clothingAssetAuthoring->setMeshes(lod,
		        mAuthoringObjects.renderMeshAssets[i],
		        mAuthoringObjects.physicalMeshes[i],
		        numPhysicalLods, physicalLodValues,
		        25.0f, true, &progress);

		progress.completeSubtask();
	}

	setAuthoringState(AuthoringState::ClothingAssetCreated, true);
}



void ClothingAuthoring::updateDeformableParameters()
{
	float thicknessScaling = 1.0f;
	if (mMeshes.inputMesh != NULL)
	{
		physx::PxBounds3 bounds;
		mMeshes.inputMesh->getBounds(bounds);
		thicknessScaling = 0.1f * (bounds.minimum - bounds.maximum).magnitude();
	}

	if (mAuthoringObjects.clothingAssetAuthoring != NULL)
	{
		mAuthoringObjects.clothingAssetAuthoring->setSimulationThickness(mConfig.deformable.thickness * thicknessScaling);
		mAuthoringObjects.clothingAssetAuthoring->setSimulationVirtualParticleDensity(mConfig.deformable.virtualParticleDensity);
		mAuthoringObjects.clothingAssetAuthoring->setSimulationHierarchicalLevels(mConfig.deformable.hierarchicalLevels);
		mAuthoringObjects.clothingAssetAuthoring->setSimulationGravityDirection(mConfig.ui.zAxisUp ? physx::PxVec3(0.0f, 0.0f, -1.0f) : physx::PxVec3(0.0f, -1.0f, 0.0f));

		mAuthoringObjects.clothingAssetAuthoring->setSimulationDisableCCD(mConfig.deformable.disableCCD);
		mAuthoringObjects.clothingAssetAuthoring->setSimulationTwowayInteraction(mConfig.deformable.twowayInteraction);
		mAuthoringObjects.clothingAssetAuthoring->setSimulationUntangling(mConfig.deformable.untangling);

		mAuthoringObjects.clothingAssetAuthoring->setSimulationRestLengthScale(mConfig.deformable.restLengthScale);
	}
}



void ClothingAuthoring::Simulation::clear()
{
	for (size_t i = 0; i < actors.size(); i++)
	{
		for (size_t j = 0; j < actors[i].actors.size(); j++)
		{
			if (actors[i].actors[j] != NULL)
			{
				actors[i].actors[j]->release();
				actors[i].actors[j] = NULL;
			}
		}
		for (size_t j = 0; j < actors[i].previews.size(); j++)
		{
			if (actors[i].previews[j] != NULL)
			{
				actors[i].previews[j]->release();
				actors[i].previews[j] = NULL;
			}
		}
	}
	actors.clear();

	if (clearAssets)
	{
		for (size_t i = 0; i < assets.size(); i++)
		{
			for (size_t j = 0; j < assets[i].renderMeshAssets.size(); ++j)
			{
				if (assets[i].renderMeshAssets[j] != NULL)
				{
					assets[i].renderMeshAssets[j]->release();
				}
			}
			assets[i].renderMeshAssets.clear();

			assets[i].releaseRenderMeshAssets();
			assets[i].apexAsset->release();
			assets[i].apexAsset = NULL;
		}
	}
	assets.clear();
	clearAssets = true;

	actorCount = 1;
	stepsUntilPause = 0;
}



void ClothingAuthoring::Meshes::clear(physx::apex::NxUserRenderResourceManager* rrm, physx::apex::NxResourceCallback* rcb, bool groundAsWell)
{
	if (inputMesh != NULL)
	{
		inputMesh->clear(rrm, rcb);
		delete inputMesh;
		inputMesh = NULL;
	}
	//inputMeshFilename.clear(); // PH: MUST not be reset

	if (painter != NULL)
	{
		delete painter;
		painter = NULL;
	}
	physicalLodValues.clear();

	if (customPhysicsMesh != NULL)
	{
		customPhysicsMesh->clear(rrm, rcb);
		delete customPhysicsMesh;
		customPhysicsMesh = NULL;
	}
	customPhysicsMeshFilename.clear();

	if (groundMesh != NULL && groundAsWell)
	{
		groundMesh->clear(rrm, rcb);
		delete groundMesh;
		groundMesh = NULL;
	}

	if (skeleton != NULL)
	{
		delete skeleton;
		skeleton = NULL;
	}

	if (skeletonBehind != NULL)
	{
		delete skeletonBehind;
		skeletonBehind = NULL;
	}
	skeletonRemap.clear();

	collisionVolumes.clear();
	subdivideHistory.clear();
}



void ClothingAuthoring::AuthoringObjects::clear()
{
	for (size_t i = 0; i < renderMeshAssets.size(); i++)
	{
		if (renderMeshAssets[i] != NULL)
		{
			renderMeshAssets[i]->release();
		}

		renderMeshAssets[i] = NULL;
	}
	renderMeshAssets.clear();

	for (size_t i = 0; i < physicalMeshes.size(); i++)
	{
		if (physicalMeshes[i] != NULL)
		{
			physicalMeshes[i]->release();
		}
		physicalMeshes[i] = NULL;
	}
	physicalMeshes.clear();

	if (isoMesh != NULL)
	{
		isoMesh->release();
		isoMesh = NULL;
	}

	if (clothingAssetAuthoring != NULL)
	{
		clothingAssetAuthoring->release();
		clothingAssetAuthoring = NULL;
	}
}


void ClothingAuthoring::clearLoadedActorDescs()
{
	for (physx::PxU32 i = 0; i < mLoadedActorDescs.size(); ++i)
	{
		mLoadedActorDescs[i]->destroy();
	}
	mLoadedActorDescs.clear();
	mMaxTimesteps.clear();
	mMaxIterations.clear();
	mTimestepMethods.clear();
	mDts.clear();
	mGravities.clear();
}


} // namespace SharedTools

#endif // defined(PX_WINDOWS)
