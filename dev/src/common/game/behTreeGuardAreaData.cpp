/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeGuardAreaData.h"
#include "../engine/areaComponent.h"

IMPLEMENT_ENGINE_CLASS( CBehTreeGuardAreaData )

////////////////////////////////////////////////////////////////////////////
// CBehTreeGuardAreaData
////////////////////////////////////////////////////////////////////////////
Bool CBehTreeGuardAreaData::IsInGuardArea( const Vector& pos, CAreaComponent* guardArea )
{
	if ( !guardArea->GetBoundingBox().Contains( pos ) )
	{
		return false;
	}
	return guardArea->TestPointOverlap( pos );
}

Bool CBehTreeGuardAreaData::IsInPursueArea( const Vector& actorPos, CAreaComponent* guardArea )
{
	CAreaComponent* pursuitArea = GetPursuitArea();
	if ( pursuitArea )
	{
		return pursuitArea->TestPointOverlap( actorPos );
	}

	// calculate pursuit area 
	Vector closestPoint;
	Float closestDistance;

	if( guardArea->FindClosestPoint( actorPos, m_pursuitRange, closestPoint, closestDistance ) )
	{
		return closestDistance <= m_pursuitRange;
	}
	
	return false;
}

void CBehTreeGuardAreaData::SetupBaseState( CAreaComponent* guardArea, CAreaComponent* pursuitArea, Float pursuitRange )
{
	ASSERT( pursuitRange >= 0.f );
	m_baseGuardArea = guardArea;
	m_basePursuitArea = pursuitArea;
	m_basePursuitRange = pursuitRange;

	if ( !m_isInImmediateState )
	{
		m_guardArea = guardArea;
		m_pursuitArea = pursuitArea;
		m_pursuitRange = pursuitRange;
	}
}
void CBehTreeGuardAreaData::SetupImmediateState( CAreaComponent* guardArea, CAreaComponent* pursuitArea, Float pursuitRange )
{
	m_isInImmediateState = true;

	m_guardArea = guardArea;
	m_pursuitArea = pursuitArea;
	m_pursuitRange = pursuitRange;
}
void CBehTreeGuardAreaData::ResetImmediateState()
{
	m_guardArea = m_baseGuardArea;
	m_pursuitArea = m_basePursuitArea;
	m_pursuitRange = m_basePursuitRange;
	m_isInImmediateState = true;
}

CBehTreeGuardAreaData* CBehTreeGuardAreaData::Find( CAIStorage* storage )
{
	CAIStorageItem* item = storage->GetItem( CNAME( GUARD_AREA_DATA ) );
	if ( !item )
	{
		return nullptr;
	}

	return item->GetPtr< CBehTreeGuardAreaData >();
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeGuardAreaData::CInitializer
////////////////////////////////////////////////////////////////////////////
CName CBehTreeGuardAreaData::CInitializer::GetItemName() const
{
	return CNAME( GUARD_AREA_DATA );
}
void CBehTreeGuardAreaData::CInitializer::InitializeItem( CAIStorageItem& item ) const
{

}
IRTTIType* CBehTreeGuardAreaData::CInitializer::GetItemType() const
{
	return CBehTreeGuardAreaData::GetStaticClass();
}

