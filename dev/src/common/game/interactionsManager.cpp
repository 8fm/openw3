/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "interactionsManager.h"
#include "interactionComponent.h"
#include "guiManager.h"
#include "../../common/core/gatheredResource.h"
#include "../../games/r4/gameplayLightComponent.h"

CGatheredResource resControlsDefMgr( TXT("gameplay\\globals\\controls.csv"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CInteractionsManager );

RED_DEFINE_NAME( InteractiveEntity );

RED_DEFINE_STATIC_NAME( CanProcessGuiInteractions );
RED_DEFINE_STATIC_NAME( GetSelectionWeights );
RED_DEFINE_STATIC_NAME( GetSelectionData );
RED_DEFINE_STATIC_NAME( GetBlockedActions );
RED_DEFINE_STATIC_NAME( OnGuiInteractionChanged );
RED_DEFINE_STATIC_NAME( CanProcessInteractionInput );

namespace
{

static const Float TREE_UPDATE_INTERVAL = 0.5f;

struct SInteractionsCollector
{
	TDynArray< THandle< CInteractionComponent > >	m_interactions;

	enum { SORT_OUTPUT = false };

	RED_FORCE_INLINE Bool operator()( const THandle< CInteractionComponent > & data )
	{
		CInteractionComponent* ic = data.Get();
		if ( ic != nullptr && ic->IsEnabled() )
		{
			m_interactions.PushBack( ic );
		}
		return true;
	}
};

}

//////////////////////////////////////////////////////////////////////////

CInteractionsManager::CActivationEvent::CActivationEvent( CInteractionComponent* interaction /* = nullptr */,
														  CInteractionsManager::CActivator* activator /* = nullptr */,
													      Bool isActivate /* = false */ )
	: m_interaction( interaction )
	, m_activator( activator )
	, m_isActivate( isActivate )
{}

void CInteractionsManager::CActivationEvent::Call()
{
	if ( m_interaction != nullptr && m_activator != nullptr )
	{
		if ( m_isActivate )
		{
			m_interaction->OnActivate( m_activator->GetEntity() );
		}
		else
		{
			m_interaction->OnDeactivate( m_activator->GetEntity() );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

CInteractionsManager::CActivator::CActivator( CEntity* entity )
	: m_entity( entity )
	, m_activatorData( entity )
{
}

CInteractionsManager::CActivator::~CActivator()
{	
	// Unlink all interactions
	for ( Uint32 i = 0; i < m_activeInteractions.Size(); i++ )
	{
		CInteractionComponent* ic = m_activeInteractions[i].Get();
		if ( ic != nullptr )
		{
			ic->OnDeactivate( m_entity );
		}
	}
}

Bool CInteractionsManager::CActivator::CanProcessInteractions() const
{
	// We can always process interactions in general case
	return true;
}

void CInteractionsManager::CActivator::FilterInteractions( const TInteractions& interactions, Bool canProcessInteractions, TInteractions& filteredInteractions )
{
	for ( Uint32 i = 0; i < interactions.Size(); i++ )
	{
		CInteractionComponent* ic = interactions[i].Get();
		if ( ic != nullptr && ( canProcessInteractions || ic->ShouldIgnoreLocks() ) && ic->ActivationTest( m_entity, m_activatorData ) )
		{
			filteredInteractions.PushBack( ic );
		}
	}
}

void CInteractionsManager::CActivator::GenerateActivationEvents( const TInteractions& newInteractions, const TInteractions& currentInteractions, TActivationEvents& events )
{
	// in this method we assume that both interactions' lists are sorted

	Uint32 newIndex = 0;
	const Uint32 newSize = newInteractions.Size();
	Uint32 currentIndex = 0;
	const Uint32 currentSize = currentInteractions.Size();

	while ( newIndex < newSize && currentIndex < currentSize )
	{
		const TInteraction& newInteraction = newInteractions[ newIndex ];
		const TInteraction& currentInteraction = currentInteractions[ currentIndex ];

		// newInteraction is not present in currentInteractions -> create activation event
		if ( newInteraction < currentInteraction )
		{
			new ( events ) CActivationEvent( newInteraction, this, true );
			newIndex++;
		}
		// currentInteraction is not present in newInteractions -> create deactivation event
		else if ( currentInteraction < newInteraction )
		{
			new ( events ) CActivationEvent( currentInteraction, this, false );
			currentIndex++;
		}
		// interactions is present in both lists -> do nothing
		else
		{
			newIndex++;
			currentIndex++;
		}
	}

	// activate newEvents not present in currentEvents
	while ( newIndex < newSize )
	{
		new ( events ) CActivationEvent( newInteractions[ newIndex++ ], this, true );
	}

	// deactivate currentEvents not present in newEvents
	while ( currentIndex < currentSize )
	{
		new ( events ) CActivationEvent( currentInteractions[ currentIndex++ ], this, false );
	}
}

void CInteractionsManager::CActivator::ProcessInteractions( const TInteractions& interactions, TActivationEvents& events )
{
	TInteractions filteredInteractions;
	// prepare per-frame gameplay data
	m_activatorData = SActivatorData( m_entity );

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( ProcessInteractionTests );
#endif

		// Filter interactions that can be enabled by this activator
		FilterInteractions( interactions, CanProcessInteractions(), filteredInteractions );
	}

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( SortFilteredInteractions );
#endif

		// We need to sort filtered interactions, because the following method needs sorted lists
		// (for performance reasons)
		Sort( filteredInteractions.Begin(), filteredInteractions.End() );
	}

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( GenerateActivationEvents );
#endif
		
		// Generate events
		GenerateActivationEvents( filteredInteractions, m_activeInteractions, events );

		// At the end we keep sorted activeInteractions
		m_activeInteractions = filteredInteractions;
	}
}

///////////////////////////////////////////////////////////////////////////////

CInteractionsManager::CPlayerActivator::CPlayerActivator( CEntity* entity )
	: CActivator( entity )
{}

Bool CInteractionsManager::CPlayerActivator::CanProcessInteractions() const
{	
	return GCommonGame->GetPlayer()->CanProcessButtonInteractions();
}

///////////////////////////////////////////////////////////////////////////////

CInteractionsManager::CInteractionsManager()
	: m_playerActivator( nullptr )
	, m_timeSinceLastUpdate( 0.0f )
	, m_isInTick( false )
	, m_forceGuiUpdate( false )
{
	m_interactions.ReserveNodeToEntry( 12 * 1024 );
	m_interactions.ReserveEntry( 12 * 1024 );
	m_interactions.ReserveNode( 3 * 1024 );
}

CInteractionsManager::~CInteractionsManager()
{
}

void CInteractionsManager::OnGameStart( const CGameInfo& gameInfo )
{
	// Reload key definitions for the interactions manager
	LoadActionMappings();
}

void CInteractionsManager::OnGameEnd( const CGameInfo& gameInfo )
{
	ClearActionMappings();
}

void CInteractionsManager::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( InteractionsMgr );

	/////////// STAGE 0 - prepare /////////////////

	RED_ASSERT( !m_isInTick );
	m_isInTick = true;

	{
		PC_SCOPE_PIX( InteractionsMgr_Stage0_UpdatingTree );

		m_timeSinceLastUpdate += timeDelta;
		if ( m_timeSinceLastUpdate > TREE_UPDATE_INTERVAL )
		{
			UpdateTree();
			m_timeSinceLastUpdate = 0.0f;
		}
	}

	/////////// STAGE 1 - collect potential interactions /////////////////

	SInteractionsCollector collector;

	{
		PC_SCOPE_PIX( InteractionsMgr_Stage1_Player );

		if ( m_playerActivator != nullptr )
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_treeLock );
			m_interactions.TQuery( m_playerActivator->GetActivatorData().GetCenter(), collector, Box( Vector::ZERO_3D_POINT, 0.0f ), true, nullptr, 0 );

		}
	}

	/////////// STAGE 2 - test activations and generate events /////////////////

	// Reset event list
	static TActivationEvents events;
	events.ClearFast();

	{
		PC_SCOPE_PIX( InteractionsMgr_Stage2_Player );
		if ( m_playerActivator != nullptr )
		{
			m_playerActivator->ProcessInteractions( collector.m_interactions, events );
		}
	}

	/////////// STAGE 3 - fire events /////////////////
	{
		PC_SCOPE_PIX( InteractionsMgr_Stage3 );

		for ( Uint32 i = 0; i < events.Size(); i++ )
		{
			events[i].Call();
		}
	}

	/////////// STAGE 4 - update gui buttons /////////////////
	{
		PC_SCOPE_PIX( UpdateGuiButtons );

		UpdateGuiButtons();
	}

	/////////// STAGE 5 - apply cached modifications to interaction system /////////////////
	{
		PC_SCOPE_PIX( InteractionsMgr_Stage4 );

		// End tick
		RED_ASSERT( m_isInTick );
		m_isInTick = false;

		// Apply add/remove of activators
		ApplyCachedActivatorAddRemove();
	}
}

void CInteractionsManager::UpdateTree()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_treeLock );

	TInteractionsToUpdate::iterator itEnd = m_interactionsToUpdate.End();
	for ( TInteractionsToUpdate::iterator it = m_interactionsToUpdate.Begin(); it != itEnd; ++it )
	{
		CInteractionComponent* ic = ( *it ).Get();
		if ( ic != nullptr && ic->IsEnabled() )
		{
			m_interactions.UpdatePosition( ic );
		}
	}
	m_interactionsToUpdate.ClearFast();
}

void CInteractionsManager::UpdateGuiButtons()
{
	if ( GCommonGame == nullptr || GCommonGame->GetGuiManager() == nullptr )
	{
		return;
	}
	CGuiManager* guiManager = GCommonGame->GetGuiManager();
	CInteractionComponent* bestInteraction = nullptr;
	if ( m_playerActivator != nullptr )
	{
		Bool canProcessGuiInteractions = false;
		THandle< CEntity > activatorHandle( m_playerActivator->GetEntity() );

		{
#ifdef PROFILE_INTERACTION
			PC_SCOPE_PIX( CanProcessGuiInteractions );
#endif
			CallFunctionRet( this, CNAME( CanProcessGuiInteractions ), activatorHandle, canProcessGuiInteractions );
		}

		if ( canProcessGuiInteractions )
		{
#ifdef PROFILE_INTERACTION
			PC_SCOPE_PIX( ProcessGuiInteractions );
#endif

			Vector playerPosition = m_playerActivator->GetWorldPosition();
			Vector playerHeading = m_playerActivator->GetEntity()->GetWorldForward();
			playerHeading.Z = 0;
			playerHeading.Normalize2();
			const SActivatorData& activatorData = m_playerActivator->GetActivatorData();
			
			STargetSelectionWeights selectionWeights;
			STargetSelectionData selectionData;
			selectionData.m_sourcePosition = playerPosition;
			TDynArray< String > blockedActions;

			{
#ifdef PROFILE_INTERACTION
				PC_SCOPE_PIX( ObtainScriptData );
#endif

				Bool res = CallFunctionRef( this, CNAME( GetSelectionWeights ), selectionWeights ) &&
						   CallFunctionRef( this, CNAME( GetSelectionData ), selectionData ) &&
						   CallFunctionRef( this, CNAME( GetBlockedActions ), blockedActions );
				RED_ASSERT( res, TXT( "CInteractionsManager: cannot obtain selection weights or selection data" ) );
			}

			Float maxValue = 0.0f;
			Bool interactionPresent = false;
			const Bool checkBlockedActions = blockedActions.Size() > 0;

			const TInteractions& playerInteractions = m_playerActivator->GetActiveInteractions();
			TInteractions::const_iterator itEnd = playerInteractions.End();
			for ( TInteractions::const_iterator it = playerInteractions.Begin(); it != itEnd; ++it )
			{
				CInteractionComponent* ic = it->Get();
				if ( ic == nullptr )
				{
					continue;
				}

				// if there's no action or actions is blocked (for example: because of replacers)
				if ( !ic->HasAction() )
				{
					continue;
				}
				if ( checkBlockedActions && blockedActions.FindPtr( ic->GetActionName() ) != nullptr )
				{
					continue;
				}

				// We check for proper heading only if player is not in combat mode.
				// Otherwise he will always point toward combat target.
				if ( !activatorData.IsInCombat() )
				{
					Vector direction = ic->GetWorldPosition() - playerPosition;
					if ( Vector::Dot2( playerHeading, direction ) < 0.0f )
					{
						continue;
					}
				}

				if ( !ic->GetClass()->IsA( SRTTI::GetInstance().FindClass( CName( TXT("CGameplayLightComponent") ) ) ) )
				{
					interactionPresent = true;
				}
				else if ( interactionPresent && ic->GetClass()->IsA( SRTTI::GetInstance().FindClass( CName( TXT("CGameplayLightComponent") ) ) ) )
				{
					continue;
				}

				Float value = CTargetingUtils::CalcSelectionPriority( ic, selectionWeights, selectionData );
				if ( value > maxValue )
				{
					maxValue = value;
					bestInteraction = ic;
				}
			}
		}
	}
	if ( m_forceGuiUpdate || bestInteraction != guiManager->GetActiveInteraction() )
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( OnInteractionChanged );
#endif
		guiManager->SetActiveInteraction( bestInteraction, m_forceGuiUpdate );
		THandle< CInteractionComponent > interactionHandle( bestInteraction );
		CallEvent( CNAME( OnGuiInteractionChanged ), interactionHandle );
		m_forceGuiUpdate = false;
	}
}

Bool CInteractionsManager::OnGameInputEvent( const SInputAction& action )
{
	return ProcessInteractionInput( action );
}

Bool CInteractionsManager::GetGameInputForAction( const String& actionName, CName& gameInput, Float& activation ) const
{
	if ( actionName.Empty() )
	{
		return false;
	}

	if ( m_actionToGameInput.Find( actionName, gameInput ) && gameInput )
	{
		const TDynArray< SActivatedAction >& possibleActions = m_gameInputToActions[ gameInput ];
		for ( Uint32 i = 0; i < possibleActions.Size(); ++i )
		{
			const SActivatedAction& aa = possibleActions[ i ];
			if ( aa.m_actionName == actionName )
			{
				activation = aa.m_activationValue;
				return true;
			}
		}
	}

	return false;
}

Bool CInteractionsManager::GetFriendlyNameForAction( const String& actionName, String& friendlyName ) const
{
	if ( actionName.Empty() )
	{
		return false;
	}

	CName gameInput;
	if ( m_actionToGameInput.Find( actionName, gameInput ) && gameInput )
	{
		const TDynArray< SActivatedAction >& possibleActions = m_gameInputToActions[ gameInput ];
		for ( Uint32 i = 0; i < possibleActions.Size(); ++i )
		{
			const SActivatedAction& aa = possibleActions[ i ];
			if ( aa.m_actionName == actionName )
			{
				friendlyName = aa.m_friendlyName;
				return true;
			}
		}
	}

	return false;
}

Bool CInteractionsManager::ProcessInteractionInput( const SInputAction& inputAction )
{
	if ( inputAction.m_aName.Empty() )
	{
		RED_ASSERT( !inputAction.m_aName.Empty(), TXT( "CInteractionsManager::ProcessInteractionInput: empty gameInput" ) );
		return false;
	}	
	if ( m_playerActivator == nullptr )
	{
		RED_ASSERT( m_playerActivator != nullptr, TXT( "CInteractionsManager::ProcessInteractionInput: there's no playerActivator created" ) );
		return false;
	}
	if ( GCommonGame == nullptr || GCommonGame->GetGuiManager() == nullptr )
	{
		return false;
	}

	CInteractionComponent* guiInteraction = GCommonGame->GetGuiManager()->GetActiveInteraction();
	// currently there's no interaction shown on gui
	if ( guiInteraction == nullptr )
	{
		return false;
	}

	// Get actions mapped to this key
	const TDynArray< SActivatedAction >& possibleActions = m_gameInputToActions[ inputAction.m_aName ];
	for ( TDynArray< SActivatedAction >::const_iterator it = possibleActions.Begin(); it != possibleActions.End(); it++ )
	{
		const SActivatedAction& action = *it;

		// Process only actions with matching activation value
		if ( action.m_actionName == guiInteraction->GetActionName() )
		{
			Bool canExecute =  guiInteraction->CanExecute();
			if ( canExecute )
			{
#ifdef PROFILE_INTERACTION
				PC_SCOPE_PIX( CanProcessInteractionInput );
#endif
				CallFunctionRet( this, CNAME( CanProcessInteractionInput ), inputAction, canExecute );
			}
			if ( canExecute )
			{
				guiInteraction->OnExecute();
			}
			return false;
		}
	}

	return false;
}

void CInteractionsManager::LoadActionMappings()
{
	CInputManager* inputMgr = GGame->GetInputManager();
	RED_ASSERT( inputMgr != nullptr );

	// Collect known game inputs
	const TDynArray< CName >& gameInputs = inputMgr->GetGameEventNames();
	const Uint32 numGameInputs = gameInputs.Size();

	// Clear action to game input and game input to actions mappings
	m_actionToGameInput.Clear();
	m_gameInputToActions.Clear();

	// Fill mappings
	C2dArray* actionToGameInputMappings = resControlsDefMgr.LoadAndGet< C2dArray >();
	if ( actionToGameInputMappings != nullptr )
	{
		Uint32 cols, rows;
		actionToGameInputMappings->GetSize( cols, rows );
		Uint32 actionColumn = actionToGameInputMappings->GetColumnIndex( TXT( "Action name" ) );
		Uint32 friendlyNameColumn = actionToGameInputMappings->GetColumnIndex( TXT( "Friendly name" ) );
		for ( Uint32 i = 0; i < rows; i++ )
		{
			// Add internal input definition
			String actionName = actionToGameInputMappings->GetValue( actionColumn, i );
			CName gameInputName = CName( actionName );
			String friendlyName = actionToGameInputMappings->GetValue( friendlyNameColumn, i );

			AddMapping( gameInputName, actionName, 1.0f, friendlyName ); 

			RED_ASSERT( GGame != nullptr );
			RED_ASSERT( GGame->GetInputManager() != nullptr );
			GGame->GetInputManager()->RegisterListener( this, gameInputName );
		}
	}
}

void CInteractionsManager::ClearActionMappings()
{
	m_actionToGameInput.Clear();
	m_gameInputToActions.Clear();

	RED_ASSERT( GGame != nullptr );
	RED_ASSERT( GGame->GetInputManager() != nullptr );
	GGame->GetInputManager()->UnregisterListener( this );
}

void CInteractionsManager::AddMapping( CName gameInput, const String& actionName, Float activation, const String& friendlyName )
{
	// Note: Activating this game input will trigger interaction with specified action name

	// Allow finding this action through given game input
	TDynArray< SActivatedAction >& mappedActions = m_gameInputToActions[ gameInput ];
	mappedActions.PushBackUnique( SActivatedAction( actionName, activation, friendlyName ) );

	// Allow finding this game input through given action name
	m_actionToGameInput[ actionName ] = gameInput;
}

void CInteractionsManager::AddInteractionComponent( CInteractionComponent* interaction )
{
	if ( interaction == nullptr )
	{
		RED_LOG( Interactions, TXT( "Trying to register NULL as interaction component. Please debug." ) );
		return;
	}
	if ( !GGame->IsActive() )
	{
		return;
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_treeLock );
	m_interactions.Add( interaction );

	CEntity* entity = interaction->GetEntity();
	RED_ASSERT( entity != nullptr );

	// caching info about entity being interactive one
	CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity );
	if ( gameplayEntity != nullptr )
	{
		gameplayEntity->GetInfoCache().Set( GICT_IsInteractive, true );
	}

#ifndef RED_FINAL_BUILD
	// unused tags cleanup (should be removed in the near future)
	TagList tags = entity->GetTags();
	if ( tags.HasTag( CNAME( InteractiveEntity ) ) )
	{
		tags.SubtractTag( CNAME( InteractiveEntity ) );
		entity->SetTags( tags );
	}
#endif

}

void CInteractionsManager::RemoveInteractionComponent( CInteractionComponent* interaction )
{
	if ( interaction == nullptr )
	{
		RED_LOG( Interactions, TXT( "Trying to unregister NULL interaction component. Please debug." ) );
		return;
	}

	// Check if the interaction we want to remove is current gui interaction.
	// If yes, we need to force gui update (because current interaction will be null and the best one can be the same).
	if ( !m_forceGuiUpdate && GCommonGame != nullptr && GCommonGame->IsActive() )
	{
		CGuiManager* guiManager = GCommonGame->GetGuiManager();
		if ( guiManager != nullptr && interaction == guiManager->GetActiveInteraction() )
		{
			m_forceGuiUpdate = true;
		}
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_treeLock );
	m_interactions.Remove( interaction );
	m_interactionsToUpdate.Erase( interaction );
}

void CInteractionsManager::AddInteractionActivator( CEntity *activator )
{
	if ( activator == nullptr )
	{
		RED_LOG( Interactions, TXT( "Trying to register NULL as interaction activator. Please debug." ) );
		return;
	}
	if ( !GGame->IsActive() )
	{
		return;
	}

	// Cache when in tick
	if ( m_isInTick )
	{
		m_activatorsToAdd.PushBack( activator );
		return;
	}

	// Make sure entity is not registered twice
	for ( Uint32 i = 0; i < m_activators.Size(); i++ )
	{
		CActivator* activatorDef = m_activators[i];
		if ( activatorDef->GetEntity() == activator )
		{
			RED_LOG( Interactions, TXT( "Trying to register entity '%s' twice as interaction activator. Please debug." ), activator->GetFriendlyName().AsChar() );
			return;
		}
	}

	if ( activator->IsA< CPlayer >() )
	{
		// if player activator was previously created (adding replacer for example)
		if ( m_playerActivator != nullptr )
		{
			m_playerActivator->SetEntity( activator );
		}
		else
		{
			m_playerActivator = new CPlayerActivator( activator );
		}		
	}
	else
	{
		// ACHTUNG!!! Only player-activators are allowed.
		// m_activators.PushBack( new CActivator( activator ) );
		RED_LOG( Interactions, TXT( "Trying to register non-player activator %s. That's not gonna happen." ), activator->GetFriendlyName().AsChar() );
	}
}

void CInteractionsManager::RemoveInteractionActivator( CEntity *activator )
{
	if ( activator == nullptr )
	{
		RED_LOG( Interactions, TXT( "Trying to register NULL as interaction activator. Please debug." ) );
		return;
	}

	// Cache when in tick
	if ( m_isInTick )
	{
		m_activatorsToRemove.PushBack( activator );
		return;
	}

	CActivator* foundActivator = nullptr;
	if ( m_playerActivator != nullptr && activator == m_playerActivator->GetEntity() )
	{
		foundActivator = m_playerActivator;
		m_playerActivator = nullptr;
	}
	else
	{
		for ( Uint32 i=0; i < m_activators.Size(); i++ )
		{
			if ( m_activators[i]->GetEntity() == activator )
			{
				foundActivator = m_activators[i];
				m_activators.RemoveAtFast( i );
				break;
			}
		}
	}

	if ( foundActivator != nullptr )
	{
		delete foundActivator;
	}
}

void CInteractionsManager::ApplyCachedActivatorAddRemove()
{
	RED_ASSERT( !m_isInTick );

	// Apply adds
	for ( Uint32 i = 0; i < m_activatorsToAdd.Size(); i++ )	
	{
		CEntity* activator = m_activatorsToAdd[i];
		if ( activator != nullptr )
		{
			AddInteractionActivator( activator );
		}
	}

	// Apply removes
	for ( Uint32 i = 0; i < m_activatorsToRemove.Size(); i++ )
	{
		CEntity* activator = m_activatorsToRemove[i];
		if ( activator != nullptr )
		{
			RemoveInteractionActivator( activator );
		}
	}

	m_activatorsToRemove.ClearFast();
	m_activatorsToAdd.ClearFast();
}

void CInteractionsManager::RegisterForUpdate( CInteractionComponent* interaction )
{
	if ( interaction == nullptr || !interaction->IsEnabled() )
	{
		return;
	}
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_treeLock );
	m_interactionsToUpdate.Insert( interaction );
}