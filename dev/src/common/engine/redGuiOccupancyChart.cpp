/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiList.h"
#include "redGuiOccupancyChart.h"

namespace RedGui
{
	CRedGuiOccupancyChart::CRedGuiOccupancyChart( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
		: CRedGuiControl( left, top, width, height)
		, m_chartPanel(nullptr)
		, m_legendPanel(nullptr)
		, m_correctPercent(true)
		, m_checkCorrentPercent(true)
	{
		m_legendPanel = new CRedGuiPanel(width-200,0, 200, height);
		AddChild(m_legendPanel);
		m_legendPanel->SetDock(DOCK_Right);
		m_legendPanel->SetBackgroundColor(Color::CLEAR);
		m_legendPanel->SetBorderVisible(false);

		m_legendList = new CRedGuiList(0, 0, m_legendPanel->GetWidth(), m_legendPanel->GetHeight());
		m_legendList->AppendColumn( TXT("Legend"), 100 );
		m_legendPanel->AddChild(m_legendList);
		m_legendList->SetDock(DOCK_Fill);
		m_legendList->SetSelectionMode( SM_None );

		m_chartPanel = new CRedGuiPanel(0,0, width, height);
		AddChild(m_chartPanel);
		m_chartPanel->SetDock(DOCK_Fill);
		m_chartPanel->SetBackgroundColor(Color::CLEAR);
		m_chartPanel->SetBorderVisible( false );

		m_errorInfo = new CRedGuiButton(0, 0, width, height);
		m_chartPanel->AddChild(m_errorInfo);
		m_errorInfo->SetEnabled(false);
		m_errorInfo->SetVisible(false);
		m_errorInfo->SetText(TXT("Data is corrupted - sum of percent is bigger than 100%"));
		m_errorInfo->SetTextColor(Color::RED);
		m_errorInfo->SetDock(DOCK_Fill);
		m_errorInfo->SetBackgroundColor(Color::CLEAR);
	}

	CRedGuiOccupancyChart::~CRedGuiOccupancyChart()
	{
		/* intentionally empty */
	}

	void CRedGuiOccupancyChart::SetLegendVisible( Bool value )
	{
		m_legendPanel->SetVisible(value);
	}

	Bool CRedGuiOccupancyChart::GetLegendVisible() const
	{
		return m_legendPanel->GetVisible();
	}

	void CRedGuiOccupancyChart::AddData( const String& name, const Color& color, Float percent )
	{
		SRedGuiOccupancySeries series = { name, percent, color };
		m_data.PushBack(series);
		m_legendList->AddItem( name, color );

		if(m_checkCorrentPercent == true)
		{
			CheckPercentCorrectness();
		}
	}

	Bool CRedGuiOccupancyChart::UpdateData( const String& name, Float percent )
	{
		Bool added = false;
		for(Uint32 i=0; i<m_data.Size(); ++i)
		{
			if( m_data[i].m_name == name)
			{
				m_data[i].m_percent = percent;
				added = true;
				break;
			}
		}

		if(m_checkCorrentPercent == true)
		{
			CheckPercentCorrectness();
		}

		return added;
	}

	void CRedGuiOccupancyChart::ClearData()
	{
		m_data.Clear();
		m_legendList->RemoveAllItems();
	}

	void CRedGuiOccupancyChart::Draw()
	{
		GetTheme()->DrawPanel( this );
		GetTheme()->SetCroppedParent( this );

		if(m_correctPercent == true)
		{
			Uint32 xPos = 0;
			for(Uint32 i=0; i<m_data.Size(); ++i)
			{
				Uint32 deltaWidth = (Uint32)( m_chartPanel->GetWidth() * m_data[i].m_percent );
				Vector2 pos( GetAbsolutePosition().X + (Float)xPos + 1, (Float)GetAbsolutePosition().Y + 1);
				Vector2 size( (Float)deltaWidth, (Float)m_chartPanel->GetHeight() );

				GetTheme()->DrawRawFilledRectangle( pos, size, m_data[i].m_color );

				xPos += deltaWidth;
			}
		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiOccupancyChart::CheckPercentCorrectness()
	{
		Float percent = 0.0f;
		for(Uint32 i=0; i<m_data.Size(); ++i)
		{
			percent += m_data[i].m_percent;
		}

		if(percent > 1.0f)
		{
			m_correctPercent = false;
		}
		else
		{
			m_correctPercent = true;
		}

		m_errorInfo->SetVisible(!m_correctPercent);
	}

	void CRedGuiOccupancyChart::SetCheckCorrectPercent( Bool value )
	{
		m_checkCorrentPercent = value;
	}

	Bool CRedGuiOccupancyChart::GetCheckCorrectPercent() const
	{
		return m_checkCorrentPercent;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
