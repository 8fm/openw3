/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "frame.h"

#include "solutionExplorer.h"
#include "errorList.h"
#include "breakpoints.h"
#include "bookmarks.h"
#include "profiler.h"
#include "searchResults.h"
#include "searchDlg.h"
#include "gotoDlg.h"
#include "openSolutionFileDlg.h"
#include "closeAllDialog.h"
#include "findSymbolDlg.h"
#include "navigationHistory.h"
#include "colourSelectionDialog.h"
#include "credentials.h"
#include "fileStubs.h"
#include "app.h"
#include "documentView.h"
#include "changesDetectedDialog.h"

#include "events/eventbreakpoints.h"
#include "events/eventbookmarks.h"
#include "events/eventLineMove.h"
#include "events/eventGoto.h"
#include "events/eventNavigationGoto.h"
#include "events/eventOpcodeListing.h"

#include <wx/progdlg.h>
#include <wx/clipbrd.h>

#include "widgets/debuggerTabs/locals.h"
#include "widgets/debuggerTabs/watch.h"
#include "widgets/debuggerTabs/callstack.h"

#include "solution/slnContainer.h"
#include "createModDialog.h"
#include "installModDialog.h"
#include "compileModDialog.h"
#include "exporter.h"

wxIMPLEMENT_CLASS( CSSFrame, wxFrame );

BEGIN_EVENT_TABLE( CSSFrame, wxFrame )
	EVT_CLOSE( CSSFrame::OnClose )
	EVT_ACTIVATE( CSSFrame::OnActivate )
	EVT_CHOICE( XRCID("AddressHistory"), CSSFrame::OnAddressSelectionChanged )
	EVT_CHOICE( XRCID("ConnectionPort"), CSSFrame::OnConnectionPortSelectionChanged )
	EVT_TOOL( XRCID("attachDebugger"), CSSFrame::OnAttachDebuggerClicked )
	EVT_TOOL( XRCID("openScriptDirectory"), CSSFrame::OnOpenScriptsDirectory )
	EVT_TOOL( XRCID("createNewMod"), CSSFrame::OnCreateNewMod )
	EVT_TOOL( XRCID("openExistingSolution"), CSSFrame::OnOpenExistingSolution )
	EVT_TOOL( XRCID("installMod"), CSSFrame::OnInstallMod )
	EVT_TOOL( XRCID("syncSolutionToGame"), CSSFrame::OnSyncSolutionToGame )
	EVT_TOOL( XRCID("fileSave"), CSSFrame::OnSave )
	EVT_TOOL( XRCID("fileSaveAll"), CSSFrame::OnSaveAll )
	EVT_TOOL( XRCID("fileCloseAll"), CSSFrame::OnWindowCloseAllDocs )
	EVT_TOOL( XRCID("fileExit"), CSSFrame::OnExit )
	EVT_TOOL( XRCID("editUndo"), CSSFrame::OnUndo )
	EVT_TOOL( XRCID("editRedo"), CSSFrame::OnRedo )
	EVT_TOOL( XRCID("editCut"), CSSFrame::OnCut )
	EVT_TOOL( XRCID("editCopy"), CSSFrame::OnCopy )
	EVT_TOOL( XRCID("editPaste"), CSSFrame::OnPaste )
	EVT_TOOL( XRCID("debugReloadScripts"), CSSFrame::OnRefreshScripts )
	EVT_TOOL( XRCID("debugForgetCompilationAction"), CSSFrame::OnForgetCompilationAction )
	EVT_TOOL( XRCID("debugToggleBreakpoint"), CSSFrame::OnDebugToggleBreakpoint )
	EVT_TOOL( XRCID("debugClearAllBreakpoints"), CSSFrame::OnDebugClearAllBreakpoints )
	EVT_TOOL( XRCID("debugContinue"), CSSFrame::OnDebugContinue )
	EVT_TOOL( XRCID("debugStepOver"), CSSFrame::OnDebugStepOver )
	EVT_TOOL( XRCID("debugStepInto"), CSSFrame::OnDebugStepInto )
	EVT_TOOL( XRCID("debugStepOut"), CSSFrame::OnDebugStepOut )
	EVT_TOOL( XRCID("debugProfile"), CSSFrame::OnDebugProfile )
	EVT_TOOL( XRCID("debugProfileContinuous"), CSSFrame::OnDebugProfileContinuous )
	EVT_TOOL( XRCID("unfilteredLocals"), CSSFrame::OnUnfilteredLocals )
	EVT_TOOL( XRCID("sortLocals"), CSSFrame::OnSortLocals )
	EVT_TOOL( XRCID("editFind"), CSSFrame::OnFind )
	EVT_TOOL( XRCID("editFindAll"), CSSFrame::OnFindAll )
	EVT_TOOL( XRCID("editFindNext"), CSSFrame::OnFindNext )
	EVT_TOOL( XRCID("editFindPrev"), CSSFrame::OnFindPrev )
	EVT_TOOL( XRCID("editGoto"), CSSFrame::OnEditGoto )
	EVT_TOOL( XRCID("navPrev"), CSSFrame::OnNavigateBackward )
	EVT_TOOL( XRCID("navNext"), CSSFrame::OnNavigateForward )
	EVT_TOOL( XRCID("editNavigateBackward"), CSSFrame::OnNavigateBackward )
	EVT_TOOL( XRCID("editNavigateForward"), CSSFrame::OnNavigateForward )
	EVT_TOOL( XRCID("editBookmarksToggleBookmark"), CSSFrame::OnDebugToggleBookmark )
	EVT_TOOL( XRCID("editBookmarksNextBookmark"), CSSFrame::OnDebugNextBookmark )
	EVT_TOOL( XRCID("editBookmarksPreviousBookmark"), CSSFrame::OnDebugPreviousBookmark )
	EVT_TOOL( XRCID("editBookmarksClearAllBookmarks"), CSSFrame::OnDebugClearAllBookmarks )
	EVT_TOOL( XRCID("windowCloseAllDocs"), CSSFrame::OnWindowCloseAllDocs )
	EVT_TOOL( XRCID("toolsOpenSolutionFile"), CSSFrame::OnOpenSolutionFileDialog )
	EVT_TOOL( XRCID("toolsFindSymbol"), CSSFrame::OnToolsFindSymbol )
	EVT_TOOL( XRCID("optionsCodeOutlining"), CSSFrame::OnOptionsCodeOutlining )
	EVT_TOOL( XRCID("optionsCodeHighlightBrackets"), CSSFrame::OnOptionsBracketHighlighting )
	EVT_TOOL( XRCID("optionsCodeShowWhitespace"), CSSFrame::OnOptionsShowWhitespace )
	EVT_TOOL( XRCID("optionsCodeShowEOL"), CSSFrame::OnOptionsShowEOL )
	EVT_TOOL( XRCID("optionsCodeWordHighlight"), CSSFrame::OnOptionsWordHighlight )
	EVT_TOOL( XRCID("optionsCodeHoverTooltips"), CSSFrame::OnOptionsHoverTooltips )
	EVT_TOOL( XRCID("optionsCodeHoverAnnotations"), CSSFrame::OnOptionsHoverAnnotations )
	EVT_TOOL( XRCID("optionsCodeHoverDisabled"), CSSFrame::OnOptionsHoverDisabled )
	EVT_TOOL( XRCID("optionsCodeIndentationGuide"), CSSFrame::OnOptionsIndentationGuide )
	EVT_TOOL( XRCID("optionsCodeColours"), CSSFrame::OnOptionsColoursDialog )
	EVT_TOOL( XRCID("breakpointOptionStartBar" ), CSSFrame::OnOptionsBreakpointStartBar )
	EVT_TOOL( XRCID("breakpointOptionWindowToFront"), CSSFrame::OnOptionsBreakpointWindowToFront )
	EVT_TOOL( XRCID("breakpointOptionAutoSaving"), CSSFrame::OnOptionsBreakpointAutoSaving )
	EVT_TOOL( XRCID("versionControlCredentials"), CSSFrame::OnShowVersionControlCredentials )
	EVT_TOOL( XRCID("optionsAdvancedOpcodes"), CSSFrame::OnOptionsAdvancedOpcodes )
	EVT_TOOL( XRCID("optionsAdvancedPing"), CSSFrame::OnOptionsAdvancedPing )
END_EVENT_TABLE()

enum TabRightClickMenuIds
{
	trcmid_CloseAllButThis = 0,
	trcmid_DetatchDocument,
	trcmid_MoveDocumentToMainWindow,
	trcmid_CopyFullPath,

	trcmid_UnignoreChanges,

	// Must be last on list
	trcmid_MoveDocumentToSubWindow,
};

#define PreviousSolutionPathStartingId ( wxID_HIGHEST + 100 )

CSSFrame* wxTheFrame = NULL;

CSSFrame::CSSFrame( Solution* solution )
	: m_solutionExplorer( NULL )
	, m_solution( solution )
	, m_fileTabs( NULL )
	, m_fileTabWithFocus( NULL )
	, m_debuggerTabs( NULL )
	, m_errorList( NULL )
	, m_breakpoints( nullptr )
	, m_bookmarks( nullptr )
	, m_searchDialog( NULL )
	, m_findSymbolDialog( NULL )
	, m_navigationHistory( nullptr )
	, m_optionsConfig( wxEmptyString, wxEmptyString, wxT("ScriptStudioOptions.ini"), wxEmptyString, wxCONFIG_USE_LOCAL_FILE )
	, m_colourDialog( NULL )
	, m_connectionStatus( ConnStatus_Disconnected )
	, m_previousConnectionStatus( ConnStatus_Disconnected )
	, m_isProfiling( false )
	, m_isContinuousProfiling( false )
	, m_isDebugging( false )
	, m_disableOnActivate( false )
	, m_rememberedCompilationAction( EModCompilationAction::Invalid )
{
	// Set frame pointer
	wxTheFrame = this;

	m_optionsConfig.SetPath( wxT( "/Options" ) );

	// Load layout from XRC
	wxXmlResource::Get()->LoadFrame( this, nullptr, wxT("MainFrame") );

	// Options
	{
		GetMenuBar()->Check( XRCID( "optionsCodeOutlining" ), ReadOption( CONFIG_OPTIONS_OUTLINING, true ) );
		GetMenuBar()->Check( XRCID( "optionsCodeHighlightBrackets" ), ReadOption( CONFIG_OPTIONS_BRACKETHIGHLIGHTING, true ) );
		GetMenuBar()->Check( XRCID( "optionsCodeWordHighlight" ), ReadOption( CONFIG_OPTIONS_WORDHIGHLIGHTING, false ) );
		GetMenuBar()->Check( XRCID( "optionsCodeShowWhitespace" ), ReadOption( CONFIG_OPTIONS_SHOWWHITESPACE, false ) );
		GetMenuBar()->Check( XRCID( "optionsCodeShowEOL" ), ReadOption( CONFIG_OPTIONS_SHOWEOL, false ) );

		GetMenuBar()->Check( XRCID( "breakpointOptionStartBar" ), ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_FLASH_STARTBAR_ICON, true ) );
		GetMenuBar()->Check( XRCID( "breakpointOptionWindowToFront" ), ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_WINDOW_TO_FRONT, true ) );
		GetMenuBar()->Check( XRCID( "breakpointOptionAutoSaving" ), ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_AUTO_SAVING, false ) );

		GetMenuBar()->Check( XRCID( "optionsAdvancedOpcodes" ), ReadOption( CONFIG_ADVANCED_OPTIONS_PATH, CONFIG_SHOW_OPCODES, false ) );

		wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
		bar->ToggleTool( XRCID("sortLocals"), ReadOption( CONFIG_DEBUGGER_OPTIONS_PATH, CONFIG_SORT_LOCALS, false ) );
		bar->ToggleTool( XRCID("unfilteredLocals"), ReadOption( CONFIG_DEBUGGER_OPTIONS_PATH, CONFIG_SHOW_UNFILTERED, true ) );

		LoadPreviousSolutionOptions();
	}

	// Global Script Keywords
	{
		wxString path = wxStandardPaths::Get().GetDataDir() + wxT( "\\..\\redscripts.ini" );
		wxFileConfig keywordsConfig( wxEmptyString, wxEmptyString, path, path, wxCONFIG_USE_RELATIVE_PATH );
		
		keywordsConfig.SetPath( wxT( "globals" ) );
		
		long cookie;
		wxString scriptKeyword;
		if( keywordsConfig.GetFirstEntry( scriptKeyword, cookie ) )
		{
			do
			{
				wxString cppClass = keywordsConfig.Read( scriptKeyword, wxT( "NULL" ) );
				RED_ASSERT( cppClass != wxT( "NULL" ), TXT( "Could not find script keyword: %" ) RED_PRIWs, scriptKeyword.wx_str() );

				GStubSystem.RegisterGlobalKeyword( scriptKeyword.wc_str(), cppClass.wc_str() );
			}
			while( keywordsConfig.GetNextEntry( scriptKeyword, cookie ) );
		}
	}
	
	// Styles
	{
		CSSStyleManager::GetInstance().ReadStyles();
	}

	// Solution panel
	{
		wxPanel* rp = XRCCTRL( *this, "SolutionPanel", wxPanel );

		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		rp->SetSizer( sizer );

		m_solutionExplorer = new CSSSolutionExplorer( rp, m_solution );
		sizer->Add( m_solutionExplorer, 1, wxEXPAND, 0 );

		rp->Layout();
	}

	// Right splitter
	wxSplitterWindow* rightSplitter = XRCCTRL( *this, "RightSplitter", wxSplitterWindow );
	rightSplitter->SetSashPosition( -200 );

	// Documents
	{
		wxPanel* rp = XRCCTRL( *this, "DocumentsPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_fileTabs = new wxAuiNotebook( rp, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE );
		m_fileTabs->SetName( wxT( "codeTabs" ) );
		
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, &CSSFrame::OnPageChanged, this );
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, &CSSFrame::OnPageClose, this );
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, &CSSFrame::OnPageClosed, this );
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP, &CSSFrame::OnPageRightUp, this );
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND, &CSSFrame::OnPageDND, this );
		m_fileTabs->Bind( wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, &CSSFrame::OnPageDragDone, this );

		//Bind( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, &CSSFrame::OnPageEndDrag, this );
		//Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, &CSSFrame::OnBookClose, this );

		sizer1->Add( m_fileTabs, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Debugger tabs
	{
		wxPanel* rp = XRCCTRL( *this, "LogPanel", wxPanel );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_debuggerTabs = new wxAuiNotebook( rp, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_BOTTOM );
		sizer1->Add( m_debuggerTabs, 1, wxEXPAND, 0 );
		rp->SetSizer( sizer1 );
		rp->Layout();
	}

	// Create error output window
	{
		m_errorList = new CSSErrorList( m_debuggerTabs, m_solution );
		m_debuggerTabs->AddPage( m_errorList, wxT("Output"), true, wxTheSSApp->LoadBitmap( wxT("IMG_TAB_OUTPUT") ) );
		m_errorList->AddLine( wxT("Welcome to The Witcher Script Studio\n\n") );
	}

	{
		// Callstack
		CSSCallstackDebuggerTab* callStack = new CSSCallstackDebuggerTab( m_debuggerTabs );
		m_debuggerTabs->AddPage( callStack, wxT( "Callstack" ), false, wxTheSSApp->LoadBitmap( wxT( "IMG_TAB_CALLSTACK" ) ) );
		m_debuggerHelper.Bind( ssEVT_CALLSTACK_EVENT, &CSSCallstackDebuggerTab::OnCallstackEvent, callStack );
		callStack->Bind( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, &CSSFrame::OnCallstackFrameSelected, this );
		m_debuggerPanels.push_back( callStack );

		// Locals
		CSSLocalsDebuggerTab* locals = new CSSLocalsDebuggerTab( m_debuggerTabs );
		locals->SetMinSize( wxSize( 10, 10 ) );
		callStack->Bind( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, &CSSLocalsDebuggerTab::OnStackFrameSelected, locals );
		m_debuggerTabs->AddPage( locals, wxT( "Locals" ), false, wxTheSSApp->LoadBitmap( wxT( "IMG_TAB_LOCALS" ) ) );
		m_debuggerPanels.push_back( locals );

		// Watch
		CSSWatchDebuggerTab* watch = new CSSWatchDebuggerTab( m_debuggerTabs );
		watch->SetMinSize( wxSize( 10, 10 ) );
		callStack->Bind( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, &CSSWatchDebuggerTab::OnStackFrameSelected, watch );
		m_debuggerTabs->AddPage( watch, wxT( "Watch" ), false, wxTheSSApp->LoadBitmap( wxT( "IMG_TAB_WATCH" ) ) );
		m_debuggerPanels.push_back( watch );
	}

	// Create breakpoints window
	{
		m_breakpoints = new CSSBreakpoints( m_debuggerTabs, m_solution );
		m_debuggerTabs->AddPage( m_breakpoints, wxT("Breakpoints"), false, wxTheSSApp->LoadBitmap( wxT("IMG_TAB_BREAKPOINT") ) );

		m_breakpoints->Bind( ssEVT_BREAKPOINT_TOGGLED_EVENT, &CSSFrame::OnBreakpointToggled, this );
		m_breakpoints->Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );

		m_optionsConfig.SetPath( "/Breakpoints" );
		m_breakpoints->LoadConfig( m_optionsConfig );
	}

	// Create search results window
	{
		m_searchResults = new CSSSearchResults( m_debuggerTabs, m_solution );
		m_debuggerTabs->AddPage( m_searchResults, wxT("Find results"), false, wxTheSSApp->LoadBitmap( wxT("IMG_PB_FIND") ) );
	}

	// Create bookmarks window
	{
		m_bookmarks = new CSSBookmarks( m_debuggerTabs, m_solution );
		m_debuggerTabs->AddPage( m_bookmarks, wxT("Bookmarks"), false, wxTheSSApp->LoadBitmap( wxT("IMG_TAB_BOOKMARK") ) );

		m_bookmarks->Bind( ssEVT_BOOKMARK_TOGGLED_EVENT, &CSSFrame::OnBookmarkToggled, this );
		m_bookmarks->Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );

		m_optionsConfig.SetPath( "/Bookmarks" );
		m_bookmarks->LoadConfig( m_optionsConfig );
	}

	// Create navigation history window
	{
		m_navigationHistory = new CSSNavigationHistory( m_debuggerTabs );
		m_debuggerTabs->AddPage( m_navigationHistory, wxT( "Navigation History" ), false, wxTheSSApp->LoadBitmap( wxT( "IMG_TAB_NAVHISTORY" ) ) );

		m_navigationHistory->Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );
	}
	
	// Create find symbol dialog
	{
		if ( m_findSymbolDialog == NULL )
		{
			m_findSymbolDialog = new CSSFindSymbolDialog( this, m_solution );
		}
	}

	// Status bar
	{
		m_statusBar = XRCCTRL( *this, "StatusBar", wxStatusBar );
		RED_ASSERT( m_statusBar, TXT( "Could not find Status Bar wxWidgets Control" ) );

		m_statusBar->SetFieldsCount( SBField_Max );
	}

	// Update address list
	m_addressList.Add( wxT( "127.0.0.1" ) );
	UpdateAddressList( wxT( "127.0.0.1" ) );

	m_editorConnection.SetTargetAddress( wxT( "127.0.0.1" ) );
	m_editorConnection.Bind( ssEVT_CONNECTION_EVENT, &CSSFrame::OnConnectionEvent, this );

	m_compilationHelper.Bind( ssEVT_COMPILATION_STARTED_EVENT, &CSSFrame::OnCompilationStartedEvent, this );
	m_compilationHelper.Bind( ssEVT_COMPILATION_ENDED_EVENT, &CSSFrame::OnCompilationEndedEvent, this );
	m_compilationHelper.Bind( ssEVT_COMPILATION_LOG_EVENT, &CSSFrame::OnCompilationLogEvent, this );
	m_compilationHelper.Bind( ssEVT_COMPILATION_ERROR_EVENT, &CSSFrame::OnCompilationErrorEvent, this );
	m_compilationHelper.Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );
	m_compilationHelper.Bind( ssEVT_PACKAGE_SYNC_LISTING_EVENT, &CSSFrame::OnPackageSyncListingEvent, this );
	
	m_debuggerHelper.Bind( ssEVT_BREAKPOINT_TOGGLE_CONFIRMATION_EVENT, &CSSFrame::OnBreakpointToggleConfirmationEvent, this );
	m_debuggerHelper.Bind( ssEVT_BREAKPOINT_HIT_EVENT, &CSSFrame::OnBreakpointHitEvent, this );
	m_debuggerHelper.Bind( ssEVT_OPCODE_LISTING_EVENT, &CSSFrame::OnOpcodeListingEvent, this );

	m_profilerHelper.Bind( ssEVT_PROFILER_STARTED_EVENT, &CSSFrame::OnProfilerStartedEvent, this );
	m_profilerHelper.Bind( ssEVT_PROFILER_REPORT_EVENT, &CSSFrame::OnProfileReportEvent, this );

	m_pingUtil.Initialize();
	m_pingUtil.Bind( ssEVT_PING_EVENT, &CSSFrame::OnPingReceived, this );

	// Populate
	m_solutionExplorer->RepopulateTree( true, true );

	// Main bar name
	wxString frameCaption;
	frameCaption.Printf( wxT("Witcher Script Studio - %s"), m_solution->GetName().c_str() );
	SetTitle( frameCaption );

	// Layout
	Layout();

	// Attempt to connect to a debugger automatically
	if( m_connectionStatus == ConnStatus_Disconnected )
	{
		m_editorConnection.ToggleConnectionToEditor();
	}

	// Update toolbars
	UpdateButtonStates();
}

CSSFrame::~CSSFrame()
{
	if ( m_searchDialog )
	{
		m_searchDialog->Destroy();
		m_searchDialog = NULL;
	}

	wxTheFrame = NULL;
}

bool CSSFrame::OpenFile( const SolutionFilePtr& file, bool select, int frameIndex )
{
	// Empty file
	if ( !file )
	{
		return false;
	}

	if( file->IsDeleted() || !file->ExistsOnDisk() )
	{
		return false;
	}

	// Already opened, show tab
	if ( file->m_document )
	{
		size_t pageIndex = m_fileTabs->GetPageIndex( file->m_documentEx );
		m_fileTabs->SetSelection( pageIndex );
		return true;
	}

	// Create document view
	CSSDocumentEx* pageWin = new CSSDocumentEx( m_fileTabs, file, m_solution );

	pageWin->GetDocument()->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, &CSSFrame::OnDocumentClosed, this );
	pageWin->GetDocument()->Bind( wxEVT_KEY_DOWN, &CSSFrame::OnKeyDown, this );
	pageWin->GetDocument()->Bind( ssEVT_BREAKPOINT_TOGGLED_EVENT, &CSSFrame::OnBreakpointToggled, this );
	pageWin->GetDocument()->Bind( ssEVT_BOOKMARK_TOGGLED_EVENT, &CSSFrame::OnBookmarkToggled, this );
	pageWin->GetDocument()->Bind( ssEVT_LINE_MOVE_EVENT, &CSSFrame::OnLineMove, this );
	pageWin->GetDocument()->Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );
	pageWin->GetDocument()->Bind( ssEVT_NAVIGATION_GOTO_EVENT, &CSSFrame::OnNavigationGoto, this );
	pageWin->GetDocument()->Bind( ssEVT_NAVIGATION_HISTORY_EVENT, &CSSNavigationHistory::OnAdd, m_navigationHistory );

	m_breakpoints->RestoreAll( file );
	m_bookmarks->RestoreAll( file );

	wxAuiNotebook* fileTabs = NULL;

	if( frameIndex == -1 )
	{
		fileTabs = m_fileTabs;
	}
	else
	{
		wxFrame* frame = NULL;
		int count = 0;
		for( TChildFrameList::const_iterator iter = m_childFileFrames.begin(); iter != m_childFileFrames.end(); ++iter )
		{
			if( count == frameIndex )
			{
				frame = *iter;
				break;
			}
			++count;
		}

		fileTabs = wxStaticCast( frame->FindWindow( wxT( "codeTabs" ) ), wxAuiNotebook );
		
		RED_ASSERT( fileTabs, TXT( "Could not find code tabs wxWidgets control" ) );
	}

	// Add to tabs
	const size_t pageIndex = fileTabs->GetPageCount();
	fileTabs->AddPage( pageWin, file->m_name.c_str(), select );	

	// Set tab tooltip
	fileTabs->SetPageToolTip( pageIndex, file->m_absolutePath.c_str() );

	UpdateButtonStates();
	pageWin->UpdateGuiCombo();
	return true;
}

bool CSSFrame::OpenFile( const wxString& filepath, bool select, int frameIndex /*= -1 */ )
{
	SolutionFilePtr solutionFile = m_solution->FindFile( filepath.wc_str() );

	if( solutionFile )
	{
		return OpenFile( solutionFile, select, frameIndex );
	}

	return false;
}

bool CSSFrame::OpenFileAndGotoLine( const wxString& filepath, int line )
{
	SolutionFilePtr solutionFile = m_solution->FindFile( filepath.wc_str() );

	if( solutionFile )
	{
		return OpenFileAndGotoLine( solutionFile, true, line );
	}

	return false;
}

bool CSSFrame::OpenFileAndGotoLine( const SolutionFilePtr& file, bool select, int gotoLine, int frameIndex )
{
	if ( !OpenFile( file, select, frameIndex ) )
	{
		return false;
	}

	// Scroll to line
	CSSDocument* doc = file->m_document;
	if ( doc )
	{
		// Show initial line
		if ( gotoLine != -1 )
		{
			doc->ScrollToLine( gotoLine );
		}

		// Focus
		doc->SetFocus();
	}

	// Done
	return true;
}

bool CSSFrame::CloseFile( SolutionFilePtr file, bool saveLayout )
{
	// No file
	if ( !file )
	{
		return false;
	}

	// Not opened
	if ( !file->m_document )
	{
		return false;
	}

	// Close tab
	wxAuiNotebook* parentBook = wxDynamicCast( file->m_documentEx->GetParent(), wxAuiNotebook );
	int pageIndex = parentBook->GetPageIndex( file->m_documentEx );
	parentBook->RemovePage( pageIndex );

	file->m_document->Destroy();

	// Reset document state
	file->CancelModified();
	file->m_document = NULL;

	file->m_documentEx->Destroy();
	file->m_documentEx = NULL;

	// Save layout
	if ( saveLayout )
	{
		m_solutionExplorer->SaveLayout();
	}

	// Update gui
	UpdateButtonStates();
	return true;
}

void CSSFrame::SaveBreakpoints()
{
	m_optionsConfig.SetPath( "/Breakpoints" );
	m_breakpoints->SaveConfig( m_optionsConfig );

	m_optionsConfig.Flush();
}

void CSSFrame::OnClose( wxCloseEvent& event )
{
	GStubSystem.Pause();
	if ( IsAnyDocumentModified() )
	{
		if( !CloseAllDocs( true ) )
		{
			event.Veto();
			GStubSystem.Unpause();
			return;
		}
	}

	// Save document layout
	m_solutionExplorer->SaveLayout();

	m_optionsConfig.SetPath( "/Bookmarks" );
	m_bookmarks->SaveConfig( m_optionsConfig );

	SaveBreakpoints();

	event.Skip();
}

void CSSFrame::OnCodeWindowClose( wxCloseEvent& event )
{
	GStubSystem.Pause();

	wxFrame* closingFrame = wxDynamicCast( event.GetEventObject(), wxFrame );
	wxAuiNotebook* fileTab = wxDynamicCast( closingFrame->FindWindow( wxT( "codeTabs" ) ), wxAuiNotebook );
	
	if( CloseAllDocs( false, fileTab ) )
	{
		m_childFileFrames.remove( closingFrame );
		event.Skip();
	}
	else
	{
		event.Veto();
	}

	GStubSystem.Unpause();
}

void CSSFrame::ShowTab( wxWindow* tab )
{
	int idx = m_debuggerTabs->GetPageIndex( tab );
	if ( idx >= 0 )
	{
		m_debuggerTabs->SetSelection( idx );
		return;
	}

	idx = m_fileTabs->GetPageIndex( tab );
	if ( idx >= 0 )
	{
		m_fileTabs->SetSelection( idx );
		return;
	}
	
	for( TChildFrameList::const_iterator iter = m_childFileFrames.begin(); iter != m_childFileFrames.end(); ++iter )
	{
		wxAuiNotebook* noteBook = wxDynamicCast( (*iter)->FindWindow( wxT( "codeTabs" ) ), wxAuiNotebook );

		idx = noteBook->GetPageIndex( tab );
		if ( idx >= 0 )
		{
			noteBook->SetSelection( idx );
			return;
		}
	}

	tab->Show();
}

void CSSFrame::GotoNextFilesTab( bool rotate, int shift /* = 1 */ )
{
	if( m_fileTabWithFocus )
	{
		int pageCount = static_cast< int >( m_fileTabWithFocus->GetPageCount() );
		int pageIndex = m_fileTabWithFocus->GetSelection() + shift;
		if ( pageIndex >= pageCount )
		{
			if ( rotate )
			{
				pageIndex = 0;
			}
			else
			{
				pageIndex = pageCount > 0 ? pageCount - 1 : 0;
			}
		}
		else if ( pageIndex < 0 )
		{
			if ( rotate )
			{
				pageIndex = pageCount > 0 ? pageCount - 1 : 0;
			}
			else
			{
				pageIndex = 0;
			}
		}

		m_fileTabWithFocus->SetSelection( pageIndex );
	}
}

void CSSFrame::OnPageChanged( wxAuiNotebookEvent& event )
{
	// Update buttons
	UpdateButtonStates();

	wxAuiNotebook* changedBook = wxDynamicCast( event.GetEventObject(), wxAuiNotebook );

	wxFrame* parentFrame = wxDynamicCast( changedBook->GetParent(), wxFrame );

	if( parentFrame )
	{
		int selection = changedBook->GetSelection();
		wxString caption = changedBook->GetPageText( selection );

		parentFrame->SetTitle( caption );
	}
}

void CSSFrame::OnPageClose( wxAuiNotebookEvent& event )
{
	// Unbind file
	CSSDocumentEx* editor = wxDynamicCast( m_fileTabWithFocus->GetPage( event.GetSelection() ), CSSDocumentEx );
	if ( editor )
	{
		SolutionFilePtr file = editor->GetFile();

		// Ask for save
		if ( file->IsModified() )
		{
			int ret = wxMessageBox( wxT("Save file before closing ?"), wxT("Closing document"), wxICON_QUESTION | wxYES_NO | wxCANCEL | wxYES_DEFAULT );
			if ( ret == wxYES )
			{
				if ( !file->Save() )
				{
					wxMessageBox( wxT("Unable to save file !"), wxT("Error"), wxOK | wxICON_ERROR );
					event.Veto();
					return;
				}
			}
			else if ( ret == wxCANCEL )
			{
				event.Veto();
				return;
			}
		}

		// Reset document state
		file->CancelModified();

		// These are cleaned up by wx
		file->m_document = nullptr;
		file->m_documentEx = nullptr;
	}
}

void CSSFrame::OnPageClosed( wxAuiNotebookEvent& event )
{
	CloseCodeFrameIfEmpty( m_fileTabWithFocus );

	// Save layout of solution
	m_solutionExplorer->SaveLayout();

	// Update buttons
	UpdateButtonStates();
}

void CSSFrame::OnPageRightUp( wxAuiNotebookEvent& event )
{
	wxMenu menu;

	m_fileTabWithFocus->SetSelection( event.GetSelection() );

	CSSDocument* currentDocument = GetCurrentDocument();

	if( currentDocument && currentDocument->GetFile()->IgnoreChanges() )
	{
		menu.Append( trcmid_UnignoreChanges, wxT( "Stop ignoring changes to file" ) );
		menu.AppendSeparator();
	}

	menu.Append( trcmid_CloseAllButThis, wxT( "Close all but this" ) );
	menu.Append( trcmid_CopyFullPath, wxT( "Copy full path" ) );
	menu.AppendSeparator();
	menu.Append( trcmid_DetatchDocument, wxT( "Float" ) );

	if( m_fileTabWithFocus != m_fileTabs )
	{
		menu.Append( trcmid_MoveDocumentToMainWindow, wxT( "Move to Main window" ) );
	}

	int count = 0;
	for( TChildFrameList::const_iterator iter = m_childFileFrames.begin(); iter != m_childFileFrames.end(); ++iter )
	{
		if( m_fileTabWithFocus->GetParent() != *iter )
		{
			wxString optionText;
			optionText.Printf( "Move to '%s'", (*iter)->GetTitle().wc_str() );

			menu.Append( trcmid_MoveDocumentToSubWindow + count, optionText );
			menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnMoveDocument, this, trcmid_MoveDocumentToSubWindow + count );
		}

		++count;
	}

	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnUnignoreChanges, this, trcmid_UnignoreChanges );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnCloseAllButThis, this, trcmid_CloseAllButThis );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnDetatchDocument, this, trcmid_DetatchDocument );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnMoveDocument, this, trcmid_MoveDocumentToMainWindow );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSFrame::OnCopyFullPath, this, trcmid_CopyFullPath );

	PopupMenu( &menu );
}

void CSSFrame::OnPageDND( wxAuiNotebookEvent& event )
{
	event.Allow();
}

void CSSFrame::OnPageDragDone( wxAuiNotebookEvent& event )
{
	wxAuiNotebook* book = wxDynamicCast( event.GetEventObject(), wxAuiNotebook );
	CloseCodeFrameIfEmpty( book );
}

void CSSFrame::OnAttachDebuggerClicked( wxCommandEvent& event )
{
	switch( m_connectionStatus )
	{
	case ConnStatus_JustDropped:
	case ConnStatus_Connect:
	case ConnStatus_Connecting:
	case ConnStatus_JustConnected:
	case ConnStatus_Connected:
		{
			wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
			bar->EnableTool( XRCID( "attachDebugger" ), false );
		}
		break;

	case ConnStatus_Disconnected:
	case ConnStatus_JustDisconnected:
		break;
	}

	m_editorConnection.ToggleConnectionToEditor();
}

void CSSFrame::OnAddressSelectionChanged( wxCommandEvent& event )
{
	wxChoice* choice = XRCCTRL( *this, "AddressHistory", wxChoice );
	wxString addr = choice->GetStringSelection();
	if ( addr == wxT("<new>") )
	{
		wxTextEntryDialog* inputBox = new wxTextEntryDialog( this, wxT( "Enter address of remote host:" ), wxT( "Connect to game" ) );

		if ( inputBox->ShowModal() == wxID_OK )
		{
			addr = inputBox->GetValue();
			int index = m_addressList.Index( addr, false );
			if ( index == wxNOT_FOUND )
			{
				index = m_addressList.Add( addr );
			}

			// Refresh list
			UpdateAddressList( addr );

			m_editorConnection.SetTargetAddress( addr.wx_str() );
		}
		else
		{
			// Select anything
			choice->SetSelection( 0 );
		}
	}
	else
	{
		m_editorConnection.SetTargetAddress( addr.wx_str() );
	}
}

void CSSFrame::SetClientToGame()
{
	wxChoice* clientPort = XRCCTRL( *this, "ConnectionPort", wxChoice );

	clientPort->SetSelection( 1 );

	wxCommandEvent dummy;
	OnConnectionPortSelectionChanged( dummy );
}

void CSSFrame::OnConnectionPortSelectionChanged( wxCommandEvent& )
{
	wxChoice* choice = XRCCTRL( *this, "ConnectionPort", wxChoice );
	wxString port = choice->GetStringSelection();
	if ( port == wxT("editor") )
	{
		m_editorConnection.SetTargetPort( EDITOR_DEFAULT_PORT );
	}
	else if ( port == wxT("game") )
	{
		m_editorConnection.SetTargetPort( GAME_DEFAULT_PORT );
	}
}

void CSSFrame::UpdateAddressList( const wxString& addressToSelect )
{
	// Begin update
	wxChoice* choice = XRCCTRL( *this, "AddressHistory", wxChoice );
	choice->Freeze();
	choice->Clear();

	// Add addresses
	choice->Append( m_addressList );
	choice->AppendString( wxT("<new>") );

	// Select address
	int address = choice->FindString( addressToSelect, false );
	choice->SetSelection( address );

	// End update
	choice->Thaw();
	choice->Refresh();
}

void CSSFrame::OnOpenSolutionFileDialog( wxCommandEvent& event )
{
	CSSOpenSolutionFileDialog* dialog = new CSSOpenSolutionFileDialog( this );

	dialog->Bind( ssEVT_OPEN_FILE_EVENT, &CSSFrame::OnOpenSolutionFile, this );

	dialog->ShowModal();

	dialog->Destroy();
}

void CSSFrame::OnOpenSolutionFile( COpenFileEvent& event )
{
	if( OpenFile( event.GetFile(), true, -1 ) )
	{
		if( event.GenerateHistory() )
		{
			GetCurrentDocument()->GenerateNavigationHistoryEvent();
		}
	}
}

void CSSFrame::OnOpenScriptsDirectory( wxCommandEvent& event )
{
	wxDirDialog dialog( this, wxT( "Please select the root folder of the scripts directory you would like to open" ) );

	int result = dialog.ShowModal();

	if( result == wxID_CANCEL )
	{
		return;
	}

	OpenScriptsDirectory( dialog.GetPath() + wxFileName::GetPathSeparator() );
}

void CSSFrame::OpenScriptsDirectory( const wxString& path )
{
	if( !wxDirExists( path ) )
		return;

	if( !CheckAndPromptForModifiedFiles() )
		return;

	wxProgressDialog* progressDialog = new wxProgressDialog( wxT( "Opening Directory" ), wxT( "Initialising" ) );
	progressDialog->Show();
	progressDialog->Pulse();

	m_solution->OpenStandard( path.wc_str() );

	// Populate the tree on the right
	m_solutionExplorer->RepopulateTree( true, true, progressDialog );

	wxString frameCaption;
	frameCaption.Printf( wxT( "Witcher Script Studio - %ls" ), m_solution->GetName().c_str() );
	SetTitle( frameCaption );

	RememberSolution( path );

	progressDialog->Destroy();
}

void CSSFrame::OnCreateNewMod( wxCommandEvent& )
{
	CSSCreateModDialog* dialog = new CSSCreateModDialog( this );

	dialog->ShowModal();

	if( !dialog->GetCancelled() )
	{
		if( IsAnyDocumentModified() )
		{
			int saveFirst = wxMessageBox( wxT( "Save open files before continuing?" ), wxT( "Unsaved changes" ), wxYES_NO | wxCANCEL | wxCENTRE, this );

			if( saveFirst == wxYES )
			{
				SaveAll();
			}
			else if( saveFirst == wxCANCEL )
			{
				return;
			}
		}

		wxProgressDialog* progressDialog = new wxProgressDialog( wxT( "Creating mod" ), wxT( "Initialising" ) );
		progressDialog->Show();
		progressDialog->Pulse();

		wxString name = dialog->GetName();
		wxString workspace = dialog->GetWorkspacePath();
		wxString install = dialog->GetInstallPath();

		wxFileName installSourcePath = dialog->GetInstalledScriptsPath();

		wxFileName workspaceSourcePath( workspace + wxFileName::GetPathSeparator() );
		workspaceSourcePath.AppendDir( wxT( "scripts" ) );
		workspaceSourcePath.AppendDir( wxT( "source" ) );

		progressDialog->Pulse( wxT( "Importing source" ) );

		CSSExporter exporter( installSourcePath.GetFullPath(), workspaceSourcePath.GetFullPath() );
		exporter.Export();

		wxFileName workspaceLocalPath( workspace + wxFileName::GetPathSeparator() );
		workspaceLocalPath.AppendDir( wxT( "scripts" ) );
		workspaceLocalPath.AppendDir( wxT( "local" ) );
		wxString workspaceLocal = workspaceLocalPath.GetFullPath();
		wxFileName::Mkdir( workspaceLocal, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

		progressDialog->Pulse( wxT( "Creating solution" ) );

		m_solution->OpenMod( name.wc_str(), workspaceSourcePath.GetFullPath().wc_str(), workspaceLocal.wc_str(), install.wc_str() );

		wxFileName workspaceSolutionPath( workspace + wxFileName::GetPathSeparator() );
		workspaceSolutionPath.SetName( name );
		workspaceSolutionPath.SetExt( "redsln" );

		m_solution->Save( workspaceSolutionPath.GetFullPath().wc_str() );

		RememberSolution( workspaceSolutionPath.GetFullPath() );

		// Populate the tree on the right
		m_solutionExplorer->RepopulateTree( true, true, progressDialog );

		wxString frameCaption;
		frameCaption.Printf( wxT("Witcher Script Studio - %s"), name );
		SetTitle( frameCaption );

		progressDialog->Destroy();
	}

	dialog->Destroy();
}

void CSSFrame::OnOpenExistingSolution( wxCommandEvent& )
{
	wxFileDialog openSolutionDialog( this, wxT( "Open script studio solution file" ), wxEmptyString, wxEmptyString, wxT( "*.redsln" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

	if( openSolutionDialog.ShowModal() != wxID_CANCEL )
	{
		OpenExistingSolution( openSolutionDialog.GetPath() );
	}
}

bool CSSFrame::CheckAndPromptForModifiedFiles()
{
	if( IsAnyDocumentModified() )
	{
		int saveFirst = wxMessageBox( wxT( "Save modified files before continuing?" ), wxT( "Modified files detected" ), wxYES_NO | wxCANCEL | wxCENTRE, this );

		if( saveFirst == wxYES )
		{
			SaveAll();
		}
		else if( saveFirst == wxCANCEL )
		{
			return false;
		}
	}

	return true;
}

void CSSFrame::OpenExistingSolution( const wxString& path )
{
	if( !wxFileExists( path ) )
		return;

	if( !CheckAndPromptForModifiedFiles() )
		return;

	wxProgressDialog* progressDialog = new wxProgressDialog( wxT( "Opening solution" ), wxT( "Scanning directories" ) );
	progressDialog->Show();
	progressDialog->Pulse();

	m_solution->Load( path.wc_str() );

	RememberSolution( path );

	m_solutionExplorer->RepopulateTree( true, true, progressDialog );

	wxString frameCaption;
	frameCaption.Printf( wxT("Witcher Script Studio - %s"), m_solution->GetName().c_str() );
	SetTitle( frameCaption );

	progressDialog->Destroy();
}

void CSSFrame::OnInstallMod( wxCommandEvent& )
{
	InstallMod();
}

void CSSFrame::InstallMod()
{
	if( m_solution->GetType() == Solution_Mod )
	{
		CSSInstallModDialog dialog( this );

		// Use the install path stored in the solution as an initial value (generally speaking it won't move)
		dialog.SetPath( m_solution->GetInstall().c_str() );

		dialog.ShowModal();

		if( !dialog.GetCancelled() )
		{
			wxString install = dialog.GetPath() + wxFileName::GetPathSeparator();

			wxFileName modSourcePath( install );
			modSourcePath.AppendDir( wxT( "mods" ) );

			wxString name = m_solution->GetName().c_str();

			// Some weird arbitrary limitation that all mods need to have "mod" prefixed to the directory name
			if( !name.StartsWith( wxT( "mod" ) ) )
			{
				name.Prepend( wxT( "mod" ) );
			}

			modSourcePath.AppendDir( name );
			modSourcePath.AppendDir( wxT( "content" ) );
			modSourcePath.AppendDir( wxT( "scripts" ) );

			wxFileName installSourcePath( install + wxFileName::GetPathSeparator() );
			installSourcePath.AppendDir( wxT( "content" ) );
			installSourcePath.AppendDir( wxT( "content0" ) );
			installSourcePath.AppendDir( wxT( "scripts" ) );

			wxFileName modLocalPath = modSourcePath.GetFullPath();
			modLocalPath.AppendDir( wxT( "local" ) );

			wxString installSource = installSourcePath.GetFullPath();
			wxString workspaceSource = m_solution->GetPath( Solution_Standard ).c_str();
			wxString modSource = modSourcePath.GetFullPath();
			wxString workspaceLocal = m_solution->GetPath( Solution_Mod ).c_str();
			wxString modLocal = modLocalPath.GetFullPath();

			wxDiskspaceSize_t space = 0;
			if( wxGetDiskSpace( modSource, &space ) )
			{
				if( space > 0 )
				{
					int clearFiles = wxMessageBox( wxT( "Delete files at: " ) + modSource + wxT( "?" ), wxT( "Target directory is not empty" ), wxYES_NO | wxCANCEL | wxCENTRE, this );

					if( clearFiles == wxYES )
					{
						modSourcePath.Rmdir( wxPATH_RMDIR_FULL | wxPATH_RMDIR_RECURSIVE );
					}
					else if( clearFiles == wxCANCEL )
					{
						return;
					}
				}
			}

			wxProgressDialog* progressDialog = new wxProgressDialog( wxT( "Installing mod" ), wxT( "Copying source" ) );
			progressDialog->Show();
			progressDialog->Pulse();

			// Copy only the files that have changed
			CSSDiffExporter sourceExporter( workspaceSource, installSource, modSource );
			sourceExporter.Export();

			progressDialog->Pulse( wxT( "Copying local" ) );

			// Copy all the files created explicitly for the mod
			wxFileName::Mkdir( modLocal, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
			CSSExporter localExporter( workspaceLocal, modLocal );
			localExporter.Export();

			progressDialog->Destroy();
		}
	}
}

void CSSFrame::OnSyncSolutionToGame( wxCommandEvent& event )
{ 
	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT );

	packet.WriteString( "pkgSync" );

	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT, packet );
}

void CSSFrame::OnPackageSyncListingEvent( CPackageSyncListingEvent& event )
{

}

void CSSFrame::OnSave( wxCommandEvent& )
{
	Save();
}

void CSSFrame::OnSaveAll( wxCommandEvent& )
{
	SaveAll();
}

void CSSFrame::OnExit( wxCommandEvent& )
{
	Close();
}

void CSSFrame::OnUndo( wxCommandEvent& )
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->Undo();
		UpdateButtonStates();
	}
}

void CSSFrame::OnRedo( wxCommandEvent& )
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->Redo();
		UpdateButtonStates();
	}
}

void CSSFrame::OnCut( wxCommandEvent& )
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->Cut();
		UpdateButtonStates();
	}
}

void CSSFrame::OnCopy( wxCommandEvent& )
{
	for( CSSDebuggerTabBase* panel : m_debuggerPanels )
	{
		if( panel->HasFocus() && panel->IsEnabled() )
		{
			panel->CopyToClipboard();
			return;
		}
	}

	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->Copy();
		UpdateButtonStates();
	}
}

void CSSFrame::OnPaste( wxCommandEvent& event )
{
	if ( wxTheClipboard->Open() )
	{
		wxTextDataObject data;
		bool gotData = wxTheClipboard->GetData( data );
		wxTheClipboard->Close();

		if( gotData )
		{
			wxString text = data.GetText();
			for( CSSDebuggerTabBase* panel : m_debuggerPanels )
			{
				if( panel->HasFocus() && panel->IsEnabled() && panel->Paste( text ) )
				{
					return;
				}
			}
		}
	}

	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->Paste();
		UpdateButtonStates();
	}
}

void CSSFrame::DisableDebugging()
{
	// Disable flag
	m_isDebugging = false;

	m_breakpoints->ConfirmAll();

	ClearActiveCallstackFrameMarker();


	for( CSSDebuggerTabBase* panel : m_debuggerPanels )
	{
		panel->DebuggingStopped();
	}

	// Update buttons
	UpdateButtonStates();
}

void CSSFrame::OnDebugContinue( wxCommandEvent& event )
{
	m_debuggerHelper.DebugContinue();

	DisableDebugging();
}

void CSSFrame::OnDebugStepOver( wxCommandEvent& event )
{
	m_debuggerHelper.DebugStepOver();

	DisableDebugging();
}

void CSSFrame::OnDebugStepInto( wxCommandEvent& event )
{
	m_debuggerHelper.DebugStepInto();

	DisableDebugging();
}

void CSSFrame::OnDebugStepOut( wxCommandEvent& event )
{
	m_debuggerHelper.DebugStepOut();

	DisableDebugging();
}

void CSSFrame::OnDebugToggleBreakpoint( wxCommandEvent& event )
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->ToggleBreakpoint();
	}
}

void CSSFrame::OnDebugClearAllBreakpoints( wxCommandEvent& event )
{
	m_breakpoints->RemoveAll();
	m_debuggerHelper.DisableAllBreakpoints();
}

void CSSFrame::OnDebugToggleBookmark( wxCommandEvent& event )
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		doc->ToggleBookmark();
	}
}

void CSSFrame::OnDebugNextBookmark( wxCommandEvent& event )
{
	m_bookmarks->GotoNext();
}

void CSSFrame::OnDebugPreviousBookmark( wxCommandEvent& event )
{
	m_bookmarks->GotoPrev();
}

void CSSFrame::OnDebugClearAllBookmarks( wxCommandEvent& event )
{
	m_bookmarks->RemoveAll();
}

void CSSFrame::OnDebugProfile( wxCommandEvent& )
{
	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	wxSpinCtrl* frameCounterCtrl = XRCCTRL( *bar, "ProfileFrameCounter", wxSpinCtrl );
	
	m_profilerHelper.StartProfiling( static_cast< Red::System::Uint32 >( frameCounterCtrl->GetValue() ) );

	UpdateButtonStates();
}

void CSSFrame::OnDebugProfileContinuous( wxCommandEvent& event )
{
	// Continuous profiling
	if ( !m_isContinuousProfiling && !m_isProfiling )
	{
		m_profilerHelper.StartContinuousProfiling();
	}
	else
	{
		m_profilerHelper.StopContinuousProfiling();
	}

	UpdateButtonStates();
}

void CSSFrame::OnUnfilteredLocals( wxCommandEvent& )
{
	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bool isEnabled = bar->GetToolState( XRCID( "unfilteredLocals" ) );

	m_debuggerHelper.SetLocalsUnfiltered( isEnabled );

	m_optionsConfig.SetPath( CONFIG_DEBUGGER_OPTIONS_PATH );
	m_optionsConfig.Write( CONFIG_SHOW_UNFILTERED, isEnabled );
}

void CSSFrame::OnSortLocals( wxCommandEvent& )
{
	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bool isEnabled = bar->GetToolState( XRCID( "sortLocals" ) );

	m_debuggerHelper.SetLocalsSorted( isEnabled );

	m_optionsConfig.SetPath( CONFIG_DEBUGGER_OPTIONS_PATH );
	m_optionsConfig.Write( CONFIG_SORT_LOCALS, isEnabled );


	for( CSSDebuggerTabBase* panel : m_debuggerPanels )
	{
		panel->Refresh();
	}
}

void CSSFrame::OnFind( wxCommandEvent& event )
{
	FindOpenWindow( false );
}

void CSSFrame::OnFindAll( wxCommandEvent& event )
{
	FindOpenWindow( true );
}

void CSSFrame::OnFindNext( wxCommandEvent& event )
{
	FindNext();
}

void CSSFrame::OnFindPrev( wxCommandEvent& event )
{
	FindPrev();
}

void CSSFrame::OnEditGoto( wxCommandEvent& event )
{
	Goto();
}

bool CSSFrame::CloseAllDocs( bool justSave, wxAuiNotebook* tabGroup /* = NULL */, bool withoutCurrentDoc /* = false */ )
{
	vector< CSSDocument* > documents;
	if( !tabGroup )
	{
		GetDocuments( documents );
	}
	else
	{
		GetDocumentsFromFileTab( tabGroup, documents );
	}

	if( withoutCurrentDoc )
	{
		wxWindow* currentDocBase = m_fileTabWithFocus->GetPage( m_fileTabWithFocus->GetSelection() );
		CSSDocumentEx* currentDoc = wxDynamicCast( currentDocBase, CSSDocumentEx );

		for ( unsigned int i = 0; i < documents.size(); i++ )
		{
			if( documents[ i ] == currentDoc->GetDocument() )
			{
				documents[ i ] = documents.back();
				documents.pop_back();
				break;
			}
		}
	}

	vector< SolutionFilePtr > modifiedFiles;

	for ( unsigned int i = 0; i < documents.size(); ++i )
	{
		const SolutionFilePtr& file = documents[ i ]->GetFile();
		if ( file->IsModified() )
		{
			modifiedFiles.push_back( file );
		}
	}

	CSSCloseAllDialog::EResult dialogRes = CSSCloseAllDialog::END_SAVE;
	while( modifiedFiles.size() > 0 && dialogRes == CSSCloseAllDialog::END_SAVE )
	{
		CSSCloseAllDialog* dialog = new CSSCloseAllDialog( this );
		dialog->Init( modifiedFiles );
		dialogRes = static_cast< CSSCloseAllDialog::EResult >( dialog->ShowModal() );

		if( dialogRes == CSSCloseAllDialog::END_CLOSE )
		{
			int result = wxMessageBox( wxT( "Are you sure you don't want to save?" ), wxT( "Close without saving" ), wxYES_NO | wxCANCEL | wxICON_WARNING );

			if( result == wxNO )
			{
				dialogRes = CSSCloseAllDialog::END_SAVE;
			}
			else if( result  == wxCANCEL )
			{
				dialogRes = CSSCloseAllDialog::END_CANCEL;
			}
		}
		else if( dialogRes == CSSCloseAllDialog::END_SAVE )
		{
			vector< unsigned int > successes;
			vector< SolutionFilePtr > checkedFiles;
			dialog->GetChecked( checkedFiles );

			for( unsigned int i = 0; i < checkedFiles.size(); ++i )
			{
				if ( checkedFiles[ i ]->Save() )
				{
					auto result = find( modifiedFiles.begin(), modifiedFiles.end(), checkedFiles[ i ] );
					RED_FATAL_ASSERT( result != modifiedFiles.end(), "modifiedFies and checkedFiles are inconsistent" );

					modifiedFiles.erase( result );
				}
			}

			if( modifiedFiles.size() > 0 && dialogRes == CSSCloseAllDialog::END_SAVE )
			{
				wxMessageBox( wxT( "There was a problem attempting to save some files" ), wxT( "Error" ), wxOK | wxICON_ERROR );
			}
		}
	}

	if( justSave == false )
	{
		if( dialogRes == CSSCloseAllDialog::END_CLOSE || dialogRes == CSSCloseAllDialog::END_SAVE )
		{
			// Close all tabs
			for ( unsigned int i = 0; i < documents.size(); ++i )
			{
				wxWindow* parent = documents[ i ]->GetFile()->m_documentEx->GetParent();

				wxAuiNotebook* parentBook = wxDynamicCast( parent, wxAuiNotebook );

				int idx = parentBook->GetPageIndex( documents[ i ]->GetFile()->m_documentEx );

				parentBook->RemovePage( idx );

				// Unbind file
				{
					SolutionFilePtr file = documents[ i ]->GetFile();

					// Reset document state
					file->CancelModified();
					file->m_document->Destroy();
					file->m_document = NULL;
					file->m_documentEx->Destroy();
					file->m_documentEx = NULL;
				}
			}
		}
	}

	UpdateButtonStates();

	if( dialogRes == CSSCloseAllDialog::END_CANCEL )
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CSSFrame::CloseAllButThisDocs( bool justSave )
{
	CloseAllDocs( justSave, m_fileTabWithFocus, true );
}

void CSSFrame::OnWindowCloseAllDocs( wxCommandEvent& event )
{
	CloseAllDocs( false );
	GStubSystem.Unpause();
}

void CSSFrame::OnUnignoreChanges( wxCommandEvent& event )
{
	CSSDocument* currentDocument = GetCurrentDocument();

	currentDocument->GetFile()->SetIgnoreChanges( false );

	vector< CSSDocument* > documents;
	GetDocuments( documents );
	CDetectOfflineChanges detector( documents );
	detector.DisplayDialog( this );
}

void CSSFrame::OnCloseAllButThis( wxCommandEvent& event )
{
	CloseAllButThisDocs( false );
	GStubSystem.Unpause();
}

void CSSFrame::OnCopyFullPath( wxCommandEvent& event )
{
	CSSDocument* currentDocument = GetCurrentDocument();
	if ( currentDocument == NULL )
	{
		return;
	}

	CopyToClipboard( currentDocument->GetFile()->m_absolutePath.c_str() );
}

void CSSFrame::CopyToClipboard( const wxString& toCopy ) const
{
	if ( !toCopy.empty() && wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( toCopy ) );
		wxTheClipboard->Close();
	}
}

// This has to be by value or else weirdness happens
void CSSFrame::RememberSolution( wxString solutionPath )
{
	// Store new path
	m_previousOpenSolutions.Remove( solutionPath );

	if( m_previousOpenSolutions.GetCount() >= MAX_PREVIOUS_OPEN_SOLUTIONS )
	{
		m_previousOpenSolutions.RemoveAt( m_previousOpenSolutions.GetCount() - 1 );
	}

	m_previousOpenSolutions.Insert( solutionPath, 0 );

	// Store on disk
	m_optionsConfig.SetPath( CONFIG_PREVIOUS_OPEN_SOLUTIONS );

	for( int i = 0; i < MAX_PREVIOUS_OPEN_SOLUTIONS; ++i )
	{
		wxString key;
		key.Printf( wxT( "psln%i" ), i );

		if( i < m_previousOpenSolutions.GetCount() )
		{
			m_optionsConfig.Write( key, m_previousOpenSolutions[ i ] );
		}
		else
		{
			m_optionsConfig.DeleteEntry( key );
		}
	}

	m_optionsConfig.Write( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Tooltips ) );

	// Update the menu
	PopulatePreviousSolutionOptions();
}

void CSSFrame::LoadPreviousSolutionOptions()
{
	m_optionsConfig.SetPath( CONFIG_PREVIOUS_OPEN_SOLUTIONS );

	for( int i = 0; i < MAX_PREVIOUS_OPEN_SOLUTIONS; ++i )
	{
		wxString key;
		key.Printf( wxT( "psln%i" ), i );

		wxString path;

		if( m_optionsConfig.Read( key, &path ) )
		{
			m_previousOpenSolutions.Add( path );
		}
		else
		{
			break;
		}
	}

	// Update the menu
	PopulatePreviousSolutionOptions();
}

void CSSFrame::PopulatePreviousSolutionOptions()
{
	// Create menu from paths
	wxMenu* newMenu = new wxMenu();
	newMenu->Bind( wxEVT_COMMAND_TOOL_CLICKED, &CSSFrame::OnPreviousSolutionSelected, this );

	for( int i = 0; i < m_previousOpenSolutions.GetCount(); ++i )
	{
		int id = PreviousSolutionPathStartingId + i;
		newMenu->Append( id, m_previousOpenSolutions[ i ] );
	}

	// Replace existing menu
	wxMenuBar* bar = GetMenuBar();

	// The first menu should always be the file menu
	// Annoyingly, wxwidgets doesn't seem to want to let me access it via xrcid like a sane person :(
	wxMenu* parentMenu = bar->GetMenu( 0 );

	size_t position;
	wxMenuItem* oldMenuItem = parentMenu->FindChildItem( XRCID( "fileRecentSolutions" ), &position );

	parentMenu->Destroy( oldMenuItem );

	parentMenu->Insert( position, XRCID( "fileRecentSolutions" ), wxT( "Recent" ), newMenu );
}

void CSSFrame::OnPreviousSolutionSelected( wxCommandEvent& event )
{
	int pathIndex = event.GetId() - PreviousSolutionPathStartingId;

	if( pathIndex < m_previousOpenSolutions.GetCount() )
	{
		if( m_previousOpenSolutions[ pathIndex ].EndsWith( wxT( ".redsln" ) ) )
		{
			OpenExistingSolution( m_previousOpenSolutions[ pathIndex ] );
		}
		else
		{
			OpenScriptsDirectory( m_previousOpenSolutions[ pathIndex ] );
		}
	}
}

bool CSSFrame::CloseCodeFrameIfEmpty( wxAuiNotebook* codeTabs )
{
	if( codeTabs->GetPageCount() == 0 && codeTabs != m_fileTabs )
	{
		wxWindow* parentWindow = codeTabs->GetParent();

		wxFrame* parentFrame = wxDynamicCast( parentWindow, wxFrame );

		m_childFileFrames.remove( parentFrame );

		parentFrame->Close();

		return true;
	}

	return false;
}

void CSSFrame::MoveSelectedPage( wxAuiNotebook* source, wxAuiNotebook* dest )
{
	int selection = source->GetSelection();
	wxString caption = source->GetPageText( selection );

	wxWindow* page = source->GetPage( selection );

	source->RemovePage( selection );

	CloseCodeFrameIfEmpty( source );

	dest->AddPage( page, caption, true );
}

int CSSFrame::CreateNewCodeFrame( int x, int y, int height, int width )
{
	wxPoint newFramePosition( x, y );
	wxSize newFrameSize( width, height );

	CreateNewCodeFrame( newFramePosition, newFrameSize );

	return m_childFileFrames.size() - 1;
}

wxAuiNotebook* CSSFrame::CreateNewCodeFrame( const wxPoint& position, const wxSize& size )
{
	wxString newFrameName;
	newFrameName.Printf( wxT( "MOAR COAD %u" ), m_childFileFrames.size() + 1 );

	wxFrame* codeFrame = new wxFrame( this, wxID_ANY, newFrameName, position, size );
	wxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
	codeFrame->SetSizer( sizer );
	wxAuiNotebook* codeBook = new wxAuiNotebook( codeFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_EXTERNAL_MOVE );
	codeBook->SetName( wxT( "codeTabs" ) );
	sizer->Add( codeBook, 1, wxEXPAND );

	codeFrame->Bind( wxEVT_ACTIVATE, &CSSFrame::OnActivate, this );
	codeFrame->Bind( wxEVT_CLOSE_WINDOW, &CSSFrame::OnCodeWindowClose, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, &CSSFrame::OnPageChanged, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, &CSSFrame::OnPageClose, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, &CSSFrame::OnPageClosed, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_TAB_RIGHT_UP, &CSSFrame::OnPageRightUp, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_ALLOW_DND, &CSSFrame::OnPageDND, this );
	codeFrame->Bind( wxEVT_COMMAND_AUINOTEBOOK_DRAG_DONE, &CSSFrame::OnPageDragDone, this );
	codeFrame->Bind( wxEVT_KEY_DOWN, &CSSFrame::OnKeyDown, this );

	codeFrame->Show();

	m_childFileFrames.push_back( codeFrame );

	return codeBook;
}

void CSSFrame::OnDetatchDocument( wxCommandEvent& event )
{
	// Calling Show() on the new frame invalidates m_fileTabWithFocus
	wxAuiNotebook* source = m_fileTabWithFocus;

	wxAuiNotebook* codeBook = CreateNewCodeFrame();

	MoveSelectedPage( source, codeBook );
}

void CSSFrame::OnMoveDocument( wxCommandEvent& event )
{
	wxAuiNotebook* destCodeBook = NULL;

	if( event.GetId() == trcmid_MoveDocumentToMainWindow )
	{
		destCodeBook = m_fileTabs;
	}
	else
	{
		int frameId = event.GetId() - trcmid_MoveDocumentToSubWindow;

		TChildFrameList::iterator iter = m_childFileFrames.begin();
		for( int i = 0; i < frameId; ++i )
		{
			++iter;
		}

		wxFrame* destWindow = *iter;

		wxWindow* codeBookBase = destWindow->FindWindow( wxT( "codeTabs" ) );

		destCodeBook = wxDynamicCast( codeBookBase, wxAuiNotebook );
	}


	MoveSelectedPage( m_fileTabWithFocus, destCodeBook );
}

void CSSFrame::OnToolsFindSymbol( wxCommandEvent& event )
{
	if ( m_findSymbolDialog == NULL )
	{
		m_findSymbolDialog = new CSSFindSymbolDialog( this, m_solution );
	}
	m_findSymbolDialog->Init();
	m_findSymbolDialog->ShowModal();
	m_findSymbolDialog->SelectCurrentInput();
	if ( m_findSymbolDialog->GetSelectedDoc() )
	{
		m_findSymbolDialog->GetSelectedDoc()->SetFocus();
	}
}

CSSDocument* CSSFrame::GetCurrentDocument()
{
	if( m_fileTabWithFocus )
	{
		int tab = m_fileTabWithFocus->GetSelection();
		if ( tab >= 0 )
		{
			CSSDocumentEx* doc = wxDynamicCast( m_fileTabWithFocus->GetPage(tab), CSSDocumentEx );
			return doc ? doc->GetDocument() : NULL;
		}
	}

	return NULL;
}

const CSSDocument* CSSFrame::GetCurrentDocument() const
{
	if( m_fileTabWithFocus )
	{
		int tab = m_fileTabWithFocus->GetSelection();
		if ( tab >= 0 )
		{
			CSSDocumentEx* doc = wxDynamicCast( m_fileTabWithFocus->GetPage(tab), CSSDocumentEx );
			return doc ? doc->GetDocument() : NULL;
		}
	}

	return NULL;
}

void CSSFrame::GetDocumentsFromFileTab( wxAuiNotebook* fileTabs, vector< CSSDocumentEx* >& out ) const
{
	const size_t numTabs = fileTabs->GetPageCount();
	for ( size_t i = 0; i < numTabs; ++i )
	{
		CSSDocumentEx* doc = wxDynamicCast( fileTabs->GetPage( i ), CSSDocumentEx );
		if ( doc )
		{
			out.push_back( doc );
		}
	}
}

void CSSFrame::GetDocumentsFromFileTab( wxAuiNotebook* fileTabs, vector< CSSDocument* >& out ) const
{
	const size_t numTabs = fileTabs->GetPageCount();
	for ( size_t i = 0; i < numTabs; ++i )
	{
		CSSDocumentEx* doc = wxDynamicCast( fileTabs->GetPage( i ), CSSDocumentEx );
		if ( doc )
		{
			out.push_back( doc->GetDocument() );
		}
	}
}

bool CSSFrame::IsAnyDocumentModified() const
{
	vector< CSSDocument* > docs;
	GetDocuments( docs );
	for ( size_t i = 0; i < docs.size(); i++ )
	{
		CSSDocument* doc = docs[i];
		if ( doc->GetModify() )
		{
			return true;
		}
	}

	return false;
}

void CSSFrame::DocumentContentModified( CSSDocument* document )
{
	UpdateButtonStates();

	if( document )
	{
		m_solutionExplorer->UpdateIcon( document->GetFile() );
	}
}

void CSSFrame::SetDebuggingButtonStates( bool enabled )
{
	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bar->EnableTool( XRCID( "debugContinue" ), enabled );
	bar->EnableTool( XRCID( "debugStepOver" ), enabled );
	bar->EnableTool( XRCID( "debugStepInto" ), enabled );
	bar->EnableTool( XRCID( "debugStepOut" ), enabled );

	GetMenuBar()->Enable( XRCID( "debugContinue" ), enabled );
	GetMenuBar()->Enable( XRCID( "debugStepOver" ), enabled );
	GetMenuBar()->Enable( XRCID( "debugStepInto" ), enabled );
	GetMenuBar()->Enable( XRCID( "debugStepOut" ), enabled );
}

void CSSFrame::UpdateButtonStates()
{
	const bool isDocumentOpened = GetCurrentDocument() != NULL;
	const bool isDocumentModified = GetCurrentDocument() && GetCurrentDocument()->GetModify();
	const bool isDocumentEditable = GetCurrentDocument() && !GetCurrentDocument()->GetReadOnly();
	const bool canUndo = GetCurrentDocument() && GetCurrentDocument()->CanUndo();
	const bool canRedo = GetCurrentDocument() && GetCurrentDocument()->CanRedo();
	const bool isAnyDocumentModified = IsAnyDocumentModified();

	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bar->EnableTool( XRCID("fileSave"), isDocumentEditable && isDocumentModified );
	bar->EnableTool( XRCID("fileSaveAll"), isAnyDocumentModified );
	bar->EnableTool( XRCID("sccShowHistory"), isDocumentOpened );
	bar->EnableTool( XRCID("editUndo"), canUndo );
	bar->EnableTool( XRCID("editRedo"), canRedo );
	bar->EnableTool( XRCID("editCopy"), isDocumentEditable );
	bar->EnableTool( XRCID("editCut"), isDocumentEditable );
	bar->EnableTool( XRCID("editPaste"), isDocumentEditable );
	bar->EnableTool( XRCID("editFind"), isDocumentOpened );

	switch( m_connectionStatus )
	{
	case ConnStatus_Disconnected:
		bar->EnableTool( XRCID( "attachDebugger" ), true );

	case ConnStatus_JustDisconnected:
		bar->ToggleTool( XRCID( "attachDebugger" ), false );
		bar->SetToolNormalBitmap( XRCID( "attachDebugger" ), wxTheSSApp->LoadBitmap( wxT( "IMG_DEBUGGER_DISCONNECTED" ) ) );

		bar->EnableTool( XRCID("debugReloadScripts"), false );

		bar->ToggleTool( XRCID("debugProfileContinuous"), false );
		bar->EnableTool( XRCID( "debugProfileContinuous" ), false );
		bar->EnableTool( XRCID( "debugProfile" ), false );

		GetMenuBar()->Enable( XRCID("debugReloadScripts"), false );
		GetMenuBar()->Check( XRCID("debugProfileContinuous"), false );
		GetMenuBar()->Enable( XRCID("debugProfileContinuous"), false );
		GetMenuBar()->Enable( XRCID("debugProfile"), false );
		break;

		case ConnStatus_JustDropped:
		case ConnStatus_Connect:
		case ConnStatus_Connecting:
			bar->ToggleTool( XRCID( "attachDebugger" ), true );
			bar->SetToolNormalBitmap( XRCID( "attachDebugger" ), wxTheSSApp->LoadBitmap( wxT( "IMG_DEBUGGER_CONNECTING" ) ) );

			bar->EnableTool( XRCID( "debugReloadScripts" ), false );

			bar->ToggleTool( XRCID("debugProfileContinuous"), false );
			bar->EnableTool( XRCID( "debugProfileContinuous" ), false );
			bar->EnableTool( XRCID( "debugProfile" ), false );

			GetMenuBar()->Enable( XRCID("debugReloadScripts"), false );
			GetMenuBar()->Check( XRCID("debugProfileContinuous"), false );
			GetMenuBar()->Enable( XRCID("debugProfileContinuous"), false );
			GetMenuBar()->Enable( XRCID("debugProfile"), false );
			break;

		case ConnStatus_JustConnected:
		case ConnStatus_Connected:
			bar->ToggleTool( XRCID( "attachDebugger" ), true );
			bar->SetToolNormalBitmap( XRCID( "attachDebugger" ), wxTheSSApp->LoadBitmap( wxT( "IMG_DEBUGGER_CONNECTED" ) ) );

			bar->EnableTool( XRCID( "debugReloadScripts" ), true );

			bar->ToggleTool( XRCID("debugProfileContinuous"), m_isContinuousProfiling );
			bar->EnableTool( XRCID( "debugProfileContinuous" ), !m_isProfiling );
			bar->EnableTool( XRCID( "debugProfile" ), !m_isContinuousProfiling && !m_isProfiling );

			GetMenuBar()->Enable( XRCID("debugReloadScripts"), true );
			GetMenuBar()->Check( XRCID("debugProfileContinuous"), m_isContinuousProfiling );
			GetMenuBar()->Enable( XRCID("debugProfileContinuous"), !m_isProfiling );
			GetMenuBar()->Enable( XRCID("debugProfile"), !m_isContinuousProfiling && !m_isProfiling );
			break;
	}

	SetDebuggingButtonStates( m_isDebugging );
}

void CSSFrame::OnRefreshScripts( wxCommandEvent& event )
{
	ReloadScripts();
}

void CSSFrame::OnForgetCompilationAction( wxCommandEvent& event )
{
	m_rememberedCompilationAction = EModCompilationAction::Invalid;
}

void CSSFrame::Breakpoint( const wxString& file, int line )
{
	m_debuggerHelper.RequestCallstack();
}

void CSSFrame::ValidateOpenedFiles()
{
	m_searchResults->ValidateFilesInResults();

	vector< CSSDocumentEx* > documents;
	GetDocuments( documents );
	for ( size_t i = 0; i < documents.size(); ++i )
	{
		documents[ i ]->GetDocument()->ValidateFile();
		documents[ i ]->GetDocument()->GetFile()->m_documentEx = documents[ i ];
	}
}

void CSSFrame::InvalidateParsedFiles()
{
	if ( m_findSymbolDialog )
	{
		m_findSymbolDialog->ForceReparse();
	}
}

void CSSFrame::OnActivate( wxActivateEvent& event )
{
	if( m_disableOnActivate )
		return;

	m_disableOnActivate = true;

	if( event.GetActive() )
	{
		vector< CSSDocument* > documents;
		GetDocuments( documents );
		CDetectOfflineChanges detector( documents );
		detector.DisplayDialog( this );
		
		// Update icons
		m_solutionExplorer->UpdateIcons();

		// We need to keep a record of which frame has focus
		wxFrame* activeFrame = wxDynamicCast( event.GetEventObject(), wxFrame );

		if( activeFrame )
		{
			wxWindow* codeTabsBase = activeFrame->FindWindow( wxT( "codeTabs" ) );
			m_fileTabWithFocus = wxDynamicCast( codeTabsBase, wxAuiNotebook );

			RED_LOG_SPAM( RED_LOG_CHANNEL( Frame ), TXT( "Frame Activated %p in Frame %p (Main Frame: %p)" ), m_fileTabWithFocus, activeFrame, this );
		}
	}
	else
	{
		CSSDocument* document = GetCurrentDocument();

		if( document )
		{
			document->Cancel();
		}
	}

	m_disableOnActivate  = false;

	event.Skip();
}

void CSSFrame::OnOptionsCodeOutlining( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_OUTLINING, enabled );

	ReloadAllOpenDocumentStyles();
}

void CSSFrame::OnOptionsBracketHighlighting( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_BRACKETHIGHLIGHTING, enabled );

	ReloadAllOpenDocumentStyles();
}

void CSSFrame::OnOptionsShowWhitespace( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_SHOWWHITESPACE, enabled );

	ReloadAllOpenDocumentStyles();
}

void CSSFrame::OnOptionsShowEOL( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_SHOWEOL, enabled );

	ReloadAllOpenDocumentStyles();
}

void CSSFrame::OnOptionsWordHighlight( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_WORDHIGHLIGHTING, enabled );
}

void CSSFrame::OnOptionsHoverTooltips( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Tooltips ) );
}

void CSSFrame::OnOptionsHoverAnnotations( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Annotations ) );
}

void CSSFrame::OnOptionsHoverDisabled( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_OPTIONS_HOVERINFORMATION, static_cast< int >( HoverInfoType_Off ) );
}

void CSSFrame::OnOptionsIndentationGuide( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_OPTIONS_PATH );

	bool enabled = event.IsChecked();
	m_optionsConfig.Write( CONFIG_OPTIONS_INDENTATIONGUIDES, enabled );

	ReloadAllOpenDocumentStyles();
}

void CSSFrame::OnOptionsColoursDialog( wxCommandEvent& event )
{
	if( !m_colourDialog )
	{
		m_colourDialog = new CSSColourSelectionDialog( this );

		m_colourDialog->Show();

		m_colourDialog->Bind( wxEVT_CLOSE_WINDOW, &CSSFrame::OnOptionsColourDialogClose, this );
	}
}

void CSSFrame::OnOptionsColourDialogClose( wxCloseEvent& event )
{
	ReloadAllOpenDocumentStyles();

	m_colourDialog = NULL;

	// Let wxWidgets handle the actual window closing
	event.Skip();
}

void CSSFrame::OnOptionsBreakpointStartBar( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_BREAKPOINT_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_FLASH_STARTBAR_ICON, event.IsChecked() );
}

void CSSFrame::OnOptionsBreakpointWindowToFront( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_BREAKPOINT_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_WINDOW_TO_FRONT, event.IsChecked() );
}

void CSSFrame::OnOptionsBreakpointAutoSaving( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_BREAKPOINT_OPTIONS_PATH );

	m_optionsConfig.Write( CONFIG_AUTO_SAVING, event.IsChecked() );

	if ( event.IsChecked() )
	{
		SaveBreakpoints();
	}
}

void CSSFrame::OnOptionsAdvancedOpcodes( wxCommandEvent& event )
{
	m_optionsConfig.SetPath( CONFIG_ADVANCED_OPTIONS_PATH );

	bool enabled = event.IsChecked();

	m_optionsConfig.Write( CONFIG_SHOW_OPCODES, enabled );

	vector< CSSDocument* > documents;
	GetDocuments( documents );

	for( unsigned int i = 0; i < documents.size(); ++i )
	{
		documents[ i ]->SetOpcodeMargin( enabled );
		documents[ i ]->PopulateOpcodeMarkers();
	}
}

void CSSFrame::OnOptionsAdvancedPing( wxCommandEvent& event )
{
	m_pingUtil.Send();
}

void CSSFrame::OnDocumentClosed( wxAuiNotebookEvent& event )
{
	CSSDocument* document = static_cast< CSSDocument* >( event.GetEventObject() );

	document->GetFile()->SetIgnoreChanges( false );

	event.Skip();
}

void CSSFrame::OnKeyDown( wxKeyEvent& event )
{
	if( m_fileTabWithFocus != m_fileTabs )
	{
		if( event.GetKeyCode() == 'F' )
		{
			if( event.GetModifiers() & wxMOD_CONTROL )
			{
				FindOpenWindow( ( event.GetModifiers() & wxMOD_SHIFT ) != 0 );
			}
		}
		else if( event.GetKeyCode() == WXK_F3 )
		{
			if( event.GetModifiers() == wxMOD_CONTROL )
			{
				FindPrev();
			}
			else
			{
				FindNext();
			}
		}
		else if( event.GetKeyCode() == 'G' )
		{
			if( event.GetModifiers() == wxMOD_CONTROL )
			{
				Goto();
			}
		}
		else if( event.GetKeyCode() == 'S' )
		{
			if( event.GetModifiers() & wxMOD_CONTROL )
			{
				if( event.GetModifiers() & wxMOD_SHIFT )
				{
					SaveAll();
				}
				else
				{
					Save();
				}
			}
		}
	}

	// Don't eat
	event.Skip();
}

void CSSFrame::FindOpenWindow( bool findAll )
{
	wxFrame* parent = ( m_fileTabWithFocus == m_fileTabs )? this : static_cast< wxFrame* >( m_fileTabWithFocus->GetParent() );
	
	// Reparenting modal dialogs is bad apparently, so I'm doing this instead
	if( m_searchDialog && m_searchDialog->GetParent() != parent )
	{
		delete m_searchDialog;
		m_searchDialog = NULL;
	}

	if ( m_searchDialog == NULL )
	{
		m_searchDialog = new CSSSearchDialog( parent, m_solution );

		m_searchDialog->LoadHistory( m_solutionExplorer->GetLayoutConfig() );
	}

	// Make autofill an option later
	CSSDocument * doc = GetCurrentDocument();
	if ( doc != NULL )
	{
		int sel_start = doc->GetSelectionStart();
		int sel_end   = doc->GetSelectionEnd();
		if ( sel_start!= sel_end )
		{
			m_searchDialog->SetTextToFind( doc->GetSelectedText() );

#if 0
			doc->WordLeft();
			doc->WordRightExtend();
			wxString sel_text = doc->GetSelectedText();
			sel_text.Trim();
			doc->SetSelectionEnd( doc->GetSelectionStart() + sel_text.Len() );
			m_searchDialog->SetTextToFind( sel_text );
#endif
		}
		else
		{
			m_searchDialog->SetTextToFind( wxEmptyString );
		}
	}
	else
	{
		m_searchDialog->SetTextToFind( wxEmptyString );
	}

	m_searchDialog->SetFindAll( findAll );
	m_searchDialog->Show();
	m_searchDialog->SetFocus();
}

void CSSFrame::FindNext()
{
	if ( !m_searchDialog )
		return;

	m_searchDialog->FindNext();
}

void CSSFrame::FindPrev()
{
	if ( !m_searchDialog )
		return;

	m_searchDialog->FindPrev();
}

void CSSFrame::Save()
{
	CSSDocument* doc = GetCurrentDocument();
	if ( doc )
	{
		if( !doc->Save() )
		{
			wxMessageBox( ( wxT( "Could not save file: " ) + doc->GetFile()->m_solutionPath ).c_str(), wxT( "Error" ), wxOK | wxICON_ERROR );
		}

		UpdateButtonStates();
	}
}

void CSSFrame::SaveAll()
{
	bool failedToSave = false;
	wxString errorMessage = wxT( "There was a problem attempting to save some files:" );

	vector< CSSDocument* > docs;
	GetDocuments( docs );
	for ( size_t i = 0; i < docs.size(); ++i )
	{
		CSSDocument* doc = docs[i];
		if ( doc->GetModify() )
		{
			if( !doc->Save() )
			{
				failedToSave = true;
				errorMessage += wxT( "\n" );
				errorMessage += doc->GetFile()->m_solutionPath.c_str();
			}
		}
	}

	if( failedToSave )
	{
		wxMessageBox( errorMessage, wxT( "Error" ), wxOK | wxICON_ERROR );
	}

	UpdateButtonStates();
}

void CSSFrame::Goto()
{
	CSSDocument* doc = GetCurrentDocument();

	if( doc )
	{
		CSSGotoDialog* gotoDialog = new CSSGotoDialog( doc, doc->GetFile()->m_solutionPath.c_str() );

		gotoDialog->Bind( ssEVT_GOTO_EVENT, &CSSFrame::OnGoto, this );
		gotoDialog->SetMaxLines( doc->GetLineCount() );
		gotoDialog->ShowModal();

		gotoDialog->Destroy();
	}
}

void CSSFrame::SetStatusBarText( const wxChar* format, ... )
{
	va_list arglist;
	va_start( arglist, format );

	wxString text;
	text.PrintfV( format, arglist );
	SetStatusBarText( text );

	va_end( arglist );
}

void CSSFrame::ReloadAllOpenDocumentStyles()
{
	vector< CSSDocument* > documents;
	GetDocuments( documents );

	for( unsigned int i = 0; i < documents.size(); ++i )
	{
		documents[ i ]->ReloadStyling();
	}
}

void CSSFrame::ReloadScripts()
{
	// Save all files before a compilation
	SaveAll();

	bool overridePath = false;

	if( m_solution->GetType() == Solution_Mod )
	{
		EModCompilationAction action = m_rememberedCompilationAction;

		if( action == EModCompilationAction::Invalid )
		{
			CSSCompileModDialog* dialog = new CSSCompileModDialog( this );

			dialog->ShowModal();

			if( dialog->GetCancelled() )
				return;

			action = dialog->GetAction();
		}

		switch( action )
		{
		case EModCompilationAction::Install:
			InstallMod();
			break;

		case EModCompilationAction::UseWorkspace:
			overridePath = true;
			break;
		}
	}

	Red::Network::ChannelPacket packet( RED_NET_CHANNEL_SCRIPT );

	packet.WriteString( "reload" );

	packet.Write( overridePath );

	if( overridePath )
	{
		//This assumes the workspace is one directory above the source/local directories
		wxFileName workspace = ( m_solution->GetPath() + TXT( "\\" ) ).c_str();
		workspace.RemoveLastDir();

		packet.WriteString( workspace.GetFullPath().wx_str() );
	}

	Red::Network::Manager::GetInstance()->Send( RED_NET_CHANNEL_SCRIPT, packet );
}

void CSSFrame::OnConnectionEvent( CConnectionEvent& event )
{
	m_connectionStatus = event.GetStatus();

	switch( m_connectionStatus )
	{
	case ConnStatus_JustConnected:
		{
			wxChoice* choice = XRCCTRL( *this, "AddressHistory", wxChoice );
			wxString addr = choice->GetStringSelection();

			m_compilationHelper.Initialize( addr.wx_str() );
			m_debuggerHelper.Initialize( addr.wx_str() );
			m_profilerHelper.Initialize( addr.wx_str() );

			for( CSSDebuggerTabBase* panel : m_debuggerPanels )
			{
				panel->Connect( addr.wx_str() );
			}

			m_pingUtil.ConnectTo( addr.wx_str(), wxTheSSApp->GetNetworkPort() );

			// Make sure that all settings are synchronised with the editor
			m_breakpoints->ConfirmAll();

			wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
			m_debuggerHelper.SetLocalsUnfiltered( bar->GetToolState( XRCID( "unfilteredLocals" ) ) );
			m_debuggerHelper.SetLocalsSorted( bar->GetToolState( XRCID( "sortLocals" ) ) );
		}
		break;

	case ConnStatus_JustDropped:
	case ConnStatus_Disconnected:
		{
			m_isProfiling = false;
			m_isContinuousProfiling = false;
			DisableDebugging();
		}
	}

	if( m_previousConnectionStatus != m_connectionStatus )
	{
		UpdateButtonStates();
		m_previousConnectionStatus = m_connectionStatus;
	}

	m_connectionStatus = m_previousConnectionStatus;
}

void CSSFrame::OnCompilationStartedEvent( CCompilationStartedEvent& event )
{
	// Switch to error tabs
	m_debuggerTabs->SetSelection( m_debuggerTabs->GetPageIndex( m_errorList ) );

	wxChar* strictMode = ( event.IsStrictModeEnabled() )? wxT( "On" ) : wxT( "Off" );
	wxChar* configuration = ( event.IsFinalBuild() )? wxT( "Final" ) : wxT( "Debug" );

	wxString message;
	message.Printf( wxT("------ Build started: Strict Mode: %s, Configuration: %s ------\n"), strictMode, configuration );

	// Clear the error list
	m_errorList->Clear();
	m_errorList->AddLine( message );

	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bar->EnableTool( XRCID( "debugReloadScripts" ), false );
}

void CSSFrame::OnCompilationEndedEvent( CCompilationEndedEvent& event )
{
	// Switch to error tabs
	m_debuggerTabs->SetSelection( m_debuggerTabs->GetPageIndex( m_errorList ) );

	// Collate all warnings and errors
	m_errorList->AddLine( wxT( "\n" ) );
	m_errorList->List( CSSErrorList::Warning );
	m_errorList->AddLine( wxT( "\n" ) );
	m_errorList->List( CSSErrorList::Error );

	// Get error count
	const int numErrors = m_errorList->GetNumErrors();
	const int numWarnings = m_errorList->GetNumWarnings();
	
	const wxChar* status = nullptr;

	if( numErrors > 0 )
	{
		status = wxT( "Failure" );
	}
	else if( numWarnings > 0 )
	{
		status = wxT( "Acceptable" );
	}
	else
	{
		status = wxT( "Success" );
	}

	// Format message
	wxChar* errorPlural		= ( numErrors != 1 )?	wxT( "s" ) : wxEmptyString;
	wxChar* warningPlural	= ( numWarnings != 1 )?	wxT( "s" ) : wxEmptyString;

	wxString message;
	message.Printf( wxT("\n========== %s - %i error%s, %i warning%s ========== \r\n"), status, numErrors, errorPlural, numWarnings, warningPlural );
	m_errorList->AddLine( message );

	if ( event.WasBuildSuccessful() )
	{
		m_breakpoints->ConfirmAll();
	}
	
	wxToolBar* bar = XRCCTRL( *this, "ToolBar", wxToolBar );
	bar->EnableTool( XRCID( "debugReloadScripts" ), true );
}

void CSSFrame::OnCompilationLogEvent( CCompilationLogEvent& event )
{
	// Display log message
	m_errorList->AddLine( event.GetMessage() + wxT( "\n" ) );
}

void CSSFrame::OnCompilationErrorEvent( CCompilationErrorEvent& event )
{
	CSSErrorList::EType type = ( event.GetSeverity() == CCompilationErrorEvent::Severity_Error )? CSSErrorList::Error : CSSErrorList::Warning;

	m_errorList->AddLine( type, event.GetFile(), event.GetLine(), event.GetMessage() );
}

void CSSFrame::ClearActiveCallstackFrameMarker()
{
	SolutionFilePtr file = m_solution->FindFile( m_fileWithActiveCallstackFrame.wc_str() );

	if( file && file->m_document )
	{
		file->m_document->SetCallstackMarker( 0, -1 );
		file->m_document->SetCallstackMarker( 1, -1 );
	}
}

void CSSFrame::OnCallstackFrameSelected( CCallstackFrameSelectedEvent& event )
{
	OnGoto( event );

	ClearActiveCallstackFrameMarker();

	{
		SolutionFilePtr file = m_solution->FindFile( event.GetFile().wc_str() );

		if( file && file->m_document )
		{
			int line = static_cast< int >( event.GetLine() );
			const int markerType = line > 0 ? 1 : 0;
			file->m_document->SetCallstackMarker( markerType, line );
		}

		m_fileWithActiveCallstackFrame = event.GetFile();
	}

	m_isDebugging = true;
	UpdateButtonStates();
}

void CSSFrame::OnGoto( CGotoEvent& event )
{
	if( OpenFileAndGotoLine( event.GetFile(), event.GetLine() ) )
	{
		if( event.GenerateHistory() )
		{
			GetCurrentDocument()->GenerateNavigationHistoryEvent();
		}
	}
}

void CSSFrame::OnBreakpointToggleConfirmationEvent( CBreakpointToggleConfirmationEvent& event )
{
	wxString filePath = event.GetFile();

	SolutionFilePtr file = m_solution->FindFile( filePath.wc_str() );

	if( file && file->m_document )
	{
		file->m_document->ConfirmBreakpointToggle( event.GetLine(), event.ToggleConfirmed() );
	}
}

void CSSFrame::OnBreakpointHitEvent( CBreakpointHitEvent& event )
{
	if( !IsActive() )
	{
		if( ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_FLASH_STARTBAR_ICON, true ) )
		{
			RequestUserAttention();
		}

		if( ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_WINDOW_TO_FRONT, true ) )
		{
			ShowWithoutActivating();
		}
	}

	Breakpoint( event.GetFile(), event.GetLine() );
}

void CSSFrame::OnOpcodeListingEvent( CSSOpcodeListingEvent& event )
{
	using Red::System::Uint32;

	const vector< SOpcodeFunction >& functions = event.GetFunctions();

	for( Uint32 i = 0; i < functions.size(); ++i )
	{
		const SOpcodeFunction& func = functions[ i ];

		SolutionFilePtr solutionFile = m_solution->FindFile( func.m_file.wx_str() );

		if( solutionFile && solutionFile->m_document )
		{
			solutionFile->m_document->ShowOpcodes( func );
		}
	}
}

void CSSFrame::OnProfilerStartedEvent( CProfilerStartedEvent& event )
{
	if( event.IsContinuous() )
	{
		m_isContinuousProfiling = true;
	}
	else
	{
		m_isProfiling = true;
	}

	UpdateButtonStates();
}

void CSSFrame::OnProfileReportEvent( CProfilerReportEvent& event )
{
	// Create profiler
	CSSProfiler* profiler = new CSSProfiler( m_fileTabs );
	
	profiler->SetData( event.GetReport(), event.GetSize() );
	
	m_fileTabs->AddPage( profiler, wxT( "Profiling Report" ), true );

	m_isProfiling = false;
	m_isContinuousProfiling = false;

	UpdateButtonStates();
}

void CSSFrame::OnPingReceived( CPingEvent& event )
{
	wxString pingMessage;
	
	pingMessage.Printf( wxT( "Ping received: %.3lfms\n" ), event.GetPing() );

	m_errorList->AddLine( pingMessage );
}

void CSSFrame::OnBreakpointToggled( CBreakpointToggledEvent& event )
{
	const SolutionFilePtr& file = event.GetFile();
	int line = event.GetLine();
	EMarkerState state = event.GetState();

	RED_ASSERT( file, TXT( "Invalid solution file in breakpoint toggled event" ) );

	if( file->m_document )
	{
		file->m_document->SetBreakpoint( line, state );
	}

	wxFileName filePath = file->m_solutionPath.c_str();

	m_breakpoints->Set( file, line, state );
	m_debuggerHelper.ToggleBreakpoint( filePath.GetFullPath(), line, state == Marker_Enabled );

	if( ReadOption( CONFIG_BREAKPOINT_OPTIONS_PATH, CONFIG_AUTO_SAVING, false ) )
	{
		SaveBreakpoints();
	}
}

void CSSFrame::OnBookmarkToggled( CBookmarkToggledEvent& event )
{
	const SolutionFilePtr& file = event.GetFile();
	int line = event.GetLine();
	EMarkerState state = event.GetState();

	if( file->m_document )
	{
		file->m_document->SetBookmark( line, state );
	}

	m_bookmarks->Set( file, line, state );
}

void CSSFrame::OnShowVersionControlCredentials( wxCommandEvent& event )
{
	CSSVersionControlCredentialsDialog* dialog = new CSSVersionControlCredentialsDialog( this );

	dialog->Bind( ssEVT_CREDENTIALS_CHANGED_EVENT, &CSSFrame::OnCloseVersionControlCredentials, this );
	dialog->Show();
}

void CSSFrame::OnCloseVersionControlCredentials( CCredentialsChangedEvent& event )
{
	m_solution->CheckFilesStatus();
	m_solutionExplorer->UpdateIcons();

	wxTheSSApp->SaveVersionControlCredentials( event.GetCredentials() );

	event.Skip();
}

void CSSFrame::OnLineMove( CLineMoveEvent& event )
{
	m_breakpoints->Move( event.GetFile(), event.GetLine(), event.GetAdded() );
}

void CSSFrame::OnNavigateBackward( wxCommandEvent& )
{
	m_navigationHistory->GotoPrevious();
}

void CSSFrame::OnNavigateForward( wxCommandEvent& )
{
	m_navigationHistory->GotoNext();
}

void CSSFrame::OnNavigationGoto( CNavigationGotoEvent& event )
{
	if( event.IsForwards() )
	{
		m_navigationHistory->GotoNext();
	}
	else
	{
		m_navigationHistory->GotoPrevious();
	}
}
