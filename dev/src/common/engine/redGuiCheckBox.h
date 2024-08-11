/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiCheckBox : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiCheckBox(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiCheckBox();

		void Draw();

		void SetChecked( Bool value, Bool silentChange = false );
		Bool GetChecked() const;

		void SetText( const String& text, const Color& textColor = Color::WHITE );
		String GetText() const;

		// Events
		Event2_PackageBool EventCheckedChanged;

	protected:
		void NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value );
		void NotifyEventClicked( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		void CalculateSize();

		virtual void OnPendingDestruction() override final;

		CRedGuiLabel*	m_label;	//!<
		CRedGuiButton*	m_button;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
