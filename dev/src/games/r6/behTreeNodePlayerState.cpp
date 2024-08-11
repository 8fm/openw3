/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodePlayerState.h"
#include "aiTreeComponent.h"
#include "..\..\common\game\movingPhysicalAgentComponent.h"
#include "../../common/core/dataError.h"
#include "../../common/engine/utils.h"

RED_DEFINE_STATIC_NAME( requestedFacingDirection );
RED_DEFINE_STATIC_NAME( requestedMovementDirection );
RED_DEFINE_STATIC_NAME( playerControlled );

Vector2 InputHelpers::RawInputToWorldSpace( const CCameraDirector* const camDirector, Vector2 rawInput )
{
	// Calculate movement in camera space
	const Vector camSpace = camDirector->GetCameraRight() * rawInput.X + camDirector->GetCameraForward() * rawInput.Y;
		
	// Nolmalize
	return camSpace.AsVector2().Normalized();
}


void InputHelpers::VecReduce2( Vector& vec, Float amount )
{
	const Float len = vec.Mag2();
	if ( len > 0.f )
	{
		Float coef = ( max( 0.f, len - amount ) / len );
		vec.X *= coef;
		vec.Y *= coef;
	}
}

void InputHelpers::VecReduce3( Vector& vec, Float amount )
{
	const Float len = vec.Mag3();
	if ( len > 0.f )
	{
		vec *= ( max( 0.f, len - amount ) / len );
	}
}


void InputHelpers::VecAddLimited2( Vector& vec, const Vector& add, Float maxLen )
{
	vec += add;
	const Float newLen = vec.Mag2();
	if ( newLen > maxLen )
	{
		Float coef = ( maxLen / newLen );
		vec.X *= coef;
		vec.Y *= coef;
	}
}

void InputHelpers::VecAddLimited3( Vector& vec, const Vector& add, Float maxLen )
{
	vec += add;
	const Float newLen = vec.Mag3();
	if ( newLen > maxLen )
	{
		vec *= ( maxLen / newLen );
	}
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( SPlayerMovementData )	 

SPlayerMovementData::SPlayerMovementData()
	: m_velocity( Vector::ZEROS )
	, m_requestedFacingDirection( 0.f )
	, m_requestedMovementDirection( 0.f )
	, m_playerLocomotionController( NULL )
{
	m_playerLocomotionController = new CPlayerLocomotionController;
}


SPlayerMovementData::~SPlayerMovementData()
{
	delete m_playerLocomotionController;
}



void SPlayerMovementData::SetBehaviorVariables( CBehaviorGraphStack* stack ) const
{
	R6_ASSERT( stack );

	stack->SetBehaviorVariable( CNAME( Speed ), m_velocity.Mag3() );
	stack->SetBehaviorVariable( CNAME( requestedFacingDirection ), m_requestedFacingDirection ); 
	stack->SetBehaviorVariable( CNAME( requestedMovementDirection ), m_requestedMovementDirection );	
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IBehTreeNodePlayerStateInstance::IBehTreeNodePlayerStateInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )	
	, m_stateName( def.GetNodeName() )
	, m_component( nullptr )
	, m_movementDataPtr( SPlayerMovementData::CInitializer(), owner )
{
	CAITreeComponent* ai = owner->FindParent< CAITreeComponent > ();
	R6_ASSERT( ai );

	// register as a listener to locomotion events
	// this is here to fill the m_movementSegment when it is changed on a character
	const TDynArray< CComponent* >& performers = ai->GetPerformers();
	for ( Uint32 i = 0; i < performers.Size(); ++i )
	{
		CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent > ( performers[ i ] );
		if ( mac )
		{
			m_component = mac;
		}
	}

	if ( !m_component.Get() )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( GetOwner()->GetActor() ), TXT( "Behaviour Tree" ), TXT( "Player AI BehTreeNodes requires CMovingPhysicalAgentComponent to work properly." ) );
	}
}

IBehTreeNodePlayerStateInstance::~IBehTreeNodePlayerStateInstance()
{
}


Bool IBehTreeNodePlayerStateInstance::Activate()
{
	CMovingAgentComponent* mac = m_component.Get();
	if ( nullptr == mac || false == mac->IsMotionEnabled() || nullptr == mac->GetBehaviorStack() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	// bg transition
	mac->GetBehaviorStack()->GenerateBehaviorEvent( m_stateName );
	mac->GetBehaviorStack()->SetBehaviorVariable( CNAME( playerControlled ), 1.0f );

	auto plc = GetPLC();
	R6_ASSERT( plc );
	if( !( plc->IsAttached() ) )
	{
		mac->AttachLocomotionController( *plc );
		R6_ASSERT( plc->IsAttached() );
	}

	return Super::Activate();
}

