/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiAdvancedSlider : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiAdvancedSlider( Uint32 x, Uint32 y, Uint32 width, Uint32 height );
		virtual ~CRedGuiAdvancedSlider();

		// Event
		Event2_PackageFloat EventScroll;

		void Draw();

		void SetValue( Float value );
		Float GetValue() const;

		void SetMinValue( Float value );
		Float GetMinValue() const;

		void SetMaxValue( Float value );
		Float GetMaxValue() const;

		void SetStepValue( Float value);
		Float GetStepValue() const;

	private:

		virtual void OnPendingDestruction() override final;

		void NotifyScrollChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value );

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

	private:
		CRedGuiSlider*		m_slider;
		CRedGuiLabel*		m_minValue;
		CRedGuiLabel*		m_maxValue;
		CRedGuiLabel*		m_value;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
