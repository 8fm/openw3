
#include "build.h"
#include "bgNpcTrigger.h"
#include "bgNpc.h"
#include "bgNpcInteraction.h"

IMPLEMENT_ENGINE_CLASS( IBgNpcTriggerAction );
IMPLEMENT_ENGINE_CLASS( CBgNpcTriggerActionTalk );
IMPLEMENT_ENGINE_CLASS( CBgNpcTriggerActionLookAt );
IMPLEMENT_ENGINE_CLASS( CBgNpcTriggerActionSwordReaction );

IMPLEMENT_ENGINE_CLASS( CBgNpcTriggerComponent );

void CBgNpcTriggerComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	const Uint32 size = m_actions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		IBgNpcTriggerAction* action = m_actions[ i ];
		if ( action )
		{
			action->Init( world );
		}
	}
}

void CBgNpcTriggerComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	const Uint32 size = m_actions.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		IBgNpcTriggerAction* action = m_actions[ i ];
		if ( action )
		{
			action->Destroy( world );
		}
	}
}

void CBgNpcTriggerComponent::EnteredArea( CComponent* component )
{
	TBaseClass::EnteredArea( component );

	if ( component )
	{
		Bool player = component->GetEntity() == GGame->GetPlayerEntity();

		const Uint32 size = m_actions.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			IBgNpcTriggerAction* action = m_actions[ i ];
			if ( action )
			{
				action->Start( player, component );
			}
		}
	}
}

void CBgNpcTriggerComponent::ExitedArea( CComponent* component )
{
	TBaseClass::ExitedArea( component );

	if ( component )
	{
		Bool player = component->GetEntity() == GGame->GetPlayerEntity();

		const Uint32 size = m_actions.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			IBgNpcTriggerAction* action = m_actions[ i ];
			if ( action )
			{
				action->Stop( player, component );
			}
		}
	}
}

#ifndef NO_EDITOR

void CBgNpcTriggerComponent::AddVoicesetOption()
{
	CBgNpcTriggerActionTalk* action = CreateObject< CBgNpcTriggerActionTalk >( this );
	m_actions.PushBack( action );
}

void CBgNpcTriggerComponent::AddLookAtOption()
{
	CBgNpcTriggerActionLookAt* action = CreateObject< CBgNpcTriggerActionLookAt >( this );
	m_actions.PushBack( action );
}

void CBgNpcTriggerComponent::InitializeComponent()
{
	SetScale( Vector( 4.f, 4.f, 1.f ) );
}

#endif

//////////////////////////////////////////////////////////////////////////

void CBgNpcTriggerActionTalk::Start( Bool player, CComponent* component )
{
	if ( player && component )
	{
		CEntity* ent = FindParent< CEntity >();
		ASSERT( ent );

		for ( ComponentIterator< CBgInteractionComponent > it( ent ); it; ++it )
		{
			CBgInteractionComponent* comp = *it;

			if ( !comp->IsEnabled() )
			{
				comp->SetEnabled( true );
			}
			else
			{
				ASSERT( !comp->IsEnabled() );
			}
		}
	}
}

void CBgNpcTriggerActionTalk::Stop( Bool player, CComponent* component )
{
	if ( player && component )
	{
		CEntity* ent = FindParent< CEntity >();
		ASSERT( ent );

		for ( ComponentIterator< CBgInteractionComponent > it( ent ); it; ++it )
		{
			CBgInteractionComponent* comp = *it;

			if ( comp->IsEnabled() )
			{
				comp->SetEnabled( false );
			}
			else
			{
				ASSERT( comp->IsEnabled() );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CBgNpcTriggerActionLookAt::CBgNpcTriggerActionLookAt()
	: m_maxDelay( 2.5f )
	, m_onlyPlayer( false )
	, m_type( RLT_Look )
{

}

void CBgNpcTriggerActionLookAt::Start( Bool player, CComponent* component )
{
	if ( component )
	{
		if ( m_onlyPlayer && !player )
		{
			return;
		}

		CBgNpc* npc = FindParent< CBgNpc >();
		if ( npc )
		{
			CEntity* target = component->GetEntity();

			CActor* targetActor = Cast< CActor >( target );

			if ( targetActor )
			{
				CAnimatedComponent* ac = targetActor->GetRootAnimatedComponent(); 
				Int32 boneIndex = targetActor->GetHeadBone();

				SLookAtReactionBoneInfo info;
				info.m_reactionPriority = 0;
				info.m_type = m_type;
				info.m_targetOwner = ac;
				info.m_boneIndex = boneIndex;

				if ( m_maxDelay > 0.f )
				{
					info.SetDelay( GEngine->GetRandomNumberGenerator().Get< Float >( m_maxDelay ) );
				}

				npc->LookAt( info );
			}
			else if ( target )
			{
				SLookAtReactionDynamicInfo info;
				info.m_target = target;
				info.m_type = m_type;
				info.m_reactionPriority = 0;

				if ( m_maxDelay > 0.f )
				{
					info.SetDelay( GEngine->GetRandomNumberGenerator().Get< Float >( m_maxDelay ) );
				}

				npc->LookAt( info );
			}
		}
	}
}

void CBgNpcTriggerActionLookAt::Stop( Bool player, CComponent* component )
{
	
}
