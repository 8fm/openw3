/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "editorConnectionHelper.h"
#include "editorCompilationHelper.h"
#include "debuggerHelper.h"
#include "profilerHelper.h"
#include "pingHelper.h"
#include "solution/slnDeclarations.h"
#include "compileModDialog.h"

class CSSSolutionExplorer;
class CSSDocument;
class CSSDocumentEx;
class CSSErrorList;
class CSSBreakpoints;
class CSSBookmarks;
class CSSSearchResults;
class CSSSearchDialog;
class CSSFindSymbolDialog;
class CSSNavigationHistory;
class wxAuiNotebook;
class CSSColourSelectionDialog;
class CSSDebuggerTabBase;

class CBreakpointToggledEvent;
class CBookmarkToggledEvent;
class CCredentialsChangedEvent;
class COpenFileEvent;
class CGotoEvent;
class CLineMoveEvent;
class CNavigationGotoEvent;
class CSSOpcodeListingEvent;
class CCallstackFrameSelectedEvent;

#define CONFIG_OPTIONS_PATH wxT( "/Options" )
#define CONFIG_OPTIONS_OUTLINING wxT( "codeOutlining" )
#define CONFIG_OPTIONS_BRACKETHIGHLIGHTING wxT( "bracketHighlighting" )
#define CONFIG_OPTIONS_INDENTATIONGUIDES wxT( "indentationGuides" )
#define CONFIG_OPTIONS_HOVERINFORMATION wxT( "hoverInformation" )
#define CONFIG_OPTIONS_SHOWWHITESPACE wxT( "showWhitespace" )
#define CONFIG_OPTIONS_SHOWEOL wxT( "showEOL" )
#define CONFIG_OPTIONS_WORDHIGHLIGHTING wxT( "wordHighlight" )

#define CONFIG_BREAKPOINT_OPTIONS_PATH wxT( "/Options/Breakpoint" )
#define CONFIG_WINDOW_TO_FRONT wxT( "windowToFront" )
#define CONFIG_FLASH_STARTBAR_ICON wxT( "flashStartBar" )
#define CONFIG_AUTO_SAVING wxT( "autoSaving" )

#define CONFIG_DEBUGGER_OPTIONS_PATH wxT( "/Options/Debugger" )
#define CONFIG_SORT_LOCALS wxT( "sortLocals" )
#define CONFIG_SHOW_UNFILTERED wxT( "showUnfiltered" )

#define CONFIG_ADVANCED_OPTIONS_PATH wxT( "/Options/Advanced" )
#define CONFIG_SHOW_OPCODES wxT( "showOpcodes" )
#define CONFIG_DISABLE_PARSER wxT( "disableParser" )

#define MAX_PREVIOUS_OPEN_SOLUTIONS	5
#define CONFIG_PREVIOUS_OPEN_SOLUTIONS wxT( "/solution/previous" )

enum EHoverInformationType
{
	HoverInfoType_Off = 0,
	HoverInfoType_Annotations,
	HoverInfoType_Tooltips,
};

enum EStatusBarField
{
	SBField_UpdateInfo = 0,
	SBField_Line,
	SBField_Column,
	SBField_CharacterIndex,
	SBField_InsertOrOver,

	SBField_Max
};

#define CONFIG_STYLE_PATH wxT( "/Style" )
#define CONFIG_CARET_PATH wxT( "/Caret" )
#define CONFIG_CARET_WORDHIGHLIGHTINGCOLOUR wxT( "wordHighlightingColour" )

/// Main script studio frame
class CSSFrame : public wxFrame
{
	wxDECLARE_CLASS( CSSFrame );
	DECLARE_EVENT_TABLE();

	typedef std::list< wxFrame* > TChildFrameList;
	typedef std::vector< CSSDebuggerTabBase* > TDebuggerPanels;

protected:
	CSSSolutionExplorer*		m_solutionExplorer;
	Solution*					m_solution;
	wxAuiNotebook*				m_fileTabs;
	TChildFrameList				m_childFileFrames;
	wxAuiNotebook*				m_fileTabWithFocus;
	wxArrayString				m_addressList;
	wxAuiNotebook*				m_debuggerTabs;
	CSSErrorList*				m_errorList;
	CSSBreakpoints*				m_breakpoints;
	CSSBookmarks*				m_bookmarks;
	CSSSearchDialog*			m_searchDialog;
	CSSSearchResults*			m_searchResults;
	CSSFindSymbolDialog*		m_findSymbolDialog;
	CSSNavigationHistory*		m_navigationHistory;
	wxFileConfig				m_optionsConfig;
	CSSColourSelectionDialog*	m_colourDialog;
	wxStatusBar*				m_statusBar;
	CEditorConnectionHelper		m_editorConnection;
	EConnectionStatus			m_connectionStatus;
	EConnectionStatus			m_previousConnectionStatus;
	CEditorCompilationHelper	m_compilationHelper;
	CDebuggerHelper				m_debuggerHelper;
	CProfilerHelper				m_profilerHelper;
	CPingHelper					m_pingUtil;
	TDebuggerPanels				m_debuggerPanels;
	EModCompilationAction		m_rememberedCompilationAction;
	wxString					m_fileWithActiveCallstackFrame;
	wxArrayString				m_previousOpenSolutions;

	bool						m_isProfiling;
	bool						m_isContinuousProfiling;
	bool						m_isDebugging;

	bool						m_disableOnActivate;

public:
	//! Get bookmarks
	CSSBookmarks* GetBookmarks() const { return m_bookmarks; }

	//! Get the search results
	CSSSearchResults* GetSearchResults() const { return m_searchResults; }

	void ShowTab( wxWindow* tab );

	//! Switches to the next (if shift > 0) or previous (shift < 0) files tab
	void GotoNextFilesTab( bool rotate, int shift = 1 );

	template < typename T >
	inline T ReadOption( const wxString& path, const wxString& option, T defaultVal )
	{
		m_optionsConfig.SetPath( path );

		T intermediary;

		m_optionsConfig.Read( option, &intermediary, defaultVal );

		return intermediary;
	}

	template < typename T >
	inline T ReadOption( const wxString& option, T defaultVal )
	{
		return ReadOption( CONFIG_OPTIONS_PATH, option, defaultVal );
	}

	template < typename T >
	inline T ReadStyle( const wxString& style, const wxString& option, const T& defaultVal )
	{
		m_optionsConfig.SetPath( CONFIG_STYLE_PATH );
		m_optionsConfig.SetPath( style );

		T retVal;
		m_optionsConfig.Read( option, &retVal, defaultVal );

		return retVal;
	}

	template < typename T >
	inline bool ReadStyle2( const wxString& style, const wxString& option, T& outValue )
	{
		m_optionsConfig.SetPath( CONFIG_STYLE_PATH );
		m_optionsConfig.SetPath( style );

		return m_optionsConfig.Read( option, &outValue );
	}

	template <typename T >
	inline bool WriteStyle( const wxString& style, const wxString& option, const T& value )
	{
		m_optionsConfig.SetPath( CONFIG_STYLE_PATH );
		m_optionsConfig.SetPath( style );

		return m_optionsConfig.Write( option, value );
	}

	inline bool ClearStyle( const wxString& style, const wxString& option )
	{
		m_optionsConfig.SetPath( CONFIG_STYLE_PATH );
		m_optionsConfig.SetPath( style );

		return m_optionsConfig.DeleteEntry( option, true );
	}

	inline void WriteCaretStyle( const wxColour& colour, int blinkRate, int thickness, bool highlightLine, const wxColour& highlightColour, const wxColour& wordHighlightingColour )
	{
		m_optionsConfig.SetPath( CONFIG_CARET_PATH );

		m_optionsConfig.Write( wxT( "colour" ),					colour );
		m_optionsConfig.Write( wxT( "blinkrate" ),				blinkRate );
		m_optionsConfig.Write( wxT( "thickness" ),				thickness );
		m_optionsConfig.Write( wxT( "highlight" ),				highlightLine );
		m_optionsConfig.Write( wxT( "hlcolour" ),				highlightColour );
		m_optionsConfig.Write( wxT( "wordHighlightingColour" ),	wordHighlightingColour );
	}

	inline void ReadCaretStyle( wxColour& colour, int& blinkRate, int& thickness, bool& highlightLine, wxColour& highlightColour, wxColour& wordHighlightingColour )
	{
		m_optionsConfig.SetPath( CONFIG_CARET_PATH );

		m_optionsConfig.Read( wxT( "colour" ),					&colour,					colour );
		m_optionsConfig.Read( wxT( "blinkrate" ),				&blinkRate,					blinkRate );
		m_optionsConfig.Read( wxT( "thickness" ),				&thickness,					thickness );
		m_optionsConfig.Read( wxT( "highlight" ),				&highlightLine,				highlightLine );
		m_optionsConfig.Read( wxT( "hlcolour" ),				&highlightColour,			highlightColour );
		m_optionsConfig.Read( wxT( "wordHighlightingColour" ),	&wordHighlightingColour,	wordHighlightingColour );
	}

public:
	CSSFrame( Solution* solution );
	~CSSFrame();

	bool OpenFileAndGotoLine( const wxString& filepath, int line );
	bool OpenFileAndGotoLine( const SolutionFilePtr& file, bool select, int gotoLine, int frameIndex = -1 );
	bool OpenFile( const wxString& filepath, bool select, int frameIndex = -1 );
	bool OpenFile( const SolutionFilePtr& file, bool select, int frameIndex = -1 );
	bool CloseFile( SolutionFilePtr file, bool saveLayout );

	CSSDocument* GetCurrentDocument();
	const CSSDocument* GetCurrentDocument() const;

	template < typename TDoc >
	void GetDocuments( vector< TDoc* >& documents ) const
	{
		GetDocumentsFromFileTab( m_fileTabs, documents );

		for( TChildFrameList::const_iterator iter = m_childFileFrames.begin(); iter != m_childFileFrames.end(); ++iter )
		{
			wxWindow* codeTabsBase = (*iter)->FindWindow( wxT( "codeTabs" ) );
			wxAuiNotebook* codeTabs = wxDynamicCast( codeTabsBase, wxAuiNotebook );

			if( codeTabs )
			{
				GetDocumentsFromFileTab( codeTabs, documents );
			}
		}
	}

	template < typename TDoc >
	void CSSFrame::GetDocuments( vector< TDoc* >& documents )
	{
		// Please see:
		// http://stackoverflow.com/questions/123758/how-do-i-remove-code-duplication-between-similar-const-and-non-const-member-func 
		
		const_cast< vector< TDoc* > >( static_cast< const CSSFrame* >( this )->GetDocuments( documents ) );
	}

	bool IsAnyDocumentModified() const;

	void DocumentContentModified( CSSDocument* document );

	void ValidateOpenedFiles();

	void InvalidateParsedFiles();

	int CreateNewCodeFrame( int x, int y, int height, int width );

	void DisableDebugging();

	void SetStatusBarText( const wxChar* format, ... );
	void SetStatusBarText( const wxString& text ) { m_statusBar->SetStatusText( text, SBField_UpdateInfo ); }
	void SetStatusBarLine( int line )
	{
		wxString text;
		text.Printf( wxT( "Line: %i" ), line );
		m_statusBar->SetStatusText( text, SBField_Line );
	}
	
	void SetStatusBarColumn( int col )
	{
		wxString text;
		text.Printf( wxT( "Col: %i" ), col );
		m_statusBar->SetStatusText( text, SBField_Column );
	}

	void SetStatusBarCharacterPosition( int position )
	{
		wxString text;
		text.Printf( wxT( "Pos: %i" ), position );
		m_statusBar->SetStatusText( text, SBField_CharacterIndex );
	}

	void SetClientToGame();

protected:
	void OnActivate( wxActivateEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnCodeWindowClose( wxCloseEvent& event );
	void OnPageChanged( wxAuiNotebookEvent& event );
	void OnPageClose( wxAuiNotebookEvent& event );
	void OnPageClosed( wxAuiNotebookEvent& event );
	void OnPageRightUp( wxAuiNotebookEvent& event );
	void OnPageDND( wxAuiNotebookEvent& event );
	void OnPageDragDone( wxAuiNotebookEvent& event );
	void OnAttachDebuggerClicked( wxCommandEvent& event );
	void OnAddressSelectionChanged( wxCommandEvent& event );
	void OnConnectionPortSelectionChanged( wxCommandEvent& event );
	void OnOpenSolutionFileDialog( wxCommandEvent& event );
	void OnOpenSolutionFile( COpenFileEvent& event );
	void OnOpenScriptsDirectory( wxCommandEvent& event );
	void OpenScriptsDirectory( const wxString& path );
	void OnCreateNewMod( wxCommandEvent& event );
	void OnOpenExistingSolution( wxCommandEvent& event );
	bool CheckAndPromptForModifiedFiles();
	void OpenExistingSolution( const wxString& path );
	void OnInstallMod( wxCommandEvent& event );
	void OnSyncSolutionToGame( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnSaveAll( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnUndo( wxCommandEvent& event );
	void OnRedo( wxCommandEvent& event );
	void OnCut( wxCommandEvent& event );
	void OnCopy( wxCommandEvent& event );
	void OnPaste( wxCommandEvent& event );
	void OnRefreshScripts( wxCommandEvent& event );
	void OnForgetCompilationAction( wxCommandEvent& event );
	void OnDebugContinue( wxCommandEvent& event );
	void OnDebugStepOver( wxCommandEvent& event );
	void OnDebugStepInto( wxCommandEvent& event );
	void OnDebugStepOut( wxCommandEvent& event );
	void OnDebugToggleBreakpoint( wxCommandEvent& event );
	void OnDebugClearAllBreakpoints( wxCommandEvent& event );
	void OnDebugToggleBookmark( wxCommandEvent& event );
	void OnDebugNextBookmark( wxCommandEvent& event );
	void OnDebugPreviousBookmark( wxCommandEvent& event );
	void OnDebugClearAllBookmarks( wxCommandEvent& event );
	void OnDebugProfile( wxCommandEvent& event );
	void OnDebugProfileContinuous( wxCommandEvent& event );
	void OnUnfilteredLocals( wxCommandEvent& event );
	void OnSortLocals( wxCommandEvent& event );
	void OnFind( wxCommandEvent& event );
	void OnFindAll( wxCommandEvent& event );
	void OnFindNext( wxCommandEvent& event );
	void OnFindPrev( wxCommandEvent& event );
	void OnEditGoto( wxCommandEvent& event );
	void OnNavigateBackward( wxCommandEvent& event );
	void OnNavigateForward( wxCommandEvent& event );
	void OnWindowCloseAllDocs( wxCommandEvent& event );
	void OnUnignoreChanges( wxCommandEvent& event );
	void OnCloseAllButThis( wxCommandEvent& event );
	void OnCopyFullPath( wxCommandEvent& event );
	void OnDetatchDocument( wxCommandEvent& event );
	void OnMoveDocument( wxCommandEvent& event );
	void OnToolsFindSymbol( wxCommandEvent& event );
	void OnOptionsCodeOutlining( wxCommandEvent& event );
	void OnOptionsBracketHighlighting( wxCommandEvent& event );
	void OnOptionsShowWhitespace( wxCommandEvent& event );
	void OnOptionsShowEOL( wxCommandEvent& event );
	void OnOptionsWordHighlight ( wxCommandEvent& event );
	void OnOptionsHoverTooltips( wxCommandEvent& event );
	void OnOptionsHoverAnnotations( wxCommandEvent& event );
	void OnOptionsHoverDisabled( wxCommandEvent& event );
	void OnOptionsIndentationGuide( wxCommandEvent& event );
	void OnOptionsColoursDialog( wxCommandEvent& event );
	void OnOptionsBreakpointStartBar( wxCommandEvent& event );
	void OnOptionsBreakpointWindowToFront( wxCommandEvent& event );
	void OnOptionsBreakpointAutoSaving( wxCommandEvent& event );
	void OnOptionsAdvancedOpcodes( wxCommandEvent& event );
	void OnOptionsAdvancedPing( wxCommandEvent& event );
	void OnOptionsAdvancedDisableParser( wxCommandEvent& event );
	void OnShowVersionControlCredentials( wxCommandEvent& event );
	void OnCloseVersionControlCredentials( CCredentialsChangedEvent& event );
	void OnPreviousSolutionSelected( wxCommandEvent& event );

	void OnDocumentClosed( wxAuiNotebookEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnOptionsColourDialogClose( wxCloseEvent& event );

	void OnBreakpointToggled( CBreakpointToggledEvent& event );
	void OnBookmarkToggled( CBookmarkToggledEvent& event );
	void OnLineMove( CLineMoveEvent& event );
	void OnNavigationGoto( CNavigationGotoEvent& event );

protected:
	void UpdateAddressList( const wxString& addressToSelect );
	void SetDebuggingButtonStates( bool enabled );
	void UpdateButtonStates();
	void Breakpoint( const wxString& file, int line );	
	bool CloseAllDocs( bool justSave, wxAuiNotebook* tabGroup = NULL, bool withoutCurrentDoc = false ); // If tabGroup is NULL, all tabs are closed, otherwise, only tabs belonging to that AuiNoteBook will be closed
	void CloseAllButThisDocs( bool justSave );
	void GetDocumentsFromFileTab( wxAuiNotebook* fileTabs, vector< CSSDocument* >& out ) const;
	void GetDocumentsFromFileTab( wxAuiNotebook* fileTabs, vector< CSSDocumentEx* >& out ) const;
	void MoveSelectedPage( wxAuiNotebook* source, wxAuiNotebook* dest );
	bool CloseCodeFrameIfEmpty( wxAuiNotebook* codeTabs );
	wxAuiNotebook* CreateNewCodeFrame( const wxPoint& position = wxDefaultPosition, const wxSize& size = wxDefaultSize );

	void FindOpenWindow( bool findAll );
	void FindNext();
	void FindPrev();

	void SaveBreakpoints();

	void Save();
	void SaveAll();
	void Goto();
	void DebugContinue();

	void ReloadAllOpenDocumentStyles();

	//Connection Related
private:
	static const uint16_t EDITOR_DEFAULT_PORT = 37000;
	static const uint16_t GAME_DEFAULT_PORT = 37001;

	void OnConnectionEvent( CConnectionEvent& event );
	void OnPackageSyncListingEvent( CPackageSyncListingEvent& event );

	void OnCompilationStartedEvent( CCompilationStartedEvent& event );
	void OnCompilationEndedEvent( CCompilationEndedEvent& event );
	void OnCompilationErrorEvent( CCompilationErrorEvent& event );
	void OnCompilationLogEvent( CCompilationLogEvent& event );

	void ClearActiveCallstackFrameMarker();
	void OnCallstackFrameSelected( CCallstackFrameSelectedEvent& event );
	void OnGoto( CGotoEvent& event );

public:
	void ToggleBreakpoint( const SolutionFilePtr& file, int line, bool enabled );

private:
	void OnBreakpointToggleConfirmationEvent( CBreakpointToggleConfirmationEvent& event );
	void OnBreakpointHitEvent( CBreakpointHitEvent& event );
	void OnOpcodeListingEvent( CSSOpcodeListingEvent& event );

private:
	void OnProfilerStartedEvent( CProfilerStartedEvent& event );
	void OnProfileReportEvent( CProfilerReportEvent& event );
	void OnPingReceived( CPingEvent& event );

// Editor related
private:
	void InstallMod();
	void ReloadScripts();
	void CopyToClipboard( const wxString& toCopy ) const;

	void RememberSolution( wxString solutionPath );
	void LoadPreviousSolutionOptions();
	void PopulatePreviousSolutionOptions();
};

extern CSSFrame* wxTheFrame;
