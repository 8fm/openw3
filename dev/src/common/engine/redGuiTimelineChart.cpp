/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiScrollBar.h"
#include "redGuiPanel.h"
#include "redGuiManager.h"
#include "redGuiTimeline.h"
#include "redGuiTimelineChart.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultTimelineHeight = 25;
		const Uint32 GScrollPageSize = 16;
		const Uint32 GScrollWheelValue = 50;
		const Color GHighlight( 255,170, 0, 100 );
		const Color GPushed( 255, 150, 0, 100 );
		const Color GNormal( 45,45,45,255 );
		const Color GBorder( 112, 112, 112, 255 );
	}

	CRedGuiTimelineChart::CRedGuiTimelineChart( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height )
		, m_autoScroll( true )
		, m_pause( false )
		, m_frameZoom( 1 )
	{
		SetNeedKeyFocus( true );

		GRedGui::GetInstance().EventTick.Bind( this, &CRedGuiTimelineChart::NotifyOnTick );

		CreateControls();
	}

	CRedGuiTimelineChart::~CRedGuiTimelineChart()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CRedGuiTimelineChart::NotifyOnTick );
	}

	void CRedGuiTimelineChart::Draw()
	{
		GetTheme()->DrawPanel( this );

		// draw timeline items
		if( m_firstVisibleItem != -1 && m_lastVisibleItem != -1 )
		{
			DrawTimelines();
			if( m_labelsPanel->GetVisible() == true )
			{
				DrawTimelinesLabels();
			}
		}
	}

	void CRedGuiTimelineChart::DrawTimelines()
	{
		GetTheme()->SetCroppedParent( m_croppClient );

		Box2 padding = m_croppClient->GetPadding();
		Uint32 rowWidth = (Uint32)( m_croppClient->GetWidth() - ( padding.Min.X + padding.Max.X ) );
		Float x = m_croppClient->GetAbsoluteLeft() + padding.Min.X + m_firstItemPosition.X;
		Vector2 size( Vector2( (Float)m_maxItemWidth, (Float)GDefaultTimelineHeight ) );

		if( !m_pause )
		{
			m_nowTime = EngineTime::GetNow();
			if( !m_timelines.Empty() && m_timelines[ 0 ]->GetEventCount() >= m_frameZoom )
			{
				Uint32 count = m_timelines[ 0 ]->GetEventCount();
				m_fromTime = m_timelines[ 0 ]->GetEvent( count - m_frameZoom )->GetStartTime();
			}
		}

		for( Uint32 i=m_firstVisibleItem; i<(Uint32)m_lastVisibleItem; ++i )
		{
			CRedGuiTimeline* timeline = m_timelines[i];

			// set position
			Float y = m_croppClient->GetAbsoluteTop() + m_firstItemPosition.Y + (Float)( i * GDefaultTimelineHeight );
			Vector2 position( x, y-1 );

			// draw text
			DrawSingleTimeline( timeline, position, size );
		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiTimelineChart::DrawTimelinesLabels()
	{
		GetTheme()->SetCroppedParent( m_labelsPanel );

		for( Uint32 i=m_firstVisibleItem; i<(Uint32)m_lastVisibleItem; ++i )
		{
			CRedGuiTimeline* timeline = m_timelines[i];

			Box2 padding = m_labelsPanel->GetPadding();
			Uint32 rowWidth = (Uint32)( m_labelsPanel->GetWidth() - ( padding.Min.X + padding.Max.X ) );
			Float x = m_labelsPanel->GetAbsoluteLeft() + padding.Min.X;
			Float y = m_labelsPanel->GetAbsoluteTop() + m_firstItemPosition.Y + (Float)( i * GDefaultTimelineHeight );

			Vector2 position( x, y );
			Vector2 size( Vector2( (Float)m_labelsPanel->GetWidth(), (Float)GDefaultTimelineHeight ) );

			// draw text
			static Uint32 tempCounter = 0;
			if( tempCounter % 2 == 0 )
			{
				GetTheme()->DrawRawFilledRectangle( position, size, Color( 30, 30, 30, 50 ) );
			}
			else
			{
				GetTheme()->DrawRawFilledRectangle( position, size, Color( 30, 30, 30, 200 ) );
			}
			++tempCounter;
			GetTheme()->DrawRawText( position + Vector2( 10.0f, 7.0f ), timeline->GetName(), Color::WHITE );
		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiTimelineChart::DrawSingleTimeline( CRedGuiTimeline* timeline, const Vector2& position, const Vector2& size )
	{
		static Uint32 tempCounter = 0;
		if( tempCounter % 2 == 0 )
		{
			GetTheme()->DrawRawFilledRectangle( position, size, Color( 30, 30, 30, 50 ) );
		}
		else
		{
			GetTheme()->DrawRawFilledRectangle( position, size, Color( 30, 30, 30, 200 ) );
		}
		++tempCounter;

		const Uint32 eventCount = timeline->GetEventCount();

		for( Int32 i=eventCount; i>0; --i )
		{
			const IRedGuiTimelineEvent* timelineEvent = timeline->GetEvent( i - 1 );
				
			Float eventStartTime = timelineEvent->GetStartTime();

			if( eventStartTime < m_fromTime )
			{
				if( !m_pause ) timeline->ClearData( i - 1 );
				return;
			}

			Float eventX = ( ( ( float ) eventStartTime - m_fromTime ) / ( ( m_nowTime - m_startTimeSeconds ) - m_fromTime ) ) * m_croppClient->GetWidth();
			Float eventWidth = timelineEvent->GetDuration() * m_croppClient->GetWidth();
			if( eventWidth < 1.0f ) eventWidth = 1.0f;

			Vector2 eventPos( eventX + m_croppClient->GetAbsoluteLeft(), position.Y + 2.0f );

			GetTheme()->DrawRawFilledRectangle( eventPos, Vector2( eventWidth, size.Y - 4.0f) , timelineEvent->GetColor() );

			if( eventWidth > GRedGui::GetInstance().GetFontManager()->GetStringSize( timelineEvent->GetName(), RGFT_Default ).X )
			{
				GetTheme()->DrawRawText( eventPos, timelineEvent->GetName(), Color::WHITE );
			}
		}
	}

	void CRedGuiTimelineChart::CreateControls()
	{
		m_labelsPanel = new CRedGuiPanel( 0, 0, 150, 200 );
		m_labelsPanel->SetDock( DOCK_Left );
		AddChild( m_labelsPanel );

		RedGui::CRedGuiPanel* mainPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 100 );
		mainPanel->SetDock( DOCK_Fill );
		AddChild( mainPanel );
		{
			// create scrollbars
			m_horizontalBar = new CRedGuiScrollBar( 0, 0, 100, 20, false );
			m_horizontalBar->EventScrollChangePosition.Bind( this, &CRedGuiTimelineChart::NotifyScrollChangePosition );
			m_horizontalBar->SetMinTrackSize( 20 );
			m_horizontalBar->SetDock( DOCK_Bottom );
			m_horizontalBar->SetVisible( false );
			mainPanel->AddChild( m_horizontalBar );

			m_verticalBar = new CRedGuiScrollBar( 0, 0, 20, 100 );
			m_verticalBar->EventScrollChangePosition.Bind( this, &CRedGuiTimelineChart::NotifyScrollChangePosition );
			m_verticalBar->SetMinTrackSize( 20 );
			m_verticalBar->SetDock( DOCK_Right );
			m_verticalBar->SetVisible( false );
			mainPanel->AddChild( m_verticalBar );


			// create cropped client
			m_croppClient = new CRedGuiPanel( 0, 0, GetWidth(), 100 );
			m_croppClient->SetBorderVisible( false );
			m_croppClient->SetBackgroundColor( Color::CLEAR );
			m_croppClient->SetForegroundColor( Color::CLEAR );
			m_croppClient->EventMouseWheel.Bind( this, &CRedGuiTimelineChart::NotifyMouseWheel );
			m_croppClient->EventSizeChanged.Bind( this, &CRedGuiTimelineChart::NotifyClientSizeChanged );
			m_croppClient->SetDock( DOCK_Fill );
			mainPanel->AddChild( m_croppClient );
		}

		m_maxItemWidth = 0;
		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultTimelineHeight );
		m_firstItemPosition = Vector2( -(Float)( m_croppClient->GetWidth() ), 0.0f );
	}

	void CRedGuiTimelineChart::SetShowTimelineLabel( Bool value )
	{
		m_labelsPanel->SetVisible( value );
	}

	Bool CRedGuiTimelineChart::GetShowTimelineLabel() const
	{
		return m_labelsPanel->GetVisible();
	}

	void CRedGuiTimelineChart::AddTimeline( const String& name )
	{
		if( FindTimeline( name ) == nullptr )
		{
			Float sizeX = Max< Float >( GRedGui::GetInstance().GetFontManager()->GetStringSize( name, RGFT_Default ).X, m_labelsPanel->GetSize().X );
			Vector2 newSize( sizeX, m_labelsPanel->GetSize().Y ); 
			m_labelsPanel->SetSize( newSize );

			m_timelines.PushBack( new CRedGuiTimeline( name ) );
			UpdateView();
		}
	}

	void CRedGuiTimelineChart::RemoveTimeline( const String& name )
	{
		Int32 timelineIndex = FindTimelineIndex( name );
		if( timelineIndex != -1 )
		{
			delete m_timelines[timelineIndex];
			m_timelines.RemoveAt( timelineIndex );
		}
	}

	void CRedGuiTimelineChart::RemoveAllTimelines()
	{
		m_timelines.ClearPtr();
		UpdateView();
	}

	Uint32 CRedGuiTimelineChart::GetTimelineCount() const
	{
		return m_timelines.Size();
	}

	Int32 CRedGuiTimelineChart::FindTimelineIndex( const String& name )
	{
		const Uint32 timelineCount = m_timelines.Size();
		for( Uint32 i=0; i<timelineCount; ++i )
		{
			if( m_timelines[i]->GetName() == name )
			{
				return i;
			}
		}
		return -1;
	}

	CRedGuiTimeline* CRedGuiTimelineChart::FindTimeline( const String& name )
	{
		const Uint32 timelineCount = m_timelines.Size();
		for( Uint32 i=0; i<timelineCount; ++i )
		{
			if( m_timelines[i]->GetName() == name )
			{
				return m_timelines[i];
			}
		}
		return nullptr;
	}

	void CRedGuiTimelineChart::NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_verticalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.Y = -(Float)value;
			m_firstItemPosition = point;
		}
		else if( sender == m_horizontalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.X = -(Float)value;
			m_firstItemPosition = point;
		}

		UpdateView();
	}

	void CRedGuiTimelineChart::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta )
	{
		RED_UNUSED( eventPackage );

		if( !GRedGui::GetInstance().GetInputManager()->IsControlPressed() )
		{
			if( m_verticalRange != 0 )
			{
				Vector2 position = m_firstItemPosition;
				Int32 offset = -(Int32)position.Y;

				if( delta < 0 )
				{
					offset += GScrollWheelValue;
				}
				else
				{
					offset -= GScrollWheelValue;
				}

				if( offset < 0 )
				{
					offset = 0;
				}
				else if( offset > (Int32)m_verticalRange )
				{
					offset = m_verticalRange;
				}

				if( offset != position.Y )
				{
					position.Y = -(Float)offset;
					if( m_verticalBar != nullptr )
					{
						m_verticalBar->SetScrollPosition( offset );
					}
					m_firstItemPosition = position;
				}
			}
			else if( m_horizontalRange != 0 )
			{
				Vector2 position = m_firstItemPosition;
				Int32 offset = -(Int32)position.X;

				if( delta < 0 )
				{
					offset += GScrollWheelValue;
				}
				else
				{
					offset -= GScrollWheelValue;
				}

				if( offset < 0 )
				{
					offset = 0;
				}
				else if( offset > (Int32)m_horizontalRange )
				{
					offset = m_horizontalRange;
				}

				if( offset != position.X )
				{
					position.X = -(Float)offset;
					if( m_horizontalBar != nullptr )
					{
						m_horizontalBar->SetScrollPosition( offset );
					}
					m_firstItemPosition = position;
				}
			}
		}

		UpdateView();
	}

	void CRedGuiTimelineChart::NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultTimelineHeight );
		UpdateView();
	}

	void CRedGuiTimelineChart::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		RED_UNUSED( eventPackage );

		if( GetVisible() == false )
		{
			 return;
		}

		if( m_pause ) return;

		// update time
		m_currentTimeTicks = EngineTime::GetNow();
		m_currentTimeSeconds = (Float)m_currentTimeTicks;
		m_deltaTimeSeconds = m_currentTimeSeconds - m_startTimeSeconds;

		UpdateView();
	}

	void CRedGuiTimelineChart::UpdateScrollSize()
	{
		if( m_pause ) return;
		Vector2 viewSize = m_croppClient->GetSize();
		Vector2 contentSize = Vector2( (Float)m_maxItemWidth, (Float)( m_timelines.Size() * GDefaultTimelineHeight ) );

		// horizontal content doesn't fit
		if(contentSize.Y > viewSize.Y)
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == false )
				{
					m_verticalBar->SetVisible( true );

					if(m_horizontalBar != nullptr)
					{
						// recalculate horizontal bar after add vertical bar
						if( ( contentSize.X > viewSize.X ) && ( m_horizontalBar->GetVisible() == false ) )
						{
							m_horizontalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == true )
				{
					m_verticalBar->SetVisible( false );

					if( m_horizontalBar != nullptr )
					{
						// recalculate horizontal bar after remove vertical bar
						if( ( contentSize.X <= viewSize.X ) && ( m_horizontalBar->GetVisible() == true ) )
						{
							m_horizontalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// vertical content doesn't fit
		if( contentSize.X > viewSize.X )
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == false )
				{
					m_horizontalBar->SetVisible( true );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after add horizontal bar
						if( ( contentSize.Y > viewSize.Y ) && ( m_verticalBar->GetVisible() == false ) )
						{
							m_verticalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == true )
				{
					m_horizontalBar->SetVisible( false );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after remove horizontal bar
						if( ( contentSize.Y <= viewSize.Y ) && ( m_verticalBar->GetVisible() == true ) )
						{
							m_verticalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// calculate ranges
		m_verticalRange = ( viewSize.Y >= contentSize.Y ) ? 0 : (Uint32)( contentSize.Y - viewSize.Y );
		m_horizontalRange = ( viewSize.X >= contentSize.X ) ? 0 : (Uint32)( contentSize.X - viewSize.X );

		// set new values
		if( m_verticalBar != nullptr )
		{
			m_verticalBar->SetScrollPage( GScrollPageSize );
			m_verticalBar->SetScrollRange( m_verticalRange + 1 );
			if( contentSize.Y > 0 )
			{
				m_verticalBar->SetTrackSize( (Int32)( (Float)( m_verticalBar->GetLineSize() * viewSize.Y ) / (Float)( contentSize.Y ) ) );
			}
		}
		if( m_horizontalBar != nullptr )
		{
			m_horizontalBar->SetScrollPage( GScrollPageSize );
			m_horizontalBar->SetScrollRange( m_horizontalRange + 1 );
			if( contentSize.X > 0 )
			{
				m_horizontalBar->SetTrackSize( (Int32)( (Float)( m_horizontalBar->GetLineSize() * viewSize.X ) / (Float)( contentSize.X ) ) );
			}
		}
	}

	void CRedGuiTimelineChart::UpdateScrollPosition()
	{
		if( m_pause ) return;
		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.Y;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}

				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}
		if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.X;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}

		// set current visible items
		Uint32 invisibleTop = (Uint32)( (-m_firstItemPosition.Y) / GDefaultTimelineHeight );
		Uint32 invisibleBottom = invisibleTop + m_maxVerticalItemCount + 2;

		m_firstVisibleItem = ( invisibleTop == 0 ) ? 0 : invisibleTop - 1;
		m_lastVisibleItem = ( invisibleBottom > m_timelines.Size() ) ? m_timelines.Size() : invisibleBottom;
	}

	void CRedGuiTimelineChart::UpdateView()
	{
		UpdateScrollSize();
		UpdateScrollPosition();
	}

	void CRedGuiTimelineChart::ClearData()
	{
		const Uint32 timelineCount = m_timelines.Size();
		for( Uint32 i=0; i<timelineCount; ++i )
		{
			m_timelines[i]->ClearData();
		}
	}

	void CRedGuiTimelineChart::ResetTimelines()
	{
		m_startTimeTicks = EngineTime::GetNow();
		m_startTimeSeconds = (Float)( m_startTimeTicks );
	}

	Float CRedGuiTimelineChart::GetStartTimeSeconds() const
	{
		return m_startTimeSeconds;
	}

	Uint32 CRedGuiTimelineChart::GetTaskCachedCount()
	{
		Uint32 result = 0;
		for( auto i = m_timelines.Begin(); i != m_timelines.End(); ++i )
		{
			result += ( *i )->GetEventCount();
		}
		return result;
	}

	void CRedGuiTimelineChart::ScrollToRight()
	{
		if( m_autoScroll == true )
		{
			Int32 viewWidth = m_croppClient->GetWidth();
			m_firstItemPosition.X = -(Float)( (Int32)m_maxItemWidth - viewWidth );

			UpdateView();
		}
	}

	void CRedGuiTimelineChart::SetAutoScroll( Bool value )
	{
		m_autoScroll = value;
	}

	Bool CRedGuiTimelineChart::GetAutoScroll() const
	{
		return m_autoScroll;
	}

	void CRedGuiTimelineChart::SetFrameZoom( Uint32 zoom )
	{
		m_frameZoom = zoom;
	}

	void CRedGuiTimelineChart::SetPause( Bool pause )
	{
		m_pause = pause;
	}

	void CRedGuiTimelineChart::StartEvent( const String& eventName, const String& timelineName )
	{
		if( m_pause ) return;
		StartEvent( eventName, timelineName, m_currentTimeTicks );
	}

	void CRedGuiTimelineChart::StartEvent( const String& eventName, const String& timelineName, Float startTime )
	{
		if( m_pause ) return;
		CRedGuiTimeline* timeline = FindTimeline( timelineName );
		if( timeline != nullptr )
		{
			timeline->StartEvent( eventName, (Float)( startTime - m_startTimeSeconds ) );
		}
	}

	void CRedGuiTimelineChart::StopEvent( const String& eventName, const String& timelineName )
	{
		StopEvent( eventName, timelineName, m_currentTimeTicks );
	}

	void CRedGuiTimelineChart::StopEvent( const String& eventName, const String& timelineName, Float stopTime )
	{
		CRedGuiTimeline* timeline = FindTimeline( timelineName );
		if( timeline != nullptr )
		{
			timeline->StopEvent( eventName, (Float)( stopTime - m_startTimeSeconds ) );
		}
	}

	void CRedGuiTimelineChart::AddEvent( const String& eventName, const String& timelineName, Float startTime, Float stopTime, Color color )
	{
		if( m_pause ) return;
		CRedGuiTimeline* timeline = FindTimeline( timelineName );
		if( timeline != nullptr )
		{
			timeline->AddEvent( eventName, (Float)( startTime - m_startTimeSeconds ), (Float)( stopTime - m_startTimeSeconds ), color );
		}
	}

	Bool CRedGuiTimelineChart::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Move )
		{
			m_horizontalBar->SetScrollPosition( (Uint32)( m_horizontalBar->GetScrollPosition() + data.X ) );
			m_verticalBar->SetScrollPosition( (Uint32)( m_verticalBar->GetScrollPosition() + data.Y ) );
			return true;
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
