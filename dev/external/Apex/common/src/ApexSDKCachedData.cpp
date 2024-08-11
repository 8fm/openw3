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

#include "ApexSDKCachedData.h"
#include "NxParameterized.h"

namespace physx
{
namespace apex
{

bool ApexSDKCachedData::registerModuleDataCache(NiApexModuleCachedData* cache)
{
	if (cache == NULL)
	{
		return false;
	}

	for (physx::PxU32 i = 0; i < mModuleCaches.size(); ++i)
	{
		if (cache == mModuleCaches[i])
		{
			return false;
		}
	}

	mModuleCaches.pushBack(cache);

	return true;
}

bool ApexSDKCachedData::unregisterModuleDataCache(NiApexModuleCachedData* cache)
{
	if (cache == NULL)
	{
		return false;
	}

	for (physx::PxU32 i = mModuleCaches.size(); i--;)
	{
		if (cache == mModuleCaches[i])
		{
			mModuleCaches.replaceWithLast(i);
			break;
		}
	}

	return false;
}

ApexSDKCachedData::ApexSDKCachedData()
{
}

ApexSDKCachedData::~ApexSDKCachedData()
{
}

NxApexModuleCachedData* ApexSDKCachedData::getCacheForModule(NxAuthObjTypeID moduleID)
{
	for (physx::PxU32 i = 0; i < mModuleCaches.size(); ++i)
	{
		if (moduleID == mModuleCaches[i]->getModuleID())
		{
			return mModuleCaches[i];
		}
	}

	return NULL;
}

physx::PxFileBuf& ApexSDKCachedData::serialize(physx::PxFileBuf& stream) const
{
	stream.storeDword((physx::PxU32)Version::Current);

	for (physx::PxU32 i = 0; i < mModuleCaches.size(); ++i)
	{
		mModuleCaches[i]->serialize(stream);
	}

	return stream;
}

physx::PxFileBuf& ApexSDKCachedData::deserialize(physx::PxFileBuf& stream)
{
	clear();

	/*const physx::PxU32 version =*/
	stream.readDword();	// Original version

	for (physx::PxU32 i = 0; i < mModuleCaches.size(); ++i)
	{
		mModuleCaches[i]->deserialize(stream);
	}

	return stream;
}

void ApexSDKCachedData::clear()
{
	for (physx::PxU32 i = mModuleCaches.size(); i--;)
	{
		mModuleCaches[i]->clear();
	}
}

}
} // end namespace physx::apex
