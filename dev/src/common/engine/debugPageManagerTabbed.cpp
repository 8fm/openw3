/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPageManagerTabbed.h"
#include "viewport.h"
#include "renderFrame.h"
#include "inputBufferedInputEvent.h"
#include "rawInputManager.h"

#ifndef NO_DEBUG_PAGES

CDebugPageManagerTabbed::CDebugPageManagerTabbed( CGatheredResource& fontResource )
:	IDebugPageManagerBase( fontResource )
{

}

CDebugPageManagerTabbed::~CDebugPageManagerTabbed()
{

}

void CDebugPageManagerTabbed::OnTick( Float timeDelta )
{
	IDebugPageManagerBase::OnTick( timeDelta );
}

Bool CDebugPageManagerTabbed::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( 
			( key == IK_F4 && action == IACT_Press )
		|| 	( ( RIM_IS_KEY_DOWN( IK_Pad_RightShoulder ) || RIM_IS_KEY_DOWN( IK_Pad_LeftShoulder ) ) && ( key == IK_Pad_Start || key == IK_Pad_DigitRight ) && action == IACT_Press ) 
		)
	{
		if ( m_debugPages.Size() )
		{
			// Go to next page
			Int32 index = static_cast< Int32 >( m_debugPages.GetIndex( m_activePage ) );
			index = ( index + 1 ) % m_debugPages.Size();

			// Get page to select
			IDebugPage* pageToSelect = m_debugPages[ index ];
			SelectDebugPage( pageToSelect );
		}

		return true;
	}
	else if ( 
			( key == IK_F3 && action == IACT_Press )
		||	( ( RIM_IS_KEY_DOWN( IK_Pad_RightShoulder ) || RIM_IS_KEY_DOWN( IK_Pad_LeftShoulder ) ) && ( key == IK_Pad_Back_Select || key == IK_Pad_DigitLeft ) && action == IACT_Press ) 
		)
	{	
		if ( m_debugPages.Size() )
		{
			// Go to previous page
			Int32 index = static_cast< Int32 >( m_debugPages.GetIndex( m_activePage ) - 1 );
			if ( index < 0 ) index = m_debugPages.Size() - 1; 

			// Get page to select
			IDebugPage* pageToSelect = m_debugPages[ index ];
			SelectDebugPage( pageToSelect );
		}
		return true;
	}
	else if ( m_activePage != NULL && key == IK_Escape && action == IACT_Press )
	{
		SelectDebugPage( NULL );
		return true;
	}

	return IDebugPageManagerBase::OnViewportInput( view, key, action, data );
}

void CDebugPageManagerTabbed::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_activePage != NULL && !m_activePage->FullScreen() )
	{
		static const Uint32 PAGE_PER_LINE = 13;

		// Generate 'task bar'
		Uint32 taskBarDebugPageWidth = (view->GetWidth() - 110) / PAGE_PER_LINE;

		// Draw debug page task bar
		Uint32 line = 0;
		for ( Uint32 i = 0; i < m_debugPages.Size() -1; ++i )
		{
			if ( i != 0 && i % PAGE_PER_LINE == 0 )
			{
				++line;
			}

			Uint32 x = 55 + taskBarDebugPageWidth * ( i - line * PAGE_PER_LINE );
			Uint32 y = view->GetHeight() - 65 + line * 15;

			// Button
			frame->AddDebugRect( x + 1, y + 1, taskBarDebugPageWidth -2, 13, (m_debugPages[i+1] != m_activePage) ? Color(0,0,0,192) : Color(0,0,96,192) );
			frame->AddDebugFrame( x, y, taskBarDebugPageWidth, 15, Color(255,255,255,127) );

			// Caption
			const Char* pageName = m_debugPages[i+1]->GetPageName().AsChar();
			frame->AddDebugScreenFormatedText( x + 5, y + 10, (m_debugPages[i+1] != m_activePage) ? Color::WHITE : Color::GREEN, TXT("%ls"), pageName );		
		}
	}

	IDebugPageManagerBase::OnViewportGenerateFragments( view, frame );
}

#endif
