/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CResourceSorter
{
	TSortedMap < String, Uint32 > m_pathIndices;

public:
	CResourceSorter();
	void SortResourcePaths( TDynArray< String >& paths );
	void UpdateIndices( const TDynArray< String >& paths );
	void UpdateIndices( const TSortedMap< String, Uint32 >& paths );
	void SaveIndices();

public:
	Bool operator() ( const String& path1, const String& path2 ) const;

protected:
	Bool CompareResourcePaths( const String& path1, const String& path2 ) const;

	void LoadIndices();
	
};


class DirectoryIndexItemWrapper : public DirectoryItemWrapper
{
public:
	Uint32 m_index;

public:
	DirectoryIndexItemWrapper( CDirectory* directory, Uint32 index )
		: DirectoryItemWrapper( directory )
		, m_index( index )
	{
	}

};


class CEdResourceSorterFrame : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();
	
	typedef THashMap< String, CDirectory* > TDirectories;
	typedef THashMap< String, String >		TFilePaths;

private:
	wxTreeCtrl*		m_directoryTree;
	wxListBox*		m_fileList;

	CResourceSorter	m_sorter;

	TDynArray< String > m_filepaths;
	Uint32 m_currentDirectoryIndex;
	Uint32 m_directoriesCount;

public:
	CEdResourceSorterFrame( wxWindow* parent );

protected:
	void AddDirectoryToTree( CDirectory* directory, wxTreeItemId parentItem );
	void ListDirectoryFiles( CDirectory* currentDirectory );

	void UpdateFileList();
protected:
	void OnDirectorySelected( wxTreeEvent& event );
	void OnUpButton( wxCommandEvent& event );

	void UpdateIndices();
	void OnDownButton( wxCommandEvent& event );
	void OnSaveButton( wxCommandEvent& event );
};
