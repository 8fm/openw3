/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphRestoreSyncInfo.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphBlendNode.h"
#include "movingAgentComponent.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRestoreSyncInfoNode );

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphRestoreSyncInfoNode::CBehaviorGraphRestoreSyncInfoNode()
: m_syncMethod(nullptr)
, m_restoreOnActivation(true)
, m_restoreEveryFrame(false)
{
}

void CBehaviorGraphRestoreSyncInfoNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_restoreOnEventID;
	compiler << i_restoreActivationAfterUpdate;
}

void CBehaviorGraphRestoreSyncInfoNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_restoreOnEventID ] = instance.GetEventId( m_restoreOnEvent );
	instance[ i_restoreActivationAfterUpdate ] = false;
}

void CBehaviorGraphRestoreSyncInfoNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// restore before updating internals so when they update, they are updated to already synced info
	if ( m_restoreEveryFrame )
	{
		RestoreSync( instance );
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );

	// exception is restoring activation AFTER update when we are in actual state (not default that might be an entry)
	Bool & restoreActivationAfterUpdate = instance[ i_restoreActivationAfterUpdate ];
	if ( restoreActivationAfterUpdate )
	{
		restoreActivationAfterUpdate = false;
		RestoreSync( instance );
	}
}

void CBehaviorGraphRestoreSyncInfoNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	// activation just goes to default state (entry state), but on update we will go to actual state we want to be
	if ( m_restoreOnActivation )
	{
		instance[ i_restoreActivationAfterUpdate ] = true;
	}
}

Bool CBehaviorGraphRestoreSyncInfoNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( event.GetEventID() == instance[ i_restoreOnEventID ] )
	{
		RestoreSync( instance );
	}

	return TBaseClass::ProcessEvent(instance, event);
}

void CBehaviorGraphRestoreSyncInfoNode::RestoreSync( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		if ( CMovingAgentComponent const * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
		{
			CSyncInfo syncInfo;
			if ( mac->GetAnimationProxy().GetSynchronization( m_storeName, syncInfo ) )
			{
				if (m_syncMethod)
				{
					m_syncMethod->SynchronizeTo( instance, m_cachedInputNode, syncInfo );
				}
				else
				{
					// plain synchronize
					m_cachedInputNode->SynchronizeTo( instance, syncInfo );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
