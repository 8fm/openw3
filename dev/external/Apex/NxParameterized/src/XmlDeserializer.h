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

#ifndef XML_DESERIALIZER_H_
#define XML_DESERIALIZER_H_

//XML deserialization (by John Ratcliff)

#include "FastXml.h"

#include "NxSerializer.h"
#include "NxParameterized.h"
#include "NxParameterizedTraits.h"

#include "SerializerCommon.h"

namespace NxParameterized
{

typedef enum
{
	ARRAY,
	STRUCT,
	VALUE,
	SKIP
} FieldType;

struct FieldInfo
{
	PX_INLINE void init(const char *name_, FieldType type_)
	{
		name = name_;
		type = type_;
		idx = 0;
	}

	const char *name;
	FieldType type;
	physx::PxU32 idx; //For arrays
};

class ObjectInfo
{
	static const physx::PxU32 MAX_STRUCT_FIELD_STACK = 256;

	Interface *mObj;

	//Field stack
	physx::PxU32 mIndex;
	FieldInfo mFields[MAX_STRUCT_FIELD_STACK];

public:

	PX_INLINE ObjectInfo(): mObj(0), mIndex(0) {}

	PX_INLINE void init(Interface *obj)
	{
		mObj = obj;
	}

	PX_INLINE Interface *getObject()
	{
		return mObj;
	}

	PX_INLINE bool popField(const char *&name, FieldType &type)
	{
		if( !mIndex )
		{
			DEBUG_ALWAYS_ASSERT();
			return false;
		}

		FieldInfo &field = mFields[--mIndex];
		name = field.name;
		type = field.type;

#		ifndef NDEBUG
		field.name = 0;
#		endif

		if( mIndex )
		{
			FieldInfo &lastField = mFields[mIndex-1];
			if( ARRAY == lastField.type )
				++lastField.idx;
		}

		return true;
	}

	PX_INLINE void pushField(const char *name, FieldType type)
	{
		PX_ASSERT( mIndex < MAX_STRUCT_FIELD_STACK );

		if( mIndex < MAX_STRUCT_FIELD_STACK )
			mFields[mIndex++].init(name, type);
	}

	PX_INLINE physx::PxU32 getIndex() const { return mIndex; };

	PX_INLINE FieldInfo &getFieldInfo(physx::PxU32 i)
	{
		PX_ASSERT( i < mIndex );
		return mFields[i];
	}
};

class XmlDeserializer: public FAST_XML::FastXml::Callback
{
	static const physx::PxU32 MAX_REF_STACK = 8,
		MAX_ROOT_OBJ = 64;

	Serializer::ErrorType mError;

	Traits *mTraits;

	//Object stack
	physx::PxU32 mObjIndex;
	ObjectInfo mObjects[MAX_REF_STACK];

	//Array of root objects
	physx::PxU32 mRootIndex;
	Interface *mRootObjs[MAX_ROOT_OBJ];

	//Check errors in <NxParameters>
	physx::PxU32 mRootTags;
	bool mInRootElement;

	//Check DOCTYPE
	bool mHasDoctype;

	physx::PxU32 mVer;

	// read simple structs in array
	physx::PxI32* mSimpleStructRedirect;
	physx::PxU32 mSimpleStructRedirectSize;

	//Top Of Stack
	PX_INLINE ObjectInfo &tos()
	{
		PX_ASSERT( mObjIndex >= 1 && mObjIndex <= MAX_REF_STACK );
		return mObjIndex > 0 ? mObjects[mObjIndex - 1] : mObjects[0];
	}

	PX_INLINE void pushObj(Interface *obj)
	{
		if( mObjIndex >= MAX_REF_STACK )
		{
			PX_ALWAYS_ASSERT(); //included references nested too deeply
			return;
		}

		++mObjIndex;
		tos().init(obj);
	}

	PX_INLINE bool popObj()
	{
		if( mObjIndex <= 0 )
			return false;

		--mObjIndex;

		return true;
	}

	PX_INLINE void pushField(const char *name, FieldType type)
	{
		tos().pushField(mTraits->strdup(name), type);
	}

	PX_INLINE bool popField()
	{
		const char *name = 0;
		FieldType type;
		if( !tos().popField(name, type) )
			return false;

		mTraits->strfree(const_cast<char *>(name));

		return true;
	}

	bool verifyObject(Interface *obj, physx::PxU32 argc, const char **argv);

	bool initAddressString(char *dest, physx::PxU32 len, const char *name);

public:

	PX_INLINE XmlDeserializer(Traits *traits, physx::PxU32 ver):
		mError(Serializer::ERROR_NONE),
		mTraits(traits),
		mObjIndex(0),
		mRootIndex(0),
		mRootTags(0),
		mInRootElement(false),
		mHasDoctype(false),
		mVer(ver),
		mSimpleStructRedirect(NULL),
		mSimpleStructRedirectSize(0) {}

	PX_INLINE virtual ~XmlDeserializer()
	{
		if (mSimpleStructRedirect != NULL)
		{
			mTraits->free(mSimpleStructRedirect);
		}
		mSimpleStructRedirect = NULL;
		mSimpleStructRedirectSize = 0;
	}

	static PX_INLINE XmlDeserializer *Create(Traits *traits, physx::PxU32 ver)
	{
		char *buf = (char *)serializerMemAlloc(sizeof(XmlDeserializer), traits);
		return PX_PLACEMENT_NEW(buf, XmlDeserializer)(traits, ver);
	}

	PX_INLINE void destroy()
	{
		Traits *traits = mTraits;
		this->~XmlDeserializer();
		serializerMemFree(this, traits);
	}

	PX_INLINE Serializer::ErrorType getLastError() const
	{
		return mError;
	}

	PX_INLINE Interface **getObjs()
	{
		return mRootObjs;
	}

	PX_INLINE physx::PxU32 getNobjs() const
	{
		return physx::PxMin(mRootIndex, MAX_ROOT_OBJ);
	}

	//Release all created objects (in case of error)
	PX_INLINE void releaseAll()
	{
		for(physx::PxU32 i = 0; i < getNobjs(); ++i)
			mRootObjs[i]->destroy();
	}

	virtual bool processComment(const char *)
	{
		return true;
	}

	virtual bool processDoctype(const char * rootElement, const char *, const char *, const char *)
	{
		mHasDoctype = true;
		return 0 == ::strcmp(rootElement, "NxParameters");
	}

	virtual void *fastxml_malloc(physx::PxU32 size)
	{
		return mTraits->alloc(size);
	}

	virtual void fastxml_free(void *mem) 
	{
		mTraits->free(mem);
	}

	virtual bool processClose(const char *tag,physx::PxU32 depth,bool &isError);

	virtual bool processElement(
		const char *elementName,
		physx::PxI32 argc,
		const char **argv,
		const char *elementData,
		physx::PxI32 /*lineno*/
	);

	physx::PxI32* getSimpleStructRedirect(physx::PxU32 size);
};

}

#endif
