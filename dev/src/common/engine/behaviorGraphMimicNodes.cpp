/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphContainerNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphMimicNodes.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../engine/curve.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animSyncInfo.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBaseMimicNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBaseMimicNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphBaseMimicNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBehaviorMimicBlendType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsBlendNode );

CBehaviorGraphMimicsBlendNode::CBehaviorGraphMimicsBlendNode()
{
}

void CBehaviorGraphMimicsBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_prevControlValue;
}

void CBehaviorGraphMimicsBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_prevControlValue ] = 0.f;
	instance[ i_controlValue ] = GetInitialControlValueFromType();
}

void CBehaviorGraphMimicsBlendNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_prevControlValue );
	INST_PROP( i_controlValue );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicsBlendNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( type ) )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphMimicsBlendNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( A ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( B ) ) );

	if ( m_type == BMBT_Continues )
	{
		CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	}
}

String CBehaviorGraphMimicsBlendNode::GetCaption() const
{ 
	String caption = TXT("Mimic blend "); 

	if ( m_type == BMBT_Continues ) caption += TXT("[ Continues ]");
	else if ( m_type == BMBT_Max ) caption += TXT("[ Max ]");
	else if ( m_type == BMBT_Min ) caption += TXT("[ Min ]");
	else if ( m_type == BMBT_Add ) caption += TXT("[ Add ]");
	else if ( m_type == BMBT_Sub ) caption += TXT("[ Sub ]");
	else if ( m_type == BMBT_Mul ) caption += TXT("[ Mul ]");
	else if ( m_type == BMBT_AbsMax ) caption += TXT("[ AbsMax ]");

	return caption;
}

#endif

void CBehaviorGraphMimicsBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicsBlend );
	// update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// process activations
	ProcessActivations( instance );

	if ( !IsSecondInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Update( context, instance, timeDelta );
		}
	}
	else if ( !IsFirstInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Update( context, instance, timeDelta );
		}
	}
	else
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphMimicsBlendNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevControlValue ]	= instance[ i_controlValue ];
	instance[ i_controlValue ]		= m_cachedControlVariableNode ? m_cachedControlVariableNode->GetValue( instance ) : GetInitialControlValueFromType();
}

void CBehaviorGraphMimicsBlendNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( !IsSecondInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->GetSyncInfo( instance, info );
		}
	}
	else if ( !IsFirstInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->GetSyncInfo( instance, info );
		}
	}
	else
	{
		CSyncInfo firstSyncInfo;
		CSyncInfo secondSyncInfo;

		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->GetSyncInfo( instance, firstSyncInfo );
		}

		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->GetSyncInfo( instance, secondSyncInfo );
		}

		info.SetInterpolate( firstSyncInfo, secondSyncInfo, instance[ i_controlValue ] );
	}
}

void CBehaviorGraphMimicsBlendNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->SynchronizeTo( instance, info );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphMimicsBlendNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	const Float controlValue = instance[ i_controlValue ];
	const Float prevControlValue = instance[ i_prevControlValue ];

	if ( m_cachedFirstInputNode )
	{
		if ( !IsFirstInputActive( prevControlValue ) && IsFirstInputActive( controlValue ) )
		{				
			m_cachedFirstInputNode->Activate( instance );
		}

		if ( IsFirstInputActive( prevControlValue ) && !IsFirstInputActive( controlValue ) )
		{				
			m_cachedFirstInputNode->Deactivate( instance );
		}
	}	

	if ( m_cachedSecondInputNode )
	{
		if ( !IsSecondInputActive( prevControlValue ) && IsSecondInputActive( controlValue ) )
		{	
			m_cachedSecondInputNode->Activate( instance );
		}

		if ( IsSecondInputActive( prevControlValue ) && !IsSecondInputActive( controlValue ) )
		{	
			m_cachedSecondInputNode->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphMimicsBlendNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedFirstInputNode && m_cachedFirstInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	if ( m_cachedSecondInputNode && m_cachedSecondInputNode->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}


void CBehaviorGraphMimicsBlendNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	UpdateControlValue( instance );

	// activate inputs
	if ( IsFirstInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedFirstInputNode )
		{
			m_cachedFirstInputNode->Activate( instance );
		}
	}

	if ( IsSecondInputActive( instance[ i_controlValue ] ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Activate( instance );
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMimicsBlendNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedFirstInputNode )
	{
		m_cachedFirstInputNode->Deactivate( instance );
	}

	if ( m_cachedSecondInputNode )
	{
		m_cachedSecondInputNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicsBlendNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedFirstInputNode = CacheMimicBlock( TXT("A") );
	m_cachedSecondInputNode = CacheMimicBlock( TXT("B") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphMimicsBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	const Float controlValue = instance[ i_controlValue ];

	if ( !IsSecondInputActive( controlValue ) )
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->Sample( context, instance, output );
		}
	}
	else if ( !IsFirstInputActive( controlValue ) )
	{
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->Sample( context, instance, output );
		}
	}
	else if ( context.HasMimic() )
	{
		CCacheBehaviorGraphOutput cachePose1( context, true );
		CCacheBehaviorGraphOutput cachePose2( context, true );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			if ( m_cachedFirstInputNode ) 
			{
				m_cachedFirstInputNode->Sample( context, instance, *temp1 );
			}

			if ( m_cachedSecondInputNode )
			{
				m_cachedSecondInputNode->Sample( context, instance, *temp2 );
			}

			// Blending
			ASSERT( output.m_numFloatTracks == temp1->m_numFloatTracks );

			Blend( output, *temp1, *temp2, controlValue );

			// Merge events
			output.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - controlValue, controlValue );
		}
	}
}

Float CBehaviorGraphMimicsBlendNode::GetInitialControlValueFromType() const
{
	return m_type != BMBT_Continues ? 0.5f : 0.f;
}

void CBehaviorGraphMimicsBlendNode::Blend( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, const Float weight ) const
{
	ASSERT( weight <= 1.f && weight >= 0.f );

	Uint32 size = Min( output.m_numFloatTracks, Min( poseA.m_numFloatTracks, poseB.m_numFloatTracks ) );

	if ( m_type == BMBT_Continues )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = Lerp( weight, poseA.m_floatTracks[i], poseB.m_floatTracks[i] );
		}
	}
	else if ( m_type == BMBT_Min )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = Min( poseA.m_floatTracks[i], poseB.m_floatTracks[i] );
		}
	}
	else if ( m_type == BMBT_Max )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = Max( poseA.m_floatTracks[i], poseB.m_floatTracks[i] );
		}
	}
	else if ( m_type == BMBT_Add )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = poseA.m_floatTracks[i] + poseB.m_floatTracks[i];
		}
	}
	else if ( m_type == BMBT_Sub )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = poseA.m_floatTracks[i] - poseB.m_floatTracks[i];
		}
	}
	else if ( m_type == BMBT_Mul )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = poseA.m_floatTracks[i] * poseB.m_floatTracks[i];
		}
	}
	else if ( m_type == BMBT_AbsMax )
	{
		for( Uint32 i=0; i<size; ++i )
		{
			output.m_floatTracks[i] = MAbs( poseA.m_floatTracks[i] ) > MAbs( poseB.m_floatTracks[i] ) ? poseA.m_floatTracks[i] : poseB.m_floatTracks[i];
		}
	}
	else
	{
		ASSERT( 0 );
	}

	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
#ifdef USE_HAVOK_ANIMATION
		output.m_outputPose[ i ].setMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
#else
		output.m_outputPose[ i ].SetMul( poseA.m_outputPose[ i ], poseB.m_outputPose[ i ] );
#endif
	}
}

Bool CBehaviorGraphMimicsBlendNode::IsFirstInputActive( Float var ) const
{
	return var < 1.0f;
}

Bool CBehaviorGraphMimicsBlendNode::IsSecondInputActive( Float var ) const
{
	return var > 0.f;
}

void CBehaviorGraphMimicsBlendNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	const Float controlValue = instance[ i_controlValue ];

	if ( m_type == BMBT_Continues )
	{
		if ( IsFirstInputActive( instance[ i_controlValue ] ) )
		{
			if ( m_cachedFirstInputNode ) 
			{
				m_cachedFirstInputNode->ProcessActivationAlpha( instance, ( 1.0f - controlValue ) * alpha );
			}
		}

		if ( IsSecondInputActive( instance[ i_controlValue ] ) )
		{
			if ( m_cachedSecondInputNode )
			{
				m_cachedSecondInputNode->ProcessActivationAlpha( instance, controlValue * alpha );
			}
		}
	}
	else
	{
		if ( m_cachedFirstInputNode ) 
		{
			m_cachedFirstInputNode->ProcessActivationAlpha( instance, alpha );
		}
		if ( m_cachedSecondInputNode )
		{
			m_cachedSecondInputNode->ProcessActivationAlpha( instance, alpha );
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, 1.f );
	}
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicGainNode );

CBehaviorGraphMimicGainNode::CBehaviorGraphMimicGainNode()
	: m_gain( 1.f )
	, m_min( -1.f )
	, m_max( 1.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicGainNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Gain ), false ) );
}

#endif

void CBehaviorGraphMimicGainNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicGain );
	// Update input
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedGainValueNode )
	{
		m_cachedGainValueNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphMimicGainNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedGainValueNode )
	{
		m_cachedGainValueNode->Activate( instance );
	}
}

void CBehaviorGraphMimicGainNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedGainValueNode )
	{
		m_cachedGainValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicGainNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
	m_cachedGainValueNode = CacheValueBlock( TXT("Gain") );
}

void CBehaviorGraphMimicGainNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedGainValueNode )
	{
		m_cachedGainValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicGainNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float gain = m_gain;

	if ( m_cachedGainValueNode )
	{
		gain = m_cachedGainValueNode->GetValue( instance );
	}

	for ( Uint32 i=0; i<output.m_numFloatTracks; i++ )
	{
		output.m_floatTracks[ i ] = Clamp< Float >( output.m_floatTracks[ i ] * gain, m_min, m_max );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EBehaviorMimicMathOp );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicMathOpNode );

CBehaviorGraphMimicMathOpNode::CBehaviorGraphMimicMathOpNode()
{
}

void CBehaviorGraphMimicMathOpNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_value;
	compiler << i_trackIndex;
}

void CBehaviorGraphMimicMathOpNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_value ] = m_value;
	instance[ i_trackIndex ] = -1;

	CacheTrack( instance );
}

void CBehaviorGraphMimicMathOpNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_value );
	INST_PROP( i_trackIndex );
}

CSkeleton* CBehaviorGraphMimicMathOpNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetMimicSkeleton();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicMathOpNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Value ) ) );
}

#endif

void CBehaviorGraphMimicMathOpNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicMathOp );
	// Update input
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );
		instance[ i_value ] = m_cachedValueNode->GetValue( instance );
	}
}

void CBehaviorGraphMimicMathOpNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}
}

void CBehaviorGraphMimicMathOpNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicMathOpNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
	m_cachedValueNode = CacheValueBlock( TXT("Value") );
}

void CBehaviorGraphMimicMathOpNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicMathOpNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 trackIndex = instance[ i_trackIndex ];
	const Float value = instance[ i_value ];

	ASSERT( (Int32)output.m_numFloatTracks > trackIndex );

	if ( trackIndex != -1 && (Int32)output.m_numFloatTracks > trackIndex )
	{
		if ( m_mathOp == BMMO_Add )
		{
			output.m_floatTracks[ trackIndex ] += value;
		}
		else if ( m_mathOp == BMMO_Sub )
		{
			output.m_floatTracks[ trackIndex ] -= value;
		}
		else if ( m_mathOp == BMMO_Mul )
		{
			output.m_floatTracks[ trackIndex ] *= value;
		}
		else if ( m_mathOp == BMMO_Div && MAbs( value ) > 0.001f )
		{
			output.m_floatTracks[ trackIndex ] /= value;
		}
		else if ( m_mathOp == BMMO_Max )
		{
			output.m_floatTracks[ trackIndex ] = Max< Float >( output.m_floatTracks[ trackIndex ], value );
		}
		else if ( m_mathOp == BMMO_Min )
		{
			output.m_floatTracks[ trackIndex ] = Min< Float >( output.m_floatTracks[ trackIndex ], value );
		}
	}
}

void CBehaviorGraphMimicMathOpNode::CacheTrack( CBehaviorGraphInstance& instance ) const
{
	// Reset
	instance[ i_trackIndex ] = -1;

	if ( instance.GetAnimatedComponent()->GetMimicSkeleton() )
	{
		CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();
		instance[ i_trackIndex ] = FindTrackIndex( m_trackName, skeleton );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicFilterNode );

void CBehaviorGraphMimicFilterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_tracksIndex;
}

void CBehaviorGraphMimicFilterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();
	if ( skeleton == NULL )
	{
		return;
	}

	TDynArray< Int32 >& tracksIdx = instance[ i_tracksIndex ];
	tracksIdx.Resize( m_tracks.Size() );

	for ( Uint32 i=0; i<m_tracks.Size(); ++i )
	{
		tracksIdx[ i ] = FindTrackIndex( m_tracks[ i ].m_trackName, skeleton );
	}
}

void CBehaviorGraphMimicFilterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 tracksNum = (Int32)output.m_numFloatTracks;

	const TDynArray< Int32 >& tracks = instance[ i_tracksIndex ];

	for ( Uint32 i=0; i<tracks.Size(); ++i )
	{
		const Int32 track = tracks[ i ];

		if ( track != -1 && track < tracksNum )
		{
			output.m_floatTracks[ track ] *= m_tracks[ i ].m_weight;
		}
	}	
}

CSkeleton* CBehaviorGraphMimicFilterNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetMimicSkeleton();
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicFilterNodeInvert );

int compareSBehaviorGraphTrackInfo(const void* a,const void* b)
{
	int* aa = (int*)a;
	int* bb = (int*)b;
	if (*aa>*bb){return 1;}
	if (*aa==*bb){return 0;}
	return -1;
}

void CBehaviorGraphMimicFilterNodeInvert::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	qsort( instance[ i_tracksIndex ].Data(), instance[ i_tracksIndex ].Size(), sizeof(int), &compareSBehaviorGraphTrackInfo );
}

void CBehaviorGraphMimicFilterNodeInvert::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 tracksNum = (Int32)output.m_numFloatTracks;
	const TDynArray< Int32 >& tracks = instance[ i_tracksIndex ];

	int ind = 0;
	for( int j=0; j<tracksNum; j++ )
	{
		if( tracks[ ind ] == j )
		{
			output.m_floatTracks[ j ] *= m_tracks[ ind ].m_weight;
			ind++;
		}
		else
		{
			output.m_floatTracks[ j ] = 0.f;
		}
	}

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicOutputNode );

CBehaviorGraphMimicOutputNode::CBehaviorGraphMimicOutputNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicOutputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	
}

#endif

void CBehaviorGraphMimicOutputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicStageNode );

CBehaviorGraphMimicStageNode::CBehaviorGraphMimicStageNode() 	
{	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphMimicStageNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	CBehaviorGraphContainerNode::OnSpawned( info );

	m_rootNode = SafeCast< CBehaviorGraphNode >( CreateChildNode( GraphBlockSpawnInfo( CBehaviorGraphMimicOutputNode::GetStaticClass() ) ) );

	CreateMimicInput( CNAME( Input ) );
}

void CBehaviorGraphMimicStageNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		

	CBehaviorGraphContainerNode::OnRebuildSockets();
}
#endif

void CBehaviorGraphMimicStageNode::CacheConnections()
{
	// Pass to base class
	CBehaviorGraphContainerNode::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicStageNode::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Mimic stage [ %s ]"), m_name.AsChar() );

	return String( TXT("Mimic stage") );
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicParentInputNode );

CBehaviorGraphMimicParentInputNode::CBehaviorGraphMimicParentInputNode()	
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicParentInputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
}

#endif

void CBehaviorGraphMimicParentInputNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Reset
	m_cachedInputNode = NULL;

	// Cache the input
	CBehaviorGraphNode *parent = SafeCast< CBehaviorGraphNode >( GetParent() );
	while ( parent && !m_cachedInputNode )
	{
		m_cachedInputNode = parent->CacheMimicBlock( m_parentSocket.AsString().AsChar() );
		parent = Cast< CBehaviorGraphNode >( parent->GetParent() );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicAnimationManualSwitchNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicAnimationEnumSwitchNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

CGraphSocket* CBehaviorGraphMimicAnimationManualSwitchNode::CreateInputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( name ) );
}

CGraphSocket* CBehaviorGraphMimicAnimationManualSwitchNode::CreateOutputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( name ) );
}

#endif

CBehaviorGraphNode* CBehaviorGraphMimicAnimationManualSwitchNode::CacheInputBlock( const String& name )
{
	return CacheMimicBlock( name );
}

Bool CBehaviorGraphMimicAnimationManualSwitchNode::GetPoseType() const
{
	// Mimic pose
	return true;
}

CBehaviorGraphMimicAnimationEnumSwitchNode::CBehaviorGraphMimicAnimationEnumSwitchNode()
	: m_useCurve( false )
	, m_curve( NULL )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

CGraphSocket* CBehaviorGraphMimicAnimationEnumSwitchNode::CreateInputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( name ) );
}

CGraphSocket* CBehaviorGraphMimicAnimationEnumSwitchNode::CreateOutputSocket( const CName& name )
{
	return CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( name ) );
}

void CBehaviorGraphMimicAnimationEnumSwitchNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_curve = CreateObject< CCurve >( this );
}

#endif

Float CBehaviorGraphMimicAnimationEnumSwitchNode::GetBlendWeight( CBehaviorGraphInstance& instance ) const
{
	if ( CanBlend( instance ) )
	{
		Float value = Clamp( 1.f - instance[ i_blendTimer ] / instance[ i_blendTimeDuration ], 0.f, 1.f );

		if ( m_useCurve && m_curve )
		{
			return Clamp( m_curve->GetFloatValue( value ), 0.f, 1.f );
		}
		else
		{
			return m_interpolation == IT_Linear ? value : BehaviorUtils::BezierInterpolation( value );
		}
	}

	return 0.f;
}

CBehaviorGraphNode* CBehaviorGraphMimicAnimationEnumSwitchNode::CacheInputBlock( const String& name )
{
	return CacheMimicBlock( name );
}

Bool CBehaviorGraphMimicAnimationEnumSwitchNode::GetPoseType() const
{
	// Mimic pose
	return true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicRandomNode );

CBehaviorGraphNode* CBehaviorGraphMimicRandomNode::CacheInputBlock( const String& socketName )
{ 
	return CacheMimicBlock( socketName );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicRandomNode::CreateOutputSocket()
{
	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

void CBehaviorGraphMimicRandomNode::CreateInputSocket( const CName& socketName )
{
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( socketName ) );
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicRandomBlendNode );

void CBehaviorGraphMimicRandomBlendNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_alpha;	
	compiler << i_delta;	
	compiler << i_previnput;
}

void CBehaviorGraphMimicRandomBlendNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_alpha );
	INST_PROP( i_delta );
	INST_PROP( i_previnput );
}

void CBehaviorGraphMimicRandomBlendNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_alpha ] = 0;
	instance[ i_delta ] = 0;
	instance[ i_currentInput ] = NULL;
	instance[ i_previnput ] = NULL;
}

void CBehaviorGraphMimicRandomBlendNode::SelectRandomInput( CBehaviorGraphInstance& instance ) const
{
	instance[ i_previnput ] = instance[ i_currentInput ];
	instance[ i_delta ] = 0;
	instance[ i_alpha ] = 1.0f;

	TBaseClass::SelectRandomInput( instance );

	if( instance[ i_previnput ] == instance[ i_currentInput ] )
	{
		instance[ i_previnput ] = NULL;
		instance[ i_alpha ] = 0.0f;
	}
}

void CBehaviorGraphMimicRandomBlendNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicRandomBlend );
	if ( !instance[ i_currentInput ] )
	{
		SelectRandomInput( instance );

		if ( !instance[ i_currentInput ] )
		{
			return;
		}
	}

	CBehaviorGraphNode* prevInput = instance[ i_previnput ];
	CBehaviorGraphNode* currInput = instance[ i_currentInput ];

	Float& delta = instance[ i_delta ];
	Float& alpha = instance[ i_alpha ];

	delta += timeDelta;

	CSyncInfo syncInfo;
	syncInfo.m_wantSyncEvents = false;
	currInput->GetSyncInfo( instance, syncInfo );

	if( delta >= syncInfo.m_totalTime)
	{
		SelectRandomInput( instance );
	}

	if( delta > m_blendDuration )
	{
		instance[ i_previnput ] = NULL;
		alpha = 0.0f;
	}

	if( currInput )
	{
		currInput->Update( context, instance, timeDelta );
	}

	if( prevInput )
	{
		prevInput->Update( context, instance, timeDelta );

		if( m_blendDuration > 0 )
		{
			alpha -= timeDelta / m_blendDuration;
		}
		else
		{
			alpha = 0.f;
		}

		alpha = Clamp( alpha, 0.f, 1.f ); 
	}
}

void CBehaviorGraphMimicRandomBlendNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	CBehaviorGraphNode* prevInput = instance[ i_previnput ];
	CBehaviorGraphNode* currInput = instance[ i_currentInput ];

	if ( currInput )
	{
		if( prevInput )
		{
			CCacheBehaviorGraphOutput cachePoseA( context, true );
			CCacheBehaviorGraphOutput cachePoseB( context, true );

			SBehaviorGraphOutput* poseA = cachePoseA.GetPose();
			SBehaviorGraphOutput* poseB = cachePoseB.GetPose();

			if ( poseA && poseB )
			{
				currInput->Sample( context, instance, *poseA );
				prevInput->Sample( context, instance, *poseB );

				const Float alpha = instance[ i_alpha ];

				output.SetInterpolate( *poseA, *poseB, alpha );
			}
		}
		else
		{
			currInput->Sample( context, instance, output );
		}
	}
}

void CBehaviorGraphMimicRandomBlendNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );

	if (! m_cachedInputNodes.Exist(instance[ i_previnput ]))
	{
		// loaded - we have no idea what it was, just null it
		instance[ i_previnput ] = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicEyesCorrectionNode );

CBehaviorGraphMimicEyesCorrectionNode::CBehaviorGraphMimicEyesCorrectionNode()
	: m_trackEyeLeft_Left( TXT( "eye_left_left" ) )
	, m_trackEyeLeft_Right( TXT( "eye_left_right" ) )
	, m_trackEyeRight_Left( TXT( "eye_right_left" ) )
	, m_trackEyeRight_Right( TXT( "eye_right_right" ) )
{
}

void CBehaviorGraphMimicEyesCorrectionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_trackEyeLeft_Left;	
	compiler << i_trackEyeLeft_Right;	
	compiler << i_trackEyeRight_Left;
	compiler << i_trackEyeRight_Right;
}

void CBehaviorGraphMimicEyesCorrectionNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_trackEyeLeft_Left );
	INST_PROP( i_trackEyeLeft_Right );
	INST_PROP( i_trackEyeRight_Left );
	INST_PROP( i_trackEyeRight_Right );
}

void CBehaviorGraphMimicEyesCorrectionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton() )
	{
		instance[ i_trackEyeLeft_Left ] = FindTrackIndex( m_trackEyeLeft_Left, skeleton );
		instance[ i_trackEyeLeft_Right ] = FindTrackIndex( m_trackEyeLeft_Right, skeleton );
		instance[ i_trackEyeRight_Left ] = FindTrackIndex( m_trackEyeRight_Left, skeleton );
		instance[ i_trackEyeRight_Right ] = FindTrackIndex( m_trackEyeRight_Right, skeleton );

		if ( instance[ i_trackEyeLeft_Left ] == -1 || instance[ i_trackEyeLeft_Right ] == -1 || instance[ i_trackEyeRight_Left ] == -1 )
		{
			instance[ i_trackEyeRight_Right ] = -1; // Marker - i_trackEyeRight_Right has lower idx - better for cache
		}
	}
	else
	{
		instance[ i_trackEyeLeft_Left ] = -1;
		instance[ i_trackEyeLeft_Right ] = -1;
		instance[ i_trackEyeRight_Left ] = -1;
		instance[ i_trackEyeRight_Right ] = -1;
	}
}

void CBehaviorGraphMimicEyesCorrectionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 trackEyeRight_Right = instance[ i_trackEyeRight_Right ];
	if ( trackEyeRight_Right != -1 ) // Check marker
	{
		const Int32 trackEyeLeft_Right = instance[ i_trackEyeLeft_Right ];
		const Int32 trackEyeRight_Left = instance[ i_trackEyeRight_Left ];
		const Int32 trackEyeLeft_Left = instance[ i_trackEyeLeft_Left ];

		ASSERT( trackEyeLeft_Right != -1 );
		ASSERT( trackEyeRight_Left != -1 );
		ASSERT( trackEyeLeft_Left != -1 );

		const Int32 numTracks = (Int32)output.m_numFloatTracks;
		if ( trackEyeLeft_Left < numTracks && trackEyeLeft_Right < numTracks && trackEyeRight_Left < numTracks && trackEyeRight_Right < numTracks )
		{
			output.m_floatTracks[ trackEyeLeft_Left ] = output.m_floatTracks[ trackEyeRight_Left ];
			output.m_floatTracks[ trackEyeLeft_Right ] = output.m_floatTracks[ trackEyeRight_Right ];
		}
	}
}

CSkeleton* CBehaviorGraphMimicEyesCorrectionNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetMimicSkeleton();
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicBlinkControllerNode );

CBehaviorGraphMimicBlinkControllerNode::CBehaviorGraphMimicBlinkControllerNode()
	: m_trackEyeLeft_Down( TXT( "eyelids_upper_left_down" ) )
	, m_trackEyeRight_Down( TXT( "eyelids_upper_right_down" ) )
{
}

void CBehaviorGraphMimicBlinkControllerNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_trackEyeLeft_Down;
	compiler << i_trackEyeRight_Down;
}

void CBehaviorGraphMimicBlinkControllerNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_trackEyeLeft_Down );
	INST_PROP( i_trackEyeRight_Down );
}

void CBehaviorGraphMimicBlinkControllerNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton() )
	{
		instance[ i_trackEyeLeft_Down ] = FindTrackIndex( m_trackEyeLeft_Down, skeleton );
		instance[ i_trackEyeRight_Down ] = FindTrackIndex( m_trackEyeRight_Down, skeleton );

		if ( instance[ i_trackEyeLeft_Down ] == -1 || instance[ i_trackEyeRight_Down ] == -1 )
		{
			instance[ i_trackEyeLeft_Down ] = -1; // Marker
		}
	}
	else
	{
		instance[ i_trackEyeLeft_Down ] = -1;
		instance[ i_trackEyeRight_Down ] = -1;
	}

	InternalReset( instance );
}

void CBehaviorGraphMimicBlinkControllerNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	
}

void CBehaviorGraphMimicBlinkControllerNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	InternalReset( instance );
}

CSkeleton* CBehaviorGraphMimicBlinkControllerNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetMimicSkeleton();
}

Bool CBehaviorGraphMimicBlinkControllerNode::IsValid( const CBehaviorGraphInstance& instance ) const
{
	const Int32 trackEyeLeft_Down = instance[ i_trackEyeLeft_Down ]; // Check marker
	return trackEyeLeft_Down != -1;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Watcher );

void CBehaviorGraphMimicBlinkControllerNode_Watcher::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );
	
	if ( IsValid( instance ) )
	{
		const Int32 trackEyeRight_Down = instance[ i_trackEyeRight_Down ];
		const Int32 trackEyeLeft_Down = instance[ i_trackEyeLeft_Down ];

		ASSERT( trackEyeRight_Down != -1 );
		ASSERT( trackEyeLeft_Down != -1 );

		const Int32 numTracks = (Int32)output.m_numFloatTracks;
		if ( trackEyeLeft_Down < numTracks && trackEyeRight_Down < numTracks )
		{
			const Float eyeRight_Down = output.m_floatTracks[ trackEyeRight_Down ];
			const Float eyeLeft_Down = output.m_floatTracks[ trackEyeLeft_Down ];

			if ( m_variableNameRight )
			{
				instance.SetInternalFloatValue( m_variableNameRight, eyeRight_Down );
			}
			if ( m_variableNameLeft )
			{
				instance.SetInternalFloatValue( m_variableNameLeft, eyeLeft_Down );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Setter );

CBehaviorGraphMimicBlinkControllerNode_Setter::CBehaviorGraphMimicBlinkControllerNode_Setter()
	: m_blinkCooldown( 1.f )
	, m_blinkValueThr( 0.6f )
{
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_state;
	compiler << i_timer;
	compiler << i_blocked;
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_state );
	INST_PROP( i_timer );
	INST_PROP( i_blocked );
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::InternalReset( CBehaviorGraphInstance& instance ) const
{
	SetState( instance, S_None );
	SetTimer( instance, 0.f );
	instance[ i_blocked ] = false;
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::SetState( CBehaviorGraphInstance& instance, CBehaviorGraphMimicBlinkControllerNode_Setter::EState state ) const
{
	instance[ i_state ] = state;
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::SetTimer( CBehaviorGraphInstance& instance, Float timeDuration ) const
{
	instance[ i_timer ] = timeDuration;
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& timer = instance[ i_timer ];
	const Int32 state = instance[ i_state ];

	switch ( state )
	{
	case S_None:
		{
			ASSERT( timer == 0.f );
			break;
		}

	case S_BlinkInProgress:
		{
			ASSERT( timer == 0.f );
			break;
		}

	case S_BlinkBlocked:
		{
			ASSERT( timer <= m_blinkCooldown );

			timer = Max( timer-timeDelta, 0.f );
			if ( timer < 0.001f )
			{
				SetState( instance, S_BlinkBlockedToNone );
				SetTimer( instance, 0.f );
			}
			break;
		}

	case S_BlinkBlockedToNone:
		{
			ASSERT( timer == 0.f );
			break;
		}

	default:
		{
			ASSERT( 0 );
			SetState( instance, S_None );
			SetTimer( instance, 0.f );
			break;
		}
	}
}

void CBehaviorGraphMimicBlinkControllerNode_Setter::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );
	
	if ( IsValid( instance ) )
	{
		const Int32 trackEyeLeft_Down = instance[ i_trackEyeLeft_Down ];
		const Int32 trackEyeRight_Down = instance[ i_trackEyeRight_Down ];

		ASSERT( trackEyeLeft_Down != -1 );
		ASSERT( trackEyeRight_Down != -1 );

		const Int32 numTracks = (Int32)output.m_numFloatTracks;
		if ( trackEyeLeft_Down < numTracks && trackEyeRight_Down < numTracks )
		{
			Float& trackValueEyeRight_Down = output.m_floatTracks[ trackEyeRight_Down ];
			Float& trackValueEyeLeft_Down = output.m_floatTracks[ trackEyeLeft_Down ];

			const Float eyeRight_Down = m_variableNameRight ? instance.GetInternalFloatValue( m_variableNameRight ) : 0.f;
			const Float eyeLeft_Down = m_variableNameLeft ? instance.GetInternalFloatValue( m_variableNameRight ) : 0.f;

			const Float timer = instance[ i_timer ];

			const Float areEyesClosedThr = 0.02f;

			const Bool isBlinkNow_watcher = eyeRight_Down > m_blinkValueThr && eyeLeft_Down > m_blinkValueThr;
			const Bool areEyesClosedNow_watcher = eyeRight_Down > areEyesClosedThr && eyeLeft_Down > areEyesClosedThr;

			const Bool isBlinkNow_any = trackValueEyeRight_Down > m_blinkValueThr && trackValueEyeLeft_Down > m_blinkValueThr;
			const Bool areEyesClosedNow_any = trackValueEyeRight_Down > areEyesClosedThr && trackValueEyeLeft_Down > areEyesClosedThr;

			const Bool wasBlocked = instance[ i_blocked ];
			Bool& isBlocked = instance[ i_blocked ];
			isBlocked = false;

			const Int32 state = instance[ i_state ];
			switch ( state )
			{
			case S_None:
				{
					if ( isBlinkNow_any )
					{
						SetState( instance, S_BlinkInProgress );
						SetTimer( instance, 0.f );
					}
					break;
				}

			case S_BlinkInProgress:
				{
					if ( !areEyesClosedNow_any )
					{
						SetState( instance, S_BlinkBlocked );
						SetTimer( instance, m_blinkCooldown );
					}
					break;
				}

			case S_BlinkBlocked:
				{
					ASSERT( timer <= m_blinkCooldown );
					
					if ( areEyesClosedNow_watcher ) // TODO - gesture animation can have a blink now and this blink will be blocked as well :(
					{
						trackValueEyeRight_Down = 0.f;
						trackValueEyeLeft_Down = 0.f;

						isBlocked = true;
					}
					break;
				}

			case S_BlinkBlockedToNone:
				{
					ASSERT( timer == 0.f );
					
					if ( areEyesClosedNow_watcher )
					{
						trackValueEyeRight_Down = 0.f;
						trackValueEyeLeft_Down = 0.f;

						isBlocked = true;
					}
					else
					{
						SetState( instance, S_None );
						SetTimer( instance, 0.f );
					}
					break;
				}

			default:
				{
					ASSERT( 0 );
					SetState( instance, S_None );
					SetTimer( instance, 0.f );
					break;
				}
			}
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphMimicBlinkControllerNode_Setter::GetBlinkStateDesc( CBehaviorGraphInstance& instance, String& desc ) const
{
	const Int32 s = instance[ i_state ];
	const Float t = instance[ i_timer ];

	if ( s == S_None )
	{
		return false;
	}

	switch ( s )
	{
	case S_BlinkInProgress:
		{
			desc = String::Printf( TXT("BlinkInProgress") );
			break;
		}

	case S_BlinkBlocked:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlocked - %1.2f [%1.1f] - <BLINK IS BLOCKED NOW>"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlocked - %1.2f [%1.1f]"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			break;
		}

	case S_BlinkBlockedToNone:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlockedToNone - <BLINK IS BLOCKED NOW>") );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlockedToNone") );
			}
			break;
		}
	}

	return true;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicBlinkControllerNode_Blend );

CBehaviorGraphMimicBlinkControllerNode_Blend::CBehaviorGraphMimicBlinkControllerNode_Blend()
	: m_blinkCooldown( 1.f )
	, m_blinkValueThr( 0.6f )
	, m_trackEyeLeft_Down( TXT( "eyelids_upper_left_down" ) )
	, m_trackEyeRight_Down( TXT( "eyelids_upper_right_down" ) )
{
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_trackEyeLeft_Down;
	compiler << i_trackEyeRight_Down;
	compiler << i_state;
	compiler << i_timer;
	compiler << i_blocked;
	compiler << i_trackEyeLeft_Idle_CurrValue;
	compiler << i_trackEyeRight_Idle_CurrValue;
	compiler << i_trackEyeLeft_Idle_PrevValue;
	compiler << i_trackEyeRight_Idle_PrevValue;
	compiler << i_trackEyeLeft_Rest_CurrValue;
	compiler << i_trackEyeRight_Rest_CurrValue;
	compiler << i_trackEyeLeft_Rest_PrevValue;
	compiler << i_trackEyeRight_Rest_PrevValue;
	compiler << i_state_Idle;
	compiler << i_state_Rest;
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton() )
	{
		instance[ i_trackEyeLeft_Down ] = FindTrackIndex( m_trackEyeLeft_Down, skeleton );
		instance[ i_trackEyeRight_Down ] = FindTrackIndex( m_trackEyeRight_Down, skeleton );

		if ( instance[ i_trackEyeLeft_Down ] == -1 || instance[ i_trackEyeRight_Down ] == -1 )
		{
			instance[ i_trackEyeLeft_Down ] = -1; // Marker
		}
	}
	else
	{
		instance[ i_trackEyeLeft_Down ] = -1;
		instance[ i_trackEyeRight_Down ] = -1;
	}

	InternalReset( instance );
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_state );
	INST_PROP( i_state_Idle );
	INST_PROP( i_state_Rest );
	INST_PROP( i_timer );
	INST_PROP( i_blocked );
	INST_PROP( i_trackEyeLeft_Down );
	INST_PROP( i_trackEyeRight_Down );
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::InternalReset( CBehaviorGraphInstance& instance ) const
{
	SetState( instance, S_None );
	SetTimer( instance, 0.f );
	instance[ i_blocked ] = false;

	instance[ i_state_Idle ] = BS_None;
	instance[ i_state_Rest ] = BS_None;

	instance[ i_trackEyeLeft_Idle_CurrValue ] = 0.f;
	instance[ i_trackEyeRight_Idle_CurrValue ] = 0.f;
	instance[ i_trackEyeLeft_Idle_PrevValue ] = 0.f;
	instance[ i_trackEyeRight_Idle_PrevValue ] = 0.f;
	instance[ i_trackEyeLeft_Rest_CurrValue ] = 0.f;
	instance[ i_trackEyeRight_Rest_CurrValue ] = 0.f;
	instance[ i_trackEyeLeft_Rest_PrevValue ] = 0.f;
	instance[ i_trackEyeRight_Rest_PrevValue ] = 0.f;
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::SetState( CBehaviorGraphInstance& instance, CBehaviorGraphMimicBlinkControllerNode_Blend::EState state ) const
{
	instance[ i_state ] = state;
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::SetTimer( CBehaviorGraphInstance& instance, Float timeDuration ) const
{
	instance[ i_timer ] = timeDuration;
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( BlinkController );
	
	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->Update( context, instance, timeDelta );
	}
	if ( m_cachedInputRest )
	{
		m_cachedInputRest->Update( context, instance, timeDelta );
	}

	{
		const Float idle_ER_Curr = instance[ i_trackEyeRight_Idle_CurrValue ];
		const Float idle_EL_Curr = instance[ i_trackEyeLeft_Idle_CurrValue ];
		const Float rest_ER_Curr = instance[ i_trackEyeRight_Rest_CurrValue ];
		const Float rest_EL_Curr = instance[ i_trackEyeLeft_Rest_CurrValue ];

		Float& idle_ER_Prev = instance[ i_trackEyeRight_Idle_PrevValue ];
		Float& idle_EL_Prev = instance[ i_trackEyeLeft_Idle_PrevValue ];
		Float& rest_ER_Prev = instance[ i_trackEyeRight_Rest_PrevValue ];
		Float& rest_EL_Prev = instance[ i_trackEyeLeft_Rest_PrevValue ];

		idle_ER_Prev = idle_ER_Curr;
		idle_EL_Prev = idle_EL_Curr;
		rest_ER_Prev = rest_ER_Curr;
		rest_EL_Prev = rest_EL_Curr;
	}

	Float& timer = instance[ i_timer ];
	const Int32 state = instance[ i_state ];

	switch ( state )
	{
	case S_None:
		{
			ASSERT( timer == 0.f );
			break;
		}

	case S_BlinkInProgress_Idle:
	case S_BlinkInProgress_Rest:
		{
			ASSERT( timer == 0.f );
			break;
		}

	case S_BlinkBlocked_Idle:
	case S_BlinkBlocked_Rest:
		{
			ASSERT( timer <= m_blinkCooldown );

			timer = Max( timer-timeDelta, 0.f );
			if ( timer < 0.001f )
			{
				if ( state == S_BlinkBlocked_Idle )
				{
					SetState( instance, S_BlinkBlockedToNone_Idle );
				}
				else
				{
					SetState( instance, S_BlinkBlockedToNone_Rest );
				}
				
				SetTimer( instance, 0.f );
			}
			break;
		}

	case S_BlinkBlockedToNone_Idle:
	case S_BlinkBlockedToNone_Rest:
		{
			ASSERT( timer == 0.f );
			break;
		}

	default:
		{
			ASSERT( 0 );
			SetState( instance, S_None );
			SetTimer( instance, 0.f );
			break;
		}
	}
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	CCacheBehaviorGraphOutput cachePose1( context, true );
	CCacheBehaviorGraphOutput cachePose2( context, true );

	SBehaviorGraphOutput* poseEyesIdle = cachePose1.GetPose();
	SBehaviorGraphOutput* poseRest = cachePose2.GetPose();

	if ( poseEyesIdle && poseRest )
	{
		if ( m_cachedInputIdle ) 
		{
			m_cachedInputIdle->Sample( context, instance, *poseEyesIdle );
		}

		if ( m_cachedInputRest )
		{
			m_cachedInputRest->Sample( context, instance, *poseRest );
		}

		ASSERT( output.m_numFloatTracks == poseEyesIdle->m_numFloatTracks );
		ASSERT( output.m_numFloatTracks == poseRest->m_numFloatTracks );

		for( Uint32 i=0; i<output.m_numFloatTracks; ++i )
		{
			output.m_floatTracks[i] = poseEyesIdle->m_floatTracks[i] + poseRest->m_floatTracks[i];
		}

		for ( Uint32 i=0; i<output.m_numBones; ++i )
		{
			output.m_outputPose[i].SetMul( poseEyesIdle->m_outputPose[i], poseRest->m_outputPose[i] );
		}

		//output.MergeEventsAndUsedAnims( *poseEyesIdle, *poseRest, 1.0f, 1.f );

		const Int32 trackEyeLeft_Down = instance[ i_trackEyeLeft_Down ];
		if ( trackEyeLeft_Down != -1 )
		{
			const Int32 trackEyeRight_Down = instance[ i_trackEyeRight_Down ];

			ASSERT( trackEyeLeft_Down != -1 );
			ASSERT( trackEyeRight_Down != -1 );

			const Int32 numTracks = (Int32)output.m_numFloatTracks;
			if ( trackEyeLeft_Down < numTracks && trackEyeRight_Down < numTracks )
			{
				Float& output_ER_Curr = output.m_floatTracks[ trackEyeRight_Down ];
				Float& output_EL_Curr = output.m_floatTracks[ trackEyeLeft_Down ];

				const Float idle_ER_Curr = poseEyesIdle->m_floatTracks[ trackEyeRight_Down ];
				const Float idle_EL_Curr = poseEyesIdle->m_floatTracks[ trackEyeLeft_Down ];
				const Float rest_ER_Curr = poseRest->m_floatTracks[ trackEyeRight_Down ];
				const Float rest_EL_Curr = poseRest->m_floatTracks[ trackEyeLeft_Down ];

				const Float timer = instance[ i_timer ];

				const Bool wasBlocked = instance[ i_blocked ];
				Bool& isBlocked = instance[ i_blocked ];
				isBlocked = false;

				Int32& state_Idle = instance[ i_state_Idle ];
				Int32& state_Rest = instance[ i_state_Rest ];

				{
					const Float idle_ER_Prev = instance[ i_trackEyeRight_Idle_PrevValue ];
					const Float idle_EL_Prev = instance[ i_trackEyeLeft_Idle_PrevValue ];
					const Float rest_ER_Prev = instance[ i_trackEyeRight_Rest_PrevValue ];
					const Float rest_EL_Prev = instance[ i_trackEyeLeft_Rest_PrevValue ];

					const Float areEyesClosedThr = 0.02f;

					const Bool idle_Closed_Curr = idle_ER_Curr > areEyesClosedThr && idle_EL_Curr > areEyesClosedThr;
					const Bool idle_Closed_Prev = idle_ER_Prev > areEyesClosedThr && idle_EL_Prev > areEyesClosedThr;
					const Bool idle_Down_Curr = idle_ER_Curr > idle_ER_Prev && idle_EL_Curr > idle_EL_Prev;
					const Bool idle_Closing = idle_Closed_Curr && idle_Down_Curr;

					const Bool rest_Closed_Curr = rest_ER_Curr > areEyesClosedThr && rest_EL_Curr > areEyesClosedThr;
					const Bool rest_Closed_Prev = rest_ER_Prev > areEyesClosedThr && rest_EL_Prev > areEyesClosedThr;
					const Bool rest_Down_Curr = rest_ER_Curr > rest_ER_Prev && rest_EL_Curr > rest_EL_Prev;
					const Bool rest_Closing = rest_Closed_Curr && rest_Down_Curr;

					ProcessBlinkState( state_Idle, idle_Closing, idle_Closed_Curr );
					ProcessBlinkState( state_Rest, rest_Closing, rest_Closed_Curr );
				}

				const Int32 state = instance[ i_state ];
				switch ( state )
				{
				case S_None:
					{
						if ( state_Rest == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );

							output_ER_Curr = rest_ER_Curr;
							output_EL_Curr = rest_EL_Curr;
						}
						else if ( state_Idle == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Idle );
							SetTimer( instance, 0.f );

							output_ER_Curr = idle_ER_Curr;
							output_EL_Curr = idle_EL_Curr;
						}
						break;
					}

				case S_BlinkInProgress_Rest:
					{
						output_ER_Curr = rest_ER_Curr;
						output_EL_Curr = rest_EL_Curr;

						if ( state_Rest == BS_None )
						{
							SetState( instance, S_BlinkBlocked_Rest );
							SetTimer( instance, m_blinkCooldown );
						}

						if ( state_Idle != BS_None )
						{
							isBlocked = true;
						}

						break;
					}

				case S_BlinkInProgress_Idle:
					{
						output_ER_Curr = idle_ER_Curr;
						output_EL_Curr = idle_EL_Curr;

						if ( state_Rest == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );
						}

						if ( state_Idle == BS_None )
						{
							SetState( instance, S_BlinkBlocked_Idle );
							SetTimer( instance, m_blinkCooldown );
						}

						if ( state_Rest != BS_None )
						{
							isBlocked = true;
						}

						break;
					}

				case S_BlinkBlocked_Rest:
					{
						ASSERT( timer <= m_blinkCooldown );

						if ( state_Rest == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );
						}

						output_ER_Curr = rest_ER_Curr;
						output_EL_Curr = rest_EL_Curr;

						if ( state_Idle != BS_None )
						{
							isBlocked = true;
						}
						break;
					}

				case S_BlinkBlocked_Idle:
					{
						ASSERT( timer <= m_blinkCooldown );

						if ( state_Rest == BS_BlinkDown ) // Spacial case - 'rest' blink can override 'idle' blink
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );
						}

						if ( state_Idle == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Idle );
							SetTimer( instance, 0.f );
						}

						output_ER_Curr = idle_ER_Curr;
						output_EL_Curr = idle_EL_Curr;

						if ( state_Rest != BS_None )
						{
							isBlocked = true;
						}
						break;
					}

				case S_BlinkBlockedToNone_Rest:
					{
						ASSERT( timer == 0.f );

						if ( state_Rest == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );
						}

						output_ER_Curr = rest_ER_Curr;
						output_EL_Curr = rest_EL_Curr;

						if ( state_Idle != BS_None )
						{
							isBlocked = true;
						}

						if ( state_Idle == BS_None && state_Rest == BS_None )
						{
							SetState( instance, S_None );
							SetTimer( instance, 0.f );
						}
						break;
					}

				case S_BlinkBlockedToNone_Idle:
					{
						ASSERT( timer == 0.f );

						if ( state_Rest == BS_BlinkDown ) // Spacial case - 'rest' blink can override 'idle' blink
						{
							SetState( instance, S_BlinkInProgress_Rest );
							SetTimer( instance, 0.f );
						}

						if ( state_Idle == BS_BlinkDown )
						{
							SetState( instance, S_BlinkInProgress_Idle );
							SetTimer( instance, 0.f );
						}

						output_ER_Curr = idle_ER_Curr;
						output_EL_Curr = idle_EL_Curr;

						if ( state_Rest != BS_None )
						{
							isBlocked = true;
						}

						if ( state_Idle == BS_None && state_Rest == BS_None )
						{
							SetState( instance, S_None );
							SetTimer( instance, 0.f );
						}
						break;
					}

				default:
					{
						ASSERT( 0 );
						SetState( instance, S_None );
						SetTimer( instance, 0.f );
						break;
					}
				}

				Float& instance_idle_ER_Curr = instance[ i_trackEyeRight_Idle_CurrValue ];
				Float& instance_idle_EL_Curr = instance[ i_trackEyeLeft_Idle_CurrValue ];
				Float& instance_rest_ER_Curr = instance[ i_trackEyeRight_Rest_CurrValue ];
				Float& instance_rest_EL_Curr = instance[ i_trackEyeLeft_Rest_CurrValue ];

				instance_idle_ER_Curr = idle_ER_Curr;
				instance_idle_EL_Curr = idle_EL_Curr;
				instance_rest_ER_Curr = rest_ER_Curr;
				instance_rest_EL_Curr = rest_EL_Curr;
			}
		}
	}
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::ProcessBlinkState( Int32& state, Bool isClosing, Bool isClosed ) const
{
	switch ( state )
	{
	case BS_None:
		{
			if ( isClosing )
			{
				state = BS_BlinkDown;
			}
			break;
		}

	case BS_BlinkDown:
		{
			if ( !isClosed )
			{
				state = BS_None;
			}
			else if ( !isClosing )
			{
				state = BS_BlinkUp;
			}	

			break;
		}

	case BS_BlinkUp:
		{
			if ( !isClosed )
			{
				state = BS_None;
			}
			break;
		}

	default:
		{
			ASSERT( 0 );
			state = BS_None;
			break;
		}
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphMimicBlinkControllerNode_Blend::GetBlinkStateDesc( CBehaviorGraphInstance& instance, String& desc ) const
{
	const Int32 s = instance[ i_state ];
	const Float t = instance[ i_timer ];

	if ( s == S_None )
	{
		return false;
	}

	switch ( s )
	{
	case S_BlinkInProgress_Idle:
		{
			desc = String::Printf( TXT("BlinkInProgress - Idle") );
			break;
		}
	case S_BlinkInProgress_Rest:
		{
			desc = String::Printf( TXT("BlinkInProgress - Rest") );
			break;
		}

	case S_BlinkBlocked_Idle:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlocked Idle - %1.2f [%1.1f] - <BLINK IS BLOCKED NOW>"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlocked Idle - %1.2f [%1.1f]"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			break;
		}
	case S_BlinkBlocked_Rest:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlocked Rest - %1.2f [%1.1f] - <BLINK IS BLOCKED NOW>"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlocked Rest - %1.2f [%1.1f]"), t, ( 1.f - ( (t) / m_blinkCooldown ) ) * 100.f );
			}
			break;
		}

	case S_BlinkBlockedToNone_Idle:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlockedToNone Idle - <BLINK IS BLOCKED NOW>") );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlockedToNone Idle") );
			}
			break;
		}
	case S_BlinkBlockedToNone_Rest:
		{
			if ( instance[ i_blocked ] )
			{
				desc = String::Printf( TXT("BlinkBlockedToNone Rest - <BLINK IS BLOCKED NOW>") );
			}
			else
			{
				desc = String::Printf( TXT("BlinkBlockedToNone Rest") );
			}
			break;
		}
	}

	const Int32 state_Idle = instance[ i_state_Idle ];
	const Int32 state_Rest = instance[ i_state_Rest ];

	switch ( state_Idle )
	{
	case BS_None:
		desc += String::Printf( TXT("\nIdle [None]") );
		break;

	case BS_BlinkDown:
		desc += String::Printf( TXT("\nIdle [Down]") );
		break;

	case BS_BlinkUp:
		desc += String::Printf( TXT("\nIdle [ Up ]") );
		break;
	}

	switch ( state_Rest )
	{
	case BS_None:
		desc += String::Printf( TXT("\nRest [None]") );
		break;

	case BS_BlinkDown:
		desc += String::Printf( TXT("\nRest [Down]") );
		break;

	case BS_BlinkUp:
		desc += String::Printf( TXT("\nRest [ Up ]") );
		break;
	}

	return true;
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( EyesIdle ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Rest ) ) );
}

#endif

void CBehaviorGraphMimicBlinkControllerNode_Blend::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	CSyncInfo firstSyncInfo;
	CSyncInfo secondSyncInfo;

	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->GetSyncInfo( instance, firstSyncInfo );
	}

	if ( m_cachedInputRest )
	{
		m_cachedInputRest->GetSyncInfo( instance, secondSyncInfo );
	}

	info.SetInterpolate( firstSyncInfo, secondSyncInfo, 0.5f ); // TODO
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->SynchronizeTo( instance, info );
	}

	if ( m_cachedInputRest )
	{
		m_cachedInputRest->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphMimicBlinkControllerNode_Blend::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedInputIdle && m_cachedInputIdle->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	if ( m_cachedInputRest && m_cachedInputRest->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}


void CBehaviorGraphMimicBlinkControllerNode_Blend::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->Activate( instance );
	}

	if ( m_cachedInputRest )
	{
		m_cachedInputRest->Activate( instance );
	}
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->Deactivate( instance );
	}

	if ( m_cachedInputRest )
	{
		m_cachedInputRest->Deactivate( instance );
	}
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputIdle = CacheMimicBlock( CNAME( EyesIdle ) );
	m_cachedInputRest = CacheMimicBlock( CNAME( Rest ) );
}

void CBehaviorGraphMimicBlinkControllerNode_Blend::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputIdle )
	{
		m_cachedInputIdle->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedInputRest )
	{
		m_cachedInputRest->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphLipsyncControlValueCorrectionNode );

CBehaviorGraphLipsyncControlValueCorrectionNode::CBehaviorGraphLipsyncControlValueCorrectionNode()
	: m_lipsyncControlTrack( -1 )
	, m_smoothTime( 0.15f )
{
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	//compiler << i_values;
	compiler << i_timer;
	//compiler << i_lastTimeDelta;
	compiler << i_startCorrEventId;
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_startCorrEventId ] = instance.GetEventId( m_startCorrEventName );

	//TDynArray< Float >& vals = instance[ i_values ];
	//vals.Resize( 10 );

	InternalReset( instance );
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	/*TDynArray< Float >& vals = instance[ i_values ];

	const Uint32 num = vals.Size();
	for ( Uint32 i=0; i<num-1; ++i )
	{
		vals[ i ] = vals[ i+1 ];
	}*/

	Float& timer = instance[ i_timer ];
	if ( timer > 0.f )
	{
		timer = Max( 0.f, timer-timeDelta );
	}

	//instance[ i_lastTimeDelta ] = timeDelta;
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_lipsyncControlTrack != -1 && (Int32)output.m_numFloatTracks > m_lipsyncControlTrack )
	{
		// Copy new value
		/*TDynArray< Float >& vals = instance[ i_values ];
		const Uint32 num = vals.Size();

		const Float value = output.m_floatTracks[ m_lipsyncControlTrack ];
		vals[ num-1 ] = value;

		// Timer
		Float& timer = instance[ i_timer ];

		const Bool wasBlending = timer > 0.f;
		if ( !wasBlending )
		{
			// Should we start blending?
			if ( value > 0.5f )
			{
				Bool allZeros( true );
				for ( Uint32 i=0; i<num-1; ++i )
				{
					const Float v = vals[ i ];
					if ( v > 0.01f )
					{
						allZeros = false;
						break;
					}
				}

				if ( allZeros )
				{
					StartTimer( instance );

					// This is not a deterministic code - I am sorry for that :( , we cannot lose even one frame here
					Float& lastTimeDelta = instance[ i_lastTimeDelta ];
					Float& timer = instance[ i_timer ];
					if ( timer > 0.f )
					{
						timer = Max( 0.f, timer-lastTimeDelta );
					}
					lastTimeDelta = 0.f;
				}
			}
		}
		
		const Bool isBlending = timer > 0.f;
		if ( isBlending )
		{
			const Float weight = 1.f - timer / m_smoothTime;
			RED_FATAL_ASSERT( weight >= 0.f && weight <= 1.f, "Anim ERROR" );

			output.m_floatTracks[ m_lipsyncControlTrack ] *= weight;
		}*/

		const Float timer = instance[ i_timer ];
		if ( timer > 0.f )
		{
			const Float w = 1.f - timer / m_smoothTime;
			RED_FATAL_ASSERT( w >= 0.f && w <= 1.f, "ANIM FATAL ERROR" );

			output.m_floatTracks[ m_lipsyncControlTrack ] *= w;
		}
	}
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	InternalReset( instance );
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_timer ] = 0.f;

	/*TDynArray< Float >& vals = instance[ i_values ];
	for ( Uint32 i=0; i<vals.Size(); ++i )
	{
		vals[ i ] = 0.f;
	}

	instance[ i_lastTimeDelta ] = 0.f;*/
}

void CBehaviorGraphLipsyncControlValueCorrectionNode::StartTimer( CBehaviorGraphInstance& instance ) const
{
	Float& timer = instance[ i_timer ];
	timer = m_smoothTime;
}

Bool CBehaviorGraphLipsyncControlValueCorrectionNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( event.GetEventID() == instance[ i_startCorrEventId ] )
	{
		StartTimer( instance );
		ret = true;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
