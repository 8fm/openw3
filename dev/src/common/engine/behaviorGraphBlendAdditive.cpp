/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behaviorGraphBlendAdditive.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animSyncInfo.h"
#include "behaviorIncludes.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendAdditiveNode );

const Float CBehaviorGraphBlendAdditiveNode::ACTIVATION_THRESHOLD = 0.001f;

CBehaviorGraphBlendAdditiveNode::CBehaviorGraphBlendAdditiveNode()
	: m_synchronize( true )
	, m_biasValue( 0.0f )
	, m_scaleValue( 1.0f ) 
	, m_syncMethod( NULL )
	, m_type( AT_Ref )
{
}

void CBehaviorGraphBlendAdditiveNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_prevControlValue;
}

void CBehaviorGraphBlendAdditiveNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_controlValue ] = 0.f;
	instance[ i_prevControlValue ] = 0.f;
}

void CBehaviorGraphBlendAdditiveNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_prevControlValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendAdditiveNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Added ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphBlendAdditiveNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Base") );
	m_cachedAddedInputNode = CacheBlock( TXT("Added") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphBlendAdditiveNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( BlendAdd );

	// update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// process activations
	ProcessActivations( instance );

	// synchronize children playback
	Synchronize( instance, timeDelta );

	// update appropriate child nodes
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );
	if ( !IsAddedInputActive( alpha ) )
	{
		if ( m_cachedInputNode ) 
		{
			m_cachedInputNode->Update( context, instance, timeDelta );
		}
	}
	else
	{
		if ( m_cachedInputNode ) 
		{
			m_cachedInputNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedAddedInputNode )
		{
			m_cachedAddedInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendAdditiveNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevControlValue ] = instance[ i_controlValue ];
	instance[ i_controlValue ] = m_cachedControlVariableNode ? m_cachedControlVariableNode->GetValue( instance ) : 0.0f;
}

void CBehaviorGraphBlendAdditiveNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( BlendAdd );

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		return;
	}
#endif

	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( !IsAddedInputActive( alpha ) )
	{
		if ( m_cachedInputNode )
		{
			m_cachedInputNode->Sample( context, instance, output );
		}
	}
	else
	{
		CCacheBehaviorGraphOutput cachePose1( context );
		CCacheBehaviorGraphOutput cachePose2( context );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			temp2->SetIdentity();

			if ( m_cachedInputNode )
			{
				m_cachedInputNode->Sample( context, instance, *temp1 );
			}

			if ( m_cachedAddedInputNode )
			{
				m_cachedAddedInputNode->Sample( context, instance, *temp2 );
			}

			if ( m_type == AT_Ref )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Ref( output, *temp1, *temp2, alpha );
			}
			else if ( m_type == AT_Local )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, *temp1, *temp2, alpha );
			}

			// Merge events and used anims
			output.MergeEventsAndUsedAnims( *temp1, 1.0f );
			output.MergeEventsAndUsedAnimsAsAdditives( *temp2, alpha );

			// Motion
			output.m_deltaReferenceFrameLocal = temp1->m_deltaReferenceFrameLocal;
		}
	}
}

void CBehaviorGraphBlendAdditiveNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{	
	Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	// base animation is played with normal speed
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncData( instance ).Reset();
	}

	// added input is synchronized to match base animation, 
	// no matter for alpha
	if ( IsAddedInputActive( alpha ) )
	{
		if ( m_synchronize && m_syncMethod )
		{
			const Float alphaZero = 0.0f;
			m_syncMethod->Synchronize( instance, m_cachedInputNode, m_cachedAddedInputNode, alphaZero, timeDelta );	
		}
		else
		{
			if ( m_cachedAddedInputNode )
			{
				m_cachedAddedInputNode->GetSyncData( instance ).Reset();
			}
		}
	}
}

void CBehaviorGraphBlendAdditiveNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	// added animation is always sync to base one
	// sync-info from additive blend node is equal to sync info of first node
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphBlendAdditiveNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_synchronize && m_syncMethod )
	{		
		if ( m_cachedInputNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedInputNode, info );
		}

		if ( m_cachedAddedInputNode )
		{
			m_syncMethod->SynchronizeTo( instance, m_cachedAddedInputNode, info );
		}
	}
	else
	{
		if ( m_cachedInputNode )
		{
			m_cachedInputNode->SynchronizeTo( instance, info );
		}

		if ( m_cachedAddedInputNode )
		{
			m_cachedAddedInputNode->SynchronizeTo( instance, info );
		}
	}
}

void CBehaviorGraphBlendAdditiveNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	Float prevAlpha = GetAlphaValue( instance[ i_prevControlValue ] );
	Float currAlpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( m_cachedAddedInputNode )
	{
		if ( !IsAddedInputActive( prevAlpha ) && IsAddedInputActive( currAlpha ) )
		{	
			m_cachedAddedInputNode->Activate( instance );

			if ( m_cachedInputNode && m_synchronize && m_syncMethod )
			{
				CSyncInfo syncInfo;
				m_cachedInputNode->GetSyncInfo( instance, syncInfo );
				m_syncMethod->SynchronizeTo( instance, m_cachedAddedInputNode, syncInfo );
			}
		}

		if ( IsAddedInputActive( prevAlpha ) && !IsAddedInputActive( currAlpha ) )
		{	
			m_cachedAddedInputNode->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphBlendAdditiveNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedInputNode && m_cachedInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	if ( m_cachedAddedInputNode && m_cachedAddedInputNode->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphBlendAdditiveNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	UpdateControlValue( instance );

	// activate inputs
	const Float alpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( IsAddedInputActive( alpha ) && m_cachedAddedInputNode )
	{
		m_cachedAddedInputNode->Activate( instance );
	}
}

void CBehaviorGraphBlendAdditiveNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	// we can safely deactivate both inputs
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedAddedInputNode ) 
	{
		m_cachedAddedInputNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

Float CBehaviorGraphBlendAdditiveNode::GetAlphaValue( Float varValue ) const
{
	Float value = ( varValue - m_biasValue ) * m_scaleValue;
	return value;
}

Bool CBehaviorGraphBlendAdditiveNode::IsAddedInputActive( Float var ) const
{
	return fabsf( var ) > ACTIVATION_THRESHOLD;
}

void CBehaviorGraphBlendAdditiveNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	Float controlAlpha = GetAlphaValue( instance[ i_controlValue ] );

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( IsAddedInputActive( controlAlpha ) )
	{
		if ( m_cachedAddedInputNode )
		{
			m_cachedAddedInputNode->ProcessActivationAlpha( instance, fabsf( controlAlpha ) * alpha );
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
