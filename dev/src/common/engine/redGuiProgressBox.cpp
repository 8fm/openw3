/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiImage.h"
#include "redGuiProgressBar.h"
#include "redGuiPanel.h"
#include "redGuiProgressBox.h"

namespace RedGui
{
	CRedGuiProgressBox::CRedGuiProgressBox()
		: CRedGuiModalWindow(0,0, 400, 150)
	{
		SetMinSize(GetSize());
		SetCaption(TXT("Waiting ..."));

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
			m_image->SetImage( Resources::GHourglassIcon );

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

			m_progressBar = new CRedGuiProgressBar(0, 0, 200, 16);
			m_progressBar->SetDock(DOCK_Fill);
			bottomPanel->AddChild(m_progressBar);
			m_progressBar->SetProgressRange(100);
		}
	}

	CRedGuiProgressBox::~CRedGuiProgressBox()
	{
		/*intentionally empty*/
	}

	void CRedGuiProgressBox::Show( const String& text, Bool autoProgress /*= false*/ )
	{
		m_progressBar->SetShowProgressInformation(!autoProgress);
		m_progressBar->SetProgressAutoTrack(autoProgress);
		SetText(text);

		// size and position
		Vector2 viewSize = GRedGui::GetInstance().GetRenderManager()->GetViewSize();
		Vector2 screenPosition = (viewSize - GetSize()) / 2;
		SetPosition(screenPosition);
		SetVisible(true);
	}

	void CRedGuiProgressBox::SetText( const String& text )
	{
		m_text->SetText(text);
		SetSize(Vector2(m_text->GetSize().X + 110, GetSize().Y));
	}

	void CRedGuiProgressBox::UpdateProgress( Float percent )
	{
		m_progressBar->SetProgressPosition( Clamp< Float >( percent, 0.0f, 100.0f ) );
	}

	void CRedGuiProgressBox::Hide()
	{
		this->SetVisible( false );
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
