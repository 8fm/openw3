/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiActionPlayAnimation.h"

#include "../../common/engine/behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CAIActionPlayAnimation )

CAIActionPlayAnimation::CAIActionPlayAnimation()
	: m_slotName( CNAME( NPC_ANIM_SLOT ) )
	, m_animationName( CName::NONE )
	, m_blendIn( 0.2f )
	, m_blendOut( 0.2f )
	, m_speed( 1.f )		
	, m_offset( 0.f )		
	, m_looped( false )
	, m_useMotionExtraction( false )		
{	
}

Bool CAIActionPlayAnimation::CanBeStartedOn( CComponent* component ) const
{
	CAnimatedComponent* ac = Cast< CAnimatedComponent > ( component );
	if ( nullptr == ac )
	{
		return false;
	}

	if ( nullptr == ac->GetBehaviorStack() )
	{
		return false;
	}

	if ( !m_slotName || !m_animationName )
	{
		return false;
	}

	return true;
}

EAIActionStatus CAIActionPlayAnimation::StartOn( CComponent* component )
{
	R6_ASSERT( m_status != ACTION_InProgress );
	R6_ASSERT( component && component->IsA< CAnimatedComponent > () );

	// at this point we are sure this IS an animated component, AND it is valid, so no need to use Cast<>()
	m_component = static_cast< CAnimatedComponent* > ( component );
	
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = m_blendIn;
	slotSetup.m_blendOut = m_blendOut;
	slotSetup.m_speedMul = m_speed;
	slotSetup.m_offset = m_offset;	
	slotSetup.m_looped = m_looped;
	slotSetup.m_motionEx = m_useMotionExtraction;
	slotSetup.m_listener = this;

	// Play animation
	if ( false == m_component.Get()->GetBehaviorStack()->PlaySlotAnimation( m_slotName, m_animationName, &slotSetup ) )
	{
		SetErrorState( TXT("Unable to play animation %s on slot %s on %s"), m_animationName.AsChar(), m_slotName.AsChar(), m_component->GetFriendlyName().AsChar() );
		return ACTION_Failed;
	}

	return TBaseClass::StartOn( component );
}

EAIActionStatus CAIActionPlayAnimation::Stop( EAIActionStatus newStatus )
{
	CAnimatedComponent* component = m_component.Get();
	if ( component && component->GetBehaviorStack() )
	{
		component->GetBehaviorStack()->StopSlotAnimation( m_slotName );
	}

	return TBaseClass::Stop( newStatus );	
}

EAIActionStatus CAIActionPlayAnimation::Reset()
{
	m_component = nullptr;
	return TBaseClass::Reset();
}

void CAIActionPlayAnimation::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode* sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
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