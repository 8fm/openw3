/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "debugWindowsManager.h"
#include "../core/feedback.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "renderFrame.h"


/// Debug page with performance on screen log
class CDebugPagePerformanceLog : public IDebugPage, public IOnScreenLog
{
protected:
	struct ScreenLogMessage
	{
		Float		m_lifeLeft;		//!< Live left to be displayed
		Color		m_color;		//!< Color of the message
		String		m_col0;			//!< Column 0
		String		m_col1;			//!< Column 1
		String		m_message;		//!< Message itself

		ScreenLogMessage( Float life, const Color& color )
			: m_lifeLeft( life )
			, m_color( color )
		{};
	};

protected:
	IOnScreenLog*						m_prevLog;
	TDynArray< ScreenLogMessage* >		m_onScreenMessages;
	Uint32								m_perfMessageCounter;
	Red::Threads::CMutex				m_accessLock;
	Bool								m_usePerfLog;

public:
	CDebugPagePerformanceLog()
		: IDebugPage( TXT("Performance Log") )
		, m_usePerfLog( false )
	{
		// Install this page as the on screen log
		m_prevLog = GScreenLog;
		GScreenLog = this;
	};

	~CDebugPagePerformanceLog()
	{
		// Restore previous log
		GScreenLog = m_prevLog;
	}

	virtual void ClearPerfWarnings()
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );
		m_onScreenMessages.ClearPtr();
	}

	virtual void PerfWarning( Float timeTook, const String& group, const Char* info, ... )
	{
		// Update only if active
		if ( m_usePerfLog )
		{
			// Format text
			va_list arglist;
			va_start( arglist, info );
			Char formattedMessage[2048];
			Red::System::VSNPrintF( formattedMessage, ARRAY_COUNT(formattedMessage), info, arglist );

			// Setup color
			Color color = Color( 200, 255, 100 );
			if ( timeTook > 0.005f ) color = Color( 255, 255, 100 );
			if ( timeTook > 0.030f ) color = Color( 255, 100, 100 );
			if ( timeTook > 0.100f ) color = Color( 255, 100, 255 );

			// Performance warnings stays on the screen for 5s
			ScreenLogMessage* msg = new ScreenLogMessage( 7.0f, color );
			msg->m_col0 = String::Printf( TXT("%1.2fms"), timeTook * 1000.0f );
			msg->m_col1 = group;
			msg->m_message = formattedMessage;

			// Add to list
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );
			m_onScreenMessages.PushBack( msg );
		}
	}

	virtual void OnTick( Float timeDelta )
	{
		// Advance timers
		TDynArray< ScreenLogMessage* > msgList = m_onScreenMessages;
		for ( Uint32 i=0; i<msgList.Size(); i++ )
		{
			ScreenLogMessage* msg = msgList[i];
			msg->m_lifeLeft -= timeDelta;
			if ( msg->m_lifeLeft < 0.0f )
			{
				m_onScreenMessages.Remove( msg );
				delete msg;
			}
		}
	}

	virtual void OnPageShown() 
	{
		IDebugPage::OnPageShown();
		ClearPerfWarnings();
		m_usePerfLog = true;
	}

	virtual void OnPageHidden() 
	{
		IDebugPage::OnPageHidden();
		ClearPerfWarnings();
		m_usePerfLog = false;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_accessLock );

		// Header
		Int32 y = 65;
		frame->AddDebugScreenText( 55, y, TXT("Performance issues:") );
		y += 15;

		// List
		Uint32 xCol0 = 40;				// Index
		Uint32 xCol1 = xCol0 + 40;		// Category
		Uint32 xCol2 = xCol1 + 100;		// Text
		Int32 maxCount = 30;
		for ( Int32 i=(Int32)m_onScreenMessages.Size()-1; i>=0 && maxCount>0; --i, --maxCount )
		{
			// Render message
			ScreenLogMessage* msg = m_onScreenMessages[i];
			frame->AddDebugScreenText( xCol0, y, msg->m_col0, msg->m_color );
			frame->AddDebugScreenText( xCol1, y, msg->m_col1, msg->m_color );
			frame->AddDebugScreenText( xCol2, y, msg->m_message, msg->m_color );

			// Move down
			y += 15;
		}
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_PerformanceLog );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
		return false;
	}
};

void CreateDebugPagePerformanceLog()
{
	IDebugPage* page = new CDebugPagePerformanceLog();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif