/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiLabel.h"
#include "redGuiImage.h"
#include "redGuiButton.h"
#include "redGuiManager.h"
#include "redGuiMessageBox.h"
#include "inputKeys.h"

namespace RedGui
{
	CRedGuiMessageBox::CRedGuiMessageBox()
		: CRedGuiModalWindow(0,0, 400, 150)
	{
		SetVisibleCaptionButton(CB_Minimize, false);
		SetVisibleCaptionButton(CB_Maximize, false);
		SetVisibleCaptionButton(CB_Exit, false);
		SetBackgroundColor(Color::CLEAR);
		SetResizable(false);
		SetMovable(false);

		// top panel
		{
			CRedGuiPanel* topPanel = new CRedGuiPanel(0,0,GetWidth(), 90);
			AddChild(topPanel);
			topPanel->SetDock(DOCK_Top);
			topPanel->SetBackgroundColor(Color::CLEAR);
			topPanel->SetPadding(Box2(20, 20, 20 ,20));

			m_image = new CRedGuiImage(0, 0, 32, 32);
			topPanel->AddChild(m_image);
			m_image->SetAlign(IA_MiddleLeft);
			m_image->SetImage( Resources::GInformationIcon );

			m_text = new CRedGuiLabel(0, 0, 100, 20);
			topPanel->AddChild(m_text);
			m_text->SetAlign(IA_MiddleRight);
			m_text->SetMargin(Box2(5, 10, 0, 10));
		}

		// bottom panel
		{
			CRedGuiPanel* bottomPanel = new CRedGuiPanel(0,0,GetWidth(), 80);
			AddChild(bottomPanel);
			bottomPanel->SetDock(DOCK_Fill);
			bottomPanel->SetBackgroundColor(Color::GRAY);
			bottomPanel->SetPadding(Box2(5, 5, 5, 5));

			m_ok = new CRedGuiButton(0, 0, 75, 25);
			bottomPanel->AddChild(m_ok);
			m_ok->SetText(TXT("OK"));
			m_ok->SetDock(DOCK_Right);
			m_ok->SetNeedKeyFocus(true);
			m_ok->EventButtonClicked.Bind(this, &CRedGuiMessageBox::NotifyEventButtonClicked);
			m_ok->EventKeyButtonPressed.Bind(this, &CRedGuiMessageBox::NotifyEventKeyButtonPressed);
		}
	}

	CRedGuiMessageBox::~CRedGuiMessageBox()
	{
	}

	void CRedGuiMessageBox::OnPendingDestruction()
	{
		m_ok->EventButtonClicked.Unbind(this, &CRedGuiMessageBox::NotifyEventButtonClicked);
		m_ok->EventKeyButtonPressed.Unbind(this, &CRedGuiMessageBox::NotifyEventKeyButtonPressed);
	}

	void CRedGuiMessageBox::Show( const String& text, const String& title, EMessabeBoxIcon icon /*= MESSAGEBOX_Info*/ )
	{
		SetCaption(title);
		m_text->SetText(text);
		SetIcon(icon);

		SetSize(Vector2(m_text->GetSize().X + 110, GetSize().Y));

		// size and position
		Vector2 viewSize = GRedGui::GetInstance().GetRenderManager()->GetViewSize();
		Vector2 screenPosition = (viewSize - GetSize()) / 2;
		SetPosition(screenPosition);

		SetVisible(true);
		GRedGui::GetInstance().GetInputManager()->SetKeyFocusControl( m_ok );
	}

	void CRedGuiMessageBox::Show( const String& text, EMessabeBoxIcon icon /*= MESSAGEBOX_Info*/ )
	{
		Show(text, TXT(""), icon);
	}

	void CRedGuiMessageBox::NotifyEventButtonClicked( CRedGuiEventPackage& eventPackage )
	{
		SetVisible(false);
	}

	void CRedGuiMessageBox::NotifyEventKeyButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, enum EInputKey key, Char text )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(sender == m_ok)
		{
			if(key == IK_Enter || key == IK_Space)
			{
				GRedGui::GetInstance().GetInputManager()->ResetKeyFocusControl();
				SetVisible(false);
			}
		}
	}

	void CRedGuiMessageBox::SetIcon( EMessabeBoxIcon icon )
	{
		switch(icon)
		{
		case MESSAGEBOX_Error:
			m_image->SetImage( Resources::GErrorIcon );
			break;
		case MESSAGEBOX_Info:
			m_image->SetImage( Resources::GInformationIcon );
			break;
		case MESSAGEBOX_Warning:
			m_image->SetImage( Resources::GWarningIcon );
			break;
		}
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
