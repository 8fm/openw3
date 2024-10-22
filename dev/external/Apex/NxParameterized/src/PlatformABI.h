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

#ifndef PLATFORM_ABI_H_
#define PLATFORM_ABI_H_

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "foundation/PxMath.h"

#include <PxMat33Legacy.h>
#include <PxMat34Legacy.h>

#include "BinaryHelper.h"
#include "NxSerializer.h"

namespace NxParameterized
{

//Describes platform ABI (endian, alignment, etc.)
struct PlatformABI
{
	enum Endian
	{
		LITTLE,
		BIG
	};
	
	Endian endian;

	//Sizes of basic types
	struct
	{
		physx::PxU32 Char,
			Bool,
			pointer,
			real;
	} sizes;

	//Alignments of basic types
	struct
	{
		physx::PxU32 Char,
			Bool,
			pointer,
			real,
			i8,
			i16,
			i32,
			i64,
			f32,
			f64;
	} aligns;

	//Does child class reuse tail padding of parent? (google for "overlaying tail padding")
	bool doReuseParentPadding;

	//Are empty base classes eliminated? (google for "empty base class optimization")
	//We may need this in future
	bool doEbo;

	//Get ABI of platform
	static Serializer::ErrorType GetPredefinedABI(const SerializePlatform &platform, PlatformABI &params);

	//Get alignment of (complex) NxParameterized data type
	physx::PxU32 getAlignment(const Definition *pd) const;

	//Get padding of (complex) NxParameterized data type
	physx::PxU32 getPadding(const Definition *pd) const;

	//Get size of (complex) NxParameterized data type
	physx::PxU32 getSize(const Definition *pd) const;

	//Helper function which calculates aligned value
	PX_INLINE static physx::PxU32 align(physx::PxU32 len, physx::PxU32 border);

	//Verifying that platforms are going to work with our serializer
	PX_INLINE bool isNormal() const;
	static bool VerifyCurrentPlatform();

	//Alignment of metadata table entry in metadata section
	//TODO: find better place for this
	PX_INLINE physx::PxU32 getMetaEntryAlignment() const;

	//Alignment of metadata info in metadata section
	//TODO: find better place for this
	PX_INLINE physx::PxU32 getMetaInfoAlignment() const;

	//Alignment of hint in metadata section
	//TODO: find better place for this
	PX_INLINE physx::PxU32 getHintAlignment() const;

	//Alignment of hint value (union of PxU32, PxU64, const char *) in metadata section
	//TODO: find better place for this
	PX_INLINE physx::PxU32 getHintValueAlignment() const;

	//Size of hint value (union of PxU32, PxU64, const char *) in metadata section
	//TODO: find better place for this
	PX_INLINE physx::PxU32 getHintValueSize() const;

	//Template for getting target alignment of T
	template <typename T> PX_INLINE physx::PxU32 getAlignment() const;

	//Template for getting target size of T
	template <typename T> PX_INLINE physx::PxU32 getSize() const;

private:
	physx::PxU32 getNatAlignment(const Definition *pd) const;
};

#include "PlatformABI.inl"

}

#endif
