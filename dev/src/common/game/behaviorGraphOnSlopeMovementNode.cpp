/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphOnSlopeMovementNode.h"
#include "../engine/behaviorGraphBlendNode.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphValueNode.h"
#include "../engine/cacheBehaviorGraphOutput.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphOnSlopeMovementNode );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( FacingDirectionWS );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphOnSlopeMovementNode::CBehaviorGraphOnSlopeMovementNode()
	: m_slopeBlendTime( 0.3f ) // slow adjustment only
	, m_slopeMaxBlendSpeed( 50.0f )
	, m_neverReachBorderValues( true )
{
	m_angles.PushBack( -45.0f );
	m_angles.PushBack( 0.0f );
	m_angles.PushBack( 45.0f );
}

void CBehaviorGraphOnSlopeMovementNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_prevLocalToWorld;
	compiler << i_slopeAngle;
	compiler << i_blendValue;
}

void CBehaviorGraphOnSlopeMovementNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_slopeAngle ] = 0.0f;
	instance[ i_blendValue ] = 0.0f;
}

void CBehaviorGraphOnSlopeMovementNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_prevLocalToWorld );
	INST_PROP( i_slopeAngle );
	INST_PROP( i_blendValue );
}

Float CBehaviorGraphOnSlopeMovementNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_blendValue ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphOnSlopeMovementNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );
}

void CBehaviorGraphOnSlopeMovementNode::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	MakeSureAnglesAreValid();
}

#endif

void CBehaviorGraphOnSlopeMovementNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	MakeSureAnglesAreValid();
}

void CBehaviorGraphOnSlopeMovementNode::MakeSureAnglesAreValid()
{
	Float* pAngle = m_angles.TypedData();
	Float* pNextAngle = m_angles.TypedData() + 1;
	const Int32 angleCountMinusOne = m_angles.SizeInt() - 1;
	for ( Int32 i = 0; i < angleCountMinusOne; ++ i, ++ pAngle, ++ pNextAngle )
	{
		// minimal angle difference is 1 deg
		*pNextAngle = Max( *pAngle + 1.0f, *pNextAngle );
	}
}

void CBehaviorGraphOnSlopeMovementNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	CalculateBlendValue( instance, timeDelta );
	instance[ i_prevLocalToWorld ] = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();
}

Float CBehaviorGraphOnSlopeMovementNode::GetSlopeAngle( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Float velocityXY = 0.0f;
	Float velocityZ = 0.0f;
	Float velocity = 0.0f;
	if ( timeDelta > 0.0f )
	{
		Matrix localToWorld = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();
		Vector translation = localToWorld.GetTranslation() - instance[ i_prevLocalToWorld ].GetTranslation();
		Vector velocityFromTranslation = translation / timeDelta;
		velocity = velocityFromTranslation.Mag3();
		velocityXY = velocityFromTranslation.Mag2();
		velocityZ = velocityFromTranslation.Z;
	}
	else if ( CMovingAgentComponent const * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
	{
		velocity = mac->GetVelocity().Mag3();
		velocityXY = mac->GetVelocity().Mag2();
		velocityZ = mac->GetVelocity().Z;
	}

	if ( velocityXY > 0.1f )
	{
		return RAD2DEG( MAsin_safe( velocityZ / velocity ) );
	}
	else
	{
		return 0.0f;
	}
}

void CBehaviorGraphOnSlopeMovementNode::CalculateBlendValue( CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Float newSlopeAngle = GetSlopeAngle( instance, timeDelta );
	Float & slopeAngle = instance[ i_slopeAngle ];
	Float outputBlend = instance[ i_blendValue ];
	slopeAngle = BlendToWithBlendTimeAndSpeed( slopeAngle, newSlopeAngle, m_slopeBlendTime, m_slopeMaxBlendSpeed, timeDelta );
	const Float* pAngle = m_angles.TypedData();
	const Float* pNextAngle = m_angles.TypedData() + 1;
	Float angleFloatIdx = 0.0f;
	const Int32 angleCountMinusOne = m_angles.SizeInt() - 1;
	for ( Int32 i = 0; i < angleCountMinusOne; ++ i, ++ pAngle, ++ pNextAngle, angleFloatIdx += 1.0f )
	{
		if ( slopeAngle >= *pAngle && slopeAngle <= *pNextAngle )
		{
			ASSERT( (*pNextAngle - *pAngle) != 0.0f, TXT("Angles should not be equal") );
			outputBlend = angleFloatIdx + Clamp( ( slopeAngle - *pAngle ) / ( *pNextAngle - *pAngle ), 0.0f, 1.0f );
		}
	}
	if ( slopeAngle >= *pAngle )
	{
		outputBlend = angleFloatIdx;
	}
	if ( m_neverReachBorderValues )
	{
		instance[ i_blendValue ] = Clamp( outputBlend, 0.01f, angleFloatIdx - 0.01f );
	}
	else
	{
		instance[ i_blendValue ] = outputBlend;
	}
}

void CBehaviorGraphOnSlopeMovementNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
}

Bool CBehaviorGraphOnSlopeMovementNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

Bool CBehaviorGraphOnSlopeMovementNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

void CBehaviorGraphOnSlopeMovementNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_prevLocalToWorld ] = instance.GetAnimatedComponent()->GetEntity()->GetLocalToWorld();
	instance[ i_slopeAngle ] = GetSlopeAngle( instance, 0.0f );

	CalculateBlendValue( instance, 0.0f );
}

void CBehaviorGraphOnSlopeMovementNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
}

void CBehaviorGraphOnSlopeMovementNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
