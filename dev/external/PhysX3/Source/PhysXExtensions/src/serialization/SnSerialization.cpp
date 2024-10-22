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

#include "PxPhysicsAPI.h"
#include "PxConstraintExt.h"
#include "PsFoundation.h"
#include "PxMetaData.h"
#include "SnConvX.h"
#include "SnSerializationRegistry.h"
#include "SnSerialUtils.h"
#include "ExtSerialization.h"
#include "PxSerializer.h"
#include "CmCollection.h"

using namespace physx;
using namespace Sn;

namespace
{
	struct RequiresCallback : public PxProcessPxBaseCallback
	{
		PX_NOCOPY(RequiresCallback)	
		RequiresCallback(physx::PxCollection& c) : collection(c) {}
		void process(PxBase& base)
		{			
			   collection.addRequired(base);
		}
    
		PxCollection& collection;
	};

	struct CompleteCallback : public PxProcessPxBaseCallback
	{
		PX_NOCOPY(CompleteCallback)
		CompleteCallback(physx::PxCollection& r, physx::PxCollection& c, const physx::PxCollection* e) :
		requires(r),  complete(c), external(e)	{}
		void process(PxBase& base)
		{
			if(complete.contains(base) || (external && external->contains(base)))
			   return;
			requires.addRequired(base);
		}

		PxCollection& requires;		
		PxCollection& complete;
		const PxCollection* external;
	};

	void getRequiresCollection(PxCollection& requires, PxCollection& collection, PxCollection& complete, const PxCollection* external, PxSerializationRegistry& sr, bool followJoints)
	{
		CompleteCallback callback(requires, complete, external);
		for (PxU32 i = 0; i < collection.getNbObjects(); ++i)
		{
			PxBase& s = collection.getObject(i);			
			const PxSerializer* serializer = sr.getSerializer(s.getConcreteType());
			PX_ASSERT(serializer);
			serializer->requires(s, callback);

			if(followJoints)
			{
				PxRigidActor* actor = s.is<PxRigidActor>();
				if(actor)
				{
					Ps::Array<PxConstraint*> objects(actor->getNbConstraints());
					actor->getConstraints(objects.begin(), objects.size());

					for(PxU32 j=0;j<objects.size();j++)
					{
						PxU32 typeId;
						PxJoint* joint = reinterpret_cast<PxJoint*>(objects[j]->getExternalReference(typeId));				
						if(typeId == PxConstraintExtIDs::eJOINT)
						{							
							const PxSerializer* serializer = sr.getSerializer(joint->getConcreteType());
							PX_ASSERT(serializer);
							serializer->requires(*joint, callback);
							if(!requires.contains(*joint))
								requires.add(*joint);							
						}
					}
				}
			}
		}	
	}
}

bool PxSerialization::isSerializable(PxCollection& collection, PxSerializationRegistry& sr, const PxCollection* externalReferences) 
{		
	bool bRet = true;	

	PxCollection* subordinateCollection = PxCreateCollection();
	PX_ASSERT(subordinateCollection);

	for(PxU32 i = 0; i < collection.getNbObjects(); ++i)
	{
		PxBase& s = collection.getObject(i);
		const PxSerializer* serializer = sr.getSerializer(s.getConcreteType());
		PX_ASSERT(serializer);
		if(serializer->isSubordinate())
			subordinateCollection->add(s);

		if(externalReferences)
		{
			PxSerialObjectId id = collection.getId(s);
			if(id != PX_SERIAL_OBJECT_ID_INVALID)
			{
				PxBase* object = externalReferences->find(id);
				if(object && (object != &s))
				{					
					subordinateCollection->release();
					Ps::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
						"NpCollection::isSerializable: object shares reference name with other object in externalReferences (reference clash).");
					return false;
				}
			}
		}		
	}

	PxCollection* requiresCollection = PxCreateCollection();
	PX_ASSERT(requiresCollection);

	RequiresCallback requiresCallback(*requiresCollection);

	for (PxU32 i = 0; i < collection.getNbObjects(); ++i)
	{
		PxBase& s = collection.getObject(i);
		const PxSerializer* serializer = sr.getSerializer(s.getConcreteType());
		PX_ASSERT(serializer);
		serializer->requires(s, requiresCallback);
	}
	
	for(PxU32 j = 0; j < subordinateCollection->getNbObjects(); ++j)
	{
		PxBase& subordinate = subordinateCollection->getObject(j);
		bRet = requiresCollection->contains(subordinate);
		if(!bRet)
		{
			requiresCollection->release();
			subordinateCollection->release();
			Ps::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
				"NpCollection::isSerializable: object is a subordinate and not required by other objects in the collection (orphan).");
			return false;		
		}
	}
	subordinateCollection->release();

	for(PxU32 j = 0; j < requiresCollection->getNbObjects(); ++j)
	{
		PxBase& s0 = requiresCollection->getObject(j);
		bRet = collection.contains(s0);
		if(!bRet && externalReferences)
		{
			bRet = externalReferences->contains(s0) &&  externalReferences->getId(s0) != PX_SERIAL_OBJECT_ID_INVALID;
		}
		if(!bRet)
		{
			requiresCollection->release();
			Ps::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
				"NpCollection::isSerializable: cannot find a required serial object (collection is not complete).");
			return false;
		}
	}
	requiresCollection->release();

	if(externalReferences)
	{
		PxCollection* oppsiteRequiresCollection = PxCreateCollection();
		PX_ASSERT(oppsiteRequiresCollection);

		RequiresCallback requiresCallback(*oppsiteRequiresCollection);

		for (PxU32 i = 0; i < externalReferences->getNbObjects(); ++i)
		{
			PxBase& s = externalReferences->getObject(i);			
			const PxSerializer* serializer = sr.getSerializer(s.getConcreteType());
			PX_ASSERT(serializer);
			serializer->requires(s, requiresCallback);
		}

		for(PxU32 j = 0; j < oppsiteRequiresCollection->getNbObjects(); ++j)
		{
			PxBase& s0 = oppsiteRequiresCollection->getObject(j);

			if(collection.contains(s0))
			{
				oppsiteRequiresCollection->release();
				Ps::getFoundation().error(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
					"NpCollection::isSerializable: object in externalReferences requires an object in collection (circular dependency).");
				return false;
			}
		}
		oppsiteRequiresCollection->release();
	}

	return true;
}

void PxSerialization::complete(PxCollection& collection, PxSerializationRegistry& sr, const PxCollection* exceptFor, bool followJoints)
{	
	PxCollection* curCollection = PxCreateCollection();
	PX_ASSERT(curCollection);	
	curCollection->add(collection);

	PxCollection* requiresCollection = PxCreateCollection();
	PX_ASSERT(requiresCollection);

	do
	{		
		getRequiresCollection(*requiresCollection, *curCollection, collection, exceptFor, sr, followJoints);		
		
		collection.add(*requiresCollection);
		PxCollection* swap = curCollection;	
		curCollection = requiresCollection;
		requiresCollection = swap;
		(static_cast<Cm::Collection*>(requiresCollection))->mArray.clear();

	}while(curCollection->getNbObjects() > 0);

	requiresCollection->release();
	curCollection->release();
	
}

void PxSerialization::createSerialObjectIds(PxCollection& collection, const PxSerialObjectId base)
{
	PxSerialObjectId localBase = base;
	PxU32 nbObjects = collection.getNbObjects();

	for (PxU32 i = 0; i < nbObjects; ++i)
	{
		while(collection.find(localBase))
		{
			localBase++;
		}

		PxBase& s = collection.getObject(i);		
		if(PX_SERIAL_OBJECT_ID_INVALID == collection.getId(s))
		{
			collection.addId(s, localBase);
			localBase++;
		}
	}
}

namespace physx { namespace Sn
{
	static PxU32 addToStringTable(physx::shdfnd::Array<char>& stringTable, const char* str)
	{
		if(!str)
			return 0xffffffff;

		PxI32 length = PxI32(stringTable.size());
		const char* table = stringTable.begin();
		const char* start = table;
		while(length)
		{
			if(strcmp(table, str)==0)
				return PxU32(table - start);

			const char* saved = table;
			while(*table++);
			length -= PxU32(table - saved);
			PX_ASSERT(length>=0);
		}

		const PxU32 offset = stringTable.size();

		while(*str)
			stringTable.pushBack(*str++);
		stringTable.pushBack(0);
		return offset;
	}
} };

void PxSerialization::dumpBinaryMetaData(PxOutputStream& outputStream, PxSerializationRegistry& sr)
{
	class MetaDataStream : public PxOutputStream
	{
	public:
		bool addNewType(const char* typeName)
		{			
			for(PxU32 i=0;i<types.size();i++)
			{			
				if(strcmp(types[i], typeName)==0)
					return false;
			}
			types.pushBack(typeName);
			return true;
		}
		virtual	PxU32 write(const void* src, PxU32 count)
		{
			PX_ASSERT(count==sizeof(PxMetaDataEntry));
			const PxMetaDataEntry* entry = (const PxMetaDataEntry*)src;
			if(( entry->flags & PxMetaDataFlag::eTYPEDEF) || ((entry->flags & PxMetaDataFlag::eCLASS) && (!entry->name)) )
                 newType = addNewType(entry->type);
			if(newType)
			   metaData.pushBack(*entry);
			return count;
		}		
		shdfnd::Array<PxMetaDataEntry> metaData;
		shdfnd::Array<const char*> types;
		bool newType;
	}s;

	SerializationRegistry& sn = static_cast<SerializationRegistry&>(sr);
	sn.getBinaryMetaData(s);

	shdfnd::Array<char>	stringTable;

	PxU32 nb = s.metaData.size();
	PxMetaDataEntry* entries = s.metaData.begin();
	for(PxU32 i=0;i<nb;i++)
	{
		entries[i].type = (const char*)size_t(addToStringTable(stringTable, entries[i].type));
		entries[i].name = (const char*)size_t(addToStringTable(stringTable, entries[i].name));
	}

	PxU32 platformTag = getBinaryPlatformTag();

#ifdef PX_X64
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::ePC);
	const PxU32 tiledHeightFieldSamples = 0;
#endif
#if defined(PX_X86) || defined(__CYGWIN__)
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::ePC);
	const PxU32 tiledHeightFieldSamples = 0;
#endif
#ifdef PX_X360
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::eXENON);
	const PxU32 tiledHeightFieldSamples = 0;
#endif
#ifdef PX_PS3
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::ePLAYSTATION3);
	const PxU32 tiledHeightFieldSamples = 1;
#endif
#ifdef PX_ARM
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::eARM);
	const PxU32 tiledHeightFieldSamples = 0;
#endif
#ifdef PX_WIIU
	const PxU32 gaussMapLimit = PxGetGaussMapVertexLimitForPlatform(PxPlatform::eWIIU);
	const PxU32 tiledHeightFieldSamples = 0;
#endif

	const PxU32 header = PX_MAKE_FOURCC('M','E','T','A');
	const PxU32 version = PX_PHYSICS_VERSION;
	const PxU32 ptrSize = sizeof(void*);
	PxU32 buildNumber = 0;
#if defined(PX_BUILD_NUMBER)
	buildNumber =  PX_BUILD_NUMBER;
#endif
	outputStream.write(&header, 4);
	outputStream.write(&version, 4);
	outputStream.write(&buildNumber, 4);
	outputStream.write(&ptrSize, 4);
	outputStream.write(&platformTag, 4);
	outputStream.write(&gaussMapLimit, 4);
	outputStream.write(&tiledHeightFieldSamples, 4);

	outputStream.write(&nb, 4);
	outputStream.write(entries, nb*sizeof(PxMetaDataEntry));

	PxU32 length = stringTable.size();
	const char* table = stringTable.begin();
	outputStream.write(&length, 4);
	outputStream.write(table, length);
}

PxBinaryConverter* PxSerialization::createBinaryConverter(PxSerializationRegistry& sr)
{
	return PX_NEW(ConvX)(sr);
}
