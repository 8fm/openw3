/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "debugCheckBox.h"
#include "debugPage.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "renderFrame.h"

#ifndef NO_DEBUG_PAGES

const Uint32 IDebugCheckBox::LINE_HEIGHT = 14;

IDebugCheckBox::IDebugCheckBox( IDebugCheckBox* parent, const String& name, Bool canExpand, Bool canCheck )
	: m_parent( parent )
	, m_name( name )
	, m_canExpand( canExpand )
	, m_canCheck( canCheck )
	, m_isExpanded( false )
{
	// Add this to child list
	if ( m_parent )
	{
		m_parent->m_children.PushBack( this );
	}
}

void IDebugCheckBox::CalculateColor( Color& color, Bool isSelected )
{
	if ( isSelected )
	{
		color = Color::YELLOW;
	}
	else
	{
		color = Color::WHITE;
	}
}

void IDebugCheckBox::OnTick( Float timeDelta )
{
	for ( Uint32 i = 0; i < m_children.Size(); ++i )
	{
		IDebugCheckBox* child = m_children[i];
		child->OnTick( timeDelta );
	}	
}

Bool IDebugCheckBox::OnNotify( const Uint32 notificationCode, void* ptr )
{
	Bool ret = false;

	for ( Uint32 i = 0; i < m_children.Size(); ++i )
	{
		IDebugCheckBox* child = m_children[i];
		ret |= child->OnNotify( notificationCode, ptr );
	}	

	return ret;
}

void IDebugCheckBox::OnRenderComment( CRenderFrame* frame, Uint32 x, Uint32 y, const Color& color, const RenderOptions& options )
{

}

void IDebugCheckBox::OnRender( CRenderFrame* frame, Uint32 x, Uint32 &y, Uint32 &counter, const RenderOptions& options )
{
	// Check if option fits in a frame
	if ( y > options.m_maxY )
	{
		return;
	}

	// Get the special marker
	String marker = TXT("");
	if ( m_canExpand )
	{
		if ( m_isExpanded )
		{
			marker = TXT("-");
		}
		else
		{
			marker = TXT("+");
		}
	}
	else if ( m_canCheck && IsChecked() )
	{
		marker = TXT(">");
	}

	if ( counter++ >= options.m_startLine )
	{
		// Draw marker text
		Color markerColor = Color::WHITE;
		if ( m_canCheck && IsChecked() ) markerColor = Color::GREEN;
		frame->AddDebugScreenText( x-10+1, y+1, marker, Color::BLACK );
		frame->AddDebugScreenText( x-10, y, marker, markerColor );

		// Determine color
		Color color = Color::WHITE;
		const Bool isSelected = ( this == options.m_selected );
		CalculateColor( color, isSelected );
		frame->AddDebugScreenText( x+1, y+1, m_name, Color::BLACK );
		frame->AddDebugScreenText( x, y, m_name, color );

		// Draw comment
		OnRenderComment( frame, x, y, color, options );

		// Advance a line
		y += LINE_HEIGHT;

	}

	// Calculate layout for children
	if ( m_isExpanded )
	{
		for ( Uint32 i=0; i<m_children.Size(); i++ )
		{
			IDebugCheckBox* box = m_children[i];
			box->OnRender( frame, x+15, y, counter, options );
		}
	}
}

Bool IDebugCheckBox::OnInput( enum EInputKey key, enum EInputAction action, Float data )
{
	// Enter
	if ( action == IACT_Press )
	{
		switch(key)
		{
		case IK_Space:
			// Toggle
			if ( m_canCheck )
			{
				OnToggle();
				return true;
			}
			break;
		case IK_Enter:
		case IK_Pad_A_CROSS:
			// Expand/Collapse
			if ( m_canExpand )
			{
				Expand( !m_isExpanded );
				return true;
			}
			else if ( m_canCheck )
			{
				OnToggle();
				return true;
			}
			break;			
		}
	}
	else if ( action == IACT_Axis )
	{
		switch(key)
		{
		case IK_Pad_RightTrigger:
			// Toggle
			if ( m_canCheck )
			{
				OnToggle();
				return true;
			}
			break;			
		}
	}

	// Not handled
	return false;
}

IDebugCheckBox::~IDebugCheckBox()
{
	// Remove from parent
	if ( m_parent )
	{
		m_parent->m_children.Remove( this );
	}

	// Clear child list
	TDynArray< IDebugCheckBox* > options = m_children;
	m_children.Clear();

	// Delete local children
	for ( Uint32 i=0; i<options.Size(); i++ )
	{
		delete options[i];
	}
}

void IDebugCheckBox::Expand( Bool isExpanded )
{
	m_isExpanded = m_canExpand && isExpanded;
}

void IDebugCheckBox::Linearize( TDynArray< IDebugCheckBox* >& items )
{
	// Add this item
	items.PushBack( this );

	// Collect children
	if ( m_isExpanded )
	{
		for ( Uint32 i=0; i<m_children.Size(); i++ )
		{
			IDebugCheckBox* box = m_children[i];
			box->Linearize( items );
		}
	}
}

void IDebugCheckBox::Clear()
{
	// Clear child list
	TDynArray< IDebugCheckBox* > options = m_children;
	m_children.Clear();

	// Delete local children
	for ( Uint32 i=0; i<options.Size(); i++ )
	{
		delete options[i];
	}
}

IDebugCheckBox* IDebugCheckBox::FindChildByName( const String& name )
{
	for ( Uint32 i = 0 ; i < m_children.Size(); ++i )
	{
		if( m_children[i]->m_name == name )
		{
			return m_children[i];
		}
	}

	return NULL;
}

Uint32 IDebugCheckBox::GetNumLinear()
{
	Uint32 num = 1;

	if ( m_isExpanded )
	{
		for ( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			num += m_children[i]->GetNumLinear();
		}
	}

	return num;
}

void IDebugCheckBox::GetLinearIndex( IDebugCheckBox* d, Uint32 &index )
{
	++index;

	if ( d == this )
	{
		return;
	}
	
	if ( m_isExpanded )
	{
		for ( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			GetLinearIndex( m_children[i], index );
		}
	}

}

CDebugOptionsTree::CDebugOptionsTree( Uint32 x, Uint32 y, Uint32 width, Uint32 height, IDebugPage *page, Bool drawBackground )
	: m_active( NULL )
	, m_x( x )
	, m_y( y )
	, m_width( width )
	, m_height( height )
	, m_startLine( 0 )
	, m_parentPage( page )
	, m_drawBackground( drawBackground )
{
	Red::System::MemoryZero( &m_keyRepeaterInfo, sizeof( m_keyRepeaterInfo ) );
}


CDebugOptionsTree::~CDebugOptionsTree()
{
	Clear();
}

void CDebugOptionsTree::OnRender( CRenderFrame* frame )
{
	// Setup render options
	IDebugCheckBox::RenderOptions options;
	options.m_selected = m_active;
	options.m_maxX = m_x + m_width;
	options.m_maxY = m_y + m_height;
	options.m_startLine = m_startLine;	

	if( m_drawBackground )
	{
		frame->AddDebugRect( m_x, m_y, m_width, m_height, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( m_x, m_y, m_width, m_height, Color::WHITE );
		Uint32 num = GetNumLinear();
		Uint32 howMuchPerPage = ( m_height / IDebugCheckBox::LINE_HEIGHT );
		Float neededPages = (num>howMuchPerPage) ? ( (Float)num / (Float)howMuchPerPage ) : 1.0f;
		Uint32 width = (Uint32)(( m_height - 10 ) / neededPages);
		Float percent = (Float)m_startLine / (Float)( num );
		Uint32 startPoint = (Uint32)(( m_height - 10 ) * percent);
		frame->AddDebugFrame( m_x + 6, m_y + 5 + startPoint, 8, width, Color::WHITE );
		frame->AddDebugRect( m_x + 7, m_y + 6 + startPoint, 6, width - 2, Color( 0, 0, 255, 64 ) );
	}


	// Draw options
	Uint32 posY = m_y + 15;
	Uint32 counter = 0;

	for ( Uint32 i=0; i<m_roots.Size(); i++ )
	{
		// Render
		IDebugCheckBox* root = m_roots[i];
		root->OnRender( frame, m_x + 30, posY, counter, options );
		
		if ( posY > ( m_y + m_height ) )
		{
			break;
		}
	}
}

Bool CDebugOptionsTree::OnInput( enum EInputKey key, enum EInputAction action, Float data )
{
	// Pass to active item
	if ( m_active && m_active->OnInput( key, action, data ) )
	{
		// Linearize list
		TDynArray< IDebugCheckBox* > items;
		for ( Uint32 i=0; i<m_roots.Size(); i++ )
		{
			IDebugCheckBox* root = m_roots[i];
			root->Linearize( items );
		}

		// Navigate
		if ( !items.Empty() )
		{
			const Uint32 index = Clamp< Uint32 >( static_cast< Uint32 >( items.GetIndex( m_active ) ) + 1, 0, items.Size() - 1 );
			const Uint32 howMuch = ( m_height / IDebugCheckBox::LINE_HEIGHT ) - 2;

			if ( index <= m_startLine )
			{
				if ( m_startLine > 0 )
				{
					m_startLine = index;
				}
			}

			if ( index > ( m_startLine + howMuch ) )
			{
				m_startLine = index - howMuch;
			}
			if ( items.Size() < ( m_startLine + howMuch ) )
			{
				m_startLine = (items.Size() >= howMuch + 1) ? (items.Size() - howMuch - 1) : 0;
			}

		}

		return true;
	}

	// Go to previous/next item
	if ( key == IK_Pad_DigitDown || key == IK_Pad_DigitUp || key == IK_Up || key == IK_Down || key == IK_Left || key == IK_Right || key == IK_PageDown || key == IK_PageUp )
	{		
		// Linearize list
		TDynArray< IDebugCheckBox* > items;
		for ( Uint32 i=0; i<m_roots.Size(); i++ )
		{
			IDebugCheckBox* root = m_roots[i];
			root->Linearize( items );
		}

		// Navigate
		if ( !items.Empty() )
		{
			// Navigate
			if ( key == IK_Down || key == IK_Pad_DigitDown )
			{
				if ( action == IACT_Press )
				{
					const Uint32 index = Clamp< Uint32 >( static_cast< Uint32 >( items.GetIndex( m_active ) ) + 1, 0, items.Size() - 1 );
					SelectItem( items[ index ] );

					if ( index > ( m_startLine + ( m_height / IDebugCheckBox::LINE_HEIGHT ) - 2 ) )
					{
						++m_startLine;
					}
					if ( ! m_keyRepeaterInfo.m_keyIsPressed[0] )
					{
						m_keyRepeaterInfo.m_keyIsPressed[0] = true;
						m_keyRepeaterInfo.m_isKeyRepeated[0] = false;
						m_keyRepeaterInfo.m_startTime[0] = m_keyRepeaterInfo.m_Timer[0] = (Float) Red::System::Clock::GetInstance().GetTimer().GetSeconds();
					}
				}
				else
				{
					m_keyRepeaterInfo.m_keyIsPressed[0] = false;
				}

			}
			else if ( key == IK_Up || key == IK_Pad_DigitUp )
			{
				if ( action == IACT_Press )
				{
					if ( items.GetIndex( m_active ) != 0 )
					{
						const Uint32 index = Clamp< Uint32 >( static_cast< Uint32 >( items.GetIndex( m_active ) ) - 1, 0, items.Size() - 1 );
						SelectItem( items[ index ] );

						if ( index <= m_startLine )
						{
							if ( m_startLine > 0 )
							{
								--m_startLine;
							}
						}
					}
					if ( ! m_keyRepeaterInfo.m_keyIsPressed[1] )
					{
						m_keyRepeaterInfo.m_keyIsPressed[1] = true;
						m_keyRepeaterInfo.m_isKeyRepeated[1] = false;
						m_keyRepeaterInfo.m_startTime[1] = m_keyRepeaterInfo.m_Timer[1] = (Float) Red::System::Clock::GetInstance().GetTimer().GetSeconds();
					}
				}
				else
				{
					m_keyRepeaterInfo.m_keyIsPressed[1] = false;
				}
			}
			else if ( key == IK_PageUp )
			{
				if ( action == IACT_Press ) 
				{
					Uint32 howMuch = ( m_height / IDebugCheckBox::LINE_HEIGHT ) - 2;
					if ( m_active && ( static_cast< Uint32 >( items.GetIndex( m_active ) ) >= howMuch ) )
					{
						const Uint32 index = Clamp< Uint32 >( static_cast< Uint32 >( items.GetIndex( m_active ) ) - howMuch, 0, items.Size() - 1 );
						SelectItem( items[ index ] );

						if ( index <= m_startLine )
						{
							if ( m_startLine > 0 )
							{
								m_startLine = index;
							}
						}
					}
					else
					{
						if ( items.Size() > 0 )
						{
							SelectItem( items[ 0 ] );
							m_startLine = 0;
						}
					}
					if ( ! m_keyRepeaterInfo.m_keyIsPressed[3] )
					{
						m_keyRepeaterInfo.m_keyIsPressed[3] = true;
						m_keyRepeaterInfo.m_isKeyRepeated[3] = false;
						m_keyRepeaterInfo.m_startTime[3] = m_keyRepeaterInfo.m_Timer[3] = (Float) Red::System::Clock::GetInstance().GetTimer().GetSeconds();
					}
				}
				else
				{
					m_keyRepeaterInfo.m_keyIsPressed[3] = false;
				}
			}
			else if ( key == IK_PageDown )
			{
				if ( action == IACT_Press )
				{
					Uint32 howMuch = ( m_height / IDebugCheckBox::LINE_HEIGHT ) - 2;
					const Uint32 index = Clamp< Uint32 >( static_cast< Uint32 >( items.GetIndex( m_active ) ) + howMuch, 0, items.Size() - 1 );
					SelectItem( items[ index ] );

					if ( index > ( m_startLine + howMuch ) )
					{
						m_startLine = index - howMuch;
					}
					if ( ! m_keyRepeaterInfo.m_keyIsPressed[2] )
					{
						m_keyRepeaterInfo.m_keyIsPressed[2] = true;
						m_keyRepeaterInfo.m_isKeyRepeated[2] = false;
						m_keyRepeaterInfo.m_startTime[2] = m_keyRepeaterInfo.m_Timer[2] = (Float) Red::System::Clock::GetInstance().GetTimer().GetSeconds();
					}
				}
				else
				{
					m_keyRepeaterInfo.m_keyIsPressed[2] = false;
				}
			}
			else if ( (key == IK_Left) && (action == IACT_Press) && m_active )
			{
				if( m_active->GetParent() )
				{
					SelectItem( m_active->GetParent() );

					const Uint32 index = static_cast< Uint32 >( items.GetIndex( m_active ) );

					if ( index <= m_startLine )
					{
						if ( m_startLine > 0 )
						{
							m_startLine = index;
						}
					}
				}
			}
			else if ( (key == IK_Right) && (action == IACT_Press) && m_active )
			{
				if ( m_active->HasChildren() && m_active->IsExpanded() )
				{
					SelectItem( m_active->GetChildren()[0] );
				}
			}

			// Processed
			return true;
		}
	}

	// Not processed
	return false;
}

void CDebugOptionsTree::Clear()
{
	// Remove selection
	SelectItem( NULL );

	// Clear all items
	m_roots.ClearPtr();
}

void CDebugOptionsTree::AddRoot( IDebugCheckBox* root )
{
	if ( root )
	{
		m_roots.PushBack( root );
	}
}

void CDebugOptionsTree::SelectItem( IDebugCheckBox* item )
{
	// Deselect current one
	if ( m_active )
	{
		m_active->OnLostFocus();
	}

	// Set new one
	m_active = item;

	// Select new one
	if ( m_active )
	{
		m_active->OnGetFocus();
	}
}

Uint32 CDebugOptionsTree::GetNumLinear()
{
	Uint32 size = 1;
	for ( Uint32 i = 0; i < m_roots.Size(); ++i )
	{
		size += m_roots[i]->GetNumLinear();
	}
	return size;
}

void CDebugOptionsTree::GetLinearIndex( IDebugCheckBox* d, Uint32 &index )
{
	for ( Uint32 i = 0; i < m_roots.Size(); ++i )
	{
		m_roots[i]->GetLinearIndex( d, index );
	}
}

Bool CDebugOptionsTree::OnNotify( const Uint32 notificationCode, void* ptr )
{
	Bool ret = false;

	for ( Uint32 i = 0; i < m_roots.Size(); ++i )
	{
		ret |= m_roots[i]->OnNotify( notificationCode, ptr );
	}

	return ret;
}

void CDebugOptionsTree::OnTick( Float timeDelta )
{
	// 4 key repeaters
	for ( Uint32 i = 0; i < 4; ++i )
	{
		if ( m_keyRepeaterInfo.m_keyIsPressed[i])
		{
			m_keyRepeaterInfo.m_Timer[i] += timeDelta;
			if ( !m_keyRepeaterInfo.m_isKeyRepeated[i] && m_keyRepeaterInfo.m_Timer[i] - m_keyRepeaterInfo.m_startTime[i] > 0.2f )
			{
				m_keyRepeaterInfo.m_isKeyRepeated[i] = true;
				m_keyRepeaterInfo.m_startTime[i] = m_keyRepeaterInfo.m_Timer[i];
			}
			else if ( m_keyRepeaterInfo.m_isKeyRepeated[i] && m_keyRepeaterInfo.m_Timer[i] - m_keyRepeaterInfo.m_startTime[i] > 0.1f )
			{
				if( m_parentPage )
				{
					switch ( i )
					{
					case 0:
						{
							m_parentPage->OnViewportInput( NULL, IK_Down, IACT_Press, 0 );
							break;
						}
					case 1:
						{
							m_parentPage->OnViewportInput( NULL, IK_Up, IACT_Press, 0 );
							break;
						}
					case 2:
						{
							m_parentPage->OnViewportInput( NULL, IK_PageDown, IACT_Press, 0 );
							break;
						}
					case 3:
						{
							m_parentPage->OnViewportInput( NULL, IK_PageUp, IACT_Press, 0 );
							break;
						}
					}
					
				}
				m_keyRepeaterInfo.m_startTime[i] = m_keyRepeaterInfo.m_Timer[i];
			}
		}

	}

	for ( Uint32 i = 0; i < m_roots.Size(); ++i )
	{
		IDebugCheckBox* checkBox = m_roots[i];
		checkBox->OnTick( timeDelta );
	}
}

IDebugCheckBox* CDebugOptionsTree::FindRootByName( const String& name )
{
	for ( Uint32 i = 0; i < m_roots.Size(); ++i )
	{
		if ( m_roots[i]->GetName() == name )
		{
			return m_roots[i];
		}
	}

	return NULL;
}

#endif