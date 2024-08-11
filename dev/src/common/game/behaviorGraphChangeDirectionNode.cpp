/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphChangeDirectionNode.h"
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

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphChangeDirectionNode );
IMPLEMENT_RTTI_ENUM( EChangeDirectionSide );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( FacingDirectionWS );
RED_DEFINE_STATIC_NAME( requestedFacingDirection );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphChangeDirectionNode::CBehaviorGraphChangeDirectionNode()
	: m_updateOnlyOnActivation( false )
	, m_dirBlendTime( 0.4f ) // slow adjustment only
	, m_dirMaxBlendSpeed( 50.0f )
	, m_overshootAngle( 30.0f )
	, m_requestedFacingDirectionWSVariableName( CNAME( requestedFacingDirection ) )
	, m_anyDirection( true )
{
	m_angles.PushBack( 45.0f ); // TODO - do not allocate array inside constructor. it will be override during serialization - use callback from editor when node is created first time
	m_angles.PushBack( 180.0f );
}

void CBehaviorGraphChangeDirectionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_startingDirectionWS;
	compiler << i_facingDirection;
	compiler << i_blendValue;
}

void CBehaviorGraphChangeDirectionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_startingDirectionWS ] = 0.0f;
	instance[ i_facingDirection ] = 0.0f;
	instance[ i_blendValue ] = 0.0f;
}

void CBehaviorGraphChangeDirectionNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_startingDirectionWS );
	INST_PROP( i_facingDirection );
	INST_PROP( i_blendValue );
}

Float CBehaviorGraphChangeDirectionNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_blendValue ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphChangeDirectionNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( FacingDirectionWS ), false ) );
}

void CBehaviorGraphChangeDirectionNode::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	MakeSureAnglesAreValid();
}

String CBehaviorGraphChangeDirectionNode::GetCaption() const 
{
	if ( m_angles.Size() > 0 )
	{
		return String::Printf( TXT("Blend direction [%1.2f - %1.2f]"), m_angles[ 0 ], m_angles.Back() );
	}
	else
	{
		return TXT("Blend direction"); 
	}
}

#endif

void CBehaviorGraphChangeDirectionNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Get control variables
	m_cachedRequestedFacingDirectionWSValueNode = CacheValueBlock( TXT("FacingDirectionWS") );

	MakeSureAnglesAreValid();
}

void CBehaviorGraphChangeDirectionNode::MakeSureAnglesAreValid()
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

void CBehaviorGraphChangeDirectionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	if ( m_updateOnlyOnActivation )
	{
		return;
	}

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Update( context, instance, timeDelta );
	}
	
	Float& facingDirection = instance[ i_facingDirection ];
	Float requestedFacingDirection = GetRequestedFacingDirection( instance );

	facingDirection = BlendToWithBlendTimeAndSpeed( facingDirection, requestedFacingDirection, m_dirBlendTime, m_dirMaxBlendSpeed, timeDelta );

	CalculateBlendValue( instance );
}

Float CBehaviorGraphChangeDirectionNode::GetRequestedFacingDirection( CBehaviorGraphInstance& instance ) const
{
	Float startingYaw = instance[ i_startingDirectionWS ];
	Float destYawWS( 0.f );

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		destYawWS = m_cachedRequestedFacingDirectionWSValueNode->GetValue( instance );
	}
	else if ( const Float* value = instance.GetFloatValuePtr( m_requestedFacingDirectionWSVariableName ) )
	{
		destYawWS = *value;
	}

	const Float angleDiff = EulerAngles::NormalizeAngle180( EulerAngles::AngleDistance( startingYaw, destYawWS ) );

	return angleDiff;
}

void CBehaviorGraphChangeDirectionNode::CalculateBlendValue( CBehaviorGraphInstance& instance ) const
{
	Float outputBlend = 0.0f;
	if ( m_angles.Size() >= 2 )
	{
		// Note: This code doesn't work with m_anghles greater then 180
		Float facingDirection = instance[ i_facingDirection ];
		ASSERT( facingDirection <= 180.f && facingDirection >= -180.f );
		facingDirection += ( MSign( facingDirection ) * m_overshootAngle );
		if ( m_anyDirection )
		{
			facingDirection = MAbs( facingDirection );
		}

		const Float* pAngle = m_angles.TypedData();
		const Float* pNextAngle = m_angles.TypedData() + 1;
		Float angleFloatIdx = 0.0f;
		const Int32 angleCountMinusOne = m_angles.SizeInt() - 1;
		for ( Int32 i = 0; i < angleCountMinusOne; ++i, ++pAngle, ++pNextAngle, angleFloatIdx += 1.0f )
		{
			if ( facingDirection >= *pAngle && facingDirection <= *pNextAngle )
			{
				ASSERT( (*pNextAngle - *pAngle) != 0.0f, TXT("Angles should not be equal") );
				outputBlend = angleFloatIdx + Clamp( ( facingDirection - *pAngle ) / ( *pNextAngle - *pAngle ), 0.0f, 1.0f );
			}
		}
		if ( facingDirection >= *pAngle )
		{
			outputBlend = angleFloatIdx;
		}
	}
	instance[ i_blendValue ] = outputBlend;
}

void CBehaviorGraphChangeDirectionNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphChangeDirectionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		retVal = m_cachedRequestedFacingDirectionWSValueNode->ProcessEvent( instance, event ) || retVal;
	}

	return retVal;
}

Bool CBehaviorGraphChangeDirectionNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		retVal = m_cachedRequestedFacingDirectionWSValueNode->ProcessForceEvent( instance, event ) || retVal;
	}

	return retVal;
}

void CBehaviorGraphChangeDirectionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	// activate variables
	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Activate( instance );
	}

	// store starting direction
	const Float currentYaw = instance.GetAnimatedComponent()->GetWorldYaw();
	instance[ i_startingDirectionWS ] = currentYaw;

	Float& facingDirection = instance[ i_facingDirection ];
	facingDirection = GetRequestedFacingDirection( instance );

	CalculateBlendValue( instance );
}

void CBehaviorGraphChangeDirectionNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphChangeDirectionNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedRequestedFacingDirectionWSValueNode )
	{
		m_cachedRequestedFacingDirectionWSValueNode->ProcessActivationAlpha( instance, alpha );
	}

}

void CBehaviorGraphChangeDirectionNode::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	TBaseClass::GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
	var.PushBack( m_requestedFacingDirectionWSVariableName );
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
