/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communityDebugger.h"
#include "../../common/game/communityAgentStub.h"
#include "../../common/game/communityUtility.h"
#include "../../common/game/communityErrorReport.h"
#include "../../common/core/diskFile.h"


CCommunityDebugger::CCommunityDebugger()
	: m_communitySystem( NULL )
	, m_currentAgentIndex( -1 )
{
	m_msg.m_spawnedBeg = TXT("<font color=\"green\">");
	m_msg.m_spawnedEnd = TXT("</font>");
	m_msg.m_stubBeg    = TXT("<font color=\"blue\">");
	m_msg.m_stubEnd    = TXT("</font>");
	m_msg.m_currentBeg = TXT("<font color=\"black\">");
	m_msg.m_currentEnd = TXT("</font>");
	m_msg.m_spawned    = TXT("&lt;spawned&gt;");
	m_msg.m_stub       = TXT("&lt;stub&gt;");

	m_csDebugData.m_currentAgentStub = NULL;
	m_csDebugData.m_currentAPID = ActionPointBadID;
}

Bool CCommunityDebugger::Init()
{
	m_communitySystem = GCommonGame->GetSystem< CCommunitySystem >();

	if ( m_communitySystem )
	{
		m_isInitialized = true;
	}
	else
	{
		m_isInitialized = false;
	}

	return m_isInitialized;
}

void CCommunityDebugger::DeInit()
{
	m_communitySystem = NULL;
	m_isInitialized = false;
	m_csDebugData.m_currentAgentStub = NULL;
	m_csDebugData.m_currentAPID = ActionPointBadID;

	m_currentAgentIndex = -1;
	m_agentsShortInfo.Clear();
	m_currentAgentInfo = TXT("");
	
	m_currentActionPointIndex = -1;
	m_actionPointsShortInfo.Clear();
	m_currentActionPointInfo = TXT("");

	m_currentDespawnPlaceIndex = -1;
	m_despawnPlacesShortInfo.Clear();

	m_currentStoryPhaseIndex = -1;
	m_storyPhasesShortInfo.Clear();
}

void CCommunityDebugger::Update()
{
	if ( !m_isInitialized ) return;

	// update agents short info
	m_agentsShortInfo.Clear();
	for ( Uint32 agentStubIndex = 0; agentStubIndex < m_communitySystem->m_agentsStubs.Size(); ++agentStubIndex )
	{
		const SAgentStub *agentStub = m_communitySystem->m_agentsStubs[ agentStubIndex ];

		String agentShortInfo = String::Printf( TXT("%d : "), agentStubIndex );
		if ( agentStub->m_npc.Get() )
		{
			agentShortInfo += m_msg.m_spawnedBeg;
			agentShortInfo += m_msg.m_spawned + agentStub->m_npc.Get()->GetFriendlyName();
			agentShortInfo += m_msg.m_spawnedEnd;
		}
		else
		{
			agentShortInfo += m_msg.m_stubBeg;
			agentShortInfo += m_msg.m_stub;
			agentShortInfo += m_msg.m_stubEnd;
		}
		m_agentsShortInfo.PushBack( agentShortInfo );
	}

	// update agent detailed info
	if ( m_currentAgentIndex != -1 && (Uint32)m_currentAgentIndex < GetAgentsNum() )
	{
		m_currentAgentInfo = GetInfoForAgentStub( m_communitySystem->m_agentsStubs[ m_currentAgentIndex ] );
	}

	// update action points short info
	m_actionPointsShortInfo.Clear();
	typedef THashMap< TActionPointID, CActionPointComponent* >::const_iterator ActionPointsCI;
	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	for ( auto& apEntry : apMan->m_actionPointsById )
	{
		const CActionPointComponent* ap = apEntry.m_second;
		String actionPointShortInfo;
		actionPointShortInfo += /*apEntryCI->m_first.AsString() + TXT(" : ") + */ap->GetFriendlyName() +
			( ap->m_isOccupied ? TXT("Occupied") : TXT("Free") ) + ( ap->m_haveItemsReady ? TXT("") : TXT("NotReady") );
		m_actionPointsShortInfo.PushBack( actionPointShortInfo );
	}

	// update action points detailed info
	if ( m_currentActionPointIndex != -1 && (Uint32)m_currentActionPointIndex < GetActionPointsNum() )
	{
		m_currentActionPointInfo = GetInfoForActionPoint( GetActionPointByIndex( m_currentActionPointIndex ) );
	}

	// update despawn places short info
/*	m_despawnPlacesShortInfo.Clear();
	for ( THashMap< CGUID, const CWayPointComponent* >::const_iterator layerDespawnPlace = m_communitySystem->m_layersDespawnPlaces.Begin();
		  layerDespawnPlace != m_communitySystem->m_layersDespawnPlaces.End();
		  ++layerDespawnPlace )
	{
		const CGUID &layerGUID = layerDespawnPlace->m_first;
		const CWayPointComponent *despawnPlace = layerDespawnPlace->m_second;
		String despawnPlaceShortInfo;

		if ( CLayer *layer = GGame->GetActiveWorld()->FindLayer( layerGUID ) )
		{
			if ( CLayerInfo *layerInfo = layer->GetLayerInfo() )
			{
				despawnPlaceShortInfo += TXT("Layer name: ") + layerInfo->GetFriendlyName() + TXT(" : ");
			}
		}
		despawnPlaceShortInfo += despawnPlace->GetFriendlyName();

		m_despawnPlacesShortInfo.PushBack( despawnPlaceShortInfo );
	}*/

	// update story phases short info
	m_storyPhasesShortInfo.Clear();
	const TDynArray< THandle< CCommunity > >& spawnsets = m_communitySystem->GetActiveSpawnsets();
	for ( TDynArray< THandle< CCommunity > >::const_iterator spIt = spawnsets.Begin();
		spIt != spawnsets.End(); ++spIt )
	{
		const CCommunity* spawnset = (*spIt).Get();
		if ( !spawnset)
		{
			continue;
		}

		String storyPhaseShortInfo;
		if ( spawnset->GetActivePhaseName() == CName::NONE )
		{
			storyPhaseShortInfo = TXT( "Default" );
		}
		else
		{
			storyPhaseShortInfo = spawnset->GetActivePhaseName().AsString();
		}

		storyPhaseShortInfo += TXT( " : " );

		if ( spawnset->GetFile() )
		{
			storyPhaseShortInfo += spawnset->GetFile()->GetDepotPath();
		}
		else
		{
			storyPhaseShortInfo += TXT( "<<unidentified spawnset>>" );
		}

		m_storyPhasesShortInfo.PushBack( storyPhaseShortInfo );
	}
}

//////////////////////////////////////////////////////////////////////////

const TDynArray< String >& CCommunityDebugger::GetAgentsShortInfo()
{
	return m_agentsShortInfo;
}

Uint32 CCommunityDebugger::GetAgentsNum()
{
	return m_communitySystem ? m_communitySystem->m_agentsStubs.Size() : 0;
}

Bool CCommunityDebugger::SetCurrentAgent( Uint32 agentIndex )
{
	if ( !m_isInitialized ) return false;

	if ( agentIndex < GetAgentsNum() )
	{
		m_currentAgentIndex = agentIndex;
		m_csDebugData.m_currentAgentStub = m_communitySystem->m_agentsStubs[ agentIndex ];
		m_communitySystem->SetDebugData( m_csDebugData );
		return true;
	}
	else
	{
		return false;
	}
}

const String& CCommunityDebugger::GetCurrentAgentInfo()
{
	return m_currentAgentInfo;
}

Bool CCommunityDebugger::GetAgentInfo( Uint32 agentIndex, String &agentInfo )
{
	if ( !m_isInitialized ) return false;

	if ( agentIndex < GetAgentsNum() )
	{
		agentInfo = GetInfoForAgentStub( m_communitySystem->m_agentsStubs[ agentIndex ] );
		return true;
	}
	else
	{
		return false;
	}
}

String CCommunityDebugger::GetInfoForAgentStub( const SAgentStub *agentStub )
{
	String result;

	result += TXT("<html><head></head><body>");

	if ( agentStub->m_npc.Get() )
	{
		result += TXT("<h2>Spawned</h2><br><br>");

		CNewNPC *npc = agentStub->m_npc.Get();
		String npcState = npc->GetCurrentStateName().AsString();

		result += TXT("NPC friendly name: ") + npc->GetFriendlyName() + TXT("<br>");
		result += TXT("NPC template friendly name: ") + npc->GetTemplate()->GetFriendlyName() + TXT("<br>");
		result += TXT("NPC state: <b>") + npcState + TXT("</b><br><br>");

		// NPC Vector Status
		result += TXT("<h3>NPC state vector status</h3><br>");
		// Combat
		result += TXT("<h4>Combat Info</h4><br>");

		if ( npc->GetTarget() )
		{
			result += TXT("Target friendly name: ") + npc->GetTarget()->GetFriendlyName() + TXT("<br>");
		}

		// Working
		result += TXT("<h4>Working Info</h4><br>");

		// Rest info
		result += TXT("<h4>Miscelaneous Info</h4><br>");
		result += TXT("<b>Noticed objects</b>:<br>");
		const TDynArray< NewNPCNoticedObject >& noticedObjects = npc->GetNoticedObjects();
		for ( TDynArray< NewNPCNoticedObject >::const_iterator noticedObj = noticedObjects.Begin();
			noticedObj != noticedObjects.End();
			++noticedObj )
		{
			result += noticedObj->ToString();
			result.Append( TXT("<br>"), 4 );
		}
	}
	else
	{
		result += TXT("<h2>Agent stub</h2><br><br>");

		result += TXT("Current state: ") + CCommunityUtility::GetFriendlyAgentStateName( agentStub->m_state ) + TXT("<br><br>");

		const NewNPCSchedule& schedule = agentStub->GetSchedule();
		if ( agentStub->m_state == CAS_WorkInProgress )
		{
			CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

			result += TXT("Current AP component name: ") + apMan->GetFriendlyAPName( schedule.m_activeActionPointID );
		}


		result += TXT("Processing timer: " ) + String::Printf( TXT("%f"), agentStub->m_processingTimer ) + TXT("<br>");
	}

	result += TXT("Appearance: ") + agentStub->m_appearance.AsString() + TXT("<br>");

	result += TXT("</body></html>");

	return result;
}

//////////////////////////////////////////////////////////////////////////

const TDynArray< String >& CCommunityDebugger::GetActionPointsShortInfo()
{
	return m_actionPointsShortInfo;
}

Uint32 CCommunityDebugger::GetActionPointsNum()
{
	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	return apMan ? apMan->m_actionPointsById.Size() : 0;
}

Bool CCommunityDebugger::SetCurrentActionPoint( Uint32 apIndex )
{
	if ( !m_isInitialized ) return false;

	if ( apIndex < GetActionPointsNum() )
	{
		m_currentActionPointIndex = apIndex;
		m_csDebugData.m_currentAPID = GetActionPointIDByIndex( apIndex );
		m_communitySystem->SetDebugData( m_csDebugData );
		return true;
	}
	else
	{
		return false;
	}
}

const String& CCommunityDebugger::GetCurrentActionPointInfo()
{
	return m_currentActionPointInfo;
}

Bool CCommunityDebugger::GetActionPointInfo( Uint32 apIndex, String &apInfo )
{
	if ( !m_isInitialized ) return false;

	if ( apIndex < GetActionPointsNum() )
	{
		apInfo = GetInfoForActionPoint( GetActionPointByIndex( apIndex ) );
		return true;
	}
	else
	{
		return false;
	}
}

TActionPointID CCommunityDebugger::GetActionPointIDByIndex( Uint32 index )
{
	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( apMan == NULL ) return ActionPointBadID;

	for ( auto& actionPointEntry : apMan->m_actionPointsById )
	{
		if ( index == 0 )
		{
			return actionPointEntry.m_first;
		}
		--index;
	}
	return ActionPointBadID;
}

CActionPointComponent* CCommunityDebugger::GetActionPointByIndex( Uint32 index )
{
	Uint32 currentIndex = 0;

	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	if ( apMan == NULL ) return NULL;

	for ( auto& actionPointEntry : apMan->m_actionPointsById )
	{
		if ( index == 0 )
		{
			return actionPointEntry.m_second;
		}
		--index;
	}

	return NULL;
}

String CCommunityDebugger::GetInfoForActionPoint( const CActionPointComponent *actionPoint )
{
	String result;

	//result += TXT("Action Point name: ") + actionPoint->GetName() + TXT("<br>");
	result += TXT("Action Point friendly name: ") + actionPoint->GetFriendlyName() + TXT("<br>");

	result += TXT("AP Categories: ");
	for ( auto it = actionPoint->GetActionCategories().Begin(), end = actionPoint->GetActionCategories().End(); it != end; )
	{
		result += it->AsString();
		if ( ++it != actionPoint->GetActionCategories().End() )
		{
			result += TXT(", ");
		}
	}
	result += TXT("<br>");

	if ( ! actionPoint->m_isOccupied )
	{
		result += TXT("Action point is free.<br>");
	}
	else
	{
		result += TXT("Action point is occupied.<br>");
	}

	if ( !actionPoint->m_haveItemsReady )
	{
		result += TXT("Action point has items dropped away, waiting for reset<br>");
	}
	/*else
	{
		result += TXT("Action point ");
	}*/

	if ( ! actionPoint->GetPreferredNextAPsTagList().Empty() )
	{
		result += TXT("Preferred next APs: ") + GetStringFromArray( actionPoint->GetPreferredNextAPsTagList().GetTags() ) + TXT("<br>");
	}
	else
	{
		result += TXT("No preferred next APs.<br>");
	}

	if ( actionPoint->m_apFlags & CActionPointComponent::AP_FLAG_INVALID_WP_POS )
	{
		result += TXT("Unusable due to invalid waypoint position.<br>");
	}

	if ( actionPoint->m_apFlags & CActionPointComponent::AP_FLAG_MISSING_WP )
	{
		result += TXT("Unusable due to missing waypoint.<br>");
	}

	if ( actionPoint->m_problemReports > 0 )
	{
		result += TXT("Caused ") + ToString(actionPoint->m_problemReports) + TXT(" problem reports");
		if ( actionPoint->m_problemReports > MAX_FAUL_AP_REPORTS )
		{
			result += TXT("(TOO MUCH REPORTS, AP DISABLED)");
		}
	}

	/*if ( actionPoint->IsGrabType() )
	{
		result += TXT("Action Point is a GRAB type.");
	}*/

	return result;
}

//////////////////////////////////////////////////////////////////////////

const TDynArray< String >& CCommunityDebugger::GetDespawnPlacesShortInfo()
{
	return m_despawnPlacesShortInfo;
}

Uint32 CCommunityDebugger::GetDespawnPlacesNum()
{
	return 0;//m_communitySystem ? m_communitySystem->m_layersDespawnPlaces.Size() : 0;
}

Bool CCommunityDebugger::SetCurrentDespawnPlace( Uint32 despawnPlaceIndex )
{
	if ( !m_isInitialized ) return false;

	if ( despawnPlaceIndex < GetDespawnPlacesNum() )
	{
		m_currentDespawnPlaceIndex = despawnPlaceIndex;
//		m_csDebugData.m_despawnWayPoint = (m_communitySystem->m_layersDespawnPlaces.Begin() + despawnPlaceIndex)->m_second;
		m_communitySystem->SetDebugData( m_csDebugData );
		return true;
	}
	else
	{
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////

const TDynArray< String >& CCommunityDebugger::GetStoryPhasesShortInfo()
{
	return m_storyPhasesShortInfo;
}

Bool CCommunityDebugger::SetCurrentStoryPhase( Uint32 storyPhaseIndex )
{
	return false;
	/*
	if ( storyPhaseIndex < GetStoryPhasesNum() )
	{
		m_currentStoryPhaseIndex = storyPhaseIndex;
		return true;
	}
	else
	{
		return false;
	}*/
}

//////////////////////////////////////////////////////////////////////////

String CCommunityDebugger::GetStringFromArray( const TDynArray< CName > &names )
{
	String result;

	for ( TDynArray< CName >::const_iterator name = names.Begin();
		  name != names.End(); )
	{
		result += name->AsString();
		if ( ++name != names.End() )
		{
			result += TXT(", ");
		}
	}

	return result;
}
