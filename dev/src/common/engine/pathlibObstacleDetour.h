/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"

namespace PathLib
{

class CAreaDescription;
class CNavNodesGraphBase;

class CObstaclesDetourInfo
{
protected:
	TDynArray< Uint32 >			m_detourBases;
	TDynArray< Vector3 >		m_detourPoints;

public:
	// detour shapes are basically sizes of specified detours
	Uint32						DetousShapesCount() const														{ return m_detourBases.Size(); }
	const Vector3&				DetourShapeVert( Uint32 shape, Uint32 vert ) const								{ return m_detourPoints[ m_detourBases[ shape ] + vert ]; }
	Uint32						DetourShapeVertsCount( Uint32 shape ) const										{ return  ( (shape+1 < m_detourBases.Size())? m_detourBases[ shape+1 ] : m_detourPoints.Size()) - m_detourBases[ shape ]; }

	Uint32						VertsCount() const																{ return m_detourPoints.Size(); }
	const Vector3&				Vert( Uint32 index ) const														{ return m_detourPoints[ index ]; }
	Vector3&					Vert( Uint32 index )															{ return m_detourPoints[ index ]; }

	void						Push( const ObstacleDetour& detour )											{ Uint32 base = m_detourPoints.Size(); m_detourBases.PushBack( base ); m_detourPoints.Grow( detour.Size() ); Red::System::MemoryCopy( &m_detourPoints[ base ], &detour[ 0 ], detour.DataSize() ); }
};

namespace DetourComputationAlgorithm
{
	void GenerateDetour( CAreaDescription* areaDescription, CNavNodesGraphBase* sourceGraph, CNavNodesGraphBase* targetGraph, Uint32 category,  const CObstaclesDetourInfo& detourInfo );
};

};				// namespace PathLib


