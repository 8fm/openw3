
#include "build.h"

#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphOutput.h"
#include "../engine/behaviorGraphSocket.h"
#include "../engine/cacheBehaviorGraphOutput.h"

#include "behaviorGraphGameplayAdditiveNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"

#include "../engine/behaviorGraphUtils.inl"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SGameplayAdditiveLevel );
IMPLEMENT_ENGINE_CLASS( SGameplayAdditiveAnimation );
IMPLEMENT_ENGINE_CLASS( SGameplayAdditiveAnimRuntimeData );

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGameplayAdditiveNode );


CBehaviorGraphGameplayAdditiveNode::CBehaviorGraphGameplayAdditiveNode()
	: m_gatherEvents( false )
{
}

void CBehaviorGraphGameplayAdditiveNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_actTime;
	compiler << i_firstUpdate;
	compiler << i_timeDelta;

	compiler << i_animLevel0;
	compiler << i_animLevel1;

	compiler << i_animsForLevel0;
	compiler << i_animsForLevel1;

	compiler << i_cooldownsForLevel0;
	compiler << i_cooldownsForLevel1;
}

void CBehaviorGraphGameplayAdditiveNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );

	FindAnims( instance );
}

void CBehaviorGraphGameplayAdditiveNode::FindAnims( CBehaviorGraphInstance& instance ) const
{
	TDynArray< CSkeletalAnimationSetEntry* >& animsLevel0 = instance[ i_animsForLevel0 ];
	TDynArray< CSkeletalAnimationSetEntry* >& animsLevel1 = instance[ i_animsForLevel1 ];

	CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
	if ( cont )
	{
		FillAnimsForLevel( cont, m_level_0, animsLevel0 );
		FillAnimsForLevel( cont, m_level_1, animsLevel1 );
	}
}

void CBehaviorGraphGameplayAdditiveNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphGameplayAdditiveNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
}

void CBehaviorGraphGameplayAdditiveNode::GetAnimProgress( CBehaviorGraphInstance& instance, Float& p1, Float& p2 ) const
{
	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	p1 = data0.m_animation ? data0.m_currTime / data0.m_animation->GetDuration() : 0.f;
	p2 = data1.m_animation ? data1.m_currTime / data1.m_animation->GetDuration() : 0.f;
}

void CBehaviorGraphGameplayAdditiveNode::GetAnimNames( CBehaviorGraphInstance& instance, CName& n1, CName& n2 ) const
{
	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	n1 = data0.m_animation ? data0.m_animation->GetName() : CName::NONE;
	n2 = data1.m_animation ? data1.m_animation->GetName() : CName::NONE;
}

#endif

void CBehaviorGraphGameplayAdditiveNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Base") );
}

void CBehaviorGraphGameplayAdditiveNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	instance[ i_timeDelta ] = timeDelta;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	Float& actTime = instance[ i_actTime ];
	actTime = Min( actTime+timeDelta, 1000.f );

	UpdateCooldowns( instance, timeDelta );

	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	if ( m_level_0.m_useLevel )
	{
		UpdateLevel( instance, m_level_0, data0, instance[ i_animsForLevel0 ], instance[ i_cooldownsForLevel0 ], actTime, timeDelta );
	}
	
	if ( m_level_1.m_useLevel )
	{
		UpdateLevel( instance, m_level_1, data1, instance[ i_animsForLevel1 ], instance[ i_cooldownsForLevel1 ], actTime, timeDelta );
	}
}

void CBehaviorGraphGameplayAdditiveNode::UpdateLevel(	CBehaviorGraphInstance& instance, const SGameplayAdditiveLevel& level, SGameplayAdditiveAnimRuntimeData& animData, 
														const TDynArray< CSkeletalAnimationSetEntry* >& anims, 
														TDynArray< Float >& cooldowns, 
														Float actTime, Float timeDelta ) const
{
	Bool& firstUpdate = instance[ i_firstUpdate ];

	if ( !level.m_synchronize )
	{
		if ( animData.IsPlaying() )
		{
			if ( animData.WillBeEnd( timeDelta ) )
			{
				// Animation will be finished in this frame
				//Float timeForNextAnim = timeDelta - timeToEnd;

				//...
				SelectNextAnim( instance, level, animData, anims, cooldowns, actTime );
			}
			else
			{
				animData.Update( timeDelta );
			}
		}
		else
		{
			SelectNextAnim( instance, level, animData, anims, cooldowns, actTime );
		}
	}
	else if ( m_cachedInputNode )
	{
		CSyncInfo info;
		info.m_wantSyncEvents = false;

		m_cachedInputNode->GetSyncInfo( instance, info );

		if ( info.m_prevTime > info.m_currTime || firstUpdate )
		{
			if ( animData.IsPlaying() )
			{
				animData.AddLoopAndSync( info );
			}

			if ( !animData.IsPlaying() || animData.IsFinished() )
			{
				SelectNextAnim( instance, level, animData, anims, cooldowns, actTime );
			}
		}
		else if ( animData.IsPlaying() )
		{
			animData.Sync( info );

			if ( animData.IsFinished() )
			{
				animData.Reset();
			}
		}
	}

	firstUpdate = false;
}

void CBehaviorGraphGameplayAdditiveNode::UpdateCooldowns( CBehaviorGraphInstance& instance, Float dt ) const
{
	TDynArray< Float >& arr0 = instance[ i_cooldownsForLevel0 ];
	TDynArray< Float >& arr1 = instance[ i_cooldownsForLevel1 ];

	UpdateCooldowns( arr0, dt );
	UpdateCooldowns( arr1, dt );
}

void CBehaviorGraphGameplayAdditiveNode::UpdateCooldowns( TDynArray< Float >& arr, Float dt ) const
{
	const Uint32 size = arr.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		arr[ i ] = Max( arr[ i ] - dt , 0.f );
	}
}

void CBehaviorGraphGameplayAdditiveNode::SelectNextAnim(	CBehaviorGraphInstance& instance, const SGameplayAdditiveLevel& level, SGameplayAdditiveAnimRuntimeData& animData, 
															const TDynArray< CSkeletalAnimationSetEntry* >& anims, 
															TDynArray< Float >& cooldowns, 
															Float actTime ) const
{
	// Reset current animation
	const Int32 prevAnim = animData.m_index;
	if ( prevAnim != -1 )
	{
		// Set cooldown
		cooldowns[ prevAnim ] = !level.m_animations[ prevAnim ].m_onlyOnce ? level.m_animations[ prevAnim ].m_cooldown : FLT_MAX;
	}

	animData.Reset();

	// Select new animation
	Uint32 size = anims.Size();

	ASSERT( size == level.m_animations.Size() );

	Float list[ 16 ];
	size = Min< Uint32 >( size, 16 );

	Float total = 0;
	for ( Uint32 i=0; i<size; ++i )
	{
		list[ i ] = 0;

		if ( anims[ i ] == NULL || cooldowns[ i ] > 0.001f )
		{
			continue;
		}

		const SGameplayAdditiveAnimation& animationProp = level.m_animations[ i ];

		Float delay = animationProp.m_delay;
		if ( delay > 0.f && delay > actTime )
		{
			continue;
		}

		Float chance = animationProp.m_chance;

		if ( chance > 0.f )
		{
			total += chance;
			
			list[ i ] = chance;
		}
	}

	if ( total > 0.f )
	{
		Float val = GEngine->GetRandomNumberGenerator().Get< Float >() * total;

		for ( Uint32 i=0; i<size; ++i )
		{
			val -= list[ i ];

			if ( val <= 0.f )
			{
				Int32 newIndex = (Int32)i;

				const SGameplayAdditiveAnimation& animationProp = level.m_animations[ i ];

				animData.m_index = newIndex;
				animData.m_animation = anims[ i ];

				if ( animationProp.m_useSpeedRange )
				{
					animData.m_speed = GEngine->GetRandomNumberGenerator().Get< Float >( animationProp.m_speedRangeMin , animationProp.m_speedRangeMax );
				}

				if ( animationProp.m_useWeightRange )
				{
					animData.m_weight = GEngine->GetRandomNumberGenerator().Get< Float >( animationProp.m_weightRangeMin , animationProp.m_weightRangeMax );
				}

				return;
			}
		}

		ASSERT( !TXT("No to dupa zbita") );
	}
}

void CBehaviorGraphGameplayAdditiveNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	const Bool add0 = data0.IsPlaying() && data0.m_animation->GetAnimation();
	const Bool add1 = data1.IsPlaying() && data1.m_animation->GetAnimation();

	if ( add0 || add1 )
	{
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* pose = cachePose.GetPose();
		if ( pose )
		{
			if ( add0 )
			{
				AddAdditivePose( instance, data0, *pose, context, output );
			}

			if ( add1 )
			{
				AddAdditivePose( instance, data1, *pose, context, output );
			}
		}
	}
}

void CBehaviorGraphGameplayAdditiveNode::AddAdditivePose( const CBehaviorGraphInstance& instance, SGameplayAdditiveAnimRuntimeData& animData, SBehaviorGraphOutput& temp, SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const
{
	ASSERT( animData.m_animation );
	ASSERT( temp.m_numBones == output.m_numBones );
	ASSERT( animData.m_animation->GetAnimation() );

	// Don't use compressed poses for additive animations
	UpdateAndSampleBlendWithCompressedPose( animData.m_animation, instance[ i_timeDelta ], animData.m_blendTimer, animData.m_currTime, context, temp, instance.GetAnimatedComponent()->GetSkeleton() );
	
	const Float weight = animData.m_weight;

	BehaviorUtils::BlendingUtils::BlendAdditive_Local( output, output, temp, animData.m_weight );

	output.NormalizeRotations();

	if( m_gatherEvents )
	{
		animData.m_animation->GetEventsByTime( animData.m_prevTime, animData.m_currTime, ( animData.m_loop > 0.f ? 1 : 0 ), 1.f, NULL, &output );
		output.MergeEvents( temp, weight );
	}
	output.AppendUsedAdditiveAnim( SBehaviorUsedAnimationData( animData.m_animation, animData.m_currTime ) );
	output.MergeUsedAnimsAsAdditives( temp, weight );
}

void CBehaviorGraphGameplayAdditiveNode::FillAnimsForLevel( CSkeletalAnimationContainer* cont, const SGameplayAdditiveLevel& level, TDynArray< CSkeletalAnimationSetEntry* >& anims ) const
{
	const Uint32 levelAnimSize = level.m_animations.Size();

	anims.Resize( levelAnimSize );

	for ( Uint32 i=0; i<levelAnimSize; ++i )
	{
		const SGameplayAdditiveAnimation& levelAnim = level.m_animations[ i ];
	
		const CName& animName = levelAnim.m_animationName;
		if ( animName != CName::NONE )
		{
			anims[ i ] = cont->FindAnimation( animName );
		}
		else
		{
			anims[ i ] = NULL;
		}
	}
}

void CBehaviorGraphGameplayAdditiveNode::FillCooldownsForLevel( const SGameplayAdditiveLevel& level, TDynArray< Float >& cooldowns ) const
{
	const Uint32 levelAnimSize = level.m_animations.Size();

	cooldowns.Resize( levelAnimSize );

	for ( Uint32 i=0; i<levelAnimSize; ++i )
	{
		//const SGameplayAdditiveAnimation& levelAnim = level.m_animations[ i ];
		//cooldowns[ i ] = levelAnim.m_cooldown;
		cooldowns[ i ] = 0.f;
	}
}

void CBehaviorGraphGameplayAdditiveNode::ResetCooldownsForLevelOnDeact( const SGameplayAdditiveLevel& level, TDynArray< Float >& cooldowns ) const
{
	const Uint32 size = cooldowns.Size();

	ASSERT( level.m_animations.Size() == size );

	for ( Uint32 i=0; i<size; ++i )
	{
		if ( level.m_animations[ i ].m_onlyOnce )
		{
			cooldowns[ i ] = 0.f;
		}
	}
}

void CBehaviorGraphGameplayAdditiveNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphGameplayAdditiveNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphGameplayAdditiveNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return m_cachedInputNode ? m_cachedInputNode->ProcessEvent( instance, event ) : false;
}

void CBehaviorGraphGameplayAdditiveNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	instance[ i_actTime ] = 0.f;
	instance[ i_firstUpdate ] = true;
}

void CBehaviorGraphGameplayAdditiveNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Deactivate( instance );
	}

	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	data0.Reset();
	data1.Reset();

	TDynArray< Float >& cooldownsLevel0 = instance[ i_cooldownsForLevel0 ];
	TDynArray< Float >& cooldownsLevel1 = instance[ i_cooldownsForLevel1 ];

	ResetCooldownsForLevelOnDeact( m_level_0, cooldownsLevel0 );
	ResetCooldownsForLevelOnDeact( m_level_1, cooldownsLevel1 );
}

void CBehaviorGraphGameplayAdditiveNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_actTime ] = 0.f;
	instance[ i_firstUpdate ] = true;

	SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	data0.Reset();
	data1.Reset();

	TDynArray< Float >& cooldownsLevel0 = instance[ i_cooldownsForLevel0 ];
	TDynArray< Float >& cooldownsLevel1 = instance[ i_cooldownsForLevel1 ];

	FillCooldownsForLevel( m_level_0, cooldownsLevel0 );
	FillCooldownsForLevel( m_level_1, cooldownsLevel1 );
}

void CBehaviorGraphGameplayAdditiveNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphGameplayAdditiveNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphGameplayAdditiveNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	FindAnims( instance );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphGameplayAdditiveNode::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	const SGameplayAdditiveAnimRuntimeData& data0 = instance[ i_animLevel0 ];
	const SGameplayAdditiveAnimRuntimeData& data1 = instance[ i_animLevel1 ];

	if ( data0.IsPlaying() && data0.m_animation->GetAnimation() )
	{
		SBehaviorUsedAnimationData usageInfo( data0.m_animation, data0.m_currTime );

		collectorArray.PushBack( usageInfo );
	}

	if ( data1.IsPlaying() && data1.m_animation->GetAnimation() )
	{
		SBehaviorUsedAnimationData usageInfo( data1.m_animation, data1.m_currTime );

		collectorArray.PushBack( usageInfo );
	}
}
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
