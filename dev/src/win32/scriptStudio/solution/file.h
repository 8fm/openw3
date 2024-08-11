/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_FILE_H__
#define __SCRIPT_STUDIO_SOLUTION_FILE_H__

#include "slnDeclarations.h"

#include "versionControl/interface/versionControl.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
class CSSDocument;
class CSSDocumentEx;
struct SSFileStub;

class SolutionDir;

/// File in the solution
class SolutionFile
{
public:
	SolutionDir*				m_dir;
	wstring						m_name;
	wstring						m_solutionPath;
	wstring						m_absolutePath;
	CSSDocument*				m_document;
	CSSDocumentEx*				m_documentEx;
	SSFileStub*					m_stubs;

protected:
	bool						m_isModified;
	wxDateTime					m_modificationTime; // last file modification time
	VersionControl::FileStatus	m_status;
	bool						m_ignoreChanges;

public:
	SolutionFile( SolutionDir* dir, const wstring& solutionPath, const wstring& absolutePath, const wstring& name );
	~SolutionFile();

	void CreateSymbolData( bool scheduleForParsing );
	void DestroySymbolData();

	// Source Control
	bool Add();
	bool CheckOut();
	bool CheckIn();
	bool Revert();
	void RefreshStatus();

	bool IsReadOnlyFlagSet() const;
	bool ClearReadOnlyFlag() const;

	RED_INLINE VersionControl::FileStatus GetStatus() const;
	RED_INLINE bool IsInDepot() const;
	RED_INLINE bool IsAdded() const;
	RED_INLINE bool IsDeleted() const;
	RED_INLINE bool IsCheckedOut() const;
	RED_INLINE bool IsOutOfDate() const;
	RED_INLINE bool CanModify() const;

	bool IsModified() const;
	bool MarkModified();
	bool CancelModified();
	bool Save();
	bool ExistsOnDisk() const;

	wxString GetText();
	void UpdateModificationTime();
	bool CheckModificationTime();

	RED_INLINE void SetIgnoreChanges( bool ignore );
	RED_INLINE bool IgnoreChanges() const;
};

//////////////////////////////////////////////////////////////////////////
// Inline Functions
VersionControl::FileStatus SolutionFile::GetStatus() const
{
	return m_status;
}

bool SolutionFile::IsInDepot() const
{
	return m_status.HasFlag( VersionControl::VCSF_InDepot );
}

bool SolutionFile::IsAdded() const
{
	return m_status.HasFlag( VersionControl::VCSF_Added );
}

bool SolutionFile::IsDeleted() const
{
	return m_status.HasFlag( VersionControl::VCSF_Deleted );
}

bool SolutionFile::IsCheckedOut() const
{
	return m_status.HasFlag( VersionControl::VCSF_CheckedOut );
}

bool SolutionFile::IsOutOfDate() const
{
	return m_status.HasFlag( VersionControl::VCSF_OutOfDate );
}

bool SolutionFile::CanModify() const
{
	return ( IsCheckedOut() || ( !IsInDepot() && !IsReadOnlyFlagSet() ) );
}

void SolutionFile::SetIgnoreChanges( bool ignore )
{
	m_ignoreChanges = ignore;
}

bool SolutionFile::IgnoreChanges() const
{
	return m_ignoreChanges;
}

#endif // __SCRIPT_STUDIO_SOLUTION_FILE_H__
