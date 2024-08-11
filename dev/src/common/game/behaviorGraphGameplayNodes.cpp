
#include "build.h"

#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStack.h"

#include "behaviorGraphGameplayNodes.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGameplaySoundEventsNode )

void CBehaviorGraphGameplaySoundEventsNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_prevTime;
	compiler << i_currTime;
	compiler << i_animation;
}

void CBehaviorGraphGameplaySoundEventsNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	FindAnimation( instance );

	InternalReset( instance );
}

void CBehaviorGraphGameplaySoundEventsNode::FindAnimation( CBehaviorGraphInstance& instance ) const
{
	if ( instance.GetAnimatedComponent() )
	{
		CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
		instance[ i_animation ] = cont ? cont->FindAnimation( m_animationName ) : NULL;
	}
	else
	{
		instance[ i_animation ] = NULL;
	}
}

void CBehaviorGraphGameplaySoundEventsNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevTime ] = 0.f;
	instance[ i_currTime ] = 0.f;
}

void CBehaviorGraphGameplaySoundEventsNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphGameplaySoundEventsNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		CSyncInfo infoA;
		m_cachedInputNode->GetSyncInfo( instance, infoA );

		TBaseClass::OnUpdate( context, instance, timeDelta );

		CSyncInfo infoB;
		m_cachedInputNode->GetSyncInfo( instance, infoB );

		instance[ i_prevTime ] = infoA.m_currTime;
		instance[ i_currTime ] = infoB.m_currTime;
	}
}

void CBehaviorGraphGameplaySoundEventsNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_cachedInputNode )
	{
		const Float prevTime = instance[ i_prevTime ];
		const Float currTime = instance[ i_currTime ];

		const CSkeletalAnimationSetEntry* anim = instance[ i_animation ];
		if ( anim )
		{
			anim->GetEventsByTime( prevTime, currTime, 0, 1.f, NULL, &output );
		}
	}
}

void CBehaviorGraphGameplaySoundEventsNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	InternalReset( instance );
}

void CBehaviorGraphGameplaySoundEventsNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	FindAnimation( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRandomAnimTimeNode );

CBehaviorGraphRandomAnimTimeNode::CBehaviorGraphRandomAnimTimeNode()
	: m_animSpeedMin( 0.85f )
	, m_animSpeedMax( 1.15f )
	, m_animStartTimeOffset( 0.2f )
	, m_animStartTimePrecent( 0.f )
{

}

void CBehaviorGraphRandomAnimTimeNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_firstUpdate;
}

void CBehaviorGraphRandomAnimTimeNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

void CBehaviorGraphRandomAnimTimeNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	Bool& firstUpdate = instance[ i_firstUpdate ];
	if ( firstUpdate )
	{
		const Float randSpeed = BehaviorUtils::RandF( m_animSpeedMin, m_animSpeedMax );

		CSyncInfo info;
		info.m_prevTime = 0.f;

		if ( m_animStartTimePrecent > 0.f )
		{
			CSyncInfo ninfo;
			m_cachedInputNode->GetSyncInfo( instance, ninfo );

			info.m_currTime = ninfo.m_totalTime * BehaviorUtils::RandF( m_animStartTimePrecent );
		}
		else
		{
			info.m_currTime = BehaviorUtils::RandF( m_animStartTimeOffset );
		}

		m_cachedInputNode->SynchronizeTo( instance, info );

		CBehaviorNodeSyncData& syncData = m_cachedInputNode->GetSyncData( instance );
		syncData.m_timeMultiplier = randSpeed;

		firstUpdate = false;
	}
	else
	{
		TBaseClass::OnUpdate( context, instance, timeDelta );
	}
}

void CBehaviorGraphRandomAnimTimeNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );

	InternalReset( instance );
}

void CBehaviorGraphRandomAnimTimeNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	InternalReset( instance );
}

void CBehaviorGraphRandomAnimTimeNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_firstUpdate ] = true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMorphTrackNode );

CBehaviorGraphMorphTrackNode::CBehaviorGraphMorphTrackNode()
	: m_trackIndex( 0 )
{

}

void CBehaviorGraphMorphTrackNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_trackIndex < output.m_numFloatTracks )
	{
		CAnimationMorphAction* action = context.GetOneFrameAllocator().CreateOnStack< CAnimationMorphAction >();
		if ( action )
		{
			action->m_value = output.m_floatTracks[ m_trackIndex ];
			context.AddPostAction( action );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
