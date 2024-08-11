/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behaviorGraphSynchronizeAnimationsNode.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphUtils.inl"
#include "behaviorGraphSocket.h"
#include "../engine/entity.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/skeletalAnimationContainer.h"
#include "animatedComponent.h"

//////////////////////////////////////////////////////////////////////////

// #define DEBUG_SYNC_ANIMS

#ifdef DEBUG_SYNC_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSynchronizeAnimationsToParentNode );
IMPLEMENT_ENGINE_CLASS( SSynchronizeAnimationToParentDefinition );
IMPLEMENT_ENGINE_CLASS( SSynchronizeAnimationToParentInstance );

//////////////////////////////////////////////////////////////////////////

void SSynchronizeAnimationToParentDefinition::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	if ( ! m_animationName.Empty() )
	{
		anims.PushBackUnique( m_animationName );
	}
}

//////////////////////////////////////////////////////////////////////////

void SSynchronizeAnimationToParentInstance::Setup( CBehaviorGraphInstance& instance, const SSynchronizeAnimationToParentDefinition& definition )
{
	CSkeletalAnimationContainer* animationContainer = instance.GetAnimatedComponent()->GetAnimationContainer();
	m_parentAnimationName = definition.m_parentAnimationName;
	m_animation = definition.m_animationName.Empty()? nullptr : animationContainer->FindAnimationRestricted( definition.m_animationName );
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphSynchronizeAnimationsToParentNode::CBehaviorGraphSynchronizeAnimationsToParentNode()
	: m_syncToInput( false )
	, m_autoFill( true )
	, m_animationStayMultiplier( 0.05f )
	, m_syncToParentsNormalAnims( true )
	, m_syncToParentsOverlayAnims( false )
	, m_skipNormalAnimsForOverlays( true )
	, m_syncDefaultToAnyLoopedAnim( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_instanceTimeActive;
	compiler << i_default;
	compiler << i_anims;
	compiler << i_playbacks;
	compiler << i_currDefaultTime;
	compiler << i_useWeight;
	compiler << i_firstUpdate;
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_instanceTimeActive ] = 0.0f;
	instance[ i_currDefaultTime ] = 0.0f;
	instance[ i_useWeight ] = 0.0f;
	instance[ i_firstUpdate ] = true;

	// setup default
	instance[ i_default ].Setup( instance, m_default );

	// animations
	TDynArray< SSynchronizeAnimationToParentInstance >& anims = instance[ i_anims ];
	anims.Resize( m_anims.Size() );
	TDynArray< SSynchronizeAnimationToParentInstance >::iterator instAnim = anims.Begin();
	for ( TDynArray< SSynchronizeAnimationToParentDefinition >::const_iterator animDef = m_anims.Begin(); animDef != m_anims.End(); ++ animDef, ++ instAnim )
	{
		instAnim->Setup( instance, *animDef );
	}
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_instanceTimeActive );
	INST_PROP( i_default );
	INST_PROP( i_anims );
	INST_PROP( i_playbacks );
	INST_PROP( i_currDefaultTime );
	INST_PROP( i_useWeight );
	INST_PROP( i_firstUpdate );
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphSynchronizeAnimationsToParentNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedInputNode && m_cachedInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		return;
	}
#endif
	Bool noInputPose = true; // flags that indicates whether we have anything attached to input - if not then I assume that output contains only T pose.
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
		noInputPose = false;
	}

	const SBehaviorUsedAnimations* syncTo = nullptr;

	if ( m_syncToInput )
	{
		syncTo = &output.m_usedAnims;
	}
	else
	{
		CAnimatedComponent* parentAC = nullptr;
		if ( const CAnimatedComponent* ac = instance.GetAnimatedComponent() )
		{
			if ( CEntity* ownerE = ac->GetEntity() )
			{
				if ( ownerE->GetRootAnimatedComponent() != ac )
				{
					// maybe we're just another component inside same entity?
					parentAC = ownerE->GetRootAnimatedComponent();
					if ( parentAC == ac )
					{
						parentAC = nullptr;
					}
				}
				else if ( CHardAttachment* ha = ownerE->GetTransformParent() )
				{
					if ( CNode* parentNode = ha->GetParent() )
					{
						// handle both attachment to slot and to entity
						if ( CEntity* parentEntity = Cast< CEntity >( parentNode ) )
						{
							parentAC = parentEntity->GetRootAnimatedComponent();
						}
						else if ( CObject* parent = parentNode->GetParent() )
						{
							if ( CEntity* parentEntity = Cast< CEntity >( parent ) )
							{
								parentAC = parentEntity->GetRootAnimatedComponent();
							}
						}
					}
				}
			}
		}
		if ( parentAC )
		{
			syncTo = &parentAC->GetRecentlyUsedAnims();
		}
	}

	Float& timeDelta = instance[ i_timeDelta ];
	Float instanceTimeActive = instance.GetTimeActive();
	Float& prevInstanceTimeActive = instance[ i_instanceTimeActive ];
	if ( timeDelta == 0.0f )
	{
		// fallback, in case we're not updated (why?!) but we're used
		timeDelta = Clamp( instanceTimeActive - prevInstanceTimeActive, 0.0f, 0.1f );
	}
	prevInstanceTimeActive = instanceTimeActive;
	SSimpleAnimationPlaybackSet& playbacks = instance[ i_playbacks ];

	Float modifyUsageWeightSubtract = timeDelta * 1.0f / Clamp( 1.0f - m_animationStayMultiplier, 0.01f, 1.0f );
#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("ready for anim switch %.3f td %.3f"), modifyUsageWeightSubtract, timeDelta );
#endif
	// ready for anim switch - this is to clear any anims that are not used
	playbacks.ReadyForAnimSwitch( modifyUsageWeightSubtract );

	const SSynchronizeAnimationToParentInstance& defaultAnim = instance[ i_default ];
	Float defaultWeight = 1.0f;

#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("start syncing anims (d:%.3f)"), defaultWeight );
#endif
	// check what animations parent plays
	if ( syncTo )
	{
		if ( m_syncToParentsOverlayAnims )
		{
			defaultWeight = Max( 0.0f, defaultWeight - UseAnimationsInPlaybackSet( instance, syncTo->m_overlayAnims, m_skipNormalAnimsForOverlays? &syncTo->m_anims : nullptr, playbacks, timeDelta, defaultWeight ) );
		}
		if ( m_syncToParentsNormalAnims )
		{
			defaultWeight = Max( 0.0f, defaultWeight - UseAnimationsInPlaybackSet( instance, syncTo->m_anims, nullptr, playbacks, timeDelta, defaultWeight ) );
		}
	}
	// add default anims if needed
	if ( defaultWeight > 0.0f && defaultAnim.m_animation )
	{
#ifdef DEBUG_SYNC_ANIMS
		RED_LOG(syncAnims, TXT("   default (%.3f)"), defaultWeight );
#endif
		Float & currDefaultTime = instance[ i_currDefaultTime ];
		Float prevDefaultTime = currDefaultTime;
		currDefaultTime += timeDelta;
		Float duration = defaultAnim.m_animation->GetDuration();
		Int32 loops = 0;
		while ( currDefaultTime > duration )
		{
			++ loops;
			currDefaultTime -= duration;
		}
		playbacks.AddAnimAt( defaultAnim.m_animation, currDefaultTime, prevDefaultTime, loops, 1.0f, false, defaultWeight );
#ifdef DEBUG_SYNC_ANIMS
		RED_LOG(syncAnims, TXT("     default (%.3f) playing %s @%.3f >%.3f %i"), defaultWeight, defaultAnim.m_animation->GetName().AsChar(), currDefaultTime, prevDefaultTime, loops );
#endif
	}
#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("end syncing anims (d:%.3f)"), defaultWeight );
#endif

	// clear all unused (and re calculate default weight)
	defaultWeight = Max(0.0f, 1.0f - playbacks.SwitchOffAnimIfNotUsedAndReadyForSamplingWithNormalization( 0.1f, timeDelta ));

#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("normalized syncing anims (d:%.3f)"), defaultWeight );
#endif

	Bool & firstUpdate = instance[ i_firstUpdate ];

	Float & useWeight = instance[ i_useWeight ];
	Float targetUseWeight = 1.0f - defaultWeight;
	if ( firstUpdate )
	{
		useWeight = targetUseWeight;
	}
	else
	{
		if ( useWeight > targetUseWeight )
		{
			// go down as quickly as taret use weight goes
			useWeight = targetUseWeight;
		}
		else
		{
			// go up slower
			useWeight = BlendToWithBlendTime( useWeight, targetUseWeight, 0.1f, timeDelta );
		}
	}

#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("updated use weight u:%.3f"), useWeight );
#endif

	// [Notes]: this is kind of optimization, if we know, that we will override pose from >output< completely, no temp pose will be created.
	if ( useWeight >= 0.99f || noInputPose )
	{
		playbacks.SamplePlayback( context, instance, output, timeDelta, true );
	}
	else
	{
		CCacheBehaviorGraphOutput syncedPosePlayback( context );
		SBehaviorGraphOutput* syncedPose = syncedPosePlayback.GetPose();
		playbacks.SamplePlayback( context, instance, *syncedPose, timeDelta, true );
		output.SetInterpolate( output, *syncedPose, useWeight );
		output.MergeUsedAnims( output, *syncedPose, useWeight );
	}

	timeDelta = 0.0f; // clear it for next use
	firstUpdate = false;
}

Float CBehaviorGraphSynchronizeAnimationsToParentNode::UseAnimationInPlaybackSet( CBehaviorGraphInstance& instance, const SBehaviorUsedAnimationData * usedAnim, const CSkeletalAnimationSetEntry * playAnim, SSimpleAnimationPlaybackSet& playbacks, Float timeDelta, Float weightMultiplier ) const
{
	Float usedAnimDuration = usedAnim->m_animation->GetDuration();
	Float orgDurationToNewDuration = usedAnimDuration != 0.0f? playAnim->GetDuration() / usedAnimDuration : 1.0f;
	Float currTime = orgDurationToNewDuration * usedAnim->m_currTime;
	Float prevTime = orgDurationToNewDuration * ( usedAnim->m_currTime - usedAnim->m_playbackSpeed * timeDelta );
	Int32 loops = 0;
	Float duration = usedAnim->m_animation->GetDuration();
	while ( prevTime < 0.0f )
	{
		++ loops;
		prevTime += duration;
	}
	while ( prevTime > duration )
	{
		++ loops;
		prevTime -= duration;
	}
	Float useWeight = usedAnim->m_weight * weightMultiplier;
	playbacks.AddAnimAt( playAnim, currTime, prevTime, loops, usedAnim->m_playbackSpeed, usedAnim->m_looped, useWeight );
#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("     play %s (%.3f) @%.3f (@%.1f%% -> @%.1f%%) >%.3f %i (from %s)"), playAnim->GetName().AsChar(), useWeight, currTime, ( usedAnim->m_currTime / usedAnimDuration ) * 100.0f, ( currTime / playAnim->GetDuration() ) * 100.0f, prevTime, loops, playAnim->GetAnimSet()->GetDepotPath().AsChar() );
#endif
	return useWeight;
}

Float CBehaviorGraphSynchronizeAnimationsToParentNode::UseAnimationsInPlaybackSet( CBehaviorGraphInstance& instance, const SBehaviorUsedAnimationDataSet & anims, const SBehaviorUsedAnimationDataSet * notInAnims, SSimpleAnimationPlaybackSet& playbacks, Float timeDelta, Float weightMultiplier ) const
{
	Float usedWeight = 0.0f;
	const SBehaviorUsedAnimationData * usedAnim = anims.GetUsedData();
#ifdef DEBUG_SYNC_ANIMS
	RED_LOG(syncAnims, TXT("  sync anims %i!"), anims.GetNum() );
#endif
	for ( Uint32 idx = 0; idx < anims.GetNum(); ++ idx, ++ usedAnim )
	{
#ifdef DEBUG_SYNC_ANIMS
		RED_LOG(syncAnims, TXT("   %i. parent %s (%.3f) @%.3f?"), idx, usedAnim->m_animation->GetName().AsChar(), usedAnim->m_weight, usedAnim->m_currTime );
#endif
		if ( notInAnims )
		{
			// check if anim does appear in other array (example: to disallow fullbody anims to be treated as overrides when override uses fullbody as input)
			Bool skipAnim = false;
			const SBehaviorUsedAnimationData * notAnim = notInAnims->GetUsedData();
			for ( Uint32 nidx = 0; nidx < notInAnims->GetNum(); ++ nidx, ++ notAnim )
			{
				if ( usedAnim->m_animation == notAnim->m_animation )
				{
					skipAnim = true;
					break;
				}
			}
			if ( skipAnim )
			{
				continue;
			}
		}
		const SSynchronizeAnimationToParentInstance* pair = FindForParents( instance, usedAnim->m_animation );
		if ( pair && pair->m_animation )
		{
			usedWeight += UseAnimationInPlaybackSet( instance, usedAnim, pair->m_animation, playbacks, timeDelta, weightMultiplier );
		}
		else if ( m_syncDefaultToAnyLoopedAnim && usedAnim->m_looped )
		{
			const SSynchronizeAnimationToParentInstance& defaultAnim = instance[ i_default ];
			if ( defaultAnim.m_animation )
			{
				usedWeight += UseAnimationInPlaybackSet( instance, usedAnim, defaultAnim.m_animation, playbacks, timeDelta, weightMultiplier );
			}
		}
	}
	return usedWeight;
}

const SSynchronizeAnimationToParentInstance* CBehaviorGraphSynchronizeAnimationsToParentNode::FindForParents( CBehaviorGraphInstance& instance, const CSkeletalAnimationSetEntry* parentAnimation ) const
{
	if ( parentAnimation )
	{
		CName parentAnimationName = parentAnimation->GetName();
		{
			const TDynArray< SSynchronizeAnimationToParentInstance >& anims = instance[ i_anims ];
			for ( TDynArray< SSynchronizeAnimationToParentInstance >::const_iterator anim = anims.Begin(); anim != anims.End(); ++ anim )
			{
				if ( anim->m_parentAnimationName == parentAnimationName )
				{
					if ( anim->m_animation )
					{
						return &(*anim);
					}
					else
					{
						return nullptr;
					}
				}
			}
		}
		if ( m_autoFill )
		{
			TDynArray< SSynchronizeAnimationToParentInstance >& anims = instance[ i_anims ];
			const CSkeletalAnimationSetEntry* anim = instance.GetAnimatedComponent()->GetAnimationContainer()->FindAnimationRestricted( parentAnimationName );
			//ASSERT( anim != nullptr, TXT("There is no animation to match %s"), parentAnimationName.AsChar() );
			anims.PushBack( SSynchronizeAnimationToParentInstance( parentAnimationName, anim? anim : instance[ i_default ].m_animation ) );
			return &(anims.Last());
		}
	}
	return nullptr;
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::CollectUsedAnimations( TDynArray< CName >& anims ) const
{
	m_default.CollectUsedAnimations( anims );
	for ( TDynArray< SSynchronizeAnimationToParentDefinition >::const_iterator animDef = m_anims.Begin(); animDef != m_anims.End(); ++ animDef )
	{
		animDef->CollectUsedAnimations( anims );
	}
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );
	instance[ i_timeDelta ] = 0.0f;
	instance[ i_instanceTimeActive ] = 0.0f;
	instance[ i_currDefaultTime ] = 0.0f;
	instance[ i_useWeight ] = 0.0f;
	instance[ i_firstUpdate ] = true;
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphSynchronizeAnimationsToParentNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );
	instance[ i_playbacks ].ClearAnims();
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphSynchronizeAnimationsToParentNode::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	TBaseClass::CollectAnimationUsageData( instance, collectorArray );

	instance[ i_playbacks ].CollectAnimationUsageData( instance, collectorArray );
}
#endif

void CBehaviorGraphSynchronizeAnimationsToParentNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef DEBUG_SYNC_ANIMS
#pragma optimize("",on)
#endif