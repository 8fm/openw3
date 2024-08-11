/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGraphicContext.h"
#include "redGuiDesktop.h"
#include "redGuiManager.h"
#include "../core/configVar.h"
#include "inputKeys.h"
#include "renderFrame.h"
#include "renderSettings.h"

RED_DEFINE_NAME( RedGuiDefaultTheme );
RED_DEFINE_NAME( RedGuiGradientTheme );

namespace Config
{
	TConfigVar<Int32, Validation::IntRange<0,255>> cvGlobalAlpha( "RedGui", "GlobalAlpha", 127, eConsoleVarFlag_Save );
}

namespace RedGui
{	
	CRedGuiManager::CRedGuiManager()
		: m_fontManager(nullptr)
		, m_inputManager(nullptr)
		, m_layerManager(nullptr)
		, m_themeManager(nullptr)
		, m_renderManager(nullptr)
		, m_toolTipManager(nullptr)
		, m_enabled(false)
		, m_exclusiveInput( RGEI_Both )
	{
		/* this shouldn't be empty - when we use singleton, we expect it to work without additional initialization */
		m_fontManager = new CRedGuiFontManager();
		m_inputManager = new CRedGuiInputManager();
		m_renderManager = new CRedGuiRenderManager();
		m_themeManager = new CRedGuiThemeManager( m_renderManager->GetGraphicContext() );
		m_layerManager = new CRedGuiLayerManager();
		m_toolTipManager = new CRedGuiToolTipManager( this );

	}

	CRedGuiManager::~CRedGuiManager()
	{
		DeleteReportedControl();		

		if(m_fontManager != nullptr)
		{
			delete m_fontManager;
			m_fontManager = nullptr;
		}

		if(m_layerManager != nullptr)
		{
			delete m_layerManager;
			m_layerManager = nullptr;
		}

		if(m_themeManager != nullptr)
		{
			delete m_themeManager;
			m_themeManager = nullptr;
		}

		if(m_renderManager != nullptr)
		{
			delete m_renderManager;
			m_renderManager = nullptr;
		}

		if(m_toolTipManager != nullptr)
		{
			delete m_toolTipManager;
			m_toolTipManager = nullptr;
		}

		if(m_inputManager != nullptr)
		{
			delete m_inputManager;
			m_inputManager = nullptr;
		}
	}

	void CRedGuiManager::Init()
	{
		m_messageBox = new RedGui::CRedGuiMessageBox();
		
		m_inputManager->LoadPointerImages();
	}

	void CRedGuiManager::Shutdown()
	{
	}

	Bool CRedGuiManager::GetEnabled() const
	{
		return m_enabled;
	}

	void CRedGuiManager::SetEnabled(Bool value)
	{
		m_enabled = value;

		if( m_enabled == true )
		{
			m_inputManager->ShowCursor();
		}
		else
		{
			m_inputManager->HideCursor();
		}
	}

	Bool CRedGuiManager::IsBudgetModeOn()
	{
		return GetLayerManager()->IsBudgetMode();
	}

	void CRedGuiManager::SetBudgetMode( Bool value )
	{
		GetLayerManager()->SetBudgetMode( value );
		static Uint32 previousGlobalAlpha = 127;
		if ( value )
		{
			previousGlobalAlpha = GetRenderManager()->GetGraphicContext()->GetGlobalAlpha();
			GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( 60 );
		}
		else
		{
			GetRenderManager()->GetGraphicContext()->SetGlobalAlpha( previousGlobalAlpha );
		}
	}

	void CRedGuiManager::OnTick(Float timeDelta)
	{
		GetInputManager()->OnTick( timeDelta );
		EventTick( nullptr, timeDelta );

		DeleteReportedControl();
	}

	CRedGuiFontManager* CRedGuiManager::GetFontManager() const
	{
		return m_fontManager;
	}

	CRedGuiInputManager* CRedGuiManager::GetInputManager() const
	{
		return m_inputManager;
	}

	CRedGuiLayerManager* CRedGuiManager::GetLayerManager() const
	{
		return m_layerManager;
	}

	CRedGuiThemeManager* CRedGuiManager::GetThemeManager() const
	{
		return m_themeManager;
	}

	CRedGuiRenderManager* CRedGuiManager::GetRenderManager() const
	{
		return m_renderManager;
	}

	CRedGuiToolTipManager* CRedGuiManager::GetToolTipManager() const
	{
		return m_toolTipManager;
	}

	void CRedGuiManager::OnViewportCalculateCamera( IViewport* view, CRenderCamera& camera )
	{
		EventCalculateCamera( RedGui::CRedGuiEventPackage( nullptr ), view, camera);
	}

	Bool CRedGuiManager::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// combo for catch all
		{
			static Bool halfResult = false;

			if( key == IK_Pad_Back_Select )
			{
				halfResult = true;
			}
			else if( key == IK_Pad_LeftThumb  )
			{
				if( halfResult == true )
				{
					// calculate next value
					Uint32 nextValue = (Uint32)m_exclusiveInput + 1;
					if( nextValue == RedGui::RGEI_Count )
					{
						m_exclusiveInput = RGEI_OnlyRedGui;
					}

					SetExclusiveInput( m_exclusiveInput );
					halfResult = false;
				}
			}
			else
			{
				halfResult = false;
			}
		}

		if( m_exclusiveInput != RGEI_OnlyGame )
		{
			Bool result = GetInputManager()->OnViewportInput( view, key, action, data );

			RedGui::CRedGuiControl* mouseFocusedCtrl = GetInputManager()->GetMouseFocusControl();
			if( mouseFocusedCtrl == nullptr || mouseFocusedCtrl->GetParent() == GetActiveDesktop() )
			{
				result = false;
			}

			Bool result2 = EventViewportInput( RedGui::CRedGuiEventPackage( nullptr ), view, key, action, data);

			if( m_exclusiveInput == RGEI_OnlyRedGui )
			{
				return true;
			}
			else
			{
				return ( result || result2 );
			}
		}

		return false;
	}

	void CRedGuiManager::MessageBox( const String& text, const String& title, EMessabeBoxIcon icon /*= MESSAGEBOX_Info*/ )
	{
		if ( m_messageBox )
		{
			m_messageBox->Show(text, title, icon);
		}
	}

	void CRedGuiManager::MessageBox( const String& text, EMessabeBoxIcon icon /*= MESSAGEBOX_Info*/ )
	{
		if ( m_messageBox )
		{
			m_messageBox->Show(text, icon);
		}
	}

	Bool CRedGuiManager::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
	{
		Bool result = GetInputManager()->OnViewportClick( view, button, state, x, y );

		RedGui::CRedGuiControl* focusedCtrl = GetInputManager()->GetMouseFocusControl();
		if( focusedCtrl == nullptr || focusedCtrl->GetParent() == GetActiveDesktop() )
		{
			result = false;
		}

		Bool result2 = EventViewportClick( RedGui::CRedGuiEventPackage( nullptr ), view, button, state, Vector2((Float)x, (Float)y) );

		return ( result || result2 );
	}

	void CRedGuiManager::DisposeControl( CRedGuiControl* control )
	{
		if( control != nullptr )
		{
			InvalidateControl( control );
			m_controlsToDelete.PushBackUnique( control );
		}
	}

	void CRedGuiManager::InvalidateControl( CRedGuiControl* control )
	{
		if(GetInputManager()->GetMouseFocusControl() == control)
		{
			GetInputManager()->ResetMouseCaptureControl();
			GetInputManager()->ResetMouseFocusControl();
		}

		if(GetInputManager()->GetKeyFocusControl() == control)
		{
			GetInputManager()->ResetKeyFocusControl();
		}
	}

	void CRedGuiManager::DeleteReportedControl()
	{
		ArrayControlPtr controlsToDelete;
		controlsToDelete.SwapWith( m_controlsToDelete );

		for( Uint32 index = 0, end = controlsToDelete.Size(); index != end; ++index ) 
		{
			controlsToDelete[ index ]->NotifyPendingDestruction();
		}

		for( Uint32 index = 0, end = controlsToDelete.Size(); index != end; ++index ) 
		{
			delete controlsToDelete[ index ];
		}
	}

	Bool CRedGuiManager::OnViewportTrack( const CMousePacket& packet )
	{
		Bool result = GetInputManager()->OnViewportTrack( packet );

		RedGui::CRedGuiControl* focusedCtrl = GetInputManager()->GetMouseFocusControl();
		if( focusedCtrl == nullptr || focusedCtrl->GetParent() == GetActiveDesktop() )
		{
			result = false;
		}

		Bool result2 = EventVieportTrack( RedGui::CRedGuiEventPackage( nullptr ), packet );

		return ( result || result2 );
	}

	void CRedGuiManager::OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame )
	{
		CRenderFrameInfo& frameInfo = frame->GetFrameInfo();
		Bool showVisualDebug = frameInfo.IsShowFlagOn( SHOW_VisualDebug );

		// enable visual debug for the time of rendering debug pages
		frameInfo.SetShowFlag( SHOW_VisualDebug, true );

		GetLayerManager()->Draw( frame );
		EventViewportGenerateFragments( RedGui::CRedGuiEventPackage( nullptr ), view, frame );
		
		// set visual debug visibility to previous value
		frameInfo.SetShowFlag( SHOW_VisualDebug, showVisualDebug );
	}

	void CRedGuiManager::RegisterDesktop( CRedGuiDesktop* desktop )
	{
		desktop->AttachToLayer( TXT("Main") );
		m_desktops.PushBack( desktop );
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( m_messageBox );
	}

	void CRedGuiManager::UnregisterDesktop( CRedGuiDesktop* desktop )
	{
		desktop->DetachFromLayer();
		m_desktops.Remove( desktop );
		desktop->Dispose();
		m_activeDesktop = 0;
	}

	void CRedGuiManager::OnViewportSetDimensions( IViewport* view, Int32 width, Int32 height )
	{
		if( Config::cvForcedRendererOverlayResolution.Get() )
		{
			width = Config::cvForcedRendererResolutionWidth.Get();
			height = Config::cvForcedRendererResolutionHeight.Get();	
		}

		GetLayerManager()->ResizeView( Vector2( (Float)width, (Float)height ) );
		GetRenderManager()->SetViewSize( Vector2( (Float)width, (Float)height ) );
		EventViewportSetDimensions( RedGui::CRedGuiEventPackage( nullptr ), view, width, height );
	}

	void CRedGuiManager::RegisterWindowInActiveDesktop( CRedGuiWindow* window )
	{
		if( m_desktops.Size() > 0 && window )
		{
			m_desktops[m_activeDesktop]->AddWindow( window );
		}
	}

	void CRedGuiManager::UnregisterWindowFromActiveDesktop( CRedGuiWindow* window )
	{
		if( m_desktops.Size() > 0 && window )
		{
			m_desktops[m_activeDesktop]->RemoveWindow( window );
		}
	}

	CRedGuiDesktop* CRedGuiManager::GetActiveDesktop() const
	{
		if( m_desktops.Size() > 0 )
		{
			return m_desktops[m_activeDesktop];
		}

		return nullptr;
	}

	Uint32 CRedGuiManager::GetActiveDesktopIndex() const
	{
		return m_activeDesktop;
	}

	void CRedGuiManager::SetActiveDesktopIndex( Uint32 index )
	{
		if( index < m_desktops.Size() )
		{
			m_activeDesktop = index;
		}
	}

	void CRedGuiManager::SetExclusiveInput( ERedGuiExclusiveInput value )
	{
		m_exclusiveInput = value;
		EventExclusiveInputChanged( RedGui::CRedGuiEventPackage( nullptr ), (Uint32)m_exclusiveInput );
	}

	ERedGuiExclusiveInput CRedGuiManager::GetExclusiveInput() const
	{
		return m_exclusiveInput;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
