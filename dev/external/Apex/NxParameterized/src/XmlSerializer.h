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

#ifndef NX_XML_SERIALIZER_H
#define NX_XML_SERIALIZER_H

//XML serialization (by John Ratcliff)

#include "NxSerializer.h"
#include "AbstractSerializer.h"

#include "PxIOStream.h"

namespace NxParameterized
{

struct traversalState;

bool isXmlFormat(physx::PxFileBuf &stream);

// XML serializer implementation
class XmlSerializer : public AbstractSerializer
{
	ErrorType peekNumObjects(char *data, physx::PxU32 len, physx::PxU32 &numObjects);

	Serializer::ErrorType peekClassNames(physx::PxFileBuf &stream, char **classNames, physx::PxU32 &numClassNames);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType traverseParamDefTree(
		const Interface &obj,
		physx::PxIOStream &stream,
		traversalState &state,
		Handle &handle,
		bool printValues = true);

	Serializer::ErrorType emitElementNxHints(
		physx::PxIOStream &stream,
		Handle &handle,
		traversalState &state,
		bool &includedRef);

	Serializer::ErrorType emitElement(
		const Interface &obj,
		physx::PxIOStream &stream,
		const char *elementName,
		Handle &handle,
		bool includedRef,
		bool printValues,
		bool isRoot = false);
#endif

protected:

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType internalSerialize(physx::PxFileBuf &fbuf,const Interface **objs, physx::PxU32 n, bool doMetadata);
#endif

	Serializer::ErrorType internalDeserialize(physx::PxFileBuf &stream, DeserializedData &res, bool &doesNeedUpdate);

public:

	XmlSerializer(Traits *traits): AbstractSerializer(traits) {}

	~XmlSerializer() {}

	virtual void release(void)
	{
		this->~XmlSerializer();
		serializerMemFree(this,mTraits);
	}

	PX_INLINE static physx::PxU32 version()
	{
		return 0x00010000;
	}

	ErrorType peekNumObjectsInplace(const void * data, physx::PxU32 dataLen, physx::PxU32 & numObjects);

	ErrorType peekNumObjects(physx::PxFileBuf &stream, physx::PxU32 &numObjects);

	ErrorType deserializeMetadata(physx::PxFileBuf & /*stream*/, Definition ** /*defs*/, physx::PxU32 & /*ndefs*/)
	{
		return Serializer::ERROR_NOT_IMPLEMENTED;
	}
};

} // namespace NxParameterized

#endif
