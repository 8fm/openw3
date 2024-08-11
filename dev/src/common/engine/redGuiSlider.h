/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiSlider : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiSlider(Uint32 x, Uint32 y, Uint32 width,Uint32 height);
		virtual ~CRedGuiSlider();

		// Event
		Event2_PackageFloat EventScroll;

		void Draw();

		void SetValue( Float value, Bool silentChange = false );
		Float GetValue() const;

		void SetMinValue( Float value );
		Float GetMinValue() const;

		void SetMaxValue( Float value );
		Float GetMaxValue() const;

		void SetStepValue( Float value);
		Float GetStepValue() const;

	protected:
		void UpdateTrack( Bool silentChange = false );
		void TrackMove( Float left, Float top, const Vector2& lastClickedPoint );

		Float GetTrackSize() const;
		Float GetSliderLineLenght() const;

		virtual void OnMouseWheel( Int32 delta );
		virtual void OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button );
		virtual void OnSizeChanged( const Vector2& oldSize, const Vector2& newSize );

		void NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta);
		void NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);               
		
		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

		CRedGuiButton*	m_track;				//!<
		CRedGuiPanel*	m_line;					//!<
		Float			m_value;				//!<
		Float			m_minValue;				//!<
		Float			m_maxValue;				//!<
		Float			m_stepValue;			//!<

		Vector2			m_preActionOffset;		//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
