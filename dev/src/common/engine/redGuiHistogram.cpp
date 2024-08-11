/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiPanel.h"
#include "redGuiButton.h"
#include "redGuiList.h"
#include "redGuiHistogram.h"

namespace RedGui
{
	CRedGuiHistogram::CRedGuiHistogram( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
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

	CRedGuiHistogram::~CRedGuiHistogram()
	{
		/* intentionally empty */
	}

	void CRedGuiHistogram::SetLegendVisible( Bool value )
	{
		m_legendPanel->SetVisible(value);
	}

	Bool CRedGuiHistogram::GetLegendVisible() const
	{
		return m_legendPanel->GetVisible();
	}

	void CRedGuiHistogram::AddData( const String& name, const Color& color, Float percent )
	{
		SRedGuiOccupancySeries series;
		series.m_name = name;
		series.m_color = color;
		for ( Uint32 i=0; i < kHistoryLength - 1; ++i )
		{
			series.m_percent[i] = 0;
		}
		series.m_percent[kHistoryLength - 1] = percent;

		m_data.PushBack(series);
		m_legendList->AddItem( name, color );
		
		if(m_checkCorrentPercent == true)
		{
			CheckPercentCorrectness();
		}
	}

	void CRedGuiHistogram::UpdateDate( const String& name, Float percent )
	{
		for(Uint32 i=0; i<m_data.Size(); ++i)
		{
			if( m_data[i].m_name == name)
			{
				for ( Uint32 j=0; j < kHistoryLength - 1; ++j )
				{
					m_data[i].m_percent[j] = m_data[i].m_percent[j + 1];
				}
				m_data[i].m_percent[kHistoryLength - 1] = percent;
				break;
			}
		}

		if(m_checkCorrentPercent == true)
		{
			CheckPercentCorrectness();
		}
	}

	void CRedGuiHistogram::ClearData()
	{
		m_data.Clear();
		m_legendList->RemoveAllItems();
	}

	void CRedGuiHistogram::Draw()
	{
		Vector2 base = GetAbsolutePosition();
		GetTheme()->DrawPanel( this );
		GetTheme()->SetCroppedParent( this );

		if(m_correctPercent == true)
		{
			Int32 width = m_chartPanel->GetWidth(), height = m_chartPanel->GetHeight();

			for ( Int32 x=0; x < width; ++x )
			{
				Int32 historyPointIndex = Clamp( x * kHistoryLength / width, 0, kHistoryLength - 1 );
				Int32 y = 0;
				
				for ( Int32 i=0; i < m_data.SizeInt(); ++i )
				{
					Float lineHeight = m_data[i].m_percent[historyPointIndex] * ((Float)height);
					Int32 finalLineHeight = ( i == m_data.SizeInt() - 1 ? ( height - y ) : (Int32)lineHeight );
					Vector2 start( (Float)x, (Float)( height - y - 1 ) );
					Vector2 end( (Float)x, (Float)( height - ( y + finalLineHeight ) ) - 1 );
					Color color = m_data[i].m_color;
					color.A = 255;
					GetTheme()->DrawRawLine( base + start, base + end, color, true );
					y += finalLineHeight;
				}
			}

		}

		GetTheme()->ResetCroppedParent();
	}

	void CRedGuiHistogram::CheckPercentCorrectness()
	{
		Float percent = 0.0f;
		for(Uint32 i=0; i<m_data.Size(); ++i)
		{
			percent += m_data[i].m_percent[kHistoryLength - 1];
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

	void CRedGuiHistogram::SetCheckCorrectPercent( Bool value )
	{
		m_checkCorrentPercent = value;
	}

	Bool CRedGuiHistogram::GetCheckCorrectPercent() const
	{
		return m_checkCorrentPercent;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
