
#include "build.h"
#include "bgNpcInteraction.h"
#include "bgNpc.h"
#include "../../common/game/commonGame.h"
#include "../../common/game/interactionsManager.h"

IMPLEMENT_ENGINE_CLASS( CBgInteractionComponent );

CBgInteractionComponent::CBgInteractionComponent()
{
	
}

#ifndef NO_EDITOR
void CBgInteractionComponent::InitializeComponent()
{
	m_manualTestingOnly = true;
	m_actionName = TXT("Talk");
}
#endif

void CBgInteractionComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	TBaseClass::SetEnabled( false );
}

void CBgInteractionComponent::OnDetached( CWorld* world )
{
	if ( IsEnabled() )
	{
		SetEnabled( false );
	}

	TBaseClass::OnDetached( world );
}

void CBgInteractionComponent::SetEnabled( Bool enabled )
{
	TBaseClass::SetEnabled( enabled );

	if ( enabled )
	{
		GCommonGame->GetSystem< CInteractionsManager >()->AddInteractionComponent( this );
	}
	else
	{
		GCommonGame->GetSystem< CInteractionsManager >()->RemoveInteractionComponent( this );
	}
}

void CBgInteractionComponent::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	m_friendlyName = TXT( "talk" );

	m_manualTestingOnly = true;
}

void CBgInteractionComponent::OnExecute()
{
	TBaseClass::OnExecute();

	CBgNpc* npc = SafeCast< CBgNpc >( GetEntity() );
	if ( npc )
	{
		ASSERT( !npc->IsPlayingVoiceset() );

		if ( npc->CanPlayVoiceset() )
		{
			npc->PlayDefaultVoiceset();
		}
	}
}

Bool CBgInteractionComponent::ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const
{
	if ( activator->IsPlayer() )
	{
		CBgNpc* npc = SafeCast< CBgNpc >( GetEntity() );
		if ( npc && npc->CanPlayVoiceset() )
		{
			return TBaseClass::ActivationTest( activator, activatorData );
		}
	}
	
	return false;
}

void CBgInteractionComponent::OnActivate( CEntity* activator )
{
	TBaseClass::OnActivate( activator );

	if ( activator && GCommonGame->GetPlayerEntity() == activator /* activator->IsA< CPlayer >() */ )
	{
#if 0 // GFx 3
		GWitcherGame->GetHudInstance()->SetTargetActor( GetEntity(), false );
#endif
	}
}

void CBgInteractionComponent::OnDeactivate( CEntity* activator )
{	
	TBaseClass::OnDeactivate( activator );

	if ( activator && GCommonGame->GetPlayerEntity() == activator /* activator->IsA< CPlayer >() */ )
	{
#if 0 // GFx 3
		GWitcherGame->GetHudInstance()->SetTargetActor( NULL, false );
#endif
	}
}