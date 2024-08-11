/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "SciLexer.h"
#include "Scintilla.h"
#include "breakpoints.h"

#include "CodeTooltip.h"

#include "events/eventbreakpoints.h"
#include "events/eventbookmarks.h"
#include "events/eventLineMove.h"
#include "events/eventNavigationHistory.h"
#include "events/eventGoto.h"
#include "events/eventOpcodeListing.h"

#include "documentView.h"

#include "app.h"
#include "frame.h"
#include "fileStubs.h"
#include "lexData.h"

#include "solution/slnDeclarations.h"
#include "solution/slnContainer.h"

IMPLEMENT_CLASS( CSSDocument, CSSStyledDocument );

BEGIN_EVENT_TABLE( CSSDocument, CSSStyledDocument )
	EVT_STC_MARGINCLICK( wxID_ANY, CSSDocument::OnMarginClick )
	EVT_STC_CHANGE( wxID_ANY, CSSDocument::OnContentModified )
	EVT_STC_CHARADDED( wxID_ANY, CSSDocument::OnCharAdded )
	EVT_STC_ROMODIFYATTEMPT( wxID_ANY, CSSDocument::OnRequestCheckout )
	EVT_KEY_DOWN( CSSDocument::OnKeyDown )
	EVT_STC_UPDATEUI( wxID_ANY, CSSDocument::OnUpdateUI )
	EVT_TIMER( 12345, CSSDocument::OnModifiedTimer )
	EVT_STC_DWELLSTART( wxID_ANY, CSSDocument::OnMouseDwellStart )
	EVT_STC_DWELLEND( wxID_ANY, CSSDocument::OnMouseDwellEnd )
	EVT_ENTER_WINDOW( CSSDocument::OnMouseEnter )
	EVT_LEAVE_WINDOW( CSSDocument::OnMouseExit )
	EVT_STC_MODIFIED( wxID_ANY, CSSDocument::OnModified )
	EVT_LEFT_DOWN( CSSDocument::OnClick )
	EVT_STC_DOUBLECLICK( wxID_ANY, CSSDocument::OnDoubleClick )
END_EVENT_TABLE()

CSSDocument::CSSDocument( wxWindow* parent, const SolutionFilePtr& file, Solution* solution )
:	CSSStyledDocument( parent )
,	m_doNotMonitorChanges( true )
,	m_file( file )
,	m_modifiedTimer( this, 12345 )
,	m_previousMatchingBracePairIndex( -1 )
,	m_scintillaForceRedrawAnnotationHack( false )
,	m_mouseOverWindow( false )
,	m_highlightedWord( wxT("") )
,	m_solution( solution )
{
	SendMsg( SCI_SETSCROLLWIDTHTRACKING, 1, 0 );

	// We only want document modified events for the text changing, not the style
	SetModEventMask
	(
		wxSTC_MOD_INSERTTEXT |
		wxSTC_MOD_DELETETEXT
	);

	SetMouseDwellTime( 500 );
	wxToolTip::SetReshow( 500 );
	wxToolTip::SetDelay( 500 );
	wxToolTip::SetMaxWidth( 500 );

	// Auto complete icons
	ClearRegisteredImages();
	RegisterImage( 0, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_CONST") ) );
	RegisterImage( 1, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_CONST_IMPORTED") ) );
	RegisterImage( 2, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_CLASS") ) );
	RegisterImage( 3, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_CLASS_IMPORTED") ) );
	RegisterImage( 4, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_ENUM") ) );
	RegisterImage( 5, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_ENUM_IMPORTED") ) );
	RegisterImage( 6, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_EVENT") ) );
	RegisterImage( 7, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_EVENT_IMPORTED") ) );
	RegisterImage( 8, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_FIELD") ) );
	RegisterImage( 9, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_FIELD_IMPORTED") ) );
	RegisterImage( 10, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_METHOD") ) );
	RegisterImage( 11, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_METHOD_IMPORTED") ) );
	RegisterImage( 12, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_STRUCT") ) );
	RegisterImage( 13, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_STRUCT_IMPORTED") ) );
	RegisterImage( 14, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_STATE") ) );
	RegisterImage( 15, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_ENUMOPTION") ) );
	RegisterImage( 16, wxTheSSApp->LoadBitmap( wxT("IMG_OBJECT_ENTRY") ) );

	// Auto complete style
	AutoCompSetIgnoreCase( true );
	AutoCompSetAutoHide( false );
	AutoCompSetMaxHeight( 10 );
	AutoCompSetMaxWidth( 200 );

	// Multiple selection
	SetMultipleSelection( true );
	SetRectangularSelectionModifier( wxSTC_SCMOD_ALT );
	SetVirtualSpaceOptions( wxSTC_SCVS_RECTANGULARSELECTION );
	SetAdditionalSelectionTyping( false );
	SetAdditionalCaretsVisible( false );

	// Load file content
	LoadFile();
}

CSSDocument::~CSSDocument()
{
}

void CSSDocument::ValidateFile()
{
	SolutionFilePtr newFile = m_solution->FindFile( m_file->m_solutionPath );

	if ( !newFile )
	{
		wxTheFrame->CloseFile( m_file, false );
		return;
	}

	m_file = newFile;
	m_file->m_document = this;
}

void CSSDocument::LoadFile()
{
	// Do not monitor file changes
	m_doNotMonitorChanges = true;

	// Enable file access
	SetReadOnly( false );

	// Reset file
	Cancel();
	SetUndoCollection( false );

	// Load file
	wxString absolutePath = m_file->m_absolutePath.c_str();

	bool fileExistsOnDisk = wxFileExists( absolutePath );

	if( fileExistsOnDisk )
	{
		wxFile readFile( absolutePath );

		if ( readFile.IsOpened() )
		{
			// Load string from file
			const wxFileOffset size = readFile.Length();
			char* rawText = (char*) RED_ALLOCA( size + 2 );
			readFile.Read( rawText, size );

			// Null terminate
			rawText[ size + 0 ] = 0;
			rawText[ size + 1 ] = 0;

			wxBOM bom = wxConvAuto::DetectBOM( rawText, size );

			wxString text;

			switch( bom )
			{
			case wxBOM_None:
			case wxBOM_Unknown:
				text = wxString( rawText, wxConvLocal );
				break;

			default:
				text = wxString( rawText, wxConvAuto() );
			}

			SetText( text );
		}
	}

	// Enable undo
	SetUndoCollection( true );
	EmptyUndoBuffer();
	SetSavePoint();

	// Enable readonly for files that are not checked out
	SetReadOnly( !m_file->CanModify() );
	m_doNotMonitorChanges = false;

	// Is not modified
	m_file->CancelModified();

	if( fileExistsOnDisk )
	{
		// Update last modification time
		m_file->UpdateModificationTime();
	}
}

void CSSDocument::Undo()
{
	wxStyledTextCtrl::Undo();

	if ( GetModify() )
	{
		m_file->MarkModified();
	}
	else 
	{
		m_file->CancelModified();
	}
}

void CSSDocument::Redo()
{
	wxStyledTextCtrl::Redo();

	if ( GetModify() )
	{
		m_file->MarkModified();
	}
	else 
	{
		m_file->CancelModified();
	}
}

bool CSSDocument::Save()
{
	if( !m_file->IgnoreChanges() )
	{
		wxString filePath( m_file->m_absolutePath.c_str() );
		wxFileName filePathHelper( filePath );
		wxString directoryPath = filePathHelper.GetPath();

		// Create path
		if ( !wxDir::Make( directoryPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
		{
			return false;
		}

		// Save file
		wxFile writeFile( filePath, wxFile::write );
		if ( writeFile.IsOpened() )
		{
			// Save code to file (as Unicode)
			wxString text = GetText();

			size_t count = 0;
			const char* bom = wxConvAuto::GetBOMChars( wxBOM_UTF16LE, &count );
			writeFile.Write( bom, count );
			writeFile.Write( text, wxMBConvUTF16() );
			writeFile.Close();

			// Save file
			SetSavePoint();

			// Mark as not modified
			m_file->CancelModified();

			m_file->UpdateModificationTime();

			// Saved
			return true;
		}
	}

	return false;
}

void CSSDocument::OnRequestCheckout( wxStyledTextEvent& )
{
	if ( !m_doNotMonitorChanges )
	{
		if( m_file->IsInDepot() )
		{
			if ( !m_file->IsCheckedOut() )
			{
				if ( wxMessageBox( wxString( "Checkout ") + m_file->m_absolutePath.c_str() + wxT( "?" ), wxT( "File in source control" ), wxICON_QUESTION | wxOK | wxCANCEL | wxCANCEL_DEFAULT ) == wxOK )
				{
					// Try to checkout file
					m_file->CheckOut();
					m_file->RefreshStatus();

					// Still not checked out
					if ( !m_file->IsCheckedOut() )
					{
						wxMessageBox( wxT("Unable to checkout file"), wxT("Checkout error"), wxOK | wxICON_ERROR );
						return;
					}

					// Reload file if modification occured
					if( GetFile()->CheckModificationTime() == false )
					{
						LoadFile();
					}

					// Update frame buttons
					wxTheFrame->DocumentContentModified( this );
				}
			}
		}
		else if( m_file->IsReadOnlyFlagSet() )
		{
			if ( wxMessageBox( wxString( "Remove read only flag for " ) + m_file->m_absolutePath.c_str() + wxT( "?" ), wxT( "Unmanaged file (Or source control disabled)" ), wxICON_QUESTION | wxOK | wxCANCEL | wxCANCEL_DEFAULT ) == wxOK )
			{
				m_file->ClearReadOnlyFlag();
				m_file->RefreshStatus();

				// Update frame buttons
				wxTheFrame->DocumentContentModified( this );
			}
		}

		// Restore focus
		SetFocus();
	}
}

void CSSDocument::OnMarginClick( wxStyledTextEvent& event )
{
	int margin = event.GetMargin();
	int pos  = event.GetPosition();
	int line = LineFromPosition( pos );

	switch( margin )
	{
	case MARGIN_Breakpoints:
	case MARGIN_LineNumbers:
		{
			ToggleBreakpoint( line );
		}
		break;

	case MARGIN_CodeFolding:
		{
			int foldLevel = GetFoldLevel( line );
			int foldParent = GetFoldParent( line );

			RED_LOG( RED_LOG_CHANNEL( Document ), TXT( "Fold Level: %i, Parent: %i" ), foldLevel, foldParent );

			if( foldLevel & SC_FOLDLEVELHEADERFLAG )
			{
				RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), TXT( "Folding!" ) );
				ToggleFold( line );
			}
		}
		break;

	case MARGIN_Opcodes:
		{
			int lineMarkers = MarkerGet( line );

			SSClassStub* classStub = nullptr;
			SSFunctionStub* functionStub = nullptr;

			GStubSystem.FindFunctionAtLine( m_file->m_solutionPath, line + 1, classStub, functionStub );

			if( functionStub )
			{
				// If we've got a valid breakpoint, or a breakpoint waiting to be validated, delete it
				if ( lineMarkers & ( FLAG( MARKER_OpcodesHidden ) ) )
				{
					MarkerDelete( line, MARKER_OpcodesHidden );
					MarkerAdd( line, MARKER_OpcodesShown );

					Red::Network::Manager* network = Red::Network::Manager::GetInstance();
					if( network && network->IsInitialized() )
					{
						Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

						RED_VERIFY( packet.WriteString( "OpcodeBreakdownRequest" ) );
						RED_VERIFY( packet.WriteString( functionStub->m_name.c_str() ) );

						if( classStub )
						{
							RED_VERIFY( packet.Write( true ) );
							RED_VERIFY( packet.WriteString( classStub->m_name.c_str() ) );
						}
						else
						{
							RED_VERIFY( packet.Write( false ) );
						}

						network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
					}
				}
				else
				{
					MarkerDelete( line, MARKER_OpcodesShown);
					MarkerAdd( line, MARKER_OpcodesHidden );

					int numLinesToClear = functionStub->m_lastLine - functionStub->m_context.m_line;

					for( int i = 0; i < numLinesToClear; ++i )
					{
						// The wxSTC widget does not have a simple "ClearAnnotation()" wrapper function, so we need to manually call scintilla directly
						SendMsg( 2540, line + i );

						int redrawFromPosition = PositionFromLine( line - 1 );
						int redrawToPosition = PositionFromLine( line + 1 );
						HACKForceRedraw( redrawFromPosition, redrawToPosition );
					}
				}
			}
		}
		break;
	}
}

void CSSDocument::OnContentModified( wxStyledTextEvent& event )
{
	// Make sure the event is for this document, and not a child (such as a code tool tip)
	if( event.GetEventObject() == this)
	{
		if ( !m_doNotMonitorChanges )
		{
			// Try to checkout file
			m_file->MarkModified();

			// Schedule code analyze after 500ms
			if ( !GetReadOnly() )
			{
				m_modifiedTimer.Stop();
				m_modifiedTimer.Start( 500, true );
			}

			// Update frame buttons
			wxTheFrame->DocumentContentModified( this );
		}

		wxString text;
		text << GetLineCount();
		int width = TextWidth( STYLE_LINENUMBER, text + wxT(' ') );
		SetMarginWidth( 0, width );
	}

	event.Skip();
}

void CSSDocument::OnModified( wxStyledTextEvent& event )
{
	if
	(
		// Don't do anything if we're expecting changes (like a file reload)
		!m_doNotMonitorChanges &&

		// Make sure the modification type is one we're interested in
		event.GetModificationType() &
		(
			wxSTC_MOD_INSERTTEXT |
			wxSTC_MOD_DELETETEXT
		)

		// Make sure the event is for this document, and not a child (such as a code tool tip)
		&& event.GetEventObject() == this
	)
	{
		int added = event.GetLinesAdded();
		int line = LineFromPosition( event.GetPosition() );

		if( added != 0 )
		{
			CLineMoveEvent* moveEvent = new CLineMoveEvent( m_file, ++line, added );
			QueueEvent( moveEvent );
		}
	}

	event.Skip();
}

void CSSDocument::SetBookmark( int line, EMarkerState state )
{
	// Scintilla adjustment
	--line;

	MarkerDelete( line, MARKER_Bookmark );
	MarkerDelete( line, MARKER_DisabledBookmark );

	switch( state )
	{
	case Marker_Enabled:
		MarkerAdd( line, MARKER_Bookmark );
		break;

	case Marker_Disabled:
		MarkerAdd( line, MARKER_DisabledBookmark );
		break;
	}
}

void CSSDocument::SetBreakpoint( int line, EMarkerState state )
{
	// Scintilla adjustment
	--line;

	MarkerDelete( line, MARKER_Breakpoint );
	MarkerDelete( line, MARKER_DisabledBreakpoint );
	MarkerDelete( line, MARKER_UnconfirmedBreakpoint );

	switch( state )
	{
	case Marker_Enabled:
		MarkerAdd( line, MARKER_UnconfirmedBreakpoint );
			break;

	case Marker_Disabled:
		MarkerAdd( line, MARKER_DisabledBreakpoint );
		break;
	}
}

void CSSDocument::ToggleBookmark()
{
	// Do not set when not active
	if ( HasFocus() )
	{
		// Get the current line
		const int currentLine = GetCurrentLine();
		ToggleBookmark( currentLine );
	}
}

void CSSDocument::ToggleBookmark( int line )
{
	int lineMarkers = MarkerGet( line );

	// If we've got a valid bookmark, delete it
	if ( lineMarkers & ( FLAG( MARKER_Bookmark ) ) )
	{
		// "+1" because we start counting lines from 1, but scintilla counts from 0
		CBookmarkToggledEvent* event = new CBookmarkToggledEvent( m_file, line + 1, Marker_Deleted );
		QueueEvent( event );
	}

	// Otherwise, we've got a disabled bookmark, or no bookmark at all
	else
	{
		// "+1" because we start counting lines from 1, but scintilla counts from 0
		CBookmarkToggledEvent* event = new CBookmarkToggledEvent( m_file, line + 1, Marker_Enabled );
		QueueEvent( event );
	}
}

void CSSDocument::ToggleBreakpoint()
{
	// Do not set when not active
	if ( HasFocus() )
	{
		// Get the current line
		const int currentLine = GetCurrentLine();
		ToggleBreakpoint( currentLine );
	}
}

void CSSDocument::ToggleBreakpoint( int line )
{
	int lineMarkers = MarkerGet( line );

	// If we've got a valid breakpoint, or a breakpoint waiting to be validated, delete it
	if ( lineMarkers & ( FLAG( MARKER_UnconfirmedBreakpoint ) | FLAG( MARKER_Breakpoint ) ) )
	{
		// "+1" because we start counting lines from 1, but scintilla counts from 0
		CBreakpointToggledEvent* event = new CBreakpointToggledEvent( m_file, line + 1, Marker_Deleted );
		QueueEvent( event );
	}

	// Otherwise, we've got a disabled breakpoint, or no breakpoint at all
	else
	{
		// "+1" because we start counting lines from 1, but scintilla counts from 0
		CBreakpointToggledEvent* event = new CBreakpointToggledEvent( m_file, line + 1, Marker_Enabled );
		QueueEvent( event );
	}
}

void CSSDocument::ConfirmBreakpointToggle( int line, bool success )
{
	// decrement because we start counting lines from 1, but scintilla counts from 0
	--line;

	if ( success && MarkerGet( line ) & FLAG( MARKER_UnconfirmedBreakpoint ) )
	{
		MarkerDelete( line, MARKER_UnconfirmedBreakpoint );
		MarkerAdd( line, MARKER_Breakpoint );
	}
}

void CSSDocument::SetCallstackMarker( int type, int line )
{
	// Because we start counting lines from 1, but scintilla counts from 0
	--line;

	if ( line < 0 )
	{
		MarkerDeleteAll( type ? MARKER_FunctionArrow : MARKER_Arrow );
	}
	else
	{
		MarkerAdd( line, type ? MARKER_FunctionArrow : MARKER_Arrow );
	}
}

void CSSDocument::OnModifiedTimer( wxTimerEvent& )
{
	if( wxTheFrame )
	{
		wxTheFrame->SetStatusBarText( wxT( "Reparsing: %s" ), m_file->m_name.c_str() );
	}

	// Reparse document
	GStubSystem.Schedule( m_file );
}

static bool IsAlphaNum( wxChar ch )
{
	if ( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ( ch == '_' ) )
	{
		return true;
	}

	return  false;
}

static wxArrayString Tokenize( const wxString& line )
{
	wxArrayString tokens;

	// Initialize string
	wxChar buffer[ 1024 ];
	Red::System::StringCopy( buffer, line.c_str(), ARRAY_COUNT( buffer ) );

	// Tokenize
	wxChar* cur = buffer;
	while ( *cur )
	{
		// White space
		if ( *cur <= 32 )
		{
			cur++;
			continue;
		}

		// Special char
		if ( !IsAlphaNum( *cur ) )
		{
			// Create token from single char
			wxChar str[] = { *cur, 0 };
			tokens.Add( str );

			// Go to next
			cur++;
			continue;
		}

		// Start a token
		wxString token;
		while ( *cur )
		{
			// Whitespace or not an alphanum, break
			if ( !IsAlphaNum( *cur ) )
			{
				break;
			}

			// Add to token
			wxChar str[] = { *cur, 0 };
			token += str;

			// Move
			cur++; 
		}

		// Add token to token list
		tokens.Add( token );
	}

	// Tokenized
	return tokens;
}

static wxArrayString StripBrackets( const wxArrayString& tokens, wxChar leftBraket, wxChar rightBraket )
{
	// Trash any unmatched brackets
	int count = 0;
	for ( int i=(int)tokens.GetCount()-1; i>=0; i-- )
	{
		wxString token = tokens[i];
		if ( !token.IsEmpty() )
		{
			// Opening bracket
			if ( token.GetChar(0) == leftBraket )
			{
				count--;

				// Unmatched bracket
				if ( count < 0 )
				{
					// Return stripped token list
					wxArrayString newTokens;
					for ( size_t j=i+1; j<tokens.GetCount(); j++ )
					{
						newTokens.Add( tokens[j] );
					}
					return newTokens;
				}
			}

			// Closing bracket
			if ( tokens[i].GetChar(0) == rightBraket )
			{
				count++;
			}
		}
	}

	// Unmatched brackets, crap
	if ( count != 0 )
	{
		wxArrayString emptyTokenList;
		return emptyTokenList;
	}

	// No changes
	return tokens;
}

static bool IsNonTrash( const wxString& token )
{
	// Brackets :)
	if ( token.Length() == 1 )
	{
		// Keep brackets
		wxChar ch = token[0];
		if ( ch == '(' || ch == ')' || ch == '[' || ch == ']' )
		{
			return true;
		}

		// Keep dot
		if ( ch == '.' )
		{
			return true;
		}
	}

	// Check for invalid char
	for ( size_t i=0; i<token.Length(); i++ )
	{
		if ( !IsAlphaNum( token[i] ) )
		{
			return false;
		}
	}


	// Not a trash
	return true;
}

static wxArrayString StripCrap( const wxArrayString& tokens )
{
	// Trash any non keywords and non brackets
	for ( int i=(int)tokens.GetCount()-1; i>=0; --i )
	{
		wxString token = tokens[i];
		if ( !IsNonTrash( token ) )
		{
			// Return stripped token list
			wxArrayString newTokens;
			for ( size_t j=i+1; j<tokens.GetCount(); ++j )
			{
				newTokens.Add( tokens[j] );
			}
			return newTokens;
		}
	}

	// No changes
	return tokens;
}

static wxArrayString StripSymbolPath( const wxString& line )
{
	// Tokenize using dots
	wxArrayString tokens = Tokenize( line );

	// Strip unneeded brackets
	tokens = StripBrackets( tokens, '(', ')' );
	tokens = StripBrackets( tokens, '[', ']' );

	// remove crap
	tokens = StripCrap( tokens );

	// Done
	return tokens;
}

bool CSSDocument::StripSymbolPath( wxArrayString& tokens, wxString& partialSymbol )
{
	// Get current line
	wxString line = GetLine( GetCurrentLine() );
	if ( !line.IsEmpty() )
	{
		// Clamp to caret position
		int caretPosition = GetCurrentPos();
		caretPosition -= PositionFromLine( GetCurrentLine() );

		return StripSymbolPath( tokens, partialSymbol, line, caretPosition );
	}

	return false;
}

bool CSSDocument::StripSymbolPath( wxArrayString& tokens, wxString& partialSymbol, wxString line, int caretLinePosition )
{
	// Clamp string
	if ( caretLinePosition > 0 )
	{
		// Cut the line
		if
		(
			line[caretLinePosition-1] == wxT( '.' ) ||
			line[caretLinePosition-1] == wxT( '(' ) ||
			line[caretLinePosition-1] == wxT( ')' ) ||
			line[caretLinePosition-1] == wxT( '[' ) ||
			line[caretLinePosition-1] == wxT( ']' )
		)
		{
			// No partial symbol entered
			partialSymbol = wxEmptyString;
			line = line.Left( caretLinePosition-1 );

			// Get the symbol path in form of tokens
			tokens = ::StripSymbolPath( line );
			return !tokens.IsEmpty();
		}
		else
		{
			// Get the symbol path in form of tokens
			line = line.Left( caretLinePosition );
			tokens = ::StripSymbolPath( line );
			if ( !tokens.IsEmpty() )
			{
				// Last symbol is the partial token match
				partialSymbol = tokens.Last();
				tokens.RemoveAt( tokens.GetCount()-1 );

				// Remove handing dot
				if ( !tokens.IsEmpty() && tokens.Last() == wxT(".") )
				{
					tokens.RemoveAt( tokens.GetCount()-1 );
				}

				// Done
				return true;
			}
		}
	}

	// No valid data
	return false;
}

static int CompareTypeInfo(const void* a, const void* b)
{
	SSBasicStub* typeA = *(SSBasicStub**)a;
	SSBasicStub* typeB = *(SSBasicStub**)b;
	return _wcsicmp( typeA->m_name.c_str(), typeB->m_name.c_str() );
}

static int FunctionNameCompare( const void* a, const void* b )
{
	SSFunctionStub* typeA = *(SSFunctionStub**)a;
	SSFunctionStub* typeB = *(SSFunctionStub**)b;
	wxString strA = ( typeA->m_className + wxT( "::" ) + typeA->m_name ).c_str();
	wxString strB = ( typeB->m_className + wxT( "::" ) + typeB->m_name ).c_str();
	
	return strA.CmpNoCase( strB );
}

static int WxFunctionNameCompare( const wxString& a, const wxString& b )
{
	return a.CmpNoCase( b );
}

static wxString FormatAutoComplete( const vector< SSBasicStub* >& possibleSymbols )
{
	wxString str;

	// Sort list
	vector< SSBasicStub* > symbols = possibleSymbols;
	qsort( &symbols[0], symbols.size(), sizeof( SSBasicStub* ), CompareTypeInfo );

	// Add symbol entries
	wxArrayString addedElements;
	for ( size_t i=0; i<symbols.size(); i++ )
	{
		SSBasicStub* stub = symbols[i];
		if ( !stub->m_name.empty() )
		{
			// Do not add duplicated symbols
			if ( addedElements.Index( stub->m_name.c_str() ) != -1 )
			{
				continue;
			}

			// Add separator
			str += str.IsEmpty() ? wxEmptyString : wxT(" ");

			// Add name
			str += stub->m_name.c_str();
			addedElements.Add( stub->m_name.c_str() );

			// Determine icon
			if ( stub->m_type == STUB_Function )
			{
				if ( stub->m_flags.Has( wxT("entry") ) )
				{
					str += wxT("?16"); // Entry function
				}
				else if ( stub->m_flags.Has( wxT("event") ) )
				{
					if ( stub->m_flags.Has( wxT("import") ) )
					{
						str += wxT("?7"); // Event imported
					}
					else
					{
						str += wxT("?6"); // Event
					}
				}
				else
				{
					if ( stub->m_flags.Has( wxT("import") ) )
					{
						str += wxT("?11"); // Method imported
					}
					else
					{
						str += wxT("?10"); // Method
					}
				}
			}
			else if ( stub->m_type == STUB_Property )
			{
				if ( stub->m_flags.Has( wxT("import") ) )
				{
					str += wxT("?9"); // Property imported
				}
				else
				{
					str += wxT("?8"); // Property
				}
			}
			else if ( stub->m_type == STUB_Class )
			{
				if ( stub->m_flags.Has( wxT("import") ) )
				{
					str += wxT("?3"); // Class imported
				}
				else
				{
					str += wxT("?2"); // Class 
				}
			}
			else if ( stub->m_type == STUB_Struct )
			{
				if ( stub->m_flags.Has( wxT("import") ) )
				{
					str += wxT("?13"); // Class imported
				}
				else
				{
					str += wxT("?12"); // Class 
				}
			}
			else if ( stub->m_type == STUB_Enum )
			{
				if ( stub->m_flags.Has( wxT("import") ) )
				{
					str += wxT("?5"); // Enum imported
				}
				else
				{
					str += wxT("?4"); // Enum
				}
			}
			else if ( stub->m_type == STUB_State )
			{
				str += wxT("?14"); // State
			}
			else if ( stub->m_type == STUB_EnumOption )
			{
				str += wxT("?15"); // Enum option
			}
			else
			{
				str += wxT("?0"); // Dunno
			}
		}
	}

	return str;
}

void CSSDocument::TryAutoComplete( bool force )
{
	SSStubSystemReadLock lock;

	// Get context 
	SSClassStub* classContext = NULL;
	SSFunctionStub* functionContext = NULL;
	if ( GStubSystem.FindFunctionAtLine( m_file->m_solutionPath, GetCurrentLine()+1, classContext, functionContext ) )
	{
		// Get tokens
		wxString partialSymbol;
		wxArrayString tokens;
		if ( StripSymbolPath( tokens, partialSymbol ) )
		{
			// Get the tokens 
			vector< wstring > symbolPath;
			for ( size_t i=0; i<tokens.size(); i++ )
			{
				symbolPath.push_back( tokens[i].wc_str() );
			}

			// Get visible symbols to show
			vector< SSBasicStub* > symbols;			
			if ( GStubSystem.CollectVisibleSymbols( classContext, functionContext, symbolPath, partialSymbol.wc_str(), force, symbols ) )
			{
				// Show the auto complete window
				wxString data = FormatAutoComplete( symbols );
				AutoCompShow( partialSymbol.Length(), data );
			}
		}
	}
}

SSFunctionStub* CSSDocument::TryFunctionCall( const SolutionFilePtr& file, int line, int caretLinePosition, const wxString& lineText )
{
	SSStubSystemReadLock lock;

	// Get context 
	SSClassStub* classContext = NULL;
	SSFunctionStub* functionContext = NULL;
	if ( GStubSystem.FindFunctionAtLine( file->m_solutionPath, line, classContext, functionContext ) )
	{
		// Get tokens
		wxString partialSymbol;
		wxArrayString tokens;

		bool success = false;
		if( caretLinePosition == -1 )
		{
			success = StripSymbolPath( tokens, partialSymbol );
		}
		else
		{
			success = StripSymbolPath( tokens, partialSymbol, lineText, caretLinePosition );
		}


		if ( success )
		{
			// Get the tokens 
			vector< wstring > symbolPath;
			for ( size_t i = 0; i < tokens.size(); ++i )
			{
				symbolPath.push_back( tokens[i].wc_str() );
			}

			// Resolve function
			return GStubSystem.ResolveFunctionCall( classContext, functionContext, symbolPath );
		}
	}

	return NULL;
}

void CSSDocument::TryCallTip()
{
	SSFunctionStub* func = TryFunctionCall( m_file, GetCurrentLine() + 1 );
	if ( func )
	{
		wxString callTip;

		// Return type
		if ( !func->m_retValueType.empty() )
		{
			callTip += func->m_retValueType.c_str();
			callTip += wxT(" : ");
		}

		// Function name
		callTip += func->m_name.c_str();
		callTip += wxT("(");

		// Params
		for ( size_t i=0; i<func->m_params.size(); i++ )
		{
			// Separator
			if ( i )
			{
				callTip += wxT(", ");
			}

			// Param type
			callTip += func->m_params[i]->m_name.c_str();
			callTip += wxT(" : ");
			callTip += func->m_params[i]->m_typeName.c_str();
		}

		// Tail
		callTip += wxT(")");

		// Show call tip
		CallTipShow( GetCurrentPos(), callTip );
	}
}

void CSSDocument::TryGotoImplementation()
{
	TryGotoImplementation( GetCurrentPos() );
}

void CSSDocument::TryGotoImplementation( int position )
{
	SSStubSystemReadLock lock;

	// Get context
	SSClassStub* classContext = NULL;
	SSFunctionStub* functionContext = NULL;
	if ( GStubSystem.FindFunctionAtLine( m_file->m_solutionPath, GetCurrentLine() + 1, classContext, functionContext ) )
	{
		int start = WordStartPosition( position, true );
		int end = WordEndPosition( position, true );
		wxString symbolAtPosition = GetTextRange( start, end );

		// Get tokens
		wxString partialSymbol;
		wxArrayString tokens;
		if ( StripSymbolPath( tokens, partialSymbol ) )
		{
			// Get the tokens 
			vector< wstring > symbolPath;
			for ( size_t i = 0; i < tokens.size(); ++i )
			{
				wstring currToken = tokens[i].wc_str();
				if ( currToken != wxT( "return" ) && currToken != wxT( "new" ) )
				{
					symbolPath.push_back( currToken );
				}
			}

			// Get visible symbols to show
			vector< SSBasicStub* > symbols;
			if ( GStubSystem.CollectVisibleSymbols( classContext, functionContext, symbolPath, symbolAtPosition.wc_str(), true, symbols ) )
			{
				if ( symbols.size() > 0 )
				{
					RED_LOG( RED_LOG_CHANNEL( GotoImplementation ), TXT( "%ls(%u)" ), symbols[ 0 ]->m_context.m_file.c_str(), symbols[ 0 ]->m_context.m_line );

					const wchar_t* file = symbols[ 0 ]->m_context.m_file.c_str();
					int lineNumber = symbols[ 0 ]->m_context.m_line;

					CGotoEvent* newEvent = new CGotoEvent( file, lineNumber, true );
					QueueEvent( newEvent );
				} 
			}
		}
	}
}

void CSSDocument::OnCharAdded( wxStyledTextEvent& event )
{
	// Dot ! Try auto complete
	if ( event.GetKey() == '.' )
	{
		TryAutoComplete( false );
		return;
	}

	// Bracket, try function call
	if ( event.GetKey() == '(' )
	{
		TryCallTip();
		return;
	}

	// New line, auto indent
	if  ( event.GetKey() == '\r' )
	{
		// Get length of current line
		int curLine = GetCurrentLine();
		int lineLength = SendMsg( SCI_LINELENGTH, curLine );

		// Valid new line
		if ( curLine > 0 && lineLength <= 2 )
		{
			wxString line = GetLine( curLine-1 );

			// Separate indentation
			for ( size_t pos=0; pos<line.Length(); pos++ )
			{
				if  ( line[pos] != ' ' && line[pos] != '\t' )
				{
					line = line.Left( pos );
					break;
				}
			}

			// Replace selection
			ReplaceSelection( line );
		}
	}
}

void CSSDocument::OnKeyDown( wxKeyEvent& event )
{
	if ( event.ControlDown() && event.GetKeyCode() == WXK_SPACE )
	{
		TryAutoComplete( true );
		return;
	}
	else if ( event.ControlDown() && event.GetKeyCode() == WXK_TAB )
	{
		if ( event.ShiftDown() )
		{
			wxTheFrame->GotoNextFilesTab( true, -1 );
		}
		else
		{
			wxTheFrame->GotoNextFilesTab( true, 1 );
		}
		return;
	}
	else if ( event.AltDown() && event.GetKeyCode() == 'G' )
	{
		TryGotoImplementation();
		return;
	}
	else if ( event.AltDown() && event.GetKeyCode() == 'M' )
	{
		m_file->m_documentEx->SetComboFocus();
		return;
	}
	else if ( event.AltDown() && event.GetKeyCode() == 'D' )
	{
		SSClassStub* classStub = nullptr;
		SSFunctionStub* functionStub = nullptr;

		GStubSystem.FindFunctionAtLine( m_file->m_solutionPath, GetCurrentLine() + 1, classStub, functionStub );

		if( functionStub )
		{
			Red::Network::Manager* network = Red::Network::Manager::GetInstance();
			if( network && network->IsInitialized() )
			{
				Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT_DEBUGGER );

				RED_VERIFY( packet.WriteString( "OpcodeBreakdownRequest" ) );
				RED_VERIFY( packet.WriteString( functionStub->m_name.c_str() ) );

				if( classStub )
				{
					RED_VERIFY( packet.Write( true ) );
					RED_VERIFY( packet.WriteString( classStub->m_name.c_str() ) );
				}
				else
				{
					RED_VERIFY( packet.Write( false ) );
				}

				network->Send( RED_NET_CHANNEL_SCRIPT_DEBUGGER, packet );
			}
		}

		return;
	}
	else if( !event.ControlDown() && !event.ShiftDown() && event.AltDown() && event.GetKeyCode() == WXK_UP )
	{
// 		BeginUndoAction();
// 
// 		// Selection position to be preserved
// 		int originalSelectionPositionStart = GetAnchor();
// 		int originalSelectionPositionEnd = GetCurrentPos();
// 
// 		// 
// 		bool swapped = false;
// 		if( originalSelectionPositionStart > originalSelectionPositionEnd )
// 		{
// 			wxSwap( originalSelectionPositionStart, originalSelectionPositionEnd );
// 			swapped = true;
// 		}
// 
// 		int originalSelectionPositionStartColumn = GetColumn( originalSelectionPositionStart );
// 		int originalSelectionPositionEndColumn = GetColumn( originalSelectionPositionEnd );
// 
// 		int originalCaretLine = GetCurrentLine();
// 		int originalCaretColumn = GetColumn( GetCurrentPos() );
// 
// 		// modified selection range that will actually be cut (we want to move the whole line, not just what's selected)
// 		int startLine = LineFromPosition( originalSelectionPositionStart );
// 		int modifiedSelectionPositionStart = PositionFromLine( startLine );
// 
// 		int endLine = LineFromPosition( originalSelectionPositionEnd );
// 		int modifiedSelectionPositionEnd = PositionFromLine( endLine ) + LineLength( endLine );
// 
// 		// Extract the data to move
// 		wxString textToMove = GetTextRange( modifiedSelectionPositionStart, modifiedSelectionPositionEnd );
// 
// 		// Delete the lines in the old position
// 		SetCurrentPos( modifiedSelectionPositionStart );
// 
// 		for( int i = 0; i <= ( endLine - startLine ); ++i )
// 		{
// 			LineDelete();
// 		}
// 
// 		LineUp();
// 		LineUp();
// 		LineEnd();
// 		NewLine();
// 		LineDelete();
// 
// 		int newPosition = GetCurrentPos();
// 		InsertText( newPosition, textToMove );
// 
// 		int newStartLine = GetCurrentLine();
// 		int newEndLine = newStartLine + ( endLine - startLine );
// 
// 		int newSelectionPositionStart = FindColumn( newStartLine, originalSelectionPositionStartColumn );
// 		int newSelectionPositionEnd = FindColumn( newEndLine, originalSelectionPositionEndColumn );
// 
// 		if( swapped )
// 		{
// 			wxSwap( newSelectionPositionStart, newSelectionPositionEnd );
// 		}
// 
// 		SetAnchor( newSelectionPositionStart );
// 		SetCurrentPos( newSelectionPositionEnd );
// 
// 		EndUndoAction();
// 
// 		return;
// 		

		MoveSelectedLines( true );
		return;
	}
	else if( event.AltDown() && event.GetKeyCode() == WXK_DOWN )
	{
		MoveSelectedLines( false );

		return;
	}
	else
	{
		m_file->m_documentEx->UpdateContext();
	}

	// Do not eat
	event.Skip();
}


int CSSDocument::FindBracket( int startingPos, const wxString& brackets, bool backwards, int limit )
{
	int position = startingPos;
	int direction = ( backwards )? -1 : 1;

	for( int i = 0; i < limit; ++i )
	{
		wxChar charAtPos = GetCharAt( position );

		if( brackets.Find( charAtPos ) != wxNOT_FOUND )
		{
			return position;
		}

		position += direction;
	}

	return wxNOT_FOUND;
}

void CSSDocument::OnUpdateUI( wxStyledTextEvent& )
{
	wxTheFrame->SetStatusBarLine( GetCurrentLine() + 1 );
	wxTheFrame->SetStatusBarColumn( GetColumn( GetCurrentPos() ) + 1 );
	wxTheFrame->SetStatusBarCharacterPosition( GetCurrentPos() );

	// Bracket Highlighting
	if( wxTheFrame->ReadOption( CONFIG_OPTIONS_BRACKETHIGHLIGHTING, true ) )
	{
		const SolutionFilePtr& file = GetFile();

		if( file )
		{
			if( m_previousMatchingBracePairIndex != -1 )
			{
	// 			const SSBracketPair& bracketPair = file->m_lexData->m_brackets[ m_previousMatchingBracePairIndex ];

				BraceHighlight( INVALID_POSITION , INVALID_POSITION );

				// Manual styling
	// 			StartStyling( bracketPair.m_openingPosition, 0xffffffff );
	// 			SetStyling( 1, STYLE_DEFAULT );
	// 
	// 			StartStyling( bracketPair.m_closingPosition, 0xffffffff );
	// 			SetStyling( 1, STYLE_DEFAULT );
	// 			

				m_previousMatchingBracePairIndex = 0;
			}

			int caretPos = GetCurrentPos();

			wxString brackets = wxT( "{[(}])" );

			// are we on a bracket?
			int bracketPos = FindBracket( caretPos, brackets, true, 2 );

			//RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), TXT( "Search for closing bracket at %i (Result: %i)" ), caretPos, bracketPos );

			if( bracketPos == wxNOT_FOUND )
			{
				bracketPos = FindBracket( caretPos - 2, wxT( "{" ), true, 100 );

				//RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), TXT( "Search for opening bracket at %i (Result: %i)" ), caretPos - 2, bracketPos );
			}

			if( bracketPos != wxNOT_FOUND )
			{
				SSStubSystemReadLock lock;

				SSLexicalData* lexData = file->m_stubs->m_lexData;
				if( lexData && lexData->m_bracketPositionToIndex.find( bracketPos ) != lexData->m_bracketPositionToIndex.end() )
				{
					unsigned int index = lexData->m_bracketPositionToIndex[ bracketPos ];

					RED_ASSERT( index < lexData->m_brackets.size(), TXT( "Invalid Bracket Index: %u/%u" ), index, lexData->m_brackets.size() );

					const SSBracketPair& bracketPair = lexData->m_brackets[ index ];

					// stop small pairs of brackets being highlighted if the caret isn't actually inside them
					if( bracketPair.m_close.m_position >= caretPos )
					{
						// Manual styling
						// 				StartStyling( bracketPair.m_openingPosition, 0xffffffff );
						// 				SetStyling( 1, STYLE_BRACELIGHT );
						// 
						// 				StartStyling( bracketPair.m_closingPosition, 0xffffffff );
						// 				SetStyling( 1, STYLE_BRACELIGHT );
						BraceHighlight( bracketPair.m_open.m_position, bracketPair.m_close.m_position );

						m_previousMatchingBracePairIndex = index;

						//RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), TXT( "Found matching braces '%c' at %i, %i" ), bracketPair.m_open.m_type, bracketPair.m_open.m_position, bracketPair.m_close.m_position );
					}
				}
			}
			else
			{
				//RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), TXT( "No Brackets found" ) );
			}
		}
	}
	else
	{
		if( m_previousMatchingBracePairIndex != -1 )
		{
			BraceHighlight( INVALID_POSITION , INVALID_POSITION );
			m_previousMatchingBracePairIndex = -1;
		}
	}
}

SSBasicStub* CSSDocument::FindStub( int hoverLine, const wxString& hoverWord, int hoverWordEnd )
{
	const wchar_t* hoverStr = hoverWord.wc_str();

	// Search global classes
	SSBasicStub* stub = GStubSystem.FindClass( hoverStr );

	// Search global enums
	if( !stub )
	{
		stub = GStubSystem.FindEnum( hoverStr );

		if( !stub )
		{
			stub = GStubSystem.FindEnumByMember( hoverStr );
		}
	}

	if( !stub )
	{
		int caretPosition = hoverWordEnd + 1;

		// Get context 
		SSClassStub* classContext = NULL;
		SSFunctionStub* functionContext = NULL;
		if ( GStubSystem.FindFunctionAtLine( m_file->m_solutionPath, hoverLine + 1, classContext, functionContext ) )
		{
			wxString lineText = GetLineText( hoverLine ).Trim();

			if( lineText[ lineText.Length() - 1 ] == ';' )
			{
				lineText.RemoveLast();
			}

			// Get tokens
			wxString partialSymbol;
			wxArrayString tokens;
			if ( StripSymbolPath( tokens, partialSymbol, lineText, caretPosition ) )
			{
				// Get the tokens 
				vector< wstring > symbolPath;
				for ( size_t i=0; i<tokens.size(); i++ )
				{
					symbolPath.push_back( tokens[i].wc_str() );
				}

				// Resolve function
				stub = GStubSystem.ResolveFunctionCall( classContext, functionContext, symbolPath );

				if( !stub )
				{
					if( functionContext )
					{
						if( partialSymbol != wxEmptyString )
						{
							stub = functionContext->FindField( partialSymbol.wc_str(), true );
						}
						else if( tokens.size() > 0 )
						{
							stub = functionContext->FindField( tokens.Last().wc_str(), true );
						}

						if( !stub && functionContext->m_name == hoverStr )
						{
							stub = functionContext;
						}
					}
				}

				if( !stub )
				{
					if( classContext )
					{
						if( partialSymbol != wxEmptyString )
						{
							stub = classContext->FindField( partialSymbol.wc_str(), true );
						}
						else if( tokens.size() > 0 )
						{
							stub = classContext->FindField( tokens.Last().wc_str(), true );
						}
					}
				}
			}
		}
	}

	return stub;
}

void CSSDocument::OnMouseDwellStart( wxStyledTextEvent& event )
{
	EHoverInformationType hoverInfoSetting = static_cast< EHoverInformationType >( wxTheFrame->ReadOption( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Tooltips ) ) );

	// Hover Information
	if( hoverInfoSetting != HoverInfoType_Off && m_mouseOverWindow )
	{
		int start = WordStartPosition( event.GetPosition(), true );
		int end = WordEndPosition( event.GetPosition(), true );

		wxString hoverWord = GetTextRange( start, end );

		int hoverLine = LineFromPosition( start );

		if( hoverWord.length() > 0 )
		{
			int stubLine = -1;
			wxString stubText;

			SSStubSystemReadLockNonBlocking lock;

			if( !lock.TryLock() )
			{
				return;
			}

			int positionAtStartOfLine = PositionFromLine( hoverLine );
			SSBasicStub* stub = TryFunctionCall( m_file, hoverLine, end + 1 - positionAtStartOfLine, GetLine( hoverLine ) );

			if( !stub )
			{
				stub = FindStub( hoverLine, hoverWord, end - positionAtStartOfLine );
			}

			if( stub )
			{
				SolutionFilePtr stubFile = m_solution->FindFile( stub->m_context.m_file );
				SSComment* comment = NULL;
				wxString commentText;

				if( stubFile )
				{

					// "-1" because we start counting lines from 1, but scintilla counts from 0
					stubLine = stub->m_context.m_line - 1;

					if( stubFile->m_document )
					{
						stubText = stubFile->m_document->GetLineText( stubLine );
					}
					else
					{
						 stubText = stubFile->GetText();

						// Is there a better way to remove X lines from a block of text?
						for( int i = 0; i < stubLine; ++i )
						{
							stubText = stubText.AfterFirst( L'\n' );
						}
						stubText = stubText.BeforeFirst( L'\n' );
					}

					SSLexicalData* lexData = stubFile->m_stubs->m_lexData;

					// Grab comment
					if( lexData->m_commentStartLineToIndex.find( stub->m_context.m_line ) != stubFile->m_stubs->m_lexData->m_commentStartLineToIndex.end() )
					{
						int commentLine = lexData->m_commentStartLineToIndex[ stub->m_context.m_line ];
						comment = &lexData->m_comments[ commentLine ];
					}
					else if( lexData->m_commentSucceedingCodeLinetoIndex.find( stub->m_context.m_line ) != lexData->m_commentSucceedingCodeLinetoIndex.end() )
					{
						int commentLine = lexData->m_commentSucceedingCodeLinetoIndex[ stub->m_context.m_line ];
						comment = &lexData->m_comments[ commentLine ];
					}

					// Grab comment text and remove comment text from stubtext (if applicable)
					if( comment )
					{
						commentText = comment->m_text.c_str();

						if( comment->m_startLine == stub->m_context.m_line )
						{
							stubText.Replace( commentText, wxEmptyString, false );
						}
					}
				}
				else
				{
					return;
				}

				stubText.Trim( false );
				stubText.Trim();

				// Join the strings together
				wxString annotationText = stubText + wxT( "\xD" ) + commentText;
				annotationText.Replace( wxT( "\n" ), wxEmptyString );
				annotationText.Replace( wxT( "\xD" ), wxT( "\n" ) );

				// Style time
				wxString styleBytes;

				if( stubFile->m_document )
				{
					int stubStartColumn = stubFile->m_document->GetLineIndentation( stubLine );
					int stubStartPosition = stubFile->m_document->FindColumn( stubLine, stubStartColumn );

#define DEBUG_HOVERINFO
#ifdef DEBUG_HOVERINFO
					wxString debuggy = wxT( "MMM PIE ->" );
					wxString debuggy2 = wxT( "MMM CAEK->" );
#endif // DEBUG_HOVERINFO
					for( unsigned int i = 0; i < stubText.size(); ++i )
					{
						wxChar style = stubFile->m_document->GetStyleAt( stubStartPosition + i );

						if( style == 0 )
						{
							style = STYLE_DEFAULT;
						}

						styleBytes += style;

#ifdef DEBUG_HOVERINFO
						wxChar dc = stubFile->m_document->GetCharAt( stubStartPosition + i );

						debuggy += dc;
						debuggy2 += (wxChar)( style + L'A' );
#endif // DEBUG_HOVERINFO
					}

#ifdef DEBUG_HOVERINFO
					RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), debuggy.wc_str() );
					RED_LOG_SPAM( RED_LOG_CHANNEL( Document ), debuggy2.wc_str() );
#endif // DEBUG_HOVERINFO
				}
				else
				{
					styleBytes = wxString( static_cast< wxChar >( STYLE_DEFAULT ), stubText.Length() );
				}

				for( unsigned int i = styleBytes.Length(); i <= annotationText.length(); ++i )
				{
					styleBytes += SCE_C_COMMENT;
				}

				//Additional text (not extracted from scintilla)
				wxString additionalText;

				if( stub->m_type == STUB_Enum )
				{
					SSEnumStub* enumStub = static_cast< SSEnumStub* >( stub );

					SSEnumOptionStub* optionStub = enumStub->FindOption( hoverWord.wc_str() );

					if( optionStub )
					{
						additionalText.Printf( wxT( "%s = %i" ), optionStub->m_name.c_str(), optionStub->m_value );
					}

					if( !additionalText.IsEmpty() )
					{
						wxString additionalTextStyles;

						for( unsigned int i = 0; i <= additionalText.length(); ++i )
						{
							additionalTextStyles += STYLE_DEFAULT;
						}

						annotationText.Printf( wxT( "%s\n%s" ), additionalText, annotationText );
						styleBytes.Printf( wxT( "%s%s" ), additionalTextStyles, styleBytes );
					}
				}

				if( hoverInfoSetting == HoverInfoType_Annotations )
				{
					// Display
					AnnotationSetText( hoverLine, annotationText );
					AnnotationSetStyles( hoverLine, styleBytes );
					AnnotationSetVisible( ANNOTATION_BOXED );

					// HACK: Force redraw by calling colourise (fixed in later version of scintilla)
					int redrawFromPosition = PositionFromLine( stubLine - 2 );
					int redrawToPosition = PositionFromLine( stubLine + 3 );
					HACKForceRedraw( redrawFromPosition, redrawToPosition );

					m_scintillaForceRedrawAnnotationHack = true;
				}
				else
				{
					wxPoint position = wxGetMousePosition();
					position.y += TextHeight( stubLine );

					new CSSCodeToolTip( this, annotationText, position );
				}
			}
		}
	}
}

void CSSDocument::OnMouseDwellEnd( wxStyledTextEvent& event )
{
	if( wxTheFrame )
	{
		EHoverInformationType hoverInfoSetting = static_cast< EHoverInformationType >( wxTheFrame->ReadOption( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Tooltips ) ) );

		// Hover Information
		if( hoverInfoSetting == HoverInfoType_Annotations )
		{
			AnnotationClearAll();

			if( m_scintillaForceRedrawAnnotationHack )
			{
				// HACK: Force redraw by calling colourise (fixed in later version of scintilla)
				int redrawFromPosition = PositionFromLine( LineFromPosition( event.GetPosition() ) - 2 );
				int redrawToPosition = PositionFromLine( LineFromPosition( redrawFromPosition ) + 3 );
				HACKForceRedraw( redrawFromPosition, redrawToPosition );

				m_scintillaForceRedrawAnnotationHack = false;
			}
		}
	}
}

void CSSDocument::OnMouseEnter( wxMouseEvent& )
{
	m_mouseOverWindow = true;
}

void CSSDocument::OnMouseExit( wxMouseEvent& )
{
	m_mouseOverWindow = false;
}

void CSSDocument::GenerateNavigationHistoryEvent()
{
	int position = GetCurrentPos();
	GenerateNavigationHistoryEvent( position );
}

void CSSDocument::GenerateNavigationHistoryEvent( int position )
{
	if( position != -1 )
	{
		int line = LineFromPosition( position );
		wxString lineText = GetLineText( line );

		CNavigationHistoryEvent* newEvent = new CNavigationHistoryEvent( m_file->m_solutionPath.c_str(), lineText, line + 1 );
		QueueEvent( newEvent );
	}
}

void CSSDocument::ShowOpcodes( const SOpcodeFunction& func )
{
	using Red::System::Uint32;
	const Uint32 numLines = func.m_lines.size();

	for( Uint32 i = 0; i < numLines; ++i )
	{
		AnnotationSetText( func.m_lines[ i ].m_line - 1, func.m_lines[ i ].m_details );
		AnnotationSetVisible( ANNOTATION_BOXED );
		AnnotationSetStyle( func.m_lines[ i ].m_line - 1, SCINTILLA_OPCODE_STYLE_INDEX );

		int redrawFromPosition = PositionFromLine( func.m_lines[ i ].m_line - 2 );
		int redrawToPosition = PositionFromLine( func.m_lines[ i ].m_line + 2 );
		HACKForceRedraw( redrawFromPosition, redrawToPosition );
	}
}

void CSSDocument::OnClick( wxMouseEvent& event )
{
	ClearKeywordsHighlight();

	int position = PositionFromPoint( wxPoint( event.GetX(), event.GetY() ) );
	GenerateNavigationHistoryEvent( position );

	event.Skip();
}

void CSSDocument::OnDoubleClick( wxStyledTextEvent& event )
{
	if ( wxTheFrame->ReadOption( CONFIG_OPTIONS_PATH, CONFIG_OPTIONS_WORDHIGHLIGHTING, true ) )
	{
		int start = WordStartPosition( event.GetPosition(), true );
		int end = WordEndPosition( event.GetPosition(), true );
		int textLen = end - start;

		//don't process anything if nothing has been selected
		if ( textLen > 0 )
		{
			ClearKeywordsHighlight();

			m_highlightedWord = GetTextRange( start, end );

			HighlightKeywords( textLen );
		}
	}

	event.Skip();
}

void CSSDocument::HighlightKeywords( int wordLength )
{
	int curPos = 0;

	while ( true )
	{
		curPos = FindText( curPos, GetLength(), m_highlightedWord, 6 ); //flag == 6 means WHOLE_WORD + CASE_SENSITIVE + CURRENT DOCUMENT
		
		//keep track of the current position so we can clear the indicator later on
		m_wordPositions.push_back( curPos );

		if ( curPos < 0 )
			break;

		MarkerAdd( LineFromPosition( curPos ), MARKER_WordHighlight );


		IndicatorSetStyle( 0, wxSTC_INDIC_ROUNDBOX );
		IndicatorSetForeground( 0, wxTheFrame->ReadOption( CONFIG_CARET_PATH, CONFIG_CARET_WORDHIGHLIGHTINGCOLOUR, *wxLIGHT_GREY ) );
		IndicatorSetAlpha( 0, 75 );
		IndicatorFillRange( curPos, wordLength );

		curPos += wordLength;
	}
}

void CSSDocument::ClearKeywordsHighlight()
{
	//we haven't highlighted anything
	if ( m_wordPositions.size() == 0 )
		return;

	//clear all indicators then clear out all margin markers
	for ( int i = 0; i < m_wordPositions.size(); i++ )
	{
		IndicatorClearRange( m_wordPositions[i], m_highlightedWord.length() );
	}

	MarkerDeleteAll( MARKER_WordHighlight );
}


void CSSDocument::OnReparsed( wxCommandEvent& )
{
	PopulateOpcodeMarkers();
}

void CSSDocument::PopulateOpcodeMarkers()
{
	if( IsMarginShown( MARGIN_Opcodes ) )
	{
		using Red::System::Uint32;

		MarkerDeleteAll( MARKER_OpcodesShown );
		MarkerDeleteAll( MARKER_OpcodesHidden );

		AnnotationClearAll();

		SSFileStub* file = m_file->m_stubs;

		PopulateOpcodeMarkers( file->m_functions );

		for( Uint32 i = 0; i < file->m_classes.size(); ++i )
		{
			PopulateOpcodeMarkers( file->m_classes[ i ]->m_functions );
		}
	}
}

void CSSDocument::PopulateOpcodeMarkers( const vector< SSFunctionStub* >& functionStubs )
{
	using Red::System::Uint32;

	for( Uint32 i = 0; i < functionStubs.size(); ++i )
	{
		SSFunctionStub* stub = functionStubs[ i ];

		if( !stub->m_flags.Has( L"import" ) )
		{
			MarkerAdd( stub->m_context.m_line - 1, MARKER_OpcodesHidden );
		}
	}
}

void CSSDocument::MoveSelectedLines( bool up )
{
	BeginUndoAction();

	// Selection position to be preserved
	int originalSelectionPositionStart = GetAnchor();
	int originalSelectionPositionEnd = GetCurrentPos();

	// 
	bool swapped = false;
	if( originalSelectionPositionStart > originalSelectionPositionEnd )
	{
		wxSwap( originalSelectionPositionStart, originalSelectionPositionEnd );
		swapped = true;
	}

	int originalSelectionPositionStartColumn = GetColumn( originalSelectionPositionStart );
	int originalSelectionPositionEndColumn = GetColumn( originalSelectionPositionEnd );

	int originalCaretLine = GetCurrentLine();
	int originalCaretColumn = GetColumn( GetCurrentPos() );

	// modified selection range that will actually be cut (we want to move the whole line, not just what's selected)
	int startLine = LineFromPosition( originalSelectionPositionStart );
	int modifiedSelectionPositionStart = PositionFromLine( startLine );

	int endLine = LineFromPosition( originalSelectionPositionEnd );
	int modifiedSelectionPositionEnd = PositionFromLine( endLine ) + LineLength( endLine );

	// Extract the data to move
	wxString textToMove = GetTextRange( modifiedSelectionPositionStart, modifiedSelectionPositionEnd );

	// Delete the lines in the old position
	SetCurrentPos( modifiedSelectionPositionStart );

	for( int i = 0; i <= ( endLine - startLine ); ++i )
	{
		LineDelete();
	}

	if( up )
	{
		LineUp();
		LineUp();
	}

	LineEnd();
	NewLine();
	LineDelete();

	int newPosition = GetCurrentPos();
	InsertText( newPosition, textToMove );

	int newStartLine = GetCurrentLine();
	int newEndLine = newStartLine + ( endLine - startLine );

	int newSelectionPositionStart = FindColumn( newStartLine, originalSelectionPositionStartColumn );
	int newSelectionPositionEnd = FindColumn( newEndLine, originalSelectionPositionEndColumn );

	if( swapped )
	{
		wxSwap( newSelectionPositionStart, newSelectionPositionEnd );
	}

	SetAnchor( newSelectionPositionStart );
	SetCurrentPos( newSelectionPositionEnd );

	EndUndoAction();
}

//////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_CLASS( CSSDocumentEx, wxWindow );

// Event table
BEGIN_EVENT_TABLE( CSSDocumentEx, wxWindow )
END_EVENT_TABLE();

wxDEFINE_EVENT( ssEVT_DOCUMENT_REPARSED_EVENT, wxCommandEvent );

CSSDocumentEx::CSSDocumentEx( wxWindow* parent, const SolutionFilePtr& file, Solution* solution )
	: wxWindow( parent, wxID_ANY )
	, m_updateLastLine( -1 )
	, m_updateLastFunctionContext( NULL )
	, m_updateCurrentSelIdx( 0 )
{
	m_codeContextCombo = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_DROPDOWN | wxCB_READONLY | wxTE_PROCESS_ENTER );
	m_ssDocument = new CSSDocument( this, file, solution );
	
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_codeContextCombo, 0, wxEXPAND, 0 );
	sizer->Add( m_ssDocument, 1, wxEXPAND, 0 );
	SetSizer( sizer );
	Layout();

	file->m_document = m_ssDocument;
	file->m_documentEx = this;

	Bind( ssEVT_DOCUMENT_REPARSED_EVENT, &CSSDocument::OnReparsed, m_ssDocument );

	m_codeContextCombo->Bind( wxEVT_SET_FOCUS, &CSSDocumentEx::OnComboSetFocus, this );
	m_codeContextCombo->Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CSSDocumentEx::OnComboBoxEnter, this );

	Reparse();
}

CSSDocumentEx::~CSSDocumentEx()
{

}

wxString CSSDocumentEx::GetFunctionSignature( const SSFunctionStub* functionStub )
{
	wxString symbolSignature = ( functionStub->m_name + wxT(" ( ") ).c_str();
	const vector< SSPropertyStub* >& params = functionStub->m_params;
	for ( unsigned int i = 0; i < params.size(); )
	{
		symbolSignature += ( params[ i ]->m_name + wxT(" : ") + params[i]->m_typeName ).c_str();
		if ( ++i < params.size() )
		{
			symbolSignature += wxT(", ");
		}
	}
	symbolSignature += wxT(" )");

	return symbolSignature;
}

wxString CSSDocumentEx::GetEnumSignature( const SSEnumStub* enumStub )
{
	wxString symbolSignature = ( wxT( "enum " ) + enumStub->m_name + wxT( " [ " ) ).c_str();;
	
	for ( unsigned int i = 0; i < enumStub->m_options.size(); )
	{
		SSEnumOptionStub* enumOptionStub = enumStub->m_options[ i ];

		symbolSignature += enumOptionStub->m_name.c_str();
		if ( ++i < enumStub->m_options.size() )
		{
			symbolSignature += wxT( ", " );
		}
	}
	symbolSignature += wxT( " ]" );

	return symbolSignature;
}

void CSSDocumentEx::Reparse()
{
	const SSFileStub* stubs = GetFile()->m_stubs;

	if ( stubs && m_codeContextCombo )
	{
		m_stubs.clear();
		m_comboItems.clear();

		// Collect all enums
		for ( size_t i = 0; i < stubs->m_enums.size(); ++i )
		{
			SSEnumStub* enumStub = stubs->m_enums[ i ];

			m_comboItems.Add( GetEnumSignature( enumStub ) );
			m_stubs.push_back( enumStub );
		}

		// Collect all global functions
		for ( size_t i = 0; i < stubs->m_functions.size(); ++i )
		{
			SSFunctionStub* functionStub = stubs->m_functions[ i ];

			m_comboItems.Add( GetFunctionSignature( functionStub ) );
			m_stubs.push_back( functionStub );
		}

		wxArrayString tempComboItems;
		std::vector< SSFunctionStub* > tempStubs;
		
		// Collect class methods
		for ( size_t i = 0; i < stubs->m_classes.size(); ++i )
		{
			SSClassStub* classStub = stubs->m_classes[ i ];

			for ( size_t k = 0; k < classStub->m_functions.size(); ++k )
			{
				tempComboItems.Add( wxString( classStub->m_name.c_str() ) + wxT( "::" ) + GetFunctionSignature( classStub->m_functions[ k ] ) );
				tempStubs.push_back( classStub->m_functions[ k ] );
			}
		}

		if( tempStubs.size() > 0 )
		{
			// Sort them
			tempComboItems.Sort( WxFunctionNameCompare );
			qsort( &tempStubs[ 0 ], tempStubs.size(), sizeof( SSFunctionStub* ), FunctionNameCompare );

			// Add to final list
			for ( size_t i = 0; i < tempComboItems.size(); ++i )
			{
				m_comboItems.push_back( tempComboItems[ i ] );
			}
			for ( size_t i = 0; i < tempStubs.size(); ++i )
			{
				m_stubs.push_back( tempStubs[ i ] );
			}
		}

		wxCommandEvent* event = new wxCommandEvent( ssEVT_DOCUMENT_REPARSED_EVENT, wxID_ANY );
		QueueEvent( event );
	}
}

#ifdef RED_ASSERTS_ENABLED
void CSSDocumentEx::CheckBracket( const SSBracket& bracket )
{
	int bracketChar = m_ssDocument->GetCharAt( bracket.m_position );
	wxChar bracketCharWx = static_cast< wxChar >( bracketChar );

	wxChar bracketCharType = bracket.m_type;

	wxString message;
	message.Printf( wxT( "Reparse file %s: Recorded bracket '%c' at position %i does not match character '%c' (%i)" ), GetFile()->m_name.c_str(), bracketCharType, bracket.m_position, bracketCharWx, bracketChar );

	RED_ASSERT( bracketCharWx == bracketCharType, message.wx_str() );
}
#endif

void CSSDocumentEx::SetComboFocus() const
{
	m_codeContextCombo->SetFocus();
}

void CSSDocumentEx::OnComboSetFocus( wxFocusEvent& )
{
	UpdateGuiCombo();
	m_codeContextCombo->Popup();
	m_codeContextCombo->Select( m_updateCurrentSelIdx );
}

void CSSDocumentEx::OnComboBoxEnter( wxCommandEvent& )
{
	int selIdx = m_codeContextCombo->GetSelection();
	if ( selIdx == wxNOT_FOUND ) return;

	if ( selIdx >= 0 && selIdx < static_cast< int >( m_stubs.size() ) )
	{
		wxTheFrame->GetCurrentDocument()->GotoLine( m_stubs[ selIdx ]->m_context.m_line - 1 );
		wxTheFrame->GetCurrentDocument()->SetFocus();
	}
}

void CSSDocumentEx::UpdateContext()
{
	int currentLine = m_ssDocument->GetCurrentLine();

	// Optimization
	if ( currentLine == m_updateLastLine )
	{
		return;
	}
	m_updateLastLine = currentLine;

	SSStubSystemReadLock lock;

	// Get context 
	SSClassStub* classContext = NULL;
	SSFunctionStub* functionContext = NULL;
	if ( GStubSystem.FindFunctionAtLine( GetFile()->m_solutionPath, currentLine+1, classContext, functionContext ) )
	{
		// Optimization
		if ( functionContext == m_updateLastFunctionContext )
		{
			return;
		}
		m_updateLastFunctionContext = functionContext;

		if ( functionContext )
		{
			for ( unsigned int i = 0; i < m_stubs.size(); ++i )
			{
				if ( m_stubs[i] == functionContext )
				{
					m_codeContextCombo->SetSelection( i );
					m_updateCurrentSelIdx = i;
					return;
				}
			}
		}
	}

	// Reset selection
	m_codeContextCombo->SetSelection( -1 );
}

void CSSDocumentEx::UpdateGuiCombo()
{
	m_codeContextCombo->Set( m_comboItems );
}
