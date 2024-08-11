/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugChoice.h"
#include "inputKeys.h"
#include "inputBufferedInputEvent.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

IDebugChoice::IDebugChoice( IDebugCheckBox* parent, const String& name )
	: IDebugCheckBox( parent, name, false, false )
	, m_separation( 60 )
{

}

IDebugChoice::~IDebugChoice()
{

}

void IDebugChoice::OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
{
	//grab stats
	Bool isSelected = this == options.m_selected;

	String option = GetSelection();

	frame->AddDebugScreenText( x + m_separation, y, option, isSelected ? Color::YELLOW : Color::WHITE );
	
	IDebugCheckBox::OnRender( frame, x, y, counter, options );
}

Bool IDebugChoice::OnInput( enum EInputKey key, enum EInputAction action, Float data )
{
	// Navigate
	if ( ( key == IK_Right || key == IK_Pad_DigitRight ) && action == IACT_Press )
	{
		OnNext();
		return true;
	}
	else if ( ( key == IK_Left || key == IK_Pad_DigitLeft ) && action == IACT_Press )
	{
		OnPrev();
		return true;
	}

	if ( IDebugCheckBox::OnInput( key, action, data ) )
	{
		return true;
	}

	// Not handled
	return false;
}

//////////////////////////////////////////////////////////////////////////

CDebugStringChoice::CDebugStringChoice( IDebugCheckBox* parent, const String& name, const TDynArray< String, MC_Debug >& list )
	: IDebugChoice( parent, name )
	, m_list( list )
	, m_index( 0 )
{
	ASSERT( m_list.Size() > 0 );
}

Bool CDebugStringChoice::SetSelection( const String& str )
{
	const Uint32 size = m_list.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( str == m_list[ i ] )
		{
			m_index = i;
			return true;
		}
	}
	return false;
}

void CDebugStringChoice::OnNext()
{
	if ( m_list.Size() == 0 )
	{
		return;
	}

	if ( m_index + 1 < m_list.Size() )
	{
		m_index += 1;
	}
	else
	{
		m_index = 0;
	}
}

void CDebugStringChoice::OnPrev()
{
	if ( m_list.Size() == 0 )
	{
		return;
	}

	if ( m_index - 1 > 0 )
	{
		m_index -= 1;
	}
	else
	{
		m_index = m_list.Size(); 
	}
}

String CDebugStringChoice::GetSelection() const
{
	if ( m_list.Size() == 0 )
	{
		return TXT("<Empty>");
	}

	return m_list[ m_index ];
}

#endif
