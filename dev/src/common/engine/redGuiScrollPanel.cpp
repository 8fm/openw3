/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiScrollBar.h"
#include "redGuiScrollPanel.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GScrollPageSize	= 16;
		const Uint32 GScrollWheelValue	= 50;
	}

	CRedGuiScrollPanel::CRedGuiScrollPanel(Uint32 left, Uint32 top, Uint32 width, Uint32 height)
		: CRedGuiControl(left, top, width, height)
		, m_realClient(nullptr)
		, m_croppClient(nullptr)
		, m_vScroll(nullptr)
		, m_hScroll(nullptr)
		, m_visibleVScroll(false)
		, m_visibleHScroll(false)
		, m_vRange(0)
		, m_hRange(0)
	{
		SetNeedKeyFocus( false );
		SetBorderVisible(true);
		SetBackgroundColor(Color::CLEAR);
		SetForegroundColor(Color::CLEAR);

		const Int32 w = Clamp( GetWidth(), 0, INT_MAX );
		const Int32 h = Clamp( GetHeight(), 0, INT_MAX );
		m_vScroll = new CRedGuiScrollBar(w, 0, 20, h);
		m_vScroll->EventScrollChangePosition.Bind(this, &CRedGuiScrollPanel::NotifyScrollChangePosition);
		m_vScroll->SetMinTrackSize(20);
		AddChild(m_vScroll);
		m_vScroll->SetDock(DOCK_Right);
		m_vScroll->SetVisible(false);

		m_hScroll = new CRedGuiScrollBar(0, h, w, 20, false);
		m_hScroll->EventScrollChangePosition.Bind(this, &CRedGuiScrollPanel::NotifyScrollChangePosition);
		m_hScroll->SetMinTrackSize(20);
		AddChild(m_hScroll);
		m_hScroll->SetDock(DOCK_Bottom);
		m_hScroll->SetVisible(false);

		// create a canvas
		m_croppClient = new CRedGuiPanel(0, 0, w, h);
		m_croppClient->SetBorderVisible(false);
		m_croppClient->SetBackgroundColor(Color::CLEAR);
		m_croppClient->SetForegroundColor(Color::CLEAR);
		m_croppClient->EventMouseWheel.Bind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		m_croppClient->EventSizeChanged.Bind(this, &CRedGuiScrollPanel::NotifyCroppRectEventSizeChanged);
		m_croppClient->EventSizeChanged.Bind(this, &CRedGuiScrollPanel::NotifyEventSizeChanged);
		m_croppClient->EventKeyButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyKeyButtonPressed);
		m_croppClient->EventKeyButtonReleased.Bind(this, &CRedGuiScrollPanel::NotifyKeyButtonReleased);
		m_croppClient->EventMouseButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyMousePressed);
		m_croppClient->EventKeyLostFocus.Bind(this, &CRedGuiScrollPanel::NotifyKeyLostFocus);
		m_croppClient->EventMouseLostFocus.Bind(this, &CRedGuiScrollPanel::NotifyMouseLostFocus);
		m_croppClient->EventKeySetFocus.Bind(this, &CRedGuiScrollPanel::NotifyKeySetFocus);
		m_croppClient->EventMouseSetFocus.Bind(this, &CRedGuiScrollPanel::NotifyMouseSetFocus);
		m_croppClient->EventMouseButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyMouseButtonPressed);
		m_croppClient->EventMouseButtonReleased.Bind(this, &CRedGuiScrollPanel::NotifyMouseButtonReleased);
		SetControlClient(m_croppClient);
		m_croppClient->SetDock(DOCK_Fill);

		// create a canvas, the real owner of the children
		m_realClient = new CRedGuiPanel( 0, 0, 0, 0 );
		m_realClient->SetAutoSize(true);
		m_realClient->SetBackgroundColor(Color::CLEAR);
		m_realClient->SetForegroundColor(Color::CLEAR);
		m_realClient->SetBorderVisible(false);
		m_realClient->SetAnchor(ANCHOR_None);
		m_realClient->EventMouseWheel.Bind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		m_realClient->EventSizeChanged.Bind(this, &CRedGuiScrollPanel::NotifyEventSizeChanged);
		m_realClient->EventKeyButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyKeyButtonPressed);
		m_realClient->EventKeyButtonReleased.Bind(this, &CRedGuiScrollPanel::NotifyKeyButtonReleased);
		m_realClient->EventMouseButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyMousePressed);
		m_realClient->EventKeyLostFocus.Bind(this, &CRedGuiScrollPanel::NotifyKeyLostFocus);
		m_realClient->EventMouseLostFocus.Bind(this, &CRedGuiScrollPanel::NotifyMouseLostFocus);
		m_realClient->EventKeySetFocus.Bind(this, &CRedGuiScrollPanel::NotifyKeySetFocus);
		m_realClient->EventMouseSetFocus.Bind(this, &CRedGuiScrollPanel::NotifyMouseSetFocus);
		m_realClient->EventMouseButtonPressed.Bind(this, &CRedGuiScrollPanel::NotifyMouseButtonPressed);
		m_realClient->EventMouseButtonReleased.Bind(this, &CRedGuiScrollPanel::NotifyMouseButtonReleased);
		m_croppClient->SetControlClient(m_realClient);

		UpdateView();
	}

	CRedGuiScrollPanel::~CRedGuiScrollPanel()
	{
	}

	void CRedGuiScrollPanel::OnPendingDestruction()
	{
		m_vScroll->EventScrollChangePosition.Unbind(this, &CRedGuiScrollPanel::NotifyScrollChangePosition);
		m_hScroll->EventScrollChangePosition.Unbind(this, &CRedGuiScrollPanel::NotifyScrollChangePosition);
		m_croppClient->EventMouseWheel.Unbind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		m_croppClient->EventSizeChanged.Unbind(this, &CRedGuiScrollPanel::NotifyCroppRectEventSizeChanged);
		m_croppClient->EventSizeChanged.Unbind(this, &CRedGuiScrollPanel::NotifyEventSizeChanged);
		m_croppClient->EventKeyButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyKeyButtonPressed);
		m_croppClient->EventKeyButtonReleased.Unbind(this, &CRedGuiScrollPanel::NotifyKeyButtonReleased);
		m_croppClient->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyMousePressed);
		m_croppClient->EventKeyLostFocus.Unbind(this, &CRedGuiScrollPanel::NotifyKeyLostFocus);
		m_croppClient->EventMouseLostFocus.Unbind(this, &CRedGuiScrollPanel::NotifyMouseLostFocus);
		m_croppClient->EventKeySetFocus.Unbind(this, &CRedGuiScrollPanel::NotifyKeySetFocus);
		m_croppClient->EventMouseSetFocus.Unbind(this, &CRedGuiScrollPanel::NotifyMouseSetFocus);
		m_croppClient->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyMouseButtonPressed);
		m_croppClient->EventMouseButtonReleased.Unbind(this, &CRedGuiScrollPanel::NotifyMouseButtonReleased);
		m_realClient->EventMouseWheel.Unbind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		m_realClient->EventSizeChanged.Unbind(this, &CRedGuiScrollPanel::NotifyEventSizeChanged);
		m_realClient->EventKeyButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyKeyButtonPressed);
		m_realClient->EventKeyButtonReleased.Unbind(this, &CRedGuiScrollPanel::NotifyKeyButtonReleased);
		m_realClient->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyMousePressed);
		m_realClient->EventKeyLostFocus.Unbind(this, &CRedGuiScrollPanel::NotifyKeyLostFocus);
		m_realClient->EventMouseLostFocus.Unbind(this, &CRedGuiScrollPanel::NotifyMouseLostFocus);
		m_realClient->EventKeySetFocus.Unbind(this, &CRedGuiScrollPanel::NotifyKeySetFocus);
		m_realClient->EventMouseSetFocus.Unbind(this, &CRedGuiScrollPanel::NotifyMouseSetFocus);
		m_realClient->EventMouseButtonPressed.Unbind(this, &CRedGuiScrollPanel::NotifyMouseButtonPressed);
		m_realClient->EventMouseButtonReleased.Unbind(this, &CRedGuiScrollPanel::NotifyMouseButtonReleased);
	}
	

	void CRedGuiScrollPanel::SetSize( const Vector2& size)
	{
		CRedGuiControl::SetSize(size);
		if(m_croppClient != nullptr)
		{
			m_croppClient->SetCoord(Box2(Vector2(0,0), size));
			m_croppClient->SetOriginalRect(Box2(Vector2(0,0), size));
		}
	}

	void CRedGuiScrollPanel::SetSize(Int32 width, Int32 height)
	{
		CRedGuiControl::SetSize(width, height);
		if(m_croppClient != nullptr)
		{
			m_croppClient->SetCoord(Box2(Vector2(0,0), Vector2((Float)width, (Float)height)));
			m_croppClient->SetOriginalRect(Box2(Vector2(0,0), Vector2((Float)width, (Float)height)));
		}
	}

	void CRedGuiScrollPanel::SetCoord(const Box2& coord)
	{
		CRedGuiControl::SetCoord(coord);
		if(m_croppClient != nullptr)
		{
			Box2 tempBox(Vector2(0,0), coord.Max);
			m_croppClient->SetCoord(tempBox);
			m_croppClient->SetOriginalRect(tempBox);
		}
	}

	void CRedGuiScrollPanel::SetCoord(Int32 left, Int32 top, Int32 width, Int32 height)
	{
		CRedGuiControl::SetCoord(left, top, width, height);
		if(m_croppClient != nullptr)
		{
			Box2 tempBox(Vector2(0,0), Vector2((Float)width, (Float)height));
			m_croppClient->SetCoord(tempBox);
			m_croppClient->SetOriginalRect(tempBox);
		}
	}

	void CRedGuiScrollPanel::SetOutOfDate()
	{
		CRedGuiControl::SetOutOfDate();

		if(m_croppClient != nullptr)
		{
			m_croppClient->SetOutOfDate();
		}
		if(m_realClient != nullptr)
		{
			m_realClient->SetOutOfDate();
		}
	}

	void CRedGuiScrollPanel::SetVisibleVScroll(Bool value)
	{
		m_visibleVScroll = value;
		UpdateView();
	}

	Bool CRedGuiScrollPanel::IsVisibleVScroll() const
	{
		return m_visibleVScroll;
	}

	void CRedGuiScrollPanel::SetVisibleHScroll(Bool value)
	{
		m_visibleHScroll = value;
		UpdateView();
	}

	Bool CRedGuiScrollPanel::IsVisibleHScroll() const
	{
		return m_visibleHScroll;
	}

	Box2 CRedGuiScrollPanel::GetViewCoord() const
	{
		return GetCoord();
	}

	void CRedGuiScrollPanel::ResetScrollPosition()
	{
		if(m_vScroll != nullptr)
		{
			m_vScroll->SetScrollPosition(0);
		}
		if(m_hScroll != nullptr)
		{
			m_hScroll->SetScrollPosition(0);
		}
		m_realClient->SetPosition(0,0);
	}

	void CRedGuiScrollPanel::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiScrollPanel::AddChild(CRedGuiControl* child)
	{
		child->EventMouseWheel.Bind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		CRedGuiControl::AddChild(child);
		if(m_realClient != nullptr)
		{
			m_realClient->EventSizeChanged.Bind(child, &CRedGuiControl::NotifyEventParentSizeChange);
			child->SetCroppedParent(m_croppClient);
		}
		UpdateView();
	}

	void CRedGuiScrollPanel::RemoveChild(CRedGuiControl* child)
	{
		child->EventMouseWheel.Unbind(this, &CRedGuiScrollPanel::NotifyMouseWheel);
		if(m_realClient != nullptr)
		{
			m_realClient->EventSizeChanged.Unbind(child, &CRedGuiControl::NotifyEventParentSizeChange);
		}
		CRedGuiControl::RemoveChild(child);
		UpdateView();
	}

	void CRedGuiScrollPanel::NotifyMousePressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button)
	{
		RED_UNUSED( eventPackage );

		EventMouseButtonPressed(this, mousePosition, button);
	}

	void CRedGuiScrollPanel::NotifyMouseReleased(CRedGuiControl* sender, const Vector2& mousePosition, enum EMouseButton button)
	{
		EventMouseButtonReleased(this, mousePosition, button);
	}

	void CRedGuiScrollPanel::NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 position)
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if(m_realClient == nullptr)
		{
			return;
		}

		Vector2 realClientPosition = (m_realClient != nullptr) ? m_realClient->GetAbsolutePosition() : Vector2(0,0);
		if(sender == m_vScroll)
		{
			Vector2 point = m_realClient->GetPosition();
			point.Y = -(Float)position;
			m_realClient->SetPosition(point);
		}
		else if(sender == m_hScroll)
		{
			Vector2 point = m_realClient->GetPosition();
			point.X = -(Float)position;
			m_realClient->SetPosition(point);
		}

		// send event
		if(realClientPosition != m_realClient->GetAbsolutePosition())
		{
			EventMoveDelta(this, m_realClient->GetAbsolutePosition()-realClientPosition );
		}
	}

	void CRedGuiScrollPanel::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta)
	{
		RED_UNUSED( eventPackage );

		if(m_realClient == nullptr)
		{
			return;
		}

		Vector2 realClientPosition = (m_realClient != nullptr) ? m_realClient->GetAbsolutePosition() : Vector2(0,0);
		if(m_vRange != 0)
		{
			Vector2 position = m_realClient->GetPosition();
			Int32 offset = -(Int32)position.Y;

			if(delta < 0)
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if(offset < 0)
			{
				offset = 0;
			}
			else if(offset > (Int32)m_vRange)
			{
				offset = m_vRange;
			}

			if(offset != position.Y)
			{
				position.Y = -(Float)offset;
				if(m_vScroll != nullptr)
				{
					m_vScroll->SetScrollPosition(offset);
				}
				m_realClient->SetPosition(position);
			}
		}
		else if(m_hRange != 0)
		{
			Vector2 position = m_realClient->GetPosition();
			Int32 offset = -(Int32)position.X;

			if(delta < 0)
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if(offset < 0)
			{
				offset = 0;
			}
			else if(offset > (Int32)m_hRange)
			{
				offset = m_hRange;
			}

			if(offset != position.X)
			{
				position.X = -(Float)offset;
				if(m_hScroll != nullptr)
				{
					m_hScroll->SetScrollPosition(offset);
				}
				m_realClient->SetPosition(position);
			}
		}

		// send event
		if(realClientPosition != m_realClient->GetAbsolutePosition())
		{
			EventMoveDelta(this, m_realClient->GetAbsolutePosition()-realClientPosition );
		}
	}

	void CRedGuiScrollPanel::UpdateView()
	{
		UpdateScrollSize();
		UpdateScrollPosition();
	}

	void CRedGuiScrollPanel::UpdateControl()
	{
		CRedGuiControl::UpdateControl();

		if(GetOutOfDate() == true)
		{
			UpdateView();
		}
	}

	void CRedGuiScrollPanel::EraseContent()
	{
		if(m_hScroll != nullptr)
		{
			m_hScroll->SetVisible(false);
		}
		if(m_vScroll != nullptr)
		{
			m_vScroll->SetVisible(false);
		}
	}

	Vector2 CRedGuiScrollPanel::GetContentSize()
	{
		if(m_realClient != nullptr)
		{
			return m_realClient->GetSize();
		}

		return Vector2(0.0f, 0.0f);
	}

	Vector2 CRedGuiScrollPanel::GetContentPosition()
	{
		if(m_realClient != nullptr)
		{
			return m_realClient->GetPosition();
		}

		return Vector2(0.0f, 0.0f);
	}

	Vector2 CRedGuiScrollPanel::GetViewSize()
	{
		if(m_croppClient != nullptr)
		{
			return m_croppClient->GetSize();
		}

		return Vector2(0.0f, 0.0f);
	}

	void CRedGuiScrollPanel::SetContentPosition( const Vector2& value)
	{
		if(m_realClient != nullptr)
		{
			m_realClient->SetPosition(value);
		}
	}

	Uint32 CRedGuiScrollPanel::GetVScrollPage()
	{
		return GScrollPageSize;
	}

	Uint32 CRedGuiScrollPanel::GetHScrollPage()
	{
		return GScrollPageSize;
	}

	void CRedGuiScrollPanel::NotifyEventSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldsize, Vector2 newSize )
	{
		RED_UNUSED( eventPackage );

		UpdateView();
	}

	void CRedGuiScrollPanel::NotifyCroppRectEventSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldsize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		m_realClient->SetOriginalRect(m_croppClient->GetCoord());
		m_realClient->SetOutOfDate();
	}

	void CRedGuiScrollPanel::UpdateScrollSize()
	{
		if(m_croppClient == nullptr)
		{
			return;
		}

		Vector2 viewSize = GetViewSize();
		Vector2 contentSize = GetContentSize();

		// horizontal content doesn't fit
		if(contentSize.Y > viewSize.Y)
		{
			if(m_vScroll != nullptr)
			{
				if(m_vScroll->GetVisible() == false || m_visibleVScroll == true)
				{
					m_vScroll->SetVisible(true);

					if(m_hScroll != nullptr)
					{
						// recalculate horizontal bar after add vertical bar
						if( ( (contentSize.X > viewSize.X) && (m_hScroll->GetVisible() == false) ) || (m_visibleHScroll == true) )
						{
							m_hScroll->SetVisible(true);
						}
					}
				}
			}
		}
		else
		{
			if(m_vScroll != nullptr)
			{
				if(m_vScroll->GetVisible() == true)
				{
					m_vScroll->SetVisible(false);

					if(m_hScroll != nullptr)
					{
						// recalculate horizontal bar after remove vertical bar
						if( (contentSize.X <= viewSize.X) && (m_hScroll->GetVisible() == true) )
						{
							m_hScroll->SetVisible(false);
						}
					}
				}
			}
		}

		// vertical content doesn't fit
		if(contentSize.X > viewSize.X)
		{
			if(m_hScroll != nullptr)
			{
				if(m_hScroll->GetVisible() == false || m_visibleHScroll == true)
				{
					m_hScroll->SetVisible(true);

					if(m_vScroll != nullptr)
					{
						// recalculate vertical bar after add horizontal bar
						if( ( (contentSize.Y > viewSize.Y) && (m_vScroll->GetVisible() == false) ) || m_visibleVScroll == true )
						{
							m_vScroll->SetVisible(true);
						}
					}
				}
			}
		}
		else
		{
			if(m_hScroll != nullptr)
			{
				if(m_hScroll->GetVisible() == true)
				{
					m_hScroll->SetVisible(false);

					if(m_vScroll != nullptr)
					{
						// recalculate vertical bar after remove horizontal bar
						if( (contentSize.Y <= viewSize.Y) && (m_vScroll->GetVisible() == true) )
						{
							m_vScroll->SetVisible(false);
						}
					}
				}
			}
		}

		// calculate ranges
		m_vRange = (viewSize.Y >= contentSize.Y) ? 0 : (Uint32)(contentSize.Y - viewSize.Y);
		m_hRange = (viewSize.X >= contentSize.X) ? 0 : (Uint32)(contentSize.X - viewSize.X);

		// set new values
		if(m_vScroll != nullptr)
		{
			Uint32 page = GetVScrollPage();
			m_vScroll->SetScrollPage(page);
			m_vScroll->SetScrollRange(m_vRange + 1);
			if(contentSize.Y > 0)
			{
				m_vScroll->SetTrackSize((Int32)( (Float)(m_vScroll->GetLineSize() * viewSize.Y) / (Float)(contentSize.Y) ));
			}
		}
		if(m_hScroll != nullptr)
		{
			Uint32 page = GetHScrollPage();
			m_hScroll->SetScrollPage(page);
			m_hScroll->SetScrollRange(m_hRange + 1);
			if(contentSize.X > 0)
			{
				m_hScroll->SetTrackSize((Int32)( (Float)(m_hScroll->GetLineSize() * viewSize.X) / (Float)(contentSize.X) ));
			}
		}
	}

	void CRedGuiScrollPanel::UpdateScrollPosition()
	{
		if(m_realClient == nullptr)
		{
			return;
		}

		if(m_vRange != 0)
		{
			Vector2 position = m_realClient->GetPosition();
			Int32 offset = -(Int32)position.Y;

			if(offset < 0)
			{
				offset = 0;
			}
			else if(offset > (Int32)m_vRange)
			{
				offset = m_vRange;
			}

			if(offset != position.Y)
			{
				position.Y = -(Float)offset;
				if(m_vScroll != nullptr)
				{
					m_vScroll->SetScrollPosition(offset);
				}
				m_realClient->SetPosition(position);
			}
		}
		if(m_hRange != 0)
		{
			Vector2 position = m_realClient->GetPosition();
			Int32 offset = -(Int32)position.X;

			if(offset < 0)
			{
				offset = 0;
			}
			else if(offset > (Int32)m_hRange)
			{
				offset = m_hRange;
			}

			if(offset != position.X)
			{
				position.X = -(Float)offset;
				if(m_hScroll != nullptr)
				{
					m_hScroll->SetScrollPosition(offset);
				}
				m_realClient->SetPosition(position);
			}
		}
	}

	void CRedGuiScrollPanel::ScrollToDown()
	{
		// TODO
	}

	void CRedGuiScrollPanel::NotifyKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text )
	{
		RED_UNUSED( eventPackage );

		EventKeyButtonPressed(this, key, text);
	}

	void CRedGuiScrollPanel::SetPointer( EMousePointer pointer )
	{
		m_croppClient->SetPointer(pointer);
		m_realClient->SetPointer(pointer);
	}

	void CRedGuiScrollPanel::NotifyKeyButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key )
	{
		RED_UNUSED( eventPackage );

		EventKeyButtonReleased(this, key);
	}

	void CRedGuiScrollPanel::NotifyKeyLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocused )
	{
		RED_UNUSED( eventPackage );

		EventKeyLostFocus(this, newFocused);
	}

	void CRedGuiScrollPanel::NotifyMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocused )
	{
		RED_UNUSED( eventPackage );

		EventMouseLostFocus(this, newFocused);
	}

	void CRedGuiScrollPanel::NotifyKeySetFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* oldFocused )
	{
		RED_UNUSED( eventPackage );

		EventKeySetFocus(this, oldFocused);
	}

	void CRedGuiScrollPanel::NotifyMouseSetFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* oldFocused )
	{
		RED_UNUSED( eventPackage );

		EventMouseSetFocus(this, oldFocused);
	}

	void CRedGuiScrollPanel::Move( const Vector2& delta )
	{
		m_hScroll->SetScrollPosition( (Uint32)( m_hScroll->GetScrollPosition() + delta.X ) );
		m_vScroll->SetScrollPosition( (Uint32)( m_vScroll->GetScrollPosition() + delta.Y ) );
	}

	void CRedGuiScrollPanel::NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		EventMouseButtonPressed(this, mousePosition, button);
	}

	void CRedGuiScrollPanel::NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		EventMouseButtonReleased(this, mousePosition, button);
	}

	Bool CRedGuiScrollPanel::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Move )
		{
			this->Move( data );
			return true;
		}

		return false;
	}

} // namespace RedGui

#endif	// NO_RED_GUI
