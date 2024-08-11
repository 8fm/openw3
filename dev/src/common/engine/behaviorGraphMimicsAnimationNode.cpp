/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphMimicsAnimationNode.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/entity.h"
#include "../engine/graphConnectionRebuilder.h"
#include "skeletalAnimationEntry.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "baseEngine.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsAnimationNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicsAnimationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Animation ) ) );		
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( ForcedTime ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ), false ) );
}

#endif

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicsAnimationNode::GetCaption() const
{
	if ( m_animationName.Empty() )
	{
		return TXT("Mimic animation"); 
	}

	return String::Printf( TXT("Mimic animation [ %s ]"), m_animationName.AsString().AsChar() );
}

#endif

void CBehaviorGraphMimicsAnimationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	const Float localTime = instance[ i_localTime ];
	const Float prevTime = instance[ i_prevTime ];
	const Int32 loops = instance[ i_loops ];

	if ( !animation || !animation->GetAnimation() )
	{
		const CEntity *entity = instance.GetAnimatedComponent()->GetEntity();
		const CObject *entityTemplate = entity ? entity->GetTemplate() : NULL;

 		BEH_WARN
		(
			TXT("Mimics animation '%ls' used in graph '%ls' missing in entity '%ls' (character may collapse!) "), 
 			m_animationName.AsString().AsChar(),
 			GetGraph() ? GetGraph()->GetFriendlyName().AsChar() : TXT( "[unknown]" ),
 			entityTemplate ? entityTemplate->GetFriendlyName().AsChar() : TXT( "[unknown]" )
		);
	}
	else if ( context.HasMimic() )
	{
		// Sample animation
 		Bool ret = animation->GetAnimation()->Sample( localTime,
									output.m_numBones,
									output.m_numFloatTracks,
									output.m_outputPose, 
 									output.m_floatTracks );

		output.Touch();

		if ( !ret )
		{
			return;
		}

		const Float eventsAlpha = 1.0f;

		// Add events from animation to final output pose
		animation->GetEventsByTime( prevTime, localTime, loops, eventsAlpha, NULL, &output );

		output.AppendUsedAnim( SBehaviorUsedAnimationData( animation, localTime ) );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsEventAnimationNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicsEventAnimationNode::GetCaption() const
{
	if ( m_animationName.Empty() )
	{
		return TXT("Mimic event animation"); 
	}

	return String::Printf( TXT("Mimic event animation [ %s ]"), m_animationName.AsString().AsChar() );
}

void CBehaviorGraphMimicsEventAnimationNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	m_loopPlayback = false;
}

void CBehaviorGraphMimicsEventAnimationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ), false ) );
}

#endif

CBehaviorGraphMimicsEventAnimationNode::CBehaviorGraphMimicsEventAnimationNode()
{
	m_loopPlayback = false;
}

void CBehaviorGraphMimicsEventAnimationNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_running;
	compiler << i_event;
	compiler << i_animationFinished;
}

void CBehaviorGraphMimicsEventAnimationNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_running ] = false;
	instance[ i_animationFinished ] = false;
	instance[ i_event ] = instance.GetEventId( m_eventName );
}

void CBehaviorGraphMimicsEventAnimationNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	Bool& running = instance[ i_running ];
	Bool& animationFinished = instance[ i_animationFinished ];

	if ( running )
	{
		TBaseClass::OnUpdate( context, instance, timeDelta );

		if ( animationFinished )
		{
			InternalReset( instance );

			animationFinished = false;
			running = false;
		}
	}
	else
	{
		ASSERT( !animationFinished );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	const Bool running = instance[ i_running ];
	if ( running )
	{
		TBaseClass::Sample( context, instance, output );
	}
}

Bool CBehaviorGraphMimicsEventAnimationNode::HasAnimation( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animation ] != nullptr;
}

Bool CBehaviorGraphMimicsEventAnimationNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode )
	{
		ret |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	if ( HasAnimation( instance ) && event.GetEventID() == instance[ i_event ] )
	{
		instance[ i_running ] = true;

		ret = true;
	}

	return ret;
}

void CBehaviorGraphMimicsEventAnimationNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_running ] = false;
	instance[ i_animationFinished ] = false;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
	else
	{
		TBaseClass::GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
	else
	{
		TBaseClass::SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphMimicsEventAnimationNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
}

CBehaviorGraph* CBehaviorGraphMimicsEventAnimationNode::GetParentGraph() 
{ 
	return GetGraph();
}

void CBehaviorGraphMimicsEventAnimationNode::OnAnimationFinished( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnAnimationFinished( instance );

	instance[ i_animationFinished ] = true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsGeneratorNode );

CBehaviorGraphMimicsGeneratorNode::CBehaviorGraphMimicsGeneratorNode()
{
}

CSkeleton* CBehaviorGraphMimicsGeneratorNode::GetBonesSkeleton( CAnimatedComponent* component ) const 
{ 
	return component->GetMimicSkeleton(); 
}

void CBehaviorGraphMimicsGeneratorNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_trackIndex;
	compiler << i_weight;
}

void CBehaviorGraphMimicsGeneratorNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_trackIndex ] = -1;
	instance[ i_weight ] = m_weight;

	CacheTrack( instance );
}

void CBehaviorGraphMimicsGeneratorNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_trackIndex );
	INST_PROP( i_weight );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicsGeneratorNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Pose ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

void CBehaviorGraphMimicsGeneratorNode::OnPropertyPostChange( IProperty *prop )
{
// 	if ( prop->GetName() == TXT("trackName") )
// 	{
// 		CacheTrack();
// 	}
}

#endif

void CBehaviorGraphMimicsGeneratorNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicsGenerator );
	// Update variable
	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedWeightVariableNode->GetValue( instance );
	}

	if ( m_cachedPoseNumVariableNode )
	{
		m_cachedPoseNumVariableNode->Update( context, instance, timeDelta );

		instance[ i_trackIndex ] = (Int32)Clamp< Float >( m_cachedPoseNumVariableNode->GetValue( instance ), 0.f, 1000.f );
	}
}

void CBehaviorGraphMimicsGeneratorNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
}

void CBehaviorGraphMimicsGeneratorNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
}

Bool CBehaviorGraphMimicsGeneratorNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	// Not handled
	return false;
}


void CBehaviorGraphMimicsGeneratorNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Activate( instance );
	}

	if ( m_cachedPoseNumVariableNode )
	{
		m_cachedPoseNumVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMimicsGeneratorNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Deactivate( instance );
	}

	if ( m_cachedPoseNumVariableNode )
	{
		m_cachedPoseNumVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicsGeneratorNode::CacheTrack( CBehaviorGraphInstance& instance ) const
{
	// Reset
	instance[ i_trackIndex ] = -1;

	//dex++: implemented using common interface
	CSkeleton* mimicsSkeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();
	if ( NULL != mimicsSkeleton )
	{
		const Uint32 numTracks = mimicsSkeleton->GetTracksNum();
		for ( Uint32 i=0; i<numTracks; i++ )
		{
			const AnsiChar* trackName = mimicsSkeleton->GetTrackNameAnsi(i);
			if ( 0 == AUniAnsiStrCmp( m_trackName.AsChar(), trackName ) )
			{
				instance[ i_trackIndex ] = i;

				break;
			}
		}
	}
	//dex--
}

void CBehaviorGraphMimicsGeneratorNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedWeightVariableNode = CacheValueBlock( TXT("Weight") );
	m_cachedPoseNumVariableNode = CacheValueBlock( TXT("Pose") );
}

void CBehaviorGraphMimicsGeneratorNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedPoseNumVariableNode )
	{
		m_cachedPoseNumVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicsGeneratorNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	const Int32 trackIndex = instance[ i_trackIndex ];

	if ( context.HasMimic() && (Int32)output.m_numFloatTracks > trackIndex && trackIndex != -1 )
	{
		output.m_floatTracks[ trackIndex ] = instance[ i_weight ];
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EMimicGeneratorType );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicsModifierNode );

void CBehaviorGraphMimicsModifierNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_weight;
	compiler << i_params;
}

void CBehaviorGraphMimicsModifierNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );
	instance[ i_weight ] = 1.0;
	instance[ i_params ].Resize( GetParamsNum() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicsModifierNode::OnRebuildSockets()
{
	TBaseClass::OnRebuildSockets();
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

String CBehaviorGraphMimicsModifierNode::GetCaption() const 
{ 
	switch ( m_type )
	{
	case MGT_Null:
		return String( TXT("Generator [ Null ]") ); 
	case MGT_Damp:
		return String( TXT("Modifier [ Damp ]") ); 
	case MGT_Cheeks:
		return String( TXT("Modifier [ Cheeks Constraint ]") ); 
	case MGT_HeadSin:
		return String( TXT("Modifier [ Head sin ]") ); 

	default:
		return String( TXT("NO CAPTION !!!") ); 
	}
}

#endif

void CBehaviorGraphMimicsModifierNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicsModifier );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update variable
	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedWeightVariableNode->GetValue( instance );
	}
}

void CBehaviorGraphMimicsModifierNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Activate( instance );
	}
}

void CBehaviorGraphMimicsModifierNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicsModifierNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedWeightVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphMimicsModifierNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedWeightVariableNode )
	{
		m_cachedWeightVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphMimicsModifierNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( context.HasMimic() )
	{
		Float weight = instance[ i_weight ];
		TDynArray< Float >& params = instance[ i_params ];

		switch ( m_type )
		{
		case MGT_Null:
			ModifyFloatTracks_Null( output, weight, params );
			break;
		case MGT_Damp:
			ModifyFloatTracks_Damp( output, weight, params  );
			break;
		case MGT_Cheeks:
			ModifyFloatTracks_Cheeks( output, weight, params  );
			break;
		case MGT_HeadSin:
			ModifyFloatTracks_HeadSin( output, weight, params  );
			break;
		default:
			ASSERT( 0 );
			break;
		}
	}
}

Uint32 CBehaviorGraphMimicsModifierNode::GetParamsNum() const
{
	switch ( m_type )
		{
		case MGT_Cheeks:
			return 1;
		default:
			return 0;
		}
}

void CBehaviorGraphMimicsModifierNode::ModifyFloatTracks_Null( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const
{
	ASSERT( params.Size() == 0 );
	//for ( Uint32 i=0; i<output.m_numFloatTracks; ++i )
	//{
	//	output.m_floatTracks[ i ] = output.m_floatTracks[ i ] * weight;
	//}
}

void CBehaviorGraphMimicsModifierNode::ModifyFloatTracks_Damp( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const
{
	ASSERT( params.Size() == 0 );
	//Float &lEyeTrack = output.m_floatTracks[8];
	//for ( Uint32 i=0; i<output.m_numFloatTracks; ++i )
	//{
	//	output.m_floatTracks[ i ] = output.m_floatTracks[ i ] * weight;
	//}
}

void CBehaviorGraphMimicsModifierNode::ModifyFloatTracks_Cheeks( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const
{
	ASSERT( params.Size() == 1 );

	Float &lLipUpper = output.m_floatTracks[8];
	Float &lWhistle = output.m_floatTracks[16];
	Float &lCheekLeft = output.m_floatTracks[29];
	Float &lCheekRight = output.m_floatTracks[30];
	//Float &lSquintLeft = output.m_floatTracks[38];
	//Float &lSquintRight = output.m_floatTracks[39];

	Float lCheekAdditive = 0.2f * lLipUpper +  0.8f * lWhistle;
	//Float lSquintAdditive = lCheekAdditive;

	lCheekAdditive = lCheekAdditive * 0.8f;
	if (lCheekAdditive > 1.f)
	{
		lCheekAdditive = 1.f;
	}
	if (lCheekAdditive < -0.5f )
	{
		lCheekAdditive = -0.5f;
	}
	lCheekLeft = weight * (lCheekLeft + lCheekAdditive);
	lCheekRight = weight * (lCheekRight + lCheekAdditive);

/*

	lSquintAdditive = lSquintAdditive * 0.8f;
	if (lSquintAdditive > 1.f)
	{
		lSquintAdditive = 1.f;
	}
	if (lSquintAdditive < -1.f )
	{
		lSquintAdditive = -1.f;
	}

	lSquintLeft = weight * (lSquintLeft + lSquintAdditive);
	lSquintRight = weight * (lSquintRight + lSquintAdditive);


*/
	params[ 0 ] = 1.f;





}

void CBehaviorGraphMimicsModifierNode::ModifyFloatTracks_HeadSin( SBehaviorGraphOutput &output, Float weight, TDynArray< Float >& params ) const
{
	if ( output.m_numFloatTracks > 0 )
	{
		static Float time = 0.f;
		time += GEngine->GetLastTimeDelta();
		Float var = sinf( 2.f * M_PI * time );
		output.m_floatTracks[ output.m_numFloatTracks - 1 ] = var;
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
