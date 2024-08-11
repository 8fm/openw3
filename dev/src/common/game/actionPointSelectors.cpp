/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "actionPointSelectors.h"

#include "aiSpawnTreeParameters.h"
#include "behTreeInstance.h"
#include "commonGame.h"
#include "communitySystem.h"
#include "gameWorld.h"
#include "behTreeWorkData.h"
#include "movableRepresentationPathAgent.h"
#include "../engine/tagManager.h"
#include "../engine/areaComponent.h"


IMPLEMENT_ENGINE_CLASS( CActionPointSelector )
IMPLEMENT_ENGINE_CLASS( CCommunityActionPointSelector )
IMPLEMENT_ENGINE_CLASS( CTimetableActionPointSelector )
IMPLEMENT_ENGINE_CLASS( CSimpleActionPointSelector )
IMPLEMENT_ENGINE_CLASS( CWanderActionPointSelector )

//////////////////////////////////////////////////////////////////////////
// CActionPointSelector
/////////////////////////////////////////////////////////////////////////
CActionPointSelector::CActionPointSelector()
{
	EnableReferenceCounting( true );
}


CActionPointSelectorInstance* CActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return NULL;
}

CName CActionPointSelectorInstance::SelectRandomSupportedCategory( const TDynArray< CName >& categoriesList, const CActionPointComponent*const actionPoint )
{
	auto& randomGenerator = GEngine->GetRandomNumberGenerator();
	CName actionPointCategory;
	Uint32 validCategoryCount = 0;
	for( CName category : categoriesList )
	{
		if( actionPoint->GetActionCategories().Exist( category ) )
		{
			++validCategoryCount;
			if( randomGenerator.Get< Uint32 >( validCategoryCount ) == 0 )
			{
				actionPointCategory = category;
			}
		}			
	}
	return actionPointCategory;
}

void CActionPointSelectorInstance::OnNodeActivated()
{
}
void CActionPointSelectorInstance::OnNodeDeactivated()
{

}
Bool CActionPointSelectorInstance::IsExternallyReservingAP() const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CCommunityActionPointSelector
//////////////////////////////////////////////////////////////////////////
CActionPointSelectorInstance* CCommunityActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CCommunityActionPointSelectorInstance();
}
Bool CCommunityActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{
	const CNewNPC *const npc = behTreeInstance->GetNPC();
	if ( npc == nullptr )
	{
		return false;
	}

	const NewNPCScheduleProxy& scheduleProxy = npc->GetActionSchedule();
	scheduleProxy.AquireNextAP( npc, forceRadius );
	actionPointId = scheduleProxy.GetActiveAP();
	actionPointCategory = scheduleProxy.GetActionCategoryName();

	//looping in our AP through the CommunityEditor
	if ( scheduleProxy.Get() )
	{
		loopInAP = scheduleProxy.Get()->m_isUsingLastActionPoint;
	}

	return actionPointId != ActionPointBadID;
}

Bool CCommunityActionPointSelectorInstance::IsExternallyReservingAP() const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CTimetableActionPointSelector
//////////////////////////////////////////////////////////////////////////
CActionPointSelectorInstance* CTimetableActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CTimetableActionPointSelectorInstance( *this );
}
Bool CTimetableActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{
	const CNewNPC *const npc = behTreeInstance->GetNPC();
	if ( m_timetable.Empty() )
	{
		// no timetable - nothing to look for
		return false;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == NULL )
	{
		return false;
	}

	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
	if ( actionPointManager == NULL )
	{
		return false;
	}

	SActionPointFilter apFilter;
	communitySystem->FindCurrentActionPointFilter( m_timetable, apFilter );
	apFilter.m_onlyFree = true;
	apFilter.m_askingNPC = npc;
	if ( forceRadius > 0.f )
	{
		apFilter.m_sphere = Sphere( npc->GetWorldPositionRef(), forceRadius );
	}
	actionPointManager->FindActionPoint( actionPointId, apFilter );
	actionPointCategory = apFilter.m_category;


	return actionPointId != ActionPointBadID;
}

//////////////////////////////////////////////////////////////////////////
// CSimpleActionPointSelector
//////////////////////////////////////////////////////////////////////////

CSimpleActionPointSelectorInstance::CSimpleActionPointSelectorInstance( CSimpleActionPointSelector& def, CBehTreeSpawnContext& context )
	: CActionPointSelectorInstance()
	, m_categories( def.m_categories )
	, m_areaTags( def.m_areaTags )
	, m_apTags( def.m_apTags )
	, m_apArea( NULL )
	, m_keepActionPointOnceSelected( def.m_keepActionPointOnceSelected )
	, m_useNearestMatchingAP( def.m_useNearestMatchingAP )
{
	CAreaComponent* apArea = NULL;
	// Get custom ap area
	CName apAreaTag = def.m_apAreaTag;
	if( !apAreaTag.Empty() )
	{
		// Tag?
		CEntity* areaEntity = GCommonGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( apAreaTag );
		if( areaEntity )
		{
			apArea = areaEntity->FindComponent< CAreaComponent >();
		}
	}	

	// Default to encounter area
	if( !apArea)
	{
		apArea = CIdleBehaviorsDefaultParameters::GetDefaultActionPointArea( context );
	}

	m_apArea = apArea;
}

CActionPointSelectorInstance* CSimpleActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CSimpleActionPointSelectorInstance( *this, context );
}
CName CSimpleActionPointSelectorInstance::SelectRandomSupportedCategory( const CActionPointComponent*const actionPoint )const
{
	return Super::SelectRandomSupportedCategory( m_categories, actionPoint );
}
Bool CSimpleActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{
	// [ Step ] Init pointers
	const CBehTreeWorkData *const workData = workDataPtr.Get();
	if ( workData == nullptr )
	{
		return false;
	}

	if( m_apTags.Empty() )
	{
		return false;
	}

	const CNewNPC *const npc = behTreeInstance->GetNPC();

	if( !npc )
	{
		return false;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == NULL )
	{
		return false;
	}

	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
	if ( actionPointManager == NULL )
	{
		return false;
	}
	// [ Step ] init ID
	actionPointId		= ActionPointBadID;
	actionPointCategory	= CName::NONE;

	// [ Step ] Setting up action point filter
	SActionPointFilter apFilter;
	apFilter.m_onlyFree			= true;
	apFilter.m_actionPointTags	= m_apTags;
	apFilter.m_matchAll			= false;
	apFilter.m_askingNPC		= npc;

	if ( forceRadius > 0.0f )
	{
		apFilter.m_sphere = Sphere( npc->GetWorldPositionRef(), forceRadius );
	}

	// [ Step ] Do we Keep action point ?
	if ( m_keepActionPointOnceSelected )
	{
		//looping in our AP through the AP Spawner
		loopInAP = true;

		// [ Step ] First try to select the Ap that the NPC reserved before
		const SActionPointId & reservedAPId = workData->GetReservedAP();
		if ( reservedAPId != ActionPointBadID )
		{
			const CActionPointComponent*const actionPoint = actionPointManager->GetAP( reservedAPId );
			if ( actionPoint )
			{			
				actionPointId		= reservedAPId;
				actionPointCategory	= SelectRandomSupportedCategory( actionPoint );
				return true;
			}
		}
		
		// [ Step ] No reserved AP so use the last AP and check it has not be taken over by someone else 
		// This happens if the work tree has been interupted ( ex: reaction ) and another NPC took over the AP
		const SActionPointId & lastAPId		= workData->GetLastAP();
		if ( lastAPId != ActionPointBadID )
		{
			const CActionPointComponent*const actionPoint = actionPointManager->GetAP( lastAPId );
			if ( actionPoint && actionPoint->Filter( apFilter ) )
			{
				actionPointId		= lastAPId;
				actionPointCategory	= SelectRandomSupportedCategory( actionPoint );
				return true;
			}
		}
	}

	
	// [ Step ] Query action points
	CActionPointManager::APFilteredIterator it( *actionPointManager, apFilter );

	// [ Step ] Loop through available action points and select best :
	

	if( m_useNearestMatchingAP )
	{
		Float nearestDistanceSqrt = -1;
		const Vector3& npcPos = npc->GetWorldPositionRef();

		for( ;it;++it )
		{
			const CActionPointComponent* ap = *it;
			Float distanceSqrt = ( npcPos - ap->GetWorldPositionRef() ).SquareMag();
			if( nearestDistanceSqrt < 0 || nearestDistanceSqrt > distanceSqrt )
			{
				nearestDistanceSqrt = distanceSqrt;
				actionPointId		= ap->GetID();
				actionPointCategory = SelectRandomSupportedCategory( ap );
			}
		}
	}
	else
	{
		Uint32 apFoundCount = 0;
		CAreaComponent* area = m_apArea.Get();
		while ( it )
		{
			const CActionPointComponent* ap = *it;

			// [ Step ] Check which categories the action point supports and take first available one
			Uint32 validCategoryCount = 0;
			CName localActionPointCategory = SelectRandomSupportedCategory( ap );

			// [Step] If supported categories and point area test passed
			// Randomly select this action point :
			if ( !localActionPointCategory.Empty() && ( !area || ( area->GetBoundingBox().Contains( ap->GetActionExecutionPosition() ) && area->TestPointOverlap( ap->GetActionExecutionPosition() ) ) ) )
			{
				++apFoundCount;
				if ( GEngine->GetRandomNumberGenerator().Get< Uint32 >( apFoundCount ) == 0 )
				{
					actionPointId		= ap->GetID();
					actionPointCategory = localActionPointCategory;
				}
			}

			++it;
		}
	}
	return actionPointId != ActionPointBadID;
}


//////////////////////////////////////////////////////////////////////////
// CWanderActionPointSelectorInstance
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( SEncounterActionPointSelectorPair );

CWanderActionPointSelector::CWanderActionPointSelector()
	: m_categories ()
	, m_apTags ()
	, m_areaTags ()
	, m_delay ( 10.0f )
	, m_radius( 15.f )	
	, m_chooseClosestAP ( false )
{

}

CActionPointSelectorInstance* CWanderActionPointSelector::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CWanderActionPointSelectorInstance( *this, context );
}

CWanderActionPointSelectorInstance::CWanderActionPointSelectorInstance( CWanderActionPointSelector& def, CBehTreeSpawnContext& context )
	: CActionPointSelectorInstance()
	, m_categories( def.m_categories )
	, m_apTags( def.m_apTags )
	, m_areaTags( def.m_areaTags )
	, m_delay( def.m_delay )
	, m_delayTimer( 0.0f )
	, m_radius( def.m_radius )
	, m_reset( false )
	, m_elligibleCategories()
	, m_apArea( NULL )
	, m_ignoreDirection( false )
	, m_chooseClosestAP( def.m_chooseClosestAP )
{
	m_elligibleCategories.Resize( m_categories.Size() );

	// Get custom ap area
	CAreaComponent* apArea = NULL;
	// Get custom ap area
	CName apAreaTag = def.m_apAreaTag;
	if( !apAreaTag.Empty() )
	{
		// Tag?
		CEntity* areaEntity = GCommonGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( apAreaTag );
		if( areaEntity )
		{
			apArea = areaEntity->FindComponent< CAreaComponent >();
		}
	}	

	// Default to encounter area
	if( !apArea)
	{
		apArea = CIdleBehaviorsDefaultParameters::GetDefaultActionPointArea( context );
	}

	m_apArea = apArea;
}

Bool CWanderActionPointSelectorInstance::SelectActionPoint( const CBehTreeInstance* behTreeInstance, const CBehTreeWorkDataPtr & workDataPtr, SActionPointId& actionPointId, SActionPointId& lastActionPointId, CName& actionPointCategory, Bool& loopInAP, Float forceRadius )
{
	if( m_apTags.Empty() )
	{
		return false;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem == NULL )
	{
		return false;
	}

	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
	if ( actionPointManager == NULL )
	{
		return false;
	}

	if ( m_categories.Size() == 0 )
	{
		return false;
	}

	const CActor *const actor = behTreeInstance->GetActor();
	CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}


	//// Resetting chances
	if ( m_delayTimer < behTreeInstance->GetLocalTime() )
	{
		m_reset = true;
	}

	if ( m_reset )
	{
		m_reset			= false;
		m_delayTimer	= behTreeInstance->GetLocalTime() + m_delay;
		m_elligibleCategories.ClearFast();
		auto& randomGenerator = GEngine->GetRandomNumberGenerator();
		for ( Uint32 i = 0; i < m_categories.Size(); ++i )
		{
			const Float randVar = randomGenerator.Get< Float >( 100.0f );
			if ( randVar < m_categories[ i ].m_chance )
			{
				m_elligibleCategories.PushBack( m_categories[ i ].m_name );
			}
		}
	}
	actionPointId = ActionPointBadID;

	//// Getting a point one meter away from the actor on its path 
	const Vector &actorPos	= actor->GetWorldPositionRef();
	Vector3 dirToPath( 0.0f, 0.0f, 0.0f );
	CPathAgent *const pathAgent = mac->GetPathAgent();
	Vector3 pathPoint;
	if ( pathAgent->IsPathfollowing() && pathAgent->GetPathPointInDistance( 1.0f, pathPoint ) )
	{
		dirToPath	= pathPoint - actorPos.AsVector3();
	}
	else // if not possible dirToPath will be orient of actor :
	{
		dirToPath	= actor->GetWorldForward();
	}

	SActionPointFilter apFilter;

	apFilter.m_category			= CName::NONE; // We need to fetch all the categories available.
	apFilter.m_onlyFree			= true;
	apFilter.m_actionPointTags	= m_apTags;
	apFilter.m_matchAll			= false;
	apFilter.m_askingNPC		= actor;

	Float radius = forceRadius > 0.0f ? forceRadius : m_radius;
	apFilter.m_sphere = Sphere( actorPos, radius );

	CActionPointManager::APFilteredIterator it( *actionPointManager, apFilter );

	Uint32 apFoundCount		= 0;
	CAreaComponent* area	= m_apArea.Get();
	CName localActionPointCategory;

	Float closestDistanceSqr = -1;
	Float currentDistanceSqr;

	while ( it )
	{
		const CActionPointComponent* ap	= *it;
		const Vector3 dirToActionPoint				= ap->GetGoToPosition().AsVector3() - actorPos.AsVector3();
		// filter out action points that are behind us :
		if ( m_ignoreDirection || dirToPath.Dot( dirToActionPoint ) > 0.0f )
		{
			// Category choice
			CName selectedAPCategory = SelectRandomSupportedCategory( m_elligibleCategories, ap );

			// If supported categories and point area test passed
			if ( !selectedAPCategory.Empty() && ( area == NULL || ( area->GetBoundingBox().Contains( ap->GetActionExecutionPosition() ) && area->TestPointOverlap( ap->GetActionExecutionPosition() ) ) ) )
			{
				++apFoundCount;
				if( m_chooseClosestAP )
				{
					currentDistanceSqr = ( actorPos - ap->GetWorldPosition() ).SquareMag3();

					if( closestDistanceSqr < 0 || currentDistanceSqr < closestDistanceSqr )
					{
						actionPointId		= ap->GetID();
						actionPointCategory = selectedAPCategory;
						closestDistanceSqr	= currentDistanceSqr;
					}
				}
				else
				{
					if ( GEngine->GetRandomNumberGenerator().Get< Uint32 >( apFoundCount ) == 0 )
					{
						actionPointId		= ap->GetID();
						actionPointCategory = selectedAPCategory;
					}
				}
			}
		}
		++it;
	}

	Bool success = apFoundCount > 0;
	if( success == false )
	{
		actionPointCategory = CName::NONE;
	}

	return success;
}

void CWanderActionPointSelectorInstance::OnNodeActivated()
{
}
void CWanderActionPointSelectorInstance::OnNodeDeactivated()
{
	// reset all counters because we are done using the previously selected action point
	m_reset = true;
}
