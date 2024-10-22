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

#ifndef NX_ABSTRACT_SERIALIZER_H
#define NX_ABSTRACT_SERIALIZER_H

#include "NxParameterized.h"
#include "NxParameterizedTraits.h"
#include "NxSerializer.h"

#include "NxSerializerInternal.h"
#include "NxTraitsInternal.h"

#include "SerializerCommon.h"

namespace NxParameterized
{

// Base for other serializers which takes care of common stuff

class AbstractSerializer : public Serializer
{
public:
	AbstractSerializer(Traits *traits):
		mDoUpdate(true),
		mPlatform(GetCurrentPlatform()),
		mTraits(traits) {}

	virtual ~AbstractSerializer() {}

	Traits *getTraits() const { return mTraits; }

	//This is used in static Serializer::deserializer
	void setTraits(Traits *traits) { mTraits = traits; }

	void setAutoUpdate(bool doUpdate)
	{
		mDoUpdate = doUpdate;
	}

	Serializer::ErrorType peekInplaceAlignment(physx::PxFileBuf& /*stream*/, physx::PxU32& /*align*/)
	{
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}

	Serializer::ErrorType setTargetPlatform(const SerializePlatform &platform)
	{
		mPlatform = platform;
		return Serializer::ERROR_NONE; //Only pdb cares about platforms
	}

	Serializer::ErrorType serialize(physx::PxFileBuf &stream,const NxParameterized::Interface **objs, physx::PxU32 nobjs, bool doMetadata)
	{
#ifdef WITHOUT_APEX_SERIALIZATION
		PX_UNUSED(stream);
		PX_UNUSED(objs);
		PX_UNUSED(nobjs);
		PX_UNUSED(doMetadata);

		return Serializer::ERROR_NOT_IMPLEMENTED;
#else

		NX_BOOL_ERR_CHECK_WARN_RETURN(
			stream.isOpen(),
			Serializer::ERROR_STREAM_ERROR,
			"Stream not opened" );

		for(physx::PxU32 i = 0; i < nobjs; ++i)
		{
			NX_PARAM_ERR_CHECK_WARN_RETURN(
				objs[i]->callPreSerializeCallback(),
				Serializer::ERROR_PRESERIALIZE_FAILED,
				"Preserialize callback failed" );
		}

		return internalSerialize(stream, objs, nobjs, doMetadata);
#endif
	}

	Serializer::ErrorType deserialize(physx::PxFileBuf &stream, Serializer::DeserializedData &res, bool &isUpdated)
	{
		NX_BOOL_ERR_CHECK_WARN_RETURN(
			stream.isOpen(),
			Serializer::ERROR_STREAM_ERROR,
			"Stream not opened" );

		isUpdated = false;
		bool doesNeedUpdate = true;
		NX_ERR_CHECK_RETURN( internalDeserialize(stream, res, doesNeedUpdate) );
		return doesNeedUpdate && mDoUpdate ? upgrade(res, isUpdated) : Serializer::ERROR_NONE;
	}

	Serializer::ErrorType deserializeInplace(void *data, physx::PxU32 dataLen, Serializer::DeserializedData &res, bool &isUpdated)
	{
		isUpdated = false;
		bool doesNeedUpdate = true;
		NX_ERR_CHECK_RETURN( internalDeserializeInplace(data, dataLen, res, doesNeedUpdate) );
		return doesNeedUpdate && mDoUpdate ? upgrade(res, isUpdated) : Serializer::ERROR_NONE;
	}

protected:

	bool mDoUpdate;
	SerializePlatform mPlatform;
	Traits *mTraits;

#ifndef WITHOUT_APEX_SERIALIZATION
	virtual Serializer::ErrorType internalSerialize(
		physx::PxFileBuf &stream,
		const NxParameterized::Interface **objs,
		physx::PxU32 n,
		bool doMetadata) = 0;
#endif

	// doesNeedUpdate allows serializer to avoid costly depth-first scanning of included refs
	virtual Serializer::ErrorType internalDeserialize(
		physx::PxFileBuf &stream,
		Serializer::DeserializedData &res,
		bool &doesNeedUpdate) = 0;

	// See note for internalDeserialize
	virtual Serializer::ErrorType internalDeserializeInplace(
		void * /*data*/,
		physx::PxU32 /*dataLen*/,
		Serializer::DeserializedData & /*res*/,
		bool & /*doesNeedUpdate*/)
	{
		DEBUG_ALWAYS_ASSERT();
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}

private:
	Serializer::ErrorType upgrade(Serializer::DeserializedData &res, bool &isUpdated)
	{
		//Upgrade legacy objects
		NX_BOOL_ERR_CHECK_WARN_RETURN(
			UpgradeLegacyObjects(res, isUpdated, mTraits),
			Serializer::ERROR_CONVERSION_FAILED,
			"Upgrading legacy objects failed" );

		return Serializer::ERROR_NONE;
	}
};

} // namespace NxParameterized

#endif
