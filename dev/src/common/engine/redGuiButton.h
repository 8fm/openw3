/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiButton : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiButton(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiButton();

		// Events
		Event1_Package EventButtonClicked;
		Event2_PackageBool EventCheckedChanged;

		// image mode
		void SetImageMode(Bool value);
		Bool GetImageMode() const;

		// fit to text mode
		void SetFitToText(Bool value);
		Bool GetFitToText() const;

		// manage text on the button
		void SetText(const String& text, const Color& textColor = Color::WHITE);
		String GetText() const;

		void SetTextColor(const Color& color);
		Color GetTextColor() const;

		void SetTextAlign(EInternalAlign align);
		EInternalAlign GetTextAlign() const;

		void SetImage( CGatheredResource& resource );
		void SetImage( THandle< CBitmapTexture > texture );
		void SetImage(const String& pathToFile);

		void SetToggleMode(Bool value);
		Bool GetToggleMode() const;

		void SetToggleValue( Bool value, Bool silentChange = false );
		Bool GetToggleValue() const;
		
		// get internal control
		CRedGuiImage* GetImageBox();

		ERedGuiState GetState() const;
		
		// internal function
		void InternalSetMouseFocus(Bool focus);

		virtual void Draw();

	private:
		virtual void OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button);
		virtual void OnMouseButtonReleased( const Vector2& mousePosition, enum EMouseButton button);

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		void ExecuteClick();
		void OnSizeChanged( const Vector2& oldSize, const Vector2& newSize );
		void FitToText();

		Bool			m_isMousePressed;	//!<

		Bool			m_toggleMode;		//!<
		Bool			m_isOn;				//!<

		CRedGuiImage*	m_image;			//!<
		Bool			m_imageMode;		//!<
		Bool			m_fitToText;		//!<

		CRedGuiLabel*	m_label;			//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
