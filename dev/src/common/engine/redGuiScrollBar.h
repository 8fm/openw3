/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiScrollBar : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiScrollBar(Uint32 left, Uint32 top, Uint32 width, Uint32 height, Bool verticalAligment = true);
		virtual  ~CRedGuiScrollBar();

		//Events
		Event2_PackageUint32 EventScrollChangePosition;

		// Set vertical alignment (from default is vertical)
		void SetVerticalAlignment(Bool value);
		// Get vertical alignemnt (from default is vertical)
		Bool GetVerticalAlignment() const;

		// Set scroll range
		void SetScrollRange(Uint32 value);
		// Get scroll range
		Uint32 GetScrollRange() const;

		// Set scroll position
		void SetScrollPosition( Int32 value );
		// Get scroll position
		Uint32 GetScrollPosition() const;

		// Set scroll page
		void SetScrollPage(Uint32 value);
		// Get scroll page
		Uint32 GetScrollPage() const;

		Int32 GetLineSize() const;

		void SetTrackSize(Int32 value);
		Int32 GetTrackSize() const;

		void SetMinTrackSize(Int32 value);
		Int32 GetMinTrackSize() const;

		void SetMoveToClick(Bool value);
		Bool GetClickToMove() const;

		void MoveScrollToStart();
		void MoveScrollToEnd();

		void MoveScrollToStartSide();
		void MoveScrollToEndSide();

		void MovePageDown();
		void MovePageUp();

		// Set control position (position of left top corner)
		virtual void SetPosition( const Vector2& position);
		virtual void SetPosition(Int32 left, Int32 top);
		// Set control size
		virtual void SetSize( const Vector2& size);
		virtual void SetSize(Int32 width, Int32 height);
		// Set control position and size
		virtual void SetCoord(const Box2& coord);
		virtual void SetCoord(Int32 left, Int32 top, Int32 width, Int32 height);

		virtual void Draw();

	protected:
		void UpdateTrack();
		void TrackMove( Int32 left, Int32 top, const Vector2& lastClickedPoint );

		virtual void OnMouseWheel(Int32 delta);
		virtual void OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button );

		void NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta);
		void NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);
		void NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button);          

		Int32 GetTrackPlaceLength() const;

		virtual void OnPendingDestruction() override final;

	private:

		CRedGuiButton*	m_start;				//!<
		CRedGuiButton*	m_end;					//!<
		CRedGuiButton*	m_track;				//!<

		Vector2			m_preActionOffset;		//!<

		Uint32			m_rangeStart;			//!<
		Uint32			m_rangeEnd;				//!<

		Uint32			m_scrollRange;			//!<
		Uint32			m_scrollPosition;		//!<
		Uint32			m_scrollPage;			//!<

		Int32			m_minTrackSize;			//!<
		Bool			m_moveToClick;			//!<

		Bool			m_verticalAlignment;	//!<
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
