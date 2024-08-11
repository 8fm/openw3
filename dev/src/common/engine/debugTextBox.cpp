/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugTextBox.h"
#include "renderFrame.h"
#include "debugCheckBox.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"

#ifndef NO_DEBUG_PAGES

void CDebugTextBox::OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
{
	// Check if option fits in a frame
	if ( y > options.m_maxY )
	{
		return;
	}

	if ( counter++ >= options.m_startLine )
	{
		// Determine color
		Color color = Color::WHITE;
		const Bool isSelected = ( this == options.m_selected );
		CalculateColor( color, isSelected );
		frame->AddDebugScreenText( x+1, y+1, m_name, Color::BLACK );
		frame->AddDebugScreenText( x, y, m_name, color );

		// Draw text box
		const Uint32 boxPos = x + m_separation;
		frame->AddDebugFrame( boxPos, y-10, m_boxSize, 13, color );
		frame->AddDebugScreenText( boxPos+3, y+1, m_text.AsChar(), Color::BLACK );
		frame->AddDebugScreenText( boxPos+2, y, m_text.AsChar(), color );

		// Draw comment
		OnRenderComment( frame, x, y, color, options );

		// Advance a line
		y += LINE_HEIGHT;
	}
}

Bool CDebugTextBox::OnInput( enum EInputKey key, enum EInputAction action, Float data )
{
	if( action == IACT_Press)
	{
		if( key == IK_Backspace )
		{
			if( m_text.Size() > 1 )
			{
				// Pop the null character
				m_text.PopBackFast();

				// Insert a new one in new place
				Char* data = (Char*)m_text.Data();
				data[ m_text.Size() - 1 ] = 0;
			}
			return true;
		}

		Char charToInsert = 0;

		if( (key >= IK_0 && key <= IK_9) || (key >= IK_A && key <= IK_Z) )
		{
			charToInsert = (Char)key;;
		}
		else if( key == IK_Period )
		{
			charToInsert = TXT('.');
		}
		else if( key == IK_Space )
		{
			charToInsert = TXT(' ');
		}

		if( charToInsert )
		{
			m_text.Append( charToInsert );
			return true;
		}
	}

	return false;
}

#endif