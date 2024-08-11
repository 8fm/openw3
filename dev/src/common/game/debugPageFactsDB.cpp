/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "../game/factsDB.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

/// Debug page with facts data base informations on screen log
class CDebugPageFactsDB : public IDebugPage //, public IOnScreenLog
{
protected:
	enum EScrollType
	{
		ST_None,
		ST_Down,
		ST_Up,
	};

	Float		m_refreshTimer; // data refresh timer
	Float		m_scrollTimer;
	Int32			m_msgShift;
	Int32			m_subPageCurrent;
	Int32			m_subPageMax;
	EScrollType	m_currScroll;

public:
	CDebugPageFactsDB()
		: IDebugPage( TXT("Facts") )
		, m_refreshTimer( 0 )
		, m_scrollTimer( 0 )
		, m_msgShift( 0 )
		, m_subPageCurrent( 0 )
		, m_subPageMax( 1 )
		, m_currScroll( ST_None )
	{
	};

	~CDebugPageFactsDB()
	{
	}

	virtual void OnTick( Float timeDelta )
	{
		// Refreshing

		m_refreshTimer += timeDelta;
		if ( m_refreshTimer > 1.0f ) // update every second
		{
			m_refreshTimer = 0;
			UpdateData();
		}

		// Scrolling

		m_scrollTimer += timeDelta;
		if ( m_scrollTimer > 0.1f )
		{
			m_scrollTimer = 0;

			if ( m_currScroll == ST_Down )
			{
				ScroolMsgDown();
			}
			else if ( m_currScroll == ST_Up )
			{
				ScroolMsgUp();
			}
		}
	}

	virtual void OnPageShown() 
	{
		IDebugPage::OnPageShown();
	}

	virtual void OnPageHidden() 
	{
		IDebugPage::OnPageHidden();
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		// Header
		Int32 y = 65;
		frame->AddDebugScreenText( 55, y, TXT("Facts") );
		y += 15;

		// List
		Uint32 xCol0 = 75;				// First column
		//Uint32 xCol1 = xCol0 + 40;		// Second column
		//Uint32 xCol2 = xCol1 + 100;		// Third column
		Int32 maxCount = 30;
		for ( Int32 i = m_msgShift; i < (Int32)m_factsToDisplay.Size() && maxCount > 0; ++i, --maxCount )
		{
			frame->AddDebugScreenText( xCol0, y, m_factsToDisplay[i], Color::WHITE );

			// Move down
			y += 15;
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// Send the event
		if ( action == IACT_Press )
		{
			switch ( key )
			{
			case IK_Down:
			case IK_Pad_DigitDown:
				m_currScroll = ST_Down;
				break;
			case IK_Up:
			case IK_Pad_DigitUp:
				m_currScroll = ST_Up;
				break;
			case IK_Pad_Y_TRIANGLE:
			case IK_Tab:
				NextSubPage();
				break;
			}
		}
		else if ( action == IACT_Release )
		{
			switch ( key )
			{
			case IK_Down:
			case IK_Pad_DigitDown:
				m_currScroll = ST_None;
				break;
			case IK_Up:
			case IK_Pad_DigitUp:
				m_currScroll = ST_None;
				break;
			}
		}

		// Not processed
		return false;
	}

private:
	void ScroolMsgDown( Int32 size = 1 )
	{
		m_msgShift += size;

		KeepMsgShiftInBounds( (Int32)m_factsToDisplay.Size() );
	}

	void ScroolMsgUp( Int32 size = 1 )
	{
		m_msgShift -= size;
		if ( m_msgShift < 0 ) m_msgShift = 0;
	}

	void KeepMsgShiftInBounds( Int32 maxSize )
	{
		if ( m_msgShift >= maxSize ) m_msgShift = maxSize - 1;
		if ( m_msgShift < 0 ) m_msgShift = 0;
	}

	void NextSubPage()
	{
		if ( ++m_subPageCurrent == m_subPageMax )
		{
			m_subPageCurrent = 0;
		}
	}

	void UpdateData()
	{
		m_factsToDisplay.Clear();

		CFactsDB *factsDB = GCommonGame->GetSystem< CFactsDB >();
		if ( factsDB == NULL ) return;
		TDynArray< String > factsIDs;
		factsDB->GetIDs( factsIDs );
		for ( Uint32 i = 0; i < factsIDs.Size(); ++i )
		{
			TDynArray< const CFactsDB::Fact* > facts;
			factsDB->GetFacts( factsIDs[i], facts );

			String factValue;
			for ( Uint32 k = 0; k < facts.Size(); )
			{
				factValue += ToString( facts[k]->m_value );
				if ( ++k < facts.Size() )
				{
					factValue += TXT(" ; ");
				}
			}

			m_factsToDisplay.PushBack( factsIDs[i] + TXT(" = ") + factValue );
		}

		KeepMsgShiftInBounds( (Int32)m_factsToDisplay.Size() );
	}

	TDynArray< String > m_factsToDisplay;
};

void CreateDebugPageFactsDB()
{
	IDebugPage* page = new CDebugPageFactsDB();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
