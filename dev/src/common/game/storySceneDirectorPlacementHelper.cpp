/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneDirectorPlacementHelper.h"
#include "sceneLog.h"
#include "storyScenePlayer.h"
#include "../engine/tagManager.h"
#include "../engine/pathlibWorld.h"
#include "storySceneCutsceneSection.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

const float safePlacementSearchRadiusStep = 0.5f;

RED_DEFINE_STATIC_NAME( SAFE_SCENE_POSITION );

CStorySceneDirectorPlacementHelper::CStorySceneDirectorPlacementHelper()
	: m_pivotActors( nullptr )
	, m_initialPlacementFallbackNode( nullptr )
	, m_directorParent( nullptr )
{

}

void CStorySceneDirectorPlacementHelper::Init( const CStoryScenePlayer* directorParent )
{
	m_directorParent = directorParent;
}

Bool CStorySceneDirectorPlacementHelper::FindUnsafePlacementFallback( const TDynArray< Vector > & convexHull, const Vector & initPosition, Vector & retPosition, const CWorld* gameWorld, Float maxRadius, Float radiusStep , Float slotExtent )
{
	LOG_GAME( TXT("FindUnsafePlacementFallback started at hint position %s"), ToString( initPosition ).AsChar() );	

	const CPathLibWorld* pathLib = gameWorld->GetPathLibWorld();

	Box convexHullBBox( Box::RESET_STATE );
	ForEach( convexHull, [&convexHullBBox] ( const Vector& v )
		{
			convexHullBBox.AddPoint( v );
		}
	);

	//test point initial position
	if( pathLib->TestLocation( initPosition, PathLib::CT_DEFAULT ) )
	{
		if(	pathLib->ConvexHullCollisionTest( convexHull, convexHullBBox, initPosition ) )
		{
			LOG_GAME( TXT("FindUnsafePlacementFallback:: - hint position is valid") );
			retPosition = initPosition;
			return true;
		}
	}

	// Generate a set of points to be tested ( a set of circles with increasing radiuses )
	// Test collision at test points until no collision
	radiusStep = Min( radiusStep, 0.5f );
	Float externalRadius = Max( maxRadius, 1.0f );
	Float currentRadius = Max( radiusStep, 1.0f );
	Vector testedPosition;

	while( currentRadius <= externalRadius  )
	{  
		//Uint32 circleDensity = 6 * Uint32( max( currentRadius, 1.0f ) );
		Uint32 circleDensity = Uint32( 6 * ( 1.0f + ( currentRadius - 1.0f ) / 4.0f ) );
		for ( Uint32 j=0; j<circleDensity; ++j )
		{
			float angle = 2.0f * M_PI * float( j ) / float( circleDensity );
			testedPosition = Vector(initPosition.X + MCos( angle )*currentRadius,  initPosition.Y - MSin( angle )*currentRadius, initPosition.Z + 0.5f);

			if( pathLib->TestLocation( testedPosition, PathLib::CT_DEFAULT ) )
			{
				if(	pathLib->ConvexHullCollisionTest( convexHull, convexHullBBox, testedPosition ) )
				{
					LOG_GAME( TXT("FindUnsafePlacementFallback:: - hint position is valid") );
					retPosition = testedPosition;
					return true;
				}
			}
		}
		currentRadius += radiusStep;
	}

	LOG_GAME( TXT("TryFindValidPositionInRange - no valid positions found!!") );
	return false;
}

void CStorySceneDirectorPlacementHelper::CleanupPlacements()
{
	m_safePlacements.Clear();
}

Float CStorySceneDirectorPlacementHelper::SnapPlacement( EngineTransform& placement, const CWorld* world )
{
	Vector position = placement.GetPosition();
	EulerAngles rotation = placement.GetRotation();
	Float originalZ = position.Z;

	// snap the height to the mesh we're walking on
	if ( world != NULL )
	{
		TraceResultPlacement traceResult;
		if ( CTraceTool::StaticAgentPlacementTraceTest( GGame->GetActiveWorld(), position, MAC_TRACE_TEST_RADIUS, traceResult ) )
		{
			ASSERT( traceResult.m_isValid );
			ASSERT( traceResult.m_height != INVALID_Z );
			
			// Safety: dont let snapping throw us into some big hole
			if ( Abs( position.Z - traceResult.m_height ) < 5.0f )
			{
				position.Z = traceResult.m_height; 

				if ( traceResult.m_isTerrainCollision == false )
				{
					// Big hack: when we collide with some physics object we will snap 5cm higher
					// this will compensate for collision convexes made smaller after the dialog has been set
					position.Z += 0.05f;
				}
			}
		}
	}
	

	rotation.Roll = 0.0f;
	rotation.Pitch = 0.0f;

	placement.SetPosition( position );
	placement.SetRotation( rotation );

	return position.Z - originalZ;
}

void CStorySceneDirectorPlacementHelper::FindDialogsetPlacement( const CStorySceneDialogsetInstance* dialogset, EngineTransform& placement ) const
{
	placement.Identity();

	SCENE_ASSERT( m_directorParent );

	if ( dialogset && m_directorParent->GetReferencePlacement( dialogset, placement ) )
	{
		return;
	}

	//This part is meant to not teleport you to 0,0,0 in the unlikely case when there is no dialogset or the scene waypoint is missing
	//but since the camera lights are still relative to scene root even in cutscenes we need to have consistent behaviour in game and in editor for those cases
	//this is a minimal condition that should satisfy both cases 
	const CStorySceneSection* section = m_directorParent->GetCurrentSection();
	if ( m_directorParent->CanUseDefaultScenePlacement() && dialogset && Cast< const CStorySceneCutsceneSection >( section ) )
	{
		placement = m_directorParent->InformAndGetDefaultScenePlacement();
	}
}

void CStorySceneDirectorPlacementHelper::GetSlotPlacement( const CStorySceneDialogsetSlot* slot, const CStorySceneDialogsetInstance* dialogset, EngineTransform& placement ) const
{
	EngineTransform dialogsetPlacement;
	FindDialogsetPlacement( dialogset, dialogsetPlacement );
	
	if ( slot != NULL )
	{
		const EngineTransform& slotPlacement = slot->GetSlotPlacement();

		Matrix dialogsetPlacementMatrix;
		dialogsetPlacement.CalcLocalToWorld( dialogsetPlacementMatrix );

		Vector slotPosition = dialogsetPlacementMatrix.TransformPoint( slotPlacement.GetPosition() );
		EulerAngles slotRotation = dialogsetPlacement.GetRotation() + slotPlacement.GetRotation();

		placement.SetPosition( slotPosition );
		placement.SetRotation( slotRotation );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
