/**	
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "actionPointComponent.h"
#include "jobTreeNode.h"
#include "jobTree.h"
#include "actionPoint.h"
#include "communitySystem.h"
#include "../engine/performableAction.h"
#include "entityActionsRouter.h"
#include "../core/gatheredResource.h"
#include "../core/dataError.h"
#include "../core/engineTime.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "../engine/renderFrame.h"
#include "../engine/bitmapTexture.h"
#include "../engine/utils.h"
#include "../engine/deniedAreaComponent.h"
#include "../engine/game.h"

IMPLEMENT_RTTI_ENUM( EWorkPlacementImportance );
IMPLEMENT_ENGINE_CLASS( CActionPointComponent );


CGatheredResource resActionPointIcon( TXT("engine\\textures\\icons\\actionpointicon.xbm"), RGF_NotCooked );

const Float CActionPointComponent::MAXIMUM_Z_DISPLACEMENT = 0.5f;
const Float CActionPointComponent::PRECISE_PLACEMENT_MARGIN = 0.01f;
const Float CActionPointComponent::PRECISE_ANGLE_MARGIN = 0.011f;

CActionPointComponent::CActionPointComponent()
	: m_placementImportance( WPI_Precise )
	, m_breakableByCutscene( true )
	, m_isEnabled( true )
	, m_activateOnStart( true )
	, m_isActive( true )
	, m_apFlags( 0 )
	, m_fireSourceDependent( false )
	, m_reservationTimeout( 0.f )
	, m_forceKeepIKactive( false )
{
}

Bool CActionPointComponent::IsFree( const CActor* const askingNPC ) const 
{ 
	if( !m_isActive )
	{
		return false;
	}

	if( !m_isOccupied )
	{
		return true;
	}

	if( m_isOccupied ==  CActionPointManager::REASON_RESERVATION )
	{
		if( m_reservationTimeout < GGame->GetEngineTime() )
		{
			m_reservationTimeout = 0.f;
			m_isOccupied &= ~CActionPointManager::REASON_RESERVATION;
			return true;
		}
		else
		{
			CNewNPC* occupying = m_occupyingNPC.Get();
			if( occupying == nullptr || (askingNPC && askingNPC == occupying) )
			{
				return true;
			}
		}
	}
	return false;
}


void CActionPointComponent::SetReserved( Uint8 reason, const CNewNPC* occupyingNPC, Float reservationTime )
{ 
	m_isOccupied |= reason; 
	m_occupyingNPC = const_cast< CNewNPC* >( occupyingNPC ); 
	if( reservationTime > 0.f )
	{
		EngineTime cTime = GGame->GetEngineTime();
		m_reservationTimeout = cTime + reservationTime;
	}
}

void CActionPointComponent::SetActive( Bool active )
{
	if ( active != m_isActive )
	{
		m_isActive = active;
		if ( !active && (m_isOccupied & (~CActionPointManager::REASON_RESERVATION)) != 0 )
		{
			if ( CNewNPC* occupyingNPC = m_occupyingNPC.Get() )
			{
				occupyingNPC->SignalGameplayEvent( CNAME( OnTimetableChanged ) );
				if ( NewNPCSchedule* schedule = occupyingNPC->GetActionSchedule().Get() )
				{			
					schedule->m_activeActionPointID = ActionPointBadID;
				}
			}
		}
	}
}

Bool CActionPointComponent::MatchAllTags( const TagList& tags ) const
{
	const TagList& entityTags = GetEntity()->GetTags();

	for ( auto tag : tags.GetTags() )
	{
		if ( !m_tags.HasTag( tag ) && !entityTags.HasTag( tag ) )
		{
			return false;
		}
	}
	return true;
}

Bool CActionPointComponent::Filter( const SActionPointFilter& filter )const
{
	if( m_isActive == false )
	{
		return false;
	}

	// is ap disabled
	if ( m_problemReports > MAX_FAUL_AP_REPORTS || ( m_apFlags & ( CActionPointComponent::AP_FLAG_INVALID_WP_POS | CActionPointComponent::AP_FLAG_MISSING_WP ) ) )
	{
		return false;
	}

	// does ap category match
	if ( filter.m_category && !GetActionCategories().Exist( filter.m_category ) )
	{
		return false;
	}

	// is ap occupied
	if ( filter.m_onlyFree && !IsFree( filter.m_askingNPC ) )
	{
		return false;
	}

	// filter by layer
	if ( filter.m_layerGuid != CGUID::ZERO && filter.m_layerGuid != GetLayerGUID() )
	{
		return false;
	}

	// filter by tags
	if ( filter.m_matchAll ? !MatchAllTags( filter.m_actionPointTags ) : !MatchAnyTag( filter.m_actionPointTags ) )
	{
		return false;
	}

	// 
	if ( m_haveItemsReady == false )
	{
		return false;
	}

	// spatial filter
	if ( filter.m_sphere.GetRadius() > 0.f && !filter.m_sphere.Contains( GetWorldPosition() ) )
	{
		return false;
	}

	return true;
}

void CActionPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	CActionPoint* entAsAP = Cast< CActionPoint >( GetEntity() );
	if ( entAsAP )
	{
		if( entAsAP->m_overrideActionBreakableInComponent )
		{
			m_actionBreakable = entAsAP->m_actionBreakable;
		}		
	}
	else
	{
		DATA_HALT( DES_Minor, GetActionPointResource(), TXT("Community"), TXT( "Action point entity is not of CActionPointClass. Entity: %s" ), GetEntity()->GetFriendlyName().AsChar() );
	}
	m_apID = TActionPointID( GetGUID(), GetEntity()->GetGUID() );

	ConditionalAttachToAPManager( world );
}

void CActionPointComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	ConditionalAttachToAPManager( world );
}

void CActionPointComponent::ConditionalAttachToAPManager( CWorld* world )
{
	// Unregister ap from game
	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	CActionPointManager* actionPointManager = communitySystem ? communitySystem->GetActionPointManager() : nullptr;
	if ( actionPointManager )
	{
		actionPointManager->UnregisterActionPoint( GetID() );
	}

	// Attach
	if ( IsAttached() && IsEnabled() )
	{
		// Get inventory
		m_inventoryComponent = GetEntity()->FindComponent< CInventoryComponent >();

		// If we have valid path engine position register in the world

#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
		if ( IsPositionValid() )
		{
#endif
			if ( actionPointManager )
			{
				actionPointManager->RegisterActionPoint( this );
			}
#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
		}
		else
		{
			WARN_GAME( TXT("Action Point Error: Unable to register action point '%ls': INVALID PLACEMENT"), GetFriendlyName().AsChar() );
		}
#endif

		// Log if not valid
		#ifndef NO_EDITOR_ENTITY_VALIDATION
		TDynArray< String > logReport;
		if ( !OnValidate( logReport ) )
		{
			VALIDATION_FAIL( TXT("Validation failed for %s"), GetFriendlyName().AsChar() );
			for ( Uint32 i=0; i<logReport.Size(); ++i )
			{
				VALIDATION_FAIL( logReport[i].AsChar() );
			}
		}
		#endif
	}
}

void CActionPointComponent::SetEnabled( Bool enabled )
{
	if ( m_isEnabled != enabled )
	{
		SetShouldSave( true );

		// Toggle
		m_isEnabled = enabled;

		// Reattach
		if ( IsAttached() )
		{
			CWorld* attachedWorld = GetEntity()->GetLayer()->GetWorld();
			ConditionalAttachToAPManager( attachedWorld );
		}
	}
}

void CActionPointComponent::WaypointGenerateEditorFragments( CRenderFrame* frame )
{
	TBaseClass::WaypointGenerateEditorFragments( frame );

	if ( IsVisible() )
	{
		CBitmapTexture* icon = GetSpriteIcon();
		if ( icon == NULL )
		{
			return;
		}

		Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
		const Float size = 0.25f*screenScale;

		Color spriteColor = CalcSpriteColor();

		if ( !m_isEnabled && GGame->IsActive() && GetLayer()->GetWorld() == GGame->GetActiveWorld() )
		{
			spriteColor = Color( 40, 40, 40 );
		}
		else
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
		if ( IsPositionValid() == false )
		{
			spriteColor = Color::RED;
		}
		else 
#endif
		{
			Vector validPlacement;
			if ( IsApPlacementValid( &validPlacement ) == false )
			{
				spriteColor = Color::YELLOW;

				if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ErrorState ) == true )
				{
					Vector apPosition = GetWorldPositionRef();
					String apPlacementMessage = String::Printf( TXT( "Valid Z: %0.4f (%0.4f)" ), validPlacement.Z, validPlacement.Z - apPosition.Z );
					frame->AddDebugText( GetWorldPositionRef(), apPlacementMessage, true );
				}
			}
			else
			{
				spriteColor = CalcSpriteColor();		
			}
		}

		

#ifndef NO_COMPONENT_GRAPH
		frame->AddSprite( GetLocalToWorld().GetTranslation(), size, spriteColor, GetHitProxyID(), icon, IsOverlay() );
#else
		frame->AddSprite( GetLocalToWorld().GetTranslation(), size, spriteColor, CHitProxyID(), icon, IsOverlay() );
#endif
	}

#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	if ( !IsPositionValid() )
	{
		frame->AddDebugText( GetWorldPosition() + Vector(0,0,1.7f), TXT("AP NOT REACHABLE!"), true );
	}
#endif
}

#ifndef NO_DATA_VALIDATION
void CActionPointComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// Action point component is not inside an CActionPoint entity
	if ( GetEntity() && !GetEntity()->IsA< CActionPoint >() && !isInTemplate )
	{
		DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity should have a CActionPoint entity class but has '%ls'"), GetEntity()->GetClass()->GetName().AsString().AsChar() );
	}

	// No job tree used
	CJobTree* tree = m_jobTreeRes.Get();
	if ( !tree )
	{
		DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has no job tree") );
	}
	else
	{
		// Get root node
		CJobTreeNode* treeRoot = tree->GetRootNode();
		if ( !treeRoot )
		{
			DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has a job tree without a root node") );
		}

		// No categories defined
		const TDynArray< TAPCategory >& categories = treeRoot->GetCategories();
		if ( categories.Empty() )
		{
			DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has a job tree with no job categories") );
		}

		// Check places
		TDynArray< CName > placesNames;
		treeRoot->CollectWorkingPlacesNames( treeRoot, placesNames );

		// Check if waypoint component exists for each place specified
		CEntity* entity = GetEntity();
		for ( Uint32 i=0; i<placesNames.Size(); ++i )
		{
			const CName& place = placesNames[i];
			if ( place != CName::NONE )
			{
				CComponent* found = entity->FindComponent( place.AsString() );
				if ( !found )
				{
					DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity uses non existing work place '%ls'"), place.AsString().AsChar() );
				}
				else if ( !found->IsA< CWayPointComponent >() )
				{
					DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity uses work place '%ls' that is not an CWayPointComponent"), place.AsString().AsChar() );
				}
			}
		}
	}

	if ( !isInTemplate && GetEntity() && GetEntity()->GetLayer() )
	{
		// Check the next preferred AP
		if ( !m_preferredNextAPs.Empty() && GetEntity()->GetLayer() )
		{
			// Get the world we are attached to
			TDynArray< CActionPoint* > nextAps;
			const LayerEntitiesArray& entities = GetEntity()->GetLayer()->GetEntities();
			for ( Uint32 i=0; i<entities.Size(); ++i )
			{
				CActionPoint* apEntity = Cast< CActionPoint >( entities[i] );
				if ( apEntity && apEntity != GetEntity() )
				{
					// Check tags, it they match it's a possible next AP
					if ( TagList::MatchAll( m_preferredNextAPs, apEntity->GetTags() ) )
					{
						nextAps.PushBack( apEntity );
					}
				}
			}

			if ( nextAps.Empty() )
			{
				// No APs entities found at all
				DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has missing preferred AP tagged '%ls'"), m_preferredNextAPs.ToString().AsChar() );
			}
			else
			{
				// Test AP components
				Uint32 numComponents = 0;
				Bool hasValidNextApComponent = false;
				for ( Uint32 i=0; i<nextAps.Size(); ++i )
				{
					for ( ComponentIterator< CActionPointComponent > it( nextAps[i] ); it; ++it )
					{
						CActionPointComponent* apc = *it;
						++numComponents;
						if ( apc->CanDoAction( CNAME( sequence ) ) )
						{
							hasValidNextApComponent = true;
							break;
						}
					}
				}

				// No valid components
				if ( !hasValidNextApComponent )
				{
					if ( numComponents )
					{
						DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has preferred AP tagged '%ls' that does not support the 'sequence' category"), m_preferredNextAPs.ToString().AsChar() );
					}
					else
					{
						DATA_HALT( DES_Major, GetActionPointResource(), TXT("Community"), TXT("Action point entity has preferred AP tagged '%ls' that does not have any action point components"), m_preferredNextAPs.ToString().AsChar() );
					}

				}
			}
		}
	}
}
#endif

Bool CActionPointComponent::IsApPlacementValid( Vector* validPlacement /*= NULL */ ) const
{
	Bool placementValid = true;
	
	if ( GGame == NULL || GGame->GetActiveWorld().Get() == NULL )
	{
		return true;
	}

	if ( GGame->IsActive() == true )
	{
		return true;
	}

	CPhysicsWorld* physicsWorld = nullptr;
	if ( GGame->GetActiveWorld()->GetPhysicsWorld( physicsWorld ) )
	{
		Vector apExecutionPosition = GetWorldPositionRef();
		CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );

		SPhysicsContactInfo terrainContactInfo;
		Bool isNearTerrain = physicsWorld->RayCastWithSingleResult( 
			apExecutionPosition + Vector( 0.0f, 0.0f, MAXIMUM_Z_DISPLACEMENT ),
			apExecutionPosition - Vector( 0.0f, 0.0f, MAXIMUM_Z_DISPLACEMENT ),
			include, 0,
			terrainContactInfo ) == TRV_Hit;
		
		if ( isNearTerrain )
		{
			if ( validPlacement != NULL )
			{
				*validPlacement = terrainContactInfo.m_position;
			}

			Bool needsPrecision = true;
			CJobTree* jobTree = m_jobTreeRes.Get();
			if ( jobTree != NULL )
			{
				//needsPrecision = jobTree->GetSettings().m_needsPrecision;
			}
			if ( needsPrecision == true )
			{
				placementValid = MAbs( apExecutionPosition.Z - terrainContactInfo.m_position.Z ) <= PRECISE_PLACEMENT_MARGIN;
			}
			else
			{
				apExecutionPosition.Z = terrainContactInfo.m_position.Z;
				placementValid = true;
			}
		}
		else
		{
			return false;
		}

		//Float characterRadius = SCCTDefaults::DEFAULT_RADIUS;
		//Float characterHeight = SCCTDefaults::DEFAULT_HEIGHT;

		//placementValid &= physicsWorld->SweepTestAnyResult( 
		//	apExecutionPosition + Vector( 0.0f, 0.0f, characterRadius ), 
		//	apExecutionPosition + Vector( 0.0f, 0.0f, characterHeight - characterRadius ), 
		//	characterRadius, 
		//	characterCollisionMask ) == false;
	}
	return placementValid;
}

Bool CActionPointComponent::CanDoAction( const TDynArray< TAPCategory > &actionCategories ) const
{
	for ( TDynArray< TAPCategory >::const_iterator actionCategory = actionCategories.Begin();
		  actionCategory != actionCategories.End();
		  ++actionCategory )
	{
		if ( CanDoAction( *actionCategory ) ) return true;
	}
	return false;
}

TAPCategory CActionPointComponent::GetRandomActionCategory() const
{
	const TDynArray< TAPCategory >& categoryList = GetActionCategories();
	if ( categoryList.Empty() )
	{
		return CName::NONE;
	}
	else
	{
		Uint32 acIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( categoryList.Size() );
		return categoryList[ acIndex ];
	}
}

CBitmapTexture* CActionPointComponent::GetSpriteIcon() const
{
	return resActionPointIcon.LoadAndGet< CBitmapTexture >();
}

Color CActionPointComponent::CalcSpriteColor() const
{
	return TBaseClass::CalcSpriteColor();
}

void CActionPointComponent::ResetItems()
{
	if ( m_inventoryComponent )
	{
		m_inventoryComponent->SpawnMountedItems();
	}
}

#ifndef NO_EDITOR_ENTITY_VALIDATION
Bool CActionPointComponent::OnValidate( TDynArray< String >& log ) const
{
	// Validate base class
	Bool result = TBaseClass::OnValidate( log );

	// ap is pointless if it has no job tree
	CJobTree *jobTreeRes = m_jobTreeRes.Get();
	if ( jobTreeRes == NULL )
	{
		log.PushBack( String::Printf( TXT("%s has no job tree"), m_name.AsChar() ) );
		return false;
	}

	CJobTreeNode* jobTreeRoot = jobTreeRes->GetRootNode();
	// Make sure job tree has at least root node (this should never happen) 
	ASSERT( jobTreeRoot );
	if ( !jobTreeRoot )
	{
		log.PushBack( String::Printf( TXT("%s has broken job tree"), m_name.AsChar() ) );
		return false;
	}

	// Collect place names from job tree
	TDynArray< CName >	placesNames;
	jobTreeRoot->CollectWorkingPlacesNames( jobTreeRoot, placesNames );

	// Check if waypoint component exists for each place specified
	CEntity* entity = GetEntity();
	for ( Uint32 i=0; i<placesNames.Size(); ++i )
	{
		const CName& place = placesNames[i];
		if ( place == CName::NONE )
		{
			continue;
		}
		CComponent* found = entity->FindComponent( place.AsString() );
		if ( !found )
		{
			result = false;
			log.PushBack( String::Printf( TXT("%s has no waypoint %s"), m_name.AsChar(), place.AsString().AsChar() ) );
			continue;
		}
		if ( !found->IsA< CWayPointComponent >() )
		{
			result = false;
			log.PushBack( String::Printf( TXT("Component %s is expected to be a waypoint due to job tree 'place' definition"), place.AsString().AsChar() ) );
			continue;
		}		
	}

	return result;
}
#endif

EAttachmentGroup CActionPointComponent::GetAttachGroup() const
{
	return ATTACH_GROUP_0;
}

const TagList& CActionPointComponent::GetPreferredNextAPsTagList() const
{
	return m_preferredNextAPs;
}

Bool CActionPointComponent::HasPreferredNextAPs() const
{
	return m_preferredNextAPs.Empty() == false;
}

void CActionPointComponent::WorkStarted()
{
	IPerformableAction::PerformAll( m_eventWorkStarted, GetEntity() );	
}

void CActionPointComponent::WorkEnded()
{
	IPerformableAction::PerformAll( m_eventWorkEnded, GetEntity() );
}

Bool CActionPointComponent::IsBreakable() const 
{ 
	return m_actionBreakable;
}

EWorkPlacementImportance CActionPointComponent::GetPlacementImportance() const 
{ 
	return m_placementImportance;
}

const TDynArray< TAPCategory > &CActionPointComponent::GetActionCategories() const
{
	CJobTree* tree = m_jobTreeRes.Get();
	if ( tree && tree->GetRootNode() )
	{
		return tree->GetRootNode()->GetCategories();
	}
	else
	{
		static TDynArray< TAPCategory > emptyCategoryList;
		return emptyCategoryList;
	}
}

Bool CActionPointComponent::CanDoAction( const TAPCategory &actionCategory ) const
{
	const TDynArray< TAPCategory >& categoryList = GetActionCategories();
	return categoryList.Exist( actionCategory );
}

void CActionPointComponent::funcGetID( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( SActionPointId, GetID() );
}

const CResource* CActionPointComponent::GetActionPointResource() const
{
	return CResourceObtainer::GetResource( this );
}
