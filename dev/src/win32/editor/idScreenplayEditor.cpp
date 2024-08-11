#include "build.h"

#if 0
#include "interactiveDialogEditor.h"
#include "interactiveDialogGraphEditor.h"
#include "idScreenplayEditor.h"

#include "../../games/r6/idGraph.h"
#include "../../games/r6/idBasicBlocks.h"
#include "../../games/r6/idResource.h"
#include "../../games/r6/idTopic.h"
#include "../../games/r6/idGraphBlockText.h"
#include "../../games/r6/idGraphBlockChoice.h"

const wxString CEdInteractiveDialogEditor::TOKEN_TEXT_BLOCK_BEGIN[] =
{
	wxString( TXT("Text block:") ),
	wxString( TXT("Text:") ),
	wxString( TXT("Block:") )
};
const wxString CEdInteractiveDialogEditor::TOKEN_CHOICE_BLOCK_BEGIN[] =
{
	wxString( TXT("Choice block:") ),
	wxString( TXT("Choice:") ),
	wxString( TXT("Choices:") )
};
const wxString CEdInteractiveDialogEditor::PROPERTY_GUID_NAME( TXT("GUID") );

//------------------------------------------------------------------------------------------------------------------
// Screenplay control
//------------------------------------------------------------------------------------------------------------------
CEdScreenplayControl::CEdScreenplayControl( wxWindow* parent, CEdInteractiveDialogEditor* editor ) : wxRichTextCtrl( parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRE_MULTILINE | wxVSCROLL | wxNO_BORDER | wxWANTS_CHARS | wxALWAYS_SHOW_SB )
	, m_editor( editor )
	, m_currentBlockName( CName::NONE )
	, m_currentTokenIndex( -1 )
	, m_caretOffset( 0 )
	, m_caretAtEmptyLine( false )
	, m_scrollToCaret( false )
{
}

void CEdScreenplayControl::Init( wxPanel* screenplayPanel )
{
	SetFontScale( 1.4f );	// TODO: expose
	screenplayPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	screenplayPanel->GetSizer()->Add( this, 1, wxEXPAND );

	wxArrayInt tabs;
	tabs.Add( 300 ); // TODO: expose
	wxTextAttrEx attr;
	attr.SetFlags( wxTEXT_ATTR_TABS );
	attr.SetTabs( tabs );

	static wxRichTextParagraphStyleDefinition style( TXT("ps") );
	style.SetStyle( attr );

	static wxRichTextStyleSheet sheet;
	sheet.AddParagraphStyle( &style );
	SetStyleSheet( &sheet );

	Bind(	wxEVT_COMMAND_RICHTEXT_CONTENT_INSERTED,	&CEdScreenplayControl::OnScreenplayContentChange, this );
	Bind(	wxEVT_COMMAND_RICHTEXT_CONTENT_DELETED,		&CEdScreenplayControl::OnScreenplayContentChange, this );
	Bind(	wxEVT_COMMAND_KILL_FOCUS,					&CEdScreenplayControl::OnScreenplayKillFocus, this );
}

void CEdScreenplayControl::PositionCaret( wxRichTextParagraphLayoutBox* container /*= NULL */ )
{
	__super::PositionCaret( container );
	m_editor->HandleScreenplayUserInput();
}

void CEdScreenplayControl::OnScreenplayKillFocus( wxCommandEvent& event )
{
	event.Skip();
	m_editor->ApplyScreenplayChangesIfNeeded();
	m_editor->m_graphEditor->HiliteBlock( nullptr );
}

void CEdScreenplayControl::Paste()
{
	m_editor->OnBeginScreenplayPasting();
	__super::Paste();
	m_editor->OnEndScreenplayPasting();
}

void CEdScreenplayControl::UpdateRelativeCaretPosition( Int32 lineStartPosition, Int32 lineEndPosition, Int32 tokenIndex, CName blockName )
{
	const Int32 absoluteCaretPos = GetCaretPosition();
	if ( absoluteCaretPos < lineEndPosition )
	{
		//RED_LOG( CNAME( Dialog ), TXT("abs: %ld"), absoluteCaretPos );
		return;
	}

	m_caretOffset = absoluteCaretPos - lineStartPosition;
	m_currentTokenIndex = tokenIndex;
	m_currentBlockName = blockName;

	const wxString text = GetBuffer().GetTextForRange( wxRichTextRange( absoluteCaretPos, absoluteCaretPos + 1 ) ); 
	m_caretAtEmptyLine = ( text == TXT("\n\n") );
	//RED_LOG( CNAME( Dialog ), TXT("abs: %ld, rel car pos: %s el: %ld off: %ld, txt: %s"), absoluteCaretPos, blockName.AsChar(), m_currentTokenIndex, m_caretOffset, text.wx_str() );
}

void CEdScreenplayControl::OnScreenplayContentChange( wxCommandEvent& event )
{	
	//RED_LOG( CNAME( Dialog ), TXT("Content change") );
	event.Skip();
	
	m_editor->HandleScreenplayUserInput();
}

void CEdScreenplayControl::SetRelativeCaretPosition( CName blockName, Int32 tokenIndex, Int32 caretOffset, Bool emptyLine )
{
	m_currentBlockName = blockName;
	m_currentTokenIndex = tokenIndex;
	m_caretOffset = caretOffset;
	m_caretAtEmptyLine = emptyLine;
}


//------------------------------------------------------------------------------------------------------------------
// Editor class code, related to screenplay control
//------------------------------------------------------------------------------------------------------------------
void CEdInteractiveDialogEditor::FillScreenplay()
{
	// Some decalrations first =======================================================
	static wxTextAttr emptyStyle;

	struct SScopedScreenplayFreezer
	{
		wxRichTextCtrl*		m_ctrl;
		Bool&				m_flag;

		RED_INLINE SScopedScreenplayFreezer( wxRichTextCtrl* ctrl, Bool& flag )
			: m_ctrl( ctrl )
			, m_flag( flag )
		{
			ctrl->Freeze();
			ctrl->BeginSuppressUndo();
			flag = true;
		}

		RED_INLINE ~SScopedScreenplayFreezer()
		{
			m_ctrl->EndSuppressUndo();
			m_ctrl->Thaw();
			m_flag = false;
		}
	};

	// Now, the code =================================================================

	const Int32 scrollPos = m_screenplayCtrl->GetScrollPos( wxVERTICAL );

	Double timeWithRefresh( 0.0 ), timeWithoutRefresh( 0.0 );
	{
		Red::System::ScopedStopClock clkWith( timeWithRefresh );
		{
			SScopedScreenplayFreezer freezer( m_screenplayCtrl, m_fillingScreenplayNow );
			{
				Red::System::ScopedStopClock clkWithout( timeWithoutRefresh );

				m_screenplayCaretPos = m_screenplayCtrl->GetCaretPosition();
				m_screenplayLinePos = m_screenplayCtrl->GetBuffer().GetVisibleLineNumber( m_screenplayCaretPos, true );

				
				m_screenplayCtrl->Clear();
				m_screenplayCtrl->SetDefaultStyle( emptyStyle );

				if ( nullptr == m_dialogTopic )
				{
					return;
				}

				if ( false == m_screenplayUnrecogizedText.Empty() )
				{
					CheckRelativeCaretPosition( CName::NONE, 0 );
					m_screenplayCtrl->WriteText( wxString( m_screenplayUnrecogizedText.AsChar() ) );
				}

				TDynArray< const CIDGraphBlock* > blockArray;
				blockArray.Reserve( m_dialogTopic->GetGraph()->GraphGetBlocks().Size() ); 
				m_dialogTopic->GatherBlocksByGraphOrder( blockArray );  // this ordering is important here, so we can't just use itarator

				Bool firstBlock( true );
				wxRichTextProperties props;

				for ( Uint32 i = 0; i < blockArray.Size(); ++i )
				{
					const CIDGraphBlock* block = blockArray[ i ];
					const CIDGraphBlockText* textBlock = Cast< const CIDGraphBlockText > ( block );
					const CIDGraphBlockChoice* choiceBlock = Cast< const CIDGraphBlockChoice > ( block );
					if ( nullptr == textBlock && nullptr == choiceBlock )
					{
						continue;
					}

					// end previous block
					if ( false == firstBlock )
					{
						//m_screenplayCtrl->Newline();
						WriteScreenplayText( wxString( L'\n' ), props );
					}

					Int32 blockLineIndex( 0 );
					CheckRelativeCaretPosition( block->GetName(), blockLineIndex );

					wxString guid;
					guid.resize( RED_GUID_STRING_BUFFER_SIZE );
					block->GetGuid().ToString( const_cast< Char* > ( guid.wx_str() ), RED_GUID_STRING_BUFFER_SIZE );
					props.Clear();
					props.SetProperty( PROPERTY_GUID_NAME, guid );

					// begin block
					m_screenplayCtrl->BeginAlignment( wxTEXT_ALIGNMENT_CENTER );

					m_screenplayCtrl->BeginBold();
					WriteScreenplayText( textBlock ? TOKEN_TEXT_BLOCK_BEGIN[ 0 ] : TOKEN_CHOICE_BLOCK_BEGIN[ 0 ], props );
					m_screenplayCtrl->EndBold();

					WriteScreenplayText( wxString( L' ' ).Append( block->GetName().AsString().AsChar() ).Append( L'\n' ), props );

					// output block comment
					WriteScreenplayComment( block->GetComment(), block->GetName(), blockLineIndex, props );
					m_screenplayCtrl->EndAlignment();

					m_screenplayCtrl->BeginParagraphStyle( TXT("ps") );
					
					if ( textBlock )
					{
						FillScreenplayBlock( props, textBlock, blockLineIndex );
					}
					else if ( choiceBlock )
					{
						FillScreenplayBlock( props, choiceBlock, blockLineIndex );
					}

					m_screenplayCtrl->EndParagraphStyle();

					firstBlock = false;
				}

				// reposition the caret			
				m_screenplayCtrl->MoveCaret( m_screenplayCaretPos );
				if ( m_screenplayCtrl->IsCaretAtEmptyLine() )
				{
					m_screenplayCtrl->Newline();
				}
			}
		}
	}

	if ( m_screenplayCtrl->ShouldScrollToCaret() )
	{
		m_screenplayCtrl->ScrollIntoView( m_screenplayCaretPos, WXK_HOME );
	}
	else
	{
		// scroll the window to previous pos
		m_screenplayCtrl->Scroll( wxPoint( -1, scrollPos ) );
	}

	// remember the value
	m_currentScreenplayContent = m_screenplayCtrl->GetValue();

	// forget about scrolling to caret
	m_screenplayCtrl->SetScrollToCaret( false );

	RED_LOG( CNAME( Dialog ), TXT("Screenplay update summary: it took %.2lf msec without refresh and %.2lf msec with refresh."), timeWithoutRefresh * 1000.0, timeWithRefresh * 1000.0 );
}


void CEdInteractiveDialogEditor::CheckRelativeCaretPosition( CName blockName, Int32 linePos )
{
	if ( blockName == m_screenplayCtrl->GetCurrentBlockName() && linePos <= m_screenplayCtrl->GetCurrentBlockToken() )
	{
		// caret needs to be here
		m_screenplayCaretPos = m_screenplayCtrl->GetCaretPosition() + m_screenplayCtrl->GetCaretOffset();
		m_screenplayLinePos = m_screenplayCtrl->GetBuffer().GetVisibleLineNumber( m_screenplayCaretPos, true, true );
	}
}


void CEdInteractiveDialogEditor::FillScreenplayBlock( const wxRichTextProperties& props, const CIDGraphBlockText* block, Int32& blockLineIndex )
{
	static const wxColour voicetagColor( 80, 80, 80 );

	// output lines
	for ( Uint32 k = 0; k < block->GetNumLines(); ++k )
	{
		const SIDTextLine& line = block->GetLine( k );
		CheckRelativeCaretPosition( block->GetName(), ++blockLineIndex );

		m_screenplayCtrl->BeginTextColour( voicetagColor );
		m_screenplayCtrl->BeginBold();
		if ( line.m_receiver && line.m_receiver != CNAME( Default ) )
		{
			WriteScreenplayText( wxString( line.m_speaker.AsString().AsChar() ).Append( TXT(" to ") ).Append( line.m_receiver.AsString().AsChar() ).Append( TXT(":\t") ), props );
		}
		else
		{
			WriteScreenplayText( wxString( line.m_speaker.AsString().AsChar() ).Append( TXT(":\t") ), props );
		}
		m_screenplayCtrl->EndBold();
		m_screenplayCtrl->EndTextColour();
		WriteScreenplayText( wxString( line.m_text.GetString().AsChar() ).Append( L'\n' ), props );

		// ...with comment
		WriteScreenplayComment( line.m_comment, block->GetName(), blockLineIndex, props );

	}

	CheckRelativeCaretPosition( block->GetName(), ++blockLineIndex );
}


void CEdInteractiveDialogEditor::WriteScreenplayComment( const String& comment, CName blockName, Int32& blockLineIndex, const wxRichTextProperties& props )
{
	if ( false == comment.Empty() )
	{
		CTokenizer tokenizer( comment, TXT("\n") );
		for ( Uint32 i = 0; i < tokenizer.GetNumTokens(); ++i )
		{
			CheckRelativeCaretPosition( blockName, ++blockLineIndex );
			m_screenplayCtrl->BeginItalic();
			WriteScreenplayText( wxString( tokenizer.GetToken( i ).AsChar() ).Append( L'\n' ), props );
			m_screenplayCtrl->EndItalic();
		}
	}	
}


void CEdInteractiveDialogEditor::FillScreenplayBlock( const wxRichTextProperties& props, const CIDGraphBlockChoice* block, Int32& blockLineIndex )
{
	static const wxColour optionColor( 80, 80, 180 );

	// output choices
	for ( Uint32 k = 0; k < block->GetNumOptions(); ++k )
	{
		const SIDOption& opt = block->GetOption( k );
		CheckRelativeCaretPosition( block->GetName(), ++blockLineIndex );

/*
		const CIDGraphSocket* outSocket =  block->FindOptionOutput( &opt );
		if ( outSocket )
		{
			const TDynArray< CGraphConnection* >& connections = outSocket->GetConnections();
			if ( false == connections.Empty() )
			{
				const CGraphSocket* inSocket = connections[ 0 ]->GetDestination();
				if ( inSocket == outSocket )
				{
					inSocket = connections[ 0 ]->GetSource();
				}
				ASSERT( inSocket != outSocket );

				if ( nullptr != inSocket )
				{
					const CIDGraphBlock* connBlock = Cast< const CIDGraphBlock > ( inSocket->GetParent() );
					if ( connBlock )
					{

					}
				}
			}
		}*/

		m_screenplayCtrl->BeginTextColour( optionColor );
		m_screenplayCtrl->BeginBold();
		WriteScreenplayText( wxString( CEnum::ToString< EHudChoicePosition > ( opt.m_hudPosition ).StringAfter( TXT("_") ).AsChar() ).Append( TXT(":\t") ), props );
		m_screenplayCtrl->EndBold();
		m_screenplayCtrl->EndTextColour();
				
		WriteScreenplayText( wxString( opt.m_text.GetString().AsChar() ).Append( L'\n' ), props );

		// ...with comment
		WriteScreenplayComment( opt.m_comment, block->GetName(), blockLineIndex, props );
	}

	CheckRelativeCaretPosition( block->GetName(), ++blockLineIndex );
}



void CEdInteractiveDialogEditor::WriteScreenplayText( const wxString& text, const wxRichTextProperties& props )
{
	m_screenplayCtrl->WriteText( text );
	wxRichTextObject* obj = m_screenplayCtrl->GetBuffer().GetLeafObjectAtPosition( m_screenplayCtrl->GetCaretPosition() );
	if ( obj )
	{
		obj->SetProperties( props );
	}
	else
	{
		ASSERT( false );
	}
}

void CEdInteractiveDialogEditor::HandleScreenplayUserInput()
{
	if ( m_fillingScreenplayNow )
	{
		// not a USER input
		return;
	}

	if ( nullptr == m_dialogTopic )
	{
		// topic not selected
		return;
	}
	
	Int32 oldCaretPos = m_screenplayCaretPos;
	m_screenplayCaretPos = m_screenplayCtrl->GetCaretPosition();

	if ( oldCaretPos != m_screenplayCaretPos )
	{
		OnScreenplayCaretPositionChange( oldCaretPos );
	}

	HiliteBlockUnderScreenplayCaret();
}

void CEdInteractiveDialogEditor::OnScreenplayCaretPositionChange( Int32 oldPos )
{
	//RED_LOG( CNAME( Dialog ), TXT("Caret pos change: %ld to %ld"), oldPos, m_caretPos );
	Int32 linePos = m_screenplayCtrl->GetBuffer().GetVisibleLineNumber( m_screenplayCaretPos, true );
	if ( m_screenplayLinePos != linePos )
	{
		m_screenplayLinePos = linePos;
		ApplyScreenplayChangesIfNeeded();
	}
}

void CEdInteractiveDialogEditor::ApplyScreenplayChanges()
{
	struct STokenParser
	{
		const CInteractiveDialog* m_dialog;

		RED_INLINE STokenParser( const CInteractiveDialog* dialog )
			: m_dialog( dialog )
		{
		}

		RED_INLINE Bool IsEmptyToken( const String& token )		
		{ 
			return token.Empty(); 
		}

		RED_INLINE Bool IsTextBlockToken( const String& token, CName& outBlockName ) 
		{ 
			for ( Uint32 k = 0; k < ARRAY_COUNT( TOKEN_TEXT_BLOCK_BEGIN ); ++k )
			{
				if ( token.BeginsWith( TOKEN_TEXT_BLOCK_BEGIN[ k ].wx_str() ) )
				{
					// treat rest of the string as a name, skip white spaces after "Block:"
					Uint32 toSkip = ( TOKEN_TEXT_BLOCK_BEGIN[ k ].length() );
					for ( Uint32 i = toSkip; i < token.GetLength(); ++i, ++toSkip )
					{
						if ( token[ i ] != ' ' && token[ i ] != '\t' )
						{
							break;
						}
					}
					outBlockName = CName( token.RightString( token.GetLength() - toSkip ) );
					return true;
				}
			}

			return false;
		}

		RED_INLINE Bool IsChoiceBlockToken( const String& token, CName& outBlockName ) 
		{ 
			for ( Uint32 k = 0; k < ARRAY_COUNT( TOKEN_CHOICE_BLOCK_BEGIN ); ++k )
			{
				if ( token.BeginsWith( TOKEN_CHOICE_BLOCK_BEGIN[ k ].wx_str() ) )
				{
					// treat rest of the string as a name, skip white spaces after "Block:"
					Uint32 toSkip = ( TOKEN_CHOICE_BLOCK_BEGIN[ k ].length() );
					for ( Uint32 i = toSkip; i < token.GetLength(); ++i, ++toSkip )
					{
						if ( token[ i ] != ' ' && token[ i ] != '\t' )
						{
							break;
						}
					}
					outBlockName = CName( token.RightString( token.GetLength() - toSkip ) );
					return true;
				}
			}

			return false;
		}

		RED_INLINE Bool IsInterlocutorToken( const String& token, CName& outInterlocutorId )
		{
			const Uint32 len = token.GetLength();
			if ( len < 2 || token[ len - 1 ] != ':' )
			{
				return false;
			}

			const TDynArray< SIDInterlocutorDefinition >& definitions = m_dialog->GetInterlocutorDefinitions();
			const String name = token.LeftString( len - 1 );

			for ( Uint32 i = 0; i < definitions.Size(); ++i )
			{
				if ( name.EqualsNC( definitions[ i ].m_interlocutorId.AsString() ) )
				{
					outInterlocutorId = definitions[ i ].m_interlocutorId;
					return true;
				}
			}

			return false;
		}

		RED_INLINE Bool AreDoubleInterlocutorTokens( const CTokenizer& tokenizer, CName& outSpeaker, CName& outReceiver )
		{
			if ( ( false == tokenizer.GetToken( 1 ).EqualsNC( TXT("to") ) )	&& ( false == tokenizer.GetToken( 1 ).EqualsNC( TXT("do") ) ) )
			{
				return false;
			}

			const String lastToken = tokenizer.GetToken( 2 );
			const Uint32 len = lastToken.GetLength();
			if ( len < 2 || lastToken[ len - 1 ] != ':' )
			{
				return false;
			}

			CName receiver;
			const TDynArray< SIDInterlocutorDefinition >& definitions = m_dialog->GetInterlocutorDefinitions();
			String name = lastToken.LeftString( len - 1 );			
			if ( name.EqualsNC( TXT("nobody") ) )
			{
				receiver = CNAME( Nobody );
			}
			else if ( name.EqualsNC( TXT("default" ) ) )
			{
				return false;
			}
			else
			{
				for ( Uint32 i = 0; i < definitions.Size(); ++i )
				{
					if ( name.EqualsNC( definitions[ i ].m_interlocutorId.AsString() ) )
					{
						receiver = definitions[ i ].m_interlocutorId;
						break;
					}
				}

				if ( !receiver )
				{
					return false;
				}
			}

			name = tokenizer.GetToken( 0 );

			for ( Uint32 i = 0; i < definitions.Size(); ++i )
			{
				if ( name.EqualsNC( definitions[ i ].m_interlocutorId.AsString() ) )
				{
					outSpeaker = definitions[ i ].m_interlocutorId;
					outReceiver = receiver;
					return true;
				}
			}

			return false;
		}

		RED_INLINE void FillStubText( const CTokenizer& tokenizer, Uint32 firstToken, String& outString )
		{
			outString = String::EMPTY;
			for ( Uint32 i = firstToken; i < tokenizer.GetNumTokens(); ++i )					
			{
				if ( i > firstToken )
				{
					outString.Append( ' ' );
				}
				outString += tokenizer.GetToken( i );					
			}
		}

		RED_INLINE Bool IsTextLineToken( const String& token, SIDLineStub& stub )
		{
			CTokenizer tokenizer( token, TXT("\t ") );
			if ( tokenizer.GetNumTokens() < 2 )
			{
				return false;
			}

			if ( IsInterlocutorToken( tokenizer.GetToken( 0 ), stub.m_speaker ) )
			{
				FillStubText( tokenizer, 1, stub.m_text );
				return true;
			}
				
			if ( AreDoubleInterlocutorTokens( tokenizer, stub.m_speaker, stub.m_receiver ) )
			{
				FillStubText( tokenizer, 3, stub.m_text );
				return true;
			}

			return false;
		}	

		RED_INLINE Bool IsOptionToken( const String& token, EHudChoicePosition& outOptionHudPosition )
		{
			const Uint32 len = token.GetLength();
			if ( len < 2 || token[ len - 1 ] != ':' )
			{
				return false;
			}

			const String hudPos = token.LeftString( len - 1 );

			for ( Uint32 i = 0; i < CHOICE_Max; ++i )
			{
				if ( CEnum::ToString< EHudChoicePosition > ( static_cast< EHudChoicePosition > ( i ) ).StringAfter( TXT("_") ).EqualsNC( hudPos ) )
				{
					outOptionHudPosition = static_cast< EHudChoicePosition > ( i );
					return true;
				}
			}

			return false;
		}

		RED_INLINE Bool IsChoiceLineToken( const String& token, EHudChoicePosition& outOptionHudPosition, String& outOptionText )
		{
			CTokenizer tokenizer( token, TXT("\t ") );
			if ( IsOptionToken( tokenizer.GetToken( 0 ), outOptionHudPosition ) )
			{
				outOptionText = String::EMPTY;
				for ( Uint32 i = 1; i < tokenizer.GetNumTokens(); ++i )					
				{
					if ( i > 1 )
					{
						outOptionText.Append( ' ' );
					}
					outOptionText += tokenizer.GetToken( i );					
				}
				return true;
			}

			return false;
		}
	};

	ASSERT( m_pendingBlockOperations.Empty(), TXT("m_pendingBlockOperations needs to be empty before calling ApplyScreenplayChanges()") );

	Double time( 0.0 );
	{
		Red::System::ScopedStopClock clk( time );

		STokenParser parser( m_dialog );
		m_screenplayUnrecogizedText.ClearFast(); 
		m_screenplayCtrl->UpdateRelativeCaretPosition( 0, 0, 0, CName::NONE ); 

		const Int32 numLines = m_screenplayCtrl->GetBuffer().GetLineCount();
		Int32 thisLineStart, thisLineEnd( -1 );

		for ( Int32 i = 0; i < numLines; ++i )
		{
			String token = m_screenplayCtrl->GetLineText( i );
			thisLineStart = thisLineEnd + 1;
			thisLineEnd += token.GetLength() + 1;

			// Search for first block
			CName blockName, trash;
			if ( parser.IsTextBlockToken( token, blockName ) )
			{
				Int32 thisBlockTokenIndex( 0 );
				m_screenplayCtrl->UpdateRelativeCaretPosition( thisLineStart, thisLineEnd, thisBlockTokenIndex, blockName );

				// try to fetch the properties
				wxString blockGuid;
				wxRichTextObject* obj = m_screenplayCtrl->GetBuffer().GetLeafObjectAtPosition( thisLineStart + 1 );
				if ( obj )
				{
					blockGuid = obj->GetProperties().GetPropertyString( PROPERTY_GUID_NAME );
					//RED_LOG( CNAME( Dialog ), TXT("blockGuid %s read from %s"), blockGuid.wx_str(), obj->GetTextForRange( wxRichTextRange( thisLineStart, thisLineEnd ) ).wx_str() );
				}

				// analyze block
				Uint32 currentLine = 0;
				TDynArray< SIDLineStub > stubs( 1 );  // allocate one "in advance"
				SIDLineStub* currentStub = &stubs[ 0 ];
				String blockComment;
				String* currentComment = &blockComment;

				while ( i < numLines )
				{
					token = m_screenplayCtrl->GetLineText( ++i );

					if ( parser.IsTextBlockToken( token, trash ) || parser.IsChoiceBlockToken( token, trash ) )
					{
						// found next block title
						break;
					}

					thisLineStart = thisLineEnd + 1;
					thisLineEnd += token.GetLength() + 1;

					if ( parser.IsEmptyToken( token ) )
					{
						continue;
					}

					m_screenplayCtrl->UpdateRelativeCaretPosition( thisLineStart, thisLineEnd, ++thisBlockTokenIndex, blockName );

					if ( parser.IsTextLineToken( token, *currentStub ) )
					{
						// found new line of text
						stubs.Grow( 1 );
						currentComment = &stubs[ currentLine ].m_comment;
						currentStub = &stubs[ ++currentLine ];
						continue;
					}

					// treat everything else as a comment
					if ( false == currentComment->Empty() )
					{
						currentComment->Append( '\n' );
					}

					// append comment line 
					currentComment->Append( token.AsChar(), token.GetLength() );
				}

				// deallocate one allocated "in advance"
				stubs.PopBackFast();
				--i;

				CGUID guid;
				if ( false == blockGuid.empty() )
				{
					guid.FromString( blockGuid.wx_str() );
				}

				// update data in block (or spawn a new one)
				UpdateTextBlock( guid, blockName, stubs, blockComment );
				continue;
			}
			else if ( parser.IsChoiceBlockToken( token, blockName ) )
			{
				Int32 thisBlockTokenIndex( 0 );
				m_screenplayCtrl->UpdateRelativeCaretPosition( thisLineStart, thisLineEnd, thisBlockTokenIndex, blockName );

				// try to fetch the properties
				wxString blockGuid;
				wxRichTextObject* obj = m_screenplayCtrl->GetBuffer().GetLeafObjectAtPosition( thisLineStart + 1 );
				if ( obj )
				{
					blockGuid = obj->GetProperties().GetPropertyString( PROPERTY_GUID_NAME );
					//RED_LOG( CNAME( Dialog ), TXT("blockGuid %s read from %s"), blockGuid.wx_str(), obj->GetTextForRange( wxRichTextRange( thisLineStart, thisLineEnd ) ).wx_str() );
				}

				// analyze block
				Uint32 currentOpt = 0;
				SIDOptionStub stubs[ CHOICE_Max ];

				String optionText;
				String blockComment;
				String* currentComment = &blockComment;
				EHudChoicePosition pos;
				Uint32 index = 0;

				while ( i < numLines )
				{
					token = m_screenplayCtrl->GetLineText( ++i );

					if ( parser.IsTextBlockToken( token, trash ) || parser.IsChoiceBlockToken( token, trash ) )
					{
						// found next block title
						break;
					}

					thisLineStart = thisLineEnd + 1;
					thisLineEnd += token.GetLength() + 1;

					if ( parser.IsEmptyToken( token ) )
					{
						continue;
					}

					m_screenplayCtrl->UpdateRelativeCaretPosition( thisLineStart, thisLineEnd, ++thisBlockTokenIndex, blockName );

					if ( parser.IsChoiceLineToken( token, pos, optionText ) )
					{
						// found new option
						ASSERT( pos < CHOICE_Max && pos >= 0 );

						if ( stubs[ pos ].m_position != pos ) // if this slot is not yet taken
						{
							stubs[ pos ].m_text = optionText;
							stubs[ pos ].m_position = pos; // this marks slot as taken
							stubs[ pos ].m_index = index++;
							currentComment = &stubs[ pos ].m_comment;
							continue;
						}
					}

					// treat everything else as a comment
					if ( false == currentComment->Empty() )
					{
						currentComment->Append( '\n' );
					}

					// append comment line 
					currentComment->Append( token.AsChar(), token.GetLength() );
				}

				--i;

				CGUID guid;
				if ( false == blockGuid.empty() )
				{
					guid.FromString( blockGuid.wx_str() );
				}

				// update data in block (or spawn a new one)
				UpdateChoiceBlock( guid, blockName, stubs, blockComment );
				continue;
			}
			else
			{
				// not a block, keep it as unrecognized text
				m_screenplayUnrecogizedText.Append( token.AsChar(), token.GetLength() );
				m_screenplayUnrecogizedText.Append( L'\n' );
				RED_LOG( CNAME( Dialog ), TXT("Unrecognized token: %s"), token.AsChar() );
			}

		}
	}

	RED_LOG( CNAME( Dialog ), TXT("Analyze took: %lf msec."), time * 1000.0 );

	if ( ValidateUpdate() )
	{
		DoPendingBlockOperations();
	}
}


Bool CEdInteractiveDialogEditor::ValidateUpdate()
{
	// debug crap
// 	Char buf[ 256 ];
// 	for ( Uint32 i = 0; i < m_pendingBlockOperations.Size(); ++i )
// 	{
// 		const SBlockOp& op = m_pendingBlockOperations[ i ];
// 		const Char* tab[] = { TXT("NOTHING"), TXT("UPDATE"), TXT("ADD"), TXT("DELETE") }; 
// 		
// 		op.m_block.Get()->GetGuid().ToString( buf, 256 );
// 		RED_LOG( CNAME( Dialog ), TXT("%s block %s with guid %s at address %x"), tab[ op.m_op ], op.m_block.Get()->GetName().AsChar(), buf, op.m_block.Get() );
// 	}

	// check if we're not updating or adding a block with the same name twice
	TDynArray< CName > uniqueNames;
	Bool allNamesAreUnique( true );
	for ( Uint32 i = 0; i < m_pendingBlockOperations.Size(); ++i )
	{
		CIDGraphBlock* block = m_pendingBlockOperations[ i ].m_block.Get();
		if ( block )
		{
			if ( uniqueNames.Exist( block->GetName() ) )
			{
				allNamesAreUnique = false;
				break;
			}
			uniqueNames.PushBack( block->GetName() ); 
		}
	}

	if ( false == allNamesAreUnique )
	{
		wxMessageBox( TXT("Block names have to be unique."), TXT("Error"), wxOK | wxICON_ERROR );
		m_pendingBlockOperations.Clear();
		return false;
	}

	// each text block in the topic needs to be on the pending operations list.
	// If it's not - then it means user just deleted it in screenplay panel.
	Bool changedGraphStructure( false );
	TDynArray< CIDGraphBlockText* > deletedTextBlocks;
	for ( CIDTopic::BlockIterator< CIDGraphBlockText > it ( m_dialogTopic ); it; ++it )
	{
		Bool found( false );
		CIDGraphBlockText* block = ( *it );

		//block->GetGuid().ToString( buf, 256 );
		//RED_LOG( CNAME( Dialog ), TXT("block %s with guid %s at address %x"), block->GetName().AsChar(), buf, block );

		for ( Uint32 i = 0; i < m_pendingBlockOperations.Size(); ++i )
		{
			if ( m_pendingBlockOperations[ i ].m_block.Get() == block )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{			
			deletedTextBlocks.PushBack( block );
			if ( !changedGraphStructure )
			{
				for ( Uint32 k = 0; k < block->GetSockets().Size(); ++k )
				{
					if ( block->GetSockets()[ k ]->HasConnections() )
					{
						changedGraphStructure = true;
						break;
					}
				}
			}			 
		}
	}

	TDynArray< CIDGraphBlockChoice* > deletedChoiceBlocks;
	for ( CIDTopic::BlockIterator< CIDGraphBlockChoice > it ( m_dialogTopic ); it; ++it )
	{
		Bool found( false );
		CIDGraphBlockChoice* block = ( *it );

		for ( Uint32 i = 0; i < m_pendingBlockOperations.Size(); ++i )
		{
			if ( m_pendingBlockOperations[ i ].m_block.Get() == block )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{			
			deletedChoiceBlocks.PushBack( block );
			if ( !changedGraphStructure )
			{
				for ( Uint32 k = 0; k < block->GetSockets().Size(); ++k )
				{
					if ( block->GetSockets()[ k ]->HasConnections() )
					{
						changedGraphStructure = true;
						break;
					}
				}
			}			 
		}
	}

	if ( false == deletedTextBlocks.Empty() || false == deletedChoiceBlocks.Empty() )
	{
		// some blocks were deleted, if this changes the graph structure, we need to ask user
		if ( changedGraphStructure )
		{
			wxString question;			
			if ( false == deletedTextBlocks.Empty() )
			{
				question = TXT("Following text blocks are about to be deleted:\n\n");
				for ( Uint32 i = 0; i < deletedTextBlocks.Size(); ++i )
				{
					CIDGraphBlockText* block = deletedTextBlocks[ i ];
					question.Append( block->GetName().AsChar() );
					if ( block->GetNumLines() > 0 )
					{
						const SIDTextLine& line = block->GetLine( 0 );
						if ( line.m_speaker )
						{
							question.Append( String::Printf( TXT(" (%s: %s...)"), line.m_speaker.AsChar(), line.m_text.GetString().LeftString( 35 ).AsChar() ).AsChar() );
						}
					}
					question.Append( L'\n' );
				}
				if ( false == deletedChoiceBlocks.Empty() )
				{ 
					question.Append( L'\n' );
				}
			}
			if ( false == deletedChoiceBlocks.Empty() )
			{
				question.Append( TXT("Following choice blocks are about to be deleted:\n\n") );
				for ( Uint32 i = 0; i < deletedChoiceBlocks.Size(); ++i )
				{
					CIDGraphBlockChoice* block = deletedChoiceBlocks[ i ];
					question.Append( block->GetName().AsChar() );
					question.Append( L'\n' );
				}
			}
			question.Append( TXT("\nSome of those blocks have connections to other blocks in the graph.\nThis will break those connections and cannot be undone.\n\nAre you sure?") );
			
			if ( wxNO == wxMessageBox( question, TXT("Warning"), wxYES_NO | wxICON_WARNING ) )
			{
				m_pendingBlockOperations.Clear();
				return false;
			}
		}

		// ok, spawn delete ops
		for ( Uint32 i = 0; i < deletedTextBlocks.Size(); ++i )
		{
			SIDBlockEditorOp &op = m_pendingBlockOperations[ m_pendingBlockOperations.Grow( 1 ) ];
			op.m_block = deletedTextBlocks[ i ];
			op.m_op = BLOCK_Delete;
		}

		for ( Uint32 i = 0; i < deletedChoiceBlocks.Size(); ++i )
		{
			SIDBlockEditorOp &op = m_pendingBlockOperations[ m_pendingBlockOperations.Grow( 1 ) ];
			op.m_block = deletedChoiceBlocks[ i ];
			op.m_op = BLOCK_Delete;
		}
	}

	return true;
}

void CEdInteractiveDialogEditor::UpdateTextBlock( const CGUID& guid, const CName& name, const TDynArray< SIDLineStub >& stubs, const String& blockComment )
{
	// Try to find a block 
	CIDGraphBlockText* blockMatchedByLines = nullptr;
	CIDGraphBlockText* blockMatchedByName = nullptr;
	CIDGraphBlockText* blockMatchedByGUID = nullptr;

	for ( CIDTopic::BlockIterator< CIDGraphBlockText > it( m_dialogTopic ); it; ++it )
	{
		CIDGraphBlockText* block = *it;

		if ( name && block->GetName() == name )
		{
			ASSERT( nullptr == blockMatchedByName, TXT("Two blocks with the same name?") );
			blockMatchedByName = block;
		}

		if ( false == guid.IsZero() && block->GetGuid() == guid )
		{
			ASSERT( nullptr == blockMatchedByGUID, TXT("Two blocks with the same GUID?") );
			blockMatchedByGUID = block;
		}

		if ( block->GetNumLines() != stubs.Size() )
		{
			continue;
		}

		Bool allLinesMatching( true );
		for ( Uint32 i = 0; i < stubs.Size(); ++i )
		{
			const SIDTextLine& line = block->GetLine( i );
			if ( false == stubs[ i ].MatchesLine( block->GetLine( i ) ) )
			{
				allLinesMatching = false;
				break;
			}
		}

		if ( allLinesMatching && block->GetComment() == blockComment && 
			( false == stubs.Empty() || false == blockComment.Empty() ) )
		{
			blockMatchedByLines = block;
		}
	}

	SIDBlockEditorOp& op = m_pendingBlockOperations[ m_pendingBlockOperations.Grow( 1 ) ];
	if ( blockMatchedByGUID )
	{
		//RED_LOG( CNAME( Dialog ), TXT("upd Block matched by GUID %08x-%08x-%08x-%08x %s %x"), guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D, name.AsChar(), blockMatchedByGUID );   

		op.m_block = blockMatchedByGUID;
		op.m_comment = blockComment;
		op.m_name = name;
		op.m_stubs = stubs;
		op.m_op = BLOCK_Update;
		op.m_flags = UPDATE_Name | UPDATE_Lines | UPDATE_Comment;
	}
	else
	{
		if ( blockMatchedByLines == blockMatchedByName && ( nullptr != blockMatchedByLines ) )
		{
			//RED_LOG( CNAME( Dialog ), TXT("nothing Block matched by LINES %08x-%08x-%08x-%08x %s %x"), guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D, name.AsChar(), blockMatchedByLines );   

			// nothing changed, do nothing, just put the block on the list to make sure it won't be deleted
			op.m_block = blockMatchedByLines;
			op.m_op = BLOCK_DoNothing;
			return;
		}

		if ( nullptr != blockMatchedByLines )
		{
			//RED_LOG( CNAME( Dialog ), TXT("upd Block matched by LINES %08x-%08x-%08x-%08x %s %x"), guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D, name.AsChar(), blockMatchedByLines );   

			op.m_block = blockMatchedByLines;
			op.m_name = name;
			op.m_op = BLOCK_Update;
			op.m_flags = UPDATE_Name;
			return;
		}

		if ( nullptr != blockMatchedByName )
		{  
			//RED_LOG( CNAME( Dialog ), TXT("upd Block matched by NAME %08x-%08x-%08x-%08x %s %x"), guid.parts.A, guid.parts.B, guid.parts.C, guid.parts.D, name.AsChar(), blockMatchedByName );   

			op.m_block = blockMatchedByName;
			op.m_comment = blockComment;
			op.m_stubs = stubs;
			op.m_op = BLOCK_Update;
			op.m_flags = UPDATE_Lines | UPDATE_Comment;
			return;
		}

		//RED_LOG( CNAME( Dialog ), TXT("add Block %s"), name.AsChar() );   

		// nothing matched, spawn a new block!
		op.m_block = nullptr;
		op.m_comment = blockComment;
		op.m_name = name;
		op.m_stubs = stubs;
		op.m_op = BLOCK_AddText;
		op.m_flags = UPDATE_Name | UPDATE_Lines | UPDATE_Comment;
	}
}

void CEdInteractiveDialogEditor::UpdateChoiceBlock( const CGUID& guid, const CName& name, const SIDOptionStub* stubs, const String& blockComment )
{
	// Try to find a block 
	CIDGraphBlockChoice* blockMatchedByOptions = nullptr;
	CIDGraphBlockChoice* blockMatchedByName = nullptr;
	CIDGraphBlockChoice* blockMatchedByGUID = nullptr;

	for ( CIDTopic::BlockIterator< CIDGraphBlockChoice > it( m_dialogTopic ); it; ++it )
	{
		CIDGraphBlockChoice* block = *it;

		if ( name && block->GetName() == name )
		{
			ASSERT( nullptr == blockMatchedByName, TXT("Two blocks with the same name?") );
			blockMatchedByName = block;
		}

		if ( false == guid.IsZero() && block->GetGuid() == guid )
		{
			ASSERT( nullptr == blockMatchedByGUID, TXT("Two blocks with the same GUID?") );
			blockMatchedByGUID = block;
		}

		Bool optionsMatching( true );
		for ( Uint32 i = 0; i < CHOICE_Max; ++i )
		{
			const SIDOption* option = block->FindOption( static_cast< EHudChoicePosition > ( i ) );
			if ( stubs[ i ].m_position == i )
			{
				// stub filled				
				if ( nullptr == option || false == stubs[ i ].MatchesOption( option ) )
				{
					optionsMatching = false;
					break;
				}
			}
			else
			{
				// stub not filled
				if ( option )
				{
					optionsMatching = false;
					break;
				}
			}
		}

		if ( optionsMatching && block->GetComment() == blockComment )
		{
			blockMatchedByOptions = block;
		}
	}

	SIDBlockEditorOp& op = m_pendingBlockOperations[ m_pendingBlockOperations.Grow( 1 ) ];
	if ( blockMatchedByGUID )
	{
		op.m_block = blockMatchedByGUID;
		op.m_comment = blockComment;
		op.m_name = name;
		SIDBlockEditorOp::CopyOptions( op.m_options, stubs );
		op.m_op = BLOCK_Update;
		op.m_flags = UPDATE_Name | UPDATE_Options | UPDATE_Comment;
	}
	else
	{
		if ( blockMatchedByOptions == blockMatchedByName && ( nullptr != blockMatchedByOptions ) )
		{
			// nothing changed, do nothing, just put the block on the list to make sure it won't be deleted
			op.m_block = blockMatchedByOptions;
			op.m_op = BLOCK_DoNothing;
			return;
		}

		if ( nullptr != blockMatchedByOptions )
		{
			op.m_block = blockMatchedByOptions;
			op.m_name = name;
			op.m_op = BLOCK_Update;
			op.m_flags = UPDATE_Name;
			return;
		}

		if ( nullptr != blockMatchedByName )
		{  
			op.m_block = blockMatchedByName;
			op.m_comment = blockComment;
			SIDBlockEditorOp::CopyOptions( op.m_options, stubs );
			op.m_op = BLOCK_Update;
			op.m_flags = UPDATE_Options | UPDATE_Comment;
			return;
		}

		// nothing matched, spawn a new block!
		op.m_block = nullptr;
		op.m_comment = blockComment;
		op.m_name = name;
		SIDBlockEditorOp::CopyOptions( op.m_options, stubs );
		op.m_op = BLOCK_AddChoice;
		op.m_flags = UPDATE_Name | UPDATE_Options | UPDATE_Comment;
	}
}

void CEdInteractiveDialogEditor::DoUpdateChoiceBlockOptions( CIDGraphBlockChoice* block, const SIDOptionStub* stubs )
{
	for ( Uint32 i = 0; i < CHOICE_Max; ++i )
	{
		SIDOption* option = block->FindOption( static_cast< EHudChoicePosition > ( i ) );
		if ( stubs[ i ].m_position == i )
		{
			// stub filled
			if ( nullptr == option )
			{
				// add
				option = block->AddOption( static_cast< EHudChoicePosition > ( i ), stubs[ i ].m_index );
				m_dialog->MarkModified();
			}

			// update
			if ( option->m_comment != stubs[ i ].m_comment )
			{
				option->m_comment = stubs[ i ].m_comment; 
				m_dialog->MarkModified();
			}

			if ( option->m_text.GetString() != stubs[ i ].m_text )
			{
				option->m_text.SetString( stubs[ i ].m_text );
				m_dialog->MarkModified();
			}			
		}
		else
		{
			// stub not filled
			if ( option )
			{
				block->RemoveOption( *option );
				m_dialog->MarkModified();
			}
		}
	}

	block->ValidateOptions();
	block->OnRebuildSockets();
}

CIDGraphBlockText* CEdInteractiveDialogEditor::DoAddTextBlock()
{
	CIDGraphBlockText* createdBlock = m_graphEditor->CreateAndAddDialogBlock< CIDGraphBlockText > ();
	OnDoCreateBlock( createdBlock );
	return createdBlock;
}

CIDGraphBlockChoice* CEdInteractiveDialogEditor::DoAddChoiceBlock()
{
	CIDGraphBlockChoice* createdBlock = m_graphEditor->CreateAndAddDialogBlock< CIDGraphBlockChoice > ();
	OnDoCreateBlock( createdBlock );
	return createdBlock;
}

void CEdInteractiveDialogEditor::OnDoCreateBlock( CIDGraphBlock* createdBlock )
{
	// reposition
	const CIDGraphBlock* lastBlock = m_dialogTopic->GetLastBlockByGraphXPosition();
	if ( lastBlock )
	{
		Vector pos = lastBlock->GetPosition() + CEdInteractiveDialogGraphEditor::BLOCK_POS_ADD;
		createdBlock->SetPosition( pos );
	}

	// repaint graph
	m_graphEditor->Repaint();
}

void CEdInteractiveDialogEditor::DoUpdateBlockName( CIDGraphBlock* block, const CName& name )
{
	if ( block->GetName() != name )
	{
		m_dialog->MarkModified();
		block->SetName( name );
		m_dialogTopic->EnsureBlockNameUniqueness( block );
		block->OnRebuildSockets();
		m_graphEditor->Repaint();
	}
}

void CEdInteractiveDialogEditor::DoUpdateBlockComment( CIDGraphBlock* block, const String& blockComment )
{
	if ( block->GetComment() != blockComment )
	{
		m_dialog->MarkModified();
		block->SetComment( blockComment );
		block->OnRebuildSockets();
		m_graphEditor->Repaint();
	}
}

void CEdInteractiveDialogEditor::DoUpdateTextBlockLines( CIDGraphBlockText* block, const TDynArray< SIDLineStub > &stubs )
{
	Uint32 numLines = block->GetNumLines();
	if ( numLines != stubs.Size() )
	{
		m_dialog->MarkModified();
	}

	if ( block->GetNumLines() > stubs.Size() )
	{
		// need to delete some lines
		block->SetNumLines( stubs.Size() );
	}

	for ( Uint32 i = 0; i < stubs.Size(); ++i )
	{
		if ( i < numLines )
		{
			// is there a need for an update?
			if ( false == stubs[ i ].MatchesLine( block->GetLine( i ) ) )
			{
				m_dialog->MarkModified();
				block->SetLine( i, stubs[ i ] );
			}
		}
		else
		{
			// MarkModified() was already called
			block->SetLine( i, stubs[ i ] );
		}
	}

	block->OnRebuildSockets();
	m_graphEditor->Repaint();
}

void CEdInteractiveDialogEditor::FetchScreenplayContent()
{
	const wxString newContent = m_screenplayCtrl->GetValue();
	m_lastScreenplayContent = m_currentScreenplayContent;
	m_currentScreenplayContent = newContent.wc_str();
}

Bool CEdInteractiveDialogEditor::TestForScreenplayChange() const
{
	struct Smart
	{
		struct Iterator : public Red::System::NonCopyable
		{
			const String& m_s;
			Uint32 m_pos;
			RED_INLINE Iterator( const String& s ) : m_s( s ), m_pos( 0 ) {}
			RED_INLINE Char operator*() const { return m_pos < m_s.Size() ? m_s[ m_pos ] : 0; }
			RED_INLINE operator Bool() const { return m_pos < m_s.Size(); }
			RED_INLINE void operator++() 
			{ 
				while ( ++m_pos < m_s.Size() )
				{ 
					Char c = **this; 
					if ( c != '\n' && c != '\t' ) 
					{
						break; 
					}
				}
			}
		};

		static Bool Compare( const String& s1, const String& s2 )
		{
			Iterator i1( s1 );
			Iterator i2( s2 );
			for ( ; i1 && i2; ++i1, ++i2 )
			{
				if ( ( *i1 ) != ( *i2 ) )
				{
					return false;
				}
			}

			return !( i1 ) && !( i2 ); 
		}
	};

	const Bool areEqual = Smart::Compare( m_lastScreenplayContent, m_currentScreenplayContent );
	return !areEqual;
}

void CEdInteractiveDialogEditor::ApplyScreenplayChangesIfNeeded()
{
	if ( m_fillingScreenplayNow )
	{
		return;
	}

	if ( nullptr == m_dialogTopic )
	{
		return;
	}

	FetchScreenplayContent();
	if ( TestForScreenplayChange() )
	{
		ApplyScreenplayChanges();
		FillScreenplay();
	}
}

void CEdInteractiveDialogEditor::HiliteBlockUnderScreenplayCaret()
{
	wxString blockGuid;
	wxRichTextObject* obj = m_screenplayCtrl->GetBuffer().GetLeafObjectAtPosition( m_screenplayCtrl->GetAdjustedCaretPosition( m_screenplayCtrl->GetCaretPosition() ) );
	if ( obj )
	{
		blockGuid = obj->GetProperties().GetPropertyString( PROPERTY_GUID_NAME );
	}

	CGUID guid;
	if ( false == blockGuid.empty() )
	{
		guid.FromString( blockGuid.wx_str() );
	}

	// RED_LOG( CNAME( Dialog ), TXT("%ld, %ld, block: %s"), m_screenplayCtrl->GetCaretPosition(), m_screenplayCtrl->GetAdjustedCaretPosition( m_screenplayCtrl->GetCaretPosition() ), blockGuid.wx_str() );  

	if ( false == guid.IsZero() )
	{
		for ( CIDTopic::BlockIterator< CIDGraphBlock > it( m_dialogTopic ); it; ++it )
		{
			if ( ( *it )->GetGuid() == guid )
			{
				m_graphEditor->HiliteBlock( *it );
				SetPropertiesObject( *it, false );
				return;
			}
		}
	}

	m_graphEditor->HiliteBlock( nullptr );
}

void CEdInteractiveDialogEditor::OnBeginScreenplayPasting()
{
	m_fillingScreenplayNow = true;
}

void CEdInteractiveDialogEditor::OnEndScreenplayPasting()
{
	m_fillingScreenplayNow = false;
	ApplyScreenplayChangesIfNeeded();
}

void CEdInteractiveDialogEditor::OnBlockSelected( CIDGraphBlock* block )
{
	if ( block->IsA< CIDGraphBlockText > () || block->IsA< CIDGraphBlockChoice > () )
	{
		m_screenplayCtrl->SetRelativeCaretPosition( block->GetName(), INT_MAX, 0, false );
		m_screenplayCtrl->SetScrollToCaret( true );
		FillScreenplay();
	}
}

#endif







