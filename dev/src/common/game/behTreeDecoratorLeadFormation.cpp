/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorLeadFormation.h"

#include "../engine/renderFrame.h"

#include "behTreeNodeAtomicAction.h"
#include "behTreeInstance.h"
#include "formation.h"
#include "formationMemberDataSlot.h"
#include "movementGoal.h"
#include "moveSteeringLocomotionSegment.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorLeadFormationDefinition )

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLeadFormationInstance
////////////////////////////////////////////////////////////////////////


CBehTreeNodeDecoratorLeadFormationInstance::CBehTreeNodeDecoratorLeadFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_formation( def.m_formation.GetVal( context ) )
	, m_delayedRearrange( 0.f )
	, m_reshapeOnMoveAction( def.m_reshapeOnMoveAction.GetVal( context ) )
	, m_isFormationPaused( false )
{
	m_steeringInput.m_memberData = nullptr;
	if ( m_formation )
	{
		m_owner->AddDependentObject( m_formation );
	}
}

void CBehTreeNodeDecoratorLeadFormationInstance::OnDestruction()
{
	if ( m_formation )
	{
		m_owner->RemoveDependentObject( m_formation );
	}

	Super::OnDestruction(); 
}

Bool CBehTreeNodeDecoratorLeadFormationInstance::Activate()
{
	CActor* actor = m_owner->GetActor();
	if ( !m_leaderDataPtr )
	{
		
		if ( !m_formation || !m_leaderDataPtr.Setup( m_owner, m_formation, actor ) )
		{
			DebugNotifyActivationFail();
			return false;
		}
	}

	m_steeringInput.m_leaderData = m_leaderDataPtr.Get();

	CMovingAgentComponent* movingAgentComponent	= actor->GetMovingAgentComponent();
	if ( !movingAgentComponent )
	{
		DebugNotifyActivationFail();
		return false;
	}
	CMoveLocomotion* locomotion					= movingAgentComponent->GetLocomotion();
	if ( !locomotion )
	{
		DebugNotifyActivationFail();
		return false;
	}

	locomotion->AddTargeter_MoveLSSteering( this );
	locomotion->AddMoveLocomotionListener( this );			// if segment is updated we must update targeter as well
	
	return Super::Activate();
}

void CBehTreeNodeDecoratorLeadFormationInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	if ( actor )
	{
		CMovingAgentComponent *const movingAgentComponent = actor->GetMovingAgentComponent();
		if ( movingAgentComponent )
		{
			CMoveLocomotion *const locomotion = movingAgentComponent->GetLocomotion();
			if ( locomotion )
			{
				locomotion->RemoveTargeter_MoveLSSteering( this );
				locomotion->RemoveMoveLocomotionListener( this );
			}
		}
	}

	return Super::Deactivate();
}

void CBehTreeNodeDecoratorLeadFormationInstance::Update()
{
	if ( m_delayedRearrange > 0.f && m_delayedRearrange < m_owner->GetLocalTime() )
	{
		m_leaderDataPtr->Reorganize();
		m_delayedRearrange = 0.f;
	}
	if ( m_delayedResume > 0.f && m_delayedResume < m_owner->GetLocalTime() )
	{
		m_leaderDataPtr->ResumeFormation();
		m_delayedResume = 0.f;
		m_isFormationPaused = true;
	}

	Super::Update();
}

Bool CBehTreeNodeDecoratorLeadFormationInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CBehTreeNodeAtomicActionInstance::MovementStartedEventName() && m_reshapeOnMoveAction )
	{
		Float resumeTime = m_owner->GetLocalTime() + 2.f;
		m_delayedRearrange = resumeTime;
		m_delayedResume = resumeTime;
		m_leaderDataPtr->PauseFormation();
		m_isFormationPaused = true;
	}
	return Super::OnEvent( e );
}

void CBehTreeNodeDecoratorLeadFormationInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CAISlotFormationLeaderData* slotData = m_leaderDataPtr->AsSlotFormationLeaderData();
	String text = String::Printf( TXT("Leader members: %d"), m_leaderDataPtr->GetMemberList().Size() );
	if ( slotData )
	{
		text += String::Printf( TXT(" break ratio: %0.2f"), slotData->GetCurrentBreakRatio() );
	}
	frame->AddDebugText( m_owner->GetActor()->GetWorldPositionRef(), text, -30, -5, true, Color::CYAN );
	
}

void CBehTreeNodeDecoratorLeadFormationInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( false );

	SFormationSteeringInput::SetGeneralFormationData( goal, &m_steeringInput );
}
Bool CBehTreeNodeDecoratorLeadFormationInstance::IsFinished() const
{
	return false;
}

// IMoveLocomotionListener
void CBehTreeNodeDecoratorLeadFormationInstance::OnSegmentPushed( IMoveLocomotionSegment* segment )
{
	// if segment is updated we must update targeter as well
	CMoveLSSteering *const moveLSSteering = segment->AsCMoveLSSteering();
	if ( moveLSSteering )
	{
		moveLSSteering->AddTargeter( this );
	}
}




