#pragma once

#include "styleManager.h"

enum EMarker
{
	MARKER_Bookmark,
	MARKER_DisabledBookmark,
	MARKER_Breakpoint,
	MARKER_UnconfirmedBreakpoint,
	MARKER_DisabledBreakpoint,
	MARKER_Arrow,
	MARKER_FunctionArrow,
	MARKER_WordHighlight,
	MARKER_OpcodesHidden,
	MARKER_OpcodesShown
};

#define OPCODES_MARGIN_MASK ( FLAG( MARKER_OpcodesHidden ) | FLAG( MARKER_OpcodesShown ) )
#define MAIN_MARGIN_MASK ( ~OPCODES_MARGIN_MASK & ~SC_MASK_FOLDERS )

enum EMargin
{
	MARGIN_LineNumbers,
	MARGIN_Breakpoints,
	MARGIN_CodeFolding,
	MARGIN_Opcodes,
};

#define SCINTILLA_OPCODE_STYLE_INDEX 20

class CSSStyledDocument : public wxStyledTextCtrl
{
	wxDECLARE_CLASS( CSSStyledDocument );

public:
	CSSStyledDocument( wxWindow* parent );
	~CSSStyledDocument();

	void ReloadStyling();

	void SetupStyle( int stcStyleId, EStyle style );

	void SetOutlining( bool enabled );
	void SetIndentationGuides( bool enabled );
	void SetOpcodeMargin( bool enabled );

	void ScrollToLine( int lineNumber );

	void HACKForceRedraw( int redrawFromPosition, int redrawToPosition );

	bool IsMarginShown( EMargin margin ) const { return ( m_marginsShown & FLAG( margin ) ) != 0; }

private:
	unsigned int m_marginsShown;
};
