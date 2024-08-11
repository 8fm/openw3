/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SCRIPT_STUDIO_SOLUTION_DIR_H__
#define __SCRIPT_STUDIO_SOLUTION_DIR_H__

#include "slnDeclarations.h"

/// Folder in the solution
class SolutionDir
{
private:
	SolutionDir*				m_parent;
	wstring						m_name;
	wstring						m_solutionPath;
	wstring						m_absolutePath;
	vector< SolutionFilePtr >	m_files;
	vector< SolutionDir* >		m_directories;

public:
	SolutionDir( SolutionDir* parent, const wstring& solutionPath, const wstring& absolutePath, const wstring& name );
	~SolutionDir();

	void Scan();
	bool Empty() const;

	SolutionDir* CreateDir( const wstring& name );
	SolutionFilePtr CreateFile( const wstring& name );

	SolutionDir* FindDir( const wstring& name );
	SolutionFilePtr FindFile( const wstring& name );

	SolutionFilePtr FindFileRecurse( const wstring& path );

	void RefreshFilesStatus();

	void GetFiles( vector< SolutionFilePtr>& files );

	RED_INLINE const vector< SolutionFilePtr >::const_iterator BeginFiles() const;
	RED_INLINE const vector< SolutionFilePtr >::const_iterator EndFiles() const;
	RED_INLINE const vector< SolutionDir* >::const_iterator BeginDirectories() const;
	RED_INLINE const vector< SolutionDir* >::const_iterator EndDirectories() const;

	RED_INLINE const wstring& GetSolutionPath() const;
	RED_INLINE const wstring& GetAbsolutePath() const;
	RED_INLINE const wstring& GetName() const;

private:
	void ScanInternal();
	void ScanVersionControl();

	void FreeDirectories( vector<SolutionDir*> & directories );
	void FreeFiles();
};

//////////////////////////////////////////////////////////////////////////
// Inline Functions
const vector< SolutionFilePtr >::const_iterator SolutionDir::BeginFiles() const
{
	return m_files.cbegin();
}

const vector< SolutionFilePtr >::const_iterator SolutionDir::EndFiles() const
{
	return m_files.cend();
}

const vector< SolutionDir* >::const_iterator SolutionDir::BeginDirectories() const
{
	return m_directories.cbegin();
}

const vector< SolutionDir* >::const_iterator SolutionDir::EndDirectories() const
{
	return m_directories.cend();
}

const wstring& SolutionDir::GetSolutionPath() const
{
	return m_absolutePath;
}

const wstring& SolutionDir::GetAbsolutePath() const
{
	return m_absolutePath;
}

const wstring& SolutionDir::GetName() const
{
	return m_name;
}


#endif // __SCRIPT_STUDIO_SOLUTION_DIR_H__
