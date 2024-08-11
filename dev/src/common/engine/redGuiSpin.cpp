/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiTextBox.h"
#include "redGuiSpin.h"

namespace RedGui
{
	CRedGuiSpin::CRedGuiSpin( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl(left, top, width, height)
		, m_line(nullptr)
		, m_upButton(nullptr)
		, m_downButton(nullptr)
		, m_value(0)
		, m_minValue(0)
		, m_maxValue(100)
	{
		SetNeedKeyFocus( true );
		SetBackgroundColor( Color::CLEAR );

		CRedGuiPanel* buttonsPanel = new CRedGuiPanel(0,0,20, 20);
		buttonsPanel->SetBorderVisible(false);
		buttonsPanel->SetDock(DOCK_Right);
		AddChild(buttonsPanel);
		{
			m_upButton = new CRedGuiButton(0,0,20,10);
			m_upButton->SetImage( Resources::GUpArrowIcon );
			m_upButton->SetBorderVisible(false);
			buttonsPanel->AddChild(m_upButton);
			m_upButton->SetDock(DOCK_Top);
			m_upButton->EventButtonClicked.Bind(this, &CRedGuiSpin::NotifyEventButtonClicked);

			m_downButton = new CRedGuiButton(0,0,20,10);
			m_downButton->SetImage( Resources::GDownArrowIcon );
			m_downButton->SetBorderVisible(false);
			buttonsPanel->AddChild(m_downButton);
			m_downButton->SetDock(DOCK_Bottom);
			m_downButton->EventButtonClicked.Bind(this, &CRedGuiSpin::NotifyEventButtonClicked);
		}

		m_line = new CRedGuiTextBox( 0, 0, 100, 20 );
		m_line->SetBorderVisible(false);
		m_line->SetDock(DOCK_Fill);
		m_line->SetText(ToString(m_value));
		m_line->EventMouseWheel.Bind( this, &CRedGuiSpin::NotifyEventMouseWheel );
		m_line->EventTextEnter.Bind( this, &CRedGuiSpin::NotifyEventTextEnter );
		AddChild(m_line);

		SetNewValue( 0, true );
	}

	CRedGuiSpin::~CRedGuiSpin()
	{
		
	}

	void CRedGuiSpin::OnPendingDestruction()
	{
		m_line->EventMouseWheel.Unbind( this, &CRedGuiSpin::NotifyEventMouseWheel );
		m_upButton->EventButtonClicked.Unbind(this, &CRedGuiSpin::NotifyEventButtonClicked);
		m_downButton->EventButtonClicked.Unbind(this, &CRedGuiSpin::NotifyEventButtonClicked);
	}

	void CRedGuiSpin::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiSpin::NotifyEventButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(sender == m_upButton)
		{
			SetNewValue( GetValue() + 1, false );
		}
		else if(sender == m_downButton)
		{
			SetNewValue( GetValue() - 1, false );
		}
	}

	Int32 CRedGuiSpin::GetValue() const
	{
		return m_value;
	}

	void CRedGuiSpin::SetValue( Int32 value, Bool silentChange /*= false*/ )
	{
		SetNewValue( value, silentChange );
	}

	Int32 CRedGuiSpin::GetMinValue() const
	{
		return m_minValue;
	}

	void CRedGuiSpin::SetMinValue( Int32 value )
	{
		m_minValue = value;
	}

	Int32 CRedGuiSpin::GetMaxValue() const
	{
		return m_maxValue;
	}

	void CRedGuiSpin::SetMaxValue( Int32 value )
	{
		m_maxValue = value;
	}

	void CRedGuiSpin::NotifyEventMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		const Int32 oldValue = m_value;

		if( value > 0 )
		{
			SetNewValue( GetValue() + 1, false );
		}
		else if( value < 0 )
		{
			SetNewValue( GetValue() - 1, false );
		}
	}

	Bool CRedGuiSpin::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Up )
		{
			SetNewValue( GetValue() + 1, false );
			return true;
		}
		else if( event == RGIE_Down )
		{
			SetNewValue( GetValue() - 1, false );
			return true;
		}

		return false;
	}

	void CRedGuiSpin::SetNewValue( Int32 newValue, Bool silentChange )
	{
		m_value = Clamp< Int32 >( newValue, m_minValue, m_maxValue );
		m_line->SetText( ToString( m_value ) );

		if( silentChange == false )
		{
			EventValueChanged( this, m_value );
		}
	}

	void CRedGuiSpin::NotifyEventTextEnter( RedGui::CRedGuiEventPackage& eventPackage )
	{
		Int32 tempValue = 0;
		Int32 currentValue = GetValue();
		if( FromString< Int32 >( m_line->GetText(), tempValue ) == true )
		{
			SetNewValue( tempValue, false );
		}
		else
		{
			SetNewValue( currentValue, false );
		}
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
