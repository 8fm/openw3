/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "resourceSorter.h"

#include "../../common/core/depot.h"

const String SCENE_INDICES_TABLE_PATH = TXT( "gameplay\\globals\\scene_sequences.seq" );

CResourceSorter::CResourceSorter()
{
	LoadIndices();
}

void CResourceSorter::SortResourcePaths( TDynArray< String >& paths )
{
	Sort( paths.Begin(), paths.End(), *this );
}

Bool CResourceSorter::CompareResourcePaths( const String& path1, const String& path2 ) const
{
	Uint32 pathIndex1 = -1;
	Uint32 pathIndex2 = -1;

	m_pathIndices.Find( path1, pathIndex1 );
	m_pathIndices.Find( path2, pathIndex2 );

	if ( pathIndex1 == pathIndex2 )
	{
		return path1 < path2;
	}

	return pathIndex1 < pathIndex2;
}

void CResourceSorter::UpdateIndices( const TDynArray< String >& paths )
{
	for ( Uint32 i = 0; i < paths.Size(); ++i )
	{
		VERIFY( m_pathIndices.Set( paths[ i ], i ) );
	}
}

void CResourceSorter::UpdateIndices( const TSortedMap< String, Uint32 >& paths )
{
	for ( auto pathIter = paths.Begin(); pathIter != paths.End(); ++pathIter )
	{
		VERIFY( m_pathIndices.Set( pathIter->m_first, pathIter->m_second ) );
	}
}

void CResourceSorter::LoadIndices()
{
	CDiskFile* indicesTableFile = GDepot->FindFile( SCENE_INDICES_TABLE_PATH );
	if ( indicesTableFile == NULL )
	{
		return;
	}

	indicesTableFile->Sync();
	IFile* indicesTableReader = indicesTableFile->CreateReader();

	if ( indicesTableReader != NULL )
	{
		*indicesTableReader << m_pathIndices;

		delete indicesTableReader;
	}
}

void CResourceSorter::SaveIndices()
{
	// Get or create the file in depot

	CDiskFile* indicesTableFile = GDepot->FindFile( SCENE_INDICES_TABLE_PATH );
	
	Bool fileExists = true;
	if ( indicesTableFile == NULL )
	{
		fileExists = false;
		indicesTableFile = GDepot->CreateNewFile( SCENE_INDICES_TABLE_PATH.AsChar() );
	
		if ( indicesTableFile == NULL )
		{
			ERR_EDITOR( TXT( "Cannot add file '%s' to depot" ), SCENE_INDICES_TABLE_PATH.AsChar() );
			return;
		}
	}

	// If file exists, make sure it's checked out

	if ( fileExists )
	{
		indicesTableFile->GetStatus();
		if ( !indicesTableFile->IsCheckedOut() )
		{
			indicesTableFile->CheckOut();
		}
	}

	// Write to file

	IFile* indicesTableWriter = indicesTableFile->CreateWriter( indicesTableFile->GetAbsolutePath() );
	if ( !indicesTableWriter )
	{
		return;
	}

	*indicesTableWriter << m_pathIndices;
	delete indicesTableWriter;

	// If file didn't exist, add it to changelist

	if ( !fileExists )
	{
		indicesTableFile->GetDirectory()->Repopulate();
		if ( !indicesTableFile->Add() )
		{
			return;
		}
	}

	// Auto-submit changed/added file

	indicesTableFile->Submit();
}

Bool CResourceSorter::operator()( const String& path1, const String& path2 ) const
{
	return CompareResourcePaths( path1, path2 );
}



BEGIN_EVENT_TABLE( CEdResourceSorterFrame, wxSmartLayoutPanel )
	EVT_TREE_SEL_CHANGED( XRCID( "DirectoryTree" ), CEdResourceSorterFrame::OnDirectorySelected )
	EVT_BUTTON( XRCID( "UpButton" ), CEdResourceSorterFrame::OnUpButton )
	EVT_BUTTON( XRCID( "DownButton" ), CEdResourceSorterFrame::OnDownButton )
	EVT_BUTTON( XRCID( "SaveButton" ), CEdResourceSorterFrame::OnSaveButton )
END_EVENT_TABLE()

CEdResourceSorterFrame::CEdResourceSorterFrame( wxWindow* parent )
: wxSmartLayoutPanel( parent, TEXT( "ResourceSorterFrame" ), false )
{
	m_directoryTree		= XRCCTRL( *this, "DirectoryTree",	wxTreeCtrl );
	m_fileList			= XRCCTRL( *this, "FileList",		wxListBox );

	m_currentDirectoryIndex = 1;
	m_fileList->Clear();

	m_directoriesCount = 0;
	AddDirectoryToTree( GDepot, 0 );
}

void CEdResourceSorterFrame::AddDirectoryToTree( CDirectory* directory, wxTreeItemId parentItem )
{
	wxTreeItemId directoryTreeId = 0;

	DirectoryIndexItemWrapper* directoryWrapper = new DirectoryIndexItemWrapper( directory, m_directoriesCount++ );

	if ( parentItem == wxTreeItemId( 0 ) )
	{
		directoryTreeId = m_directoryTree->AddRoot( directory->GetName().AsChar(), 
			0, -1, directoryWrapper );
	}
	else
	{
		directoryTreeId = m_directoryTree->AppendItem( parentItem, directory->GetName().AsChar(), 
			1, -1, directoryWrapper );
	}

	if ( directoryTreeId.IsOk() == false )
	{
		return;
	}

	for ( CDirectory* dir : directory->GetDirectories() )
	{
		AddDirectoryToTree( dir, directoryTreeId );
	}
}

void CEdResourceSorterFrame::ListDirectoryFiles( CDirectory* currentDirectory )
{
	String  resourceExtension = TXT( ".w2scene" );

	const TFiles & directoryFiles = currentDirectory->GetFiles();
	for ( TFiles::const_iterator fileIter = directoryFiles.Begin();
		fileIter != directoryFiles.End(); ++fileIter )
	{
		String filename = (*fileIter)->GetFileName();
		String filepath = (*fileIter)->GetDepotPath();

		if ( filepath.EndsWith( resourceExtension ) == false )
		{
			continue;
		}
		
		m_filepaths.PushBack( filepath );
	}

	for ( CDirectory * dir : currentDirectory->GetDirectories() )
	{
		ListDirectoryFiles( dir );
	}
}

void CEdResourceSorterFrame::OnDirectorySelected( wxTreeEvent& event )
{
	

	wxTreeItemId selectedDirectoryId = m_directoryTree->GetSelection();
	DirectoryIndexItemWrapper* itemDataWrapper 
		= static_cast< DirectoryIndexItemWrapper* >( m_directoryTree->GetItemData( selectedDirectoryId ) );
	CDirectory* currentDirectory = itemDataWrapper->m_directory;

	m_currentDirectoryIndex = itemDataWrapper->m_index;

	m_fileList->Clear();

	if ( currentDirectory == NULL )
	{
		return;
	}

	m_filepaths.Clear();

	ListDirectoryFiles( currentDirectory );

	m_sorter.SortResourcePaths( m_filepaths );

	UpdateFileList();

}

void CEdResourceSorterFrame::OnUpButton( wxCommandEvent& event )
{
	wxArrayInt selectedItems;
	m_fileList->GetSelections( selectedItems );

	for ( Uint32 i = 0; i < selectedItems.GetCount(); ++i )
	{
		Uint32 selectedIndex = selectedItems[ i ];
		if ( selectedIndex == 0 )
		{
			continue;
		}
		m_filepaths.Swap( selectedIndex, selectedIndex - 1 );
		selectedItems[ i ] = selectedIndex - 1;
	}

	UpdateIndices();

	for ( Uint32 i = 0; i < selectedItems.GetCount(); ++i )
	{
		m_fileList->SetSelection( selectedItems[ i ] );
	}

}

void CEdResourceSorterFrame::OnDownButton( wxCommandEvent& event )
{
	wxArrayInt selectedItems;
	m_fileList->GetSelections( selectedItems );

	for ( Uint32 i = 0; i < selectedItems.GetCount(); ++i )
	{
		Uint32 selectedIndex = selectedItems[ i ];
		if ( selectedIndex == m_fileList->GetCount() - 1 )
		{
			continue;
		}
		m_filepaths.Swap( selectedIndex, selectedIndex + 1 );
		selectedItems[ i ] = selectedIndex + 1;
	}

	UpdateIndices();

	for ( Uint32 i = 0; i < selectedItems.GetCount(); ++i )
	{
		m_fileList->SetSelection( selectedItems[ i ] );
	}
}

void CEdResourceSorterFrame::UpdateFileList()
{
	m_fileList->Freeze();
	m_fileList->Clear();
	for( TDynArray< String >::iterator filepathIter = m_filepaths.Begin();
		filepathIter != m_filepaths.End(); ++filepathIter )
	{
		String filename = filepathIter->StringAfter( TXT( "\\" ), true );
		m_fileList->Append( filename.AsChar() );
	}
	m_fileList->Thaw();
}

void CEdResourceSorterFrame::OnSaveButton( wxCommandEvent& event )
{
	m_sorter.SaveIndices();
}

void CEdResourceSorterFrame::UpdateIndices()
{
	TSortedMap< String, Uint32 > fileIndices;
	fileIndices.Reserve( m_filepaths.Size() );
	for ( Uint32 i = 0; i < m_filepaths.Size(); ++i )
	{
		VERIFY( fileIndices.Set( m_filepaths[ i ], m_currentDirectoryIndex * 10000 + i + 1 ) );
	}
	m_sorter.UpdateIndices( fileIndices );

	UpdateFileList();
}