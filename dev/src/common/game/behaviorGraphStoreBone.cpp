/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphStoreBone.h"
#include "../engine/behaviorGraphInstance.h"
#include "movingAgentComponent.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStoreBoneNode );

void CBehaviorGraphStoreBoneNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIdx;
}

void CBehaviorGraphStoreBoneNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIdx ] = FindBoneIndex( m_boneName, instance );
}

void CBehaviorGraphStoreBoneNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Int32 boneIdx = instance[ i_boneIdx ];
	if (! m_storeName.Empty() && boneIdx != INDEX_NONE)
	{
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			mac->AccessAnimationProxy().StoreTransform( m_storeName, output.GetBoneModelTransform( instance.GetAnimatedComponent(), boneIdx ) );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStoreAnimEventNode );

void CBehaviorGraphStoreAnimEventNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_animEventName )
	{
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			const Uint32 numEvents = output.m_numEventsFired;
			for ( Uint32 i=0; i<numEvents; ++i )
			{
				const CAnimationEventFired& e = output.m_eventsFired[ i ];
				const CName evtName = e.GetEventName();
				if ( evtName == m_animEventName )
				{
					mac->AccessAnimationProxy().StoreAnimEvent( e );
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRestoreAnimEventNode );

void CBehaviorGraphRestoreAnimEventNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	if ( m_animEventName )
	{
		if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			CAnimationEventFired re;
			if ( mac->AccessAnimationProxy().GetAnimEvent( m_animEventName, re ) )
			{
				Bool found( false );

				const Uint32 numEvents = output.m_numEventsFired;
				for ( Uint32 i=0; i<numEvents; ++i )
				{
					CAnimationEventFired& e = output.m_eventsFired[ i ];
					const CName evtName = e.GetEventName();
					if ( evtName == m_animEventName )
					{
						e.m_alpha = re.m_alpha;
						found = true;
						break;
					}
				}

				if ( !found )
				{
					output.AppendEvent( re, 1.f );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
