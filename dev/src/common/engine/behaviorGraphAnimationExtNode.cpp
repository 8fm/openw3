/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphAnimationExtNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationExtNode );

CBehaviorGraphAnimationExtNode::CBehaviorGraphAnimationExtNode()	
	: m_fireLoopEventBackOffset( 0.f )
	, m_animStartOffset( 0.f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationExtNode::GetCaption() const
{
	if ( m_animationName.Empty() )
		return TXT("Animation Ext"); 

	return String::Printf( TXT( "Animation Ext [ %s ]" ), m_animationName.AsString().AsChar() );
}

#endif

void CBehaviorGraphAnimationExtNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimationExt );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_fireLoopEventBackOffset > 0.f )
	{
		const Float duration = GetDuration( instance );
		if ( m_fireLoopEventBackOffset < duration )
		{
			const Float prevTime = instance[ i_prevTime ];
			const Float localTime = instance[ i_localTime ];

			Float timeToFire = m_playbackSpeed >= 0.0? duration - m_fireLoopEventBackOffset : m_fireLoopEventBackOffset;

			if ( prevTime < localTime && prevTime <= timeToFire && localTime >= timeToFire )
			{
				FireLoopEvent( instance );
			}
		}
	}
}

void CBehaviorGraphAnimationExtNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_localTime ]	= m_animStartOffset;
	instance[ i_prevTime ] = m_animStartOffset;
}
