/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiActionInteraction.h"
#include "r6behTreeInstance.h"
#include "r6InteractionComponent.h"

#include "../../common/engine/behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CAIActionInteraction )

CAIActionInteraction::CAIActionInteraction()
	: m_component( nullptr )
	, m_interruptionRequested( false ) 
{	
}

Bool CAIActionInteraction::CanBeStartedOn( CComponent* component ) const
{
	// component have to be a moving agent 
	CMovingAgentComponent* mac = Cast< CMovingAgentComponent > ( component );
	if ( nullptr == mac )
	{
		return false;
	}

	// we need a behavior stack
	if ( nullptr == mac->GetBehaviorStack() )
	{
		return false;
	}

	// component needs to be enabled
	if ( false == mac->IsMotionEnabled() )
	{
		return false;
	}

	CR6InteractionComponent* interaction = GetCurrentInteractionComponent();
	if ( nullptr == interaction )
	{
		return false;
	}

	// check if interacion can be used with out component
	if ( false == interaction->IsUsableFor( mac ) )
	{
		return false;
	}

	// get the interacion point 
	Vector position = interaction->GetInteractLocationForNode( mac );
	EulerAngles rotation = interaction->GetInteractRotationForNode( mac );
	const Float posTol = interaction->GetPositionTolerance();
	const Float rotTol = interaction->GetRotationTolerance();

	// check if our component's position is good
	if ( false == CheckInteractPosition( position, mac->GetWorldPositionRef(), posTol ) )
	{
		return false;
	}

	// check if our component's rotation is good
	if ( false == CheckInteractRotation( rotation.Yaw, mac->GetWorldRotation().Yaw, rotTol ) )
	{
		return false;
	}

	return true;
}

EAIActionStatus CAIActionInteraction::StartOn( CComponent* component )
{
	R6_ASSERT( m_status != ACTION_InProgress );
	R6_ASSERT( component && component->IsA< CMovingAgentComponent > () );

	// at this point we are sure this IS a moving agent component, AND it is valid, so no need to use Cast<>()
	m_component = static_cast< CMovingAgentComponent* > ( component );
	
	m_interaction = GetCurrentInteractionComponent();
	R6_ASSERT( m_interaction.Get() );

	if ( false == m_interaction->Use( component ) )
	{
		CNode* targetNode = FindActionTarget();
		R6_ASSERT( targetNode );
		SetErrorState( TXT("Can't use interaction %s with %s"), targetNode->GetFriendlyName().AsChar(), component->GetFriendlyName().AsChar() );
		return ACTION_Failed;
	}

	return TBaseClass::StartOn( component );
}

EAIActionStatus CAIActionInteraction::Tick( Float timeDelta )
{
	CR6InteractionComponent* interaction = m_interaction.Get();
	if ( nullptr == interaction )	
	{
		return Cancel( TXT("Interaction despawned.") );
	}
	
	if ( false == interaction->Update( timeDelta ) || false == interaction->IsUsedBy( m_component.Get() ) )
	{
		// action have finished
		return Stop( ACTION_Successful );
	}

	return TBaseClass::Tick( timeDelta );
}


EAIActionStatus CAIActionInteraction::Stop( EAIActionStatus newStatus )
{
	if ( newStatus != ACTION_Successful )
	{
		CR6InteractionComponent* interaction = m_interaction.Get();
		if ( interaction )	
		{
			interaction->Abort( m_component.Get() );
		}
	}

	return TBaseClass::Stop( newStatus );	
}

EAIActionStatus CAIActionInteraction::Reset()
{
	m_component = nullptr;
	m_interaction = nullptr;
	m_interruptionRequested = false;
	return TBaseClass::Reset();
}

void CAIActionInteraction::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode* sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
{
	if ( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted )
	{
		TBaseClass::Stop( ACTION_Successful );
	}
	else
	{
		TBaseClass::Stop( ACTION_Failed );
	}
}

RED_INLINE Bool CAIActionInteraction::CheckInteractPosition( const Vector& pos1, const Vector& pos2, Float tol ) const
{
	return pos1.DistanceSquaredTo( pos2 ) <= tol * tol;
}

RED_INLINE Bool CAIActionInteraction::CheckInteractRotation( Float rot1, Float rot2, Float tol ) const
{
	return MAbs( EulerAngles::AngleDistance( rot1, rot2 ) ) <= tol;
}

EAIActionStatus CAIActionInteraction::RequestInterruption()
{
	if ( false == m_interruptionRequested )
	{
		CR6InteractionComponent* interaction = m_interaction.Get();
		if ( nullptr == interaction )
		{
			return Cancel( TXT("interaction despawned") );
		}

		interaction->FinishUsing( m_component.Get() );
		m_interruptionRequested = true;
	}

	return m_status;
}

CR6InteractionComponent* CAIActionInteraction::GetCurrentInteractionComponent() const
{
	// targetNode is an interaction to perform
	CNode* targetNode = FindActionTarget();
	if ( nullptr == targetNode )
	{
		return nullptr;
	}

	// check if it's an interaction
	CR6InteractionComponent* interaction = CR6InteractionComponent::CheckNodeForInteraction( targetNode );
	return interaction;
}

