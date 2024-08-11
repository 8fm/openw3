#include "build.h"
#include "styledDocument.h"

#include "SciLexer.h"
#include "Scintilla.h"
#include "frame.h"
#include "fileStubs.h"

IMPLEMENT_CLASS( CSSStyledDocument, wxStyledTextCtrl );

#define ANNOTATION_OFFSET STYLE_LASTPREDEFINED
#define MARGIN_OFFSET ( STYLE_LASTPREDEFINED * 2 )
#define CALLTIP_OFFSET ( STYLE_LASTPREDEFINED * 3 )

// Primary keywords
const wxChar scKeywords[] = wxT("class var extends in array return if for state switch case default break continue do while else enum struct super parent virtual_parent new delete function event savepoint");

// Secondary keywords
const wxChar scSecKeywords[] = wxT("local final const editable timer cleanup inlined latent abstract transient out optional import private protected public exec entry storyscene saved quest reward statemachine autobind any single");

// Types
const wxChar scTypes[] = wxT( "byte int float bool name string true false void NULL this" );

CSSStyledDocument::CSSStyledDocument( wxWindow* parent )
:	wxStyledTextCtrl( parent )
,	m_marginsShown( 0 )
{
	ReloadStyling();
}

CSSStyledDocument::~CSSStyledDocument()
{

}

void CSSStyledDocument::ReloadStyling()
{
	// Setup lexer type
	SetLexer( SCLEX_CPP );
	SetStyleBits( 7 );

	wxString globalKeywords;
	const map< wstring, wstring >& globalsMap = GStubSystem.GetGlobalsMap();
	for( map< wstring, wstring >::const_iterator iter = globalsMap.begin(); iter != globalsMap.end(); ++iter )
	{
		globalKeywords += L' ';
		globalKeywords += iter->first.c_str();
	}

	// Setup CPP keywords
	SetKeyWords( 0, scKeywords + globalKeywords );
	SetKeyWords( 1, scSecKeywords );
	//SetKeyWords( 2, globalKeywords );	// Unused by scintilla cpp lexer
	SetKeyWords( 3, scTypes );

	wxColour styleOutliningMargin( 235, 235, 235 );
	wxColour styleOutliningFoldFore = CSSStyleManager::GetInstance().GetForegroundColour( Style_Default );
	wxColour styleOutliningFoldBack = CSSStyleManager::GetInstance().GetBackgroundColour( Style_Default );
	wxColour styleHighlightWord = CSSStyleManager::GetInstance().GetWordHighlightColour();

	// Annotations
	AnnotationSetStyleOffset( ANNOTATION_OFFSET );
	
	MarginSetStyleOffset( MARGIN_OFFSET );

	// Call once and reset to set the standard global default
	SetupStyle( STYLE_DEFAULT, Style_Default );
	StyleClearAll();

	// Call again to force annotation style settings that were reset by StyleClearAll()
	SetupStyle( STYLE_DEFAULT, Style_Default );

	SetupStyle( STYLE_DEFAULT + ANNOTATION_OFFSET, Style_HoverInfoDefault );

	// Comments
	SetupStyle( SCE_C_COMMENT, Style_Comment );
	SetupStyle( SCE_C_COMMENTLINE, Style_Comment );
	SetupStyle( SCE_C_COMMENTDOC, Style_Comment );

	SetupStyle( SCE_C_COMMENT + ANNOTATION_OFFSET, Style_HoverInfoComment );
	SetupStyle( SCE_C_COMMENTLINE + ANNOTATION_OFFSET, Style_HoverInfoComment );
	SetupStyle( SCE_C_COMMENTDOC + ANNOTATION_OFFSET, Style_HoverInfoComment );

	// Constants
	SetupStyle( SCE_C_NUMBER, Style_Number );
	SetupStyle( SCE_C_STRING, Style_String );
	SetupStyle( SCE_C_CHARACTER, Style_Character );

	SetupStyle( SCE_C_NUMBER + ANNOTATION_OFFSET, Style_HoverInfoNumber );
	SetupStyle( SCE_C_STRING + ANNOTATION_OFFSET, Style_HoverInfoString );
	SetupStyle( SCE_C_CHARACTER + ANNOTATION_OFFSET, Style_HoverInfoCharacter );

	// Keywords
	SetupStyle( SCE_C_WORD, Style_Word );
	SetupStyle( SCE_C_WORD2, Style_Word2 );

	SetupStyle( SCE_C_WORD + ANNOTATION_OFFSET, Style_HoverInfoWord );
	SetupStyle( SCE_C_WORD2 + ANNOTATION_OFFSET, Style_HoverInfoWord2 );

	SetupStyle( SCE_C_GLOBALCLASS, Style_GlobalClass );
	SetupStyle( SCE_C_IDENTIFIER, Style_Identifier );
	SetupStyle( SCE_C_OPERATOR, Style_Operator );

	SetupStyle( SCE_C_GLOBALCLASS + ANNOTATION_OFFSET, Style_HoverInfoGlobalClass );
	SetupStyle( SCE_C_IDENTIFIER + ANNOTATION_OFFSET, Style_HoverInfoIdentifier );
	SetupStyle( SCE_C_OPERATOR + ANNOTATION_OFFSET, Style_HoverInfoOperator );
	
	SetupStyle( SCINTILLA_OPCODE_STYLE_INDEX + ANNOTATION_OFFSET, Style_Opcodes );

	//Brace Highlighting
	SetupStyle( STYLE_BRACELIGHT, Style_BracketHighlight );
	StyleSetBold( STYLE_BRACEBAD, TRUE );

	// Margins
	SetMargins( 0, 0 );

	SetMarginWidth( MARGIN_LineNumbers, 50 );
	SetMarginType( MARGIN_LineNumbers, SC_MARGIN_NUMBER );

	SetMarginWidth( MARGIN_Breakpoints, 16 );
	SetMarginType( MARGIN_Breakpoints, SC_MARGIN_SYMBOL );
	SetMarginSensitive( MARGIN_Breakpoints, true );
	m_marginsShown |= FLAG( MARGIN_Breakpoints );

	const int mainMarginMask = MAIN_MARGIN_MASK;
	SetMarginMask( MARGIN_Breakpoints, mainMarginMask );

	SetFoldMarginColour( true, styleOutliningMargin );
	SetFoldMarginHiColour( true, styleOutliningMargin );

	wxString maxLineNoText;
	maxLineNoText << GetLineCount();
	int width = TextWidth( STYLE_LINENUMBER, maxLineNoText + wxT( ' ' ) );
	SetMarginWidth( MARGIN_LineNumbers, width );
	m_marginsShown |= FLAG( MARGIN_LineNumbers );

	// Line numbers margin
	SetupStyle( STYLE_LINENUMBER, Style_LineNumbers );

	// Markers
	MarkerDefine( MARKER_Breakpoint, SC_MARK_CIRCLE );
	MarkerSetBackground( MARKER_Breakpoint, wxColour( 192, 0, 0 ) );

	MarkerDefine( MARKER_UnconfirmedBreakpoint, SC_MARK_CIRCLE );
	MarkerSetForeground( MARKER_UnconfirmedBreakpoint, wxColour( 128, 0, 0 ) );
	MarkerSetBackground( MARKER_UnconfirmedBreakpoint, wxColour( 192, 192, 192 ) );
	
	MarkerDefine( MARKER_DisabledBreakpoint, SC_MARK_CIRCLE );
	MarkerSetForeground( MARKER_DisabledBreakpoint, wxColour( 128, 0, 0 ) );
	MarkerSetBackground( MARKER_DisabledBreakpoint, wxColour( 255, 211, 107 ) );

	MarkerDefine( MARKER_Arrow, SC_MARK_ARROW );
	MarkerSetBackground( MARKER_Arrow, wxColour( 255, 255, 0 ) );

	MarkerDefine( MARKER_WordHighlight, SC_MARK_LEFTRECT );
	MarkerSetBackground( MARKER_WordHighlight, styleHighlightWord );

	MarkerDefine( MARKER_FunctionArrow, SC_MARK_ARROW );
	MarkerSetBackground( MARKER_FunctionArrow, wxColour( 0, 192, 0 ) );

	MarkerDefine( MARKER_Bookmark, SC_MARK_SMALLRECT );
	MarkerDefine( MARKER_DisabledBookmark, SC_MARK_SMALLRECT );
	MarkerSetBackground( MARKER_Bookmark, wxColour( 123, 218, 237 ) );
	MarkerSetBackground( MARKER_DisabledBookmark, wxColour( 255, 211, 107 ) );

	// Icon next to an open fold block
	MarkerDefine( SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS );
	MarkerSetBackground( SC_MARKNUM_FOLDEROPEN, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDEROPEN, styleOutliningFoldBack );

	// Icon next to a collapsed fold block
	MarkerDefine( SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS );
	MarkerSetBackground( SC_MARKNUM_FOLDER, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDER, styleOutliningFoldBack );

	// Icon next to code in a fold block
	MarkerDefine( SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE );
	MarkerSetBackground( SC_MARKNUM_FOLDERSUB, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDERSUB, styleOutliningFoldBack );

	// Icon next to an open sub fold block
	MarkerDefine( SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED );
	MarkerSetBackground( SC_MARKNUM_FOLDEROPENMID, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDEROPENMID, styleOutliningFoldBack );

	// Icon next to the end of an open sub fold block
	MarkerDefine( SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER );
	MarkerSetBackground( SC_MARKNUM_FOLDERMIDTAIL, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDERMIDTAIL, styleOutliningFoldBack );

	// Icon next to a collapsed sub fold block
	MarkerDefine( SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED );
	MarkerSetBackground( SC_MARKNUM_FOLDEREND, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDEREND, styleOutliningFoldBack );

	// Icon at the very end of a fold block
	MarkerDefine( SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER );
	MarkerSetBackground( SC_MARKNUM_FOLDERTAIL, styleOutliningFoldFore );
	MarkerSetForeground( SC_MARKNUM_FOLDERTAIL, styleOutliningFoldBack );

	// Icon to show that opcodes are currently hidden
	MarkerDefine( MARKER_OpcodesHidden, SC_MARK_ARROW );
	MarkerSetBackground( MARKER_OpcodesHidden, styleOutliningFoldFore );
	MarkerSetForeground( MARKER_OpcodesHidden, styleOutliningFoldBack );

	// Icon to show that opcodes are currently visible
	MarkerDefine( MARKER_OpcodesShown, SC_MARK_ARROWDOWN );
	MarkerSetBackground( MARKER_OpcodesShown, styleOutliningFoldFore );
	MarkerSetForeground( MARKER_OpcodesShown, styleOutliningFoldBack );


	// Creates a horizontal line to indicate that code has been hidden at this point
	SetFoldFlags( SC_FOLDFLAG_LINEAFTER_CONTRACTED );

	SetOutlining( wxTheFrame->ReadOption( CONFIG_OPTIONS_OUTLINING, true ) );
	SetIndentationGuides( wxTheFrame->ReadOption( CONFIG_OPTIONS_INDENTATIONGUIDES, true ) );
	SetOpcodeMargin( wxTheFrame->ReadOption( CONFIG_ADVANCED_OPTIONS_PATH, CONFIG_SHOW_OPCODES, false ) );

	// Tabs
	SetTabWidth( 4 );

	// Setup style for calltips
	CallTipUseStyle( CALLTIP_OFFSET );
	SetupStyle( STYLE_CALLTIP, Style_Calltip );

	SetViewWhiteSpace( wxTheFrame->ReadOption( CONFIG_OPTIONS_SHOWWHITESPACE, false ) );
	SetViewEOL( wxTheFrame->ReadOption( CONFIG_OPTIONS_SHOWEOL, false ) );

	// Caret
	SetCaretForeground( CSSStyleManager::GetInstance().GetCaretColour() );
	SetCaretPeriod( CSSStyleManager::GetInstance().GetCaretBlinkRate() );
	SetCaretWidth( CSSStyleManager::GetInstance().GetCaretThickness() );
	SetCaretLineVisible( CSSStyleManager::GetInstance().GetCaretHighlight() );
	SetCaretLineBackground( CSSStyleManager::GetInstance().GetCaretHighlightColour() );
	SetCaretLineBackAlpha( 50 );
}

void CSSStyledDocument::SetupStyle( int stcStyleId, EStyle style )
{
	StyleSetForeground( stcStyleId, CSSStyleManager::GetInstance().GetForegroundColour( style ) );
	StyleSetBackground( stcStyleId, CSSStyleManager::GetInstance().GetBackgroundColour( style ) );
	StyleSetFont( stcStyleId, const_cast< wxFont& >( CSSStyleManager::GetInstance().GetFont( style ) ) );
}

void CSSStyledDocument::SetOutlining( bool enabled )
{
	if( enabled )
	{
		SetProperty( wxT( "fold" ), wxT( "1" ) );

		SetMarginWidth( MARGIN_CodeFolding, 16 );
		SetMarginType( MARGIN_CodeFolding, SC_MARGIN_SYMBOL );
		SetMarginSensitive( MARGIN_CodeFolding, true );
		SetMarginMask( MARGIN_CodeFolding, SC_MASK_FOLDERS );

		m_marginsShown |= FLAG( MARGIN_CodeFolding );
	}
	else
	{
		SetProperty( wxT( "fold" ), wxT( "0" ) );
		SetMarginWidth( MARGIN_CodeFolding, 0 );

		m_marginsShown &= ~FLAG( MARGIN_CodeFolding );
	}
}

void CSSStyledDocument::SetIndentationGuides( bool enabled )
{
	if( enabled )
	{
		wxStyledTextCtrl::SetIndentationGuides( SC_IV_REAL );
	}
	else
	{
		wxStyledTextCtrl::SetIndentationGuides( SC_IV_NONE );
	}
}

void CSSStyledDocument::SetOpcodeMargin( bool enabled )
{
	if( enabled )
	{
		SetMarginWidth( MARGIN_Opcodes, 16 );
		SetMarginType( MARGIN_Opcodes, SC_MARGIN_SYMBOL );
		SetMarginSensitive( MARGIN_Opcodes, true );
		SetMarginMask( MARGIN_Opcodes, OPCODES_MARGIN_MASK );

		m_marginsShown |= FLAG( MARGIN_Opcodes );
	}
	else
	{
		SetMarginWidth( MARGIN_Opcodes, 0 );

		// The scintilla formatting appears to go screwy on lines that have markers when we hide the margin
		MarkerDeleteAll( MARKER_OpcodesHidden );
		MarkerDeleteAll( MARKER_OpcodesShown );
		AnnotationClearAll();

		m_marginsShown &= ~FLAG( MARGIN_Opcodes );
	}

}

void CSSStyledDocument::HACKForceRedraw( int redrawFromPosition, int redrawToPosition )
{
	if( redrawFromPosition == -1 )
	{
		redrawFromPosition = 0;
	}

	if( redrawToPosition == -1 )
	{
		redrawToPosition = GetLastPosition();
	}

	Colourise( redrawFromPosition, redrawToPosition );
}

void CSSStyledDocument::ScrollToLine( int targetLine )
{
	// Adjust for the fact that we count from 1 whereas scintilla counts from 0
	--targetLine;

	int scrollDirection = ( targetLine < GetCurrentLine() )? -1 : 1;

	int linesVisible = LinesOnScreen();
	int scrollToLine = targetLine + ( ( linesVisible / 2 ) * scrollDirection );

	GotoLine( scrollToLine );

	int targetPosition = PositionFromLine( targetLine );

	SetSelection( targetPosition, targetPosition );
}
