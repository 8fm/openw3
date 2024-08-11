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

#include "CmCollection.h"
#include "CmIO.h"

using namespace physx;
using namespace Cm;

void Collection::add(PxBase& object, PxSerialObjectId id)
{
	PX_CHECK_AND_RETURN(!contains(object), "PxCollection::add called with PxBase already contained in the collection!");
	PX_CHECK_AND_RETURN(id == PX_SERIAL_OBJECT_ID_INVALID || !find(id), "PxCollection::add called with PxSerialObjectId already used in the collection!");
	mArray.pushBack(&object);

	if (id != PX_SERIAL_OBJECT_ID_INVALID)
	{			
		addId(object, id);
	}
}

void Collection::remove(PxBase& object)
{
	PX_CHECK_AND_RETURN(contains(object), "PxCollection::remove called with PxBase not contained in the collection!");
	mArray.findAndReplaceWithLast(&object);

	//TODO, fast way to find ref
	UserHashMapResolver::Iterator it = mIds.getIterator();
	while (!it.done() && it->second != &object)
		++it;

	//This is twice unoptimal since our hash doesn't have erase given an iterator
	if (!it.done() && it->second == &object)
		mIds.erase(it->first);
}

void Collection::addRequired(PxBase& object)
{
	if(!contains(object))
		mArray.pushBack(&object);
}

bool Collection::contains(PxBase& object) const
{
	//TODO: should this be faster?	
	return mArray.end() != mArray.find(&object);
}

void Collection::addId(PxBase& object, PxSerialObjectId id)
{
	PX_CHECK_AND_RETURN(contains(object), "PxCollection::addId called with PxBase not contained in the collection!");
	PX_CHECK_AND_RETURN(id != PX_SERIAL_OBJECT_ID_INVALID, "PxCollection::addId called with PxSerialObjectId being set to PX_SERIAL_OBJECT_ID_INVALID!");

	if(mIds.insert(id, &object))
		mIds[id] = &object;
}

void Collection::removeId(PxSerialObjectId id)
{
	PX_CHECK_AND_RETURN(id != PX_SERIAL_OBJECT_ID_INVALID, "PxCollection::removeId called with PxSerialObjectId being set to PX_SERIAL_OBJECT_ID_INVALID!");
	PX_CHECK_AND_RETURN(mIds.find(id), "PxCollection::removeId called with PxSerialObjectId not contained in the collection!");
	mIds.erase(id);
}

PxBase* Collection::find(PxSerialObjectId id) const
{
	PX_CHECK_AND_RETURN_NULL(id != PX_SERIAL_OBJECT_ID_INVALID, "PxCollection::find called with PxSerialObjectId being set to PX_SERIAL_OBJECT_ID_INVALID!");
	const UserHashMapResolver::Entry* e = mIds.find(id);
	return e ? static_cast<PxBase*>(e->second) : NULL;
}

void Collection::add(PxCollection& _collection)
{
	Collection& collection = static_cast<Collection&>(_collection);
	PX_CHECK_AND_RETURN(this != &collection, "PxCollection::add(PxCollection&) called with itself!");

	mArray.reserve(mArray.capacity() + collection.mArray.size());
	for (PxU32 i = 0; i < collection.mArray.size(); ++i)
	{
		//this would be another reason to use a set.
		addRequired(*collection.mArray[i]);
	}

	UserHashMapResolver::Iterator it = collection.mIds.getIterator();
	while (!it.done())
	{
		if(!mIds.insert(it->first, it->second))
		{
			PX_CHECK_MSG(mIds[it->first] == it->second, "PxCollection::add(PxCollection&) called with conflicting id!");
		}
		++it;
	}
}

void Collection::remove(PxCollection& _collection)
{
	Collection& collection = static_cast<Collection&>(_collection);
	PX_CHECK_AND_RETURN(this != &collection, "PxCollection::remove(PxCollection&) called with itself!");

	for (PxU32 i = 0; i < collection.mArray.size(); ++i)
	{
		//this would be another reason to use a set.
		if (contains(*collection.mArray[i]))
			remove(*collection.mArray[i]);
	}
}

PxU32 Collection::getNbObjects() const
{
	return mArray.size();
}

PxBase& Collection::getObject(PxU32 index) const
{
	PX_ASSERT(index < mArray.size());
	return *mArray[index];
}

PxU32 Collection::getObjects(PxBase** userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	PX_CHECK_AND_RETURN_NULL(userBuffer != NULL, "PxCollection::getObjects called with userBuffer NULL!");
	PX_CHECK_AND_RETURN_NULL(bufferSize != 0, "PxCollection::getObjects called with bufferSize 0!");
	PxU32 dstIndex = 0;
	for (PxU32 srcIndex = startIndex; srcIndex < mArray.size() && dstIndex < bufferSize; ++srcIndex)
		userBuffer[dstIndex++] = mArray[srcIndex];

	return dstIndex;
}

PxU32 Collection::getNbIds() const
{
	return mIds.size();
}

PxSerialObjectId Collection::getId(const PxBase& object) const
{	
	UserHashMapResolver::Iterator srcIt = (const_cast<UserHashMapResolver&>(mIds)).getIterator();

	while (!srcIt.done())
	{
		if(static_cast<PxBase*>(srcIt->second) == &object)
		{
			break; //return srcIt->first.mUserID;;
		}
		srcIt++;
	}

	return srcIt.done() ? PX_SERIAL_OBJECT_ID_INVALID : srcIt->first;
}
PxU32 Collection::getIds(PxSerialObjectId* userBuffer, PxU32 bufferSize, PxU32 startIndex) const
{
	PX_CHECK_AND_RETURN_NULL(userBuffer != NULL, "PxCollection::getIds called with userBuffer NULL!");
	PX_CHECK_AND_RETURN_NULL(bufferSize != 0, "PxCollection::getIds called with bufferSize 0!");
	PxU32 dstIndex = 0;

	UserHashMapResolver::Iterator srcIt = (const_cast<UserHashMapResolver&>(mIds)).getIterator();

	while (!srcIt.done() &&  dstIndex < bufferSize)
	{
		if(srcIt->first != PX_SERIAL_OBJECT_ID_INVALID)
		{
			if(startIndex > 0)
				startIndex--;
			else
				userBuffer[dstIndex++] = srcIt->first;
		}
		srcIt++;
	}

	return dstIndex;
}

PxCollection*	PxCreateCollection()
{
	return PX_NEW(Collection);
}
