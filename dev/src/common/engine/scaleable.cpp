/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scaleable.h"
#include "game.h"
#include "world.h"
#include "materialInstance.h"

CLODableManager::CLODableManager()
	: m_tickManager( nullptr )
	, m_forceUpdateAll( false )
	, m_position( Vector::ZEROS )
	, m_positionValid( false )
{
	SetMaxUpdateTime( 0.0001f );
	SetDistances( 25.0f, 100.0f );
}

void CLODableManager::SetReferencePosition( const Vector& position )
{
	m_position = position;
	m_positionValid = true;
}

void CLODableManager::SetMaxUpdateTime( Float maxUpdateTime )
{
	m_lodables.SetMaxBudgetedProcessingTime( maxUpdateTime );
}

void CLODableManager::SetDistances( Float budgetableDistance, Float disableDistance )
{
	m_budgetableDistance = budgetableDistance;
	m_budgetableDistanceSqr = Red::Math::MSqr( budgetableDistance );

	m_disableDistance = disableDistance;
	m_disableDistanceSqr = Red::Math::MSqr( disableDistance );
}

void CLODableManager::UpdateLODs()
{
	if ( ! m_positionValid )
	{
		return;
	}

	struct LODSwitcher
	{
		CLODableManager* m_manager;

		LODSwitcher( CLODableManager* manager )
			: m_manager( manager )
		{}

		RED_FORCE_INLINE void Process( ILODable* lodable ) const
		{
			const ILODable::LOD newLOD = lodable->ComputeLOD( m_manager );
			if ( newLOD != lodable->GetCurrentLOD() )
			{
				lodable->UpdateLOD( newLOD, m_manager );
			}
		}
	} lodSwitcher( this );

	// Process LOD changes within allowed time frame

	if ( m_forceUpdateAll )
	{
		m_lodables.ProcessAll( lodSwitcher );
		m_forceUpdateAll = false;
	}
	else
	{
		m_lodables.Process( lodSwitcher );
	}
}