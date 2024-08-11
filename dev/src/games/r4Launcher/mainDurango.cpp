/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/redSystem/logFile.h"

#include "../../common/core/loadingProfiler.h"
#include "../../common/core/gameApplication.h"
#include "../../common/core/engineTime.h"
#include "../../common/engine/rawInputGamepadReading.h"
#include "../../common/engine/baseEngine.h"
#include "../../common/engine/userProfile.h"

#include "gameApplicationDurango.h"
#include "states.h"

#ifdef RED_PLATFORM_DURANGO

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;


#ifdef RED_LOGGING_ENABLED
	Red::System::Log::File fileLogger( TXT( "d:\\r4launcher_durango.log" ), true );
#endif

Windows::UI::Core::CoreDispatcher^ GCoreDispatcher;

class CGameApplication;

#ifndef RED_FINAL_BUILD
	#define GAME_STATE_DEBUG( txt )	OutputDebugStringA( "AppState: " txt "\n" )
#else
	#define GAME_STATE_DEBUG( txt ) ((void)0)
#endif

class CMessagePump : public IMessagePump
{
public:
	virtual void PumpMessages() override final;
	virtual ~CMessagePump() {}

private:
	Red::System::StopClock m_timer;
};

//#define EVENT_WATCHDOG

#ifdef EVENT_WATCHDOG
static EngineTime LastUpdateTime;
static HANDLE GEventWatchdogThread;
#define LOG_WATCHDOG(...) { Char tempArray[256]; swprintf( tempArray, 256, __VA_ARGS__ ); OutputDebugString(tempArray); OutputDebugString(TXT("\n")); }
static const Float WATCHDOG_TIMEOUT = 1.f;
extern class CDeferredInit* GDeferredInit;
static DWORD RED_STDCALL XBEventWatchdogFunc(void* param)
{
	static Bool DoBreak = true;
	static Bool DoBreakDeferredInit = false;

	EngineTime startTime = LastUpdateTime;

	for(;;)
	{
		const EngineTime prev = LastUpdateTime;
		EngineTime now = EngineTime::GetNow();
		if ( now - prev > WATCHDOG_TIMEOUT )
		{
 			if ( DoBreak && (!GDeferredInit || DoBreakDeferredInit ) )
 			{
 				LOG_WATCHDOG(TXT("XBEventWatchdogFunc timeout"));
 				__debugbreak();
 				LastUpdateTime = EngineTime::GetNow(); // avoid infinite debugbreaks if you want to continue
 			}
		}
		Sleep(10);
	}

	return 0;
}
#endif // EVENT_WATCHDOG

void CMessagePump::PumpMessages()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Main thread only!!" );

#ifdef EVENT_WATCHDOG
	if ( !GEventWatchdogThread )
	{
		LastUpdateTime = EngineTime::GetNow();
		GEventWatchdogThread = ::CreateThread( nullptr, 16*1024, XBEventWatchdogFunc, nullptr, 0, nullptr );
	}

#endif // EVENT_WATCHDOG

	if ( m_timer.GetDelta() > 0.1 )
	{
#ifdef EVENT_WATCHDOG
		const EngineTime now = EngineTime::GetNow();
		const Float diff = now - LastUpdateTime;
		if ( diff > WATCHDOG_TIMEOUT )
		{
			LOG_WATCHDOG(TXT("No messages pumped for %.2f sec"), diff );
		}

		LastUpdateTime = now;
#endif

		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents( CoreProcessEventsOption::ProcessAllIfPresent );
		m_timer.Reset();
	}

	if( GUserProfileManager )
		GUserProfileManager->Update();
}

ref class GameView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	GameView();

	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

protected:

	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnResourceAvailabilityChanged( Platform::Object^ sender, Platform::Object^ args );
	void OnVisibilityChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::VisibilityChangedEventArgs^ args );
	void OnFocusChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args );
	void OnWindowActivatedChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args );

	void OnKeyDown( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::KeyEventArgs^ keyEventArgs );
	void OnKeyUp( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::KeyEventArgs^ keyEventArgs );

// Doesn't work anymore?
//	void OnPointerMoved( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::PointerEventArgs^ pointerEventArgs );

private:
	CGameApplication*						m_gameApplication;
	CMessagePump							m_messagePump;

private:
	Windows::UI::Core::CoreDispatcher^		m_dispatcher;
};

GameView::GameView()
:	m_gameApplication( nullptr )
{
}

void GameView::Initialize( Windows::ApplicationModel::Core::CoreApplicationView^ applicationView )
{
	GAME_STATE_DEBUG( "GameView Initialize" );

	applicationView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &GameView::OnActivated);
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &GameView::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &GameView::OnResuming);
	CoreApplication::ResourceAvailabilityChanged += ref new Windows::Foundation::EventHandler< Platform::Object^ >( this, &GameView::OnResourceAvailabilityChanged );
	CoreApplication::DisableKinectGpuReservation = true;

	extern IMessagePump* GMessagePump;
	GMessagePump = &m_messagePump;
}

//////////////////////////////////////////////////////////////////////////
// Uninitialize
//	Called if Run() returns (it shouldn't). As an emergency measure, we should
//  run the shutdown state anyway
void GameView::Uninitialize()
{
	GAME_STATE_DEBUG( "GameView Unitialize" );

	extern IMessagePump* GMessagePump;
	GMessagePump = nullptr;

	DestroyGameApplication( m_gameApplication );
}

//////////////////////////////////////////////////////////////////////////
// SetWindow
//	Called when the system creates a window for us. From this point we can init d3d
void GameView::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	GAME_STATE_DEBUG( "GameView SetWindow" );
	
	window->VisibilityChanged += ref new TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^>(this, &GameView::OnVisibilityChanged);
	window->Activated += ref new TypedEventHandler< Windows::UI::Core::CoreWindow^, Windows::UI::Core::WindowActivatedEventArgs^ >(this, &GameView::OnFocusChanged);

	window->KeyDown += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, KeyEventArgs^>( this, &GameView::OnKeyDown );
	window->KeyUp += ref new Windows::Foundation::TypedEventHandler<CoreWindow^, KeyEventArgs^>( this, &GameView::OnKeyUp );

	m_dispatcher = window->Dispatcher;
	GCoreDispatcher = m_dispatcher;
}

//////////////////////////////////////////////////////////////////////////
// OnActivated
//	Called when the application is launched. If the previous state is Terminated,
//	we need to reload from the previous session
void GameView::OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
{
	GAME_STATE_DEBUG( "GameView OnActivated" );

	if( args->Kind == ActivationKind::Launch )
	{
		LaunchActivatedEventArgs^ launchArgs = static_cast< LaunchActivatedEventArgs^ >( args );

		const Char* commandLine = launchArgs->Arguments->Data();

		m_gameApplication = CreateGameApplication( commandLine );
	}
	else if( args->Kind == ActivationKind::Protocol )
	{
		ProtocolActivatedEventArgs^ protocolArgs = static_cast< ProtocolActivatedEventArgs^ >( args );

		//TODO: Convert the URI arguments into command line parameters if we need to
		if( args->PreviousExecutionState != ApplicationExecutionState::Running )
		{
			m_gameApplication = CreateGameApplication( TXT( "" ) );
		}
	}

	CoreWindow^ window = CoreWindow::GetForCurrentThread();
	window->Activated  += ref new TypedEventHandler<CoreWindow^,WindowActivatedEventArgs^>( this, &GameView::OnWindowActivatedChanged );
	window->Activate();
}

//////////////////////////////////////////////////////////////////////////
// OnSuspending
//	The app is about to go into suspend mode (no system process, but still in memory)
//	We need to save the current game state in case the app is terminated later
void GameView::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
	Red::System::Timer suspendTimer;
#endif

	// Get the Suspending Event
	Windows::ApplicationModel::SuspendingDeferral^ SuspendingEvent = args->SuspendingOperation->GetDeferral();

	GAME_STATE_DEBUG( "GameView OnSuspending" );

	GEngine->OnSuspend( CoreApplication::ResourceAvailability == Core::ResourceAvailability::Constrained );

	// finish saving the game now, before we Complete()
	while ( SAVE_Saving == GUserProfileManager->GetSaveGameProgress() ) 
	{ 
		Red::Threads::YieldCurrentThread(); 
	}

	#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
		Double seconds = suspendTimer.GetSeconds();
		String debugMessage = String::Printf( TXT("RED - Suspended %f"), seconds );
		OutputDebugString( debugMessage.AsChar() );
	#endif

	// Signal that we have finished and are ready to suspend
	SuspendingEvent->Complete();
}

//////////////////////////////////////////////////////////////////////////
// OnResuming
//	Resuming from Suspended state. Game state does not need to be restored,
//	but user / input changes must be handled
void GameView::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	GAME_STATE_DEBUG( "GameView OnResuming" );

	GEngine->OnResume( CoreApplication::ResourceAvailability == Core::ResourceAvailability::Constrained );

	//if( CoreApplication::ResourceAvailability == Core::ResourceAvailability::Constrained )
	//{
	//	m_gameApplication->RequestState( GameConstrained );
	//}
	//else
	//{
	//	m_gameApplication->RequestState( GameRunning );
	//}
}

//////////////////////////////////////////////////////////////////////////
// OnResourceAvailabilityChanged
//	We have either gone from constrained - normal or normal - constrained mode!
void GameView::OnResourceAvailabilityChanged( Platform::Object^ sender, Platform::Object^ args )
{
	GAME_STATE_DEBUG( "GameView OnResourceAvailabilityChanged" );

	/*
	MTA model and event called on a worker thread

	https://forums.xboxlive.com/AnswerPage.aspx?qid=aa97f459-c294-4291-ac6d-664cd327e417&tgt=1#MainContentPlaceHolder_AnswersGrid_AnswerInfoDiv02785881-3887-47fe-96e4-ada8c343abfb_0
	*/

	if( GEngine )
	{
		m_dispatcher->RunAsync
		(
			Windows::UI::Core::CoreDispatcherPriority::Normal,
			ref new Windows::UI::Core::DispatchedHandler
			(
				[]()
				{
					const auto availability = CoreApplication::ResourceAvailability;
					switch( availability )
					{
					case Core::ResourceAvailability::Full:
					case Core::ResourceAvailability::FullWithExtendedSystemReserve:
						//m_gameApplication->RequestState( GameRunning );
					
						GEngine->OnExitConstrainedRunningMode();
						GEngine->OnEnterNormalRunningMode();

						break;

					case Core::ResourceAvailability::Constrained:
						//m_gameApplication->RequestState( GameConstrained );

						GEngine->OnEnterConstrainedRunningMode();

						break;
					}
				}
			)
		);
	}
}

//////////////////////////////////////////////////////////////////////////
// OnVisibilityChanged
//	Called when the visibility of the window changes (change in size, hidden / shown, etc)
void GameView::OnVisibilityChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::VisibilityChangedEventArgs^ args )
{}

void GameView::OnFocusChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args )
{}

void GameView::OnWindowActivatedChanged( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::WindowActivatedEventArgs^ args )
{
	if ( args->WindowActivationState == CoreWindowActivationState::Deactivated )
	{
		// Probably Kinect heard a system voice command or just "Xbox" by itself and popped up a menu.
		// TODO: The system queues these up (e.g., "Xbox..." then nothing, so coalesce.
	}
}


//////////////////////////////////////////////////////////////////////////
// Load
//	The purpose of this method is to get the application entry point.
//	There are strict limits as to how much time we can spend in this function,
//	so only kick off async things
void GameView::Load(Platform::String^ entryPoint)
{
	GAME_STATE_DEBUG( "GameView Load" );
}

//////////////////////////////////////////////////////////////////////////
// Run
//	Called by the system after initialization is complete. Kicks off the game state machine
void GameView::Run()
{
	GAME_STATE_DEBUG( "GameView Run" );
	GLoadingProfiler.Start();
	m_gameApplication->RequestState( GameActivate );
	m_gameApplication->Run();
	GAME_STATE_DEBUG( "GameView Run returned" );
}

ref class GameViewFactory : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
	{
		return ref new GameView();
	}
};

// Application entry point
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	GameViewFactory^ factory = ref new GameViewFactory();
	Windows::ApplicationModel::Core::CoreApplication::Run(factory);
	return 0;
}

#define MOUSE_HACK_SPEED_STEP 5
#define MOUSE_HACK_SPEED_MAX 50
Bool GHackMouseMode;
static Int32 GHackMouseSpeed = MOUSE_HACK_SPEED_STEP;

//////////////////////////////////////////////////////////////////////////
// DurangoHackGetGamepadReading
//	This is a massive hack and requires this game project in order to link engine on Durango.
//	This needs to go!
void DurangoHackGetGamepadReading( SRawInputGamepadReading& reading )
{
	if ( GHackMouseMode )
	{
		reading.IsValid = false;
		return;
	}

	Windows::Foundation::Collections::IVectorView< Windows::Xbox::Input::IGamepad^ >^ allGamepads = Windows::Xbox::Input::Gamepad::Gamepads;

	if( allGamepads->Size > 0 )
	{
		Windows::Xbox::Input::IGamepad^ gamepad = allGamepads->GetAt( 0 );
		Windows::Xbox::Input::IGamepadReading^ gamepadReading = gamepad->GetCurrentReading();


		reading.IsValid = true;

		// Buttons
		reading.IsAPressed					=	gamepadReading->IsAPressed				;
		reading.IsBPressed					=	gamepadReading->IsBPressed				;
		reading.IsXPressed					=	gamepadReading->IsXPressed				;
		reading.IsYPressed					=	gamepadReading->IsYPressed				;
		reading.IsViewPressed				=	gamepadReading->IsViewPressed			;
		reading.IsMenuPressed				=	gamepadReading->IsMenuPressed			;
		reading.IsDPadDownPressed			=	gamepadReading->IsDPadDownPressed		;
		reading.IsDPadUpPressed				=	gamepadReading->IsDPadUpPressed			;
		reading.IsDPadLeftPressed			=	gamepadReading->IsDPadLeftPressed		;
		reading.IsDPadRightPressed			=	gamepadReading->IsDPadRightPressed		;
		reading.IsLeftShoulderPressed		=	gamepadReading->IsLeftShoulderPressed	;
		reading.IsLeftThumbstickPressed		=	gamepadReading->IsLeftThumbstickPressed	;
		reading.IsRightShoulderPressed		=	gamepadReading->IsRightShoulderPressed	;
		reading.IsRightThumbstickPressed	=	gamepadReading->IsRightThumbstickPressed;

		reading.LeftThumbstickX				=	gamepadReading->LeftThumbstickX			;
		reading.LeftThumbstickY				=	gamepadReading->LeftThumbstickY			;
		reading.LeftTrigger					=	gamepadReading->LeftTrigger				;
		reading.RightThumbstickX			=	gamepadReading->RightThumbstickX		;
		reading.RightThumbstickY			=	gamepadReading->RightThumbstickY		;
		reading.RightTrigger				=	gamepadReading->RightTrigger			;
	}
}

static CRawKeyboardReadingBuffer SDurangoKeyboardReading;

void DurangoHackGetKeyboardReading( SRawKeyboardEvent events[], Uint32* /*[inout]*/ numEvents )
{
	SDurangoKeyboardReading.GetKeyEvents( events, numEvents );
}

//TBD: Maybe have a thread polling for smoother "mouse"

static SRawMouseReading GHackMouse;

void DurangoHackUpdateMouseReading()
{
	// Reset
	GHackMouse.dx = GHackMouse.dy = 0;
	GHackMouse.leftButtonDown = false;
	GHackMouse.rightButtonDown = false;
	GHackMouse.middleButtonDown = false;

	// Grab state from all gamepads
	Windows::Foundation::Collections::IVectorView< Windows::Xbox::Input::IGamepad^ >^ allGamepads = Windows::Xbox::Input::Gamepad::Gamepads;
	if( allGamepads->Size > 0 )
	{
		Windows::Xbox::Input::IGamepad^ gamepad = allGamepads->GetAt( 0 );
		Windows::Xbox::Input::RawGamepadReading rawReading;
		try
		{		
			rawReading = gamepad->GetRawCurrentReading();
		}
		catch ( Platform::Exception^ ex )
		{
			ERR_ENGINE(TXT("Exception getting mouse reading: error code = 0x%x, message = %s"), ex->HResult, ex->Message );
			return;
		}

		// Update delta
		const Float mouseMoveFactor = static_cast< Float >( GHackMouseSpeed );
		const Float deadZone = 7849 / 32768.0f;
		const Float range = 1.f - deadZone;

		Float dx = rawReading.LeftThumbstickX;
		Float dy = rawReading.LeftThumbstickY;

		if ( dx > +deadZone )
		{
			dx = ( dx - deadZone ) / range;
		}
		else if ( dx < -deadZone )
		{
			dx = ( dx + deadZone ) / range;
		}
		else
		{
			dx = 0;
		}

		if ( dy > +deadZone )
		{
			dy = ( dy - deadZone ) / range;
		}
		else if ( dy < -deadZone )
		{
			dy = ( dy + deadZone ) / range;
		}
		else
		{
			dy = 0;
		}

		dx *= mouseMoveFactor;
		dy *= mouseMoveFactor;

		// Makes more sense as a mouse this way
		dy *= -1;

		GHackMouse.dx += dx;
		GHackMouse.dy += dy;

		// Update position

		GHackMouse.x += dx;
		GHackMouse.y += dy;

		if ( GHackMouse.x > 1280 )
		{
			GHackMouse.x = 1280;
		}

		if ( GHackMouse.x < 0 )
		{
			GHackMouse.x = 0;
		}

		if ( GHackMouse.y > 720 )
		{
			GHackMouse.y = 720 ;
		}

		if ( GHackMouse.y < 0 )
		{
			GHackMouse.y = 0;
		}

		// Update buttons
		Uint32 buttonMask = static_cast< Uint32 >( rawReading.Buttons );
		GHackMouse.leftButtonDown |= ( buttonMask & static_cast< Uint32 >( Windows::Xbox::Input::GamepadButtons::X ) ) != 0 ? true : false;
		GHackMouse.rightButtonDown |= ( buttonMask & static_cast< Uint32 >( Windows::Xbox::Input::GamepadButtons::A ) ) != 0 ? true : false;
		GHackMouse.middleButtonDown |= ( buttonMask & static_cast< Uint32 >( Windows::Xbox::Input::GamepadButtons::Y ) ) != 0 ? true : false;
	}
}

SRawMouseReading DurangoHackGetMouseReading()
{
	if ( ! GHackMouseMode )
	{
		return SRawMouseReading();
	}
	return GHackMouse;
}

//////////////////////////////////////////////////////////////////////////
// OnKeyDown
// Called when a key is pressed. 
void GameView::OnKeyDown( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::KeyEventArgs^ keyEventArgs )
{
	const UINT key = static_cast< UINT >( keyEventArgs->VirtualKey );

	if ( GHackMouseMode && key == VK_NUMPAD8 )
	{
		GHackMouseSpeed += MOUSE_HACK_SPEED_STEP;
		if ( GHackMouseSpeed > MOUSE_HACK_SPEED_MAX )
		{
			GHackMouseSpeed = MOUSE_HACK_SPEED_MAX;
		}
		return;
	}

	if ( GHackMouseMode && key == VK_NUMPAD2 )
	{
		GHackMouseSpeed -= MOUSE_HACK_SPEED_STEP;
		if ( GHackMouseSpeed < MOUSE_HACK_SPEED_STEP )
		{
			GHackMouseSpeed = MOUSE_HACK_SPEED_STEP;
		}
		return;
	}

	if ( key == VK_MULTIPLY )
	{
		GHackMouseMode = !GHackMouseMode;
// 		if ( ! GHackMouseMode )
// 		{
// 			GHackMouseSpeed = MOUSE_HACK_SPEED_STEP;
// 		}
		return;
	}

	if( key < 256 )
	{
		SDurangoKeyboardReading.AddKeyEvent( key, true );
	}
}

//////////////////////////////////////////////////////////////////////////
// OnKeyUp
//  Called when a key is released
//
// [Note you currently must press F12 after boot on the alpha kit, in order for 
// keyboard events to be sent to the app.]
void GameView::OnKeyUp( Windows::UI::Core::CoreWindow^ window, Windows::UI::Core::KeyEventArgs^ keyEventArgs )
{
	const UINT key = static_cast< UINT >( keyEventArgs->VirtualKey );
	if( key < 256 )
	{
		SDurangoKeyboardReading.AddKeyEvent( key, false );
	}
}

#endif // RED_PLATFORM_DURANGO

