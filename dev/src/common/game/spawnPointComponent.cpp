/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "spawnPointComponent.h"

#include "../core/gatheredResource.h"

#include "../engine/bitmapTexture.h"
#include "../engine/renderFrame.h"

#include "gameWorld.h"
#include "spawnPointManager.h"



IMPLEMENT_ENGINE_CLASS( CSpawnPointComponent );

CGatheredResource resSpawnPointIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );

void CSpawnPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Register in the game world
#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
	if ( IsPositionValid() )
#endif
	{
		CGameWorld* ww = SafeCast< CGameWorld >( world );
		ww->AddSpawnPointComponent( this );
	}
}

void CSpawnPointComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	
	// Unregister
#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
	if ( IsPositionValid() )
#endif
	{
		CGameWorld* ww = SafeCast< CGameWorld >( world );
		ww->RemoveSpawnPointComponent( this );
	}
}

Uint32 CSpawnPointComponent::GetMinimumStreamingDistance() const
{
	return 100; // ??
}

void CSpawnPointComponent::WaypointGenerateEditorFragments( CRenderFrame* frame )
{
	// Generate base fragments
	TBaseClass::WaypointGenerateEditorFragments( frame );

	// Generate extra
	frame->AddDebugAngledRange( GetWorldPosition(), 0, m_radius, 360, 0, IsSelected() ? Color::YELLOW : Color::BLUE, true );
}

Color CSpawnPointComponent::CalcSpriteColor() const
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	// Show invalid spawn point
	if ( !IsPositionValid() && m_usedByPathLib )
	{
		return Color::RED;
	}
#endif

	if ( IsSelected() )
	{
		return Color::YELLOW;
	}
	else
	{
		return Color::BLUE;
	}
}

CBitmapTexture* CSpawnPointComponent::GetSpriteIcon() const
{
	return resSpawnPointIcon.LoadAndGet< CBitmapTexture >();
}

#ifndef NO_EDITOR
Bool CSpawnPointComponent::RemoveOnCookedBuild()
{
	return m_notUsedByCommunity;
}
#endif

Bool CSpawnPointComponent::GetSpawnPoint( Bool isAppear, Vector &spawnPoint /* out */ )
{
	const Int32 MAX_RANDOM_TRIALS = 3; // how many times we will try to use random spawn position

	//// First, try to use random spawn points
	//for ( int i = 0; i < MAX_RANDOM_TRIALS; ++i )
	//{
	//	Vector spawnPointCandidate = GetRandomPoint( GetWorldPosition(), m_radius );
	//	if ( IsSpawnPointAppropriate( spawnPointCandidate, isAppear ) )
	//	{
	//		spawnPoint = spawnPointCandidate;
	//		return true;
	//	}
	//}

	//// No random spawn point is appropriate, so try to use one of the predefined list
	//TDynArray< Vector > spawnPointCandidates;
	//GenerateSpawnPoints( GetWorldPosition(), m_radius, spawnPointCandidates );
	//for ( TDynArray< Vector >::iterator spawnPointCandidateI = spawnPointCandidates.Begin();
	//	  spawnPointCandidateI != spawnPointCandidates.End();
	//	  ++spawnPointCandidateI )
	//{
	//	if ( IsSpawnPointAppropriate( *spawnPointCandidateI, isAppear ) )
	//	{
	//		spawnPoint = *spawnPointCandidateI;
	//		return true;
	//	}
	//}
	Vector spawnPointCandidate;

	// First, try to use random spawn points
	for ( int i = 0; i < MAX_RANDOM_TRIALS; ++i )
	{
		if ( SpawnPointManager::GetRandomReachablePoint( GetWorldPosition(), m_radius, spawnPointCandidate ) 
			&& (SpawnPointManager::IsPointSeenByPlayer( spawnPointCandidate ) == isAppear) )
		{
			spawnPoint = spawnPointCandidate;
			return true;
		}
	}

	// No random spawn point is appropriate, so find any empty space
	if ( SpawnPointManager::GetFreeReachablePoint( GetWorldPosition(), m_radius, spawnPointCandidate )
		&& ( SpawnPointManager::IsPointSeenByPlayer( spawnPointCandidate ) == isAppear) )
	{
		spawnPoint = spawnPointCandidate;
		return true;
	}

	return false;
}

void CSpawnPointComponent::funcGetSpawnPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, isAppear, false );
	GET_PARAMETER_REF( Vector, spawnPosition, Vector(0,0,0) );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetSpawnPoint( isAppear, spawnPosition ) );
}
