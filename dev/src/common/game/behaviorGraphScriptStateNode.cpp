
#include "build.h"

#include "behaviorGraphScriptStateNode.h"
#include "../../common/game/behaviorGraphEventHandler.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphScriptStateNode );

void CBehaviorGraphScriptStateNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

#ifndef NO_EDITOR
	OnUpdateName();
#endif
}

void CBehaviorGraphScriptStateNode::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	OnUpdateName();
}

void CBehaviorGraphScriptStateNode::OnUpdateName()
{
	m_nameAsName = CName( m_name );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphScriptStateNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Script state - %s"), m_name.AsChar() );
	}
	else
	{
		// For state with just one animation return animation name as caption
		if( m_nodes.Size() == 2 )
		{
			const CBehaviorGraphAnimationNode* animNode = NULL;
			if( m_nodes[0]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[0] );
			}
			else if( m_nodes[1]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[1] );
			}

			if( animNode )
			{
				const CName& animName = animNode->GetAnimationName();
				if( !animName.Empty() )
				{
					return animName.AsString();
				}
			}
		}

		return String( TXT("Script state") );
	}
}

Color CBehaviorGraphScriptStateNode::GetTitleColor() const
{
	return Color( 212, 206, 224 );
}

String CBehaviorGraphScriptComponentStateNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Script component state - %s"), m_name.AsChar() );
	}
	else
	{
		// For state with just one animation return animation name as caption
		if( m_nodes.Size() == 2 )
		{
			const CBehaviorGraphAnimationNode* animNode = NULL;
			if( m_nodes[0]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[0] );
			}
			else if( m_nodes[1]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[1] );
			}

			if( animNode )
			{
				const CName& animName = animNode->GetAnimationName();
				if( !animName.Empty() )
				{
					return animName.AsString();
				}
			}
		}

		return String( TXT("Script component state") );
	}
}

Color CBehaviorGraphScriptComponentStateNode::GetTitleColor() const
{
	return Color( 255, 206, 204 );
}

#endif

void CBehaviorGraphScriptStateNode::CEvent::Handle( CBehaviorGraphInstance* instance ) const
{
	instance->GetAnimatedComponent()->GetEntity()->CallEvent( m_name );
}

void CBehaviorGraphScriptStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	SendEvent( instance, m_activationScriptEvent );
}

void CBehaviorGraphScriptStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	SendEvent( instance, m_deactivationScriptEvent );

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphScriptStateNode::OnBecomesCurrentState( CBehaviorGraphInstance& instance ) const
{
	SendEvent( instance, m_becomesCurrentStateScriptEvent );

	TBaseClass::OnBecomesCurrentState( instance );
}

void CBehaviorGraphScriptStateNode::OnNoLongerCurrentState( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnNoLongerCurrentState( instance );

	SendEvent( instance, m_noLongerCurrentStateScriptEvent );
}

void CBehaviorGraphScriptStateNode::OnFullyBlendedIn( CBehaviorGraphInstance& instance ) const
{
	SendEvent( instance, m_fullyBlendedInScriptEvent );

	TBaseClass::OnFullyBlendedIn( instance );
}

void CBehaviorGraphScriptStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );
	CW3BehaviorGraphEventHandler::LazyCreate( instance );
}

void CBehaviorGraphScriptStateNode::SendEvent( CBehaviorGraphInstance& instance, const CName& event ) const
{
	if ( ! event.Empty() )
	{
		instance.NotifyOfScriptedNodesNotification( event, m_nameAsName );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphScriptComponentStateNode );

void CBehaviorGraphScriptComponentStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_component;
}

void CBehaviorGraphScriptComponentStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_component ] = instance.GetAnimatedComponent()->GetEntity()->FindComponent( m_componentName );
}

void CBehaviorGraphScriptComponentStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if( !instance[ i_component ].Get() )
	{
		instance[ i_component ] = instance.GetAnimatedComponent()->GetEntity()->FindComponent( m_componentName );
	}

	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphScriptComponentStateNode::SendEvent( CBehaviorGraphInstance& instance, const CName& event ) const
{
	if ( event )
	{
		CEvent e;
		e.i_component = i_component;
		e.m_event = event;
		CW3BehaviorGraphEventHandler::Get( instance )->AddEvent( e );
	}
}
void CBehaviorGraphScriptComponentStateNode::CEvent::Handle( CBehaviorGraphInstance* instance ) const
{
	CComponent* component = (*instance)[ i_component ].Get();
	if ( component )
	{
		component->CallEvent( m_event );
	}
}
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphScriptStateReportingNode );

void CBehaviorGraphScriptStateReportingNode::CEvent::Handle( CBehaviorGraphInstance* instance ) const
{
	CActor* actor = Cast< CActor >( instance->GetAnimatedComponent()->GetEntity() );
	if ( actor )
	{
		actor->SignalGameplayAnimEvent( m_eventType, m_stateName );
	}
}

void CBehaviorGraphScriptStateReportingNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	CEvent e;
	e.m_eventType =  CNAME( AI_AnimStateActivated );
	e.m_stateName = m_stateName;
	CW3BehaviorGraphEventHandler::Get( instance )->AddEvent( e );

	//CActor* actor = Cast< CActor >( instance.GetAnimatedComponent()->GetEntity() );
	//if ( actor )
	//{
	//	SAnimEventData data;
	//	data.m_stateName = m_stateName;

	//	actor->SignalGameplayAnimEvent( GET_Tick, CNAME( AI_AnimStateActivated ), &data );
	//}
}

void CBehaviorGraphScriptStateReportingNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	m_stateName = CName( GetName() );
}

void CBehaviorGraphScriptStateReportingNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	CEvent e;
	e.m_eventType =  CNAME( AI_AnimStateDeactivated );
	e.m_stateName = m_stateName;
	CW3BehaviorGraphEventHandler::Get( instance )->AddEvent( e );

	//CActor* actor = Cast< CActor >( instance.GetAnimatedComponent()->GetEntity() );
	//if ( actor )
	//{
	//	SAnimEventData data;
	//	data.m_stateName = m_stateName;

	//	actor->SignalGameplayAnimEvent( GET_Tick, CNAME( AI_AnimStateDeactivated ), &data );
	//}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphScriptStateReportingNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Script reporting state - %s"), m_name.AsChar() );
	}
	else
	{
		// For state with just one animation return animation name as caption
		if( m_nodes.Size() == 2 )
		{
			const CBehaviorGraphAnimationNode* animNode = NULL;
			if( m_nodes[0]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[0] );
			}
			else if( m_nodes[1]->IsA< CBehaviorGraphAnimationNode >() )
			{
				animNode = static_cast< const CBehaviorGraphAnimationNode* >( m_nodes[1] );
			}

			if( animNode )
			{
				const CName& animName = animNode->GetAnimationName();
				if( !animName.Empty() )
				{
					return animName.AsString();
				}
			}
		}

		return String( TXT("Script reporting state") );
	}
}

Color CBehaviorGraphScriptStateReportingNode::GetTitleColor() const
{
	return Color( 152, 156, 224 );
}

#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
