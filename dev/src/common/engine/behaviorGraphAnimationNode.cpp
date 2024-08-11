/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "graphConnectionRebuilder.h"
#include "skeletalAnimationEntry.h"
#include "skeletalAnimationSet.h"
#include "skeletalAnimationContainer.h"
#include "entity.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animSyncInfo.h"
#include "animatedComponent.h"
#include "behaviorGraphUtils.inl"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimationSyncToken );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationNode );

RED_DEFINE_STATIC_NAME( CurrentTime );

CBehaviorGraphAnimationNode::CBehaviorGraphAnimationNode()	
	: m_loopPlayback( true )
	, m_playbackSpeed( 1.0f )
	, m_applyMotion( true )
	, m_extractMotionTranslation( true )
	, m_extractMotionRotation( true )
	, m_fireLoopEvent( false )
	, m_useFovTrack( false )
	, m_useDofTrack( false )
	, m_autoFireEffects( true )
	, m_gatherEvents( true )
	, m_gatherSyncTokens( false )
{
}

void CBehaviorGraphAnimationNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_localTime;
	compiler << i_prevTime;
	compiler << i_loops;
	compiler << i_loopEventFired;
	compiler << i_animation;
	compiler << i_firstUpdate;
	compiler << i_syncTokens;
	compiler << i_effectsFired;
	compiler << i_internalBlendTime;
	compiler << i_timeDelta;
	compiler << i_setGroupIndex;
}

void CBehaviorGraphAnimationNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_localTime ] = 0.f;
	instance[ i_prevTime ] = 0.f;
	instance[ i_loops ] = 0;
	instance[ i_loopEventFired ] = false;
	instance[ i_animation ] = NULL;
	instance[ i_firstUpdate ] = false;
	instance[ i_effectsFired ] = false;
	instance[ i_internalBlendTime ] = 0.f;
	instance[ i_timeDelta ] = 0.f;
	instance[ i_setGroupIndex ] = -1;

	RefreshAnimation( instance, GetAnimationName() );
}

void CBehaviorGraphAnimationNode::OnUpdateAnimationCache( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnUpdateAnimationCache( instance );

	RefreshAnimation( instance, GetAnimationName() );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Animation ) ) );		
	CreateSocket( CBehaviorGraphVariableOutputSocketSpawnInfo( CNAME( CurrentTime ), false ) );		
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( ForcedTime ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Speed ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( ForcedProp ), false ) );
}

String CBehaviorGraphAnimationNode::GetCaption() const
{
	if ( m_animationName.Empty() )
		return TXT("Animation"); 

	return String::Printf( TXT("Animation [ %s ]"), m_animationName.AsString().AsChar() );
}

#endif

void CBehaviorGraphAnimationNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_localTime );
	INST_PROP( i_prevTime );
	INST_PROP( i_loops );
	INST_PROP( i_loopEventFired );
	INST_PROP( i_firstUpdate );
	INST_PROP( i_animation );
	INST_PROP( i_effectsFired );
	INST_PROP( i_internalBlendTime );
	INST_PROP( i_timeDelta );
	INST_PROP( i_setGroupIndex );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Color CBehaviorGraphAnimationNode::GetTitleColor() const
{
	return Color( 255, 64, 64 );
}

#endif

CSkeletalAnimationSetEntry* CBehaviorGraphAnimationNode::GetAnimation( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animation ];
}

void CBehaviorGraphAnimationNode::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	if ( m_animationName != CName::NONE )
	{
		anims.PushBackUnique( m_animationName );
	}
}

Float CBehaviorGraphAnimationNode::GetValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_localTime ];
}

Float CBehaviorGraphAnimationNode::GetAnimTime( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_localTime ];
}

Float CBehaviorGraphAnimationNode::GetAnimDuration( CBehaviorGraphInstance& instance ) const
{
	const CSkeletalAnimationSetEntry* anim = GetAnimation( instance );
	return anim && anim->GetAnimation() ? anim->GetAnimation()->GetDuration() : 0.f;
}

Float CBehaviorGraphAnimationNode::GetAnimProgress( CBehaviorGraphInstance& instance ) const
{
	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	Float localTime = instance[ i_localTime ];

	if ( animation && animation->GetAnimation() && animation->GetAnimation()->GetDuration() > 0.f )
	{
		ASSERT( localTime >= 0.f && localTime <= animation->GetAnimation()->GetDuration() );
		return localTime / animation->GetAnimation()->GetDuration();
	}
	else
	{
		return 0.f;
	}
}

void CBehaviorGraphAnimationNode::SetAnimationName( const CName &name )
{
	m_animationName = name;
}

void CBehaviorGraphAnimationNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedForceTimeNode = CacheValueBlock( TXT("ForcedTime") );
	m_cachedSpeedTimeNode = CacheValueBlock( TXT("Speed") );
	m_cachedForcePropNode = CacheValueBlock( TXT("ForcedProp") );
}

void CBehaviorGraphAnimationNode::SetAnimTime( CBehaviorGraphInstance& instance, Float time ) const
{
	instance[ i_prevTime ] = time;
	instance[ i_localTime ] = time;
}

void CBehaviorGraphAnimationNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( Animation );

	if ( m_cachedForceTimeNode )
	{
		m_cachedForceTimeNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedSpeedTimeNode )
	{
		m_cachedSpeedTimeNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedForcePropNode )
	{
		m_cachedForcePropNode->Update( context, instance, timeDelta );
	}

	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	const CSkeletalAnimation* skAnimation = animation ? animation->GetAnimation() : NULL;

	Float& prevTime = instance[ i_prevTime ];
	Float& localTime = instance[ i_localTime ];
	Int32& loops = instance[ i_loops ];
	Bool& loopEventFired = instance[ i_loopEventFired ];
	Bool& firstUpdate = instance[ i_firstUpdate ];
	Bool& effectsFired = instance[ i_effectsFired ];
	instance[ i_timeDelta ] = timeDelta;

	ASSERT( !Red::Math::NumericalUtils::IsNan( localTime ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( prevTime) );

	// multiply by sync data multiplier
	if ( skAnimation )
	{
		//WARN_CONDITION( !skAnimation->IsCompressed() && GGame->GetGameplayConfig().m_logMissingAnimations, TXT("Animation '%ls' is not compressed - please reimport it. Animset: '%ls'."), animation->GetName().AsChar(), animation->GetAnimSet()->GetDepotPath().AsChar() );
		//DATA_ASSERT( skAnimation->IsCompressed(), FC_Minor, animation->GetParentResource()->GetFile()->GetDepotPath().AsChar(), TXT( "Animation" ), TXT("Animation '%ls' is not compressed - please reimport it. Animset: '%ls'."), animation->GetName().AsChar(), animation->GetAnimSet()->GetDepotPath().AsChar() );
		
		Float animDuration = GetDuration( instance );
		ASSERT( animDuration != 0.f, TXT("There's an animation \"%s\" in \"%s\" with zero time."),
			instance[ i_animation ] && instance[ i_animation ]->GetAnimation()? instance[ i_animation ]->GetAnimation()->GetName().AsChar() : TXT("--"),
			instance[ i_animation ] && instance[ i_animation ]->GetAnimSet()? instance[ i_animation ]->GetAnimSet()->GetDepotPath().AsChar() : TXT("--"));
		if ( animDuration <= 0.f ) // Will loop infinitely without an anim.
		{
			return;
		}

		// Check speed
		Float playbackSpeed = GetPlaybackSpeed( instance );
		if ( m_cachedSpeedTimeNode )
		{
			playbackSpeed = m_cachedSpeedTimeNode->GetValue( instance );
		}

		// Advance time
		if ( m_cachedForceTimeNode )
		{
			localTime = Min( m_cachedForceTimeNode->GetValue( instance ), animDuration );
			prevTime = localTime;
		}
		else if ( m_cachedForcePropNode )
		{
			Float prop = Clamp( m_cachedForcePropNode->GetValue( instance ), 0.f, 1.f );
			localTime = Min( animDuration * prop, animDuration );
			prevTime = localTime;
		}
		else
		{
			prevTime = localTime;
			localTime += timeDelta * playbackSpeed;
		}

		loops = 0;

		// Start of animation
		if ( firstUpdate )
		{
			// Reset flag
			firstUpdate = false;

			FirstUpdate( instance );
		}

		if ( localTime >= animDuration )
		{
			if ( IsLooped( instance ) )
			{			
				while( localTime >= animDuration )
				{
					localTime -= animDuration;
					++loops;					
				}

				if (m_playbackSpeed >= 0.0f) // just in case we would start at the end for any reason
				{
					OnAnimationFinished( instance );
				}

				// clear fired flag, so that event can be fired again on next loop
				loopEventFired = false;
			}
			else
			{
				localTime = animDuration;
				if (m_playbackSpeed >= 0.0f) // just in case we would start at the end for any reason
				{
					OnAnimationFinished( instance );
				}
			}
		}

		if ( localTime < 0.0f )
		{
			if ( IsLooped( instance ) )
			{
				while( localTime < 0.0f )
				{
					localTime += animDuration;
					--loops;					
				}

				if (m_playbackSpeed < 0.0f) // just in case we would start before start for any reason
				{
					OnAnimationFinished( instance );
				}

				// clear fired flag, so that event can be fired again on next loop
				loopEventFired = false;
			}
			else
			{
				localTime = 0.0f;
				if (m_playbackSpeed < 0.0f) // just in case we would start before start for any reason
				{
					OnAnimationFinished( instance );
				}
			}
		}
	
		if ( !effectsFired && m_autoFireEffects )
		{
			if ( localTime != prevTime )
			{
				// Add post action
				CAnimationEffectAction* action = context.GetOneFrameAllocator().CreateOnStack< CAnimationEffectAction >();
				
				if ( action )
				{
					action->m_effect = skAnimation->GetName();
					action->m_time = localTime;
					context.AddPostAction( action );
					effectsFired = true; 
				}
			}
		}

		TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
		for ( Uint32 i=0; i<tokens.Size(); ++i )
		{
			CSyncInfo syncInfo;
			GetSyncInfo( instance, syncInfo );
			tokens[i]->Sync( skAnimation->GetName(), syncInfo, 1.f );
			if ( !tokens[ i ]->IsValid() )
			{
				tokens[i]->Reset();
				delete tokens[i];
				tokens.RemoveAt( i );
				--i;
			}
		}
	}
}

void CBehaviorGraphAnimationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( Animation );

	ANIM_NODE_PRE_SAMPLE

	// Pre physics tick
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	const Float localTime = instance[ i_localTime ];
	const Float prevTime = instance[ i_prevTime ];
	const Int32 loops = instance[ i_loops ];
	Float & internalBlendTime = instance[ i_internalBlendTime ];
	const Float timeDelta = instance[ i_timeDelta ];
	Int32 eventGroupRangeIndex = instance[ i_setGroupIndex ];

	const CSkeletalAnimation* skAnimation = animation ? animation->GetAnimation() : NULL;

	ASSERT( !Red::Math::NumericalUtils::IsNan( localTime ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( prevTime) );

	if ( skAnimation )
	{
		Bool ret = false;

#ifdef DISABLE_SAMPLING_AT_LOD3
		if ( context.GetLodLevel() >= BL_Lod3 )
		{
			//..
		}
		else 
#endif
		{
			const CSkeleton* skeleton = animatedComponent->GetSkeleton();
			ret = UpdateAndSampleBlendWithCompressedPose( animation, timeDelta, internalBlendTime, localTime, context, output, skeleton );
			ASSERT( ret );
		}

		if ( ret )
		{
			output.Touch();
		}		

		// Float tracks
		if ( !UseFovTrack( instance ) && output.m_numFloatTracks > SBehaviorGraphOutput::FTT_FOV )		
			output.m_floatTracks[SBehaviorGraphOutput::FTT_FOV] = 0.f;

		if ( !UseDofTrack( instance ) && output.m_numFloatTracks > SBehaviorGraphOutput::FTT_DOF_BlurDistNear )	
		{
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_Override] = 0.f;
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_FocusDistFar] = 0.f;
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_BlurDistFar] = 0.f;
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_Intensity] = 0.f;
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_FocusDistNear] = 0.f;
			output.m_floatTracks[SBehaviorGraphOutput::FTT_DOF_BlurDistNear] = 0.f;
		}

		// Pose correction
		if ( ShouldDoPoseCorrection() && context.ShouldCorrectPose() )
		{
			context.SetPoseCorrection( output );
		}

		// Trajectory extraction
		if ( animatedComponent->UseExtractedTrajectory() && animatedComponent->HasTrajectoryBone() )
		{
			output.ExtractTrajectory( animatedComponent );
		}
		else if ( output.m_numBones > 0 )
		{
#ifdef USE_HAVOK_ANIMATION // VALID
			output.m_outputPose[ 0 ].m_rotation.normalize();
#else
			output.m_outputPose[ 0 ].Rotation.Normalize();
#endif
		}

		// Motion extraction
		if ( skAnimation->HasExtractedMotion() && ApplyMotion( instance ) )
		{		
			output.m_deltaReferenceFrameLocal = skAnimation->GetMovementBetweenTime( prevTime, localTime, loops );
			if ( ! m_extractMotionTranslation )
			{
				output.m_deltaReferenceFrameLocal.SetTranslation( AnimVector4::ZEROS );
			}
			if ( ! m_extractMotionRotation )
			{
				output.m_deltaReferenceFrameLocal.SetRotation( AnimQuaternion::IDENTITY );
			}
		}
		else
		{
			output.m_deltaReferenceFrameLocal = AnimQsTransform::IDENTITY;
		}

		// Events
		if ( m_gatherEvents )
		{
			const Float eventsAlpha = 1.0f;
			const CName sfxTag =
#ifdef USE_EXT_ANIM_EVENTS
			animatedComponent && animatedComponent->GetEntity() ? animatedComponent->GetEntity()->GetSfxTag() : CName::NONE;
#else
			CName::NONE;
#endif

			// Gather events from this animation
			animation->GetEventsByTime( prevTime, localTime, loops, eventsAlpha, NULL, &output, sfxTag, eventGroupRangeIndex );
		}
		Float playbackSpeed = GetPlaybackSpeed( instance );
		if ( m_cachedSpeedTimeNode )
		{
			playbackSpeed = m_cachedSpeedTimeNode->GetValue( instance );
		}
		output.AppendUsedAnim( SBehaviorUsedAnimationData( animation, localTime, 1.0f, playbackSpeed, m_loopPlayback, instance[ i_firstUpdate ] ) );
	}
	else
	{
		/*
		CEntity *entity = animatedComponent ? animatedComponent->GetEntity() : NULL;
		CObject *entityTemplate = entity ? entity->GetTemplate() : NULL;
		WARN_ENGINE( TXT("Animation %s used in graph %s missing in entity %s (character may collapse!) "), 
															m_animationName.AsChar(),
															GetGraph() ? GetGraph()->GetFriendlyName().AsChar() : TXT("[unknown]"),
															entityTemplate ? entityTemplate->GetFriendlyName().AsChar() : TXT("[unknown]" ) );*/														
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphAnimationNode::FirstUpdate( CBehaviorGraphInstance& instance ) const
{
	// Collect animation playback synchronization tokens
	CollectSyncTokens( instance );
}

Float CBehaviorGraphAnimationNode::GetDuration( CBehaviorGraphInstance& instance ) const
{
	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	return animation ? animation->GetDuration() : 1.f;
}

void CBehaviorGraphAnimationNode::SendMissingAnimationEvents( CBehaviorGraphInstance& instance ) const
{
	/*const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];

	if ( animation && animation->GetAnimation() )
	{
		const Float duration = animation->GetDuration();
		const Float localTime = instance[ i_localTime ];

		if ( localTime < duration )
		{

		}

		if ( m_gatherEvents )
		{
			CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

			const Float eventsAlpha = 1.0f;

			TDynArray< CAnimationEventFired > events;
			animation->GetEventsByTime( animatedComponent->GetEntity(), localTime, duration, 0, animatedComponent, eventsAlpha, &events, NULL );
		}
		Float playbackSpeed = GetPlaybackSpeed( instance );
		if ( m_cachedSpeedTimeNode )
		{
		playbackSpeed = m_cachedSpeedTimeNode->GetValue( instance );
		}
		output.AppendUsedAnim( SBehaviorUsedAnimationData( animation, localTime, 1.0f, playbackSpeed, m_loopPlayback ) );
	}
	*/
}

void CBehaviorGraphAnimationNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	CSkeletalAnimationSetEntry* animation = instance[ i_animation ];

	if ( animation && animation->GetAnimation() )
	{
		// Get times
		info.m_prevTime		= instance[ i_prevTime ];
		info.m_currTime		= instance[ i_localTime ];
		info.m_totalTime	= animation->GetAnimation()->GetDuration();

		//RED_MESSAGE( "CBehaviorGraphAnimationNode::GetSyncInfo - TODO" )
		if( info.m_wantSyncEvents == true )
		{
			animation->GetAllEvents( info.m_syncEvents );
		}
	}
}

void CBehaviorGraphAnimationNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	instance[ i_localTime ] = info.m_currTime;
	instance[ i_prevTime ]	= info.m_prevTime;			//< doesn't really matter, should be updated during update anyway
	instance[ i_loops ]		= 0;
}

void CBehaviorGraphAnimationNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_localTime ]			= 0.0f;
	instance[ i_prevTime ]			= 0.0f;
	instance[ i_loops ]				= 0;
	instance[ i_loopEventFired ]	= false;
	instance[ i_firstUpdate	]		= true;
	instance[ i_effectsFired ]		= false;
}

void CBehaviorGraphAnimationNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphAnimationNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( ShouldAddAnimationUsage() )
	{
		if ( CSkeletalAnimationSetEntry* animEntry = instance[ i_animation ] )
		{
			if ( CSkeletalAnimation* anim = animEntry->GetAnimation() )
			{
				anim->AddUsage();
			}
		}
	}

	InternalReset( instance );

	if ( m_cachedForceTimeNode )
	{
		m_cachedForceTimeNode->Activate( instance );
	}

	if ( m_cachedSpeedTimeNode )
	{
		m_cachedSpeedTimeNode->Activate( instance );
	}

	if ( m_cachedForcePropNode )
	{
		m_cachedForcePropNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	SendMissingAnimationEvents( instance );

	if ( m_gatherSyncTokens )
	{
		ClearSyncTokens( instance );
	}

	TBaseClass::OnDeactivated( instance );

	if ( m_cachedForceTimeNode )
	{
		m_cachedForceTimeNode->Deactivate( instance );
	}

	if ( m_cachedSpeedTimeNode )
	{
		m_cachedSpeedTimeNode->Deactivate( instance );
	}

	if ( m_cachedForcePropNode )
	{
		m_cachedForcePropNode->Deactivate( instance );
	}

	if ( ShouldAddAnimationUsage() )
	{
		if ( CSkeletalAnimationSetEntry* animEntry = instance[ i_animation ] )
		{
			if ( CSkeletalAnimation* anim = animEntry->GetAnimation() )
			{
				anim->ReleaseUsage();
			}
		}
	}
}

void CBehaviorGraphAnimationNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedForceTimeNode )
	{
		m_cachedForceTimeNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedSpeedTimeNode )
	{
		m_cachedSpeedTimeNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedForcePropNode )
	{
		m_cachedForcePropNode->ProcessActivationAlpha( instance, alpha );
	}
}

Bool CBehaviorGraphAnimationNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];
	const CSkeletalAnimation* skAnimation = animation ? animation->GetAnimation() : NULL;
	if ( skAnimation )
	{
		skAnimation->Touch();
		skAnimation->Preload();
		return skAnimation->IsFullyLoaded();
	}
	return true;
}

void CBehaviorGraphAnimationNode::OnAnimationFinished( CBehaviorGraphInstance& instance ) const
{
	FireLoopEvent( instance );
}

void CBehaviorGraphAnimationNode::FireLoopEvent( CBehaviorGraphInstance& instance ) const
{
	if ( !m_fireLoopEvent )
		return;

	if ( instance[ i_loopEventFired ] )
		return;

	if ( instance.GenerateEvent( m_loopEventName ) )
	{
		instance[ i_loopEventFired ] = true;
	}

	/*
#ifndef NO_EDITOR_EVENT_SYSTEM
	// Generate some internal event, used to sync editor effect
	if ( GIsEditor )
	{
		String message(TXT("Animated node generated behaviour event "));
		message += m_loopEventName.AsChar();

		SBehaviorGraphAnimEventInfo eventInfo;
		eventInfo.m_message = message;
		eventInfo.m_eventName = m_loopEventName;
		eventInfo.m_entity = instance.GetAnimatedComponent()->GetEntity();
		SEvents::GetInstance().DispatchEvent( CNAME( BehaviorEventGenerated ), CreateEventData( eventInfo ) );
	}
#endif
	*/
}

void CBehaviorGraphAnimationNode::RefreshAnimation( CBehaviorGraphInstance& instance, const CName& animName ) const
{
	if ( IsActive( instance ) )
	{
		if ( instance[ i_animation ] && instance[ i_animation ]->GetAnimation() )
		{
			instance[ i_animation ]->GetAnimation()->ReleaseUsage();
		}
	}

	if ( instance.GetAnimatedComponent() )
	{
		CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
		instance[ i_animation ] = cont->FindAnimationRestricted( animName );
		const CName& tag = instance.GetAnimatedComponent()->GetEntity()->GetSfxTag();
		if( tag != CName::NONE && instance[ i_animation ] )
		{
			instance[ i_setGroupIndex ] = instance[ i_animation ]->FindEventGroupRangeIndex( tag );
		}
		else
		{
			instance[ i_setGroupIndex ] = -1;
		}
	}
	else
	{
		instance[ i_animation ] = NULL;
		instance[ i_setGroupIndex ] = -1;
	}

	if ( IsActive( instance ) )
	{
		if ( instance[ i_animation ] && instance[ i_animation ]->GetAnimation() )
		{
			instance[ i_animation ]->GetAnimation()->AddUsage();
		}
	}
}

Bool CBehaviorGraphAnimationNode::IsLooped( CBehaviorGraphInstance& instance ) const
{
	return m_loopPlayback;
}

Bool CBehaviorGraphAnimationNode::ApplyMotion( CBehaviorGraphInstance& instance ) const
{
	return m_applyMotion;
}

Bool CBehaviorGraphAnimationNode::UseFovTrack( CBehaviorGraphInstance& instance ) const
{
	return m_useFovTrack;
}

Bool CBehaviorGraphAnimationNode::UseDofTrack( CBehaviorGraphInstance& instance ) const
{
	return m_useDofTrack;
}

Float CBehaviorGraphAnimationNode::GetPlaybackSpeed( CBehaviorGraphInstance& instance ) const
{
	return m_playbackSpeed;
}

Bool CBehaviorGraphAnimationNode::ShouldDoPoseCorrection() const
{
	return true;
}

Bool CBehaviorGraphAnimationNode::ShouldAddAnimationUsage() const
{
	return true;
}

CBehaviorGraph* CBehaviorGraphAnimationNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

// temp
Bool CBehaviorGraphAnimationNode::SetTempRuntimeAnimationName( CBehaviorGraphInstance& instance, const CName &name ) const
{
	RefreshAnimation( instance, name );

	return instance[ i_animation ] ? true : false;
}

void CBehaviorGraphAnimationNode::ResetTempRuntimeAnimation( CBehaviorGraphInstance& instance ) const
{
	if ( IsActive( instance ) )
	{
		if ( instance[ i_animation ] && instance[ i_animation ]->GetAnimation() )
		{
			instance[ i_animation ]->GetAnimation()->ReleaseUsage();
		}
	}
	instance[ i_animation ] = NULL;
}

Bool CBehaviorGraphAnimationNode::IsTempSlotActive( CBehaviorGraphInstance& instace ) const
{
	return instace[ i_animation ] != NULL;
}

void CBehaviorGraphAnimationNode::CollectSyncTokens( CBehaviorGraphInstance& instance ) const
{
	if ( m_gatherSyncTokens )
	{
		const CEntity *entity = instance.GetAnimatedComponent()->GetEntity();

		const CSkeletalAnimationSetEntry* animation = instance[ i_animation ];

		TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
		entity->OnCollectAnimationSyncTokens( animation->GetName(), tokens );
	}
}

void CBehaviorGraphAnimationNode::ClearSyncTokens( CBehaviorGraphInstance& instance ) const
{
	TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
	for ( Uint32 i=0; i<tokens.Size(); ++i )
	{
		tokens[i]->Reset();
		delete tokens[i];
	}
	tokens.Clear();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAnimationNode::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	if (! m_animationName.Empty())
	{
		SBehaviorUsedAnimationData usageInfo( instance[ i_animation ], instance[i_localTime], 1.0f );

		collectorArray.PushBack( usageInfo );
	}
}
#endif

void CBehaviorGraphAnimationNode::OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const
{
	TBaseClass::OnLoadingSnapshot( instance, snapshotData );
	ClearSyncTokens( instance );
}

void CBehaviorGraphAnimationNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	RefreshAnimation( instance, GetAnimationName() );
	FirstUpdate( instance );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
