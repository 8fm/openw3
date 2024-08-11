/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "solutionExplorer.h"
#include "bookmarks.h"
#include "checkInDialog.h"
#include "openSolutionFileDlg.h"
#include "frame.h"
#include "documentView.h"
#include <wx/progdlg.h>

#include "solution/slnContainer.h"
#include "solution/dir.h"

enum ESolutionId
{
	SolId_Createfile = wxID_HIGHEST,
	SolId_AddDirectory,

	SolId_OpenContainingFolder,
	SolId_RescanDirectory,

	SolId_GetLatest,
	SolId_Add,
	SolId_Checkout,
	SolId_Checkin,
	SolId_Revert,
	SolId_Diff,
	SolId_Delete,
};

wxIMPLEMENT_CLASS( CSSSolutionExplorer, wxGenericTreeCtrl );

BEGIN_EVENT_TABLE( CSSSolutionExplorer, wxGenericTreeCtrl )
	EVT_TREE_ITEM_ACTIVATED( wxID_ANY, CSSSolutionExplorer::OnOpenFile )
	EVT_TREE_ITEM_MENU( wxID_ANY, CSSSolutionExplorer::OnContextMenu )
END_EVENT_TABLE()

class CSSSolutionItemData : public wxTreeItemData
{
public:
	enum EType
	{
		SolType_File = 0,
		SolType_Directory,

		SolType_Max
	};

public:
	CSSSolutionItemData( EType itemType )
	:	m_type( itemType )
	{

	}

	inline EType GetType() const { return m_type; }

protected:
	EType m_type;
};

class CSSSolutionDirectoryData : public CSSSolutionItemData
{
public:
	SolutionDir* m_directory;

public:
	CSSSolutionDirectoryData( SolutionDir* dir )
	:	CSSSolutionItemData( CSSSolutionItemData::SolType_Directory )
	,	m_directory( dir )
	{

	}
};

class CSSSolutionFileData : public CSSSolutionItemData
{
public:
	SolutionFilePtr m_file;

public:
	CSSSolutionFileData( const SolutionFilePtr& file )
	:	CSSSolutionItemData( CSSSolutionItemData::SolType_File )
	,	m_file( file )
	{

	}
};

CSSSolutionExplorer::CSSSolutionExplorer( wxWindow* parent, Solution* solution )
	: wxGenericTreeCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_MULTIPLE | wxTR_HIDE_ROOT )
	, m_layout( wxEmptyString, wxEmptyString, wxT("ScriptStudioLayout.ini"), wxEmptyString, wxCONFIG_USE_LOCAL_FILE )
	, m_solution( solution )
{
	// Create image list
	wxImageList* images = new wxImageList( 9, 9, true, 2 );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_EXPAND" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_EXPAND" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_COLLAPSE" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_COLLAPSE" ) ) );
	AssignButtonsImageList( images );

	AssignImageList( wxTheSSApp->CreateSolutionImageList() );
}

CSSSolutionExplorer::~CSSSolutionExplorer()
{
}

void CSSSolutionExplorer::SaveExpandedNodes( wxTreeItemId item, const wxString& path )
{
	if ( IsExpanded( item ) )
	{
		// Save state
		m_layout.SetPath( path );
		m_layout.Write( wxT("IsExpanded"), true );

		// Recurse
		wxTreeItemIdValue cookie;
		wxTreeItemId cur = GetFirstChild( item, cookie );
		while ( cur.IsOk() )
		{
			wxString caption = GetItemText( cur );
			SaveExpandedNodes( cur, path + wxT("/") + caption  );
			cur = GetNextChild( cur, cookie );
		}
	}
}

void CSSSolutionExplorer::LoadExpandedNodes( wxTreeItemId item, const wxString& path )
{
	// Load state
	m_layout.SetPath( path );
	bool isExpanded = false;
	m_layout.Read( wxT("IsExpanded"), &isExpanded );

	// Expand node
	if ( isExpanded )
	{
		// Expand this node
		Expand( item );

		// Recurse
		wxTreeItemIdValue cookie;
		wxTreeItemId cur = GetFirstChild( item, cookie );
		while ( cur.IsOk() )
		{
			wxString caption = GetItemText( cur );
			LoadExpandedNodes( cur, path + wxT("/") + caption  );
			cur = GetNextChild( cur, cookie );
		}
	}
}

wxString CSSSolutionExplorer::ItemToPath( wxTreeItemId item )
{
	wxString path;
	while ( item.IsOk() )
	{
		if ( path.IsEmpty() )
		{
			path = GetItemText( item );
		}
		else
		{
			path = GetItemText( item ) + wxString( wxT("/") ) + path;
		}

		item = GetItemParent( item );
	}

	return path;
}

void CSSSolutionExplorer::SaveLayout()
{
	// Remove all state info
	m_layout.DeleteAll();

	// Get opened files
	vector< CSSDocument* > docs;
	wxTheFrame->GetDocuments( docs);

	// Save Floating Frame positions

	std::vector< wxFrame* > openFrames;

	for ( unsigned int i = 0; i < docs.size(); ++i )
	{
		CSSDocumentEx* doc = docs[i]->GetFile()->m_documentEx;

		if( doc )
		{
			wxFrame* parentFrame = wxDynamicCast( doc->GetGrandParent(), wxFrame );

			wxString parentFrameName;

			if( parentFrame )
			{
				// Items in the main frame are inside a sub-panel, whereas the frame itself is the parent for a floating code frame

				vector< wxFrame* >::iterator iter = find( openFrames.begin(), openFrames.end(), parentFrame );

				if( iter == openFrames.end() )
				{
					wxString framePath;
					framePath.Printf( wxT( "CodeFrame%u" ), openFrames.size() );
					m_layout.SetPath( wxT( "/" ) + framePath );

					wxSize frameSize = parentFrame->GetSize();
					wxPoint framePos = parentFrame->GetPosition();

					m_layout.Write( wxT( "sizeX" ), frameSize.GetX() );
					m_layout.Write( wxT( "sizeY" ), frameSize.GetY() );
					m_layout.Write( wxT( "positionX" ), framePos.x );
					m_layout.Write( wxT( "positionY" ), framePos.y );

					openFrames.push_back( parentFrame );

					parentFrameName = framePath;
				}
				else
				{
					int frameIndex = iter - openFrames.begin();
					parentFrameName.Printf( wxT( "CodeFrame%i" ), frameIndex );
				}
			}
			else
			{
				parentFrameName = wxT( "Main" );
			}

			// Write associated frame to doc entry
			wxString groupName;
			groupName.Printf( wxT("/OpenedFile%u"), i );
			m_layout.SetPath( groupName );

			m_layout.Write( wxT( "ParentFrame" ), parentFrameName );
		}
	}

	m_layout.SetPath( wxT( "/CodeFrames" ) );
	m_layout.Write( wxT( "NumFrames" ), (unsigned long)openFrames.size() );

	// Save expanded nodes
	wxTreeItemId id = GetRootItem();
	SaveExpandedNodes( id, wxT("/Solution") );

	// Save other
	m_layout.SetPath( wxT("/Solution") );

	// Save first visible node
	wxTreeItemId firstVisible = GetFirstVisibleItem();
	m_layout.Write( wxT("FirstVisible"), ItemToPath( firstVisible ) );

	// Save selected node
	wxTreeItemId selectedNode = GetFocusedItem();
	m_layout.Write( wxT("Selected"), ItemToPath( selectedNode ) );

	// Save number of opened files
	m_layout.Write( wxT("NumOpened"), (int)docs.size() );

	// Save opened files
	for ( unsigned int i = 0; i < docs.size(); ++i )
	{
		// Format group name
		wxString groupName;
		groupName.Printf( wxT("/OpenedFile%u"), i );
		m_layout.SetPath( groupName );

		// Save file config
		CSSDocument* doc = docs[i];
		m_layout.Write( wxT("File"), doc->GetFile()->m_solutionPath.c_str() ); 

		// Is the current one
		bool isCurrent = docs[i] == wxTheFrame->GetCurrentDocument();
		m_layout.Write( wxT("IsSelected"), isCurrent ); 
		
		// Save the number of first visible line
		m_layout.Write( wxT("FirstVisibleLine"), doc->GetFirstVisibleLine() );

		// Save cursor position
		int line = doc->LineFromPosition( doc->GetCurrentPos() );
		int lineStart = doc->PositionFromLine( line );
		m_layout.Write( wxT("CursorX"), doc->GetCurrentPos() - lineStart );
		m_layout.Write( wxT("CursorY"), line );
	}

	// Flush
	m_layout.Flush();
}

void CSSSolutionExplorer::RestoreLayout( bool openFiles )
{
	m_layout.SetPath( wxT( "/CodeFrames" ) );

	int numFrames = 0;
	m_layout.Read( wxT( "NumFrames" ), &numFrames );

	map< wxString, int > frameIds;

	for( int i = 0; i < numFrames; ++i )
	{
		wxString framePath;
		framePath.Printf( wxT( "CodeFrame%i" ), i );
		m_layout.SetPath( wxT( "/" ) + framePath );

		int w = m_layout.ReadLong( "sizeX", 0 );
		int h = m_layout.ReadLong( "sizeY", 0 );
		int x = m_layout.ReadLong( "positionX", 0 );
		int y = m_layout.ReadLong( "positionY", 0 );

		int id = wxTheFrame->CreateNewCodeFrame( x, y, h, w );

		frameIds[ framePath ] = id;
	}

	// Load expanded nodes
	wxTreeItemId id = GetRootItem();
	LoadExpandedNodes( id, wxT("/Solution") );

	// Open initial files
	m_layout.SetPath( wxT("/Solution") );
	if ( openFiles )
	{
		// Get the number of opened files
		int numFiles = 0;
		m_layout.Read( wxT( "NumOpened" ), &numFiles );

		// Open files
		for ( int i = 0; i < numFiles; ++i )
		{
			// Format group name
			wxString groupName;
			groupName.Printf( wxT( "/OpenedFile%i" ), i );
			m_layout.SetPath( groupName );

			// Get path of opened file
			wxString filePath;
			m_layout.Read( wxT( "File" ), &filePath );

			// Find file
			SolutionFilePtr file = m_solution->FindFile( filePath.wc_str() );
			if ( file )
			{
				bool select = false;
				int cursorX = 0, cursorY = 0, firstLine = 0;
				m_layout.Read( wxT( "CursorX" ), &cursorX );
				m_layout.Read( wxT( "CursorY" ), &cursorY );
				m_layout.Read( wxT( "FirstVisibleLine" ), &firstLine );
				m_layout.Read( wxT( "IsSelected" ), &select );

				wxString parentFrame;
				m_layout.Read( wxT( "ParentFrame" ), &parentFrame, wxT( "Main" ) );

				int frameID = -1;
				if( parentFrame != wxT( "Main" ) )
				{
					frameID = frameIds[ parentFrame ];
				}

				wxTheFrame->OpenFileAndGotoLine( file, select, firstLine, frameID );
			}
		}
	}
}

ESolutionImage CSSSolutionExplorer::GetFileImage( const SolutionFilePtr& file )
{
	if ( file->CanModify() )
	{
		if ( file->IsInDepot() )
		{
			if( file->IsAdded() )
			{
				if ( file->IsModified() )
				{
					return SOLIMG_FileAddModified;
				}
				else
				{
					return SOLIMG_FileAdd;
				}
			}
			else
			{
				if ( file->IsModified() )
				{
					if( file->IsOutOfDate() )
					{
						return SOLIMG_FileModifiedOutOfDate;
					}
					else
					{
						return SOLIMG_FileModified;
					}
				}
				else
				{
					if( file->IsOutOfDate() )
					{
						return SOLIMG_FileNormalOutOfDate;
					}
					else
					{
						return SOLIMG_FileNormal;
					}
				}
			}
		}
		else
		{
			if ( file->IsModified() )
			{
				return SOLIMG_NotInDepotModified;
			}
			else
			{
				return SOLIMG_NotInDepot;
			}
		}
	}
	else
	{
		if( file->IsOutOfDate() )
		{
			return SOLIMG_FileLockedOutOfDate;
		}
		else if( file->IsDeleted() )
		{
			return SOLIMG_Deleted;
		}
		else
		{
			return SOLIMG_FileLocked;
		}
	}
}

void CSSSolutionExplorer::FillDirectory( SolutionDir* dir, wxTreeItemId parent, ESolutionImage collapsedImg, ESolutionImage expandedImg )
{
	wxTreeItemId item = AppendItem( parent, dir->GetName().c_str(), collapsedImg );
	SetItemImage( item, expandedImg, wxTreeItemIcon_Expanded );
	SetItemData( item, new CSSSolutionDirectoryData( dir ) );
	m_dirMapping[ dir ] = item;

	for ( vector< SolutionDir* >::const_iterator childDir = dir->BeginDirectories(); childDir != dir->EndDirectories(); ++childDir )
	{
		FillDirectory( *childDir, item );
	}

	for ( vector< SolutionFilePtr >::const_iterator fileIter = dir->BeginFiles(); fileIter != dir->EndFiles(); ++fileIter )
	{
		const SolutionFilePtr& file = *fileIter;

		wxTreeItemId fileItem = AppendItem( item, file->m_name.c_str(), GetFileImage( file ) );
		SetItemData( fileItem, new CSSSolutionFileData( file ) );
		m_fileMapping[ file ] = fileItem;
	}
}

void CSSSolutionExplorer::RepopulateTree( bool openFiles, bool saveLayout, wxProgressDialog* progressDialog )
{
	// Save layout of the solution tree ( expanded items, selection, etc )
	if ( saveLayout && !m_dirMapping.empty() && !m_fileMapping.empty() )
	{
		SaveLayout();
	}

	// Being update
	Freeze();
	DeleteAllItems();

	// Clear mapping

	if( progressDialog )
	{
		progressDialog->Pulse( wxT( "Clearing Solution Explorer Directory map" ) );
	}

	m_dirMapping.clear();

	if( progressDialog )
	{
		progressDialog->Pulse( wxT( "Clearing Solution Explorer File map" ) );
	}
	m_fileMapping.clear();

	// Start with the root
	wxTreeItemId root = AddRoot( wxT( "Solution" ) );

	// Add elements
	if( progressDialog )
	{
		progressDialog->Pulse( wxT( "Populating Solution Explorer" ) );
	}

	for( int i = 0; i < Solution_Max; ++i )
	{
		SolutionDir* dir = m_solution->GetRoot( static_cast< ESolutionType >( i ) );

		if( dir )
		{
			FillDirectory( dir, root, SOLIMG_Classes, SOLIMG_Classes );
		}
	}

	// Expand root
	Expand( root );
	
	// Load layout
	RestoreLayout( openFiles );

	// Refresh
	Thaw();
	Refresh();

	if( progressDialog )
	{
		progressDialog->Pulse( wxT( "Rebuilding 'Open Solution File' dialog cache" ) );
	}
	RebuildOpenSolutionFileCache();
}

SolutionDir* CSSSolutionExplorer::GetSelectedDir()
{
	wxTreeItemId item = GetFocusedItem();
	if ( item.IsOk() )
	{
		CSSSolutionItemData* data = static_cast< CSSSolutionItemData* >( GetItemData( item ) );

		if( data->GetType() == CSSSolutionItemData::SolType_Directory )
		{
			return static_cast< CSSSolutionDirectoryData* >( data )->m_directory;
		}
	}

	return nullptr;
}

SolutionFilePtr CSSSolutionExplorer::GetSelectedFile()
{
	wxTreeItemId item = GetFocusedItem();
	if ( item.IsOk() )
	{
		CSSSolutionItemData* data = static_cast< CSSSolutionItemData* >( GetItemData( item ) );

		if( data->GetType() == CSSSolutionItemData::SolType_File )
		{
			return static_cast< CSSSolutionFileData* >( data )->m_file;
		}
	}

	return nullptr;
}

void CSSSolutionExplorer::UpdateIcon( SolutionFilePtr file )
{
	const int icon = GetFileImage( file );
	const wxTreeItemId& item = m_fileMapping[ file ];
	
	if ( icon != GetItemImage( item, wxTreeItemIcon_Normal ) )
	{
		SetItemImage( item, icon, wxTreeItemIcon_Normal );
	}
}

void CSSSolutionExplorer::UpdateIcons()
{
	// Update icons for files
	for ( map< SolutionFilePtr, wxTreeItemId >::const_iterator i = m_fileMapping.begin(); i != m_fileMapping.end(); ++i )
	{
		const int icon = GetFileImage( i->first );
		if ( icon != GetItemImage( i->second, wxTreeItemIcon_Normal ) )
		{
			SetItemImage( i->second, icon, wxTreeItemIcon_Normal );
		}
	}
}

int CSSSolutionExplorer::GetSolutionFiles( vector< SolutionFilePtr >& solutionFiles /* out */ )
{
	int result = 0; // the number of solution files

	solutionFiles.clear();

	for ( map< SolutionFilePtr, wxTreeItemId >::iterator i = m_fileMapping.begin(); i != m_fileMapping.end(); ++i, ++result )
	{
		solutionFiles.push_back( i->first );
	}

	return result;
}

void CSSSolutionExplorer::OnOpenFile( wxTreeEvent& event )
{
	CSSSolutionItemData* data = static_cast< CSSSolutionItemData* >( GetItemData( event.GetItem() ) );

	if ( data )
	{
		if( data->GetType() == CSSSolutionItemData::SolType_File )
		{
			CSSSolutionFileData* fileData = static_cast< CSSSolutionFileData* >( data );

			const SolutionFilePtr& file = fileData->m_file;

			if ( file && fileData->GetType() != CSSSolutionItemData::SolType_Directory )
			{
				wxTheFrame->OpenFile( file, true );
				SaveLayout();
			}
		}
	}
}

void CSSSolutionExplorer::OnCreateFile( wxCommandEvent& event )
{
	SolutionDir* dir = GetSelectedDir();
	if ( dir )
	{
		wxTextEntryDialog* inputBox = new wxTextEntryDialog( this, dir->GetAbsolutePath().c_str(), wxT( "Add new file" ) );

		if ( inputBox->ShowModal() == wxID_OK )
		{
			wstring fileName = inputBox->GetValue().wc_str();

			if ( !fileName.empty() )
			{
				// File exists 
				if ( dir->FindFile( fileName ) )
				{
					wxMessageBox( wxT( "File already exists in directory" ), wxT( "Error" ), wxOK | wxICON_ERROR );
					return;
				}

				// Create file
				const wchar_t* extension = TXT( ".ws" );
				const unsigned int extensionLength = Red::System::StringLengthCompileTime( ".ws" );
				if( fileName.length() < extensionLength || fileName.compare( fileName.length() - extensionLength, extensionLength, extension ) != 0 )
				{
					fileName += extension;
				}

				SolutionFilePtr file = dir->CreateFile( fileName );
				if ( file )
				{
					// Create the empty stub on the hard drive
					{
						wxFile writeFile( file->m_absolutePath.c_str(), wxFile::write );
					}

					// Open the file
					wxTheFrame->OpenFile( file, true );
					if ( !file->Save() )
					{
						wxMessageBox( wxT("Unable to save created file"), wxT("Error"), wxOK | wxICON_ERROR );
						return;
					}

					// Update file status
					file->RefreshStatus();

					// Append the file item to the tree
					wxTreeItemId levelItem = AppendItem( m_menuFocusedItem, fileName.c_str(), GetFileImage( file ) );
					if ( levelItem )
					{
						Expand( m_menuFocusedItem );
						SetItemData( levelItem, new CSSSolutionFileData( file ) );
						m_fileMapping[ file ] = levelItem;
						SelectItem( levelItem );
					}
				}
			}
		}
	}

	RebuildOpenSolutionFileCache();

	// Redraw view
	Refresh();
}

void CSSSolutionExplorer::OnCreateDirectory( wxCommandEvent& event )
{
	SolutionDir* dir = GetSelectedDir();
	if ( dir )
	{
		wxTextEntryDialog* inputBox = new wxTextEntryDialog( this, dir->GetAbsolutePath().c_str(), wxT( "Add new directory" ) );

		if ( inputBox->ShowModal() == wxID_OK )
		{
			wstring fileName = inputBox->GetValue().wc_str();
			if ( !fileName.empty() )
			{
				// File exists 
				if ( dir->FindDir( fileName ) )
				{
					wxMessageBox( wxT("Directory already exists in directory"), wxT("Error"), wxOK | wxICON_ERROR );
					return;
				}

				// Create directory
				SolutionDir* subDir = dir->CreateDir( fileName );
				if ( subDir )
				{
					// Append tree item
					wxTreeItemId levelItem = AppendItem( m_menuFocusedItem, fileName.c_str(), SOLIMG_DirClosed );
					if ( levelItem )					
					{
						Expand( m_menuFocusedItem );
						SetItemImage( levelItem, SOLIMG_DirOpened, wxTreeItemIcon_Expanded );
						SetItemData( levelItem, new CSSSolutionDirectoryData( subDir ) );
						
						UnselectAll();
						ClearFocusedItem();

						SelectItem( levelItem );
						SetFocusedItem( levelItem );
					}
				}
			}
		}
	}

	// Redraw view
	Refresh();
}

void CSSSolutionExplorer::OnRescanDir( wxCommandEvent& event )
{
	SolutionDir* dir = GetSelectedDir();
	if ( dir )
	{
		RescanDir( dir );
	}
}

void CSSSolutionExplorer::RescanDir( SolutionDir* dir )
{
	RED_ASSERT( dir, TXT( "Invalid Solution directory specified" ) );

	wxProgressDialog* progressDialog = new wxProgressDialog( wxT( "Repopulating Solution Explorer" ), wxEmptyString );
	progressDialog->Show();

	progressDialog->Pulse( wxT( "Scanning directory structure" ) );
	dir->Scan();

	if ( wxTheFrame )
	{
		wxTheFrame->ValidateOpenedFiles();
	}

	progressDialog->Pulse( wxT( "Updating source control/file status" ) );
	m_solution->CheckFilesStatus();
	RepopulateTree( false, false, progressDialog );

	progressDialog->Destroy();
}

void CSSSolutionExplorer::GetSelectedFilesInternal( vector< SolutionFilePtr >& files, wxTreeItemId id, bool expandDirectories, VersionControl::FileStatus* includeFilter, VersionControl::FileStatus* excludeFilter )
{
	CSSSolutionItemData* data = static_cast< CSSSolutionItemData* >( GetItemData( id ) );

	if( data->GetType() == CSSSolutionItemData::SolType_File )
	{
		const SolutionFilePtr& file = static_cast< CSSSolutionFileData* >( data )->m_file;

		if( ( !includeFilter || ( file->GetStatus() & *includeFilter ) ) && ( !excludeFilter || !( file->GetStatus() & *excludeFilter ) ) )
		{
			files.push_back( file );
		}
	}
	else if( data->GetType() == CSSSolutionItemData::SolType_Directory && expandDirectories )
	{
		SolutionDir* dir = static_cast< CSSSolutionDirectoryData* >( data )->m_directory;

		dir->GetFiles( files );
	}
}

void CSSSolutionExplorer::GetSelectedFiles( vector< SolutionFilePtr >& files, bool expandDirectories, VersionControl::FileStatus* includeFilter, VersionControl::FileStatus* excludeFilter )
{
	wxArrayTreeItemIds selectedItems;
	size_t numberOfSelectedItems = GetSelections( selectedItems );

	for( size_t i = 0; i < numberOfSelectedItems; ++i )
	{
		GetSelectedFilesInternal( files, selectedItems[ i ], expandDirectories, includeFilter, excludeFilter );
	}
}

void CSSSolutionExplorer::ToFileList( const vector< SolutionFilePtr >& files_in, VersionControl::Filelist* files_out )
{
	for( size_t i = 0; i < files_in.size(); ++i )
	{
		wxString intermediary = files_in[ i ]->m_absolutePath.c_str();

		files_out->Add( intermediary.mb_str() );
	}
}

void CSSSolutionExplorer::OnAdd( wxCommandEvent& event )
{
	VersionControl::FileStatus excludeFilter;
	excludeFilter.SetFlag( VersionControl::VCSF_InDepot );

	vector< SolutionFilePtr > files;
	GetSelectedFiles( files, true, nullptr, &excludeFilter );

	if( files.size() > 0 )
	{
		VersionControl::Filelist* fileList = wxTheSSApp->GetVersionControl()->CreateFileList();
		ToFileList( files, fileList );

		if( !wxTheSSApp->GetVersionControl()->Add( fileList ) )
		{
			wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

			wxMessageBox( errorMsg, wxT( "Add failed" ), wxOK | wxICON_WARNING );
		}

		wxTheSSApp->GetVersionControl()->DestroyFileList( fileList );
	}

	for( size_t i = 0; i < files.size(); ++i )
	{
		files[ i ]->RefreshStatus();
	}

	UpdateIcons();
}

void CSSSolutionExplorer::OnCheckOut( wxCommandEvent& event )
{
	VersionControl::FileStatus includeFilter;
	includeFilter.SetFlag( VersionControl::VCSF_InDepot );

	vector< SolutionFilePtr > files;
	GetSelectedFiles( files, true, &includeFilter );

	if( files.size() > 0 )
	{
		VersionControl::Filelist* fileList = wxTheSSApp->GetVersionControl()->CreateFileList();
		ToFileList( files, fileList );

		if( !wxTheSSApp->GetVersionControl()->Checkout( fileList ) )
		{
			wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

			wxMessageBox( errorMsg, wxT( "Check out failed" ), wxOK | wxICON_WARNING );
		}

		wxTheSSApp->GetVersionControl()->DestroyFileList( fileList );
	}

	for( size_t i = 0; i < files.size(); ++i )
	{
		files[ i ]->RefreshStatus();
	}

	UpdateIcons();
}

void CSSSolutionExplorer::OnCheckIn( wxCommandEvent& event )
{
	VersionControl::FileStatus includeFilter;
	includeFilter.SetFlag( VersionControl::VCSF_CheckedOut );
	includeFilter.SetFlag( VersionControl::VCSF_Deleted );
	includeFilter.SetFlag( VersionControl::VCSF_Added );

	vector< SolutionFilePtr > files;
	GetSelectedFiles( files, true, &includeFilter );

	CSSCheckinDialog* dialog = new CSSCheckinDialog( this, m_previousSubmitDescription );
	dialog->Bind( ssEVT_CHECKIN_EVENT, &CSSSolutionExplorer::OnCheckInConfirmed, this );

	for( size_t i = 0; i < files.size(); ++i )
	{
		dialog->AddFile( files[ i ],  GetFileImage( files[ i ] ) );
	}

	dialog->Show();
}

void CSSSolutionExplorer::OnCheckInConfirmed( CCheckInEvent& event )
{
	// Organise file paths into an array of string pointers for the version control interface
	const wxString& description = event.GetDescription();
	const vector< SolutionFilePtr > files = event.GetFiles();

	unsigned int numberOfFilesToSubmit = files.size();

	for( size_t i = 0; i < numberOfFilesToSubmit; ++i )
	{
		if( files[ i ]->IsModified() )
		{
			files[ i ]->Save();
		}
	}

	if( numberOfFilesToSubmit > 0 )
	{
		vector< wxString > intermediaries;
		intermediaries.resize( numberOfFilesToSubmit );

		const char** fileListForVersionControl = static_cast< const char** >( alloca( sizeof( char* ) * numberOfFilesToSubmit ) );

		for( size_t i = 0; i < numberOfFilesToSubmit; ++i )
		{
			// If the file is not in the depot, add it
			if ( !files[ i ]->IsInDepot() )
			{
				files[ i ]->Add();
			}

			intermediaries[ i ] = wxString( files[ i ]->m_absolutePath.c_str() );

			const char* intermediary = intermediaries[ i ].mb_str();

			fileListForVersionControl[ i ] = intermediary;
		}

		// Submit!
		if( wxTheSSApp->GetVersionControl()->Submit( fileListForVersionControl, numberOfFilesToSubmit, description.char_str() ) )
		{
			// Refresh!
			for( size_t i = 0; i < numberOfFilesToSubmit; ++i )
			{
				// Update file state
				files[ i ]->RefreshStatus();
			}

			// Update icons
			UpdateIcons();

			// Update toolbar
			wxTheFrame->DocumentContentModified( NULL );

			m_previousSubmitDescription = wxEmptyString;
		}
		else
		{
			wxString errorMessage( wxTheSSApp->GetVersionControl()->GetLastError() );
			wxMessageBox( errorMessage, wxT( "Failed to Check in" ), wxOK | wxICON_WARNING );

			m_previousSubmitDescription = description;
		}
	}
}

void CSSSolutionExplorer::OnRevert( wxCommandEvent& event )
{
	if ( wxMessageBox( wxT( "Discard changes to files?" ), wxT( "Warning" ), wxICON_QUESTION | wxOK | wxCANCEL | wxCANCEL_DEFAULT ) == wxOK )
	{
		VersionControl::FileStatus includeFilter;
		includeFilter.SetFlag( VersionControl::VCSF_InDepot );

		vector< SolutionFilePtr > files;
		GetSelectedFiles( files, true, &includeFilter );

		if( files.size() > 0 )
		{
			VersionControl::Filelist* fileList = wxTheSSApp->GetVersionControl()->CreateFileList();
			ToFileList( files, fileList );

			if( wxTheSSApp->GetVersionControl()->Revert( fileList ) )
			{
				for( size_t i = 0; i < files.size(); ++i )
				{
					files[ i ]->RefreshStatus();

					// Reverted
					if ( !files[ i ]->IsCheckedOut() )
					{
						// Reload from disk
						if ( files[ i ]->m_document )
						{
							files[ i ]->m_document->LoadFile();
						}

						// It's not modified any more
						files[ i ]->CancelModified();
					}
				}

				UpdateIcons();
			}
			else
			{
				wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

				wxMessageBox( errorMsg, wxT( "Revert failed" ), wxOK | wxICON_WARNING );
			}

			wxTheSSApp->GetVersionControl()->DestroyFileList( fileList );
		}
	}
}

void CSSSolutionExplorer::OnDiff( wxCommandEvent& event )
{
	SolutionFilePtr file = GetSelectedFile();
	if ( file )
	{
		//GPerforce.Diff( file->m_absolutePath.c_str() );
		if(wxTheFrame)
		{
			wxTheFrame->SetFocus();
		}				
	}
}

void CSSSolutionExplorer::OnGetLatest( wxCommandEvent& event )
{
	VersionControl::FileStatus includeFilter;
	includeFilter.SetFlag( VersionControl::VCSF_OutOfDate );

	VersionControl::FileStatus excludeFilter;
	excludeFilter.SetFlag( VersionControl::VCSF_Added );
	excludeFilter.SetFlag( VersionControl::VCSF_CheckedOut );
	excludeFilter.SetFlag( VersionControl::VCSF_Deleted );

	vector< SolutionFilePtr > files;
	GetSelectedFiles( files, true, &includeFilter, &excludeFilter );

	if( files.size() > 0 )
	{
		VersionControl::Filelist* fileList = wxTheSSApp->GetVersionControl()->CreateFileList();
		ToFileList( files, fileList );

		if( !wxTheSSApp->GetVersionControl()->Sync( fileList ) )
		{
			wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

			wxMessageBox( errorMsg, wxT( "Sync failed" ), wxOK | wxICON_WARNING );
		}

		wxTheSSApp->GetVersionControl()->DestroyFileList( fileList );
	}

	for( size_t i = 0; i < files.size(); ++i )
	{
		if( files[ i ]->m_document )
		{
			files[ i ]->m_document->LoadFile();
		}

		files[ i ]->RefreshStatus();
	}

	UpdateIcons();
}

void CSSSolutionExplorer::OnRemove( wxCommandEvent& event )
{
	if ( wxMessageBox( wxT( "Delete selected files?" ), wxT( "Warning" ), wxICON_QUESTION | wxYES_NO ) == wxYES )
	{
		vector< SolutionDir* > directories;

		// Revert Checked out files
		{
			VersionControl::FileStatus includeFilter;
			includeFilter.SetFlag( VersionControl::VCSF_CheckedOut );

			vector< SolutionFilePtr > files;
			GetSelectedFiles( files, true, &includeFilter );

			VersionControl::Filelist* revertFileList = wxTheSSApp->GetVersionControl()->CreateFileList();
			ToFileList( files, revertFileList );

			if( revertFileList->Size() > 0 )
			{
				if( wxTheSSApp->GetVersionControl()->Revert( revertFileList ) )
				{
					for( size_t i = 0; i < files.size(); ++i )
					{
						files[ i ]->RefreshStatus();
					}
				}
				else
				{
					wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

					wxMessageBox( errorMsg, wxT( "Revert failed" ), wxOK | wxICON_WARNING );
				}
			}

			wxTheSSApp->GetVersionControl()->DestroyFileList( revertFileList );
		}

		// Delete Files under source control
		{
			VersionControl::FileStatus includeFilter;
			includeFilter.SetFlag( VersionControl::VCSF_InDepot );

			vector< SolutionFilePtr > files;
			GetSelectedFiles( files, true, &includeFilter );

			if( files.size() > 0 )
			{
				VersionControl::Filelist* deletionFileList = wxTheSSApp->GetVersionControl()->CreateFileList();

				ToFileList( files, deletionFileList );

				for( size_t i = 0; i < files.size(); ++i )
				{
					// Record directory for later rescan
					if( find( directories.begin(), directories.end(), files[ i ]->m_dir ) == directories.end() )
					{
						directories.push_back( files[ i ]->m_dir );
					}

					// Close the document
					if ( files[ i ]->m_document )
					{
						wxTheFrame->CloseFile( files[ i ], true );
					}

					// Remove markers
					wxTheFrame->GetBookmarks()->RemoveAll( files[ i ] );
				}

				// Finally, mark for deletion
				if( !wxTheSSApp->GetVersionControl()->Delete( deletionFileList ) )
				{
					wxString errorMsg = wxTheSSApp->GetVersionControl()->GetLastError();

					wxMessageBox( errorMsg, wxT( "Delete failed" ), wxOK | wxICON_WARNING );
				}

				for( size_t i = 0; i < files.size(); ++i )
				{
					files[ i ]->RefreshStatus();
				}

				UpdateIcons();

				wxTheSSApp->GetVersionControl()->DestroyFileList( deletionFileList );
			}
		}

		// Delete Local files
		{
			VersionControl::FileStatus excludeFilter;
			excludeFilter.SetFlag( VersionControl::VCSF_InDepot );

			vector< SolutionFilePtr > files;
			GetSelectedFiles( files, true, nullptr, &excludeFilter );

			for( size_t i = 0; i < files.size(); ++i )
			{
				// Record directory for later rescan
				if( find( directories.begin(), directories.end(), files[ i ]->m_dir ) == directories.end() )
				{
					directories.push_back( files[ i ]->m_dir );
				}

				// Close the document
				if ( files[ i ]->m_document )
				{
					wxTheFrame->CloseFile( files[ i ], true );
				}

				// Remove bookmarks
				wxTheFrame->GetBookmarks()->RemoveAll( files[ i ] );

				wxRemoveFile( files[ i ]->m_absolutePath.c_str() );
			}
		}

		// Update the affected directories
		for( size_t i = 0; i < directories.size(); ++i )
		{
			RescanDir( directories[ i ] );
		}
	}
}

void CSSSolutionExplorer::OnOpenContainingFolder( wxCommandEvent& event )
{
	vector< SolutionFilePtr > files;
	GetSelectedFiles( files, false );

	for( size_t i = 0; i < files.size(); ++i )
	{
		wxFileName file( files[ i ]->m_absolutePath.c_str() );

		wxLaunchDefaultBrowser( file.GetPath() );
	}
}

void CSSSolutionExplorer::AddMenuItem( wxMenu* menu, int id, wxChar* label, wxChar* imageName, void (CSSSolutionExplorer::*eventHandler)( wxCommandEvent& ) )
{
	wxBitmap image = wxTheSSApp->LoadBitmap( imageName );

	wxMenuItem* item = new wxMenuItem( menu, id, label );
	item->SetBitmap( image );

	menu->Append( item );

	menu->Bind( wxEVT_COMMAND_MENU_SELECTED, eventHandler, this, id );
}

void CSSSolutionExplorer::OnContextMenu( wxTreeEvent& event )
{
	// Create popup menu
	wxMenu popupMenu;

	m_menuFocusedItem = event.GetItem();

	// Is this a directory
	CSSSolutionItemData* data = static_cast< CSSSolutionItemData* >( GetItemData( m_menuFocusedItem ) );
	if ( data->GetType() == CSSSolutionItemData::SolType_Directory )
	{
		AddMenuItem( &popupMenu, SolId_Createfile, wxT( "Add File..." ), wxT( "IMG_ADD_FILE" ), &CSSSolutionExplorer::OnCreateFile );
		AddMenuItem( &popupMenu, SolId_AddDirectory, wxT( "Add directory..." ), wxT( "IMG_ADD_DIRECTORY" ), &CSSSolutionExplorer::OnCreateDirectory );

		popupMenu.AppendSeparator();

		AddMenuItem( &popupMenu, SolId_RescanDirectory, wxT( "Rescan" ), wxT( "IMG_RESCAN" ), &CSSSolutionExplorer::OnRescanDir );

		popupMenu.AppendSeparator();

		AddMenuItem( &popupMenu, SolId_GetLatest, wxT( "Sync" ), wxT( "IMG_SCC_GETLATEST" ), &CSSSolutionExplorer::OnGetLatest );

		popupMenu.AppendSeparator();

		AddMenuItem( &popupMenu, SolId_Checkout, wxT( "Check out" ), wxT( "IMG_SCC_CHECKOUT" ), &CSSSolutionExplorer::OnCheckOut );
		AddMenuItem( &popupMenu, SolId_Checkin, wxT( "Check in" ), wxT( "IMG_SCC_CHECKIN" ), &CSSSolutionExplorer::OnCheckIn );
		AddMenuItem( &popupMenu, SolId_Revert, wxT( "Revert" ), wxT( "IMG_SCC_REVERT" ), &CSSSolutionExplorer::OnRevert );
	}
	else
	{
		CSSSolutionFileData* fileData = static_cast< CSSSolutionFileData* >( data );
		const SolutionFilePtr file = fileData->m_file;

		if( !file->IgnoreChanges() )
		{
			// Add
			if ( !file->IsInDepot() )
			{
				AddMenuItem( &popupMenu, SolId_Add, wxT( "Add to depot" ), wxT( "IMG_SCC_ADD" ), &CSSSolutionExplorer::OnAdd );
			}
			else if( file->IsDeleted() )
			{
				AddMenuItem( &popupMenu, SolId_Checkin, wxT( "Check in" ), wxT( "IMG_SCC_CHECKIN" ), &CSSSolutionExplorer::OnCheckIn );
				AddMenuItem( &popupMenu, SolId_Revert, wxT( "Revert" ), wxT( "IMG_SCC_REVERT" ), &CSSSolutionExplorer::OnRevert );
			}
			else
			{
				AddMenuItem( &popupMenu, SolId_GetLatest, wxT( "Sync" ), wxT( "IMG_SCC_GETLATEST" ), &CSSSolutionExplorer::OnGetLatest );

				if( file->IsCheckedOut() )
				{
					AddMenuItem( &popupMenu, SolId_Checkin, wxT( "Check in" ), wxT( "IMG_SCC_CHECKIN" ), &CSSSolutionExplorer::OnCheckIn );
					AddMenuItem( &popupMenu, SolId_Revert, wxT( "Revert" ), wxT( "IMG_SCC_REVERT" ), &CSSSolutionExplorer::OnRevert );
					//AddMenuItem( &popupMenu, SolId_Diff, wxT( "Diff" ), wxT( "IMG_SCC_DIFF" ), &CSSSolutionExplorer::OnDiff );
				}
				else
				{
					AddMenuItem( &popupMenu, SolId_Checkout, wxT( "Check out" ), wxT( "IMG_SCC_CHECKOUT" ), &CSSSolutionExplorer::OnCheckOut );
				}
			}

			AddMenuItem( &popupMenu, SolId_OpenContainingFolder, wxT( "Open containing folder" ), wxT( "IMG_SHOW_IN_DIRECTORY" ), &CSSSolutionExplorer::OnOpenContainingFolder );

			popupMenu.AppendSeparator();

			AddMenuItem( &popupMenu, SolId_Delete, wxT( "Delete" ), wxT( "IMG_SCC_REMOVE" ), &CSSSolutionExplorer::OnRemove );
		}
	}

	// Show the menu
	PopupMenu( &popupMenu );
}

void CSSSolutionExplorer::RebuildOpenSolutionFileCache()
{
	vector< SolutionFilePtr > solutionFiles;

	GetSolutionFiles( solutionFiles );
	CSSOpenSolutionFileDialog::BuildCache( solutionFiles );
}
