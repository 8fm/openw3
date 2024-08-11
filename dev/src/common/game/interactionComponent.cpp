/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "interactionComponent.h"

#include "questsSystem.h"
#include "interactionsManager.h"

#include "guiManager.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/drawableComponent.h"


CGatheredResource resControlsDef( TXT("gameplay\\globals\\controls.csv"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CInteractionComponent );

RED_DEFINE_STATIC_NAME( IsEnabledOnHorse );
RED_DEFINE_STATIC_NAME( OnInteractionAttached );

CInteractionComponent::CInteractionComponent()
	: m_checkCameraVisibility( true )
	, m_reportToScript( false )
	, m_isEnabledInCombat( false )
	, m_shouldIgnoreLocks( false )
	, m_lastUpdatedPosition( Vector::ZEROS )
	, m_flags( 0 )
{
}

void CInteractionComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

#ifdef PROFILE_INTERACTION
	PC_SCOPE_PIX( CInteractionComponent_OnAttached );
#endif

	OnActionNameChanged();

	// Only for game world
	if ( world == GGame->GetActiveWorld() && !IsManualTestingOnly() )
	{
		// Register in interaction system
		GCommonGame->GetSystem< CInteractionsManager >()->AddInteractionComponent( this );
		m_lastUpdatedPosition = GetWorldPositionRef();
	}
}

void CInteractionComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Only for game world
	if ( world == GGame->GetActiveWorld() )
	{
		// Unregister from interaction manager
		if ( !IsManualTestingOnly() )
		{
			CInteractionsManager* manager = GCommonGame->GetSystem< CInteractionsManager >();
			// When the game is shutdown, the systems are set to NULL so make sure we have
			// an interaction manager before trying to use it
			if ( manager )
			{
				manager->RemoveInteractionComponent( this );
			}
		}
	}
}

void CInteractionComponent::OnStreamIn()
{
	TBaseClass::OnStreamIn();

	CEntity* entity = GetEntity();
	if ( entity != nullptr )
	{
		THandle< CInteractionComponent > thisHandle( this );
		entity->CallEvent( CNAME( OnInteractionAttached ), thisHandle );
	}
}


void CInteractionComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	saver->WriteValue( CNAME(e), m_isEnabledInCombat );
}

void CInteractionComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	m_isEnabledInCombat = loader->ReadValue< Bool >( CNAME(e) );
}

void CInteractionComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CInteractionComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
	if ( IsEnabled() )
	{
		UpdatePositionInManager( false );
	}
}

void CInteractionComponent::SetEnabled( Bool enabled )
{
	TBaseClass::SetEnabled( enabled );
	if ( IsEnabled() )
	{
		UpdatePositionInManager( true );
	}
}

void CInteractionComponent::SetFriendlyName( String friendlyName )
{
	TBaseClass::SetFriendlyName( friendlyName );

	m_flags &= ~ICF_HasGenericFriendlyName;
}

void CInteractionComponent::UpdatePositionInManager( Bool force )
{
	// last update position is still zero
	// so the component hasn't been registered in manager yet
	if ( m_lastUpdatedPosition == Vector::ZEROS )
	{
		return;
	}
	const Vector& worldPos = GetWorldPositionRef();
	const Float MOVE_TRESHOLD = 0.5f;
	if ( force || !( worldPos - m_lastUpdatedPosition ).AsVector3().IsAlmostZero( MOVE_TRESHOLD ) )
	{
		m_lastUpdatedPosition = worldPos;
		CInteractionsManager* manager = GCommonGame->GetSystem< CInteractionsManager >();
		if ( manager != nullptr )
		{
			manager->RegisterForUpdate( this );
		}
	}
}

Bool CInteractionComponent::NotifyActivation( CEntity* activator )
{
	// Activated by empty activator, this is not allowed
	if ( !activator )
	{
		RED_LOG( Interactions, TXT( "Interaction '%ls' was activated by NULL activator." ), GetFriendlyName().AsChar() );
		return false;
	}

	// Make sure we are not activating interaction twice by the same entity
	if ( m_activatorsList.Exist( activator ) )
	{
		RED_LOG( Interactions, TXT( "Interaction '%ls' activated twice by entity '%ls'" ), GetFriendlyName().AsChar(), activator->GetFriendlyName().AsChar() );
		return false;
	}

	// Add to list of activators
	m_activatorsList.PushBackUnique( activator );

	return true;
}

void CInteractionComponent::OnActivate( CEntity* activator )
{
	if ( !NotifyActivation( activator ) )
	{
		return;
	}

	// Report activation to script
	if ( m_reportToScript && CallEvent( CNAME( OnInteractionActivated ), GetName(), THandle<CEntity>(activator) ) != CR_EventSucceeded )
	{
		// HACK... needed because looting deletes the entity...
		if ( !activator->HasFlag( OF_Discarded ) && !GetEntity()->HasFlag( OF_Discarded ) )
		{
			GetEntity()->CallEvent( CNAME( OnInteractionActivated ), GetName(), THandle<CEntity>(activator) );
		}
	}
}

Bool CInteractionComponent::NotifyDeactivation( CEntity* activator )
{
	// Deactivated by empty activator, this is not allowed
	if ( !activator )
	{
		RED_LOG( Interactions, TXT( "Interaction '%ls' was deactivated by NULL activator." ), GetFriendlyName().AsChar() );
		return false;
	}

	// Make sure we are not deactivated by entity we was not activated before
	if ( !m_activatorsList.Exist( activator ) )
	{
		RED_LOG( Interactions, TXT( "Interaction '%ls' was not activated before by entity '%ls'" ), GetFriendlyName().AsChar(), activator->GetFriendlyName().AsChar() );
		return false;
	}

	// Remove from list
	m_activatorsList.Remove( activator );

	return true;
}

void CInteractionComponent::OnDeactivate( CEntity* activator )
{
	if ( !NotifyDeactivation( activator ) )
	{
		return;
	}

	// Report deactivation to script
	if ( m_reportToScript && CallEvent( CNAME( OnInteractionDeactivated ), GetName(), THandle<CEntity>(activator) ) != CR_EventSucceeded )
	{
		// HACK... needed because looting deletes the entity...
		if ( !activator->HasFlag( OF_Discarded ) && !GetEntity()->HasFlag( OF_Discarded ) )
		{
            GetEntity()->CallEvent( CNAME( OnInteractionDeactivated ), GetName(), THandle<CEntity>(activator) );
		}
	}
}

Bool CInteractionComponent::NotifyExecution()
{
	// INVENTORY HACK.. TO BE REMOVED
	if ( GetEntity()->IsDestroyed() )
	{
		return false;
	}

	// Notify owner
	GetEntity()->OnProcessInteractionExecute( this );

	// Notify activators
	for ( Uint32 i=0; i < m_activatorsList.Size(); i++ )
	{
		CEntity* activator = m_activatorsList[i];
		if ( activator && activator != GetEntity() )
		{
			activator->OnProcessInteractionExecute( this );
		}
	}

	// Pass to quest system
	if ( GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->OnInteractionExecuted( GetName(), GetEntity() );
	}

	return true;
}

void CInteractionComponent::OnExecute()
{
	RED_ASSERT( !m_activatorsList.Empty() );

	// Report to script
	{
		THandle< CEntity > activator( m_activatorsList.Empty() ? NULL : m_activatorsList[0] );
		
		if( CallEvent( CNAME( OnInteraction ), m_actionName, activator ) != CR_EventSucceeded )
		{
			GetEntity()->CallEvent( CNAME( OnInteraction ), m_actionName, activator );
		}
	}

	if ( !NotifyExecution() )
	{
		return;
	}
}

String CInteractionComponent::GetInteractionFriendlyName()
{
	if ( m_friendlyName.Empty() )
	{
		m_flags |= ICF_HasGenericFriendlyName;
		UpdateGenericFriendlyName();
	}
	return m_friendlyName;
}

Bool CInteractionComponent::UpdateGenericFriendlyName()
{
	// if there is no custom friendly name for current interaction, get generic one from controls.csv
	CInteractionsManager* interactionsManager = GCommonGame->GetSystem< CInteractionsManager >();
	if ( interactionsManager != nullptr )
	{
		String friendlyName;
		if ( interactionsManager->GetFriendlyNameForAction( m_actionName, friendlyName ) )
		{
			m_friendlyName = friendlyName;
			return true;
		}
	}
	return false;
}

EInputKey CInteractionComponent::GetInteractionKey() const
{
	CName gameInput;
	Float activation;

	auto IM = GGame->GetInputManager();
	RED_ASSERT( IM );


	Bool useKeyboard	= IM->LastUsedPCInput();
	Bool usePad			= IM->LastUsedGamepad();

	if( !useKeyboard && !usePad )
	{
		return IK_None;
	}

	if ( GCommonGame->GetSystem< CInteractionsManager >()->GetGameInputForAction( m_actionName, gameInput, activation ) && gameInput )
	{
		return IM->FindFirstIKForGameInput( gameInput, activation, usePad );
	}

	// Not found
	return IK_None;
}

CName CInteractionComponent::GetInteractionGameInput() const
{
	CName gameInput;
	Float activation;
	GCommonGame->GetSystem< CInteractionsManager >()->GetGameInputForAction( m_actionName, gameInput, activation );
	return gameInput;
}

void CInteractionComponent::SetActionName( const String& actionName )
{
	m_actionName = actionName;
	OnActionNameChanged();
}

Bool CInteractionComponent::CanExecute() const
{
	if ( IsTalkInteraction() )
	{
		CActor* actor = Cast< CActor >( GetEntity() );
		if ( actor != nullptr )
		{
			return actor->CanTalk( false );
		}
	}
	return true;
}

void CInteractionComponent::OnActionNameChanged()
{
	if ( m_actionName == TXT( "Talk" ) )
	{
		m_flags |= ICF_IsTalkInteraction;
	}
	else
	{
		m_flags &= ~ICF_IsTalkInteraction;
	}
	if ( m_flags & ICF_HasGenericFriendlyName )
	{
		UpdateGenericFriendlyName();
	}
}

Bool CInteractionComponent::ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const
{
	// This method is called VERY often, so please do not submit this line uncommented:
	//PC_SCOPE( ActivationTest );

	// Disabled interactions cannot be activated
	if ( !m_isEnabled )
	{
		return false;
	}

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( CombatTest );
#endif

		// fun fact: IsEnabledInCombat is ignored when we're swimming
		if ( !m_isEnabledInCombat && ( activatorData.IsInCombat() && !activatorData.IsSwimming() ) )
		{
			return false;
		}
	}

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( VehicleTest );
#endif

		if ( !IsEnabledOnVehicles() && activatorData.IsUsingVehicle() )
		{
			return false;
		}
	}

	// Base "fast" test
	if ( !CInteractionAreaComponent::ActivationFastTest( activatorData ) )
	{
		return false;
	}

	// Camera visibility test
	if ( m_checkCameraVisibility )
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( CheckCameraVisibility );
#endif

		if ( !CameraVisibilityTest( activatorData ) )
		{
			return false;
		}
	}

	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( CircumstancesTest );
#endif

		// Test some more creepy internal gameplay circumstances test
		if ( !CircumstancesTest( activatorData ) )
		{
			return false;
		}
	}

	// Base "slow" test
	if ( !CInteractionAreaComponent::ActivationSlowTest( activatorData ) )
	{
		return false;
	}

	return true;
}

Bool CInteractionComponent::CameraVisibilityTest( const SActivatorData& activatorData ) const
{
	CEntity* parentEntity = GetEntity();
	RED_ASSERT( parentEntity != nullptr, TXT( "InteractionComponent is not attached to entity" ) );
	if ( !parentEntity->WasVisibleLastFrame() )
	{
		// If entity wasn't visible we need to check if it has any DrawableComponents.
		CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( parentEntity );
		if ( gameplayEntity != nullptr && gameplayEntity->GetInfoCache().Get( gameplayEntity, GICT_HasDrawableComponents ) )
		{
			return false;
		}
		// There are no drawable components, so let's preform real "is in camera" test.
		if ( !GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( GetWorldPositionRef() + Vector( 0.0f, 0.0f, m_height * 0.5f, 0.0f ) ) )
		{
			return false;
		}
	}
	return true;
}

Bool CInteractionComponent::CircumstancesTest( const SActivatorData& activatorData ) const
{
	if ( IsTalkInteraction() && activatorData.IsPlayer() )
	{
#ifdef PROFILE_INTERACTION
		PC_SCOPE_PIX( OnInteractionTalkTest );
#endif
		if ( !activatorData.CanStartTalk() )
		{
			return false;
		}
		CActor* actor = Cast< CActor >( GetEntity() );
		if ( actor != nullptr )
		{
			if ( !actor->CanTalk( true ) || ( !m_isEnabledInCombat && actor->IsInCombat() ) )
			{
				return false;
			}
			return ( CR_EventSucceeded == actor->CallEvent( CNAME( OnInteractionTalkTest ) ) );
		}
	}

	return true;
}

void CInteractionComponent::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	// as there is only one property with 2da value selection editor, it is ok to ignore the property parameter.
	valueProperties.m_array = resControlsDef.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("Action name");
}

void CInteractionComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// don't draw too distant interaction while playing game
	if ( GCommonGame != nullptr && GCommonGame->GetPlayer() != nullptr )
	{
		const Vector3 diff = GCommonGame->GetPlayer()->GetWorldPositionRef() - GetWorldPositionRef();
		// to avoid unnecessary mess
		if ( MAbs( diff.X ) > 30 || MAbs( diff.Y ) > 30 || MAbs( diff.Z ) > 10 )
		{
			return;
		}
	}

	if ( flag == SHOW_Logic )
	{
		Vector worldPosition = GetWorldPosition();

		// Draw activators
		Int32 offset = 0;
		Vector textPos = worldPosition + Vector( 0.f, 0.f, 1.f );
		Color color = GetColor();
		for ( Uint32 i=0; i<m_activatorsList.Size(); ++i )
		{
			CEntity* ent = m_activatorsList[i];
			const String& entName = ent->GetName();
			frame->AddDebugText( textPos, entName, 0, offset, false, color );
			offset -= 1;
		}
	}
}

Bool CInteractionComponent::IsEnabledOnVehicles() const
{
	// Since this information is stored in "scripted" isEnabledOnHorse
	// and we don't want to change its name - thus resave all entities
	// - we need to use scripted variable and store its value here.
	if ( !( m_flags & ICF_IsEnabledOnVehiclesCached ) )
	{
		Bool ret = false;
		CallFunctionRet( const_cast< CInteractionComponent* >( this ), CNAME( IsEnabledOnHorse ), ret );
		m_flags |= ICF_IsEnabledOnVehiclesCached;
		if ( ret )
		{
			m_flags |= ICF_IsEneabledOnVehicles;
		}
	}
	return ( m_flags & ICF_IsEneabledOnVehicles ) > 0;
}

void CInteractionComponent::GetStorageBounds( Box& box ) const
{
	box.Min = GetWorldPosition() - Vector( m_rangeMax, m_rangeMax, s_activatorHeightTolerance );
	box.Max = GetWorldPosition() + Vector( m_rangeMax, m_rangeMax, m_height + s_activatorHeightTolerance );
}

Color CInteractionComponent::GetColor() const
{
	return IsActive() ? Color(255,0,0) : Color(100,255,100);
}

//////////////////////////////////////////////////////////////////

void CInteractionComponent::funcGetActionName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( m_actionName );
}

void CInteractionComponent::funcSetActionName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, actionName, String::EMPTY );
	FINISH_PARAMETERS;

	SetActionName( actionName );
}

void CInteractionComponent::funcGetInteractionFriendlyName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( GetInteractionFriendlyName() );
}

void CInteractionComponent::funcGetInteractionKey( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( (Int32)GetInteractionKey() );
}


void CInteractionComponent::funcGetInputActionName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetInteractionGameInput() );
}