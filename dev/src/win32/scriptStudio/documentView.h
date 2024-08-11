/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "styledDocument.h"
#include "solution/slnDeclarations.h"

struct SSFunctionStub;
struct SSBasicStub;
struct SSEnumStub;

enum EMarkerState;
struct SSBracket;
struct SOpcodeFunction;

class CSSDocument : public CSSStyledDocument
{
	wxDECLARE_CLASS( CSSDocument );
	DECLARE_EVENT_TABLE();

protected:
	SolutionFilePtr		m_file;						//!< Associated file
	bool				m_doNotMonitorChanges;		//!< Do not monitor content changes	
	bool				m_ignoreClickDueToActivation;
	wxTimer				m_modifiedTimer;			//!< Timer used to signal that content was modified

	int					m_previousMatchingBracePairIndex;

	bool				m_scintillaForceRedrawAnnotationHack;
	bool				m_mouseOverWindow;

	wxString			m_highlightedWord;
	vector				< int > m_wordPositions;

	Solution*			m_solution;

public:
	//! Get file edited by this document tab
	inline SolutionFilePtr GetFile() { return m_file; }

	void ValidateFile();

public:
	CSSDocument( wxWindow* parent, const SolutionFilePtr& file, Solution* solution );
	~CSSDocument();

	bool Save();
	void Undo();
	void Redo();

	// Breakpoints
	void SetBreakpoint( int line, EMarkerState state );
	void ToggleBreakpoint();
	void ToggleBreakpoint( int line );
	void ConfirmBreakpointToggle( int line, bool success );

	//
	void SetCallstackMarker( int type, int line );

	void LoadFile();

	// Bookmarks
	void SetBookmark( int line, EMarkerState state );
	void ToggleBookmark();
	void ToggleBookmark( int line );

	void GenerateNavigationHistoryEvent();
	void GenerateNavigationHistoryEvent( int position );

	void ShowOpcodes( const SOpcodeFunction& function );
	void OnReparsed( wxCommandEvent& event );

	void PopulateOpcodeMarkers();

private:
	bool StripSymbolPath( wxArrayString& tokens, wxString& partialSymbol );
	bool StripSymbolPath( wxArrayString& tokens, wxString& partialSymbol, wxString line, int caretLinePosition );
	void TryAutoComplete( bool force );
	SSFunctionStub* TryFunctionCall( const SolutionFilePtr& file, int line, int caretLinePosition = -1, const wxString& lineText = wxEmptyString );
	void TryCallTip();
	void TryGotoImplementation();
	void TryGotoImplementation( int position );
	int FindBracket( int startingPos, const wxString& brackets, bool backwards, int limit );
	void ClearKeywordsHighlight();
	void HighlightKeywords( int wordLength );
	void PopulateOpcodeMarkers( const vector< SSFunctionStub* >& functionStubs );
	SSBasicStub* FindStub( int hoverLine, const wxString& hoverWord, int hoverWordPosition );
	void MoveSelectedLines( bool up );

protected:
	void OnModifiedTimer( wxTimerEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnMarginClick( wxStyledTextEvent& event );
	void OnContentModified( wxStyledTextEvent& event );
	void OnRequestCheckout( wxStyledTextEvent& event );
	void OnCharAdded( wxStyledTextEvent& event );
	void OnUpdateUI( wxStyledTextEvent& event );
	void OnMouseDwellStart( wxStyledTextEvent& event );
	void OnMouseDwellEnd( wxStyledTextEvent& event );
	void OnMouseEnter( wxMouseEvent& event );
	void OnMouseExit( wxMouseEvent& event );
	void OnModified( wxStyledTextEvent& event );
	void OnClick( wxMouseEvent& event );
	void OnDoubleClick( wxStyledTextEvent& event );
};

//////////////////////////////////////////////////////////////////////////

wxDECLARE_EVENT( ssEVT_DOCUMENT_REPARSED_EVENT, wxCommandEvent );

class CSSDocumentEx : public wxWindow
{
	wxDECLARE_CLASS( CSSDocumentEx );

	DECLARE_EVENT_TABLE();
public:
	CSSDocumentEx( wxWindow *parent, const SolutionFilePtr& file, Solution* solution );
	virtual ~CSSDocumentEx();

	void OnComboSetFocus( wxFocusEvent &event );
	void OnComboBoxEnter( wxCommandEvent &event );

	void Reparse();
	void SetComboFocus() const;
	void UpdateContext();
	void UpdateGuiCombo();

	static wxString GetFunctionSignature( const SSFunctionStub *functionStub );
	static wxString GetEnumSignature( const SSEnumStub *enumStub );

	inline SolutionFilePtr GetFile() { return m_ssDocument->GetFile(); }
	inline CSSDocument* GetDocument() { return m_ssDocument; }

#ifdef RED_ASSERTS_ENABLED
private:
	void CheckBracket( const SSBracket& bracket );
#endif

private:
	CSSDocument*			m_ssDocument;
	wxComboBox*				m_codeContextCombo;
	vector< SSBasicStub* >	m_stubs;
	wxArrayString			m_comboItems;

	// Update
	int             m_updateLastLine;
	SSFunctionStub* m_updateLastFunctionContext;
	unsigned int    m_updateCurrentSelIdx;
};
