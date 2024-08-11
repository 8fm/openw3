/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiWindow : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiWindow(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiWindow();

		// Events
		Event1_Package EventWindowChangeCoord;
		Event1_Package EventWindowOpened;
		Event1_Package EventWindowClosed;

		virtual void SetCaption(const String& value);
		virtual String GetCaption();

		Uint32 GetTitleBarHeight() const;

		virtual void SetVisible(Bool value);

		virtual void SetSize(const Vector2& value);

		// Set control position (position of left top corner)
		virtual void SetPosition(const Vector2& position);
		virtual void SetPosition(Int32 left, Int32 top);

		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		void SetMovable(Bool value);
		Bool GetMovable() const;

		void SetResizable(Bool value);
		Bool GetResizable() const;

		void AddChild( CRedGuiControl* child );
		void RemoveChild( CRedGuiControl* child );

		// Set padding
		void SetPadding( const Box2& padding);

		void SetEnabledCaptionButton(enum ECaptionButton button, Bool value);
		Bool GetEnabledCaptionButton(enum ECaptionButton button) const;

		void SetVisibleCaptionButton(enum ECaptionButton button, Bool value);
		Bool GetVisibleCaptionButton(enum ECaptionButton button) const;

		void SetTitleBarVisible( Bool value );
		Bool GetTitleBarVisible() const;

		void SetMenuBar(CRedGuiMenuBar* menuBar);
		CRedGuiMenuBar* GetMenuBar() const;

		void SetHelpControl(CRedGuiControl* helpControl);

		void Draw();

		void Maximize();
		void Minimize();

		Bool IsMinimized() const;
		Bool IsMaximized() const;

		Bool IsAWindow();

	protected:
		void PropagateWindowClosed();
		void PropagateWindowOpened();
		virtual void OnWindowClosed(CRedGuiControl* control) { /*intentinoally empty */ };
		virtual void OnWindowOpened(CRedGuiControl* control) { /*intentinoally empty */ };

		void OnMouseLostFocus( CRedGuiControl* newControl );

		void NotifyMousePressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );

		void NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseBorderDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void NotifyKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text );

	private:
		void CheckMinMaxSize( Vector2& size );
		void OnKeyChangeRootFocus( Bool focus );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override;

	private:
		CRedGuiPanel*	m_captionPanel;			//!<
		CRedGuiButton*	m_buttonWithText;		//!<
		CRedGuiButton*	m_help;					//!<
		CRedGuiButton*	m_minimize;				//!<
		CRedGuiButton*	m_maximize;				//!<
		CRedGuiButton*	m_exit;					//!<
		CRedGuiPanel*	m_menuBarPanel;			//!<
		CRedGuiMenuBar* m_menuBar;				//!<

		CRedGuiPanel*	m_leftBorder;			//!<
		CRedGuiPanel*	m_rightBorder;			//!<
		CRedGuiPanel*	m_bottomBorder;			//!<
		CRedGuiPanel*	m_leftBottomCorner;		//!<
		CRedGuiPanel*	m_rightBottomCorner;	//!<

		CRedGuiImage*	m_backgroundImage;		//!<
		CRedGuiControl*	m_internalKeyFocus;		//!<
		CRedGuiControl*	m_helpControl;			//!<

		Bool			m_mouseRootFocus;		//!<
		Bool			m_keyRootFocus;			//!<

		Vector2			m_PreMovePosition;		//!<
		Vector2			m_PreMoveSize;			//!<

		CRedGuiControl* m_client;				//!<
		Bool			m_movable;				//!<
		Bool			m_resizable;			//!<

		Bool			m_isMaximize;			//!<
		Bool			m_isMinimize;			//!<

		Bool			m_canMoveByPad;			//!<
		Bool			m_canResizeByPad;		//!<

		Box2			m_minPreviousCoord;		//!<
	};
	
}	// namespace RedGui

#endif	// NO_RED_GUI
