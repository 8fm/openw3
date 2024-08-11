/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif
#include "debugPage.h"
#include "debugCounter.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "renderFrame.h"
#include "baseEngine.h"


#ifndef NO_DEBUG_PAGES

/// Debug page with general performance counters
class CDebugPageDebugCounters : public IDebugPage
{
public:
	CDebugPageDebugCounters()
		: IDebugPage( TXT("Budget counters") )
	{};

	void DrawCounter( CRenderFrame* frame, Int32 x, Uint32& y, Uint32 width, Uint32 height, IDebugCounter* counter )
	{
		if ( width < 3 )
		{
			width = 3;
		}

		if ( height < 3 )
		{
			height = 3;
		}

		// Get the color
		Color color = counter->GetColor();
		const Float prc = counter->GetCurrent() / counter->GetMax();
		DrawProgressBar( frame, x, y, width, height, prc, color );

		// Draw text
		frame->AddDebugScreenText( x+5, y+10, counter->GetText() );

		// Over budget :)
		if ( prc > 1.0f )
		{
			// Draw text
			frame->AddDebugScreenText( x + 5 + width, y+10, TXT("OVER BUDGET!"), Color::RED );
		}
	}

	//! Generate debug viewport fragments
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

		const Uint32 width = 300;
		const Uint32 height = 15;
		Uint32 y = 65;

		// Get debug counters
		const TDynArray< IDebugCounter* >& counters = GEngine->GetDebugCounters();
		for ( Uint32 i=0; i<counters.Size(); i++ )
		{
			DrawCounter( frame, 55, y, width, height, counters[i] );
			y += height + 5;
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
		return false;
	}
};

void CreateDebugPageBudget()
{
	IDebugPage* page = new CDebugPageDebugCounters();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif