/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "doorComponent.h"
#include "doorAttachment.h"
#include "lockableEntity.h"
#include "../core/gameSave.h"
#include "../engine/tickManager.h"

IMPLEMENT_RTTI_ENUM( EDoorState );
IMPLEMENT_ENGINE_CLASS( CDoorComponent );

RED_DEFINE_STATIC_NAME( OnActionNameChanged )
RED_DEFINE_STATIC_NAME( OnOpened )
RED_DEFINE_STATIC_NAME( OnClosed )
RED_DEFINE_STATIC_NAME( OnCombatStarted )
RED_DEFINE_STATIC_NAME( OnCombatEnded )
RED_DEFINE_STATIC_NAME( pushableAccumulatedYaw )


//////////////////////////////////////////////////////////////////////////////////////////////
CDoorComponent::CDoorComponent()
	: m_initialState( Door_Closed )
	, m_currentState( Door_Closed )
	, m_desiredState( Door_Closed )
	, m_openName( TXT( "Open" ) )
	, m_closeName( TXT( "Close" ) )
	, m_doorsEnebled( true )
	, m_isTrapdoor( false )
	, m_initialized ( false )
	, m_streamInit( false )
	, m_lastCalledCombatNotification( CName::NONE )
{ 
}

Bool CDoorComponent::IsLocked()
{
	// only lockable entities CAN be locked.
	W3LockableEntity* lockableEntity = Cast<W3LockableEntity>( GetEntity() );
	return ( lockableEntity ? lockableEntity->IsLocked() : false );
}

RED_DEFINE_STATIC_NAME( GetOpeningAngle )

Bool CDoorComponent::LazyInit( Bool openOtherSide )
{
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		Float openingAngle;
		if( CallFunctionRet( GetEntity(), CNAME( GetOpeningAngle ), openingAngle ) )
		{			 
			doorAttachment->SetOpenAngle( openOtherSide ? -openingAngle : openingAngle );
		}
		
		if( !m_initialized )
		{
			doorAttachment->OnAttached();	
			m_initialized = true;
		}
	}

	return m_initialized;
}

void CDoorComponent::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	m_currentState = m_initialState;
	m_desiredState = m_initialState;
}

void CDoorComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	Init();
}

void CDoorComponent::OnDetached( CWorld* world )
{
	Deinit();
	TBaseClass::OnDetached( world );
}

void CDoorComponent::OnStreamIn()
{
	TBaseClass::OnStreamIn();
	Init();
}

void CDoorComponent::OnStreamOut()
{
	Deinit();
	TBaseClass::OnStreamOut();
}

void CDoorComponent::Init()
{

	if( m_streamInit || !GetWorld() )
	{
		return;
	}

	CTickManager* tickManager = GetWorld()->GetTickManager();
	tickManager->AddToGroupDelayed( this, TICK_Main );

	SuppressTick( true, SR_Default );

	UpdateActionName();

	//CallEvent( CNAME( OnComponentAttached ) );

	m_streamInit = true;

	// after save desired state may be set to open
	if( m_desiredState == Door_Open )
	{
		Open( true, true );
	}

	auto ent = GetEntity();
	if( ent )
	{
		ent->CallEvent( CName( TXT( "OnDoorActivation" ) ) );
	}	
}



void CDoorComponent::Deinit()
{
	if( !m_streamInit )
	{
		return;
	}

	m_streamInit = false;

	CTickManager* tickManager = GetWorld()->GetTickManager();
	tickManager->RemoveFromGroup( this, TICK_Main );

	// notify door attachments of component being detached...
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		doorAttachment->OnDetached();
	}
}

void CDoorComponent::HandleCombat()
{
	CPlayer* player = Cast< CPlayer >( GGame->GetPlayerEntity() );
	if( !player )
		return;
	if( player->IsInCombat() )
	{
		if( m_lastCalledCombatNotification != CNAME( OnCombatStarted ) )
		{
			GetEntity()->CallEvent( CNAME( OnCombatStarted ) );
			m_lastCalledCombatNotification = CNAME( OnCombatStarted );
		}
	}
	else
	{
		if( m_lastCalledCombatNotification != CNAME( OnCombatEnded ) )
		{
			GetEntity()->CallEvent( CNAME( OnCombatEnded ) );
			m_lastCalledCombatNotification = CNAME( OnCombatEnded );
		}
	}
}

void CDoorComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );
	HandleCombat();

	// add potential door user once they are within radius
	Float activationRadiusSq = ( GetRangeMax() + 1.0f ) * ( GetRangeMax() + 1.0f );	

	// remove door users that have left the activation radius
	for( Uint32 i=0; i<m_doorUsers.Size(); ++i )
	{
		CActor* user = m_doorUsers[ i ].Get();
		if( !user )
			continue;
		if( !user || ( user->GetWorldPosition() - GetWorldPosition() ).SquareMag3() > activationRadiusSq )
		{
			m_doorUsers.RemoveAt( i-- );
		}
	}

	// update attachments
	Bool hasFinished = true;
	if( m_doorsEnebled || m_currentState != m_desiredState )
	{
	
		if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
		{
			// let door attachment update the door piece
			if( doorAttachment->Update( timeDelta ) )
			{
				// needs more time...
				hasFinished = false;
			}
		}
	}
	else
	{
		hasFinished = false;
	}

	if( hasFinished )
	{
		if( m_currentState != m_desiredState )
		{
			// update state
			m_currentState = m_desiredState;

			UpdateActionName();

			GetEntity()->CallEvent( CNAME( OnActionNameChanged ) );

			if( m_currentState == Door_Closed )
			{
				GetEntity()->CallEvent( CNAME( OnClosed ) );
			}
			else if( m_currentState == Door_Open )
			{
				GetEntity()->CallEvent( CNAME( OnOpened ) );
			}
		}

		if( m_doorUsers.Empty() )
		{
			SuppressTick( true, SR_Default );
		}		
	}

#if 0
	// temp debug
	GGame->GetVisualDebug()->AddText( CName( TXT( "T1" ) ), TXT( "Tick Active" ), GetWorldPosition(), true, 0, Color::WHITE, true, 0.1f );
#endif
}

void CDoorComponent::OnSaveGameplayState( IGameSaver* saver )
{
	CComponent::OnSaveGameplayState( saver );

	saver->WriteValue( CNAME( state ), m_desiredState );

	Float accumulatedYaw = 0.0f;
	if ( CDoorAttachment_GameplayPush* doorAttachment = Cast< CDoorAttachment_GameplayPush >( FindDoorAttachment() ) )
	{
		accumulatedYaw = doorAttachment->GetAccumulatedYaw();		
	}

	saver->WriteValue( CNAME( pushableAccumulatedYaw ), accumulatedYaw );		
}

void CDoorComponent::OnLoadGameplayState( IGameLoader* loader )
{
	CComponent::OnLoadGameplayState( loader );

	EDoorState currentState;
	loader->ReadValue( CNAME( state ), currentState );

	if( m_streamInit )
	{
		if ( currentState == Door_Open )
		{
			Open( true, true );
		}
		else
		{
			Close( true );
		}
	}
	else
	{
		m_desiredState = currentState;
	}

	m_savedPushableAccumulatedYaw = 0;
	loader->ReadValue( CNAME( pushableAccumulatedYaw ), m_savedPushableAccumulatedYaw );		
	if( CDoorAttachment_GameplayPush* attachment = Cast< CDoorAttachment_GameplayPush >( FindDoorAttachment() ) )
	{
		attachment->ForceAccumulatedYaw( m_savedPushableAccumulatedYaw );
	}
}

IDoorAttachment* CDoorComponent::FindDoorAttachment()
{
	for( auto it = GetChildAttachments().Begin(), end = GetChildAttachments().End(); it != end; ++it )
	{
		IDoorAttachment* doorAttachment = Cast<IDoorAttachment>( *it );
		if ( doorAttachment && !doorAttachment->IsBroken() )
		{
			return doorAttachment;
		}
	}
	m_initialized = false;
	return nullptr;
}


void CDoorComponent::AddDoorUser( CActor* actor )
{
	if( !actor )
	{
		return;
	}

	if( !LazyInit() )
	{
		return;
	}
	// add user
	m_doorUsers.PushBackUnique( actor );

	// start ticking
	Unsuppress();
}

Bool CDoorComponent::NotifyActivation( CEntity* activator )
{
	// add activator to list of door users
	AddDoorUser( Cast< CActor >( activator ) );
	return TBaseClass::NotifyActivation( activator );
}

void CDoorComponent::UpdateActionName()
{
	// update action name

	IDoorAttachment* doorAttachment = FindDoorAttachment();
	if( !doorAttachment )
		return;
	if( !IsLocked() && !doorAttachment->IfNeedsInteraction() )
	{
		m_actionName = String::EMPTY;
	}
	else
	{
		switch(m_currentState)
		{
		case Door_Open:
			if ( IsTrapdoor() )
			{
				// Trapdoor cannot be closed once opened.
				// Setting empty action name won't disable the whole component
				// just the interactions.
				m_actionName = String::EMPTY;
			}
			else
			{
				m_actionName = m_closeName;
			}
			break;

		case Door_Closed:
			m_actionName = m_openName;
			break;
		}
	}
	SetActionName( m_actionName );
}

void CDoorComponent::SetDesiredState( EDoorState newState )
{
	// set new desired state
	m_desiredState = newState;

	// unsuppress component (i.e. resume ticking)
	Unsuppress();

	// no action while opening / closing / locking
	m_actionName.Clear();
}

void CDoorComponent::OnExecute()
{
	// can't interact with the door while it is moving
	if( IsMoving() )
	{
		return;
	}

	// update action name
	switch(m_currentState)
	{
	case Door_Open:
		Close();
		break;

	case Door_Closed:
		if( !IsLocked() )
		{
			GetEntity()->CallEvent( CNAME( OnPlayerOpenedDoors ) );
			Open();
		}

		break;
	}

	TBaseClass::OnExecute();
	UpdateActionName();
}

void CDoorComponent::Unsuppress()
{
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		if ( IsTickSuppressed( SR_Default ) )
		{
			SuppressTick( false, SR_Default );
			doorAttachment->Unsuppressed();
		}
	}
}

void CDoorComponent::SetStateForced()
{
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		doorAttachment->SetStateForced();
	}
}

void CDoorComponent::funcOpen( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, forced, false );
	GET_PARAMETER( Bool, unlock, false );
	FINISH_PARAMETERS;

	if ( forced )
	{
		SetStateForced();
	}
	
	Open( false, unlock );
}

void CDoorComponent::funcClose( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, forced, false );
	FINISH_PARAMETERS;	

	if ( forced )
	{
		SetStateForced();
	}

	Close();
}

void CDoorComponent::funcIsOpen( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;	
	RETURN_BOOL( IsOpen() );
}

void CDoorComponent::funcIsLocked( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsLocked() ); 
}

void CDoorComponent::funcAddForceImpulse( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, origin, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, force, 0.0f );
	FINISH_PARAMETERS;

	if( !m_doorsEnebled )
	{
		return;
	}

	if( !LazyInit() )
	{
		return;
	}	

	// make sure we are ticked after adding a force
	Unsuppress();

	// pass impacts on to door attachments
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		doorAttachment->AddForceImpulse( origin, force );
	}	
}

void CDoorComponent::funcInstantClose( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Close( true );
}

void CDoorComponent::funcInstantOpen( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, unlock, false );
	FINISH_PARAMETERS;

	Open( true, unlock );
}


void CDoorComponent::funcAddDoorUser( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, nullptr );
	FINISH_PARAMETERS;

	AddDoorUser( actor.Get() );
}

void CDoorComponent::Open( Bool instant, Bool openLock, Bool otherSide )
{
	ASSERT( m_streamInit )
	if ( IsLocked() && openLock )
	{
		auto ent = GetEntity();
		if( ent )
		{
			ent->CallEvent( CName( TXT( "OnLockForced" ) ) );
		}
	}
	if( IsLocked() )
	{
		return;
	}

	if( !LazyInit( otherSide ) )
	{
		m_desiredState = Door_Open;
		return;
	}

	if ( instant )
	{
		// pass impacts on to door attachments
		if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
		{
			doorAttachment->InstantOpen();
			m_desiredState	=	Door_Open;
			m_currentState	=	Door_Open;
			UpdateActionName();
		}
	}
	else
	{
		SetDesiredState( Door_Open );
	}
}

void CDoorComponent::Close( Bool instant )
{
	ASSERT( m_streamInit )
	if( !LazyInit() )
	{
		m_desiredState = Door_Closed;
		return;
	}

	if ( instant )
	{
		// pass impacts on to door attachments
		if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
		{
			doorAttachment->InstantClose();
			m_desiredState	=	Door_Closed;
			m_currentState	=	Door_Closed;
			UpdateActionName();
		}
	}
	else
	{
		RED_ASSERT( !IsLocked() || m_currentState == Door_Closed, TXT("Opened door shouldn't be locked") );
		
		SetDesiredState( Door_Closed );
	}	
}

void CDoorComponent::SetEnabled( Bool enabled )
{
	if( enabled )
	{
		if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
		{
			if( !doorAttachment->IsInteractive() )
			{
				return;
			}
		}
	}
	TBaseClass::SetEnabled( enabled );
}

Bool CDoorComponent::IsInteractive()
{
	if ( IDoorAttachment* doorAttachment = FindDoorAttachment() )
	{
		return doorAttachment->IsInteractive();
	}
	return false;
}

void CDoorComponent::funcEnebleDoors( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, eneble, false );
	FINISH_PARAMETERS;

	m_doorsEnebled = eneble;
}

void CDoorComponent::funcIsInteractive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool isInteractive = IsInteractive();
	RETURN_BOOL( isInteractive );
}

void CDoorComponent::funcIsTrapdoor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	RETURN_BOOL( m_isTrapdoor );
}

void CDoorComponent::funcInvertMatrixForDoor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Matrix, mat.FullInverted() );
}

void CDoorComponent::funcUnsuppress( CScriptStackFrame& stack, void* result )
{	
	FINISH_PARAMETERS;
	Unsuppress();
}
