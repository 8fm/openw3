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
#ifndef APEX_CSG_HULL_H
#define APEX_CSG_HULL_H

#include "PsShare.h"
#include "authoring/ApexCSGMath.h"
#include "PsArray.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace ApexCSG
{

/* Convex hull that handles unbounded sets. */

class Hull
{
public:
	struct Edge
	{
		physx::PxU32	m_indexV0;
		physx::PxU32	m_indexV1;
		physx::PxU32	m_indexF1;
		physx::PxU32	m_indexF2;
	};

	struct EdgeType
	{
		enum Enum
		{
			LineSegment,
			Ray,
			Line
		};
	};

	PX_INLINE					Hull()
	{
		setToAllSpace();
	}
	PX_INLINE					Hull(const Hull& geom)
	{
		*this = geom;
	}

	PX_INLINE	void			setToAllSpace()
	{
		clear();
		allSpace = true;
	}
	PX_INLINE	void			setToEmptySet()
	{
		clear();
		emptySet = true;
	}

	void			intersect(const Plane& plane, Real distanceTol);

	PX_INLINE	void			transform(const Mat4Real& tm, const Mat4Real& cofTM);

	PX_INLINE	physx::PxU32	getFaceCount() const
	{
		return faces.size();
	}
	PX_INLINE	const Plane&	getFace(physx::PxU32 faceIndex) const
	{
		return faces[faceIndex];
	}

	PX_INLINE	physx::PxU32	getEdgeCount() const
	{
		return edges.size();
	}
	PX_INLINE	const Edge&		getEdge(physx::PxU32 edgeIndex) const
	{
		return edges[edgeIndex];
	}

	PX_INLINE	physx::PxU32	getVertexCount() const
	{
		return vertexCount;
	}
	PX_INLINE	const Pos&		getVertex(physx::PxU32 vertexIndex) const
	{
		return *(const Pos*)(vectors.begin() + vertexIndex);
	}

	PX_INLINE	bool			isEmptySet() const
	{
		return emptySet;
	}
	PX_INLINE	bool			isAllSpace() const
	{
		return allSpace;
	}

	Real			calculateVolume() const;

	// Edge accessors
	PX_INLINE	EdgeType::Enum	getType(const Edge& edge) const
	{
		return (EdgeType::Enum)((physx::PxU32)(edge.m_indexV0 >= vertexCount) + (physx::PxU32)(edge.m_indexV1 >= vertexCount));
	}
	PX_INLINE	const Pos&		getV0(const Edge& edge)	const
	{
		return *(Pos*)(vectors.begin() + edge.m_indexV0);
	}
	PX_INLINE	const Pos&		getV1(const Edge& edge)	const
	{
		return *(Pos*)(vectors.begin() + edge.m_indexV1);
	}
	PX_INLINE	const Dir&		getDir(const Edge& edge)	const
	{
		PX_ASSERT(edge.m_indexV1 >= vertexCount);
		return *(Dir*)(vectors.begin() + edge.m_indexV1);
	}
	PX_INLINE	physx::PxU32	getF1(const Edge& edge)	const
	{
		return edge.m_indexF1;
	}
	PX_INLINE	physx::PxU32	getF2(const Edge& edge)	const
	{
		return edge.m_indexF2;
	}

	// Serialization
	void			serialize(physx::PxFileBuf& stream) const;
	void			deserialize(physx::PxFileBuf& stream, physx::PxU32 version);

protected:
	PX_INLINE	void			clear();

	bool			testConsistency(Real distanceTol, Real angleTol) const;

	// Faces
	physx::Array<Plane>		faces;
	physx::Array<Edge>		edges;
	physx::Array<Vec4Real>	vectors;
	physx::PxU32						vertexCount;	// vectors[i], i >= vertexCount, are used to store vectors for ray and line edges
	bool						allSpace;
	bool						emptySet;
};

PX_INLINE void
Hull::transform(const Mat4Real& tm, const Mat4Real& cofTM)
{
	for (physx::PxU32 i = 0; i < faces.size(); ++i)
	{
		Plane& face = faces[i];
		face = cofTM * face;
		face.normalize();
	}

	for (physx::PxU32 i = 0; i < vectors.size(); ++i)
	{
		Vec4Real& vector = vectors[i];
		vector = tm * vector;
	}
}

PX_INLINE void
Hull::clear()
{
	vectors.reset();
	edges.reset();
	faces.reset();
	vertexCount = 0;
	allSpace = false;
	emptySet = false;
}


};	// namespace ApexCSG

#endif

#endif // #define APEX_CSG_HULL_H
