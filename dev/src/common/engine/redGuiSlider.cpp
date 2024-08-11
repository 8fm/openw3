/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiSlider.h"

namespace RedGui
{
	namespace
	{
		const Float GDefaultTrackSize = 10.0f ;
		const Float m_stepValue = 5.0f;
	}

	CRedGuiSlider::CRedGuiSlider( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl( x, y, width, height )
		, m_value( 0.0f )
		, m_minValue( 0.0f )
		, m_maxValue( 100.0f )
		, m_stepValue( 5.0f )
		, m_track( nullptr )
		, m_line( nullptr )
	{
		SetNeedKeyFocus( true );
		SetBorderVisible( false );
		SetBackgroundColor( Color::CLEAR );

		m_line = new CRedGuiPanel( (Uint32)GDefaultTrackSize/2, GetHeight()/2, GetWidth()-(Uint32)GDefaultTrackSize, 1 );
		m_line->EventMouseWheel.Bind( this, &CRedGuiSlider::NotifyMouseWheel );
		m_line->SetAlign( IA_MiddleCenter );
		m_line->SetNeedKeyFocus( false );
		AddChild( m_line );

		m_track = new CRedGuiButton( 0,1, (Uint32)GDefaultTrackSize, GetHeight()-2 );
		m_track->SetNeedKeyFocus( false );
		AddChild( m_track );

		m_track->EventMouseDrag.Bind( this, &CRedGuiSlider::NotifyMouseDrag );
		m_track->EventMouseWheel.Bind( this, &CRedGuiSlider::NotifyMouseWheel );
		m_track->EventMouseButtonPressed.Bind( this, &CRedGuiSlider::NotifyMouseButtonPressed );
		m_track->EventMouseButtonReleased.Bind( this, &CRedGuiSlider::NotifyMouseButtonReleased );

		UpdateTrack();
	}

	CRedGuiSlider::~CRedGuiSlider()
	{
		
	}

	void CRedGuiSlider::OnPendingDestruction()
	{
		m_line->EventMouseWheel.Unbind( this, &CRedGuiSlider::NotifyMouseWheel );
		m_line->EventMouseWheel.Unbind( this, &CRedGuiSlider::NotifyMouseWheel );
		m_track->EventMouseDrag.Unbind( this, &CRedGuiSlider::NotifyMouseDrag );
		m_track->EventMouseWheel.Unbind( this, &CRedGuiSlider::NotifyMouseWheel );
		m_track->EventMouseButtonPressed.Unbind( this, &CRedGuiSlider::NotifyMouseButtonPressed );
		m_track->EventMouseButtonReleased.Unbind( this, &CRedGuiSlider::NotifyMouseButtonReleased );
	}

	void CRedGuiSlider::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiSlider::SetValue( Float value, Bool silentChange /*= false*/ )
	{
		if( value < m_minValue )
		{
			m_value = m_minValue;
		}
		else if( value > m_maxValue )
		{
			m_value = m_maxValue;
		}
		else
		{
			m_value = value;
		}

		UpdateTrack( silentChange );
	}

	Float CRedGuiSlider::GetValue() const
	{
		return m_value;
	}

	void CRedGuiSlider::SetMinValue( Float value )
	{
		m_minValue = value;

		if( m_minValue > m_maxValue )
		{
			m_minValue = m_maxValue;
		}
		if( m_value < m_minValue )
		{
			m_value = m_minValue;
		}

		UpdateTrack();
	}

	Float CRedGuiSlider::GetMinValue() const
	{
		return m_minValue;
	}

	void CRedGuiSlider::SetMaxValue( Float value )
	{
		m_maxValue = value;

		if( m_maxValue < m_minValue )
		{
			m_maxValue = m_minValue;
		}
		if( m_value > m_maxValue )
		{
			m_value = m_maxValue;
		}

		UpdateTrack();
	}

	Float CRedGuiSlider::GetMaxValue() const
	{
		return m_maxValue;
	}

	void CRedGuiSlider::UpdateTrack( Bool silentChange /*= false*/ )
	{
		Float percent = ( m_value - m_minValue ) / ( m_maxValue - m_minValue );
		Int32 position = (Int32)( GetSliderLineLenght() * percent );

		m_track->SetPosition( position, (Int32)m_track->GetPosition().Y );

		// send event
		if( silentChange == false )
		{
			EventScroll( this, m_value );
		}
	}

	void CRedGuiSlider::TrackMove( Float left, Float top, const Vector2& lastClickedPoint )
	{
		Float start = m_preActionOffset.X + ( left - lastClickedPoint.X );
		if( start < 0.0f )
		{
			start = 0.0f;
		}
		else if( start > ( GetWidth() - GetTrackSize() ) )
		{
			start = GetWidth() - GetTrackSize();
		}

		Float range = m_maxValue - m_minValue;
		Float percent = start / GetSliderLineLenght();
		m_value = (range * percent) + m_minValue;

		UpdateTrack();
	}
	
	Float CRedGuiSlider::GetTrackSize() const
	{
		return GDefaultTrackSize;
	}

	Float CRedGuiSlider::GetSliderLineLenght() const
	{
		return (Float)GetWidth() - GetTrackSize();
	}

	void CRedGuiSlider::OnMouseWheel( Int32 delta )
	{
		RedGui::CRedGuiEventPackage eventPackage( nullptr );
		NotifyMouseWheel( eventPackage, delta );
		CRedGuiControl::OnMouseWheel( delta );
	}

	void CRedGuiSlider::NotifyMouseDrag( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		if( button == MB_Left )
		{
			const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition( MB_Left );
			TrackMove( mousePosition.X, mousePosition.Y, point );
		}
	}

	void CRedGuiSlider::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta)
	{
		RED_UNUSED( eventPackage );

		if( delta < 0 )
		{
			m_value -= m_stepValue;
		}
		else
		{
			m_value += m_stepValue;
		}

		SetValue( m_value );
	}

	void CRedGuiSlider::NotifyMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		EventMouseButtonPressed( this, mousePosition, button );

		if( button != MB_Left )
		{
			return;
		}

		if( sender == m_track )
		{
			m_preActionOffset = sender->GetPosition();
		}
	}

	void CRedGuiSlider::NotifyMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		UpdateTrack();
	}

	void CRedGuiSlider::OnSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		m_line->SetSize( GetWidth()-(Int32)GDefaultTrackSize, 1 );
		UpdateTrack( true );
	}

	void CRedGuiSlider::OnMouseButtonClick( const Vector2& mousePosition, enum EMouseButton button )
	{
		if( button != MB_Left )
		{
			return;
		}

		if( GRedGui::GetInstance().GetInputManager()->IsShiftPressed() == true )
		{
			m_preActionOffset = m_track->GetPosition();
			const Vector2 point = Vector2( ( m_track->GetAbsolutePosition().X + ( m_track->GetSize().X / 2 ) ), m_track->GetAbsolutePosition().Y );
			TrackMove( mousePosition.X, mousePosition.Y, point );
		}
		else
		{
			Int32 delta = 0;
			if( mousePosition.X < m_track->GetAbsolutePosition().X )
			{
				delta = -1;
			}
			if( mousePosition.X > ( m_track->GetAbsolutePosition().X + m_track->GetSize().X ) )
			{
				delta = 1;
			}
			RedGui::CRedGuiEventPackage eventPackage( this );
			NotifyMouseWheel( eventPackage, delta );
		}
	}

	void CRedGuiSlider::SetStepValue( Float value )
	{
		if( value <= 0.0f )
		{
			m_stepValue = 0.1f;
		}
		else if( value >= m_maxValue )
		{
			m_stepValue = m_maxValue / 2.0f;
		}
		else
		{
			m_stepValue = value;
		}
	}

	Float CRedGuiSlider::GetStepValue() const
	{
		return m_stepValue;
	}

	Bool CRedGuiSlider::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Left || event == RGIE_Down )
		{
			SetValue( m_value - m_stepValue );
			return true;
		}
		else if( event == RGIE_Right || event == RGIE_Up )
		{
			SetValue( m_value + m_stepValue );
			return true;
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
