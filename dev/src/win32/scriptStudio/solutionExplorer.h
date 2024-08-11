/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "solution/slnDeclarations.h"
#include "app.h"
#include "wx/generic/treectlg.h"

class CCheckInEvent;
enum ESolutionImage;
class SolutionDir;
class SolutionFile;
class wxProgressDialog;

namespace VersionControl
{
	class FileStatus;
	class Filelist;
}

/// Solution explorer 
class CSSSolutionExplorer : public wxGenericTreeCtrl
{
	wxDECLARE_CLASS( CSSSolutionExplorer );
	DECLARE_EVENT_TABLE();

protected:
	map< SolutionDir*, wxTreeItemId >		m_dirMapping;
	map< SolutionFilePtr, wxTreeItemId >	m_fileMapping;
	wxFileConfig							m_layout;
	wxString								m_previousSubmitDescription;
	wxTreeItemId							m_menuFocusedItem;
	Solution*								m_solution;

public:
	CSSSolutionExplorer( wxWindow* parent, Solution* solution );
	~CSSSolutionExplorer();

	void RepopulateTree( bool openFiles, bool saveLayout, wxProgressDialog* progressDialog = nullptr );
	void SaveLayout();
	void UpdateIcon( SolutionFilePtr file );
	void UpdateIcons();
	int GetSolutionFiles( vector< SolutionFilePtr >& solutionFiles /* out */ );

	wxFileConfig& GetLayoutConfig() { return m_layout; }

protected:
	void RestoreLayout( bool openFiles );
	void FillDirectory( SolutionDir* dir, wxTreeItemId item, ESolutionImage collapsedImg = SOLIMG_DirClosed, ESolutionImage expandedImg = SOLIMG_DirOpened );
	ESolutionImage GetFileImage( const SolutionFilePtr& file );
	void SaveExpandedNodes( wxTreeItemId item, const wxString& path );
	void LoadExpandedNodes( wxTreeItemId item, const wxString& path );
	wxString ItemToPath( wxTreeItemId item );
	SolutionDir* GetSelectedDir();
	SolutionFilePtr GetSelectedFile();

	void AddMenuItem( wxMenu* menu, int id, wxChar* label, wxChar* imageName, void (CSSSolutionExplorer::*eventHandler)( wxCommandEvent& ) );

	void RescanDir( SolutionDir* dir );

	void RebuildOpenSolutionFileCache();

protected:
	void OnOpenFile( wxTreeEvent& event );
	void OnItemExpanded( wxTreeEvent& event );
	void OnContextMenu( wxTreeEvent& event );
	void OnCreateFile( wxCommandEvent& event );
	void OnCreateDirectory( wxCommandEvent& event );
	void OnRescanDir( wxCommandEvent& event );
	void OnAdd( wxCommandEvent& event );
	void OnCheckOut( wxCommandEvent& event );
	void OnCheckIn( wxCommandEvent& event );
	void OnCheckInConfirmed( CCheckInEvent& event );
	void OnRevert( wxCommandEvent& event );
	void OnDiff( wxCommandEvent& event );
	void OnGetLatest( wxCommandEvent& event );
	void OnRemove( wxCommandEvent& event );
	void OnOpenContainingFolder( wxCommandEvent& event );

	void GetSelectedFilesInternal( vector< SolutionFilePtr >& files, wxTreeItemId id, bool expandDirectories, VersionControl::FileStatus* includeFilter, VersionControl::FileStatus* excludeFilter );
	void GetSelectedFiles( vector< SolutionFilePtr >& files, bool expandDirectories = true, VersionControl::FileStatus* includeFilter = nullptr, VersionControl::FileStatus* excludeFilter = nullptr );
	void ToFileList( const vector< SolutionFilePtr >& files_in, VersionControl::Filelist* files_out );
};
