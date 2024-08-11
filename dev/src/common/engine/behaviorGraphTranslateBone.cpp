/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphTranslateBone.h"
#include "behaviorGraphValueNode.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphSocket.h"
#include "behaviorProfiler.h"
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphTranslateBoneNode );

CBehaviorGraphTranslateBoneNode::CBehaviorGraphTranslateBoneNode()
	: m_axis( 0.0f, 0.0f, 1.0f )
	, m_scale( 1.0f )
	, m_biasValue( 0.0f )
	, m_minValue( -10.0f )
	, m_maxValue( 10.0f )
	, m_clampValue( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphTranslateBoneNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Value ) ) );
}
#endif

void CBehaviorGraphTranslateBoneNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_currentValue;
}

void CBehaviorGraphTranslateBoneNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_currentValue ] = 0.f;
}

void CBehaviorGraphTranslateBoneNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_currentValue );
}

void CBehaviorGraphTranslateBoneNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( TranslateBone );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update variable
	if ( m_cachedValueNode )
	{
		// Let it compute
		m_cachedValueNode->Update( context, instance, timeDelta );

		// Grab and process value
		instance[ i_currentValue ] = m_cachedValueNode->GetValue( instance ) * m_scale + m_biasValue;
	}
	else
	{
		// Use default value
		instance[ i_currentValue ] = m_biasValue;
	}

	// Clamp if needed
	if ( m_clampValue )
	{
		instance[ i_currentValue ] = Clamp( instance[ i_currentValue ], m_minValue, m_maxValue );
	}
}

void CBehaviorGraphTranslateBoneNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( TranslateBone );

	TBaseClass::Sample( context, instance, output );

	const Float currentValue = instance[ i_currentValue ];
	const Int32 boneIndex = instance[ i_boneIndex ];

	// Apply local transformation
	if ( boneIndex != -1 && currentValue != 0.0f )
	{
#ifdef USE_HAVOK_ANIMATION
		// Assemble transform		
		hkQsTransform translation( hkVector4( m_axis.X * currentValue, m_axis.Y * currentValue, m_axis.Z * currentValue, 0.0f ), 
								   hkQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ),
								   hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );


		// Apply transform
		ASSERT( boneIndex < (Int32)output.m_numBones );
		output.m_outputPose[ boneIndex ].setMul( output.m_outputPose[ boneIndex ], translation );
#else
		// Assemble transform		
		RedQsTransform translation( RedVector4( m_axis.X * currentValue, m_axis.Y * currentValue, m_axis.Z * currentValue, 0.0f ), 
								   RedQuaternion( 0.0f, 0.0f, 0.0f, 1.0f ),
								   RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );


		// Apply transform
		ASSERT( boneIndex < (Int32)output.m_numBones );
		output.m_outputPose[ boneIndex ].SetMul( output.m_outputPose[ boneIndex ], translation );
#endif
	}
}


void CBehaviorGraphTranslateBoneNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}
}

void CBehaviorGraphTranslateBoneNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphTranslateBoneNode::CacheConnections()
{
	m_cachedInputNode = CacheBlock( TXT("Input") );
	m_cachedValueNode = CacheValueBlock( TXT("Value") );
}

void CBehaviorGraphTranslateBoneNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}
}
