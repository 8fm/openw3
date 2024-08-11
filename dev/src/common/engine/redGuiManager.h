/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////
// RED NAMES

#ifndef NO_RED_GUI 

RED_DECLARE_NAME( RedGuiDefaultTheme );
RED_DECLARE_NAME( RedGuiGradientTheme );


#include "redGuiTypes.h"
#include "redGuiInputManager.h"
#include "redGuiFontManager.h"
#include "redGuiLayerManager.h"
#include "redGuiThemeManager.h"
#include "redGuiRenderManager.h"
#include "redGuiToolTipManager.h"
#include "redGuiMessageBox.h"

namespace RedGui
{
	class CRedGuiManager
	{
	public:
		CRedGuiManager();
		virtual ~CRedGuiManager();

		// Events
		// TODO
		Event2_PackageFloat EventTick;

		// TODO
		Event2_PackageUint32 EventExclusiveInputChanged;

		// TODO
		Event2_PackageMousePacket EventVieportTrack;

		//TODO
		Event3_PackageViewportCamera EventCalculateCamera;

		// TODO
		Event5_PackageViewportInput EventViewportInput;

		// TODO
		Event5_PackageViewportInt32BoolVector2	EventViewportClick;

		// TODO
		Event3_PackageViewportGenerateFragments EventViewportGenerateFragments;

		// TODO
		Event4_PackageViewportInt32Int32 EventViewportSetDimensions;

		// Initialise RedGui and all RedGui managers
		void Init();
		// Shutdown RedGui and all RedGui managers
		void Shutdown();

		// Get enabled
		Bool GetEnabled() const;
		// Set enabled
		void SetEnabled(Bool value);

		Bool IsBudgetModeOn();
		void SetBudgetMode( Bool value );

		// Called in each frame
		void OnTick( Float timeDelta );
		void OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame );
		Bool OnViewportTrack( const CMousePacket& packet );
		void OnViewportCalculateCamera( IViewport* view, CRenderCamera& camera );
		Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
		void OnViewportSetDimensions( IViewport* view, Int32 width, Int32 height );

		// Message box
		void MessageBox(const String& text, const String& title, EMessabeBoxIcon icon = MESSAGEBOX_Info);
		void MessageBox(const String& text, EMessabeBoxIcon icon = MESSAGEBOX_Info);

		// manage the controls
		void DisposeControl( CRedGuiControl* control );
		void InvalidateControl( CRedGuiControl* control );

		// manage the desktop
		void RegisterDesktop( CRedGuiDesktop* desktop );
		void UnregisterDesktop( CRedGuiDesktop* desktop );

		CRedGuiDesktop* GetActiveDesktop() const;
		Uint32 GetActiveDesktopIndex() const;
		void SetActiveDesktopIndex( Uint32 index );

		// manage the windows
		void RegisterWindowInActiveDesktop( CRedGuiWindow* window );
		void UnregisterWindowFromActiveDesktop( CRedGuiWindow* window );

		// exclusive input for red gui
		void SetExclusiveInput( ERedGuiExclusiveInput value );
		ERedGuiExclusiveInput GetExclusiveInput() const;

		// Get managers
		CRedGuiFontManager* GetFontManager() const;
		CRedGuiInputManager* GetInputManager() const;
		CRedGuiLayerManager* GetLayerManager() const;
		CRedGuiThemeManager* GetThemeManager() const;
		CRedGuiRenderManager* GetRenderManager() const;
		CRedGuiToolTipManager* GetToolTipManager() const;

	private:
		void DeleteReportedControl();

		Bool					m_enabled;			//!< 
		Uint32					m_activeDesktop;	//!< 

		ArrayDesktopPtr			m_desktops;
		ArrayControlPtr			m_controlsToDelete;	//!< controls reported to delete

		CRedGuiFontManager*		m_fontManager;		//!< font manager
		CRedGuiInputManager*	m_inputManager;		//!< input manager
		CRedGuiLayerManager*	m_layerManager;		//!< layer manager
		CRedGuiThemeManager*	m_themeManager;		//!< theme manager
		CRedGuiRenderManager*	m_renderManager;	//!< render manager
		CRedGuiToolTipManager*	m_toolTipManager;	//!< tooltip manager

		CRedGuiMessageBox*		m_messageBox;		//!< global prefab of MessageBox

		ERedGuiExclusiveInput	m_exclusiveInput;
	};

}	// namespace RedGui

//////////////////////////////////////////////////////////////////////////
typedef TSingleton< RedGui::CRedGuiManager, TNoDestructionLifetime > GRedGui; // crashes at exit!!!
//////////////////////////////////////////////////////////////////////////

#endif	// NO_RED_GUI
