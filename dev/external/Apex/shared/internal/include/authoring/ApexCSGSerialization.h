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
#ifndef APEX_CSG_SERIALIZATION_H
#define APEX_CSG_SERIALIZATION_H

#include "PsShare.h"
#include "ApexSharedUtils.h"
#include "ApexStream.h"
#include "authoring/ApexCSGDefs.h"
#include "authoring/ApexCSGHull.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace physx
{
namespace apex
{

/* Version for serialization */
struct Version
{
	enum Enum
	{
		Initial = 0,
		RevisedMeshTolerances,
		UsingOnlyPositionDataInVertex,
		SerializingTriangleFrames,
		UsingGSA,
		SerializingMeshBounds,
		AddedInternalTransform,

		Count,
		Current = Count - 1
	};
};


// Vec<T,D>
template<typename T, int D>
PX_INLINE physx::PxFileBuf&
operator << (physx::PxFileBuf& stream, const ApexCSG::Vec<T, D>& v)
{
	for (physx::PxU32 i = 0; i < D; ++i)
	{
		stream << v[i];
	}
	return stream;
}

template<typename T, int D>
PX_INLINE physx::PxFileBuf&
operator >> (physx::PxFileBuf& stream, ApexCSG::Vec<T, D>& v)
{
	for (physx::PxU32 i = 0; i < D; ++i)
	{
		stream >> v[i];
	}

	return stream;
}


// Edge
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Hull::Edge& e)
{
	stream << e.m_indexV0 << e.m_indexV1 << e.m_indexF1 << e.m_indexF2;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ApexCSG::Hull::Edge& e)
{
	PX_UNUSED(version);	// Initial

	stream >> e.m_indexV0 >> e.m_indexV1 >> e.m_indexF1 >> e.m_indexF2;
}


// Region
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Region& r)
{
	stream << r.side;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ApexCSG::Region& r)
{
	if (version < Version::UsingGSA)
	{
		ApexCSG::Hull hull;
		hull.deserialize(stream, version);
	}

	stream >> r.side;
}


// Surface
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Surface& s)
{
	stream << s.planeIndex;
	stream << s.triangleIndexStart;
	stream << s.triangleIndexStop;
	stream << s.totalTriangleArea;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ApexCSG::Surface& s)
{
	PX_UNUSED(version);	// Initial

	stream >> s.planeIndex;
	stream >> s.triangleIndexStart;
	stream >> s.triangleIndexStop;
	stream >> s.totalTriangleArea;
}


// Triangle
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Triangle& t)
{
	for (physx::PxU32 i = 0; i < 3; ++i)
	{
		stream << t.vertices[i];
	}
	stream << t.submeshIndex;
	stream << t.smoothingMask;
	stream << t.extraDataIndex;
	stream << t.normal;
	stream << t.area;
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ApexCSG::Triangle& t)
{
	for (physx::PxU32 i = 0; i < 3; ++i)
	{
		stream >> t.vertices[i];
		if (version < Version::UsingOnlyPositionDataInVertex)
		{
			ApexCSG::Dir v;
			stream >> v;	// normal
			stream >> v;	// tangent
			stream >> v;	// binormal
			ApexCSG::UV uv;
			for (physx::PxU32 uvN = 0; uvN < NxVertexFormat::MAX_UV_COUNT; ++uvN)
			{
				stream >> uv;	// UVs
			}
			ApexCSG::Color c;
			stream >> c;	// color
		}
	}
	stream >> t.submeshIndex;
	stream >> t.smoothingMask;
	stream >> t.extraDataIndex;
	stream >> t.normal;
	stream >> t.area;
}

// Interpolator
PX_INLINE void
serialize(physx::PxFileBuf& stream, const ApexCSG::Interpolator& t)
{
	t.serialize(stream);
}

PX_INLINE void
deserialize(physx::PxFileBuf& stream, physx::PxU32 version, ApexCSG::Interpolator& t)
{
	t.deserialize(stream, version);
}

}
};	// namespace physx::apex

#endif

#endif // #define APEX_CSG_SERIALIZATION_H
