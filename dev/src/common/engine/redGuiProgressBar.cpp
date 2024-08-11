/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiProgressBar.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GAutoStepCount = 50;
	}
	
	CRedGuiProgressBar::CRedGuiProgressBar(Uint32 x, Uint32 y, Uint32 width, Uint32 height) 
		: CRedGuiControl(x, y, width, height)
		, m_trackStep(1)
		, m_range(0)
		, m_endPosition(0)
		, m_autoPosition(0.0f)
		, m_autoTrack(false)
		, m_showProgressInfo(true)
	{
		SetBorderVisible(false);

		m_progressLabel = new CRedGuiLabel(0, 0, GetWidth(), GetHeight());
		m_progressLabel->SetAlign(IA_MiddleCenter);
		AddChild(m_progressLabel);

		CRedGuiPanel* client = new CRedGuiPanel(0, 0, width, height);
		client->SetBorderVisible(false);
		client->SetMargin(Box2(1, 1, 1, 1));
		client->SetBackgroundColor(Color::DARK_GREEN);
		SetControlClient(client);

		this->BringToFront(m_progressLabel);
	}

	CRedGuiProgressBar::~CRedGuiProgressBar()
	{
		GRedGui::GetInstance().EventTick.Unbind(this, &CRedGuiProgressBar::FrameEntered);
	}

	void CRedGuiProgressBar::SetPosition( const Vector2& position)
	{
		CRedGuiControl::SetPosition(position);
	}

	void CRedGuiProgressBar::SetPosition(Int32 left, Int32 top)
	{
		CRedGuiControl::SetPosition(left, top);
	}

	void CRedGuiProgressBar::SetSize( const Vector2& size)
	{
		CRedGuiControl::SetSize(size);
		UpdateTrack();
	}

	void CRedGuiProgressBar::SetSize(Int32 width, Int32 height)
	{
		CRedGuiControl::SetSize(width, height);
		UpdateTrack();
	}

	void CRedGuiProgressBar::SetCoord(const Box2& coord)
	{
		CRedGuiControl::SetCoord(coord);
		UpdateTrack();
	}

	void CRedGuiProgressBar::SetCoord(Int32 left, Int32 top, Int32 width, Int32 height)
	{
		CRedGuiControl::SetCoord(left, top, width, height);
		UpdateTrack();
	}

	void CRedGuiProgressBar::SetProgressRange( Float value)
	{
		if(m_autoTrack == true)
		{
			return;
		}

		m_range = value;
		if(m_endPosition > m_range)
		{
			m_endPosition = m_range;
		}
		if(m_range < 0)
		{
			m_range = 0;
		}
		UpdateTrack();
	}

	Float CRedGuiProgressBar::GetProgressRange() const
	{
		return m_range;
	}

	void CRedGuiProgressBar::SetProgressPosition( Float value)
	{
		if(m_autoTrack == true)
		{
			return;
		}

		m_endPosition = value;
		if(m_endPosition > m_range)
		{
			m_endPosition = m_range;
		}
		UpdateTrack();
	}

	Float CRedGuiProgressBar::GetProgressPosition() const
	{
		return m_endPosition;
	}

	void CRedGuiProgressBar::SetProgressAutoTrack(Bool value)
	{
		if(m_autoTrack == value)
		{
			return;
		}

		m_autoTrack = value;
		if(m_autoTrack == true)
		{
			GRedGui::GetInstance().EventTick.Bind(this, &CRedGuiProgressBar::FrameEntered);
			m_endPosition = 0;
			m_autoPosition = 0.0f;
		}
		else
		{
			GRedGui::GetInstance().EventTick.Unbind(this, &CRedGuiProgressBar::FrameEntered);
			m_range = m_endPosition = 0;
		}
		UpdateTrack();
	}

	Bool CRedGuiProgressBar::GetProgressAutoTrack() const
	{
		return m_autoTrack;
	}

	void CRedGuiProgressBar::SetShowProgressInformation(Bool value)
	{
		m_showProgressInfo = value;
		m_progressLabel->SetVisible(m_showProgressInfo);
	}

	Bool CRedGuiProgressBar::GetShowProgressInformation() const
	{
		return m_showProgressInfo;
	}

	void CRedGuiProgressBar::Draw()
	{
		GetTheme()->DrawPanel(this);
	}

	void CRedGuiProgressBar::FrameEntered( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta)
	{
		if(m_autoTrack == false)
		{
			return;
		}

		m_autoPosition += (GAutoStepCount * timeDelta);
		Uint32 pos = (Uint32)m_autoPosition;

		if(pos > m_range)
		{
			m_autoPosition = 0.0f;
		}

		m_endPosition = m_autoPosition;

		UpdateTrack();
	}

	void CRedGuiProgressBar::UpdateTrack()
	{
		if( m_range == 0 || m_endPosition == 0)
		{
			GetClientControl()->SetVisible(false);
			return;
		}
		else
		{
			GetClientControl()->SetVisible(true);

			Int32 newWidth = (Int32)(((Float)m_endPosition/(Float)m_range) * (Float)GetWidth());
			GetClientControl()->SetCoord(0, 0, newWidth, GetHeight() );
		}

		String progressText = ( m_progressInfo.Empty() == true ) ? ToString(((Float)m_endPosition/(Float)m_range) * 100.0f) + TXT("%") : m_progressInfo;
		m_progressLabel->SetText(progressText);

		EventProgressChanged(this, ((Float)m_endPosition/(Float)m_range) * 100.0f);
	}

	void CRedGuiProgressBar::SetProgressBarColor( const Color& color )
	{
		GetClientControl()->SetBackgroundColor( color );
	}

	Color CRedGuiProgressBar::GetProgressBarColor() const
	{
		return GetClientControl()->GetBackgroundColor();
	}

	void CRedGuiProgressBar::SetProgressInformation( const String& text )
	{
		m_progressInfo = text;
		m_progressLabel->SetText( m_progressInfo );
	}

	String CRedGuiProgressBar::GetProgressInformation() const
	{
		return m_progressInfo;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
