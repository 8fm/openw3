/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphCharacterRotationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCharacterRotationNode );

CBehaviorGraphCharacterRotationNode::CBehaviorGraphCharacterRotationNode()
	: m_axis( 0.0f, 0.0f, 1.0f, 1.0f )
	, m_rotationSpeedMultiplier( 1.0f )
{
}

void CBehaviorGraphCharacterRotationNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_rotationDelta;
	compiler << i_biasValue;
	compiler << i_lastBiasValue;
}

void CBehaviorGraphCharacterRotationNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_rotationDelta ] = 0.f;
	instance[ i_biasValue ] = 0.f;
	instance[ i_lastBiasValue ] = 0.f;
}

void CBehaviorGraphCharacterRotationNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_rotationDelta );
	INST_PROP( i_biasValue );
	INST_PROP( i_lastBiasValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphCharacterRotationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angle ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Bias ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MaxAngle ) ) );
}
#endif

void CBehaviorGraphCharacterRotationNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( CharRot );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& rotationDelta = instance[ i_rotationDelta ];
	rotationDelta = 0.0f;

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );

		Float rotationSpeed = m_cachedControlVariableNode->GetValue( instance ) * m_rotationSpeedMultiplier;
		ASSERT( !Red::Math::NumericalUtils::IsNan( rotationSpeed ) );

		rotationDelta = timeDelta * DEG2RAD( rotationSpeed );
	}

	if ( m_cachedAngleVariableNode )
	{	
		m_cachedAngleVariableNode->Update( context, instance, timeDelta );
		
		rotationDelta = DEG2RAD( m_cachedAngleVariableNode->GetValue( instance ) );
		ASSERT( !Red::Math::NumericalUtils::IsNan( rotationDelta ) );
	}

	if ( m_cachedMaxAngleVariableNode )
	{
		m_cachedMaxAngleVariableNode->Update( context, instance, timeDelta );

		const Float maxAngle = DEG2RAD( MAbs( m_cachedMaxAngleVariableNode->GetValue( instance ) ) );

		rotationDelta = Clamp( rotationDelta, -maxAngle, maxAngle );
	}

	instance[ i_lastBiasValue ] = instance[ i_biasValue ];

	if ( m_cachedBiasVariableNode )
	{
		m_cachedBiasVariableNode->Update( context, instance, timeDelta );
		instance[ i_biasValue ] = m_cachedBiasVariableNode->GetValue( instance );
	}	
	else
	{
		instance[ i_biasValue ] = 0.0f;
	}
}

void CBehaviorGraphCharacterRotationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( CharRot );

	TBaseClass::Sample( context, instance, output );

	const Vector axis = m_axis.Normalized3();

	const Float rotDelta = instance[ i_rotationDelta ];
	const Float biasDelta = DEG2RAD( instance[ i_biasValue ] - instance[ i_lastBiasValue ] );
#ifdef USE_HAVOK_ANIMATION
	// apply rotation
	hkQuaternion rotationQuat( hkVector4( axis.X, axis.Y, axis.Z, axis.W ), rotDelta );

	hkQuaternion rotationBiasQuat( hkVector4( axis.X, axis.Y, axis.Z, axis.W ), biasDelta );

	hkQsTransform rotationBias( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ),
								rotationBiasQuat,
								hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	hkQsTransform rotation( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ),
							rotationQuat,
							hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( rotationBias.isOk() );
	ASSERT( rotation.isOk() );
	ASSERT( output.m_deltaReferenceFrameLocal.isOk() );

	output.m_deltaReferenceFrameLocal.setMul( rotationBias, output.m_deltaReferenceFrameLocal );
	output.m_deltaReferenceFrameLocal.setMul( output.m_deltaReferenceFrameLocal, rotation );

	ASSERT( output.m_deltaReferenceFrameLocal.isOk() );
#else
	// apply rotation
	RedQuaternion rotationQuat( RedVector4( axis.X, axis.Y, axis.Z, axis.W ), rotDelta );

	RedQuaternion rotationBiasQuat( RedVector4( axis.X, axis.Y, axis.Z, axis.W ), biasDelta );

	RedQsTransform rotationBias( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ),
								 rotationBiasQuat,
								 RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	RedQsTransform rotation( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ),
							 rotationQuat,
							 RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

	ASSERT( rotationBias.IsOk() );
	ASSERT( rotation.IsOk() );
	ASSERT( output.m_deltaReferenceFrameLocal.IsOk() );

	output.m_deltaReferenceFrameLocal.SetMul( rotationBias, output.m_deltaReferenceFrameLocal );
	output.m_deltaReferenceFrameLocal.SetMul( output.m_deltaReferenceFrameLocal, rotation );

	ASSERT( output.m_deltaReferenceFrameLocal.IsOk() );
#endif
	instance[ i_rotationDelta ] = 0.0f;
}

void CBehaviorGraphCharacterRotationNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedBiasVariableNode )
	{
		m_cachedBiasVariableNode->Activate( instance );
	}

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->Activate( instance );
	}

	if ( m_cachedMaxAngleVariableNode )
	{
		m_cachedMaxAngleVariableNode->Activate( instance );
	}
}

void CBehaviorGraphCharacterRotationNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedBiasVariableNode )
	{
		m_cachedBiasVariableNode->Deactivate( instance );
	}

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->Deactivate( instance );
	}

	if ( m_cachedMaxAngleVariableNode )
	{
		m_cachedMaxAngleVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphCharacterRotationNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Speed") );
	m_cachedBiasVariableNode = CacheValueBlock( TXT("Bias") );
	m_cachedAngleVariableNode = CacheValueBlock( TXT("Angle") );
	m_cachedMaxAngleVariableNode = CacheValueBlock( TXT("MaxAngle") );
}

void CBehaviorGraphCharacterRotationNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedBiasVariableNode )
	{
		m_cachedBiasVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedAngleVariableNode )
	{
		m_cachedAngleVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedMaxAngleVariableNode )
	{
		m_cachedMaxAngleVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphCharacterMotionToWSNode );

void CBehaviorGraphCharacterMotionToWSNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

#ifdef USE_HAVOK_ANIMATION
	Matrix w2l;
	instance.GetAnimatedComponent()->GetWorldToLocal( w2l );

	Vector& motion = TO_VECTOR_REF( output.m_deltaReferenceFrameLocal.m_translation );

	motion = w2l.TransformVector( motion );
#endif
}
