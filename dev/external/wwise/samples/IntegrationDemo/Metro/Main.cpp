//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#define AK_IMPLEMENT_THREAD_EMULATION
#include <AK/Tools/Win32/ThreadEmulation.h>

#include "Main.h"
#include "BasicTimer.h"
#include "IntegrationDemo.h"
#include "Helpers.h"
#include "InputMgr.h"
#include "Platform.h"

#include <AK/SoundEngine/Common/AkMemoryMgr.h>		// Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Common/IAkStreamMgr.h>		// Streaming Manager
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>	// Music Engine
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>	// AkStreamMgrModule
#include <AK/Plugin/AllPluginsFactories.h>

#include "AkFilePackageLowLevelIOBlocking.h"

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>	// Communication between Wwise and the game (excluded in release build)
#endif

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace concurrency;

/////////////////////////////////////////////////////////////////////////////////
//                              MEMORY HOOKS SETUP
//
//                             ##### IMPORTANT #####
//
// These custom alloc/free functions are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
/////////////////////////////////////////////////////////////////////////////////

namespace AK
{
	void * AllocHook( size_t in_size )
	{
		return malloc( in_size );
	}
	void FreeHook( void * in_ptr )
	{
		free( in_ptr );
	}
	void * VirtualAllocHook(
		void * in_pMemAddress,
		size_t in_size,
		DWORD in_dwAllocationType,
		DWORD in_dwProtect
		)
	{
		return malloc( in_size );
	}
	void VirtualFreeHook( 
		void * in_pMemAddress,
		size_t in_size,
		DWORD in_dwFreeType
		)
	{
		free( in_pMemAddress );
	}
}

CAkFilePackageLowLevelIOBlocking g_lowlevelIO;

Main::Main() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}

void Main::Initialize(CoreApplicationView^ applicationView)
{
	applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Main::OnActivated);

	CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &Main::OnSuspending);

	CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &Main::OnResuming);

//	m_renderer = ref new CubeRenderer();

	m_textRenderer = ref new SimpleTextRenderer();
}

void Main::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &Main::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &Main::OnVisibilityChanged);

	window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &Main::OnWindowClosed);

	window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Main::OnPointerPressed);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Main::OnPointerReleased);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Main::OnPointerMoved);

//	m_renderer->Initialize(CoreWindow::GetForCurrentThread());

	m_textRenderer->Initialize(
		CoreWindow::GetForCurrentThread(),
		DisplayProperties::LogicalDpi
		);

	AkMemSettings memSettings;
	AkStreamMgrSettings stmSettings;
	AkDeviceSettings deviceSettings;
	AkInitSettings initSettings;
	AkPlatformInitSettings platformInitSettings;
	AkMusicSettings musicInit;

	IntegrationDemo::Instance().GetDefaultSettings(memSettings, stmSettings, deviceSettings, initSettings, platformInitSettings, musicInit);

	// Initialize the various components of the application and show the window
	AkOSChar szError[500];
	if ( !IntegrationDemo::Instance().Init( memSettings, stmSettings, deviceSettings, initSettings, platformInitSettings, musicInit, NULL, szError, IntegrationDemoHelpers::AK_ARRAYSIZE(szError), 1366, 768 ) )
	{
		AkOSChar szMsg[550];
		__AK_OSCHAR_SNPRINTF( szMsg, IntegrationDemoHelpers::AK_ARRAYSIZE(szMsg), AKTEXT("Failed to initialize the Integration Demo.\r\n%s"), szError );
		AKPLATFORM::OutputDebugMsg( szMsg );
	}

	m_gestureRecognizer = ref new GestureRecognizer(); 
	m_gestureRecognizer->GestureSettings = 
        GestureSettings::DoubleTap; 
 
	m_gestureRecognizer->Tapped += 
        ref new TypedEventHandler<GestureRecognizer^, TappedEventArgs^>(this, &Main::OnTapped); 

}

void Main::Load(Platform::String^ entryPoint)
{
}

void Main::Run()
{
	BasicTimer^ timer = ref new BasicTimer();

	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			timer->Update();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			m_textRenderer->Update(timer->Total, timer->Delta);
			//m_textRenderer->Render();
			// Process the current frame, quit if Update() is false.
			if ( ! IntegrationDemo::Instance().Update() )
			{
				break;
			}
			IntegrationDemo::Instance().Render();
		
			// Ends the frame and regulates the application's framerate
			IntegrationDemo::Instance().EndFrame();
			m_textRenderer->Present(); // This call is synchronized to the display frame rate.
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void Main::Uninitialize()
{
	// Terminate the various components of the application
	IntegrationDemo::Instance().Term();
}

void Main::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_textRenderer->UpdateForWindowSizeChange();
}

void Main::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void Main::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

void Main::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	IntegrationDemo::Instance().OnPointerEvent( PointerEventType_Pressed, (int) args->CurrentPoint->Position.X, (int) args->CurrentPoint->Position.Y );
	m_gestureRecognizer->ProcessDownEvent(args->CurrentPoint); 
}

void Main::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
	IntegrationDemo::Instance().OnPointerEvent( PointerEventType_Released, (int) args->CurrentPoint->Position.X, (int) args->CurrentPoint->Position.Y );
	m_gestureRecognizer->ProcessUpEvent(args->CurrentPoint); 
}

void Main::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	IntegrationDemo::Instance().OnPointerEvent( PointerEventType_Moved, (int) args->CurrentPoint->Position.X, (int) args->CurrentPoint->Position.Y );
	m_gestureRecognizer->ProcessMoveEvents(args->GetIntermediatePoints()); 
}

void Main::OnTapped( GestureRecognizer^ gestureRecognizer, TappedEventArgs^ args ) 
{ 
    if (args->TapCount == 1) // the tap event is a double tap
		IntegrationDemo::Instance().OnPointerEvent( PointerEventType_Tapped, (int) 0, (int) 0 );
	else
		IntegrationDemo::Instance().OnPointerEvent( PointerEventType_DoubleTapped, (int) 0, (int) 0 );
} 

void Main::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void Main::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		// Insert your code here.

		deferral->Complete();
	});
}
 
void Main::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
    return ref new Main();
}

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}
