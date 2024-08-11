/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behTreeNodeSelectActionPoint.h"

#include "../engine/renderFrame.h"

#include "actionPointManager.h"
#include "actionPointSelectors.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "communitySystem.h"
#include "newNpc.h"
#include "newNpcSchedule.h"


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeSelectActionPointDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

const Float	CBehTreeNodeSelectActionPointDecoratorInstance::AP_RESERVATION_TIME = 30; //seconds

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
CBehTreeNodeSelectActionPointDecoratorInstance::CBehTreeNodeSelectActionPointDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) 
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_workData( owner )
	, m_apSelector( NULL )
	, m_chosenAP( ActionPointBadID )
	, m_selectTimout( 0.0f )
	, m_activationTimeout( 0.f )
	, m_delayOnFailure( def.m_delayOnFailure.GetVal( context ) )
	, m_delayOnSuccess( def.m_delayOnSuccess.GetVal( context ) )
	, m_delayOnInterruption( def.m_delayOnInterruption.GetVal( context ) )
{
	CActionPointSelector* actionPointSelector = context.GetVal< CActionPointSelector* >( CNAME( actionPointSelector ), NULL );
	if ( actionPointSelector )
	{
		m_apSelector = actionPointSelector->SpawnInstance( context );
	}
	if ( !m_apSelector )
	{
		// use community action point selector by default
		m_apSelector = new CCommunityActionPointSelectorInstance();
	}

	m_doReserveSelectedAP = !m_apSelector->IsExternallyReservingAP();
	
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( ResetLastAp );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_DelayWork );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}

void CBehTreeNodeSelectActionPointDecoratorInstance::OnDestruction() 
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( ResetLastAp );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_DelayWork );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

CBehTreeNodeSelectActionPointDecoratorInstance::~CBehTreeNodeSelectActionPointDecoratorInstance()
{
	if ( m_apSelector )
	{
		delete m_apSelector;
	}
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( ResetLastAp ) )
	{
		m_workData->ResetLastAp();
		return true;
	}
	else if ( e.m_eventName == CNAME( AI_DelayWork ) )
	{
		SGameplayEventParamFloat* param = e.m_gameplayEventData.Get< SGameplayEventParamFloat >();
		Float delay =  param ? param->m_value : 1.f;

		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + delay );
	}
	return false;
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::CheckCondition()
{
	if ( m_isActive )
	{
		return true;
	}

	if ( m_activationTimeout > m_owner->GetLocalTime() )
	{
		return false;
	}

	if ( !m_owner->GetNPC() )
	{
		return false;
	}

	if ( !DoSelection() )
	{
		m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnFailure );
		return false;
	}

	return true;
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::IsAvailable()
{
	if ( !CheckCondition() )
	{
		DebugNotifyAvailableFail();
		return false;
	}
	return true;
}

Int32 CBehTreeNodeSelectActionPointDecoratorInstance::Evaluate()
{
	if ( !CheckCondition() )
	{
		DebugNotifyAvailableFail();
		return -1;
	}
	return m_priority;
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::Activate()
{
	// just one test - on activation
	CNewNPC* npc = m_owner->GetNPC();
	if ( !npc )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( !DoSelection() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();

	if ( m_doReserveSelectedAP && !actionPointManager->IsFree( m_chosenAP, npc ) )
	{
		// someone just stole our AP
		m_chosenAP = ActionPointBadID;
		m_chosenCategory = CName::NONE;
		DebugNotifyActivationFail();
		return false;
	}

	SetActiveActionPoint( m_chosenAP, m_chosenCategory );
	m_apSelector->OnNodeActivated();
	
	if ( !Super::Activate() )
	{
		m_apSelector->OnNodeDeactivated();
		UnlockActionPoint();
		// we failed 
		DebugNotifyActivationFail();
		return false;
	}

	m_workData->StartPerform();

	return true;
}

void CBehTreeNodeSelectActionPointDecoratorInstance::Deactivate()
{
	if( m_workData->IsBeingInterrupted() )
	{
		ReserveSelectedAP();
	}
	m_workData->StopPerform();
	UnlockActionPoint();
	
	Super::Deactivate();
}
Bool CBehTreeNodeSelectActionPointDecoratorInstance::Interrupt()
{
	m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + m_delayOnInterruption );
	m_workData->Interrupt();	
	return Super::Interrupt();
}

void CBehTreeNodeSelectActionPointDecoratorInstance::ReserveSelectedAP()
{
	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	if ( communitySystem )
	{
		CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();
		actionPointManager->SetReserved( m_chosenAP, CActionPointManager::REASON_RESERVATION, m_owner->GetNPC(), AP_RESERVATION_TIME );
	}
	
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::DoSelection()
{

	CBehTreeWorkData& workData = *m_workData;

	if ( m_chosenAP != ActionPointBadID )
	{
		return true;
	}

	// SelectNewActionPoint is quite heavy so we make sure it is not called too  often
	Float localTime = m_owner->GetLocalTime();
	if ( m_selectTimout > localTime )
	{
		return false;
	}

	m_selectTimout = localTime + GEngine->GetRandomNumberGenerator().Get< Float >( 1.5f , 2.0f );

	if ( workData.IsTryingToSpawnToWork( m_owner ) )
	{
		if ( SelectNewActionPoint( workData.GetLastAP(), m_chosenAP, m_chosenCategory, workData.GetSpawnToWorkAPDistance() ) )
		{
			return true;
		}
		workData.CancelSpawnToWork();
	}

	return SelectNewActionPoint( workData.GetLastAP(), m_chosenAP, m_chosenCategory );
}




Bool CBehTreeNodeSelectActionPointDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( WorkDone ) )
	{
		SActionPointId* apId = e.m_gameplayEventData.Get< SActionPointId >();
		if ( apId != NULL )
		{
			CName workCategory;
			SelectNewActionPoint( m_workData->GetSelectedAP(), *apId, workCategory );
			SetActiveActionPoint( *apId, workCategory ); 
		}
		return false;
	}
	else if( e.m_eventName == CNAME( OnTimetableChanged ) )
	{
		CBehTreeWorkData& workData = *m_workData;
		SelectNewActionPoint( workData.GetLastAP(), m_chosenAP, m_chosenCategory, workData.GetSpawnToWorkAPDistance() );				
		if ( !m_child->OnEvent( e ))
		{
			m_selectTimout = 0.0f;
			Complete( BTTO_SUCCESS );
		}
		return false;
	}
	return Super::OnEvent( e );
}

void CBehTreeNodeSelectActionPointDecoratorInstance::Complete( eTaskOutcome outcome )
{
	Float delay = ( outcome == BTTO_FAILED ) ? m_delayOnFailure : m_delayOnSuccess;
	m_activationTimeout = Max( m_activationTimeout, m_owner->GetLocalTime() + delay * GEngine->GetRandomNumberGenerator().Get< Float >( 0.75f, 1.25f ) );
	
	Super::Complete( outcome );
}

#ifdef DEBUG_ACTION_POINT_RESERVATION
void CBehTreeNodeSelectActionPointDecoratorInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	const SActionPointId& apID = m_workData->GetReservedAP();
	if ( apID != ActionPointBadID )
	{
		String text = String::Printf( TXT("Reserved AP [%08X]"), apID.CalcHash() );
		frame->AddDebugText( m_owner->GetActor()->GetWorldPositionRef(), text, -180, -6 );
	}
	else
	{
		frame->AddDebugText( m_owner->GetActor()->GetWorldPositionRef(), TXT("No reserved"), -180, -6 );
	}
}
#endif

void CBehTreeNodeSelectActionPointDecoratorInstance::OnSubgoalCompleted( eTaskOutcome outcome )
{
	if ( outcome == BTTO_FAILED || m_workData->IsBeingInterrupted() )
	{
		Super::OnSubgoalCompleted( BTTO_FAILED );
		return;
	}
	if ( m_workData->GetSelectedAP() == ActionPointBadID || !m_child->Activate() )
	{
		Complete( BTTO_FAILED );
	}
	//SActionPointId newActionPoint;
	//CName workCategory;
	//SelectNewActionPoint( m_workData->GetLastAP(), newActionPoint, workCategory );
	//SetActiveActionPoint( newActionPoint, workCategory );
	//if ( newActionPoint == ActionPointBadID || !m_child->Activate() )
	//{
	//	Complete( BTTO_FAILED );
	//}
}

Bool CBehTreeNodeSelectActionPointDecoratorInstance::SelectNewActionPoint( const SActionPointId& lastActionPoint, SActionPointId& newActionPoint, CName& workCategory, Float forceRadius ) const
{
	CNewNPC* npc = m_owner->GetNPC();
	
	if( !npc )
	{
		return false;
	}

	CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	CActionPointManager* actionPointManager = communitySystem->GetActionPointManager();

	CBehTreeWorkData& workData = *m_workData;

	if ( actionPointManager->HasPreferredNextAPs( workData.GetLastAP() ) == false )
	{
		if ( m_apSelector )
		{
			SActionPointId lastApId = lastActionPoint == ActionPointBadID ? workData.GetLastAP() : lastActionPoint;
			Bool loopInAP = false;
			m_apSelector->SelectActionPoint( m_owner, m_workData, newActionPoint, lastApId, workCategory, loopInAP, forceRadius );
			workData.m_hasLoopingSequence = loopInAP;
		}
	}
	else
	{
		newActionPoint = actionPointManager->FindNextFreeActionPointInSequence( workData.GetLastAP() );
	}

	return newActionPoint != ActionPointBadID;
}

void CBehTreeNodeSelectActionPointDecoratorInstance::SetActiveActionPoint( const SActionPointId& actionPoint, const CName& category )
{
	m_workData->SelectAP( actionPoint, category, m_doReserveSelectedAP );
}

void CBehTreeNodeSelectActionPointDecoratorInstance::UnlockActionPoint()
{
	m_workData->ClearAP( m_doReserveSelectedAP );
	m_chosenAP = ActionPointBadID;
	m_chosenCategory = CName::NONE;
}
