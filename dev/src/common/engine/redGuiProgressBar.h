/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiProgressBar : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiProgressBar(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiProgressBar();

		// Event
		Event2_PackageValue EventProgressChanged;

		// Set control position (position of left top corner)
		virtual void SetPosition( const Vector2& position);
		virtual void SetPosition(Int32 left, Int32 top);
		// Set control size
		virtual void SetSize( const Vector2& size);
		virtual void SetSize(Int32 width, Int32 height);
		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		void SetProgressRange(Float value);
		Float GetProgressRange() const;

		void SetProgressPosition(Float value);
		Float GetProgressPosition() const;

		void SetProgressBarColor( const Color& color );
		Color GetProgressBarColor() const;

		void SetProgressAutoTrack(Bool value);
		Bool GetProgressAutoTrack() const;

		void SetShowProgressInformation(Bool value);
		Bool GetShowProgressInformation() const;

		void SetProgressInformation( const String& text );
		String GetProgressInformation() const;

		void Draw();

	private:
		void FrameEntered( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta);
		void UpdateTrack();

		Int32			m_trackStep;		//!<

		Float			m_range;			//!<
		Float			m_endPosition;		//!<
		Float			m_autoPosition;		//!<
		Bool			m_autoTrack;		//!<
		Bool			m_showProgressInfo;	//!<
		String			m_progressInfo;		//!< 
				
		CRedGuiLabel*	m_progressLabel;	//!<

	};

}	// namespace RedGui

#endif	// NO_RED_GUI
