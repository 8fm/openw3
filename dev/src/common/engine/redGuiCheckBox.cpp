/**	
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiLabel.h"
#include "redGuiImage.h"
#include "redGuiCheckBox.h"

namespace RedGui
{
	namespace
	{
		Uint32 GDefaultCheckBoxSize = 13;
	}

	CRedGuiCheckBox::CRedGuiCheckBox(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
		: CRedGuiControl(x, y, GDefaultCheckBoxSize, GDefaultCheckBoxSize)
		, m_label(nullptr)
		, m_button(nullptr)
	{
		SetBorderVisible(false);
		SetBackgroundColor(Color::CLEAR);
		SetNeedKeyFocus( true );

		CRedGuiPanel* checkpanel = new CRedGuiPanel(0,0, GDefaultCheckBoxSize, GDefaultCheckBoxSize);
		checkpanel->SetBackgroundColor(Color::CLEAR);
		checkpanel->SetBorderVisible(false);
		AddChild(checkpanel);
		checkpanel->SetDock(DOCK_Left);

		CRedGuiPanel* whiteArea = new CRedGuiPanel(0,0, GDefaultCheckBoxSize, GDefaultCheckBoxSize);
		whiteArea->SetBackgroundColor(Color::WHITE);
		whiteArea->SetBorderVisible(false);
		whiteArea->SetAlign(IA_MiddleLeft);
		checkpanel->AddChild(whiteArea);

		m_button = new CRedGuiButton(0,0, GDefaultCheckBoxSize, GDefaultCheckBoxSize);
		m_button->SetToggleMode(true);
		m_button->SetBackgroundColor(Color::WHITE);
		m_button->SetImage( Resources::GCheckIcon );
		m_button->GetImageBox()->SetVisible(false);
		m_button->SetBackgroundColor(Color::CLEAR);
		m_button->SetNeedKeyFocus( false );
		m_button->EventCheckedChanged.Bind(this, &CRedGuiCheckBox::NotifyEventCheckedChanged );
		whiteArea->AddChild(m_button);
		m_button->SetDock(DOCK_Fill);

		m_label = new CRedGuiLabel(0,0,0,0);
		m_label->SetEnabled(false);
		m_label->SetMargin(Box2(5, 1, 5, 0));
		m_label->SetNeedKeyFocus( false );
		m_label->EventMouseButtonClick.Bind(this, &CRedGuiCheckBox::NotifyEventClicked );
		AddChild(m_label);
		m_label->SetDock(DOCK_Left);

		CalculateSize();
	}

	CRedGuiCheckBox::~CRedGuiCheckBox()
	{
		
	}

	void CRedGuiCheckBox::OnPendingDestruction()
	{
		m_button->EventCheckedChanged.Unbind(this, &CRedGuiCheckBox::NotifyEventCheckedChanged);
		m_label->EventMouseButtonClick.Unbind(this, &CRedGuiCheckBox::NotifyEventClicked);
	}

	void CRedGuiCheckBox::NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value )
	{
		m_button->GetImageBox()->SetVisible(value);
		EventCheckedChanged(this, value);
	}

	void CRedGuiCheckBox::NotifyEventClicked( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button  )
	{
		NotifyEventCheckedChanged( eventPackage, !m_button->GetToggleValue() );
	}

	void CRedGuiCheckBox::Draw()
	{
		GetTheme()->DrawPanel(this);
	}

	void CRedGuiCheckBox::SetChecked( Bool value, Bool silentChange /*= false*/ )
	{
		m_button->SetToggleValue( value, silentChange );
		m_button->GetImageBox()->SetVisible(value);
	}

	Bool CRedGuiCheckBox::GetChecked() const
	{
		return m_button->GetToggleValue();
	}

	void CRedGuiCheckBox::SetText( const String& text, const Color& textColor /*= Color::WHITE */ )
	{
		m_label->SetText( text, textColor );
		CalculateSize();
	}

	String CRedGuiCheckBox::GetText() const
	{
		return m_label->GetText();
	}

	void CRedGuiCheckBox::CalculateSize()
	{
		Uint32 width = (Uint32)m_label->GetSize().X + (Uint32)m_label->GetMargin().Min.X + 
			(Uint32)m_label->GetMargin().Max.X + (Uint32)m_button->GetSize().X + 
			(Uint32)m_button->GetMargin().Min.X + (Uint32)m_button->GetMargin().Max.X;

		SetSize(width, GDefaultCheckBoxSize);
		SetOriginalRect(GetCoord());
	}

	Bool CRedGuiCheckBox::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Execute )
		{
			SetChecked( !GetChecked() );
			return true;
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
