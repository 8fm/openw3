/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiGroupBox.h"

namespace RedGui
{
	CRedGuiGroupBox::CRedGuiGroupBox( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl(left, top, width, height)
		, m_expandButton(nullptr)
	{
		SetNeedKeyFocus( true );
		CRedGuiControl::SetBackgroundColor( Color::CLEAR );

		m_captionPanel = new CRedGuiPanel(0,0, width, 20);
		m_captionPanel->SetBackgroundColor(Color(63, 63, 70));
		AddChild(m_captionPanel);
		m_captionPanel->SetDock(DOCK_Top);

		m_expandButton = new CRedGuiButton(0,0, 13, 13);
		m_expandButton->SetBackgroundColor(Color::CLEAR);
		m_expandButton->SetToggleMode(true);
		m_expandButton->SetImage( Resources::GMinusIcon );
		m_expandButton->SetToggleValue(true);
		m_expandButton->EventCheckedChanged.Bind(this, &CRedGuiGroupBox::NotifyEventCheckedChanged);
		m_expandButton->SetMargin(Box2(5, 5, 5, 5));
		m_expandButton->SetBorderVisible(false);
		m_expandButton->SetBackgroundColor(Color::CLEAR);
		m_captionPanel->AddChild(m_expandButton);
		m_expandButton->SetDock(DOCK_Left);
		m_expandButton->SetNeedKeyFocus( true );

		m_titleButton = new CRedGuiButton(0,0,100, 15);
		m_titleButton->SetBackgroundColor(Color::CLEAR);
		m_titleButton->SetBorderVisible(false);
		m_titleButton->SetFitToText(true);
		m_titleButton->SetEnabled(false);
		m_captionPanel->AddChild(m_titleButton);
		m_titleButton->SetDock(DOCK_Left);

		m_contentPanel = new CRedGuiPanel(0,0, width, height);
		SetControlClient(m_contentPanel);
		m_contentPanel->SetDock(DOCK_Fill);
	}

	CRedGuiGroupBox::~CRedGuiGroupBox()
	{
		
	}

	void CRedGuiGroupBox::OnPendingDestruction()
	{
		m_expandButton->EventCheckedChanged.Unbind(this, &CRedGuiGroupBox::NotifyEventCheckedChanged);
	}

	void CRedGuiGroupBox::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiGroupBox::NotifyEventCheckedChanged( CRedGuiEventPackage& eventPackage, Bool value )
	{
		if(value == true)
		{
			Expand();
		}
		else
		{
			Collapse();
		}


		if(GetParent() != nullptr)
		{
			GetParent()->SetOutOfDate();
		}
	}

	String CRedGuiGroupBox::GetText() const
	{
		return m_titleButton->GetText();
	}

	void CRedGuiGroupBox::SetText( const String& text )
	{
		m_titleButton->SetText(text);
	}

	void CRedGuiGroupBox::Collapse()
	{
		m_contentPanel->SetVisible(false);
		m_expandButton->SetImage( Resources::GPlusIcon );
		m_minPreviousCoord = GetCoord();
		SetCoord(Box2(GetPosition(), Vector2((Float)GetWidth(), m_captionPanel->GetSize().Y + 1 )));
		SetOriginalRect(GetCoord());
	}

	void CRedGuiGroupBox::Expand()
	{
		m_contentPanel->SetVisible(true);
		m_expandButton->SetImage( Resources::GMinusIcon );
		SetCoord(Box2(GetPosition(), m_minPreviousCoord.Max));
		SetOriginalRect(GetCoord());
	}

	void CRedGuiGroupBox::SetBackgroundColor( const Color& color )
	{
		m_contentPanel->SetBackgroundColor( color );
	}

} // namespace RedGui

#endif	// NO_RED_GUI
