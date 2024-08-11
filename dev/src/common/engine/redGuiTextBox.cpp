/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "../core/clipboardBase.h"
#include "redGuiTextBox.h"
#include "redGuiManager.h"
#include "redGuiPanel.h"
#include "redGuiScrollBar.h"
#include "redGuiMenuItem.h"
#include "redGuiMenu.h"
#include "fonts.h"
#include "inputKeys.h"

namespace RedGui
{
	namespace
	{
		const Uint32	GDefaultLineHeight = 18;
		const Uint32	GDefaultCursorHeight = 12;
		const Uint32	GScrollPageSize = 16;
		const Uint32	GScrollWheelValue = 50;
		const Float		GCursorBlinkTime = 0.5f;
		const Color		GBackground( 30, 30, 30, 255 );
		const Color		GDisabledBackground( 75, 75, 75, 255 );
		const Color		GActiveLine( 15, 15, 15, 255 );
		const Color		GBorder( 128, 128, 128, 255 );
		const Color		GSelection( 86, 156, 214, 100 );
		const Color		GDisabledSelection( 10, 10, 10, 150 );
	}

	CRedGuiTextBox::CRedGuiTextBox( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl( x, y, width, height )
		, m_croppClient( nullptr )
		, m_horizontalBar( nullptr )
		, m_verticalBar( nullptr )
		, m_contextMenu( nullptr )
		, m_selectionPosition(0,0)
		, m_cursorPosition(0, 0)
		, m_fullText( String::EMPTY )
		, m_multiLine(false)
		, m_readOnly(false)
		, m_firstVisibleLine( -1 )
		, m_lastVisibleLine( -1 )
		, m_verticalRange( 0 )
		, m_horizontalRange( 0 )
		, m_activeLineIndex( -1 )
	{
		SetBorderVisible( true );
		SetBackgroundColor( GBackground );
		SetPointer( MP_Text );
		SetNeedKeyFocus( true );
		GRedGui::GetInstance().EventTick.Bind( this, &CRedGuiTextBox::NotifyOnTick );

		// create scrollbars
		m_horizontalBar = new CRedGuiScrollBar( 0, height, width, 20, false );
		m_horizontalBar->EventScrollChangePosition.Bind( this, &CRedGuiTextBox::NotifyScrollChangePosition );
		m_horizontalBar->SetMinTrackSize( 20 );
		m_horizontalBar->SetDock( DOCK_Bottom );
		m_horizontalBar->SetVisible( false );
		AddChild( m_horizontalBar );

		m_verticalBar = new CRedGuiScrollBar( width, 0, 20, height );
		m_verticalBar->EventScrollChangePosition.Bind( this, &CRedGuiTextBox::NotifyScrollChangePosition );
		m_verticalBar->SetMinTrackSize( 20 );
		m_verticalBar->SetDock( DOCK_Right );
		m_verticalBar->SetVisible( false );
		AddChild( m_verticalBar );

		// create cropped client
		m_croppClient = new CRedGuiPanel( 0, 0, width, height );
		m_croppClient->SetBorderVisible( false );
		m_croppClient->SetBackgroundColor( Color::CLEAR );
		m_croppClient->SetForegroundColor( Color::CLEAR );
		m_croppClient->SetEnabled( false );
		m_croppClient->SetDock( DOCK_Fill );
		m_croppClient->SetPointer( MP_Text );
		AddChild( m_croppClient );

		CreateContextMenu();

		m_maxLineWidth = (Uint32)( m_croppClient->GetSize().X );
		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultLineHeight );
		m_firstLinePosition = Vector2( 0.0f, 0.0f );
		SetText( TXT("") );

		m_activeLineIndex = -1;
	}

	CRedGuiTextBox::~CRedGuiTextBox()
	{
		
	}

	void CRedGuiTextBox::OnPendingDestruction()
	{
		GRedGui::GetInstance().EventTick.Unbind(this, &CRedGuiTextBox::NotifyOnTick);

		m_horizontalBar->EventScrollChangePosition.Unbind( this, &CRedGuiTextBox::NotifyScrollChangePosition );
		m_verticalBar->EventScrollChangePosition.Unbind( this, &CRedGuiTextBox::NotifyScrollChangePosition );

		RED_LOG( RED_LOG_CHANNEL( RedGui ), m_fullText.AsChar() );
	}

	String CRedGuiTextBox::GetSelectedText() const
	{
		const Uint32 start = (Uint32)m_selectionPosition.X;
		const Uint32 count = (Uint32)( m_selectionPosition.Y - m_selectionPosition.X );
		return m_fullText.MidString( start, count );
	}

	String CRedGuiTextBox::GetText() const
	{
		return m_fullText;
	}

	void CRedGuiTextBox::SetText(const String& text)
	{
		m_fullText = text;

		if( m_passwordMode == true )
		{
			m_passwordMask = text;
			for( Char& letter : m_passwordMask )
			{
				letter = TXT('*');
			}
		}

		SplitTextToLines();
		CursorMoveEndOfLine( false );
	}

	void CRedGuiTextBox::SetReadOnly(Bool value)
	{
		m_readOnly = value;
	}

	Bool CRedGuiTextBox::GetReadOnly() const
	{
		return m_readOnly;
	}

	void CRedGuiTextBox::SetMultiLine( Bool value )
	{
		if( m_passwordMode == true )
		{
			m_multiLine = false;
			return;
		}

		if(m_multiLine == value)
		{
			return;
		}

		m_multiLine = value;
		SplitTextToLines();
	}

	Bool CRedGuiTextBox::GetMultiLine() const
	{
		return m_multiLine;
	}

	void CRedGuiTextBox::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		RED_UNUSED( eventPackage );

		if( GetVisible() == true && m_isActive == true )
		{
			static Float cursorTimer = 0.0f;
			cursorTimer += timeDelta;

			// the time between flashes of the cursor
			if(cursorTimer > GCursorBlinkTime)
			{
				m_cursorVisible = !m_cursorVisible;
				cursorTimer = 0.0f;
			}
		}
	}

	void CRedGuiTextBox::Draw()
	{
		PC_SCOPE( RedGuiTextBoxDraw );

		 GetTheme()->DrawPanel(this);

		 if( m_firstVisibleLine != -1 && m_lastVisibleLine != -1 )
		 {
			 GetTheme()->SetCroppedParent( m_croppClient );

			 // 
			 {
				 PC_SCOPE( RedGuiTextBoxDrawAllLines );

				 for( Uint32 i=m_firstVisibleLine; i<(Uint32)m_lastVisibleLine; ++i )
				 {
					 if( m_croppClient->GetEnabled() == true )
					 {
						 GetTheme()->DrawRawFilledRectangle( Vector2( m_croppClient->GetAbsoluteLeft() + m_firstLinePosition.X, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(i*GDefaultLineHeight) ), Vector2( (Float)m_maxLineWidth, (Float)GDefaultLineHeight ), ( GetEnabled() == true ) ? GBackground : GDisabledBackground );
					 }

					 if( (Int32)i == m_activeLineIndex )
					 {
						 GetTheme()->DrawRawFilledRectangle( Vector2( m_croppClient->GetAbsoluteLeft() + m_firstLinePosition.X, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(i*GDefaultLineHeight) ), Vector2( (Float)m_maxLineWidth, (Float)GDefaultLineHeight ), GActiveLine );
						 if( m_multiLine == true )
						 {
							 GetTheme()->DrawRawRectangle( Vector2( m_croppClient->GetAbsoluteLeft() + m_firstLinePosition.X, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(i*GDefaultLineHeight) ), Vector2( (Float)m_maxLineWidth, (Float)GDefaultLineHeight ), GBorder );
						 }
					 }

					 // draw text
					 DrawLineText( Vector2( m_croppClient->GetAbsoluteLeft() + m_firstLinePosition.X, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(i*GDefaultLineHeight) ), i );
				 }
			 }

			 // draw selection
			 DrawSelectionForLines();

			 // draw cursor
			 if( m_cursorVisible == true )
			 {
				 GetTheme()->DrawRawFilledRectangle( m_cursorPosition , Vector2( 1.0f, (Float)GDefaultLineHeight - 4.0f ), Color::WHITE );
			 }

			 GetTheme()->ResetCroppedParent();
		 }
	}

	void CRedGuiTextBox::DrawLineText( const Vector2& position, Uint32 lineIndex )
	{
		PC_SCOPE( RedGuiTextBoxDrawSingleLine );

		const SRedGuiTextLine* line = m_lines[lineIndex];

		// calculate horizontal position
		const Float horizontalDelta = m_croppClient->GetPosition().X;

		// calculate vertical position
		const Float verticalDelta = ( GDefaultLineHeight - 10 ) / 2.0f;		// 10 is default text height

		// compose final position
		const Vector2 finalPosition = position + Vector2( horizontalDelta, verticalDelta );


		const String& currentText = ( m_passwordMode == true ) ? m_passwordMask : m_fullText;

		if( ( line->m_startPosition + line->m_letterCount ) == currentText.GetLength() && lineIndex == ( m_lines.Size() - 1 ) )
		{
			GetTheme()->DrawRawText( finalPosition, currentText.MidString( line->m_startPosition, line->m_letterCount ), Color::WHITE );
		}
		else
		{
			GetTheme()->DrawRawText( finalPosition, currentText.MidString( line->m_startPosition, line->m_letterCount - 1 ), Color::WHITE );
		}
	}

	void CRedGuiTextBox::DrawSelectionForLines()
	{
		PC_SCOPE( RedGuiTextBoxDrawSelection );
		if( m_selectionPosition.X != m_selectionPosition.Y )
		{
			Uint32 selectedLetters = 0;
			const Uint32 selectionLength = (Uint32)( m_selectionPosition.Y - m_selectionPosition.X );

			for( Uint32 i=m_firstVisibleLine; i<(Uint32)m_lastVisibleLine; ++i )
			{
				SRedGuiTextLine* line = m_lines[i];

				if( (Int32)line->m_startPosition > (Int32)m_selectionPosition.Y || (Int32)( line->m_startPosition + line->m_letterCount ) < (Int32)m_selectionPosition.X )
				{
					continue;
				}

				const String& currentText = ( m_passwordMode == true ) ? m_passwordMask : m_fullText;

				// calculate start index
				Int32 startIndex = ( ( m_selectionPosition.X - line->m_startPosition ) > 0 ) ? ( (Int32)m_selectionPosition.X - line->m_startPosition ) : 0;
				Float startRectangle = GetTextSize( currentText.MidString( line->m_startPosition /*+ startIndex*/, startIndex ) ).X;

				// calculate end index
				Int32 count = selectionLength - selectedLetters;

				Int32 textCount = ( (Int32)( line->m_letterCount - startIndex ) > count ) ? count : line->m_letterCount - startIndex;
				String selectedText = currentText.MidString( line->m_startPosition + startIndex, textCount );
				Float widthRectangle = GetTextSize( selectedText ).X;

				if( selectedText.GetLength() > 0 )
				{
					Char sign = selectedText[ selectedText.Size() - 2 ];
					if( sign == TXT('\n') )
					{
						widthRectangle += 5;
					}
				}

				selectedLetters += textCount;

				GetTheme()->DrawRawFilledRectangle( Vector2( (Float)m_croppClient->GetAbsoluteLeft() + m_firstLinePosition.X + startRectangle, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(i*GDefaultLineHeight) ), Vector2( widthRectangle, (Float)GDefaultLineHeight ), ( m_isActive == true ) ? GSelection : GDisabledSelection );
			}
		}
	}

	Bool CRedGuiTextBox::ManageCursorPosition( enum EInputKey key, Bool shiftModificator )
	{
		if( key == IK_Left || key == IK_Pad_DigitLeft )
		{
			CursorMoveLeft( shiftModificator );
			return true;
		}
		else if( key == IK_Right || key == IK_Pad_DigitRight )
		{
			CursorMoveRight( shiftModificator );
			return true;
		}
		else if( key == IK_Down || key == IK_Pad_DigitDown )
		{
			CursorMoveDown( shiftModificator );
			return true;
		}
		else if( key == IK_Up || key == IK_Pad_DigitUp )
		{
			CursorMoveUp( shiftModificator );
			return true;
		}
		else if( key == IK_Home )
		{
			CursorMoveStartOfLine( shiftModificator );
			return true;
		}
		else if( key == IK_End )
		{
			CursorMoveEndOfLine( shiftModificator );
			return true;
		}
		else if( key == IK_PageDown )
		{
			if( m_multiLine == true )
			{
				m_verticalBar->MovePageDown();
				CursorGraphicToText();
			}
			return true;
		}
		else if( key == IK_PageUp )
		{
			if( m_multiLine == true )
			{
				m_verticalBar->MovePageUp();
				CursorGraphicToText();
			}
			return true;
		}

		return false;
	}

	void CRedGuiTextBox::CursorMoveUp( Bool shiftModificator )
	{
		if( (Uint32)m_cursorPosition.Y > GDefaultLineHeight )
		{
			m_cursorPosition.Y -= (Float)GDefaultLineHeight;

			CursorGraphicToText();
			UpdateSelection( shiftModificator );
		}
	}

	void CRedGuiTextBox::CursorMoveDown( Bool shiftModificator )
	{
		if( (Uint32)m_cursorPosition.Y - ( m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y ) < ( ( m_lines.Size() - 1 ) * GDefaultLineHeight ) )
		{
			m_cursorPosition.Y += (Float)GDefaultLineHeight;

			CursorGraphicToText();
			UpdateSelection( shiftModificator );
		}
	}

	void CRedGuiTextBox::CursorMoveLeft( Bool shiftModificator )
	{
		// remove selection and set cursor
		if( m_selectionPosition.X != m_selectionPosition.Y && shiftModificator == false )
		{
			m_cursorPositionInText = (Int32)m_selectionPosition.X;
		}
		else
		{
			// move cursor position
			if( m_cursorPositionInText > 0 )
			{
				--m_cursorPositionInText;
			}
			else
			{
				return;
			}
		}

		// update cursor
		CursorTextToGraphic();
		UpdateSelection( shiftModificator );
	}

	void CRedGuiTextBox::CursorMoveRight( Bool shiftModificator )
	{
		// remove selection and set cursor
		if(  m_selectionPosition.X != m_selectionPosition.Y && shiftModificator == false )
		{
			m_cursorPositionInText = (Int32)m_selectionPosition.Y;
		}
		else
		{
			// move cursor position
			if( m_cursorPositionInText < (Int32)m_fullText.GetLength() )
			{
				++m_cursorPositionInText;
			}
			else
			{
				return;
			}
		}

		// update cursor
		CursorTextToGraphic();
		UpdateSelection( shiftModificator );
	}

	void CRedGuiTextBox::CursorMoveEndOfLine( Bool shiftModificator )
	{
		if( m_activeLineIndex != -1 )
		{
			if( m_activeLineIndex == (Int32)( m_lines.Size() - 1 ) )
			{
				m_cursorPositionInText = m_lines[m_activeLineIndex]->m_startPosition + m_lines[m_activeLineIndex]->m_letterCount;
			}
			else
			{
				m_cursorPositionInText = m_lines[m_activeLineIndex]->m_startPosition + m_lines[m_activeLineIndex]->m_letterCount - 1;
			}
			m_horizontalBar->MoveScrollToEnd();

			CursorTextToGraphic();
			UpdateSelection( shiftModificator );
		}
	}

	void CRedGuiTextBox::CursorMoveStartOfLine( Bool shiftModificator )
	{
		if( m_activeLineIndex != -1 )
		{
			m_cursorPositionInText = m_lines[m_activeLineIndex]->m_startPosition;
			m_horizontalBar->MoveScrollToStart();

			CursorTextToGraphic();
			UpdateSelection( shiftModificator );
		}
	}

	void CRedGuiTextBox::InsertChar(Char text)
	{
		String lhs = m_fullText.LeftString( m_cursorPositionInText );
		lhs.Append(&text, 1);
		String rhs = m_fullText.RightString( m_fullText.GetLength() - m_cursorPositionInText );
		m_fullText = lhs.Append( rhs.AsChar(), rhs.GetLength() );

		// pasword mask
		if( m_passwordMode == true )
		{
			m_passwordMask.Append( TXT("*"), 1 );
		}

		SplitTextToLines();
		CursorMoveRight( false );
	}

	void CRedGuiTextBox::DeleteChar( Bool deleteBack )
	{
		if( (Int32)m_selectionPosition.X != (Int32)m_selectionPosition.Y )
		{
			// delete selected text
			DeleteSelectedText();
			return;
		}

		if(deleteBack == true)
		{
			if(m_cursorPositionInText > 0)
			{
				CursorMoveLeft( false );
				DeleteChar( false );
				return;
			}
		}
		else
		{
			if( m_fullText.GetLength() > 0 && m_cursorPositionInText >= 0 && m_cursorPositionInText < (Int32)m_fullText.GetLength() )
			{
				String lhs = m_fullText.LeftString( m_cursorPositionInText );
				String rhs = m_fullText.RightString( m_fullText.GetLength() - (m_cursorPositionInText + 1 ));
				m_fullText = lhs.Append( rhs.AsChar(), rhs.GetLength() );
				SplitTextToLines();
			}
		}

		// password mask
		if( m_passwordMode == true && m_passwordMask.Size() > 0 )
		{
			m_passwordMask.PopBack();
		}
	}

	void CRedGuiTextBox::TokenizeText( TDynArray< String >& tokens, Char separator /*= TXT('\n')*/ )
	{
		const Uint32 textLength = m_fullText.GetLength();

		if( textLength == 0 )
		{
			tokens.PushBack( TXT("") );
		}
		else
		{
			Uint32 lastIndex = 0;
			for( Uint32 i=0; i<textLength; ++i )
			{
				if( m_fullText[i] == separator )
				{
					tokens.PushBack( m_fullText.MidString( lastIndex, i - lastIndex ) );
					lastIndex = i+1;
				}
			}

			if( lastIndex != textLength )
			{
				tokens.PushBack( m_fullText.MidString( lastIndex, textLength - lastIndex ) );
			}
			else
			{
				Char mark = m_fullText[m_fullText.Size()-2];
				if( mark == TXT('\n') )
				{
					tokens.PushBack( TXT("") );
				}
			}
		}
	}
	
	void CRedGuiTextBox::SplitTextToLines()
	{
		m_lines.ClearPtr();
		m_maxLineWidth = (Uint32)( m_croppClient->GetSize().X );

		TDynArray< String > tokens;
		TokenizeText( tokens );
		Uint32 tokenCount = tokens.Size();

		if( m_multiLine == true )
		{
			Uint32 currentXPos = 0;

			for( Uint32 i=0; i<tokenCount; ++i )
			{
				const Float textWidth = GetTextSize( tokens[i] ).X;
				Uint32 textLength = tokens[i].GetLength() + ( i == ( tokenCount-1 ) ? 0 : 1 );	// this is new line mark \n
				m_lines.PushBack( new SRedGuiTextLine( currentXPos, textLength, textWidth ) );
				RecalculateMaxWidth( tokens[i] );
				currentXPos += textLength;
			}
		}
		else
		{
			const Float textWidth = GetTextSize( tokens[0] ).X;
			m_lines.PushBack( new SRedGuiTextLine( 0, tokens[0].GetLength(), textWidth ) );
			RecalculateMaxWidth( tokens[0] );			
			m_firstVisibleLine = 0;
			m_lastVisibleLine = 1;
		}

		UpdateView();
	}

	void CRedGuiTextBox::CreateContextMenu()
	{
		m_contextMenu = new CRedGuiMenu(0,0, 100, 0);
		m_contextMenu->SetVisible( false );
		m_contextMenu->SetCroppedParent( nullptr );
		m_contextMenu->AttachToLayer( TXT("Menus") );
		m_contextMenu->EventMenuItemSelected.Bind( this, &CRedGuiTextBox::NotifyEventButtonClickedContextMenu );

		m_contextMenu->AppendItem( TXT("Undo") );
		m_contextMenu->AppendItem( TXT("Redo") );
		m_contextMenu->AppendSeparator();
		m_contextMenu->AppendItem( TXT("Cut") );
		m_contextMenu->AppendItem( TXT("Copy") );
		m_contextMenu->AppendItem( TXT("Paste") );
		m_contextMenu->AppendItem( TXT("Delete") );
		m_contextMenu->AppendSeparator();
		m_contextMenu->AppendItem( TXT("Select all") );
	}

	void CRedGuiTextBox::NotifyEventButtonClickedContextMenu( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiMenuItem* menuItem )
	{
		if( menuItem->GetText() == TXT("Undo") )
		{
			// TODO
		}
		else if( menuItem->GetText() == TXT("Redo") )
		{
			// TODO
		}
		else if( menuItem->GetText() == TXT("Cut") )
		{
			CutText();
		}
		else if( menuItem->GetText() == TXT("Copy") )
		{
			CopyText();
		}
		else if( menuItem->GetText() == TXT("Paste") )
		{
			PasteText();
		}
		else if( menuItem->GetText() == TXT("Delete") )
		{
			DeleteSelectedText();
		}
		else if( menuItem->GetText() == TXT("Select all") )
		{
			SelectAll();
		}
	}

	void CRedGuiTextBox::CopyText()
	{
		if( m_passwordMode == false )
		{
			// get text from clipboard
			if( GClipboard->Open(CF_Text) == false)
			{
				return;
			}

			String text = m_fullText.MidString( (size_t)m_selectionPosition.X, (size_t)(m_selectionPosition.Y - m_selectionPosition.X) );

			GClipboard->Copy( text );
			GClipboard->Close();
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Copy operation is unavailable for password mode"), RedGui::MESSAGEBOX_Warning );
		}
	}

	void CRedGuiTextBox::PasteText()
	{
		if( m_passwordMode == false )
		{
			// read-only protection
			if(m_readOnly == true)
			{
				return;
			}

			DeleteSelectedText();

			// get text from clipboard
			if( GClipboard->Open(CF_Text) == false)
			{
				return;
			}

			String text = String::EMPTY;
			GClipboard->Paste( text );
			GClipboard->Close();

			String lhs = m_fullText.LeftString( m_cursorPositionInText );
			lhs.Append(text.AsChar(), text.GetLength() );
			String rhs = m_fullText.RightString( m_fullText.GetLength() - m_cursorPositionInText );
			m_fullText = lhs.Append( rhs.AsChar(), rhs.GetLength() );

			SplitTextToLines();

			// move cursor position to end of paste text
			m_cursorPositionInText += text.GetLength();
			CursorTextToGraphic();
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Paste operation is unavailable for password mode"), RedGui::MESSAGEBOX_Warning );
		}
	}

	void CRedGuiTextBox::CutText()
	{
		if( m_passwordMode == false )
		{
			// read-only protection
			if(m_readOnly == true)
			{
				return;
			}

			CopyText();
			DeleteSelectedText();

			SplitTextToLines();
		}
		else
		{
			GRedGui::GetInstance().MessageBox( TXT("Cut operation is unavailable for password mode"), RedGui::MESSAGEBOX_Warning );
		}
	}

	void CRedGuiTextBox::DeleteSelectedText()
	{
		// read-only protection
		if(m_readOnly == true)
		{
			return;
		}

		String lhs = m_fullText.LeftString( (size_t)m_selectionPosition.X );
		String rhs = m_fullText.RightString( m_fullText.GetLength() - ( (size_t)m_selectionPosition.Y ));
		m_fullText = lhs.Append( rhs.AsChar(), rhs.GetLength() );

		// password mask
		if( m_passwordMode == true )
		{
			String lhs = m_passwordMask.LeftString( (size_t)m_selectionPosition.X );
			String rhs = m_passwordMask.RightString( m_passwordMask.GetLength() - ( (size_t)m_selectionPosition.Y ));
			m_passwordMask = lhs.Append( rhs.AsChar(), rhs.GetLength() );
		}

		m_cursorPositionInText = (Uint32)m_selectionPosition.X;
		SplitTextToLines();
		CursorTextToGraphic();		
		ResetSelection();
	}

	void CRedGuiTextBox::SelectAll()
	{
		m_selectionPosition.Set( 0, (Float)m_fullText.GetLength() );

		m_cursorPositionInText = (Uint32)m_selectionPosition.Y;
		CursorTextToGraphic();
	}

	void CRedGuiTextBox::NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_verticalBar )
		{
			Vector2 point = m_firstLinePosition;
			point.Y = -(Float)value;
			m_firstLinePosition = point;
		}
		else if( sender == m_horizontalBar )
		{
			Vector2 point = m_firstLinePosition;
			point.X = -(Float)value;
			m_firstLinePosition = point;
		}

		UpdateView();
	}

	Int32 CRedGuiTextBox::CheckPoint( const Vector2& position )
	{
		const Vector2 relativePosition = position - m_croppClient->GetAbsolutePosition();
		Uint32 deltaPosition = (Uint32)-m_firstLinePosition.Y;
		Uint32 visibleItemIndex = (Uint32)( ( relativePosition.Y + deltaPosition ) / (Float)GDefaultLineHeight );

		if( visibleItemIndex >= m_lines.Size() )
		{
			if( m_lines.Size() > 0 )
			{
				// if someone click in the text area but not on the line, choose the last line in text area
				return m_lines.Size() - 1;
			}
			return -1;
		}

		return visibleItemIndex;
	}

	Vector2 CRedGuiTextBox::GetTextSize( const String& text )
	{
		CFont* font = GetFont();
		if(font == nullptr)
		{
			return Vector2( 0.0f, 0.0f );
		}

		Int32 unusedX, unusedY;
		Uint32 textWidth, textHeight;
		font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

		return Vector2( (Float)textWidth, (Float)textHeight );
	}

	void CRedGuiTextBox::UpdateView()
	{
		UpdateScrollSize();
		UpdateScrollPosition();
	}

	void CRedGuiTextBox::UpdateScrollPosition()
	{
		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstLinePosition;
			Int32 offset = -(Int32)position.Y;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}

				// calculate first and last render item
				m_firstLinePosition = position;
			}
		}
		else if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstLinePosition;
			Int32 offset = -(Int32)position.X;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				// calculate first and last render item
				m_firstLinePosition = position;
			}
		}

		// set current visible items
		Uint32 invisibleTop = (Uint32)( (-m_firstLinePosition.Y) / GDefaultLineHeight );
		Uint32 invisibleBottom = invisibleTop + m_maxVerticalItemCount + 1;

		m_firstVisibleLine = ( invisibleTop == 0 ) ? 0 : invisibleTop - 1;
		m_lastVisibleLine = ( invisibleBottom > m_lines.Size() ) ? m_lines.Size() : invisibleBottom;

		m_cursorPosition = Vector2( m_cursorPosition.X, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(m_activeLineIndex*GDefaultLineHeight) + 2.0f );
	}

	void CRedGuiTextBox::UpdateScrollSize()
	{
		Vector2 viewSize = m_croppClient->GetSize();
		Vector2 contentSize = Vector2( (Float)m_maxLineWidth, (Float)( m_lines.Size() * GDefaultLineHeight ) );

		if( m_multiLine == true )
		{
			// horizontal content doesn't fit
			if(contentSize.Y > viewSize.Y)
			{
				if( m_verticalBar != nullptr )
				{
					if( m_verticalBar->GetVisible() == false )
					{
						m_verticalBar->SetVisible( true );

						if(m_horizontalBar != nullptr)
						{
							// recalculate horizontal bar after add vertical bar
							if( ( contentSize.X > viewSize.X ) && ( m_horizontalBar->GetVisible() == false ) )
							{
								m_horizontalBar->SetVisible( true );
							}
						}
					}
				}
			}
			else
			{
				if( m_verticalBar != nullptr )
				{
					if( m_verticalBar->GetVisible() == true )
					{
						m_verticalBar->SetVisible( false );

						if( m_horizontalBar != nullptr )
						{
							// recalculate horizontal bar after remove vertical bar
							if( ( contentSize.X <= viewSize.X ) && ( m_horizontalBar->GetVisible() == true ) )
							{
								m_horizontalBar->SetVisible( false );
							}
						}
					}
				}
			}

			// vertical content doesn't fit
			if( contentSize.X > viewSize.X )
			{
				if( m_horizontalBar != nullptr )
				{
					if( m_horizontalBar->GetVisible() == false )
					{
						m_horizontalBar->SetVisible( true );

						if( m_verticalBar != nullptr )
						{
							// recalculate vertical bar after add horizontal bar
							if( ( contentSize.Y > viewSize.Y ) && ( m_verticalBar->GetVisible() == false ) )
							{
								m_verticalBar->SetVisible( true );
							}
						}
					}
				}
			}
			else
			{
				if( m_horizontalBar != nullptr )
				{
					if( m_horizontalBar->GetVisible() == true )
					{
						m_horizontalBar->SetVisible( false );

						if( m_verticalBar != nullptr )
						{
							// recalculate vertical bar after remove horizontal bar
							if( ( contentSize.Y <= viewSize.Y ) && ( m_verticalBar->GetVisible() == true ) )
							{
								m_verticalBar->SetVisible( false );
							}
						}
					}
				}
			}
		}

		// calculate ranges
		m_verticalRange = ( viewSize.Y >= contentSize.Y ) ? 0 : (Uint32)( contentSize.Y - viewSize.Y );
		m_horizontalRange = ( viewSize.X >= contentSize.X ) ? 0 : (Uint32)( contentSize.X - viewSize.X );

		// set new values
		if( m_verticalBar != nullptr )
		{
			m_verticalBar->SetScrollPage( GScrollPageSize );
			m_verticalBar->SetScrollRange( m_verticalRange + 1 );
			if( contentSize.Y > 0 )
			{
				m_verticalBar->SetTrackSize( (Int32)( (Float)( m_verticalBar->GetLineSize() * viewSize.Y ) / (Float)( contentSize.Y ) ) );
			}
		}
		if( m_horizontalBar != nullptr )
		{
			m_horizontalBar->SetScrollPage( GScrollPageSize );
			m_horizontalBar->SetScrollRange( m_horizontalRange + 1 );
			if( contentSize.X > 0 )
			{
				m_horizontalBar->SetTrackSize( (Int32)( (Float)( m_horizontalBar->GetLineSize() * viewSize.X ) / (Float)( contentSize.X ) ) );
			}
		}
	}

	void CRedGuiTextBox::RecalculateMaxWidth( const String& newText )
	{
		m_maxLineWidth = Max< Uint32 >( (Uint32)GetTextSize( newText ).X, m_maxLineWidth );
	}

	void CRedGuiTextBox::RecalculateCursorPosition( const Vector2& mousePosition )
	{
		Int32 indexInFullText = -1;
		const String& currentText = ( m_passwordMode == true ) ? m_passwordMask : m_fullText;

		if( m_activeLineIndex != -1 )
		{
			SRedGuiTextLine* line = m_lines[m_activeLineIndex];

			// delta between mouse position and start line position
			Int32 offsetFromLeft = Max<Int32>( 0, (Int32)( mousePosition.X - m_firstLinePosition.X - m_croppClient->GetAbsoluteLeft() ) );	
			String substring = currentText.MidString( line->m_startPosition, line->m_letterCount );
			Int32 substringWidth = (Int32)GetTextSize( substring ).X;

			if( offsetFromLeft < substringWidth )	// clicked on the text in line
			{
				if( line->m_letterCount > 0 )
				{
					Int32 textWidth = 0;
					for( Uint32 i=0; i<line->m_letterCount; ++i )
					{
						String substring = currentText.MidString( line->m_startPosition, i );
						textWidth = (Int32)GetTextSize( substring ).X;
						if( textWidth > offsetFromLeft )
						{
							indexInFullText = line->m_startPosition + i - 1;
							break;
						}
					}
				}
				else
				{
					indexInFullText = line->m_startPosition;
				}
			}
			else	// clicked on the empty place in line
			{
				if( line->m_letterCount > 0 )
				{
					// special case
					if( m_activeLineIndex == (Int32)( m_lines.Size() - 1 ) )
					{
						indexInFullText = line->m_startPosition + line->m_letterCount;
					}
					else
					{
						indexInFullText = line->m_startPosition + line->m_letterCount - 1;
					}
				}
				else
				{
					indexInFullText = line->m_startPosition;
				}
			}
		}
		
		if( indexInFullText != -1 )
		{
			m_cursorPositionInText = indexInFullText;
		}
		CursorTextToGraphic();
	}

	void CRedGuiTextBox::CursorGraphicToText()
	{
		Int32 lineIndex = (Int32)( m_cursorPosition.Y - ( + m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y ) ) / GDefaultLineHeight;
		if( lineIndex < 0 || lineIndex > (Int32)m_lines.Size() - 1 )
		{
			CursorTextToGraphic();
		}
		else
		{
			Int32 indexInFullText = -1;

			m_activeLineIndex = lineIndex;

			SRedGuiTextLine* line = m_lines[m_activeLineIndex];
			Int32 offsetFromLeft = (Int32)m_cursorPosition.X - ( (Int32)m_firstLinePosition.X + m_croppClient->GetAbsoluteLeft() );
			String substring = m_fullText.MidString( line->m_startPosition, line->m_letterCount );
			Int32 substringWidth = (Int32)GetTextSize( substring ).X;

			if( offsetFromLeft < substringWidth )	// clicked on the text in line
			{
				if( line->m_letterCount > 0 )
				{
					Int32 textWidth = 0;
					Uint32 letterCount = line->m_letterCount;
					if( m_activeLineIndex == (Int32)( m_lines.Size() - 1 ) )
					{
						++letterCount;
					}

					for( Uint32 i=0; i<letterCount; ++i )
					{
						String substring = m_fullText.MidString( line->m_startPosition, i );
						textWidth = (Int32)GetTextSize( substring ).X;
						if( textWidth > offsetFromLeft )
						{
							indexInFullText = line->m_startPosition + i - 1;
							break;
						}
					}
				}
				else
				{
					indexInFullText = line->m_startPosition;
				}
			}
			else	// clicked on the empty place in line
			{
				if( line->m_letterCount > 0 && ( m_activeLineIndex != (Int32)( m_lines.Size() - 1 ) ) )
				{
					indexInFullText = line->m_startPosition + line->m_letterCount - 1;
				}
				else
				{
					indexInFullText = line->m_startPosition + line->m_letterCount;
				}
			}

			m_cursorPositionInText = indexInFullText;
			CursorTextToGraphic();
		}
	}

	void CRedGuiTextBox::CursorTextToGraphic()
	{
		Uint32 letterCount = 0;
		const Uint32 lineCount = m_lines.Size();
		Bool setCursor = false;
		for( Uint32 i=0; i<lineCount; ++i )
		{
			letterCount += m_lines[i]->m_letterCount;
			if( (Int32)letterCount > m_cursorPositionInText )
			{
				SRedGuiTextLine* line = m_lines[i];
				Uint32 textWidth = 0;

				m_activeLineIndex = i;
				const Uint32 count = m_cursorPositionInText - line->m_startPosition;
				const String& currentText = ( m_passwordMode == true ) ? m_passwordMask : m_fullText;
				textWidth = (Uint32)GetTextSize( currentText.MidString( line->m_startPosition, count ) ).X;
				m_cursorPosition = Vector2( (Float)m_firstLinePosition.X + m_croppClient->GetAbsoluteLeft() + textWidth, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(m_activeLineIndex*GDefaultLineHeight) + 2.0f );
				setCursor = true;
				break;
			}
		}

		if( setCursor == false )
		{
			m_activeLineIndex = lineCount - 1;

			SRedGuiTextLine* line = m_lines[m_activeLineIndex];
			const Uint32 count = m_cursorPositionInText - line->m_startPosition;
			const String& currentText = ( m_passwordMode == true ) ? m_passwordMask : m_fullText;
			Uint32 textWidth = (Uint32)GetTextSize( currentText.MidString( line->m_startPosition, count ) ).X;
			m_cursorPosition = Vector2( (Float)m_firstLinePosition.X + m_croppClient->GetAbsoluteLeft() + textWidth, m_croppClient->GetAbsoluteTop() + m_firstLinePosition.Y + (Float)(m_activeLineIndex*GDefaultLineHeight) + 2.0f );
		}

		AdjustAreaToCursor();
	}

	void CRedGuiTextBox::SetEnabled( Bool value )
	{
		m_croppClient->SetEnabled( value );

		if( value == true )
		{
			SetBackgroundColor( GBackground );
		}
		else
		{
			m_activeLineIndex = -1;
			SetBackgroundColor( GDisabledBackground );
		}
	}

	void CRedGuiTextBox::AdjustAreaToCursor()
	{
		// from left
		if( m_cursorPosition.X < m_croppClient->GetAbsoluteLeft() )
		{
			m_horizontalBar->MoveScrollToEndSide();
		}

		// from right
		if( m_cursorPosition.X > m_croppClient->GetAbsoluteLeft() + m_croppClient->GetWidth() )
		{
			m_horizontalBar->MoveScrollToStartSide();
		}

		// from top
		if( m_cursorPosition.Y < m_croppClient->GetAbsoluteTop() )
		{
			m_verticalBar->MoveScrollToEndSide();
		}

		// from bottom
		if( ( m_cursorPosition.Y + GDefaultCursorHeight ) > m_croppClient->GetAbsoluteTop() + m_croppClient->GetHeight() )
		{
			m_verticalBar->MoveScrollToStartSide();
		}
	}

	void CRedGuiTextBox::ResetSelection()
	{
		m_selectionPosition.X = m_selectionPosition.Y = (Float)m_cursorPositionInText;
		m_startSelection = m_cursorPositionInText;
		m_startSelectionLineIndex = m_activeLineIndex;
	}

	void CRedGuiTextBox::UpdateSelection( Bool shiftModificator )
	{
		if( shiftModificator == true )
		{
			if( m_startSelection < (Int32)m_cursorPositionInText )
			{
				m_selectionPosition.X = (Float)m_startSelection;
				m_selectionPosition.Y = (Float)m_cursorPositionInText;
			}
			else
			{
				m_selectionPosition.X = (Float)m_cursorPositionInText;
				m_selectionPosition.Y = (Float)m_startSelection;
			}
		}
		else
		{
			m_startSelection = m_cursorPositionInText;
			m_selectionPosition.X = m_selectionPosition.Y = (Float)m_cursorPositionInText;
		}
	}

	void CRedGuiTextBox::OnKeyChangeRootFocus( Bool focus )
	{
		if( focus == false )
		{
			m_contextMenu->SetVisible( false );
		}
	}

	Bool CRedGuiTextBox::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( event == RGIE_Execute || event == RGIE_Select || event == RGIE_Back )
		{
			return true;
		}

		return false;
	}

	void CRedGuiTextBox::SetPasswordMode( Bool value )
	{
		m_multiLine = false;
		m_passwordMode = value;
	}

	Bool CRedGuiTextBox::GetPasswordMode() const
	{
		return m_passwordMode;
	}

	void CRedGuiTextBox::SetVisible( Bool value )
	{
		CRedGuiControl::SetVisible( value );

		if( value == true )
		{
			m_activeLineIndex = -1;
		}
	}

	void CRedGuiTextBox::OnKeySetFocus( CRedGuiControl* oldControl )
	{
		m_isActive = true;
		m_verticalBar->MoveScrollToEnd();
		m_activeLineIndex = m_lastVisibleLine - 1;
		CursorMoveEndOfLine( false );
	}

	void CRedGuiTextBox::OnKeyLostFocus( CRedGuiControl* newControl )
	{
		m_isActive = false;
		m_cursorVisible = false;
		m_activeLineIndex = -1;
	}

	void CRedGuiTextBox::OnMouseDrag( const Vector2& mousePosition, enum EMouseButton button )
	{
		if(button == MB_Left)
		{
			const Vector2& point = GRedGui::GetInstance().GetInputManager()->GetLastPressedPosition( MB_Left );

			m_activeLineIndex = CheckPoint( mousePosition );
			RecalculateCursorPosition( mousePosition );
			AdjustAreaToCursor();

			if( m_activeLineIndex == m_startSelectionLineIndex || m_multiLine == false )
			{
				if( point.X < mousePosition.X )
				{
					m_selectionPosition.X = (Float)m_startSelection;
					m_selectionPosition.Y = (Float)m_cursorPositionInText;
				}
				else if( point.X > mousePosition.X )
				{
					m_selectionPosition.X = (Float)m_cursorPositionInText;
					m_selectionPosition.Y = (Float)m_startSelection;
				}
			}
			else
			{
				if( m_activeLineIndex < m_startSelectionLineIndex )
				{
					m_selectionPosition.X = (Float)m_cursorPositionInText;
					m_selectionPosition.Y = (Float)m_startSelection;
				}
				else
				{
					m_selectionPosition.X = (Float)m_startSelection;
					m_selectionPosition.Y = (Float)m_cursorPositionInText;
				}
			}
		}
	}

	void CRedGuiTextBox::OnKeyButtonPressed( enum EInputKey key, Char text )
	{
		const Bool shiftModificator = GRedGui::GetInstance().GetInputManager()->IsShiftPressed();
		const Bool controlModificator = GRedGui::GetInstance().GetInputManager()->IsControlPressed();
		const Bool altModificator = GRedGui::GetInstance().GetInputManager()->IsAltPressed();

		// general functionality of the whole text
		if( controlModificator == true )
		{
			if(key == IK_C)
			{
				CopyText();
			}
			else if(key == IK_V)
			{
				PasteText();
			}
			else if(key == IK_X)
			{
				CutText();
			}
			else if(key == IK_A)
			{
				SelectAll();
			}
			return;
		}

		// manage the cursor position
		if( ManageCursorPosition( key, shiftModificator ) == true )
		{
			return;
		}

		// keys, which can not be interpreted as char
		if( key == IK_Shift || key == IK_Ctrl || key == IK_Alt || key == IK_CapsLock || key == IK_Insert 
			|| key == IK_PageDown || key == IK_PageUp || key == IK_ScrollLock || key == IK_PrintScrn || key == IK_Pause
			|| key == IK_Escape || key == IK_NumLock || key == IK_LeftMouse || key == IK_MiddleMouse || key == IK_RightMouse
			|| key == IK_Tab || key == IK_LShift || key == IK_RShift || key == IK_LControl || key == IK_RControl)
		{
			return;
		}

		// read-only protection
		if(m_readOnly == true)
		{
			return;
		}

		// delete character
		if( key == IK_Delete )
		{
			DeleteChar(false);
			return;
		}
		else if( key == IK_Backspace )
		{
			DeleteChar(true);
			return;
		}
		else if( key == IK_Enter )
		{
			if( m_multiLine == false )
			{
				EventTextEnter( this );
				return;
			}

			m_firstLinePosition.X = 0.0f;
		}

		// support for marks which are from range [a-z] and [0-9]
		if( ( key >= IK_A && key <= IK_Z ) || ( key >= IK_0 && key <= IK_9 ) )
		{
			// check shift key
			{
				String temp;
				temp += text;

				if( shiftModificator == true )
				{
					temp.MakeUpper();
				}
				else
				{
					temp.MakeLower();
				}

				text = temp.AsChar()[0];
			}
		}

		// if text is selected
		if( m_selectionPosition.X != m_selectionPosition.Y )
		{
			DeleteSelectedText();
		}

		// add character to whole text
		InsertChar( text );
		EventTextChanged( this, m_fullText );
	}

	void CRedGuiTextBox::OnMouseWheel( Int32 delta )
	{
		if( m_multiLine == true )
		{
			if( m_verticalRange != 0 )
			{
				Vector2 position = m_firstLinePosition;
				Int32 offset = -(Int32)position.Y;

				if( delta < 0 )
				{
					offset += GScrollWheelValue;
				}
				else
				{
					offset -= GScrollWheelValue;
				}

				if( offset < 0 )
				{
					offset = 0;
				}
				else if( offset > (Int32)m_verticalRange )
				{
					offset = m_verticalRange;
				}

				if( offset != position.Y )
				{
					position.Y = -(Float)offset;
					if( m_verticalBar != nullptr )
					{
						m_verticalBar->SetScrollPosition( offset );
					}
					m_firstLinePosition = position;
				}
			}
			else if( m_horizontalRange != 0 )
			{
				Vector2 position = m_firstLinePosition;
				Int32 offset = -(Int32)position.X;

				if( delta < 0 )
				{
					offset += GScrollWheelValue;
				}
				else
				{
					offset -= GScrollWheelValue;
				}

				if( offset < 0 )
				{
					offset = 0;
				}
				else if( offset > (Int32)m_horizontalRange )
				{
					offset = m_horizontalRange;
				}

				if( offset != position.X )
				{
					position.X = -(Float)offset;
					if( m_horizontalBar != nullptr )
					{
						m_horizontalBar->SetScrollPosition( offset );
					}
					m_firstLinePosition = position;
				}
			}

			UpdateView();
		}
	}

	void CRedGuiTextBox::OnMouseButtonPressed( const Vector2& mousePosition, enum EMouseButton button )
	{
		if( button == MB_Left )
		{
			m_activeLineIndex = CheckPoint( mousePosition );
			RecalculateCursorPosition( mousePosition );
			ResetSelection();
		}

		if(button == MB_Right)
		{
			m_contextMenu->SetPosition(mousePosition);
			m_contextMenu->SetVisible(true);
		}
		else
		{
			m_contextMenu->SetVisible(false);
		}

		m_isActive = true;
	}

	void CRedGuiTextBox::OnSizeChanged( const Vector2& oldSize, const Vector2& newSize )
	{
		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultLineHeight );
		m_firstLinePosition.X = 0.0f;

		SplitTextToLines();
		UpdateView();
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
