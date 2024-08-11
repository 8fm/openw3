/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "../core/depot.h"

#include "swfResource.h"
#include "viewport.h"
#include "guiGlobals.h" 
#include "game.h"
#include "localizationManager.h"
#include "renderCommands.h"

#include "renderer.h"
#include "renderScaleform.h"
#include "renderScaleformCommands.h"

#include "flashMovieScaleform.h"

#include "renderScaleform.h"

#include "scaleformSystem.h"
#include "scaleformLoader.h"

#include "flashPlayerScaleform.h"

CFlashPlayerScaleform::CFlashPlayerScaleform()
	: CFlashPlayer()
	, m_system( CScaleformSystem::StaticInstance() )
	, m_isInitialized( false )
	, m_isInTick( false )
	, m_isCenterMouseRequested( false )
{
	ASSERT( ::SIsMainThread() );
	
	RED_FATAL_ASSERT( m_system, "No system");
	m_system->RegisterScaleformPlayer( this );

	m_inputManager.SetInputEventListener( this );
}

CFlashPlayerScaleform::~CFlashPlayerScaleform()
{
	ASSERT( ::SIsMainThread() );

	m_inputManager.SetInputEventListener( nullptr );

	Shutdown();
}

//////////////////////////////////////////////////////////////////////////

void CFlashPlayerScaleform::Init()
{
	if ( m_isInitialized )
	{
		return;
	}

	m_isInitialized = true;

	CScaleformSystem* sys = CScaleformSystem::StaticInstance();
	if ( ! sys )
	{
		return;
	}

	m_loader = sys->GetLoader();
}

//////////////////////////////////////////////////////////////////////////

void CFlashPlayerScaleform::Shutdown()
{
	if ( m_system )
	{
		NotifyShuttingDown();
		
		if ( GRender )
		{
			GRender->Flush();
		}

		m_loader.Clear();

		RED_FATAL_ASSERT( m_system, "No system");
		m_system->UnregisterScaleformPlayer( this );
		m_system = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////

void CFlashPlayerScaleform::OnGFxKeyEvent( const GFx::KeyEvent& event )
{
	ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->OnGFxKeyEvent( event ); } );
}

void CFlashPlayerScaleform::OnGFxMouseEvent( const GFx::MouseEvent& event )
{
	ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->OnGFxMouseEvent( event ); } );
}

void CFlashPlayerScaleform::OnGFxGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event )
{
	ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->OnGFxGamePadAnalogEvent( event ); } );
}

void CFlashPlayerScaleform::OnExternalInterface( GFx::Movie* movie, const SFChar* methodName, const GFx::Value* args, SFUInt argCount )
{
	ASSERT( movie );
	ASSERT( methodName );

	String stringMethodName = FLASH_UTF8_TO_TXT( methodName );

	TDynArray< CFlashValue > flashValueArgs;
	flashValueArgs.Reserve( argCount );
	for ( Uint32 i = 0; i < argCount; ++i )
	{
		flashValueArgs.PushBack( CFlashValue( args[i] ) );
	}

	CFlashMovieScaleform* flashMovieScaleform = reinterpret_cast< CFlashMovieScaleform* >( movie->GetUserData() );
	ASSERT( flashMovieScaleform );
	if ( ! flashMovieScaleform )
	{
		GUI_ERROR( TXT("OnExternalInterface: can't get flash movie wrapper in callback '%ls'"), stringMethodName.AsChar() );
		return;
	}

	// Use override handler instead.
	if ( flashMovieScaleform->GetExternalInterfaceOverride() )
	{
		IFlashExternalInterfaceHandler* overrideHandler = flashMovieScaleform->GetExternalInterfaceOverride();
		CFlashValue retval;
		overrideHandler->OnFlashExternalInterface( stringMethodName, flashMovieScaleform, flashValueArgs, retval );
		movie->SetExternalInterfaceRetVal( retval.AsGFxValue() );
		return;
	}

	IFlashExternalInterfaceHandler* flashExternalInterfaceHandler = nullptr;
	if ( ! m_externalInterfaceHandlers.Find( stringMethodName, flashExternalInterfaceHandler ) )
	{
		GUI_WARN( TXT("OnExternalInterface: no registered native function for '%ls'"), stringMethodName.AsChar() );
		return;
	}

	ASSERT( flashExternalInterfaceHandler );
	if ( ! flashExternalInterfaceHandler )
	{
		GUI_ERROR( TXT("OnExternalInterface: native function for '%ls' is NULL!"), stringMethodName.AsChar() );
		return;
	}

	CFlashValue retval;
	flashExternalInterfaceHandler->OnFlashExternalInterface( stringMethodName, flashMovieScaleform, flashValueArgs, retval );
	movie->SetExternalInterfaceRetVal( retval.AsGFxValue() );
}

void CFlashPlayerScaleform::OnUserEvent( GFx::Movie* movie, const GFx::Event& event )
{
	// FIXME: Queue thread safe and dispatch on tick, since can now fire from another thread
#if 0
	RED_UNUSED( movie );
	switch ( event.Type )
	{
	case GFx::Event::DoShowMouse:
		{
			ForEach( m_mouseCursorHandlers, [&]( IFlashMouseCursorHandler* handler ) { handler->OnFlashShowMouseCursor(); } );
		}
		break;
	case GFx::Event::DoHideMouse:
		{
			ForEach( m_mouseCursorHandlers, [&]( IFlashMouseCursorHandler* handler ) { handler->OnFlashHideMouseCursor(); } );
		}
		break;
	case GFx::Event::DoSetMouseCursor:
		{
			GFx::MouseCursorEvent mouseCursorEvent = static_cast< const GFx::MouseCursorEvent& >( event );
			typedef IFlashMouseCursorHandler::ECursorType ECursorType;
			ECursorType cursorType = static_cast< ECursorType >( mouseCursorEvent.mCursorShape );
			ForEach( m_mouseCursorHandlers, [&]( IFlashMouseCursorHandler* handler ) { handler->OnFlashSetMouseCursor( cursorType ); } );
		}
		break;
		// If you want to support different stage scale modes from actionscript.
		//case GFx::Event::EnableClipping:
		//case GFx::Event::DisableClipping:
		//break;
	default:
		break;
	}
#endif // if 0
}

Bool CFlashPlayerScaleform::RegisterStatusListener( IFlashPlayerStatusListener* statusListener )
{
	ASSERT( statusListener );
	if ( ! statusListener )
	{
		GUI_ERROR( TXT("RegisterStatusListener: attempted to register NULL listener") );
		return false;
	}

	if ( ! m_statusListeners.PushBackUnique( statusListener ) )
	{
		GUI_ERROR( TXT("RegisterStatusListener: Listener already registered.") );
		return false;
	}
	return true;
}

Bool CFlashPlayerScaleform::UnregisterStatusListener( IFlashPlayerStatusListener* statusListener )
{
	ASSERT( statusListener );
	if ( ! statusListener )
	{
		GUI_ERROR( TXT("UnregisterStatusListener: attempted to unregister NULL listener") );
		return false;
	}
	if ( ! m_statusListeners.Remove( statusListener ) )
	{
		GUI_ERROR( TXT("UnregisterStatusListener: Listener not found.") );
		return false;
	}
	return true;
}

Bool CFlashPlayerScaleform::RegisterExternalInterface( const String& methodName, IFlashExternalInterfaceHandler* flashExternalInterfaceHandler )
{
	ASSERT( flashExternalInterfaceHandler );
	if ( ! flashExternalInterfaceHandler )
	{
		GUI_ERROR( TXT("RegisterExternalInterface: attempted to register NULL handler") );
		return false;
	}
	if ( ! m_externalInterfaceHandlers.Insert( methodName, flashExternalInterfaceHandler ) )
	{
		GUI_ERROR( TXT("RegisterExternalInterface: Failed to register callback for method name '%ls' "), methodName.AsChar() );
		return false;
	}
	return true;
}

Bool CFlashPlayerScaleform::UnregisterExternalInterface( const String& methodName )
{
	if ( ! m_externalInterfaceHandlers.Erase( methodName ) )
	{
		GUI_ERROR( TXT("RegisterExternalInterface: Failed to unregister callback for method name '%ls' "), methodName.AsChar() );
		return false;
	}
	return true;
}

Bool CFlashPlayerScaleform::RegisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler )
{
	ASSERT( flashMouseCursorHandler );
	if ( ! flashMouseCursorHandler )
	{
		GUI_ERROR( TXT("RegisterMouseCursorHandler: attempted to register NULL handler") );
		return false;
	}
	if ( ! m_mouseCursorHandlers.PushBackUnique( flashMouseCursorHandler ) )
	{
		GUI_ERROR( TXT("RegisterMouseCursorHandler: Failed to register mouse cursor handler") );
		return false;
	}
	return true;
}

Bool CFlashPlayerScaleform::UnregisterMouseCursorHandler( IFlashMouseCursorHandler* flashMouseCursorHandler )
{
	ASSERT( flashMouseCursorHandler );
	if ( ! flashMouseCursorHandler )
	{
		GUI_ERROR( TXT("UnregisterMouseCursorHandler: attempted to unregister NULL handler") );
		return false;
	}
	if ( ! m_mouseCursorHandlers.Remove( flashMouseCursorHandler ) )
	{
		GUI_ERROR( TXT("UnregisterMouseCursorHandler: Failed to unregister mouse cursor handler") );
		return false;
	}
	return true;
}

void CFlashPlayerScaleform::NotifyShuttingDown()
{
	ForEach( m_statusListeners, [&]( IFlashPlayerStatusListener* statusListener ) { statusListener->OnFlashPlayerShuttingDown(); } );
}

void CFlashPlayerScaleform::OnScaleformInit()
{
	Init();
}

void CFlashPlayerScaleform::OnScaleformShutdown()
{
	Shutdown();
}

Bool CFlashPlayerScaleform::RegisterScaleformMovie( CFlashMovieScaleform* movie )
{
	ASSERT( ::SIsMainThread() );
	ASSERT( ! m_isInTick );
	if ( m_movieWeakRefs.InsertUnique( movie ) != m_movieWeakRefs.End() )
	{
		movie->SetViewport( m_viewport );

		//TBD:
		//movie->SetFocus(); // ---> GFx::SetFocusEvent

		return true;
	}
	return false;
}

Bool CFlashPlayerScaleform::UnregisterScaleformMovie( CFlashMovieScaleform* movie )
{
	ASSERT( ::SIsMainThread() );
	ASSERT( ! m_isInTick );
	return m_movieWeakRefs.RemoveFast( movie );
}

//////////////////////////////////////////////////////////////////////////

void CFlashPlayerScaleform::Tick( Float timeDelta, const Rect& viewport )
{
	PC_SCOPE_PIX( FlashPlayerTick );

	if ( m_isInTick )
	{
		GUI_ERROR( TXT("Tick: Recursive tick") );
		return;
	}
	Red::System::ScopedFlag<Bool> scopedTick( m_isInTick = true, false );

	ASSERT( ::SIsMainThread() );

	if ( GRender && !GRender->IsDeviceLost() )
	{
		PC_SCOPE_PIX( GUISystem_TickAll );

		UpdateViewportLayout( viewport );

		if ( m_viewport.Height() == 0 && m_viewport.Width() == 0 )
		{
			return;
		}
	
		m_inputManager.SetViewport( viewport );
		if ( m_isCenterMouseRequested )
		{
			m_isCenterMouseRequested = false;
			m_inputManager.CenterMouse();
		}
		if ( m_isMousePosChangeRequested )
		{
			m_isMousePosChangeRequested = false;
			m_inputManager.ChangeMousePosition(m_desiredMouseX, m_desiredMouseY);
		}
		m_inputManager.ProcessInput();

		{
			PC_SCOPE_PIX( FlashPlayerAdvance );
			ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->Tick( timeDelta ); } );
		}
	}
}

void CFlashPlayerScaleform::Capture( Bool force )
{
	PC_SCOPE_PIX( FlashPlayerCapture );
	ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->Capture( force ); } );
}

void CFlashPlayerScaleform::UpdateViewportLayout( const Rect& viewport )
{
	PC_SCOPE_PIX( FlashPlayerTickNewUpdate );

	ASSERT( ::SIsMainThread() );

	IViewport* vp = GGame->GetViewport();
	if ( nullptr == vp )
	{
		return;
	}

	m_viewport = viewport;

	ForEach( m_movieWeakRefs, [&]( CFlashMovieScaleform* movie ) { movie->SetViewport( viewport ); } );
}

CFlashMovie* CFlashPlayerScaleform::CreateMovie( const TSoftHandle< CSwfResource >& swfHandle, const SFlashMovieInitParams& initParams )
{
	CScaleformLoader::SCreateMovieContext context;

	context.m_externalInterfaceHandler = this;
	context.m_userEventHandler = this;

	GFx::Movie* movie = m_loader->CreateMovie( swfHandle, context, initParams.m_waitForLoadFinish );
	if ( ! movie ) 
	{
		return nullptr;
	}

	Uint32 flags = eFlashMovieFlags_None;
	if ( initParams.m_attachOnStart )
	{
		flags |= eFlashMovieFlags_AttachOnStart;
	}
	if ( initParams.m_notifyPlayer )
	{
		flags |= eFlashMovieFlags_NotifyPlayer;
	}

	CFlashMovieScaleform* flashMovie = new CFlashMovieScaleform( this, movie, SFlashMovieLayerInfo( initParams.m_renderGroup, initParams.m_layer ), flags );

	return flashMovie;
}

#endif // USE_SCALEFORM
