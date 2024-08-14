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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PXS_MATERIALMANAGER
#define PXS_MATERIALMANAGER

#include "PxsMaterialCore.h"
#include "PsAlignedMalloc.h"
#ifdef PX_PS3
#include "ps3/PxPS3Config.h"
#endif

namespace physx
{
	struct PxsMaterialInfo
	{
		PxU16 mMaterialIndex0;
		PxU16 mMaterialIndex1;
	};

	class PxsMaterialManager 
	{
	public:
		PxsMaterialManager()
		{
#ifndef __SPU__
			const PxU32 matCount = 
#ifdef PX_PS3
			PX_PS3_MAX_MATERIAL_COUNT;
#else
			128;
#endif

			materials = (PxsMaterialCore*)physx::shdfnd::AlignedAllocator<16>().allocate(sizeof(PxsMaterialCore)*matCount,  __FILE__, __LINE__);
			maxMaterials = matCount;
			for(PxU32 i=0; i<matCount; ++i)
			{
				materials[i].setMaterialIndex(MATERIAL_INVALID_HANDLE);
			}
#endif
		}

		~PxsMaterialManager()
		{
#ifndef __SPU__
			physx::shdfnd::AlignedAllocator<16>().deallocate(materials);
#endif
		}

		void setMaterial(PxsMaterialCore* mat)
		{
			const PxU32 materialIndex = mat->getMaterialIndex();
			resize(materialIndex+1);
			materials[materialIndex] = *mat;
		}

		void updateMaterial(PxsMaterialCore* mat)
		{
			materials[mat->getMaterialIndex()] =*mat;
		}

		void removeMaterial(PxsMaterialCore* mat)
		{
			mat->setMaterialIndex(MATERIAL_INVALID_HANDLE);
		}

		PX_FORCE_INLINE PxsMaterialCore* getMaterial(const PxU32 index)const
		{
			PX_ASSERT(index <  maxMaterials);
			return &materials[index];
		}

		PxU32 getMaxSize()const 
		{
			return maxMaterials;
		}

		void resize(PxU32 minValueForMax)
		{
#ifndef __SPU__

			if(maxMaterials>=minValueForMax)
				return;

			const PxU32 numMaterials = maxMaterials;
			
			maxMaterials = (minValueForMax+31)&~31;
			PxsMaterialCore* mat = (PxsMaterialCore*)physx::shdfnd::AlignedAllocator<16>().allocate(sizeof(PxsMaterialCore)*maxMaterials,  __FILE__, __LINE__);
			for(PxU32 i=0; i<numMaterials; ++i)
			{
				mat[i] = materials[i];
			}
			for(PxU32 i = numMaterials; i < maxMaterials; ++i)
			{
				mat[i].setMaterialIndex(MATERIAL_INVALID_HANDLE);
			}

			physx::shdfnd::AlignedAllocator<16>().deallocate(materials);

			materials = mat;
#endif
		}

		PxsMaterialCore* materials;//make sure materials's start address is 16 bytes align
		PxU32 maxMaterials;
		PxU32 mPad[2];
	};

	class PxsMaterialManagerIterator
	{
	
	public:
		PxsMaterialManagerIterator(PxsMaterialManager& manager) : mManager(manager), index(0)
		{
		}

		bool hasNextMaterial()
		{
			while(index < mManager.getMaxSize() && mManager.getMaterial(index)->getMaterialIndex() == MATERIAL_INVALID_HANDLE)
				index++;
			return index < mManager.getMaxSize();
		}

		PxsMaterialCore* getNextMaterial()
		{
			PX_ASSERT(index < mManager.getMaxSize());
			return mManager.getMaterial(index++);
		}

	private:
		PxsMaterialManagerIterator& operator=(const PxsMaterialManagerIterator&);
		PxsMaterialManager& mManager;
		PxU32 index;

	};

}

#endif
