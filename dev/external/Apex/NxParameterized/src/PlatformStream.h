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

#ifndef PLATFORM_STREAM_H_
#define PLATFORM_STREAM_H_

#include <stdio.h>

#include "NxParameterized.h"
#include "NxSerializer.h"
#include "NxSerializerInternal.h"
#include "PlatformABI.h"

namespace NxParameterized
{

// Base class for ABI-aware streams

// TODO:
// PlatformInput/OutputStream should probably be visitors

class PlatformStream
{
	void operator =(const PlatformStream &); //Don't

protected:
	Traits *mTraits;

	PlatformABI mTargetParams, mCurParams;

	struct Agregate
	{
		enum Type
		{
			STRUCT,
			ARRAY
		};

		Type type;
		physx::PxU32 align;

		Agregate(Type t, physx::PxU32 a): type(t), align(a) {}
	};

	//Agregate stack holds information about structs and their nesting (for automatic insertion of tail pad)
	physx::Array<Agregate, Traits::Allocator> mStack;

public:
	PlatformStream(const PlatformABI &targetParams, Traits *traits)
		: mTraits(traits),
		mTargetParams(targetParams),
		mStack(Traits::Allocator(traits))
	{
		// TODO: constructors should not fail...
		if( Serializer::ERROR_NONE != PlatformABI::GetPredefinedABI(GetCurrentPlatform(), mCurParams) )
		{
			PX_ALWAYS_ASSERT();
			return;
		}
	}

	//Array's copy constructor is broken so we implement it by hand
	PlatformStream(const PlatformStream &s)
		: mTraits(s.mTraits),
		mTargetParams(s.mTargetParams),
		mCurParams(s.mCurParams),
		mStack(Traits::Allocator(s.mTraits))
	{
		mStack.reserve(s.mStack.size());
		for(physx::PxU32 i = 0; i < s.mStack.size(); ++i)
			mStack.pushBack(s.mStack[i]);
	}

	void dump() const
	{
		printf("Dumping PlatformStream at %p:\n", this);
	}

	const PlatformABI &getTargetABI() const
	{
		return mTargetParams;
	}

	physx::PxU32 getTargetSize(const Definition *pd) const
	{
		return mTargetParams.getSize(pd);
	}

	physx::PxU32 getTargetAlignment(const Definition *pd) const
	{
		return mTargetParams.getAlignment(pd);
	}

	physx::PxU32 getTargetPadding(const Definition *pd) const
	{
		return mTargetParams.getPadding(pd);
	}

	//Align offset to be n*border
	PX_INLINE physx::PxU32 getAlign(physx::PxU32 off, physx::PxU32 border, bool &isAligned) const
	{
		PX_ASSERT(border <= 128);

		//Array members are not aligned
		//See http://www.gamasutra.com/view/feature/3975/data_alignment_part_2_objects_on_.php
		if( !mStack.empty() && Agregate::ARRAY == mStack.back().type )
		{
			isAligned = false;
			return off;
		}

		isAligned = true;
		const physx::PxU32 mask = border - 1;
		return (off + mask) & ~mask;
	}

	//Align offset to be n*border
	PX_INLINE physx::PxU32 getAlign(physx::PxU32 off, physx::PxU32 border) const
	{
		bool tmp;
		return getAlign(off, border, tmp);
	}
};

}

#endif