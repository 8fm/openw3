/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "debugCheckBox.h"
#include "asyncAnimTickManager.h"
#include "animationManager.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

class CDebugPageBgObjects : public IDebugPage
{
	CDebugOptionsTree*					m_tree;
	Bool								m_showDetails;

public:
	CDebugPageBgObjects()
		: IDebugPage( TXT("Bg") )
		, m_tree( NULL )
		, m_showDetails( false )
	{
	};

	~CDebugPageBgObjects()
	{
		delete m_tree;
		m_tree = NULL;
	}

	virtual void OnTick( Float timeDelta )
	{
		if ( m_showDetails )
		{
			m_tree->OnTick( timeDelta );
		}
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 70, 120, Color(255, 0, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		CAsyncAnimTickManager::SDebugInfo info;
		GAnimationManager->Debug_GetAsynTickMgr()->Debug_GetInfo( info );

		const Uint32 width = 600;
		const Uint32 height = 50;
		Uint32 y = 100;
		const Uint32 x = 100;

		// BG
		frame->AddDebugRect( x - 1, y - 1, width + 2, height + 2, Color( 255, 255, 0, 128 ) );
		frame->AddDebugRect( x + 1, y + 1, width - 2, height - 1, Color( 0, 0, 0, 128 ) );

		Int32 currX = x + 1;

		{
			Float p = info.m_currTime / info.m_timeBudget;
			Int32 barWidth = Clamp< Int32 >( (Int32)( p* ( width - 2 ) ), 0, width - 2 );
			frame->AddDebugRect( currX, y,  barWidth, height, Color( 255, 255, 0, 128 ) );
		}

		y += height + 20;

		{
			frame->AddDebugScreenFormatedText( currX, y, TXT("Budget: %1.3f"), info.m_timeBudget );
			y += 15;

			if ( info.m_currTime > info.m_timeBudget )
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Real: %1.3f - OVERBUDGET"), info.m_currTime );
			}
			else
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Real: %1.3f"), info.m_currTime );
			}
			y += 15;

			if ( info.m_currTimeBudget < 1.f )
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Curr: %1.3f - OVERBUDGET"), info.m_currTimeBudget );
			}
			else
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Curr: %1.3f"), info.m_currTimeBudget );
			}
			y += 15;

			if ( info.m_syncTime < 1.f )
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Sync time: %1.3f (%1.3f)"), info.m_syncTime, info.m_syncTime - info.m_waitingTime );
			}
			else
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Sync time: %1.3f (%1.3f) - OVERBUDGET"), info.m_syncTime, info.m_syncTime - info.m_waitingTime );
			}
			y += 15;

			frame->AddDebugScreenFormatedText( currX, y, TXT("Waiting time: %1.3f"), info.m_waitingTime );
			y += 15;
		}

		y += 10;

		{
			frame->AddDebugScreenFormatedText( currX, y, TXT("Sync num: %d"), info.m_syncNum );
			y += 15;

			frame->AddDebugScreenFormatedText( currX, y, TXT("Async num: %d"), info.m_asyncNum );
			y += 15;

			if ( info.m_asyncRestNum == 0 )
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Async rest num: %d"), info.m_asyncRestNum );
			}
			else
			{
				frame->AddDebugScreenFormatedText( currX, y, TXT("Async rest num: %d - OVERBUDGET"), info.m_asyncRestNum );
			}
			y += 15;

			frame->AddDebugScreenFormatedText( currX, y, TXT("Buckets: %d"), info.m_bucketsNum );
			y += 15;

			frame->AddDebugScreenFormatedText( currX, y, TXT("Canceled: %d"), info.m_canceledObjects );
			y += 15;
		}

		y += 10;

		{
			{
				const TDynArray< CAsyncAnimTickManager::SDebugInfo::SBucketDebugInfo, MC_Debug >& biArr = info.m_bucketsInfo;

				Uint32 place = 0;
				Int32 tempX = 0;

				for ( Uint32 j=0; j<biArr.Size(); ++j )
				{
					const CAsyncAnimTickManager::SDebugInfo::SBucketDebugInfo& bucketInfo = biArr[ j ];

					if ( bucketInfo.m_size == 0 )
					{
						continue;
					}

					frame->AddDebugScreenFormatedText( tempX + currX, y, TXT("Time: %1.3f, Max: %1.3f, Size: %d"), bucketInfo.m_restTime, bucketInfo.m_maxTime, bucketInfo.m_size );
					
					place += 1;
					tempX += 200;

					if ( place == 4 )
					{
						tempX = 0;
						y += 15;
						place = 0;
					}
				}
			}
		}

		// Create tree
		if ( !m_tree )
		{
			/*const Uint32 width = frame->GetFrameInfo().GetWidth() - 100;
			const Uint32 height = frame->GetFrameInfo().GetHeight() - legendY - 80;

			legendY += 15;

			m_tree = new CDebugOptionsTree( 50, legendY, width, height, this );

			for ( Uint32 i=0; i<m_elems.Size(); ++i )
			{
				IDebugCheckBox* group = new IDebugCheckBox( NULL, GetName( i ), true, false );

				{
					const TDynArray< SFrameBudgetElem::SPerfCounterStatArray >& arrayAdd = m_elems[ i ].GetCountersAdd();
					const TDynArray< SFrameBudgetElem::SPerfCounterStatArray >& arrayRemove = m_elems[ i ].GetCountersRemove();

					for ( Uint32 j=0; j<arrayAdd.Size(); ++j )
					{
						new CDebugFrameBudgetElemBox( group, arrayAdd[ j ] );
					}
				}

				m_tree->AddRoot( group );
			}*/
		}

		// Render tree
		if ( m_showDetails )
		{
			m_tree->OnRender( frame );
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible(true);
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		if ( m_showDetails && m_tree && m_tree->OnInput( key, action, data ) )
		{
			return true;
		}
		else if ( ( key == IK_T && action == IACT_Press ) || ( key == IK_Pad_DigitRight && action == IACT_Press ) )
		{
			m_showDetails = !m_showDetails;
		}

		// Not processed
		return false;
	}
};

void CreateDebugPageBgObjects()
{
	IDebugPage* page = new CDebugPageBgObjects();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif