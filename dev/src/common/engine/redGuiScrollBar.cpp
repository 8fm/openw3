/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiButton.h"
#include "redGuiScrollBar.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GScrollMouseWheelCount = 50;
		const Uint32 GDefaultScrollSize = 19;
	}
	
	CRedGuiScrollBar::CRedGuiScrollBar(Uint32 left, Uint32 top, Uint32 width, Uint32 height, Bool verticalAligment /*= true*/)
		: CRedGuiControl(left, 100, (verticalAligment == true) ? GDefaultScrollSize : width, (verticalAligment == true) ? height : GDefaultScrollSize)
		, m_start(nullptr)
		, m_end(nullptr)
		, m_track(nullptr)
		, m_scrollRange(0)
		, m_scrollPosition(0)
		, m_scrollPage(0)
		, m_minTrackSize(0)
		, m_rangeStart(0)
		, m_rangeEnd(0)
		, m_moveToClick(false)
		, m_verticalAlignment(verticalAligment)
	{
		// set default settings
		m_scrollPage = 1;
		m_minTrackSize = 0;
		m_rangeEnd = GDefaultScrollSize;
		m_rangeStart = GDefaultScrollSize;
		SetBackgroundColor(Color(62,62,66));
		SetBorderVisible(false);
		SetNeedKeyFocus( false );

		// create start button
		m_start = new CRedGuiButton(0,0, GDefaultScrollSize, GDefaultScrollSize);
		m_start->EventMouseButtonPressed.Bind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_start->EventMouseWheel.Bind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_start->SetDock((m_verticalAlignment == true) ? DOCK_Top : DOCK_Left);
		m_start->SetImage( ( m_verticalAlignment == true ) ? Resources::GUpArrowIcon : Resources::GLeftArrowIcon );
		m_start->SetBackgroundColor(Color(62,62,66));
		m_start->SetBorderVisible(false);
		m_start->SetNeedKeyFocus( false );
		AddChild(m_start);

		// create end button
		const Int32 butWidth = GetWidth()-GDefaultScrollSize;
		const Int32 butHeight = GetHeight()-GDefaultScrollSize;
		m_end = new CRedGuiButton((verticalAligment == true) ? 0 : Clamp( butWidth, 0, INT_MAX ), (verticalAligment == true) ? Clamp( butHeight, 0, INT_MAX ) : 0, GDefaultScrollSize, GDefaultScrollSize);
		m_end->EventMouseButtonPressed.Bind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_end->EventMouseWheel.Bind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_end->SetDock((m_verticalAlignment == true) ? DOCK_Bottom : DOCK_Right);
		m_end->SetImage( ( m_verticalAlignment == true ) ? Resources::GDownArrowIcon : Resources::GRightArrowIcon  );
		m_end->SetBackgroundColor(Color(62,62,66));
		m_end->SetBorderVisible(false);
		m_end->SetNeedKeyFocus( false );
		AddChild(m_end);

		// create track button
		m_track = new CRedGuiButton((verticalAligment == true) ? 5 : GetWidth()/2, (verticalAligment == true) ? GetHeight()/2 : 5, 9, 9);
		m_track->EventMouseDrag.Bind(this, &CRedGuiScrollBar::NotifyMouseDrag);
		m_track->EventMouseWheel.Bind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_track->EventMouseButtonPressed.Bind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_track->EventMouseButtonReleased.Bind(this, &CRedGuiScrollBar::NotifyMouseButtonReleased);
		m_track->SetBackgroundColor(Color(104,104,104));
		m_track->SetBorderVisible(false);
		m_track->SetTheme( RED_NAME( RedGuiDefaultTheme ) );
		m_track->SetNeedKeyFocus( false );
		AddChild(m_track);

		UpdateTrack();
	}

	CRedGuiScrollBar::~CRedGuiScrollBar()
	{
		
	}

	void CRedGuiScrollBar::OnPendingDestruction()
	{
		m_start->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_start->EventMouseWheel.Unbind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_end->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_end->EventMouseWheel.Unbind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_track->EventMouseDrag.Unbind(this, &CRedGuiScrollBar::NotifyMouseDrag);
		m_track->EventMouseWheel.Unbind(this, &CRedGuiScrollBar::NotifyMouseWheel);
		m_track->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollBar::NotifyMouseButtonPressed);
		m_track->EventMouseButtonReleased.Unbind(this, &CRedGuiScrollBar::NotifyMouseButtonReleased);
	}

	void CRedGuiScrollBar::SetVerticalAlignment(Bool value)
	{
		m_verticalAlignment = value;
		UpdateTrack();
	}

	Bool CRedGuiScrollBar::GetVerticalAlignment() const
	{
		return m_verticalAlignment;
	}

	void CRedGuiScrollBar::SetScrollRange(Uint32 value)
	{
		if(m_scrollRange == value)
		{
			return;
		}

		m_scrollRange = value;
		m_scrollPosition = (m_scrollPosition < m_scrollRange) ? m_scrollPosition : 0;
		UpdateTrack();
	}

	Uint32 CRedGuiScrollBar::GetScrollRange() const
	{
		return m_scrollRange;
	}

	void CRedGuiScrollBar::SetScrollPosition( Int32 value )
	{
		if( value >= (Int32)m_scrollRange )
		{
			value = m_scrollRange;
		}
		if( value < 0 )
		{
			value = 0;
		}

		if( (Int32)m_scrollPosition == value)
		{
			return;
		}

		m_scrollPosition = value;
		UpdateTrack();

		// send event about the change
		EventScrollChangePosition(this, m_scrollPosition);
	}

	Uint32 CRedGuiScrollBar::GetScrollPosition() const
	{
		return m_scrollPosition;
	}

	void CRedGuiScrollBar::SetScrollPage(Uint32 value)
	{
		m_scrollPage = value;
	}

	Uint32 CRedGuiScrollBar::GetScrollPage() const
	{
		return m_scrollPage;
	}

	Int32 CRedGuiScrollBar::GetLineSize() const
	{
		return GetTrackPlaceLength() - (m_rangeStart + m_rangeEnd);
	}

	void CRedGuiScrollBar::SetTrackSize(Int32 value)
	{
		if(m_track != nullptr)
		{
			if(m_verticalAlignment == true)
			{
				m_track->SetSize(m_track->GetWidth(), (value < m_minTrackSize) ? m_minTrackSize : value);
			}
			else
			{
				m_track->SetSize( (value < m_minTrackSize) ? m_minTrackSize : value, m_track->GetHeight());
			}
		}

		UpdateTrack();
	}

	Int32 CRedGuiScrollBar::GetTrackSize() const
	{
		if(m_track != nullptr)
		{
			if(m_verticalAlignment == true)
			{
				return m_track->GetHeight();
			}
			else
			{
				return m_track->GetWidth();
			}
		}

		return 1;
	}

	void CRedGuiScrollBar::SetMinTrackSize(Int32 value)
	{
		m_minTrackSize = value;
	}

	Int32 CRedGuiScrollBar::GetMinTrackSize() const
	{
		return m_minTrackSize;
	}

	void CRedGuiScrollBar::SetMoveToClick(Bool value)
	{
		m_moveToClick = value;
	}

	Bool CRedGuiScrollBar::GetClickToMove() const
	{
		return m_moveToClick;
	}

	void CRedGuiScrollBar::SetPosition( const Vector2& position)
	{
		CRedGuiControl::SetPosition(position);
	}

	void CRedGuiScrollBar::SetPosition(Int32 left, Int32 top)
	{
		CRedGuiControl::SetPosition(left, top);
	}

	void CRedGuiScrollBar::SetSize( const Vector2& size)
	{
		CRedGuiControl::SetSize(size);
		UpdateTrack();
	}

	void CRedGuiScrollBar::SetSize(Int32 width, Int32 height)
	{
		CRedGuiControl::SetSize(width, height);
		UpdateTrack();
	}

	void CRedGuiScrollBar::SetCoord(const Box2& coord)
	{
		CRedGuiControl::SetCoord(coord);
		UpdateTrack();
	}

	void CRedGuiScrollBar::SetCoord(Int32 left, Int32 top, Int32 width, Int32 height)
	{
		CRedGuiControl::SetCoord(left, top, width, height);
		UpdateTrack();
	}

	void CRedGuiScrollBar::Draw()
	{
		GetTheme()->DrawPanel(this);
	}

	void CRedGuiScrollBar::UpdateTrack()
	{
		if(m_track == nullptr)
		{
			return;
		}

		ForcePick(m_track);

		// size in the pixels
		Int32 position = GetLineSize();

		if(m_verticalAlignment == true)
		{
			if( (m_scrollRange < 2) || (position <= m_track->GetHeight()) )
			{
				m_track->SetVisible(false);
				return;
			}

			// show track button
			if(m_track->GetVisible() == false)
			{
				m_track->SetVisible(true);
			}

			// calculate new position for track
			position = ( ( position - GetTrackSize() ) * m_scrollPosition ) / (m_scrollRange - 1) + m_rangeStart;
			m_track->SetPosition(m_track->GetLeft(), position);
		}
		else
		{
			if( (m_scrollRange < 2) ||(position <= m_track->GetWidth()) )
			{
				m_track->SetVisible(false);
				return;
			}

			// show track button
			if(m_track->GetVisible() == false)
			{
				m_track->SetVisible(true);
			}

			// calculate new position for track
			position = ( ( position - GetTrackSize() ) * m_scrollPosition ) / (m_scrollRange - 1) + m_rangeStart;
			m_track->SetPosition(position, m_track->GetTop());
		}
	}

	void CRedGuiScrollBar::TrackMove( Int32 left, Int32 top, const Vector2& lastClickedPoint )
	{
		if(m_track == nullptr)
		{
			return;
		}

		if(m_verticalAlignment == true)
		{
			Int32 start = (Int32)m_preActionOffset.Y + (top - (Int32)lastClickedPoint.Y);
			if(start < (Int32)m_rangeStart)
			{
				start = m_rangeStart;
			}
			else if(start > ( GetTrackPlaceLength() - (Int32)m_rangeEnd - m_track->GetHeight() ))
			{
				start = GetTrackPlaceLength() - (Int32)m_rangeEnd - m_track->GetHeight();
			}
			if(m_track->GetTop() != start)
			{
				m_track->SetPosition(m_track->GetLeft(), start);
			}

			Int32 position = start - (Int32)m_rangeStart + (GetLineSize() - GetTrackSize()) / ( ((Int32)m_scrollRange - 1) * 2);
			position = position * ((Int32)m_scrollRange - 1) / (GetLineSize() - GetTrackSize());

			if(position < 0)
			{
				position = 0;
			}
			else if(position >= (Int32)m_scrollRange)
			{
				position = (Int32)m_scrollRange - 1;
			}
			if(position == (Int32)m_scrollPosition)
			{
				return;
			}

			m_scrollPosition = position;
		}
		else
		{
			Int32 start = (Int32)m_preActionOffset.X + (left - (Int32)lastClickedPoint.X);
			if(start < (Int32)m_rangeStart)
			{
				start = m_rangeStart;
			}
			else if(start > (GetTrackPlaceLength() - (Int32)m_rangeEnd - m_track->GetWidth()))
			{
				start = GetTrackPlaceLength() - m_rangeEnd - m_track->GetWidth();
			}
			if(m_track->GetLeft() != start)
			{
				m_track->SetPosition(start, m_track->GetTop());
			}

			Int32 position = start - m_rangeStart + (GetLineSize() - GetTrackSize()) / ( (m_scrollRange - 1) * 2);
			position = position * (m_scrollRange - 1) / (GetLineSize() - GetTrackSize());

			if(position < 0)
			{
				position = 0;
			}
			else if(position >= (Int32)m_scrollRange)
			{
				position = m_scrollRange -1;
			}
			if(position == (Int32)m_scrollPosition)
			{
				return;
			}

			m_scrollPosition = position;
		}

		UpdateTrack();

		// send event about the change
		EventScrollChangePosition(this, m_scrollPosition);
	}

	void CRedGuiScrollBar::OnMouseWheel(Int32 delta)
	{
		RedGui::CRedGuiEventPackage eventPackage( nullptr );
		NotifyMouseWheel( eventPackage, delta);
		CRedGuiControl::OnMouseWheel(delta);
	}

	void CRedGuiScrollBar::NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		RED_UNUSED( eventPackage );

		if(button == MB_Left)
		{
			const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition( MB_Left );
			TrackMove( (Int32)mousePosition.X, (Int32)mousePosition.Y, point );
		}
	}

	void CRedGuiScrollBar::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta)
	{
		RED_UNUSED( eventPackage );

		if(m_scrollRange < 2)
		{
			return;
		}

		Int32 offset = m_scrollPosition;
		if(delta < 0)
		{
			offset += GScrollMouseWheelCount;
		}
		else
		{
			offset -= GScrollMouseWheelCount;
		}

		if(offset < 0)
		{
			offset = 0;
		}
		else if(offset > (Int32)(m_scrollRange - 1))
		{
			offset = m_scrollRange - 1;
		}

		if(offset != (Int32)m_scrollPosition)
		{
			m_scrollPosition = offset;

			EventScrollChangePosition(this, m_scrollPosition);
			UpdateTrack();
		}
	}

	void CRedGuiScrollBar::NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		EventMouseButtonPressed(this, mousePosition, button);

		if(button != MB_Left)
		{
			return;
		}

		if(m_moveToClick == true && sender != m_track && sender != m_start && sender != m_end)
		{
			if(m_track != nullptr)
			{
				m_preActionOffset = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition(MB_Left);
				const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetMousePosition() - m_track->GetParent()->GetAbsolutePosition();

				m_preActionOffset.X -= GetTrackSize()/2;
				m_preActionOffset.Y -= GetTrackSize()/2;

				const Vector2& lastClickedPoint = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition( MB_Left );
				TrackMove( (Int32)point.X, (Int32)point.Y, lastClickedPoint );
			}
		}
		else if(sender == m_start)
		{
			if(m_scrollPosition == 0)
			{
				return;
			}

			MovePageUp();
		}
		else if(sender == m_end)
		{
			if( (m_scrollRange < 2) || (m_scrollPosition > (m_scrollRange - 1)) )
			{
				return;
			}

			MovePageDown();
		}
		else if(sender == m_track)
		{
			m_preActionOffset = sender->GetPosition();
		}
	}

	void CRedGuiScrollBar::NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		RED_UNUSED( eventPackage );

		UpdateTrack();
	}

	Int32 CRedGuiScrollBar::GetTrackPlaceLength() const
	{
		if( m_track != nullptr && m_track->GetParent() != nullptr )
		{
			if(m_verticalAlignment == true)
			{
				return m_track->GetParent()->GetHeight();
			}
			else
			{
				return m_track->GetParent()->GetWidth();
			}
		}

		return 0;
	}

	void CRedGuiScrollBar::OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button )
	{
		if( button != MB_Left )
		{
			return;
		}

		if( GRedGui::GetInstance().GetInputManager()->IsShiftPressed() == true )
		{
			m_preActionOffset = m_track->GetPosition();
			const Vector2 point = Vector2( m_track->GetAbsolutePosition().X, ( m_track->GetAbsolutePosition().Y + ( m_track->GetSize().Y / 2 ) ) );
			TrackMove( (Int32)mousePosition.X, (Int32)mousePosition.Y, point );
		}
		else
		{
			Int32 delta = 0;
			if( mousePosition.Y < m_track->GetAbsolutePosition().Y )
			{
				delta = 1;
			}
			if( mousePosition.Y > ( m_track->GetAbsolutePosition().Y + m_track->GetSize().Y ) )
			{
				delta = -1;
			}
			RedGui::CRedGuiEventPackage eventPackage( this );
			NotifyMouseWheel( eventPackage, delta );
		}
	}

	void CRedGuiScrollBar::MoveScrollToStartSide()
	{
		RedGui::CRedGuiEventPackage eventPackage( this );
		NotifyMouseWheel( eventPackage, -1 );
	}

	void CRedGuiScrollBar::MoveScrollToEndSide()
	{
		RedGui::CRedGuiEventPackage eventPackage( this );
		NotifyMouseWheel( eventPackage, 1 );
	}

	void CRedGuiScrollBar::MoveScrollToStart()
	{
		m_scrollPosition = 0;
		EventScrollChangePosition(this, m_scrollPosition);
		UpdateTrack();
	}

	void CRedGuiScrollBar::MoveScrollToEnd()
	{
		m_scrollPosition = m_scrollRange - 1;
		EventScrollChangePosition(this, m_scrollPosition);
		UpdateTrack();
	}

	void CRedGuiScrollBar::MovePageDown()
	{
		if( (m_scrollPosition + m_scrollPage) < (m_scrollRange -1) )
		{
			m_scrollPosition += m_scrollPage;
		}
		else
		{
			m_scrollPosition = m_scrollRange -1;
		}

		EventScrollChangePosition(this, m_scrollPosition);
		UpdateTrack();
	}

	void CRedGuiScrollBar::MovePageUp()
	{
		if(m_scrollPosition > m_scrollPage)
		{
			m_scrollPosition -= m_scrollPage;
		}
		else
		{
			m_scrollPosition = 0;
		}

		EventScrollChangePosition(this, m_scrollPosition);
		UpdateTrack();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
