#pragma once

#include "build.h"

#include "../../common/engine/abstractPathRater.h"

class CEnemyAwareComponent;

class CSafestParhRater : public IPathRater
{
	static const Float	COVER_SEARCH_MARGIN;
	static const Int32	MAX_COVERS;
	static const Float	DISTANCE_MULTIPLER;	
	static const Float	MIN_COST_MULTIPLER;	

private:
	CEntity*				m_ownerEntity;
	CEnemyAwareComponent*	m_enemyAwareComponent;

public:
	RED_INLINE void SetOwnetEntity( CEntity* owner ){ m_ownerEntity = owner; }

	void PathFindingStarted() override;	
	PathLib::PathCost CountRealRate( const Vector3& from, const PathLib::CPathLink* pathLink ) override;
	PathLib::PathCost CountHeurusticRate( const Vector3& from, const Vector3& to ) override;

private:
	void FindCovers( const Vector& from, const Vector& to, TDynArray< THandle< CNode > >& coveringCovers, Int32 coversLimit, const Vector& nearestKnownEnemyPosition );
	Int32 CountCoveringCovers( const Vector& from, const Vector& to, Int32 coversLimit, const Vector& nearestKnownEnemyPosition );
};