/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "gameplayLightComponent.h"
#include "cityLightManager.h"

#include "r4GuiManager.h"
#include "r4Hud.h"

#include "../../common/engine/lightComponent.h"
#include "../../common/engine/flareComponent.h"
#include "focusModeController.h"


IMPLEMENT_ENGINE_CLASS( CGameplayLightComponent )
IMPLEMENT_ENGINE_CLASS( CPersistentLightComponent )


CGameplayLightComponent::CGameplayLightComponent()
	: m_isLightOn( false )
	, m_isCityLight( false )
	, m_isInteractive( true )
	, m_isAffectedByWeather( false )
	, m_currentState( false )
	, m_timeToToggle( 0.0f )
{
	m_cityLightManager = GCommonGame->GetSystem< CCityLightManager >();

	m_checkLineOfSight = true;
	m_reportToScript = true;
	m_rangeMax = 1.5f;
}

void CGameplayLightComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CGameplayLightComponent_OnAttached );

#ifndef NO_EDITOR
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_GameplayLightComponent );
#endif

	//register with the manager only if you're a city light or are affected by weather (otherwise, we don't need to manage you)
	if(m_isCityLight || m_isAffectedByWeather)
	{
		m_cityLightManager->RegisterComponent(this);
	}

	//our component is an interaction component so different interaction methods should match each other
	m_isEnabled = m_isInteractive;

	SetInitialLight( m_isLightOn );
}

void CGameplayLightComponent::OnDetached( CWorld* world )
{
	//only city lights or lights affected by weather are currently registered with our manager
	if(m_isCityLight || m_isAffectedByWeather)
	{
		m_cityLightManager->UnregisterComponent(this);
	}

#ifndef NO_EDITOR
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_GameplayLightComponent );
#endif

	//we're detaching and our lights and effects will be destroyed, reset the
	//state here so the next time we're attached we will re-create them
	m_currentState = false;

	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CGameplayLightComponent::OnPropertyPostChange( IProperty* property )
{
#ifndef NO_EDITOR
	if ( property->GetName() == CNAME( isLightOn ) )
	{
		SetLight( m_isLightOn );
	}
#endif

	TBaseClass::OnPropertyPostChange( property );
}

void CGameplayLightComponent::SetLight( Bool toggle, Bool system )
{
	//skip the work if we're sending in the same state
 	if ( m_currentState == toggle )
 		return;

	ProcessToggle( toggle, system, false );
}

void CGameplayLightComponent::SetFadeLight( Bool toggle, Bool system )
{
	//skip the work if we're sending in the same state
	if ( m_currentState == toggle )
		return;

	ProcessToggle( toggle, system, true );
}

//this had to be added so that the lights are processed fully once on startup
//the upper SetLight() has the m_currentState check for returning immediately and it screws up
//lights turning on/off at startup based on its value vs toggle
void CGameplayLightComponent::SetInitialLight( Bool toggle )
{
	//initial processing is done according to a system (immediate) toggle, see below
	ProcessToggle( toggle, true, false );
}

void CGameplayLightComponent::ProcessToggle( Bool toggle, Bool system, Bool allowLightsFade )
{
	//differentiating between system and gameplay toggles
	//system: toggle fx immediately
	//gameplay: play out full fx animations (interaction, signs...)

	CEntity* entity = GetEntity();
	if ( entity )
	{
		//toggle any light components		
		for ( ComponentIterator<CLightComponent> it ( GetEntity() ); it; ++it )
		{
			CLightComponent* lightComponent = *it ;
			if ( lightComponent->IsEnabled() == toggle )
			{
				continue;
			}

			const Bool origFadeOnToggle = lightComponent->GetFadeOnToggle();
			RED_FATAL_ASSERT( !origFadeOnToggle, "FadeOnToggle was used for something more than this?" );
			lightComponent->SetFadeOnToggle( allowLightsFade );
			lightComponent->SetEnabled( toggle );
			lightComponent->SetFadeOnToggle( origFadeOnToggle );
		}

		CFocusActionComponent* focusComponent = entity->FindComponent< CFocusActionComponent >();
		m_tagList = entity->GetTags();

		//fx, interaction action, tag (for explosions), and focus icon
		if( toggle )
		{
			if( !entity->IsPlayingEffect(CNAME(fire), false) )
			{
				entity->PlayEffect(CNAME(fire));
			}		

			SetActionName( TXT("Extinguish") );
			
			m_tagList.AddTag( CNAME(CarriesOpenFire) );
			entity->SetTags( m_tagList );

			if ( focusComponent && m_isInteractive )
				focusComponent->SetActionName( CNAME(Aard) );
		}
		else
		{
			if ( system )
				entity->DestroyAllEffects();
			else
				entity->StopAllEffects();

			SetActionName( TXT("Ignite") );

			m_tagList.SubtractTag( CNAME(CarriesOpenFire) );
			entity->SetTags( m_tagList );

			if ( focusComponent && m_isInteractive )
				focusComponent->SetActionName( CNAME(Igni) );
		}

		//refresh interaction immediately
		UpdateInteraction();
	}

	m_isLightOn = toggle;
	m_currentState = toggle;
}

void CGameplayLightComponent::UpdateInteraction()
{
	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( guiManager )
	{
		CR4Hud* hud = Cast< CR4Hud >( guiManager->GetHud() );

		if ( hud )
		{
			hud->ForceUpdateInteractions();
		}
	}
}

void CGameplayLightComponent::SetInteractive( Bool toggle )
{
	m_isInteractive = toggle; 
	m_isEnabled = toggle;

	CEntity* entity = GetEntity();
	if ( entity )
	{
		CFocusActionComponent* focusComponent = entity->FindComponent< CFocusActionComponent >();

		if ( focusComponent )
		{
			if ( toggle )
			{
				if ( m_isLightOn )			
					focusComponent->SetActionName( CNAME(Aard) );
				else
					focusComponent->SetActionName( CNAME(Igni) );
			}
			else
				focusComponent->SetActionName( CName::NONE );
		}
	}
}

///////////////////////////////////////////////SCRIPTS/////////////////////////////////////////////////////////////////////

void CGameplayLightComponent::funcSetLight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SetLight( value, false );
}

void CGameplayLightComponent::funcSetFadeLight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SetFadeLight( value, false );
}

void CGameplayLightComponent::funcSetInteractive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SetInteractive( value );
}

void CGameplayLightComponent::funcIsLightOn( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isLightOn ); 
}

void CGameplayLightComponent::funcIsCityLight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isCityLight ); 
}

void CGameplayLightComponent::funcIsInteractive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isInteractive ); 
}

void CGameplayLightComponent::funcIsAffectedByWeather( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_isAffectedByWeather ); 
}

///////////////////////////////////////////////DEBUG/////////////////////////////////////////////////////////////////////

void CGameplayLightComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_GameplayLightComponent )
	{
		CEntity* entity = GetEntity();

		const Float distance = entity->GetWorldPosition().DistanceSquaredTo( GCommonGame->GetPlayer()->GetWorldPosition() );

		if ( entity && distance <= 900 )
		{
			Vector pos = Vector( 0.f, 0.f, 2.2f ) + entity->GetLocalToWorld().GetTranslation();

			String text;
			text = entity->GetName();
		
			text += String::Printf( TXT("\nLight:       %d"), m_isLightOn );
			text += String::Printf( TXT("\nCityLight:   %d"), m_isCityLight );
			text += String::Printf( TXT("\nInteractive: %d"), m_isInteractive );
			text += String::Printf( TXT("\nWeather:     %d"), m_isAffectedByWeather );

			frame->AddDebugText( pos, text );
		}
	}
}

void CPersistentLightComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	Bool isLightOn = IsLightOn();
	Bool isInteractive = IsInteractive();

	CGameSaverBlock block( saver, GetClass()->GetName() );

	saver->WriteValue( CNAME( isLightOn ), isLightOn );
	saver->WriteValue( CNAME( Interaction ), isInteractive );
}

void CPersistentLightComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	CGameSaverBlock block( loader, GetClass()->GetName() );

	Bool isLightOn = loader->ReadValue< Bool >( CNAME( isLightOn ), IsLightOn() );
	Bool isInteractive = loader->ReadValue< Bool >( CNAME( Interaction ), IsInteractive() );

	SetLight( isLightOn );
	SetInteractive( isInteractive );
}
