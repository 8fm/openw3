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

#ifndef NX_BIN_SERIALIZER_H
#define NX_BIN_SERIALIZER_H

// APB serializer

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "NxParameterized.h"
#include "NxSerializer.h"

#include "AbstractSerializer.h"

#include "ApbDefinitions.h"

namespace NxParameterized
{

class DefinitionImpl;
class PlatformInputStream;
class PlatformOutputStream;
struct PlatformABI;

class BinSerializer : public AbstractSerializer
{
	// Methods for updating legacy formats
	Serializer::ErrorType updateInitial2AllCounted(BinaryHeader &hdr, char *start);
	Serializer::ErrorType updateAllCounted2WithAlignment(BinaryHeader &hdr, char *start);

	Serializer::ErrorType readMetadataInfo(const BinaryHeader &hdr, PlatformInputStream &s, DefinitionImpl *def);

	// Read array of arbitrary type (slow version)
	Serializer::ErrorType readArraySlow(Handle &handle, PlatformInputStream &s);

	// Read NxParameterized object data
	Serializer::ErrorType readObject(NxParameterized::Interface *&obj, PlatformInputStream &data);

	// Read binary data of NxParameterized object addressed by handle
	Serializer::ErrorType readBinaryData(Handle &handle, PlatformInputStream &data);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType storeMetadataInfo(const Definition *def, PlatformOutputStream &s);

	// Store array of arbitrary type (slow version)
	Serializer::ErrorType storeArraySlow(Handle &handle, PlatformOutputStream &s);

	// Print binary data for part of NxParameterized object addressed by handle
	Serializer::ErrorType storeBinaryData(const NxParameterized::Interface &obj, Handle &handle, PlatformOutputStream &res, bool isRootObject = true);
#endif

	BinSerializer(BinSerializer &); // Don't
	void operator=(BinSerializer &); // Don't

	Serializer::ErrorType verifyFileHeader(
		const BinaryHeader &hdr,
		const BinaryHeaderExt *ext,
		physx::PxU32 dataLen ) const;

	Serializer::ErrorType getPlatformInfo(
		BinaryHeader &hdr,
		BinaryHeaderExt *ext,
		PlatformABI &abi ) const;

	Serializer::ErrorType verifyObjectHeader(const ObjHeader &hdr, const Interface *obj, Traits *traits) const;

protected:

	Serializer::ErrorType internalDeserialize(physx::PxFileBuf &stream, Serializer::DeserializedData &res, bool &doesNeedUpdate);
	Serializer::ErrorType internalDeserializeInplace(void *mdata, physx::PxU32 dataLen, Serializer::DeserializedData &res, bool &doesNeedUpdate);

#ifndef WITHOUT_APEX_SERIALIZATION
	Serializer::ErrorType internalSerialize(physx::PxFileBuf &stream,const NxParameterized::Interface **objs, physx::PxU32 nobjs, bool doMetadata);
#endif

public:
	BinSerializer(Traits *traits): AbstractSerializer(traits) {}

	void release()
	{
		Traits *t = mTraits;
		this->~BinSerializer();
		serializerMemFree(this, t);
	}

	static const physx::PxU32 Magic = APB_MAGIC;
	static const physx::PxU32 Version = BinVersions::WithExtendedHeader;

	Serializer::ErrorType peekNumObjects(physx::PxFileBuf &stream, physx::PxU32 &numObjects);

	Serializer::ErrorType peekNumObjectsInplace(const void *data, physx::PxU32 dataLen, physx::PxU32 &numObjects);

	Serializer::ErrorType peekClassNames(physx::PxFileBuf &stream, char **classNames, physx::PxU32 &numClassNames);

	Serializer::ErrorType peekInplaceAlignment(physx::PxFileBuf& stream, physx::PxU32& align);

	Serializer::ErrorType deserializeMetadata(physx::PxFileBuf &stream, DeserializedMetadata &desData);
};

bool isBinaryFormat(physx::PxFileBuf &stream);

Serializer::ErrorType peekBinaryPlatform(physx::PxFileBuf &stream, SerializePlatform &platform);

} // namespace NxParameterized

#endif
