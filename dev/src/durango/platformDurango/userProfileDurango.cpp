/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include <eh.h>
#include <ppltasks.h>
#include <collection.h>

#include "userProfileDurango.h"

#if !defined( NO_TELEMETRY )
#include "../../common/core/redTelemetryServicesManager.h"
#endif

#include "../../common/engine/gameSaveManager.h"
#include "../../common/game/gameSaver.h"

#include "inputDeviceManagerDurango.h"

extern Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;

#define INVALID_USERID static_cast< Uint32 >( -1 )

#define LOG_REFRESH( format, ... ) RED_LOG( ProfileRefresh, format, ##__VA_ARGS__ )

#define NEEDS_REFRESH( type, reason ) LOG_REFRESH( MACRO_TXT( "Refresh Requested - Reason: %ls" ), MACRO_TXT( reason ) ); NeedsRefresh( type )

namespace Config
{
	TConfigVar< Float > cvHackStartScreenInputDelay( "Hack", "StartScreenInputDelay", 1.f );
}
static CTimeCounter HACK_StartScreenTimer;

//////////////////////////////////////////////////////////////////////////
// CUserProfileManagerDurango
//////////////////////////////////////////////////////////////////////////
CUserProfileManagerDurango::CUserProfileManagerDurango()
	: m_userQueuedForSignin( nullptr )
	, m_activeUserStub( nullptr )
	, m_signoutDeferral( nullptr )
	, m_activeGamepad( nullptr )
	, m_activeUserObject( nullptr )
	, m_liveContext( nullptr )
	, m_activeUserId( INVALID_USERID )
	, m_userObjectReestablished( false )
	, m_cachedGamepads( nullptr )
	, m_managerState( eWaitingForNewActiveUserEngagement )
	, m_plmSuspended( false )
	, m_accountPickerActive( false )
	, m_accountPickerOperation( nullptr )
	, m_currentLoadOpProgress( LOAD_NotInitialized )
	, m_loadSaveState( STATE_None )
	, m_connectedStorageSpaceAsyncOperation( nullptr )
	, m_connectedStorageSpace( nullptr )
	, m_saveBuffer( ref new Buffer( SAVE_BUFFER_SIZE ) )
	, m_screenshotBuffer( ref new Buffer( CGameSaver::SCREENSHOT_BUFFER_SIZE ) )
	, m_userSettingsBuffer( ref new Buffer( MAX_USERSETTINGS_SIZE ) )
	, m_contentDescriptorBuffer( ref new Buffer( MAX_CONTENTDESCRIPTOR_SIZE ) )
	, m_screenshotDataReadAction( nullptr )
	, m_initializedLoadSaveInfoIndex( -1 )
	, m_saveInfosUpdateState( LIST_NeedsUpdate )
	, m_screenshotDataRead( false )
	, m_userSettingsState( SETTINGS_NotAvailable )
	, m_refreshRequested( RT_None )
	, m_refreshOngoing( RT_None )
	, m_ignoreSaveUserSettingsRequest( true )
	, m_saveLock( -1 )
	, m_writerCreated( false )
{
	InitEventHandlers();

	// Standard initialisation
	NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Initialisation (Constructor)" );
}

CUserProfileManagerDurango::~CUserProfileManagerDurango()
{
}

RED_INLINE Bool CUserProfileManagerDurango::IsValidActiveUser( IUser^ user )
{
	if( user )
	{
		Bool isSignedIn	= user->IsSignedIn;
		Bool isGuest	= user->IsGuest;

		return isSignedIn && !isGuest;
	}

	return false;
}

RED_INLINE Bool CUserProfileManagerDurango::HasValidActiveUser() const
{
	if( IsValidActiveUser( m_activeUserStub ) )
	{
		return true;
	}

	// This is necessary as even if we're technically signed out, we've already picked out the new
	// active user, so if the game asks we respond true
	return IsValidActiveUser( m_userQueuedForSignin );
}

const Char*	CUserProfileManagerDurango::GetPlatformUserIdAsString()
{
	if ( m_activeUserStub )
	{
		return m_activeUserStub->XboxUserId->Begin();
	}
	return NULL;
}

void CUserProfileManagerDurango::InitEventHandlers()
{
	// Ignoring as ControllerPairingChanged as the whitepaper suggests to for "lobby" apps.
	// TBD: Once we have an active user and controller, it might be better to do a "cheaper" Refresh that just checks if the user or gamepad in the event is the active one.
	//FIXME: TCR TBD - since these events are async, how soon must you react to them? Or does it not matter if it's by the next tick?

	typedef Windows::Xbox::System::UserAddedEventArgs			UserAddedEventArgs;
	typedef Windows::Xbox::System::UserRemovedEventArgs			UserRemovedEventArgs;
	typedef Windows::Xbox::System::SignOutStartedEventArgs		SignOutStartedEventArgs;
	typedef Windows::Xbox::System::SignOutCompletedEventArgs	SignOutCompletedEventArgs;
	typedef Windows::Xbox::Input::GamepadAddedEventArgs			GamepadAddedEventArgs;
	typedef Windows::Xbox::Input::GamepadRemovedEventArgs		GamepadRemovedEventArgs;

	User::UserAdded += ref new Windows::Foundation::EventHandler< UserAddedEventArgs^ >
	(
		[ this ]( Platform::Object^, UserAddedEventArgs^ args )
		{
			// User added. We may not need to do anything at this point, but it doesn't hurt
			NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "User Added" );
		}
	);

	User::UserRemoved += ref new Windows::Foundation::EventHandler< UserRemovedEventArgs^ >
	(
		[ this ]( Platform::Object^, UserRemovedEventArgs^ args )
		{
			// A User has been removed, so we need to do a complete refresh in case the user removed was the active user
			NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "User Removed" );
		}
	);

	User::SignOutStarted += ref new Windows::Foundation::EventHandler< SignOutStartedEventArgs^ >
	(
		[ this ]( Platform::Object^, SignOutStartedEventArgs^ args )
		{
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_signoutDeferralMutex );

				if( m_activeUserStub && args->User == FindUserFromInterface( m_activeUserStub ) )
				{
					RED_FATAL_ASSERT( m_signoutDeferral == nullptr, "It should be impossible to receive a sign out message for the same account more than once" );

					m_signoutDeferral = args->GetDeferral();
					LOG_ENGINE(TXT("SignOutStarted deferral acquired"));

					NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Sign out process has begun" );
				}
				else
				{
					const Bool hasActiveUserStub = m_activeUserStub != nullptr;
					LOG_ENGINE(TXT("SignOutStarted deferral not required. hasActiveUserStub=%d"), hasActiveUserStub);
				}
			}
		}
	);

	Gamepad::GamepadAdded += ref new Windows::Foundation::EventHandler< GamepadAddedEventArgs^ >
	(
		[ this ]( Platform::Object^, GamepadAddedEventArgs^ args )
		{
			// Controller added, simply refresh the cached list of pads
			NEEDS_REFRESH( RT_Basic, "Controller Added" );
		}
	);

	Gamepad::GamepadRemoved += ref new Windows::Foundation::EventHandler< GamepadRemovedEventArgs^ >
	(
		[ this ]( Platform::Object^, GamepadRemovedEventArgs^ args )
		{
			if( args->Gamepad == m_activeGamepad )
			{
				// Active controller removed, do a complete refresh (without any automatic controller assignment -
				// the user needs to select the pad manually)
				NEEDS_REFRESH( RT_ManualControllerEngagement, "Active controller removed" );
			}
			else
			{
				// Controller removed, not the one used by the active player (or no active player),
				// simply refresh the cached list of pads
				NEEDS_REFRESH( RT_Basic, "Controller removed" );
			}
		}
	);
}

void CUserProfileManagerDurango::AddGamepadListener( IUserProfileGamepadListenerDurango* listener )
{
	m_gamepadListeners.PushBack( listener );
}

void CUserProfileManagerDurango::RemoveGamepadListener( IUserProfileGamepadListenerDurango* listener )
{
	m_gamepadListeners.Remove( listener );
}

void CUserProfileManagerDurango::OnEnterSuspendState()
{
	RED_LOG( Controllers, TXT( "Enter Suspend" ) );
	m_plmSuspended = true;
}

void CUserProfileManagerDurango::OnExitSuspendState()
{
	RED_LOG( Controllers, TXT( "Exit Suspend" ) );

	m_plmSuspended = false;

	// We've resumed from suspend. Since we haven't had any events from the OS
	// (Or those we have had are unreliable and therefore ignored) we need to
	// automatically reestablish the active user (if they signed out and back again),
	// and the active pad (if it was disconnected and/or reconnected)
	NEEDS_REFRESH( RT_ReacquireAfterSuspend, "Resume after suspend" );
}

void CUserProfileManagerDurango::OnEnterConstrainedState()
{
	RED_LOG( Controllers, TXT( "Enter Constrain" ) );
}

void CUserProfileManagerDurango::OnExitConstrainedState()
{
	RED_LOG( Controllers, TXT( "Exit Constrain" ) );
}

void CUserProfileManagerDurango::NeedsRefresh( TRefreshType newType )
{
	// This way we ignore erroneous signin/out events from Visual Studio
	// until we're no longer suspended, at which point we refresh anyway
	if ( !m_plmSuspended )
	{
		m_refreshRequested.Or( newType );
	}
}

void CUserProfileManagerDurango::Update()
{
#ifdef RED_LOGGING_ENABLED
	// Logs on a timeout or if something relevant changed
	LogUserProfileState();
#endif
	
	CUserProfileManager::Update();

	OnSaveSystemUpdate();

#ifndef RED_FINAL_BUILD
	static Bool blarg = true;

	if( blarg )
	{
		DebugPrintUserAndControllerBindings( m_activeUserStub );

		blarg = false;
	}
#endif // RED_FINAL_BUILD

	// Note: even if constrained, we need to still process events, like users signing out.
	RefreshTick();

	// If have a valid active user, then handle that case and don't let refreshing interfere
	// The refresh async callback will update the active user and gamepad on the UI thread
	if ( !HandleInput() )
	{
		HandleUserEngagement();
	}
}

Bool CUserProfileManagerDurango::HandleInput()
{
	if ( m_managerState == eHasSignedInUserWithGamepad )
	{
		return ResetState();
	}

	return false;
}

Bool CUserProfileManagerDurango::ResetState()
{
	Bool hasValidActiveUser = IsValidActiveUser( m_activeUserStub );
	Bool hasValidQueuedUser = IsValidActiveUser( m_userQueuedForSignin );

#ifdef RED_LOGGING_ENABLED
	const EManagerState prevManagerState = m_managerState;
#endif

	// Saving but no valid user (sign out just occurred)
	const Bool isSaveSystemBusy = IsSaveSystemBusy(); // cache a transient value
	const Bool hasSignoutDeferral = m_signoutDeferral != nullptr;
	if( !hasValidActiveUser && ( isSaveSystemBusy || hasSignoutDeferral ) )
	{
		m_managerState = eWaitingForUserSignOut;
		LOG_REFRESH( TXT( "ResetState() prevManagerState=%ls -> eWaitingForUserSignOut, isSaveSystemBusy=%d, hasSignoutDeferral=%d" ),
			GetManagerStateTxtForLog(prevManagerState), isSaveSystemBusy, hasSignoutDeferral );
	}

	else if( !hasValidActiveUser && !hasValidQueuedUser )
	{
		m_managerState = eWaitingForNewActiveUserEngagement;
		LOG_REFRESH( TXT( "ResetState() prevManagerState=%ls -> eWaitingForNewActiveUserEngagement" ),
			GetManagerStateTxtForLog(prevManagerState) );
	}

	// queued active user and a refresh scheduled
	else if( hasValidQueuedUser && m_refreshRequested.GetValue() >= RT_ManualControllerEngagement )
	{
		m_managerState = eWaitingForNewActiveUserRefresh;
		LOG_REFRESH( TXT( "ResetState() prevManagerState=%ls -> eWaitingForNewActiveUserRefresh" ), GetManagerStateTxtForLog(prevManagerState) );
	}

	// active user and no game pad
	else if ( !HasValidActiveGamepad() )
	{
		m_managerState = eWaitingForCurrentActiveUserEngagement;
		LOG_REFRESH( TXT( "ResetState() prevManagerState=%ls -> eWaitingForCurrentActiveUserEngagement" ), GetManagerStateTxtForLog(prevManagerState) );
	}

	// All good
	else
	{
		m_managerState = eHasSignedInUserWithGamepad;
		//LOG_REFRESH( TXT( "ResetState() eHasSignedInUserWithGamepad" ) );
		return true;
	}

	return false;
}

#ifdef RED_LOGGING_ENABLED
const Char* CUserProfileManagerDurango::GetSaveStateTxtForLog( ELoadSaveState state )
{
	switch( state )
	{
		case STATE_None:				return TXT("STATE_None");
		case STATE_InitializeLoad:		return TXT("STATE_InitializeLoad");
		case STATE_LoadInitialized:		return TXT("STATE_LoadInitialized");
		case STATE_ModalUI:				return TXT("STATE_ModalUI");
		case STATE_NoSaveMode:			return TXT("STATE_NoSaveMode");
		case STATE_Updating:			return TXT("STATE_Updating");
		case STATE_DeletingSave:		return TXT("STATE_DeletingSave");
		case STATE_SaveInitialized:		return TXT("STATE_SaveInitialized");
		case STATE_Saving:				return TXT("STATE_Saving");
		case STATE_Ready:				return TXT("STATE_Ready");
		default:
			return TXT("<Unknown>");
	}
};

const Char* CUserProfileManagerDurango::GetManagerStateTxtForLog( EManagerState state )
{
	switch(state)
	{
	case eWaitingForNewActiveUserEngagement:		return TXT("eWaitingForNewActiveUserEngagement");
	case eWaitingForNewActiveUserRefresh:			return TXT("eWaitingForNewActiveUserRefresh");
	case eWaitingForCurrentActiveUserEngagement:	return TXT("eWaitingForCurrentActiveUserEngagement");
	case eWaitingForUserSignIn:						return TXT("eWaitingForUserSignIn");
	case eWaitingForUserSignOut:					return TXT("eWaitingForUserSignOut");
	case eHasSignedInUserWithGamepad:				return TXT("eHasSignedInUserWithGamepad");
	default:
		return TXT("<Unknown>");
	}
}

const Char* CUserProfileManagerDurango::GetRefreshStateTxtForLog( ERefreshState state )
{
	switch(state)
	{
	case eRS_Inactive:			return TXT("eRS_Inactive");
	case eRS_CachePads:			return TXT("eRS_CachePads");
	case eRS_User:				return TXT("eRS_User");
	case eRS_ConnectedStorage:	return TXT("eRS_ConnectedStorage");
	case eRS_LiveContext:		return TXT("eRS_LiveContext");
	case eRS_Controllers:		return TXT("eRS_Controllers");
	case eRS_Finished:			return TXT("eRS_Finished");
	default:
		return TXT("<Unknown>");
	};
}

const Char* CUserProfileManagerDurango::GetRefreshSubstateTxtForLog( ERefreshSubState substate )
{
	switch (substate)
	{
	case ERefreshSubState::eRTS_Inactive:	return TXT("eRTS_Inactive");
	case ERefreshSubState::eRTS_Start:		return TXT("eRTS_Start");
	case ERefreshSubState::eRTS_Ongoing:	return TXT("eRTS_Ongoing");
	default:
		return TXT("<Unknown>");
	}
}

const Char* CUserProfileManagerDurango::GetRefreshTypeTxtForLog( TRefreshType type )
{
	switch( type )
	{
	case RT_None:									return TXT("RT_None");
	case RT_Basic:									return TXT("RT_Basic");
	case RT_ManualControllerEngagement:				return TXT("RT_ManualControllerEngagement");
	case RT_AutomaticControllerReestablishment:		return TXT("RT_AutomaticControllerReestablishment");
	case RT_ReacquireAfterSuspend:					return TXT("RT_ReacquireAfterSuspend");
	default:
		return TXT("<Unknown>");
	}
}

void CUserProfileManagerDurango::LogUserProfileState()
{
	static EngineTime lastLogHeartbeat;
	static const Float LOG_HEARTBEAT = 30.f;

	struct LogInfo
	{
		EManagerState		m_managerState;
		TRefreshType		m_refreshRequested;
		ELoadSaveState		m_loadSaveState;
		Uint32				m_disableInput;
		TRefreshType		m_refreshOngoing;
		Uint32				m_refreshStatesActive;
		Uint32				m_refreshStatesFinished;
		ERefreshState		m_refreshState;
		ERefreshSubState	m_refreshSubState;
		Bool				m_isSaveSystemBusy;
		Bool				m_exitRefresh;
		Bool				m_hasSignoutDeferral;

		LogInfo()
		{
			Red::System::MemoryZero( this, sizeof(*this) );
		}

		Bool operator==(const LogInfo& rhs ) const
		{
			return m_managerState == rhs.m_managerState &&
				   m_refreshRequested == rhs.m_refreshRequested &&
				   m_loadSaveState == rhs.m_loadSaveState &&
				   m_disableInput == rhs.m_disableInput &&
				   m_refreshOngoing == rhs.m_refreshOngoing &&
				   m_refreshStatesActive == rhs.m_refreshStatesActive &&
				   m_refreshStatesFinished == rhs.m_refreshStatesFinished &&
				   m_refreshState == rhs.m_refreshState &&
				   m_refreshSubState == rhs.m_refreshSubState &&
				   m_isSaveSystemBusy == rhs.m_isSaveSystemBusy &&
				   m_exitRefresh == rhs.m_exitRefresh &&
				   m_hasSignoutDeferral == rhs.m_hasSignoutDeferral;
		}

		Bool operator!=( const LogInfo& rhs ) const { return !(*this == rhs); }

		static void Print( LogInfo& info )
		{
			WARN_ENGINE(TXT("CUserProfileManagerDurango in state '%ls'"), GetManagerStateTxtForLog( info.m_managerState ));
			WARN_ENGINE(TXT("\thasSignoutDeferral=%d, refreshRequested(mask)=0x%08X, loadSaveState=%ls, isSaveSystemBusy=%d"),
				info.m_hasSignoutDeferral, info.m_refreshRequested, GetSaveStateTxtForLog(info.m_loadSaveState), info.m_isSaveSystemBusy);	
			WARN_ENGINE(TXT("\tdisableInput=0x%08X, refreshOngoing=%ls"), info.m_disableInput, GetRefreshTypeTxtForLog(info.m_refreshOngoing) );
			WARN_ENGINE(TXT("\trefreshStatesActive=0x%08X, refreshStatesFinished=0x%08X, refreshState=%ls, refreshSubstate=%ls"),
				info.m_refreshStatesActive, info.m_refreshStatesFinished, GetRefreshStateTxtForLog(info.m_refreshState),
				GetRefreshSubstateTxtForLog(info.m_refreshSubState));
			WARN_ENGINE(TXT("\texitRefresh=%d"), info.m_exitRefresh );
		}
	};

	// These value could possible change while retrieving them and together not be consistent, but should stablize eventually
	// if stuck in some loop
	static LogInfo prevLogInfo;
	LogInfo info;
	info.m_managerState = m_managerState;
	info.m_refreshRequested = m_refreshRequested.GetValue();
	info.m_loadSaveState = (ELoadSaveState)m_loadSaveState.GetValue();
	info.m_disableInput = m_disableInput;
	info.m_refreshOngoing = m_refreshOngoing;
	info.m_refreshStatesActive = m_refreshStatesActive;
	info.m_refreshStatesFinished = m_refreshStatesFinished;
	info.m_refreshState = m_refreshState;
	info.m_refreshSubState = m_refreshSubState;
	info.m_isSaveSystemBusy = IsSaveSystemBusy();
	info.m_exitRefresh = m_exitRefresh;
	info.m_hasSignoutDeferral = m_signoutDeferral != nullptr;
	const EngineTime now = EngineTime::GetNow();
	const Float diff = now - lastLogHeartbeat;

	// Log the "waiting" states
	if ( info != prevLogInfo || diff > LOG_HEARTBEAT )
	{
		lastLogHeartbeat = now;
		LogInfo::Print( info );
		prevLogInfo = info;
	}
}
#endif // RED_LOGGING_ENABLED

void CUserProfileManagerDurango::HandleUserEngagement()
{
	// Continue handling updates even if an async refresh kicked off.
	// The results will be handled synchronously and we shouldn't delay
	// checking if we got something viable.
	switch ( m_managerState )
	{
	case eWaitingForNewActiveUserEngagement:
		CheckForNewActiveUserEngagement();
		break;
	case eWaitingForCurrentActiveUserEngagement:
		CheckForCurrentActiveUserEngagement();
		break;

	case eWaitingForUserSignOut:
		if( m_signoutDeferral && m_refreshRequested.GetValue() < RT_AutomaticControllerReestablishment )
		{
			// This scenario can happen only under rare conditions - such as a force sign out happening during cloud storage sync
			NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Still have a deferral object but there's no refresh scheduled to deal with it" );
		}
		break;

	case eWaitingForNewActiveUserRefresh:
	case eWaitingForUserSignIn:
		/* do nothing */
		break;

	default:
		RED_HALT( "Unexpected state '%u'", m_managerState );
		break;
	}
}

RED_INLINE void CUserProfileManagerDurango::RefreshStateActivated( ERefreshState state )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	m_refreshStatesActive |= FLAG( state );

	if( m_refreshState < state )
	{
		LOG_ENGINE(TXT("RefreshStateActivated updating refreshState=%ls"), GetRefreshStateTxtForLog(state));
		m_refreshState = state;
	}
}

RED_INLINE void CUserProfileManagerDurango::RefreshStateComplete( ERefreshState state, Bool exitRefresh )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	LOG_ENGINE(TXT("RefreshStateComplete, state=%ls, exitRefresh=%d"), GetRefreshStateTxtForLog(state), exitRefresh);

	if ( (m_refreshStatesActive & FLAG(state)) == 0 )
	{
		ERR_ENGINE(TXT("RefreshStateComplete called for non-active state, state=%ls, exitRefresh=%d"), GetRefreshStateTxtForLog(state), exitRefresh);
	}

	m_refreshStatesFinished |= FLAG( state );

	if( exitRefresh )
	{
		m_exitRefresh = true;
	}
}

void CUserProfileManagerDurango::CachePadsAsync()
{
	RED_FATAL_ASSERT( m_refreshOngoing != RT_None, "Called RefreshAsync() incorrectly - Should only be called from Update()" );

	// FIXME: Integrate as a task or not?
	auto op = concurrency::create_async
	(
		[]() -> TGamepadVectorView^
		{
			try
			{
				return Gamepad::Gamepads;
			}
			catch( Platform::Exception^ ex )
			{
				RED_LOG_ERROR( XBL, TXT( "Exception getting gamepad collection: error code = 0x%x, message = %ls" ), ex->HResult, ex->Message );
			}

			return nullptr;
		}
	);

	concurrency::create_task( op ).then
	(
		[ this ]( TGamepadVectorView^ gamepads )
		{
			// FIXME: I thought in an ASTA this was already supposed to be bound to the thread. Apparently not, go figure.
			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, gamepads ]()
					{
						m_cachedGamepads = gamepads;

						RefreshStateComplete( eRS_CachePads );
					}
				)
			);
		}
	);
}

void CUserProfileManagerDurango::UserRefresh()
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	// If the players user profile is invalid
	// If their is a new profile waiting to replace the existing one
	// If the players user profile has begun the signout process

	const Bool hasValidActiveUserStub = IsValidActiveUser( m_activeUserStub );
	const Bool hasUserQueuedForSignin = m_userQueuedForSignin != nullptr;
	const Bool hasSignoutDeferral = m_signoutDeferral != nullptr;

	LOG_ENGINE(TXT("UserRefresh hasValidActiveUserStub=%d, hasUserQueuedForSignin=%d, hasSignoutDeferral=%d, m_refreshOngoing=%ls"),
		hasValidActiveUserStub, hasUserQueuedForSignin, hasSignoutDeferral, GetRefreshTypeTxtForLog(m_refreshOngoing) );

	if ( !hasValidActiveUserStub || hasUserQueuedForSignin || hasSignoutDeferral )
	{
		switch( m_refreshOngoing )
		{
		case RT_ManualControllerEngagement:
		case RT_AutomaticControllerReestablishment:

			if( !UpdateActiveUser( m_userQueuedForSignin ) )
			{
				RefreshStateComplete( eRS_User, true );
			}
			break;

		case RT_ReacquireAfterSuspend:
			if( !ReacquireActiveUser() )
			{
				RefreshStateComplete( eRS_User, true );
			}
			break;
		}
	}
	else
	{
		RefreshStateComplete( eRS_User );
	}
}

void CUserProfileManagerDurango::ControllerRefresh()
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	// Deal with controller disconnect during suspend
	RED_ASSERT( m_cachedGamepads );
	Bool activeGamepadConnected = false;

	Uint32 numCachedPads = m_cachedGamepads->Size;

	try
	{
		Uint32 numPairedGamepads = 0;
		IGamepad^ potentialNewGamepad = nullptr;
		Bool existingGamepadActive = false;

		if( m_refreshOngoing >= RT_AutomaticControllerReestablishment )
		{
			for ( Uint32 i = 0; i < numCachedPads; ++i )
			{
				IGamepad^ gamepad = m_cachedGamepads->GetAt( i );

				// First establish if the controller is still connected to the system
				if( m_activeGamepad && m_activeGamepad == gamepad )
				{
					// Controller is still there, continue with life as usual
					RED_LOG( Controllers, TXT( "OnRefreshComplete() Existing gamepad is still active" ) );
					RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls" ), i, m_activeGamepad->Id, m_activeGamepad->Type->Data() );
					
					existingGamepadActive = true;
					break;
				}

				// If this isn't the controller we're looking for, check to see if it can be one we fall back to in the event we don't find it
				if( gamepad->User && gamepad->User->Id == m_activeUserId )
				{
					++numPairedGamepads;
					potentialNewGamepad = gamepad;

					RED_LOG( Controllers, TXT( "OnRefreshComplete() Found potential new controller (Total: %u)" ), numPairedGamepads );
					RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls" ), i, potentialNewGamepad->Id, potentialNewGamepad->Type->Data() );
				}
			}
		}

		if( !existingGamepadActive )
		{
			// If we reach this point, our controller was definitely disconnected, we need to see if we found a valid new one in our search
			if( numPairedGamepads == 1 )
			{
				// We did!
				RED_LOG( Controllers, TXT( "OnRefreshComplete() Switching to potential controller!" ) );
				RED_LOG( Controllers, TXT( " -  ), ID: %lu, Type: %ls" ), potentialNewGamepad->Id, potentialNewGamepad->Type->Data() );
				UpdateActiveGamepad( potentialNewGamepad );
			}
			else
			{
				// Otherwise, there are either no gamepads paired to this user or multiple gamepads paired to this user.
				// Either way, we will need to reestablish the active gamepad, so just reset the active gamepad, which
				// will force the engagement prompt and reestablish a new active gamepad

				RED_LOG( Controllers, TXT( "OnRefreshComplete() Need to wait for user engagement" ) );
				UpdateActiveGamepad( nullptr );
			}
		}
	}
	catch( Platform::Exception^ ex )
	{
		RED_LOG_ERROR( XBL, TXT( "Exception getting gamepad: error code = 0x%x, message = %ls" ), ex->HResult, ex->Message );
	}
}

void CUserProfileManagerDurango::CheckForNewActiveUserEngagement()
{
	// Wait for our hero, the start screen, to prevent us from breaking sign in by processing a new user and signing them in
	// before the game has even had a chance to end. Maybe also check for active game or inStartGame too, but we'll see.
	// Putting the check up here, since don't want to change too much at this stage, and eAPDR_Startup is otherwise set initially - not by any scripts
	if ( (m_disableInput & FLAG(eAPDR_Startup)) != 0 )
	{
		return;
	}

	// Give it a second to actually show the start screen so you don't completely skip it if mashing buttons during the bumpers
	if ( HACK_StartScreenTimer.GetTimePeriod() < Config::cvHackStartScreenInputDelay.Get() )
	{
		return;
	}

	IGamepad^ engagedGamepad = GetFirstEngagedGamepad();
	if ( !engagedGamepad )
	{
		return;
	}

	UpdateActiveGamepad( engagedGamepad );

	// Don't allow the account picker to be shown at this stage in time if explicitly disabled
	if( m_disableInput )
		return;

	if( m_refreshOngoing != RT_None )
		return;

	User^ engagedUser = engagedGamepad->User;
	if ( IsValidActiveUser( engagedUser ) )
	{
		// There was already a user bound to the pad, so we don't need to display the account picker
		QueueEvent( EUserEvent::UE_SignInStarted );

		LOG_REFRESH( TXT( "New user queued for sign in (New User Engagement)" ) );
		m_userQueuedForSignin = engagedUser;
		NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Active user engagement" );
	}
	else
	{
		// Note another gamepad can actually select the user by design on the Xbox One (e.g., maybe a friend or parent is selecting for you).
		// The IGamepad^ handle remains valid even if disconnected, although
		// we'll pick up on the change event (which is asynchronous, so the validity matters although we might erroneously think a button was
		// pressed if we're looking for release, so look for "not pressed -> pressed" for events instead of "press -> not pressed".
		if( !m_accountPickerActive )
		{
			PromptUserSignInAsync( engagedGamepad );
		}
	}

	ResetState();
}

void CUserProfileManagerDurango::CheckForCurrentActiveUserEngagement()
{
	if ( !HasValidActiveUser() )
	{
		m_managerState = eWaitingForNewActiveUserEngagement;
		return;
	}

	IGamepad^ engagedGamepad = GetFirstEngagedGamepad();
	if ( engagedGamepad )
	{
		UpdateActiveGamepad( engagedGamepad );

		// Prevent the game from locking down the active pad until input processing for the user profile manager has been re-enabled
		// Ignore the ingame flag as that's to prevent the account picker from appearing
		if( m_disableInput & ~FLAG( CUserProfileManager::eAPDR_Ingame ) )
		{
			return;
		}

		m_managerState = eHasSignedInUserWithGamepad;
	}
}

Windows::Xbox::Input::IGamepad^ CUserProfileManagerDurango::GetFirstEngagedGamepad()
{
	// Initial async refresh not completed yet?
	if ( ! m_cachedGamepads )
	{
		return nullptr;
	}

	// Undocumented, but official samples and forum posts suggest that index zero has the most recently connected gamepad
	// If wrong, it doesn't especially matter since *some* gamepad is engaged, and we have to check the list somehow. 
	// The "left to right" API is about the physical position wrt the Kinect.
	const Uint32 numGamepads = m_cachedGamepads->Size;
	for ( Uint32 i = 0; i < numGamepads; ++i )
	{
		try
		{
			IGamepad^ gamepad = m_cachedGamepads->GetAt( i );

			RawGamepadReading gamepadReading = gamepad->GetRawCurrentReading();

			Uint32 pressedButtons = static_cast< Uint32 >( gamepadReading.Buttons );

			// "Press any key"
			if( pressedButtons )
			{
				return gamepad;
			}
		}
		catch ( Platform::Exception^ ex )
		{
			RED_LOG_ERROR( XBL, TXT( "Exception during gamepad reading: error code = 0x%x, message = %ls" ), ex->HResult, ex->Message );
		}
	}

	return nullptr;
}

void CUserProfileManagerDurango::PromptUserSignInAsync( IGamepad^ engagedGamepad )
{
	RED_FATAL_ASSERT( engagedGamepad, "A valid gamepad is required for the account picker" );
	RED_ASSERT( !m_accountPickerActive, TXT( "Account picker is already visible!" ) );

	typedef Windows::Xbox::UI::AccountPickerOptions AccountPickerOptions;

	typedef Windows::Xbox::UI::AccountPickerResult	AccountPickerResult;
	typedef Windows::Foundation::AsyncOperationCompletedHandler< AccountPickerResult^ > TAccountPickerResultHandler;
	typedef Windows::Foundation::IAsyncOperation< AccountPickerResult^ > TAsyncAccountPickerResultOp;
	typedef Windows::Foundation::AsyncStatus AsyncStatus;

	RED_LOG( Controllers, TXT( "Displaying Account picker using pad -> ID: %lu, Type: %ls" ), engagedGamepad->Id, engagedGamepad->Type->Data() );

	QueueEvent( EUserEvent::UE_SignInStarted );

	m_accountPickerOperation = Windows::Xbox::UI::SystemUI::ShowAccountPickerAsync( engagedGamepad, AccountPickerOptions::None );
	m_accountPickerOperation->Completed = ref new TAccountPickerResultHandler
	(
		[ this ]( TAsyncAccountPickerResultOp^ asyncInfo, AsyncStatus asyncStatus )
		{
			// Figure out which account was selected
			IUser^ selectedUser = nullptr;
			try
			{
				if ( asyncStatus == AsyncStatus::Completed )
				{
					selectedUser = asyncInfo->GetResults()->User;
				}
			}
			catch ( Platform::Exception^ ex )
			{
				RED_LOG_ERROR( XBL, TXT( "Exception getting ShowAccountPickerAsync results: error code = 0x%x, message = %s" ), ex->HResult, ex->Message );
			}
			
			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, selectedUser ]()
					{
						if( selectedUser && selectedUser != m_activeUserStub )
						{
							// User selected, begin sign in procedure
							LOG_REFRESH( TXT( "New user queued for sign in from account picker" ) );
							m_userQueuedForSignin = selectedUser;

							NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Account Picker (User Engagement)" );
						}
						else
						{
							QueueEvent( EUserEvent::UE_SignInCancelled );

							ResetState();
						}

						// Notify the game that the account picker has closed
						m_accountPickerActive = false;
						m_accountPickerOperation = nullptr;

						QueueEvent( EUserEvent::UE_AccountPickerClosed );
					}
				)
			);
		}
	);

	m_accountPickerActive = true;
	QueueEvent( EUserEvent::UE_AccountPickerOpened );
}

void CUserProfileManagerDurango::UpdateActiveGamepad( IGamepad^ gamepad )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

#ifndef RED_FINAL_BUILD
	if( m_activeGamepad )
	{
		RED_LOG( Controllers, TXT( "Old: ID: %lu, Type: %ls, User: %ls" ), m_activeGamepad->Id, m_activeGamepad->Type->Data(), (m_activeGamepad->User)? m_activeGamepad->User->DisplayInfo->GameDisplayName->Data() : L"No bound user" );
	}
	else
	{
		RED_LOG( Controllers, TXT( "Old: NULL" ) );
	}

	if( gamepad )
	{
		RED_LOG( Controllers, TXT( "New: ID: %lu, Type: %ls, User: %ls" ), gamepad->Id, gamepad->Type->Data(), (gamepad->User)? gamepad->User->DisplayInfo->GameDisplayName->Data() : L"No bound user" );
	}
	else
	{
		RED_LOG( Controllers, TXT( "New: NULL" ) );
	}
#endif // RED_FINAL_BUILD

	if ( ( m_activeUserStub || gamepad ) && m_activeGamepad != gamepad )
	{
		m_activeGamepad = gamepad;

		for( Uint32 i = 0; i < m_gamepadListeners.Size(); ++i )
		{
			m_gamepadListeners[ i ]->OnUserProfileGamepadChanged( m_activeGamepad );
		}
	}
}

Bool CUserProfileManagerDurango::UpdateActiveUser( IUser^ untracked )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	// This will force the ref count to be incremented for the duration of this function
	// (As it doesn't appear to be for the parameter directly - this could be a compiler bug, or how WinRT "Handles to objects" )
	// If m_userQueuedForSignin was passed as a parameter to this function, when it is null'd below the parameter is also invalidated
	IUser^ user = untracked;

	// 1. First user sign in:
	//        nullptr != 0xSOMEPTR || nullptr
	// 
	// 2. User Switch:
	//   - A. 0xOLDPTR != 0xNEWPTR || nullptr (First run through)
	//   - B. nullptr != 0xNEWPTR || nullptr (Second run through)
	//   
	// 3. Sign out:
	//   - A. 0xOLDPTR != nullptr || 0xSOMEPTR (First run through)
	//   - B. nullptr != nullptr || 0xSOMEPTR (Subsequent run through)
	
	const Bool hasSignoutDeferral = m_signoutDeferral != nullptr;
	const Bool changedUser = m_activeUserStub != untracked;
	LOG_ENGINE(TXT("UpdateActiveUser changedUser=%d, hasSignoutDeferral=%d"), changedUser, hasSignoutDeferral);
	if ( changedUser || hasSignoutDeferral )
	{
		IUser^ previousUser = m_activeUserStub;

		m_activeUserStub = nullptr;
		m_userQueuedForSignin = nullptr;
		m_activeUserObject = nullptr;
		m_liveContext = nullptr;
		m_activeUserId = INVALID_USERID;

		Bool delayUserSwitch = false;

		// If there is a user already signed in (And we know it's a different user to the one we're attempting to switch to as we checked that above)
		// then we need to send the signed out event and wait a frame so the game can deal with whatever it needs to
		// So store the new user and use that to call back into this function next frame
		if( previousUser )
		{
			QueueEvent( EUserEvent::UE_SignedOut );
			if ( user )
			{
				LOG_REFRESH( TXT( "New user queued for sign in. Signing existing user out..." ) );
			}
			else
			{
				LOG_REFRESH( TXT( "No new user queued for sign in. Signing existing user out..." ) );
			}

			// Wait for the start screen to set us promiscuous again if we weren't in the start screen/main menu
			if ( GGame && (GGame->IsActive() || GGame->CERT_HACK_IsInStartGame()) )
			{
				m_disableInput |= FLAG( CUserProfileManager::eAPDR_Startup );
			}

			delayUserSwitch = true;
		}
		else
		{
			// Just as we do if we already had a signed in user, if the save system is currently busy, we need to bail out
			// and wait for the save to finish before trying to sign in again
			// So store the new user and use that to call back into this function next frame
			if( IsSaveSystemBusy() )
			{
				LOG_REFRESH( TXT( "New user queued for sign in (Waiting while save system is still active with previous user object)" ) );

				delayUserSwitch = true;
			}
		}

		if( delayUserSwitch )
		{
			m_userQueuedForSignin = user;

			if( m_userQueuedForSignin )
			{
				// We need to keep refreshing every frame until it's safe to actually sign the new user in
				NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Disassociating with active user before continuing with new user sign in" );
			}

			if( m_signoutDeferral )
			{
				// We need to keep refreshing every frame until it's safe to actually sign the new user in
				NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Sign out deferred to allow for saving" );
			}

			LOG_ENGINE(TXT("DelayUserSwitch in managerstate %ls"), GetManagerStateTxtForLog(m_managerState));

			return false;
		}

		// Finished saving, allow the OS to finish the sign out procedure
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_signoutDeferralMutex );

			if( m_signoutDeferral )
			{
				LOG_ENGINE(TXT("Releasing signout deferral"));
				m_signoutDeferral->Complete();
				m_signoutDeferral = nullptr;
			}
		}

		LOG_ENGINE(TXT("Releasing connected storage space and resetting save system state"));
		m_connectedStorageSpace = nullptr;
		SET_STATE( STATE_None );

		#ifdef SAVE_SERVER_ENABLED
			m_debugSaveServer.Close();
		#endif // SAVE_SERVER_ENABLED

		if ( user )
		{
			LOG_ENGINE(TXT("UpdateActiveUser calling FindUserFromInterfaceAsync for new user..."));
			FindUserFromInterfaceAsync( user );

			return true;
		}
	}

	return false;
}

void CUserProfileManagerDurango::FindUserFromInterfaceAsync( IUser^ user )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );
	RED_FATAL_ASSERT( user, "Undefined IUser^ object" );

	LOG_ENGINE(TXT("FindUserFromInterfaceAsync"));

	m_activeUserStub = user;
	m_activeUserId = m_activeUserStub->Id;
	
	auto op = concurrency::create_async
	(
		[ user ]() -> User^
		{
			return CUserProfileManagerDurango::FindUserFromInterface( user );
		}
	);

	concurrency::create_task( op ).then
	(
		[ this ]( User^ user )
		{
			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, user ]()
					{
						m_activeUserObject = user;

						if( m_activeUserObject )
						{
							QueueEvent( EUserEvent::UE_SignedIn );
						}
						else
						{
							QueueEvent( EUserEvent::UE_SignInCancelled );
						}

						RefreshStateComplete( eRS_User );
					}
				)
			);
		}
	);
}

Windows::Xbox::System::User^ CUserProfileManagerDurango::FindUserFromInterface( IUser^ user )
{
	RED_FATAL_ASSERT( !SIsMainThread(), "Don't want to call blocking functions on the main thread" );
	RED_FATAL_ASSERT( user, "Undefined IUser^ object" );

	// Cross OS call
	auto users = User::Users;

	for( Uint32 iUser = 0; iUser < users->Size; ++iUser )
	{
		User^ userObject = users->GetAt( iUser );

		if( userObject->IsSignedIn && Platform::String::CompareOrdinal( userObject->XboxUserId, user->XboxUserId ) == 0 )
		{
			return userObject;
		}
	}

	return nullptr;
}

void CUserProfileManagerDurango::FindUserFromIdAsync( Uint32 id )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );

	RED_LOG( RECOVER, TXT( "FindUserFromIdAsync()" ) );

	auto op = concurrency::create_async
	(
		[ id ]() -> User^
		{
			auto users = User::Users;

			for( Uint32 iUser = 0; iUser < users->Size; ++iUser )
			{
				User^ user = users->GetAt( iUser );

				if( user->Id == id )
				{
					RED_LOG( RECOVER, TXT( "FindUserFromIdAsync() -> Found user" ) );
					return user;
				}
			}

			// If we've reached here, the user has deleted the account with the associated ID (because reasons!)
			RED_LOG( RECOVER, TXT( "FindUserFromIdAsync() -> No User" ) );
			return nullptr;
		}
	);

	concurrency::create_task( op ).then
	(
		[ this ]( User^ user )
		{
			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, user ]()
					{
						RED_LOG( RECOVER, TXT( "FindUserFromIdAsync() -> Handler" ) );

						if( user )
						{
							RED_LOG( RECOVER, TXT( "FindUserFromIdAsync() -> Set user and continue" ) );

							m_activeUserStub = m_activeUserObject = user;

							m_activeUserId = user->Id;

							RefreshStateComplete( eRS_User );
						}
						else
						{
							RED_LOG( RECOVER, TXT( "FindUserFromIdAsync() -> Reset user and exit" ) );

							UpdateActiveGamepad( nullptr );
							UpdateActiveUser( nullptr );

							RefreshStateComplete( eRS_User, true );
						}
					}
				)
			);
		}
	);
}

void CUserProfileManagerDurango::CreateLiveContextAsync( User^ user )
{
	RED_FATAL_ASSERT( SIsMainThread(), "This should only be called from the main thread" );
	RED_FATAL_ASSERT( user, "Undefined User^ object" );
	
	auto op = concurrency::create_async
	(
		[ user ]() -> LiveContext^
		{
			return CUserProfileManagerDurango::CreateLiveContext( user );
		}
	);

	concurrency::create_task( op ).then
	(
		[ this ]( LiveContext^ context )
		{
			GCoreDispatcher->RunAsync
			(
				Windows::UI::Core::CoreDispatcherPriority::Normal,
				ref new Windows::UI::Core::DispatchedHandler
				(
					[ this, context ]()
					{
						m_liveContext = context;
						RefreshStateComplete( eRS_LiveContext );
					}
				)
			);
		}
	);
}

CUserProfileManagerDurango::LiveContext^ CUserProfileManagerDurango::CreateLiveContext( User^ user )
{
	RED_FATAL_ASSERT( !SIsMainThread(), "Don't want to call blocking functions on the main thread" );
	RED_FATAL_ASSERT( user, "Undefined User^ object" );

	LiveContext^ context = nullptr;
	try
	{
		// Get the XboxLiveContext
		context = ref new LiveContext( user );
	}
	catch ( Platform::Exception^ ex )
	{
		RED_LOG_ERROR( XBL, TXT( "Failed to create XboxLiveContext: HResult = 0x%x, Message = %s" ), ex->HResult, ex->Message );

		context = nullptr;
	}

	return context;
}

Bool CUserProfileManagerDurango::ReacquireActiveUser()
{
	if( m_activeUserId != INVALID_USERID )
	{
		FindUserFromIdAsync( m_activeUserId );

		return true;
	}

	return false;
}

void CUserProfileManagerDurango::SetActiveUserDefault()
{
	User^ defaultUser = nullptr;

	// Get all the Users
	auto users = Windows::Xbox::System::User::Users;

	// How many do we have?
	Uint32 numUsers = users->Size;

	if( numUsers == 1 )
	{
		// Get the user
		User^ user = users->GetAt( 0 );

		// Check to ensure that the user is signed in and is *not* a guest
		if( !user->IsGuest && user->IsSignedIn )
		{
			// Set the defaultUser to be this user
			defaultUser = user;
		}
	}
	else
	{
		// We either have zero users, or more than one. In both cases we will leave the defaultUser
		// as nullPtr, which will throw up a sign in box later on to confirm who is playing the game
	}

	// This checks internally whether defaultUser is null
	IGamepad^ gamepad = FindUserGamepad( defaultUser );

	if( gamepad )
	{
		UpdateActiveGamepad( gamepad );

		LOG_REFRESH( TXT( "Default user queued for sign in" ) );
		m_userQueuedForSignin = defaultUser;
		NEEDS_REFRESH( RT_AutomaticControllerReestablishment, "Set active user default" );
	}
}

// AKA here's the start screen!
void CUserProfileManagerDurango::SetActiveUserPromiscuous()
{
	LOG_ENGINE(TXT("SetActiveUserPromiscuous"));

	// Safe to show the account picker now
	m_disableInput &= ~FLAG( CUserProfileManager::eAPDR_Startup );

	HACK_StartScreenTimer.ResetTimer();

	UpdateActiveGamepad( nullptr );
	UpdateActiveUser( nullptr );
	ResetState();
}

Bool CUserProfileManagerDurango::HasActiveUser() const
{
	return HasValidActiveUser();
}

void CUserProfileManagerDurango::ChangeActiveUser()
{
	// This check exists to prevent multiple overlapping calls to this function (such as when someone spams the button on the pad that calls this function)
	if( !m_accountPickerActive )
	{
		extern IInputDeviceManager* GInputDeviceManager;
		CInputDeviceManagerDurango* durangoInputDeviceManager = static_cast< CInputDeviceManagerDurango* >( GInputDeviceManager );

		IGamepad^ gamepad = durangoInputDeviceManager->GetActiveGamepad();

		if( gamepad )
		{
			PromptUserSignInAsync( gamepad );
		}
	}
}

Windows::Xbox::Input::IGamepad^ CUserProfileManagerDurango::FindUserGamepad( IUser^ user )
{
	if( user )
	{
		auto numControllers = user->Controllers->Size;

		for ( Uint32 i = 0; i < numControllers; ++i )
		{
			IController^ controller = user->Controllers->GetAt( i );

			Platform::String^ type = controller->Type;
			if ( type == "Windows.Xbox.Input.Gamepad" )
			{
				// We have found the users gamepad (we don't care about any other controllers)
				return static_cast< IGamepad^ >( controller );
			}
		}
	}

	return nullptr;
}

Bool CUserProfileManagerDurango::GetSafeArea( Float& x, Float& y )
{
	const Float horizontal	= ( 1920.0f - 100.0f ) / 1920.0f;
	const Float vertical	= ( 1080.0f - 40.0f ) / 1080.0f;

	x = horizontal;
	y = vertical;

	return true;
}

//void CUserProfileManagerDurango::GetAllAchievements( TDynArray< CName >& achievements ) const
//{
//	if ( m_activeUserStub == nullptr || m_liveContext == nullptr )
//	{
//		return;
//	}
//
//	Platform::String^ userID = m_activeUserObject->XboxUserId;
//
//	// Set up variables for paging
//	Uint32 FirstItem=0;
//	Uint32 ItemsRetrieved=10;
//
//	Uint32 titleID = wcstoul( Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), nullptr, 10 );
//
//	auto pAsyncOp = m_liveContext->AchievementService->GetAchievementsForTitleIdAsync(
//		userID,										// Xbox LIVE user Id
//		titleID,									// Title Id to get achievement data for
//		AchievementType::All,						// AchievementType filter: All mean to get Persistent and Challenge achievements
//		false,										// All possible achievements including accurate unlocked data
//		AchievementOrderBy::TitleId,				// AchievementOrderBy filter: Default means no particular order
//		FirstItem,									// The number of achievement items to skip
//		ItemsRetrieved								// The maximum number of achievement items to return in the response
//		);
//
//	concurrency::create_task( pAsyncOp )
//		.then( [this, ItemsRetrieved] (concurrency::task<AchievementsResult^> achievementsTask)
//	{
//		try
//		{
//			AchievementsResult^ achievements = achievementsTask.get();
//
//			if (achievements != nullptr && achievements->Items != nullptr)
//			{
//				RED_LOG( XBL, TXT("%d achievements found"), achievements->Items->Size );
//
//				if ( achievements->Items->Size == 0 )
//				{
//					return;
//				}
//
//				for( UINT index = 0; index < achievements->Items->Size; index++ )
//				{
//					Achievement^ ach = achievements->Items->GetAt(index);
//					RED_LOG( XBL, TXT("Achievement found with id: %ls name: %ls"), ach->Id->Data(), ach->Name->Data() );
//				}
//			}
//
//			//// Get next page until the end
//			//AchievementsResult^ lastResult = achievements;
//			//while(lastResult != nullptr)
//			//{
//			//	// Note: GetNextAsync will throw an exception with HResult INET_E_DATA_NOT_AVAILABLE
//			//	// when there are no more items to fetch. This exception will be thrown when the task
//			//	// is created in GetNextAsync() as opposed to when it's executed.
//			//	// This behavior may be a little unintuitive because it's different than how
//			//	// GetAchievementsForTitleIdAsync behaves
//			//	concurrency::create_task(lastResult->GetNextAsync(ItemsRetrieved))
//			//		.then([this, &lastResult](AchievementsResult^ achievements)
//			//	{
//			//		lastResult = achievements;
//			//		if (achievements != nullptr && achievements->Items != nullptr)
//			//		{
//			//			for( UINT index = 0; index < achievements->Items->Size; index++ )
//			//			{
//			//				Achievement^ ach = achievements->Items->GetAt(index);
//			//				RED_LOG( XBL, TXT("Achievement found with id: %ls name: %ls"), ach->Id->Data(), ach->Id->Data() );
//			//			}
//			//		}
//			//	}).wait();
//			//}
//		}
//		catch (Platform::Exception^ ex)
//		{
//			if (ex->HResult == INET_E_DATA_NOT_AVAILABLE)
//			{
//				// we hit the end of the achievements
//				RED_LOG( XBL, TXT("Achievement list end") );
//			}
//			else
//			{
//				RED_HALT("GetAchievementsForTitleIdAsync failed");
//			}
//		}
//
//	});
//
//}

Bool CUserProfileManagerDurango::SendTelemetryEvent( const String& service, const String& name, const String& category, const String& value )
{
	IRedTelemetryServiceInterface* redTelemetryService = SRedTelemetryServicesManager::GetInstance().GetService( service );

	if ( redTelemetryService )
	{
		redTelemetryService->LogV( name, category, value );

		return true;
	}

	return false;
}

void CUserProfileManagerDurango::UnlockAchievement( const CName& name )
{
#if !defined( NO_TELEMETRY )
	String* value = m_nameToXboxAchievementMap.FindPtr( name );

	if( value )
	{
		const String telemetryName( TXT( "au" ) );
		const String telemetryCat( TXT( "ha" ) );

#ifndef RED_FINAL_BUILD
		const String debugService( TXT( "telemetry" ) );
		SendTelemetryEvent( debugService, telemetryName, telemetryCat, *value );
#endif //! RED_FINAL_BUILD

		// This always happens in addition to the code above
		const String xboxService( TXT( "xbox_data_platform" ) );
		SendTelemetryEvent( xboxService, telemetryName, telemetryCat, *value );
	}
#else
	RED_UNUSED( name );
#endif // NO_TELEMETRY
}

void CUserProfileManagerDurango::MapPresenceInit( Uint32 numEntries )
{
	const Uint32 newCapacity = m_presenceIDs.Capacity() + numEntries;
	m_presenceIDs.Reserve( newCapacity );
}

void CUserProfileManagerDurango::MapPresence( const CName& name, const String& platform, const String& id )
{
	if( platform == TXT( "xb1" ) )
	{
		m_presenceIDs.Set( name, id );
	}
}

void CUserProfileManagerDurango::DoUserPresence( const CName& presenceName )
{
	if ( m_liveContext )
	{
		String presenceIDString;
		if ( m_presenceIDs.Find( presenceName, presenceIDString ) )
		{
			// The config ID below is an example. 
			// The real ID to use is defined when you set up the game on Xbox Development Portal. 
			Platform::String^ aServiceConfigurationId = Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId;
			Platform::String^ presenceID = ref new Platform::String( presenceIDString.AsChar() );

			PresenceData^ presenceData = ref new PresenceData
			(
				aServiceConfigurationId,
				presenceID
			);

			auto setPresenceAction = m_liveContext->PresenceService->SetPresenceAsync
			(
				true,				// isUserActive
				presenceData		// richPresenceData is optional
			);

			concurrency::create_task( setPresenceAction )
			.then
			(
				[] ( concurrency::task<void> t )
				{
					try
					{
						t.get();
					}
					catch ( Platform::Exception^ ex )
					{
						RED_LOG_ERROR( XBL, TXT( "SetPresenceAsync exception: HResult = 0x%x, Message = %s" ), ex->HResult, ex->Message );
					}
				//	RED_LOG( XBL, TXT("Presence set"));
				}
			);
		}
		else
		{
			RED_LOG( XBL, TXT("Unknown presence ID"));
		}
	}
}

Bool CUserProfileManagerDurango::IsAchievementMapped( const CName& name ) const  
{
	return m_nameToXboxAchievementMap.KeyExist( name );
}

void CUserProfileManagerDurango::MapAchievementInit( Uint32 numAchievements )  
{
	const Uint32 newCapacity = numAchievements + m_nameToXboxAchievementMap.Capacity();
	m_nameToXboxAchievementMap.Reserve( newCapacity );
}

void CUserProfileManagerDurango::MapAchievement( const CName& name, const String& platform, const String& id )
{
	if( platform == TXT( "xb" ) )
	{
		m_nameToXboxAchievementMap.Set( name, id );
	}
}

//void CUserProfileManagerDurango::GetUserPresence( String& presenceString )
//{
//	Platform::String^ userID = m_activeUserObject->XboxUserId;
//	auto getPresenceAction = m_liveContext->PresenceService->GetPresenceAsync( userID );
//
//	String& tempPresenceString = presenceString;
//
//	concurrency::create_task( getPresenceAction )
//		.then([tempPresenceString&] (concurrency::task<PresenceRecord^> presenceRecord)
//	{
//		PresenceRecord^ presence = presenceRecord.get();
//
//		auto titleRecords = presence->PresenceTitleRecords;
//
//		for( UINT index = 0; index < titleRecords->Size; index++ )
//		{
//			auto titleRecord = titleRecords->GetAt(index);
//
//			if ( titleRecord->IsTitleActive )
//			{
//				Platform::String^ presenceName = titleRecord->Presence;
//				RED_LOG( XBL, TXT("Presence %ls for active title %d - %ls"), presenceName->Data(), titleRecord->TitleId, titleRecord->TitleName->Data() );
//				tempPresenceString.Printf( TXT("%ls"), presenceName->Data());
//			}
//		}
//	}).wait();
//}

void CUserProfileManagerDurango::PrintUserStats( const String& statName )
{
#ifndef RED_FINAL_BUILD
	if ( m_liveContext )
	{
		Platform::String^ aServiceConfigurationId = Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId;
		Platform::String^ statisticName = ref new Platform::String( statName.AsChar() );

		auto asyncOp = m_liveContext->UserStatisticsService->GetSingleUserStatisticsAsync
		(
			m_liveContext->User->XboxUserId,
			aServiceConfigurationId,
			statisticName
		);

		String messageToPrint;
		Concurrency::create_task( asyncOp ).then
		(
			[ &messageToPrint ] ( Microsoft::Xbox::Services::UserStatistics::UserStatisticsResult^ result ) 
			{
				messageToPrint += TXT("XboxUserId: ");
				messageToPrint +=  result->XboxUserId->Begin();			
 				for( UINT index = 0; index < result->ServiceConfigurationStatistics->Size; ++index )
 				{
 					Microsoft::Xbox::Services::UserStatistics::ServiceConfigurationStatistic^ serviceConfigStatistic = result->ServiceConfigurationStatistics->GetAt(index);
 					messageToPrint += TXT("\nServiceConfigurationId: ");
					messageToPrint += serviceConfigStatistic->ServiceConfigurationId->Begin();
					if( serviceConfigStatistic->Statistics->Size > 0 )
					{
						messageToPrint += TXT("\nStat status: ");
						for( UINT indexStat = 0; indexStat < serviceConfigStatistic->Statistics->Size; ++indexStat )
						{
							Microsoft::Xbox::Services::UserStatistics::Statistic^ statistic = serviceConfigStatistic->Statistics->GetAt( indexStat );
							messageToPrint += TXT("\n");
							messageToPrint += TXT("\nStatisticName: ");
							messageToPrint += statistic->StatisticName->Begin();
							messageToPrint += TXT("\nStatisticType: ");
							messageToPrint += statistic->StatisticType.ToString()->Begin();
							messageToPrint += TXT("\nValue: ");
							messageToPrint += statistic->Value->Begin();
						}
					}
					else
					{
						messageToPrint += TXT("\nStat status: NO DATA");
					}
 				}
			}
		).wait();

		RED_LOG( XBL, messageToPrint.AsChar() );
	}
#endif //! RED_FINAL_BUILD
}

void CUserProfileManagerDurango::PrintUserAchievement( const String& achievementName )
{
#ifndef RED_FINAL_BUILD
	if ( m_liveContext )
	{
		Platform::String^ aServiceConfigurationId = Windows::Xbox::Services::XboxLiveConfiguration::PrimaryServiceConfigId;
		Platform::String^ achiName = ref new Platform::String( achievementName.AsChar() );
		Uint32 titleID = wcstoul( Windows::Xbox::Services::XboxLiveConfiguration::TitleId->Data(), nullptr, 10 );

		auto getAchievementsOp = m_liveContext->AchievementService->GetAchievementsForTitleIdAsync
		(
			m_liveContext->User->XboxUserId,
			titleID,
			Microsoft::Xbox::Services::Achievements::AchievementType::All,
			false,
			Microsoft::Xbox::Services::Achievements::AchievementOrderBy::Default,
			0, // skipItems
			100  // maxItems
		);

		Bool achievementNotFound = true;
		String messageToPrint;
		Concurrency::create_task( getAchievementsOp ).then( [ &achievementNotFound, &messageToPrint, achievementName, achiName ] ( Microsoft::Xbox::Services::Achievements::AchievementsResult^ achievements )
		{
			for( UINT index = 0; index < achievements->Items->Size; index++ )
			{
				Microsoft::Xbox::Services::Achievements::Achievement^ achievement = achievements->Items->GetAt(index);
				if( achievement->Name == achiName )
				{
					achievementNotFound = false;
					messageToPrint += String::Printf( TXT( "Achievement: %s [%s]!\n" ), achievementName.AsChar(), achievement->ProgressState.ToString()->Begin() );
				}
			}
		}).wait();

		if( achievementNotFound )
		{
			messageToPrint += String::Printf( TXT( "Achievement: %s NOT present!") , achievementName.AsChar()  );
		}
		RED_LOG( XBL, messageToPrint.AsChar() );
	}
#endif //! RED_FINAL_BUILD
}

String CUserProfileManagerDurango::GetActiveUserDisplayNameRaw() const
{
	if( m_activeUserStub )
	{
		Platform::String^ gamertag = m_activeUserStub->DisplayInfo->GameDisplayName;

		String displayName = gamertag->Data();

		return displayName;
	}

	return String::EMPTY;
}

void CUserProfileManagerDurango::DisplayUserProfileSystemDialog()
{
	if( m_activeUserStub )
	{
		Windows::Xbox::UI::SystemUI::ShowProfileCardAsync( m_activeUserStub, m_activeUserStub->XboxUserId );
	}
	else
	{
		RED_LOG_ERROR( XBL, TXT( "Cannot display user profile system dialog if there is no active user" ) );
	}
}

void CUserProfileManagerDurango::DisplayHelp()
{
	if( m_activeUserStub )
	{
		Windows::Xbox::ApplicationModel::Help::Show( m_activeUserStub );
	}
}

void CUserProfileManagerDurango::DisplayStore()  
{
	if ( m_activeUserStub )
	{
		typedef Windows::ApplicationModel::Package Package;
		typedef Windows::ApplicationModel::PackageId PackageId;
		typedef Windows::Xbox::ApplicationModel::Store::ProductItemTypes ProductItemTypes;
		typedef Windows::Xbox::ApplicationModel::Store::Product Product;

		PackageId^ packageId = Package::Current->Id;
		Platform::String^ name = packageId->Name;
		Product::ShowMarketplaceAsync( m_activeUserStub, ProductItemTypes::Game, name, ProductItemTypes::Durable );
	}
}

void CUserProfileManagerDurango::DebugPrintUserAndControllerBindings( IUser^ activeUser )
{
#ifndef RED_FINAL_BUILD

	typedef Windows::Xbox::Input::Controller Controller;
	Uint32 numControllers = Controller::Controllers->Size;

	RED_LOG( Controllers, TXT( "--- Controllers connected to the system (%u) ---" ), numControllers );

	for ( Uint32 i = 0; i < numControllers; ++i )
	{
		IController^ controller = Controller::Controllers->GetAt( i );

		if( controller )
		{
			User^ user = controller->User;

			if( user )
			{
				RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls, User: %ls" ), i, controller->Id, controller->Type->Data(), user->DisplayInfo->GameDisplayName->Data() );
			}
			else
			{
				RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls, No bound user" ), i, controller->Id, controller->Type->Data() );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	auto users = User::Users;
	Uint32 numUsers = users->Size;

	RED_LOG( Controllers, TXT( "--- Users connected to the system (%u) ---" ), numUsers );
	for( Uint32 i = 0; i < numUsers; ++i )
	{
		User^ user = users->GetAt( i );

		if( user )
		{
			auto numControllers = user->Controllers->Size;

			RED_LOG( Controllers, TXT( "Controllers for user: %ls" ), user->DisplayInfo->GameDisplayName->Data() );

			for ( Uint32 i = 0; i < numControllers; ++i )
			{
				IController^ controller = user->Controllers->GetAt( i );

				RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls" ), i, controller->Id, controller->Type->Data() );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	if( activeUser )
	{
		Uint32 numControllers = activeUser->Controllers->Size;

		RED_LOG( Controllers, TXT( "--- Controllers (%u) for Active User: %ls ---" ), numControllers, activeUser->DisplayInfo->GameDisplayName->Data() );

		for ( Uint32 i = 0; i < numControllers; ++i )
		{
			IController^ controller = activeUser->Controllers->GetAt( i );

			RED_LOG( Controllers, TXT( " - %u), ID: %lu, Type: %ls" ), i, controller->Id, controller->Type->Data() );
		}
	}

#endif //! RED_FINAL_BUILD
}

void CUserProfileManagerDurango::RefreshTick()
{
	// Note: even if constrained, we need to still process events, like users signing out.
	if ( m_refreshOngoing == RT_None )
	{
		TRefreshType type = m_refreshRequested.Exchange( RT_None );

		if ( type != RT_None )
		{
			// Filter the flags to only the most significant refresh type
			m_refreshOngoing = FLAG( Red::System::BitUtils::BitScanReverse( type ) );

			LOG_REFRESH( TXT( "Start: Refresh type: %ls in managerState %ls" ), GetRefreshTypeTxtForLog(m_refreshOngoing), GetManagerStateTxtForLog(m_managerState) );

			m_refreshState = eRS_CachePads;
			m_refreshSubState = ERefreshSubState::eRTS_Start;
		}
		else
		{
			// No refresh in progress
			return;
		}
	}

	if( m_refreshSubState == ERefreshSubState::eRTS_Start )
	{
		// Initialise values for this state
		m_refreshStatesActive = 0;
		m_refreshStatesFinished = 0;

		m_refreshSubState = ERefreshSubState::eRTS_Ongoing;
		m_exitRefresh = false;

		switch( m_refreshState )
		{

		// 
		case eRS_CachePads:
			LOG_REFRESH( TXT( "eRS_CachePads" ) );
			RefreshStateActivated( eRS_CachePads );

			CachePadsAsync();

			// If a basic refresh was requested, we don't want to let the refresh advance onto checking
			// User validity or controllers (or indeed anything else)
			if( m_refreshOngoing == RT_Basic )
			{
				m_exitRefresh = true;
				break;
			}

			// Fallthrough
		case eRS_User:
			LOG_REFRESH( TXT( "eRS_User" ) );
			RefreshStateActivated( eRS_User );
			m_userObjectReestablished = false;
			UserRefresh();
			break;

		case eRS_ConnectedStorage:
			LOG_REFRESH( TXT( "eRS_ConnectedStorage" ) );
			RED_FATAL_ASSERT( m_activeUserObject, "Can't register user services without a valid user object" );

			if( !m_connectedStorageSpace || m_connectedStorageSpace->User != m_activeUserObject )
			{
				LOG_REFRESH( TXT( "eRS_Services -> Connected Storage" ) );

				RefreshStateActivated( eRS_ConnectedStorage );
				PrepareConnectedStorage();
				
				#ifdef SAVE_SERVER_ENABLED
					m_debugSaveServer.Open();
				#endif // SAVE_SERVER_ENABLED
			}

			// Fallthrough
		case eRS_LiveContext:
			LOG_REFRESH( TXT( "eRS_LiveContext" ) );
			RED_FATAL_ASSERT( m_activeUserObject, "Can't register user services without a valid user object" );

			if( !m_liveContext || m_liveContext->User != m_activeUserObject )
			{
				LOG_REFRESH( TXT( "eRS_Services -> LiveContext" ) );

				RefreshStateActivated( eRS_LiveContext );
				CreateLiveContextAsync( m_activeUserObject );
			}

			// Fallthrough
		case eRS_Controllers:
			LOG_REFRESH( TXT( "eRS_Controllers" ) );
			RefreshStateActivated( eRS_Controllers );

			LOG_REFRESH( TXT( "eRS_Services -> Controllers" ) );
			ControllerRefresh();
			RefreshStateComplete( eRS_Controllers );
			break;

		default:
			RED_FATAL( "Unrecognised and unhandled durango user profile manager refresh type specified: %u", m_refreshState );
		}
	}

	// Check for completion
	if( m_refreshSubState == ERefreshSubState::eRTS_Ongoing )
	{
		// Have all active states finished?
		// Mask by active states in case something sets a finished state that was never requested
		// Although must make sure there isn't a race where some system could acutally "finish" a state that hasn't
		// event been requested yet! Maybe through some other unforseen path.
		if( (m_refreshStatesFinished & m_refreshStatesActive) == m_refreshStatesActive )
		{
			// Specific state resolution

			// Send the event now that connected storage has been initialised
			if( ( m_refreshStatesFinished & FLAG( eRS_ConnectedStorage ) ) && m_activeUserStub )
			{
				QueueEvent( EUserEvent::UE_LoadSaveReady );
			}

			// Generic state resolution
			if( !m_exitRefresh )
			{
				// Move onto the next state in the refresh process
				ERefreshState newState = static_cast< ERefreshState >( m_refreshState + 1 );

				m_refreshState = newState;
				m_exitRefresh = newState == eRS_Finished;
			}

			if( m_exitRefresh )
			{
				// All done!
				LOG_REFRESH( TXT( "Exiting" ) );

				ResetState();

				m_refreshState = eRS_Inactive;
				m_refreshOngoing = RT_None;
			}
			else
			{
				m_refreshSubState = ERefreshSubState::eRTS_Start;
			}
		}
	}
}

void CUserProfileManagerDurango::SetIgnoreSavingUserSettings(Bool value)
{
	m_ignoreSaveUserSettingsRequest.SetValue( value );
}
