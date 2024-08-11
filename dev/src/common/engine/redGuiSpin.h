/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiSpin: public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiSpin(Uint32 left, Uint32 top, Uint32 width, Uint32 height);
		virtual ~CRedGuiSpin();

		// Events
		Event2_PackageInt32 EventValueChanged;

		Int32 GetValue() const;
		void SetValue( Int32 value, Bool silentChange = false );

		Int32 GetMinValue() const;
		void SetMinValue(Int32 value);

		Int32 GetMaxValue() const;
		void SetMaxValue(Int32 value);
		
		virtual void Draw();

	private:
		void NotifyEventButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );

		void SetNewValue( Int32 newValue, Bool silentChange );

		void NotifyEventMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 value );
		void NotifyEventTextEnter( RedGui::CRedGuiEventPackage& eventPackage );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

	private:
		Int32			m_value;		//!<
		Int32			m_minValue;		//!<
		Int32			m_maxValue;		//!<

		CRedGuiTextBox* m_line;			//!<
		CRedGuiButton*	m_upButton;		//!<
		CRedGuiButton*	m_downButton;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
